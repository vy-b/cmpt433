#include "joystick.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

bool isDirectionPressed(enum joystick direction){
    char filename[100];
    snprintf(filename,100,"/sys/class/gpio/gpio%d/value",direction);
    FILE *pFile = fopen(filename, "r");
    if (pFile == NULL) {
        printf("ERROR: Unable to open file (%s) for read\n", filename);
        exit(-1);
    }
    // Read string (line)
    const int MAX_LENGTH = 1024;
    char isPressed[MAX_LENGTH];
    fgets(isPressed, MAX_LENGTH, pFile);
    // Close
    fclose(pFile);
    // printf("Read: '%s'\n", isPressed);
    return !atoi(isPressed);
}


enum joystick randomizeDirection(void){
    srand(time(0));
    if (rand()%2)
        return UP;
    return DOWN;
}