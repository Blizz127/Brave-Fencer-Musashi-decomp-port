/* SPDX-License-Identifier: MIT */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../pc_port/mips_formatter.c"
#include "musashi_sio_controller.h"
#include "musashi_timer2.h"

#define IRQ_TEST_STATUS UINT32_C(0x1f801070)
#define IRQ_TEST_DATA UINT32_C(0x1f801040)
#define IRQ_TEST_TIMER UINT32_C(0x1f801120)
#define IRQ_TEST_ENTRY UINT32_C(0x8005d734)

/* The focused input link has no graphics device. Record any request so a
 * callback cannot appear successful after accidentally touching DMA2. */
static unsigned irq_test_gpu_requests;
int musashi_gpu_dma2_get_state(const MusashiGpuDma2 *gpu, MusashiGpuDma2State *state)
{
    (void)gpu;
    (void)state;
    ++irq_test_gpu_requests;
    return 0;
}
/* SPDX-License-Identifier: MIT */
/* The test callbacks append observations on actual accesses. The first
 * controller has port index 0 and the absent controller has port index 1. */
typedef struct IrqProtocolByte {
    uint64_t when;
    unsigned port;
    uint8_t byte;
} IrqProtocolByte;

typedef struct IrqProtocolWrite {
    IrqProtocolByte sent;
    int ack_pending;               /* Provider state immediately after writing DATA. */
    uint64_t ack_due;              /* Absolute provider deadline when pending. */
} IrqProtocolWrite;

typedef struct IrqProtocolRound {
    IrqProtocolWrite writes[16];
    IrqProtocolByte reads[24];
    uint64_t acknowledge[16];      /* Absolute cycle when an ACK interrupt is observed. */
    size_t write_count, read_count, acknowledge_count;
    unsigned pad_reads, sampled_port;
    uint16_t supplied_buttons;     /* Active-low packet returned by the bound test pad. */
    uint64_t enter_epoch, return_epoch, serial_epoch, timer_epoch;
    unsigned baud, control, ack_pending, ack_status;
} IrqProtocolRound;

typedef struct IrqProtocolHistory {
    unsigned completed;
    uint16_t last_supplied;
    uint8_t successful_packet[4];
    uint8_t last_response_marker;
} IrqProtocolHistory;
/* SPDX-License-Identifier: MIT */
/* Test device for a digital-only pad. Its wire state is independent of the
 * production SIO provider, whose pad also accepts configuration requests. */
enum {
    IRQ_DIG_DATA = 0x1f801040u, IRQ_DIG_STATUS = 0x1f801044u,
    IRQ_DIG_MODE = 0x1f801048u, IRQ_DIG_CONTROL = 0x1f80104au,
    IRQ_DIG_BAUD = 0x1f80104eu,
    IRQ_DIG_TX_ENABLE = 1u, IRQ_DIG_SELECT = 2u, IRQ_DIG_ACK_CLEAR = 16u,
    IRQ_DIG_RESET = 64u, IRQ_DIG_ACK_ENABLE = 1u << 12,
    IRQ_DIG_PORT_2 = 1u << 13, IRQ_DIG_RX_READY = 2u,
    IRQ_DIG_TX_READY = 5u, IRQ_DIG_INTERRUPT = 1u << 9,
    IRQ_DIG_PAD_IRQ = 1u << 7
};

enum {
    IRQ_DIG_ADDRESS, IRQ_DIG_COMMAND, IRQ_DIG_MARKER, IRQ_DIG_LOW,
    IRQ_DIG_HIGH, IRQ_DIG_END
};

static int irq_digital_initialize(MusashiSioController *s, const MusashiSioPadDevice *pad)
{
    if (!s || !pad || !pad->owner_thread || !pad->sample_buttons || !pad->raise_irq)
        return 0;
    uint64_t owner = pad->owner_thread(pad->userdata);
    if (!owner)
        return 0;
    *s = (MusashiSioController){0};
    s->pad_device = *pad;
    s->owner_token = owner;
    s->pad_bound = 1;
    s->disconnected_cards = 1;
    s->status = IRQ_DIG_TX_READY;
    return 1;
}

static int irq_digital_owned(MusashiSioController *s)
{
    if (!s || !s->pad_bound || s->faulted)
        return 0;
    if (s->owner_token != s->pad_device.owner_thread(s->pad_device.userdata)) {
        s->faulted = 1;
        return 0;
    }
    return 1;
}

static int irq_digital_advance(void *opaque, uint64_t epoch)
{
    MusashiSioController *s = opaque;
    if (!irq_digital_owned(s))
        return 0;
    if (s->cycle_initialized && epoch < s->cycle) {
        s->faulted = 1;
        return 0;
    }
    s->cycle = epoch;
    s->cycle_initialized = 1;
    if (s->ack_pending && epoch >= s->ack_due) {
        s->ack_pending = 0;
        if (s->control & IRQ_DIG_ACK_ENABLE) {
            s->status |= IRQ_DIG_INTERRUPT;
            if (!s->pad_device.raise_irq(s->pad_device.userdata, IRQ_DIG_PAD_IRQ)) {
                s->faulted = 1;
                return 0;
            }
        }
    }
    return 1;
}

static int irq_digital_read8(void *opaque, uint32_t address, uint8_t *out)
{
    MusashiSioController *s = opaque;
    if (!out || address != IRQ_DIG_DATA || !irq_digital_owned(s))
        return 0;
    *out = UINT8_MAX;
    if (s->rx_count) {
        *out = s->rx_fifo[s->rx_head];
        s->rx_head = (s->rx_head + 1) % sizeof(s->rx_fifo);
        if (!--s->rx_count)
            s->status &= (uint16_t)~IRQ_DIG_RX_READY;
    }
    return 1;
}

static int irq_digital_write8(void *opaque, uint32_t address, uint8_t value)
{
    MusashiSioController *s = opaque;
    if (address != IRQ_DIG_DATA || !irq_digital_owned(s) ||
        !(s->control & IRQ_DIG_TX_ENABLE) || !s->cycle_initialized)
        return 0;
    if (s->rx_count >= sizeof(s->rx_fifo) || s->ack_pending)
        return 0;

    uint8_t reply = UINT8_MAX;
    int acknowledge = 0;
    if ((s->control & IRQ_DIG_SELECT) && s->selected_port == 0) {
        switch (s->pad_state) {
        case IRQ_DIG_ADDRESS:
            if (value == 1) {
                uint16_t buttons;
                if (!s->pad_device.sample_buttons(s->pad_device.userdata, &buttons)) {
                    s->faulted = 1;
                    return 0;
                }
                s->sampled_buttons = buttons | UINT16_C(6);
                s->pad_state = IRQ_DIG_COMMAND;
                acknowledge = 1;
            } else {
                s->pad_state = IRQ_DIG_END;
            }
            break;
        case IRQ_DIG_COMMAND:
            if (value == 0x42) {
                reply = 0x41;
                s->pad_state = IRQ_DIG_MARKER;
                acknowledge = 1;
            } else {
                s->pad_state = IRQ_DIG_END;
            }
            break;
        case IRQ_DIG_MARKER:
            reply = 0x5a;
            s->pad_state = IRQ_DIG_LOW;
            acknowledge = 1;
            break;
        case IRQ_DIG_LOW:
            reply = (uint8_t)s->sampled_buttons;
            s->pad_state = IRQ_DIG_HIGH;
            acknowledge = 1;
            break;
        case IRQ_DIG_HIGH:
            reply = (uint8_t)(s->sampled_buttons >> 8);
            s->pad_state = IRQ_DIG_END;
            break;
        }
    }
    if (acknowledge) {
        uint64_t delay = (uint64_t)s->baud * 8u;
        if (!delay || delay > UINT64_MAX - s->cycle) {
            s->faulted = 1;
            return 0;
        }
        s->ack_due = s->cycle + delay;
        s->ack_pending = 1;
    }
    s->rx_fifo[(s->rx_head + s->rx_count) % sizeof(s->rx_fifo)] = reply;
    ++s->rx_count;
    ++s->transmitted_bytes;
    s->status |= IRQ_DIG_RX_READY;
    return 1;
}

static int irq_digital_read16(void *opaque, uint32_t address, uint16_t *out)
{
    MusashiSioController *s = opaque;
    if (!out || !irq_digital_owned(s))
        return 0;
    switch (address) {
    case IRQ_DIG_STATUS: *out = s->status; break;
    case IRQ_DIG_MODE: *out = s->mode; break;
    case IRQ_DIG_CONTROL: *out = s->control; break;
    case IRQ_DIG_BAUD: *out = s->baud; break;
    default: return 0;
    }
    return 1;
}

static int irq_digital_write16(void *opaque, uint32_t address, uint16_t value)
{
    MusashiSioController *s = opaque;
    if (!irq_digital_owned(s))
        return 0;
    switch (address) {
    case IRQ_DIG_MODE:
        s->mode = value;
        break;
    case IRQ_DIG_BAUD:
        s->baud = value;
        break;
    case IRQ_DIG_CONTROL: {
        int port = !!(value & IRQ_DIG_PORT_2);
        int reset = !!(value & IRQ_DIG_RESET);
        if (reset || !(value & IRQ_DIG_SELECT) || port != s->selected_port) {
            s->pad_state = IRQ_DIG_ADDRESS;
            s->ack_pending = 0;
            s->rx_head = s->rx_count = 0;
            s->status &= (uint16_t)~IRQ_DIG_RX_READY;
        }
        if (reset || (value & IRQ_DIG_ACK_CLEAR))
            s->status &= (uint16_t)~IRQ_DIG_INTERRUPT;
        s->control = reset ? 0 : value & (uint16_t)~IRQ_DIG_ACK_CLEAR;
        s->selected_port = reset ? 0 : port;
        break;
    }
    default:
        return 0;
    }
    return 1;
}

typedef struct IrqFixture {
    MusashiBootMemory *memory;
    uint8_t *baseline;
    uint8_t scratchpad_baseline[MUSASHI_SCRATCHPAD_SIZE];
    uint16_t last_timer_count;
    unsigned timer_reads, timer_wraps;
    struct {
        uint8_t before[16];
        MusashiBiosCallbackFrame frame;
        uint8_t after[16];
    } guarded;
    MusashiSioController sio;
    MusashiTimer2 timer;
    MusashiExecutionClock clock;
    MusashiSioPadDevice pad;
    MusashiInputDevice input;
    IrqProtocolRound trace;
    uint64_t epoch;
    uint16_t buttons;
    uint32_t status;
    unsigned attempts, accepted, veto_at, io_attempts;
    int freeze_timer, overflow, digital_only;
    void (*clock_observe)(struct IrqFixture *);
    void *observer;
} IrqFixture;

static uint64_t irq_test_thread(void *opaque)
{
    (void)opaque;
    return UINT64_C(731);
}

static int irq_test_buttons(void *opaque, uint16_t *out)
{
    IrqFixture *f = opaque;
    ++f->trace.pad_reads;
    f->trace.sampled_port = (unsigned)f->sio.selected_port;
    *out = f->buttons;
    return 1;
}

static int irq_test_ack(void *opaque, uint16_t mask)
{
    IrqFixture *f = opaque;
    if (mask != (UINT16_C(1) << 7)) {
        f->overflow = 1;
        return 0;
    }
    f->status |= mask;
    if (f->trace.acknowledge_count >= sizeof(f->trace.acknowledge)/sizeof(f->trace.acknowledge[0])) {
        f->overflow = 1;
        return 0;
    }
    f->trace.acknowledge[f->trace.acknowledge_count++] = f->epoch;
    return 1;
}

static int irq_test_charge(void *opaque, uint32_t cycles)
{
    IrqFixture *f = opaque;
    ++f->attempts;
    if (f->clock_observe)
        f->clock_observe(f);
    if (f->veto_at == f->attempts || UINT64_MAX - f->epoch < cycles)
        return 0;
    f->epoch += cycles;
    if (!(f->digital_only ? irq_digital_advance(&f->sio, f->epoch)
                           : musashi_sio_controller_advance(&f->sio, f->epoch)) ||
        (!f->freeze_timer && !musashi_timer2_advance(&f->timer, f->epoch)))
        return 0;
    ++f->accepted;
    return 1;
}

static int irq_test_read32(void *opaque, uint32_t address, uint32_t *out)
{
    IrqFixture *f = opaque;
    ++f->io_attempts;
    if (address != IRQ_TEST_STATUS)
        return 0;
    *out = f->status;
    return 1;
}

static int irq_test_write32(void *opaque, uint32_t address, uint32_t value)
{
    IrqFixture *f = opaque;
    ++f->io_attempts;
    if (address != IRQ_TEST_STATUS)
        return 0;
    f->status &= value;
    return 1;
}

static int irq_test_read16(void *opaque, uint32_t address, uint16_t *out)
{
    IrqFixture *f = opaque;
    ++f->io_attempts;
    if (address == IRQ_TEST_TIMER) {
        if (!musashi_timer2_read16(&f->timer, address, out))
            return 0;
        if (f->timer_reads && *out < f->last_timer_count)
            ++f->timer_wraps;
        ++f->timer_reads;
        f->last_timer_count = *out;
        return 1;
    }
    if (address == IRQ_TEST_TIMER + 4 || address == IRQ_TEST_TIMER + 8)
        return musashi_timer2_read16(&f->timer, address, out);
    return f->digital_only ? irq_digital_read16(&f->sio, address, out)
                           : musashi_sio_controller_read16(&f->sio, address, out);
}

static int irq_test_write16(void *opaque, uint32_t address, uint16_t value)
{
    IrqFixture *f = opaque;
    ++f->io_attempts;
    return f->digital_only ? irq_digital_write16(&f->sio, address, value)
                           : musashi_sio_controller_write16(&f->sio, address, value);
}

static int irq_test_read8(void *opaque, uint32_t address, uint8_t *out)
{
    IrqFixture *f = opaque;
    ++f->io_attempts;
    if (address != IRQ_TEST_DATA || !(f->digital_only ? irq_digital_read8(&f->sio, address, out)
                         : musashi_sio_controller_read8(&f->sio, address, out)))
        return 0;
    if (f->trace.read_count >= sizeof(f->trace.reads)/sizeof(f->trace.reads[0])) {
        f->overflow = 1;
        return 0;
    }
    IrqProtocolByte *event = &f->trace.reads[f->trace.read_count++];
    *event = (IrqProtocolByte){ f->epoch, (unsigned)f->sio.selected_port, *out };
    return 1;
}

static int irq_test_write8(void *opaque, uint32_t address, uint8_t value)
{
    IrqFixture *f = opaque;
    ++f->io_attempts;
    if (address != IRQ_TEST_DATA || !(f->digital_only ? irq_digital_write8(&f->sio, address, value)
                         : musashi_sio_controller_write8(&f->sio, address, value)))
        return 0;
    if (f->trace.write_count >= sizeof(f->trace.writes)/sizeof(f->trace.writes[0])) {
        f->overflow = 1;
        return 0;
    }
    IrqProtocolWrite *event = &f->trace.writes[f->trace.write_count++];
    event->sent = (IrqProtocolByte){f->epoch, (unsigned)f->sio.selected_port, value};
    event->ack_pending = f->sio.ack_pending;
    event->ack_due = f->sio.ack_due;
    return 1;
}

static void irq_close(IrqFixture *f)
{
    free(f->baseline);
    free(f->memory);
    f->baseline = NULL;
    f->memory = NULL;
}

static int irq_open_profile(IrqFixture *f, const uint8_t *exe, size_t size, uint64_t phase,
                            int digital_only)
{
    MusashiResetGraphPrefixStop stop = {0};
    memset(f, 0, sizeof(*f));
    f->memory = calloc(1, sizeof(*f->memory));
    f->baseline = malloc(sizeof(f->memory->bytes));
    if (!f->memory || !f->baseline || !musashi_boot_map_exe(f->memory, exe, size) ||
        musashi_boot_run_8005f0c8_prefix(f->memory, &stop) != MUSASHI_RESETGRAPH_PREFIX_COMPLETE)
        goto fail;
    memset(f->guarded.before, 0xa7, sizeof(f->guarded.before));
    memset(f->guarded.after, 0x3d, sizeof(f->guarded.after));
    musashi_boot_bios_callback_frame_init(&f->guarded.frame);
    memset(f->memory->bytes, 0xbc, 65536);
    memcpy(f->baseline, f->memory->bytes, sizeof(f->memory->bytes));
    memcpy(f->scratchpad_baseline, f->memory->scratchpad, sizeof(f->scratchpad_baseline));
    f->digital_only = digital_only;
    f->pad = (MusashiSioPadDevice){.userdata = f, .owner_thread = irq_test_thread,
        .sample_buttons = irq_test_buttons, .raise_irq = irq_test_ack};
    if (digital_only) {
        if (!irq_digital_initialize(&f->sio, &f->pad))
            goto fail;
    } else {
        musashi_sio_controller_init_disconnected(&f->sio);
        if (!musashi_sio_controller_bind_digital_pad(&f->sio, &f->pad))
            goto fail;
    }
    f->epoch = phase;
    musashi_timer2_init(&f->timer, 0);
    if (!musashi_timer2_advance(&f->timer, phase) || !(digital_only ? irq_digital_advance(&f->sio, phase)
                         : musashi_sio_controller_advance(&f->sio, phase)))
        goto fail;
    f->clock = (MusashiExecutionClock){.userdata=f, .advance=irq_test_charge};
    f->input = (MusashiInputDevice){.userdata=f, .read32=irq_test_read32, .write32=irq_test_write32,
        .read16=irq_test_read16, .write16=irq_test_write16, .read8=irq_test_read8,
        .write8=irq_test_write8, .clock=&f->clock};
    return 1;
fail:
    irq_close(f);
    return 0;
}

static int irq_open(IrqFixture *f, const uint8_t *exe, size_t size, uint64_t phase)
{
    return irq_open_profile(f, exe, size, phase, 0);
}

static const uint32_t irq_allowed_writes[][2] = {
    {469392,469472}, {493976,494632}, {495008,495012},
    {495084,495088}, {495396,495400}, {807712,807716}
};

static int irq_bounds_ok(const IrqFixture *f)
{
    size_t cursor = 0;
    for (size_t i = 0; i < sizeof(irq_allowed_writes)/sizeof(irq_allowed_writes[0]); ++i) {
        size_t stop = irq_allowed_writes[i][0];
        if (memcmp(f->baseline + cursor, f->memory->bytes + cursor, stop - cursor))
            return 0;
        cursor = irq_allowed_writes[i][1];
    }
    if (memcmp(f->baseline + cursor, f->memory->bytes + cursor, sizeof(f->memory->bytes) - cursor))
        return 0;
    for (size_t i = 0; i < sizeof(f->guarded.before); ++i)
        if (f->guarded.before[i] != 0xa7 || f->guarded.after[i] != 0x3d)
            return 0;
    return !f->overflow && !irq_test_gpu_requests &&
           !memcmp(f->scratchpad_baseline, f->memory->scratchpad, sizeof(f->scratchpad_baseline));
}

static void irq_new_round(IrqFixture *f, uint16_t buttons)
{
    f->trace = (IrqProtocolRound){0};
    f->trace.enter_epoch = f->epoch;
    f->trace.supplied_buttons = buttons;
    f->buttons = buttons;
    f->attempts = f->accepted = f->veto_at = f->io_attempts = 0;
    f->freeze_timer = f->overflow = 0;
    f->clock_observe = NULL;
    f->observer = NULL;
}

static int irq_invoke(IrqFixture *f, int32_t *result, MusashiResetGraphPrefixStop *stop)
{
    int rc = musashi_boot_execute_input_bios_callback(f->memory, &f->input, &f->guarded.frame,
                                                    IRQ_TEST_ENTRY, 1, result, stop);
    f->trace.return_epoch = f->epoch;
    f->trace.serial_epoch = f->sio.cycle;
    f->trace.timer_epoch = f->timer.last_cycles;
    f->trace.control = f->sio.control;
    f->trace.baud = f->sio.baud;
    f->trace.ack_pending = f->sio.ack_pending;
    f->trace.ack_status = f->status;
    return rc;
}
/* SPDX-License-Identifier: MIT */
static int irq_protocol_failure(unsigned round, const char *expected)
{
    fprintf(stderr, "digital IRQ callback %u: %s\n", round, expected);
    return 0;
}

static int irq_protocol_timing(const IrqProtocolRound *r, unsigned round, int digital)
{
    const size_t sent = digital ? 6u : 3u;
    const size_t ack = digital ? 4u : 1u;
    if (r->write_count != sent || r->read_count != sent + 2u ||
        r->acknowledge_count != ack)
        return irq_protocol_failure(round, "unexpected serial read, write or interrupt count");
    if (r->pad_reads != 1 || r->sampled_port != 0)
        return irq_protocol_failure(round, "expected one sample of the first controller");
    if (r->baud != 136 || r->control || r->ack_pending || r->ack_status)
        return irq_protocol_failure(round, "serial controller did not finish idle at its configured baud");
    if (r->return_epoch <= r->enter_epoch || r->return_epoch != r->serial_epoch ||
        r->return_epoch != r->timer_epoch)
        return irq_protocol_failure(round, "execution clock and both device epochs disagree");
    size_t seen_ack = 0;
    for (size_t i = 0; i < sent; ++i) {
        const IrqProtocolWrite *write = &r->writes[i];
        const uint64_t when = write->sent.when;
        const int absent = i + 1u == sent;
        const int expects_ack = i < ack;
        if (write->sent.port != (unsigned)absent ||
            when < r->enter_epoch || when >= r->return_epoch ||
            (i && when <= r->writes[i - 1u].sent.when))
            return irq_protocol_failure(round, "invalid transmit port, order or timestamp");
        if (!!write->ack_pending != expects_ack)
            return irq_protocol_failure(round, "unexpected ACK scheduling after DATA write");
        if (expects_ack) {
            const uint64_t delay = UINT64_C(8) * r->baud;
            const uint64_t raised = r->acknowledge[seen_ack++];
            if (write->ack_due < when || write->ack_due - when != delay ||
                raised < write->ack_due || raised > r->writes[i + 1u].sent.when)
                return irq_protocol_failure(round, "ACK did not follow its transmit deadline");
        }
    }
    for (size_t i = 0; i < r->read_count; ++i) {
        const IrqProtocolByte *read = &r->reads[i];
        const unsigned port = i >= sent ? 1u : 0u;
        if (read->port != port || read->when < r->enter_epoch ||
            read->when > r->return_epoch || (i && read->when < r->reads[i - 1u].when))
            return irq_protocol_failure(round, "invalid receive port, order or timestamp");
    }
    return 1;
}

static int irq_protocol_response(const IrqProtocolRound *r, unsigned round, int digital)
{
    const uint8_t idle = UINT8_MAX;
    const uint16_t buttons = r->supplied_buttons | UINT16_C(6);
    const size_t absent_write = r->write_count - 1u;
    const size_t absent_read = r->read_count - 2u;
    if (r->reads[0].when >= r->writes[0].sent.when ||
        r->reads[absent_read].when >= r->writes[absent_write].sent.when)
        return irq_protocol_failure(round, "each port must drain its idle byte before transmitting its address");
    if (r->return_epoch - r->writes[absent_write].sent.when < UINT64_C(3200))
        return irq_protocol_failure(round, "service returned before its disconnected-port wait");
    for (size_t i = 0; i < r->read_count; ++i) {
        uint8_t expected = idle;
        if (digital) {
            switch (i) {
            case 2: expected = 0x41; break; /* Standard digital pad ID. */
            case 3: expected = 0x5a; break; /* Response marker. */
            case 4: expected = (uint8_t)buttons; break;
            case 5: expected = (uint8_t)(buttons >> 8); break;
            }
        }
        if (r->reads[i].byte != expected)
            return irq_protocol_failure(round, "unexpected DATA reply from the selected port");
        if (i != 0 && i != absent_read) {
            size_t write = i < absent_read ? i - 1u : absent_write;
            if (r->reads[i].when < r->writes[write].sent.when ||
                (write < absent_write && r->reads[i].when > r->writes[write + 1u].sent.when))
                return irq_protocol_failure(round, "DATA reply did not follow its request");
        }
    }
    return 1;
}

static int irq_protocol_round_ok(const IrqProtocolRound *r, unsigned round)
{
    const int digital = round == 0 || (round >= 4 && round < 7);
    if (!r || round >= 7)
        return irq_protocol_failure(round, "test requires one of seven controller rounds");
    if (!irq_protocol_timing(r, round, digital) || !irq_protocol_response(r, round, digital))
        return 0;
    for (size_t i = 0; i < r->write_count; ++i) {
        uint8_t expected = 0;
        if (i == 0 || i + 1u == r->write_count)
            expected = 1; /* Controller address on each selected port. */
        else if (i == 1)
            expected = digital ? 0x42 : 0x43; /* Poll or configuration request. */
        if (r->writes[i].sent.byte != expected)
            return irq_protocol_failure(round, "unexpected DATA request to the selected port");
    }
    return 1;
}

static int irq_protocol_packet_ok(const uint8_t *ram, size_t size,
                                  const IrqProtocolRound *trace, unsigned round,
                                  IrqProtocolHistory *history)
{
    /* Data object offsets within the mapped two-megabyte RAM backing. */
    const size_t reply = 0x789b8u, published = 0x78da0u;
    const size_t retry = 0x78a48u + 70u, handler_result = 0x729c4u;
    const uint8_t initial_publication[4] = { UINT8_MAX, 0, 0, 0 };
    const uint8_t *previous;
    unsigned expected_retry;
    if (!ram || !trace || !history || round >= 7 || history->completed != round ||
        size < published + 4)
        return irq_protocol_failure(round, "invalid RAM or controller history");
    if (round && trace->supplied_buttons == history->last_supplied)
        return irq_protocol_failure(round, "fixture must change the input sample each round");
    previous = round ? history->successful_packet : initial_publication;
    if (memcmp(ram + published, previous, 4))
        return irq_protocol_failure(round, "published packet does not match the prior valid sample");
    expected_retry = round == 0 ? 0 : round < 3 ? 1 : UINT8_MAX;
    if (ram[retry] != expected_retry)
        return irq_protocol_failure(round, "unexpected controller configuration retry state");
    if (ram[handler_result] != 1 || ram[handler_result + 1] != 0 ||
        ram[handler_result + 2] != 0 || ram[handler_result + 3] != 0)
        return irq_protocol_failure(round, "handler did not publish a 32-bit success result");
    if (round == 0 || round >= 4) {
        uint8_t response[4];
        for (size_t i = 0; i < sizeof(response); ++i)
            response[i] = trace->reads[i + 2].byte;
        if (memcmp(ram + reply, response, sizeof(response)))
            return irq_protocol_failure(round, "current reply in RAM differs from observed serial bytes");
        history->successful_packet[0] = 0;
        history->successful_packet[1] = response[0];
        history->successful_packet[2] = response[2];
        history->successful_packet[3] = response[3];
        history->last_response_marker = response[1];
    } else {
        const uint8_t retained_reply[4] = {
            0, history->last_response_marker,
            history->successful_packet[2], history->successful_packet[3]
        };
        if (memcmp(ram + reply, retained_reply, sizeof(retained_reply)))
            return irq_protocol_failure(round, "rejected configuration corrupted the preceding digital reply");
    }
    history->last_supplied = trace->supplied_buttons;
    ++history->completed;
    return 1;
}
/* SPDX-License-Identifier: MIT */
typedef struct IrqStockHistory {
    IrqProtocolHistory packet;
    uint64_t end_epoch;
} IrqStockHistory;

static int irq_stock_protocol_ok(const IrqProtocolRound *r, const uint8_t *ram,
                                 size_t size, unsigned round, IrqStockHistory *history)
{
    /* Serial command bytes and the callback's corresponding configuration
     * progress are device-level observations from two fresh mounted runs. */
    static const struct { uint8_t command, progress; } configure[] = {
        {0x43, 1}, {0x45, 2}, {0x4c, 3}, {0x47, 4}, {0x43, 254}, {0x42, 255}
    };
    const size_t reply = 0x789b8u, published = 0x78da0u;
    const size_t retry = 0x78a48u + 70u, result = 0x729c4u;
    if (!r || !ram || !history || round >= 7 || history->packet.completed != round ||
        size < published + 4u)
        return irq_protocol_failure(round, "invalid stock RAM or packet history");
    if (round == 0) {
        if (!irq_protocol_round_ok(r, 0) ||
            !irq_protocol_packet_ok(ram, size, r, 0, &history->packet))
            return 0;
        history->end_epoch = r->return_epoch;
        return 1;
    }

    if (r->write_count != 10 || r->read_count != 12 || r->acknowledge_count != 8 ||
        r->pad_reads != 1 || r->sampled_port != 0 || r->supplied_buttons == history->packet.last_supplied)
        return irq_protocol_failure(round, "incorrect configured transaction or pad sample count");
    if (r->baud != 136 || r->control || r->ack_pending || r->ack_status ||
        r->enter_epoch != history->end_epoch || r->return_epoch <= r->enter_epoch ||
        r->serial_epoch != r->return_epoch || r->timer_epoch != r->return_epoch)
        return irq_protocol_failure(round, "configured controller failed to close at the shared epoch");

    for (size_t i = 0; i < r->write_count; ++i) {
        const IrqProtocolWrite *w = &r->writes[i];
        const unsigned absent = i + 1u == r->write_count;
        uint8_t expected = 0;
        if (i == 0 || absent)
            expected = 1;
        else if (i == 1)
            expected = configure[round - 1u].command;
        else if (round == 1 && i == 3)
            expected = 1; /* Enter configuration. Exit uses the same command with zero. */
        if (w->sent.byte != expected || w->sent.port != absent ||
            w->sent.when < r->enter_epoch || w->sent.when >= r->return_epoch ||
            (i && w->sent.when <= r->writes[i - 1u].sent.when))
            return irq_protocol_failure(round, "wrong configured command, payload, port or TX epoch");
        if (!!w->ack_pending == (int)absent)
            return irq_protocol_failure(round, "wrong configured ACK scheduling at a DATA write");
        if (!absent) {
            if (w->ack_due < w->sent.when || w->ack_due - w->sent.when != UINT64_C(8) * r->baud)
                return irq_protocol_failure(round, "configured ACK deadline differs from TX plus eight baud units");
            if (i < r->acknowledge_count && (r->acknowledge[i] < w->ack_due ||
                r->acknowledge[i] > r->writes[i + 1u].sent.when))
                return irq_protocol_failure(round, "configured IRQ arrived outside its transfer");
        }
    }
    /* The final configuration payload schedules an ACK. Releasing this
     * selection cancels it. It must not be raised on the absent second port. */
    if (r->reads[0].when >= r->writes[0].sent.when ||
        r->reads[10].when >= r->writes[9].sent.when)
        return irq_protocol_failure(round, "configured port did not drain its idle byte before addressing");
    if (r->return_epoch - r->writes[9].sent.when < UINT64_C(3200))
        return irq_protocol_failure(round, "configured service skipped the disconnected-port wait");

    const uint16_t buttons = r->supplied_buttons | UINT16_C(6);
    const uint8_t header[4] = {0xf3, 0x5a, (uint8_t)buttons, (uint8_t)(buttons >> 8)};
    for (size_t i = 0; i < r->read_count; ++i) {
        const IrqProtocolByte *read = &r->reads[i];
        const unsigned absent = i >= 10;
        uint8_t expected = UINT8_MAX;
        if (i >= 2 && i < 6)
            expected = header[i - 2];
        else if (i >= 6 && !absent)
            expected = 0;
        if (read->byte != expected || read->port != absent || read->when < r->enter_epoch ||
            read->when > r->return_epoch || (i && read->when < r->reads[i - 1u].when))
            return irq_protocol_failure(round, "wrong configured reply byte, port or receive epoch");
        if (i != 0 && i != 10) {
            size_t tx = absent ? 9 : i - 1u;
            if (read->when < r->writes[tx].sent.when ||
                (tx < 9 && read->when > r->writes[tx + 1u].sent.when))
                return irq_protocol_failure(round, "configured reply is outside its request");
        }
    }
    if (memcmp(ram + reply, header, sizeof(header)) ||
        memcmp(ram + published, history->packet.successful_packet, 4))
        return irq_protocol_failure(round, "configured reply or prior digital publication differs from the wire");
    if (ram[retry] != configure[round - 1u].progress || ram[result] != 1 ||
        ram[result + 1] || ram[result + 2] || ram[result + 3])
        return irq_protocol_failure(round, "wrong configuration progress or guest handler result");
    history->packet.last_supplied = r->supplied_buttons;
    ++history->packet.completed;
    history->end_epoch = r->return_epoch;
    return 1;
}
/* SPDX-License-Identifier: MIT */
enum IrqFailure {
    IRQ_FAIL_FIRST_CLOCK,
    IRQ_FAIL_LATER_CLOCK,
    IRQ_FAIL_TIMER,
    IRQ_FAIL_OUTER_LINK,
    IRQ_FAIL_INNER_LINK,
    IRQ_FAIL_OUTER_MAPPED_LINK,
    IRQ_FAIL_INNER_MAPPED_LINK
};

static int irq_sticky_refusal(IrqFixture *f)
{
    uint8_t scratch[sizeof(f->guarded.frame.scratch)];
    MusashiBootMemory *before = malloc(sizeof(*before));
    if (!before)
        return 0;
    memcpy(before, f->memory, sizeof(*before));
    memcpy(scratch, f->guarded.frame.scratch, sizeof(scratch));
    unsigned char serial_state[sizeof(f->sio)], timer_state[sizeof(f->timer)];
    unsigned char trace_state[sizeof(f->trace)];
    memcpy(trace_state, &f->trace, sizeof(trace_state));
    unsigned io_attempts = f->io_attempts;
    unsigned timer_reads = f->timer_reads, timer_wraps = f->timer_wraps;
    uint16_t last_timer_count = f->last_timer_count;
    uint32_t irq_status = f->status;
    memcpy(serial_state, &f->sio, sizeof(serial_state));
    memcpy(timer_state, &f->timer, sizeof(timer_state));
    uint64_t epoch = f->epoch;
    uint64_t timer_cycle = f->timer.last_cycles;
    uint64_t serial_cycle = f->sio.cycle;
    uint64_t sent = f->sio.transmitted_bytes;
    unsigned attempts = f->attempts;
    unsigned accepted = f->accepted;
    int32_t result = INT32_C(-40843);
    MusashiResetGraphPrefixStop stop = {0};
    int invoked = irq_invoke(f, &result, &stop);
    int ok = !invoked && result == INT32_C(-40843) &&
             f->guarded.frame.faulted && !f->guarded.frame.busy &&
             attempts == f->attempts && accepted == f->accepted &&
             epoch == f->epoch && timer_cycle == f->timer.last_cycles &&
             serial_cycle == f->sio.cycle && sent == f->sio.transmitted_bytes &&
             f->status == irq_status && f->io_attempts == io_attempts &&
             f->timer_reads == timer_reads && f->timer_wraps == timer_wraps &&
             f->last_timer_count == last_timer_count &&
             !memcmp(trace_state, &f->trace, sizeof(trace_state)) &&
             !memcmp(&f->sio, serial_state, sizeof(serial_state)) &&
             !memcmp(&f->timer, timer_state, sizeof(timer_state)) &&
             !memcmp(before, f->memory, sizeof(*before)) &&
             !memcmp(scratch, f->guarded.frame.scratch, sizeof(scratch)) &&
             irq_bounds_ok(f);
    free(before);
    return ok;
}

/* The private callback frame owns these little-endian saved-link fields. */
enum { IRQ_OUTER_LINK_FIELD = 248, IRQ_INNER_LINK_FIELD = 192 };
enum { IRQ_CALLBACK_BUDGET = 16384 };

typedef struct IrqReturnWatch {
    size_t field;
    uint32_t saved, replacement;
    unsigned observed_at;
    int replace;
} IrqReturnWatch;

static void irq_set_link_field(uint8_t *p, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i)
        p[i] = (uint8_t)(value >> (8u * i));
}

static int irq_link_producer(uint32_t word)
{
    unsigned op = word >> 26;
    unsigned rt = (word >> 16) & 31u;
    if (op == 3)
        return 1;
    if (op == 1)
        return rt == 16 || rt == 17;
    return op == 0 && (word & 63u) == 9 && rt == 0 &&
           ((word >> 11) & 31u) == 31 && ((word >> 6) & 31u) == 0;
}

static void irq_watch_return(IrqFixture *f)
{
    IrqReturnWatch *watch = f->observer;
    if (!watch || !f->guarded.frame.busy || watch->observed_at)
        return;
    uint8_t *field = &f->guarded.frame.scratch[watch->field];
    uint32_t saved = (uint32_t)field[0] | (uint32_t)field[1] << 8 |
                     (uint32_t)field[2] << 16 | (uint32_t)field[3] << 24;
    if (watch->field == IRQ_OUTER_LINK_FIELD) {
        if (saved != UINT32_MAX)
            return;
    } else {
        uint32_t producer;
        if (saved < 8 || (saved & 3u) ||
            !musashi_boot_read32(f->memory, saved - 8, &producer) ||
            !irq_link_producer(producer))
            return;
    }
    watch->observed_at = f->attempts;
    watch->saved = saved;
    if (watch->replace)
        irq_set_link_field(field, watch->replacement);
}

static int irq_register_return(MusashiBootMemory *memory, uint32_t pc)
{
    uint32_t word = 0;
    if ((pc & 3u) || !musashi_boot_read32(memory, pc, &word))
        return 0;
    /* SPECIAL jr rs; require a live non-zero source and reserved fields clear. */
    return (word >> 26) == 0 && ((word >> 21) & 31u) != 0 &&
           (word & UINT32_C(0x1fffff)) == 8;
}

static int irq_prepare_fault(IrqFixture *f, enum IrqFailure failure, IrqReturnWatch *watch,
                             int corrupt)
{
    int inner = failure == IRQ_FAIL_INNER_LINK || failure == IRQ_FAIL_INNER_MAPPED_LINK;
    int mapped = failure == IRQ_FAIL_OUTER_MAPPED_LINK || failure == IRQ_FAIL_INNER_MAPPED_LINK;
    if (inner) {
        int32_t warm_result = INT32_C(-19093);
        MusashiResetGraphPrefixStop warm_stop = {0};
        irq_new_round(f, UINT16_C(0xe7af));
        if (!irq_invoke(f, &warm_result, &warm_stop) || warm_result != 0 ||
            warm_stop.call_address != UINT32_MAX || warm_stop.target_address != UINT32_MAX ||
            f->guarded.frame.busy || f->guarded.frame.faulted || !irq_bounds_ok(f))
            return 0;
    }
    irq_new_round(f, UINT16_C(0xd7ea));
    if (failure >= IRQ_FAIL_OUTER_LINK) {
        watch->field = inner ? IRQ_INNER_LINK_FIELD : IRQ_OUTER_LINK_FIELD;
        watch->replacement = mapped ? IRQ_TEST_ENTRY : UINT32_C(0x65719b5d);
        if (mapped) {
            uint32_t mounted_word;
            if ((watch->replacement & 3u) ||
                !musashi_boot_read32(f->memory, watch->replacement, &mounted_word))
                return 0;
        }
        watch->replace = corrupt;
        /* A previous round's stale stack cell must never trigger this watch. */
        irq_set_link_field(f->guarded.frame.scratch + watch->field, UINT32_C(0x7658511d));
        f->observer = watch;
        f->clock_observe = irq_watch_return;
    }
    return 1;
}

static int irq_pristine_return_issue(const uint8_t *exe, size_t length, enum IrqFailure failure,
                                     uint64_t phase, const IrqReturnWatch *corrupted,
                                     uint32_t origin, unsigned attempts)
{
    /* A failing transfer can report after the branch, its slot or the enclosing
     * edge check. Stop the untouched companion on a nearby issue by clock veto.
     * Compare owner-derived PCs, without a catalog of return instructions. */
    unsigned first = attempts > 5 ? attempts - 5 : 1;
    for (unsigned tick = first; tick <= attempts + 3; ++tick) {
        IrqFixture f;
        if (!irq_open(&f, exe, length, phase))
            return 0;
        IrqReturnWatch clean = {0};
        if (!irq_prepare_fault(&f, failure, &clean, 0)) {
            irq_close(&f);
            return 0;
        }
        f.veto_at = tick;
        int32_t result = INT32_C(-12109);
        MusashiResetGraphPrefixStop stop = {0};
        int ran = irq_invoke(&f, &result, &stop);
        int matched = !ran && result == INT32_C(-12109) && f.attempts == tick &&
                      f.accepted + 1 == tick && clean.observed_at == corrupted->observed_at &&
                      clean.saved == corrupted->saved && stop.call_address == origin &&
                      stop.target_address == origin && irq_register_return(f.memory, origin) &&
                      f.guarded.frame.faulted && !f.guarded.frame.busy && irq_bounds_ok(&f);
        irq_close(&f);
        if (matched)
            return 1;
    }
    return 0;
}

static int irq_run_fault(IrqFixture *f, const uint8_t *exe, size_t length,
                         enum IrqFailure failure, uint64_t phase)
{
    IrqReturnWatch watch = {0};
    if (!irq_prepare_fault(f, failure, &watch, 1))
        return 0;
    uint64_t start = f->epoch;
    uint64_t timer_start = f->timer.last_cycles;
    uint64_t sent = f->sio.transmitted_bytes;
    uint8_t previous_packet[4], current_packet[4];
    const size_t current_offset = 494008, previous_offset = 495008;
    memcpy(current_packet, f->memory->bytes + current_offset, sizeof(current_packet));
    memcpy(previous_packet, f->memory->bytes + previous_offset, sizeof(previous_packet));
    if (failure == IRQ_FAIL_FIRST_CLOCK)
        f->veto_at = 1;
    else if (failure == IRQ_FAIL_LATER_CLOCK)
        f->veto_at = 37;
    else if (failure == IRQ_FAIL_TIMER)
        f->freeze_timer = 1;
    int32_t result = INT32_C(-67193);
    MusashiResetGraphPrefixStop stop = {0};
    if (irq_invoke(f, &result, &stop) || result != INT32_C(-67193) ||
        !f->guarded.frame.faulted || f->guarded.frame.busy || !irq_bounds_ok(f))
        return 0;
    if ((failure == IRQ_FAIL_FIRST_CLOCK || failure == IRQ_FAIL_TIMER) &&
        (memcmp(current_packet, f->memory->bytes + current_offset, sizeof(current_packet)) ||
         memcmp(previous_packet, f->memory->bytes + previous_offset, sizeof(previous_packet)))) {
        fprintf(stderr, "digital IRQ packet records changed before any transmission in case %u\n",
                (unsigned)failure);
        return 0;
    }
    if (failure == IRQ_FAIL_FIRST_CLOCK || failure == IRQ_FAIL_LATER_CLOCK) {
        if (f->attempts != f->veto_at || f->accepted + 1 != f->attempts ||
            (failure == IRQ_FAIL_FIRST_CLOCK &&
             (f->epoch != start || f->sio.transmitted_bytes != sent)) ||
            (failure == IRQ_FAIL_LATER_CLOCK && f->epoch <= start))
            return 0;
    } else if (failure == IRQ_FAIL_TIMER) {
        if (f->attempts != IRQ_CALLBACK_BUDGET || f->accepted != f->attempts ||
            f->epoch <= start || f->sio.cycle != f->epoch ||
            f->timer.last_cycles != timer_start || f->sio.transmitted_bytes != sent)
            return 0;
    } else if (!watch.observed_at || watch.replacement == watch.saved || f->attempts < watch.observed_at ||
               f->attempts > IRQ_CALLBACK_BUDGET || f->accepted != f->attempts ||
               stop.target_address != watch.replacement ||
               !irq_register_return(f->memory, stop.call_address) ||
               !irq_pristine_return_issue(exe, length, failure, phase, &watch,
                                          stop.call_address, f->attempts)) {
        return 0;
    }
    return irq_sticky_refusal(f);
}

static int irq_failure_cases(const uint8_t *exe, size_t length)
{
    const uint64_t phase = UINT64_C(3749);
    for (enum IrqFailure failure = IRQ_FAIL_FIRST_CLOCK; failure <= IRQ_FAIL_INNER_MAPPED_LINK; ++failure) {
        IrqFixture f;
        if (!irq_open(&f, exe, length, phase))
            return 0;
        int ok = irq_run_fault(&f, exe, length, failure, phase);
        irq_close(&f);
        if (!ok) {
            fprintf(stderr, "digital IRQ failure case %u did not match its observed bounds\n",
                    (unsigned)failure);
            return 0;
        }
    }
    return 1;
}
/* SPDX-License-Identifier: MIT */
/* Every expected destination and link below comes from the mounted word at
 * the observed origin, decoded independently using the MIPS-I instruction
 * fields. The source supplies neither a game instruction nor an edge list. */
static uint32_t irq_direct_destination(uint32_t origin, uint32_t instruction)
{
    return ((origin + 4u) & UINT32_C(0xf0000000)) |
           ((instruction & UINT32_C(0x03ffffff)) << 2);
}

static int irq_arch_issue(FormatterCpu *cpu, uint32_t word);

static int irq_single_call_credit(IrqFixture *fixture, const FormatterCpu *source,
                                  uint32_t target, unsigned link)
{
    MusashiBootMemory *memory = fixture->memory;
    FormatterCpu cpu = *source;
    unsigned remaining = 1;
    MusashiResetGraphPrefixStop stop = {.call_address = UINT32_MAX, .target_address = UINT32_MAX};
    FormatterCpu expected = *source;
    uint32_t word;
    if (!input_callback_word(memory, source, source->pc, &word) || !irq_arch_issue(&expected, word) ||
        !input_callback_step(memory, &cpu, &remaining, &stop) || remaining ||
        cpu.pc != source->pc + 4u || cpu.npc != target || cpu.r[link] != source->pc + 8u ||
        !cpu.delay_slot || cpu.branch_pc != source->pc || expected.pc != cpu.pc ||
        expected.npc != cpu.npc || memcmp(expected.r, cpu.r, sizeof(cpu.r)) ||
        stop.call_address != UINT32_MAX || stop.target_address != UINT32_MAX)
        return 0;
    MusashiBootMemory *frozen = malloc(sizeof(*frozen));
    if (!frozen)
        return 0;
    memcpy(frozen, memory, sizeof(*frozen));
    IrqFixture frozen_device;
    FormatterCpu frozen_cpu;
    memcpy(&frozen_device, fixture, sizeof(frozen_device));
    memcpy(&frozen_cpu, &cpu, sizeof(frozen_cpu));
    int stopped = !input_callback_step(memory, &cpu, &remaining, &stop) &&
                  stop.call_address == cpu.pc && stop.target_address == cpu.pc &&
                  !input_callback_step(memory, &cpu, &remaining, &stop) && !remaining &&
                  stop.call_address == cpu.pc && stop.target_address == cpu.pc &&
                  !memcmp(frozen, memory, sizeof(*frozen)) &&
                  !memcmp(&frozen_device, fixture, sizeof(frozen_device)) &&
                  !memcmp(&frozen_cpu, &cpu, sizeof(frozen_cpu));
    free(frozen);
    return stopped;
}

static int irq_mutated_source_refused(MusashiBootMemory *memory, const FormatterCpu *cpu,
                                      uint32_t original, uint32_t target)
{
    /* A fresh non-transfer instruction has no valid claim on this call. */
    const uint32_t replacement = (9u << 26) | (7u << 21) | (11u << 16) | 37u;
    uint32_t word;
    if (!musashi_boot_write32(memory, cpu->pc, replacement))
        return 0;
    uint32_t data_word;
    int changed = musashi_boot_read32(memory, cpu->pc, &data_word) && data_word == replacement;
    int fetched = input_callback_word(memory, cpu, cpu->pc, &word);
    /* The immutable executable view can remain live when ordinary data RAM
     * changes. The RAM replacement must never acquire the original grant. */
    int refused = !fetched || (word == replacement && !input_callback_call(memory, cpu, word, target)) ||
                  (word == original && input_callback_call(memory, cpu, word, target) &&
                   !input_callback_call(memory, cpu, replacement, target));
    int restored = musashi_boot_write32(memory, cpu->pc, original);
    return changed && refused && restored;
}

static int irq_admitted_direct(MusashiBootMemory *memory, const FormatterCpu *initial,
                               uint32_t address, FormatterCpu *cpu,
                               uint32_t *word, uint32_t *target, unsigned *owned)
{
    uint32_t mounted;
    if (!musashi_boot_read32(memory, address, word) || (*word >> 26) != 3)
        return 0;
    *cpu = *initial;
    cpu->pc = address;
    cpu->npc = address + 4u;
    cpu->branch_pc = 0;
    cpu->delay_slot = 0;
    if (!input_callback_word(memory, cpu, address, &mounted) || mounted != *word)
        return 0;
    if (owned)
        ++*owned;
    *target = irq_direct_destination(address, *word);
    return input_callback_call(memory, cpu, *word, *target);
}

static int irq_witness_return(IrqFixture *f, const FormatterCpu *source, uint32_t target);

static int irq_direct_cases(IrqFixture *fixture, const FormatterCpu *initial,
                            unsigned minimum, unsigned *covered, unsigned *owned, uint32_t *witnessed_leaf)
{
    MusashiBootMemory *memory = fixture->memory;
    unsigned tested = 0;
    uint32_t first = 0, other = 0;
    /* Establish two actual admitted entries for aligned, wrong-target
     * controls independently of the subsequent per-origin checks. */
    for (uint32_t offset = 0; offset < MUSASHI_RAM_SIZE && !other; offset += 4) {
        uint32_t address = UINT32_C(0x80000000) + offset;
        FormatterCpu cpu;
        uint32_t word, target;
        if (!irq_admitted_direct(memory, initial, address, &cpu, &word, &target, NULL))
            continue;
        if (!first)
            first = target;
        else if (first != target)
            other = target;
    }
    if (!other)
        return 0;
    for (uint32_t offset = 0; offset < MUSASHI_RAM_SIZE; offset += 4) {
        uint32_t address = UINT32_C(0x80000000) + offset;
        FormatterCpu cpu;
        uint32_t word, target;
        if (!irq_admitted_direct(memory, initial, address, &cpu, &word, &target, owned))
            continue;
        uint32_t wrong_owned = target == first ? other : first;
        if (input_callback_call(memory, &cpu, word ^ 1u, target) ||
            input_callback_call(memory, &cpu, word, target ^ 2u) ||
            input_callback_call(memory, &cpu, word, wrong_owned) ||
            !irq_mutated_source_refused(memory, &cpu, word, target) ||
            !irq_single_call_credit(fixture, &cpu, target, 31)) {
            fprintf(stderr, "mounted direct-call control failed at %08x\n", (unsigned)address);
            return 0;
        }
        if (!*witnessed_leaf && irq_witness_return(fixture, &cpu, target))
            *witnessed_leaf = target;
        ++tested;
    }
    *covered = tested;
    return minimum && tested >= minimum;
}
/* Hypothetical arithmetic/control encodings for the architectural oracle.
 * Actual input-family permission is checked against the mounted callback. */
static uint32_t irq_test_add(unsigned to, unsigned from, int delta)
{
    return (9u << 26) | (from << 21) | (to << 16) | (uint16_t)delta;
}

static uint32_t irq_test_register_transfer(unsigned from, unsigned link, int writes_link)
{
    return (from << 21) | (link << 11) | (writes_link ? 9u : 8u);
}

/* Independent MIPS-I model for these three control forms and the arithmetic
 * used in the hypothetical slots. The same model checks the ordinary
 * issuer's registered transfers on owner media below. */
static int irq_arch_issue(FormatterCpu *cpu, uint32_t word)
{
    unsigned opcode = word >> 26;
    unsigned rs = (word >> 21) & 31u;
    unsigned rt = (word >> 16) & 31u;
    unsigned rd = (word >> 11) & 31u;
    uint32_t next = cpu->npc + 4u;
    unsigned link = 0;
    int control = 0;
    if (opcode == 3u) {
        control = 1;
        next = irq_direct_destination(cpu->pc, word);
        link = 31;
    } else if (!opcode && ((word & 63u) == 8u || (word & 63u) == 9u)) {
        if (rt || ((word >> 6) & 31u) || ((word & 63u) == 8u && rd))
            return 0;
        control = 1;
        next = cpu->r[rs];
        link = rd;
    } else if (opcode == 9u) {
        uint32_t displacement = word & UINT32_C(0xffff);
        if (displacement & UINT32_C(0x8000))
            displacement |= UINT32_C(0xffff0000);
        if (rt)
            cpu->r[rt] = cpu->r[rs] + displacement;
    } else {
        return 0;
    }
    if (control && ((next & 3u) || cpu->delay_slot))
        return 0;
    if (link)
        cpu->r[link] = cpu->pc + 8u;
    cpu->delay_slot = (uint8_t)control;
    if (control)
        cpu->branch_pc = cpu->pc;
    cpu->pc = cpu->npc;
    cpu->npc = next;
    return 1;
}

static int irq_oracle_program_step(uint32_t start, const uint32_t words[13], FormatterCpu *cpu)
{
    uint32_t offset = cpu->pc - start;
    return !(offset & 3u) && offset / 4u < 13u && irq_arch_issue(cpu, words[offset / 4u]);
}

static int irq_synthetic_program(const FormatterCpu *initial)
{
    const uint32_t start = UINT32_C(0x801ddd40);
    uint32_t words[13];
    for (size_t i = 0; i < 13; ++i)
        words[i] = irq_test_add(19, 19, 11);
    const uint32_t direct = start + 8u * 4u;
    const uint32_t indirect = start + 11u * 4u;
    words[0] = (3u << 26) | ((direct >> 2) & UINT32_C(0x03ffffff));
    words[1] = irq_test_add(7, 6, 13);
    words[2] = irq_test_register_transfer(12, 12, 1);
    words[3] = irq_test_add(8, 12, -4);
    words[8] = irq_test_add(13, 7, -9);
    words[9] = irq_test_register_transfer(31, 0, 0);
    words[10] = irq_test_add(14, 13, 37);
    words[11] = irq_test_register_transfer(12, 0, 0);
    words[12] = irq_test_add(15, 14, 41);
    FormatterCpu cpu = *initial;
    cpu.pc = start;
    cpu.npc = start + 4u;
    cpu.delay_slot = 0;
    cpu.branch_pc = 0;
    cpu.r[6] = 93;
    cpu.r[12] = indirect;
    if (!irq_oracle_program_step(start, words, &cpu) || cpu.pc != start + 4u || cpu.npc != direct ||
        cpu.r[31] != start + 8u || !cpu.delay_slot || cpu.branch_pc != start)
        return 0;
    if (!irq_oracle_program_step(start, words, &cpu) || cpu.pc != direct || cpu.r[7] != 106)
        return 0;
    if (!irq_oracle_program_step(start, words, &cpu) || cpu.r[13] != 97 ||
        !irq_oracle_program_step(start, words, &cpu) || cpu.npc != start + 8u ||
        !irq_oracle_program_step(start, words, &cpu) || cpu.r[14] != 134 || cpu.pc != start + 8u)
        return 0;
    if (!irq_oracle_program_step(start, words, &cpu) || cpu.pc != start + 12u || cpu.npc != indirect ||
        cpu.r[12] != start + 16u || !cpu.delay_slot || cpu.branch_pc != start + 8u)
        return 0;
    if (!irq_oracle_program_step(start, words, &cpu) || cpu.r[8] != start + 12u ||
        cpu.pc != indirect || !irq_oracle_program_step(start, words, &cpu) || cpu.npc != start + 16u ||
        !irq_oracle_program_step(start, words, &cpu) || cpu.r[15] != 175 || cpu.pc != start + 16u)
        return 0;
    return 1;
}
static int irq_leaf_register_word(uint32_t word)
{
    unsigned op = word >> 26, fn = word & 63u;
    return (op >= 8u && op <= 15u) || (!op && (fn <= 7u || (fn >= 32u && fn <= 43u)));
}

static int irq_isolated_leaf_word(const FormatterCpu *cpu, uint32_t word)
{
    unsigned op = word >> 26;
    if (irq_leaf_register_word(word) || op == 2u || (op >= 4u && op <= 7u) ||
        (op == 1u && ((word >> 16) & 31u) <= 1u))
        return 1;
    if ((op >= 32u && op <= 38u) || (op >= 40u && op <= 43u) || op == 46u) {
        uint32_t displacement = word & UINT32_C(0xffff);
        if (displacement & UINT32_C(0x8000))
            displacement |= UINT32_C(0xffff0000);
        uint32_t address = cpu->r[(word >> 21) & 31u] + displacement;
        return (address & UINT32_C(0x1fffffff)) < MUSASHI_RAM_SIZE - 4u;
    }
    return 0;
}

static int irq_witness_return(IrqFixture *f, const FormatterCpu *source, uint32_t target)
{
    MusashiBootMemory *copy = malloc(sizeof(*copy));
    if (!copy)
        return 0;
    memcpy(copy, f->memory, sizeof(*copy));
    MusashiBiosCallbackFrame frame = f->guarded.frame;
    FormatterCpu child = *source;
    child.bios_frame = &frame;
    uint32_t call_slot;
    int returned = 0;
    unsigned credits = 2;
    MusashiResetGraphPrefixStop stop = {0};
    if (!input_callback_word(copy, &child, source->pc + 4u, &call_slot) ||
        !irq_isolated_leaf_word(&child, call_slot) ||
        !input_callback_step(copy, &child, &credits, &stop) ||
        child.r[31] != source->pc + 8u || child.npc != target ||
        !input_callback_step(copy, &child, &credits, &stop) || credits || child.pc != target) {
        free(copy);
        return 0;
    }
    for (unsigned count = 0; count < 80; ++count) {
        uint32_t word;
        if (!input_callback_word(copy, &child, child.pc, &word))
            break;
        if ((word >> 26) == 0 && (word & 63u) == 8u && ((word >> 21) & 31u) == 31u) {
            uint32_t slot;
            FormatterCpu oracle = child;
            if (!input_callback_word(copy, &child, child.pc + 4u, &slot) ||
                !irq_isolated_leaf_word(&child, slot) || !irq_arch_issue(&oracle, word) ||
                !formatter_step(copy, &child) || child.pc != oracle.pc ||
                child.npc != oracle.npc || child.delay_slot != oracle.delay_slot ||
                child.branch_pc != oracle.branch_pc || child.npc != source->pc + 8u ||
                memcmp(child.r, oracle.r, sizeof(child.r)))
                break;
            returned = formatter_step(copy, &child) && child.pc == source->pc + 8u;
            break;
        }
        if (!irq_isolated_leaf_word(&child, word) || !formatter_step(copy, &child))
            break;
    }
    free(copy);
    return returned;
}

/* A fresh configuration record points to an actually executed leaf that it
 * does not own. The real outer dispatcher must report the rejected call. */
static int irq_refused_record(const uint8_t *exe, size_t size, uint32_t leaf)
{
    const uint32_t record = UINT32_C(0x80078a48);
    IrqFixture f;
    if (!irq_open(&f, exe, size, 91))
        return 0;
    const int32_t sentinel = -91357;
    int32_t result = sentinel;
    MusashiResetGraphPrefixStop stop = {0};
    uint32_t after, mounted;
    FormatterCpu cpu = {0};
    cpu.bios_frame = &f.guarded.frame;
    cpu.clock = &f.clock;
    cpu.input_device = &f.input;
    uint8_t *marker = musashi_boot_ram_span(f.memory, record + 0x46u, 1);
    int ok = 0;
    if (!leaf || !marker || !musashi_boot_write32(f.memory, record + 0x14u, leaf))
        goto end;
    *marker = 2;
    if (irq_invoke(&f, &result, &stop) || result != sentinel ||
        f.guarded.frame.busy || !f.guarded.frame.faulted || stop.target_address != leaf ||
        !musashi_boot_read32(f.memory, record + 0x14u, &after) || after != leaf ||
        f.sio.transmitted_bytes || f.trace.write_count || !irq_bounds_ok(&f))
        goto end;
    cpu.pc = stop.call_address;
    cpu.npc = cpu.pc + 4u;
    if (!input_callback_word(f.memory, &cpu, cpu.pc, &mounted) ||
        (mounted >> 26) != 0 || (mounted & 63u) != 9u)
        goto end;
    ok = 1;
end:
    if (!ok)
        fprintf(stderr, "unsupported mounted input record result=%d target-match=%d wire=%u\n",
                result, stop.target_address == leaf, (unsigned)f.trace.write_count);
    irq_close(&f);
    return ok;
}
/* Data ABI of the input record. Actual code addresses, source words and
 * live entry values are discovered from the owner's mounted input image. */
#define IRQ_CONTROL_SELECTOR UINT32_C(0x800729a0)
#define IRQ_CONTROL_TABLE    UINT32_C(0x800729e0)
#define IRQ_CONTROL_SLOTS    5u

/* Data-only source identities of the registered callback pointers. The
 * minimums come from separate complete device trajectories. No code site,
 * loaded instruction, or target address is prescribed by this census. */
static const struct {
    uint32_t cell;
    unsigned witnessed;
} irq_suppliers[] = {
    {UINT32_C(0x8007295c),2}, {UINT32_C(0x80072960),0},
    {UINT32_C(0x80072964),2}, {UINT32_C(0x80072968),1},
    {UINT32_C(0x80072974),1}, {UINT32_C(0x8007297c),1}, {UINT32_C(0x80072980),1}
};

typedef struct IrqEdgeCensus {
    struct { uint32_t source, target; } pairs[128];
    unsigned count, per_cell[sizeof(irq_suppliers) / sizeof(irq_suppliers[0])];
} IrqEdgeCensus;

static int irq_remember_pair(IrqEdgeCensus *c, uint32_t source, uint32_t target)
{
    for (unsigned i = 0; i < c->count; ++i)
        if (c->pairs[i].source == source && c->pairs[i].target == target)
            return 1;
    if (c->count >= sizeof(c->pairs) / sizeof(c->pairs[0]))
        return 0;
    c->pairs[c->count].source = source;
    c->pairs[c->count].target = target;
    ++c->count;
    return 1;
}

static int irq_supplier_pairs(IrqFixture *f, const FormatterCpu *site, uint32_t word,
                              const uint32_t targets[IRQ_CONTROL_SLOTS], IrqEdgeCensus *c)
{
    MusashiBootMemory *memory = f->memory;
    unsigned rs = (word >> 21) & 31u, link = (word >> 11) & 31u;
    if (!rs || !link)
        return 1;
    for (unsigned i = 0; i < sizeof(irq_suppliers) / sizeof(irq_suppliers[0]); ++i) {
        uint32_t cell = irq_suppliers[i].cell, target;
        if (!musashi_boot_read32(memory, cell, &target))
            return 0;
        if (!target || (target & 3u))
            continue;
        FormatterCpu cpu = *site;
        cpu.r[rs] = target;
        if (!input_callback_call(memory, &cpu, word, target))
            continue;
        unsigned alternative = 0;
        while (alternative < IRQ_CONTROL_SLOTS && targets[alternative] == target)
            ++alternative;
        if (alternative == IRQ_CONTROL_SLOTS)
            return 0;
        uint32_t other = targets[alternative];
        if (!musashi_boot_write32(memory, cell, other))
            return 0;
        int original_lost = !input_callback_call(memory, &cpu, word, target);
        if (!musashi_boot_write32(memory, cell, target))
            return 0;
        if (!original_lost)
            continue;
        if (input_callback_call(memory, &cpu, word ^ (1u << 6), target) ||
            input_callback_call(memory, &cpu, word, target ^ 2u) ||
            input_callback_call(memory, &cpu, word, other) ||
            !irq_mutated_source_refused(memory, &cpu, word, target) ||
            !irq_single_call_credit(f, &cpu, target, link) ||
            !irq_remember_pair(c, site->pc, target))
            return 0;
        ++c->per_cell[i];
    }
    return 1;
}


static int irq_indirect_slot_seeds(MusashiBootMemory *memory, const FormatterCpu *cpu,
                                   uint32_t instruction, uint32_t targets[IRQ_CONTROL_SLOTS])
{
    unsigned source = (instruction >> 21) & 31u;
    for (unsigned i = 0; i < IRQ_CONTROL_SLOTS; ++i) {
        FormatterCpu c = *cpu;
        c.r[source] = targets[i];
        if (!musashi_boot_write32(memory, IRQ_CONTROL_SELECTOR, i) ||
            !input_callback_call(memory, &c, instruction, targets[i]))
            return 0;
    }
    return 1;
}

static int irq_refused_slot_effect(IrqFixture *f, const FormatterCpu *seed,
                                   uint32_t instruction, uint32_t rejected_target)
{
    FormatterCpu actual = *seed;
    unsigned from = (instruction >> 21) & 31u;
    unsigned link = (instruction >> 11) & 31u;
    uint32_t slot;
    if (!input_callback_word(f->memory, seed, seed->pc + 4u, &slot))
        return 0;
    /* Decode the observed store using the ISA, giving its base and value
     * independent test operands. Confirm the memory change survives the
     * refused resolved target. Restore the fixture cell on every path. */
    unsigned base = (slot >> 21) & 31u, value = (slot >> 16) & 31u;
    if ((slot >> 26) != 43u || !base || !value || base == value || base == from ||
        value == from || base == link || value == link)
        return 0;
    uint32_t delta = slot & UINT32_C(0xffff);
    if (delta & UINT32_C(0x8000))
        delta |= UINT32_C(0xffff0000);
    uint32_t old, observed;
    if (!musashi_boot_read32(f->memory, IRQ_CONTROL_SELECTOR, &old))
        return 0;
    uint32_t marker = old == 2u ? 1u : 2u;
    actual.r[base] = IRQ_CONTROL_SELECTOR - delta;
    actual.r[value] = marker;
    actual.r[from] = rejected_target;
    FormatterCpu origin = actual;
    unsigned remaining = 2;
    MusashiResetGraphPrefixStop stop = {0};
    int ok = input_callback_step(f->memory, &actual, &remaining, &stop) &&
             actual.pc == seed->pc + 4u && actual.npc == rejected_target &&
             musashi_boot_read32(f->memory, IRQ_CONTROL_SELECTOR, &observed) && observed == old &&
             input_callback_step(f->memory, &actual, &remaining, &stop) && !remaining &&
             actual.pc == rejected_target && !input_callback_call(f->memory, &origin, instruction, actual.pc) &&
             musashi_boot_read32(f->memory, IRQ_CONTROL_SELECTOR, &observed) && observed == marker;
    return musashi_boot_write32(f->memory, IRQ_CONTROL_SELECTOR, old) && ok;
}

static int irq_check_selector(IrqFixture *f, const FormatterCpu *seed, uint32_t word,
                              uint32_t targets[IRQ_CONTROL_SLOTS])
{
    MusashiBootMemory *memory = f->memory;
    unsigned source = (word >> 21) & 31u;
    unsigned link = (word >> 11) & 31u;
    if (!source || !link)
        return 0;
    for (unsigned i = 0; i < IRQ_CONTROL_SLOTS; ++i) {
        unsigned alternate = 0;
        while (alternate < IRQ_CONTROL_SLOTS && targets[alternate] == targets[i])
            ++alternate;
        if (alternate == IRQ_CONTROL_SLOTS)
            return 0;
        uint32_t target = targets[i], wrong = targets[alternate];
        uint32_t cell = IRQ_CONTROL_TABLE + 4u * i;
        FormatterCpu cpu = *seed;
        cpu.r[source] = target;
        if (!musashi_boot_write32(memory, IRQ_CONTROL_SELECTOR, i) ||
            !input_callback_call(memory, &cpu, word, target) ||
            input_callback_call(memory, &cpu, word ^ (1u << 6), target) ||
            input_callback_call(memory, &cpu, word, wrong) ||
            input_callback_call(memory, &cpu, word, target ^ 2u) ||
            !irq_mutated_source_refused(memory, &cpu, word, target) ||
            !irq_single_call_credit(f, &cpu, target, link))
            return 0;
        cpu.r[source] = wrong;
        FormatterCpu issued = cpu;
        unsigned one = 1;
        MusashiResetGraphPrefixStop end = {0};
        if (!input_callback_step(memory, &issued, &one, &end) ||
            issued.pc != cpu.pc + 4u || issued.npc != wrong || one ||
            input_callback_call(memory, &cpu, word, issued.npc))
            return 0;
        cpu.r[source] = target;
        if (!musashi_boot_write32(memory, IRQ_CONTROL_SELECTOR, alternate) ||
            input_callback_call(memory, &cpu, word, target) ||
            !musashi_boot_write32(memory, IRQ_CONTROL_SELECTOR, IRQ_CONTROL_SLOTS) ||
            input_callback_call(memory, &cpu, word, target) ||
            !musashi_boot_write32(memory, IRQ_CONTROL_SELECTOR, i) ||
            !musashi_boot_write32(memory, cell, wrong))
            return 0;
        int refused = !input_callback_call(memory, &cpu, word, target);
        if (!musashi_boot_write32(memory, cell, target) || !refused)
            return 0;
    }
    uint32_t mapped_but_unowned = UINT32_C(0x801ddd40);
    uint32_t mapped_word;
    if (!musashi_boot_read32(memory, mapped_but_unowned, &mapped_word) ||
        !musashi_boot_write32(memory, IRQ_CONTROL_SELECTOR, 0))
        return 0;
    FormatterCpu cpu = *seed;
    cpu.r[source] = mapped_but_unowned;
    if (input_callback_call(memory, &cpu, word, mapped_but_unowned))
        return 0;
    return irq_refused_slot_effect(f, &cpu, word, mapped_but_unowned);
}

static int irq_selector_cases(IrqFixture *f, const FormatterCpu *initial, unsigned *covered, unsigned *unique, uint32_t *issuer)
{
    IrqEdgeCensus census = {0};
    uint32_t targets[IRQ_CONTROL_SLOTS], original_selector;
    MusashiBootMemory *memory = f->memory;
    if (!musashi_boot_read32(memory, IRQ_CONTROL_SELECTOR, &original_selector))
        return 0;
    for (unsigned i = 0; i < IRQ_CONTROL_SLOTS; ++i) {
        if (!musashi_boot_read32(memory, IRQ_CONTROL_TABLE + 4u * i, &targets[i]) ||
            !targets[i] || (targets[i] & 3u))
            return 0;
    }
    unsigned sites = 0;
    int ok = 1;
    for (uint32_t offset = 0; offset < MUSASHI_RAM_SIZE; offset += 4) {
        uint32_t address = UINT32_C(0x80000000) + offset;
        uint32_t word, mounted;
        if (!musashi_boot_read32(memory, address, &word) ||
            (word >> 26) != 0 || (word & 63u) != 9u)
            continue;
        FormatterCpu cpu = *initial;
        cpu.pc = address;
        cpu.npc = address + 4u;
        cpu.branch_pc = 0;
        cpu.delay_slot = 0;
        if (!input_callback_word(memory, &cpu, address, &mounted) || mounted != word)
            continue;
        if (!irq_supplier_pairs(f, &cpu, word, targets, &census)) {
            ok = 0;
            break;
        }
        if (!irq_indirect_slot_seeds(memory, &cpu, word, targets))
            continue;
        if (!irq_check_selector(f, &cpu, word, targets)) {
            fprintf(stderr, "mounted indirect selector control failed at %08x\n", (unsigned)address);
            ok = 0;
            break;
        }
        *issuer = cpu.pc;
        for (unsigned i = 0; i < IRQ_CONTROL_SLOTS; ++i)
            if (!irq_remember_pair(&census, cpu.pc, targets[i]))
                ok = 0;
        ++sites;
    }
    if (!musashi_boot_write32(memory, IRQ_CONTROL_SELECTOR, original_selector))
        ok = 0;
    *covered = sites * IRQ_CONTROL_SLOTS;
    *unique = census.count;
    for (unsigned i = 0; i < sizeof(irq_suppliers) / sizeof(irq_suppliers[0]); ++i)
        if (census.per_cell[i] < irq_suppliers[i].witnessed)
            ok = 0;
    return ok && sites && census.count >= 13;
}


static int irq_outer_refused_selector(const uint8_t *exe, size_t size, uint32_t issuer)
{
    IrqFixture f;
    if (!issuer || !irq_open(&f, exe, size, 61))
        return 0;
    uint32_t outside = UINT32_C(0x801ddd40), data, before = 0, after = UINT32_MAX;
    uint32_t original[IRQ_CONTROL_SLOTS];
    unsigned installed = 0;
    int32_t result = -28409;
    MusashiResetGraphPrefixStop stop = {0};
    int ok = 0;
    if (!musashi_boot_read32(f.memory, outside, &data) ||
        !musashi_boot_read32(f.memory, IRQ_CONTROL_SELECTOR, &before))
        goto done;
    for (unsigned i = 0; i < IRQ_CONTROL_SLOTS; ++i) {
        uint32_t cell = IRQ_CONTROL_TABLE + 4u * i;
        if (!musashi_boot_read32(f.memory, cell, &original[i]) ||
            !musashi_boot_write32(f.memory, cell, outside))
            goto done;
        ++installed;
    }
    ok = !irq_invoke(&f, &result, &stop) && result == -28409 &&
         stop.call_address == issuer && stop.target_address == outside &&
         f.guarded.frame.faulted && !f.guarded.frame.busy &&
         musashi_boot_read32(f.memory, IRQ_CONTROL_SELECTOR, &after) &&
         after == before + 1u;
done:
    while (installed) {
        --installed;
        if (!musashi_boot_write32(f.memory, IRQ_CONTROL_TABLE + 4u * installed, original[installed]))
            ok = 0;
    }
    ok = ok && irq_bounds_ok(&f);
    if (!ok)
        fprintf(stderr, "outer rejected selector origin=%d target=%d result=%d selector=%u/%u bounds=%d\n",
                stop.call_address == issuer, stop.target_address == outside, result,
                (unsigned)before, (unsigned)after, irq_bounds_ok(&f));
    irq_close(&f);
    return ok;
}
static int irq_control_cases(const uint8_t *exe, size_t size)
{
    IrqFixture f;
    if (!irq_open(&f, exe, size, 103))
        return 0;
    FormatterCpu initial = {0};
    initial.bios_frame = &f.guarded.frame;
    initial.input_device = &f.input;
    initial.clock = &f.clock;
    initial.r[29] = MUSASHI_BIOS_CALLBACK_STACK_TOP;
    initial.r[30] = initial.r[29];
    unsigned direct = 0, owned = 0, selector = 0, pairs = 0;
    uint32_t leaf = 0, issuer = 0;
    int architectural = irq_synthetic_program(&initial);
    int direct_ok = irq_direct_cases(&f, &initial, 44, &direct, &owned, &leaf);
    int selector_ok = irq_selector_cases(&f, &initial, &selector, &pairs, &issuer);
    int outer = irq_outer_refused_selector(exe, size, issuer);
    int record = irq_refused_record(exe, size, leaf);
    int bounds = irq_bounds_ok(&f);
    fprintf(stderr, "mounted IRQ controls: architecture=%d owned-JAL=%u direct=%u/%d selector=%u/%d indirect=%u outer=%d return=%d record=%d bounds=%d\n",
            architectural, owned, direct, direct_ok, selector, selector_ok, pairs, outer, !!leaf, record, bounds);
    irq_close(&f);
    return architectural && direct_ok && selector_ok && outer && leaf && record && bounds;
}

static int irq_success_phase(const uint8_t *exe, size_t size, uint64_t phase, int check_wrap, int digital_only)
{
    IrqFixture f;
    if (!irq_open_profile(&f, exe, size, phase, digital_only))
        return 0;
    IrqProtocolHistory history = {0};
    IrqStockHistory stock_history = {0};
    int passed = 1;
    for (unsigned round = 0; round != 7; ++round) {
        irq_new_round(&f, (uint16_t)(UINT16_C(0xb219) + round * UINT16_C(0x647)));
        uint8_t prior_scratch[sizeof(f.guarded.frame.scratch)];
        memcpy(prior_scratch, f.guarded.frame.scratch, sizeof(prior_scratch));
        int32_t result = INT32_C(-9187);
        MusashiResetGraphPrefixStop stop = {0};
        int ran = irq_invoke(&f, &result, &stop);
        if (!ran || result != 0 || stop.call_address != UINT32_MAX || stop.target_address != UINT32_MAX ||
            f.guarded.frame.busy || f.guarded.frame.faulted || !irq_bounds_ok(&f) ||
            !f.attempts || f.accepted != f.attempts ||
            (round == 0 && !memcmp(prior_scratch, f.guarded.frame.scratch, sizeof(prior_scratch)))) {
            fprintf(stderr, "digital IRQ phase %llu round %u ended at %08x / %08x after %u steps (result %d)\n",
                    (unsigned long long)phase, round, (unsigned)stop.call_address,
                    (unsigned)stop.target_address, f.accepted, (int)result);
            passed = 0;
            break;
        }
        int observed = digital_only
            ? irq_protocol_round_ok(&f.trace, round) &&
              irq_protocol_packet_ok(f.memory->bytes, sizeof(f.memory->bytes), &f.trace, round, &history)
            : irq_stock_protocol_ok(&f.trace, f.memory->bytes, sizeof(f.memory->bytes), round, &stock_history);
        if (!observed) {
            passed = 0;
            break;
        }
    }
    if (!f.timer_reads || (check_wrap && !f.timer_wraps)) {
        fputs("digital IRQ callback did not observe the requested Timer2 phase\n", stderr);
        passed = 0;
    }
    irq_close(&f);
    return passed;
}

static int irq_other_interrupt(const uint8_t *exe, size_t size)
{
    IrqFixture f;
    if (!irq_open(&f, exe, size, 0))
        return 0;
    const uint32_t other_interrupt = UINT32_C(1) << 2;
    f.status = other_interrupt;
    irq_new_round(&f, UINT16_C(0xb52d));
    int32_t result = -813;
    MusashiResetGraphPrefixStop stop = {0};
    int ok = irq_invoke(&f, &result, &stop) && result == 0 &&
             stop.call_address == UINT32_MAX && stop.target_address == UINT32_MAX &&
             f.trace.acknowledge_count == 4 && f.status == other_interrupt &&
             !f.guarded.frame.busy && !f.guarded.frame.faulted && irq_bounds_ok(&f);
    irq_close(&f);
    if (!ok)
        fputs("digital IRQ acknowledgement changed an unrelated pending interrupt\n", stderr);
    return ok;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fputs("Supply the locally owned mapped executable path.\n", stderr);
        return EXIT_FAILURE;
    }
    FILE *file = fopen(argv[1], "rb");
    if (!file || fseek(file, 0, SEEK_END)) {
        if (file) fclose(file);
        return EXIT_FAILURE;
    }
    long size = ftell(file);
    if (size <= 0 || fseek(file, 0, SEEK_SET)) {
        fclose(file);
        return EXIT_FAILURE;
    }
    uint8_t *exe = malloc((size_t)size);
    int loaded = exe && fread(exe, 1, (size_t)size, file) == (size_t)size;
    fclose(file);
    if (!loaded) {
        free(exe);
        return EXIT_FAILURE;
    }

    int cold = irq_success_phase(exe, (size_t)size, 0, 0, 1);
    int stock_cold = irq_success_phase(exe, (size_t)size, 0, 0, 0);
    int wrapping = irq_success_phase(exe, (size_t)size, UINT64_C(65500) * 8, 1, 1);
    int stock_wrapping = irq_success_phase(exe, (size_t)size, UINT64_C(65500) * 8, 1, 0);
    int other_interrupt = irq_other_interrupt(exe, (size_t)size);
    int failures = irq_failure_cases(exe, (size_t)size);
    int controls = irq_control_cases(exe, (size_t)size);
    free(exe);
    return cold && stock_cold && wrapping && stock_wrapping && other_interrupt && failures && controls ? EXIT_SUCCESS : EXIT_FAILURE;
}
