/* SPDX-License-Identifier: MIT */
#include "retry_screen.h"
#include "forest_walk.h"
#include "forest_opening_data.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { LINES = 11, MAX_KEYS = 8, QUARTER_BACK = 3072, TURN_MASK = 4095,
       WORLD_CUTOFF = 20, MUSIC_WAIT = 70, SCORE_REQUEST = 1 + MUSIC_WAIT + 2,
       COLD_SCORE_READY = 28, POST_LINE_WAIT = 210, RELEASE_WAIT = 40 };
typedef enum {
    STAGGER, SPIN, PAUSE, FALL, BOUNCE, WAIT_FOR_LINE, SPEAK, RESTORE, WAIT_FOR_RELEASE, RELEASE, FADE, FINISHED, FAILED
} DefeatPhase;
typedef struct {
    BfmForestPose key[MAX_KEYS];
    unsigned count;
} DefeatClip;

struct BfmRetryScreen {
    const BfmForestField *field;
    BfmGameplayRun *run;
    ForestPlayerArchive archive;
    BfmForestCamera camera;
    BfmForestModelLighting light;
    int radius, orbit_pitch, orbit_yaw, offset[3];
    ForestVoice *voice;
    BfmForestMessage *page;
    uint32_t line[LINES + 1], visible;
    DefeatClip clip[2];
    unsigned clip_index, key, yaw, pitch, age, timer, generation;
    int velocity, shorten;
    int16_t feet[3];
    DefeatPhase phase;
    BfmRetryAudioEvent audio_event;
};

/* The shared player program names its model and successive frame records.
 * The first sixteen bytes of each frame are the animation header. */
static int load_clip(const ForestPlayerArchive *archive, uint32_t program, DefeatClip *clip)
{
    uint8_t words[(3 + MAX_KEYS + 1) * 4], packed[240];
    if (!forest_player_archive_read(archive, program, words, sizeof words) ||
        u32le(words) != 4 || u32le(words + 8) != 19) return 0;
    for (unsigned i = 0; i <= MAX_KEYS; ++i) {
        uint32_t frame = u32le(words + (3 + i) * 4);
        if (!frame) {
            clip->count = i;
            return i != 0;
        }
        if (i == MAX_KEYS || frame > UINT32_MAX - 16 ||
            !forest_player_archive_read(archive, frame + 16, packed, sizeof packed) ||
            !bfm_forest_pose_decode(packed, sizeof packed, &clip->key[i])) return 0;
    }
    return 0;
}

/* The same resident dialogue programs are indexed from the active overlay.
 * The field archives relocate the table to different addresses. */
static int read_resources(BfmRetryScreen *s, uint16_t room)
{
    uint32_t table;
    switch (room) {
    case 0x3008: table = 0x80188e8c; break;
    case 0x3009: table = 0x801858c8; break;
    case 0x300a: table = 0x80188bbc; break;
    case 0x300b: table = 0x8018cf6c; break;
    case 0x300d: table = 0x8018004c; break;
    default: return 0;
    }
    const uint8_t *mounted = bfm_forest_room_bytes(s->field, table, 0x9c + 3 * 16);
    if (!mounted) return 0;
    s->line[LINES] = 0x80110c3c;
    for (unsigned i = LINES; i > 0; --i) {
        uint32_t address = u32le(mounted + (i - 1) * 4);
        if ((address & 3u) || address >= s->line[i] || s->line[i] - address > 128) return 0;
        s->line[i - 1] = address;
    }
    for (unsigned n = 0; n < 3; ++n) {
        const uint8_t *lamp = mounted + 0x9c + 16 * n;
        int axis[3];
        uint32_t squared = 0;
        for (unsigned k = 0; k < 3; ++k) {
            uint32_t word = u32le(lamp + 4 * k);
            axis[k] = s16le(lamp + 4 * k);
            if (word != (uint32_t)axis[k]) return 0;
            squared += (uint32_t)(axis[k] * axis[k]);
        }
        if (!squared) return 0;
        unsigned shift = 0;
        while ((squared >> shift) > 255) shift += 2;
        int length = (int)sqrt((squared >> shift) << shift);
        for (unsigned k = 0; k < 3; ++k) {
            s->light.direction[n][k] = -axis[k] * 4096 / length;
            s->light.color[k][n] = lamp[12+k] * 4096 / 255;
        }
        s->light.ambient[n] = 2048;
    }
    return 1;
}

void bfm_retry_screen_update_outgoing_view(BfmRetryScreen *s, const BfmForestCamera *last_camera,
                                          const int32_t last_target[3])
{
    if (!s || !last_camera || !last_target || s->age >= WORLD_CUTOFF) return;
    s->camera = *last_camera;
    /* The incoming target's depth is the old camera arm. The third row
     * of the submitted view supplies its orientation. */
    const int16_t *forward = last_camera->rotation[2];
    int64_t depth = 0;
    for (unsigned k = 0; k < 3; ++k) {
        s->offset[k] = last_target[k] - s->feet[k];
        depth += (int64_t)forward[k] * last_target[k];
    }
    s->radius = last_camera->translation[2] + (int)(depth / 4096);
    const double turn = 4096.0 / 6.2831853071795864769;
    s->orbit_pitch = (int)lround(atan2(forward[1], hypot(forward[0], forward[2])) * turn);
    s->orbit_yaw = (int)lround(atan2(-forward[0], -forward[2]) * turn) & TURN_MASK;
}

unsigned bfm_retry_screen_yaw(const BfmRetryScreen *s) { return s ? s->yaw : 0; }

BfmRetryScreen *bfm_retry_screen_open(const MusashiDiscMedia *disc, const BfmForestField *field,
                                      uint16_t room_id, BfmGameplayRun *gameplay,
                                      const BfmForestCamera *last_camera, const int32_t last_target[3],
                                      const int16_t feet[3],
                                      unsigned yaw, uint32_t visible_parts)
{
    if (!disc || !field || !gameplay || !last_camera || !last_target || !feet ||
        room_id < 0x3008 || room_id > 0x300e) return NULL;
    BfmRetryScreen *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->field = field;
    s->run = gameplay;
    memcpy(s->feet, feet, sizeof s->feet);
    s->yaw = yaw & TURN_MASK;
    s->visible = visible_parts;
    bfm_retry_screen_update_outgoing_view(s, last_camera, last_target);
    if (!forest_player_archive_open(disc, &s->archive) ||
        !load_clip(&s->archive, 0x800d58d8, &s->clip[0]) ||
        !load_clip(&s->archive, 0x800d5904, &s->clip[1]) ||
        !read_resources(s, room_id) || !(s->voice = bfm_defeat_voice_load(disc))) {
        bfm_retry_screen_free(s);
        return NULL;
    }
    return s;
}

static int open_line(BfmRetryScreen *s)
{
    unsigned i = bfm_gameplay_random(s->run) % LINES;
    uint8_t program[128];
    size_t length = s->line[i+1] - s->line[i];
    if (length > sizeof program || length < 6 ||
        !forest_player_archive_read(&s->archive, s->line[i], program, length)) return 0;
    s->page = bfm_field_message_open(program, length);
    return s->page != NULL;
}

static void tumble(BfmRetryScreen *s, unsigned speed)
{
    /* Holding a face button advances only the current angular phase faster.
     * A newly entered phase starts on the next game update. */
    DefeatPhase initial = s->phase;
    for (unsigned i = 0; i < speed && initial == s->phase; ++i) {
        switch (s->phase) {
        case SPIN:
            s->yaw = (s->yaw + (s->velocity >> 16)) & TURN_MASK;
            s->velocity -= 0x3c000;
            if (s->velocity < 0) {
                s->phase = PAUSE;
                s->timer = 4;
            }
            break;
        case PAUSE:
            if (!--s->timer) {
                s->phase = FALL;
                s->velocity = 91 * 65536;
            }
            break;
        case FALL:
        case BOUNCE:
            s->pitch = (s->pitch - (s->velocity >> 16)) & TURN_MASK;
            s->velocity += 0x4c000;
            if (s->pitch < QUARTER_BACK) {
                s->pitch = QUARTER_BACK;
                if (s->phase == FALL) {
                    s->phase = BOUNCE;
                    s->velocity = -22 * 65536;
                } else {
                    s->phase = WAIT_FOR_LINE;
                    s->clip_index = 1;
                    s->key = 0;
                }
            }
            break;
        default:
            return;
        }
    }
}

static int approach(int delta, int divisor)
{
    while (divisor > 1 && delta / divisor == 0) divisor /= 2;
    return delta / divisor;
}

static int ease_angle(int from, int to)
{
    int delta = to - (from & TURN_MASK);
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    return (from + approach(delta, 4)) & TURN_MASK;
}

static int move_camera(BfmRetryScreen *s)
{
    int projection = s->camera.projection;
    projection += approach(300 - projection, 4);
    s->radius += approach(300 - s->radius, 4);
    s->orbit_pitch = ease_angle(s->orbit_pitch, 341);
    s->orbit_yaw = ease_angle(s->orbit_yaw, 2048);
    BfmForestTransform arm;
    bfm_forest_actor_pitch_yaw((unsigned)s->orbit_pitch, (unsigned)s->orbit_yaw, s->feet, &arm);
    int32_t target[3], eye[3];
    for (unsigned i = 0; i < 3; ++i) {
        int desired = i == 1 ? -40 : 0;
        s->offset[i] += approach(desired - s->offset[i], 16);
        target[i] = s->feet[i] + s->offset[i];
        int64_t v = (int64_t)arm.rotation[i][2] * s->radius;
        eye[i] = target[i] + (int32_t)(v / 4096 - (v % 4096 < 0));
    }
    return bfm_forest_camera_look_at(eye, target, (uint16_t)projection, &s->camera);
}

int bfm_retry_screen_step(BfmRetryScreen *s, unsigned held_pad, unsigned pressed_pad)
{
    const unsigned face = BFM_FOREST_TRIANGLE | BFM_FOREST_CIRCLE | BFM_FOREST_CROSS | BFM_FOREST_SQUARE;
    if (!s || s->phase == FAILED) return -1;
    s->audio_event = BFM_RETRY_AUDIO_NONE;
    if (s->phase == FINISHED) return 1;
    bfm_forest_voice_step(s->voice);
    ++s->age;
    if (s->age == SCORE_REQUEST) s->audio_event = BFM_RETRY_AUDIO_START;
    /* The transfer installs and evaluates the camera before the ordinary
     * camera pass evaluates it again on that same update. */
    if ((s->age == WORLD_CUTOFF && !move_camera(s)) ||
        (s->age >= WORLD_CUTOFF && !move_camera(s))) {
        s->phase = FAILED;
        return -1;
    }
    if (s->phase != STAGGER && s->key + 1 < s->clip[s->clip_index].count) ++s->key;
    if (s->phase == STAGGER) {
        if (s->age == 1) {
            s->audio_event = BFM_RETRY_AUDIO_FADE_FIELD;
            s->velocity = 227 * 65536;
            s->phase = SPIN;
        }
    } else if (s->phase <= BOUNCE) {
        tumble(s, held_pad & face ? 4 : 1);
    } else {
        s->yaw = (s->yaw + 22) & TURN_MASK;
        switch (s->phase) {
        case WAIT_FOR_LINE:
            if (s->age >= SCORE_REQUEST + COLD_SCORE_READY + 1) {
                if (!open_line(s)) {
                    s->phase = FAILED;
                    return -1;
                }
                s->phase = SPEAK;
            }
            break;
        case SPEAK: {
            if (pressed_pad & face) s->shorten = 1;
            const BfmForestMessageFrame *frame = bfm_forest_voice_page(
                s->voice, s->page, &s->generation, !!(pressed_pad & BFM_FOREST_CROSS),
                !!(pressed_pad & BFM_FOREST_CIRCLE));
            if (!frame) {
                s->phase = FAILED;
                return -1;
            }
            if (frame->events & BFM_FOREST_MESSAGE_CLOSED) {
                s->phase = RESTORE;
            }
            break;
        }
        case RESTORE:
            bfm_forest_status_chapter_retry(&s->run->player);
            s->phase = WAIT_FOR_RELEASE;
            /* Entering the grace state and submitting its fade command
             * each take their own game-service update. */
            s->timer = s->shorten ? 2 : POST_LINE_WAIT + 2;
            break;
        case WAIT_FOR_RELEASE:
            if (!--s->timer) {
                s->audio_event = BFM_RETRY_AUDIO_FADE_OUT;
                s->phase = RELEASE;
                /* The score reaches zero, acknowledges the release, and
                 * the scene transfer takes ownership before its wipe. */
                s->timer = RELEASE_WAIT + 4;
            }
            break;
        case RELEASE:
            if (!--s->timer) {
                s->phase = FADE;
                s->timer = 0;
            }
            break;
        case FADE:
            if (++s->timer == 16) s->phase = FINISHED;
            break;
        default:
            break;
        }
    }
    return s->phase == FINISHED;
}

int bfm_retry_screen_keep_world(const BfmRetryScreen *s)
{
    return s && s->age < WORLD_CUTOFF;
}

static unsigned dim_channel(unsigned channel, unsigned subtract)
{
    unsigned value = (channel & 255) >> 3;
    if (value <= subtract) return 0;
    value -= subtract;
    return value << 3 | value >> 2;
}

/* The outgoing transfer overlays uniform subtractive grey on the completed
 * picture. Its brightness increases by sixteen per update. */
static void fade_actor(uint32_t pixels[240][320], unsigned update)
{
    unsigned subtract = 2 * update;
    for (unsigned y = 10; y < 230; ++y)
        for (unsigned x = 0; x < 320; ++x) {
            uint32_t p = pixels[y][x];
            unsigned r = dim_channel(p >> 16, subtract);
            unsigned g = dim_channel(p >> 8, subtract);
            unsigned b = dim_channel(p, subtract);
            pixels[y][x] = 0xff000000u | (r << 16) | (g << 8) | b;
        }
}

int bfm_retry_screen_draw(const BfmRetryScreen *s, const BfmForestHud *hud,
                          const BfmForestMessageArt *art, uint32_t pixels[240][320])
{
    if (!s || !pixels) return 0;
    if (!bfm_retry_screen_keep_world(s))
        for (unsigned y = 0; y < 240; ++y)
            for (unsigned x = 0; x < 320; ++x) pixels[y][x] = 0xff000000u;
    if (s->phase == FINISHED) return 1;
    BfmForestTransform root;
    bfm_forest_actor_pitch_yaw(s->pitch, s->yaw, s->feet, &root);
    if (!bfm_forest_draw_player_only(s->field, &s->camera, &s->clip[s->clip_index].key[s->key],
                                     &root, s->visible, bfm_retry_screen_keep_world(s) ? NULL : &s->light, pixels)) return 0;
    if (hud && s->phase <= SPEAK) bfm_forest_hud_draw(hud, &s->run->player, pixels);
    if (art && s->phase == SPEAK)
        bfm_forest_message_art_draw(art, bfm_forest_message_frame(s->page), pixels);
    if (s->phase == FADE) fade_actor(pixels, s->timer);
    return 1;
}

BfmRetryAudioEvent bfm_retry_screen_audio_event(BfmRetryScreen *s)
{
    if (!s) return BFM_RETRY_AUDIO_NONE;
    BfmRetryAudioEvent event = s->audio_event;
    s->audio_event = BFM_RETRY_AUDIO_NONE;
    return event;
}

ForestVoice *bfm_retry_screen_voice(const BfmRetryScreen *s) { return s ? s->voice : NULL; }
unsigned bfm_retry_screen_generation(const BfmRetryScreen *s) { return s ? s->generation : 0; }
void bfm_retry_screen_free(BfmRetryScreen *s)
{
    if (!s) return;
    bfm_forest_message_free(s->page);
    bfm_forest_voice_free(s->voice);
    free(s);
}
