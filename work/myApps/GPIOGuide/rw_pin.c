#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#define EXPORT_FILE "/sys/class/gpio/export"
#define USR0_BRIGHTNESS "/sys/class/leds/beaglebone:green:usr0/brightness"

enum joystick {UP=26,DOWN=46,RIGHT=47,LEFT=65,NONE=27};
static void export_pins(enum joystick pin);
static void runCommand(char* command);
static bool isDirectionPressed(enum joystick direction);
static long long chooseWaitTime(void);
static enum joystick chooseDirection(void);
static void changeBrightness(int lednum, char* onOff);
static long long getTimeInMs(void);
static void sleepForMs(long long delayInMs);
static void initialize_game(void);
int main(int argc,char* args[])
{
    
    
    // // Call nanosleep
    // for (int i = 0; i < 5; i++) {
    //     long seconds = 0.5;
    //     long nanoseconds = 500000000;
    //     struct timespec reqDelay = {seconds, nanoseconds};
    //     nanosleep(&reqDelay, (struct timespec *) NULL);
    //     printf("Delayed print %d.\n", i);
    // }
    initialize_game();
    enum joystick toPress = chooseDirection();
    enum joystick notToPress = RIGHT;
    long long start_time1,start_time2,elapsed_time = getTimeInMs();
    bool wrong_input,quit = false;
    long long current_best = 5000;
    while (!isDirectionPressed(RIGHT) && !isDirectionPressed(LEFT)){
        
        wrong_input=false;
        printf("Get ready...\n");
        long long waitTime = chooseWaitTime();
        start_time1 = getTimeInMs();
        while ((getTimeInMs()-start_time1)!=waitTime){
            if (isDirectionPressed(UP)||isDirectionPressed(DOWN)){
                printf("Too soon!\n");
                wrong_input = true;
                sleepForMs(1000);
                break;
            }
            else if (isDirectionPressed(RIGHT)||isDirectionPressed(LEFT)){
                quit = true;
                break;
            }
        }
        if (wrong_input){
            continue;
        }
        if (quit){
            printf("User chose to quit.\n");
            changeBrightness(0,"0");
            changeBrightness(3,"0");
            break;
        }
        start_time2 = getTimeInMs();
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
        while ((elapsed_time = getTimeInMs()-start_time2)<5000){
            if(isDirectionPressed(toPress)){
                printf("Correct!\n");
                if (elapsed_time<current_best){
                    printf("New best time!\n");
                    current_best = elapsed_time;
                }
                printf("Your reaction time was %llu; best so far in the game is %llu\n",elapsed_time,current_best);
                changeBrightness(0,"0");
                changeBrightness(3,"0");
                sleepForMs(1000);
                break;
            }
            else if (isDirectionPressed(notToPress)){
                printf("Wrong\n");
                changeBrightness(0,"0");
                changeBrightness(3,"0");
                sleepForMs(1000);
                break;
            }
            else if (isDirectionPressed(RIGHT)||isDirectionPressed(LEFT)){
                quit = true;
                changeBrightness(0,"0");
                changeBrightness(3,"0");
                break;
            }
        }
        if (elapsed_time>=5000){
            printf("no input within 5000ms, quitting!\n");
            changeBrightness(0,"0");
            changeBrightness(3,"0");
            break;
        }
        
    }
    


}

void export_pins(enum joystick pin){
    // Use fopen() to open the file for write access.
    FILE *pFile = fopen(EXPORT_FILE, "w");
    if (pFile == NULL) {
        printf("ERROR: Unable to open export file.\n");
        exit(1);
    }
    // Write to data to the file using fprintf():
    fprintf(pFile, "%d", pin);
    // Close the file using fclose():
    fclose(pFile);
    // Call nanosleep() to sleep for ~300ms before use.
    sleepForMs(300);
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
        printf("--> %s", buffer); // Uncomment for debugging
    }
    // Get the exit code from the pipe; non-zero is an error:
    int exitCode = WEXITSTATUS(pclose(pipe));
    if (exitCode != 0) {
        perror("Unable to execute command:");
        printf(" command: %s\n", command);
        printf(" exit code: %d\n", exitCode);
    }
}

static bool isDirectionPressed(enum joystick direction){

    // isValidDirection?
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

static void setTriggers(int lednum){
    char filename[100];
    snprintf(filename,100,"/sys/class/leds/beaglebone:green:usr%d/trigger",lednum);
    FILE *pLedTriggerFile = fopen(filename, "w");
    if (pLedTriggerFile == NULL) {
        printf("ERROR OPENING %s.\n", filename);
        exit(1);
    }
    int charWritten = fprintf(pLedTriggerFile, "none");
    if (charWritten <= 0) {
        printf("ERROR WRITING DATA");
        exit(1);
    }
    fclose(pLedTriggerFile);
}

static void changeBrightness(int lednum, char* onOff){
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


static enum joystick chooseDirection(void){
    srand(time(0));
    if (rand()%2)
        return UP;
    return DOWN;
}
static void initialize_game(void){
    runCommand("config-pin p8.43 gpio");
    runCommand("config-pin p8.14 gpio");
    runCommand("config-pin p8.15 gpio");
    runCommand("config-pin p8.16 gpio");
    runCommand("config-pin p8.17 gpio");
    runCommand("config-pin p8.18 gpio");

    export_pins(UP);
    // export_pins(DOWN);
    // export_pins(LEFT);
    // export_pins(RIGHT);

    for (int i = 0; i<4; i++){
        setTriggers(i);
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

static long long chooseWaitTime(void){
    float lower = 0.5, upper = 3;

    srand(time(0));

    printf("The random wait time is: ");
    float num = (lower + 1) + (((float) rand()) / (float) RAND_MAX) * (upper - (lower + 1));
    printf("%f\n", num);
    return (long long)(num*1000);
}