#include "scene_command.h"

extern u32 D_80185CE0[];
extern void func_80178BF8(void);
extern void func_801746A4(void);
extern void func_8017435C(void);

void func_80178B18(void *actor, const u8 *scene)
{
    /* The second command in the launch sequence takes the selected scene address. */
    D_80185CE0[3] = (u32)(SceneAddressInteger)scene;
    func_80178BF8();
    func_801746A4();
    func_8017435C();
    func_80178CBC(actor, D_80185CE0);
}
