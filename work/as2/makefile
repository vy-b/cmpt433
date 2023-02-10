TARGETS= light_sampler 

OUTFILE = light_sampler 
OUTDIR = $(HOME)/cmpt433/public/myApps

CROSS_COMPILE = arm-linux-gnueabihf-
CC_C = $(CROSS_COMPILE)gcc
DEPS = sampler.c periodTimer.c udpListen.c led_display.c
CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L  -Wshadow

all: $(TARGETS)

$(TARGETS):
	$(CC_C) $(CFLAGS) $@.c $(DEPS) -o $(OUTDIR)/$@ -lpthread

clean:
	rm $(OUTDIR)/$(OUTFILE)

