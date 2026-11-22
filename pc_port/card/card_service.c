#include "card_service.h"
#include "card_image.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(CARD_IMAGE_SIZE == CS_IMAGE_BYTES, "image size mismatch");

enum { SLOT_COUNT = 2, FD_FIRST = 2, FD_LIMIT = 16 };
enum { NO_FILE = 0x02, BAD_FD = 0x09, IO_ERROR = 0x10, EXISTS = 0x11,
       BAD_DEVICE = 0x13, BAD_RANGE = 0x16, NO_FD = 0x18, NO_SPACE = 0x1c };
typedef enum { NOTHING, FILE_READ, FILE_WRITE, RAW_READ, RAW_WRITE,
               INFO, INFO_SUB, LOAD, WRITE_TEST } Operation;
typedef struct {
    int used, stale, literal_path, error;
    uint32_t mode;
    uint64_t open_id;
    unsigned slot;
    uint32_t position, size, first_sector;
    char name[CARD_MAX_NAME + 1];
} Descriptor;
typedef struct {
    Operation op;
    uint32_t address, count;
    unsigned sector;
    int fd, swapped, bypass_change;
} Request;
typedef struct { uint32_t group, spec; } Event;
typedef struct {
    Request request;
    Event events[2];
    uint8_t listed[CS_DIRENT_BYTES];
    unsigned first_event, event_count;
    int status, changed, write_error;
} Slot;
typedef struct {
    unsigned slot;
    int canonical_port;
    char name[CARD_MAX_NAME + 1];
} CardPath;
struct CardService {
    CsHost host;
    CsAsyncReadObserver read_observer;
    int initialized, started, files, error, ignore_next_change, pumping;
    unsigned last_port;
    uint64_t last_open_id;
    uint64_t completed_open_id;
    CsCompletedRead completed_read;
    Slot slots[SLOT_COUNT];
    Descriptor descriptors[FD_LIMIT];
    uint8_t reserved[FD_LIMIT];
    struct { int active; CardPath path; uint32_t after_sector; } search;
    uint8_t original[CS_IMAGE_BYTES], scratch[CS_IMAGE_BYTES];
    int missing;
};

static CardImage image_of(CardService *s) {
    CardImage image = { s->scratch, sizeof s->scratch };
    return image;
}

CardService *cs_create(CsHost host) {
    return cs_create_with_read_observer(host, NULL);
}

CardService *cs_create_with_read_observer(CsHost host, CsAsyncReadObserver observer) {
    CardService *s;
    if (!host.map || !host.snapshot || !host.commit || !host.event) return NULL;
    s = calloc(1, sizeof *s);
    if (s) {
        unsigned slot;
        s->host = host;
        s->read_observer = observer;
        for (slot = 0; slot < SLOT_COUNT; ++slot) s->slots[slot].status = 1;
    }
    return s;
}

void cs_destroy(CardService *s) { free(s); }

static CsMedia load_image(CardService *s, unsigned slot) {
    CsMedia m = s->host.snapshot(s->host.context, slot, s->original);
    s->missing = m == CS_MEDIA_MISSING;
    if (s->missing)
        return card_format(image_of(s)) == CARD_OK ? CS_MEDIA_OK : CS_MEDIA_ERROR;
    if (m == CS_MEDIA_OK) memcpy(s->scratch, s->original, sizeof s->scratch);
    return m;
}

static int save_image(CardService *s, unsigned slot) {
    return s->host.commit(s->host.context, slot,
                          s->missing ? NULL : s->original, s->scratch);
}

CsMedia cs_prepare(CardService *s, unsigned slot) {
    CsMedia m;
    if (!s || slot >= SLOT_COUNT) return CS_MEDIA_ERROR;
    m = load_image(s, slot);
    if (m != CS_MEDIA_OK) return m;
    if (card_validate(image_of(s)) != CARD_OK)
        return card_is_blank(image_of(s)) ? CS_MEDIA_UNFORMATTED : CS_MEDIA_ERROR;
    if (s->missing && !save_image(s, slot)) return CS_MEDIA_ERROR;
    return CS_MEDIA_OK;
}

void cs_init_card(CardService *s) { s->initialized = 1; }
void cs_start_card(CardService *s) { s->started = s->initialized; }
void cs_stop_card(CardService *s) { s->started = 0; }
int cs_init_files(CardService *s) {
    s->files = s->started;
    return s->files;
}

static int busy(const Slot *slot) {
    return slot->request.op != NOTHING || slot->event_count != 0;
}

static void invalidate(CardService *s, unsigned slot, const char *name) {
    int fd;
    memset(s->slots[slot].listed, 0, sizeof s->slots[slot].listed);
    for (fd = FD_FIRST; fd < FD_LIMIT; ++fd) {
        Descriptor *d = &s->descriptors[fd];
        if (d->used && d->slot == slot && (!name || strcmp(d->name, name) == 0)) {
            d->stale = 1;
            if (d->open_id == s->completed_open_id) s->completed_open_id = 0;
        }
    }
}

void cs_media_changed(CardService *s, unsigned slot) {
    if (slot >= SLOT_COUNT) return;
    invalidate(s, slot, NULL);
    if (s->search.active && s->search.path.slot == slot) s->search.active = 0;
    s->slots[slot].changed = 1;
    s->slots[slot].write_error = 0;
    s->slots[slot].request.swapped = 1;
}

static uint8_t *guest(CardService *s, uint32_t addr, uint32_t n, int writing) {
    if (!addr || !n || n - 1u > UINT32_MAX - addr) return NULL;
    return s->host.map(s->host.context, addr, n, writing);
}

static int path_from_guest(CardService *s, uint32_t addr, CardPath *path) {
    char buf[2 + 8 + 1 + CARD_MAX_NAME + 1];
    size_t n, i = 2;
    uint32_t port = 0;
    for (n = 0; n < sizeof buf; ++n) {
        uint8_t *p;
        if (n > UINT32_MAX - addr || !(p = guest(s, addr + (uint32_t)n, 1, 0)))
            return BAD_RANGE;
        buf[n] = (char)*p;
        if (!buf[n]) break;
    }
    if (n == sizeof buf || n < 3 || buf[0] != 'b' || buf[1] != 'u')
        return BAD_DEVICE;
    while (i < n && buf[i] != ':') {
        unsigned char c = (unsigned char)buf[i];
        unsigned digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10u;
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10u;
        else return BAD_DEVICE;
        if (i - 2 >= 8) return BAD_DEVICE;
        port = (port << 4) | digit;
        ++i;
    }
    if (i >= n || (port != 0 && port != 0x10)) return BAD_DEVICE;
    ++i;
    if (n - i > CARD_MAX_NAME) return BAD_RANGE;
    path->slot = port >> 4;
    path->canonical_port = i == 5 && buf[2] == (char)('0' + path->slot) && buf[3] == '0';
    memcpy(path->name, buf + i, n - i + 1);
    return 0;
}

int cs_reserve_fd(CardService *s, int32_t fd, int reserved) {
    if (fd < FD_FIRST || fd >= FD_LIMIT || s->descriptors[fd].used) return 0;
    s->reserved[fd] = reserved != 0;
    return 1;
}

static int room_for_fd(const CardService *s) {
    int fd;
    for (fd = FD_FIRST; fd < FD_LIMIT; ++fd)
        if (!s->descriptors[fd].used && !s->reserved[fd]) return fd;
    return -1;
}

static int matches(const char *pattern, const char *name, int glob) {
    for (;;) {
        char p = *pattern++;
        if (glob && p == '*') return 1;
        if (p != '?' && p != *name) return 0;
        if (!*name) return 1;
        ++name;
    }
}

static CardResult find_entry(CardService *s, const char *name, CardEntry *entry) {
    CardEntry list[CARD_MAX_FILES];
    size_t i, count;
    CardResult result;
    if (!strchr(name, '?')) return card_open(image_of(s), name, entry);
    result = card_list(image_of(s), list, &count);
    if (result != CARD_OK) return result;
    for (i = 0; i < count; ++i) {
        if (matches(name, list[i].name, 0)) {
            *entry = list[i];
            return CARD_OK;
        }
    }
    return CARD_NOT_FOUND;
}

static int code_for(CardResult r) {
    switch (r) {
    case CARD_NOT_FOUND: return NO_FILE;
    case CARD_EXISTS: return EXISTS;
    case CARD_NO_SPACE: return NO_SPACE;
    case CARD_ARGUMENT:
    case CARD_RANGE: return BAD_RANGE;
    default: return IO_ERROR;
    }
}

static int fail(CardService *s, Descriptor *d, int error) {
    s->error = error;
    if (d) d->error = error;
    return -1;
}

static Descriptor *descriptor(CardService *s, int32_t fd) {
    if (fd >= FD_FIRST && fd < FD_LIMIT && s->descriptors[fd].used)
        return &s->descriptors[fd];
    s->error = BAD_FD;
    return NULL;
}

static int available(CardService *s, unsigned slot) {
    return s->started && s->files && !busy(&s->slots[slot]);
}

int32_t cs_open(CardService *s, uint32_t path_address, uint32_t mode) {
    CardPath path;
    CardEntry entry;
    CardResult r;
    Descriptor *d;
    int err = path_from_guest(s, path_address, &path);
    int fd = room_for_fd(s);
    if (err) return fail(s, NULL, err);
    if (!mode || !path.name[0]) return fail(s, NULL, BAD_RANGE);
    if (fd < 0) return fail(s, NULL, NO_FD);
    if (!available(s, path.slot) || load_image(s, path.slot) != CS_MEDIA_OK)
        return fail(s, NULL, IO_ERROR);
    r = find_entry(s, path.name, &entry);
    if (mode & 0x200u) {
        if (r == CARD_OK) return fail(s, NULL, EXISTS);
        if (r != CARD_NOT_FOUND) return fail(s, NULL, code_for(r));
        if (strpbrk(path.name, "*?")) return fail(s, NULL, BAD_RANGE);
        r = card_create(image_of(s), path.name, mode >> 16);
        if (r == CARD_OK) r = card_open(image_of(s), path.name, &entry);
        if (r == CARD_OK && !save_image(s, path.slot))
            return fail(s, NULL, IO_ERROR);
    }
    if (r != CARD_OK) return fail(s, NULL, code_for(r));
    d = &s->descriptors[fd];
    memset(d, 0, sizeof *d);
    d->used = 1;
    d->slot = path.slot;
    d->size = entry.size;
    d->first_sector = entry.first_sector;
    d->mode = mode;
    d->literal_path = path.canonical_port && strcmp(path.name, entry.name) == 0;
    /* On exhaustion ordinary opens still work. Never reissue an ID. */
    if (s->last_open_id != UINT64_MAX) d->open_id = ++s->last_open_id;
    memcpy(d->name, entry.name, sizeof d->name);
    s->slots[path.slot].changed = 0;
    return fd;
}

static void word_at(uint8_t *bytes, unsigned at, uint32_t value) {
    unsigned i;
    for (i = 0; i != 4; ++i) bytes[at + i] = (uint8_t)(value >> (8u * i));
}

static void make_dirent(uint8_t row[CS_DIRENT_BYTES], const char *name, uint32_t size,
                        uint32_t first_sector) {
    memset(row, 0, CS_DIRENT_BYTES);
    memcpy(row, name, strlen(name));
    word_at(row, 0x14, 0x50);
    word_at(row, 0x18, size);
    word_at(row, 0x20, first_sector);
}

uint64_t cs_async_read_descriptor(const CardService *s, const void *host_owner,
                                  unsigned slot, int32_t fd, const char *name,
                                  const uint8_t row[CS_DIRENT_BYTES], uint32_t file_size,
                                  uint32_t position, uint32_t count, const CsCompletedRead *completed) {
    if (!s || !host_owner || s->host.context != host_owner || !name || !name[0] || !row ||
        slot >= SLOT_COUNT || fd < FD_FIRST || fd >= FD_LIMIT ||
        !s->started || !s->files || s->pumping || !cs_idle(s))
        return 0;
    const Descriptor *d = &s->descriptors[fd];
    const Slot *media = &s->slots[slot];
    if (!d->used || d->stale || d->error || d->slot != slot || d->mode != 0x8001u ||
        !d->literal_path || strcmp(d->name, name) || d->size != file_size || media->changed ||
        media->write_error || media->status != 1 || d->position != position ||
        (position | count) % CS_SECTOR_BYTES || position > d->size ||
        count > d->size - position)
        return 0;
    if (completed && (s->completed_open_id != d->open_id || !s->completed_open_id ||
        s->completed_read.position != completed->position ||
        s->completed_read.destination != completed->destination ||
        s->completed_read.count != completed->count)) return 0;
    uint8_t expected[CS_DIRENT_BYTES];
    make_dirent(expected, d->name, d->size, d->first_sector);
    if (memcmp(expected, row, sizeof expected) || memcmp(row, media->listed, sizeof expected))
        return 0;
    return d->open_id;
}

int32_t cs_seek(CardService *s, int32_t fd, int32_t displacement, unsigned origin) {
    int64_t p = displacement;
    Descriptor *d = descriptor(s, fd);
    if (!d) return -1;
    if (d->stale || !available(s, d->slot)) return fail(s, d, IO_ERROR);
    if (origin == 1) p += d->position;
    else if (origin != 0) return fail(s, d, BAD_RANGE);
    if (p < 0 || p > INT32_MAX) return fail(s, d, BAD_RANGE);
    s->completed_open_id = 0;
    d->position = (uint32_t)p;
    return (int32_t)p;
}

int32_t cs_close(CardService *s, int32_t fd) {
    Descriptor *d = descriptor(s, fd);
    if (!d) return -1;
    if (busy(&s->slots[d->slot]) && s->slots[d->slot].request.fd == fd)
        return fail(s, d, IO_ERROR);
    if (d->open_id == s->completed_open_id) s->completed_open_id = 0;
    memset(d, 0, sizeof *d);
    return fd;
}

int cs_errno(const CardService *s) { return s->error; }
int cs_fd_error(CardService *s, int32_t fd) {
    Descriptor *d = descriptor(s, fd);
    return d ? d->error : -1;
}

static uint32_t media_spec(CsMedia m) {
    return m == CS_MEDIA_ABSENT ? CS_TIMEOUT : CS_ERROR;
}

/* Set position and expose read data only after all checks have completed. */
static uint32_t file_transfer(CardService *s, Descriptor *d, uint32_t addr,
                              uint32_t count, int writing) {
    CardResult r;
    size_t actual = 0;
    uint8_t *span = guest(s, addr, count, !writing);
    CsMedia m;
    /* Another queued operation may have completed since this one was submitted. */
    s->completed_open_id = 0;
    if (!span) return CS_ERROR;
    m = load_image(s, d->slot);
    if (m != CS_MEDIA_OK) return media_spec(m);
    if (s->slots[d->slot].write_error) {
        s->slots[d->slot].write_error = 0;
        return CS_ERROR;
    }
    if (writing) {
        r = card_write(image_of(s), d->name, d->position, span, count);
        if (r != CARD_OK || !save_image(s, d->slot)) return CS_ERROR;
    } else {
        /* card_read can return a short read on a changed image. Do not
           expose such bytes to the guest unless the whole transfer passed. */
        uint8_t *out = s->original;
        r = card_read(image_of(s), d->name, d->position, out, count, &actual);
        if (r != CARD_OK || actual != count) return CS_ERROR;
        memcpy(span, out, actual);
        if (d->mode & 0x8000u) {
            s->completed_read = (CsCompletedRead){d->position, addr, count};
            s->completed_open_id = d->open_id;
            if (s->read_observer)
                s->read_observer(s->host.context, d->slot, (int32_t)(d - s->descriptors),
                                 d->open_id, &s->completed_read, out);
        }
    }
    d->position += count;
    return CS_DONE;
}

static void queue(CardService *s, unsigned slot, Request request, int status) {
    s->slots[slot].request = request;
    s->slots[slot].status = status;
}

static int32_t transfer(CardService *s, int32_t fd, uint32_t addr,
                        uint32_t count, int writing) {
    Descriptor *d = descriptor(s, fd);
    Request request = {0};
    uint32_t remain;
    if (!d) return -1;
    if (d->stale || !available(s, d->slot)) return fail(s, d, IO_ERROR);
    if (!count || ((d->position | count) % CS_SECTOR_BYTES) ||
        d->position >= d->size || !addr || count - 1u > UINT32_MAX - addr)
        return fail(s, d, BAD_RANGE);
    remain = d->size - d->position;
    if (writing && count > remain) return fail(s, d, BAD_RANGE);
    if (count > remain) count = remain;
    if (!guest(s, addr, count, !writing)) return fail(s, d, BAD_RANGE);
    s->completed_open_id = 0;
    if (!(d->mode & 0x8000u)) {
        if (file_transfer(s, d, addr, count, writing) != CS_DONE)
            return fail(s, d, IO_ERROR);
        return (int32_t)count;
    }
    request.op = writing ? FILE_WRITE : FILE_READ;
    request.fd = fd;
    request.address = addr;
    request.count = count;
    queue(s, d->slot, request, writing ? 4 : 2);
    return 0;
}

int32_t cs_read(CardService *s, int32_t fd, uint32_t addr, uint32_t count) {
    return transfer(s, fd, addr, count, 0);
}
int32_t cs_write(CardService *s, int32_t fd, uint32_t addr, uint32_t count) {
    return transfer(s, fd, addr, count, 1);
}


uint32_t cs_next(CardService *s, uint32_t addr) {
    CardEntry entries[CARD_MAX_FILES];
    const CardEntry *next = NULL;
    CardPath *p = &s->search.path;
    uint8_t *out;
    uint8_t dirent[CS_DIRENT_BYTES];
    size_t i, count;
    if (!s->search.active || !available(s, p->slot)) return 0;
    if (!(out = guest(s, addr, sizeof dirent, 1))) return 0;
    if (load_image(s, p->slot) != CS_MEDIA_OK ||
        card_list(image_of(s), entries, &count) != CARD_OK) return 0;
    for (i = 0; i < count; ++i) {
        const CardEntry *e = &entries[i];
        if (e->first_sector > s->search.after_sector &&
            (!next || e->first_sector < next->first_sector) &&
            matches(p->name, e->name, 1)) next = e;
    }
    if (!next) {
        s->search.active = 0;
        return 0;
    }
    make_dirent(dirent, next->name, next->size, next->first_sector);
    memcpy(s->slots[p->slot].listed, dirent, sizeof dirent);
    memcpy(out, dirent, sizeof dirent);
    s->search.after_sector = next->first_sector;
    return addr;
}

uint32_t cs_first(CardService *s, uint32_t pattern, uint32_t addr) {
    CardPath path;
    if (path_from_guest(s, pattern, &path) || !available(s, path.slot) ||
        room_for_fd(s) < 0 || !guest(s, addr, CS_DIRENT_BYTES, 1)) return 0;
    /* A new listing supersedes this slot's previous result even if it is empty. */
    memset(s->slots[path.slot].listed, 0, sizeof s->slots[path.slot].listed);
    s->search.path = path;
    s->search.after_sector = 0;
    s->search.active = 1;
    return cs_next(s, addr);
}

static int mutate_directory(CardService *s, uint32_t addr, int format) {
    CardPath path;
    CardEntry entry;
    CardResult r;
    int err = path_from_guest(s, addr, &path);
    if (!err && room_for_fd(s) < 0) err = NO_FD;
    if (!err && (!available(s, path.slot) || load_image(s, path.slot) != CS_MEDIA_OK))
        err = IO_ERROR;
    if (!err && (format ? path.name[0] != 0 : path.name[0] == 0)) err = BAD_RANGE;
    if (err) { fail(s, NULL, err); return 0; }
    if (format) {
        r = card_format(image_of(s));
    } else {
        r = find_entry(s, path.name, &entry);
        if (r == CARD_OK) r = card_delete(image_of(s), entry.name);
    }
    if (r != CARD_OK) { fail(s, NULL, code_for(r)); return 0; }
    if (!save_image(s, path.slot)) { fail(s, NULL, IO_ERROR); return 0; }
    invalidate(s, path.slot, format ? NULL : entry.name);
    if (format && s->search.active && s->search.path.slot == path.slot)
        s->search.active = 0;
    s->slots[path.slot].changed = 0;
    return 1;
}

int cs_erase(CardService *s, uint32_t addr) { return mutate_directory(s, addr, 0); }
int cs_format(CardService *s, uint32_t addr) { return mutate_directory(s, addr, 1); }

static int submit_card(CardService *s, unsigned port, Operation op,
                       unsigned sector, uint32_t addr) {
    unsigned index;
    Request r = {0};
    int status = 8;
    if (!s->started || (port != 0 && port != 0x10)) return 0;
    index = port >> 4;
    if (busy(&s->slots[index])) return 0;
    if (op == RAW_READ || op == RAW_WRITE) {
        if (sector >= CS_IMAGE_BYTES / CS_SECTOR_BYTES ||
            !guest(s, addr, CS_SECTOR_BYTES, op == RAW_READ)) return 0;
        s->completed_open_id = 0;
        status = op == RAW_READ ? 2 : 4;
        r.bypass_change = s->ignore_next_change;
        s->ignore_next_change = 0;
    }
    if (op == LOAD) status = 2;
    if (op == WRITE_TEST) status = 4;
    r.op = op;
    r.sector = sector;
    r.address = addr;
    r.fd = -1;
    queue(s, index, r, status);
    return 1;
}

int cs_sector_read(CardService *s, unsigned p, unsigned n, uint32_t a) {
    return submit_card(s, p, RAW_READ, n, a);
}
int cs_sector_write(CardService *s, unsigned p, unsigned n, uint32_t a) {
    return submit_card(s, p, RAW_WRITE, n, a);
}
int cs_card_info(CardService *s, unsigned p) {
    return submit_card(s, p, INFO, 0, 0);
}
int cs_card_info_sub(CardService *s, unsigned p) {
    return submit_card(s, p, INFO_SUB, 0, 0);
}
int cs_card_load(CardService *s, unsigned p) {
    return submit_card(s, p, LOAD, 0, 0);
}
int cs_write_test(CardService *s, unsigned p) {
    return submit_card(s, p, WRITE_TEST, 0, 0);
}
void cs_new_card(CardService *s) { s->ignore_next_change = 1; }

static void add_event(Slot *slot, uint32_t group, uint32_t spec) {
    Event e = {group, spec};
    slot->events[slot->event_count++] = e;
}

static uint32_t raw_transfer(CardService *s, unsigned index, const Request *r) {
    Slot *slot = &s->slots[index];
    const Operation op = r->op;
    uint8_t *medium, *ram = NULL;
    CsMedia m;
    if (op == RAW_READ || op == RAW_WRITE) {
        s->completed_open_id = 0;
        ram = guest(s, r->address, CS_SECTOR_BYTES, op == RAW_READ);
        if (!ram) return CS_ERROR;
    }
    m = load_image(s, index);
    if (m != CS_MEDIA_OK) return media_spec(m);
    if (slot->write_error) {
        slot->write_error = 0;
        return CS_PREVIOUS_WRITE;
    }
    if (slot->changed && !r->bypass_change && op != LOAD && op != WRITE_TEST)
        return CS_CHANGED;
    switch (op) {
    case INFO:
    case INFO_SUB:
        return CS_DONE;
    case LOAD:
        /* A readable physical response and a usable directory are distinct. */
        return CS_DONE;
    case WRITE_TEST:
        /* Acknowledge insertion without changing reserved sector contents. */
        if (!save_image(s, index)) return CS_ERROR;
        slot->changed = 0;
        return CS_DONE;
    case RAW_READ:
    case RAW_WRITE:
        medium = s->scratch + (size_t)r->sector * CS_SECTOR_BYTES;
        if (op == RAW_READ) {
            memcpy(ram, medium, CS_SECTOR_BYTES);
        } else {
            memcpy(medium, ram, CS_SECTOR_BYTES);
            if (save_image(s, index)) {
                slot->changed = 0;
                if (r->sector < CARD_BLOCK_SIZE / CS_SECTOR_BYTES)
                    invalidate(s, index, NULL);
            } else {
                slot->write_error = 1;
            }
        }
        return CS_DONE;
    default:
        return CS_ERROR;
    }
}

static void finish(CardService *s, unsigned index) {
    Slot *slot = &s->slots[index];
    Request *r = &slot->request;
    uint32_t result;
    if (r->op == FILE_READ || r->op == FILE_WRITE) {
        Descriptor *d = &s->descriptors[r->fd];
        result = r->swapped ? CS_CHANGED :
                 file_transfer(s, d, r->address, r->count, r->op == FILE_WRITE);
        if (result == CS_DONE) add_event(slot, (uint32_t)r->fd, CS_DONE);
        else fail(s, d, IO_ERROR);
        add_event(slot, CS_EVENT_SW, result);
    } else {
        result = r->swapped ? CS_CHANGED : raw_transfer(s, index, r);
        add_event(slot, CS_EVENT_HW, result);
        if (r->op == INFO || r->op == LOAD) {
            if (result == CS_PREVIOUS_WRITE) result = CS_ERROR;
            if (r->op == LOAD && result == CS_DONE) {
                int recognized;
                CardImage image = image_of(s);
                if (card_header_signature(image, &recognized) != CARD_OK)
                    result = CS_ERROR;
                else if (!recognized) result = CS_CHANGED;
                else if (card_validate(image) != CARD_OK) result = CS_ERROR;
                else slot->changed = 0;
            }
            add_event(slot, CS_EVENT_SW, result);
        }
    }
    slot->status = result == CS_DONE ? 1 : result == CS_TIMEOUT ? 0x11 : 0x21;
    s->last_port = index << 4;
    r->op = NOTHING;
}

unsigned cs_pump(CardService *s) {
    unsigned i, progress = 0;
    if (s->pumping) return 0;
    s->pumping = 1;
    for (i = 0; i < SLOT_COUNT; ++i) {
        Slot *slot = &s->slots[i];
        if (slot->request.op != NOTHING) {
            finish(s, i);
            progress |= 1;
        }
        while (slot->first_event < slot->event_count) {
            Event e = slot->events[slot->first_event];
            s->last_port = i << 4;
            if (!s->host.event(s->host.context, e.group, e.spec)) {
                progress |= 4;
                break;
            }
            ++slot->first_event;
            progress |= 2;
        }
        if (slot->first_event == slot->event_count) {
            slot->first_event = 0;
            slot->event_count = 0;
        }
    }
    s->pumping = 0;
    return progress;
}

int cs_card_status(const CardService *s, unsigned slot) {
    return slot < SLOT_COUNT ? s->slots[slot].status : 0x21;
}
int cs_card_wait(CardService *s, unsigned slot) {
    if (slot < SLOT_COUNT && s->slots[slot].request.op != NOTHING) cs_pump(s);
    return cs_card_status(s, slot);
}
unsigned cs_card_chan(const CardService *s) { return s->last_port; }
int cs_idle(const CardService *s) {
    return !busy(&s->slots[0]) && !busy(&s->slots[1]);
}
