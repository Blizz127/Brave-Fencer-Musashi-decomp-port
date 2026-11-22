#ifndef BFM_ACTOR_POSITION_UPDATES_H
#define BFM_ACTOR_POSITION_UPDATES_H

#include "../main_0012/scene_object_probes.h"

typedef void (*SceneActorUpdateHandler)(Obj8014A2E4 *);

extern s16 D_801270C0;
extern u32 D_8011DB08;
void func_8017849C(void);
void func_801787D4(void);
void func_8012A908(void);
void func_8017869C(Obj8014A2E4 *actor);
s32 func_80130D0C(Obj8014A2E4 *actor);
void func_8013E370(void);

#if !defined(__mips__) || defined(__LP64__)
enum {
    BFM_ACTOR_UPDATE_BAD_TABLE = 1,
    BFM_ACTOR_UPDATE_BAD_HANDLER,
    BFM_ACTOR_UPDATE_BAD_CHILD
};

int bfm_actor_update_handler_key(u32 table, u16 type, u32 *key);
SceneActorUpdateHandler bfm_actor_update_handler(u32 key);
u8 *bfm_actor_update_child_span(u32 child, unsigned offset, unsigned width);
void bfm_actor_update_refused(unsigned reason, u32 key, unsigned detail);
#endif

#endif
