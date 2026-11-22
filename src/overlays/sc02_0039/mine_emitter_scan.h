#ifndef BFM_MINE_EMITTER_SCAN_H
#define BFM_MINE_EMITTER_SCAN_H

#include "mine_child_spawn.h"
#include "mine_guest_abi.h"
#include "../main_0012/scene_object_probes.h"

void func_8017E910(Obj8014A2E4 *actor);
#if BFM_MINE_DIRECT_GUEST_ADDRESS
void func_8002D4C8(u16 command, u16 argument);
#else
void bfm_mine_emitter_audio(u16 command, u16 argument);
#endif
#endif
