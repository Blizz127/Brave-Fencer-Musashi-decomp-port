/* SPDX-License-Identifier: MIT */
#include "field_inventory_actions.h"
#include "../pc_port/forest_player_bytes.h"
#include <string.h>

enum {
    BAG_SLOTS = 12, FIRST_SLOT = 47, FIRST_DATE = 20,
    DAY = 0x34, MAX_HP = 0x3a, HP = 0x3c, MAX_BP = 0x3e, BP = 0x40,
    CONDITION = 0x1c, STORED_CONDITION = 0x54,
    WAKEFULNESS = 0x2c, STORED_WAKEFULNESS = 0x60,
    LAST_ITEM = 119
};

enum { CURE = 1, MINT, STRONG_MINT, AGED_CHEESE, EXTEND_HP };
typedef struct {
    int hp, bp;
    unsigned sound, special;
} ItemUse;

/* Manual item effects. The mounted catalogue also contains equipment, quest
 * items, and S-Revive, which is triggered on defeat. */
static const ItemUse uses[LAST_ITEM + 1] = {
    [1]   = {150, 150, 0x45c, 0},       /* RiceBall */
    [2]   = {-5, 25, 0x463, 0},         /* SourBall */
    [3]   = {-25, 25, 0x465, 0},        /* Moldball */
    [4]   = {80, 0, 0x45c, 0},          /* Gel */
    [5]   = {150, 0, 0x45c, 0},         /* W-Gel */
    [6]   = {250, 5, 0x45c, 0},         /* C-Drink */
    [7]   = {999, 5, 0x45c, 0},         /* EX-Drink */
    [8]   = {0, 5, 0x464, MINT},
    [9]   = {0, 5, 0x464, CURE},        /* Antidote */
    [11]  = {30, 10, 0x45c, 0},         /* Orange */
    [12]  = {-5, 2, 0x465, 0},          /* Sporange */
    [13]  = {0, 25, 0x45c, 0},          /* RyeBread */
    [14]  = {0, 50, 0x45c, 0},          /* Bagel */
    [15]  = {0, 80, 0x45c, 0},          /* Scone */
    [16]  = {0, 150, 0x45c, 0},         /* Gr-Bread */
    [17]  = {-5, 10, 0x463, 0},         /* Ol'Bread */
    [18]  = {-10, 10, 0x465, 0},        /* RotBread */
    [76]  = {5, 0, 0x45c, 0},           /* Well water */
    [83]  = {10, 10, 0x45c, 0},         /* Milk */
    [84]  = {-5, 5, 0x465, 0},          /* SourMilk */
    [85]  = {50, 50, 0x45c, 0},         /* Yogurt */
    [87]  = {10, 0, 0x464, CURE},       /* Aqualin */
    [104] = {0, 300, 0x45c, 0},         /* JamBread */
    [105] = {0, 100, 0x45c, 0},         /* Biscuit */
    [106] = {300, 300, 0x45c, 0},       /* NeatBall */
    [107] = {0, 0, 0x45c, AGED_CHEESE},
    [109] = {0, 5, 0x464, STRONG_MINT},
    [112] = {999, 0, 0x45c, EXTEND_HP},
    [113] = {5, 0, 0x45c, 0},           /* Juice */
    [114] = {100, 50, 0x45c, 0},        /* Pea Soup */
    [115] = {150, 100, 0x45c, 0},       /* Cake */
    [116] = {150, 200, 0x45c, 0},       /* Gravy */
    [117] = {200, 150, 0x45c, 0},       /* Salad */
    [118] = {250, 300, 0x45c, 0},       /* Lasagna */
    [119] = {500, 500, 0x45c, 0}        /* PorkChop */
};

unsigned bfm_field_inventory_use_sound(unsigned item)
{
    if (!item) return 0;
    return item <= LAST_ITEM && uses[item].sound ? uses[item].sound : 0x45fu;
}

static const uint8_t *catalogue_row(const uint8_t *main, size_t bytes, unsigned item)
{
    const uint32_t origin = UINT32_C(0x800cedf8);
    size_t offset = (UINT32_C(0x8010ede8) - origin) + 12u * item;
    if (!main || !item || item > LAST_ITEM || offset > bytes || bytes - offset < 12)
        return NULL;
    uint32_t name = u32le(main + offset);
    return name >= origin && name - origin < bytes ? main + offset : NULL;
}

static void write16(uint8_t *p, unsigned value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void resource_add(uint8_t *player, unsigned field, unsigned maximum, int amount)
{
    uint16_t value = (uint16_t)(u16le(player + field) + amount);
    write16(player + field, value > maximum ? maximum : value);
}

static void wake_up(uint8_t *p, unsigned amount)
{
    /* The fractional part is retained until the integer part exceeds 160. */
    uint32_t value = u32le(p) + (amount << 16);
    if ((value >> 16) > 160 && (value >> 16) < 0x8000u)
        value = UINT32_C(160) << 16;
    write16(p, value);
    write16(p + 2, value >> 16);
}

static void mirror_player(BfmGameplayRun *run, const BfmSaveRecord *save,
                          unsigned field, unsigned width)
{
    memcpy(run->player.record + field, save->progress + field, width);
}

static void clear_slot(unsigned slot, BfmSaveRecord *save, BfmGameplayRun *run)
{
    save->variables[FIRST_SLOT + slot] = run->banks.variables[FIRST_SLOT + slot] = 0;
}

int bfm_field_inventory_use(const uint8_t *main, size_t main_bytes, unsigned slot,
                            BfmSaveRecord *save, BfmGameplayRun *run)
{
    if (!save || !run || !run->has_player || slot >= BAG_SLOTS) return 0;
    unsigned item = save->variables[FIRST_SLOT + slot];
    if (!catalogue_row(main, main_bytes, item) || !uses[item].sound) return 0;

    ItemUse use = uses[item];
    uint8_t *p = save->progress;
    switch (use.special) {
    case CURE:
        memset(p + CONDITION, 0, 4);
        memset(p + STORED_CONDITION, 0, 4);
        mirror_player(run, save, CONDITION, 4);
        mirror_player(run, save, STORED_CONDITION, 4);
        break;
    case MINT:
    case STRONG_MINT: {
        unsigned amount = use.special == MINT ? 80 : 144;
        wake_up(p + WAKEFULNESS, amount);
        wake_up(p + STORED_WAKEFULNESS, amount);
        mirror_player(run, save, WAKEFULNESS, 4);
        mirror_player(run, save, STORED_WAKEFULNESS, 4);
        break;
    }
    case AGED_CHEESE: {
        uint16_t difference = (uint16_t)(u16le(p + DAY) -
                                         u16le(save->variables + FIRST_DATE + 2 * slot));
        int age = difference <= INT16_MAX ? difference : (int)difference - 65536;
        use.hp = use.bp = 25 * (age >= 19 ? 20 : age + 2);
        break;
    }
    case EXTEND_HP:
        resource_add(p, MAX_HP, 500, 25);
        mirror_player(run, save, MAX_HP, 2);
        break;
    }

    /* Even a zero recovery applies the current cap in the cure/cheese paths. */
    int changes_hp = use.hp || use.special == CURE || use.special == AGED_CHEESE;
    int changes_bp = use.bp || use.special == AGED_CHEESE;
    if (use.hp < 0 && use.special != AGED_CHEESE) {
        int hp = u16le(p + HP) + use.hp;
        write16(p + HP, hp < 1 ? 1u : (unsigned)hp);
    } else if (changes_hp) {
        resource_add(p, HP, u16le(p + MAX_HP), use.hp);
    }
    if (changes_bp) resource_add(p, BP, u16le(p + MAX_BP), use.bp);
    if (changes_hp) mirror_player(run, save, HP, 2);
    if (changes_bp) mirror_player(run, save, BP, 2);
    clear_slot(slot, save, run);
    return 1;
}

int bfm_field_inventory_discard(const uint8_t *main, size_t main_bytes, unsigned slot,
                                BfmSaveRecord *save, BfmGameplayRun *run)
{
    if (!save || !run || !run->has_player || slot >= BAG_SLOTS) return 0;
    unsigned item = save->variables[FIRST_SLOT + slot];
    const uint8_t *row = catalogue_row(main, main_bytes, item);
    if (!row) return 0;
    int value = s16le(row + 4);
    if (value <= 0 && (value != INT16_MIN || item == 39)) return 0;
    clear_slot(slot, save, run);
    return 1;
}

int bfm_field_inventory_swap(unsigned a, unsigned b, BfmSaveRecord *save, BfmGameplayRun *run)
{
    if (!save || !run || !run->has_player || a >= BAG_SLOTS || b >= BAG_SLOTS) return 0;
    uint8_t *items = save->variables + FIRST_SLOT;
    uint8_t first = items[a];
    items[a] = run->banks.variables[FIRST_SLOT + a] = items[b];
    items[b] = run->banks.variables[FIRST_SLOT + b] = first;

    unsigned first_date = FIRST_DATE + 2 * a, second_date = FIRST_DATE + 2 * b;
    unsigned date = u16le(save->variables + first_date);
    write16(save->variables + first_date, u16le(save->variables + second_date));
    write16(save->variables + second_date, date);
    memcpy(run->banks.variables + first_date, save->variables + first_date, 2);
    memcpy(run->banks.variables + second_date, save->variables + second_date, 2);
    return 1;
}
