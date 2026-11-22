#include "mine_emitter_stages.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    MineActorStage header;
    unsigned char rest[0x108];
} ActorFixture;

_Static_assert(sizeof(ActorFixture) == 0x10c, "field actor size");
_Static_assert(offsetof(MineActorStage, stage) == 2, "stage offset");

MineStageAction D_8019B5A4[2];
MineStageAction D_8019B634[2];
static MineActorStage *expected_actor;
static unsigned int counts[4];

static void record(unsigned int state, MineActorStage *actor)
{
    ActorFixture *f = (ActorFixture *)actor;
    assert(actor == expected_actor);
    ++counts[state];
    actor->stage ^= 1;
    f->rest[0x100] += state + 1;
}

static void parent_start(MineActorStage *actor) { record(0, actor); }
static void parent_update(MineActorStage *actor) { record(1, actor); }
static void child_start(MineActorStage *actor) { record(2, actor); }
static void child_update(MineActorStage *actor) { record(3, actor); }

static void check(MineStageAction select, ActorFixture *actor, unsigned int id)
{
    unsigned int previous[4], i;
    ActorFixture expected = *actor;
    memcpy(previous, counts, sizeof(previous));
    expected_actor = &actor->header;
    if (id < 4) {
        expected.header.stage ^= 1;
        expected.rest[0x100] += id + 1;
    }
    select(&actor->header);
    assert(memcmp(actor, &expected, sizeof(expected)) == 0);
    for (i = 0; i < 4; ++i)
        assert(counts[i] == previous[i] + (i == id));
}

int main(void)
{
    ActorFixture a;
    unsigned int i;
    const unsigned short unsupported[] = {2, 3, 0x0100, 0x0101, 0x7fff, 0x8001, 0x8000, 0xffff};
    memset(&a, 0x5a, sizeof(a));
    D_8019B5A4[0] = parent_start;
    D_8019B5A4[1] = parent_update;
    D_8019B634[0] = child_start;
    D_8019B634[1] = child_update;
    a.header.kind = 0x1a8;
    a.header.stage = 0;
    check(func_8017E758, &a, 0);
    check(func_8017E758, &a, 1);
    a.header.kind = 0x1a9;
    check(func_8017EE9C, &a, 2);
    check(func_8017EE9C, &a, 3);

    /* The outer object table has already chosen the routine. */
    a.header.kind = 0x1234;
    a.header.stage = 1;
    check(func_8017E758, &a, 1);
    check(func_8017EE9C, &a, 2);
    D_8019B634[1] = parent_start;
    check(func_8017EE9C, &a, 0);
    D_8019B634[0] = 0;
    check(func_8017EE9C, &a, 4);
    D_8019B5A4[1] = 0;
    a.header.stage = 1;
    check(func_8017E758, &a, 4);

    for (i = 0; i < sizeof(unsupported) / sizeof(unsupported[0]); ++i) {
        a.header.stage = unsupported[i];
        check(func_8017E758, &a, 4);
        check(func_8017EE9C, &a, 4);
    }
    func_8017E758(0);
    func_8017EE9C(0);
    assert(counts[0] == 2 && counts[1] == 2 && counts[2] == 2 && counts[3] == 1);
    return 0;
}
