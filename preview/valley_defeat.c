/* SPDX-License-Identifier: MIT */
#include "valley_defeat.h"
#include "woods_revive.h"
#include <stdlib.h>

struct BfmValleyDefeat {
    const MusashiDiscMedia *disc;
    const uint8_t *scene;
    size_t scene_bytes;
    BfmVillageFieldDefeat actor;
    BfmForestMessageArt *writing;
    BfmRetryScreen *retry;
};

/* Runtime offsets within the player's US SC03 member 31. The field supplies
 * its own window contours, defeat line index/lights and recovery destination. */
enum {
    VALLEY_DEFEAT_TABLE = 0x801846d8 - 0x80128158,
    VALLEY_SAVED_DESTINATION = 0x801848c4 - 0x80128158
};

BfmValleyDefeat *bfm_valley_defeat_open(const MusashiDiscMedia *disc, const uint8_t *scene,
    size_t scene_bytes, const uint8_t *main, size_t main_bytes, uint16_t tiles[512][1024],
    BfmVillageField *field, const unsigned *age)
{
    if (!disc || !scene || !main || !main_bytes || !tiles || !field || !age ||
        scene_bytes < VALLEY_DEFEAT_TABLE + BFM_RETRY_ROOM_DATA_BYTES ||
        scene_bytes < VALLEY_SAVED_DESTINATION + sizeof(((BfmSaveRecord *)0)->destination))
        return NULL;
    BfmValleyDefeat *defeat = calloc(1, sizeof *defeat);
    if (!defeat) return NULL;
    defeat->disc = disc;
    defeat->scene = scene;
    defeat->scene_bytes = scene_bytes;
    defeat->actor = (BfmVillageFieldDefeat){.field=field, .main=main, .main_bytes=main_bytes,
                                            .tiles=tiles, .age=age};
    return defeat;
}

void bfm_valley_defeat_free(BfmValleyDefeat *defeat)
{
    if (!defeat) return;
    bfm_retry_screen_free(defeat->retry);
    bfm_forest_message_art_free(defeat->writing);
    free(defeat);
}

BfmForestMessageArt *bfm_valley_defeat_window_art(BfmValleyDefeat *defeat)
{
    if (!defeat) return NULL;
    if (!defeat->writing)
        defeat->writing = bfm_valley_message_art_load(defeat->disc, defeat->scene, defeat->scene_bytes,
                                                     defeat->actor.tiles);
    return defeat->writing;
}

BfmValleyDefeatResult bfm_valley_defeat_resolve(BfmValleyDefeat *defeat, BfmVillageMotion *motion,
    BfmSaveRecord *save, BfmGameplayRun *run, const BfmVillageCamera *presented,
    BfmForestMessage **opened)
{
    if (!defeat || !run || !run->has_player || !motion || !save || !presented || !opened || *opened ||
        bfm_forest_status_values(&run->player).hp != 0) return BFM_VALLEY_DEFEAT_ERROR;
    if (defeat->retry) return BFM_VALLEY_RETRY;
    if (!bfm_valley_defeat_window_art(defeat)) return BFM_VALLEY_DEFEAT_ERROR;

    unsigned slot;
    BfmForestMessage *notice = NULL;
    BfmWoodsReviveResult result = bfm_woods_revive_prepare(run, save, 0, defeat->actor.main,
                                                          defeat->actor.main_bytes, &slot, &notice);
    if (result == BFM_WOODS_REVIVE_ERROR) return BFM_VALLEY_DEFEAT_ERROR;
    if (result == BFM_WOODS_REVIVE_READY) {
        if (!bfm_village_motion_woods_revive(motion, bfm_field_tired_stance(run->player.record))) {
            bfm_forest_message_free(notice);
            return BFM_VALLEY_DEFEAT_ERROR;
        }
        bfm_woods_revive_restore(run, save, slot);
        bfm_village_motion_woods_condition(motion, bfm_field_tired_stance(run->player.record));
        *opened = notice;
        return BFM_VALLEY_REVIVED;
    }

    defeat->actor.motion = motion;
    defeat->actor.save = save;
    defeat->retry = bfm_village_field_defeat_open(&defeat->actor, defeat->disc, defeat->scene,
                            defeat->scene_bytes, VALLEY_DEFEAT_TABLE, presented, run);
    return defeat->retry ? BFM_VALLEY_RETRY : BFM_VALLEY_DEFEAT_ERROR;
}

BfmRetryScreen *bfm_valley_defeat_retry(const BfmValleyDefeat *defeat)
{
    return defeat ? defeat->retry : NULL;
}

void bfm_valley_defeat_draw_notice(const BfmValleyDefeat *defeat, const BfmForestMessageFrame *notice,
                                  uint32_t pixels[240][320])
{
    if (defeat && defeat->writing && notice)
        bfm_forest_message_art_draw(defeat->writing, notice, pixels);
}

int bfm_valley_defeat_draw_retry(const BfmValleyDefeat *defeat, const BfmForestHud *hud,
                               uint32_t pixels[240][320])
{
    return defeat && defeat->retry && bfm_retry_screen_draw(defeat->retry, hud, defeat->writing, pixels);
}

int bfm_valley_defeat_destination(const BfmValleyDefeat *defeat, const BfmGameplayRun *run,
                                BfmBedroomEntry *out)
{
    if (!defeat || !defeat->retry) return 0;
    BfmWoodsRetryPage page = bfm_retry_screen_woods_destination(defeat->retry);
    return bfm_woods_retry_field_destination(defeat->disc, run, page, defeat->scene,
                                            defeat->scene_bytes, VALLEY_SAVED_DESTINATION, out);
}
