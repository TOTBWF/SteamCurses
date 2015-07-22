
all: parser.c parser.h steamcurses.c
	gcc -g -Wall -std=c11 -o steamcurses steamcurses.c parser.c -lmenu -lncurses
