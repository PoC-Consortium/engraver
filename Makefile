CC=gcc
CFLAGS=-Wall -m64 -O3 -mtune=native -D_FILE_OFFSET_BITS=64

include osdetect.mk

ifeq ($(OS),Mac)
SHABAL=shabal64-darwin.o
else
SHABAL=shabal64.o
endif

all:		plot64

dist:		clean all
		mkdir bin
		mv plot64 bin
		tar -czf cg_obup.tgz *

plot64:	        plot.c $(SHABAL) helper64.o mshabal_sse4.o mshabal256_avx2.o 
		$(CC) $(CFLAGS) -o plot64 plot.c $(SHABAL) helper64.o mshabal_sse4.o mshabal256_avx2.o -lpthread -std=gnu99

helper64.o:	helper.c
		$(CC) $(CFLAGS) -c -o helper64.o helper.c		

shabal64.o:	shabal64.s
		$(CC) $(CFLAGS) -c -o shabal64.o shabal64.s

mshabal_sse4.o: mshabal_sse4.c
		$(CC) $(CFLAGS) -c -o mshabal_sse4.o mshabal_sse4.c

mshabal256_avx2.o: mshabal256_avx2.c
		$(CC) $(CFLAGS) -mavx2 -c -o mshabal256_avx2.o mshabal256_avx2.c

shabal64-darwin.o:	shabal64-darwin.s
		gcc -Wall -m64 -c -o $@ $^

test:		plot64
		./test.pl

clean:
		rm -f mshabal_sse4.o mshabal256_avx2.o shabal64.o shabal64-darwin.o helper64.o plot64 helper64.o cg_obup.tgz bin/*
