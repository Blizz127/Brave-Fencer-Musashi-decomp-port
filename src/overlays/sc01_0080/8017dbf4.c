/* SPDX-License-Identifier: MIT */
#include "tower_plate.h"

extern char D_801891AC[];

/* Reveal the plate at the position recorded before the pillar was hit. */
void func_8017DBF4(TowerPlate *plate)
{
    func_8001C214(plate->model, D_801891AC);
    func_8012A828(plate, D_80183884);
    plate->state = 3;
    plate->x = plate->home_x;
    plate->z = plate->home_z;
    plate->flags &= ~0x8800;
    plate->model->flags &= ~0x10;
    plate->timer = 0;
}
