CFLAGS += $(shell pkg-config sdl3 --cflags)
LDFLAGS += -lm $(shell pkg-config sdl3 --libs)
SHADERS := $(wildcard shaders/*.hlsl)
SPV_FILES := $(SHADERS:%=%.spv)

default: render

render: render.c
	${CC} -o $@.bin $< ${CFLAGS} ${LDFLAGS}

%-run: %
	./$<.bin

gl: gl.c
	${CC} -o $@.bin $< ${CFLAGS} -lGL ${LDFLAGS}

gpu: gpu.c shaders
	${CC} -o $@.bin $< ${CFLAGS} ${LDFLAGS}

.PHONY: shaders
shaders:
	shadercross shaders/2d.vert.hlsl -o shaders/2d.vert.spv
	shadercross shaders/2d.frag.hlsl -o shaders/2d.frag.spv

clean:
	rm -f *.bin

