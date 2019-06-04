# cross:=arm-linux-gnueabihf

host:=
cc:=gcc

ifdef cross
	host=$(cross)
	cc=$(cross)-gcc
endif

ROOT=$(shell pwd)

obj-wmix+= ./src/wmix.c ./src/wmix.h \
		./src/wav.c ./src/wav.h \
		./src/id3.c ./src/id3.h

obj-user+= ./src/wmix_user.c \
		./src/wmix_user.h \
		./src/wav.c ./src/wav.h \
		./src/main.c

target:
	@$(cc) -Wall -o wmix $(obj-wmix) -L./libs/lib -I./libs/include -lpthread -lasound -lm -ldl -lmad
	@$(cc) -Wall -o test $(obj-user) -lpthread

all: dpkg-alsa alsa dpkg-mad mad
	@echo "---------- all complete !! ----------"

dpkg-mad:
	tar -xzf ./libmad-0.15.1b.tar.gz -C ./libs

mad:
	@cd $(ROOT)/libs/libmad-0.15.1b && \
	./configure --prefix=$(ROOT)/libs --host=$(host) --enable-speed && \
	sed -i 's/-fforce-mem//g' ./Makefile && \
	make -j4 && make install && \
	cd -

dpkg-alsa:
	tar -xjf ./alsa-lib-1.1.9.tar.bz2 -C ./libs

alsa:
	@cd $(ROOT)/libs/alsa-lib-1.1.9 && \
	./configure --prefix=$(ROOT)/libs --host=$(host) && \
	make -j4 && make install && \
	cd -

clean:
	@rm -rf ./test ./wmix

cleanAll :
	@rm -rf ./test ./wmix ./libs/* -rf
