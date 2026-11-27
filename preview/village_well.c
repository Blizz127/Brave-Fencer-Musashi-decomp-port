/* SPDX-License-Identifier: MIT */
#include "village_well.h"
#include "village_talk_camera.h"
#include "../pc_port/forest_player_archive.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

static const uint32_t origin = UINT32_C(0x80128158);
static const uint32_t room = UINT32_C(0x80192b18);

typedef enum {
    APPROACH, FACE_WELL, BEFORE_SEARCH, SEARCHING, FACE_CAMERA,
    RAISE, NOTICE, LOWER, FULL, SETTLE, RETURN
} WellPhase;

struct BfmVillageWell {
    WellPhase phase;
    BfmBedroomTravel *travel;
    BfmVillageTalkCamera shot;
    const uint8_t *station, *program;
    size_t length;
    uint32_t full_message;
    uint8_t full_text[131];
    unsigned item, before_search, search_delay, after_pickup, full_delay, wait;
    int gesture_done, closed;
};

static const uint8_t *span(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    if (address < origin || count > bytes || address - origin > bytes - count) return NULL;
    return scene + (address - origin);
}

void bfm_village_well_free(BfmVillageWell *well)
{
    if (!well) return;
    bfm_bedroom_travel_free(well->travel);
    free(well);
}

static int reachable(const BfmSaveRecord *save, const uint8_t *station, const int16_t player[3])
{
    unsigned story = u32le(save->progress + 8), hour = save->progress[0x39];
    /* The residents occupy the rim between ten and four, except during
     * the two village closures. The room removes the well actor entirely
     * for the intervening story segment. */
    if (story >= 200 && story < 630) return 0;
    int villagers_absent = (story >= 100 && story < 130) || (story >= 950 && story < 1000);
    if (hour >= 10 && hour < 16 && !villagers_absent) return 0;
    if (player[0] > 640 || player[2] < -896) return 0;
    int64_t squared = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        int64_t d = (int)player[axis] - s16le(station + 2 * axis);
        squared += d*d;
    }
    return squared <= 160*160;
}

int bfm_village_well_begin(BfmVillageWell **out, const uint8_t *scene, size_t bytes,
                           const BfmSaveRecord *save, const BfmBedroomRoot *player,
                           const BfmVillageCamera *view)
{
    if (!out || *out || !scene || !save || !player || !view) return 0;
    const uint8_t *station = span(scene, bytes, UINT32_C(0x801913d4), 8);
    if (!station) return 0;
    if (!reachable(save, station, player->xyz)) return 1;

    const uint8_t *cue = span(scene, bytes, UINT32_C(0x80191438), 112);
    if (!cue || u32le(cue) != 33 || u32le(cue+8) != 3 || u32le(cue+72) != 59)
        return 0;
    const uint8_t *camera = span(scene, bytes, u32le(cue+4), 16);
    const uint8_t *full = span(scene, bytes, u32le(cue+68), 24);
    const uint8_t *award = span(scene, bytes, u32le(cue+76), 28);
    if (!camera || !full || !award || u32le(award) || u32le(award+4) != 76 ||
        u32le(full+8) != 0 || u32le(full+20) > 255 || u32le(cue+36) > 255 ||
        u32le(cue+52) > 255 || u32le(cue+92) > 255) return 0;
    const uint8_t *text = span(scene, bytes, u32le(award+24), 59);
    if (!text || text[0] != 5 || text[1] != 2 || text[58] != 0) return 0;

    BfmVillageWell *well = calloc(1, sizeof *well);
    if (!well) return 0;
    well->station = station;
    well->program = text;
    well->length = 59;
    well->item = u32le(award+4);
    well->full_message = u32le(full+12);
    well->full_delay = u32le(full+20);
    well->before_search = u32le(cue+36);
    well->search_delay = u32le(cue+52);
    well->after_pickup = u32le(cue+92);
    well->travel = bfm_bedroom_travel_from_root(scene, bytes, room, player);
    if (!well->travel || !bfm_village_talk_camera_frame(&well->shot, station, camera, view) ||
        !bfm_bedroom_travel_walk(well->travel, s16le(cue+12), s16le(cue+14), 0)) {
        bfm_village_well_free(well);
        return 0;
    }
    *out = well;
    return 1;
}

void bfm_village_well_close(BfmVillageWell *well)
{
    if (well && (well->phase == NOTICE || well->phase == FULL)) well->closed = 1;
}

static int open_full(BfmVillageWell *well, const MusashiDiscMedia *disc,
                     BfmForestMessage **opened)
{
    ForestPlayerArchive archive;
    if (!forest_player_archive_open(disc, &archive) ||
        !forest_player_archive_read(&archive, well->full_message, well->full_text,
                                    sizeof well->full_text) ||
        well->full_text[0] != 5 || well->full_text[sizeof well->full_text-1] != 0) return 0;
    *opened = bfm_field_message_borrow(well->full_text, sizeof well->full_text);
    well->phase = FULL;
    return *opened != NULL;
}

int bfm_village_well_step(BfmVillageWell **active, BfmVillageMotion *motion,
                          BfmVillageCamera *view, BfmSaveRecord *save,
                          const MusashiDiscMedia *disc, BfmForestMessage **opened,
                          int *received)
{
    BfmVillageWell *well = *active;
    if (!well || !motion || !view || !save || !opened || !received) return 0;
    *received = 0;
    if (!bfm_bedroom_travel_step(well->travel, well->shot.eye, well->shot.target)) return 0;
    const BfmBedroomRoot *root = bfm_bedroom_travel_root(well->travel);
    BfmBedroomAction action = BFM_BEDROOM_NONE;
    switch (well->phase) {
    case APPROACH:
        if (!root->busy && root->locomotion_edge == BFM_TRAVEL_NONE) well->phase = FACE_WELL;
        break;
    case FACE_WELL:
        if (bfm_bedroom_travel_turn_to_point(well->travel,
                 s16le(well->station), s16le(well->station+4))) {
            well->wait = well->before_search;
            well->phase = BEFORE_SEARCH;
        }
        break;
    case BEFORE_SEARCH:
        if (!well->wait || !--well->wait) {
            action = BFM_BEDROOM_DRAW_WATER;
            well->wait = well->search_delay;
            well->phase = SEARCHING;
        }
        break;
    case SEARCHING:
        if (well->wait) --well->wait;
        if (bfm_village_motion_player(motion)->pose.finished) well->gesture_done = 1;
        if (!well->wait && well->gesture_done) {
            if (bfm_save_inventory_space(save) < 0) {
                if (!open_full(well, disc, opened)) return 0;
            } else well->phase = FACE_CAMERA;
        }
        break;
    case FACE_CAMERA:
        if (bfm_bedroom_travel_turn_to_point(well->travel, well->shot.eye[0], well->shot.eye[2])) {
            action = BFM_BEDROOM_RECEIVE;
            well->phase = RAISE;
        }
        break;
    case RAISE:
        *opened = bfm_field_message_borrow(well->program, well->length);
        if (!*opened) return 0;
        well->phase = NOTICE;
        break;
    case NOTICE:
        if (well->closed) {
            action = BFM_BEDROOM_LOWER;
            well->phase = LOWER;
        }
        break;
    case LOWER:
        if (bfm_village_motion_player(motion)->pose.finished) {
            if (!bfm_save_add_item(save, well->item)) return 0;
            *received = 1;
            well->wait = well->after_pickup;
            well->phase = SETTLE;
        }
        break;
    case FULL:
        if (well->closed) {
            well->wait = well->full_delay;
            well->phase = SETTLE;
        }
        break;
    case SETTLE:
        if (!well->wait || !--well->wait) {
            bfm_village_talk_camera_release(&well->shot);
            well->phase = RETURN;
        }
        break;
    case RETURN:
        break;
    }
    bfm_village_talk_camera_step(&well->shot, view);
    if (well->phase == RETURN && !well->shot.phase) {
        if (!bfm_village_motion_place(motion, root)) return 0;
        bfm_village_well_free(well);
        *active = NULL;
        return 1;
    }
    return bfm_village_motion_well(motion, root, action);
}
