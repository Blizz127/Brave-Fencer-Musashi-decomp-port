/* SPDX-License-Identifier: MIT */
#include "tower_plate.h"

extern void func_8017D72C(TowerPlate *);
extern void (*D_80187028[])(TowerPlate *);

/* A hit interrupts either statue phase before the floor plate is exposed. */
void func_8017DE38(TowerPlate *plate)
{
    unsigned state = (u16)plate->state;

    if ((state == 1 || state == 2) && (plate->flags & 1)) {
        func_8017D72C(plate);
        return;
    }
    D_80187028[(u16)plate->state](plate);
}
