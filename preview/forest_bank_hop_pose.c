/* SPDX-License-Identifier: MIT */
#include "forest_bank_hop_pose.h"

#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

enum { SECTOR = 2048, PACKET_BYTES = 256, MAX_KEYS = 16 };
static const uint32_t skill_origin = UINT32_C(0x800cae08);
static const uint32_t takeoff_program = UINT32_C(0x800cb1f4);
static const uint32_t air_program = UINT32_C(0x800cb234);

typedef struct {
    unsigned count, face_slot;
    const uint8_t *key[MAX_KEYS];
} HopClip;

enum HopPhase { INACTIVE, TAKEOFF, AIR };
struct BfmForestBankHopPose {
    HopClip takeoff, air;
    enum HopPhase phase;
    unsigned key;
    size_t bytes;
    uint8_t data[];
};

/* Locate the uncompressed ability section selected by the resident catalog.
 * Read the clip and pose data after its executable prefix. */
static int locate_poses(const MusashiDiscMedia *disc, size_t *offset, size_t *bytes)
{
    ForestPlayerArchive resident;
    uint8_t resource[8], directory[SECTOR], header[16];
    size_t total;
    if (!forest_player_archive_open(disc, &resident) ||
        !forest_player_archive_read(&resident, UINT32_C(0x800d3794), resource, sizeof resource) ||
        u32le(resource + 4) != 91 ||
        !musashi_disc_media_read_root_range(disc, "MAIN.CD", 0, directory, sizeof directory, &total))
        return 0;
    uint32_t member = u32le(resource), count = u32le(directory);
    if (member >= count || count > (SECTOR - 8) / 8) return 0;
    const uint8_t *entry = directory + 8 + member * 8;
    size_t sector = u32le(entry), remaining = u32le(entry + 4);
    if (!sector || sector > total / SECTOR) return 0;
    size_t cursor = sector * SECTOR;
    if (remaining > total - cursor) return 0;
    while (remaining >= SECTOR) {
        if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", cursor, header, sizeof header, &total) ||
            memcmp(header, "PAC", 4)) return 0;
        size_t length = u32le(header + 12);
        if (length < SECTOR || length > remaining) return 0;
        size_t sectors = (length - 1) / SECTOR + 1;
        if (sectors > remaining / SECTOR) return 0;
        if (header[4] == 1) {
            size_t prefix = SECTOR + takeoff_program - skill_origin;
            if (u32le(header + 4) != 1 || length <= prefix ||
                length - SECTOR > UINT32_C(0x800cedf8) - skill_origin) return 0;
            *offset = cursor + prefix;
            *bytes = length - prefix;
            return 1;
        }
        cursor += sectors * SECTOR;
        remaining -= sectors * SECTOR;
    }
    return 0;
}

static const uint8_t *mounted(const BfmForestBankHopPose *poses, uint32_t address, size_t bytes)
{
    if (address < takeoff_program || (address & 3)) return NULL;
    size_t offset = address - takeoff_program;
    return offset <= poses->bytes && bytes <= poses->bytes - offset ? poses->data + offset : NULL;
}

static int read_clip(const BfmForestBankHopPose *poses, uint32_t address, int loop, HopClip *clip)
{
    const uint8_t *header = mounted(poses, address, 24);
    /* Both clips declare the player's rig, choose a face, show Fusion and
     * sheathe Lumina before presenting direct, one-field body keys. */
    if (!header || u32le(header) != 4 || u32le(header + 4) != UINT32_C(0x800de2a4) ||
        u32le(header + 8) != 15 || u32le(header + 16) != 16 || u32le(header + 20) != 19)
        return 0;
    clip->face_slot = u32le(header + 12);
    if (!clip->face_slot || clip->face_slot > 255) return 0;
    for (unsigned i = 0; i <= MAX_KEYS; ++i) {
        const uint8_t *word = mounted(poses, address + 24 + 4 * i, 4);
        if (!word) return 0;
        uint32_t value = u32le(word);
        if (value <= 1) return value == (unsigned)loop && clip->count;
        if (i == MAX_KEYS || value <= air_program) return 0;
        const uint8_t *packet = mounted(poses, value, PACKET_BYTES);
        if (!packet) return 0;
        clip->key[clip->count++] = packet;
    }
    return 0;
}

BfmForestBankHopPose *bfm_forest_bank_hop_pose_open(const MusashiDiscMedia *disc)
{
    size_t offset, bytes, total;
    if (!locate_poses(disc, &offset, &bytes)) return NULL;
    BfmForestBankHopPose *poses = calloc(1, sizeof *poses + bytes);
    if (!poses) return NULL;
    poses->bytes = bytes;
    if (!musashi_disc_media_read_root_range(disc, "MAIN.CD", offset, poses->data, bytes, &total) ||
        !read_clip(poses, takeoff_program, 0, &poses->takeoff) ||
        !read_clip(poses, air_program, 1, &poses->air)) {
        free(poses);
        return NULL;
    }
    return poses;
}

void bfm_forest_bank_hop_pose_reset(BfmForestBankHopPose *poses)
{
    if (poses) {
        poses->phase = INACTIVE;
        poses->key = 0;
    }
}

void bfm_forest_bank_hop_pose_free(BfmForestBankHopPose *poses)
{
    free(poses);
}

int bfm_forest_bank_hop_pose_step(BfmForestBankHopPose *poses, int takeoff, uint32_t actor_flags,
                                 BfmBedroomMotion *ordinary, int face_already_stepped,
                                 BfmBedroomPose *body)
{
    if (!poses || !ordinary || !body || (poses->phase == INACTIVE && !takeoff)) return 0;
    enum HopPhase phase = poses->phase;
    const HopClip *clip = phase == AIR ? &poses->air : &poses->takeoff;
    unsigned key = poses->key;

    /* The common clock advances before the action. A new selection later in
     * this field displays key zero immediately. Re-selecting the current
     * takeoff during an early rebound keeps its existing animation clock. */
    enum HopPhase next_phase = phase;
    if (phase != INACTIVE && ++key == clip->count) {
        next_phase = AIR;
        key = 0;
    }
    if (takeoff && next_phase != TAKEOFF) {
        next_phase = TAKEOFF;
        key = 0;
    }

    /* The body selects a face after the facial tick, so its first facial
     * descriptor takes effect on the following field. */
    unsigned face = phase == INACTIVE ? 0 : clip->face_slot;
    if (!bfm_bedroom_motion_external_face(ordinary, face, face_already_stepped, body))
        return 0;
    clip = next_phase == AIR ? &poses->air : &poses->takeoff;
    body->pose = clip->key[key];
    body->hide_fusion = !!(actor_flags & 4);
    body->show_weapon = !body->hide_fusion;
    body->show_lumina = 0;
    body->draw_current = 1;
    poses->phase = next_phase;
    poses->key = key;
    return 1;
}
