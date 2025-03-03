CC=gcc
CFLAGS=-Wall -Wextra -Werror

all: clean build

default: build

build: server.cpp client.cpp
	gcc -Wall -Wextra -o server server.cpp -lm -lstdc++
	gcc -Wall -Wextra -o client client.cpp -lm -lstdc++

clean:
	rm -f server client output.txt project2.zip

zip: 
	zip project2.zip server.cpp client.cpp utils.h Makefile README report.txt
