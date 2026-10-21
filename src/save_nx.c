#include "types.h"
#include <string.h>
#include <sys/stat.h>
#include "nx_sdl.h"

#ifdef __SWITCH__
#define SAVE_DIR "sdmc:/switch/bounce-nx"
#else
#define SAVE_DIR "save-nx"
#endif
#define SAVE_PATH SAVE_DIR "/save.dat"
#define SAVE_TEMP SAVE_DIR "/save.tmp"
#define SAVE_MAGIC 0x424F554E
static SaveData data;
static int dirty;

void save_init(void) {
    SaveData loaded;
    memset(&data, 0, sizeof(data));
    data.magic = SAVE_MAGIC;
    data.best_level = data.current_level = 1;
    data.current_lives = 3;
#ifdef __SWITCH__
    mkdir("sdmc:/switch", 0777);
#endif
    mkdir(SAVE_DIR, 0777);
    FILE *file = fopen(SAVE_PATH, "rb");
    if (file) {
        if (fread(&loaded, 1, sizeof(loaded), file) == sizeof(loaded) &&
            loaded.magic == SAVE_MAGIC) data = loaded;
        fclose(file);
    }
    if (data.best_level < 1 || data.best_level > MAX_LEVEL) data.best_level = 1;
    if (data.current_level < 1 || data.current_level > MAX_LEVEL) data.current_level = 1;
    if (data.current_lives < 0 || data.current_lives > 5) data.current_lives = 3;
    if (data.best_score < 0) data.best_score = 0;
    if (data.current_score < 0) data.current_score = 0;
    data.has_continue = data.has_continue != 0;
    dirty = 0;
}
void save_flush(void) {
    if (!dirty) return;
    FILE *file = fopen(SAVE_TEMP, "wb");
    if (!file) { nx_log("Cannot open save file"); return; }
    int ok = fwrite(&data, 1, sizeof(data), file) == sizeof(data);
    if (fflush(file) != 0) ok = 0;
    if (fclose(file) != 0) ok = 0;
    if (ok && rename(SAVE_TEMP, SAVE_PATH) == 0) dirty = 0;
    else nx_log("Cannot commit save file");
}
void save_shutdown(void) { save_flush(); }
void save_update_records(int level, int score) {
    if (level > data.best_level && level <= MAX_LEVEL) { data.best_level = level; dirty = 1; }
    if (score > data.best_score) { data.best_score = score; dirty = 1; }
}
void save_set_progress(int level, int score, int lives, int active) {
    data.current_level = level;
    data.current_score = score;
    data.current_lives = lives;
    data.has_continue = active != 0;
    dirty = 1;
    save_flush();
}
SaveData *save_get_data(void) { return &data; }
