/* SPDX-License-Identifier: MIT */
#include "village_illness_grocery.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

static const uint32_t origin = UINT32_C(0x80128158);
enum { ROOM = 0x80190780u, MAYOR = 378, HOTELO = 379, REMEDY_FLAG = 276 };
enum { ARMED, RUNNING, RETURNING };
enum {
    RESIDENT_PAGE, HERO_PAGE, RETURN, WALK,
    READY = 6, TURN, SPEAK = 11, RELEASE_SHOT = 32, SHOT, RESIDENT_BODY = 35,
    CALLBACK = 51, GO_TO, CALL, ANCHOR = 55, RESET_HERO = 57, PROJECT_HERO,
    DELAY = 63, RESIDENT_FACE = 66, RESIDENT_TURN = 72
};

typedef struct { unsigned action; int16_t x, z; } HeroOrder;

struct BfmVillageGrocery {
    const uint8_t *scene, *station, *scripts;
    size_t bytes;
    BfmBedroomTravel *travel;
    uint32_t cursor, return_to;
    unsigned state, wait, page, resident_face;
    int camera_restored;
    int hero_x, hero_right, resident_x, resident_right;
    HeroOrder orders[4];
    unsigned count, restart_speech;
};

static const uint8_t *mounted(const BfmVillageGrocery *g, uint32_t address, size_t length)
{
    if (address < origin || length > g->bytes || address-origin > g->bytes-length) return NULL;
    return g->scene + (address-origin);
}

BfmVillageGrocery *bfm_village_grocery_open(const uint8_t *scene, size_t bytes)
{
    if (!scene) return NULL;
    BfmVillageGrocery *g = calloc(1,sizeof *g);
    if (!g) return NULL;
    g->scene = scene;
    g->bytes = bytes;
    const uint8_t *root = mounted(g,ROOM,0xcc);
    g->scripts = mounted(g,UINT32_C(0x8018effc),12);
    g->station = mounted(g,UINT32_C(0x8018e960),8);
    if (root && g->scripts && g->station) {
        const uint32_t first = u32le(root+0xb4), end = u32le(root+0xb8);
        const uint8_t *actors = end >= first ? mounted(g,first,end-first) : NULL;
        int mayor = 0, hotelo = 0;
        if (actors && (end-first)%20 == 0) {
            for (size_t i=0; i<end-first; i+=20) {
                unsigned type = u16le(actors+i+6);
                if (!type) break;
                if (type == MAYOR) mayor = 1;
                if (type == HOTELO) hotelo = 1;
            }
        }
        uint32_t initial = u32le(g->scripts), reminder = u32le(g->scripts+4);
        uint32_t next = u32le(g->scripts+8);
        if (mayor && hotelo && initial < reminder && reminder < next &&
            mounted(g,initial,next-initial)) return g;
    }
    free(g);
    return NULL;
}

void bfm_village_grocery_free(BfmVillageGrocery *g)
{
    if (g) bfm_bedroom_travel_free(g->travel);
    free(g);
}

int bfm_village_grocery_active(const BfmVillageGrocery *g)
{
    return g && (g->state == RUNNING || g->state == RETURNING);
}

void bfm_village_grocery_camera_restored(BfmVillageGrocery *g)
{
    if (g && g->state == RETURNING) g->camera_restored = 1;
}

static int queue(BfmVillageGrocery *g, unsigned op, const uint8_t *arg)
{
    if (g->count >= sizeof g->orders/sizeof g->orders[0]) return 0;
    g->orders[g->count++] = (HeroOrder){op,s16le(arg),s16le(arg+2)};
    return 1;
}

/* The player runs his installed command before the Mayor dequeues at most
 * one new order. The newly selected worker first runs on the next field. */
static int move_hero(BfmVillageGrocery *g, BfmVillageMotion *motion, const BfmBedroomView *view)
{
    if (!bfm_bedroom_travel_step(g->travel,view->eye,view->target)) return 0;
    const int64_t *world = bfm_bedroom_travel_position(g->travel);
    int32_t xyz[3] = {(int32_t)world[0],(int32_t)world[1],(int32_t)world[2]};
    if (!bfm_village_motion_staged_position(motion,xyz)) return 0;
    const BfmBedroomRoot *root = bfm_bedroom_travel_root(g->travel);
    if (g->restart_speech) {
        int ended;
        if (!bfm_village_motion_restart_staged_program(motion,root,UINT32_C(0x800d5a34),&ended)) return 0;
        g->restart_speech = 0;
    } else if (!bfm_village_motion_staged(motion,root)) {
        return 0;
    }

    if (root->busy || !g->count) return 1;
    HeroOrder order = g->orders[0];
    --g->count;
    memmove(g->orders,g->orders+1,g->count*sizeof order);
    switch (order.action) {
    case WALK:
        if (!bfm_bedroom_travel_walk(g->travel,order.x,order.z,0)) return 0;
        break;
    case TURN:
        if (!bfm_bedroom_travel_turn(g->travel,(unsigned)order.x,0)) return 0;
        break;
    case SPEAK:
        g->restart_speech = 1;
        return 1;
    case READY:
        /* This order marks the player ready; his selected body keeps playing. */
        return 1;
    default:
        return 0;
    }
    /* Retire travel's dispatch on the selecting actor pass. The next player
     * visit can now start walking or take the first angular step. */
    return bfm_bedroom_travel_step(g->travel,view->eye,view->target);
}

static int project_x(const BfmBedroomView *view, const int16_t xyz[3])
{
    int xy[3];
    return bfm_bedroom_project_packet(view,xyz,xy) ? xy[0]-160 : 0;
}

static int open_page(BfmVillageGrocery *g, unsigned op, uint32_t address,
    BfmSaveRecord *save, BfmVillageGroceryFrame *frame)
{
    const uint32_t text_start = UINT32_C(0x801908b0), text_end = UINT32_C(0x8019117c);
    if (address < text_start || address >= text_end) return 0;
    size_t capacity = text_end-address;
    const uint8_t *page = mounted(g,address,capacity);
    if (!page || page[0] != 5) return 0;
    /* The text records in this street use nonzero formatting operands. */
    const uint8_t *end = memchr(page,0,capacity);
    if (!end) return 0;
    frame->opened = bfm_field_message_borrow_flags(page,(size_t)(end-page)+1,save->flags);
    if (!frame->opened) return 0;
    frame->speaker = op == HERO_PAGE ? 0 : g->resident_face == 21 ? MAYOR : HOTELO;
    frame->speaker_x = op == HERO_PAGE ? g->hero_x : g->resident_x;
    frame->portrait_on_right = op == HERO_PAGE ? g->hero_right : g->resident_right;
    frame->resident_portrait = g->resident_face;
    g->page = 1;
    return 1;
}

static int resident_callback(uint32_t callback, BfmSaveRecord *save, BfmGameplayRun *run,
                             BfmVillageGroceryFrame *frame)
{
    switch (callback) {
    case UINT32_C(0x8017e534): frame->hotelo_command = 1; break;
    case UINT32_C(0x8017e510): frame->hotelo_command = 2; break;
    case UINT32_C(0x8017e57c): frame->hotelo_command = 3; break;
    case UINT32_C(0x8017e558): frame->hotelo_command = 4; break;
    case UINT32_C(0x8017e5a0): frame->hotelo_command = 5; break;
    case UINT32_C(0x8017e4e4):
        memset(save->progress+8,0,4);
        save->progress[8] = 210;
        save->flags[REMEDY_FLAG/8] |= 1u<<(REMEDY_FLAG%8);
        if (run) {
            run->story_flags[REMEDY_FLAG/8] |= 1u<<(REMEDY_FLAG%8);
            if (run->has_player) memcpy(run->player.record+8,save->progress+8,4);
        }
        break;
    default:
        return 0;
    }
    return 1;
}

BfmVillageGroceryResult bfm_village_grocery_step(BfmVillageGrocery *g,
    BfmVillageMotion *motion, const BfmBedroomView *view, int square, int cast_ready,
    int page_closed, BfmSaveRecord *save, BfmGameplayRun *run, BfmVillageGroceryFrame *frame)
{
    if (!g || !motion || !view || !save || !frame) return BFM_GROCERY_ERROR;
    *frame = (BfmVillageGroceryFrame){0};
    const BfmVillagePlayer *player = bfm_village_motion_player(motion);
    if (!player) return BFM_GROCERY_ERROR;
    if (g->state == ARMED) {
        unsigned story = u32le(save->progress+8);
        const int16_t *xyz = player->root.xyz;
        if (!square || !cast_ready || !player->controlled || (player->events & BFM_VILLAGE_TRANSFER) ||
            story < 200 || story >= 230 || xyz[0] < 560 || xyz[0] > 880 ||
            xyz[2] < 368 || xyz[2] > 560) return BFM_GROCERY_FREE;
        g->travel = bfm_bedroom_travel_from_root(g->scene,g->bytes,ROOM,&player->root);
        if (!g->travel) return BFM_GROCERY_ERROR;
        g->state = RUNNING;
        /* Square was sampled by the player before the Mayor's actor pass.
         * Start the shot, queue the walk and arm the timer on this field. */
        g->cursor = u32le(g->scripts + (story >= 210 ? 4 : 0));
    } else if (!move_hero(g,motion,view)) {
        return BFM_GROCERY_ERROR;
    }
    if (g->state == RETURNING) {
        if (!g->camera_restored) return BFM_GROCERY_LOCKED;
        if (!bfm_village_motion_release_staged(motion)) return BFM_GROCERY_ERROR;
        bfm_bedroom_travel_free(g->travel);
        *g = (BfmVillageGrocery){
            .scene = g->scene, .bytes = g->bytes, .station = g->station, .scripts = g->scripts
        };
        return BFM_GROCERY_RELEASED;
    }
    if (g->page && !page_closed) return BFM_GROCERY_LOCKED;
    g->page = 0;
    if (g->wait && --g->wait) return BFM_GROCERY_LOCKED;

    /* A resident framing subroutine returns in the same director service.
     * Every real grocery batch reaches a window, wait or end within 16 rows. */
    for (unsigned i=0; i<32; ++i) {
        const uint8_t *row = mounted(g,g->cursor,8);
        if (!row || (g->cursor&3)) return BFM_GROCERY_ERROR;
        uint32_t op = u32le(row), arg = u32le(row+4);
        g->cursor += 8;
        switch (op) {
        case RESIDENT_PAGE: case HERO_PAGE:
            return open_page(g,op,arg,save,frame) ? BFM_GROCERY_LOCKED : BFM_GROCERY_ERROR;
        case WALK: case TURN: case READY: case SPEAK:
            if (!queue(g,op,row+4)) return BFM_GROCERY_ERROR;
            break;
        case SHOT:
            frame->camera_frame = mounted(g,arg,16);
            if (!frame->camera_frame) return BFM_GROCERY_ERROR;
            frame->camera_station = g->station;
            break;
        case RELEASE_SHOT:
            frame->release_camera = 1;
            break;
        case RESIDENT_BODY:
            if (!mounted(g,arg,8)) return BFM_GROCERY_ERROR;
            frame->mayor_program = arg;
            break;
        case CALLBACK:
            if (!resident_callback(arg,save,run,frame)) return BFM_GROCERY_ERROR;
            break;
        case RESIDENT_TURN:
            frame->mayor_yaw = u16le(row+4);
            frame->mayor_turn = u16le(row+6);
            break;
        case RESIDENT_FACE:
            if (arg != 18 && arg != 21) return BFM_GROCERY_ERROR;
            g->resident_face = arg;
            frame->resident_portrait = arg;
            break;
        case ANCHOR:
            g->resident_x = s16le(row+4);
            g->resident_right = u16le(row+6) != 0;
            break;
        case RESET_HERO:
            g->hero_x = g->hero_right = 0;
            break;
        case PROJECT_HERO: {
            const int16_t station[3] = {s16le(g->station),s16le(g->station+2),s16le(g->station+4)};
            g->hero_x = project_x(view,player->root.xyz);
            g->hero_right = project_x(view,station) < g->hero_x;
            break;
        }
        case DELAY:
            if (!arg || arg > UINT16_MAX) return BFM_GROCERY_ERROR;
            g->wait = arg;
            return BFM_GROCERY_LOCKED;
        case CALL: case GO_TO:
            if (!mounted(g,arg,8) || g->return_to) return BFM_GROCERY_ERROR;
            if (op == CALL) g->return_to = g->cursor;
            g->cursor = arg;
            break;
        case RETURN:
            if (g->return_to) {
                g->cursor = g->return_to;
                g->return_to = 0;
                break;
            }
            g->state = RETURNING;
            return BFM_GROCERY_LOCKED;
        default:
            return BFM_GROCERY_ERROR;
        }
    }
    return BFM_GROCERY_ERROR;
}
