#ifndef SC03_0002_LENO_COMMAND_H
#define SC03_0002_LENO_COMMAND_H

struct LenoActor {
    unsigned char before_command[0x102];
    short command;
};

enum {
    LENO_ACTOR_CLASS = 614,
    LENO_CALL = 1,
    LENO_RELEASE = 2
};

extern struct LenoActor *func_8012E544(int actor_class);

#endif
