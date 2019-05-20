#CC:=arm-linux-gnueabihf-
CC:=

INCS = -I./lib/alsa/include -I./lib/mad/include

ifeq ($(CC),arm-linux-gnueabihf-)
LIBS = -L./lib/alsa/lib
else
LIBS =
endif

target:
	$(CC)gcc -Wall -o wmix wmix.c wav.c id3.c $(LIBS) $(INCS) -lpthread -lasound -lm -ldl -lmad
	$(CC)gcc -Wall -o test main.c wmix_user.c $(LIBS) $(INCS) -lpthread

clean:
	@rm -rf test wmix
