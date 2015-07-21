#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "parser.h"


char* get_vals(char* str) {
  int start = 0;
  int end = 0;
  int index = 0;
  while(str[index] != '\0') {
    if(str[index] == '\"') {
      start = end;
      end = index + 1;
    }
    index++;
  }
  end--;
  char* out =(char*) malloc(sizeof(char)*(end - start + 1));
  strncpy(out , &str[start], end - start);
  out[end - start] = '\0';
  return out;
}

void sort_games(game_t** games, int size) {
  for(int i = 1; i < size; i++) {
    for(int j = 1; j < size; j++) {
      if(strcmp(games[j - 1]->name, games[j]->name) > 0) {
        game_t* tmp = games[j];
        games[j] = games[j - 1];
        games[j - 1] = tmp;
      }
    }
  }
}

void parse_manifests(int* size, int* capacity, game_t*** games, char* steam_path, int is_wine) {
  // Set up directory streams and whatnot
  DIR* d;
  struct dirent* dir;
  FILE* f;
  char buf[256];
  d = opendir(steam_path);

  // Get all files in the steam path dir
  if(d) {
    while((dir = readdir(d)) != NULL) {
      if(strstr(dir->d_name, "appmanifest") != NULL) {

        // Create and fill out game struct
        game_t* g = (game_t*) malloc(sizeof(game_t));
        g->is_wine = is_wine;
        char* path = strdup(steam_path);
        char* fname = strdup(dir->d_name);
        char* full_path = (char*)malloc(strlen(path) + strlen(fname) + 1);
        strcpy(full_path, path);
        strcat(full_path, fname);
        f = fopen(full_path, "r");
        while(fgets(buf, sizeof(buf), f)) {
          if(strstr(buf, "appid") != NULL) {
            g->appid = get_vals(buf);
          } else if(strstr(buf, "name") != NULL) {
            g->name = get_vals(buf);
          }
        }
        fclose(f);
        (*games)[*size] = g;
        *size += 1;
        free(path);
        free(fname);

        //Resize array as needed
        if(*size >= *capacity) {
          *games = (game_t**) realloc(*games, *capacity*2*sizeof(game_t*));
          *capacity *= 2;
        }
      }
    }
    closedir(d);
  } else {
    printf("Could not open steam directory %s\n", steam_path);
  }
}


