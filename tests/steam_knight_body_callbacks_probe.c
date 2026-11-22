#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "steam_knight_hit.h"

void func_8018415C(SteamKnightPart *core);
void func_80186ADC(SteamKnightPart *body);

_Static_assert(sizeof(SteamKnightPart) == 0x10c, "actor record size");
_Static_assert(offsetof(SteamKnightPart, field_0e) == 0x0e, "marker gate");
_Static_assert(offsetof(SteamKnightPart, model_address) == 0x20, "model handle");
_Static_assert(offsetof(SteamKnightPart, field_ae) == 0xae, "marker value");
_Static_assert(offsetof(SteamKnightPart, linked_address) == 0xcc, "attached actor");
_Static_assert(offsetof(SteamKnightPart, field_fc) == 0xfc, "marker flags");

static const u32 body_record = 0x801961b8u;
static const u32 attachment_record = 0x801961e0u;

enum ActorHandle { ATTACHED = 17, SELF = 19 };
enum ModelHandle { FIRST = 31, SECOND, THIRD, INTO_LINK, INTO_LINK_MODEL, INTO_BODY_MODEL };

typedef struct {
    u8 prefix[128];
    SteamKnightPart body, linked, alternate;
    u8 models[3][160];
} Scene;

typedef struct {
    u32 body_model, link, linked_model;
    u32 resolved_link, resolved_model;
    size_t first_store, second_store;
} Layout;

#define SLOT(i) (offsetof(Scene, models) + (i) * 160 + 0x80)
#define BODY_FIELD(member) (offsetof(Scene, body) + offsetof(SteamKnightPart, member))
#define LINK_FIELD(member) (offsetof(Scene, linked) + offsetof(SteamKnightPart, member))

static Scene scene, after_first;
static const Layout *layout;
static int lookups;

static void write_record(Scene *state, size_t offset, u32 token)
{
    assert(offset + sizeof token <= sizeof *state);
    memcpy((u8 *)state + offset, &token, sizeof token);
}

SteamKnightPart *bfm_steam_knight_break_actor(u32 handle)
{
    assert(lookups == 1 && handle == layout->resolved_link);
    assert(memcmp(&scene, &after_first, sizeof scene) == 0);
    ++lookups;
    if (handle == SELF) return &scene.body;
    if (handle == ATTACHED) return &scene.linked;
    assert(handle == body_record);
    return &scene.alternate;
}

u8 *bfm_steam_knight_core_model(u32 handle)
{
    if (lookups == 0) {
        assert(handle == layout->body_model);
    } else {
        assert(lookups == 2 && handle == layout->resolved_model);
        assert(memcmp(&scene, &after_first, sizeof scene) == 0);
    }
    ++lookups;

    if (handle == body_record) return scene.models[2];
    switch (handle) {
    case FIRST: return scene.models[0];
    case SECOND: return scene.models[1];
    case THIRD: return scene.models[2];
    case INTO_LINK: return (u8 *)&scene + BODY_FIELD(linked_address) - 0x80;
    case INTO_LINK_MODEL: return (u8 *)&scene + LINK_FIELD(model_address) - 0x80;
    default:
        assert(handle == INTO_BODY_MODEL);
        return (u8 *)&scene + BODY_FIELD(model_address) - 0x80;
    }
}

static void check_restore(void)
{
    /* The last four layouts place a model slot directly over a handle field. */
    const Layout layouts[] = {
        {FIRST, ATTACHED, SECOND, ATTACHED, SECOND, SLOT(0), SLOT(1)},
        {FIRST, ATTACHED, FIRST, ATTACHED, FIRST, SLOT(0), SLOT(0)},
        {FIRST, SELF, SECOND, SELF, FIRST, SLOT(0), SLOT(0)},
        {INTO_LINK, ATTACHED, SECOND, body_record, THIRD, BODY_FIELD(linked_address), SLOT(2)},
        {INTO_LINK_MODEL, ATTACHED, SECOND, ATTACHED, body_record, LINK_FIELD(model_address), SLOT(2)},
        {INTO_BODY_MODEL, SELF, SECOND, SELF, body_record, BODY_FIELD(model_address), SLOT(2)},
        {FIRST, ATTACHED, INTO_LINK, ATTACHED, INTO_LINK, SLOT(0), BODY_FIELD(linked_address)},
    };
    for (size_t i = 0; i < sizeof layouts / sizeof layouts[0]; ++i) {
        Scene expected;
        layout = &layouts[i];
        for (size_t j = 0; j < sizeof scene; ++j)
            ((u8 *)&scene)[j] = (u8)(j * 13 + i * 19);
        scene.body.model_address = layout->body_model;
        scene.body.linked_address = layout->link;
        scene.linked.model_address = layout->linked_model;
        scene.alternate.model_address = THIRD;
        memcpy(&after_first, &scene, sizeof scene);
        write_record(&after_first, layout->first_store, body_record);
        memcpy(&expected, &after_first, sizeof expected);
        write_record(&expected, layout->second_store, attachment_record);
        lookups = 0;
        func_80186ADC(&scene.body);
        assert(lookups == 3 && memcmp(&scene, &expected, sizeof scene) == 0);
    }
}

static void check_marker(void)
{
    static const s16 gates[] = {0, 1, -1, -32768, 32767};
    static const s16 markers[] = {-1, 0, 123};
    static const u16 flags[] = {0, 2, 0x8001, 0xfffd, 0xffff};
    for (size_t i = 0; i < sizeof gates / sizeof gates[0]; ++i) {
        for (size_t j = 0; j < sizeof markers / sizeof markers[0]; ++j) {
            for (size_t k = 0; k < sizeof flags / sizeof flags[0]; ++k) {
                Scene expected;
                memset(&scene, 0x5b, sizeof scene);
                scene.body.field_0e = gates[i];
                scene.body.field_ae = markers[j];
                scene.body.field_fc = flags[k];
                memcpy(&expected, &scene, sizeof expected);
                if (gates[i] == 0) {
                    expected.body.field_ae = -1;
                    expected.body.field_fc = flags[k] | 2;
                }
                for (int repeat = 0; repeat < 2; ++repeat) {
                    func_8018415C(&scene.body);
                    assert(memcmp(&scene, &expected, sizeof scene) == 0);
                }
            }
        }
    }
}

int main(void)
{
    check_restore();
    check_marker();
    return 0;
}
