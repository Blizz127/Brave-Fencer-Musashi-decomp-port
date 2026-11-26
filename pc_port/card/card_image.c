#include "card_image.h"

#include <string.h>

enum {
    FRAME_SIZE = 128, FRAMES_PER_BLOCK = 64, FRAME_COUNT = 1024,
    DEFECT_COUNT = 20, DEFECT_TABLE = 16, REPLACEMENTS = 36,
    HEAD = 0x51, MIDDLE = 0x52, TAIL = 0x53,
    FREE_BLOCK = 0xa0, DELETED_HEAD = 0xa1, DELETED_TAIL = 0xa3,
    END_LINK = 0xffff
};

typedef struct {
    uint16_t defects[DEFECT_COUNT];
    uint8_t *records[CARD_MAX_FILES];
} Directory;

static uint16_t get16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)get16(p) | ((uint32_t)get16(p + 2) << 16);
}

static void put16(uint8_t *p, uint16_t n)
{
    p[0] = (uint8_t)n;
    p[1] = (uint8_t)(n >> 8);
}

static void put32(uint8_t *p, uint32_t n)
{
    put16(p, (uint16_t)n);
    put16(p + 2, (uint16_t)(n >> 16));
}

static uint8_t checksum(const uint8_t *frame)
{
    unsigned i;
    uint8_t value = 0;
    for (i = 0; i < FRAME_SIZE - 1; ++i)
        value ^= frame[i];
    return value;
}

static void seal_frame(uint8_t *frame)
{
    frame[FRAME_SIZE - 1] = checksum(frame);
}

static int sealed(const uint8_t *frame)
{
    return frame[FRAME_SIZE - 1] == checksum(frame);
}

static uint8_t *physical_frame(CardImage image, unsigned sector)
{
    return image.bytes + sector * FRAME_SIZE;
}

static uint8_t *logical_frame(CardImage image, const Directory *dir, unsigned sector)
{
    unsigned i;
    for (i = 0; i < DEFECT_COUNT; ++i) {
        if (dir->defects[i] == sector)
            return physical_frame(image, REPLACEMENTS + i);
    }
    return physical_frame(image, sector);
}

static int load_defects(CardImage image, Directory *dir)
{
    unsigned i;
    for (i = 0; i < DEFECT_COUNT; ++i) {
        const uint8_t *record = physical_frame(image, DEFECT_TABLE + i);
        uint32_t sector = get32(record);
        unsigned earlier;
        if (!sealed(record))
            return 0;
        if (sector == UINT32_MAX) {
            dir->defects[i] = END_LINK;
            continue;
        }
        if (sector >= FRAME_COUNT ||
            (sector >= DEFECT_TABLE && sector < REPLACEMENTS + DEFECT_COUNT))
            return 0;
        for (earlier = 0; earlier < i; ++earlier) {
            if (dir->defects[earlier] == sector)
                return 0;
        }
        dir->defects[i] = (uint16_t)sector;
    }
    return 1;
}

static CardResult load_mapping(CardImage image, Directory *dir)
{
    if (!image.bytes)
        return CARD_ARGUMENT;
    if (image.length != CARD_IMAGE_SIZE || !load_defects(image, dir))
        return CARD_INVALID;
    return CARD_OK;
}

static int has_signature(const uint8_t *frame)
{
    return frame[0] == 0x4d && frame[1] == 0x43;
}

int card_is_blank(CardImage image)
{
    size_t i;
    if (!image.bytes || image.length != CARD_IMAGE_SIZE)
        return 0;
    for (i = 0; i < image.length; ++i) {
        if (image.bytes[i] != 0)
            return 0;
    }
    return 1;
}

CardResult card_header_signature(CardImage image, int *recognized)
{
    Directory dir;
    CardResult result;
    if (!recognized)
        return CARD_ARGUMENT;
    result = load_mapping(image, &dir);
    if (result == CARD_OK) {
        *recognized = has_signature(logical_frame(image, &dir, 0));
    } else if (result == CARD_INVALID && card_is_blank(image)) {
        *recognized = 0;
        return CARD_OK;
    }
    return result;
}

static int valid_name(const char *name)
{
    size_t i;
    if (!name || !name[0])
        return 0;
    for (i = 1; i <= CARD_MAX_NAME; ++i) {
        if (!name[i])
            return 1;
    }
    return 0;
}

static const char *record_name(const uint8_t *record)
{
    return (const char *)(record + 10);
}

static int is_free(unsigned state)
{
    return state >= FREE_BLOCK && state <= DELETED_TAIL;
}

static unsigned chain_state(unsigned position, unsigned count)
{
    if (position == 0)
        return HEAD;
    return position + 1u == count ? TAIL : MIDDLE;
}

static int claim_chain(const Directory *dir, unsigned head,
                       uint8_t claimed[CARD_MAX_FILES])
{
    const uint8_t *first = dir->records[head];
    uint32_t size = get32(first + 4);
    unsigned blocks = (unsigned)(size / CARD_BLOCK_SIZE);
    unsigned block = head;
    unsigned i;

    if (size == 0 || size > CARD_BLOCK_SIZE * CARD_MAX_FILES ||
        size % CARD_BLOCK_SIZE != 0 || !valid_name(record_name(first)))
        return 0;
    for (i = 0; i < head; ++i) {
        const uint8_t *other = dir->records[i];
        if (other[0] == HEAD && strcmp(record_name(other), record_name(first)) == 0)
            return 0;
    }
    for (i = 0; i < blocks; ++i) {
        unsigned next;
        if (block >= CARD_MAX_FILES || claimed[block] ||
            dir->records[block][0] != chain_state(i, blocks))
            return 0;
        claimed[block] = 1;
        next = get16(dir->records[block] + 8);
        if (i + 1u == blocks && next != END_LINK)
            return 0;
        block = next;
    }
    return 1;
}

static CardResult load_directory(CardImage image, Directory *dir)
{
    uint8_t claimed[CARD_MAX_FILES] = {0};
    const uint8_t *header;
    unsigned i;
    CardResult result = load_mapping(image, dir);
    if (result != CARD_OK)
        return result;
    header = logical_frame(image, dir, 0);
    if (!has_signature(header) || !sealed(header))
        return CARD_INVALID;
    for (i = 0; i < CARD_MAX_FILES; ++i) {
        uint8_t *record = logical_frame(image, dir, i + 1u);
        uint32_t state = get32(record);
        if (!sealed(record) ||
            (state != HEAD && state != MIDDLE && state != TAIL &&
             !(state >= FREE_BLOCK && state <= DELETED_TAIL)))
            return CARD_INVALID;
        dir->records[i] = record;
    }
    for (i = 0; i < CARD_MAX_FILES; ++i) {
        if (dir->records[i][0] == HEAD && !claim_chain(dir, i, claimed))
            return CARD_INVALID;
    }
    for (i = 0; i < CARD_MAX_FILES; ++i) {
        if (!is_free(dir->records[i][0]) && !claimed[i])
            return CARD_INVALID;
    }
    return CARD_OK;
}

CardResult card_validate(CardImage image)
{
    Directory dir;
    return load_directory(image, &dir);
}

int card_is_unallocated(CardImage image)
{
    Directory dir;
    if (load_directory(image, &dir) != CARD_OK) return 0;
    for (unsigned i = 0; i < CARD_MAX_FILES; ++i) {
        const uint8_t *record = dir.records[i];
        if (record[0] != FREE_BLOCK || get32(record + 4) != 0 ||
            get16(record + 8) != END_LINK ||
            memcmp(record + 10, (const uint8_t[CARD_MAX_NAME]){0}, CARD_MAX_NAME))
            return 0;
    }
    return 1;
}

CardResult card_format(CardImage image)
{
    uint8_t *header = image.bytes;
    unsigned i;
    if (!header || image.length != CARD_IMAGE_SIZE)
        return CARD_ARGUMENT;
    memset(image.bytes, 0, image.length);
    header[0] = 0x4d;
    header[1] = 0x43;
    seal_frame(header);
    for (i = 1; i <= CARD_MAX_FILES; ++i) {
        uint8_t *record = physical_frame(image, i);
        record[0] = FREE_BLOCK;
        put16(record + 8, END_LINK);
        seal_frame(record);
    }
    for (i = DEFECT_TABLE; i < REPLACEMENTS; ++i) {
        uint8_t *record = physical_frame(image, i);
        put32(record, UINT32_MAX);
        seal_frame(record);
    }
    memset(physical_frame(image, REPLACEMENTS), 0xff,
           (63u - REPLACEMENTS) * FRAME_SIZE);
    memcpy(physical_frame(image, 63), header, FRAME_SIZE);
    return CARD_OK;
}

static CardResult locate(CardImage image, const char *name, Directory *dir,
                         unsigned *head)
{
    CardResult result;
    unsigned i;
    if (!valid_name(name))
        return CARD_ARGUMENT;
    result = load_directory(image, dir);
    if (result != CARD_OK)
        return result;
    for (i = 0; i < CARD_MAX_FILES; ++i) {
        const uint8_t *record = dir->records[i];
        if (record[0] == HEAD && strcmp(record_name(record), name) == 0) {
            *head = i;
            return CARD_OK;
        }
    }
    return CARD_NOT_FOUND;
}

static void describe(const Directory *dir, unsigned head, CardEntry *entry)
{
    const uint8_t *record = dir->records[head];
    memset(entry, 0, sizeof(*entry));
    memcpy(entry->name, record + 10, strlen(record_name(record)));
    entry->size = get32(record + 4);
    entry->first_sector = (head + 1u) * FRAMES_PER_BLOCK;
}

CardResult card_list(CardImage image, CardEntry entries[CARD_MAX_FILES], size_t *count)
{
    Directory dir;
    CardResult result;
    size_t used = 0;
    unsigned i;
    if (!entries || !count)
        return CARD_ARGUMENT;
    result = load_directory(image, &dir);
    if (result != CARD_OK)
        return result;
    for (i = 0; i < CARD_MAX_FILES; ++i) {
        if (dir.records[i][0] == HEAD)
            describe(&dir, i, &entries[used++]);
    }
    *count = used;
    return CARD_OK;
}

CardResult card_open(CardImage image, const char *name, CardEntry *entry)
{
    Directory dir;
    unsigned head;
    CardResult result;
    if (!entry)
        return CARD_ARGUMENT;
    result = locate(image, name, &dir, &head);
    if (result == CARD_OK)
        describe(&dir, head, entry);
    return result;
}

CardResult card_create(CardImage image, const char *name, size_t blocks)
{
    Directory dir;
    unsigned chosen[CARD_MAX_FILES];
    unsigned count = 0;
    unsigned head, i;
    CardResult result;
    if (blocks == 0 || blocks > CARD_MAX_FILES)
        return CARD_RANGE;
    result = locate(image, name, &dir, &head);
    if (result == CARD_OK)
        return CARD_EXISTS;
    if (result != CARD_NOT_FOUND)
        return result;
    for (i = 0; i < CARD_MAX_FILES && count < blocks; ++i) {
        if (is_free(dir.records[i][0]))
            chosen[count++] = i;
    }
    if (count < blocks)
        return CARD_NO_SPACE;
    for (i = 0; i < count; ++i) {
        unsigned slot = chosen[i];
        uint8_t *record = dir.records[slot];
        unsigned sector;
        put32(record, chain_state(i, count));
        put32(record + 4, i == 0 ? count * CARD_BLOCK_SIZE : 0);
        put16(record + 8, i + 1u == count ? END_LINK : (uint16_t)chosen[i + 1u]);
        memset(record + 10, 0, CARD_MAX_NAME + 1u);
        if (i == 0)
            memcpy(record + 10, name, strlen(name));
        seal_frame(record);
        for (sector = 0; sector < FRAMES_PER_BLOCK; ++sector)
            memset(logical_frame(image, &dir, (slot + 1u) * FRAMES_PER_BLOCK + sector),
                   0, FRAME_SIZE);
    }
    return CARD_OK;
}

static void transfer(CardImage image, const Directory *dir, unsigned block,
                     size_t offset, size_t length, uint8_t *dst, const uint8_t *src)
{
    if (!length)
        return;
    while (offset >= CARD_BLOCK_SIZE) {
        block = get16(dir->records[block] + 8);
        offset -= CARD_BLOCK_SIZE;
    }
    while (length) {
        unsigned sector = (block + 1u) * FRAMES_PER_BLOCK + (unsigned)(offset / FRAME_SIZE);
        size_t within = offset % FRAME_SIZE;
        size_t chunk = FRAME_SIZE - within;
        uint8_t *p = logical_frame(image, dir, sector) + within;
        if (length < chunk)
            chunk = length;
        if (dst) {
            memcpy(dst, p, chunk);
            dst += chunk;
        } else {
            memcpy(p, src, chunk);
            src += chunk;
        }
        length -= chunk;
        offset += chunk;
        if (offset == CARD_BLOCK_SIZE && length) {
            block = get16(dir->records[block] + 8);
            offset = 0;
        }
    }
}

CardResult card_read(CardImage image, const char *name, size_t offset,
                     void *data, size_t length, size_t *actual)
{
    Directory dir;
    unsigned head;
    size_t size;
    CardResult result;
    if (!actual || (length && !data))
        return CARD_ARGUMENT;
    result = locate(image, name, &dir, &head);
    if (result != CARD_OK)
        return result;
    size = get32(dir.records[head] + 4);
    if (offset > size)
        return CARD_RANGE;
    if (length > size - offset)
        length = size - offset;
    transfer(image, &dir, head, offset, length, data, NULL);
    *actual = length;
    return CARD_OK;
}

CardResult card_write(CardImage image, const char *name, size_t offset,
                      const void *data, size_t length)
{
    Directory dir;
    unsigned head;
    size_t size;
    CardResult result;
    if (length && !data)
        return CARD_ARGUMENT;
    result = locate(image, name, &dir, &head);
    if (result != CARD_OK)
        return result;
    size = get32(dir.records[head] + 4);
    if (offset > size || length > size - offset)
        return CARD_RANGE;
    transfer(image, &dir, head, offset, length, NULL, data);
    return CARD_OK;
}

CardResult card_delete(CardImage image, const char *name)
{
    Directory dir;
    unsigned block;
    CardResult result = locate(image, name, &dir, &block);
    if (result != CARD_OK)
        return result;
    while (block != END_LINK) {
        uint8_t *record = dir.records[block];
        record[0] = (uint8_t)(DELETED_HEAD + (record[0] - HEAD));
        seal_frame(record);
        block = get16(record + 8);
    }
    return CARD_OK;
}
