/* Place each primary entry, then mark the secondary entries as unplaced. */
#include "mine_initial_placement.h"

void func_8013E4B4(void)
{
    MineInitialPlacement *row = D_801274C8;
    MineInitialPlacement *secondary;

    if (row && row->type) {
        do {
            func_8012C750(row);
            ++row;
        } while (row->type);
    }

    /* A primary actor's initializer can replace the secondary list. */
    row = D_801274CC;
    if (row && row->type) {
        secondary = row;
        do {
            secondary->flags &= 0x7fff;
            ++secondary;
        } while (secondary->type);
    }
}
