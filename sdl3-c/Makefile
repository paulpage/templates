CFLAGS=-g `pkg-config --cflags sdl3`
LDFLAGS=`pkg-config --cflags --libs sdl3`

default: main

main: main.c
	${CC} -o $@ $< ${CFLAGS} ${LDFLAGS}

run: main
	./main

clean:
	rm -f main

