CFLAGS=-ggdb3 $(shell sdl-config --cflags)
LDFLAGS=-ggdb4 $(shell sdl-config --libs) -lSDL_mixer


all: sdl_test

sdl_test.o: sdl_test.c
	gcc $(CFLAGS) -c sdl_test.c 

sdl_test: sdl_test.o
	gcc  $(LDFLAGS) -o sdl_test sdl_test.o

clean::
	rm -f *.o sdl_test *~
