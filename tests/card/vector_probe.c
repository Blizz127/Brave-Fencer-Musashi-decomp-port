#include "card_vector.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { GUEST_SIZE = 4096, PATH = 17, DATA = 271, DIR = 605 };
static const uint32_t prior_v0 = UINT32_C(0xe738a64d);
static const uint32_t failed = UINT32_MAX;

#define CHECK(condition) do {                                                 \
    if (!(condition)) {                                                      \
        fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);       \
        exit(EXIT_FAILURE);                                                  \
    }                                                                        \
} while (0)

typedef struct {
    uint8_t guest[GUEST_SIZE];
    uint8_t image[2][CS_IMAGE_BYTES];
    int mounted[2];
    unsigned commits;
    unsigned events;
    uint32_t event_class[8];
    uint32_t event_spec[8];
} Host;

static uint8_t *map(void *context, uint32_t address, size_t count, int writable)
{
    Host *h = context;
    (void)writable;
    if (address == 0 || address >= GUEST_SIZE || count > GUEST_SIZE - address) {
        return NULL;
    }
    return h->guest + address;
}

static CsMedia snapshot(void *context, unsigned slot, uint8_t *dst)
{
    Host *h = context;
    CHECK(slot < 2);
    if (!h->mounted[slot]) {
        return CS_MEDIA_MISSING;
    }
    memcpy(dst, h->image[slot], CS_IMAGE_BYTES);
    return CS_MEDIA_OK;
}

static int commit(void *context, unsigned slot, const uint8_t *old, const uint8_t *src)
{
    Host *h = context;
    CHECK(slot < 2);
    CHECK(old ? h->mounted[slot] && !memcmp(old, h->image[slot], CS_IMAGE_BYTES)
              : !h->mounted[slot]);
    h->mounted[slot] = 1;
    memcpy(h->image[slot], src, CS_IMAGE_BYTES);
    ++h->commits;
    return 1;
}

static int notice(void *context, uint32_t kind, uint32_t spec)
{
    Host *h = context;
    CHECK(h->events < 8);
    h->event_class[h->events] = kind;
    h->event_spec[h->events] = spec;
    ++h->events;
    return 1;
}

static uint32_t path(Host *h, const char *name)
{
    CHECK(strlen(name) + 1 < DATA - PATH);
    memcpy(h->guest + PATH, name, strlen(name) + 1);
    return PATH;
}

enum { A = 0xa0, B = 0xb0 };
static CsVectorResult call(CsVector *v, uint32_t vec, uint32_t sel, uint32_t a0, uint32_t a1, uint32_t a2)
{
    return cs_vector_dispatch(v, vec, sel, a0, a1, a2, UINT32_C(0xfedcba98));
}
static void pass(CsVector *v, uint32_t vec, uint32_t sel, uint32_t a0, uint32_t a1)
{
    CsVectorResult r = call(v, vec, sel, a0, a1, DATA);
    if (r.recognized || r.write_v0 || r.file_error) {
        fprintf(stderr, "unexpected route %02"PRIx32"/%02"PRIx32" a0=%"PRIx32"\n", vec, sel, a0);
        exit(EXIT_FAILURE);
    }
}
static uint32_t value(CsVector *v, uint32_t vec, uint32_t sel, uint32_t a0, uint32_t a1, uint32_t a2)
{
    CsVectorResult r = call(v, vec, sel, a0, a1, a2);
    if (!r.recognized || !r.write_v0) {
        fprintf(stderr, "missing result %02"PRIx32"/%02"PRIx32" a0=%"PRIx32"\n", vec, sel, a0);
        exit(EXIT_FAILURE);
    }
    return r.v0;
}
static void void_call(CsVector *v, uint32_t vec, uint32_t sel, uint32_t a0)
{
    uint32_t register_v0 = prior_v0;
    CsVectorResult r = call(v, vec, sel, a0, 0, 0);
    CHECK(r.recognized && !r.file_error);
    if (r.write_v0) {
        register_v0 = r.v0;
    }
    CHECK(!r.write_v0 && register_v0 == prior_v0);
}
static void path_routing(CsVector *v, Host *h)
{
    const char *other[] = { "cdrom:\\FILE.DAT;1", "tty:", "buxx:X", "bufile:X",
        "bu12z:X", "BU00:X", "b", "bu001234567:X", "bu00", "bu00/X" };
    for (size_t i = 0; i < sizeof(other) / sizeof(*other); ++i) {
        uint32_t name = path(h, other[i]);
        pass(v, A, 0, name, 1);
        pass(v, B, 0x32, name, 1);
        pass(v, B, 0x41, name, 0);
        pass(v, B, 0x42, name, DIR);
        pass(v, B, 0x45, name, 0);
    }
    pass(v, A, 0, UINT32_MAX, 1);
    pass(v, B, 0x32, 0, 1);
    memcpy(h->guest + GUEST_SIZE - 4, "bu00", 4);
    pass(v, A, 0, GUEST_SIZE - 4, 1);
    const char *ports[] = { "bu:", "bu42:X", "buFF:X", "bu12345678:X" };
    for (size_t i = 0; i < sizeof(ports) / sizeof(*ports); ++i) {
        CsVectorResult r = call(v, A, 0, path(h, ports[i]), 1, 0);
        CHECK(r.recognized && r.write_v0 && r.v0 == failed && r.file_error);
    }
    memcpy(h->guest + GUEST_SIZE - 5, "bu00:", 5);
    CsVectorResult r = call(v, B, 0x32, GUEST_SIZE - 5, 1, 0);
    CHECK(r.recognized && r.write_v0 && r.v0 == failed && r.file_error);
    pass(v, B, 0x54, 0, 0);
}

static void file_aliases(CsVector *v, Host *h)
{
    CHECK(cs_reserve_fd(v->service, 2, 1));
    pass(v, A, 1, 2, 0);
    pass(v, B, 0x34, 2, DATA);
    pass(v, B, 0x55, 2, 0);
    uint32_t fd = value(v, B, 0x32, path(h, "bu00:ONE"), 0x10203, 0);
    CHECK(fd == 3);
    for (unsigned i = 0; i < CS_SECTOR_BYTES; ++i) {
        h->guest[DATA + i] = (uint8_t)(i * 47 + 11);
    }
    CHECK(value(v, A, 3, fd, DATA, CS_SECTOR_BYTES) == CS_SECTOR_BYTES);
    CHECK(value(v, B, 0x33, fd, (uint32_t)-(int)CS_SECTOR_BYTES, 1) == 0);
    memset(h->guest + DATA - 1, 0x6f, CS_SECTOR_BYTES + 2);
    CHECK(value(v, B, 0x34, fd, DATA, CS_SECTOR_BYTES) == CS_SECTOR_BYTES);
    for (unsigned i = 0; i < CS_SECTOR_BYTES; ++i) {
        CHECK(h->guest[DATA + i] == (uint8_t)(i * 47 + 11));
    }
    CHECK(h->guest[DATA - 1] == 0x6f && h->guest[DATA + CS_SECTOR_BYTES] == 0x6f);
    CsVectorResult bad = call(v, A, 2, fd, DATA, 1);
    CHECK(bad.recognized && bad.write_v0 && bad.v0 == failed && bad.file_error);
    CHECK(value(v, B, 0x55, fd, 0, 0) == (uint32_t)bad.file_error);
    pass(v, B, 0x54, fd, 0);
    void_call(v, B, 0x4c, 0);
    void_call(v, B, 0x4b, 0);
    CHECK(value(v, A, 4, fd, 0, 0) == fd);
    pass(v, B, 0x55, fd, 0);
    pass(v, B, 0x36, fd, 0);
    pass(v, A, 2, fd, DATA);
    CHECK(cs_reserve_fd(v->service, 2, 0));
    fd = value(v, A, 0, path(h, "bu00:TWO"), 0x10203, 0);
    CHECK(fd == 2);
    CHECK(value(v, B, 0x35, fd, DATA, CS_SECTOR_BYTES) == CS_SECTOR_BYTES);
    CHECK(value(v, A, 1, fd, 0, 0) == 0);
    CHECK(value(v, A, 2, fd, DATA, CS_SECTOR_BYTES) == CS_SECTOR_BYTES);
    CHECK(value(v, B, 0x36, fd, 0, 0) == fd);
}

static void searches(CsVector *v, Host *h)
{
    pass(v, B, 0x43, DIR, 0);
    CHECK(value(v, B, 0x42, path(h, "bu00:*"), DIR, 0) == DIR);
    CHECK(value(v, B, 0x43, DIR, 0, 0) == DIR);
    CHECK(value(v, B, 0x43, DIR, 0, 0) == 0);
    CHECK(value(v, B, 0x42, path(h, "bu00:*"), DIR, 0) == DIR);
    pass(v, B, 0x42, path(h, "cdrom:\\*"), DIR);
    pass(v, B, 0x43, DIR, 0);
    CHECK(value(v, B, 0x42, path(h, "bu00:*"), DIR, 0) == DIR);
    CHECK(value(v, B, 0x42, path(h, "bu00:*"), GUEST_SIZE - 1, 0) == 0);
    CHECK(value(v, B, 0x43, DIR, 0, 0) == 0);
    CHECK(value(v, B, 0x42, path(h, "bu00:NO-MATCH"), DIR, 0) == 0);
    CHECK(value(v, B, 0x43, DIR, 0, 0) == 0);
    CHECK(value(v, B, 0x45, path(h, "bu00:TWO"), 0, 0) == 1);
    CHECK(value(v, B, 0x41, path(h, "bu00:"), 0, 0) == 1);
    CHECK(value(v, B, 0x42, path(h, "bu00:*"), DIR, 0) == 0);
}

static void drain(CsVector *v)
{
    for (unsigned i = 0; i < 16 && !cs_idle(v->service); ++i) {
        cs_pump(v->service);
    }
    CHECK(cs_idle(v->service));
}

static void raw_channels(CsVector *v, Host *h)
{
    CHECK(cs_prepare(v->service, 1) == CS_MEDIA_OK);
    h->events = 0;
    CHECK(value(v, A, 0xab, 0x10, 0, 0) == 1);
    CHECK(value(v, B, 0x5c, 1, 0, 0) == 8 && h->events == 0);
    drain(v);
    CHECK(h->events == 2 && h->event_class[0] == CS_EVENT_HW &&
          h->event_class[1] == CS_EVENT_SW && h->event_spec[0] == CS_DONE &&
          h->event_spec[1] == CS_DONE);
    CHECK(value(v, B, 0x58, 0, 0, 0) == 0x10);
    h->events = 0;
    CHECK(value(v, B, 0x4d, 0x10, 0, 0) == 1);
    CHECK(value(v, B, 0x5d, 1, 0, 0) == 1);
    drain(v);
    CHECK(h->events == 1 && h->event_class[0] == CS_EVENT_HW && h->event_spec[0] == CS_DONE);
    h->events = 0;
    memset(h->guest + DATA, 0x2e, CS_SECTOR_BYTES);
    unsigned saved = h->commits;
    CHECK(value(v, B, 0x4e, 0x10, 1023, DATA) == 1);
    CHECK(value(v, B, 0x5c, 1, 0, 0) == 4 && h->events == 0 && h->commits == saved);
    drain(v);
    CHECK(h->commits == saved + 1 && h->events == 1 &&
          h->event_class[0] == CS_EVENT_HW && h->event_spec[0] == CS_DONE);
    memset(h->guest + DATA - 1, 0x91, CS_SECTOR_BYTES + 2);
    CHECK(value(v, B, 0x4f, 0x10, 1023, DATA) == 1 && h->guest[DATA + 3] == 0x91);
    CHECK(value(v, B, 0x5c, 1, 0, 0) == 2);
    drain(v);
    for (unsigned i = 0; i < CS_SECTOR_BYTES; ++i) {
        CHECK(h->guest[DATA + i] == 0x2e);
    }
    CHECK(h->guest[DATA - 1] == 0x91 && h->guest[DATA + CS_SECTOR_BYTES] == 0x91);
    CHECK(value(v, B, 0x4f, 0x10, 1024, DATA) == 0 && cs_idle(v->service));
    CHECK(value(v, B, 0x4e, 0x10, 1, GUEST_SIZE - 2) == 0 && cs_idle(v->service));
    h->events = 0;
    CHECK(value(v, A, 0xac, 0x10, 0, 0) == 1);
    drain(v);
    CHECK(h->events == 2 && h->event_class[1] == CS_EVENT_SW);
    CHECK(value(v, A, 0xaf, 0x10, 0, 0) == 1);
    drain(v);
}

static void init_alias(CsHost callbacks, Host *h, uint32_t selector)
{
    CardService *s = cs_create(callbacks);
    CHECK(s);
    CsVector v;
    cs_vector_init(&v, s, callbacks);
    CHECK(cs_prepare(s, 0) == CS_MEDIA_OK);
    uint32_t name = path(h, "bu00:INIT");
    CHECK(value(&v, A, 0, name, 0x10203, 0) == failed);
    void_call(&v, B, 0x4a, 1);
    void_call(&v, B, 0x4b, 0);
    void_call(&v, A, selector, 0);
    uint32_t fd = value(&v, A, 0, name, 0x10203, 0);
    CHECK(fd >= 2 && fd < 16);
    CHECK(value(&v, B, 0x36, fd, 0, 0) == fd);
    CHECK(value(&v, B, 0x45, name, 0, 0) == 1);
    void_call(&v, B, 0x4c, 0);
    CHECK(value(&v, B, 0x4f, 0, 1, DATA) == 0);
    CHECK(cs_idle(s));
    cs_destroy(s);
}

int main(void)
{
    Host *h = calloc(1, sizeof(*h));
    CHECK(h);
    CsHost callbacks = { .context = h, .map = map, .snapshot = snapshot,
                         .commit = commit, .event = notice };
    init_alias(callbacks, h, 0x55);
    init_alias(callbacks, h, 0x70);
    CardService *s = cs_create(callbacks);
    CHECK(s);
    CsVector v;
    cs_vector_init(&v, s, callbacks);
    CHECK(cs_prepare(s, 0) == CS_MEDIA_OK);
    pass(&v, 0xc0, 0x32, path(h, "bu00:ONE"), 1);
    pass(&v, 0x1a0, 0, PATH, 1);
    pass(&v, A, 0x100, PATH, 1);
    pass(&v, A, 0x54, 0, 0);
    pass(&v, B, 0x99, 0, 0);
    pass(&v, B, 0x44, PATH, PATH);
    void_call(&v, B, 0x4a, 1);
    void_call(&v, B, 0x4b, 0);
    void_call(&v, A, 0x55, 0);
    void_call(&v, A, 0x70, 0);
    void_call(&v, B, 0x50, 0);
    path_routing(&v, h);
    file_aliases(&v, h);
    searches(&v, h);
    raw_channels(&v, h);
    void_call(&v, B, 0x4c, 0);
    cs_destroy(s);
    free(h);
    puts("PASS vector ABI, fallthrough, descriptor and search ownership, aliases, and asynchronous hardware");
    return EXIT_SUCCESS;
}
