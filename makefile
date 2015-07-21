all:
	gcc -std=c99 -lncurses -lmenu -g -o steamcurses steamcurses.c parser.c 
