/* SPDX-License-Identifier: MIT */
#include "village_rotor.h"

enum { AT_REST, ACCELERATING, COASTING, SLOWING };
enum { REMOVED_AT_STORY = 1170 };

int bfm_village_rotor_load(BfmVillageRotor *rotor, const uint8_t *scene,
                          size_t bytes, unsigned story_progress)
{
    if (!rotor) return 0;
    *rotor = (BfmVillageRotor){0};
    if (story_progress >= REMOVED_AT_STORY) return 1;
    rotor->mesh = bfm_village_rotor_mesh_load(scene, bytes);
    return rotor->mesh != NULL;
}

void bfm_village_rotor_clear(BfmVillageRotor *rotor)
{
    if (rotor) {
        bfm_village_rotor_mesh_free(rotor->mesh);
        *rotor = (BfmVillageRotor){0};
    }
}

static void turn(BfmVillageRotor *rotor)
{
    rotor->yaw = (rotor->yaw + rotor->speed) & 4095u;
}

void bfm_village_rotor_step(BfmVillageRotor *rotor, BfmGameplayRun *run, unsigned story_progress)
{
    if (!rotor || !rotor->mesh || !run) return;
    if (rotor->remaining) --rotor->remaining;
    switch (rotor->phase) {
    case AT_REST:
        if (!rotor->remaining) {
            rotor->speed = 0;
            rotor->remaining = 30 + (bfm_gameplay_random(run) & 31u);
            rotor->phase = ACCELERATING;
        }
        break;
    case ACCELERATING:
        rotor->speed += 6;
        turn(rotor);
        if (!rotor->remaining) {
            rotor->remaining = 60 + (bfm_gameplay_random(run) & 63u);
            rotor->phase = COASTING;
        }
        break;
    case COASTING:
        turn(rotor);
        if (!rotor->remaining) rotor->phase = SLOWING;
        break;
    case SLOWING:
        rotor->speed -= 3;
        turn(rotor);
        if (rotor->speed < 9) {
            rotor->remaining = 60 + (bfm_gameplay_random(run) & 63u);
            rotor->phase = AT_REST;
        }
        break;
    }
    if (story_progress >= REMOVED_AT_STORY) bfm_village_rotor_clear(rotor);
}

int bfm_village_rotor_append(const BfmVillageRotor *rotor, BfmBedroomFigure *figure,
                            const BfmBedroomView *view)
{
    return !rotor || !rotor->mesh || bfm_village_rotor_mesh_append(rotor->mesh, figure, view, rotor->yaw);
}
