#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#define USR0_TRIGGER "/sys/class/leds/beaglebone:green:usr0/trigger"
#define USR0_BRIGHTNESS "/sys/class/leds/beaglebone:green:usr0/brightness"

int main(int argc,char* args[])
{
    FILE *pLedTriggerFile = fopen(USR0_TRIGGER, "w");
    if (pLedTriggerFile == NULL) {
        printf("ERROR OPENING %s.\n", USR0_TRIGGER);
        exit(1);
    }
    int charWritten = fprintf(pLedTriggerFile, "none");
    if (charWritten <= 0) {
        printf("ERROR WRITING DATA");
        exit(1);
    }
    fclose(pLedTriggerFile);

    printf("Timing test\n");
    for (int i = 0; i < 5; i++) {
        long seconds = 1;
        long nanoseconds = 500000000;
        struct timespec reqDelay = {seconds, nanoseconds};
        nanosleep(&reqDelay, (struct timespec *) NULL);
        printf("Delayed print %d.\n", i);
    }

    FILE *pLedBrightnessFile = fopen(USR0_BRIGHTNESS, "w");
    if (pLedBrightnessFile == NULL) {
        printf("ERROR OPENING %s.\n", USR0_BRIGHTNESS);
        exit(1);
    }
    int charWritten2 = fprintf(pLedBrightnessFile, "1");
    if (charWritten2 <= 0) {
        printf("ERROR WRITING DATA");
        exit(1);
    }
    fclose(pLedBrightnessFile);
    return 0;
}
