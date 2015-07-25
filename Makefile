all: injector final

install: injector final 
	install -m 0755 steamcurses /usr/bin
	install -m 0755 steam_injector.so /usr/lib32

injector: steam_injector.c
	gcc -m32 -shared -fPIC  steam_injector.c -o steam_injector.so -ldl -lX11

debug: parser.c parser.h steamcurses.c
	gcc -g -Wall -Wextra -std=c11 -o steamcurses steamcurses.c parser.c -lmenu -lncurses

final: parser.c parser.h steamcurses.c
	gcc -std=c11 -s -o steamcurses steamcurses.c parser.c -lmenu -lncurses


