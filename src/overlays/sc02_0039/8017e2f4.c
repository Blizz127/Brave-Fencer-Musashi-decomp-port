/* Probe five longitudinal segments across the mine actor's collision box. */
#include "mine_collision_actor_active.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
extern s32 func_8012DEB8(char *actor, int start, int end);

s32 func_8017E2F4(Obj8014A2E4 *actor, SceneSurfaceBox *box)
{
    SceneProbeVector start, end;
    s16 middle;
    start.x = end.x = box->bounds[0];
    start.y = end.y = box->bounds[2] - 16;
    start.z = box->bounds[4];
    end.z = box->bounds[5];
    if (func_8012DEB8((char *)actor, (int)&start, (int)&end))
        return 1;
    middle = (box->bounds[0] + box->bounds[1]) >> 1;
    start.x = end.x = middle;
    if (func_8012DEB8((char *)actor, (int)&start, (int)&end))
        return 1;
    start.x = end.x = (box->bounds[0] + middle) >> 1;
    if (func_8012DEB8((char *)actor, (int)&start, (int)&end))
        return 1;
    start.x = end.x = (box->bounds[1] + middle) >> 1;
    if (func_8012DEB8((char *)actor, (int)&start, (int)&end))
        return 1;
    start.x = end.x = box->bounds[1];
    if (func_8012DEB8((char *)actor, (int)&start, (int)&end))
        return 1;
    return 0;
}
#else
#include "mine_actor_fields.h"

static int16_t midpoint(int a, int b)
{
    int sum = a + b;
    return (int16_t)(sum / 2 - (sum < 0 && sum % 2 != 0));
}

int mine_actor_sweep(Obj8014A2E4 *actor, const MineActiveServices *services, int *hit)
{
    if (!actor || !services || !services->box || !services->segment || !hit)
        return 0;
    const s16 *bounds = services->box->bounds;
    MineSweepPoint start, end;
    int16_t middle = 0;
    start.y = end.y = (int16_t)mine_actor_signed16((uint16_t)(bounds[2] - 16));
    start.z = bounds[4];
    end.z = bounds[5];
    for (unsigned i = 0; i < 5; ++i) {
        switch (i) {
        case 0: start.x = bounds[0]; break;
        case 1: middle = start.x = midpoint(bounds[0], bounds[1]); break;
        case 2: start.x = midpoint(bounds[0], middle); break;
        case 3: start.x = midpoint(bounds[1], middle); break;
        default: start.x = bounds[1]; break;
        }
        end.x = start.x;
        int result;
        if (!services->segment(services->context, actor, &start, &end, &result))
            return 0;
        if (result) {
            *hit = 1;
            return 1;
        }
    }
    *hit = 0;
    return 1;
}
#endif
