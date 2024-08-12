#include <termios.h>

typedef struct termios Termios;

void set_termios(Termios *state);

Termios get_termios();

Termios make_raw();
