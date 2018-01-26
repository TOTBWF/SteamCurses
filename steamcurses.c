#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <curses.h>
#include <menu.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "parser.h"
#include "steamcurses.h"
#include "manifest_generator.h"

void launch_game_cmd(char* cmd) {
  // Fork off the game on a new thread
  pid_t pid;
  if((pid = fork()) < 0) {
    perror("Forking Error!");
    exit(1);
  }
  if(pid == 0) {
    // Pipe stderr -> stdout -> logfile
    dup2(fileno(g_logfile), fileno(stdout));
    dup2(fileno(stdout), fileno(stderr));
    system(cmd);
    exit(0);
  }
}

void launch_native_game(char* appid) {
  char* cmd;
  asprintf(&cmd, "steam -applaunch %s", appid);
  launch_game_cmd(cmd);
  free(cmd);
}

void launch_wine_game(char* appid, char* username, char* password) {
  char* cmd;
  asprintf(&cmd, "wine $WINEPREFIX/drive_c/Program\\ Files/Steam/Steam.exe -silent -login %s %s -applaunch %s", g_username, g_password, appid);
  launch_game_cmd(cmd);
  free(cmd);
}

void print_title(WINDOW* win, char* title) {
  int length = strlen(title);
  int x, y;
  getmaxyx(win, y, x);
  mvwprintw(win, 0, x/2 - length/2, title);
  refresh();
}

void print_help() {
  printf("Usage: ./steamcurses [options]\n");
  printf("  -c --config_path:       use a given config for launch\n");
  printf("  -m --update_manifests:  update the generated manifest files\n");
  printf("	-h --help:		          print this help message and exit\n");
  printf("Config Options:\n");
  printf("  username: the steam username to use. REQUIRED!\n");
  printf("  steam_path: the path to the native steamapps directory.\n");
  printf("  wine_steam_path: the path to the wine steamapps directory.\n");
}

void print_menu(WINDOW* win, MENU* menu, game_t** games) {
    int c;
    // Only call getch once
    while((c = wgetch(win)) != 'q') {
      switch(c) {
        case KEY_DOWN: {
          menu_driver(menu, REQ_DOWN_ITEM);
          break;
        }
        case KEY_UP: {
          menu_driver(menu, REQ_UP_ITEM);
          break;
        }
        case 10: {
          game_t* curr_game = games[menu->curitem->index];
          char* appid = fetch_value(curr_game->key_value_pairs, "appid", curr_game->size); 
          if(curr_game->is_wine) {
            launch_wine_game(appid, g_username, g_password);
          } else {
            launch_native_game(appid);
          }
          break;
        }
      }
    }
}

WINDOW* init_ncurses() {
    // NCurses stuff
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Get the max size of the window, and init windows
    WINDOW* win = newwin(LINES - 3, COLS - 1, 0, 0);
    // q
    // Give the menu window focus
    keypad(win, TRUE);
    return win;
}


ITEM** init_items(game_t** games, int size) {
    // Populate the array of items
    ITEM **items = (ITEM**)calloc(size + 1, sizeof(ITEM*));
    for (int i = 0; i < size; i++) {
      char* name = fetch_value(games[i]->key_value_pairs, "name", games[i]->size);
      if(!games[i]->is_wine) {
        items[i] = new_item(name, NULL);
      } else {
        items[i] = new_item(name, "(Wine)");
      }
      items[i]->index = i;
    }
    items[size] = (ITEM*) NULL;
    return items;
}

MENU* init_menu(ITEM** items, WINDOW* win) {
    MENU* menu = new_menu((ITEM**)items);
    set_menu_format(menu, LINES - 4, 0);
    set_menu_win(menu, win);
    set_menu_sub(menu, derwin(win, LINES - 4, COLS - 2, 1, 0));
    
    // Make things pretty
    mvprintw(LINES - 1, 0, "q to exit");
    char title[] = "SteamCurses";
    print_title(win, title);
    refresh();

    // Post the menu
    post_menu(menu);
    wrefresh(win);
    return menu;
}

void init(char** home, char** steamcurses_dir) {
  // The current home dir, used to generate default locations 
  *home = getenv("HOME");
  if(home == NULL) {
    printf("Error! $HOME is not valid or null\n");
    exit(1);
  }
  
  // Set the directory that we use to store files
  asprintf(steamcurses_dir, "%s/.steamcurses/", *home);

  // Make sure that directory exists, and if it doesnt, create it
  struct stat st;
  if(stat(*steamcurses_dir, &st) == -1) {
    // Create the dir if it doesnt exits
    mkdir(*steamcurses_dir, 0755);
  }

  // Set the log path, and init logging
  char* log_path;
  asprintf(&log_path, "%s/steamcurses.log", *steamcurses_dir);
  g_logfile = fopen(log_path, "a");
  free(log_path);
}

int main(int argc, char* argv[]) {
  char* steam_path = NULL;
  char* wine_steam_path = NULL;
  char* steamcurses_dir = NULL;
  char* config_path = NULL;
  char* home = NULL;
  g_username = NULL;
  g_password = NULL;
  int update_manifests = 0;
  FILE* config_file = NULL;

  // Perform basic init
  init(&home, &steamcurses_dir);

  // Deal with user input
  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--update_manifests") == 0) {
      update_manifests = 1;
    } else if(strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config_path") == 0) {
      config_path = argv[++i];
    } else {
      print_help();
      exit(0);
    }
  }

  // If we arent provided a config path, then look in the default location
  if(config_path == NULL) {
    asprintf(&config_path, "%s/steamcurses.conf", steamcurses_dir);
  }

  // Check to see if the config file actually exists
  if(access(config_path, F_OK) == -1) {
    // If the file does not exist, create the file and fill out fields
    fprintf(g_logfile, "Creating config file %s...\n", config_path);
    config_file = fopen(config_path, "a+");
    // Construct the opening skeleton of the file
    fprintf(config_file, "\"config\"\n{\n");
    char buf[1024];
    printf(config_path, "Config File created at path %s\n");
    printf("Enter your steam username: ");
    scanf("%s", buf);
    fprintf(config_file, " \"username\" \"%s\"\n", buf);
    // Fill out the rest of the file
    fprintf(config_file, "}\n");
    // Let the user know about the other options
    printf("You can use this config file to specify the path to your steamapp directorys\n");
    printf("Just use the steam_path and wine_steam_path options!\n");
  } else {
    // The file already exists, so just open it for reading
    config_file = fopen(config_path, "r");
  }

  // Read the values from the config
  int config_size = 0;
  kvp_t** config_kvp = parse_manifest(config_path, &config_size);
  g_username = fetch_value(config_kvp, "username", config_size);
  steam_path = fetch_value(config_kvp, "steam_path", config_size);
  wine_steam_path = fetch_value(config_kvp, "wine_steam_path", config_size);

  // If we don't get provided input, make an educated guess or throw errors
  if(steam_path == NULL) {
    asprintf(&steam_path, "%s/.steam/steam/steamapps/", home);
  }

  if(g_username == NULL) {
    printf("Error, no username provided!\n");
    print_help();
    exit(1);
  }

  g_password = (char*) getpass("Password: ");
 
  pid_t pid;

  // Fork off the steam process
  if((pid=fork()) < 0) {
    perror("Forking Error!");
    exit(1);
  }

  if(pid == 0) {
    // Figure out path to the executable, used for loading the injector

    char* bin_path;
    asprintf(&bin_path, "%s/.steam/bin", home);
    fprintf(g_logfile, "%s\n", bin_path);

    // Pipe stderr -> stdout -> logfile
    dup2(fileno(g_logfile), fileno(stdout));
    dup2(fileno(stdout), fileno(stderr));
    // Set up launch command
    char* cmd;
    asprintf(&cmd, "LD_PRELOAD=/usr/lib32/steam_injector.so LD_LIBRARY_PATH=%s %s/steam -silent -login %s %s", bin_path, bin_path, g_username, g_password);
    system(cmd);
    exit(0);
  } else {
    // Parent Process
    // Parse the manifests to get a list of installed games
    int size = 0;
    int capacity = 100;
    game_t** games = (game_t**) malloc(capacity * sizeof(game_t*));
    parse_manifests(&size, &capacity, &games, steam_path, 0);
    if(wine_steam_path) {
      parse_manifests(&size, &capacity, &games, wine_steam_path, 1);
    }
    sort_games(games, size);
    // We use these generated manifests to view interesting info about the games
    if(update_manifests) {
      // Create a folder specifically for the generated manifests
      char* manifest_path;
      asprintf(&manifest_path, "%s/manifests/", steamcurses_dir);
      printf("Generating manifests, this can take a while if you have a lot of games...\n");
      generate_manifests(manifest_path, games, size); 
      printf("Manifest generation complete\n");
      free(manifest_path);
    }

    WINDOW* win;
    ITEM** my_items;
    MENU* my_menu;

    // Set up ncurses
    win = init_ncurses();

    // Populate the array of items
    my_items = init_items(games, size);
  
    // Set up the menu
    my_menu = init_menu(my_items, win); 
  
    // Display the menu
    print_menu(win, my_menu, games);

    // Perform clean up
    unpost_menu(my_menu);
    free_menu(my_menu);
    for(int i = 0; i < size; i++) {
      for(int j = 0; j < games[i]->size; j++) {
        free_kvp(games[i]->key_value_pairs[j]);
      }
      free(games[i]->key_value_pairs);
      free(games[i]);
      free_item(my_items[i]);
    }
    free(config_path);
    free_kvp(*config_kvp);
    free(config_kvp);
    fclose(g_logfile);
    // Free Strings that were allocated
    free(steam_path);
    if(wine_steam_path != NULL) {
      free(wine_steam_path);
    }

    free(games);
    free(my_items);
    free(steamcurses_dir);
    games = NULL;
    my_items = NULL;
    steam_path = NULL;
    endwin();
    system("steam -shutdown 1>/dev/null 2>/dev/null");
    return 0;
  }
}

