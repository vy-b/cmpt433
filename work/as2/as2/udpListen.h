#ifndef _UDP_H_
#define _UDP_H_

#include <pthread.h>

extern pthread_cond_t okToShutdown;
extern pthread_mutex_t shutdownMutex;
void start_udpThread(void);
void stop_udpThread(void);

#endif