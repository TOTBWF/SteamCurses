#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "parser.h"
#include "steamcurses.h"

// Sorts a given list of games alphabetacally
void sort_games(game_t** games, int size) {
  for(int i = 1; i < size; i++) {
    for(int j = 1; j < size; j++) {
      // Do the first level of search, and select the first result
      // We dont need to use the output size so just toss it
      char* name_curr = fetch_value(games[j - 1]->key_value_pairs, "name", games[j - 1]->size);
      char* name_next = fetch_value(games[j]->key_value_pairs, "name", games[j]->size);
      // Find the names in the new kvp
      if(strcmp(name_curr, name_next) > 0) {
        game_t* tmp = games[j];
        games[j] = games[j - 1];
        games[j - 1] = tmp;
      }
    }
  }
}

// Fetches the first Key Value Pair that matches the key from an array of Key Value Pairs, NOT RECURSIVE
kvp_t* fetch_match(kvp_t** pairs, char* key, int size) {
  int index = 0;

  while(index < size) {
    if(strcmp(pairs[index]->key, key) == 0) {
      return pairs[index];
    } 
    index++;
  }
  return NULL;
}

// Fetches a list of Key Value pairs that match the key, NOT RECURSIVE
kvp_t** fetch_matching(kvp_t** pairs, char* key, int size, int* output_size) {
  int capacity = 5;
  kvp_t** matches = (kvp_t**)malloc(capacity*sizeof(kvp_t**));
  int index = 0;

  while(index < size) {
    if(strcmp(pairs[index]->key, key) == 0) {
      matches[*output_size] = pairs[index];
      *output_size += 1;

      // Check to see if the list needs to be resized
      if(*output_size >= capacity) {
        capacity *= 2;
        matches = (kvp_t**)realloc(matches, capacity*sizeof(kvp_t*));
      }
    } 
    index++;
  }
  return matches;
}

// Fetches a list of Values from a list of Key Value Pairs that match the key, RECURSIVE
char** fetch_values(kvp_t** pairs, char* key, int size, int* output_size) {
  int capacity = 5;
  char** matches = (char**)malloc(capacity*sizeof(char*));
  int index = 0;

  while(index < size) {
    // If there is no value, then this is a parent
    if(pairs[index]->value == NULL) {
      // Recursively search
      int s = 0;
      char** out = fetch_values(pairs[index]->children, key, pairs[index]->num_children, &s);
      // Increase capacity and copy any matches
      capacity += s;
      matches = (char**)realloc(matches, capacity*sizeof(char*));
      memcpy(matches[*output_size], out, s);
      *output_size += s;
    } else {
      if(strcmp(pairs[index]->key, key) == 0) {
        matches[*output_size] = pairs[index]->value;
        *output_size += 1;

        // Check to see if the list needs to be resized
        if(*output_size >= capacity) {
          capacity *= 2;
          matches = (char**)realloc(matches, capacity*sizeof(char*));
        }
      } 
    }
    index++;
  }
  return matches;
}

// Fetches the first occurance of a key from an array of Key Value Pairs, RECURSIVE
char* fetch_value(kvp_t** pairs, char* key, int size) {
  int index = 0;
  while(index < size) {
    // If there is no value, then this is a parent
    if(pairs[index]->value == NULL) {
      // Recursively search
      char* out = fetch_value(pairs[index]->children, key, pairs[index]->num_children);
      if(out != NULL) {
        return out;
      }
    } else {
      if(strcmp(pairs[index]->key, key) == 0) {
        return pairs[index]->value;
      } 
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

kvp_t** parse_lines(FILE* f, int* size, int* capacity) {
  // Allocate arrau of kvps
  kvp_t** pairs = (kvp_t**) malloc(*capacity*sizeof(kvp_t*));

  char buf[1024];
  while(fgets(buf, sizeof(buf), f)) {
    // Start iterating through the line
    kvp_t* key_pair = (kvp_t*)malloc(sizeof(kvp_t));
    key_pair->key = NULL;
    key_pair->value = NULL;
    key_pair->children = NULL;
    key_pair->num_children = 0;
    key_pair->capacity_children = 0;

    int index = 0;
    int open_quote = 0;
    int closed_quote = 0;
    int quote_num = 0;

    // Iterate through the line
    while(buf[index]) {
      if(buf[index] == '}') {
        // Check to see if next char is EOL
        if(buf[index + 1] == '\n') {
        // End parent object, so break out of recursion
        // Free the generated line
        free(key_pair);
        return pairs;
        }
      } else if(buf[index] == '\"') {
        if(++quote_num % 2 == 0) {
          closed_quote = index;
          if(quote_num == 2) {
            char* key = (char*)malloc(sizeof(char)*(closed_quote - open_quote + 1));
            strncpy(key, &buf[open_quote], closed_quote - open_quote);
            key[closed_quote - open_quote] = '\0';
            key_pair->key = key;
          } else if(quote_num == 4) {
            char* val = (char*)malloc(sizeof(char)*(closed_quote - open_quote + 1));
            strncpy(val, &buf[open_quote], closed_quote - open_quote);
            val[closed_quote - open_quote] = '\0';
            key_pair->value = val;
          }
        } else {
          open_quote = index + 1;
        }
      }
    index++;
    }

    // Line contains important info
    if(key_pair->key != NULL) {

      // Add the new kvp to the array
      pairs[*size] = key_pair;
      *size += 1;

      // Check to see if the array needs to be resized
      if(*size >= *capacity) {
        *capacity *= 2;
        pairs = (kvp_t**) realloc(pairs, *capacity * sizeof(kvp_t));
      }
      
      // If there is no value, then the new kvp is a parent
      if(key_pair->value == NULL) {
        // allocate space for the new parent
        // Recurse into the new parent
        fprintf(g_logfile, "PARSER: Recursing into Key %s\n", key_pair->key);
        key_pair->capacity_children = 10;
        key_pair->children = parse_lines(f, &(key_pair->num_children), &(key_pair->capacity_children));
        fprintf(g_logfile, "PARSER: Recursing out of Key %s\n", key_pair->key);
      } else {
        fprintf(g_logfile, "PARSER: Adding Key %s with value %s\n", key_pair->key, key_pair->value);
      }
    } else {
      // We wont use this line, so free the kvp
      free(key_pair);
    }
    // Clear the buffer
    memset(buf, '\0', sizeof(buf));
  }
  // We should never hit this point
  return pairs;
}


kvp_t** parse_manifest(char* path, int* size) {

  FILE* f = fopen(path, "r");
  if(f == NULL) {
    printf("Error opening manifest file %s\n", path);
    exit(1);
  } 
  int capacity = 100;
  kvp_t** key_pairs = parse_lines(f, size, &capacity);
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
        g->steam_path = steam_path;
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


void free_kvp(kvp_t* pair) {
  if(pair->num_children > 0) {
    for(int i = 0; i < pair->num_children; i++) {
      free_kvp(pair->children[i]);
      pair->children[i] = NULL;
    }
    free(pair->children);
    free(pair->key);
  } else {
    free(pair->key);
    free(pair->value);
  }
  // Null out all pointers
  pair->key = NULL;
  pair->value = NULL;
  pair->children = NULL;
  pair->num_children = 0;
  pair->capacity_children = 0;
  free(pair);
}
