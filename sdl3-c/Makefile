CFLAGS += $(shell pkg-config sdl3 --cflags)
LDFLAGS += $(shell pkg-config sdl3 --libs)

default: main

main: main.c
	${CC} -o $@ $< ${CFLAGS} ${LDFLAGS}

run: main
	./main

clean:
	rm -f main

