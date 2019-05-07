CC=arm-linux-gnueabihf-
#CC=

LIBS = -L./lib/alsa/lib
INCS = -I./lib/alsa/include

target:
	$(CC)gcc -Wall -o m main.c play.c wav.c -lasound -lm -ldl $(LIBS) $(INCS)

clean:
	@rm -rf m 
