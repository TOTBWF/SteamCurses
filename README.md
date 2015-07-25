# SteamCurses
A basic NCurses client for Steam on Linux

### Features:
- Supports both wine and native games at the same time

### Building:
**Dependencies:**
* Steam
* ncurses-dev package


For Ubuntu flavors:
  * ```sudo apt-get install steam```

  * ```sudo apt-get install libncurses5-dev```

Arch:
  * ```sudo pacman -S ncurses```

  * ```sudo pacman -S steam```

  * It can also be found under the AUR as steamcurses-git

Simply run ```make install``` to build and install.

### Usage:
```
 ./steamcurses -u <username> [options]
   -u --username:         your Steam username
   -p --steam_path:       the path to your steamapps directory
   -w --wine_steam_path:  the path to your wine steamapps directory
   -m --update_manifests: update the generated manifests
   -h --help:             print help message
```
