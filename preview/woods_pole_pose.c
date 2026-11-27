/* SPDX-License-Identifier: MIT */
#include "woods_pole_pose.h"
#include "../pc_port/forest_player_bytes.h"

static const uint32_t resident_base = UINT32_C(0x800cedf8);
enum { SHEATHE_FUSION = 1, SHEATHE_LUMINA = 2, POSE_BYTES = 256 };

static int resident_range(uint32_t address, size_t length, size_t resident_bytes)
{
    if (address < resident_base || (address & 3)) return 0;
    size_t offset = address - resident_base;
    return offset <= resident_bytes && length <= resident_bytes - offset;
}

/* The grip programs describe a model and facial channel, then emit a run of
 * body keys. The climb loops with an externally driven clock. The slide holds
 * its single key while the field controller rotates the player. */
static int read_clip(BfmWoodsPoleClip *clip, const uint8_t *main, size_t bytes,
                     uint32_t program, int climb)
{
    if (!resident_range(program, 4, bytes)) return 0;
    size_t pos = program - resident_base;
    int model = 0, manual_clock = 0;
    while (bytes - pos >= 4) {
        const uint8_t *word = main + pos;
        uint32_t item = u32le(word);
        pos += 4;
        if (resident_range(item, POSE_BYTES, bytes)) {
            if (!clip->count) clip->keys = word;
            if (++clip->count > 255) return 0;
            continue;
        }
        if (item == 0 || item == 1) {
            if (!model || !clip->face || !clip->count) return 0;
            if (climb) return item == 1 && manual_clock;
            return item == 0 && clip->count == 1;
        }
        if (clip->count) return 0;
        switch (item) {
        case 3: /* Body-key duration. Zero hands the clock to the action. */
            if (bytes - pos < 4 || u32le(main + pos) != 0) return 0;
            pos += 4;
            manual_clock = 1;
            break;
        case 4: /* Mounted model declaration. */
            if (bytes - pos < 4 || !resident_range(u32le(main + pos), 4, bytes)) return 0;
            pos += 4;
            model = 1;
            break;
        case 15: /* Facial program in the room's shared bank. */
            if (bytes - pos < 4) return 0;
            clip->face = u32le(main + pos);
            if (!clip->face || clip->face >= 35) return 0;
            pos += 4;
            break;
        case 17: clip->sheathed |= SHEATHE_FUSION; break;
        case 19: clip->sheathed |= SHEATHE_LUMINA; break;
        default: return 0;
        }
    }
    return 0;
}

int bfm_woods_pole_pose_load(BfmWoodsPolePose *pose, const uint8_t *scene, size_t scene_bytes,
                             const uint8_t *main, size_t main_bytes)
{
    const size_t room = UINT32_C(0x801961e8) - UINT32_C(0x80128158);
    if (!pose || !scene || !main || scene_bytes < room + 0x50) return 0;
    uint32_t table = u32le(scene + room + 0x4c);
    if (!resident_range(table, 22 * 4, main_bytes)) return 0;
    const uint8_t *bank = main + (table - resident_base);
    BfmWoodsPolePose next = {.main = main};
    if (!read_clip(&next.climb, main, main_bytes, u32le(bank + 20 * 4), 1) ||
        !read_clip(&next.slide, main, main_bytes, u32le(bank + 21 * 4), 0)) return 0;
    *pose = next;
    return 1;
}

int bfm_woods_pole_pose_apply(BfmWoodsPolePose *pose, BfmBedroomMotion *ordinary,
                              BfmWoodsPoleMove move, int caught, int face_already_stepped,
                              BfmBedroomPose *frame)
{
    if (!pose || !ordinary || !frame || !pose->main || (!caught && !pose->attached) ||
        (move != BFM_POLE_HOLD && move != BFM_POLE_ASCEND && move != BFM_POLE_DESCEND)) return 0;
    BfmWoodsPolePose next = *pose;
    /* The face pre-step precedes the grip action. A newly selected body
     * requests its face for the following field, including the catch. */
    unsigned prior_face = next.sliding ? next.slide.face : next.climb.face;
    if (caught) prior_face = 0;
    if (caught) {
        next.attached = 1;
        next.key = next.sliding = next.armed = 0;
        next.sheathed = next.climb.sheathed;
    } else if (next.armed) {
        next.key = (next.key + 1) % next.climb.count;
    }
    unsigned sliding = move == BFM_POLE_HOLD ? next.sliding : move == BFM_POLE_DESCEND;
    if (sliding != next.sliding) next.key = 0;
    next.sliding = sliding;
    next.armed = move == BFM_POLE_ASCEND;
    const BfmWoodsPoleClip *clip = sliding ? &next.slide : &next.climb;
    next.sheathed |= clip->sheathed;
    if (!bfm_bedroom_motion_external_face(ordinary, prior_face, face_already_stepped, frame)) return 0;
    frame->pose = next.main + (u32le(clip->keys + 4 * next.key) - resident_base);
    if (next.sheathed & SHEATHE_FUSION) frame->show_weapon = 0;
    if (next.sheathed & SHEATHE_LUMINA) frame->show_lumina = 0;
    frame->draw_current = 1;
    *pose = next;
    return 1;
}
