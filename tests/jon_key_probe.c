#include "jon_key.h"
#include <assert.h>

static unsigned char old_resource;
unsigned char D_801EB318[1], D_801F1468[1];
static JonKeyActor subject;
static JonKeyPart original_part, replacement_part;
static int calls;

void func_8012C588(int id, JonKeyActor *actor)
{
    assert(calls == 0 && id == 141 && actor == &subject);
    assert(actor->part == &original_part);
    assert(actor->part->resource == &old_resource);
    actor->part = &replacement_part;
    ++calls;
}

void func_8012A828(JonKeyActor *actor, unsigned char *data)
{
    assert(calls == 1 && actor == &subject && data == D_801F1468);
    assert(actor->part == &replacement_part);
    assert(actor->part->resource == D_801EB318);
    ++calls;
}

int main(void)
{
    original_part.resource = &old_resource;
    subject.part = &original_part;
    func_801F0048(&subject);
    assert(calls == 2 && original_part.resource == &old_resource);
    return 0;
}
