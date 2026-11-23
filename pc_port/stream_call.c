#include "stream_call.h"
#include "musashi_stream_timing.h"
#include "formatter_integer.h"
#include <stdlib.h>

typedef struct StreamReceipt {
    uint32_t issue, address, value;
    unsigned char width, written;
} StreamReceipt;
typedef struct StreamRun {
    const MusashiStreamCall *call;
    MusashiStreamTiming timing;
    StreamReceipt *receipts;
    unsigned count, capacity, issued, retired;
    uint32_t phase, literals, copied;
    MusashiStreamCallResult reason;
} StreamRun;

static uint32_t code_at(const uint8_t *p, uint32_t pc) {
    p += pc - MUSASHI_STREAM_ENTRY;
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static unsigned memory_width(unsigned op) {
    switch (op) {
    case 32: case 36: case 40: return 1;
    case 33: case 37: case 41: return 2;
    case 35: case 43: return 4;
    default: return 0;
    }
}
int musashi_stream_witness_supports(const uint8_t *code) {
    uint32_t r[32] = {0}, next = 0;
    if (!code) return 0;
    for (uint32_t offset = 0; offset < MUSASHI_STREAM_CODE_BYTES; offset += 4) {
        uint32_t pc = MUSASHI_STREAM_ENTRY + offset;
        uint32_t word = code_at(code, pc);
        if (!memory_width(word >> 26) && !formatter_integer_effect(word, pc, r, &next)) return 0;
    }
    return 1;
}
int musashi_stream_bundled_delay(const uint8_t *code, const MusashiStreamCpu *cpu) {
    if (!cpu->delay_slot || (cpu->branch_pc & 3u) ||
        cpu->branch_pc - MUSASHI_STREAM_ENTRY >= MUSASHI_STREAM_CODE_BYTES) return 0;
    uint32_t word = code_at(code, cpu->branch_pc);
    return word >> 26 == 0 && (word & 63u) == 8u;
}

/* Exceptional recovery executes the existing formatter's pure integer/flow
 * operators on a private register set. Each data access must match an actual
 * retired receipt. There is no live memory, device, clock or observer here. */
static int recover(const StreamRun *run, MusashiStreamCpu *cpu) {
    unsigned used = 0;
    for (unsigned i = 1; i <= run->retired; ++i) {
        uint32_t pc = cpu->pc, npc = cpu->npc;
        if ((pc & 3u) || pc - MUSASHI_STREAM_ENTRY >= MUSASHI_STREAM_CODE_BYTES) return 0;
        uint32_t word = code_at(run->call->authenticated_code, pc);
        unsigned op = word >> 26, width = memory_width(op);
        int branch = (op >= 1 && op <= 7) || (op == 0 && (word & 63u) == 8);
        if (cpu->delay_slot && branch) return 0;
        uint32_t next = npc + 4u;
        int receipt_due = used < run->count && run->receipts[used].issue == i;
        if (receipt_due != (width != 0)) return 0;
        if (width) {
            const StreamReceipt *r = &run->receipts[used++];
            unsigned rs = word >> 21 & 31u, rt = word >> 16 & 31u;
            uint32_t address = cpu->r[rs] + (uint32_t)(int32_t)(int16_t)word;
            uint32_t mask = width == 1 ? 255u : width == 2 ? 65535u : UINT32_MAX;
            if (r->address != address || r->width != width || r->written != (op >= 40)) return 0;
            if (r->written) {
                if ((cpu->r[rt] & mask) != r->value) return 0;
            } else {
                uint32_t value = r->value;
                if (op == 32 && value & 128u) value |= 0xffffff00u;
                if (op == 33 && value & 32768u) value |= 0xffff0000u;
                cpu->r[rt] = value;
            }
        } else if (!formatter_integer_effect(word, pc, cpu->r, &next)) return 0;
        cpu->r[0] = 0;
        cpu->branch_pc = branch ? pc : 0;
        cpu->delay_slot = (unsigned)branch;
        cpu->pc = npc;
        cpu->npc = next;
    }
    return used == run->count;
}

static int charge(StreamRun *r, unsigned cost) {
    const MusashiStreamCall *call = r->call;
    if (call->frame->faulted) { r->reason = MUSASHI_STREAM_FAULT; return 0; }
    if (!call->current(call->owner, r->phase)) return 0;
    if (!*call->budget) {
        call->frame->budget_exhausted = 2;
        r->reason = MUSASHI_STREAM_FAULT;
        return 0;
    }
    --*call->budget;
    ++call->frame->instructions;
    if (!r->issued && call->entered) call->entered(call->owner);
    ++r->issued;
    if (!call->clock->advance(call->clock->userdata, cost) || call->frame->faulted) {
        r->reason = MUSASHI_STREAM_FAULT;
        return 0;
    }
    return 1;
}
static int pure_issues(StreamRun *r, unsigned count) {
    while (count--) {
        if (!charge(r, 2)) return 0;
        ++r->retired;
    }
    return 1;
}
static int begin_access(StreamRun *r, MusashiStreamEvent event, int writing, uint32_t value) {
    unsigned before;
    if (!musashi_stream_computations_before(&r->timing, event, writing, value, &before)) return 0;
    if (r->count == r->capacity) {
        /* At most 2,048 input bytes, 65 output bytes per complete match, and
         * three data accesses per emitted match byte. This ceiling includes
         * a high byte restored from the preceding sector and all saved fields. */
        unsigned next = r->capacity ? r->capacity * 2u : 1024u;
        if (next > 262144u) return 0;
        StreamReceipt *grown = realloc(r->receipts, (size_t)next * sizeof(*grown));
        if (!grown) return 0;
        r->receipts = grown;
        r->capacity = next;
    }
    return pure_issues(r, before) && charge(r, 3);
}
static void observe(StreamRun *r, MusashiStreamEvent event, unsigned writing,
    uint32_t address, unsigned width, uint32_t value) {
    r->receipts[r->count++] = (StreamReceipt){r->issued, address, value, (unsigned char)width,
                                              (unsigned char)writing};
    ++r->retired;
    musashi_stream_timing_observe(&r->timing, event, (int)writing, value);
    if (writing && event == MUSASHI_STREAM_SAVED_PHASE) r->phase = value;
    if (event == MUSASHI_STREAM_LITERAL_OUTPUT) ++r->literals;
    if (event == MUSASHI_STREAM_MATCH_OUTPUT) ++r->copied;
}
static int read_event(void *data, const MusashiStreamBus *bus, MusashiStreamEvent event,
    uint32_t address, unsigned width, uint32_t *value) {
    StreamRun *r = data;
    if (!begin_access(r, event, 0, 0)) return 0;
    if (!bus->read(bus->userdata, address, width, value)) {
        r->reason = MUSASHI_STREAM_FAULT;
        return 0;
    }
    observe(r, event, 0, address, width, *value);
    return 1;
}
static int write_event(void *data, const MusashiStreamBus *bus, MusashiStreamEvent event,
    uint32_t address, unsigned width, uint32_t value) {
    StreamRun *r = data;
    if ((event == MUSASHI_STREAM_LITERAL_RING || event == MUSASHI_STREAM_MATCH_RING) &&
        address - 0x1f800000u >= MUSASHI_SCRATCHPAD_SIZE) return 0;
    if ((event == MUSASHI_STREAM_LITERAL_OUTPUT || event == MUSASHI_STREAM_MATCH_OUTPUT) &&
        address - 0x80000000u >= MUSASHI_RAM_SIZE) return 0;
    if (!begin_access(r, event, 1, value)) return 0;
    if (!bus->write(bus->userdata, address, width, value)) {
        r->reason = MUSASHI_STREAM_FAULT;
        return 0;
    }
    observe(r, event, 1, address, width, value);
    return 1;
}

MusashiStreamCallResult musashi_stream_execute_work(const MusashiStreamCall *call,
    const MusashiStreamBus *bus, MusashiStreamCpu *cpu, MusashiStreamWork *work) {
    if (!call || !call->authenticated_code || !call->clock || !call->clock->advance ||
        !call->frame || !call->frame->busy || call->frame->faulted || !call->budget ||
        !call->current || !bus || !cpu || cpu->pc != MUSASHI_STREAM_ENTRY ||
        cpu->npc != cpu->pc + 4u || cpu->branch_pc || cpu->delay_slot) return MUSASHI_STREAM_DEFER;
    StreamRun r = {.call = call, .phase = call->phase, .reason = MUSASHI_STREAM_SCALAR};
    const MusashiStreamEvents events = {&r, read_event, write_event};
    uint32_t returned[32];
    unsigned tail;
    if (musashi_stream_decode_gpr(bus, &events, cpu->r, returned) &&
        musashi_stream_tail_computations(&r.timing, returned[2], &tail) && pure_issues(&r, tail)) {
        for (unsigned i = 0; i < 32; ++i) cpu->r[i] = returned[i];
        cpu->pc = returned[31]; cpu->npc = cpu->pc + 4u;
        if (work) *work = (MusashiStreamWork){r.issued, 2u * r.issued + r.count, r.literals, r.copied};
        r.reason = MUSASHI_STREAM_RETURNED;
    } else if (!r.issued && r.reason == MUSASHI_STREAM_SCALAR) r.reason = MUSASHI_STREAM_DEFER;
    else if (!recover(&r, cpu)) r.reason = MUSASHI_STREAM_WITNESS_FAILED;
    free(r.receipts);
    return r.reason;
}

MusashiStreamCallResult musashi_stream_execute(const MusashiStreamCall *call,
    const MusashiStreamBus *bus, MusashiStreamCpu *cpu) {
    return musashi_stream_execute_work(call, bus, cpu, NULL);
}
