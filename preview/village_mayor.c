/* SPDX-License-Identifier: MIT */
#include "village_mayor.h"
#include "../pc_port/forest_player_bytes.h"

#include <string.h>

static const uint32_t module_origin = UINT32_C(0x801ef468);
enum { FIRST_VISIT, TURNED_DOWN, DOG_REQUESTED, DOG_RETURNED };
enum { DOG_ACCEPTED_FLAG = 223, DOG_DECLINED_FLAG = 224 };

static const uint8_t *span(const BfmVillageMayorDialogue *talk, uint32_t address, size_t size)
{
    if (address < module_origin || size > talk->bytes || address-module_origin > talk->bytes-size)
        return NULL;
    return talk->module + (address-module_origin);
}

static int flag_set(const BfmSaveRecord *save, unsigned flag)
{
    return !!(save->flags[flag / 8] & (1u << (flag % 8)));
}

/* The returned-dog visit calls the resident's clock greeting before the
 * Mayor event resumes. The table belongs to the lower village scene. */
static int returned_greeting(BfmVillageMayorDialogue *talk, const uint8_t *scene,
                             size_t bytes, BfmSaveRecord *save)
{
    const uint32_t base = UINT32_C(0x80128158);
    const size_t greetings = UINT32_C(0x801c099c) - base;
    const size_t residents = UINT32_C(0x801c0a9c) - base;
    const size_t hours = UINT32_C(0x801c0ad0) - base;
    const size_t resident = UINT32_C(0x80192874) - base;
    if (!scene || bytes < hours + 48) return 0;
    unsigned slot = u16le(scene + resident + 16);
    unsigned hour = save->progress[0x39];
    if (slot >= sizeof save->timestamps / 4 || hour >= 24) return 0;
    unsigned column = u16le(scene + residents + 2*slot);
    int row = s16le(scene + hours + 2*hour) - 1;
    uint8_t *stamp = save->timestamps + 4*slot + 2;
    int previous = s16le(stamp);
    int day = u16le(save->progress + 0x34);
    if (previous) {
        int last_day = previous / 24;
        int last_hour = previous % 24;
        if (day - last_day >= 5) row = 7;
        else if (day == last_day && (int)hour - last_hour >= 2) row = 6;
    }
    if (column >= 8 || row < 0 || row >= 8) return 0;
    uint32_t address = u32le(scene + greetings + 4*(8*(unsigned)row + column));
    if (address < base || address-base >= bytes) return 0;
    /* The timed greeting contains zero in its operands. The message
     * controller reads it through the script's own terminator. */
    talk->greeting = scene + (address-base);
    talk->greeting_bytes = bytes - (address-base);
    unsigned now = 24u*(unsigned)day + hour;
    stamp[0] = (uint8_t)now;
    stamp[1] = (uint8_t)(now >> 8);
    return 1;
}

int bfm_village_mayor_dialogue_begin(BfmVillageMayorDialogue *talk, const uint8_t *module,
                                     size_t bytes, const uint8_t *scene, size_t scene_bytes,
                                     BfmSaveRecord *save)
{
    if (!talk || !module || !save) return 0;
    *talk = (BfmVillageMayorDialogue){.module=module, .bytes=bytes};
    unsigned story = u32le(save->progress + 8);
    if (story >= 20 && story < 30) {
        talk->branch = DOG_RETURNED;
        talk->cursor = UINT32_C(0x801f0ae4);
        return span(talk,talk->cursor,8) && returned_greeting(talk,scene,scene_bytes,save);
    }
    if (flag_set(save, DOG_ACCEPTED_FLAG)) talk->branch = DOG_REQUESTED;
    else if (flag_set(save, DOG_DECLINED_FLAG)) talk->branch = TURNED_DOWN;
    /* The shared porch camera and the resident's entrance finish before the
     * first message instruction in each of the three encounter programs. */
    talk->cursor = talk->branch == DOG_REQUESTED ? UINT32_C(0x801f0a84) :
                   talk->branch == TURNED_DOWN ? UINT32_C(0x801f0ab4) : UINT32_C(0x801f094c);
    return span(talk, talk->cursor, 8) != NULL;
}

void bfm_village_mayor_dialogue_close(BfmVillageMayorDialogue *talk, unsigned answer)
{
    if (talk && talk->waiting) {
        talk->waiting = 0;
        talk->answer = answer;
        talk->delay = 1;
        const uint8_t *next = span(talk, talk->cursor, 8);
        if (!answer && next && u32le(next) == 53 && u32le(next+4) == UINT32_C(0x801c08f8))
            talk->finished = 1;
    }
}

int bfm_village_mayor_dialogue_step(BfmVillageMayorDialogue *talk, BfmSaveRecord *save,
                                    BfmForestMessage **opened)
{
    if (!talk || !save || !opened) return 0;
    *opened = NULL;
    if (!talk->cursor || talk->waiting || talk->finished) return 1;
    if (talk->delay) { --talk->delay; return 1; }
    if (talk->greeting) {
        *opened = bfm_field_message_borrow(talk->greeting,talk->greeting_bytes);
        if (!*opened) return 0;
        talk->greeting = NULL;
        talk->waiting = 1;
        return 1;
    }
    for (unsigned i=0; i<8; ++i) {
        const uint8_t *command = span(talk, talk->cursor, 8);
        if (!command) return 0;
        unsigned opcode = u32le(command);
        uint32_t arg = u32le(command+4);
        talk->cursor += 8;
        switch (opcode) {
        case 0: case 1: {
            /* Both participants borrow NUL-terminated text in this member.
             * The colour and pacing operands in these records are nonzero. */
            const uint8_t *program = span(talk, arg, 4);
            if (!program || program[0] != 5 || program[1] != 2) return 0;
            size_t capacity = talk->bytes - (arg-module_origin);
            const uint8_t *end = memchr(program, 0, capacity);
            if (!end) return 0;
            *opened = bfm_field_message_borrow(program, (size_t)(end-program)+1);
            if (!*opened) return 0;
            talk->hero = opcode == 1;
            talk->waiting = 1;
            return 1;
        }
        case 2:
            if (!talk->answer) { talk->finished = 1; return 1; }
            if (talk->answer > 2 || talk->branch == DOG_REQUESTED || talk->branch == DOG_RETURNED) return 0;
            {
                int yes = talk->answer == 1;
                unsigned flag = yes ? DOG_ACCEPTED_FLAG : DOG_DECLINED_FLAG;
                save->flags[flag/8] |= (uint8_t)(1u << (flag%8));
                if (talk->branch == TURNED_DOWN)
                    talk->cursor = yes ? UINT32_C(0x801f09d4) : UINT32_C(0x801f0a3c);
                else
                    talk->cursor = yes ? UINT32_C(0x801f099c) : UINT32_C(0x801f0a0c);
                talk->answer = 0;
            }
            break;
        case 11: case 12: case 13:
            /* Release/start the offer reaction between consecutive speakers. */
            if (arg) return 0;
            break;
        case 15:
            if (!arg || arg > 255) return 0;
            talk->delay = arg;
            return 1;
        case 53:
            if (arg != UINT32_C(0x801c08f8)) return 0;
            talk->finished = 1;
            return 1;
        default:
            return 0;
        }
    }
    return 0;
}
