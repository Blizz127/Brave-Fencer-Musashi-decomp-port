/* SPDX-License-Identifier: MIT */
#include "village_mayor_camera.h"
#include "../pc_port/forest_player_bytes.h"

static const uint8_t *record_at(const uint8_t *member, size_t size, uint32_t base,
                                uint32_t address, size_t length)
{
    if (!member || address < base || length > size || address - base > size - length)
        return NULL;
    return member + (address - base);
}

int bfm_village_mayor_camera_begin(BfmVillageMayorCamera *camera,
                                   const uint8_t *scene, size_t bytes,
                                   const uint8_t *mayor, size_t mayor_bytes,
                                   const BfmVillageCamera *normal)
{
    const uint32_t field_base = UINT32_C(0x80128158);
    const uint32_t event_base = UINT32_C(0x801ef468);
    if (!camera || !normal) return 0;
    *camera = (BfmVillageMayorCamera){0};
    const uint8_t *station = record_at(mayor, mayor_bytes, event_base, 0x801f0900, 8);
    const uint8_t *cue = record_at(mayor, mayor_bytes, event_base, 0x801f093c, 8);
    const uint8_t *depart = record_at(mayor, mayor_bytes, event_base, 0x801f09c4, 8);
    if (!station || !cue || u32le(cue) != 0x21 || !depart || u32le(depart) != 0x35)
        return 0;
    const uint8_t *framing = record_at(mayor, mayor_bytes, event_base, u32le(cue + 4), 16);
    const uint8_t *exit = record_at(scene, bytes, field_base, u32le(depart + 4), 80);
    if (!framing || !exit) return 0;

    /* The goodbye continues to hold the shot during the porch turn, walk
     * and door closure. The room specifies the waits. Two scheduler ticks
     * bridge the end of the window, the departure script and camera return. */
    unsigned delay = 2;
    for (unsigned i = 0; i < 10; ++i) {
        unsigned op = u32le(exit + 8 * i);
        if (op == 0x0f) {
            unsigned ticks = u32le(exit + 8 * i + 4);
            if (ticks > 0xffff) return 0;
            delay += ticks;
        }
        if (op == 0x20) {
            camera->release_delay = delay;
            return bfm_village_talk_camera_frame(&camera->orbit, station, framing, normal);
        }
    }
    return 0;
}

void bfm_village_mayor_camera_step(BfmVillageMayorCamera *camera, unsigned elapsed,
                                  unsigned release_age, int returning,
                                  BfmVillageCamera *normal)
{
    /* The porch interaction first aligns the player to the doorway. */
    if (!camera || elapsed < 17) return;
    if (returning && release_age >= camera->release_delay)
        bfm_village_talk_camera_release(&camera->orbit);
    bfm_village_talk_camera_step(&camera->orbit, normal);
}
