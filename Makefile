#CC=gcc
CC=gcc-7
CFLAGS=-Wall -m64 -O3 -march=native -fomit-frame-pointer -D_FILE_OFFSET_BITS=64

all:		plot64

dist:		clean all
		mkdir bin
		mv plot64 bin
		tar -czf cg_obup.tgz *

plot64:	        plot.c shabal64.o helper64.o mshabal_sse4.o mshabal256_avx2.o apex_memmove.o
		$(CC) $(CFLAGS) -o plot64 plot.c shabal64.o helper64.o mshabal_sse4.o mshabal256_avx2.o apex_memmove.o -lpthread -std=gnu99

helper64.o:	helper.c
		$(CC) $(CFLAGS) -c -o helper64.o helper.c		

shabal64.o:	shabal64.s
		$(CC) $(CFLAGS) -c -o shabal64.o shabal64.s

mshabal_sse4.o: mshabal_sse4.c
		$(CC) $(CFLAGS) -c -o mshabal_sse4.o mshabal_sse4.c

mshabal256_avx2.o: mshabal256_avx2.c apex_memmove.o
		$(CC) $(CFLAGS) -mavx2 -c -o mshabal256_avx2.o apex_memmove.o mshabal256_avx2.c

apex_memmove.o: apex_memmove.c
		$(CC) $(CFLAGS) -mavx2 -c -o apex_memmove.o apex_memmove.c

test:		plot64
		./test.pl

list:
		$(CC) $(CFLAGS) shabal64.o helper64.o mshabal_sse4.o mshabal256_avx2.o apex_memmove.o -lpthread -std=gnu99 -Wa,-adhln -g plot.c > plot.s

clean:
		rm -f mshabal_sse4.o mshabal256_avx2.o shabal64.o helper64.o plot64 helper64.o apex_memmove.o cg_obup.tgz bin/*
