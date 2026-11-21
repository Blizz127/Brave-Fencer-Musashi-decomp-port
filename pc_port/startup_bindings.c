#include "musashi_boot_memory.h"
#include <string.h>


/* These declarations bind project-owned gameplay memory routines. */
extern void func_80016714_native(uint8_t *destination, int32_t count);
extern void func_80029044_native(uint8_t *destination);
extern void func_8002906C_native(uint8_t *destination);

int musashi_boot_search_bytes(MusashiBootMemory *memory, uint32_t address,
                              int32_t target, int32_t count, uint32_t *result) {
    uint8_t *source = NULL;
    uint8_t *found;
    if (!memory || !result)
        return 0;
    if (address && count > 0) {
        source = musashi_boot_ram_span(memory, address, (size_t)count);
        if (!source)
            return 0;
    }
    found = source ? memchr(source, (unsigned char)target, (size_t)count) : NULL;
    *result = found ? address + (uint32_t)(found - source) : 0;
    return 1;
}

#define CONSOLE_CLASS_TABLE 0x800728a9u
#define CONSOLE_COLUMN 0x80078978u
#define CONSOLE_FALLBACK 0x80074498u

typedef struct ConsoleOutput {
    MusashiBootMemory *memory;
    const uint8_t *classes;
    uint32_t column;
    MusashiConsoleSink sink;
    void *userdata;
} ConsoleOutput;

static int console_output_init(ConsoleOutput *out, MusashiBootMemory *memory,
                               MusashiConsoleSink sink, void *userdata) {
    if (!memory || !sink)
        return 0;
    out->memory = memory;
    out->classes = musashi_boot_ram_span(memory, CONSOLE_CLASS_TABLE, 256);
    out->sink = sink;
    out->userdata = userdata;
    return out->classes && musashi_boot_read32(memory, CONSOLE_COLUMN, &out->column);
}

static int console_write_byte(ConsoleOutput *out, uint8_t byte) {
    if (out->sink) {
        if (!musashi_boot_write32(out->memory, CONSOLE_COLUMN, out->column))
            return 0;
        out->sink(out->userdata, &byte, 1);
    }
    return 1;
}

static unsigned console_byte_width(const ConsoleOutput *out, uint8_t byte) {
    return (out->classes[byte] & 0x97u) != 0;
}

static int console_output_byte(ConsoleOutput *out, uint8_t byte) {
    unsigned width, writes = 1;
    if (byte == '\n') {
        out->column += console_byte_width(out, '\r');
        if (!console_write_byte(out, '\r'))
            return 0;
        out->column = 0;
        return console_write_byte(out, '\n');
    }
    if (byte == '\t') {
        byte = ' ';
        width = console_byte_width(out, byte);
        if (width)
            writes = 8u - (out->column & 7u);
        else if (out->column & 7u)
            return 0; /* This classification cannot reach the next tab stop. */
    } else {
        width = console_byte_width(out, byte);
    }
    for (unsigned i = 0; i < writes; ++i) {
        out->column += width;
        if (!console_write_byte(out, byte))
            return 0;
    }
    return 1;
}

int musashi_boot_console_char(MusashiBootMemory *memory, int32_t value,
                              MusashiConsoleSink sink, void *userdata) {
    ConsoleOutput out;
    return console_output_init(&out, memory, sink, userdata) &&
           console_output_byte(&out, (uint8_t)value);
}

static const uint8_t *console_string_end(MusashiBootMemory *memory, uint32_t address) {
    const uint8_t *start = musashi_boot_ram_span(memory, address, 1);
    return start ? memchr(start, 0, (size_t)(memory->bytes + MUSASHI_RAM_SIZE - start)) : NULL;
}

int musashi_boot_console_string(MusashiBootMemory *memory, uint32_t address,
                                MusashiConsoleSink sink, void *userdata) {
    ConsoleOutput out;
    const uint8_t *start, *end;
    if (!console_output_init(&out, memory, sink, userdata))
        return 0;
    start = musashi_boot_ram_span(memory, address, 1);
    end = console_string_end(memory, address);
    if (!end || !console_string_end(memory, CONSOLE_FALLBACK))
        return 0;

    /* Custom classifications can leave an unaligned tab unable to advance.
     * Check the whole string first so an invalid tab emits no partial text. */
    if (!console_byte_width(&out, ' ') && memchr(start, '\t', (size_t)(end - start))) {
        ConsoleOutput check = out;
        check.sink = NULL;
        for (const uint8_t *p = start; p < end; ++p)
            if (!console_output_byte(&check, *p))
                return 0;
    }
    for (const uint8_t *p = start; p < end && *p; ++p)
        if (!console_output_byte(&out, *p))
            return 0;
    return 1;
}

int musashi_boot_call_8005c29c(MusashiBootMemory *memory, uint32_t address,
                              int32_t value, uint32_t count) {
    uint8_t *destination = musashi_boot_ram_span(memory, address, count ? count : 1);
    if (!destination)
        return 0;
    memset(destination, (unsigned char)value, (size_t)count);
    return 1;
}

int musashi_boot_call_80016714(MusashiBootMemory *memory, uint32_t address,
                               uint32_t count) {
    uint8_t *destination;
    if (!memory || count > INT32_MAX)
        return 0;
    destination = musashi_boot_ram_span(memory, address, count ? count : 1);
    if (!destination)
        return 0;
    func_80016714_native(destination, (int32_t)count);
    return 1;
}

int musashi_boot_call_8001903c(MusashiBootMemory *memory) {
    return musashi_boot_call_80016714(memory, 0x800747c0u, 8u);
}

int musashi_boot_call_80029044(MusashiBootMemory *memory) {
    uint8_t *destination = musashi_boot_ram_span(memory, 0x800ae648u, 0x40u);
    if (!destination)
        return 0;
    func_80029044_native(destination);
    return 1;
}

int musashi_boot_call_8002906c(MusashiBootMemory *memory) {
    uint8_t *destination = musashi_boot_ram_span(memory, 0x800ba1b8u, 0x100u);
    if (!destination)
        return 0;
    func_8002906C_native(destination);
    return 1;
}

int musashi_boot_call_80043060(MusashiBootMemory *memory, int32_t argument,
                              int32_t *previous) {
    uint32_t old_word;
    if (!previous || !musashi_boot_read32(memory, 0x8006cbe8u, &old_word) ||
        !musashi_boot_write32(memory, 0x8006cbe8u, (uint32_t)argument))
        return 0;
    memcpy(previous, &old_word, sizeof(old_word));
    return 1;
}

MusashiPrefixStatus musashi_boot_run_prefix(MusashiBootMemory *memory,
                                           MusashiPrefixStop *stop) {
    uint32_t guard;
    int32_t previous;
    if (!stop || !musashi_boot_read32(memory, 0x80062998u, &guard))
        return MUSASHI_PREFIX_INVALID_INPUT;

    /* Platform CRT boundary 800100A0: the pinned executable encodes a literal
     * zero constructor count at 800100D0/D4. Preserve an existing nonzero guard.
     * This adapter is not a pure-C match for that handwritten assembly. */
    if (guard == 0)
        musashi_boot_write32(memory, 0x80062998u, 1);

    /* Call at 800101AC, then store at 800101BC. Use the decomp-owned exchange
     * body, not an equivalent assignment implemented here. All addresses are
     * fixed valid RAM spans after the initial pointer check. */
    musashi_boot_call_80043060(memory, 0, &previous);
    musashi_boot_write32(memory, 0x80074778u, 0x3e0u);

    /* No call to 800141F0 and no success-returning replacement for its missing
     * dependencies. This bounded diagnostic does not replace the game main. */
    stop->call_address = 0x800101c0u;
    stop->target_address = 0x800141f0u;
    return MUSASHI_PREFIX_UNSUPPORTED_CALL;
}
