CC = gcc
CFLAGS = -Wall -Wextra -Werror -g

all: posch-quoted

.PHONY: clean
clean:
	rm -f posch-quoted

