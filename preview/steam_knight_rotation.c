/* SPDX-License-Identifier: MIT */
#include "steam_knight_rotation.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

/* Both weapons' anchors precede each mounted body pose. */
int bfm_lumina_rotation_load(const MusashiDiscMedia *disc, BfmLuminaRotation *motion)
{
    ForestPlayerArchive archive;
    uint8_t commands[128], packed[16];
    BfmLuminaRotation clip = {0};
    if (!disc || !motion || !forest_player_archive_open(disc, &archive) ||
        !forest_player_archive_read(&archive, UINT32_C(0x800d5360), commands, sizeof commands))
        return 0;
    if (u32le(commands) != 4 || u32le(commands + 8) != 15 ||
        u32le(commands + 12) != 21 || u32le(commands + 16) != 19) return 0;
    int struck = 0;
    for (size_t i = 5; i < sizeof commands / 4; ++i) {
        uint32_t command = u32le(commands + 4*i);
        switch (command) {
        case 0:
            if (!clip.held || struck || clip.held + 1 >= clip.count) return 0;
            *motion = clip;
            return 1;
        case 7:  /* Weapon motion marker. */
        case 18: /* Draw Lumina. */
        case 19: /* Start or resume body channel. */
            break;
        case 20:
            struck = !struck;
            break;
        case 2:
            if (!struck || clip.held || !clip.count) return 0;
            clip.held = clip.count - 1;
            break;
        default: {
            if (struck || (command & 3) || command > UINT32_MAX - sizeof packed ||
                clip.count == BFM_ROTATION_KEYS ||
                !forest_player_archive_read(&archive, command, packed, sizeof packed)) return 0;
            BfmLuminaRotationKey *key = &clip.keys[clip.count++];
            key->body = command + sizeof packed;
            memcpy(key->anchors[0], packed, 6);
            memcpy(key->anchors[1], packed + 8, 6);
            break;
        }
        }
    }
    return 0;
}

enum { SPIN_UPDATES = 11, FIRST_TURN = 286, ACCELERATION = 30 };
static int turn(unsigned n)
{
    return n ? (int)(n * FIRST_TURN + n * (n - 1) / 2 * ACCELERATION) : 0;
}
const BfmLuminaRotationKey *bfm_lumina_rotation_pose(const BfmLuminaRotation *clip,
    unsigned age, int16_t initial, int16_t *heading)
{
    if (!clip || !clip->held || !heading) return NULL;
    unsigned key = age > 2 ? age - 2 : 0;
    unsigned release = clip->held + SPIN_UPDATES + 2;
    unsigned elapsed = key > clip->held ? key - clip->held : 0;
    if (elapsed > SPIN_UPDATES - 1) elapsed = SPIN_UPDATES - 1;
    *heading = (int16_t)((initial + turn(elapsed)) & 4095);
    if (key > clip->held) key = clip->held;
    if (age >= release) key = clip->held + 1 + age - release;
    return key < clip->count ? &clip->keys[key] : NULL;
}

static void anchor_world(const BfmForestTransform *root, const int8_t anchor[3],
                         int16_t point[3])
{
    for (unsigned axis = 0; axis < 3; ++axis) {
        int32_t dot = 0;
        for (unsigned i = 0; i < 3; ++i) dot += root->rotation[axis][i] * anchor[i];
        point[axis] = (int16_t)(root->translation[axis] + dot / 4096 - (dot % 4096 < 0));
    }
}

static size_t blades(const BfmLuminaRotationKey *key, int heading, const int16_t feet[3],
                     BfmSteamFlameRay out[2])
{
    BfmForestTransform root;
    bfm_forest_actor_root((unsigned)heading, feet, &root);
    for (unsigned i = 0; i < 2; ++i) {
        anchor_world(&root, key->anchors[i][1], out[i].from);
        anchor_world(&root, key->anchors[i][0], out[i].to);
    }
    return 2;
}

size_t bfm_lumina_rotation_rays(const BfmLuminaRotation *clip, unsigned age,
    const int16_t feet[3], int16_t initial, BfmSteamFlameRay rays[BFM_ROTATION_RAYS])
{
    if (!clip || !clip->held || !feet || !rays || age <= clip->held) return 0;
    unsigned n = age - (clip->held + 1);
    if (n >= SPIN_UPDATES) return 0;
    const BfmLuminaRotationKey *key = &clip->keys[clip->held];
    if (!n) {
        blades(key - 1, initial, feet, rays);
        blades(key, initial, feet, rays + 2);
        return 4;
    }
    /* Subdivide the swept angle so the long blade travels less than three
     * world units between samples. Both weapons keep their actual length. */
    int from = turn(n-1), to = turn(n);
    int steps = (to - from + 15) / 16;
    if ((size_t)(steps + 1) * 2 > BFM_ROTATION_RAYS) return 0;
    size_t count = 0;
    for (int i = 0; i <= steps; ++i)
        count += blades(key, initial + from + (to-from)*i/steps, feet, rays + count);
    return count;
}
