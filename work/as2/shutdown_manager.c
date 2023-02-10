#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include "shutdown_manager.h"

void ShutdownManager_waitForShutdown(pthread_cond_t *pOkToShutdown, pthread_mutex_t *pShutdownMutex)
{
	pthread_mutex_lock(pShutdownMutex);
	{
		pthread_cond_wait(pOkToShutdown, pShutdownMutex);
	}
    pthread_mutex_unlock(pShutdownMutex);
}

void ShutdownManager_triggerShutdown(pthread_cond_t *pOkToShutdown)
{
	pthread_cond_signal(pOkToShutdown);
}

int ShutdownManager_isShuttingDown(pthread_t thread)
{
	return pthread_cancel(thread);
}

void free_mutexes(void *mutex)
{
	pthread_mutex_t* pmutex;
	pmutex = (pthread_mutex_t*) mutex;
	pthread_mutex_destroy(pmutex);
}


void free_cond(void *cond)
{
	pthread_cond_t* pcond;
	pcond = (pthread_cond_t*) cond;
	pthread_cond_destroy(pcond);
}

void free_malloc(void* item)
{
	if(item)
		free(item);
}
