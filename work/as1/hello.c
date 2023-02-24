#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "joystick.h"

#define EXPORT_FILE "/sys/class/gpio/export"

// static void export_pins(enum joystick pin); // Uncomment definition and all references to export pins
static void runCommand(char* command);
static void changeBrightness(int lednum, char* onOff);
static void flashLEDs(int timesFlashed, int delayOnMs, int delayOffMs);
static void setTriggers(int lednum, char* triggerMode);
static long long chooseWaitTime(void);
static long long getTimeInMs(void);
static void sleepForMs(long long delayInMs);
static void resetLEDs(void);
static void game_init(void);

int main(int argc,char* args[])
{
    game_init();
    printf("Hello embedded world, from Vy!\n");
    printf("When the LEDs light up, press the joystick in that direction!\n");
    printf("(Press left or right to exit)\n");

    // Initialize values
    enum joystick toPress,notToPress = randomizeDirection();
    long long timeSinceStart,timeSinceReady,elapsedTime = getTimeInMs();
    bool wrongInput,quit = false;
    long long currentBest = 5000;

    while (!isDirectionPressed(RIGHT) && !isDirectionPressed(LEFT)){
        toPress = randomizeDirection();
        wrongInput=false;

        printf("Get ready...\n");
        long long waitTime = chooseWaitTime();
        timeSinceStart = getTimeInMs();

        while ((getTimeInMs()-timeSinceStart)!=waitTime){
            if (isDirectionPressed(UP)||isDirectionPressed(DOWN)){
                printf("Too soon!\n");
                wrongInput = true;
                sleepForMs(500);
                break;
            }
            else if (isDirectionPressed(RIGHT)||isDirectionPressed(LEFT)){
                quit = true;
                break;
            }
        }

        if (wrongInput)
            continue;

        if (quit){
            resetLEDs();
            break;
        }

        timeSinceReady = getTimeInMs();

        if (toPress == UP){
            changeBrightness(0,"1");
            notToPress = DOWN;
            printf("Press UP now!\n");
        }
        else {
            changeBrightness(3,"1");
            notToPress = UP;
            printf("Press DOWN now!\n");
        }

        while ((elapsedTime = getTimeInMs()-timeSinceReady)<5000){
            if(isDirectionPressed(toPress)){
                printf("Correct!\n");
                if (elapsedTime<currentBest){
                    printf("New best time!\n");
                    currentBest = elapsedTime;
                }
                printf("Your reaction time was %llu; best so far in the game is %llu\n",elapsedTime,currentBest);
                flashLEDs(2,50,200);
                resetLEDs();
                sleepForMs(1000);
                break;
            }
            else if (isDirectionPressed(notToPress)){
                printf("Incorrect.\n");
                flashLEDs(10,10,90);
                resetLEDs();
                sleepForMs(1000);
                break;
            }
            else if (isDirectionPressed(RIGHT)||isDirectionPressed(LEFT)){
                quit = true;
                resetLEDs();
                break;
            }
        }

        if (elapsedTime>=5000){
            printf("no input within 5000ms, quitting!\n");
            resetLEDs();
            break;
        }
        
    }
    if (quit)
        printf("User chose to quit.\n");

    return 0;
}

static void resetLEDs(void)
{
    changeBrightness(0,"0");
    changeBrightness(3,"0");
}

static void runCommand(char* command)
{
    // Execute the shell command (output into pipe)
    FILE *pipe = popen(command, "r");
    // Ignore output of the command; but consume it
    // so we don't get an error when closing the pipe.
    char buffer[1024];
    while (!feof(pipe) && !ferror(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) == NULL)
            break;
        // printf("--> %s", buffer); // Uncomment for debugging
    }
    // Get the exit code from the pipe; non-zero is an error:
    int exitCode = WEXITSTATUS(pclose(pipe));
    if (exitCode != 0) {
        perror("Unable to execute command:");
        printf(" command: %s\n", command);
        printf(" exit code: %d\n", exitCode);
    }
}

// void export_pins(enum joystick pin){
//     // Use fopen() to open the file for write access.
//     FILE *pFile = fopen(EXPORT_FILE, "w");
//     if (pFile == NULL) {
//         printf("ERROR: Unable to open export file.\n");
//         exit(1);
//     }
//     // Write to data to the file using fprintf():
//     fprintf(pFile, "%d", pin);
//     // Close the file using fclose():
//     fclose(pFile);
//     // Call nanosleep() to sleep for ~300ms before use.
//     sleepForMs(300);
// }


static void setTriggers(int lednum, char* triggerMode)
{
    char filename[100];
    snprintf(filename,100,"/sys/class/leds/beaglebone:green:usr%d/trigger",lednum);
    FILE *pLedTriggerFile = fopen(filename, "w");
    if (pLedTriggerFile == NULL) {
        printf("ERROR OPENING %s.\n", filename);
        exit(1);
    }
    int charWritten = fprintf(pLedTriggerFile, triggerMode);
    if (charWritten <= 0) {
        printf("ERROR WRITING DATA");
        exit(1);
    }
    fclose(pLedTriggerFile);
}

static void changeBrightness(int lednum, char* onOff)
{
    char filename[100];
    snprintf(filename,100,"/sys/class/leds/beaglebone:green:usr%d/brightness",lednum);
    FILE *pLedBrightnessFile = fopen(filename, "w");
    if (pLedBrightnessFile == NULL) {
        printf("ERROR OPENING %s.\n", filename);
        exit(1);
    }
    int charWritten2 = fprintf(pLedBrightnessFile, onOff);
    if (charWritten2 <= 0) {
        printf("ERROR WRITING DATA");
        exit(1);
    }
    fclose(pLedBrightnessFile);
}

static void flashLEDs(int timesFlashed, int delayOnMs, int delayOffMs )
{
    for (int j = 0; j<timesFlashed;j++){
        for (int i = 0; i < 4; i++){
            changeBrightness(i,"1");
        }
        sleepForMs(delayOnMs);
        for (int i = 0; i < 4; i++){
            changeBrightness(i,"0");
        }
        sleepForMs(delayOffMs);
    }
}


static void game_init(void)
{
    runCommand("config-pin p8.43 gpio");
    runCommand("config-pin p8.14 gpio");
    runCommand("config-pin p8.15 gpio");
    runCommand("config-pin p8.16 gpio");
    runCommand("config-pin p8.17 gpio");
    runCommand("config-pin p8.18 gpio");

    for (int i = 0; i<4; i++){
        setTriggers(i,"none");
        changeBrightness(i,"0");
    }
}

static long long getTimeInMs(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long long seconds = spec.tv_sec;
    long long nanoSeconds = spec.tv_nsec;
    long long milliSeconds = seconds * 1000
    + nanoSeconds / 1000000;
    return milliSeconds;
}

static void sleepForMs(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}

static long long chooseWaitTime(void)
{
    float lower = 0.5, upper = 3;
    srand(time(0));
    float num = (lower + 1) + (((float) rand()) / (float) RAND_MAX) * (upper - (lower + 1));
    return (long long)(num*1000);
}