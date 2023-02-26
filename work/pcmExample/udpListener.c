/*
 * UDP Listening program for light sampler on port 12345
 * By Vy Bui, adapted from Dr. Brian Fraser's UDP demo
 *
 * Usage:
 *	On the target, run light_sampler program to start all threads.
 *	On the host:
 *		> netcat -u 192.168.7.2 12345
 *
 *	On the host, type 'help' to see a list of commands
 *
 *  Target will return relevant info from the light sampler.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>			// for strncmp()
#include <unistd.h>			// for close()
#include "audiomixer.h"
#define MSG_MAX_LEN 1024
#define PORT        12345
static pthread_t threadUdp;
static int running = 1;
static void* handleRequest(char* messageRx, char* messageTx, struct sockaddr_in sinRemote, int socketDescriptor);
void* udpListenThread()
{
	printf("Vy's Net Listen Test on UDP port %d:\n", PORT);
	printf("Connect using: \n");
	printf("    netcat -u 127.0.0.1 %d\n", PORT);

	// Address
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;                   // Connection may be from network
	sin.sin_addr.s_addr = htonl(INADDR_ANY);    // Host to Network long
	sin.sin_port = htons(PORT);                 // Host to Network short
	
	// Create the socket for UDP
	int socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);

	// Bind the socket to the port (PORT) that we specify
	bind (socketDescriptor, (struct sockaddr*) &sin, sizeof(sin));	

	while (running) {
		// Get the data (blocking)
		// Will change sin (the address) to be the address of the client.
		// Note: sin passes information in and out of call!
		struct sockaddr_in sinRemote;
		unsigned int sin_len = sizeof(sinRemote);
		
		char messageRx[MSG_MAX_LEN];
		char messageTx[MSG_MAX_LEN];

		// Pass buffer size - 1 for max # bytes so room for the null (string data)
		int bytesRx = recvfrom(socketDescriptor,
			messageRx, MSG_MAX_LEN - 1, 0,
			(struct sockaddr *) &sinRemote, &sin_len);

		// Check for errors (-1)

		// Make it null terminated (so string functions work)
		// - recvfrom given max size - 1, so there is always room for the null
		messageRx[bytesRx] = 0;
		printf("Message received (%d bytes): \n\n'%s'\n", bytesRx, messageRx);
		
        
		handleRequest(messageRx, messageTx, sinRemote, socketDescriptor);
        
		
	}

	// Close
	close(socketDescriptor);
	pthread_cancel(pthread_self());

	return NULL;
}
void* handleRequest(char* messageRx, char* messageTx, struct sockaddr_in sinRemote, int socketDescriptor)
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
void stop_udpThread(void){
	pthread_join(threadUdp, NULL);
}
