/*
 *  Small program to read a 16-bit, signed, 44.1kHz wave file and play it.
 *  Written by Brian Fraser, heavily based on code found at:
 *  http://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_min_8c-example.html
 */

#include <alsa/asoundlib.h>
#include <signal.h>
#include <stdbool.h>
#include <limits.h>
#include "audiomixer.h"
#include "joystickInput.h"
// File used for play-back:
// If cross-compiling, must have this file available, via this relative path,
// on the target when the application is run. This example's Makefile copies the wave-files/
// folder along with the executable to ensure both are present.
#define BASE_DRUM "wave-files/100051__menegass__gui-drum-bd-hard.wav"
#define HI_HAT "wave-files/100053__menegass__gui-drum-cc.wav"
#define SNARE "wave-files/100059__menegass__gui-drum-snare-soft.wav"

#define SAMPLE_RATE   44100
#define NUM_CHANNELS  1
#define SAMPLE_SIZE   (sizeof(short)) 	// bytes per sample

int running = 1;
// Store data of a single wave file read into memory.
// Space is dynamically allocated; must be freed correctly!

// Prototypes:
void rockBeat();
void randomBeat();
void playBeat(int currentMode);
static wavedata_t baseDrumFile;
static wavedata_t hiHatFile;
static wavedata_t snareFile;
static float timeForHalfBeatMs;
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

int main(void)
{
	// printf("Beginning play-back of %s\n", BASE_DRUM);
	AudioMixer_init();
	joystick_startThread();
	// Load wave file we want to play:
	AudioMixer_readWaveFileIntoMemory(BASE_DRUM, &baseDrumFile);
    
	AudioMixer_readWaveFileIntoMemory(HI_HAT,&hiHatFile);
	
	AudioMixer_readWaveFileIntoMemory(SNARE, &snareFile);
	
	while (true){
		int tempo = AudioMixer_getTempo();
		int currentMode = AudioMixer_getMode();
		timeForHalfBeatMs = (60 / (float)tempo / 2)*1000;
		playBeat(currentMode);
	}
	
	AudioMixer_join();
	joystick_stopThread();
	printf("Done!\n");
	return 0;
}
void playBeat(int currentMode){
	if (currentMode == 1){
		rockBeat();
	}
	else if (currentMode == 2){

	}
	else {
		randomBeat();
	}
}
void randomBeat()
{
	for (int i = 0; i < 4; i++){
		pthread_mutex_lock(&audioMutex);
		{
			AudioMixer_queueSound(&baseDrumFile);
		}
		pthread_mutex_unlock(&audioMutex);
		sleepForMs(timeForHalfBeatMs);
	}
	
	pthread_mutex_lock(&audioMutex);
	{
		AudioMixer_queueSound(&snareFile);
	}
	pthread_mutex_unlock(&audioMutex);
	
	sleepForMs(timeForHalfBeatMs);

	for (int i = 0; i < 3; i++){
		pthread_mutex_lock(&audioMutex);
		{
			AudioMixer_queueSound(&baseDrumFile);
		}
		pthread_mutex_unlock(&audioMutex);
		sleepForMs(timeForHalfBeatMs);
	}
}
void rockBeat()
{
	pthread_mutex_lock(&audioMutex);
	{
		printf("basedrum and hihat\n");
		AudioMixer_queueSound(&hiHatFile);
		AudioMixer_queueSound(&baseDrumFile);
	}
	pthread_mutex_unlock(&audioMutex);
	sleepForMs(timeForHalfBeatMs);
	pthread_mutex_lock(&audioMutex);
	{
		printf("hihat\n");
		AudioMixer_queueSound(&hiHatFile);
	}
	pthread_mutex_unlock(&audioMutex);
	sleepForMs(timeForHalfBeatMs);
	pthread_mutex_lock(&audioMutex);
	{
		printf("hihat and snare\n");
		AudioMixer_queueSound(&hiHatFile);
		AudioMixer_queueSound(&snareFile);
	}
	pthread_mutex_unlock(&audioMutex);
	sleepForMs(timeForHalfBeatMs);
	pthread_mutex_lock(&audioMutex);
	{
		AudioMixer_queueSound(&hiHatFile);
	}
	pthread_mutex_unlock(&audioMutex);
	sleepForMs(timeForHalfBeatMs);
}