#include "saved_room_actor.h"

extern const u8 *const D_8019F144[];
extern s16 func_8017D430(void);
extern void func_80139A44(const u8 *text, u16 slot);

s32 func_8017F6C8(struct FileActor *actor)
{
    s16 choice = func_8017D430();
    s16 chapter_index;

    actor->field10A = choice;
    if (choice == 0) {
        return 0;
    }

    /* Book and chapter numbers start at one. Each book has five catalogue slots. */
    chapter_index = 5 * ((choice >> 8) - 1) + ((choice & 7) - 1);
    func_80139A44(D_8019F144[chapter_index], 0);
    return 1;
}
