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

gentest : gen main
	echo>F1 && echo>F2 && ./gen 4 50 15 F1 && ./gen 5 50 15 F2 && ./main F1 F2

gen : tests/fractGen.c
	cc -std=c99 tests/fractGen.c -o gen

clean :
	rm main gen F* && cd libfractal/ && rm libfractal.a *.o
