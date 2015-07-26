// Key Value Pairs
typedef struct kvp_t {
  char* key;
  char* value;
  struct kvp_t** children;
  int num_children;
  int capacity_children;
} kvp_t;

typedef struct game {
  kvp_t** key_value_pairs;
  int size;
  char* exec_path;
  char* steam_path;
  int is_wine;
} game_t;


// Fetches the value of the first instance of a KVP with a matching key out of an array of KVPs, RECURSIVE
char* fetch_value(kvp_t** pairs, char* key, int size);
// Fetches all values associated with the given key out of an array of KVPs, RECURSIVE
char** fetch_values(kvp_t** pairs, char* key, int size, int* output_size);
// Fetches the kvp associated with the first instance of a matching key out of an array of KVPs, NOT RECURSIVE
kvp_t* fetch_match(kvp_t** pairs, char* key, int size);
// Fetches all kvp associated with a matching key out of an array of KVPs, NOT RECURSIVE
kvp_t** fetch_matching(kvp_t** pairs, char* key, int size, int* output_size);
// Parses a single manifest and turns it into an array of KVPs
kvp_t** parse_manifest(char* path, int* size);
// Parses all manifests in a given path and turns them into an array of game structs
void parse_manifests(int* size, int* capacity, game_t*** games, char* steam_path, int is_wine);
// Alphabetacally sorts an array of games
void sort_games(game_t** games, int size);
// Frees a kvp and all of its children
void free_kvp(kvp_t* pair);
