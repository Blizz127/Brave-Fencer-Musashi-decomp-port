/* Synthetic fixture. Model references use guest-width values on every host. */
#ifdef BFM_TEST_HIGH_GUEST_ADDRESS
#define _GNU_SOURCE
#include <sys/mman.h>
#include <unistd.h>
#endif
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../src/overlays/sc01_0077/8017f5bc.c"

static FallingActor owner, child;
static FallModel previous_model, replacement_model, child_model;

static FallingActor *spawn_result;
static int calls;

_Static_assert(offsetof(FallingActor, state) == 2 && offsetof(FallingActor, model) == 0x20, "guest state and pointer");
_Static_assert(offsetof(FallingActor, x) == 6 && offsetof(FallingActor, y) == 0xa && offsetof(FallingActor, z) == 0xe, "guest coordinates");
_Static_assert(offsetof(FallingActor, phase) == 0x34 && offsetof(FallingActor, flags) == 0x5c && offsetof(FallingActor, variant) == 0x70, "guest phase, flags and variant");
_Static_assert(offsetof(FallingActor, timer) == 0x1c && offsetof(FallingActor, offset) == 0xdc && offsetof(FallingActor, velocity) == 0xe0 && offsetof(FallingActor, acceleration) == 0xe4, "guest simulation fields");
_Static_assert(sizeof(FallPlacement) == 20 && offsetof(FallPlacement, user) == 0x10 && offsetof(FallPlacement, aim) == 0xc, "placement layout");

static u32 address_of(const void *value)
{
    uintptr_t address = (uintptr_t)value;
    assert(address <= UINT32_MAX);
    return (u32)address;
}

s32 func_8012C51C(void *descriptor, u8 *owner_data)
{
    FallPlacement *placement = descriptor;
    FallingActor *parent = (FallingActor *)owner_data;
    assert(parent == &owner && owner.state == 2 && owner.phase == 0);
    assert(owner.timer == 16 && owner.offset == 0);
    assert(owner.velocity == 0x100000 && owner.acceleration == 0x40000);
    assert(owner.flags == 0x73ff && previous_model.rotation_x == 0);
    assert(placement->x == 0xef && placement->y == 0xabcd && placement->z == 0x1e);
    assert(placement->type == 47 && placement->selector == 0x600);
    assert(placement->flags == 0 && placement->parameter == 0 && placement->user == 0);
    /* The caller does not initialize +0xc. The witness never reads that field. */
    calls++;
    owner.model = address_of(&replacement_model);
    return (s32)address_of(spawn_result);
}

static void run(u8 variant, FallingActor *result)
{
    FallingActor expected;
    memset(&owner, 0xa5, sizeof(owner));
    if (result && result != &owner)
        memset(result, 0x5a, sizeof(*result));
    owner.x = 0xffff;
    owner.y = 0xabcd;
    owner.z = 0xfffe;
    owner.variant = variant;
    owner.flags = 0xffff;
    owner.model = address_of(&previous_model);
    if (result && result != &owner)
        result->model = address_of(&child_model);
    previous_model.rotation_x = 0x2222;
    previous_model.rotation_y = 0xbaad;
    replacement_model.rotation_y = 0xcafe;
    child_model.rotation_y = 0x7654;
    expected = owner;
    expected.state = 2;
    expected.phase = 0;
    expected.flags = 0x73ff;
    expected.velocity = 0x100000;
    expected.acceleration = 0x40000;
    expected.offset = 0;
    expected.timer = 16;
    if (variant != 4)
        expected.model = address_of(&replacement_model);
    spawn_result = result;
    calls = 0;
    func_8017F5BC((u8 *)&owner);
    assert(calls == (variant != 4));
    assert(memcmp(&owner, &expected, sizeof(owner)) == 0);
    assert(previous_model.rotation_x == 0 && previous_model.rotation_y == 0xbaad);
    assert(replacement_model.rotation_y == 0xcafe);
    assert(child_model.rotation_y == ((variant != 4 && result && result != &owner) ? 0xcafe : 0x7654));
}

int main(void)
{
    run(4, &child);   /* Bridge variant skips spawning. */
    run(0, NULL);     /* Null after replacing the owner model. */
    run(2, &child);   /* Child gets the replacement model's Y rotation. */
    run(255, &owner); /* Returned actor aliases its owner. */
#ifdef BFM_TEST_HIGH_GUEST_ADDRESS
    /* Exercise the signed-word ABI with bit 31 set. This separate UBSan fixture
     * uses a Linux address occupied by shadow memory in an ASan process. */
    {
        size_t size = (size_t)sysconf(_SC_PAGESIZE);
        void *address = (void *)(uintptr_t)0x90000000u;
        void *mapped = mmap(address, size, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
        assert(mapped == address && sizeof(FallingActor) <= size);
        assert((s32)address_of(mapped) < 0);
        run(2, mapped);
        assert(munmap(mapped, size) == 0);
    }
#endif
    return 0;
}
