#ifndef MUSASHI_STREAM_CALL_H
#define MUSASHI_STREAM_CALL_H
#include "musashi_stream_decode.h"
#include "musashi_boot_memory.h"

#define MUSASHI_STREAM_ENTRY 0x80018730u
#define MUSASHI_STREAM_CODE_BYTES (0x80018918u - MUSASHI_STREAM_ENTRY)

typedef struct MusashiStreamCpu {
    uint32_t r[32], pc, npc, branch_pc;
    unsigned delay_slot;
} MusashiStreamCpu;
typedef struct MusashiStreamCall {
    const uint8_t *authenticated_code;
    const MusashiExecutionClock *clock;
    MusashiCdIrqFrame *frame;
    unsigned *budget;
    void *owner;
    /* Before each issue, check the CD caller, clock and protected guest RAM.
     * The phase comparison follows the decoder's own accepted state writes. */
    int (*current)(void *, uint32_t expected_phase);
    void (*entered)(void *);
    uint32_t phase;
} MusashiStreamCall;
typedef enum MusashiStreamCallResult {
    MUSASHI_STREAM_DEFER, MUSASHI_STREAM_RETURNED,
    MUSASHI_STREAM_SCALAR, MUSASHI_STREAM_FAULT, MUSASHI_STREAM_WITNESS_FAILED
} MusashiStreamCallResult;

/* Exceptional execution only: validated from the authenticated image, without
 * reading original instruction bytes in the compiled high-level decoder. */
int musashi_stream_witness_supports(const uint8_t *code);
int musashi_stream_bundled_delay(const uint8_t *code, const MusashiStreamCpu *);
typedef struct MusashiStreamWork {
    uint32_t issues, cycles, literals, copied;
} MusashiStreamWork;

/* Work is published only for a completed source call. */
MusashiStreamCallResult musashi_stream_execute_work(const MusashiStreamCall *,
    const MusashiStreamBus *, MusashiStreamCpu *cpu, MusashiStreamWork *work);
MusashiStreamCallResult musashi_stream_execute(const MusashiStreamCall *,
    const MusashiStreamBus *, MusashiStreamCpu *cpu);
#endif
