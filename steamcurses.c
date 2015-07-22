#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <curses.h>
#include <menu.h>
#include <unistd.h>
#include <string.h>
#include "parser.h"

int launch_game(char* appid, int is_wine) {
    char* cmd = NULL;
    if(is_wine) {
      asprintf(&cmd, "wine $WINEPREFIX/drive_c/Program\\ Files/Steam/Steam.exe -applaunch -silent %s 1>> ~/.steam/steamcurses.log 2>> ~/.steam/steamcurses.log", appid);
    } else {
      asprintf(&cmd, " 1>> ~/.steam/steamcurses.log 2>> ~/.steam/steamcurses.log", appid);
    }
    int status = system(cmd);
    free(cmd);
    cmd = NULL;
    return status;
}

void print_title(WINDOW* win, char* title) {
  int length = strlen(title);
  int x, y;
  getmaxyx(win, y, x);
  mvwprintw(win, 0, x/2 - length/2, title);
  refresh();
}

void print_help() {
  printf("Usage:\n");
  printf("-u --username: Your Steam username\n");
  printf("-p --steam_path: The path tho your steamapps directory\n");
  printf("-w --wine_steam_path: The path to your wine steamapps directory\n");
  printf("-h --help: Print this help page\n");
}

int print_menu(WINDOW* win, MENU* menu, game_t** games) {
    int c;
    int status = 0;
    // Only call getch once
    while((c = wgetch(win)) != 'q') {
      switch(c) {
        case KEY_DOWN:
          menu_driver(menu, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(menu, REQ_UP_ITEM);
          break;
        case 10:
          status = launch_game(games[menu->curitem->index]->appid, games[menu->curitem->index]->is_wine);
          break;
      }
    }
    return status;
}

WINDOW* init_ncurses() {
    // NCurses stuff
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Get the max size of the window, and init windows
    WINDOW* win = newwin(LINES - 3, COLS - 1, 0, 0);
    // Give the menu window focus
    keypad(win, TRUE);
    return win;
}


ITEM** init_items(game_t** games, int size) {
    // Populate the array of items
    ITEM **items = (ITEM**)calloc(size + 1, sizeof(ITEM*));
    for (int i = 0; i < size; i++) {
      if(!games[i]->is_wine) {
        items[i] = new_item(games[i]->name, NULL);
      } else {
        items[i] = new_item(games[i]->name, "(Wine)");
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

  // Generate the log path
  char* log_path;
  asprintf(&log_path, "%s/.steam/steamcurses.log", home);
  FILE* parent_log = fopen(log_path, "w");

  char* steam_path = NULL;
  char* wine_steam_path = NULL;
  char* username = NULL;
  char* password = NULL;
  
  // Deal with user input
  for(int i = 1; i < argc; i++) {
    if(strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--username") == 0) {
        username = argv[++i];
    } else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--steam_path") == 0) {
      steam_path = argv[++i];
    } else if(strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--wine_steam_path") == 0) {
      wine_steam_path = argv[++i];
    } else {
      print_help();
      exit(0);
    }
  }

  // If we don't get provided input, make an educated guess or throw errors
  if(steam_path == NULL) {
    asprintf(&steam_path, "%s/.steam/steam/steamapps/", home);
  }
  if(username == NULL) {
    printf("Error, no username provided!\n");
    print_help();
    exit(1);
  }

  password = (char*) getpass("Password: ");
 

  // Parse the manifests to get a list of installed games
  int size = 0;
  int capacity = 100;
  game_t** games = (game_t**) malloc(capacity * sizeof(game_t*));
  parse_manifests(&size, &capacity, &games, steam_path, 0);
  if(wine_steam_path) {
    parse_manifests(&size, &capacity, &games, wine_steam_path, 1);
  }
  sort_games(games, size);

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
    free(games[i]->name);
    free(games[i]->appid);
    free(games[i]);
    free_item(my_items[i]);
  }
  fclose(parent_log);
  free(games);
  free(my_items);
  free(log_path);
  games = NULL;
  my_items = NULL;
  log_path = NULL;
  steam_path = NULL;
  endwin();
  system("steam -shutdown");
}

