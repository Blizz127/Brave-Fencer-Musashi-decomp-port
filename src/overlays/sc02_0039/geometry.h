#ifndef BFM_SC02_0039_GEOMETRY_H
#define BFM_SC02_0039_GEOMETRY_H
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
} ScreenPoint;

typedef struct {
    s16 x;
    s16 y;
    s16 z;
} SpatialPoint;

typedef union {
    ScreenPoint point;
    u32 word;
} PackedPoint;

/* The PSX compiler loads these stack locals as little-endian words. */
#if defined(__mips__) && defined(__LITTLE_ENDIAN__)
#define SCREEN_POINT_WORD(value) ((value).word)
#else
#define SCREEN_POINT_WORD(value) ((u32)(u16)(value).point.x | ((u32)(u16)(value).point.y << 16))
#endif

typedef struct {
    u8 side, facing;
    ScreenPoint first, second, origin;
} FacingRegion;

extern s32 func_8017D484(const FacingRegion *region, const SpatialPoint *position,
                         const SpatialPoint *next, s32 skip);
extern s32 func_8017D5F4(const ScreenPoint *, const ScreenPoint *, const SpatialPoint *);
extern s32 func_8017D6AC(const ScreenPoint *, const ScreenPoint *, const SpatialPoint *);
extern s32 func_800495EC(u32 first, u32 second, u32 third);
#endif
