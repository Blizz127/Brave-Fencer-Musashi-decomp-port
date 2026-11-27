/* SPDX-License-Identifier: MIT */
#ifndef SC01_0009_RIBSON_AUDIENCE_H
#define SC01_0009_RIBSON_AUDIENCE_H

struct CourtActor {
    short first_field;
    short state;
};

enum { AUDIENCE_PROGRESS = 59 };

extern void func_800291A0(int index, int value);
extern void func_80029514(int story);

#endif
