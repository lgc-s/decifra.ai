#include "systems/save.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SAVE_MAGIC   0x41495652u   /* "RVIA" */
#define SAVE_VERSION 1u

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t checksum;      /* FNV-1a do conteúdo de Progress */
    Progress data;
} SaveFile;

static uint32_t fnv1a(const void *buf, size_t n) {
    const unsigned char *p = buf;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= 16777619u; }
    return h;
}

bool save_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

bool save_write(const Progress *p, const char *path) {
    SaveFile sf;
    memset(&sf, 0, sizeof sf);
    sf.magic = SAVE_MAGIC;
    sf.version = SAVE_VERSION;
    sf.size = (uint32_t)sizeof(Progress);
    sf.data = *p;
    sf.checksum = fnv1a(&sf.data, sizeof sf.data);

    char tmp[256];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return false;
    bool ok = fwrite(&sf, sizeof sf, 1, f) == 1;
    ok = (fclose(f) == 0) && ok;
    if (!ok) { remove(tmp); return false; }
    remove(path);                       /* rename não sobrescreve em todas as plataformas */
    if (rename(tmp, path) != 0) { remove(tmp); return false; }
    return true;
}

static bool valid_ranges(const Progress *p) {
    if (p->current_act < 1 || p->current_act > NUM_ACTS) return false;
    if (p->unlocked_act < 1 || p->unlocked_act > NUM_ACTS) return false;
    if (p->trust < 0 || p->trust > 100) return false;
    if (p->difficulty < 0 || p->difficulty > 2) return false;
    if (p->volume < 0 || p->volume > 10) return false;
    if (p->ending < -1 || p->ending > 3) return false;
    for (int i = 0; i < NUM_CALLS; i++)
        if (p->call_choice[i] < -1 || p->call_choice[i] > 3) return false;
    return true;
}

bool save_read(Progress *p, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    SaveFile sf;
    size_t n = fread(&sf, sizeof sf, 1, f);
    fclose(f);
    if (n != 1) return false;
    if (sf.magic != SAVE_MAGIC || sf.version != SAVE_VERSION) return false;
    if (sf.size != sizeof(Progress)) return false;
    if (sf.checksum != fnv1a(&sf.data, sizeof sf.data)) return false;
    if (!valid_ranges(&sf.data)) return false;
    *p = sf.data;
    return true;
}
