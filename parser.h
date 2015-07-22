typedef struct kvp {
  char* key;
  char* value;
} kvp_t;

typedef struct game {
  kvp_t** key_value_pairs;
  int size;
  int is_wine;
} game_t;


char* fetch_value(kvp_t** pairs, char* key, int size);
void parse_manifests(int* size, int* capacity, game_t*** games, char* steam_path, int is_wine);
void sort_games(game_t** games, int size);
