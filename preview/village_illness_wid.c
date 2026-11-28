/* SPDX-License-Identifier: MIT */
#include "village_illness_wid.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

static const uint32_t origin = UINT32_C(0x80128158);
enum { ROOM = 0x80190780u, WID = 380 };
enum { IDLE, ENTER, TALK, PICKUP, LEAVE, NO_ANSWER, RETURN };
enum { BEFORE_WALK, WALK_TO_PORCH, FACE_DOOR, START_KNOCK, KNOCKING, BACK_AWAY,
       WAIT_REACTION, START_REACTION, REACTING, STAGED };
enum { QUEUED, CHECK_BAG, BAG, CHECK_OFFSET, FACE_CAMERA, SHOW_KEY, LOWER_KEY, RESTORE_YAW };

struct BfmVillageWid {
    const uint8_t *scene, *station, *door, *silent, *shot, *entry, *exit, *item;
    size_t bytes;
    BfmBedroomTravel *travel;
    uint32_t cursor;
    unsigned phase, age, hero, pickup, award_yaw, door_delay;
    int generic, react_requested;
    int waiting, restored, hero_x, wid_x;
};

static const uint8_t *span(const BfmVillageWid *w, uint32_t address, size_t size)
{
    if (address < origin || size > w->bytes || address-origin > w->bytes-size) return NULL;
    return w->scene + (address-origin);
}

BfmVillageWid *bfm_village_wid_open(const uint8_t *scene, size_t bytes)
{
    if (!scene) return NULL;
    BfmVillageWid *w = calloc(1,sizeof *w);
    if (!w) return NULL;
    w->scene = scene;
    w->bytes = bytes;
    const uint8_t *director = span(w,UINT32_C(0x8018f190),56);
    const uint8_t *placement = span(w,UINT32_C(0x80193f04),20);
    w->station = span(w,UINT32_C(0x8018f038),28);
    w->door = span(w,UINT32_C(0x80193e50),20);
    const uint8_t *generic = span(w,UINT32_C(0x801bbf70),16);
    w->shot = span(w,UINT32_C(0x8018e4f0),16);
    if (director && placement && u16le(placement+6) == WID && w->station && w->shot &&
        w->door && u16le(w->door+6) == 97 && u16le(w->door+8) == 16 &&
        generic && u32le(generic+8) == 53 &&
        !s16le(w->station+6) && u16le(w->station+24) == 23 && u32le(director+4) == 23 &&
        u32le(director+8) == 30 && u32le(director+16) == 53 && u32le(director+40) == 53) {
        w->entry = span(w,u32le(director+20),136);
        w->exit = span(w,u32le(director+44),80);
        w->silent = span(w,u32le(generic+12),72);
        w->door_delay = 6;
        if (w->silent && u32le(w->silent) == 4 && u32le(w->silent+32) == 15 &&
            u32le(w->silent+48) == 15 && u32le(w->silent+56) == 32 &&
            w->entry && w->exit && u32le(w->entry+8) == 4 && u32le(w->entry+40) == 15 &&
            u32le(w->entry+56) == 15 && u32le(w->entry+80) == 39 &&
            u32le(w->entry+88) == 15 && u32le(w->exit+8) == 37 &&
            u32le(w->exit+24) == 39 && u32le(w->exit+64) == 32) return w;
    }
    free(w);
    return NULL;
}

void bfm_village_wid_free(BfmVillageWid *w)
{
    if (w) bfm_bedroom_travel_free(w->travel);
    free(w);
}

int bfm_village_wid_active(const BfmVillageWid *w)
{
    return w && w->phase != IDLE;
}

int bfm_village_wid_windmill_active(const BfmVillageWid *w)
{
    return bfm_village_wid_active(w) && !w->generic;
}

void bfm_village_wid_camera_restored(BfmVillageWid *w)
{
    if (w && w->phase == RETURN) w->restored = 1;
}

static int project_x(const BfmBedroomView *view, const int16_t xyz[3])
{
    int projected[3];
    return bfm_bedroom_project_packet(view,xyz,projected) ? projected[0]-160 : 0;
}

static void walk_cue(BfmVillageWindmillCue *cue, const uint8_t *command)
{
    cue->walk_q8 = s16le(command+4);
    cue->walk_fields = u16le(command+6);
}

static BfmForestMessage *open_page(BfmVillageWid *w, uint32_t address, BfmSaveRecord *save)
{
    const uint32_t end = UINT32_C(0x801916f8);
    if (address < UINT32_C(0x801911fc) || address >= end || end-address < 5) return NULL;
    const uint8_t *p = span(w,address,end-address);
    if (!p || p[0] != 5 || p[1] != 2) return NULL;
    /* The receive notice has a zero portrait in its four-byte header.
     * The mounted text after these headers has no zero formatting operands. */
    const uint8_t *stop = memchr(p+4,0,(size_t)(end-address)-4);
    return stop ? bfm_field_message_borrow_flags(p,(size_t)(stop-p)+1,save->flags) : NULL;
}

/* The porch queues four player actions in one director update. Each is
 * installed after the previous player worker reports its real completion. */
static int porch_player(BfmVillageWid *w, BfmVillageMotion *motion, const BfmBedroomView *view,
                        BfmVillageWidFrame *frame)
{
    if (!bfm_bedroom_travel_step(w->travel,view->eye,view->target)) return 0;
    const BfmBedroomRoot *root = bfm_bedroom_travel_root(w->travel);
    const uint8_t *home = w->generic ? w->door : w->station;
    int knock = w->hero == START_KNOCK;
    if (w->hero == FACE_DOOR &&
        bfm_bedroom_travel_turn_to_point(w->travel,s16le(home) + (w->generic ? s16le(w->silent+4) : 0),s16le(home+4)))
        w->hero = START_KNOCK;
    if (knock) w->hero = KNOCKING;
    const int64_t *world = bfm_bedroom_travel_position(w->travel);
    int32_t xyz[3] = {(int32_t)world[0],(int32_t)world[1],(int32_t)world[2]};
    if (!bfm_village_motion_staged_position(motion,xyz)) return 0;
    if (w->hero == START_REACTION || w->hero == REACTING) {
        int ended = 0;
        uint32_t body = UINT32_C(0x800d599c);
        int ok = w->hero == START_REACTION ?
            bfm_village_motion_restart_staged_program(motion,root,body,&ended) :
            bfm_village_motion_staged_program(motion,root,body,&ended);
        w->hero = ended ? STAGED : REACTING;
        return ok;
    }
    if (!bfm_village_motion_porch(motion,root,knock)) return 0;
    const BfmBedroomPose *pose = &bfm_village_motion_player(motion)->pose;
    if (pose->knock_contact) frame->sound = 0x516;
    if (w->hero == BEFORE_WALK) {
        const uint8_t *offset = w->generic ? w->silent+4 : w->entry+12;
        int16_t x = (int16_t)(s16le(home)+s16le(offset));
        int16_t z = (int16_t)(s16le(home+4)+s16le(offset+2));
        if (!bfm_bedroom_travel_walk(w->travel,x,z,0)) return 0;
        w->hero = WALK_TO_PORCH;
    } else if (w->hero == WALK_TO_PORCH && !root->busy) {
        w->hero = FACE_DOOR;
    } else if (w->hero == KNOCKING && pose->finished) {
        if (w->generic) {
            w->hero = w->react_requested ? START_REACTION : WAIT_REACTION;
            return 1;
        }
        if (!bfm_bedroom_travel_retreat(w->travel,s16le(w->entry+36))) return 0;
        w->hero = BACK_AWAY;
    } else if (w->hero == WAIT_REACTION && w->react_requested) {
        w->hero = START_REACTION;
    } else if (w->hero == BACK_AWAY && !root->busy) {
        w->hero = STAGED;
    } else {
        return 1;
    }
    /* The selecting player pass installs a travel worker. Its first movement
     * service is on the following field. */
    if (w->hero == WALK_TO_PORCH || w->hero == BACK_AWAY)
        return bfm_bedroom_travel_step(w->travel,view->eye,view->target);
    return 1;
}

static void camera_begin(const BfmVillageWid *w, const int16_t xyz[3], BfmVillageWidFrame *frame)
{
    frame->start_camera = 1;
    for (unsigned i=0; i<3; ++i) {
        frame->camera_target[i] = xyz[i] + s16le(w->shot+2*i);
        frame->camera_eye[i] = xyz[i] + s16le(w->shot+8+2*i);
    }
}

static void silent_field(BfmVillageWid *w, BfmVillageWidFrame *frame)
{
    unsigned wait = u32le(w->silent+36);
    if (!w->age) {
        int16_t hinge[3] = {s16le(w->door),s16le(w->door+2),s16le(w->door+4)};
        camera_begin(w,hinge,frame);
    } else if (w->age == wait) {
        w->react_requested = 1;
    } else if (w->age == wait + u32le(w->silent+52)) {
        frame->release_camera = 1;
        w->phase = RETURN;
    }
    ++w->age;
}

static void entry_field(BfmVillageWid *w, const int16_t resident[3], BfmVillageMotion *motion,
                        const BfmBedroomView *view, BfmSaveRecord *save, BfmGameplayRun *run,
                        BfmVillageWidFrame *frame)
{
    unsigned open = u32le(w->entry+44);
    unsigned emerge = open + u32le(w->entry+60);
    unsigned speak = emerge + u32le(w->entry+92);
    if (!w->age) {
        frame->resident_portrait = u16le(w->station+24);
        camera_begin(w,resident,frame);
        frame->cast.show = -1;
    } else if (w->age == open) {
        frame->cast.door = 1;
    } else if (w->age == emerge) {
        frame->cast.show = 1;
        frame->cast.program = u32le(w->station+16);
        walk_cue(&frame->cast,w->entry+80);
    } else if (w->age == speak) {
        frame->cast.program = u32le(w->station+20);
        /* Wid's last forward step precedes the script's projection. The
         * host applies today's cast movement after this director returns. */
        int speed = s16le(w->entry+84);
        int step = speed/256 - (speed%256 < 0);
        int16_t porch[3] = {s16le(w->station),s16le(w->station+2),
            (int16_t)(s16le(w->station+4) + step*(int)u16le(w->entry+86))};
        w->wid_x = project_x(view,porch);
        w->hero_x = project_x(view,bfm_village_motion_player(motion)->root.xyz);
        /* Wid has no timed greeting record. His slot still remembers the visit. */
        unsigned offset = 4u*u16le(w->station+24)+2;
        unsigned stamp = 24u*u16le(save->progress+0x34) + save->progress[0x39];
        save->timestamps[offset] = (uint8_t)stamp;
        save->timestamps[offset+1] = (uint8_t)(stamp>>8);
        if (run) memcpy(run->banks.timestamps+offset,save->timestamps+offset,2);
        unsigned story = u32le(save->progress+8);
        if (story < 210) w->cursor = UINT32_C(0x8018f054);
        else if (story < 220) w->cursor = UINT32_C(0x8018f080);
        else w->cursor = UINT32_C(0x8018f0f8);
        w->phase = TALK;
    }
    ++w->age;
}

static void exit_field(BfmVillageWid *w, BfmVillageWidFrame *frame)
{
    unsigned depart = u32le(w->exit+20);
    unsigned hide = depart + u32le(w->exit+44);
    unsigned release = hide + u32le(w->exit+60);
    if (!w->age) {
        frame->cast.program = u32le(w->station+16);
        frame->cast.turn = s16le(w->exit+12);
        frame->cast.turn_fields = u16le(w->exit+14);
    } else if (w->age == depart) {
        frame->cast.door = 2;
        walk_cue(&frame->cast,w->exit+24);
    } else if (w->age == hide) {
        frame->cast.show = -1;
    } else if (w->age == release) {
        frame->release_camera = 1;
        w->phase = RETURN;
    }
    ++w->age;
}

/* The player owns the receive notice while the resident waits. The lowering
 * animation publishes the bag entry before the resident's two farewell lines. */
static int award_player(BfmVillageWid *w, BfmVillageMotion *motion, const BfmBedroomView *view,
                        int page_closed, int inventory_closed, BfmSaveRecord *save,
                        BfmGameplayRun *run, BfmVillageWidFrame *frame)
{
    if (w->pickup == BAG && !inventory_closed) return 1;
    const BfmBedroomRoot *root = bfm_bedroom_travel_root(w->travel);
    uint32_t body = 0;
    switch (w->pickup) {
    case QUEUED:
        w->award_yaw = root->yaw;
        w->pickup = CHECK_BAG;
        break;
    case CHECK_BAG: case BAG:
        if (bfm_save_inventory_space(save) < 0) {
            frame->request_inventory = 1;
            w->pickup = BAG;
        } else {
            w->pickup = CHECK_OFFSET;
        }
        break;
    case CHECK_OFFSET:
        /* The key descriptor requests no extra step or held object model. */
        w->pickup = FACE_CAMERA;
        break;
    case FACE_CAMERA:
        if (bfm_bedroom_travel_turn_to_point(w->travel,view->eye[0],view->eye[2])) {
            body = UINT32_C(0x800d5da8);
            frame->sound = 1702;
            frame->pickup_effect = 1;
            frame->opened = open_page(w,u32le(w->item+24),save);
            if (!frame->opened) return 0;
            w->pickup = SHOW_KEY;
        }
        break;
    case SHOW_KEY:
        if (page_closed) {
            body = UINT32_C(0x800d5dc0);
            w->pickup = LOWER_KEY;
        }
        break;
    case LOWER_KEY:
        body = UINT32_C(0x800d5dc0);
        break;
    case RESTORE_YAW:
        if (root->yaw == w->award_yaw) w->phase = TALK;
        else bfm_bedroom_travel_look(w->travel,(int)w->award_yaw);
        break;
    default:
        return 0;
    }

    int ended = 0;
    if (body) {
        if (w->pickup == LOWER_KEY && !page_closed) {
            if (!bfm_village_motion_staged_program(motion,root,body,&ended)) return 0;
        } else if (!bfm_village_motion_restart_staged_program(motion,root,body,&ended)) {
            return 0;
        }
    } else if (!bfm_village_motion_staged(motion,root)) {
        return 0;
    }
    if (w->pickup == LOWER_KEY && ended) {
        int slot = bfm_save_inventory_space(save);
        if (slot < 0 || !bfm_save_add_item(save,u32le(w->item+4))) return 0;
        if (run) {
            run->banks.variables[47+slot] = save->variables[47+slot];
            memcpy(run->banks.variables+20+2*slot,save->variables+20+2*slot,2);
        }
        frame->key_received = 1;
        w->pickup = RESTORE_YAW;
    }
    return 1;
}

static int dialogue(BfmVillageWid *w, int closed, BfmSaveRecord *save, BfmGameplayRun *run,
                    BfmVillageWidFrame *frame)
{
    if (w->waiting && !closed) return 1;
    w->waiting = 0;
    for (unsigned i=0; i<3; ++i) {
        const uint8_t *command = span(w,w->cursor,8);
        if (!command) return 0;
        unsigned op = u32le(command);
        uint32_t arg = u32le(command+4);
        w->cursor += 8;
        switch (op) {
        case 0: case 1:
            frame->opened = open_page(w,arg,save);
            if (!frame->opened) return 0;
            frame->speaker = op ? 0 : WID;
            frame->speaker_x = op ? w->hero_x : w->wid_x;
            frame->portrait_on_right = op ? w->wid_x < w->hero_x : w->wid_x >= w->hero_x;
            w->waiting = 1;
            return 1;
        case 59: {
            w->item = span(w,arg,28);
            const uint8_t *wait = span(w,w->cursor,8);
            if (!w->item || u32le(w->item) != 0x8001 || u32le(w->item+4) != 89 ||
                u32le(w->item+8) || u32le(w->item+12) || u32le(w->item+16) ||
                u32le(w->item+20) || !wait || u32le(wait) != 64) return 0;
            w->cursor += 8;
            w->phase = PICKUP;
            w->pickup = QUEUED;
            return 1;
        }
        case 51:
            if (arg != UINT32_C(0x8017e914) || w->pickup != RESTORE_YAW) return 0;
            memset(save->progress+8,0,4);
            save->progress[8] = 220;
            if (run && run->has_player) memcpy(run->player.record+8,save->progress+8,4);
            break;
        case 2:
            w->phase = LEAVE;
            w->age = 0;
            return 1;
        default:
            return 0;
        }
    }
    return 0;
}

BfmVillageGroceryResult bfm_village_wid_step(BfmVillageWid *w,
    const BfmVillageIllnessWindmill *cast, BfmVillageMotion *motion, const BfmBedroomView *view,
    int square, int page_closed, int inventory_closed, BfmSaveRecord *save,
    BfmGameplayRun *run, BfmVillageWidFrame *frame)
{
    if (!w || !cast || !motion || !view || !save || !frame) return BFM_GROCERY_ERROR;
    *frame = (BfmVillageWidFrame){0};
    const BfmVillagePlayer *player = bfm_village_motion_player(motion);
    const int16_t *resident = bfm_village_illness_windmill_position(cast);
    if (!player || !resident) return BFM_GROCERY_ERROR;
    unsigned debounce = w->door_delay;
    if (w->door_delay) --w->door_delay;
    if (w->phase == IDLE) {
        unsigned story = u32le(save->progress+8);
        if (!square || !player->controlled || (player->events & BFM_VILLAGE_TRANSFER) ||
            story < 200 || story >= 230) return BFM_GROCERY_FREE;
        w->generic = !bfm_village_illness_windmill_ready(cast,save,player->root.xyz);
        if (w->generic) {
            int x = player->root.xyz[0] - s16le(w->door);
            int z = player->root.xyz[2] - s16le(w->door+4);
            if (debounce || !bfm_village_illness_windmill_closed(cast) ||
                x < -128 || x >= 128 || z < -96 || z >= 0) return BFM_GROCERY_FREE;
        }
        w->travel = bfm_bedroom_travel_from_root(w->scene,w->bytes,ROOM,&player->root);
        if (!w->travel) return BFM_GROCERY_ERROR;
        w->hero = BEFORE_WALK;
        w->waiting = w->restored = w->react_requested = 0;
        w->age = 0;
        w->phase = w->generic ? NO_ANSWER : ENTER;
    } else if (w->phase == PICKUP) {
        if (!award_player(w,motion,view,page_closed,inventory_closed,save,run,frame))
            return BFM_GROCERY_ERROR;
    } else if (!porch_player(w,motion,view,frame)) {
        return BFM_GROCERY_ERROR;
    }
    if (w->phase == ENTER)
        entry_field(w,resident,motion,view,save,run,frame);
    if (w->phase == NO_ANSWER) silent_field(w,frame);
    if (w->phase == TALK && !dialogue(w,page_closed,save,run,frame)) return BFM_GROCERY_ERROR;
    if (w->phase == LEAVE) exit_field(w,frame);
    if (w->phase == RETURN && w->restored) {
        if (!bfm_village_motion_release_staged(motion)) return BFM_GROCERY_ERROR;
        bfm_bedroom_travel_free(w->travel);
        w->travel = NULL;
        w->phase = IDLE;
        return BFM_GROCERY_RELEASED;
    }
    return BFM_GROCERY_LOCKED;
}
