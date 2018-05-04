main : libfractal/libfractal.a
	cc -std=c99 -pthread main.c libfractal/libfractal.a -lSDL -o main

libfractal/libfractal.a :
	cd libfractal/ && $(MAKE)

lib :
	cd libfractal/ && $(MAKE)

tests : split
	./split && rm split

split : tests/testsplit.c
	cc tests/testsplit.c -lcunit -o split

clean :
	rm main && cd libfractal/ && rm libfractal.a *.o
