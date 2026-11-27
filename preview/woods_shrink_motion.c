/* SPDX-License-Identifier: MIT */
#include "woods_shrink_motion.h"
#include "scene_tiles.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum { SECTOR = 2048, PACKET = 256, MAX_KEYS = 16, EMPTY_SLOT = 34 };
typedef struct {
    unsigned count;
    uint8_t packets[MAX_KEYS][PACKET];
} Body;
typedef struct {
    uint16_t x, y, width, height;
    uint8_t pixels[16 * 40 * 2]; /* Largest of the two mounted facial rectangles. */
} FaceRect;
typedef struct {
    uint32_t word;
    unsigned slot;
    FaceRect rect[2];
} Face;
struct BfmWoodsShrinkMotion {
    Body body[3];
    Face face[4];
};

typedef struct {
    uint8_t *code;
    size_t size;
    uint16_t (*tiles)[1024];
} Skill;

/* The skill's selector names a MAIN member. Its first sparse upload holds
 * animation in video memory and its uncompressed type-one block owns the
 * two body programs and direct poses. */
static int mount_skill(const MusashiDiscMedia *disc, const ForestPlayerArchive *resident, Skill *s)
{
    uint8_t resource[8], directory[SECTOR], header[16];
    size_t total;
    if (!forest_player_archive_read(resident, UINT32_C(0x800d3784), resource, sizeof resource) ||
        u32le(resource + 4) != 14 ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", 0, directory, sizeof directory, &total))
        return 0;
    uint32_t member = u32le(resource), count = u32le(directory);
    if (member >= count || count > (SECTOR - 8) / 8) return 0;
    const uint8_t *entry = directory + 8 + 8 * member;
    size_t sector = u32le(entry), remaining = u32le(entry + 4);
    if (!sector || sector > total / SECTOR) return 0;
    size_t offset = sector * SECTOR;
    if (remaining > total - offset) return 0;
    while (remaining >= SECTOR) {
        if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", offset, header, sizeof header, &total) ||
            memcmp(header, "PAC", 4)) return 0;
        size_t length = u32le(header + 12);
        if (length < SECTOR || length > remaining) return 0;
        size_t sectors = (length - 1) / SECTOR + 1;
        if (sectors > remaining / SECTOR) return 0;
        if (header[4] == 1) {
            s->size = length - SECTOR;
            if (header[5] || !s->size || s->size > UINT32_C(0x800cedf8) - UINT32_C(0x800cae08)) return 0;
            s->code = malloc(s->size);
            s->tiles = calloc(512, sizeof *s->tiles);
            return s->code && s->tiles &&
                musashi_disc_media_read_root_range(disc, "MAIN.CD", offset + SECTOR, s->code, s->size, &total) &&
                bfm_scene_read_tiles(disc, "MAIN.CD", member, s->tiles);
        }
        remaining -= sectors * SECTOR;
        offset += sectors * SECTOR;
    }
    return 0;
}

static const uint8_t *skill_bytes(const Skill *s, uint32_t address, size_t bytes)
{
    const uint32_t origin = UINT32_C(0x800cae08);
    if (address < origin || (address & 3)) return NULL;
    size_t offset = address - origin;
    return offset <= s->size && bytes <= s->size - offset ? s->code + offset : NULL;
}

static int table_entry(const ForestPlayerArchive *resident, uint32_t table, unsigned slot, uint32_t *entry)
{
    uint8_t data[4];
    if (table > UINT32_MAX - slot * 4 || !forest_player_archive_read(resident, table + slot * 4, data, sizeof data))
        return 0;
    *entry = u32le(data);
    return 1;
}

static int read_face(Face *face, const ForestPlayerArchive *resident, uint32_t table, unsigned slot)
{
    uint32_t address;
    uint8_t program[16], headers[36];
    if (!table_entry(resident, table, slot, &address) ||
        !forest_player_archive_read(resident, address, program, sizeof program)) return 0;
    /* The normal face starts with a frame duration; each of these slots
     * contains exactly one image or one resident tag followed by the end. */
    const uint8_t *key = u32le(program) == 3 ? program + 8 : program;
    if (u32le(key + 4) != 0) return 0;
    face->slot = slot;
    uint32_t image = u32le(key);
    if ((image & UINT32_C(0xfffffff8)) == UINT32_C(0x40000000)) {
        face->word = image;
        return 1;
    }
    if (!forest_player_archive_read(resident, image, headers, sizeof headers) || headers[32] != 0xff)
        return 0;
    for (unsigned i = 0; i < 2; ++i) {
        FaceRect *r = &face->rect[i];
        const uint8_t *h = headers + i * 16;
        r->x = u16le(h + 4); r->y = u16le(h + 6);
        r->width = u16le(h + 8); r->height = u16le(h + 10);
        if (!r->width || !r->height || r->width + r->x > 1024 || r->height + r->y > 512) return 0;
        size_t bytes = 2u * r->width * r->height;
        if (bytes > sizeof r->pixels ||
            !forest_player_archive_read(resident, u32le(h + 12), r->pixels, bytes)) return 0;
    }
    return 1;
}

static int rectangle_pose(const Skill *s, const uint8_t *rect, uint8_t packet[PACKET])
{
    if (!rect || u16le(rect + 4) != 32 || u16le(rect + 6) != 4) return 0;
    unsigned x = u16le(rect), y = u16le(rect + 2);
    if (x > 1024 - 32 || y > 512 - 4) return 0;
    for (unsigned row = 0; row < 4; ++row) {
        for (unsigned col = 0; col < 32; ++col) {
            unsigned index = 2 * (row * 32 + col);
            uint16_t value = s->tiles[y + row][x + col];
            packet[index] = (uint8_t)value;
            packet[index + 1] = (uint8_t)(value >> 8);
        }
    }
    return 1;
}

/* Within the wind-up a direct pose is followed by alternating reads from
 * two staging buffers. A rectangle command prepares the next packet, and
 * its corresponding display command consumes a normal animation tick. */
static int read_skill_body(Body *body, const Skill *s, uint32_t program, uint32_t rig,
                           int loop, unsigned *face_slot)
{
    const uint8_t *header = skill_bytes(s, program, 24);
    if (!header || u32le(header) != 4 || u32le(header + 4) != rig || u32le(header + 8) != 15 ||
        u32le(header + 16) != 17 || u32le(header + 20) != 19) return 0;
    *face_slot = u32le(header + 12);
    if (*face_slot > 255) return 0;
    const uint8_t *pending[2] = {NULL, NULL};
    for (unsigned i = 6; i < 64; ++i) {
        const uint8_t *word = skill_bytes(s, program + 4 * i, 4);
        if (!word) return 0;
        uint32_t value = u32le(word);
        if (value <= 1) return value == (unsigned)loop && body->count && !pending[0] && !pending[1];
        if (value == 21 || value == 22) {
            const uint8_t *operand = skill_bytes(s, program + 4 * ++i, 4);
            unsigned bank = value - 21;
            if (pending[bank] || !operand || !(pending[bank] = skill_bytes(s, u32le(operand), 8))) return 0;
            continue;
        }
        if (body->count == MAX_KEYS) return 0;
        uint8_t *packet = body->packets[body->count++];
        if (value == 23 || value == 24) {
            unsigned bank = value - 23;
            if (!rectangle_pose(s, pending[bank], packet)) return 0;
            pending[bank] = NULL;
        } else {
            const uint8_t *pose = skill_bytes(s, value, PACKET);
            if (!pose) return 0;
            memcpy(packet, pose, PACKET);
        }
    }
    return 0;
}

static int read_empty(Body *body, const ForestPlayerArchive *resident, uint32_t table, uint32_t *rig)
{
    uint32_t address;
    uint8_t program[4 * (MAX_KEYS + 4)];
    if (!table_entry(resident, table, EMPTY_SLOT, &address) ||
        !forest_player_archive_read(resident, address, program, sizeof program) ||
        u32le(program) != 4 || u32le(program + 8) != 19) return 0;
    *rig = u32le(program + 4);
    for (unsigned i = 3; i < sizeof program / 4; ++i) {
        uint32_t key = u32le(program + 4 * i);
        if (!key) return body->count != 0;
        if (body->count == MAX_KEYS || !forest_player_archive_read(resident, key, body->packets[body->count], PACKET))
            return 0;
        ++body->count;
    }
    return 0;
}

BfmWoodsShrinkMotion *bfm_woods_shrink_motion_open(const MusashiDiscMedia *disc,
                                                  uint32_t primary_table, uint32_t face_table)
{
    ForestPlayerArchive resident;
    if (!disc || !forest_player_archive_open(disc, &resident)) return NULL;
    BfmWoodsShrinkMotion *motion = calloc(1, sizeof *motion);
    Skill skill = {0};
    uint32_t rig;
    unsigned windup, release;
    if (!motion || !read_empty(&motion->body[BFM_SHRINK_EMPTY], &resident, primary_table, &rig) ||
        !mount_skill(disc, &resident, &skill) ||
        !read_skill_body(&motion->body[BFM_SHRINK_WINDUP], &skill, UINT32_C(0x800cc01c), rig, 0, &windup) ||
        !read_skill_body(&motion->body[BFM_SHRINK_RELEASE], &skill, UINT32_C(0x800cc0d8), rig, 1, &release) ||
        !read_face(&motion->face[BFM_SHRINK_WINDUP], &resident, face_table, windup) ||
        !read_face(&motion->face[BFM_SHRINK_RELEASE], &resident, face_table, release) ||
        !read_face(&motion->face[BFM_SHRINK_EMPTY], &resident, face_table, 0x1a) ||
        !read_face(&motion->face[BFM_SHRINK_RECOVER_FACE], &resident, face_table, 0x11)) {
        free(motion);
        motion = NULL;
    }
    free(skill.code);
    free(skill.tiles);
    return motion;
}

void bfm_woods_shrink_motion_free(BfmWoodsShrinkMotion *motion)
{
    free(motion);
}

unsigned bfm_woods_shrink_motion_count(const BfmWoodsShrinkMotion *motion, BfmWoodsShrinkClip clip)
{
    return motion && (unsigned)clip < 3 ? motion->body[clip].count : 0;
}

const uint8_t *bfm_woods_shrink_motion_pose(const BfmWoodsShrinkMotion *motion,
                                           BfmWoodsShrinkClip clip, unsigned key)
{
    return key < bfm_woods_shrink_motion_count(motion, clip) ? motion->body[clip].packets[key] : NULL;
}

unsigned bfm_woods_shrink_motion_face_slot(const BfmWoodsShrinkMotion *motion, BfmWoodsShrinkClip clip)
{
    return motion && (unsigned)clip < 4 ? motion->face[clip].slot : 0;
}

uint32_t bfm_woods_shrink_motion_face_word(const BfmWoodsShrinkMotion *motion, BfmWoodsShrinkClip clip)
{
    return motion && (unsigned)clip < 4 ? motion->face[clip].word : 0;
}

int bfm_woods_shrink_motion_face(const BfmWoodsShrinkMotion *motion, BfmWoodsShrinkClip clip,
                                uint16_t tiles[512][1024])
{
    if (!motion || (unsigned)clip >= 4 || !tiles) return 0;
    const Face *face = &motion->face[clip];
    if (face->word) return 1;
    for (unsigned i = 0; i < 2; ++i) {
        const FaceRect *r = &face->rect[i];
        for (unsigned y = 0; y < r->height; ++y)
            for (unsigned x = 0; x < r->width; ++x)
                tiles[r->y + y][r->x + x] = u16le(r->pixels + 2 * (y * r->width + x));
    }
    return 1;
}
