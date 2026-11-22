#ifndef MUSASHI_MDEC_CONTROLLER_H
#define MUSASHI_MDEC_CONTROLLER_H
#include "musashi_boot_memory.h"
#include "musashi_dma_controller.h"

/* MDEC reset, uploaded tables, color RLE/IDCT decode and DMA0/1 transport.
 * Monochrome, direct CPU pixel reads and DMA beyond a decoded frame
 * while its compressed input is still active remain unsupported.
 * The serialized native owner supplies RAM, shared DMA IRQ, and absolute time.
 * Buffered DMA1 output reaches RAM before its delayed IRQ. Its completion
 * uses the measured software-reference rate; console bus timing is unverified.
 */
typedef struct MusashiMdecController {
    MusashiBootMemory *ram;
    MusashiDmaController *dma;
    uint32_t control, command, remaining, received, count;
    uint32_t madr[2], bcr[2], chcr[2];
    uint64_t cycle, due, transfers;
    uint8_t quant[128];
    int16_t scale[64];
    uint16_t input[131072];
    uint8_t output[1024 * 768];
    uint32_t output_size, output_cursor;
    uint32_t output_request_address, output_request_bytes;
    uint64_t output_due;
    uint8_t input_waits_for_output, input_dma_error;
    uint8_t output_event_pending, output_finishes_frame;
} MusashiMdecController;
void musashi_mdec_init(MusashiMdecController *, MusashiBootMemory *, MusashiDmaController *);
int musashi_mdec_read32(MusashiMdecController *, uint32_t, uint32_t *);
int musashi_mdec_write32(MusashiMdecController *, uint32_t, uint32_t);
int musashi_mdec_advance(MusashiMdecController *, uint64_t);
#endif
