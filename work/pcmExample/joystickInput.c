/*
 Joystick input thread to read input from Zen cape joystick
 Responds to input by calling functions from Audio Mixer module
 */
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>			// for strncmp()
#include "joystickInput.h"
#include "audiomixer.h"
static pthread_t joystickThreadId;
static int running = 1;
static void* joystickThread();
void joystick_startThread(void)
{
    pthread_create(&joystickThreadId,NULL, joystickThread,NULL);
}
void joystick_shutdown(void)
{
    printf("stopping joystick thread\n");
    running = 0;
    pthread_cancel(joystickThreadId);
}
void joystick_joinThread(void)
{
    pthread_join(joystickThreadId,NULL);
}

void* joystickThread()
{
    while (running)
    {
        if (isDirectionPressed(CENTER)){
           AudioMixer_cycleNextMode();
        }
        else if(isDirectionPressed(RIGHT)){
            AudioMixer_setTempo(AudioMixer_getTempo()+5);
        }
        else if(isDirectionPressed(LEFT)){
            AudioMixer_setTempo(AudioMixer_getTempo()-5);
        }
        else if(isDirectionPressed(UP)){
            AudioMixer_setVolume(AudioMixer_getVolume()+5);
        }
        else if(isDirectionPressed(DOWN)){
            AudioMixer_setVolume(AudioMixer_getVolume()-5);
        }
        sleepForMs(150);
    }
    return NULL;
}
bool isDirectionPressed(enum joystick direction)
{
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
