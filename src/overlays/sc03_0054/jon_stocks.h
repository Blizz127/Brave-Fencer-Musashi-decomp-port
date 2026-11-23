#ifndef BFM_JON_STOCKS_H
#define BFM_JON_STOCKS_H

typedef struct JonStocksActor {
    unsigned char reserved00[2];
    unsigned short state;
    unsigned char reserved04[48];
    unsigned short branch;
} JonStocksActor;

#if !defined(__mips__)
#define func_801EF558 bfm_village_jon_initialize
#define func_80178CBC bfm_village_jon_select_script
#define func_8018766C bfm_village_jon_query
#define func_800D0F4C bfm_village_jon_item_service
#define D_801F1480 bfm_village_jon_state0
#define D_801F1482 bfm_village_jon_state1
#define D_801F10C0 bfm_village_jon_script_before40
#define D_801F10E0 bfm_village_jon_script_before50
#define D_801F10F8 bfm_village_jon_script_flag228
#define D_801F1118 bfm_village_jon_script_before60
#define D_801F1138 bfm_village_jon_script_query_zero
#define D_801F1158 bfm_village_jon_script_query_nonzero
#endif

extern unsigned short D_801F1480, D_801F1482;
extern unsigned char D_801F10C0[], D_801F10E0[], D_801F10F8[];
extern unsigned char D_801F1118[], D_801F1138[], D_801F1158[];
extern int func_80029504(void);
extern void func_80029514(int progress);
extern void func_8002D4C8(unsigned short command, unsigned short argument);
extern unsigned int func_80029124(unsigned int flag, unsigned int set);
extern int func_80029178(unsigned int flag);
extern int func_800D0F4C(int item);
extern void func_80178CBC(JonStocksActor *actor, unsigned char *script);
extern int func_8018766C(int first, int second);
void func_801EF558(JonStocksActor *actor);

#endif
