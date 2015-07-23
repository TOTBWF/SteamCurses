# SteamCurses
A basic NCurses client for Steam on Linux

### Features:
- Supports both wine and native games at the same time

### Building:
**Dependencies:** Steam, ncurses-dev packages.

Ubuntu users may have to use the libncurses5-dev package.

Simply run ```make``` to build.

### Usage:
```
 ./steamcurses -u <username> [options]
   -u --username:         your Steam username
   -p --steam_path:       the path to your steamapps directory
   -w --wine_steam_path:  the path to your wine steamapps directory
   -h --help:             print help message
```
