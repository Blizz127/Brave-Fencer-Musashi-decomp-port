/* Synthetic console contract. The only data here are test inputs. */
#include "musashi_boot_memory.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>

static MusashiBootMemory memory;
static const uint32_t table = 0x800728a9u;
static const uint32_t column_address = 0x80078978u;
static const uint32_t text = 0x80010100u;

typedef struct Output {
    uint8_t data[32];
    uint32_t columns[32];
    unsigned count;
} Output;

static void record(void *userdata, const uint8_t *bytes, int32_t count) {
    Output *out = userdata;
    assert(count == 1 && out->count < sizeof(out->data));
    out->data[out->count] = *bytes;
    assert(musashi_boot_read32(&memory, column_address, &out->columns[out->count]));
    ++out->count;
}

int main(void) {
    Output out = {0};
    uint8_t *classes = musashi_boot_ram_span(&memory, table, 256);
    uint32_t column;
    const uint32_t wanted[] = {UINT32_MAX, 0, 1, 0, 1};
    assert(classes);
    classes['A'] = 0x10;
    classes[' '] = 0x80;
    classes['\r'] = 0x04;
    classes[0x91] = 0x02;
    memcpy(musashi_boot_ram_span(&memory, text, 5), "A\t\n\x91", 5);
    assert(musashi_boot_write32(&memory, column_address, UINT32_MAX - 1u));
    assert(musashi_boot_console_string(&memory, text, record, &out));
    assert(out.count == 5 && !memcmp(out.data, "A \r\n\x91", 5));
    assert(!memcmp(out.columns, wanted, sizeof(wanted)));

    /* Changing the classification makes an unaligned tab unable to advance. */
    classes[' '] = 0;
    assert(musashi_boot_write32(&memory, column_address, 0));
    out.count = 0;
    assert(!musashi_boot_console_string(&memory, text, record, &out));
    assert(out.count == 0 && musashi_boot_read32(&memory, column_address, &column) && !column);
    assert(musashi_boot_console_char(&memory, 0x109, record, &out));
    assert(out.count == 1 && out.data[0] == ' ' && !out.columns[0]);

    assert(!musashi_boot_console_string(&memory, 0, record, &out));
    assert(!musashi_boot_console_char(&memory, 'A', NULL, &out));
    memory.bytes[MUSASHI_RAM_SIZE - 1] = 1;
    assert(!musashi_boot_console_string(&memory, 0x801fffffu, record, &out));
    assert(out.count == 1);
    return 0;
}
