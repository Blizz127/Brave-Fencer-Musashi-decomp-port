#include "psx_types.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

extern s32 func_8012C890(void *entry, u8 *slot, u8 *owner);
extern void func_80016714_native(u8 *data, s32 size);

u8 D_8018290C[4];
u16 D_801270C4[1];
u16 D_801274E4[1];
u32 D_8011DB08;

static _Alignas(4) u8 entry[20], slot[268], model[152], parent[268];
static u32 table[2], clear_length[2];
static u8 *clear_target[2];
static unsigned scenario, initialized, cleared;

static u32 address(const void *p)
{
    uintptr_t value = (uintptr_t)p;
    assert(value <= UINT32_MAX);
    return (u32)value;
}
static void put16(u8 *p, u16 value) { memcpy(p, &value, 2); }
static void put32(u8 *p, u32 value) { memcpy(p, &value, 4); }
static u16 get16(u8 *p) { u16 value; memcpy(&value, p, 2); return value; }
static u32 get32(u8 *p) { u32 value; memcpy(&value, p, 4); return value; }
static u16 child_kind(void) { return (u16)((scenario >> 1) & 3); }
static int has_model(void) { return (scenario & 1) != 0; }
static int rejected(void) { return (scenario & 8) != 0; }
static int had_owner(void) { return (scenario & 16) != 0; }

void func_80016714(s8 *p, s32 n)
{
    assert(cleared < 2);
    clear_target[cleared] = (u8 *)p;
    clear_length[cleared++] = (u32)n;
    func_80016714_native((u8 *)p, n);
}

static void initialize_slot(void *data)
{
    assert(data == slot && ++initialized == 1);
    assert(get16(slot) == 1 && get16(slot + 2) == 0);
    assert(get32(slot + 0x78) == address(D_8018290C));
    assert(get16(slot + 0x70) == get16(entry + 8));
    assert(get16(slot + 0x72) == 0x0203);
    assert(get16(slot + 0x36) == (scenario & 32 ? 65535 : 152));
    assert(D_801270C4[0] == (scenario & 32 ? 1 : 153));
    assert(get32(slot + 0x64) == (had_owner() ? address(parent) : 0));
    assert(get32(slot + 0x68) == (had_owner() ? 0 : address(entry)));
    assert(get16(slot + 0xfc) == 87 && get32(slot + 0xdc) == 0x1234u + scenario);
    assert(get16(entry + 10) == 0x8a03 && D_801274E4[0] == 0x4100);
    for (unsigned i = 0; i < 3; ++i) {
        assert(get16(slot + 6 + 4*i) == get16(entry + 2*i));
        assert(get16(slot + 0x88 + 2*i) == get16(entry + 2*i));
        assert(get16(slot + 4 + 4*i) == 0);
        put16(slot + 6 + 4*i, (u16)(65530 + i + scenario));
        put16(slot + 0x50 + 2*i, (u16)(13 + 2*i));
    }
    put32(slot + 0x20, has_model() ? address(model) : 0);
    put32(slot + 0x64, scenario & 64 ? address(parent) : 0);
    if (rejected()) D_801274E4[0] |= 1;
}

int main(void)
{
    D_8011DB08 = address(table);
    table[1] = address((void *)(uintptr_t)&initialize_slot);
    for (scenario = 0; scenario < 256; ++scenario) {
        u8 expected_model[sizeof model];
        memset(entry, 0, sizeof entry);
        memset(slot, 0x95, sizeof slot);
        memset(model, 0xa6, sizeof model);
        put16(model, child_kind());
        put16(entry + 6, 1);
        put16(entry + 8, (u16)(219 * scenario));
        put16(entry + 10, 0x0a03);
        put16(entry + 12, (u16)(scenario & 128 ? 0x7fff : 49152 + scenario));
        put16(entry + 14, 87);
        put32(entry + 16, 0x1234u + scenario);
        for (unsigned i = 0; i < 3; ++i) put16(entry + 2*i, (u16)(scenario*193 + i));
        memcpy(expected_model, model, sizeof model);
        D_801270C4[0] = scenario & 32 ? 65535 : 152;
        D_801274E4[0] = 0x4101;
        initialized = cleared = 0;
        u32 returned = (u32)func_8012C890(entry, slot, had_owner() ? parent : 0);
        assert(initialized == 1);
        if (rejected()) {
            unsigned child_clear = has_model() && (child_kind() == 1 || child_kind() == 2);
            assert(returned == 0 && cleared == 1 + child_clear);
            if (child_clear) {
                unsigned n = child_kind() == 1 ? 132 : 56;
                assert(clear_target[0] == model && clear_length[0] == n);
                memset(expected_model, 0, n);
            }
            assert(clear_target[child_clear] == slot && clear_length[child_clear] == sizeof slot);
            for (unsigned i = 0; i < sizeof slot; ++i) assert(slot[i] == 0);
            assert(get16(entry + 10) == 0x0a03);
        } else {
            assert(returned == address(slot) && cleared == 0);
            assert(get16(entry + 10) == 0x8a03);
            if (has_model()) {
                for (unsigned i = 0; i < 3; ++i)
                    put16(expected_model + 8 + 2*i, (u16)(65530 + i + scenario + 13 + 2*i));
                if (!(scenario & 64) && child_kind() == 1 && !(scenario & 128))
                    put16(expected_model + 18, get16(entry + 12));
            }
        }
        assert(memcmp(model, expected_model, sizeof model) == 0);
    }
}
