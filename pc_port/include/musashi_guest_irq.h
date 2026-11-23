#ifndef MUSASHI_GUEST_IRQ_H
#define MUSASHI_GUEST_IRQ_H

#include "musashi_irq_dispatch.h"
#include "musashi_dma_controller.h"
#include "musashi_bios_events.h"
#include "musashi_native_lane.h"

/* Only the owner's verified mappings populate these bytes. Caller-owned,
 * zero-initialized, stable-address storage lasts for one machine lifetime. */
typedef struct MusashiGuestCdCodeSpan {
    uint32_t pc;
    uint16_t offset, size;
} MusashiGuestCdCodeSpan;

struct MusashiGuestIrqDraw;
struct MusashiGuestIrqImage;
/* Stable host-owned registration, initially zeroed. Remapping clears the
 * image binding. Rebinding the host earns a new generation for its offers. */
typedef struct MusashiGuestIrqDrawHost {
    const struct MusashiGuestIrqImage *image;
    uint64_t generation;
} MusashiGuestIrqDrawHost;

typedef int (*MusashiStreamClockOwner)(void *, MusashiBootMemory *, const MusashiExecutionClock *,
    const MusashiCdRegisterDevice *, const MusashiCdIrqFrame *, MusashiEntryHook);

typedef struct MusashiGuestIrqImage {
    const struct MusashiGuestIrqImage *identity;
    MusashiBootMemory *memory;
    uint8_t resume[0x10], dispatcher[0x1e8], vblank[0x78], dma[0x184], bios_return[0xc];
    uint8_t game_vblank[0x30], sound_tick[0x20c], voice_status[0xd0], channel_tick[0x50];
    uint8_t channel_update[0x55c], mix_update[0x198], levels_update[0xe8];
    uint8_t card_vblank[0xa0], card_index[0x10];
    uint8_t card_run_request[0x6c], card_push_request[0x80];
    uint8_t card_info_request[0x1e0], card_load_request[0x20c];
    uint8_t card_clear_result[0x58], card_reset_events[0x108];
    uint8_t card_poll_events[0xdc], card_event_flags[0x3c];
    uint8_t card_test_event[0xc], card_info_bios[0xc], card_load_bios[0xc];
    /* Unvisited gaps in the busy-5 worker stay unpopulated. */
    uint8_t card_read_worker[0x150], card_seek_bios[0xc], card_read_bios[0xc];
    uint32_t card_case_targets[4];
    /* Resident game sound sequencer, captured from the mounted executable. */
    uint8_t level_slot_tick[0xa5c], level_voice_choice[0xdc], level_voice_command[0x290];
    uint8_t level_motion_mask[0x6c], level_voice_mask[0x6c];
    uint8_t title_note[0x34], file_note[0x68];
    uint8_t channel_pitch[0x21c], channel_release[0x2c];
    uint8_t channel_expiry[0x110], channel_cond_release[0x74];
    /* Packed source ranges admitted by the CD frame. Populated from this
     * machine's verified image; neither instructions nor data ship here. */
    MusashiGuestCdCodeSpan cd_spans[64];
    uint8_t cd_code[32768];
    unsigned cd_span_count;
    /* Learned during this image's verified mapping. The machine installs the
     * ordinary clock verifier for its own CD frame after mapping succeeds. */
    uint32_t stream_targets[5];
    unsigned stream_code_offset;
    int stream_witness_ready;
    void *stream_machine;
    MusashiStreamClockOwner owns_stream_clock;
    /* Cleared when a fresh mapping begins, including a rejected mapping. */
    struct MusashiMain12Owner *main12_owner;
    uint8_t draw_complete[0x2ec], draw_irq_mask[0x1c];
    uint8_t draw_frame[0x6c], draw_frame_time[0x148];
    const MusashiGuestIrqDrawHost *draw_host;
    void *draw_machine;
    int (*owns_draw)(void *, const struct MusashiGuestIrqDraw *);
} MusashiGuestIrqImage;

typedef struct MusashiGuestIrqStop {
    uint32_t pc, instructions;
    uint64_t cd_instructions;
} MusashiGuestIrqStop;

int musashi_boot_map_irq_exe(MusashiGuestIrqImage *, MusashiBootMemory *,
    const uint8_t *exe, size_t size);
/* The installed saved-register record is captured by the scheduler. CPU
 * exception entry and the one-shot terminal service belong to that scheduler. */
typedef struct MusashiGuestIrqCd {
    MusashiBootMemory *memory;
    const MusashiCdRegisterDevice *registers;
    const MusashiCdIrqServices *services;
    MusashiCdIrqFrame *frame;
    MusashiResetGraphPrefixStop *stop;
} MusashiGuestIrqCd;
typedef struct MusashiGuestIrqCard {
    MusashiBootMemory *memory;
    const MusashiCdDevice *files;
    const MusashiEventDevice *events;
    const MusashiBiosEvents *registered_events;
} MusashiGuestIrqCard;
/* Borrowed for this interrupt on the same RAM, execution clock and channel-2 lease. */
typedef struct MusashiGuestIrqDraw {
    MusashiBootMemory *memory;
    const MusashiDmaController *dma;
    const struct MusashiGpuDma2 *channel;
    const MusashiExecutionClock *clock;
    const MusashiGpuIoDevice *gpu;
    void *timer;
    int (*timer_read16)(void *, uint32_t, uint16_t *);
    uint64_t generation;
    const MusashiGuestIrqDrawHost *host;
    const struct MusashiMdecController *mdec;
} MusashiGuestIrqDraw;
int musashi_boot_bind_irq_draw(MusashiGuestIrqImage *, MusashiGuestIrqDrawHost *,
    void *machine, int (*owns)(void *, const MusashiGuestIrqDraw *));
struct MusashiCdDma3;
typedef struct MusashiGuestIrqServices {
    const MusashiCdSpuDevice *spu;
    const struct MusashiGuestIrqCd *cd;
    const MusashiGuestIrqCard *card;
    const MusashiGuestIrqDraw *draw;
    const struct MusashiCdDma3 *cd_dma3;
    /* Observe the bound SPU's current EON without performing a guest bus access. */
    int (*spu_reverb_mask)(const MusashiCdSpuDevice *, uint32_t *);
} MusashiGuestIrqServices;

int musashi_boot_execute_guest_irq_with_services(const MusashiGuestIrqImage *,
    MusashiBootMemory *, uint32_t context, const uint32_t installed[12],
    const MusashiIrqDispatchDevice *, const MusashiExecutionClock *,
    MusashiDmaController *, const MusashiGuestIrqServices *, MusashiGuestIrqStop *);
int musashi_boot_execute_guest_irq(const MusashiGuestIrqImage *, MusashiBootMemory *,
    uint32_t context, const uint32_t installed[12], const MusashiIrqDispatchDevice *,
    const MusashiExecutionClock *, MusashiDmaController *, MusashiGuestIrqStop *);

#endif
