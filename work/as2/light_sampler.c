/*
light_sampler.c
executes a terminal output thread and udp thread
*/

#include <stdio.h>
#include "sampler.h"
#include "periodTimer.h"
#include "udpListen.h"
#include "led_display.h"
int main(int argc, char* argv[])
{
    printf("Starting sample data...\n");
    Period_init();
    Sampler_startSampling();
    start_ledThread();
    start_udpThread();
    Sampler_stopSampling();
    stop_udpThread();
    stop_ledThread();
}



