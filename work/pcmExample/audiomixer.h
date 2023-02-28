// Playback sounds in real time, allowing multiple simultaneous wave files
// to be mixed together and played without jitter.
#include <pthread.h>
#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H
#include "periodTimer.h"
typedef struct {
	int numSamples;
	short *pData;
} wavedata_t;

#define AUDIOMIXER_MAX_VOLUME 100
#define AUDIOMIXER_MAX_TEMPO 500
// File used for play-back:
#define BASE_DRUM "wave-files/100051__menegass__gui-drum-bd-hard.wav"
#define HI_HAT "wave-files/100053__menegass__gui-drum-cc.wav"
#define SNARE "wave-files/100059__menegass__gui-drum-snare-soft.wav"
enum beatMode {NONE=1,ROCK=2,CUSTOM=3};
extern int stopping;
extern pthread_mutex_t audioMutex;
extern int numSoundBites;
extern Period_statistics_t* pStats;
// init() must be called before any other functions,
// cleanup() must be called last to stop playback threads and free memory.
void AudioMixer_init(void);
void AudioMixer_join(void);
void AudioMixer_cleanup(void);

// Read the contents of a wave file into the pSound structure. Note that
// the pData pointer in this structure will be dynamically allocated in
// readWaveFileIntoMemory(), and is freed by calling freeWaveFileData().
void AudioMixer_readWaveFileIntoMemory(char *fileName, wavedata_t *pSound);
void AudioMixer_freeWaveFileData(wavedata_t *pSound);

// Queue up another sound bite to play as soon as possible.
int AudioMixer_queueSound(wavedata_t *pSound);

// Get/set the volume.
// setVolume() function posted by StackOverflow user "trenki" at:
// http://stackoverflow.com/questions/6787318/set-alsa-master-volume-from-c-code
int  AudioMixer_getVolume(void);
void AudioMixer_setVolume(int newVolume);

int AudioMixer_getTempo();
void AudioMixer_setTempo(int tempo);

int AudioMixer_getMode();
int AudioMixer_setMode(enum beatMode newMode);
int AudioMixer_cycleNextMode();

void sleepForMs(long long delayInMs);
#endif