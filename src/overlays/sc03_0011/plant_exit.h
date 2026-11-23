#ifndef PLANT_EXIT_H
#define PLANT_EXIT_H

struct PlantExit {
    unsigned char reserved00[0x15];
    unsigned char phase;
    unsigned char reserved16[0x28 - 0x16];
    unsigned int timer;
};
extern int (*D_80184EC4[])(struct PlantExit *);
extern void func_8017DBD8(void);
extern void func_80029124(int flag, int value);
extern void func_8002D4C8(int effect, int mode);
extern void func_8001BFD0(void);
extern void func_800D0C48(int);
extern void func_800D1E28(void);

#endif
