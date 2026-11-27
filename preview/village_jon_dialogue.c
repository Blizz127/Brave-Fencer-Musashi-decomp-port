/* SPDX-License-Identifier: MIT */
#include "village_jon_dialogue.h"
#include "../pc_port/forest_player_bytes.h"

enum { IDLE, ASK_FOR_FOOD, OFFER_FOOD, OFFER_HELP, FAREWELL, RETURNING };
enum { REQUESTED = 227, DECLINED = 228, AGREED = 229, WELL_WATER = 76 };
static const uint32_t origin = UINT32_C(0x801ef468);
static const uint32_t framing = UINT32_C(0x801f1080);

static const uint8_t *span(const BfmVillageJonDialogue *talk, uint32_t address, size_t n)
{
    if (address < origin || n > talk->bytes || address-origin > talk->bytes-n) return NULL;
    return talk->module + (address-origin);
}

static int item_slot(const BfmSaveRecord *save, unsigned item)
{
    for (unsigned slot = 47; slot < 59; ++slot)
        if (save->variables[slot] == item) return (int)slot;
    return -1;
}

static int bread_slot(const BfmSaveRecord *save)
{
    /* Jon asks for the first kind in his list, then its first bag slot. */
    for (unsigned kind = 13; kind <= 16; ++kind) {
        int slot = item_slot(save, kind);
        if (slot >= 0) return slot;
    }
    return -1;
}

int bfm_village_jon_dialogue_eligible(const BfmSaveRecord *save)
{
    if (!save) return 0;
    unsigned story = u32le(save->progress+8);
    return story >= 30 && story < 60;
}

int bfm_village_jon_dialogue_begin(BfmVillageJonDialogue *talk, const uint8_t *module,
                                   size_t bytes, BfmSaveRecord *save)
{
    if (!talk || talk->phase != IDLE || !module || !bfm_village_jon_dialogue_eligible(save)) return 0;
    unsigned story = u32le(save->progress+8);
    BfmVillageJonDialogue next = {.module=module, .bytes=bytes, .first_visit=story<40,
                                   .phase=ASK_FOR_FOOD};
    if (!span(&next,UINT32_C(0x801f1460),8)) return 0;
    if (next.first_visit) {
        next.cursor = UINT32_C(0x801f10c0);
        const uint8_t *greet = span(&next,next.cursor,8);
        if (!greet || u32le(greet) != 51 || u32le(greet+4) != UINT32_C(0x801f0098)) return 0;
        next.cursor += 8;
    } else if (story < 50) {
        next.cursor = UINT32_C(0x801f10e0);
    } else if (save->flags[DECLINED/8] & (1u << (DECLINED%8))) {
        next.cursor = UINT32_C(0x801f10f8);
        next.phase = OFFER_HELP;
    } else {
        next.cursor = UINT32_C(0x801f1118);
        next.phase = FAREWELL;
    }
    const uint8_t *setup = span(&next,next.cursor,16);
    if (!setup || u32le(setup) != 53 || u32le(setup+4) != framing || u32le(setup+8) != 0)
        return 0;
    next.cursor += 8;
    if (next.first_visit) {
        save->progress[8] = 40;
        save->flags[REQUESTED/8] |= 1u << (REQUESTED%8);
        next.changed = 1;
    }
    *talk = next;
    return 1;
}

void bfm_village_jon_dialogue_close(BfmVillageJonDialogue *talk, unsigned answer)
{
    if (talk && talk->waiting) {
        talk->waiting = 0;
        talk->answer = answer;
    }
}

static int select_reply(BfmVillageJonDialogue *talk, BfmSaveRecord *save)
{
    if (talk->phase == ASK_FOR_FOOD) {
        int bread = bread_slot(save), water = item_slot(save,WELL_WATER);
        if (bread >= 0 && water >= 0) {
            talk->cursor = UINT32_C(0x801f1300);
            talk->phase = OFFER_FOOD;
        } else {
            talk->phase = FAREWELL;
            if (bread >= 0) talk->cursor = UINT32_C(0x801f12e0);
            else if (water >= 0) talk->cursor = UINT32_C(0x801f12c0);
            else talk->cursor = UINT32_C(0x801f12a8);
        }
    } else if (talk->phase == OFFER_FOOD) {
        if (talk->answer == 2) {
            talk->phase = FAREWELL;
            talk->cursor = UINT32_C(0x801f1310);
        } else if (talk->answer == 1) {
            int bread = bread_slot(save), water = item_slot(save,WELL_WATER);
            if (bread < 0 || water < 0) return 0;
            save->variables[bread] = save->variables[water] = 0;
            talk->changed = 1;
            talk->phase = OFFER_HELP;
            talk->cursor = UINT32_C(0x801f1338);
        } else return 0;
    } else if (talk->phase == OFFER_HELP) {
        if (talk->answer != 1 && talk->answer != 2) return 0;
        unsigned choice = talk->answer == 1 ? AGREED : DECLINED;
        save->progress[8] = 50;
        save->flags[choice/8] &= (uint8_t)~((1u << (AGREED%8)) | (1u << (DECLINED%8)));
        save->flags[choice/8] |= 1u << (choice%8);
        talk->changed = 1;
        talk->phase = FAREWELL;
        talk->cursor = choice == AGREED ? UINT32_C(0x801f13f8) : UINT32_C(0x801f13c8);
    } else return 0;
    talk->answer = 0;
    return 1;
}

BfmVillageJonDialogueEvent bfm_village_jon_dialogue_step(BfmVillageJonDialogue *talk,
    BfmVillageJonStage *stage, int camera_restored, BfmSaveRecord *save, BfmForestMessage **opened)
{
    if (!talk || !save || !opened) return BFM_JON_DIALOGUE_ERROR;
    *opened = NULL;
    if (talk->phase == RETURNING) {
        if (!camera_restored) return BFM_JON_DIALOGUE_WAIT;
        *talk = (BfmVillageJonDialogue){0};
        return BFM_JON_DIALOGUE_RELEASE;
    }
    if (talk->phase == IDLE || talk->waiting || (talk->delay && --talk->delay))
        return BFM_JON_DIALOGUE_WAIT;
    while (talk->cursor < UINT32_C(0x801f1468)) {
        const uint8_t *command = span(talk,talk->cursor,8);
        if (!command) break;
        unsigned op = u32le(command);
        uint32_t arg = u32le(command+4);
        talk->cursor += 8;
        switch (op) {
        case 0: case 1: {
            /* Voice, choices and delays live inside the mounted message. Their
             * arguments may contain zero before the text's terminator. */
            const uint8_t *text = span(talk,arg,2);
            if (!text || arg < UINT32_C(0x801f0148) || arg >= framing || text[0] != 5)
                return BFM_JON_DIALOGUE_ERROR;
            size_t length = framing-arg;
            if (!span(talk,arg,length)) return BFM_JON_DIALOGUE_ERROR;
            *opened = bfm_field_message_borrow(text,length);
            if (!*opened) return BFM_JON_DIALOGUE_ERROR;
            talk->speaker = op;
            talk->waiting = 1;
            return BFM_JON_DIALOGUE_OPENED;
        }
        case 2:
            if (!select_reply(talk,save)) return BFM_JON_DIALOGUE_ERROR;
            break;
        case 4: {
            /* Both handoff walks enqueue a turn to Jon immediately afterwards. */
            const uint8_t *turn = span(talk,talk->cursor,8);
            if (!turn || u32le(turn) != 10 ||
                !bfm_village_jon_stage_walk(stage,s16le(command+4),s16le(command+6)))
                return BFM_JON_DIALOGUE_ERROR;
            talk->cursor += 8;
            break;
        }
        case 12: case 13:
            if (!bfm_village_jon_stage_react(stage,op == 12)) return BFM_JON_DIALOGUE_ERROR;
            break;
        case 15:
            if (!arg || arg > 255) return BFM_JON_DIALOGUE_ERROR;
            talk->delay = arg;
            return BFM_JON_DIALOGUE_WAIT;
        case 32:
            talk->phase = RETURNING;
            return BFM_JON_DIALOGUE_RETURN_CAMERA;
        case 51:
            if (arg != UINT32_C(0x801f00c8)) return BFM_JON_DIALOGUE_ERROR;
            talk->send_dog = 1;
            break;
        default:
            return BFM_JON_DIALOGUE_ERROR;
        }
    }
    return BFM_JON_DIALOGUE_ERROR;
}
