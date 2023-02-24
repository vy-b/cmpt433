TARGETS= xbox

OUTFILE = xbox
OUTDIR = $(HOME)/cmpt433/public/myApps

CROSS_COMPILE = arm-linux-gnueabihf-
CC_C = $(CROSS_COMPILE)gcc
DEPS = 
CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror -Wshadow

all: $(TARGETS)

$(TARGETS):
	$(CC_C) $(CFLAGS) $@.c $(DEPS) -o $(OUTDIR)/$@ -lusb-1.0

clean:
	rm $(OUTDIR)/$(OUTFILE)
