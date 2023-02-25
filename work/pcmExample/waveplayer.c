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
#define BASE_DRUM "wave-files/100052__menegass__gui-drum-bd-soft.wav"
#define HI_HAT "wave-files/100057__menegass__gui-drum-cyn-soft.wav"
#define SNARE "wave-files/100059__menegass__gui-drum-snare-soft.wav"

#define SAMPLE_RATE   44100
#define NUM_CHANNELS  1
#define SAMPLE_SIZE   (sizeof(short)) 	// bytes per sample

int running = 1;
// Store data of a single wave file read into memory.
// Space is dynamically allocated; must be freed correctly!

// Prototypes:
snd_pcm_t *Audio_openDevice();
void Audio_playFile(snd_pcm_t *handle, wavedata_t *pWaveData);
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
	wavedata_t baseDrumFile;
	AudioMixer_readWaveFileIntoMemory(BASE_DRUM, &baseDrumFile);
    wavedata_t hiHatFile;
	AudioMixer_readWaveFileIntoMemory(HI_HAT,&hiHatFile);
	wavedata_t snareFile;
	AudioMixer_readWaveFileIntoMemory(SNARE, &snareFile);
	
	while (true){
		int tempo = AudioMixer_getTempo();
		printf("%d\n",tempo);
		float timeForHalfBeatMs = (60 / (float)tempo / 2)*1000;
		printf("%.5f\n",timeForHalfBeatMs);
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
			printf("hihat and snare\n");
			AudioMixer_queueSound(&hiHatFile);
			AudioMixer_queueSound(&snareFile);
		}
		pthread_mutex_unlock(&audioMutex);
		sleepForMs(timeForHalfBeatMs);
		pthread_mutex_lock(&audioMutex);
		{
			printf("snare\n");
			// AudioMixer_queueSound(&hiHatFile);
			AudioMixer_queueSound(&snareFile);
		}
		pthread_mutex_unlock(&audioMutex);
		sleepForMs(timeForHalfBeatMs);
		// pthread_mutex_lock(&audioMutex);
		// {
		// 	// AudioMixer_queueSound(&snareFile);
		// 	// AudioMixer_queueSound(&snareFile);
		// }
		// pthread_mutex_unlock(&audioMutex);
		// sleepForMs(timeForHalfBeatMs);
	}
	
	// wavedata_t hihat_base;
	// int sizeInBytes = (baseDrumFile.numSamples + hiHatFile.numSamples) * SAMPLE_SIZE;
	// hihat_base.pData = malloc(sizeInBytes);
	// short data = clipNum(baseDrumFile.pData) + clipNum(hiHatFile.pData);
	// // printf("%hi",hiHatFile.pData);
	// printf("%hi",data);
	// hihat_base.pData = &data;
	// hihat_base.numSamples = baseDrumFile.numSamples + hiHatFile.numSamples;
	
	// Play Audio
	
	//	Audio_playFile(handle, &sampleFile);
	//	Audio_playFile(handle, &sampleFile);

	// Cleanup, letting the music in buffer play out (drain), then close and free.
	// snd_pcm_drain(handle);
	// snd_pcm_hw_free(handle);
	// snd_pcm_close(handle);
	// free(baseDrumFile.pData);
	// free(hiHatFile.pData);
	// free(snareFile.pData);
	// free(hihat_base.pData);
	AudioMixer_join();
	joystick_stopThread();
	printf("Done!\n");
	return 0;
}

// Read in the file to dynamically allocated memory.
// !! Client code must free memory in wavedata_t !!
// void Audio_readWaveFileIntoMemory(char *fileName, wavedata_t *pWaveStruct)
// {
// 	assert(pWaveStruct);

// 	// Wave file has 44 bytes of header data. This code assumes file
// 	// is correct format.
// 	const int DATA_OFFSET_INTO_WAVE = 44;

// 	// Open file
// 	FILE *file = fopen(fileName, "r");
// 	if (file == NULL) {
// 		fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName);
// 		exit(EXIT_FAILURE);
// 	}

// 	// Get file size
// 	fseek(file, 0, SEEK_END);
// 	int sizeInBytes = ftell(file) - DATA_OFFSET_INTO_WAVE;
// 	fseek(file, DATA_OFFSET_INTO_WAVE, SEEK_SET);
// 	pWaveStruct->numSamples = sizeInBytes / SAMPLE_SIZE;

// 	// Allocate Space
// 	pWaveStruct->pData = malloc(sizeInBytes);
// 	if (pWaveStruct->pData == NULL) {
// 		fprintf(stderr, "ERROR: Unable to allocate %d bytes for file %s.\n",
// 				sizeInBytes, fileName);
// 		exit(EXIT_FAILURE);
// 	}

// 	// Read data:
// 	int samplesRead = fread(pWaveStruct->pData, SAMPLE_SIZE, pWaveStruct->numSamples, file);
// 	if (samplesRead != pWaveStruct->numSamples) {
// 		fprintf(stderr, "ERROR: Unable to read %d samples from file %s (read %d).\n",
// 				pWaveStruct->numSamples, fileName, samplesRead);
// 		exit(EXIT_FAILURE);
// 	}

// 	fclose(file);
// }

// Open the PCM audio output device and configure it.
// Returns a handle to the PCM device; needed for other actions.
snd_pcm_t *Audio_openDevice()
{
	snd_pcm_t *handle;

	// Open the PCM output
	int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		printf("Play-back open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	// Configure parameters of PCM output
	err = snd_pcm_set_params(handle,
			SND_PCM_FORMAT_S16_LE,
			SND_PCM_ACCESS_RW_INTERLEAVED,
			NUM_CHANNELS,
			SAMPLE_RATE,
			1,			// Allow software resampling
			50000);		// 0.05 seconds per buffer
	if (err < 0) {
		printf("Play-back configuration error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	return handle;
}

// Play the audio file (blocking)
void Audio_playFile(snd_pcm_t *handle, wavedata_t *pWaveData)
{
	// If anything is waiting to be written to screen, can be delayed unless flushed.
	fflush(stdout);

	// Write data and play sound (blocking)
	snd_pcm_sframes_t frames = snd_pcm_writei(handle, pWaveData->pData, pWaveData->numSamples);

	// Check for errors
	if (frames < 0)
		frames = snd_pcm_recover(handle, frames, 0);
	if (frames < 0) {
		fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n", frames);
		exit(EXIT_FAILURE);
	}
	if (frames > 0 && frames < pWaveData->numSamples)
		printf("Short write (expected %d, wrote %li)\n", pWaveData->numSamples, frames);
}
