#include <stdbool.h>

#ifndef JOYSTICK_H_
#define JOYSTICK_H_

enum joystick {UP=26,DOWN=46,RIGHT=47,LEFT=65};
bool isDirectionPressed(enum joystick direction);
enum joystick randomizeDirection(void);

#endif