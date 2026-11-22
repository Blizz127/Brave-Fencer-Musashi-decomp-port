#include "camera_transform.h"

#include <assert.h>
#include <string.h>

typedef union {
    ForestCameraMatrix matrix;
    ForestCameraVector vector;
} Slot;

typedef struct {
    Slot slots[3];
    unsigned matrix, source, destination;
    unsigned calls;
    int *parent_flags;
} Probe;

static Probe *active;

static void run(Probe *probe);

void func_8004914C(void *matrix)
{
    Probe *p = active;
    assert(p->calls == 0 && matrix == &p->slots[p->matrix].matrix);
    ++p->calls;
    *(unsigned char *)matrix += 11;
}

void func_800491AC(void *matrix)
{
    Probe *p = active;
    assert(p->calls == 1 && matrix == &p->slots[p->matrix].matrix);
    ++p->calls;
    *(unsigned char *)matrix *= 3;
}

void func_8004945C(void *source, void *destination, int *flags)
{
    Probe *p = active;
    assert(p->calls == 2);
    ++p->calls;
    assert(source == &p->slots[p->source].vector);
    assert(destination == &p->slots[p->destination].vector);
    assert(flags && flags != p->parent_flags);
    const unsigned char *storage = (const unsigned char *)&p->slots;
    for (size_t i = 0; i < sizeof(p->slots); ++i)
        assert((const void *)flags != storage + i);
    *(unsigned char *)destination = (unsigned char)(*(unsigned char *)source - 7);
    *flags = p->parent_flags ? -23 : 91;
    if (!p->parent_flags) {
        Probe nested = {.matrix = 2, .source = 2, .destination = 2, .parent_flags = flags};
        run(&nested);
        assert(active == p && *flags == 91);
    }
}

static void run(Probe *p)
{
    Probe *previous = active;
    active = p;
    unsigned char expected[3][sizeof(Slot)];
    for (unsigned i = 0; i < 3; ++i) {
        memset(&p->slots[i], 19 + 17 * i, sizeof(Slot));
        memcpy(expected[i], &p->slots[i], sizeof(Slot));
    }
    expected[p->matrix][0] = (unsigned char)((expected[p->matrix][0] + 11) * 3);
    expected[p->destination][0] = (unsigned char)(expected[p->source][0] - 7);

    func_8012F14C(&p->slots[p->matrix].matrix, &p->slots[p->source].vector,
                  &p->slots[p->destination].vector);
    assert(p->calls == 3 && memcmp(expected, p->slots, sizeof(expected)) == 0);
    active = previous;
}

int main(void)
{
    for (unsigned m = 0; m < 3; ++m) {
        for (unsigned s = 0; s < 3; ++s) {
            for (unsigned d = 0; d < 3; ++d) {
                Probe p = {.matrix = m, .source = s, .destination = d};
                run(&p);
            }
        }
    }
    return 0;
}
