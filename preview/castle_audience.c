/* SPDX-License-Identifier: MIT */
#include "castle_audience.h"
#include "castle_audience_stage.h"
#include "castle_audience_visitors.h"
#include "scene_tiles.h"
#include "bedroom_field.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum { COURT_INSET = 0x5aef0, COURTYARD_DOOR = 0x5e20c };

struct BfmCastleAudience {
    const MusashiDiscMedia *disc;
    uint8_t *scene;
    const uint8_t *main;
    size_t main_bytes;
    size_t bytes;
    uint16_t tiles[512][1024];
    BfmCastleAudienceStage *stage;
    BfmForestMessageArt *art;
    ForestVoice *voice;
    BfmBedroomEffects *effects;
    BfmForestMessage *page;
    BfmRibsonDialogue story;
    BfmCastleAudienceVisitors visitors;
    uint8_t flags[64];
    unsigned generation, exit_updates;
    int service, hero, leaving, guest_pending, musashi_x, ribson_x;
};

void bfm_castle_audience_free(BfmCastleAudience *court)
{
    if (!court) return;
    bfm_forest_message_free(court->page);
    bfm_castle_audience_visitors_free(&court->visitors);
    bfm_forest_voice_free(court->voice);
    bfm_bedroom_effects_free(court->effects);
    bfm_forest_message_art_free(court->art);
    bfm_castle_audience_stage_free(court->stage);
    free(court->scene);
    free(court);
}

BfmCastleAudience *bfm_castle_audience_open(const MusashiDiscMedia *disc, const BfmBedroomEntry *entry,
                                           const uint8_t *main, size_t main_bytes)
{
    if (!disc || !entry || !main || !main_bytes || entry->scene_member != 9 ||
        u32le(entry->state.destination) != 0x00003003 || !bfm_castle_audience_eligible(&entry->state))
        return NULL;
    BfmCastleAudience *court = calloc(1,sizeof(*court));
    if (!court) return NULL;
    court->disc = disc;
    court->scene = malloc(BFM_FIELD_SCENE_CAPACITY);
    if (!court->scene) goto failed;
    court->bytes = bfm_read_field_scene(disc,9,court->scene);
    if (court->bytes < COURTYARD_DOOR + sizeof entry->state.destination ||
        u32le(court->scene + COURTYARD_DOOR) != 0x00043000)
        goto failed;

    /* Stage the incoming field separately. A failed menu-to-court mount
     * must leave the exterior's textures available for its next frame. */
    if (!bfm_scene_read_tiles(disc,"MAIN.CD",10,court->tiles) ||
        !bfm_scene_read_tiles(disc,"SC01.CD",9,court->tiles)) goto failed;
    court->main = main;
    court->main_bytes = main_bytes;
    court->stage = bfm_castle_audience_stage_open(disc,court->scene,court->bytes,
        court->main,main_bytes,court->tiles,entry->state.progress[0x48]);
    court->art = bfm_castle_message_art_load(disc,court->scene,court->bytes,court->tiles,COURT_INSET);
    court->voice = bfm_library_voice_load(disc);
    court->effects = bfm_castle_menu_effects_load(disc);
    if (!court->stage || !court->art || !court->voice || !court->effects) goto failed;
    /* The room has submitted eighteen arrival updates when field control
     * becomes live and the court selector starts its own countdown. */
    for (unsigned i=0;i<18;++i)
        if (!bfm_castle_audience_stage_step(court->stage)) goto failed;
    memcpy(court->flags,entry->state.flags,sizeof court->flags);
    bfm_ribson_dialogue_begin(&court->story,court->scene,court->bytes);
    return court;
failed:
    bfm_castle_audience_free(court);
    return NULL;
}

static int leave_court(BfmCastleAudience *court)
{
    bfm_castle_audience_stage_begin_exit(court->stage);
    court->leaving = 1;
    court->exit_updates = 17;
    return 1;
}

int bfm_castle_audience_step(BfmCastleAudience *court, BfmSaveRecord *save, BfmGameplayRun *run,
                             int confirm, int reveal, int direction, int held, int accept, int back)
{
    if (!court || !save) return 0;
    if (court->guest_pending) return 1;
    if (!bfm_castle_audience_stage_step(court->stage)) return 0;
    bfm_forest_voice_step(court->voice);
    if (court->leaving) {
        if (court->exit_updates) --court->exit_updates;
        return 1;
    }
    if (court->page) {
        if (bfm_field_message_choose(court->page,direction,back,accept)) confirm = reveal = 0;
        const BfmForestMessageFrame *frame = bfm_forest_voice_page(
            court->voice,court->page,&court->generation,confirm,reveal);
        if (!frame) return 0;
        int result = (frame->events & BFM_FOREST_VOICE_STOP) ? 2 : 1;
        if (frame->events & BFM_FOREST_MESSAGE_CLOSED) {
            if (court->service) bfm_castle_audience_visitors_closed(&court->visitors,frame->answer);
            bfm_forest_message_free(court->page);
            court->page = NULL;
        }
        return result;
    }
    if (!court->service) {
        if (court->story.wait == 1 && !bfm_castle_audience_stage_speakers(
            court->stage,&court->musashi_x,&court->ribson_x)) return 0;
        BfmRibsonResult next = bfm_ribson_dialogue_step(&court->story,save,run,&court->page);
        if (next == BFM_RIBSON_INVALID) return 0;
        if (next == BFM_RIBSON_DEPART) return leave_court(court);
        if (next != BFM_RIBSON_SERVICE) {
            court->hero = court->story.hero;
            return 1;
        }
        if (!bfm_castle_audience_visitors_begin(&court->visitors,court->disc,court->scene,court->bytes,
            court->main,court->main_bytes) || !bfm_castle_audience_stage_speakers(
            court->stage,&court->musashi_x,&court->ribson_x))
            return 0;
        court->service = 1;
    }
    BfmCastleVisitResult next = bfm_castle_audience_visitors_step(
        &court->visitors,save,direction,held,accept,back,&court->page);
    court->hero = court->visitors.hero;
    if (next == BFM_CASTLE_VISIT_FAREWELL) return leave_court(court);
    if (next == BFM_CASTLE_VISIT_GUEST) {
        court->guest_pending = 1;
        return 1;
    }
    return next == BFM_CASTLE_VISIT_ACTIVE;
}

int bfm_castle_audience_draw(BfmCastleAudience *court, const BfmForestHud *hud, const BfmForestStatus *status,
                             uint32_t pixels[240][320])
{
    if (!court || !pixels || !bfm_castle_audience_stage_draw(court->stage,court->flags,pixels)) return 0;
    bfm_forest_hud_draw(hud,status,pixels);
    if (court->page) {
        const BfmForestMessageFrame *frame = bfm_forest_message_frame(court->page);
        int right = court->hero ? court->musashi_x > court->ribson_x : court->ribson_x >= court->musashi_x;
        int offset = court->hero ? court->musashi_x : court->ribson_x;
        return bfm_forest_message_art_draw_placed(court->art,frame,(int16_t)offset,right,pixels);
    }
    return !court->service || bfm_castle_audience_visitors_draw(&court->visitors,court->art,court->tiles,pixels);
}

const uint8_t *bfm_castle_audience_transfer(const BfmCastleAudience *court)
{
    return court && court->leaving && !court->exit_updates ? court->scene + COURTYARD_DOOR : NULL;
}

int bfm_castle_audience_guest_pending(const BfmCastleAudience *court)
{
    return court && court->guest_pending;
}

BfmBedroomEffects *bfm_castle_audience_effects(BfmCastleAudience *court)
{
    return court ? court->effects : NULL;
}

unsigned bfm_castle_audience_take_sound(BfmCastleAudience *court)
{
    if (!court) return 0;
    unsigned sound = court->visitors.sound;
    court->visitors.sound = 0;
    return sound;
}

ForestVoice *bfm_castle_audience_voice(BfmCastleAudience *court, unsigned *generation)
{
    if (generation) *generation = court ? court->generation : 0;
    return court ? court->voice : NULL;
}
