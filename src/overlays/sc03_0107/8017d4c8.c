#include "psx_types.h"

typedef struct {
    u8 unused00[0x14];
    s32 goal_radius;
    u8 unused18[0x88];
    u16 goal_pitch;
    u16 goal_yaw;
} GalleryCamera;

typedef struct {
    s16 min_x;
    s16 max_x;
    s16 min_z;
    s16 max_z;
    u16 pitch;
    u16 yaw;
    s32 radius;
} GalleryCameraZone;

/* Hold the target on the Z centerline while the player follows this corridor. */
void func_8017D4C8(GalleryCamera *camera, s16 target[3], GalleryCameraZone *zone)
{
    camera->goal_pitch = zone->pitch;
    camera->goal_yaw = zone->yaw;
    camera->goal_radius = zone->radius;
    target[2] = zone->min_z + (zone->max_z - zone->min_z) / 2;
}
