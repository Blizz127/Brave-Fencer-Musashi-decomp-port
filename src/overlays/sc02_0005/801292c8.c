/* Release a Tower effect's linked model and mark its record free. */
#include "psx_types.h"

#if defined(__mips__) && !defined(__LP64__)
extern void func_80016714(s8 *data, s32 size);
typedef u8 *ModelLink;
#define LINK(record) (*(u8 **)((record) + 0x20))
#define POINTER(link) (link)
#define KIND(model) (*(u16 *)(model))
#define CLEAR(model, size) func_80016714((s8 *)(model), (size))
#define RELEASE(record) (*(u16 *)(record) = 0)
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);
extern void func_80016714_native(u8 *data, s32 size);

typedef u32 ModelLink;

static ModelLink linked_model(const char *record)
{
    u32 address;
    memcpy(&address, record + 0x20, sizeof address);
    return address;
}

static u16 model_kind(const u8 *model)
{
    u16 kind;
    memcpy(&kind, model, sizeof kind);
    return kind;
}

static void release_record(char *record)
{
    u16 zero = 0;
    memcpy(record, &zero, sizeof zero);
}
#define LINK(record) linked_model(record)
#define POINTER(link) ((u8 *)bfm_tower_spawn_pointer(link))
#define KIND(model) model_kind(model)
#define CLEAR(model, size) func_80016714_native((model), (size))
#define RELEASE(record) release_record(record)
#endif

void func_801292C8(char *record)
{
    ModelLink link = LINK(record);
    if (link) {
        u8 *model = POINTER(link);
        s32 size = 56;
        if (KIND(model) == 1)
            size = 132;
        CLEAR(model, size);
    }
    RELEASE(record);
}
