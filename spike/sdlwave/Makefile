CFLAGS=-ggdb3 $(shell sdl-config --cflags)
LDFLAGS=-ggdb3 $(shell sdl-config --libs) -lSDL_mixer

PROGRAMS=$(basename $(wildcard *.c))

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS) *.o

new: clean all
