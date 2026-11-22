#ifndef BFM_FOREST_TUTORIAL_SOLDIER_H
#define BFM_FOREST_TUTORIAL_SOLDIER_H

#include "psx_types.h"

typedef struct {
    u8 reserved00[0x204];
    u32 countdown;
    u8 reserved208[0x0C];
    u8 stage;
} ForestTutorialScript;

typedef void (*ForestTutorialStage)(ForestTutorialScript *script);

#endif
