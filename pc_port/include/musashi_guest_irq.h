#ifndef MUSASHI_GUEST_IRQ_H
#define MUSASHI_GUEST_IRQ_H

#include "musashi_irq_dispatch.h"
#include "musashi_dma_controller.h"
#include "musashi_bios_events.h"

/* Only the owner's verified mappings populate these bytes. Caller-owned,
 * zero-initialized, stable-address storage lasts for one machine lifetime. */
typedef struct MusashiGuestCdCodeSpan {
    uint32_t pc;
    uint16_t offset, size;
} MusashiGuestCdCodeSpan;

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
    uint32_t card_case_targets[4];
    /* Packed source ranges admitted by the CD frame. Populated from this
     * machine's verified image; neither instructions nor data ship here. */
    MusashiGuestCdCodeSpan cd_spans[64];
    uint8_t cd_code[32768];
    unsigned cd_span_count;
    /* Cleared when a fresh mapping begins, including a rejected mapping. */
    struct MusashiMain12Owner *main12_owner;
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
typedef struct MusashiGuestIrqServices {
    const MusashiCdSpuDevice *spu;
    const struct MusashiGuestIrqCd *cd;
    const MusashiGuestIrqCard *card;
} MusashiGuestIrqServices;

int musashi_boot_execute_guest_irq_with_services(const MusashiGuestIrqImage *,
    MusashiBootMemory *, uint32_t context, const uint32_t installed[12],
    const MusashiIrqDispatchDevice *, const MusashiExecutionClock *,
    MusashiDmaController *, const MusashiGuestIrqServices *, MusashiGuestIrqStop *);
int musashi_boot_execute_guest_irq(const MusashiGuestIrqImage *, MusashiBootMemory *,
    uint32_t context, const uint32_t installed[12], const MusashiIrqDispatchDevice *,
    const MusashiExecutionClock *, MusashiDmaController *, MusashiGuestIrqStop *);

#endif
