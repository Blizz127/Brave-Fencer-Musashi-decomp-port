#ifndef BFM_CEMETERY_TOMB_OPENING_H
#define BFM_CEMETERY_TOMB_OPENING_H

#include "psx_types.h"
#include "event_flags.h"

typedef struct CemeteryActor {
    u16 kind;
    u16 phase;
    u16 reserved04;
    u16 x;
    u32 reserved08;
    u16 reserved0c;
    u16 z;
    u16 reserved10;
    s16 heading;
    u8 reserved14[8];
    u32 remaining;
    u8 reserved20[60];
    u16 interaction;
    u8 reserved5e[38];
    s16 sound_ticks;
} CemeteryActor;

typedef struct CemeteryPlayer {
    u8 reserved00[32];
    CemeteryActor *avatar;
    u8 reserved_after_avatar[336 - sizeof(CemeteryActor *)];
    CemeteryActor *selected;
} CemeteryPlayer;

/* Scene addresses are reused by other rooms. Keep the host symbols local to this room. */
#if !defined(__mips__)
#define func_8017E414 bfm_cemetery_open_selected_slab
#define func_8017E524 bfm_cemetery_move_slab
#define func_8017EC30 bfm_cemetery_check_key_range
#define func_8017D6C0 bfm_cemetery_start_slab_script
#define func_8017D6E8 bfm_cemetery_finish_slab_motion
#define func_8017D884 bfm_cemetery_start_key_script
#define func_8016F1AC bfm_cemetery_current_script
#define func_8012BD14 bfm_cemetery_player_distance
#define func_8012A828 bfm_cemetery_set_animation
#define func_8012C658 bfm_cemetery_spawn_actor
#define func_80143B6C bfm_cemetery_spawn_effect
#define D_80126B58 bfm_cemetery_player
#define D_801270C8 bfm_cemetery_linked_actor
#define D_801932E8 bfm_cemetery_linked_animation
#define D_80126B78 (D_80126B58.avatar)
#else
extern CemeteryActor *D_80126B78;
#endif

extern CemeteryPlayer D_80126B58;
extern CemeteryActor *volatile D_801270C8;
extern u8 D_801932E8[];

extern void func_8017D6C0(void);
extern void func_8017D6E8(void);
extern void func_8012A828(CemeteryActor *actor, const void *animation);
extern CemeteryActor *func_8012C658(int kind, int variant, CemeteryActor *parent);
extern void func_8002D4C8(unsigned short command, unsigned short argument);
extern CemeteryActor *func_80143B6C(CemeteryActor *actor, int variant);
extern int func_8005C49C(void);
extern u32 func_8016F1AC(void);
extern int func_8012BD14(CemeteryActor *actor);
extern void func_8017D884(void);

void func_8017E414(CemeteryActor *tomb);
void func_8017E524(CemeteryActor *tomb);
void func_8017EC30(CemeteryActor *key);

#endif
