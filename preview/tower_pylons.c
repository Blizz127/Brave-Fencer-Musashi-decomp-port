/* SPDX-License-Identifier: MIT */
#include "tower_pylons.h"
#include "../pc_port/forest_player_bytes.h"

enum { FULL_HEALTH = 4, FRACTURE_TICKS = 21, PRESS_TICKS = 30 };

int bfm_tower_pylons_load(BfmTowerPylons *p, const uint8_t *scene, size_t bytes)
{
    const size_t box = UINT32_C(0x80186f5c) - UINT32_C(0x80128158);
    const size_t rows = UINT32_C(0x8018a71c) - UINT32_C(0x80128158);
    if (!p || !scene || bytes < rows + 4*20 || bytes < box + 16 || u32le(scene+box)) return 0;
    *p = (BfmTowerPylons){0};
    for (unsigned k = 0; k < 3; ++k) {
        /* The room registers a scaled, rotated box. Apply its model scale
         * here so the generic segment test uses a unit-scale yaw basis. */
        int lo = s16le(scene + box + 4 + 4*k) * 7;
        int hi = s16le(scene + box + 6 + 4*k) * 7;
        if (lo < INT16_MIN*4 || hi > INT16_MAX*4 || lo > hi) return 0;
        p->box.low[k] = (int16_t)(lo/4 - (lo%4 < 0));
        p->box.high[k] = (int16_t)(hi/4 - (hi%4 < 0));
    }
    for (unsigned i = 0; i < BFM_TOWER_POSTS; ++i) {
        const uint8_t *r = scene + rows + 20*i;
        if (u16le(r+6) != 20 || u16le(r+8) != i) return 0;
        int16_t center[3] = {s16le(r), s16le(r+2), s16le(r+4)};
        BfmTowerPylon *post = &p->post[i];
        bfm_forest_actor_root(u16le(r+12), center, &post->body);
        for (unsigned k = 0; k < 3; ++k) {
            post->home[k] = center[k];
            int scaled = post->body.rotation[k][0] * 7;
            int moved = (scaled/4 - (scaled%4 < 0)) * 4;
            post->shake_offset[k] = (int16_t)(moved/4096 - (moved%4096 < 0));
        }
        post->frame.remaining = FULL_HEALTH;
        for (unsigned frame = 0; frame < 3; ++frame) p->history[frame][i] = post->frame;
    }
    return 1;
}

int bfm_tower_pylons_hit(BfmTowerPylons *p, unsigned stroke, const int16_t start[3],
                         const int16_t end[3], int16_t contact[3], int16_t normal[3])
{
    if (!p || !stroke) return 0;
    for (unsigned i = 0; i < BFM_TOWER_POSTS; ++i) {
        BfmTowerPylon *post = &p->post[i];
        int16_t center[3];
        if (!post->frame.remaining || post->last_stroke == stroke) continue;
        if (!bfm_forest_guard_contact_point(&p->box, &post->body, start, end, center, contact, normal))
            continue;
        --post->frame.remaining;
        post->frame.phase = BFM_TOWER_FRACTURE;
        post->frame.ticks = FRACTURE_TICKS;
        post->last_stroke = stroke;
        post->fresh_hit = 1;
        return 1;
    }
    return 0;
}

int bfm_tower_pylons_slide(const BfmTowerPylons *p, const int16_t from[3],
                           const int16_t to[3], int16_t result[3])
{
    if (!p) return 0;
    for (unsigned i = 0; i < BFM_TOWER_POSTS; ++i) {
        const BfmTowerPylon *post = &p->post[i];
        if (post->frame.phase <= BFM_TOWER_FRACTURE &&
            bfm_forest_body_slide(&p->box, &post->body, from, to, result)) return 1;
    }
    return 0;
}

static int32_t fragment_speed(BfmGameplayRun *run)
{
    int32_t speed = (int32_t)bfm_gameplay_random(run) * 16;
    return bfm_gameplay_random(run) & 1 ? speed : -speed;
}

static void fracture_tick(BfmTowerPylon *post, BfmGameplayRun *run)
{
    BfmTowerPylonFrame *frame = &post->frame;
    if (post->fresh_hit) {
        post->fresh_hit = 0;
        post->fragment_velocity[0] = fragment_speed(run);
        post->fragment_velocity[1] = -12 * 65536;
        post->fragment_velocity[2] = fragment_speed(run);
        for (unsigned k = 0; k < 3; ++k) {
            post->fragment_distance[k] = (int32_t)((uint32_t)post->fragment_distance[k] & 65535);
            frame->fragment[k] = 0;
        }
        return;
    }
    frame->shake_x = (frame->ticks & 1u) ? 4 : 0;
    if (!--frame->ticks) {
        frame->shake_x = 0;
        frame->phase = frame->remaining ? BFM_TOWER_COLUMN : BFM_TOWER_EXPOSED;
        return;
    }
    post->fragment_velocity[1] += 3 * 32768;
    for (unsigned k = 0; k < 3; ++k) {
        int32_t d = (post->fragment_distance[k] += post->fragment_velocity[k]);
        frame->fragment[k] = (int16_t)(d / 65536 - (d % 65536 < 0));
    }
}

static int64_t distance_squared(const BfmForestTransform *root, const int16_t player[3])
{
    int64_t total = 0;
    for (unsigned k = 0; k < 3; ++k) {
        int64_t d = player[k] - root->translation[k];
        total += d*d;
    }
    return total;
}

void bfm_tower_pylons_tick(BfmTowerPylons *p, const int16_t player[3], BfmGameplayRun *run)
{
    for (unsigned i = 0; i < BFM_TOWER_POSTS; ++i) {
        BfmTowerPylon *post = &p->post[i];
        BfmTowerPylonFrame *f = &post->frame;
        if (f->phase == BFM_TOWER_FRACTURE) {
            fracture_tick(post, run);
        } else if (f->phase == BFM_TOWER_EXPOSED) {
            int64_t dist = distance_squared(&post->body, player);
            if (post->note_delay) --post->note_delay;
            if (!p->note_started && dist <= 128*128) {
                p->note_started = 1;
                post->note_delay = PRESS_TICKS;
            } else if (!post->note_delay && p->note_closed && !p->pressing && dist <= 24*24) {
                p->pressing = 1;
                f->phase = BFM_TOWER_PRESSING;
                f->ticks = PRESS_TICKS;
                f->plate_yaw = 0;
            }
        } else if (f->phase == BFM_TOWER_PRESSING) {
            --f->ticks;
            f->plate_yaw = (f->plate_yaw + 64*(PRESS_TICKS-f->ticks)) & 4095;
            if (!f->ticks) {
                f->plate_yaw = 0;
                f->phase = BFM_TOWER_CREDITED;
                p->pressing = 0;
                ++p->credits;
            }
        }
        /* The next blade or body query sees the last committed model root. */
        for (unsigned k = 0; k < 3; ++k)
            post->body.translation[k] = post->home[k] + (f->shake_x ? post->shake_offset[k] : 0);
        p->history[p->next_frame][i] = *f;
    }
    p->next_frame = (p->next_frame + 1) % 3;
}

unsigned bfm_tower_pylons_broken(const BfmTowerPylons *p)
{
    unsigned broken = 0;
    for (unsigned i = 0; i < BFM_TOWER_POSTS; ++i)
        if (p->post[i].frame.remaining < 3) broken |= 1u << i;
    return broken;
}

unsigned bfm_tower_lit_nodes(const BfmTowerPylonFrame posts[BFM_TOWER_POSTS])
{
    unsigned lit = 0;
    for (unsigned i = 0; i < BFM_TOWER_POSTS; ++i) {
        if (posts[i].remaining >= 3 &&
            (posts[(i+1)%4].remaining >= 3 || posts[(i+3)%4].remaining >= 3)) lit |= 1u << i;
    }
    return lit;
}
