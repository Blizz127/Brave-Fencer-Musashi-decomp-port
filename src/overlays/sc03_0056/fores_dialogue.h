#ifndef MUSASHI_FORES_DIALOGUE_H
#define MUSASHI_FORES_DIALOGUE_H

typedef struct ForesActor {
    unsigned short kind;
    unsigned short phase;
} ForesActor;

#if !defined(__mips__) || defined(__LP64__)
#define D_801CC890 bfm_sc03_0056_fores_intro_script
#define D_801CC950 bfm_sc03_0056_fores_reentry_script
#define func_801CBB54 bfm_sc03_0056_fores_open
#define func_801CBB80 bfm_sc03_0056_fores_update
#define func_801CBBDC bfm_sc03_0056_fores_reopen
#define func_801CBC08 bfm_sc03_0056_fores_reentry_update
#endif

extern const unsigned char D_801CC890[];
extern const unsigned char D_801CC950[];
extern void func_80178CBC(ForesActor *, const unsigned char *);
extern int func_80178970(ForesActor *);
extern void func_80029514(int);
extern void func_80029124(int, int);
extern void func_80178D18(ForesActor *);
extern void func_80162120(void);
extern void func_8018151C(ForesActor *);

void func_801CBB54(ForesActor *actor);
void func_801CBB80(ForesActor *actor);
void func_801CBBDC(ForesActor *actor);
void func_801CBC08(ForesActor *actor);

#endif
