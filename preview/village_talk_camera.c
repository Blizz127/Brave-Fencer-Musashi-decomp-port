/* SPDX-License-Identifier: MIT */
#include "village_talk_camera.h"
#include "../pc_port/forest_player_bytes.h"

#include <math.h>
#include <string.h>

enum { FOLLOW, CLOSE, RETURNING };

static int sine(int angle)
{
    unsigned magnitude = (unsigned)(angle < 0 ? -angle : angle);
    unsigned phase = magnitude & 1023u;
    if (magnitude & 1024u) phase = 1023u - phase;
    int sample = (int)lround(4096.0 * sin(phase * (6.28318530717958647693 / 4096)));
    if (magnitude & 2048u) sample = -sample;
    return angle < 0 ? -sample : sample;
}

static int cosine(int angle)
{
    return sine(1024 + (angle < 0 ? -angle : angle));
}

static int fixed_floor(int value)
{
    return value >= 0 ? value / 4096 : -((-value + 4095) / 4096);
}

int bfm_village_talk_camera_begin(BfmVillageTalkCamera *talk, const uint8_t *scene,
                                  size_t bytes, const BfmVillageCamera *current)
{
    const uint32_t origin = UINT32_C(0x80128158);
    const size_t station_offset = UINT32_C(0x80189850) - origin;
    const size_t cue_offset = UINT32_C(0x801898ac) - origin;
    if (!talk || !current || !scene || bytes < cue_offset + 8 ||
        u32le(scene + cue_offset) != 0x21) return 0;
    uint32_t record = u32le(scene + cue_offset + 4);
    if (record < origin || record - origin > bytes - 16) return 0;
    return bfm_village_talk_camera_frame(talk, scene + station_offset,
                                        scene + (record - origin), current);
}

int bfm_village_talk_camera_frame(BfmVillageTalkCamera *talk, const uint8_t *station,
                                  const uint8_t *camera, const BfmVillageCamera *current)
{
    if (!talk || !station || !camera || !current) return 0;
    int heading = s16le(station + 6), yaw = s16le(camera + 8);
    int distance = s16le(camera + 10), pitch = s16le(camera + 12);
    if (distance <= 0) return 0;

    /* The aim is local to the resident. The orbit's bearing and elevation
     * are room-space angles. Only the aim offset turns with the resident. */
    int x = s16le(camera), z = s16le(camera + 4);
    int c = cosine(heading), s = sine(heading);
    talk->close_target[0] = s16le(station) + fixed_floor(c*x + s*z);
    talk->close_target[1] = s16le(station + 2) + s16le(camera + 2);
    talk->close_target[2] = s16le(station + 4) + fixed_floor(c*z - s*x);
    int horizontal = cosine(pitch);
    int arm[3] = { fixed_floor(sine(yaw) * horizontal), sine(pitch),
                   fixed_floor(cosine(yaw) * horizontal) };
    for (unsigned i = 0; i < 3; ++i)
        talk->close_eye[i] = talk->close_target[i] + fixed_floor(-distance * arm[i]);
    memcpy(talk->eye, current->eye, sizeof(talk->eye));
    memcpy(talk->target, current->target, sizeof(talk->target));
    memcpy(talk->entry_eye, current->eye, sizeof(talk->entry_eye));
    memcpy(talk->entry_target, current->target, sizeof(talk->entry_target));
    talk->phase = CLOSE;
    talk->return_ticks = 0;
    return 1;
}

void bfm_village_talk_camera_release(BfmVillageTalkCamera *talk)
{
    if (talk && talk->phase == CLOSE) talk->phase = RETURNING;
}

static int advance(int from, int to, int divisor)
{
    int delta = to - from;
    while (divisor > 1 && delta / divisor == 0) divisor /= 2;
    return from + delta / divisor;
}

void bfm_village_talk_camera_step(BfmVillageTalkCamera *talk, BfmVillageCamera *normal)
{
    if (!talk || !normal || talk->phase == FOLLOW) return;
    int returning = talk->phase == RETURNING;
    int divisor = returning ? 6 : 12;
    /* The shot backs out to its saved entry framing. Following the player's
     * post-approach position resumes on the update after that view is shown. */
    if (returning && ++talk->return_ticks == 25) {
        memcpy(normal->eye, talk->entry_eye, sizeof(talk->entry_eye));
        memcpy(normal->target, talk->entry_target, sizeof(talk->entry_target));
        talk->phase = FOLLOW;
        return;
    }
    for (unsigned i = 0; i < 3; ++i) {
        int eye = returning ? talk->entry_eye[i] : talk->close_eye[i];
        int target = returning ? talk->entry_target[i] : talk->close_target[i];
        normal->eye[i] = talk->eye[i] = advance(talk->eye[i], eye, divisor);
        normal->target[i] = talk->target[i] = advance(talk->target[i], target, divisor);
    }
}
