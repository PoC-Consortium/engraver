CC=gcc
CFLAGS=-Wall -m64 -O2 -mtune=skylake -D_FILE_OFFSET_BITS=64

all:		plot64

dist:		clean all
		mkdir bin
		mv plot64 bin
		tar -czf cg_obup.tgz *

plot64:	        plot.c shabal64.o helper64.o mshabal_sse4.o mshabal256_avx2.o 
		$(CC) $(CFLAGS) -o plot64 plot.c shabal64.o helper64.o mshabal_sse4.o mshabal256_avx2.o -lpthread -std=gnu99

helper64.o:	helper.c
		$(CC) $(CFLAGS) -c -o helper64.o helper.c		

shabal64.o:	shabal64.s
		$(CC) $(CFLAGS) -c -o shabal64.o shabal64.s

mshabal_sse4.o: mshabal_sse4.c
		$(CC) $(CFLAGS) -c -o mshabal_sse4.o mshabal_sse4.c

mshabal256_avx2.o: mshabal256_avx2.c
		$(CC) $(CFLAGS) -mavx2 -c -o mshabal256_avx2.o mshabal256_avx2.c

clean:
		rm -f mshabal_sse4.o mshabal256_avx2.o shabal64.o helper64.o plot64 helper64.o cg_obup.tgz bin/*
