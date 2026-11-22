#ifndef BFM_OBJECT_DISTANCE_H
#define BFM_OBJECT_DISTANCE_H

#include "psx_types.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} DistanceVector;

s32 func_80013328(char *a, char *b);
s32 func_80013350(void *a, void *b);

#endif
