/* SPDX-License-Identifier: MIT */
#include "woods_earth_shrine_release.h"
#include "../pc_port/forest_player_bytes.h"

#include <string.h>

/* US SC02 room 11. These addresses identify mounted game-language records
 * and named callbacks used by this child. All records remain on the disc. */
#define ROOM_BASE UINT32_C(0x80128158)
#define SCROLL_BOUNDS UINT32_C(0x801959a8)
#define UNROLL_SEQUENCE UINT32_C(0x80195a94)
#define PROGRAM UINT32_C(0x80195b5c)
#define SHAKE_RECORD UINT32_C(0x80195cac)
#define MESSAGE_START UINT32_C(0x80196960)
#define MESSAGE_END UINT32_C(0x80196f14)

typedef struct {
    BfmEarthShrineRelease *s;
    BfmEarthReleaseHost host;
    void *context;
} Child;

enum { WAIT_NONE, WAIT_FLOATING, WAIT_PLAYER, WAIT_MESSAGE, WAIT_RETURN };

static const uint8_t *record(const BfmEarthShrineRelease *s, uint32_t address, size_t size) {
    if (address < ROOM_BASE) return NULL;
    size_t offset = address - ROOM_BASE;
    if (offset > s->bytes || size > s->bytes - offset) return NULL;
    return s->scene + offset;
}

static int event(Child c, BfmEarthReleaseEvent what, unsigned kind, uint32_t operand,
                 const uint8_t *data, size_t size) {
    return c.host(c.context, c.s, what, kind, operand, data, size);
}

static int signal(Child c, BfmEarthReleaseEvent what, unsigned kind) {
    return event(c, what, kind, 0, NULL, 0);
}

static void crest(Child c, unsigned stage) {
    if (c.s->crest_present) signal(c, BFM_EARTH_CREST_STAGE, stage);
}

static int countdown(unsigned *timer) {
    if (*timer) --*timer;
    return *timer == 0;
}

static unsigned unroll_duration(const BfmEarthShrineRelease *s) {
    const uint8_t *sequence = record(s, UNROLL_SEQUENCE, 17 * 8);
    if (!sequence) return 0;
    unsigned fields = 0;
    for (unsigned i = 0; i < 16; ++i) {
        const uint8_t *entry = sequence + 8 * i;
        unsigned hold = u32le(entry + 4);
        if (!record(s, u32le(entry), 12) || !hold || hold > 0x7fff) return 0;
        fields += hold;
    }
    return u32le(sequence + 128) == 0 && u32le(sequence + 132) == 0xfffe ? fields : 0;
}

static int ready(Child c) {
    switch (c.s->waiting) {
    case WAIT_FLOATING:
        return c.s->stage == BFM_EARTH_FLOATING;
    case WAIT_PLAYER:
        if (!signal(c, BFM_EARTH_PLAYER_READY, 0)) return 0;
        if (c.s->award_pending && !signal(c, BFM_EARTH_AWARD_CONFIRMED, 0)) return 0;
        c.s->award_pending = 0;
        return 1;
    case WAIT_MESSAGE:
        return signal(c, BFM_EARTH_MESSAGE_CLOSED, 0);
    case WAIT_RETURN:
        return signal(c, BFM_EARTH_LOAD_RETURN, 0x18);
    default:
        return 1;
    }
}

/* Execute consecutive immediate room commands. A field wait is installed on
 * admission and cannot complete earlier than the following effective update. */
static int run_program(Child c) {
    BfmEarthShrineRelease *s = c.s;
    if (s->script_done || !countdown(&s->delay) || !ready(c)) return 1;
    s->waiting = WAIT_NONE;
    while (s->cursor < SHAKE_RECORD - PROGRAM) {
        const uint8_t *row = record(s, PROGRAM + (uint32_t)s->cursor, 8);
        if (!row) return 0;
        unsigned op = u32le(row);
        uint32_t operand = u32le(row + 4);
        BfmEarthReleaseEvent what = BFM_EARTH_FIELD_COMMAND;
        const uint8_t *data = NULL;
        size_t size = 0;
        switch (op) {
        case 0x02:
            s->script_done = 1;
            return 1;
        case 0x0f:
            s->delay = operand;
            s->cursor += 8;
            return 1;
        case 0x3e:
            if (operand == UINT32_C(0x80186290)) s->waiting = WAIT_FLOATING;
            else if (operand == UINT32_C(0x80186354)) s->waiting = WAIT_RETURN;
            else return 0;
            s->cursor += 8;
            return 1;
        case 0x40:
            s->waiting = WAIT_PLAYER;
            s->cursor += 8;
            return 1;
        case 0x00: case 0x01:
            if (operand < MESSAGE_START || operand >= MESSAGE_END) return 0;
            size = MESSAGE_END - operand;
            break;
        case 0x21:
            size = 16;
            break;
        case 0x3c:
            what = BFM_EARTH_AWARD_ACTION;
            size = 32;
            break;
        case 0x33:
            switch (operand) {
            case UINT32_C(0x801866e8): what = BFM_EARTH_BEFORE_AWARD; break;
            case UINT32_C(0x80186708): what = BFM_EARTH_AFTER_AWARD; break;
            case UINT32_C(0x80186374):
                s->light_override = 0;
                signal(c, BFM_EARTH_SPECIAL_PALETTE, 0);
                s->cursor += 8;
                continue;
            case UINT32_C(0x80186394):
                s->stage = BFM_EARTH_RESTORING;
                s->timer = 64;
                signal(c, BFM_EARTH_SOUND, 0x5ec);
                s->cursor += 8;
                continue;
            default: return 0;
            }
            break;
        case 0x04: case 0x07: case 0x0a: case 0x19: case 0x1d:
        case 0x20: case 0x46: case 0x47: case 0x4a: case 0x4b: case 0x57: case 0x58:
            break;
        default: return 0;
        }
        if (size) {
            data = record(s, operand, size);
            if (!data) return 0;
        }
        if (!event(c, what, op, operand, data, size)) return 1;
        s->cursor += 8;
        if (op == 0x3c) s->award_pending = 1;
        if (op == 0 || op == 1) {
            s->waiting = WAIT_MESSAGE;
            return 1;
        }
    }
    return 0;
}

int bfm_earth_shrine_release_begin(BfmEarthShrineRelease *s, const uint8_t *scene,
    size_t bytes, const int16_t xyz[3], BfmEarthReleaseHost host, void *context) {
    if (!s || s->stage != BFM_EARTH_SEALED || !scene || !xyz || !host) return 0;
    BfmEarthShrineRelease mounted = {.scene = scene, .bytes = bytes};
    mounted.animation_duration = unroll_duration(&mounted);
    if (!mounted.animation_duration || !record(&mounted, SCROLL_BOUNDS, 16) ||
        !record(&mounted, PROGRAM, SHAKE_RECORD - PROGRAM + 24) ||
        !record(&mounted, MESSAGE_START, MESSAGE_END - MESSAGE_START)) return 0;
    memcpy(mounted.xyz, xyz, sizeof(mounted.xyz));
    mounted.original_y = xyz[1];
    mounted.tablet_dx = 1;
    ++mounted.xyz[0];
    mounted.stage = BFM_EARTH_SHAKING;
    mounted.timer = 80;
    *s = mounted;
    Child c = {s, host, context};
    s->crest_present = !!signal(c, BFM_EARTH_CREST_STAGE, 1);
    if (!run_program(c)) return 0;
    signal(c, BFM_EARTH_SOUND, 0x5eb);
    event(c, BFM_EARTH_SCORE, 1, 0x30, NULL, 0);
    signal(c, BFM_EARTH_SOUND, 0x426);
    return 1;
}

static void move_y(BfmEarthShrineRelease *s) {
    s->speed_y += s->acceleration_y;
    s->y_fraction += s->speed_y;
    /* Keep the high signed half of the original 16.16 displacement. */
    int units = s->y_fraction / 65536 - (s->y_fraction % 65536 < 0);
    s->xyz[1] = (int16_t)(s->original_y + units);
}

static void bob(BfmEarthShrineRelease *s) {
    move_y(s);
    if (s->speed_y > 0x14000) s->acceleration_y = -0x1400;
    if (s->speed_y < -0x14000) s->acceleration_y = 0x1400;
}

static void shake_tablet(Child c) {
    BfmEarthShrineRelease *s = c.s;
    if (s->timer == 48) {
        s->light_override = 5;
        signal(c, BFM_EARTH_SPECIAL_PALETTE, 5);
    }
    if (s->timer == 16) crest(c, 2);
    int dx = (s->shake_ticks++ & 1) ? -3 : 3;
    s->tablet_dx += dx;
    s->xyz[0] += dx;
    if (!s->release_loaded) s->release_loaded = !!signal(c, BFM_EARTH_LOAD_RELEASE, 0x20);
    if (countdown(&s->timer) && s->release_loaded) {
        s->stage = BFM_EARTH_RISING;
        s->timer = 8;
        s->xyz[1] -= 160;
        s->xyz[2] -= 16;
        s->y_fraction = -160 * 65536;
        s->speed_y = -8 * 65536;
        s->scale[0] = 0x2c00;
        s->scale[1] = 0x1c00;
        signal(c, BFM_EARTH_SHATTER, 0);
        crest(c, 3);
        signal(c, BFM_EARTH_SOUND, 0x5ed);
    } else if (s->timer < 72 && !(s->timer & 3)) {
        signal(c, BFM_EARTH_DUST, 0);
    }
}

static void float_scroll(Child c, const int16_t player_xyz[3]) {
    BfmEarthShrineRelease *s = c.s;
    bob(s);
    if (!event(c, BFM_EARTH_TOUCH_SCROLL, 0, SCROLL_BOUNDS, record(s, SCROLL_BOUNDS, 16), 16))
        return;
    s->stage = BFM_EARTH_TAKEN;
    s->xyz[0] = player_xyz[0];
    s->xyz[2] = player_xyz[2];
    s->speed_y = 8 * 65536;
    s->acceleration_y = 0;
    crest(c, 0);
    s->crest_present = 0;
    signal(c, BFM_EARTH_SOUND, 0x635);
}

int bfm_earth_shrine_release_step(BfmEarthShrineRelease *s,
    const int16_t player_xyz[3], BfmEarthReleaseHost host, void *context) {
    if (!s || !host || !player_xyz) return 0;
    Child c = {s, host, context};
    switch (s->stage) {
    case BFM_EARTH_SEALED: case BFM_EARTH_RELEASED:
        return 1;
    case BFM_EARTH_SHAKING:
        shake_tablet(c);
        break;
    case BFM_EARTH_RISING:
        move_y(s);
        if (countdown(&s->timer)) {
            s->stage = BFM_EARTH_HOVERING;
            s->timer = 48;
            s->speed_y = 0;
            s->acceleration_y = 0x1400;
        }
        break;
    case BFM_EARTH_HOVERING:
        if (countdown(&s->timer)) {
            s->stage = BFM_EARTH_UNROLLING;
            s->timer = 16;
            s->scale[1] = 0x3800;
            crest(c, 4);
            if (s->crest_present) signal(c, BFM_EARTH_SOUND, 0x634);
        } else bob(s);
        break;
    case BFM_EARTH_UNROLLING:
        if (!countdown(&s->timer) && !(s->timer & 3)) signal(c, BFM_EARTH_TRAIL, 1);
        if (++s->animation_age == s->animation_duration) {
            s->stage = BFM_EARTH_SETTLING;
            s->timer = 4;
            s->speed_y = 0;
            s->acceleration_y = 0;
            crest(c, 5);
        }
        break;
    case BFM_EARTH_SETTLING:
        if (s->timer) {
            if (countdown(&s->timer)) s->speed_y = 65536;
        } else if (s->xyz[1] < s->original_y - 160) {
            move_y(s);
        } else {
            s->stage = BFM_EARTH_FLOATING;
            s->speed_y = 0;
            s->acceleration_y = 0x1400;
        }
        break;
    case BFM_EARTH_FLOATING:
        if (!run_program(c)) return 0;
        float_scroll(c, player_xyz);
        return 1;
    case BFM_EARTH_TAKEN:
        if (s->scale[0] > 0) {
            move_y(s);
            s->scale[0] -= 0x2c00 / 8;
            s->scale[1] -= 0x3800 / 8;
        }
        break;
    case BFM_EARTH_RESTORING:
        if (!run_program(c)) return 0;
        if (!(s->timer & 15))
            event(c, BFM_EARTH_RELEASE_SHAKE, 0, SHAKE_RECORD, record(s, SHAKE_RECORD, 24), 24);
        if (countdown(&s->timer)) s->stage = BFM_EARTH_WAITING_EXIT;
        return 1;
    case BFM_EARTH_WAITING_EXIT:
        if (!run_program(c)) return 0;
        if (s->script_done && signal(c, BFM_EARTH_CAMERA_READY, 0)) s->stage = BFM_EARTH_RELEASED;
        return 1;
    default:
        return 0;
    }
    return run_program(c);
}
