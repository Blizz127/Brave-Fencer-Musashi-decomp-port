#ifndef MUSASHI_STREAM_DECODE_H
#define MUSASHI_STREAM_DECODE_H

#include <stdint.h>

struct MusashiBootMemory;

#define MUSASHI_STREAM_STATE_COUNT 5u

/* Each callback performs one aligned little-endian scalar access at the
 * supplied guest address. Width is in bytes (1, 2, or 4). Reads zero-extend.
 * Return 1 on acceptance, 0 with this individual access uncommitted on fault.
 * Earlier accepted accesses remain visible, including to later reads.
 * verified_targets comes from the five-word dispatch table at 80072A38 in the
 * authenticated image. Keep it immutable and distinct from writable guest RAM. */
typedef struct MusashiStreamBus {
    void *userdata;
    int (*read)(void *, uint32_t address, unsigned width, uint32_t *value);
    int (*write)(void *, uint32_t address, unsigned width, uint32_t value);
    const uint32_t *verified_targets;
} MusashiStreamBus;

/* Names identify source operations even when the input, output, or dictionary
 * addresses alias. SAVED_PHASE reads the entry phase and writes 2, 3 or 4 on
 * suspension, or 0 on the end marker. Other SAVED fields retain their widths. */
typedef enum MusashiStreamEvent {
    MUSASHI_STREAM_SAVED_CURSOR, MUSASHI_STREAM_SAVED_MASK,
    MUSASHI_STREAM_SAVED_FLAGS, MUSASHI_STREAM_SAVED_DESTINATION,
    MUSASHI_STREAM_SAVED_PHASE, MUSASHI_STREAM_SAVED_CODE,
    MUSASHI_STREAM_DISPATCH, MUSASHI_STREAM_FIRST_FLAGS, MUSASHI_STREAM_NEXT_FLAGS,
    MUSASHI_STREAM_LITERAL, MUSASHI_STREAM_MATCH_LOW, MUSASHI_STREAM_MATCH_HIGH,
    MUSASHI_STREAM_MATCH_SOURCE, MUSASHI_STREAM_LITERAL_RING, MUSASHI_STREAM_MATCH_RING,
    MUSASHI_STREAM_LITERAL_OUTPUT, MUSASHI_STREAM_MATCH_OUTPUT
} MusashiStreamEvent;

/* Optional typed bus adapter. Each callback owns the single named access and
 * can forward it to the underlying bus when ready. The same acceptance and
 * failure contract applies. userdata and both callbacks must outlive the call. */
typedef struct MusashiStreamEvents {
    void *userdata;
    int (*read)(void *, const MusashiStreamBus *, MusashiStreamEvent,
        uint32_t address, unsigned width, uint32_t *value);
    int (*write)(void *, const MusashiStreamBus *, MusashiStreamEvent,
        uint32_t address, unsigned width, uint32_t value);
} MusashiStreamEvents;

/* Construct a bus backed by the port's cached RAM and physical scratchpad.
 * Input and output may alias this storage; neither is prefetched or staged.
 * The caller owns memory and verified_targets throughout each decode. */
MusashiStreamBus musashi_stream_memory_bus(struct MusashiBootMemory *memory,
    const uint32_t verified_targets[MUSASHI_STREAM_STATE_COUNT]);

/* One whole call of the streamed PAC decoder. source_address is the start of
 * the next 2048-byte sector. On completed return, write the guest v0 (1 needs
 * another sector, 0 finished) and return host success 1. On a refused access,
 * invalid bus, or changed dispatch target, return host failure 0, preserving
 * guest_v0 and every prior accepted access. Failure is not a retry point.
 * guest_v0 must be separate caller storage. The caller owns the device clock,
 * guest entry/return ABI, and admission to any production execution. */
int musashi_stream_decode_sector(const MusashiStreamBus *bus,
    uint32_t source_address, uint32_t *guest_v0);

/* Same call and return contract, with semantic access kinds. A null events
 * argument selects the plain bus directly. */
int musashi_stream_decode_sector_with_events(const MusashiStreamBus *bus,
    const MusashiStreamEvents *events, uint32_t source_address, uint32_t *guest_v0);

/* The typed caller's successful-return GPRs. entry and returned each contain
 * 32 registers, with a0 holding the source address. They may be the same
 * array, but must be separate from guest storage. A refusal leaves returned
 * unchanged. The enclosing CPU owns HI/LO, delay state and the actual return
 * transfer; no device clock, host callback or register publication is implied. */
int musashi_stream_decode_gpr(const MusashiStreamBus *, const MusashiStreamEvents *,
    const uint32_t entry[32], uint32_t returned[32]);

#endif
