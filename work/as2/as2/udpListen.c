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
#include "sampler.h"
#include "shutdown_helper.h"
#define MSG_MAX_LEN 1024
#define PORT        12345
static pthread_t threadUdp;
static int running = 1;
static void sendNumList(double* N_list, int n, char* messageTx, struct sockaddr_in sinRemote, int socketDescriptor);
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

	// Check for errors (-1)
	int enterPressed = 0;
	while (running) {
		// Get the data (blocking)
		// Will change sin (the address) to be the address of the client.
		// Note: sin passes information in and out of call!
		struct sockaddr_in sinRemote;
		unsigned int sin_len = sizeof(sinRemote);
		
		char messageRx[MSG_MAX_LEN];
		char messageTx[MSG_MAX_LEN];

		if (enterPressed==1){
			enterPressed = 0;
			continue;
		}
		// Pass buffer size - 1 for max # bytes so room for the null (string data)
		int bytesRx = recvfrom(socketDescriptor,
			messageRx, MSG_MAX_LEN - 1, 0,
			(struct sockaddr *) &sinRemote, &sin_len);

		// Check for errors (-1)

		// Make it null terminated (so string functions work)
		// - recvfrom given max size - 1, so there is always room for the null
		messageRx[bytesRx] = 0;
		printf("Message received (%d bytes): \n\n'%s'\n", bytesRx, messageRx);
		char prevMsg[MSG_MAX_LEN];
		
        if (bytesRx==1){
			handleRequest(prevMsg, messageTx, sinRemote, socketDescriptor);
			enterPressed = 1;
			continue;
        }
		else {
			strcpy(prevMsg, messageRx);
		}
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
	if (strncmp("help",messageRx, strlen("help"))==0){
		strcpy(messageTx, "Accepted command examples:\n");
		strcat(messageTx, "count -- display total number of samples taken.\n");
		strcat(messageTx, "length -- display number of samples in history (both max, and current).\n");
		strcat(messageTx, "history -- display the full sample history being saved.\n");
		strcat(messageTx, "get N -- display the N most recent history values.\n");
		strcat(messageTx, "dips -- display number of dips.\n");
		strcat(messageTx, "stop -- cause the server program to end.\n");
		strcat(messageTx, "<enter> -- repeat last command.\n");
	}
	else if (strncmp("count",messageRx, strlen("count"))==0){
		sprintf(messageTx, "Number of samples taken = %lld\n", Sampler_getNumSamplesTaken());
	}
	else if (strncmp("get",messageRx, strlen("get"))==0){
		int n;
		int valid = sscanf(messageRx + 4, "%d",&n);
		if (valid){
			int maxHistory = Sampler_getNumSamplesInHistory();
			if (maxHistory<n){
				sprintf(messageTx, "Please enter a number between 1 and %d\n",maxHistory);
				// Transmit a reply:
				unsigned int sin_len = sizeof(sinRemote);
				sendto( socketDescriptor,
					messageTx, strlen(messageTx),
					0,
					(struct sockaddr *) &sinRemote, sin_len);
				return NULL;
			}
			double* N_list = Sampler_get_N(n);
			sendNumList(N_list,n,messageTx,sinRemote, socketDescriptor);
			free(N_list);
			return NULL;
		}
		else{
			strcpy(messageTx, "N is not a valid number");
		}
	}
	else if (strncmp("history",messageRx, strlen("history"))==0){
		int length = 0;
		double* hist = Sampler_getHistory(&length);
		sendNumList(hist,length,messageTx,sinRemote, socketDescriptor);
		
		return NULL;
	}
	else if (strncmp("length",messageRx, strlen("length"))==0){
		sprintf(messageTx, "History can hold %d samples.\nCurrently holding %d samples.\n",Sampler_getHistorySize(), Sampler_getNumSamplesInHistory());
	}
	else if (strncmp("dips",messageRx, strlen("dips"))==0){
		sprintf(messageTx, "Dips: %d\n",Sampler_getNumDips());
	}
	else if (strncmp("stop",messageRx, strlen("dips"))==0){
		Sampler_shutdownRoutine();
		strcpy(messageTx, "Server has been shut down!");
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
void stop_udpThread(void){
	pthread_join(threadUdp, NULL);
}
static void sendNumList(double* N_list, int n, char* messageTx, struct sockaddr_in sinRemote, int socketDescriptor)
{
	char temp[317];
	int j = 0;
	
	while (j<n){
		sprintf(temp,"%5.3f  ",N_list[j]);
		strcpy(messageTx,temp);
		j++;
		for (int i = j; i < j+200 && i<n; i++){
			if (i%20==0){
				strcat(messageTx,"\n");
			}
			sprintf(temp,"%5.3f, ",N_list[i]);
			strcat(messageTx,temp);
		}
		strcat(messageTx,"\n");
		// Transmit a reply:
		unsigned int sin_len = sizeof(sinRemote);
		sendto( socketDescriptor,
			messageTx, strlen(messageTx),
			0,
			(struct sockaddr *) &sinRemote, sin_len);
		
		j+=200;
	}
}