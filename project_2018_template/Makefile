main : libfractal/libfractal.a
	cc -std=c99 -pthread main.c libfractal/libfractal.a -lSDL -o main

libfractal/libfractal.a : 
	cd libfractal/ && $(MAKE)

lib :
	cd libfractal/ && $(MAKE)

clean :
	rm main && cd libfractal/ && rm libfractal.a *.o 
