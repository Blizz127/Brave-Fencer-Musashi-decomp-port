/* A synthetic MAIN.CD directory, PAC member and 31 one-triangle components.
 * The test supplies the read-only media API used by the static library. */
#include "musashi_forest_player_mesh.h"

#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

enum {
    SECTOR = 2048, DIRECTORY_COUNT = 11, PLAYER_ENTRY = 8 + 10 * 8,
    TABLE = 0xf4ac, PART_SIZE = 72, FIRST = TABLE - MUSASHI_FOREST_PLAYER_PARTS * PART_SIZE,
    PAYLOAD = 3 * SECTOR, PAYLOAD_SIZE = 32 * SECTOR, ARCHIVE = PAYLOAD + PAYLOAD_SIZE
};
static const uint32_t origin = UINT32_C(0x800cedf8);

struct MusashiDiscMedia {
    uint8_t bytes[ARCHIVE];
    size_t size;
    unsigned reads, refuse_read;
};

int musashi_disc_media_read_root_range(const MusashiDiscMedia *handle, const char *name,
                                       size_t offset, uint8_t *out, size_t count, size_t *size) {
    struct MusashiDiscMedia *disc = (struct MusashiDiscMedia *)handle;
    assert(strcmp(name, "MAIN.CD") == 0);
    if (++disc->reads == disc->refuse_read || offset > disc->size || count > disc->size - offset)
        return 0;
    memcpy(out, disc->bytes + offset, count);
    *size = disc->size;
    return 1;
}

static void store32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void initialize(struct MusashiDiscMedia *disc) {
    uint8_t *header = disc->bytes + 2 * SECTOR;
    uint8_t *data = disc->bytes + PAYLOAD;
    memset(disc, 0, sizeof(*disc));
    disc->size = sizeof(disc->bytes);
    store32(disc->bytes, DIRECTORY_COUNT);
    store32(disc->bytes + PLAYER_ENTRY, 1);
    store32(disc->bytes + PLAYER_ENTRY + 4, ARCHIVE - SECTOR);
    memcpy(disc->bytes + SECTOR, "PAC", 4);
    store32(disc->bytes + SECTOR + 12, SECTOR);
    memcpy(header, "PAC", 4);
    header[4] = header[5] = 1;
    store32(header + 12, SECTOR + PAYLOAD_SIZE);
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        unsigned start = FIRST + i * PART_SIZE;
        uint8_t *part = data + start;
        store32(data + TABLE + 4 * i, origin + start);
        store32(part, 0x41);
        store32(part + 8, 1);
        store32(part + 12, 44); store32(part + 16, 1);
        store32(part + 20, 52); store32(part + 24, 1);
        store32(part + 28, 28); store32(part + 32, 1);
        part[40] = 4; part[41] = 3; part[43] = 0x20;
        part[44] = 123; part[45] = 20; part[46] = 3;
        part[56] = 0xff; part[57] = 0xff; part[59] = 0x80;
    }
}

int main(void) {
    enum { PART = PAYLOAD + FIRST, LIST = PAYLOAD + TABLE, HEADER = 2 * SECTOR };
    struct MusashiDiscMedia *disc = malloc(sizeof(*disc));
    assert(disc);
    assert(!musashi_forest_player_mesh_load(NULL));
    musashi_forest_player_mesh_free(NULL);
    initialize(disc);
    MusashiForestPlayerMesh *mesh = musashi_forest_player_mesh_load(disc);
    assert(mesh && disc->reads == 5);
    memset(disc->bytes, 0, sizeof(disc->bytes));
    const MusashiForestPlayerPart *part = musashi_forest_player_mesh_part(mesh, 30);
    assert(part && part->vertex_count == 1 && part->vertices[0].x == -1 &&
           part->vertices[0].y == -32768 && part->face_count == 1 &&
           part->faces[0].rgb[0] == 123 && part->faces[0].corners == 3);
    assert(!musashi_forest_player_mesh_part(mesh, MUSASHI_FOREST_PLAYER_PARTS));
    musashi_forest_player_mesh_free(mesh);

    /* Each optional read failure must discard partial decoding. */
    for (unsigned read = 1; read <= 5; ++read) {
        initialize(disc);
        disc->refuse_read = read;
        assert(!musashi_forest_player_mesh_load(disc));
    }
    const struct { size_t offset; uint32_t wrong; } invalid[] = {
        {0, 10}, {0, 256}, {PLAYER_ENTRY, 0}, {PLAYER_ENTRY, UINT32_MAX},
        {PLAYER_ENTRY + 4, UINT32_MAX}, {SECTOR, 0}, {SECTOR + 12, 1},
        {SECTOR + 12, UINT32_MAX}, {HEADER, 0}, {HEADER + 12, 100},
        {HEADER + 12, UINT32_MAX}, {HEADER + 12, SECTOR + TABLE}, {HEADER + 4, 0x201},
        {LIST + 4 * MUSASHI_FOREST_PLAYER_PARTS, 1}, {LIST, 0}, {LIST, 0x81000000},
        {LIST, origin + TABLE}, {LIST + 4, origin + FIRST}, {LIST + 4, origin + FIRST + 5},
        {PART, 66}, {PART + 4, 1}, {PART + 8, 2}, {PART + 36, 1}, {PART + 12, 40},
        {PART + 16, UINT32_MAX}, {PART + 20, UINT32_MAX}, {PART + 24, 0},
        {PART + 28, 4}, {PART + 32, 0}, {PART + 32, UINT32_MAX}, {PART + 48, 1}, {PART + 52, 1}
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        initialize(disc);
        store32(disc->bytes + invalid[i].offset, invalid[i].wrong);
        assert(!musashi_forest_player_mesh_load(disc));
    }
    const uint8_t bad_command[] = {5, 4, 1, 0x24};
    for (size_t i = 0; i < sizeof(bad_command); ++i) {
        initialize(disc);
        disc->bytes[PART + 40 + i] = bad_command[i];
        assert(!musashi_forest_player_mesh_load(disc));
    }
    initialize(disc);
    disc->size = 15;
    assert(!musashi_forest_player_mesh_load(disc));
    free(disc);
    return 0;
}
