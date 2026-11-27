/* SPDX-License-Identifier: MIT */
#include "woods_box_return.h"
#include "gameplay_run.h"
#include "forest_message.h"
#include "../pc_port/forest_player_bytes.h"
#include <stdlib.h>
#include <string.h>

static const uint8_t *mounted(const uint8_t *scene, size_t bytes, uint32_t address, size_t count)
{
    const uint32_t base = UINT32_C(0x80128158);
    if (address < base) return NULL;
    size_t offset = address - base;
    return offset <= bytes && count <= bytes - offset ? scene + offset : NULL;
}

int bfm_woods_box_route_open(BfmWoodsBoxRoute *route, unsigned room,
                            const uint8_t *scene, size_t bytes)
{
    if (!route || !scene || (room != 0x3025 && room != 0x3026)) return 0;
    int cleft = room == 0x3025;
    uint32_t header = cleft ? UINT32_C(0x801877c4) : UINT32_C(0x80190ce4);
    uint32_t root = cleft ? UINT32_C(0x8018ea38) : UINT32_C(0x801961e8);
    const uint8_t *binding = mounted(scene, bytes, header, 4);
    const uint8_t *field = mounted(scene, bytes, root, 0xcc);
    if (!binding || u32le(binding) != root || !field) return 0;

    unsigned slot = cleft ? 3u : 1u;
    const uint8_t *doors = mounted(scene, bytes, u32le(field + 0xc8), 4 * (slot + 1));
    if (!doors) return 0;
    uint32_t passage_address = u32le(doors + 4 * slot);
    const uint8_t *passage = mounted(scene, bytes, passage_address, 36);
    if (!passage || u32le(passage) != (cleft ? 0x00013026u : 0x00033025u)) return 0;

    BfmWoodsBoxRoute next = { .passage = passage, .room = room };
    uint32_t actor_address = u32le(field + 0xb4);
    for (;;) {
        const uint8_t *actor = mounted(scene, bytes, actor_address, 20);
        if (!actor || actor_address > UINT32_MAX - 20) return 0;
        unsigned type = u16le(actor + 6);
        if (!type) break;
        if (type == 126 && u32le(actor + 16) == passage_address) {
            if (next.passage_actor) return 0;
            next.passage_actor = actor;
        } else if (!cleft && type == 483 && u16le(actor + 8) == 0) {
            if (next.box_actor) return 0;
            next.box_actor = actor;
            next.box_destination = mounted(scene, bytes, u32le(actor + 16), 36);
            if (!next.box_destination || u32le(next.box_destination) != 0x00ff3026u) return 0;
        }
        actor_address += 20;
    }
    if (!next.passage_actor) return 0;
    if (!cleft) {
        next.box_script = mounted(scene, bytes, UINT32_C(0x801e85bc), 0x98);
        if (!next.box_actor || !next.box_script) return 0;
    }
    *route = next;
    return 1;
}

const uint8_t *bfm_woods_box_passage(const BfmWoodsBoxRoute *route,
                                    const int16_t foot[3], unsigned wall_contact)
{
    if (!route || !foot || !route->passage_actor || (wall_contact & 0xffu) != 3) return NULL;
    const uint8_t *gate = route->passage_actor;
    int dy = foot[1] - s16le(gate + 2);
    if (abs(foot[0] - s16le(gate)) > 256 || abs(foot[2] - s16le(gate + 4)) > 256 ||
        dy - 3 > 256 || dy + 1 < -256) return NULL;
    return route->passage;
}

const uint8_t *bfm_woods_box_arrival(const BfmWoodsBoxRoute *route,
                                    const uint8_t destination[36])
{
    if (!route || !destination || u16le(destination) != route->room || destination[3]) return NULL;
    if (destination[2] == (route->room == 0x3025 ? 3 : 1)) return route->passage;
    if (destination[2] == 0xff && route->box_destination &&
        memcmp(destination, route->box_destination, 36) == 0) return destination;
    return NULL;
}

static const uint32_t box_program = UINT32_C(0x801e85bc);

int bfm_woods_box_use_begin(BfmWoodsBoxUse *use, const BfmWoodsBoxRoute *route,
                            const uint8_t *main, size_t main_bytes)
{
    if (!use || use->route || !route || !route->box_script || !route->box_destination ||
        !main) return 0;
    *use = (BfmWoodsBoxUse){ .route = route, .main = main, .main_bytes = main_bytes,
                            .cursor = UINT32_C(0x801e8604) - box_program };
    return 1;
}

void bfm_woods_box_use_close(BfmWoodsBoxUse *use, unsigned answer)
{
    if (use && use->wait_for_answer) {
        use->wait_for_answer = 0;
        use->answer = answer;
    }
}

static BfmForestMessage *box_message(const BfmWoodsBoxUse *use, uint32_t address)
{
    const uint32_t main_origin = UINT32_C(0x800cedf8);
    if (address < UINT32_C(0x80113d04) || address > UINT32_C(0x80113ea4)) return NULL;
    size_t pos = address - main_origin;
    if (pos >= use->main_bytes || use->main[pos] != 5) return NULL;
    size_t bytes = use->main_bytes - pos;
    return bfm_field_message_borrow(use->main + pos, bytes < 256 ? bytes : 256);
}

int bfm_woods_box_use_step(BfmWoodsBoxUse *use, BfmGameplayRun *run,
                           const BfmSaveRecord *live, BfmForestMessage **opened, unsigned *cue)
{
    if (!use || !run || !live || !opened || !cue) return 0;
    *opened = NULL;
    *cue = 0;
    if (!use->route) return 1;
    if (use->lid_motion > 0 && ++use->openness == 32) use->lid_motion = 0;
    if (use->lid_motion < 0 && --use->openness == 0) {
        use->lid_motion = 0;
        *cue = 2184;
    }
    if (use->wait_for_answer || (use->wait && --use->wait)) return 1;

    /* The live room's short field script supplies its own messages, delays
     * and sound command. Only the three Box actions operate on native state. */
    for (unsigned commands = 0; commands < 0x98 / 8 && use->cursor <= 0x98 - 8; ++commands) {
        const uint8_t *item = use->route->box_script + use->cursor;
        uint32_t op = u32le(item), arg = u32le(item + 4);
        use->cursor += 8;
        switch (op) {
        case 1:
            *opened = box_message(use, arg);
            if (!*opened) return 0;
            use->wait_for_answer = 1;
            return 1;
        case 2:
            if (use->lid_motion || use->openness) return 0;
            use->route = NULL;
            return 1;
        case 15:
            if (!arg || arg > 255) return 0;
            use->wait = arg;
            return 1;
        case 51:
            if (arg == UINT32_C(0x80190460)) {
                if (use->lid_motion || use->openness != 0) return 0;
                use->lid_motion = 1;
                *cue = 1471;
            } else if (arg == UINT32_C(0x80190470)) {
                if (use->lid_motion || use->openness != 32) return 0;
                use->lid_motion = -1;
            } else if (arg == UINT32_C(0x80190498)) {
                if (!bfm_gameplay_remember_box(run, live, use->route->box_destination,
                                               sizeof live->destination)) return 0;
            } else return 0;
            break;
        case 54:
            if (arg != UINT32_C(0x80190480) || use->cursor > 0x98 - 8 ||
                (use->answer != 1 && use->answer != 2)) return 0;
            if (u32le(item + 8) != 0 || u32le(item + 12) != box_program) return 0;
            use->cursor = use->answer == 1 ? 0 : use->cursor + 8;
            break;
        case 65:
            if (!arg || arg > UINT16_MAX) return 0;
            *cue = arg;
            break;
        default:
            return 0;
        }
    }
    return 0;
}
