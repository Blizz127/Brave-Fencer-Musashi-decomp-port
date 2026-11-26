/* SPDX-License-Identifier: MIT */
#include "village_camera.h"
#include <math.h>

static int quarter_axis(unsigned angle)
{
    unsigned phase = angle & 1023u;
    if (angle & 1024u) phase = 1023u - phase;
    int sample = (int)lround(4096.0 * sin(phase * (6.28318530717958647693 / 4096.0)));
    return angle & 2048u ? -sample : sample;
}

static int scale(int a, int b)
{
    int product = a * b;
    return product >= 0 ? product / 4096 : -((-product + 4095) / 4096);
}

static void orbit(BfmVillageCamera *camera, unsigned pitch)
{
    unsigned yaw = (unsigned)camera->yaw;
    int horizontal = quarter_axis(pitch + 1024u);
    int x = scale(quarter_axis(yaw), horizontal);
    int z = scale(quarter_axis(yaw + 1024u), horizontal);
    camera->eye[0] = camera->target[0] + scale(x, camera->radius);
    camera->eye[1] = camera->target[1] + scale(-quarter_axis(pitch), camera->radius);
    camera->eye[2] = camera->target[2] + scale(z, camera->radius);
}

void bfm_village_camera_step(BfmVillageCamera *camera, const int16_t xyz[3],
                             int follow_event)
{
    if (follow_event && !camera->following) {
        camera->following = 1;
        camera->focal = 500;
        camera->radius = 1200;
        camera->yaw = 1536;
    }
    if (camera->following) {
        int delta = 2048 - camera->yaw;
        int divisor = 20;
        while (divisor > 1 && delta && delta / divisor == 0) divisor /= 2;
        int turn = delta / divisor;
        if (turn > 91) turn = 91;
        if (turn < -91) turn = -91;
        camera->yaw += turn;
        for (unsigned i = 0; i < 3; ++i) camera->target[i] = xyz[i];
        orbit(camera, 568);
    } else {
        camera->radius -= 16;
        if (camera->radius < 300) camera->radius = 300;
        camera->target[0] = xyz[0] < -304 ? -304 : xyz[0];
        camera->target[1] = -642 - 192;
        camera->target[2] = 1379;
        orbit(camera, 56);
    }
}

void bfm_village_camera_begin(BfmVillageCamera *camera, const int16_t xyz[3])
{
    *camera = (BfmVillageCamera){.focal = 300, .radius = 1800, .yaw = 2152};
    bfm_village_camera_step(camera, xyz, 0);
    bfm_village_camera_step(camera, xyz, 0);
}

void bfm_village_camera_lower(BfmVillageCamera *camera, const int16_t xyz[3],
                              int entering)
{
    if (entering) {
        *camera = (BfmVillageCamera){.focal = 500, .radius = 1200,
                                     .yaw = 2048, .following = 1};
        camera->target[1] = xyz[1];
    }

    int rise = xyz[1] - camera->target[1];
    int step = rise / 4;
    if (!step && rise)
        step = rise > 0 ? 1 : -1;
    camera->target[0] = xyz[0];
    camera->target[1] += step;
    camera->target[2] = xyz[2];
    orbit(camera, 568);
}

/* The entrance trail orbits the field origin along with the player. */
void bfm_village_camera_woods_orbit(BfmVillageCamera *camera, int bearing,
                                    int player_z, int entering)
{
    int pitch = player_z < -1280 ? 341 : 728;
    if (entering) {
        camera->woods_pitch = pitch;
    } else {
        int delta = pitch - camera->woods_pitch;
        int step = delta / 4;
        if (!step && delta) step = delta > 0 ? 1 : -1;
        camera->woods_pitch += step;
    }
    camera->yaw = (bearing - 512) & 4095;
    camera->radius = 900;
    orbit(camera, (unsigned)camera->woods_pitch);
}
