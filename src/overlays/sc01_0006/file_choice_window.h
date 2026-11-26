#ifndef BFM_FILE_CHOICE_WINDOW_H
#define BFM_FILE_CHOICE_WINDOW_H

#include "psx_types.h"

struct FileChoiceWindow {
    u32 active;
    s16 state;
    u8 reserved06[2];
    u32 flags;
    u8 reserved0C[0x1F - 0x0C];
    s8 answer;
    u8 reserved20;
    u8 row_count;
};

#endif
