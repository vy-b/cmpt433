/* display number on LED. Code adapted from Dr. Brian's guide*/
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <math.h>
#include "led_display.h"
#include "sampler.h"

static void runCommand(char *command);
static int initI2cBus(char *bus, int address);
static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value);
static void writeToFile(char *val, char *filename);
static void getDigitValues(int num, int* digit);
static void turnOnLeft(int i2cFileDesc, int *digit);
static void turnOnRight(int i2cFileDesc, int *digit);
static void sleepForMs(long long delayInMs);
static void* ledThread();
static int led_init();
static void displayNum(int i2cFileDesc, int num);
static pthread_t threadLed;

void start_ledThread(void)
{
	pthread_create(&threadLed, NULL, ledThread, NULL);
}
void stop_ledThread(void)
{
	pthread_join(threadLed, NULL);
}
void* ledThread()
{
    int i2cFileDesc = led_init();
    while (true){
        displayNum(i2cFileDesc,Sampler_getNumDips());
    }
    return NULL;
}
void displayNum(int i2cFileDesc, int num)
{
    int* digitLeft = (int*) malloc(2*sizeof(int));
    getDigitValues(num/10, digitLeft);
    int* digitRight = (int*) malloc(2*sizeof(int));
    getDigitValues(num%10, digitRight);
    writeToFile("0", RIGHT_VALUE);
    writeToFile("0", LEFT_VALUE);
    turnOnLeft(i2cFileDesc,digitLeft);
    sleepForMs(1);
    writeToFile("0", RIGHT_VALUE);
    writeToFile("0", LEFT_VALUE);
    turnOnRight(i2cFileDesc,digitRight);
    sleepForMs(1);
    free(digitLeft);
    free(digitRight);
}

int led_init()
{
    // export
    runCommand("config-pin P9_17 i2c");
    runCommand("config-pin P9_18 i2c");
    writeToFile("out", LEFT_DIRECTION);
	writeToFile("out", RIGHT_DIRECTION);

    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1,I2C_DEVICE_ADDRESS);
    writeI2cReg(i2cFileDesc, REG_DIRA, 0x00);
    writeI2cReg(i2cFileDesc, REG_DIRB, 0x00);
     
    return i2cFileDesc;
}

void LedDisplay_cleanup(int i2cFileDesc){
    
    writeI2cReg(i2cFileDesc, REG_OUTA, 0x00);
    writeI2cReg(i2cFileDesc, REG_OUTB, 0x00);

    writeToFile("0", RIGHT_VALUE);
    writeToFile("0", LEFT_VALUE);

    close(i2cFileDesc);
}
static void turnOnLeft(int i2cFileDesc, int *digit) {
    // turn on left
    writeToFile("1", LEFT_VALUE);
    writeI2cReg(i2cFileDesc, REG_OUTB, digit[0]);
    writeI2cReg(i2cFileDesc, REG_OUTA, digit[1]);
    sleepForMs(1);
    writeI2cReg(i2cFileDesc, REG_OUTA, 0x00);
    writeI2cReg(i2cFileDesc, REG_OUTB, 0x00);
}

static void turnOnRight(int i2cFileDesc, int *digit) {
    // turn on right
    writeToFile("1", RIGHT_VALUE);
    
    writeI2cReg(i2cFileDesc, REG_OUTB, digit[0]);
    writeI2cReg(i2cFileDesc, REG_OUTA, digit[1]);
    sleepForMs(1);
    writeI2cReg(i2cFileDesc, REG_OUTA, 0x00);
    writeI2cReg(i2cFileDesc, REG_OUTB, 0x00);
}
static void getDigitValues(int num, int* digit)
{
    if (num == 0)
    {
        digit[0] = 0x86;
        digit[1] = 0xa1;
    }
    else if (num == 1)
    {
        digit[0] = 0x02;
        digit[1] = 0x80;
    }
    else if (num == 2)
    {
        digit[0] = 0xe;
        digit[1] = 0x31;
    }
    else if (num == 3)
    {
        digit[0] = 0xe;
        digit[1] = 0xb0;
    }
    else if (num == 4)
    {
        digit[0] = 0x8a;
        digit[1] = 0x90;
    }
    else if (num == 5)
    {
        digit[0] = 0x8c;
        digit[1] = 0xb0;
    }
    else if (num == 6)
    {
        digit[0] = 0x88;
        digit[1] = 0xb1;
    }
    else if (num == 7)
    {
        digit[0] = 0x06;
        digit[1] = 0x80;
    }
    else if (num == 8)
    {
        digit[0] = 0x8e;
        digit[1] = 0xb1;
    }
    else if (num == 9)
    {
        digit[0] = 0x8e;
        digit[1] = 0x90;
    }
    else
    {
        digit[0] = 0x00;
        digit[1] = 0x00;
    }
}

static void runCommand(char* command)
{
    // Execute the shell command (output into pipe)
    FILE *pipe = popen(command, "r");
    char buffer[1024];
    while (!feof(pipe) && !ferror(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) == NULL)
            break;
    }
    int exitCode = WEXITSTATUS(pclose(pipe));
    if (exitCode != 0) {
        perror("Unable to execute command:");
        printf(" command: %s\n", command);
        printf(" exit code: %d\n", exitCode);
    }
}

static int initI2cBus(char *bus, int address)
{
    int i2cFileDesc = open(bus, O_RDWR);
    int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
    if (result < 0)
    {
        perror("I2C: Unable to set I2C device to slave address.");
        exit(1);
    }
    return i2cFileDesc;
}

static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value)
{
    unsigned char buff[2];
    buff[0] = regAddr;
    buff[1] = value;
    int res = write(i2cFileDesc, buff, 2);

    if (res != 2)
    {
        perror("I2C: Unable to write i2c register.");
        exit(1);
    }
}


static void sleepForMs(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *)NULL);
}


// adapted from Dr. Brian's example code for export_pins
void writeToFile(char *writeValue, char *filename){
    // Use fopen() to open the file for write access.
    FILE *pFile = fopen(filename, "w");
    if (pFile == NULL) {
        printf("ERROR: Unable to open export file.\n");
        exit(1);
    }
    // Write to data to the file using fprintf():
    fprintf(pFile, "%s", writeValue);
    // Close the file using fclose():
    fclose(pFile);
}

