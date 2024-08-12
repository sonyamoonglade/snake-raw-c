#include <termios.h>
#include <unistd.h>

typedef struct termios Termios;

void set_termios(Termios *state){
   tcsetattr(STDIN_FILENO, TCSAFLUSH, state);
}

Termios get_termios() {
   Termios prev_state;
   tcgetattr(STDIN_FILENO, &prev_state);
   return prev_state;
}

Termios make_raw(){
   Termios prev_state = get_termios();

   Termios copy = prev_state;

   // Make raw 
   prev_state.c_lflag &= ~(ICANON|ECHO); 

   set_termios(&prev_state);

   return copy;
}
