CFLAGS=-g `pkg-config --cflags sdl2`
LDFLAGS=`pkg-config --cflags --libs sdl2`

default: main

main: main.c
	${CC} -o $@ $< ${CFLAGS} ${LDFLAGS}

run: main
	./main

clean:
	rm -f main

