/*
 UDP thread to listen for requests from JS server
 by Vy Bui
 Calls functions from audio mixer thread to satisfy server request
 Sends reponse to server to update on website
 */

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>			// for strncmp()
#include <unistd.h>			// for close()
#include "audiomixer.h"
#include "udpListener.h"
#define PORT        12345
static pthread_t threadUdp;
static int running = 1;
static int socketDescriptor;
static struct sockaddr_in sinRemote;
static void* handleRequest(char* messageRx, char* messageTx);
static wavedata_t baseDrumFile;
static wavedata_t hiHatFile;
static wavedata_t snareFile;
void* udpListenThread()
{
	printf("Vy's Net Listen Test on UDP port %d:\n", PORT);
	printf("Connect using: \n");
	printf("    netcat -u 127.0.0.1 %d\n", PORT);
	AudioMixer_readWaveFileIntoMemory(BASE_DRUM, &baseDrumFile);
	AudioMixer_readWaveFileIntoMemory(HI_HAT,&hiHatFile);
	AudioMixer_readWaveFileIntoMemory(SNARE, &snareFile);
	// Address
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;                   // Connection may be from network
	sin.sin_addr.s_addr = htonl(INADDR_ANY);    // Host to Network long
	sin.sin_port = htons(PORT);                 // Host to Network short
	
	// Create the socket for UDP
	socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);

	// Bind the socket to the port (PORT) that we specify
	bind (socketDescriptor, (struct sockaddr*) &sin, sizeof(sin));	

	while (running) {
		// Get the data (blocking)
		// Will change sin (the address) to be the address of the client.
		// Note: sin passes information in and out of call!
		unsigned int sin_len = sizeof(sinRemote);
		
		char messageRx[MSG_MAX_LEN];
		char messageTx[MSG_MAX_LEN];

		// Pass buffer size - 1 for max # bytes so room for the null (string data)
		int bytesRx = recvfrom(socketDescriptor,
			messageRx, MSG_MAX_LEN - 1, 0,
			(struct sockaddr *) &sinRemote, &sin_len);

		// Make it null terminated (so string functions work)
		// - recvfrom given max size - 1, so there is always room for the null
		messageRx[bytesRx] = 0;
		
		handleRequest(messageRx, messageTx);
        
	}
	
	// Close
	close(socketDescriptor);
	pthread_cancel(pthread_self());

	return NULL;
}
void* handleRequest(char* messageRx, char* messageTx)
{
	// Compose the reply message:
	// (NOTE: watch for buffer overflows!).
	if (strncmp("modeNone",messageRx, strlen("modeNone"))==0){
        AudioMixer_setMode(NONE);
		strcpy(messageTx, "Current mode: None\n");
	}
    else if (strncmp("modeRock",messageRx, strlen("modeRock"))==0){
        AudioMixer_setMode(ROCK);
		strcpy(messageTx, "Current mode: Rock!!!\n");
	}
    else if (strncmp("modeCustom",messageRx, strlen("modeCustom"))==0){
        AudioMixer_setMode(CUSTOM);
		strcpy(messageTx, "Current mode: Custom\n");
	}
    else if (strncmp("volumeUp",messageRx, strlen("volumeUp"))==0){
        AudioMixer_setVolume(AudioMixer_getVolume()+5);
		sprintf(messageTx, "new volume: %d\n",AudioMixer_getVolume());
	}
    else if (strncmp("volumeDown",messageRx, strlen("volumeDown"))==0){
        AudioMixer_setVolume(AudioMixer_getVolume()-5);
		sprintf(messageTx, "new volume: %d\n",AudioMixer_getVolume());
	}
    else if (strncmp("tempoUp",messageRx, strlen("tempoUp"))==0){
        AudioMixer_setTempo(AudioMixer_getTempo()+5);
		sprintf(messageTx, "new tempo: %d\n",AudioMixer_getTempo());
	}
    else if (strncmp("tempoDown",messageRx, strlen("tempoDown"))==0){
        AudioMixer_setTempo(AudioMixer_getTempo()-5);
		sprintf(messageTx, "new tempo: %d\n",AudioMixer_getTempo());
	}
	else if (strncmp("getvol",messageRx, strlen("getVol"))==0){
		sprintf(messageTx, "current volume: %d\n",AudioMixer_getVolume());
	}
	else if (strncmp("gettempo",messageRx, strlen("getTempo"))==0){
		sprintf(messageTx, "current tempo: %d\n",AudioMixer_getTempo());
	}
	else if (strncmp("getUpdates",messageRx, strlen("getUpdates"))==0){
		FILE *uptime_file = fopen("/proc/uptime", "r");
		if (uptime_file == NULL) {
			printf("Error: could not open uptime file\n");
			return NULL;
		}

		double uptime;
		fscanf(uptime_file, "%lf", &uptime);

		fclose(uptime_file);
		sprintf(messageTx, "update: %d %d %d %.2f\n",AudioMixer_getTempo(),AudioMixer_getVolume(),AudioMixer_getMode(),uptime);

	}
	else if (strncmp("base",messageRx, strlen("base"))==0){
		pthread_mutex_lock(&audioMutex);
		{
			AudioMixer_queueSound(&baseDrumFile);
		}
		pthread_mutex_unlock(&audioMutex);
		strcpy(messageTx, "playing base drum\n");
	}
	else if (strncmp("hihat",messageRx, strlen("hihat"))==0){
		pthread_mutex_lock(&audioMutex);
		{
			AudioMixer_queueSound(&hiHatFile);
		}
		pthread_mutex_unlock(&audioMutex);
		strcpy(messageTx, "playing hihat\n");
	}
	else if (strncmp("snare",messageRx, strlen("snare"))==0){
		pthread_mutex_lock(&audioMutex);
		{
			AudioMixer_queueSound(&snareFile);
		}
		pthread_mutex_unlock(&audioMutex);
		strcpy(messageTx, "playing snare\n");
	}
	else if (strncmp("terminate",messageRx, strlen("terminate"))==0){
		AudioMixer_cleanup();
		AudioMixer_freeWaveFileData(&baseDrumFile);
		AudioMixer_freeWaveFileData(&hiHatFile);
		AudioMixer_freeWaveFileData(&snareFile);
		strcpy(messageTx, "Server shut down. Goodbye!\n");
		running = 0;
	}
	// Transmit a reply:
	unsigned int sin_len = sizeof(sinRemote);
	sendto( socketDescriptor,
		messageTx, strlen(messageTx),
		0,
		(struct sockaddr *) &sinRemote, sin_len);


	return NULL;
}

void start_udpThread(void){
	pthread_create(&threadUdp, NULL, udpListenThread, NULL);
}
void join_udpThread(void){
	pthread_join(threadUdp, NULL);
}
