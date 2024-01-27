CFLAGS = -std=c2x -O2 -Wall -Wextra -pedantic
LDFLAGS = -lglfw -lvulkan -ldl -lpthread

main: main.c shaders
	LD_LIBRARY_PATH=/usr/local/lib64/ gcc $(CFLAGS) -o main main.c $(LDFLAGS)

.PHONY: test clean shaders

shaders:
	glslangValidator shader.frag -V
	glslangValidator shader.vert -V

test: main
	./main

clean:
	rm -f main *.spv
