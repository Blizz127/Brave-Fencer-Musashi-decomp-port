/* SPDX-License-Identifier: MIT */
#include "woods_lardwick_award.h"
#include "field_waypoint.h"
#include "woods_twinpeak_stride.h"
#include "../pc_port/forest_player_bytes.h"

#include <limits.h>
#include <string.h>

enum {
    RESCUE_VARIABLE_BASE = 58, ALL_RESCUED = 35, FINAL_RESCUE_FIGURE = 37,
    BINCHO_SOUND = 3070, MAX_BP_LIMIT = 1634
};

static const uint8_t *mounted(const uint8_t *data, size_t bytes, uint32_t base,
                              uint32_t address, size_t length)
{
    if (!data || address < base) return NULL;
    size_t offset = address - base;
    return offset <= bytes && length <= bytes - offset ? data + offset : NULL;
}

typedef struct {
    uint32_t performance;
    unsigned wait_fields;
} BfmAwardProgram;

static int read_award_program(const uint8_t *scene, size_t bytes, uint32_t base,
                              uint32_t entry, uint32_t callback, BfmAwardProgram *out)
{
    enum { START_PERFORMANCE = 59, WAIT_FIELDS = 63, CALL_REWARD = 51,
           SYNC_PLAYER = 64, RETURN_TO_ACTOR = 2 };
    enum { READY, PERFORMING, REWARDED, SYNCHRONIZED } phase = READY;
    BfmAwardProgram program = {0};
    if (entry & 3u) return 0;
    const uint8_t *record = mounted(scene, bytes, base, entry, 8);
    if (!record) return 0;
    size_t remaining = bytes - (size_t)(entry - base);

    /* The actor advances its reward timer alongside the player performance.
     * Only waits before the callback can be represented by that timer. */
    while (remaining >= 8) {
        uint32_t operand = u32le(record + 4);
        switch (u32le(record)) {
        case START_PERFORMANCE:
            if (phase != READY || (operand & 3u) ||
                !mounted(scene, bytes, base, operand, 28)) return 0;
            program.performance = operand;
            phase = PERFORMING;
            break;
        case WAIT_FIELDS:
            if (phase != PERFORMING || !operand ||
                operand > UINT_MAX - program.wait_fields) return 0;
            program.wait_fields += operand;
            break;
        case CALL_REWARD:
            if (phase != PERFORMING || !program.wait_fields || operand != callback) return 0;
            phase = REWARDED;
            break;
        case SYNC_PLAYER:
            if (phase != REWARDED || operand) return 0;
            phase = SYNCHRONIZED;
            break;
        case RETURN_TO_ACTOR:
            if (phase != SYNCHRONIZED || operand) return 0;
            *out = program;
            return 1;
        default:
            return 0;
        }
        record += 8;
        remaining -= 8;
    }
    return 0;
}

static int open_award(BfmWoodsLardwickAward *award,
    const uint8_t *scene, size_t scene_bytes, const uint8_t *main, size_t main_bytes,
    uint32_t common_script, uint32_t callback, unsigned resident_id)
{
    const uint32_t field_base = UINT32_C(0x80128158), player_base = UINT32_C(0x800cedf8);
    /* The independently decoded common program supplies these two fields. */
    BfmAwardProgram program;
    if (!award || !read_award_program(scene, scene_bytes, field_base, common_script,
                                      callback, &program)) return 0;
    const uint8_t *performance = mounted(scene, scene_bytes, field_base,
                                         program.performance, 28);
    if (!performance) return 0;
    /* This action has no item, target actor or staging position. */
    for (unsigned offset = 0; offset < 24; offset += 4)
        if (u32le(performance + offset)) return 0;
    const uint8_t *message = mounted(main, main_bytes, player_base, u32le(performance + 24), 4);
    if (!message || message[0] != 2 || message[1] != 1 || message[2] != 0) return 0;
    size_t bytes = main_bytes - (size_t)(message - main);
    /* The style and portrait operands precede the body of this unvoiced notice. */
    const uint8_t *end = memchr(message + 3, 0, bytes - 3);
    const uint32_t offer = UINT32_C(0x8010f46c) + 8 * FINAL_RESCUE_FIGURE;
    const uint8_t *figure = mounted(main, main_bytes, player_base, offer, 3);
    if (!end || !figure) return 0;
    *award = (BfmWoodsLardwickAward){
        .message = message, .message_bytes = (size_t)(end - message) + 1,
        .wait_fields = program.wait_fields, .figure_price = u16le(figure),
        .figure_trend = figure[2], .rescue_variable = RESCUE_VARIABLE_BASE + resident_id
    };
    return 1;
}

int bfm_woods_lardwick_award_open(BfmWoodsLardwickAward *award,
    const uint8_t *scene, size_t scene_bytes, const uint8_t *main, size_t main_bytes,
    uint32_t common_script)
{
    return open_award(award, scene, scene_bytes, main, main_bytes, common_script,
                      UINT32_C(0x8017cccc), 11);
}

int bfm_mine_lake_award_open(BfmWoodsLardwickAward *award,
    const uint8_t *scene, size_t scene_bytes, const uint8_t *main, size_t main_bytes,
    uint32_t common_script, unsigned resident_id)
{
    if (resident_id != 21 && resident_id != 27) return 0;
    return open_award(award, scene, scene_bytes, main, main_bytes, common_script,
                      UINT32_C(0x8017c8f4), resident_id);
}

BfmForestMessage *bfm_woods_lardwick_award_message(const BfmWoodsLardwickAward *award)
{
    return award ? bfm_field_message_borrow(award->message, award->message_bytes) : NULL;
}

enum {
    PLAYER_IDLE, PLAYER_ENTER, PLAYER_POSITION, PLAYER_FACE_CAMERA, PLAYER_NOTICE,
    PLAYER_LOWER, PLAYER_FACE_BACK, PLAYER_DONE
};

int bfm_woods_lardwick_award_begin(BfmWoodsLardwickAward *award, const BfmVillageMotion *motion)
{
    const BfmVillagePlayer *player = bfm_village_motion_player(motion);
    if (!award || !award->message || !player || award->player_phase != PLAYER_IDLE) return 0;
    award->saved_heading = player->root.yaw;
    award->player_phase = PLAYER_ENTER;
    return 1;
}

int bfm_woods_lardwick_award_step(BfmWoodsLardwickAward *award, BfmVillageMotion *motion,
    const BfmBedroomView *view, int window_closed, BfmLardwickAwardFrame *out)
{
    const BfmVillagePlayer *player = bfm_village_motion_player(motion);
    if (!award || !view || !out || !player) return 0;
    BfmLardwickAwardFrame frame = {0};
    BfmBedroomRoot root = player->root;
    root.locomotion_edge = BFM_TRAVEL_NONE;
    const uint32_t raise = UINT32_C(0x800d5da8), lower = UINT32_C(0x800d5dc0);
    uint32_t program = 0;
    unsigned phase = award->player_phase;
    int ended = award->body_ended;

    switch (phase) {
    case PLAYER_ENTER:
        phase = PLAYER_POSITION;
        break;
    case PLAYER_POSITION:
        /* The type-zero descriptor has a zero staging vector. */
        phase = PLAYER_FACE_CAMERA;
        break;
    case PLAYER_FACE_CAMERA: {
        unsigned heading = bfm_waypoint_heading((int64_t)root.xyz[0] - view->eye[0],
                                                (int64_t)root.xyz[2] - view->eye[2]);
        if (bfm_twinpeak_turn(&root, heading)) {
            program = raise;
            frame.sound = 2115;
            frame.open_notice = 1;
            phase = PLAYER_NOTICE;
        }
        break;
    }
    case PLAYER_NOTICE:
        program = window_closed ? lower : raise;
        if (window_closed) phase = PLAYER_LOWER;
        break;
    case PLAYER_LOWER:
        program = lower;
        if (award->body_ended) phase = PLAYER_FACE_BACK;
        break;
    case PLAYER_FACE_BACK:
        program = lower;
        if (bfm_twinpeak_turn(&root, award->saved_heading)) {
            phase = PLAYER_DONE;
            frame.complete = 1;
        }
        break;
    case PLAYER_DONE:
        *out = (BfmLardwickAwardFrame){.complete = 1};
        return 1;
    default:
        return 0;
    }
    if (program) {
        if (!bfm_village_motion_staged_program(motion, &root, program, &ended)) return 0;
    } else if (!bfm_village_motion_staged(motion, &root)) return 0;
    award->player_phase = phase;
    award->body_ended = ended;
    *out = frame;
    return 1;
}

static int registered(const BfmWoodsLardwickAward *award, const BfmSaveRecord *save,
                      const BfmGameplayRun *run)
{
    unsigned index = award->rescue_variable;
    return save->variables[index] || (run && run->banks.variables[index]);
}

static void write_half(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

unsigned bfm_woods_lardwick_award_bp(BfmWoodsLardwickAward *award,
    BfmSaveRecord *save, BfmGameplayRun *run)
{
    if (!award || !award->message || award->bp_applied || !save || registered(award, save, run)) return 0;
    uint8_t *progress = run && run->has_player ? run->player.record : save->progress;
    uint16_t max_bp = (uint16_t)(u16le(progress + 0x3e) + 5);
    uint16_t bp = (uint16_t)(u16le(progress + 0x40) + 9999);
    if (max_bp > MAX_BP_LIMIT) max_bp = MAX_BP_LIMIT;
    if (bp > max_bp) bp = max_bp;
    write_half(progress + 0x3e, max_bp);
    write_half(progress + 0x40, bp);
    if (progress != save->progress) memcpy(save->progress + 0x3e, progress + 0x3e, 4);
    award->bp_applied = 1;
    return BINCHO_SOUND;
}

int bfm_woods_lardwick_award_commit(const BfmWoodsLardwickAward *award,
    BfmSaveRecord *save, BfmGameplayRun *run)
{
    if (!save || !award || !award->message) return 0;
    if (registered(award, save, run)) return 1;
    if (!award->bp_applied) return 0;

    uint8_t *progress = run && run->has_player ? run->player.record : save->progress;
    uint32_t count = u32le(progress + 0x0c) + 1u;
    for (unsigned i = 0; i < 4; ++i) progress[0x0c + i] = (uint8_t)(count >> (8 * i));
    if (progress != save->progress) memcpy(save->progress + 0x0c, progress + 0x0c, 4);
    save->variables[award->rescue_variable] = 1;
    if (run) run->banks.variables[award->rescue_variable] = 1;

    if (count >= ALL_RESCUED) {
        const size_t stock = 98 + FINAL_RESCUE_FIGURE;
        const size_t price = 2 * (FINAL_RESCUE_FIGURE - 1);
        if (!(save->variables[stock] & 0x80u)) {
            save->variables[stock] = award->figure_trend | 0x80u;
            write_half(save->figure_prices + price, award->figure_price);
            if (run) {
                run->banks.variables[stock] = save->variables[stock];
                write_half(run->banks.figure_prices + price, award->figure_price);
            }
        }
    }
    return 1;
}
