#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "parser.h"
#include "steamcurses.h"


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
      char* name_curr = fetch_value(games[j - 1]->key_value_pairs, "name", games[j - 1]->size);
      char* name_next = fetch_value(games[j]->key_value_pairs, "name", games[j]->size);
      if(strcmp(name_curr, name_next) > 0) {
        game_t* tmp = games[j];
        games[j] = games[j - 1];
        games[j - 1] = tmp;
        fprintf(g_logfile ,"Swap occured of %s and %s\n", name_curr, name_next);
      }
    }
  }
}

char* fetch_value(kvp_t** pairs, char* key, int size) {
  int index = 0;
  fprintf(g_logfile, "Fetching Value from %d Keys\n", size);
  while(index < size) {
    fprintf(g_logfile, "%s:%s\n", key, pairs[index]->key);
    if(strcmp(pairs[index]->key, key) == 0) {
      fprintf(g_logfile, "Value Found:%s\n", pairs[index]->value);
      return pairs[index]->value;
    }
    index++;
  }
  return NULL;
}


kvp_t* parse_line(char* line) {
  kvp_t* key_pair = (kvp_t*)malloc(sizeof(kvp_t));
  key_pair->key = NULL;
  key_pair->value = NULL;
  int index = 0;
  int open_quote = 0;
  int closed_quote = 0;
  int quote_num = 0;

  while(line[index] != '\0') {
    if(line[index] == '\"') {
      if(++quote_num % 2 == 0) {
        closed_quote = index;
        if(quote_num == 2) {
          char* key = (char*)malloc(sizeof(char)*(closed_quote - open_quote + 1));
          strncpy(key, &line[open_quote], closed_quote - open_quote);
          key[closed_quote - open_quote] = '\0';
          key_pair->key = key;
        } else if(quote_num == 4) {
          char* val = (char*)malloc(sizeof(char)*(closed_quote - open_quote + 1));
          strncpy(val, &line[open_quote], closed_quote - open_quote);
          val[closed_quote - open_quote] = '\0';
          key_pair->value = val;
        }
      } else {
        open_quote = index + 1;
      }
    }
    index++;
  }
  // If there arent 4 quotes (1 key and 1 value), return null
  if(key_pair->key == NULL || key_pair->value == NULL) {
    key_pair = NULL;
  }
  return key_pair;
}

kvp_t** parse_manifest(char* path, int* size) {
  int capacity = 100;
  kvp_t** key_pairs = (kvp_t**)malloc(sizeof(kvp_t*)*capacity);

  char buf[256];
  FILE* f = fopen(path, "r");
  if(f == NULL) {
    perror("Error opening manifest file %s:");
    exit(1);
  } 
  fprintf(g_logfile, "Parsing Manifest File %s\n============================\n", path);
  while(fgets(buf, sizeof(buf), f)) {
    kvp_t* pair = parse_line(buf);
    if(pair != NULL) {
      key_pairs[*size] = pair;
      fprintf(g_logfile, "Added Pair Key:%s Value:%s\n", key_pairs[*size]->key, key_pairs[*size]->value);
      *size += 1;
    }

    // Resize the array if needed
    if(*size >= capacity) {
      key_pairs = (kvp_t**) realloc(key_pairs, capacity * 2 * sizeof(kvp_t*));
      capacity *= 2;
    }
  }
  fclose(f);
  return key_pairs;
}

void parse_manifests(int* size, int* capacity, game_t*** games, char* steam_path, int is_wine) {
  // Set up directory streams and whatnot
  DIR* d;
  struct dirent* dir;
  d = opendir(steam_path);

  // Get all files in the steam path dir
  if(d) {
    while((dir = readdir(d)) != NULL) {
      // If the file is a manifest, prepare to parse
      if(strstr(dir->d_name, "appmanifest") != NULL) {

        // Create and fill out game struct
        game_t* g = (game_t*) malloc(sizeof(game_t));
        g->is_wine = is_wine;
        g->size = 0;

        // Construct the full path to the file
        char* full_path;
        asprintf(&full_path, "%s%s", steam_path, dir->d_name);
        g->key_value_pairs = parse_manifest(full_path, &(g->size));
        (*games)[*size] = g;
        *size += 1;

        // Free the memory used by the string
        free(full_path);
        full_path = NULL;

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


