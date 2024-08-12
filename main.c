#include "./include/termraw.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>

#define FPS 21
#define WIDTH 128
#define HEIGHT 31
#define BORDER_OFFSET 1
#define FOOD_SPAWN_RATE 7
#define FOOD_SIZE_EXTRA 2

#define SNAKE_HEAD_CHAR '>' 
#define SNAKE_BODY_CHAR 'O'
#define SPACE_CHAR ' '
#define FOOD_CHAR '$'

#define BUF_SIZE 3

const char KEY_UP[BUF_SIZE] = {27, 91, 65};
const char KEY_DOWN[BUF_SIZE] = {27, 91, 66};
const char KEY_RIGHT[BUF_SIZE] = {27, 91, 67};
const char KEY_LEFT[BUF_SIZE] = {27, 91, 68};

typedef struct Box {
   uint64_t x1;
   uint64_t x2;
   uint64_t y1;
   uint64_t y2;
} Box;

int box_check_collisions(Box box, uint64_t dx, uint64_t dy){
   if ((dx >= box.x1 && dx <= box.x2) && (dy >= box.y1 && dy <= box.y2)) return 1;
   return 0;
}

int box2_check_collisions(Box box1, Box box2){
   for(uint64_t x = box1.x1; x <= box1.x2; x++){
      for(uint64_t y = box1.y1; y <= box1.y2; y++){
         if(box_check_collisions(box2, x, y)) return 1;
      }
   }

   return 0;
}

void cursorupn(int n) {
   printf("\x1B[%dA", n);
}

typedef struct Point {
   int x;
   int y;
} Point;

void point_draw(char* buf, Point p, char c, uint64_t width) {
   buf[(p.y * width) + p.x] = c;
}

int randint(int n) {
  if ((n - 1) == RAND_MAX) {
    return rand();
  } else {
    assert (n <= RAND_MAX);
    int end = RAND_MAX / n;
    assert (end > 0);
    end *= n;
    int r;
    while ((r = rand()) >= end);
    return r % n;
  }
}

typedef Box Food;

Food food_create(
      uint64_t x_max,
      uint64_t y_max,
      Point* snake_coords,
      size_t coords_size,
      Food* other_foods,
      size_t foods_size){

   uint64_t x1 = randint(x_max - FOOD_SIZE_EXTRA);
   uint64_t y1 = randint(y_max - FOOD_SIZE_EXTRA);


   uint64_t x2 = x1 + FOOD_SIZE_EXTRA;
   uint64_t y2 = y1 + FOOD_SIZE_EXTRA;

   Food food = {
      .x1 = x1,
      .x2 = x2,
      .y1 = y1,
      .y2 = y2
   };

   for(size_t i = 0; i < coords_size; i++){
      Point p = snake_coords[i];
      int collides = box_check_collisions(food, p.x, p.y);
      if (collides) return food_create(x_max, y_max, snake_coords, coords_size, other_foods, foods_size);
   }

   for (size_t i = 0; i < foods_size; i++){
      Food other_food = other_foods[i];
      int collides = box2_check_collisions(other_food, food);
      if (collides) return food_create(x_max, y_max, snake_coords, coords_size, other_foods, foods_size);
   }

   return food;
}

typedef enum Direction{
   Up,
   Down,
   Left,
   Right
} Direction;

typedef struct Snake {
   Point head;
   Direction direction;
   
   Point* nodes;
   size_t nodes_len;
   size_t nodes_cap;
} Snake;

Snake* snake_init(){
   Snake* snake = malloc(sizeof(Snake));
   snake->direction = Right;
   snake->head = (Point){
      .x = 0,
      .y = 0
   };
   snake->nodes_cap = 10;
   snake->nodes_len = 0;

   Point* nodes = malloc(sizeof(Point) * snake->nodes_cap);
   snake->nodes = nodes;

   return snake;
}

void snake_nodes_grow(Snake* snake){
   size_t new_cap = snake->nodes_cap * 2;
   Point* reallocd = realloc(snake->nodes, sizeof(Point) * new_cap);
   snake->nodes = reallocd;
   snake->nodes_cap = new_cap;
}

void snake_node_append(Snake* snake, Point node) {
   snake->nodes[snake->nodes_len] = node;
   snake->nodes_len++;

   if (snake->nodes_len == snake->nodes_cap){
      snake_nodes_grow(snake);
   }
}

void snake_grow(Snake *snake) {
   if(snake->nodes_len == 0){
      // Add after head
      Point head = snake->head;
      Point tail = {
         .x = head.x - 1,
         .y = head.y,
      };
      
      snake_node_append(snake, tail);
      return;
   }
   
   // Add before
   Point prev_tail = snake->nodes[snake->nodes_len - 1];
   Point new_tail = {
      .x = prev_tail.x - 1,
      .y = prev_tail.y
   };

   snake_node_append(snake, new_tail);
}

void drop_last_node(Snake* snake){
   snake->nodes_len--;
}

void prepend_node(Snake* snake, Point node){
   if(snake->nodes_len + 1 >= snake->nodes_cap) snake_nodes_grow(snake);

   Point* new_nodes = malloc(sizeof(Point) * snake->nodes_cap);
   new_nodes[0] = node;

   for(size_t i = 1; i < snake->nodes_len; i++) new_nodes[i] = snake->nodes[i - 1];

   free(snake->nodes);

   snake->nodes = new_nodes;
   snake->nodes_len++;
}

void snake_move(Snake* snake, uint64_t max_y, uint64_t max_x){
   Point next_point;

   if(snake->direction == Up){
      next_point = (Point){
         .y = snake->head.y - 1,
         .x = snake->head.x
      };
   }

   if(snake->direction == Down){
      next_point = (Point){
         .y = snake->head.y + 1,
         .x = snake->head.x
      };
   }

   if (snake->direction == Right){
      next_point = (Point){
         .y = snake->head.y,
         .x = snake->head.x + 1
      };
   }

   if (snake->direction == Left){
      next_point = (Point){
         .y = snake->head.y,
         .x = snake->head.x - 1
      };
   }

   if (next_point.y < 0 ||
         next_point.x < 0 ||
         (uint64_t) next_point.x >= max_x ||
         (uint64_t) next_point.y >= max_y) {
      // HANDLE IT BY TELEPORTING SNAKE TO BOTTOM
      return;
   }

   if(snake->nodes_len > 0){
      prepend_node(snake, snake->head);
      drop_last_node(snake);
   }

   snake->head = next_point;

   return;
}

typedef struct IsBorder {
   char c;
   int is_border;
} IsBorder;

typedef struct Map {
   uint64_t width;
   uint64_t height;
   
   uint64_t width_with_offfset;
   uint64_t height_with_offset;

   size_t __bytesize;

   char* old_buf;
   char* new_buf;

   Food* foods;
   size_t foods_cap;
   size_t foods_len;
} Map;

Map* map_init(uint64_t width, uint64_t height) {
   Map* map = malloc(sizeof(Map));

   map->height = height;
   map->width = width;

   map->height_with_offset = height + BORDER_OFFSET * 2;
   map->width_with_offfset = width + BORDER_OFFSET * 2;

   map->__bytesize = map->height_with_offset * map->width_with_offfset;

   char* old_buf = malloc(sizeof(char) * map->__bytesize);
   char* new_buf = malloc(sizeof(char) * map->__bytesize);

   map->old_buf = old_buf;
   map->new_buf = new_buf;

   memset(old_buf, SPACE_CHAR, map->__bytesize);
   memset(new_buf, SPACE_CHAR, map->__bytesize);
   
   map->foods_cap = 10;
   map->foods_len = 0;

   Food* foods = malloc(sizeof(Food) * map->foods_cap);
   map->foods = foods;

   return map;
}

IsBorder map_is_border(Map* map, Point p) {
   uint64_t width = map->width_with_offfset;
   uint64_t height = map->height_with_offset;

   if ((p.x == 0 && p.y == 0) ||
         (p.x == 0 && (uint64_t)p.y == height - 1) ||
         ((uint64_t) p.x == width - 1 && p.y == 0) ||
         ((uint64_t) p.x == width - 1 && (uint64_t) p.y == height - 1)) {
      return (IsBorder){
         .is_border = 1,
         .c = '+'
      };
   }

   if ((p.x == 0) || ((uint64_t) p.x == width - 1 )){
       return (IsBorder) {
         .is_border= 1,
         .c = '|'
      };
   }

   if ((p.y == 0) || ((uint64_t) p.y == height - 1)){
      return (IsBorder){
         .is_border = 1,
         .c = '-'
      };
   }

   return (IsBorder){
      .is_border = 0,
   };
}

void map_draw_borders(Map* map) {
   for (uint64_t y = 0; y < map->height_with_offset; y++){
      for(uint64_t x = 0; x < map->width_with_offfset; x++){
         Point p;

         p.x = x;
         p.y = y;
         
         IsBorder result = map_is_border(map, p);
         if (result.is_border) {
            point_draw(map->new_buf, p, result.c, map->width_with_offfset);
            continue;
         }
      }
   }
}

void map_draw(Map* map, Point p, char c) {
   // TOOD: handle negative offset after 50%
   point_offset(&p, BORDER_OFFSET);
   point_draw(map->new_buf, p, c, map->width_with_offfset);
}

void map_draw_snake(Map* map, Snake* snake) {
   map_draw(map, snake->head, SNAKE_HEAD_CHAR);

   for(size_t i = 0; i < snake->nodes_len; i++) {
      Point node = snake->nodes[i];
      map_draw(map, node, SNAKE_BODY_CHAR);
   }
}

void map_draw_foods(Map* map){
   for(size_t i = 0; i < map->foods_len; i++){
      Food food = map->foods[i];

      for(uint64_t x = food.x1; x <= food.x2 && x < map->width; x++) {
         for(uint64_t y = food.y1; y <= food.y2 && y < map->height; y++) {
            Point p = {
               .x = x,
               .y = y
            };

            map_draw(map, p, FOOD_CHAR);
         }
      }

   }
}

void map_check_collisions(Map* map, Snake* snake) {
   // Check colissions with food
   for(size_t i = 0; i < map->foods_len; i++){
      Food food = map->foods[i];
      uint64_t dx = snake->head.x;
      uint64_t dy = snake->head.y;

      int collides_with_food = box_check_collisions(food, dx, dy);
      if (collides_with_food){
         // Delete this food from array
         // Swap to end and decrease len
         Food tmp = map->foods[map->foods_len - 1];
         map->foods[map->foods_len - 1] = map->foods[i];
         map->foods[i] = tmp;
         map->foods_len--;

         // Increase snake size
         snake_grow(snake);
      }
   }
}

void map_render(Map* map, Snake* snake) {
   map_draw_borders(map);
   map_draw_foods(map);
   map_draw_snake(map, snake);

   memcpy(map->old_buf, map->new_buf, map->__bytesize);

   for (uint64_t y = 0; y < map->height_with_offset; y++){

      for(uint64_t x = 0; x < map->width_with_offfset; x++){
         printf("%c", map->new_buf[y*(map->width_with_offfset)+x]);
      }

      printf("\n");
   }

   cursorupn(map->height_with_offset);

   memset(map->new_buf, SPACE_CHAR, map->__bytesize);
}

void map_save_food(Map* map, Food food) {
   if (map->foods_len + 1 >= map->foods_cap){
      size_t new_cap = map->foods_cap*2;
      Food* reallocd = realloc(map->foods, sizeof(Food) * new_cap);
      map->foods = reallocd;
      map->foods_cap = new_cap;
   }

   map->foods[map->foods_len] = food;
   map->foods_len++;
}

typedef struct Game {
   pthread_mutex_t mutex;
   Map* map;
   Snake* snake;
   uint8_t fps;
   int stop;
} Game;

Game* game_init(uint64_t width, uint64_t height, uint8_t fps){
   Game* game = malloc(sizeof(Game));

   Map* map = map_init(width, height);

   Snake* snake = snake_init();

   pthread_mutex_t mutex;
   pthread_mutex_init(&mutex, 0);

   game->stop = 0;
   game->map = map;
   game->snake = snake;
   game->fps = fps;
   game->mutex = mutex;

   return game;
}

void game_lock(Game* game){
   pthread_mutex_lock(&game->mutex);
}

void game_unlock(Game* game){
   pthread_mutex_unlock(&game->mutex);
}

void game_destroy(Game* game){
   pthread_mutex_destroy(&game->mutex);

   free(game->map->new_buf);
   free(game->map->old_buf);
   free(game->map->foods);
   free(game->map);

   free(game->snake->nodes);
   free(game->snake);

   free(game);
}

void* render_loop(void* argp){
   Game* game = (Game*) argp;
   uint8_t fps = game->fps;
   uint8_t ms_per_frame = (uint8_t)1000 / FPS;
   uint8_t frame_count = 0;
   while(1){
      // CODE BELOW IS 1 FRAME
      game_lock(game);

      if (game->stop == 1) {
         break;
         game_unlock(game);
      }
      
      snake_move(game->snake, game->map->height, game->map->width);
      map_check_collisions(game->map, game->snake);

      if (frame_count % FOOD_SPAWN_RATE == 0) {
         Food food = food_create(
               game->map->width,
               game->map->height,
               game->snake->nodes,
               game->snake->nodes_len,
               game->map->foods,
               game->map->foods_len);
         map_save_food(game->map, food);
         frame_count = 0;
      }

      map_render(game->map, game->snake);

      game_unlock(game);
      frame_count++;
      usleep(1000 * ms_per_frame);
   }

   return NULL;
}

int streq(const char* s1, const char* s2, size_t len){
   for(size_t i = 0; i < len; i++){
      if (s1[i] != s2[i]) return 0;
   }

   return 1;
}

void* read_loop(void* argp){
   Game* game = (Game*) argp;
   Termios prev_state = make_raw();

   while(1){
      char buf[BUF_SIZE];
      int result = read(STDIN_FILENO, &buf, BUF_SIZE);
      if (result == -1){
         perror("read_loop:read");
         break;
      }
      
      game_lock(game);
      if (streq(buf, KEY_UP, BUF_SIZE)) {
         game->snake->direction = Up;
      }
      
      if (streq(buf, KEY_DOWN, BUF_SIZE)) {
         game->snake->direction = Down;
      }

      if (streq(buf, KEY_LEFT, BUF_SIZE)) {
         game->snake->direction = Left;
      }

      if (streq(buf, KEY_RIGHT, BUF_SIZE)) {
         game->snake->direction = Right;
      }
      game_unlock(game);

      if (buf[0] == 'q') {
         game_lock(game);
         game->stop = 1;
         game_unlock(game);
         break;
      };
   }
   
   set_termios(&prev_state);

   return NULL;
}

int main(){
   Game* game = game_init(WIDTH, HEIGHT, FPS);

   pthread_t render;
   pthread_t reader;

   pthread_create(&render, 0, render_loop, game);
   pthread_create(&reader, 0, read_loop, game);

   pthread_join(render, NULL);
   pthread_join(reader, NULL);

   game_destroy(game);

   return 0;
}
