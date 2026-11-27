/* SPDX-License-Identifier: MIT */
#include "ribson_audience.h"

/* Ribson sends the assembled party to Hell's Valley. */
void func_8017F618(struct CourtActor *actor)
{
    func_80029514(150);
    func_800291A0(AUDIENCE_PROGRESS, 5);
    actor->state = 5;
}
