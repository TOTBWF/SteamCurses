injector: steam_injector.c
	gcc -shared -fPIC  steam_injector.c -o steam_injector.so -ldl

debug: parser.c parser.h steamcurses.c
	gcc -g -Wall -Wextra -std=c11 -o steamcurses steamcurses.c parser.c -lmenu -lncurses

final: parser.c parser.h steamcurses.c
	gcc -std=c11 -s -o steamcurses steamcurses.c parser.c -lmenu -lncurses


