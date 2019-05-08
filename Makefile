#CC=arm-linux-gnueabihf-
CC=

ifeq ($(CC),arm-linux-gnueabihf-)
LIBS = -L./lib/alsa/lib
INCS = -I./lib/alsa/include
else
LIBS =
INCS =
endif

target:
	$(CC)gcc -Wall -o m main.c play.c wav.c $(LIBS) $(INCS) -lasound -lm -ldl -lpthread

clean:
	@rm -rf m 
