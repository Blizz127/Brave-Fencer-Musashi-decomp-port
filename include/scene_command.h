#ifndef BFM_SCENE_COMMAND_H
#define BFM_SCENE_COMMAND_H

#include "psx_types.h"

/* Commands carry PS1 addresses in 32-bit words, including the scene pointer. */
#if defined(__mips__) && !defined(__LP64__)
typedef u32 SceneAddressInteger;
#else
#include <stdint.h>
typedef uintptr_t SceneAddressInteger;
#endif

/* The MAIN overlay retains this guest address until it processes the room request. */
void func_800D1724(const u8 *descriptor);
void func_80178B18(void *actor, const u8 *scene);
void func_80178CBC(void *actor, const u32 *commands);

#endif
