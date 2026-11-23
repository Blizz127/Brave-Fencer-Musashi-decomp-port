#ifndef MUSASHI_TWINPEAK_FUSION_QUEUE_GAME_H
#define MUSASHI_TWINPEAK_FUSION_QUEUE_GAME_H

typedef unsigned short FusionHalf;
typedef unsigned int FusionWord;

typedef struct FusionRequest {
    FusionHalf kind;
    FusionHalf component0;
    FusionHalf component1;
    FusionHalf component2;
    FusionWord actor;
    FusionWord parameter5;
    FusionWord parameter6;
} FusionRequest;

typedef struct FusionQueueSlot {
    FusionHalf kind;
    unsigned char unknown02[4];
    FusionHalf component0;
    unsigned char unknown08[2];
    FusionHalf component1;
    unsigned char unknown0c[2];
    FusionHalf component2;
    unsigned char unknown10[60];
    FusionWord actor;
    FusionWord parameter5;
    FusionWord parameter6;
    unsigned char unknown58[16];
} FusionQueueSlot;

typedef struct TwinpeakActionState {
    FusionHalf phase;
    unsigned char unknown04[0xb4];
} TwinpeakActionState;

typedef struct Action21Actor {
    FusionHalf current_action;
#if defined(__GNUC__) && __GNUC__ == 2
    /* The original compiler predates anonymous unions. */
    TwinpeakActionState state;
#else
    union {
        unsigned char reserved[0xb6];
        TwinpeakActionState state;
    };
#endif
    FusionHalf frame_events;
    FusionHalf next_frame_events;
} Action21Actor;

/* The original carries 32-bit guest addresses. A host actor reference is
 * resolved separately, while queue slot return values retain their guest VA. */
#if defined(__mips__) && !defined(__LP64__)
#define TWINPEAK_ACTOR_WORD(actor) ((FusionWord)(actor))
#define TWINPEAK_SLOT_WORD(index) ((FusionWord)&D_8011F9D0[index])
#else
extern FusionWord bfm_twinpeak_actor_word(const Action21Actor *actor);
#define TWINPEAK_ACTOR_WORD(actor) bfm_twinpeak_actor_word(actor)
#define TWINPEAK_SLOT_WORD(index) (0x8011f9d0u + (FusionWord)(index) * 104u)
#define D_8011F9D0 bfm_twinpeak_primary_slots
#define D_80189EAC bfm_twinpeak_action_callbacks
#define D_8018A1F4 bfm_twinpeak_action22_phases
#define func_801466F0 bfm_twinpeak_emit_queue
#define func_80146750 bfm_twinpeak_secondary_enqueue
#define func_8014680C bfm_twinpeak_primary_enqueue
#define func_80147078 bfm_twinpeak_change_action
#define func_80154A74 bfm_twinpeak_request_index
#define func_80159B70 bfm_twinpeak_reset_action
#define func_8015F89C bfm_twinpeak_throw_update
#define func_8015FA78 bfm_twinpeak_begin_action22
#define func_8015FAAC bfm_twinpeak_update_action22
#define func_8015FE70 bfm_twinpeak_finish_action22
#define func_80161208 bfm_twinpeak_tick_action
#endif

typedef void (*TwinpeakActionCallback)(Action21Actor *);
extern TwinpeakActionCallback D_80189EAC[];
extern TwinpeakActionCallback D_8018A1F4[];
extern void func_80147078(Action21Actor *, unsigned int);
extern void func_80154A74(Action21Actor *, unsigned int);
extern void func_80159B70(Action21Actor *);
extern void func_8015FA78(Action21Actor *);
extern void func_8015FAAC(Action21Actor *);
extern void func_8015FE70(Action21Actor *);
extern void func_80161208(Action21Actor *);

extern FusionQueueSlot D_8011F9D0[8];
/* The secondary callee's result is an opaque guest word passed through by
 * its caller. Its allocation policy and external implementation are separate. */
extern FusionWord func_80146750(FusionRequest *);
extern FusionWord func_8014680C(FusionRequest *);
extern void func_8015F89C(Action21Actor *actor);
extern FusionWord func_801466F0(int kind, FusionWord actor, int component0, int component1,
                               FusionHalf component2, FusionWord parameter5,
                               FusionWord parameter6, int secondary);

#endif
