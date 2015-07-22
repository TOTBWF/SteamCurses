
CFLAGS=-g -Wall -std=c11

all:
	gcc -D_GNU_SOURCE -o steamcurses steamcurses.c parser.c -lmenu -lncurses
