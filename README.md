# SteamCurses
A Basic NCurses Client for Linux Steam

### Features:
- Supports both wine and native games at the same time

### Building:
**Dependencies:**

* Steam
  * ```sudo apt-get install steam```
* ncurses-dev package
  * ```sudo apt-get install libncurses5-dev```

Ubuntu users may have to use the libncurses5-dev package.

Simply run ```make``` to build.

### Usage:
```
 -u --username: Your Steam username
 -p --steam_path: The path to your steamapps directory
 -w --wine_steam_path: The path to your wine steamapps directory
 -h --help: Print help message
```
