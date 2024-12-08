CC = gcc
CFLAGS = -std=c99 -O3 -Wall
INCL = -I../include
LIBS = -L../lib -lprand
SRCS = multistream.c
EXEC = multistream

all: $(EXEC)

$(EXEC): $(SRCS)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRCS) $(LIBS) $(INCL)

clean:
	rm $(EXEC)
