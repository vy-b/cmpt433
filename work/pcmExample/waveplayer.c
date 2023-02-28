/*
 Main program to start playback thread, udp thread, and joystick thread
 Defines different drum patterns, adds beats to queue when requested
 */

#include <alsa/asoundlib.h>
#include <signal.h>
#include <stdbool.h>
#include <limits.h>
#include "audiomixer.h"
#include "joystickInput.h"
#include "udpListener.h"
// Prototypes:
void rockBeat();
void randomBeat();
void playBeat(int currentMode);
static wavedata_t baseDrumFile;
static wavedata_t hiHatFile;
static wavedata_t snareFile;
static float timeForHalfBeatMs;



int main(void)
{
	Period_init();
	AudioMixer_init();
	joystick_startThread();
	start_udpThread();

	AudioMixer_readWaveFileIntoMemory(BASE_DRUM, &baseDrumFile);
	AudioMixer_readWaveFileIntoMemory(HI_HAT,&hiHatFile);
	AudioMixer_readWaveFileIntoMemory(SNARE, &snareFile);
	while (!stopping){
		int tempo = AudioMixer_getTempo();
		int currentMode = AudioMixer_getMode();
		timeForHalfBeatMs = (60 / (float)tempo / 2)*1000;
		playBeat(currentMode);
	}

	AudioMixer_freeWaveFileData(&baseDrumFile);
	AudioMixer_freeWaveFileData(&hiHatFile);
	AudioMixer_freeWaveFileData(&snareFile);
	printf("stopping main thread\n");

	joystick_shutdown();
	join_udpThread();
	joystick_joinThread();
	Period_cleanup();
	printf("Done!\n");
	return 0;
}
void playBeat(int currentMode){
	if (currentMode == ROCK){
		rockBeat();
	}
	else if (currentMode == NONE){
		sleepForMs(timeForHalfBeatMs);
	}
	else {
		randomBeat();
	}
}
void randomBeat()
{
	pthread_mutex_lock(&audioMutex);
	{
		AudioMixer_queueSound(&baseDrumFile);
	}
	pthread_mutex_unlock(&audioMutex);
	sleepForMs(timeForHalfBeatMs);
	
	pthread_mutex_lock(&audioMutex);
	{
		AudioMixer_queueSound(&hiHatFile);
		AudioMixer_queueSound(&snareFile);
	}
	pthread_mutex_unlock(&audioMutex);
	
	sleepForMs(timeForHalfBeatMs);

	for (int i = 0; i < 2; i++){
		pthread_mutex_lock(&audioMutex);
		{
			AudioMixer_queueSound(&baseDrumFile);
		}
		pthread_mutex_unlock(&audioMutex);
		sleepForMs(timeForHalfBeatMs/2);
	}
	pthread_mutex_lock(&audioMutex);
	{
		AudioMixer_queueSound(&hiHatFile);
		AudioMixer_queueSound(&snareFile);
	}
	pthread_mutex_unlock(&audioMutex);
	sleepForMs(timeForHalfBeatMs);
}
void rockBeat()
{
	pthread_mutex_lock(&audioMutex);
	{
		AudioMixer_queueSound(&hiHatFile);
		AudioMixer_queueSound(&baseDrumFile);
	}
	pthread_mutex_unlock(&audioMutex);
	sleepForMs(timeForHalfBeatMs);
	pthread_mutex_lock(&audioMutex);
	{
		AudioMixer_queueSound(&hiHatFile);
	}
	pthread_mutex_unlock(&audioMutex);
	sleepForMs(timeForHalfBeatMs);
	pthread_mutex_lock(&audioMutex);
	{
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