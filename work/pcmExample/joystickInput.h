#ifndef JOYSTICK_INPUT_H
#define JOYSTICK_INPUT_H
#include <stdbool.h>

void joystick_startThread(void);
void joystick_shutdown(void);
void joystick_joinThread(void);
enum joystick {UP=26,DOWN=46,RIGHT=47,LEFT=65, CENTER=27};
bool isDirectionPressed(enum joystick direction);

#endif