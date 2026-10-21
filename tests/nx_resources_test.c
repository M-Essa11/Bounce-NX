#include "types.h"
#include "local.h"
#include "cbmf.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *asset_root;
static int resource_opens;
FILE *util_open_file(const char *path, const char *mode) {
    char full[1024];
    resource_opens++;
    snprintf(full, sizeof(full), "%s/%s", asset_root, path);
    return fopen(full, mode);
}
int main(int argc, char **argv) {
    assert(argc == 2);
    asset_root = argv[1];
    mkdir("build-nx/test-cwd", 0777);
    assert(chdir("build-nx/test-cwd") == 0);
    /* Reproduce launch from a folder without an external lang/ directory. */
    assert(fopen("lang/lang.xx", "rb") == NULL);
    assert(strcmp(local_get_text(QTJ_BOUN_GAME_NAME), "Bounce") == 0);
    assert(strcmp(local_get_text(QTJ_BOUN_NEW_GAME), "New game") == 0);
    assert(strcmp(local_get_text(QTJ_BOUN_CONTINUE), "Continue") == 0);
    assert(strcmp(local_get_text(QTJ_BOUN_HIGH_SCORES), "High score") == 0);
    assert(strstr(local_get_text(QHJ_BOUN_INSTRUCTIONS_PART_2), "Press A") != NULL);
    assert(resource_opens > 0);
    const char *paths[] = {"fonts/0012-0002.cbmf", "fonts/0015-0064.cbmf",
        "fonts/0019-0128.cbmf", "fonts/0026-0002.cbmf", "fonts/0027-0032.cbmf"};
    for (unsigned i = 0; i < sizeof(paths)/sizeof(paths[0]); i++) {
        FILE *f = util_open_file(paths[i], "rb"); assert(f);
        fseek(f, 0, SEEK_END); long size = ftell(f); rewind(f);
        void *data = malloc((size_t)size); assert(data);
        assert(fread(data, 1, (size_t)size, f) == (size_t)size); fclose(f);
        CbmfFont font; assert(cbmf_mount(&font, data, (size_t)size) == CBMF_OK);
        CbmfGlyphView glyph;
        assert(cbmf_get_glyph(&font, i == 4 ? '8' : 'A', &glyph) == CBMF_OK);
        int ink = 0;
        for (unsigned y = 0; y < glyph.height; y++) for (unsigned x = 0; x < glyph.width; x++) {
            unsigned byte = glyph.bitmap[y*glyph.row_bytes + (x >> 1)];
            unsigned index = (x & 1) ? byte >> 4 : byte & 15;
            assert(index <= 1); ink += index;
        }
        assert(ink > 0);
        free(data);
    }
    puts("NX resource regression passed: embedded-only language loading and 5 indexed fonts.");
    return 0;
}
