/* SPDX-License-Identifier: MIT */
#include "tower_plate.h"

extern void func_8002D4C8(int, int);
extern void func_80174784(void);

/* Begin the pressed plate's timed movement from its current model pose. */
void func_8017DC78(TowerPlate *plate)
{
    func_8012A828(plate, D_80183884);
    plate->state = 4;
    plate->credited = 0;
    plate->timer = 30;
    plate->pose.parts.whole = plate->model->pose_y;
    plate->pose.parts.fraction = 0;
    plate->acceleration = 64 << 16;
    plate->velocity = 0;
    func_8002D4C8(1182, 0);
    func_8002D4C8(1186, 0);
    func_80174784();
}
