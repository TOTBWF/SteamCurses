#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <menu.h>
#include <unistd.h>
#include "parser.h"

int launch_game(char* appid) {
    char prefix[] = "/usr/bin/steam -applaunch ";
    char suffix[] = " 1> ~/.steam/nlog 2>&1";
    char* cmd = (char*) malloc(strlen(prefix) + strlen(appid) + strlen(suffix) + 1);
    strcpy(cmd, prefix);
    strcat(cmd, appid);
    strcat(cmd, suffix);
    //printf("%s\n",cmd);
    int status = system(cmd);
    free(cmd);
    return status;
}

void print_title(WINDOW* win, char* title) {
  int length = strlen(title);
  int x, y;
  getmaxyx(win, y, x);
  mvwprintw(win, 0, x/2 - length/2, title);
  refresh();
}


int main(int argc, char* argv[]) {
  // Check to see if username + password were provided
  if(argc != 2) {
    printf("Error! Incorrect Usage\n");
    printf("Usage: steamcurses [username]\n");
    exit(1);
  }

  char* username = argv[1];
  char* password = getpass("Password: ");
 
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
    execl("/usr/bin/steam", "/usr/bin/steam", "-silent", "-login", username, password, NULL);
  }
  else {
    // Parent Process
    // Parse the manifests to get a list of installed games
    int* size = (int*) malloc(sizeof(int*));
    game_t** g = parse_manifests(size);

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
    my_items = (ITEM**)calloc(*size +1, sizeof(ITEM*));
    for (int i = 0; i < *size; i++) {
      my_items[i] = new_item(g[i]->name, NULL);
      my_items[i]->index = i;
    }
    my_items[*size] = (ITEM*) NULL;
    
    // Set up the menu
    my_menu = new_menu((ITEM**)my_items);
    set_menu_format(my_menu, LINES - 4, 0);
    set_menu_win(my_menu, win);
    set_menu_sub(my_menu, derwin(win, LINES - 4, COLS - 2, 1, 0));
    
    // Make things pretty
    mvprintw(LINES - 2, 0, "F1 to exit");
    print_title(win, "SteamCurses");
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
          status = launch_game(g[my_menu->curitem->index]->appid);
          break;
      }
    }

    // Perform clean up
    unpost_menu(my_menu);
    free_menu(my_menu);
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

