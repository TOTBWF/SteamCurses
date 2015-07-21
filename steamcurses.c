#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <menu.h>
#include "parser.h"

int launch_game(char* appid) {
    char prefix[] = "/usr/bin/steam -applaunch ";
    char suffix[] = " 1> ~/.steam/nlog 2>&1";
    char* cmd = (char*) malloc(strlen(prefix) + strlen(appid) + strlen(suffix) + 1);
    strcpy(cmd, prefix);
    strcat(cmd, appid);
    strcat(cmd, suffix);
    printf("%s\n",cmd);
    int status = system(cmd);
    free(cmd);
}


int main(int argc, char* argv[]) {
  // Check to see if username + password were provided

  if(argc == 1) {
    printf("Error! Incorrect Usage\n");
    printf("Usage: steamcurses [username] [password]\n");
    exit(1);
  }

  char* username = argv[1];
  char* password = argv[2];

  // This is a vim styled app, so set up separate windows for each part
  WINDOW* command_win;
  WINDOW* ui_win;

  // NCurses stuff
  initscr();
  cbreak();
  noecho();
 
  // Get the max size of the window, and init windows
  int parent_x, parent_y;
  int command_size = 2;
  getmaxyx(stdscr, parent_y, parent_x);
  command_win = newwin(command_size, parent_x, parent_y - command_size, 0);
  ui_win = newwin(parent_y - command_size, parent_x, 0, 0);
  // Give the menu window focus
  keypad(ui_win, TRUE);
  // Set up logger
  FILE* parent_log = fopen("/home/reed/.steam/steamcurses.log", "w");

  // Start up steam
  pid_t child_pid;
  int commpipe[2];

  // Set up the pipe
  if(pipe(commpipe)) {
    fprintf(parent_log, "Pipe Error!\n");
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
    execl("/usr/bin/steam", "-silent", "-login", username, password, NULL);
  }
  else {
    // Parent Process
    int* size = (int*) malloc(sizeof(int*));
    game_t** g = parse_manifests(size);
    // Create the launch string
    // NCurses stuff
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    // Create array of strings to pass to the ncurses menu
    ITEM** my_items;
    int c;
    MENU* my_menu;

    my_items = (ITEM**)calloc(*size +1, sizeof(ITEM*));

    for (int i = 0; i < *size; i++) {
      my_items[i] = new_item(g[i]->name, g[i]->appid);
      my_items[i]->index = i;
    }
    my_items[*size] = (ITEM*) NULL;
    my_menu = new_menu((ITEM**)my_items);
    mvprintw(LINES - 2, 0, "F1 to exit");
    post_menu(my_menu);
    refresh();
    
    // Game Launch Status
    int status = 0;
    // Only call getch once
    while((c = getch()) != KEY_F(1)) {
      switch(c) {
        case KEY_DOWN:
          menu_driver(my_menu, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(my_menu, REQ_UP_ITEM);
          break;
        case 10:
          status = launch_game(g[my_menu->curitem->index]->appid);
          break;
      }
    }
    for(int i = 0; i < *size; i++) {
      free(g[i]->name);
      free(g[i]->appid);
      free(g[i]);
      free_item(my_items[i]);
    }
    fclose(parent_log);
    free(g);
    free(size);
    endwin();
    system("steam -shutdown");
    return 0;
  }
}

