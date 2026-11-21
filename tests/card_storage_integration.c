#include "card_host_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    CsStorage *storage;
    uint8_t guest[2048];
    uint32_t group[2], spec[2];
    unsigned events;
} Harness;

#define REQUIRE(test) do { if (!(test)) { \
    fprintf(stderr, "failure at line %d: %s\n", __LINE__, #test); \
    exit(1); \
} } while (0)

static uint8_t *map(void *p, uint32_t address, size_t n, int write) {
    Harness *h = p;
    (void)write;
    if (address >= sizeof h->guest || n > sizeof h->guest - address) return NULL;
    return h->guest + address;
}

static CsMedia snapshot(void *p, unsigned slot, uint8_t *destination) {
    Harness *h = p;
    return cs_storage_snapshot(h->storage, slot, destination);
}

static int commit(void *p, unsigned slot, const uint8_t *old_image,
                  const uint8_t *new_image) {
    Harness *h = p;
    return cs_storage_commit(h->storage, slot, old_image, new_image);
}

static int event(void *p, uint32_t group, uint32_t spec) {
    Harness *h = p;
    REQUIRE(h->events < 2);
    h->group[h->events] = group;
    h->spec[h->events++] = spec;
    return 1;
}

static uint8_t invented(unsigned i, unsigned seed) {
    return (uint8_t)(i * 79u + (i >> 3) + seed);
}

static void visit(CardService *s, Harness *h, int create, unsigned blocks,
                  const char *path, unsigned offset, unsigned seed) {
    const uint32_t payload = 515u, length = 256u;
    uint32_t i, mode = create ? 0x8202u | (blocks << 16) : 0x8001u;
    int fd;
    memcpy(h->guest + 33u, path, strlen(path) + 1u);
    fd = cs_open(s, 33u, mode);
    REQUIRE(fd >= 2);
    REQUIRE(cs_seek(s, fd, (int32_t)offset, 0) == (int32_t)offset);
    memset(h->guest + payload - 1, 0x5e, length + 2);
    if (create) {
        for (i = 0; i < length; ++i) h->guest[payload + i] = invented(i, seed);
        REQUIRE(cs_write(s, fd, payload, length) == 0);
    } else {
        REQUIRE(cs_read(s, fd, payload, length) == 0);
    }
    REQUIRE(h->events == 0);
    REQUIRE(cs_pump(s) == 3);
    REQUIRE(cs_idle(s));
    REQUIRE(h->events == 2 && h->group[0] == (uint32_t)fd && h->group[1] == CS_EVENT_SW);
    REQUIRE(h->spec[0] == CS_DONE && h->spec[1] == CS_DONE);
    REQUIRE(h->guest[payload - 1] == 0x5e && h->guest[payload + length] == 0x5e);
    for (i = 0; i < length; ++i) REQUIRE(h->guest[payload + i] == invented(i, seed));
    h->events = 0;
    REQUIRE(cs_close(s, fd) == fd);
}

int main(int argc, char **argv) {
    Harness h = {0};
    CsHost host = { &h, map, snapshot, commit, event };
    CardService *s;
    int create;
    REQUIRE(argc == 3);
    create = strcmp(argv[1], "write") == 0;
    REQUIRE(create || strcmp(argv[1], "read") == 0);
    REQUIRE(bfm_plat_memcard_set_dir(argv[2]) == BFM_PLAT_OK);
    h.storage = cs_storage_create();
    REQUIRE(h.storage);
    s = cs_create(host);
    REQUIRE(s);
    REQUIRE(cs_prepare(s, 0) == CS_MEDIA_ABSENT);
    REQUIRE(cs_storage_attach(h.storage, 0) == BFM_PLAT_OK);
    REQUIRE(cs_prepare(s, 0) == CS_MEDIA_OK);
    cs_init_card(s);
    cs_start_card(s);
    cs_init_files(s);
    visit(s, &h, create, 2, "bu00:ORCHARD", 8192u - 128u, 26u);
    visit(s, &h, create, 1, "bu00:DELTA", 128u, 112u);
    cs_destroy(s);
    cs_storage_detach(h.storage, 0);
    cs_storage_destroy(h.storage);
    puts(create ? "created two synthetic files through platform storage" :
                  "fresh process verified both guarded byte ranges through platform storage");
    return 0;
}
