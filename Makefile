#CC:=arm-linux-gnueabihf-
CC:=

INCS = -I./lib/alsa/include -I./lib/mad/include

ifeq ($(CC),arm-linux-gnueabihf-)
LIBS = -L./lib/alsa/lib
else
LIBS =
endif

target:
	$(CC)gcc -Wall -o wmix wmix.c wav.c $(LIBS) $(INCS) -lasound -lm -ldl -lpthread
	$(CC)gcc -Wall -o test main.c wmix_user.c -lpthread $(LIBS) $(INCS)
	$(CC)gcc -Wall -o mad minimad.c id3.c wmix_user.c $(LIBS) $(INCS) -lmad

clean:
	@rm -rf test wmix
