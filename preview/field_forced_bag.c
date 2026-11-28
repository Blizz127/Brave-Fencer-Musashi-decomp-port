/* SPDX-License-Identifier: MIT */
#include "field_forced_bag.h"
#include "field_inventory_actions.h"
#include "bedroom_effects.h"

enum { UP = 0x10, RIGHT = 0x20, DOWN = 0x40, LEFT = 0x80 };
enum { OPEN = 0x467, EMPTY = 0x469, BACK = 0x46b, ACCEPT = 0x46c, SELECT = 0x46d };

void bfm_field_forced_bag_begin(BfmFieldForcedBag *bag)
{
    unsigned slot = bag->view.item_slot;
    unsigned field = bag->view.hand_age;
    *bag = (BfmFieldForcedBag){
        .phase = BFM_BAG_OPEN, .marked = -1,
        .view = {.phase = BFM_FIELD_MENU_ITEMS, .item_slot = slot, .hand_age = field,
                 .hint_columns = 14, .hint_lines = 3}
    };
}

static unsigned direction(BfmFieldForcedBag *bag, BfmFieldInput input)
{
    unsigned horizontal = input.x > 0 ? RIGHT : input.x < 0 ? LEFT : 0;
    unsigned vertical = input.y > 0 ? DOWN : input.y < 0 ? UP : 0;
    unsigned held = horizontal | vertical;
    unsigned edge = input.pressed_directions & 0xf0u;
    if (edge || held != bag->held) bag->repeat = 6;
    else if (held && bag->repeat && !--bag->repeat) {
        edge = held;
        bag->repeat = 3;
    }
    bag->held = held;
    return edge;
}

static unsigned row_step(unsigned row, unsigned rows, unsigned dir)
{
    if (dir & UP) row = (row + rows - 1) % rows;
    if (dir & DOWN) row = (row + 1) % rows;
    return row;
}

static void select_item(BfmFieldForcedBag *bag, BfmFieldInput input, unsigned dir,
                        BfmSaveRecord *save, BfmGameplayRun *run, unsigned sounds[2])
{
    BfmFieldItemMenuView *v = &bag->view;
    unsigned slot = v->item_slot;
    if (input.cross_pressed && bag->marked < 0) {
        if (save->variables[47+slot]) {
            v->phase = BFM_FIELD_MENU_ACTIONS;
            v->action_row = 0;
            sounds[0] = SELECT;
            return;
        }
        sounds[0] = EMPTY;
    } else if (input.cross_pressed || input.circle_pressed) {
        if (bag->marked < 0) bag->marked = (int)slot;
        else {
            bfm_field_inventory_swap((unsigned)bag->marked, slot, save, run);
            bag->marked = -1;
        }
        sounds[0] = SELECT;
        if (input.cross_pressed) return;
    }
    unsigned column = slot / 6;
    if (dir & LEFT) column ^= 1;
    if (dir & RIGHT) column ^= 1;
    v->item_slot = 6*column + row_step(slot%6,6,dir);
    if (dir) sounds[sounds[0] ? 1 : 0] = BFM_FIELD_MENU_ROW_SOUND;
}

static void select_action(BfmFieldForcedBag *bag, BfmFieldInput input, unsigned dir,
                          const uint8_t *main, size_t bytes, BfmSaveRecord *save,
                          BfmGameplayRun *run, unsigned sounds[2])
{
    BfmFieldItemMenuView *v = &bag->view;
    if (input.cross_pressed) {
        if (v->action_row == 2) {
            v->phase = BFM_FIELD_MENU_ITEMS;
            sounds[0] = BACK;
            sounds[1] = ACCEPT;
            return;
        }
        unsigned item = save->variables[47 + v->item_slot];
        int used = v->action_row == 0;
        int ok = used ? bfm_field_inventory_use(main,bytes,v->item_slot,save,run) :
                        bfm_field_inventory_discard(main,bytes,v->item_slot,save,run);
        if (ok) {
            v->phase = BFM_FIELD_MENU_ITEMS;
            sounds[0] = used ? bfm_field_inventory_use_sound(item) : BFM_FIELD_MENU_COMMIT_SOUND;
            sounds[1] = ACCEPT;
            return;
        }
        sounds[0] = BFM_FIELD_MENU_ERROR_SOUND;
    } else if (input.triangle_pressed) {
        v->phase = BFM_FIELD_MENU_ITEMS;
        sounds[0] = BACK;
        return;
    }
    if (dir & (UP | DOWN)) {
        v->action_row = row_step(v->action_row, 3, dir);
        sounds[sounds[0] ? 1 : 0] = BFM_FIELD_MENU_ROW_SOUND;
    }
}

int bfm_field_forced_bag_step(BfmFieldForcedBag *bag, BfmFieldInput input,
    const uint8_t *main, size_t bytes, BfmSaveRecord *save, BfmGameplayRun *run, unsigned sounds[2])
{
    sounds[0] = sounds[1] = 0;
    bag->draw = bag->phase == BFM_BAG_SELECT || bag->phase == BFM_BAG_WAIT;
    if (bag->phase == BFM_BAG_OPEN) {
        if (!bag->fields) {
            bag->entry_page = run->display_page;
            sounds[0] = OPEN;
        }
        if (++bag->fields == 4) { bag->fields = 0; bag->phase = BFM_BAG_SELECT; }
    } else if (bag->phase == BFM_BAG_SELECT) {
        unsigned dir = direction(bag,input);
        unsigned old = save->variables[47+bag->view.item_slot];
        if (bag->view.phase == BFM_FIELD_MENU_ITEMS)
            select_item(bag,input,dir,save,run,sounds);
        else select_action(bag,input,dir,main,bytes,save,run,sounds);
        if (old != save->variables[47+bag->view.item_slot]) bag->view.ink_age = 0;
        ++bag->view.ink_age;
        if (bfm_save_inventory_space(save) >= 0) { bag->phase = BFM_BAG_WAIT; bag->fields = 0; }
    } else if (bag->phase == BFM_BAG_WAIT) {
        if (++bag->fields == 8) { bag->fields = 0; bag->phase = BFM_BAG_CLOSE; }
    } else if (bag->phase == BFM_BAG_CLOSE) {
        /* The menu copied the other field's ordering table on entry. If
         * the restored page is still on screen, wait one more field. */
        ++bag->fields;
        if (bag->fields > 4 ||
            (bag->fields == 4 && bag->entry_page == (run->display_page ^ 1u))) {
            bag->phase = BFM_BAG_IDLE;
            return 1;
        }
    }
    ++bag->view.hand_age;
    return 0;
}

int bfm_field_forced_bag_draw(const BfmFieldForcedBag *bag, const BfmFieldItemMenuArt *art,
    const BfmSaveRecord *save, const uint16_t tiles[512][1024], uint32_t pixels[240][320])
{
    if (!bag->draw) return 1;
    return bfm_field_forced_items_draw(art,&bag->view,save,bag->marked,tiles,pixels);
}
