#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include "shutdown_helper.h"
#include <pthread.h>


void free_mutexes(void *mutex)
{
	pthread_mutex_t* pmutex;
	pmutex = (pthread_mutex_t*) mutex;
	pthread_mutex_destroy(pmutex);
}

void free_malloc(void* item)
{
	if(item)
		free(item);
}
