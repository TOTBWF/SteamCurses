all:
	gcc -std=c11 -D_GNU_SOURCE -lncurses -lmenu -g -o steamcurses steamcurses.c parser.c manifest_generator.c
