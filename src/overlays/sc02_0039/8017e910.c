#include "mine_emitter_projection.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
#define COORD(actor, offset) (*(s16 *)((u8 *)(actor) + (offset)))
#define STORE_DEPTH(actor, value) (*(s32 *)((u8 *)(actor) + 0xe0) = (value))
#define STORE_RANGE(actor, value) (*(s16 *)((u8 *)(actor) + 0x100) = (value))
#define ROTATE func_8004A0DC
#define SET_ROTATION func_8004914C
#define SET_TRANSLATION func_800491AC
#define TRANSFORM func_8004945C
#define PROJECT func_8004955C
#else
#include "mine_actor_fields.h"
#define COORD(actor, offset) mine_actor_signed16(mine_actor_load16((u8 *)(actor) + (offset)))
#define STORE_DEPTH(actor, value) mine_actor_store32((u8 *)(actor) + 0xe0, (u32)(value))
#define STORE_RANGE(actor, value) mine_actor_store16((u8 *)(actor) + 0x100, (u16)(value))
#define ROTATE bfm_mine_emitter_rotate
#define SET_ROTATION bfm_mine_emitter_set_rotation
#define SET_TRANSLATION bfm_mine_emitter_set_translation
#define TRANSFORM bfm_mine_emitter_transform
#define PROJECT bfm_mine_emitter_project
#endif

void func_8017E910(Obj8014A2E4 *actor)
{
    SceneProbeVector vertex;
    MineEmitterProjectionMatrix matrix;
    s32 flag;
    union { s32 word; u16 coord[2]; } screen;
    s32 cue;
    s16 yaw;

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    if (!actor)
        return;
#endif
    matrix = D_800AE620;
    matrix.words[5] = COORD(actor, 6);
    matrix.words[6] = COORD(actor, 10);
    matrix.words[7] = COORD(actor, 14);
#if BFM_MINE_DIRECT_GUEST_ADDRESS
    yaw = *(s16 *)(actor->probe_object_address + 0x12);
#else
    if (!bfm_mine_emitter_model_yaw(actor->probe_object_address, &yaw))
        return;
#endif
    ROTATE(yaw, &matrix);
    SET_ROTATION(&matrix);
    SET_TRANSLATION(&matrix);
    TRANSFORM(&D_8019B5AC, &vertex, &flag);
    func_8012E32C();
    STORE_DEPTH(actor, PROJECT(&vertex, &screen.word, &cue, &flag));
    {
#if BFM_MINE_DIRECT_GUEST_ADDRESS
        u16 horizontal = screen.coord[0];
#else
        u16 horizontal = (u16)screen.word;
#endif
        s32 status = flag;
        STORE_RANGE(actor, horizontal);
        if (status < 0)
            STORE_RANGE(actor, -999);
    }
}
