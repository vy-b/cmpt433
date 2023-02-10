#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "sampler.h"
#include "led_display.h"
#include "periodTimer.h"
#define MSG_MAX_LEN 1024
static int recentIndex = 0;
static int historyCount = 0;
static int dipCount = 0;
static long long allTimeCount = 1;
static double currentAverage = 0;
static int arraySize = 1;

static pthread_t threadSampling;
static double *recent;

static pthread_mutex_t historyMutex = PTHREAD_MUTEX_INITIALIZER;
static void sleepForMs(long long delayInMs);
static long long getTimeInMs(void);
static int getVoltageReading(char *file);
static double exponential_smoothing(double currentAverage, double nth_sample, double a);
static void *samplerThread();

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
    if (newSize < arraySize)
    {
        if (newSize == 0)
        {
            newSize = 1;
        }
        double *temp = (double *)malloc(newSize * sizeof(double));
        pthread_mutex_lock(&historyMutex);
        {
            /* if old array is not full, just copy from start til last valid value */
            if (recent[(recentIndex + 1) % arraySize] == 0)
            {
                for (int i = 0; i < (recentIndex + 1) % arraySize; i++)
                {
                    temp[i] = recent[i];
                }
            }
            // array is full, copy from least recent in range to most recent
            else
            {
                for (int i = 0, j = arraySize + (recentIndex - newSize + 2); i < newSize; i++, j++)
                {
                    temp[i] = recent[j % arraySize];
                }
                recentIndex = newSize - 1;
            }
            arraySize = newSize;
            free(recent);
            recent = temp;
        }
        pthread_mutex_unlock(&historyMutex);
    }
    else if (newSize > arraySize)
    {
        double *temp = (double *)malloc(newSize * sizeof(double));
        pthread_mutex_lock(&historyMutex);
        {
            for (int i = 0; i < arraySize; i++)
            {
                temp[i] = recent[i];
            }
            for (int i = arraySize; i < newSize; i++)
            {
                temp[i] = 0;
            }
            arraySize = newSize;
            free(recent);
            recent = temp;
        }
        pthread_mutex_unlock(&historyMutex);
    }
}

// Gets N most recent items in history to print
double *Sampler_get_N(int n)
{
    // to fix
    if (n > historyCount)
    {
        return (double *)(&historyCount);
    }

    double *N_list = (double *)malloc(n * sizeof(double));
    memset(N_list, 0, n * sizeof(double));
    pthread_mutex_lock(&historyMutex);
    {
        for (int i = 0, j = arraySize + (recentIndex - n + 1); i < n; i++, j++)
        {
            N_list[i] = recent[j % arraySize];
        }
    }
    pthread_mutex_unlock(&historyMutex);
    return N_list;
}
int Sampler_getHistorySize(void)
{
    int ret = 0;
    pthread_mutex_lock(&historyMutex);
    {
        ret = arraySize;
    }
    pthread_mutex_unlock(&historyMutex);
    return ret;
}
// Get a copy of the samples in the sample history.
// Returns a newly allocated array and sets `length` to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
// Note: provides both data and size to ensure consistency.
double *Sampler_getHistory(int *length)
{
    double *ret = (double *)malloc(arraySize * sizeof(double));
    pthread_mutex_lock(&historyMutex);
    {
        ret = recent;
        *length = historyCount;
    }
    pthread_mutex_unlock(&historyMutex);
    return ret;
}
// Returns how many valid samples are currently in the history.
// May be less than the history size if the history is not yet full.
int Sampler_getNumSamplesInHistory()
{
    int ret = 0;
    pthread_mutex_lock(&historyMutex);
    {
        ret = historyCount;
    }
    pthread_mutex_unlock(&historyMutex);
    return ret;
}
// Get the average light level (not tied to the history).
double Sampler_getAverageReading(void)
{
    double ret = 0;
    pthread_mutex_lock(&historyMutex);
    {
        ret = currentAverage;
    }
    pthread_mutex_unlock(&historyMutex);
    return ret;
}
// Get the total number of light level samples taken so far.
long long Sampler_getNumSamplesTaken(void)
{
    long long ret = 0;
    pthread_mutex_lock(&historyMutex);
    {
        ret = allTimeCount;
    }
    pthread_mutex_unlock(&historyMutex);
    return ret;
}
int Sampler_getNumDips(void)
{
    int ret = 0;
    pthread_mutex_lock(&historyMutex);
    {
        ret = dipCount;
    }
    pthread_mutex_unlock(&historyMutex);
    return ret;
}
static void signal_handler()
{
    free(recent);
    pthread_cancel(threadSampling);
}
static void *samplerThread()
{
    int perSecCount = 1;
    int reading = getVoltageReading(A2D_FILE_VOLTAGE1);

    arraySize = getVoltageReading(A2D_FILE_VOLTAGE0);
    if (arraySize == 0)
    {
        arraySize = 1;
    }
    recent = (double *)malloc(arraySize * sizeof(double));
    if (recent == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    for (int i = 0; i < arraySize; i++)
    {
        recent[i] = 0;
    }
    // first run
    long long currentTime = getTimeInMs();
    perSecCount = 0;
    int lastDip = 0; // flag for encountering a dip
    int recentDips = 0;
    int secondsWithDips = 0;
    printf("Starting...\n");
    Period_statistics_t *pStats = (Period_statistics_t *)malloc(sizeof(Period_statistics_t));
    while (true)
    {
        signal(SIGINT, signal_handler);
        reading = getVoltageReading(A2D_FILE_VOLTAGE1);
        Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
        double voltage = ((double)reading / A2D_MAX_READING) * A2D_VOLTAGE_REF_V;

        currentAverage = exponential_smoothing(currentAverage, voltage, 0.001);
        // printf("new avg:%5.3f",currentAverage);
        recentIndex = (recentIndex + 1) % arraySize;
        // if dip detected
        if (lastDip == 0 && voltage <= currentAverage - 0.1)
        {
            lastDip = 1;
            voltage = currentAverage;
            dipCount++;
        }
        else if (lastDip != 0 && voltage > currentAverage + 0.07)
        {
            lastDip = 0;
        }
        pthread_mutex_lock(&historyMutex);
        {
            recent[recentIndex] = voltage;
        
            allTimeCount++;
            perSecCount++;
            if (historyCount < arraySize)
            {
                historyCount++;
            }
            else
            {
                historyCount = arraySize - 1;
            }
        }
        pthread_mutex_unlock(&historyMutex);
        if (getTimeInMs() >= currentTime + 1000)
        {
            if (recentDips == dipCount && dipCount != 0)
            {
                if (secondsWithDips > 0)
                {
                    int dipsPerSec = dipCount / secondsWithDips;
                    dipCount -= dipsPerSec;
                    secondsWithDips--;
                }
            }
            else if (dipCount!=0)
            {
                secondsWithDips++;
            }
            recentDips = dipCount;
            pthread_mutex_lock(&historyMutex);
            {
                printf("Samples/s = %d  ", perSecCount);
                printf("Pot Value %d  ", arraySize);
                printf("history size = %d    ", historyCount);
                printf("avg:%5.3f ", currentAverage);
                printf("dips: %d  ", dipCount);
                Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, pStats);
                printf("Sampling[  %5.3f,  %5.3f] avg %5.3f/%d\n", pStats->minPeriodInMs, pStats->maxPeriodInMs, pStats->avgPeriodInMs, pStats->numSamples);
                perSecCount = 0;
                int newSize = getVoltageReading(A2D_FILE_VOLTAGE0);

                if (newSize != arraySize)
                {
                    Sampler_setHistorySize(newSize);
                }
            }
            pthread_mutex_unlock(&historyMutex);
            
            /* if array is not full, just print from start til last valid value */
            if (recent[(recentIndex + 1) % arraySize] == 0)
            {
                for (int i = 0; i < recentIndex + 1; i += 200)
                {
                    printf("%5.3f    ", recent[i]);
                }
            }
            // array is full, print from the least recent to most recent (wrap around)
            else
            {
                int index = 0;
                for (index = recentIndex + 2; index < arraySize; index += 200)
                {
                    printf("%5.3f    ", recent[index]);
                }
                for (index = (arraySize + index) % arraySize; index <= recentIndex; index += 200)
                {
                    printf("%5.3f    ", recent[index]);
                }
            }
            printf("\n");
            currentTime = getTimeInMs();
        }
        

        sleepForMs(1);
    }
    return NULL;
}

static double exponential_smoothing(double curr_avg, double nth_sample, double a)
{
    // printf("exponentially smoothing previous: %5.3f, curr: %5.3f\n",currentAverage,nth_sample);
    return a * nth_sample + (1 - a) * curr_avg;
}

static int getVoltageReading(char *file)
{
    // Open file
    FILE *f = fopen(file, "r");
    if (!f)
    {
        printf("ERROR: Unable to open voltage input file. Cape loaded?\n");
        printf(" Check /boot/uEnv.txt for correct options.\n");
        exit(-1);
    }
    // Get reading
    int a2dReading = 0;
    int itemsRead = fscanf(f, "%d", &a2dReading);
    if (itemsRead <= 0)
    {
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
    nanosleep(&reqDelay, (struct timespec *)NULL);
}

static long long getTimeInMs(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long long seconds = spec.tv_sec;
    long long nanoSeconds = spec.tv_nsec;
    long long milliSeconds = seconds * 1000 + nanoSeconds / 1000000;
    return milliSeconds;
}