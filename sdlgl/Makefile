CFLAGS=`pkg-config --cflags sdl2 glew`
LDFLAGS=`pkg-config --libs sdl2 glew`

default: main

main: main.cpp
	${CXX} -o $@ $< ${CFLAGS} ${LDFLAGS}

run: main
	./main


clean:
	rm -f main
