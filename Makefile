CC=gcc
SRC=src
BUILD=build
DEPS=$(SRC)/headers
OBJS=$(SRC)/main/App.c $(SRC)/main/chart.c $(SRC)/main/get_ip.c $(SRC)/main/sockserv.c $(SRC)/main/task.c

App:	$(OBJS)
	$(CC) -o $@ $^ -I $(DEPS)

