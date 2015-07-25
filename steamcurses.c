#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/types.h>
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


// Fork of the game in a new process
void launch_game(char* appid, int is_wine) {
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

    char* cmd;
    if(is_wine) {
      // Wine Steam will prompt for password
      asprintf(&cmd, "wine $WINEPREFIX/drive_c/Program\\ Files/Steam/Steam.exe -silent -login %s %s -applaunch %s", g_username, g_password, appid);
    } else {
      asprintf(&cmd, "steam -applaunch %s", appid);
    }
    system(cmd);
    exit(0);
  }
}

void print_title(WINDOW* win, char* title) {
  int length = strlen(title);
  int x, y;
  getmaxyx(win, y, x);
  mvwprintw(win, 0, x/2 - length/2, title);
  refresh();
}

void print_help() {
  printf("Usage: ./steamcurses -u <username> [options]\n");
  printf("	-u --username:		your Steam username\n");
  printf("	-p --steam_path:	the path to your steamapps directory\n");
  printf("	-w --wine_steam_path:	the path to your wine steamapps directory\n");
  printf("	-h --help:		print this help message and exit\n");
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
          launch_game(appid, curr_game->is_wine);
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




int main(int argc, char* argv[]) {

  // The current home dir, used to generate default locations 
  char* home = getenv("HOME");
  if(home == NULL) {
    printf("Error! $HOME is not valid or null\n");
    exit(1);
  }

  char* log_path;
  asprintf(&log_path, "%s/.steam/steamcurses.log", home);
  g_logfile = fopen(log_path, "a");


  char* steam_path = NULL;
  char* wine_steam_path = NULL;
  char* steamcurses_dir = NULL;
  g_username = NULL;
  g_password = NULL;
  
  // Deal with user input
  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--username") == 0) {
        g_username = argv[++i];
    } else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--steam_path") == 0) {
      steam_path = argv[++i];
    } else if(strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--wine_steam_path") == 0) {
      wine_steam_path = argv[++i];
    } else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--directory") == 0) {
      steamcurses_dir = argv[++i];
    } else {
      print_help();
      exit(0);
    }
  }

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
    asprintf(&cmd, "LD_PRELOAD=/usr/lib32/steam_injector.so LD_LIBRARY_PATH=%s %s/steam -login %s %s", bin_path, bin_path, g_username, g_password);
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
    generate_manifests(steamcurses_dir, games, size); 

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
        free(games[i]->key_value_pairs[j]->key);
        free(games[i]->key_value_pairs[j]->value);
        free(games[i]->key_value_pairs[j]);
      }
      free(games[i]->key_value_pairs);
      free(games[i]);
      free_item(my_items[i]);
    }
    fclose(g_logfile);
    free(games);
    free(my_items);
    games = NULL;
    my_items = NULL;
    log_path = NULL;
    steam_path = NULL;
    endwin();
    system("steam -shutdown 1>/dev/null 2>/dev/null");
    return 0;
  }
}

