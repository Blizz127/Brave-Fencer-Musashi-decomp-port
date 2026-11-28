/* SPDX-License-Identifier: MIT */
#include "village_illness_mine_gate.h"
#include "valley_hinge.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>

enum { MINE_KEY = 89, OPEN_FLAG = 246, SWING_FIELDS = 48 };
enum { READY, DIRECTING, RETURNING };
static const uint32_t scene_base = UINT32_C(0x80128158);

typedef struct {
    MusashiForestPlayerMesh *mesh;
    BfmForestGuardBox box;
    BfmForestTransform transform;
    int16_t pivot[3];
    unsigned turn;
} MineLeaf;

struct BfmVillageMineGate {
    const uint8_t *scene, *station, *exit;
    size_t bytes;
    MineLeaf leaves[2];
    BfmForestPose pose;
    unsigned phase, waiting, page, choice, opening;
    uint32_t cursor, return_to;
};

static const uint8_t *mounted(const BfmVillageMineGate *g, uint32_t address, size_t n)
{
    if (address < scene_base || n > g->bytes || address-scene_base > g->bytes-n) return NULL;
    return g->scene + (address-scene_base);
}

void bfm_village_mine_gate_free(BfmVillageMineGate *g)
{
    if (!g) return;
    for (unsigned i=0; i<2; ++i) musashi_forest_player_mesh_free(g->leaves[i].mesh);
    free(g);
}

static void position_leaf(MineLeaf *leaf, unsigned index)
{
    unsigned yaw = (index ? 32u : 0u-32u) * leaf->turn;
    bfm_forest_actor_root(yaw & 4095u,leaf->pivot,&leaf->transform);
}

BfmVillageMineGate *bfm_village_mine_gate_open(const uint8_t *scene, size_t bytes,
                                               const BfmSaveRecord *save)
{
    if (!scene || !save) return NULL;
    BfmVillageMineGate *g = calloc(1,sizeof *g);
    if (!g) return NULL;
    g->scene = scene;
    g->bytes = bytes;
    const uint8_t *room = mounted(g,UINT32_C(0x80190780),0xcc);
    if (!room) goto fail;
    uint32_t first=u32le(room+0xb4), end=u32le(room+0xb8);
    const uint8_t *rows = end>=first ? mounted(g,first,end-first) : NULL;
    const uint8_t *exits = mounted(g,u32le(room+0xc8),16*4);
    if (!rows || (end-first)%20 || !exits) goto fail;
    g->exit = mounted(g,u32le(exits+15*4),36);
    g->station = mounted(g,UINT32_C(0x80190090),16);
    const uint8_t *boxes = mounted(g,UINT32_C(0x801900a0),32);
    if (!g->station || !boxes || !g->exit || u32le(g->exit)!=UINT32_C(0x02003043)) goto fail;
    const int16_t zero[3] = {0};
    bfm_forest_actor_root(0,zero,&g->pose.local[0]);
    unsigned placed=0;
    for (uint32_t off=0; off<end-first; off+=20) {
        const uint8_t *row=rows+off;
        unsigned kind=u16le(row+6), id=u16le(row+8);
        if (!kind) break;
        if (kind!=498) continue;
        if (id>1 || id!=placed) goto fail;
        ++placed;
        MineLeaf *leaf=&g->leaves[id];
        const uint8_t *box=boxes+16*id;
        if (u32le(box) || u16le(g->station+8*id+6)) goto fail;
        for (unsigned axis=0; axis<3; ++axis) {
            leaf->pivot[axis]=s16le(g->station+8*id+2*axis);
            leaf->box.low[axis]=s16le(box+4+4*axis);
            leaf->box.high[axis]=s16le(box+6+4*axis);
            if (leaf->box.low[axis]>=leaf->box.high[axis]) goto fail;
        }
        unsigned parts;
        uint32_t model=id ? UINT32_C(0x801ba8e4) : UINT32_C(0x801ba7f4);
        leaf->mesh=musashi_forest_player_mesh_from_table(scene,bytes,scene_base,model,&parts);
        if (!leaf->mesh || parts!=1) goto fail;
        if ((save->flags[OPEN_FLAG/8] & (1u<<(OPEN_FLAG%8))) || (int32_t)u32le(save->progress+8)>=230)
            leaf->turn=SWING_FIELDS;
        position_leaf(leaf,id);
    }
    if (placed==2) return g;
fail:
    bfm_village_mine_gate_free(g);
    return NULL;
}

int bfm_village_mine_gate_active(const BfmVillageMineGate *g)
{
    return g && g->phase!=READY;
}

static int key_slot(const BfmSaveRecord *save)
{
    for (int slot=47; slot<59; ++slot) if (save->variables[slot]==MINE_KEY) return slot;
    return -1;
}

static void unlock(BfmVillageMineGate *g, BfmSaveRecord *save, BfmGameplayRun *run)
{
    const unsigned mask=1u<<(OPEN_FLAG%8);
    int slot=key_slot(save);
    if (slot>=0) save->variables[slot]=0;
    save->flags[OPEN_FLAG/8] |= mask;
    if (run) {
        if (slot>=0) run->banks.variables[slot]=0;
        run->story_flags[OPEN_FLAG/8] |= mask;
    }
    /* The second placed leaf visits its actor after this callback. */
    g->opening=1;
    g->leaves[1].turn=1;
    position_leaf(&g->leaves[1],1);
}

static BfmForestMessage *open_page(const BfmVillageMineGate *g, uint32_t address,
                                   BfmSaveRecord *save)
{
    /* The voice timer contains an embedded zero byte; the next mounted page
     * bounds the stream even when its text is interrupted. */
    static const uint32_t starts[]={0x801bd8c8u,0x801bd8f4u,0x801bd938u,
                                    0x801bd964u,0x801bd990u,0x801bd9b8u,0x801bd9ecu};
    for (unsigned i=0; i+1<sizeof starts/sizeof *starts; ++i) {
        if (address!=starts[i]) continue;
        size_t n=starts[i+1]-address;
        const uint8_t *page=mounted(g,address,n);
        if (!page || page[0]!=5) return NULL;
        return bfm_field_message_borrow_flags(page,n,save->flags);
    }
    return NULL;
}

static BfmMineGateResult direct(BfmVillageMineGate *g,BfmSaveRecord *save,BfmGameplayRun *run,
                                BfmMineGateFrame *f)
{
    /* The short mounted director runs through nonblocking cues on this actor
     * visit and yields at a message or field timer. */
    for (unsigned i=0; i<24; ++i) {
        if (g->cursor<UINT32_C(0x801900c0) || g->cursor>=UINT32_C(0x801901d0) || (g->cursor&7))
            return BFM_MINE_GATE_ERROR;
        const uint8_t *row=mounted(g,g->cursor,8);
        if (!row) return BFM_MINE_GATE_ERROR;
        uint32_t op=u32le(row), arg=u32le(row+4);
        g->cursor+=8;
        switch (op) {
        case 1:
            f->opened=open_page(g,arg,save);
            if (!f->opened) return BFM_MINE_GATE_ERROR;
            g->page=arg==UINT32_C(0x801bd8f4) ? 2 : 1;
            return BFM_MINE_GATE_BUSY;
        case 2:
            if (g->return_to) {
                g->cursor=g->return_to;
                g->return_to=0;
                break;
            }
            g->phase=RETURNING;
            return BFM_MINE_GATE_BUSY;
        case 4: case 7: case 14: {
            if (f->order_count>=2) return BFM_MINE_GATE_ERROR;
            BfmMineGateOrder *order=&f->orders[f->order_count++];
            if (op==4) {
                order->type=BFM_MINE_GATE_WALK;
                order->x=(int16_t)(g->leaves[0].pivot[0]+s16le(row+4));
                order->z=(int16_t)(g->leaves[0].pivot[2]+s16le(row+6));
            } else if (op==7) {
                order->type=BFM_MINE_GATE_TURN;
                order->yaw=u16le(row+4)&4095u;
            } else {
                order->type=BFM_MINE_GATE_POSE;
                order->body_program=UINT32_C(0x800d5934);
            }
            break;
        }
        case 32: f->release_camera=1; break;
        case 33:
            f->camera_station=g->station;
            f->camera_frame=mounted(g,arg,16);
            if (!f->camera_frame) return BFM_MINE_GATE_ERROR;
            break;
        case 51:
            if (arg==UINT32_C(0x80181b20)) f->sound=0x7e0;
            else if (arg==UINT32_C(0x80181a98)) {
                f->sound=0x7a0;
                unlock(g,save,run);
            } else return BFM_MINE_GATE_ERROR;
            break;
        case 52: g->cursor=arg; break;
        case 53:
            if (g->return_to) return BFM_MINE_GATE_ERROR;
            g->return_to=g->cursor;
            g->cursor=arg;
            break;
        case 54: {
            if (arg!=UINT32_C(0x80181b08)) return BFM_MINE_GATE_ERROR;
            const uint8_t *target=mounted(g,g->cursor,8);
            if (!target) return BFM_MINE_GATE_ERROR;
            g->cursor=g->choice==2 ? u32le(target+4) : g->cursor+8;
            break;
        }
        case 63:
            if (!arg || arg>UINT16_MAX) return BFM_MINE_GATE_ERROR;
            g->waiting=arg;
            return BFM_MINE_GATE_BUSY;
        default: return BFM_MINE_GATE_ERROR;
        }
    }
    return BFM_MINE_GATE_ERROR;
}

BfmMineGateResult bfm_village_mine_gate_step(BfmVillageMineGate *g,const int16_t foot[3],
    int can_interact,int square_edge,int page_closed,unsigned answer,
    int player_ready,int camera_restored,BfmSaveRecord *save,BfmGameplayRun *run,BfmMineGateFrame *f)
{
    if (!g || !foot || !save || !f) return BFM_MINE_GATE_ERROR;
    *f=(BfmMineGateFrame){0};
    if (g->opening) {
        for (unsigned i=0; i<2; ++i) {
            if (g->leaves[i].turn<SWING_FIELDS) {
                ++g->leaves[i].turn;
                position_leaf(&g->leaves[i],i);
            }
        }
        g->opening=g->leaves[0].turn<SWING_FIELDS;
    }
    if (g->phase==READY) {
        if (!can_interact || !square_edge || g->leaves[0].turn ||
            foot[0]<1024 || foot[0]>1280 || foot[1]<-512 || foot[2]<1408 || foot[2]>1536)
            return BFM_MINE_GATE_FREE;
        g->phase=DIRECTING;
        g->cursor=key_slot(save)<0 ? UINT32_C(0x80190110) : UINT32_C(0x80190128);
    } else if (g->phase==RETURNING) {
        if (!player_ready || !camera_restored) return BFM_MINE_GATE_BUSY;
        g->phase=READY;
        return BFM_MINE_GATE_RELEASED;
    }
    if (g->page) {
        if (!page_closed) return BFM_MINE_GATE_BUSY;
        if (g->page==2) {
            if (answer!=1 && answer!=2) return BFM_MINE_GATE_ERROR;
            g->choice=answer;
        }
        g->page=0;
    }
    if (g->waiting && --g->waiting) return BFM_MINE_GATE_BUSY;
    return direct(g,save,run,f);
}

int bfm_village_mine_gate_block_body(const BfmVillageMineGate *g,const int16_t from[3],
                                     int16_t to[3],unsigned tick,int airborne)
{
    if (!g || !from || !to) return 0;
    int touched=0;
    for (unsigned i=0; i<2; ++i)
        touched |= bfm_valley_box_block_body(&g->leaves[i].box,&g->leaves[i].transform,
                                             from,to,tick,airborne);
    return touched;
}

int bfm_village_mine_gate_draw(const BfmVillageMineGate *g,BfmBedroomFigure *figure,
                               const BfmBedroomView *view)
{
    if (!g || !figure || !view) return 0;
    for (unsigned i=0; i<2; ++i) {
        BfmForestActor actor={.mesh=g->leaves[i].mesh,.pose=&g->pose,
                              .root=&g->leaves[i].transform,.visible_parts=1};
        if (!bfm_bedroom_figure_add_actor(figure,view,&actor,NULL,NULL,0)) return 0;
    }
    return 1;
}

const uint8_t *bfm_village_mine_gate_passage(const BfmVillageMineGate *g,unsigned material)
{
    if (!g || g->phase!=READY || (material&255u)!=0x13 ||
        g->leaves[0].turn!=SWING_FIELDS || g->leaves[1].turn!=SWING_FIELDS) return NULL;
    return g->exit;
}
