// Playback sounds in real time, allowing multiple simultaneous wave files
// to be mixed together and played without jitter.
#include <pthread.h>
#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

typedef struct {
	int numSamples;
	short *pData;
} wavedata_t;

#define AUDIOMIXER_MAX_VOLUME 100
enum beatMode {NONE=1,ROCK=2,CUSTOM=3};
extern pthread_mutex_t audioMutex;
extern int numSoundBites;
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
int AudioMixer_setTempo(int tempo);

int AudioMixer_getMode();
int AudioMixer_setMode(enum beatMode newMode);
int AudioMixer_cycleNextMode();

#endif