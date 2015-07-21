#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <curses.h>
#include <menu.h>
#include <unistd.h>
#include <string.h>
#include "parser.h"

int launch_game(char* appid, int is_wine) {
    char* prefix;
    if(is_wine) {
      prefix = strdup("wine $WINEPREFIX/drive_c/Program\\ Files/Steam/Steam.exe -applaunch ");
    } else {
      prefix = strdup("/usr/bin/steam -applaunch ");
    }
    char suffix[] = " 1>> ~/.steam/steamcurses.log 2>> ~/.steam/steamcurses.log";
    char* cmd = (char*) malloc(strlen(prefix) + strlen(appid) + strlen(suffix) + 1);
    strcpy(cmd, prefix);
    strcat(cmd, appid);
    strcat(cmd, suffix);
    int status = system(cmd);

    free(prefix);
    free(cmd);
    prefix = NULL;
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



int main(int argc, char* argv[]) {

  // The current, user, used to guess default locations of stuff
  char* home = getenv("HOME");

  // Generate the log path
  char log_suffix[] = "/.steam/steamcurses.log";
  char* log_path = (char*)malloc(strlen(log_suffix) + strlen(home) + 1);
  strcpy(log_path, home);
  strcat(log_path, log_suffix);

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
    char suffix[] = "/.steam/steam/steamapps/";
    steam_path = (char*) malloc(strlen(suffix) + strlen(home) + 1);
    strcpy(steam_path, home);
    strcat(steam_path, suffix);
  }
  if(username == NULL) {
    printf("Error, no username provided!\n");
    print_help();
    exit(1);
  }

  password = (char*) getpass("Password: ");
 

  // Start up steam
  pid_t child_pid;
  int commpipe[2];

  // Set up the pipe
  if(pipe(commpipe)) {
    fprintf(parent_log, "Piping Error!\n");
    exit(1);
  }

  // Fork off the steam process
  if((child_pid=fork()) < 0) {
    fprintf(parent_log, "Forking Error!\n");
    exit(1);
  }

  if(child_pid == 0) {
    // Set STDOUT to be piped to the parent
    dup2(commpipe[1], STDOUT_FILENO);
    close(commpipe[0]);
    setvbuf(stdout, (char*)NULL, _IONBF, 0);
    // Exec Steam so it can wait for our requests
    execl("/usr/bin/steam", "/usr/bin/steam", "-silent", "-login", username, password, NULL);
  }
  else {
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

    WINDOW* win;
    ITEM** my_items;
    MENU* my_menu;
    int c;

    // NCurses stuff
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Get the max size of the window, and init windows
    win = newwin(LINES - 3, COLS - 1, 0, 0);
    // Give the menu window focus
    keypad(win, TRUE);

    // Populate the array of items
    my_items = (ITEM**)calloc(size +1, sizeof(ITEM*));
    for (int i = 0; i < size; i++) {
      if(!games[i]->is_wine) {
        my_items[i] = new_item(games[i]->name, NULL);
      } else {
        my_items[i] = new_item(games[i]->name, "(Wine)");
      }
      my_items[i]->index = i;
    }
    my_items[size] = (ITEM*) NULL;
    
    // Set up the menu
    my_menu = new_menu((ITEM**)my_items);
    set_menu_format(my_menu, LINES - 4, 0);
    set_menu_win(my_menu, win);
    set_menu_sub(my_menu, derwin(win, LINES - 4, COLS - 2, 1, 0));
    
    // Make things pretty
    mvprintw(LINES - 2, 0, "F1 to exit");
    char title[] = "SteamCurses";
    print_title(win, title);
    refresh();

    // Post the menu
    post_menu(my_menu);
    wrefresh(win);
    
    // Game Launch Status
    int status = 0;
    // Only call getch once
    while((c = wgetch(win)) != KEY_F(1)) {
      switch(c) {
        case KEY_DOWN:
          menu_driver(my_menu, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(my_menu, REQ_UP_ITEM);
          break;
        case 10:
          status = launch_game(games[my_menu->curitem->index]->appid, games[my_menu->curitem->index]->is_wine);
          break;
      }
    }

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
    free(steam_path);
    games = NULL;
    my_items = NULL;
    log_path = NULL;
    steam_path = NULL;
    endwin();
    system("steam -shutdown");
    return 0;
  }
}

