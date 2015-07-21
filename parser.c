#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "parser.h"

char steam_path[] = "/home/reed/.steam/steam/steamapps/";

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

game_t** parse_manifests(int* size) {
  // Set up our array of char pointers
  // Set up directory streams and whatnot
  DIR* d;
  struct dirent* dir;
  FILE* f;
  char buf[256];
  d = opendir(steam_path);
  int games_capacity = 100;
  int games_size = 0;
  game_t** games = (game_t**) malloc(sizeof(game_t*)*games_capacity);
  // Get all files in the steam path dir
  if(d) {
    while((dir = readdir(d)) != NULL) {
      if(strstr(dir->d_name, "appmanifest") != NULL) {
        game_t* g = (game_t*) malloc(sizeof(game_t));
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
        games[games_size++] = g;
        free(path);
        free(fname);
        // Resize array as needed
        if(games_size >= games_capacity) {
          games = (game_t**) realloc(games, games_capacity*2*sizeof(game_t*));
          games_capacity *= 2;
        }
      }
    }
    closedir(d);
  } else {
    printf("Could not open steam directory %s\n", steam_path);
  }
  // Sort the games based off of their names
  for(int i = 1; i < games_size; i++) {
    for(int j = 1; j < games_size; j++) {
      if(strcmp(games[j - 1]->name, games[j]->name) > 0) {
        game_t* tmp = games[j];
        games[j] = games[j - 1];
        games[j - 1] = tmp;
      }
    }
  }
  *size = games_size;
  return games;
}
