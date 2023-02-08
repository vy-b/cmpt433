/*
 * UDP Listening program on port 22110
 * By Brian Fraser, Modified from Linux Programming Unleashed (book)
 *
 * Usage:
 *	On the target, run this program (netListenTest).
 *	On the host:
 *		> netcat -u 192.168.0.171 22110
 *		(Change the IP address to your board)
 *
 *	On the host, type in a number and press enter:
 *		4<ENTER>
 *
 *	On the target, you'll see a debug message:
 *	    Message received (2 bytes):
 *	    '4
 *	    '
 *
 *	On the host, you'll see the message:
 *	    Math: 4 + 1 = 5
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>			// for strncmp()
#include <unistd.h>			// for close()
#include "sampler.h"
#define MSG_MAX_LEN 1024
#define PORT        22110

int main()
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

	while (1) {
		// Get the data (blocking)
		// Will change sin (the address) to be the address of the client.
		// Note: sin passes information in and out of call!
		struct sockaddr_in sinRemote;
		unsigned int sin_len = sizeof(sinRemote);
		char messageRx[MSG_MAX_LEN];

		// Pass buffer size - 1 for max # bytes so room for the null (string data)
		int bytesRx = recvfrom(socketDescriptor,
			messageRx, MSG_MAX_LEN - 1, 0,
			(struct sockaddr *) &sinRemote, &sin_len);

		// Check for errors (-1)

		// Make it null terminated (so string functions work)
		// - recvfrom given max size - 1, so there is always room for the null
		messageRx[bytesRx] = 0;
		printf("Message received (%d bytes): \n\n'%s'\n", bytesRx, messageRx);
		char messageTx[MSG_MAX_LEN];
		
		char prevMsg[MSG_MAX_LEN];
        if (bytesRx==1){
            strcpy(messageRx,prevMsg);
            strcpy(messageTx,"enter pressed");
        }
		// Compose the reply message:
		// (NOTE: watch for buffer overflows!).
		if (strncmp("help",messageRx, strlen("help"))==0){
            strcpy(messageTx, "Accepted command examples:\n");
            strcat(messageTx, "count -- display total number of samples taken.\n");
            strcat(messageTx, "length -- display number of samples in history (both max, and current).\n");
            strcat(messageTx, "history -- display the full sample history being saved.\n");
            strcat(messageTx, "get 10 -- display the 10 most recent history values.\n");
            strcat(messageTx, "dips -- display number of .\n");
            strcat(messageTx, "stop -- cause the server program to end.\n");
            strcat(messageTx, "<enter> -- repeat last command.\n");
        }
        else if (strncmp("count",messageRx, strlen("count"))==0){
            sprintf(messageTx, "%lld\n", Sampler_getNumSamplesTaken());
        }
        else if (strncmp("get",messageRx, strlen("get"))==0){
            /*parse string to get N*/
            strcpy(messageTx,Sampler_get_N(8));
        }
        else if (strncmp("length",messageRx, strlen("length"))==0){
            sprintf(messageTx, "History can hold %d samples.\nCurrently holding %d samples.\n",Sampler_getHistorySize(), Sampler_getNumSamplesInHistory());
        }
        
        
        strcpy(prevMsg,messageRx);
		// Transmit a reply:
		sin_len = sizeof(sinRemote);
		sendto( socketDescriptor,
			messageTx, strlen(messageTx),
			0,
			(struct sockaddr *) &sinRemote, sin_len);

		if (strncmp(messageRx, "stop", strlen("stop")) == 0) {
			printf("Bye lol\n");
			break;
		}
	}

	// Close
	close(socketDescriptor);
	
	return 0;
}