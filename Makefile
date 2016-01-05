all: threads_test

threads_test: threads_test.c threads.c threads.h
	gcc -o threads_test threads_test.c threads.c \
	-W -Wall -Wconversion -Wextra -std=c99 -pedantic \
	-D_XOPEN_SOURCE=600 -g -lpthread -lrt

