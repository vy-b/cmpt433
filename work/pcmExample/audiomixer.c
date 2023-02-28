/*
 Complete Audio Mixer by Vy Bui
 Starts a playback thread that receives sounds added to queue by other threads
 Plays singular or combined sounds
 */
#include "audiomixer.h"
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>
#include <alloca.h> // needed for mixer
#include <signal.h>

static snd_pcm_t *handle;
#define DEFAULT_VOLUME 50
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1
#define SAMPLE_SIZE (sizeof(short)) 			// bytes per sample
// Sample size note: This works for mono files because each sample ("frame') is 1 value.
// If using stereo files then a frame would be two samples.

static unsigned long playbackBufferSize = 0;
static short *playbackBuffer = NULL;
Period_statistics_t *pStats;
// Currently active (waiting to be played) sound bites
#define MAX_SOUND_BITES 30
typedef struct {
	// A pointer to a previously allocated sound bite (wavedata_t struct).
	// Note that many different sound-bite slots could share the same pointer
	// (overlapping cymbal crashes, for example)
	wavedata_t *pSound;

	// The offset into the pData of pSound. Indicates how much of the
	// sound has already been played (and hence where to start playing next).
	int location;
} playbackSound_t;
static playbackSound_t soundBites[MAX_SOUND_BITES];
int numSoundBites = 0;
// Playback threading
void* playbackThread(void* arg);
static pthread_t playbackThreadId;
int stopping = 0;
pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;
static int currentMode = 1;
static int volume = 0;
static int tempo = 70;
void sleepForMs(long long delayInMs)
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
void AudioMixer_init(void)
{
	AudioMixer_setVolume(DEFAULT_VOLUME);
	for (int i = 0; i < MAX_SOUND_BITES; i++){
		soundBites[i].pSound = NULL;
		soundBites[i].location = 0;
	}
	
	// Initialize the currently active sound-bites being played
	// REVISIT:- Implement this. Hint: set the pSound pointer to NULL for each
	//     sound bite.

	// Open the PCM output
	int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
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
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	// Allocate this software's playback buffer to be the same size as the
	// the hardware's playback buffers for efficient data transfers.
	// ..get info on the hardware buffers:
 	unsigned long unusedBufferSize = 0;
	snd_pcm_get_params(handle, &unusedBufferSize, &playbackBufferSize);
	// ..allocate playback buffer:
	playbackBuffer = malloc(playbackBufferSize * sizeof(*playbackBuffer));
	memset(playbackBuffer,0,playbackBufferSize*sizeof(short));
	if (!playbackBuffer){
		printf("Malloc failed");
	}
	// Launch playback thread:
	pthread_create(&playbackThreadId, NULL, playbackThread, NULL);
}

void AudioMixer_join(void)
{
    pthread_join(playbackThreadId, NULL);
}
// Client code must call AudioMixer_freeWaveFileData to free dynamically allocated data.
void AudioMixer_readWaveFileIntoMemory(char *fileName, wavedata_t *pSound)
{
	assert(pSound);

	// The PCM data in a wave file starts after the header:
	const int PCM_DATA_OFFSET = 44;

	// Open the wave file
	FILE *file = fopen(fileName, "r");
	if (file == NULL) {
		fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName);
		exit(EXIT_FAILURE);
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	int sizeInBytes = ftell(file) - PCM_DATA_OFFSET;
	pSound->numSamples = sizeInBytes / SAMPLE_SIZE;

	// Search to the start of the data in the file
	fseek(file, PCM_DATA_OFFSET, SEEK_SET);

	// Allocate space to hold all PCM data
	pSound->pData = malloc(sizeInBytes);
	if (pSound->pData == 0) {
		fprintf(stderr, "ERROR: Unable to allocate %d bytes for file %s.\n",
				sizeInBytes, fileName);
		exit(EXIT_FAILURE);
	}

	// Read PCM data from wave file into memory
	int samplesRead = fread(pSound->pData, SAMPLE_SIZE, pSound->numSamples, file);
	if (samplesRead != pSound->numSamples) {
		fprintf(stderr, "ERROR: Unable to read %d samples from file %s (read %d).\n",
				pSound->numSamples, fileName, samplesRead);
		exit(EXIT_FAILURE);
	}
	// for (int i = 0; i<pSound->numSamples; i++){
	// 	printf("%hi\n",pSound->pData[i]);
	// }
}

void AudioMixer_freeWaveFileData(wavedata_t *pSound)
{
	pSound->numSamples = 0;
	free(pSound->pData);
	pSound->pData = NULL;
}

int AudioMixer_queueSound(wavedata_t *pSound)
{
	// Ensure we are only being asked to play "good" sounds:
	assert(pSound->numSamples > 0);
	assert(pSound->pData);

    int i = 0;
	while (i < MAX_SOUND_BITES){
		if (soundBites[i].pSound==NULL){
			soundBites[i].pSound = pSound;
			numSoundBites++;
			return 0;
		}
		i++;
	}
    printf("No more free slots available in soundBites array!\n");
    return -1;

	// Insert the sound by searching for an empty sound bite spot
	/*
	 * REVISIT: Implement this:
	 * 1. Since this may be called by other threads, and there is a thread
	 *    processing the soundBites[] array, we must ensure access is threadsafe.
	 * 2. Search through the soundBites[] array looking for a free slot.
	 * 3. If a free slot is found, place the new sound file into that slot.
	 *    Note: You are only copying a pointer, not the entire data of the wave file!
	 * 4. After searching through all slots, if no free slot is found then print
	 *    an error message to the console (and likely just return vs asserting/exiting
	 *    because the application most likely doesn't want to crash just for
	 *    not being able to play another wave file.
	 */





}

void AudioMixer_cleanup(void)
{
	printf("Stopping audio...\n");

	// Stop the PCM generation thread
	stopping = true;
	pthread_join(playbackThreadId, NULL);

	// Shutdown the PCM output, allowing any pending sound to play out (drain)
	snd_pcm_drain(handle);
	snd_pcm_close(handle);

	// Free playback buffer
	// (note that any wave files read into wavedata_t records must be freed
	//  in addition to this by calling AudioMixer_freeWaveFileData() on that struct.)
	pthread_mutex_destroy(&audioMutex);
	free(playbackBuffer);
	playbackBuffer = NULL;

	printf("Done stopping audio...\n");
	fflush(stdout);
}
int AudioMixer_getTempo()
{
	return tempo;
}
void AudioMixer_setTempo(int newTempo)
{
	if (newTempo < 0 || newTempo > AUDIOMIXER_MAX_TEMPO) {
		printf("ERROR: Volume must be between 0 and 500.\n");
		return;
	}
	tempo = newTempo;
	printf("Tempo changed: %d\n",newTempo);
	return;
}
int AudioMixer_getMode()
{
	return currentMode;
}
int AudioMixer_setMode(enum beatMode newMode)
{
	currentMode = newMode;
	return currentMode;
}
int AudioMixer_cycleNextMode()
{
	if (currentMode == 3){
		currentMode = 1;
	}
	else{
		currentMode++;
	}
	printf("mode changed:%d\n",currentMode);

	return currentMode;
}
int AudioMixer_getVolume()
{
	// Return the cached volume; good enough unless someone is changing
	// the volume through other means and the cached value is out of date.
	return volume;
}

// Function copied from:
// http://stackoverflow.com/questions/6787318/set-alsa-master-volume-from-c-code
// Written by user "trenki".
void AudioMixer_setVolume(int newVolume)
{
	// Ensure volume is reasonable; If so, cache it for later getVolume() calls.
	if (newVolume < 0 || newVolume > AUDIOMIXER_MAX_VOLUME) {
		printf("ERROR: Volume must be between 0 and 100.\n");
		return;
	}
	volume = newVolume;

    long min, max;
    snd_mixer_t *volHandle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "PCM";

    snd_mixer_open(&volHandle, 0);
    snd_mixer_attach(volHandle, card);
    snd_mixer_selem_register(volHandle, NULL, NULL);
    snd_mixer_load(volHandle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(volHandle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

    snd_mixer_close(volHandle);
}
static short clipNum(int num)
{
	if (num > SHRT_MAX) return SHRT_MAX-1;
	else if (num < SHRT_MIN) return SHRT_MIN+1;
	else return (short)num;
}

// Fill the `buff` array with new PCM values to output.
//    `buff`: buffer to fill with new PCM data from sound bites.
//    `size`: the number of values to store into playbackBuffer
static void fillPlaybackBuffer(short *buff, int size)
{
    memset(buff,0,size*sizeof(short));
	int i = 0;
	int curr_size = 0;
	while (i < MAX_SOUND_BITES && curr_size < size){
		if (soundBites[i].pSound!=NULL){
			if (soundBites[i].pSound->pData != 0){
				// add pcm
				int offset = soundBites[i].location;
				int j = offset;
				int k = 0;
				
				for (k = 0; j < soundBites[i].pSound->numSamples && k<size; j++,k++){
					int shrt_data=buff[k]+soundBites[i].pSound->pData[j];
					buff[k] = clipNum(shrt_data);
					offset++;
				}
				if (offset>=soundBites[i].pSound->numSamples){
					// printf("%d\n",soundBites[i].pSound->numSamples);
					// free the soundBite slot that has finished playing
					soundBites[i].pSound = NULL;
					soundBites[i].location = 0;
					numSoundBites--;
				}
				else {
					soundBites[i].location = offset;
				}
				curr_size+=SAMPLE_SIZE;
			}
		}
		i++;
	}

}


void* playbackThread(void* arg)
{
	long long currentTime = getTimeInMs();
	pStats = (Period_statistics_t *)malloc(sizeof(Period_statistics_t));
	while (!stopping) {

		// signal(SIGINT, signal_handler);
		
		pthread_mutex_lock(&audioMutex);
		{
		Period_markEvent(PERIOD_EVENT_FILL_BUFFER);
		// Generate next block of audio
		fillPlaybackBuffer(playbackBuffer, playbackBufferSize);
		if (getTimeInMs() >= currentTime + 1000){
			Period_getStatisticsAndClear(PERIOD_EVENT_FILL_BUFFER, pStats);
			printf("M%d %dbpm vol:%d Audio[%5.3f, %5.3f] avg %5.3f/%d\n",AudioMixer_getMode(),AudioMixer_getTempo(),AudioMixer_getVolume(),pStats->minPeriodInMs, pStats->maxPeriodInMs, pStats->avgPeriodInMs, pStats->numSamples);
			currentTime = getTimeInMs();
		}
		
		// Output the audio
		snd_pcm_sframes_t frames = snd_pcm_writei(handle,
				playbackBuffer, playbackBufferSize);

		// Check for (and handle) possible error conditions on output
		if (frames < 0) {
			fprintf(stderr, "AudioMixer: writei() returned %li\n", frames);
			frames = snd_pcm_recover(handle, frames, 1);
		}
		if (frames < 0) {
			fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n",
					frames);
			exit(EXIT_FAILURE);
		}
		if (frames > 0 && frames < playbackBufferSize) {
			printf("Short write (expected %li, wrote %li)\n",
					playbackBufferSize, frames);
		}
		}
		pthread_mutex_unlock(&audioMutex);
	}
	free(pStats);
	return NULL;
}















