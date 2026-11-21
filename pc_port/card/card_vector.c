#include "card_vector.h"
#include <limits.h>

enum { OTHER_SEARCH, CARD_SEARCH, EMPTY_CARD_SEARCH };

static const CsVectorResult pass = {0, 0, 0, 0};
static const CsVectorResult handled_void = {1, 0, 0, 0};

static CsVectorResult value(uint32_t word) {
    CsVectorResult r = {1, 1, word, 0};
    return r;
}

static CsVectorResult file_value(CsVector *v, int32_t word, int failed) {
    CsVectorResult r = value((uint32_t)word);
    if (failed) r.file_error = cs_errno(v->service);
    return r;
}

void cs_vector_init(CsVector *v, CardService *service, CsHost host) {
    CsVector initial = {service, host.context, host.map, 0, 0};
    *v = initial;
}

static int card_path(CsVector *v, uint32_t address) {
    unsigned i;
    if (!address) return 0;
    for (i = 0; i <= 10; ++i) {
        const uint8_t *p;
        uint8_t c;
        if (i > UINT32_MAX - address) return 0;
        p = v->map(v->context, address + i, 1, 0);
        if (!p) return 0;
        c = *p;
        if (i < 2) {
            if (c != (i == 0 ? 'b' : 'u')) return 0;
        } else {
            if (c == ':') return 1;
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
                  (c >= 'a' && c <= 'f'))) return 0;
        }
    }
    return 0;
}

static int32_t signed_word(uint32_t word) {
    if (word <= INT32_MAX) return (int32_t)word;
    return -1 - (int32_t)(UINT32_MAX - word);
}

static int owns_fd(const CsVector *v, uint32_t fd) {
    return fd < 16 && (v->file_handles & (1u << fd)) != 0;
}

/* The public A and B vectors alias five guest file API operations. */
static CsVectorResult file_call(CsVector *v, uint32_t op, uint32_t a0,
                                uint32_t a1, uint32_t a2) {
    CardService *s = v->service;
    int32_t result, fd;
    if (op == 0) {
        if (!card_path(v, a0)) return pass;
        result = cs_open(s, a0, a1);
        if (result >= 2 && result < 16)
            v->file_handles |= (uint16_t)(1u << (unsigned)result);
        return file_value(v, result, result < 0);
    }
    if (!owns_fd(v, a0)) return pass;
    fd = (int32_t)a0;
    switch (op) {
    case 1: result = cs_seek(s, fd, signed_word(a1), a2); break;
    case 2: result = cs_read(s, fd, a1, a2); break;
    case 3: result = cs_write(s, fd, a1, a2); break;
    case 4:
        result = cs_close(s, fd);
        if (result >= 0) v->file_handles &= (uint16_t)~(1u << a0);
        break;
    default: return pass;
    }
    return file_value(v, result, result < 0);
}

CsVectorResult cs_vector_dispatch(CsVector *v, uint32_t vector, uint32_t selector,
                                  uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3) {
    CardService *s;
    int result;
    (void)a3;
    if (!v || !v->service || !v->map) return pass;
    s = v->service;
    if (vector == 0xa0) {
        if (selector <= 4) return file_call(v, selector, a0, a1, a2);
        switch (selector) {
        case 0x55:
        case 0x70: cs_init_files(s); return handled_void;
        case 0xab: return value((uint32_t)cs_card_info(s, a0));
        case 0xac: return value((uint32_t)cs_card_load(s, a0));
        case 0xaf: return value((uint32_t)cs_write_test(s, a0));
        default: return pass;
        }
    }
    if (vector != 0xb0) return pass;
    if (selector >= 0x32 && selector <= 0x36)
        return file_call(v, selector - 0x32, a0, a1, a2);
    switch (selector) {
    case 0x41:
    case 0x45:
        if (!card_path(v, a0)) return pass;
        result = selector == 0x41 ? cs_format(s, a0) : cs_erase(s, a0);
        return file_value(v, result, result == 0);
    case 0x42: {
        uint32_t dirent;
        v->owns_search = card_path(v, a0) ? CARD_SEARCH : OTHER_SEARCH;
        if (!v->owns_search) return pass;
        dirent = cs_first(s, a0, a1);
        if (!dirent) v->owns_search = EMPTY_CARD_SEARCH;
        return value(dirent);
    }
    case 0x43:
        if (!v->owns_search) return pass;
        if (v->owns_search == EMPTY_CARD_SEARCH) return value(0);
        return value(cs_next(s, a0));
    case 0x4a: cs_init_card(s); return handled_void;
    case 0x4b: cs_start_card(s); return handled_void;
    case 0x4c: cs_stop_card(s); return handled_void;
    case 0x4d: return value((uint32_t)cs_card_info_sub(s, a0));
    case 0x4e: return value((uint32_t)cs_sector_write(s, a0, a1, a2));
    case 0x4f: return value((uint32_t)cs_sector_read(s, a0, a1, a2));
    case 0x50: cs_new_card(s); return handled_void;
    case 0x55:
        if (!owns_fd(v, a0)) return pass;
        return value((uint32_t)cs_fd_error(s, (int32_t)a0));
    case 0x58: return value(cs_card_chan(s));
    case 0x5c: return value((uint32_t)cs_card_status(s, a0));
    case 0x5d: return value((uint32_t)cs_card_wait(s, a0));
    default: return pass;
    }
}
