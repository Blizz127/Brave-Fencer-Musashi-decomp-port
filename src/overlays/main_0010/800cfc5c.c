/* Owner re-derivation from retail assembly (clean input set). */
/* Classifies a mode id: 1 for 0x2013 and 0x2015-0x201A, 2 for 0x2055-0x205B, 3 for 0x207A-0x2080, 4 for 0x2097-0x209D, else 0. */
#include "psx_types.h"

s32 func_800CFC5C(s32 id)
{
    if (id == 0x2013) {
        return 1;
    }
    if (id == 0x2015) {
        return 1;
    }
    if (id == 0x2016) {
        return 1;
    }
    if (id == 0x2017) {
        return 1;
    }
    if (id == 0x2018) {
        return 1;
    }
    if (id == 0x2019) {
        return 1;
    }
    if (id == 0x201A) {
        return 1;
    }
    if (id == 0x2055) {
        return 2;
    }
    if (id == 0x2056) {
        return 2;
    }
    if (id == 0x2057) {
        return 2;
    }
    if (id == 0x2058) {
        return 2;
    }
    if (id == 0x2059) {
        return 2;
    }
    if (id == 0x205A) {
        return 2;
    }
    if (id == 0x205B) {
        return 2;
    }
    if (id == 0x207A) {
        return 3;
    }
    if (id == 0x207B) {
        return 3;
    }
    if (id == 0x207C) {
        return 3;
    }
    if (id == 0x207D) {
        return 3;
    }
    if (id == 0x207E) {
        return 3;
    }
    if (id == 0x207F) {
        return 3;
    }
    if (id == 0x2080) {
        return 3;
    }
    if (id == 0x2097) {
        return 4;
    }
    if (id == 0x2098) {
        return 4;
    }
    if (id == 0x2099) {
        return 4;
    }
    if (id == 0x209A) {
        return 4;
    }
    if (id == 0x209B) {
        return 4;
    }
    if (id == 0x209C) {
        return 4;
    }
    return (id == 0x209D) * 4;
}
