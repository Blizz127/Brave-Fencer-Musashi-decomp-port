#ifndef SC02_0011_ROOTRICK_EVENT_H
#define SC02_0011_ROOTRICK_EVENT_H

typedef struct {
    unsigned char actor[0xcc];
    unsigned int event_script;
    unsigned char reserved_d0[0x10];
    int next_score;
    int score_delay;
    unsigned char reserved_e8[0x16];
    short wait;
    unsigned char reserved_100[2];
    short event_id;
    short score_phase;
} RootrickEvent;

void func_80184918(RootrickEvent *actor, int score);

#endif
