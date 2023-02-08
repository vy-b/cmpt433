#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "sampler.h"
#define MSG_MAX_LEN 1024
static int recentIndex = 0;
static int historyCount = 0;
static long long allTimeCount = 1;
static double currentAverage = 0;
static int arraySize = 1;
static pthread_t threadSampling;
static double* recent;
static void sleepForMs(long long delayInMs);
static long long getTimeInMs(void);
static int getVoltageReading(char* file);
static double exponential_smoothing(double currentAverage, double nth_sample, double a);
static void* samplerThread();
// Begin/end the background thread which samples light levels.
void Sampler_startSampling(void)
{
    pthread_create(&threadSampling, NULL, samplerThread, NULL);
}
void Sampler_stopSampling(void)
{
    pthread_join(threadSampling, NULL);
}
// Set/get the maximum number of samples to store in the history.
void Sampler_setHistorySize(int newSize)
{
    if (newSize<arraySize){
        if (newSize==0){
            newSize = 1;
        }
        double* temp = (double*) malloc(newSize*sizeof(double));
        /* if old array is not full, just copy from start til last valid value */
        if (recent[(recentIndex+1)%arraySize]==0){
            for (int i = 0; i<(recentIndex+1)%arraySize; i++){
                temp[i] = recent[i];
            }
        }
        // array is full, copy from least recent to most recent
        else {
            printf("recent=%d  ",(recentIndex+1)%arraySize);
            for (int i = 0,j=arraySize+(recentIndex-newSize+2); i<newSize; i++,j++){
                printf("%d  ",j%arraySize);
                temp[i] = recent[j%arraySize];
            }
            recentIndex = newSize-1;
        }
        arraySize = newSize;
        free(recent);
        recent = temp;
    }
    else if (newSize>arraySize){
        double* temp = (double*) malloc(newSize*sizeof(double));
        for (int i = 0; i<arraySize; i++){
            temp[i] = recent[i];
        }
        for (int i = arraySize; i<newSize; i++){
            temp[i] = 0;
        }
        arraySize = newSize;
        free(recent);
        recent = temp;
    }
}
char* Sampler_get_N(int n){
    char* returnMsg = (char*) malloc(MSG_MAX_LEN*sizeof(char));
    char temp[317];
    if (n>historyCount){
        sprintf(returnMsg, "%d", historyCount);
        return returnMsg;
    }
    for (int i = 0,j=arraySize+(recentIndex-n+2); i<n; i++,j++){
        sprintf(temp,"%f  ",recent[j%arraySize]);
        strcat(returnMsg,temp);
    }
    return returnMsg;
}
int Sampler_getHistorySize(void){
    return arraySize;
}
// Get a copy of the samples in the sample history.
// Returns a newly allocated array and sets `length` to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
// Note: provides both data and size to ensure consistency.
double* Sampler_getHistory(int *length){
    return recent;
}
// Returns how many valid samples are currently in the history.
// May be less than the history size if the history is not yet full.
int Sampler_getNumSamplesInHistory(){
    return historyCount;
}
// Get the average light level (not tied to the history).
double Sampler_getAverageReading(void){
    return currentAverage;
}
// Get the total number of light level samples taken so far.
long long Sampler_getNumSamplesTaken(void){
    return allTimeCount;
}
static void signal_handler(){
    free(recent);
    pthread_cancel(threadSampling);
}
static void* samplerThread()
{
    int perSecCount = 1;
    
    int reading = getVoltageReading(A2D_FILE_VOLTAGE1);
    
    arraySize = getVoltageReading(A2D_FILE_VOLTAGE0);
    if (arraySize==0){
        arraySize = 1;
    }
    recent = (double*) malloc(arraySize*sizeof(double));

    if (recent == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    for (int i = 0; i<arraySize; i++){
        recent[i] = 0;
    }
    double voltage = ((double)reading / A2D_MAX_READING) * A2D_VOLTAGE_REF_V;
    currentAverage = voltage;
    // first run
    long long currentTime = getTimeInMs();
    printf("Samples/s = %d  ", perSecCount);
    printf("history size = %d", historyCount);
    printf("    Pot Value %5d\n", reading);
    recent[recentIndex] = voltage;
    perSecCount = 0;
    int dipCount = 0;
    int lastDip = 0;
    while (true) {
        signal(SIGINT,signal_handler);
        reading = getVoltageReading(A2D_FILE_VOLTAGE1);
        
        voltage = ((double)reading / A2D_MAX_READING) * A2D_VOLTAGE_REF_V;
        currentAverage = exponential_smoothing(currentAverage, voltage, 0.001);
        // printf("new avg:%5.3f",currentAverage);
        // if dip detected
        if (lastDip==0 && voltage <= currentAverage-0.1){
            lastDip = 1;
            voltage = currentAverage;
            dipCount++;
        }
        else if (lastDip!=0 && voltage > currentAverage-0.1){
            lastDip=0;
        }
        if (getTimeInMs()>=currentTime+1000){
            int newSize = getVoltageReading(A2D_FILE_VOLTAGE0);
            if (newSize!=arraySize){
                Sampler_setHistorySize(newSize);
            }
            printf("Samples/s = %d  ", perSecCount);
            printf("history size = %d    ", historyCount);
            printf("Value %5d ==> %5.3fV  ", reading, voltage);
            printf("avg:%5.3f ",currentAverage);
            printf("dips: %d\n",dipCount);
            perSecCount = 0;
            /* if array is not full, just print from start til last valid value */
            if (recent[(recentIndex+1)%arraySize]==0){
                for (int i = 0; i<recentIndex+1; i+=200){
                    printf("%5.3f    ",recent[i]);
                }
            }
            // array is full, print from the least recent backwards to most recent (wrap around)
            else {
                int index = 0;
                for (index=recentIndex+1;index<arraySize; index+=200){
                    printf("%5.3f    ",recent[index]);
                }
                for (index=(arraySize+index)%arraySize;index<=recentIndex; index+=200){
                    printf("%5.3f    ",recent[index]);
                }
            }
            printf("\n");
            currentTime = getTimeInMs();

        }
        
        recentIndex = (recentIndex+1)%arraySize;
        recent[recentIndex] = voltage;
        allTimeCount++;
        perSecCount++;
        if (historyCount <= arraySize){
            historyCount++;
        }
        else {
            historyCount = arraySize;
        }
        sleepForMs(1);
    }
    return NULL;
}


static double exponential_smoothing(double curr_avg, double nth_sample, double a)
{
    // printf("exponentially smoothing previous: %5.3f, curr: %5.3f\n",currentAverage,nth_sample);
    return a*nth_sample + (1-a)*curr_avg;
}


static int getVoltageReading(char* file)
{
    // Open file
    FILE *f = fopen(file, "r");
    if (!f) {
        printf("ERROR: Unable to open voltage input file. Cape loaded?\n");
        printf(" Check /boot/uEnv.txt for correct options.\n");
        exit(-1);
    }
    // Get reading
    int a2dReading = 0;
    int itemsRead = fscanf(f, "%d", &a2dReading);
    if (itemsRead <= 0) {
        printf("ERROR: Unable to read values from voltage input file.\n");
        exit(-1);
    }
    // Close file
    fclose(f);
        return a2dReading;
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