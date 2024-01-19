CFLAGS = -std=c2x -O2 -Wall -Wextra -pedantic
LDFLAGS = -lglfw -lvulkan -ldl -lpthread

main: main.c
	LD_LIBRARY_PATH=/usr/local/lib64/ gcc $(CFLAGS) -o main main.c $(LDFLAGS)

.PHONY: test clean

test: main
	./main

clean:
	rm -f main
