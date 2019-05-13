#CC:=arm-linux-gnueabihf-
CC:=

ifeq ($(CC),arm-linux-gnueabihf-)
LIBS = -L./lib/alsa/lib
INCS = -I./lib/alsa/include
else
LIBS =
INCS =
endif

target:
	$(CC)gcc -Wall -o wmix wmix.c wav.c $(LIBS) $(INCS) -lasound -lm -ldl -lpthread
	$(CC)gcc -Wall -o test main.c wmix_user.c -lpthread

clean:
	@rm -rf test wmix
