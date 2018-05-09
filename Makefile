all: injector steamcurses

install: injector steamcurses
	install -m 0755 target/release/steamcurses /usr/bin
	install -m 0755 target/injector/steam_injector.so /usr/lib32

injector: 
	mkdir -p target/injector
	gcc -m32 -shared -fPIC src/injector/steam_injector.c -o target/injector/steam_injector.so -ldl -lX11

steamcurses:
	rustup install nightly
	rustup run nightly cargo build --release