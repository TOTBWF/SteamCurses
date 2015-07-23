
typedef struct game {
    char* appid;
    char* name;
    int is_wine;
} game_t;

void parse_manifests(int* size, int* capacity, game_t*** games, char* steam_path, int is_wine);

void sort_games(game_t** games, int size);
