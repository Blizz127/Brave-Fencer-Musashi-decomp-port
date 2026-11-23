#ifndef SC03_0012_VENT_PHASE_H
#define SC03_0012_VENT_PHASE_H

typedef struct {
    unsigned char reserved00[2];
    unsigned short phase;
    unsigned char reserved04[0xdc];
    unsigned short activated;
    unsigned char reservedE2[4];
    short sequence_delay;
    short phase_delay;
} SteamwoodVentPhase;

void func_8017DA80(SteamwoodVentPhase *actor);
extern void func_8017D780(void);
extern void func_8002D4C8(unsigned short command, unsigned short argument);
extern void func_8013C9C4(const unsigned char *sequence);
extern unsigned short D_800B99DC;
extern const unsigned char D_8017E8A4[];

#endif
