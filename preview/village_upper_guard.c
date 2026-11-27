/* SPDX-License-Identifier: MIT */
#include "village_upper_guard.h"
#include "field_waypoint.h"
#include "forest_guard_contact.h"
#include "forest_pose.h"
#include "forest_message_art.h"
#include "../pc_port/forest_player_bytes.h"

#include <stdlib.h>
#include <string.h>

enum { PARTS = 11, POSED_BYTES = PARTS * 8, IDLE = 0, WALK = 1 };
static const uint32_t scene_origin = UINT32_C(0x80128158);
static const uint32_t upper_room = UINT32_C(0x8018c794);

struct BfmVillageUpperGuard {
    MusashiForestPlayerMesh *mesh;
    BfmForestMessageArt *art;
    const uint8_t *scene;
    size_t bytes;
    const uint8_t *pose[2][20], *camera, *line;
    unsigned keys[2], clip, key, yaw;
    size_t line_bytes;
    BfmForestGuardBox box[2];
    int16_t foot[3], target[2], offset[2];
    int fractions[2], relative_turn;
    unsigned target_yaw, turn_left, timer, waits[3];
    enum { POSTED, STAGING, SPEAKING, MOVING, TURNING, SETTLING, CAMERA_RETURN } phase;
    enum { STATIONARY, TO_SIDE, FACE_SIDE } motion;
    int stepped_aside, first_conversation;
};

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t need)
{
    if (address < scene_origin || need > bytes || address-scene_origin > bytes-need) return NULL;
    return scene + (address-scene_origin);
}

int bfm_village_upper_guard_present(const BfmSaveRecord *state)
{
    return state && (state->flags[240/8] & (1u << (240%8)));
}

static const uint8_t *placement(const uint8_t *scene, size_t bytes)
{
    const uint8_t *room = mounted(scene,bytes,upper_room,0xbc);
    if (!room) return NULL;
    uint32_t first = u32le(room+0xb4), end = u32le(room+0xb8);
    if (end <= first || (end-first)%20) return NULL;
    const uint8_t *list = mounted(scene,bytes,first,end-first), *station = NULL;
    if (!list) return NULL;
    for (size_t i=0; i<end-first; i+=20) {
        unsigned kind = u16le(list+i+6);
        if (!kind) return station;
        if (kind == 468) {
            if (station) return NULL;
            station = list+i;
        }
    }
    return NULL;
}

static int read_clip(BfmVillageUpperGuard *g, unsigned clip, const uint8_t *scene,
                     size_t bytes, uint32_t address, unsigned keys)
{
    const uint8_t *seq = mounted(scene,bytes,address,(keys+1)*8);
    if (!seq || u32le(seq+keys*8) || u32le(seq+keys*8+4) != 0xffffu) return 0;
    for (unsigned i=0; i<keys; ++i) {
        uint32_t pose = u32le(seq+8*i);
        if (pose >> 24 != 0x81 || (pose & 3) || u32le(seq+8*i+4) != 1) return 0;
        g->pose[clip][i] = mounted(scene,bytes,pose & ~UINT32_C(0x01000000),POSED_BYTES);
        if (!g->pose[clip][i]) return 0;
    }
    g->keys[clip] = keys;
    return 1;
}

BfmVillageUpperGuard *bfm_village_upper_guard_open(const uint8_t *scene, size_t bytes,
                                                   const BfmSaveRecord *state)
{
    if (!scene || !bfm_village_upper_guard_present(state)) return NULL;
    const uint8_t *station = placement(scene,bytes);
    const uint8_t *shapes = mounted(scene,bytes,UINT32_C(0x80189540),32);
    const uint8_t *meeting = mounted(scene,bytes,UINT32_C(0x8018959c),144);
    if (!station || !shapes || !meeting || u32le(meeting) != 0x21 ||
        u32le(meeting+8) != 4 || u32le(meeting+0x38) != 0x4f ||
        u32le(meeting+0x48) != 0x25 || u32le(meeting+0x84) != u32le(meeting+0x2c))
        return NULL;
    BfmVillageUpperGuard *g = calloc(1,sizeof *g);
    if (!g) return NULL;
    g->scene = scene;
    g->bytes = bytes;
    unsigned parts;
    g->mesh = musashi_forest_player_mesh_from_table(scene,bytes,scene_origin,
                                                     UINT32_C(0x801bfcb4),&parts);
    g->camera = mounted(scene,bytes,u32le(meeting+4),16);
    g->line = mounted(scene,bytes,u32le(meeting+0x2c),4);
    if (!g->mesh || parts != PARTS || !g->camera || !g->line ||
        !read_clip(g,IDLE,scene,bytes,u32le(meeting+0x5c),10) ||
        !read_clip(g,WALK,scene,bytes,u32le(meeting+0x34),20)) goto fail;
    const uint8_t *end = memchr(g->line+4,0,bytes-(size_t)(g->line+4-scene));
    if (!end || g->line[0] != 5 || g->line[1] != 2 ||
        g->line[2] != 10 || g->line[3] != 10) goto fail;
    g->line_bytes = (size_t)(end+1-g->line);
    for (unsigned i=0; i<2; ++i) {
        const uint8_t *shape = shapes+16*i;
        if (u32le(shape)) goto fail;
        for (unsigned k=0; k<3; ++k) {
            g->box[i].low[k] = s16le(shape+4+4*k);
            g->box[i].high[k] = s16le(shape+6+4*k);
            if (g->box[i].low[k] > g->box[i].high[k]) goto fail;
        }
    }
    for (unsigned i=0; i<3; ++i) g->foot[i] = s16le(station+2*i);
    for (unsigned i=0; i<2; ++i) {
        g->target[i] = s16le(meeting+0x3c+2*i);
        g->offset[i] = s16le(meeting+0x0c+2*i);
    }
    g->relative_turn = s16le(meeting+0x4c);
    g->waits[0] = u32le(meeting+0x44);
    g->waits[1] = u32le(meeting+0x54);
    g->waits[2] = u32le(meeting+0x64);
    if (!g->waits[0] || !g->waits[1] || !g->waits[2]) goto fail;
    g->yaw = 3584;
    return g;
fail:
    bfm_village_upper_guard_free(g);
    return NULL;
}

void bfm_village_upper_guard_free(BfmVillageUpperGuard *g)
{
    if (g) {
        bfm_forest_message_art_free(g->art);
        musashi_forest_player_mesh_free(g->mesh);
        free(g);
    }
}

int bfm_village_upper_guard_add(const BfmVillageUpperGuard *g,
                                BfmBedroomFigure *figure, const BfmBedroomView *view)
{
    if (!g || !figure || !view) return 0;
    BfmForestTransform root;
    BfmForestPose pose;
    bfm_forest_actor_root(g->yaw,g->foot,&root);
    if (!bfm_forest_actor_pose_decode(g->pose[g->clip][g->key],POSED_BYTES,PARTS,&pose)) return 0;
    BfmForestActor body = {.mesh=g->mesh, .pose=&pose, .root=&root, .visible_parts=(1u<<PARTS)-1};
    return bfm_bedroom_figure_add_actor(figure,view,&body,NULL,NULL,0);
}

int bfm_village_upper_guard_contact(const BfmVillageUpperGuard *g,
                                    const int16_t previous[3], int16_t endpoint[3])
{
    if (!g || !previous || !endpoint) return 0;
    BfmForestTransform root;
    bfm_forest_actor_root(g->yaw,g->foot,&root);
    int16_t from[3] = {previous[0],previous[1]-16,previous[2]};
    int16_t to[3] = {endpoint[0],endpoint[1]-16,endpoint[2]}, hit[3];
    if (!bfm_forest_body_slide(&g->box[g->stepped_aside],&root,from,to,hit)) return 0;
    hit[1] += 16;
    memcpy(endpoint,hit,sizeof hit);
    return 1;
}

int bfm_village_upper_guard_begin(BfmVillageUpperGuard *g, BfmSaveRecord *save,
                                  const int16_t player[3], int square, BfmVillageGuardTalk *talk)
{
    if (!g || !save || !player || !square || !talk || g->phase != POSTED) return 0;
    int64_t radius = 0;
    for (unsigned i=0; i<3; ++i) {
        int64_t d = (int)player[i] - g->foot[i];
        radius += d*d;
    }
    if (radius >= 128*128) return 0;
    *talk = (BfmVillageGuardTalk){.first=!g->stepped_aside,.speaker_yaw=g->yaw};
    memcpy(talk->speaker,g->foot,sizeof g->foot);
    if (talk->first) {
        BfmForestTransform root;
        bfm_forest_actor_root(g->yaw,g->foot,&root);
        for (unsigned i=0; i<3; ++i) {
            int local = root.rotation[i][0]*g->offset[0] + root.rotation[i][2]*g->offset[1];
            talk->approach[i] = g->foot[i] + local/4096 - (local%4096<0);
        }
        talk->camera = g->camera;
    }
    if (save->variables[64] == 1) save->variables[64] = 2;
    g->first_conversation = talk->first;
    g->stepped_aside = 1;
    g->phase = STAGING;
    return 1;
}

int bfm_village_upper_guard_load_art(BfmVillageUpperGuard *g, const MusashiDiscMedia *disc,
                                     uint16_t tiles[512][1024])
{
    if (!g || !disc || !tiles) return 0;
    if (g->art) return 1;
    /* This village message uses portrait ten. The host caches four faces.
     * Snapshot the mounted face temporarily in the resident cache slot, then
     * restore the shared VRAM rows for other characters in this room. */
    uint16_t saved[40][8], palette[16];
    for (unsigned y=0; y<40; ++y) {
        memcpy(saved[y], &tiles[400+y][456], sizeof saved[y]);
        memcpy(&tiles[400+y][456], &tiles[440+y][456], sizeof saved[y]);
    }
    memcpy(palette,&tiles[457][352],sizeof palette);
    memcpy(&tiles[457][352],&tiles[465][352],sizeof palette);
    g->art = bfm_castle_message_art_load(disc,g->scene,g->bytes,tiles,0x5da74);
    for (unsigned y=0; y<40; ++y) memcpy(&tiles[400+y][456],saved[y],sizeof saved[y]);
    memcpy(&tiles[457][352],palette,sizeof palette);
    return g->art != NULL;
}

BfmForestMessage *bfm_village_upper_guard_message(BfmVillageUpperGuard *g, int approach_ready)
{
    if (!g || g->phase != STAGING || (g->first_conversation && !approach_ready)) return NULL;
    /* Keep the mounted text and border commands. Assign portrait ten to
     * the private art's resident slot, which was loaded from its actual
     * village VRAM coordinates. */
    uint8_t *program = malloc(g->line_bytes);
    if (!program) return NULL;
    memcpy(program,g->line,g->line_bytes);
    program[3] = 2;
    BfmForestMessage *m = bfm_field_message_open(program,g->line_bytes);
    free(program);
    if (m) g->phase = SPEAKING;
    return m;
}

int bfm_village_upper_guard_draw_message(const BfmVillageUpperGuard *g,
    const BfmBedroomView *view, const int16_t listener[3], const BfmForestMessageFrame *frame,
    uint32_t pixels[240][320])
{
    if (!g || !g->art || !view || !listener || !frame || !pixels) return 0;
    int speaker[3], hero[3];
    if (!bfm_bedroom_project(view,g->foot,speaker) || !bfm_bedroom_project(view,listener,hero))
        return 0;
    int right = speaker[0] >= hero[0];
    return bfm_forest_message_art_draw_placed(g->art,frame,(int16_t)(speaker[0]-160),right,pixels);
}

void bfm_village_upper_guard_close(BfmVillageUpperGuard *g)
{
    if (!g || g->phase != SPEAKING) return;
    if (!g->first_conversation) {
        g->phase = POSTED;
        return;
    }
    g->clip = WALK;
    g->key = 0;
    g->motion = TO_SIDE;
    g->turn_left = 16;
    g->phase = MOVING;
    g->timer = g->waits[0];
}

static void turn_toward(BfmVillageUpperGuard *g, unsigned goal, int divisor)
{
    int d = (int)goal - (int)g->yaw;
    if (d > 2048) d -= 4096;
    if (d < -2048) d += 4096;
    int step = d/divisor;
    if (!step) step = (d>0) - (d<0);
    g->yaw = (g->yaw + step) & 4095;
}

static void walk_aside(BfmVillageUpperGuard *g)
{
    int dx = g->foot[0] - g->target[0], dz = g->foot[2] - g->target[1];
    unsigned heading = bfm_waypoint_heading(dx,dz);
    if (g->turn_left) {
        turn_toward(g,heading,4);
        --g->turn_left;
        return;
    }
    g->yaw = heading;
    BfmForestTransform root;
    bfm_forest_actor_root(g->yaw,g->foot,&root);
    /* The passage and both ends of Lumpwood's short walk share a level
     * floor. Preserve sub-unit progress along each axis between fields. */
    for (unsigned i=0; i<2; ++i) {
        unsigned axis = 2*i;
        int step = g->fractions[i] - 6*root.rotation[axis][2];
        int whole = step/4096 - (step%4096<0);
        g->foot[axis] += whole;
        g->fractions[i] = step-whole*4096;
    }
    dx = g->foot[0] - g->target[0];
    dz = g->foot[2] - g->target[1];
    if ((int64_t)dx*dx + (int64_t)dz*dz <= 256) g->motion = STATIONARY;
}

int bfm_village_upper_guard_step(BfmVillageUpperGuard *g)
{
    if (!g) return 0;
    g->key = (g->key+1)%g->keys[g->clip];
    if (g->motion == TO_SIDE) walk_aside(g);
    if (g->motion == FACE_SIDE) {
        turn_toward(g,g->target_yaw,6);
        if (!--g->turn_left) {
            g->yaw = g->target_yaw;
            g->motion = STATIONARY;
        }
    }
    if (!g->timer || --g->timer) return 0;
    switch (g->phase) {
    case MOVING:
        g->target_yaw = (g->yaw + g->relative_turn) & 4095;
        g->turn_left = 30;
        g->motion = FACE_SIDE;
        g->phase = TURNING;
        g->timer = g->waits[1];
        break;
    case TURNING:
        g->clip = IDLE;
        g->key = 0;
        g->phase = SETTLING;
        g->timer = g->waits[2];
        break;
    case SETTLING:
        g->phase = CAMERA_RETURN;
        return 1;
    default:
        break;
    }
    return 0;
}

void bfm_village_upper_guard_camera_done(BfmVillageUpperGuard *g)
{
    if (g && g->phase == CAMERA_RETURN) g->phase = POSTED;
}

int bfm_village_upper_guard_busy(const BfmVillageUpperGuard *g)
{
    return g && g->phase != POSTED;
}
