#ifndef SC02_0011_CAMERA_TRANSITION_H
#define SC02_0011_CAMERA_TRANSITION_H

/* The transition retains the active camera state, including its final word. */
typedef struct {
    unsigned int words[41];
} FieldCameraState;

extern FieldCameraState *D_801151D4;
extern FieldCameraState D_80126DB8;
extern unsigned char D_80127504;

#endif
