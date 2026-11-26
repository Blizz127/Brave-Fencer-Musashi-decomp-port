/* SPDX-License-Identifier: MIT */
#include "bedroom_motion.h"
#include <stdlib.h>

enum { PART_POSE_BYTES = 256 };
static const uint32_t scene_base = 0x80128158u, main_base = 0x800cedf8u;

typedef struct {
    uint32_t base;
    const uint8_t *bytes;
    size_t length;
} Region;

typedef struct {
    uint32_t program;
    unsigned cursor, remaining, interval;
    int ended;
} Channel;

enum Activity { QUIET, TILTING, UNROLLING, RISING, YAWNING, STANDING, RETURNING, STARTING_RUN };

struct BfmBedroomMotion {
    Region main, scene;
    uint32_t body_bank, face_bank;
    Channel body, face;
    enum Activity activity;
    unsigned pitch, angular_speed;
    uint32_t face_upload;
    BfmBedroomPose frame;
};

static const uint8_t *region(const Region *r, uint32_t address, size_t length)
{
    size_t offset;
    if (address < r->base) return NULL;
    offset = address - r->base;
    if (offset > r->length || length > r->length - offset) return NULL;
    return r->bytes + offset;
}

static const uint8_t *data(const BfmBedroomMotion *m, uint32_t address, size_t length)
{
    return region(address >= scene_base ? &m->scene : &m->main, address, length);
}

static int word(const BfmBedroomMotion *m, uint32_t address, uint32_t *out)
{
    const uint8_t *p = data(m, address, 4);
    if (!p || (address & 3u)) return 0;
    *out = p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
    return 1;
}

static void start(Channel *channel, uint32_t program)
{
    *channel = (Channel){.program=program, .remaining=1, .interval=1};
}

static int select_slot(BfmBedroomMotion *m, Channel *channel, uint32_t bank, unsigned index)
{
    uint32_t program;
    if (bank > UINT32_MAX - index * 4 || !word(m, bank + index * 4, &program) ||
        !data(m, program, 4)) return 0;
    if (channel->program != program) start(channel, program);
    return 1;
}

static int next_word(const BfmBedroomMotion *m, Channel *channel, uint32_t *out)
{
    if (channel->cursor > 255 || channel->program > UINT32_MAX - channel->cursor * 4)
        return 0;
    return word(m, channel->program + channel->cursor++ * 4, out);
}

/* Body animation and facial texture share stop, restart and frame-duration
 * commands. Each channel can consume multiple controls before emitting a frame. */
static int advance(BfmBedroomMotion *m, Channel *channel, int body)
{
    if (!channel->remaining || --channel->remaining) return 1;
    channel->ended = 0;
    for (unsigned budget = 0; budget < 256; ++budget) {
        uint32_t command, operand;
        if (!next_word(m, channel, &command)) return 0;
        switch (command) {
        case 0: /* End and keep the last frame. */
        case 2: /* Hold; the action can resume at the following command. */
            channel->ended = 1;
            return 1;
        case 1:
            channel->cursor = 0;
            channel->ended = 1;
            break;
        case 3:
            if (!next_word(m, channel, &operand) || !operand || operand > 255) return 0;
            channel->interval = operand;
            break;
        case 4: /* Body resource declaration. */
            if (!body || !next_word(m, channel, &operand) || !data(m, operand, 4)) return 0;
            break;
        case 8: /* Running foot contact. The pose follows in the same update. */
            if (!body) return 0;
            break;
        case 15: /* Select a facial texture animation with this body action. */
            if (!body || !next_word(m, channel, &operand) || operand > 255 ||
                !select_slot(m, &m->face, m->face_bank, operand)) return 0;
            break;
        case 17: /* Sheathe equipped weapons for this motion. */
        case 19:
            if (!body) return 0;
            break;
        default:
            if (body) {
                m->frame.pose = data(m, command, PART_POSE_BYTES);
                if (!m->frame.pose) return 0;
            } else {
                if ((command & 0xfffffff8u) == 0x40000000u) {
                    m->frame.face_word = command;
                    m->face_upload = 0;
                } else {
                    if (!region(&m->main, command, 16)) return 0;
                    m->frame.face_word = 0;
                    m->face_upload = command;
                }
            }
            channel->remaining = channel->interval;
            return 1;
        }
    }
    return 0;
}

/* Selecting a new body pose consumes its first frame immediately. The
 * ordinary player update subsequently advances both channels once more. */
static int select_body(BfmBedroomMotion *m, unsigned index)
{
    uint32_t previous = m->body.program;
    if (!select_slot(m, &m->body, m->body_bank, index)) return 0;
    return previous == m->body.program || advance(m, &m->body, 1);
}

static int pair(BfmBedroomMotion *m, unsigned body, unsigned face)
{
    return select_body(m, body) &&
           select_slot(m, &m->face, m->face_bank, face);
}

BfmBedroomMotion *bfm_bedroom_motion_open(const uint8_t *scene, size_t scene_bytes,
                                          const uint8_t *main, size_t main_bytes)
{
    if (!scene || !main) return NULL;
    BfmBedroomMotion *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    m->scene = (Region){scene_base, scene, scene_bytes};
    m->main = (Region){main_base, main, main_bytes};
    uint32_t room;
    if (!word(m, 0x80182364u, &room) || !region(&m->scene, room, 0x54) ||
        !word(m, room + 0x4c, &m->body_bank) || !word(m, room + 0x50, &m->face_bank) ||
        !region(&m->main, m->body_bank, 40 * 4) || !region(&m->main, m->face_bank, 35 * 4)) {
        free(m);
        return NULL;
    }
    return m;
}

static int update(BfmBedroomMotion *m, BfmBedroomAction action)
{
    m->frame.finished = 0;
    switch (action) {
    case BFM_BEDROOM_LIE:
        if (!pair(m, 38, 30)) return 0;
        m->pitch = 4073;
        m->angular_speed = 0;
        m->activity = TILTING;
        break;
    case BFM_BEDROOM_RISE:
        start(&m->body, 0x80185d9cu);
        if (!advance(m, &m->body, 1)) return 0;
        m->pitch = 3072;
        m->activity = RISING;
        break;
    case BFM_BEDROOM_YAWN:
        m->frame.draw_current = 1;
        if (!pair(m, 36, 29)) return 0;
        m->activity = YAWNING;
        break;
    case BFM_BEDROOM_WALK:
        if (!pair(m, 6, 33)) return 0;
        m->activity = STARTING_RUN;
        break;
    case BFM_BEDROOM_FALL:
        /* Horizontal travel continues as Musashi leaves the mattress. */
        if (!pair(m, 15, 34)) return 0;
        m->activity = QUIET;
        break;
    case BFM_BEDROOM_LAND:
        if (!select_body(m, 16)) return 0;
        m->activity = QUIET;
        break;
    case BFM_BEDROOM_STAND:
        if (!pair(m, 4, 17)) return 0;
        m->activity = STANDING;
        break;
    case BFM_BEDROOM_REST:
        m->frame.draw_current = 0;
        if (!pair(m, 0, 17)) return 0;
        m->activity = QUIET;
        break;
    case BFM_BEDROOM_SEARCH:
        m->frame.draw_current = 1;
        if (!select_body(m, 34)) return 0;
        m->activity = STANDING;
        break;
    case BFM_BEDROOM_UPRIGHT:
        m->pitch = 0;
        break;
    case BFM_BEDROOM_NONE:
        break;
    default:
        return 0;
    }

    if (m->activity == TILTING) {
        /* At 1.75 angular units per tick squared, Musashi settles on his side. */
        m->angular_speed += 7;
        unsigned pitch = m->pitch - m->angular_speed / 4;
        if (pitch < 3072) {
            m->pitch = 3072;
            m->body.remaining = 1;
            m->body.ended = 0;
            m->activity = UNROLLING;
        } else m->pitch = pitch;
    } else if (m->activity == STARTING_RUN && action == BFM_BEDROOM_NONE) {
        /* The scripted full analog input reaches running speed on the
         * first ground update following locomotion entry. */
        if (!select_body(m, 8)) return 0;
        m->activity = QUIET;
    } else if (m->activity == RETURNING) {
        /* The scene marks the scripted player done. Normal stance selection
         * resumes on the following player update. */
        if (!pair(m, 0, 17)) return 0;
        m->activity = QUIET;
    } else if (m->body.ended && m->activity != QUIET) {
        if (m->activity == UNROLLING && !pair(m, 39, 31)) return 0;
        if (m->activity == YAWNING && !select_body(m, 37)) return 0;
        if (m->activity == STANDING) {
            if (!select_body(m, 0)) return 0;
            m->frame.draw_current = 0;
        }
        m->frame.finished = m->activity != STANDING;
        m->activity = m->activity == RISING ? RETURNING : QUIET;
    }
    if (!advance(m, &m->body, 1) || !advance(m, &m->face, 0)) return 0;
    m->frame.pitch = m->pitch;
    return 1;
}

int bfm_bedroom_motion_step(BfmBedroomMotion *motion, BfmBedroomAction action,
                            BfmBedroomPose *frame)
{
    if (!motion || !frame) return 0;
    BfmBedroomMotion next = *motion;
    if (!update(&next, action)) return 0;
    *motion = next;
    *frame = next.frame;
    return 1;
}

static unsigned halfword(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

typedef struct {
    unsigned x, y, width, height;
    const uint8_t *pixels;
} FaceUpload;

int bfm_bedroom_motion_face(const BfmBedroomMotion *m, uint16_t tiles[512][1024])
{
    if (!m || !tiles) return 0;
    if (!m->face_upload) return 1;
    /* The mounted normal facial material has a CLUT and an indexed image.
     * Validate the complete transfer before changing the caller's VRAM. */
    const uint8_t *header = region(&m->main, m->face_upload, 36);
    if (!header || header[32] != 0xff) return 0;
    FaceUpload items[2];
    for (unsigned i = 0; i < 2; ++i) {
        const uint8_t *h = header + i * 16;
        FaceUpload *u = &items[i];
        u->x = halfword(h+4); u->y = halfword(h+6);
        u->width = halfword(h+8); u->height = halfword(h+10);
        uint32_t pixels;
        if (!word(m, m->face_upload + i * 16 + 12, &pixels) ||
            u->width == 0 || u->height == 0 ||
            u->width > 1024 || u->x > 1024 - u->width ||
            u->height > 512 || u->y > 512 - u->height) return 0;
        u->pixels = region(&m->main, pixels, 2 * u->width * u->height);
        if (!u->pixels) return 0;
    }
    for (unsigned i = 0; i < 2; ++i) {
        const FaceUpload *u = &items[i];
        for (unsigned y = 0; y < u->height; ++y)
            for (unsigned x = 0; x < u->width; ++x)
                tiles[u->y + y][u->x + x] = halfword(u->pixels + 2 * (y*u->width + x));
    }
    return 1;
}

void bfm_bedroom_motion_free(BfmBedroomMotion *m)
{
    free(m);
}
