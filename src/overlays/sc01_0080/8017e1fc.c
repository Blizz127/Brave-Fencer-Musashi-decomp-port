/* SPDX-License-Identifier: MIT */
#include "tower_plate.h"

extern char D_80189450[];
extern s32 D_801270C8[];
extern s32 D_801270CC;
extern void func_8017D468(TowerPlate *);
extern int func_8012BEE8(TowerPlate *);
extern void func_801747AC(void);

/* Advance the pressed plate until its timer awards the room credit. */
void func_8017E1FC(TowerPlate *plate)
{
    if (plate->credited)
        return;
    plate->velocity += plate->acceleration;
    plate->pose.value += plate->velocity;
    plate->model->pose_y = plate->pose.parts.whole;
    func_8017D468(plate);
    if (func_8012BEE8(plate)) {
        plate->credited = 1;
        func_8001C214(plate->model, D_80189450);
        func_8012A828(plate, D_80183884);
        func_801747AC();
        if (++D_801270C8[0] >= 4)
            D_801270CC = 2;
    }
}
