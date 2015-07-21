all:
	gcc -std=c99 -lncurses -lmenu -o steamcurses steamcurses.c parser.c 
