#include "mine_collision_dispatch.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static Obj8014A2E4 *expected_actor;
static unsigned calls;
static void initialize(Obj8014A2E4 *actor)
{
    assert(actor == expected_actor);
    ++calls;
    actor->pad002[4] ^= 0x71;
}
static void update(Obj8014A2E4 *actor)
{
    assert(actor == expected_actor);
    ++calls;
    actor->pad002[9] ^= 0x2d;
}
int main(void)
{
    const MineCollisionActorActions actions = {initialize,update};
    const MineCollisionActorActions missing = {0,0};
    Obj8014A2E4 pool[3];
    unsigned char *bytes = (unsigned char *)pool;
    for (size_t i=0; i<sizeof(pool); ++i)
        bytes[i]=(unsigned char)(i*41u+23u);
    expected_actor=&pool[1];
    assert(!bfm_mine_collision_actor_update(NULL,&actions));
    assert(!bfm_mine_collision_actor_update(expected_actor,NULL));
    for (unsigned state=0; state<=UINT16_MAX; ++state) {
        Obj8014A2E4 expected[3];
        expected_actor->pad002[0]=(u8)state;
        expected_actor->pad002[1]=(u8)(state>>8);
        memcpy(expected,pool,sizeof(pool));
        assert(!bfm_mine_collision_actor_update(expected_actor,&missing));
        assert(!memcmp(expected,pool,sizeof(pool)));
        if (state==0) expected[1].pad002[4]^=0x71;
        if (state==1) expected[1].pad002[9]^=0x2d;
        assert(bfm_mine_collision_actor_update(expected_actor,&actions)==(state<2));
        assert(!memcmp(expected,pool,sizeof(pool)));
    }
    assert(calls==2);
    return 0;
}
