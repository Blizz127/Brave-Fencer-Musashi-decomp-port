#ifndef SC01_0077_CAMERA_TRANSFORM_H
#define SC01_0077_CAMERA_TRANSFORM_H

typedef struct {
    short m[3][3];
    short pad;
    int t[3];
} ForestCameraMatrix;

typedef struct {
    short x, y, z, pad;
} ForestCameraVector;

void func_8012F14C(ForestCameraMatrix *matrix, ForestCameraVector *source,
                   ForestCameraVector *destination);

#endif
