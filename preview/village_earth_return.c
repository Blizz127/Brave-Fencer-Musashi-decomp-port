/* SPDX-License-Identifier: MIT */
#include "village_earth_return.h"
#include "village_earth_return_cast.h"
#include "village_earth_portrait.h"
#include "village_talk_camera.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

static const uint32_t scene_base = UINT32_C(0x80128158);
static const uint32_t module_base = UINT32_C(0x801cbb50);
static const uint32_t player_base = UINT32_C(0x800cedf8);
static const uint32_t upper_room = UINT32_C(0x8018c794);
enum { SPEAK_KEYS = 20 };
enum { CAPTURE_TURN_POINT = 1, CAPTURE_SPEAKERS = 2 };
typedef enum {
    REGISTERED, APPROACH, TALK, RESTORE_VIEW, RETURN_MUSIC, FINISHED
} ReturnPhase;

struct BfmVillageEarthReturn {
    BfmSaveRecord *save;
    const uint8_t *scene;
    size_t bytes, module_bytes;
    uint8_t *module;
    const uint8_t *script, *end;
    BfmVillageEarthReturnCast *cast;
    BfmVillageEarthPortrait *portraits;
    BfmBedroomTravel *travel;
    BfmVillageTalkCamera shot;
    ReturnPhase phase;
    unsigned wait, sound, portrait, speaker;
    uint32_t speak_keys[SPEAK_KEYS];
    unsigned speaking_field;
    int speaking, window, hud, score_ready;
    unsigned pending_points;
    int turning_to_jon;
    int16_t look_x, look_z;
    int placed[2], explicit_offset, explicit_right;
    int16_t speaker_xyz[2][3];
};

static const uint8_t *mounted(const uint8_t *data, size_t size, uint32_t base,
                              uint32_t address, size_t span)
{
    if (!data || address < base || span > size || address - base > size - span) return NULL;
    return data + (address - base);
}

int bfm_village_earth_return_pending(const BfmSaveRecord *save)
{
    if (!save) return 0;
    uint32_t story = u32le(save->progress + 8);
    return story == 133 || story == 134;
}

static int has_jon(const uint8_t *scene, size_t bytes)
{
    const uint8_t *room = mounted(scene,bytes,scene_base,upper_room,0xbc);
    if (!room) return 0;
    uint32_t start = u32le(room+0xb4), end = u32le(room+0xb8);
    if (end < start || (end-start) % 20) return 0;
    const uint8_t *actors = mounted(scene,bytes,scene_base,start,end-start);
    if (!actors) return 0;
    for (uint32_t offset = 0; offset < end-start; offset += 20)
        if (u16le(actors+offset+6) == 328) return 1;
    return 0;
}

static int bind_speaking(BfmVillageEarthReturn *e, const uint8_t *main, size_t bytes)
{
    const uint8_t *program = mounted(main,bytes,player_base,UINT32_C(0x800d5a34),
                                     (5+SPEAK_KEYS+2)*4);
    if (!program || u32le(program) != 4 || u32le(program+8) != 19 ||
        u32le(program+12) != 15 || u32le(program+16) != 0 ||
        u32le(program+(5+SPEAK_KEYS)*4) != 6 || u32le(program+(6+SPEAK_KEYS)*4) != 3)
        return 0;
    for (unsigned i=0; i<SPEAK_KEYS; ++i) {
        uint32_t address = u32le(program+(5+i)*4);
        if (!mounted(main,bytes,player_base,address,256)) return 0;
        e->speak_keys[i] = address+16;
    }
    return 1;
}

BfmVillageEarthReturn *bfm_village_earth_return_open(const MusashiDiscMedia *disc,
    const uint8_t *scene, size_t bytes, const uint8_t *main, size_t main_bytes, BfmSaveRecord *save)
{
    if (!bfm_village_earth_return_pending(save) || !has_jon(scene,bytes)) return NULL;
    BfmVillageEarthReturn *e = calloc(1,sizeof *e);
    if (!e) return NULL;
    e->scene = scene;
    e->bytes = bytes;
    e->save = save;
    e->module = bfm_village_event_data(disc,56,&e->module_bytes);
    e->cast = bfm_village_earth_return_cast_open(scene,bytes);
    if (!e->module || !e->cast || !bind_speaking(e,main,main_bytes)) {
        bfm_village_earth_return_free(e);
        return NULL;
    }
    return e;
}

void bfm_village_earth_return_free(BfmVillageEarthReturn *e)
{
    if (!e) return;
    bfm_village_earth_portrait_free(e->portraits);
    bfm_village_earth_return_cast_free(e->cast);
    bfm_bedroom_travel_free(e->travel);
    free(e->module);
    free(e);
}

int bfm_village_earth_return_load_art(BfmVillageEarthReturn *e,
    const MusashiDiscMedia *disc, uint16_t tiles[512][1024])
{
    if (!e || e->portraits) return 1;
    e->portraits = bfm_village_earth_portrait_open(disc,e->scene,e->bytes,tiles);
    return e->portraits != NULL;
}

int bfm_village_earth_return_active(const BfmVillageEarthReturn *e)
{
    return e && e->phase != REGISTERED && e->phase != FINISHED;
}

int bfm_village_earth_return_message_closed(BfmVillageEarthReturn *e)
{
    if (!e || !e->window) return 0;
    e->window = 0;
    return 1;
}

int bfm_village_earth_return_take_hud(BfmVillageEarthReturn *e)
{
    if (!e) return 0;
    int request = e->hud;
    e->hud = 0;
    return request;
}

unsigned bfm_village_earth_return_take_sound(BfmVillageEarthReturn *e)
{
    if (!e) return 0;
    unsigned sound = e->sound;
    e->sound = 0;
    return sound;
}

int bfm_village_earth_return_add(const BfmVillageEarthReturn *e,
                                  BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    return !e || e->phase == FINISHED || bfm_village_earth_return_cast_add(e->cast,figure,view);
}

static int flag266(const BfmVillageEarthReturn *e)
{
    return !!(e->save->flags[266/8] & (1u << (266%8)));
}

static int start_script(BfmVillageEarthReturn *e, ReturnPhase phase)
{
    const uint32_t prelude = UINT32_C(0x80189a6c), talk = UINT32_C(0x80189adc);
    uint32_t start = phase == APPROACH ? prelude : talk;
    uint32_t length = (phase == APPROACH ? talk : UINT32_C(0x80189bec))-start;
    e->script = mounted(e->scene,e->bytes,scene_base,start,length);
    if (!e->script) return 0;
    e->end = e->script+length;
    e->phase = phase;
    return 1;
}

static int cast_command(BfmVillageEarthReturn *e, BfmEarthCastCommand command, uint32_t arg)
{
    return bfm_village_earth_return_cast_command(e->cast,command,arg,
                                                  bfm_bedroom_travel_root(e->travel)->xyz);
}

static int cast_callback(BfmVillageEarthReturn *e, uint32_t address)
{
    switch (address) {
    case UINT32_C(0x80182e04): return cast_command(e,BFM_EARTH_CAST_IDLE,0);
    case UINT32_C(0x80182e5c): return cast_command(e,BFM_EARTH_CAST_SPEAK,0);
    case UINT32_C(0x80182298): return cast_command(e,BFM_EARTH_CAST_CALL_DOG,0);
    case UINT32_C(0x801822f8): e->sound = 0x525; return 1;
    case UINT32_C(0x80182e30): return cast_command(e,BFM_EARTH_CAST_WALK_POSE,0);
    case UINT32_C(0x801822c8): return cast_command(e,BFM_EARTH_CAST_RELEASE_DOG,0);
    default: return 0;
    }
}

static int begin_window(BfmVillageEarthReturn *e, unsigned speaker, uint32_t address,
                        BfmForestMessage **opened)
{
    const uint8_t *text = mounted(e->module,e->module_bytes,module_base,address,1);
    if (!opened || *opened || !text || text[0] != 5) return 0;
    *opened = bfm_field_message_borrow_flags(text,e->module_bytes-(address-module_base),
                                            e->save->flags);
    if (!*opened) return 0;
    e->window = 1;
    e->speaker = speaker;
    return 1;
}

static int take_shot(BfmVillageEarthReturn *e, uint32_t address, const BfmVillageCamera *camera)
{
    const uint8_t *record = mounted(e->scene,e->bytes,scene_base,address,16);
    if (!record) return 0;
    int eye[3], target[3];
    for (unsigned axis=0; axis<3; ++axis) {
        target[axis] = s16le(record+axis*2);
        eye[axis] = s16le(record+8+axis*2);
    }
    return bfm_village_talk_camera_world(&e->shot,eye,target,camera,camera);
}

/* Read the authored field-script records from the room. Its walk and actor
 * jobs continue in parallel with each timed pause and each modal window. */
static int run_script(BfmVillageEarthReturn *e, BfmVillageCamera *camera, BfmForestMessage **opened)
{
    if (e->window || (e->wait && --e->wait)) return 1;
    while (e->script < e->end) {
        uint32_t op = u32le(e->script), arg = u32le(e->script+4);
        e->script += 8;
        switch (op) {
        case 0: case 1: return begin_window(e,op,arg,opened);
        case 2:
            if (e->script != e->end) return 0;
            if (e->phase == APPROACH) return start_script(e,TALK);
            if (e->phase != TALK || !flag266(e)) return 0;
            e->phase = RESTORE_VIEW;
            return 1;
        case 3:
            if (!bfm_bedroom_travel_walk(e->travel,(int16_t)arg,(int16_t)(arg>>16),0)) return 0;
            break;
        case 0x0a:
            e->pending_points |= CAPTURE_TURN_POINT;
            break;
        case 0x0b: e->speaking = 1; break;
        case 0x0f: case 0x3f:
            if (!arg || arg > 65535) return 0;
            e->wait = arg;
            return 1;
        case 0x1e:
            if (!take_shot(e,arg,camera)) return 0;
            break;
        case 0x20: bfm_village_talk_camera_release(&e->shot); break;
        case 0x25:
            if (!cast_command(e,BFM_EARTH_CAST_TURN,arg)) return 0;
            break;
        case 0x26:
            if (!cast_command(e,BFM_EARTH_CAST_FACE_HERO,arg)) return 0;
            break;
        case 0x27:
            if (!cast_command(e,BFM_EARTH_CAST_WALK,arg)) return 0;
            break;
        case 0x28:
            if (!cast_command(e,BFM_EARTH_CAST_APPROACH,arg)) return 0;
            break;
        case 0x2c:
            if (arg != 4 || !cast_command(e,BFM_EARTH_CAST_FINISH_LOOK,arg)) return 0;
            break;
        case 0x30:
            if (arg || !cast_command(e,BFM_EARTH_CAST_HIDE_JON,arg)) return 0;
            break;
        case 0x33:
            if (!cast_callback(e,arg)) return 0;
            break;
        case 0x37:
            e->placed[0] = 2;
            e->explicit_offset = (int16_t)arg;
            e->explicit_right = (arg>>16) != 0;
            break;
        case 0x39: case 0x3a:
            e->placed[op-0x39] = 1;
            e->pending_points |= CAPTURE_SPEAKERS;
            break;
        case 0x42:
            if (arg != 2 && arg != 24) return 0;
            e->portrait = arg;
            break;
        case 0x4a: e->hud = 1; break;
        case 0x4b: e->hud = -1; break;
        default: return 0;
        }
    }
    return 0;
}

static unsigned ambient(const BfmVillageEarthReturn *e)
{
    return e->save->progress[0x36] == 1 ? 0x0fu : 0x10u;
}

unsigned bfm_village_earth_return_score(const BfmVillageEarthReturn *e, unsigned ordinary)
{
    if (!e || e->phase == FINISHED) return ordinary;
    if (e->phase < RETURN_MUSIC) return 0x93;
    return e->phase == RETURN_MUSIC && e->wait ? 0 : ambient(e);
}

void bfm_village_earth_return_score_applied(BfmVillageEarthReturn *e, unsigned selector)
{
    if (e && e->phase == RETURN_MUSIC && !e->wait && selector == ambient(e))
        e->score_ready = 1;
}

int bfm_village_earth_return_draw_window(const BfmVillageEarthReturn *e,
    const BfmBedroomView *view, const BfmForestMessageFrame *frame, uint32_t pixels[240][320])
{
    if (!e || !e->window || !e->portraits || !view || !frame || !pixels) return 0;
    int offset, right;
    if (e->placed[e->speaker] == 2) {
        offset = e->explicit_offset;
        right = e->explicit_right;
    } else {
        int speaker[3], listener[3];
        if (!e->placed[e->speaker] ||
            !bfm_bedroom_project(view,e->speaker_xyz[e->speaker],speaker) ||
            !bfm_bedroom_project(view,e->speaker_xyz[1-e->speaker],listener)) return 0;
        offset = speaker[0]-160;
        right = speaker[0] >= listener[0];
    }
    return bfm_village_earth_portrait_draw(e->portraits,e->portrait,frame,(int16_t)offset,right,pixels);
}

int bfm_village_earth_return_step(BfmVillageEarthReturn *e, BfmVillageMotion *motion,
    BfmVillageCamera *camera, BfmGameplayRun *run, int player_ready, BfmForestMessage **opened)
{
    if (!e || e->phase == FINISHED) return 1;
    if (!motion || !camera || !run) return 0;
    const BfmVillagePlayer *player = bfm_village_motion_player(motion);
    if (!player) return 0;
    /* Jon's text can set this flag during the previous message update.
     * Preserve the independent bits owned by other live actors. */
    if (flag266(e)) run->story_flags[266/8] |= 1u << (266%8);
    if (e->phase == REGISTERED) {
        if (!player_ready)
            return bfm_village_earth_return_cast_step(e->cast,run,flag266(e)) ? 1 : 0;
        e->travel = bfm_bedroom_travel_from_root(e->scene,e->bytes,upper_room,&player->root);
        if (!e->travel || !start_script(e,APPROACH)) return 0;
    }
    /* Player and camera updates precede the actors which submit commands
     * for them. Keep updating both during the voiced windows. */
    if (!bfm_bedroom_travel_step(e->travel,camera->eye,camera->target)) return 0;
    if (e->turning_to_jon &&
        bfm_bedroom_travel_turn_to_point(e->travel,e->look_x,e->look_z))
        e->turning_to_jon = 0;
    const BfmBedroomRoot *root = bfm_bedroom_travel_root(e->travel);
    if (e->speaking) {
        if (!bfm_village_motion_staged_model(motion,root,e->speak_keys[e->speaking_field],15,0,0))
            return 0;
        e->speaking_field = (e->speaking_field+1) % SPEAK_KEYS;
    } else if (!bfm_village_motion_staged(motion,root)) return 0;

    if (e->shot.phase) bfm_village_talk_camera_step(&e->shot,camera);
    else if (e->phase == RETURN_MUSIC) bfm_village_camera_lower(camera,root->xyz,0);

    if (e->phase == APPROACH || e->phase == TALK) {
        if (!run_script(e,camera,opened)) return 0;
    } else if (e->phase == RESTORE_VIEW && !e->shot.phase) {
        /* The player has listened and the cached room camera has returned.
         * The dialogue already wrote its own separate persistent flag. */
        e->save->progress[8] = 135;
        if (run->has_player) {
            uint8_t *story = run->player.record+8;
            story[0] = 135;
            story[1] = story[2] = story[3] = 0;
        }
        e->phase = RETURN_MUSIC;
        e->wait = 30;
    } else if (e->phase == RETURN_MUSIC) {
        if (e->wait) --e->wait;
        else if (e->score_ready) {
            if (!bfm_village_motion_place(motion,root)) return 0;
            e->phase = FINISHED;
            return 2;
        }
    }
    if (!bfm_village_earth_return_cast_step(e->cast,run,flag266(e))) return 0;
    if (e->pending_points) {
        /* The field actor finishes this update's stride before the player's
         * queued look and the two balloon placements sample his position. */
        const int16_t *jon = bfm_village_earth_return_cast_position(e->cast,0);
        if (!jon) return 0;
        if (e->pending_points & CAPTURE_TURN_POINT) {
            e->look_x = jon[0];
            e->look_z = jon[2];
            e->turning_to_jon = 1;
        }
        if (e->pending_points & CAPTURE_SPEAKERS) {
            memcpy(e->speaker_xyz[0],jon,sizeof e->speaker_xyz[0]);
            memcpy(e->speaker_xyz[1],root->xyz,sizeof e->speaker_xyz[1]);
        }
        e->pending_points = 0;
    }
    return 2;
}
