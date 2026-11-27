#ifndef SC02_0011_SCROLL_ACTOR_H
#define SC02_0011_SCROLL_ACTOR_H

typedef struct {
    unsigned char actor[0xb8];
    unsigned short signals;
    unsigned char reserved_ba[0xde];
    unsigned int effect_handle;
    unsigned char reserved_19c[0x64];
    int wait;
} ScrollActor;

extern void func_80171A1C(ScrollActor *actor);

#endif
