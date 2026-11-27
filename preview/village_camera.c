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

static int yaw_step(int delta, int limit)
{
    int divisor = 20;
    while (divisor > 1 && delta && delta / divisor == 0) divisor /= 2;
    int step = delta / divisor;
    if (step > limit) return limit;
    if (step < -limit) return -limit;
    return step;
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
        camera->yaw += yaw_step(2048 - camera->yaw, 91);
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

void bfm_village_camera_turn(BfmVillageCamera *camera, BfmVillageCameraTurn *turn,
                             int direction)
{
    /* A tap selects the next eighth-turn. Holding renews the target when the
     * view is within about 75 degrees. Releasing lets the eye finish the turn. */
    enum { REPEAT_TICK = 10, DETENT = 4096 / 8, LEAD = 4096 * 75 / 360 };
    if (direction != turn->direction) {
        turn->direction = direction;
        turn->held_ticks = 0;
    }
    if (direction && turn->held_ticks < REPEAT_TICK) ++turn->held_ticks;
    int continuous = turn->held_ticks == REPEAT_TICK;
    int delta = ((turn->goal - camera->yaw + 2048) & 4095) - 2048;
    if (turn->held_ticks == 1 || (continuous && direction * delta < LEAD)) {
        turn->goal = (turn->goal + direction * DETENT) & 4095;
        delta = ((turn->goal - camera->yaw + 2048) & 4095) - 2048;
    }
    camera->yaw = (camera->yaw + yaw_step(delta, continuous ? 45 : 91)) & 4095;
}

void bfm_village_camera_begin(BfmVillageCamera *camera, const int16_t xyz[3])
{
    *camera = (BfmVillageCamera){.focal = 300, .radius = 1800, .yaw = 2152};
    bfm_village_camera_step(camera, xyz, 0);
    bfm_village_camera_step(camera, xyz, 0);
}

static int move_quarter(int current, int goal)
{
    int distance = goal - current;
    int step = distance / 4;
    if (!step && distance) step = distance > 0 ? 1 : -1;
    return current + step;
}

void bfm_village_camera_lower(BfmVillageCamera *camera, const int16_t xyz[3],
                              int entering)
{
    if (entering) {
        *camera = (BfmVillageCamera){.focal = 500, .radius = 1200,
                                     .yaw = 2048, .following = 1};
        camera->target[1] = xyz[1];
    }

    camera->target[0] = xyz[0];
    camera->target[1] = move_quarter(camera->target[1], xyz[1]);
    camera->target[2] = xyz[2];
    orbit(camera, 568);
}

BfmVillageStreetOrbit bfm_village_camera_street_orbit(const BfmVillageCamera *camera,
                                                       const BfmVillageCameraTurn *turn)
{
    return (BfmVillageStreetOrbit){.focal=camera->focal, .radius=camera->radius,
                                   .yaw=camera->yaw, .turn=*turn};
}

void bfm_village_camera_restore_street(BfmVillageCamera *camera, BfmVillageCameraTurn *turn,
                                       BfmVillageStreetOrbit orbit, const int16_t xyz[3])
{
    camera->focal = orbit.focal;
    camera->radius = orbit.radius;
    camera->yaw = orbit.yaw;
    *turn = orbit.turn;
    camera->target[1] = xyz[1];
    bfm_village_camera_lower(camera, xyz, 0);
}

/* Both farm entrances look diagonally across the bank from the same bearing. */
void bfm_village_camera_bank(BfmVillageCamera *camera, int *radius_goal, const int16_t xyz[3],
                             int entering, int shoulder_pressed)
{
    if (entering) {
        *camera = (BfmVillageCamera){.focal = 500, .radius = 1200, .yaw = 1536};
        *radius_goal = 1200;
        camera->target[1] = xyz[1];
    } else if (shoulder_pressed) {
        *radius_goal = *radius_goal == 1200 ? 900 : 1200;
    }
    camera->radius = move_quarter(camera->radius, *radius_goal);
    camera->target[0] = xyz[0];
    camera->target[1] = move_quarter(camera->target[1], xyz[1]);
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
