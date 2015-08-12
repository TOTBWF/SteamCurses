#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "parser.h"
#include "steamcurses.h"

void init_generator(char* path) {
  // Check to see if the manifest path exists
  struct stat st;
  if(stat(path, &st) == -1) {
    // Create the dir if it doesnt exits
    mkdir(path, 0755);
  }
}

void generate_manifests(char* path, game_t** games, int size) {
  fprintf(g_logfile, "Begining Manifest Generation...\n");
  // Make sure the path actually exists
  init_generator(path);

  for(int i = 0; i < size; i++) {
    printf("Generating Manifest %d out of %d\n", i + 1, size);
    // Fetch the appid
    char* appid = fetch_value(games[i]->key_value_pairs, "appid", games[i]->size);
    // Get the game install dir
    char* installdir = fetch_value(games[i]->key_value_pairs, "installdir", games[i]->size);
    // Get the games steam path
    char* steam_path = games[i]->steam_path;
    int is_wine = games[i]->is_wine;
    // Create the full path
    char* full_path; 
    asprintf(&full_path, "%s%s.manifest", path, appid);

    fprintf(g_logfile, "Checking file %s...\n", full_path);
    // Check to see if the file exists
    if(access(full_path, F_OK) == -1) {
      fprintf(g_logfile, "Creating file %s...\n", full_path);

      /*
        KLUDGE ALERT!!!
        We need to get the path to the games executable
        the only way we can do this is to use and abuse steamcmd
        The cmd app_info_print <appid> dumps all configs for a given appid
        We are going to pipe that into a manifest file , then parse it to get the details
        The full command to do so is as follows (command will be passed to shell)
        steamcmd +app_info_print <appid> +quit 1> <filepath> 2> <filepath>
       */

      char* cmd;
      asprintf(&cmd, "steamcmd +app_info_print %s +quit 1>%s 2>%s", appid, full_path, full_path);
      fprintf(g_logfile, "Generated steamcmd: %s\n", cmd);
      system(cmd);
      free(cmd);
    }
    // Parse the generated file
    int size = 0;
    kvp_t** gen_manifest = parse_manifest(full_path, &size);
    // Find the executable path
    // The heirarchy for the executable paths is as follows:
    // "<appid>" "config" "launch" "<numberi>" "executable"
    // We also need to find launch options, which are located under the arguments key at
    // "<appid>" "config" "launch" "<number>" "arguments"
    // To find the right executable, we have to look for the right oslist value (linux for linux, windows for wine)
    // That is located here:
    // "<appid>" "config" "launch" "<number> "config" "oslist"
    // The procedure to get the executable path is then:
    // 1) get a listing of all possible options in kvp_t** form
    // 2) iterate through and recursivley search for the matching oslist value
    // 3) use the executable value to get part of the path
    // 4) append that to the game directory, found in the normal appmanifest files under the installdir key
    // 5) append that to the steam_path
    // At this point you are probably thinking, WTF, why???
    // Me too.... me too...
    
    // First get a listing of all options in kvp_t form
    kvp_t* match_appids = fetch_match(gen_manifest, appid, size);
    kvp_t* match_config = fetch_match(match_appids->children, "config", match_appids->num_children);
    if(match_config == NULL) {
      printf("Parsing Error, %s could not be found in %s\n", "config", full_path);
    }
    // There should only be one 
    kvp_t* match_launch = fetch_match(match_config->children, "launch", match_config->num_children);

    char* executable = NULL;
    char* launch_options = NULL;

    // Iterate through and search for a valid launch option
    for(int j = 0; j < match_launch->num_children; j++) {
      kvp_t* curr_child = match_launch->children[j];
      // Recursively search for the oslist value for each child
      char* os_list = fetch_value(curr_child->children, "oslist", curr_child->num_children);
      if((is_wine && strcmp(os_list, "windows") == 0) || ((!is_wine) && strcmp(os_list, "linux") == 0)) {
        // we found the correct listing, so use the index to search for the executable path and the launch options
        executable = fetch_value(curr_child->children, "executable", curr_child->num_children);
        launch_options = fetch_value(curr_child->children, "arguments", curr_child->num_children);
      }
    }

    // Build the full executable path
    char* full_exec_path;
    asprintf(&full_exec_path, "%scommon/%s/%s %s", steam_path, installdir, executable, launch_options);
    fprintf(g_logfile, "Full Exec Path:%s\n", full_exec_path);
    games[i]->exec_path = full_exec_path;

    // Free all of the kvps that we created
    for(int j = 0; j < size; j++) {
      free_kvp(gen_manifest[j]);
    }
    free(gen_manifest);
    free(full_path);
  }
}



