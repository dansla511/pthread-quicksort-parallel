CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -Wshadow -Wformat=2 -Wunused-parameter -fsanitize=address,undefined -g -lpthread
MKDIR = mkdir -p

all: quick_sort

quick_sort:
	${MKDIR} ./bin/quicksort
	${CC} ${CFLAGS} ./quicksort/parallel.c -o ./bin/quicksort/parallel
	${CC} ${CFLAGS} ./quicksort/sequential.c -o ./bin/quicksort/sequential
	${CC} ${CFLAGS} ./quicksort/threadpool.c -o ./bin/quicksort/threadpool

clean:
	rm -rf bin