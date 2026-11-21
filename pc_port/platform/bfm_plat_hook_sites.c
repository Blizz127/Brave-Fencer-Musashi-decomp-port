/* Guest entry points used by the host event system.
 * The addresses and overlay filters select a hook. The fill callback reads
 * the guest arguments and state needed for its public event payload. */
#include "bfm_plat_events.h"
#include "bfm_plat_mods.h"
#include "bfm_plat_widescreen.h"

#include <stdlib.h>
#include <string.h>

static int read_u16(uint32_t a, uint32_t *out) {
    uint8_t b[2];
    int r = bfm_plat_guest_read(a, b, 2);
    if (r == BFM_PLAT_OK) *out = (uint32_t)b[0] | ((uint32_t)b[1] << 8);
    return r;
}

static int read_u32(uint32_t a, uint32_t *out) {
    uint8_t b[4];
    int r = bfm_plat_guest_read(a, b, 4);
    if (r == BFM_PLAT_OK)
        *out = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) |
               ((uint32_t)b[3] << 24);
    return r;
}

/* Build the room payload using the active overlay and current guest location. */
static void overlay_room(BfmEventRoom *room) {
    const char *o = bfm_plat_hooks_overlay();
    const char *us;
    uint32_t loc;
    room->area = room->room = 0;
    room->location = read_u16(BFM_GUEST_LOCATION, &loc) == BFM_PLAT_OK ? (int16_t)(uint16_t)loc : -1;
    if (!o || (o[0] != 'S' && o[0] != 's') || (o[1] != 'C' && o[1] != 'c')) return;
    room->area = (uint32_t)strtoul(o + 2, NULL, 10);
    us = strchr(o, '_');
    if (us) room->room = (uint32_t)strtoul(us + 1, NULL, 10);
}

/* Previous samples used to detect entry into a room or a card operation. */
static uint32_t last_phase = 0xFFFFFFFFu;
static char last_overlay[32];
static uint32_t last_card_state_save = 0xFFFFFFFFu;
static uint32_t last_card_state_load = 0xFFFFFFFFu;

/* Emit once when an overlay enters phase zero. */
static int fill_room_enter(void *u, BfmEvent e, BfmPlatEventPayload *p) {
    uint32_t phase;
    const char *o = bfm_plat_hooks_overlay();
    int changed;
    (void)u; (void)e;
    if (read_u16(BFM_GUEST_SCENE_PHASE, &phase) != BFM_PLAT_OK) return 1;
    changed = strcmp(last_overlay, o ? o : "") != 0;
    if (phase == 0 && (last_phase != 0 || changed)) {
        last_phase = phase;
        strncpy(last_overlay, o ? o : "", sizeof last_overlay - 1);
        overlay_room(&p->room);
        return 0;
    }
    last_phase = phase;
    strncpy(last_overlay, o ? o : "", sizeof last_overlay - 1);
    return 1;
}

/* State 8 signals a possible exit while a scene overlay is active. */
static int fill_room_exit(void *u, BfmEvent e, BfmPlatEventPayload *p) {
    uint32_t a0;
    (void)u; (void)e;
    if (bfm_plat_guest_reg(BFM_GUEST_REG_A0, &a0) != BFM_PLAT_OK) return 1;
    if ((a0 & 0xFFFFu) != 8u) return 1;
    if (!bfm_plat_overlay_matches("SC*", bfm_plat_hooks_overlay())) return 1;
    overlay_room(&p->room);
    last_phase = 0xFFFFFFFFu;
    return 0;
}

static int card_edge(uint32_t *last, int (*want)(uint32_t), BfmPlatEventPayload *p) {
    uint32_t st, a1;
    if (read_u32(BFM_GUEST_CARD_STATE, &st) != BFM_PLAT_OK) return 1;
    if (!want(st) || st == *last) {
        *last = st;
        return 1;
    }
    *last = st;
    if (bfm_plat_guest_reg(BFM_GUEST_REG_A1, &a1) != BFM_PLAT_OK) a1 = 0;
    p->save.slot = a1;
    return 0;
}

static int is_save_state(uint32_t s) { return s == 13u; }
static int is_load_state(uint32_t s) { return s == 17u || s == 32u; }

static int fill_save(void *u, BfmEvent e, BfmPlatEventPayload *p) {
    (void)u; (void)e;
    return card_edge(&last_card_state_save, is_save_state, p);
}

static int fill_load(void *u, BfmEvent e, BfmPlatEventPayload *p) {
    (void)u; (void)e;
    return card_edge(&last_card_state_load, is_load_state, p);
}

/* H is the unsigned low half of a0. */
static int fill_geom_screen(void *u, BfmEvent e, BfmPlatEventPayload *p) {
    uint32_t a0;
    BfmPlatProjection cur;
    (void)u; (void)e;
    if (bfm_plat_guest_reg(BFM_GUEST_REG_A0, &a0) != BFM_PLAT_OK) return 1;
    bfm_plat_projection_get(&cur);
    bfm_plat_projection_record((int32_t)(a0 & 0xFFFFu), cur.ofx, cur.ofy);
    p->projection.h = (int32_t)(a0 & 0xFFFFu);
    p->projection.ofx = cur.ofx;
    p->projection.ofy = cur.ofy;
    return 0;
}

/* X and Y arrive as signed halfwords. */
static int fill_geom_offset(void *u, BfmEvent e, BfmPlatEventPayload *p) {
    uint32_t a0, a1;
    BfmPlatProjection cur;
    (void)u; (void)e;
    if (bfm_plat_guest_reg(BFM_GUEST_REG_A0, &a0) != BFM_PLAT_OK ||
        bfm_plat_guest_reg(BFM_GUEST_REG_A1, &a1) != BFM_PLAT_OK)
        return 1;
    bfm_plat_projection_get(&cur);
    bfm_plat_projection_record(cur.h, (int16_t)(a0 & 0xFFFFu), (int16_t)(a1 & 0xFFFFu));
    p->projection.h = cur.h;
    p->projection.ofx = (int16_t)(a0 & 0xFFFFu);
    p->projection.ofy = (int16_t)(a1 & 0xFFFFu);
    return 0;
}

/* These overlays use the two gauge update entry points below. */
static const char gauge_overlays[] =
    "MAIN_012,SC01_000,SC01_001,SC01_004,SC01_005,SC01_006,SC01_008,"
    "SC01_009,SC01_074,SC01_077,SC01_080,SC01_084,SC02_000,SC02_003,"
    "SC02_004,SC02_005,SC02_011,SC02_015,SC02_016,SC02_017,SC02_021,"
    "SC02_026,SC02_027,SC02_028,SC02_031,SC02_035,SC02_037,SC02_039,"
    "SC02_041,SC03_001,SC03_002,SC03_003,SC03_006,SC03_007,SC03_010,"
    "SC03_011,SC03_012,SC03_013,SC03_014,SC03_015,SC03_023,SC03_024,"
    "SC03_028,SC03_029,SC03_030,SC03_031,SC03_089,SC03_090,SC03_091,"
    "SC03_092,SC03_093,SC03_094,SC03_095,SC03_096,SC03_097,SC03_098,"
    "SC03_099,SC03_100,SC03_101,SC03_102,SC03_103,SC03_104,SC03_105,"
    "SC03_107,SC03_108,SC03_109,SC03_110,SC03_111,SC03_112,SC03_113,"
    "SC03_114,SC03_115,SC03_116,SC03_117,SC03_118,SC03_119,SC03_121,"
    "SC03_124,SC03_125,SC03_126,SC04_000,SC04_002,SC04_003,SC04_004,"
    "SC04_005,SC04_006,SC04_007,SC04_008,SC04_009,SC04_010,SC04_011,"
    "SC04_012,SC04_015,SC04_016,SC04_018,SC04_019,SC04_020,SC04_021,"
    "SC05_000,SC05_001,SC05_002,SC05_003,SC05_004,SC05_005,SC05_006,"
    "SC05_007,SC05_008,SC05_009,SC05_010,SC05_011,SC05_017,SC05_018,"
    "SC05_019,SC06_000,SC06_006,SC06_008,SC06_010,SC06_011,SC06_013,"
    "SC06_014,SC06_015,SC06_016,SC06_018,SC06_020,SC06_022,SC06_024,"
    "SC06_025,SC06_027,SC06_029,SC06_030,SC06_032,SC06_033,SC07_000,"
    "SC07_001,SC07_002,SC07_006,SC07_007,SC07_008,SC07_009,SC07_010,"
    "SC07_011";

static int read_s16(uint32_t a, int32_t *out) {
    uint32_t v;
    int r = read_u16(a, &v);
    if (r == BFM_PLAT_OK) *out = (int16_t)(uint16_t)v;
    return r;
}

/* The event reports the value predicted at function entry. */
static int fill_damage(void *u, BfmEvent e, BfmPlatEventPayload *p) {
    uint32_t a1, max = 0;   /* The limit is optional when the guest read fails. */
    int32_t hp;
    (void)u;
    if (bfm_plat_guest_reg(BFM_GUEST_REG_A1, &a1) != BFM_PLAT_OK ||
        read_s16(e == BFM_EVENT_DAMAGE ? BFM_GUEST_HP : BFM_GUEST_BP, &hp) != BFM_PLAT_OK)
        return 1;
    read_u16(e == BFM_EVENT_DAMAGE ? BFM_GUEST_HP_MAX : BFM_GUEST_BP_MAX, &max);
    p->stat.previous = (uint16_t)hp;
    p->stat.value = (int32_t)a1 > (uint16_t)hp ? 0 : (uint16_t)hp - (int32_t)a1;
    p->stat.max = (int32_t)max;
    p->stat.flags = 0;
    bfm_plat_events_mark_code_site(e);
    return 0;
}

/* A nonzero item assignment reports one acquisition. */
static int fill_item_add(void *u, BfmEvent e, BfmPlatEventPayload *p) {
    uint32_t a0, a1;
    (void)u; (void)e;
    if (bfm_plat_guest_reg(BFM_GUEST_REG_A0, &a0) != BFM_PLAT_OK ||
        bfm_plat_guest_reg(BFM_GUEST_REG_A1, &a1) != BFM_PLAT_OK)
        return 1;
    if ((a1 & 0xFFu) == 0) return 1;             /* Clearing a slot emits no acquisition. */
    p->item.item = a1 & 0xFFu;
    p->item.count = 1;
    p->item.kind = BFM_ITEM_KIND_INVENTORY;
    p->item.flags = 0;
    bfm_plat_events_mark_code_site(BFM_EVENT_ITEM_GET);
    return 0;
}

/* The first argument supplies the inventory item number. */
static int fill_item_use(void *u, BfmEvent e, BfmPlatEventPayload *p) {
    uint32_t a0;
    (void)u; (void)e;
    if (bfm_plat_guest_reg(BFM_GUEST_REG_A0, &a0) != BFM_PLAT_OK) return 1;
    p->item.item = a0 & 0xFFu;
    p->item.count = 1;
    p->item.kind = BFM_ITEM_KIND_INVENTORY;
    p->item.flags = 0;
    return 0;
}

static const BfmPlatKnownSite known[] = {
    {0x800189A8u, NULL, BFM_EVENT_FRAME_BEGIN, "func_800189A8",
     "The host opens a frame at entry to the resident update routine. See "
     "src/main/800189a8.c.",
     "high: frame update entry", NULL},
    {0x800184F0u, NULL, BFM_EVENT_FRAME_END, "func_800184F0",
     "The host closes a frame at the resident end-of-frame routine. See "
     "src/main/800184f0.c.",
     "high: end-of-frame call", NULL},
    {0x80128158u, "SC*", BFM_EVENT_ROOM_ENTER, "func_80128158",
     "The SC overlay dispatcher reads the scene phase. The hook emits on entry to phase "
     "zero and when the overlay changes at that phase.",
     "medium: phase zero can occur outside a room transition", fill_room_enter},
    {0x80011B7Cu, NULL, BFM_EVENT_ROOM_EXIT, "func_80011B7C",
     "The resident state setter takes the requested state in a0. The hook accepts state "
     "8 while an SC overlay is active. See src/main/80011b7c.c.",
     "low: changing state alone does not confirm a completed room exit", fill_room_exit},
    {0x8002B0B4u, NULL, BFM_EVENT_SAVE, "func_8002B0B4",
     "The card dispatcher reads the current operation. The hook emits once on entry to "
     "state 13 and takes the slot from a1 when available.",
     "medium: the hook observes the start of an operation", fill_save},
    {0x8002B0B4u, NULL, BFM_EVENT_LOAD, "func_8002B0B4",
     "The card dispatcher also handles reads. The hook emits on entry to state 17 or 32 "
     "and takes the slot from a1 when available.",
     "medium: the hook observes the start of an operation", fill_load},
    {0x8004923Cu, NULL, BFM_EVENT_PROJECTION, "func_8004923C",
     "The projection hook takes screen distance from the low half of a0 and retains the "
     "current offsets. See tests/projection_source_probe.c.",
     "high: projection distance register", fill_geom_screen},
    {0x8004921Cu, NULL, BFM_EVENT_PROJECTION, "func_8004921C",
     "The projection hook reads horizontal and vertical offsets from the signed low "
     "halves of a0 and a1. See tests/projection_source_probe.c.",
     "high: projection offset registers", fill_geom_offset},
    {0x8014BC80u, gauge_overlays, BFM_EVENT_DAMAGE, "func_8014BC80",
     "In the listed overlays this hook reads the current HP and limit in guest memory, "
     "then uses a1 to compute the reported change.",
     "high: covers the listed overlay routine; other HP changes can be detected by the "
     "frame observer", fill_damage},
    {0x8014BD60u, gauge_overlays, BFM_EVENT_BP_USE, "func_8014BD60",
     "In the listed overlays the BP hook reads the current BP and limit and applies the "
     "delta in a1 to the event payload.",
     "high", fill_damage},
    {0x800D0F0Cu, NULL, BFM_EVENT_ITEM_GET, "func_800D0F0C",
     "The item assignment entry takes the item in the low byte of a1. Zero clears a slot "
     "and suppresses the item event. See src/overlays/main_0010/800d0f0c.c.",
     "high: an assignment reports one item and suppresses a duplicate frame event", fill_item_add},
    {0x800D128Cu, NULL, BFM_EVENT_ITEM_USE, "func_800D128C",
     "The inventory-use hook reports one inventory item, identified by the low byte of "
     "a0. See tests/bfm_plat_probe.c.",
     "high", fill_item_use},
};

static const BfmEvent unsited[] = {
    BFM_EVENT_BATTLE_START, BFM_EVENT_BATTLE_END
};

size_t bfm_plat_known_sites(const BfmPlatKnownSite **out) {
    if (out) *out = known;
    return sizeof known / sizeof known[0];
}

size_t bfm_plat_unsited_events(const BfmEvent **out) {
    if (out) *out = unsited;
    return sizeof unsited / sizeof unsited[0];
}

int bfm_plat_hooks_register_known(void) {
    BfmPlatHookSite s[sizeof known / sizeof known[0]];
    size_t i;
    last_phase = 0xFFFFFFFFu;
    last_overlay[0] = '\0';
    last_card_state_save = last_card_state_load = 0xFFFFFFFFu;
    memset(s, 0, sizeof s);
    for (i = 0; i < sizeof known / sizeof known[0]; i++) {
        s[i].guest_pc = known[i].guest_pc;
        s[i].event = known[i].event;
        s[i].label = known[i].symbol;
        s[i].fill = known[i].fill;
        s[i].overlay = known[i].overlay;
    }
    return bfm_plat_hooks_register(s, i);
}
