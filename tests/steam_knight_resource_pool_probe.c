#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "steam_knight_resource.h"

enum { slot_bytes = 56, slots = 128 };
u8 D_8011DB30[slots * slot_bytes + slot_bytes];

typedef union {
    TowerJointModelResource fields;
    u8 bytes[136];
} TestModel;
struct Record { u16 state; u8 rest[0x82]; };
static struct Record shared;
static TestModel *models;
static const u32 keys[2] = {0xa151672cu, 0x8efd31c8u};
static int offered, shared_available, expected_kind;
static u8 *last_slot;
static char calls[12];
static size_t call_count;

static void mark(char c)
{
    assert(call_count + 1 < sizeof calls);
    calls[call_count++] = c;
    calls[call_count] = 0;
}
static void expect(const char *sequence)
{
    assert(strcmp(calls, sequence) == 0);
    calls[call_count = 0] = 0;
}
static u16 half(const u8 *data)
{
    return (u16)data[0] | (u16)((u16)data[1] << 8);
}
static u32 key_at(const u8 *slot)
{
    u32 key;
    memcpy(&key, slot + 32, sizeof key);
    return key;
}
u16 *func_8001D074(s32 first, s32 limit)
{
    assert(first == 126 && limit == 256);
    mark('Q');
    return offered < 0 ? 0 : (u16 *)&models[offered].fields.unk0;
}
u32 bfm_tower_spawn_address(const void *ptr)
{
    assert(offered >= 0 && ptr == models[offered].bytes);
    assert(last_slot && half(last_slot) == expected_kind && key_at(last_slot) == 0);
    mark('K');
    return keys[offered];
}
void *bfm_tower_spawn_pointer(u32 key)
{
    int which = key == keys[0] ? 0 : 1;
    assert(keys[which] == key);
    mark('P');
    return models[which].bytes;
}
void func_8001CC3C(TowerJointModelResource *model, void *parent, s16 x, s16 y)
{
    assert(model == &models[offered].fields && !parent && !x && !y);
    assert(key_at(last_slot) == keys[offered]);
    mark('I');
    model->unk2 = 3;
    model->unk20 = 0;
    model->unk28 = x;
    model->unk2A = y;
    model->unk2C = 0;
}
struct Record *func_8001CFDC(int first, int limit)
{
    assert(first == 38 && limit == 133);
    mark('S');
    return shared_available ? &shared : 0;
}
void func_80016714_native(u8 *data, s32 size)
{
    if (data == models[0].bytes || data == models[1].bytes) {
        assert(size == (half(data) == 1 ? 132 : 56));
        mark('R');
    } else {
        uintptr_t offset = (uintptr_t)data - (uintptr_t)D_8011DB30;
        assert(size == slot_bytes && offset % slot_bytes == 0 && offset / slot_bytes < slots);
        last_slot = data;
        mark('C');
    }
    memset(data, 0, (size_t)size);
}
static void directory_capacity(void)
{
    int i, j;
    memset(D_8011DB30, 0xac, sizeof D_8011DB30);
    for (i = 0; i < slots; ++i) {
        u8 *slot = D_8011DB30 + i * slot_bytes;
        slot[0] = slot[1] = 0;
    }
    memset(D_8011DB30 + slots * slot_bytes, 0, slot_bytes);
    for (i = 0; i < slots; ++i) {
        u8 *slot = func_801291C0();
        assert(slot == D_8011DB30 + i * slot_bytes);
        expect("C");
        for (j = 0; j < slot_bytes; ++j) assert(slot[j] == 0);
        slot[1] = 1;
    }
    assert(func_801291C0() == 0);
    expect("");
    offered = 0;
    assert(func_8012913C(7) == 0);
    expect("Q");
    func_801292C8((char *)D_8011DB30 + 127 * slot_bytes);
    expect("");
    assert(func_801291C0() == D_8011DB30 + 127 * slot_bytes);
    expect("C");
}
static void pair_and_release(void)
{
    u8 *first, *second, snapshot[sizeof(TestModel)];
    size_t i;
    memset(D_8011DB30, 0, slots * slot_bytes);
    offered = -1;
    assert(!func_8012913C(9));
    expect("Q");

    offered = 0;
    expected_kind = 0;
    first = func_8012913C(0);
    assert(first == D_8011DB30 && !half(first) && key_at(first) == keys[0]);
    expect("QCKI");
    offered = 1;
    expected_kind = 0xfe01;
    assert(func_8012913C((u16)expected_kind) == first);
    expect("QCKI");
    assert(half(first) == 0xfe01 && key_at(first) == keys[1]);

    offered = 0;
    expected_kind = 5;
    second = func_8012913C(5);
    expect("QCKI");
    assert(second == first + slot_bytes && key_at(second) == keys[0]);
    assert(models[0].fields.unk2 == 3 && models[1].fields.unk2 == 3);
    models[1].fields.unk0 = 1;
    models[0].fields.unk0 = 0x101;
    memcpy(snapshot, models[0].bytes, sizeof snapshot);
    func_801292C8((char *)first);
    expect("PR");
    assert(!half(first) && key_at(first) == keys[1] && half(second) == 5);
    assert(memcmp(models[0].bytes, snapshot, sizeof snapshot) == 0);
    for (i = 0; i < sizeof(TestModel); ++i)
        assert(models[1].bytes[i] == (i < 132 ? 0 : 0xa6));
    func_801292C8((char *)second);
    expect("PR");
    assert(!half(second) && key_at(second) == keys[0]);
    for (i = 0; i < sizeof(TestModel); ++i)
        assert(models[0].bytes[i] == (i < 56 ? 0 : snapshot[i]));

    offered = 1;
    expected_kind = 9;
    assert(func_8012913C(9) == first && key_at(first) == keys[1]);
    expect("QCKI");
    assert(!func_8012C1B8());
    expect("S");
    shared_available = 1;
    assert(func_8012C1B8() == &shared);
    expect("S");
}
int main(void)
{
    TestModel backing[2];
    models = backing;
#if UINTPTR_MAX > UINT32_MAX
    if ((uintptr_t)models <= UINT32_MAX) {
        static TestModel image_models[2];
        models = image_models;
    }
    assert((uintptr_t)models > UINT32_MAX);
#endif
    memset(models, 0xa6, sizeof backing);
    directory_capacity();
    pair_and_release();
    return 0;
}
