#ifndef BFM_JON_KEY_H
#define BFM_JON_KEY_H

typedef struct JonKeyPart {
    unsigned int reserved00[9];
    unsigned char *resource;
} JonKeyPart;

typedef struct JonKeyActor {
    unsigned int reserved00[8];
    JonKeyPart *part;
} JonKeyActor;

#if !defined(__mips__)
#define func_801F0048 bfm_jon_finish_key_message
#define D_801EB318 bfm_jon_key_part_resource
#define D_801F1468 bfm_jon_key_actor_data
#endif

extern unsigned char D_801EB318[];
extern unsigned char D_801F1468[];
extern void func_8012C588(int id, JonKeyActor *actor);
extern void func_8012A828(JonKeyActor *actor, unsigned char *data);

void func_801F0048(JonKeyActor *actor);

#endif
