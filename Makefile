CC=gcc
SRC=src
BUILD=build

App:	$(SRC)/main/App.c
	$(CC) -o $@ $^

