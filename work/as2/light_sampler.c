/*
light_sampler.c
executes a terminal output thread and udp thread
*/

#include <stdio.h>
#include "sampler.h"


int main(int argc, char* argv[])
{
    printf("Starting sample data...\n");
    Sampler_startSampling();
    Sampler_stopSampling();
}



