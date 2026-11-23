#include "musashi_stream_decode.h"
#include "musashi_boot_memory.h"
#include <string.h>

#define SECTOR_BYTES 2048u
#define WINDOW_MASK 1023u
#define WINDOW_BASE 0x1f800000u
#define MASK_WORD   0x800747a0u
#define FLAGS_WORD  0x800747a4u
#define DEST_WORD   0x800747acu
#define CURSOR_WORD 0x800747b0u
#define CODE_WORD   0x800747b4u
#define STATE_WORD  0x800c7d24u
#define STATE_TABLE 0x80072a38u

enum StreamPhase {
    FINISHED, NEW_STREAM, NEXT_TOKEN, TOKEN_HIGH, ADVANCE_MASK
};

typedef struct StreamReader {
    const MusashiStreamBus *bus;
    const MusashiStreamEvents *events;
    uint32_t source, remaining;
    uint32_t cursor, mask, flags, destination, code;
    uint32_t last_output_argument, match_end;
    int completed_match;
    const uint32_t *entry_gpr;
    uint32_t *returned_gpr;
} StreamReader;

static uint8_t *byte_address(MusashiBootMemory *memory, uint32_t address) {
    if (memory && address - WINDOW_BASE < MUSASHI_SCRATCHPAD_SIZE)
        return &memory->scratchpad[address - WINDOW_BASE];
    return musashi_boot_ram_span(memory, address, 1);
}

static int memory_read(void *userdata, uint32_t address, unsigned width, uint32_t *value) {
    MusashiBootMemory *memory = userdata;
    if (width == 1) {
        const uint8_t *byte = byte_address(memory, address);
        if (!byte) return 0;
        *value = *byte;
        return 1;
    }
    if (width == 2) {
        uint16_t half;
        if (!musashi_boot_read16(memory, address, &half)) return 0;
        *value = half;
        return 1;
    }
    return width == 4 && musashi_boot_read32(memory, address, value);
}

static int memory_write(void *userdata, uint32_t address, unsigned width, uint32_t value) {
    MusashiBootMemory *memory = userdata;
    if (width == 1) {
        uint8_t *byte = byte_address(memory, address);
        if (!byte) return 0;
        *byte = (uint8_t)value;
        return 1;
    }
    if (width == 2) return musashi_boot_write16(memory, address, (uint16_t)value);
    return width == 4 && musashi_boot_write32(memory, address, value);
}

MusashiStreamBus musashi_stream_memory_bus(MusashiBootMemory *memory,
    const uint32_t verified_targets[MUSASHI_STREAM_STATE_COUNT]) {
    MusashiStreamBus bus = {memory, memory_read, memory_write, verified_targets};
    return bus;
}

static int read_value(const StreamReader *s, MusashiStreamEvent event,
    uint32_t address, unsigned width, uint32_t *value) {
    if (s->events)
        return s->events->read(s->events->userdata, s->bus, event, address, width, value);
    return s->bus->read(s->bus->userdata, address, width, value);
}

static int write_value(const StreamReader *s, MusashiStreamEvent event,
    uint32_t address, unsigned width, uint32_t value) {
    if (s->events)
        return s->events->write(s->events->userdata, s->bus, event, address, width, value);
    return s->bus->write(s->bus->userdata, address, width, value);
}

static int source_byte(StreamReader *s, MusashiStreamEvent event, uint32_t *value) {
    if (!read_value(s, event, s->source, 1, value)) return 0;
    ++s->source;
    --s->remaining;
    return 1;
}

static int emit_byte(StreamReader *s, MusashiStreamEvent ring, MusashiStreamEvent output,
    uint32_t value) {
    if (!write_value(s, ring, WINDOW_BASE + s->cursor, 1, value)) return 0;
    s->cursor = (s->cursor + 1u) & WINDOW_MASK;
    if (!write_value(s, output, s->destination, 1, value)) return 0;
    ++s->destination;
    return 1;
}

static int complete_call(const StreamReader *s, uint32_t phase, int marker, uint32_t *guest_v0) {
    uint32_t result = phase >= 2 && phase <= 4;
    if (s->returned_gpr) {
        uint32_t *r = s->returned_gpr;
        if (r != s->entry_gpr) memcpy(r, s->entry_gpr, 32u * sizeof(*r));
        if (marker) r[1] = 0x800c0000u;
        else if (phase < MUSASHI_STREAM_STATE_COUNT) r[1] = 0x80070000u;
        r[2] = result;
        r[3] = marker ? s->code : phase;
        r[4] = s->last_output_argument;
        if (s->completed_match) {
            r[5] = 0;
            r[6] = s->match_end;
        }
        if (marker) r[6] = 0;
        r[7] = s->source;
        r[8] = s->remaining;
        r[9] = s->cursor;
        r[10] = s->destination;
        r[11] = s->mask;
        r[12] = s->code;
        r[13] = s->flags;
        r[14] = WINDOW_BASE;
    }
    *guest_v0 = result;
    return 1;
}

static int save_sector(const StreamReader *s, enum StreamPhase phase, uint32_t *guest_v0) {
    if (!write_value(s, MUSASHI_STREAM_SAVED_PHASE, STATE_WORD, 4, phase) ||
        !write_value(s, MUSASHI_STREAM_SAVED_CURSOR, CURSOR_WORD, 4, s->cursor) ||
        !write_value(s, MUSASHI_STREAM_SAVED_MASK, MASK_WORD, 1, s->mask) ||
        !write_value(s, MUSASHI_STREAM_SAVED_FLAGS, FLAGS_WORD, 1, s->flags) ||
        !write_value(s, MUSASHI_STREAM_SAVED_DESTINATION, DEST_WORD, 4, s->destination) ||
        !write_value(s, MUSASHI_STREAM_SAVED_CODE, CODE_WORD, 2, s->code)) return 0;
    return complete_call(s, phase, 0, guest_v0);
}

static int decode_sector(const MusashiStreamBus *bus, const MusashiStreamEvents *events,
    uint32_t source_address, uint32_t *guest_v0, const uint32_t *entry_gpr, uint32_t *returned_gpr) {
    StreamReader s;
    uint32_t phase, target;
    if (!bus || !bus->read || !bus->write || !bus->verified_targets || !guest_v0 ||
        (events && (!events->read || !events->write))) return 0;
    s.bus = bus;
    s.events = events;
    s.source = source_address;
    s.remaining = SECTOR_BYTES;
    s.last_output_argument = source_address;
    s.match_end = 0;
    s.completed_match = 0;
    s.entry_gpr = entry_gpr;
    s.returned_gpr = returned_gpr;
    if (!read_value(&s, MUSASHI_STREAM_SAVED_CURSOR, CURSOR_WORD, 4, &s.cursor) ||
        !read_value(&s, MUSASHI_STREAM_SAVED_MASK, MASK_WORD, 1, &s.mask) ||
        !read_value(&s, MUSASHI_STREAM_SAVED_FLAGS, FLAGS_WORD, 1, &s.flags) ||
        !read_value(&s, MUSASHI_STREAM_SAVED_DESTINATION, DEST_WORD, 4, &s.destination) ||
        !read_value(&s, MUSASHI_STREAM_SAVED_PHASE, STATE_WORD, 4, &phase) ||
        !read_value(&s, MUSASHI_STREAM_SAVED_CODE, CODE_WORD, 2, &s.code)) return 0;
    if (phase < MUSASHI_STREAM_STATE_COUNT &&
        (!read_value(&s, MUSASHI_STREAM_DISPATCH, STATE_TABLE + 4u * phase, 4, &target) ||
         !target || target != bus->verified_targets[phase])) return 0;
    if (phase == FINISHED || phase >= MUSASHI_STREAM_STATE_COUNT)
        return complete_call(&s, phase, 0, guest_v0);
    if (phase == NEW_STREAM) {
        s.cursor = 1;
        s.mask = 1;
        if (!source_byte(&s, MUSASHI_STREAM_FIRST_FLAGS, &s.flags)) return 0;
        phase = NEXT_TOKEN;
    }

    for (;;) {
        uint32_t byte;
        if (phase == ADVANCE_MASK) {
            if (s.mask == 128u) {
                s.mask = 1;
                if (!source_byte(&s, MUSASHI_STREAM_NEXT_FLAGS, &s.flags)) return 0;
                if (!s.remaining) return save_sector(&s, NEXT_TOKEN, guest_v0);
            } else {
                s.mask = (uint8_t)(s.mask << 1);
            }
        }
        if (phase != TOKEN_HIGH && (s.flags & s.mask)) {
            if (!source_byte(&s, MUSASHI_STREAM_LITERAL, &byte) ||
                !emit_byte(&s, MUSASHI_STREAM_LITERAL_RING, MUSASHI_STREAM_LITERAL_OUTPUT, byte)) return 0;
            s.last_output_argument = byte;
        } else {
            if (phase != TOKEN_HIGH) {
                if (!source_byte(&s, MUSASHI_STREAM_MATCH_LOW, &byte)) return 0;
                s.code = (s.code & 0xff00u) | byte;
                if (!s.remaining) return save_sector(&s, TOKEN_HIGH, guest_v0);
            }
            if (!source_byte(&s, MUSASHI_STREAM_MATCH_HIGH, &byte)) return 0;
            s.code = (s.code & 0xffu) | (byte << 8);
            uint32_t from = s.code & WINDOW_MASK;
            if (!from) {
                if (!write_value(&s, MUSASHI_STREAM_SAVED_PHASE, STATE_WORD, 4, FINISHED)) return 0;
                return complete_call(&s, FINISHED, 1, guest_v0);
            }
            /* A match can wrap and can read bytes just emitted by this match. */
            for (unsigned count = (s.code >> 10) + 2u; count; --count) {
                if (!read_value(&s, MUSASHI_STREAM_MATCH_SOURCE, WINDOW_BASE + from, 1, &byte) ||
                    !emit_byte(&s, MUSASHI_STREAM_MATCH_RING, MUSASHI_STREAM_MATCH_OUTPUT, byte)) return 0;
                s.last_output_argument = WINDOW_BASE + from;
                from = (from + 1u) & WINDOW_MASK;
            }
            s.completed_match = 1;
            s.match_end = from;
        }
        if (!s.remaining) return save_sector(&s, ADVANCE_MASK, guest_v0);
        phase = ADVANCE_MASK;
    }
}

int musashi_stream_decode_gpr(const MusashiStreamBus *bus, const MusashiStreamEvents *events,
    const uint32_t entry[32], uint32_t returned[32]) {
    uint32_t result;
    if (!entry || entry[0] || !returned) return 0;
    return decode_sector(bus, events, entry[4], &result, entry, returned);
}

int musashi_stream_decode_sector_with_events(const MusashiStreamBus *bus,
    const MusashiStreamEvents *events, uint32_t source_address, uint32_t *guest_v0) {
    return decode_sector(bus, events, source_address, guest_v0, NULL, NULL);
}

int musashi_stream_decode_sector(const MusashiStreamBus *bus,
    uint32_t source_address, uint32_t *guest_v0) {
    return musashi_stream_decode_sector_with_events(bus, NULL, source_address, guest_v0);
}
