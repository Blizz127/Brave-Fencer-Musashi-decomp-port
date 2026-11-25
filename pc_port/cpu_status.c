#include "musashi_cpu_status.h"

#include <stdlib.h>
#include <string.h>

/* Status carries three pairs of interrupt-enable / user-mode bits. */
enum {
    INTERRUPTS_ON = 1u,
    USER_MODE = 2u,
    PREVIOUS_INTERRUPTS_ON = 4u,
    EXTERNAL_IRQ = 1u << 10,
    CACHE_ISOLATE = 1u << 16,
    CACHE_SWAP = 1u << 17,
    GEOMETRY_COPROCESSOR = 1u << 30
};

struct MusashiCpuStatus {
    uint64_t (*thread_id)(void *);
    void *userdata;
    uint64_t owning_thread;
    MusashiCpuStatusSnapshot frame;
};

static int open_with_status(MusashiCpuStatus **out, uint64_t (*thread_id)(void *),
                            void *userdata, uint32_t initial)
{
    if (!out || *out || !thread_id)
        return 0;
    uint64_t id = thread_id(userdata);
    if (!id)
        return 0;

    MusashiCpuStatus *cpu = calloc(1, sizeof(*cpu));
    if (!cpu)
        return 0;
    cpu->owning_thread = id;
    cpu->thread_id = thread_id;
    cpu->userdata = userdata;
    cpu->frame.sr = initial;
    *out = cpu;
    return 1;
}

int musashi_cpu_status_open(MusashiCpuStatus **out, uint64_t (*current_thread)(void *),
                            void *userdata)
{
    return open_with_status(out, current_thread, userdata, GEOMETRY_COPROCESSOR);
}

#ifdef MUSASHI_CPU_STATUS_TESTING
int musashi_cpu_status_open_fixture(MusashiCpuStatus **out,
                                    uint64_t (*current_thread)(void *),
                                    void *userdata, uint32_t initial_sr)
{
    return open_with_status(out, current_thread, userdata, initial_sr);
}
#endif

static int on_owning_thread(const MusashiCpuStatus *cpu)
{
    return cpu && cpu->thread_id(cpu->userdata) == cpu->owning_thread;
}

static int refuse(MusashiCpuStatus *cpu)
{
    cpu->frame.faulted = 1;
    return 0;
}

static int ready(MusashiCpuStatus *cpu)
{
    return on_owning_thread(cpu) && !cpu->frame.faulted;
}

int musashi_cpu_status_close(MusashiCpuStatus *cpu)
{
    if (!on_owning_thread(cpu) || (cpu->frame.active && !cpu->frame.faulted))
        return 0;
    free(cpu);
    return 1;
}

int musashi_cpu_status_snapshot(MusashiCpuStatus *cpu, MusashiCpuStatusSnapshot *out)
{
    if (!on_owning_thread(cpu))
        return 0;
    if (!out)
        return refuse(cpu);
    *out = cpu->frame;
    return 1;
}

int musashi_cpu_status_read(MusashiCpuStatus *cpu, uint32_t *out)
{
    if (!ready(cpu))
        return 0;
    if (!out)
        return refuse(cpu);
    *out = cpu->frame.sr;
    return 1;
}

static int external_irq_enabled(uint32_t sr)
{
    const uint32_t mask = EXTERNAL_IRQ | INTERRUPTS_ON;
    return (sr & mask) == mask;
}

int musashi_cpu_status_irq_eligible(MusashiCpuStatus *cpu, int *out)
{
    if (!ready(cpu))
        return 0;
    if (!out)
        return refuse(cpu);
    *out = external_irq_enabled(cpu->frame.sr);
    return 1;
}

int musashi_cpu_status_set_enabled(MusashiCpuStatus *cpu, int enabled)
{
    if (!ready(cpu))
        return 0;
    if (cpu->frame.active || (enabled != 0 && enabled != 1))
        return refuse(cpu);
    const uint32_t mask = EXTERNAL_IRQ | INTERRUPTS_ON;
    if (enabled)
        cpu->frame.sr |= mask;
    else
        cpu->frame.sr &= ~mask;
    return 1;
}

/* A source supplies the current fetch and, in a delay slot, the immediately
 * preceding branch. A host service supplies registers without a fetch. */
static int valid_context(const MusashiCpuContext *c)
{
    if (!c || !c->identity || !c->gpr || c->gpr[0] != 0)
        return 0;
    if (c->provenance == MUSASHI_CPU_CONTEXT_HOST_BIOS_SERVICE)
        return !(c->pc || c->npc || c->branch_pc || c->instruction ||
                 c->instruction_valid || c->delay_slot);
    if (c->provenance != MUSASHI_CPU_CONTEXT_SOURCE || c->instruction_valid != 1 ||
        ((c->pc | c->npc) & 3u))
        return 0;
    if (c->delay_slot == 0)
        return c->branch_pc == 0;
    return c->delay_slot == 1 && c->pc >= 4 && c->branch_pc == c->pc - 4;
}

int musashi_cpu_status_write_source(MusashiCpuStatus *cpu, const MusashiCpuContext *c,
                                    uint32_t value)
{
    if (!ready(cpu))
        return 0;
    if (cpu->frame.active || !valid_context(c) || c->provenance != MUSASHI_CPU_CONTEXT_SOURCE)
        return refuse(cpu);

    /* MTC0 takes rt as its operand. Only the Status destination is exposed. */
    uint32_t word = c->instruction;
    unsigned rt = (word >> 16) & 31u;
    if ((word >> 26) != 16 || ((word >> 21) & 31u) != 4 ||
        ((word >> 11) & 31u) != 12 || (word & 2047u) != 0 ||
        value != c->gpr[rt])
        return refuse(cpu);
    uint32_t old = cpu->frame.sr;
    /* The movie sets SwC with cache isolation off. Latch that single bit;
     * changing IsC or clearing SwC needs its own cache behavior. */
    int setting_swap = !(old & (USER_MODE | CACHE_ISOLATE)) && value == (old | CACHE_SWAP);
    if (value != old && value != (old | GEOMETRY_COPROCESSOR) && !setting_swap)
        return refuse(cpu);
    cpu->frame.sr = value;
    return 1;
}

int musashi_cpu_status_begin(MusashiCpuStatus *cpu, MusashiCpuExceptionKind kind,
                             const MusashiCpuContext *c, uint32_t hardware_pending,
                             MusashiCpuExceptionToken *out)
{
    if (!ready(cpu))
        return 0;
    MusashiCpuStatusSnapshot *f = &cpu->frame;
    if (!out || f->active || f->sequence == UINT64_MAX || (f->sr & USER_MODE) ||
        !valid_context(c) || (hardware_pending != 0 && hardware_pending != EXTERNAL_IRQ))
        return refuse(cpu);

    int source = c->provenance == MUSASHI_CPU_CONTEXT_SOURCE;
    uint32_t cause = hardware_pending;
    switch (kind) {
    case MUSASHI_CPU_EXCEPTION_IRQ:
        if (!hardware_pending || !external_irq_enabled(f->sr))
            return refuse(cpu);
        break;
    case MUSASHI_CPU_EXCEPTION_SYS1:
    case MUSASHI_CPU_EXCEPTION_SYS2: {
        unsigned selector = kind == MUSASHI_CPU_EXCEPTION_SYS1 ? 1u : 2u;
        /* SPECIAL function 12, with the port's supported zero syscall code. */
        if (source && (c->instruction != 12 || c->gpr[4] != selector))
            return refuse(cpu);
        cause |= 8u << 2;
        break;
    }
    default:
        return refuse(cpu);
    }

    uint32_t entered = (f->sr & ~63u) | ((f->sr << 2) & 63u);
    *f = (MusashiCpuStatusSnapshot){
        .sr = entered, .saved_sr = entered, .pc = c->pc, .npc = c->npc,
        .instruction = c->instruction, .branch_pc = c->branch_pc,
        .sequence = f->sequence + 1, .identity = c->identity,
        .provenance = c->provenance, .kind = kind,
        .instruction_valid = c->instruction_valid, .delay_slot = c->delay_slot,
        .cause_epc_valid = source, .active = 1
    };
    memcpy(f->gpr, c->gpr, sizeof(f->gpr));
    if (source) {
        f->cause = cause | ((uint32_t)c->delay_slot << 31);
        f->epc = c->delay_slot ? c->branch_pc : c->pc;
    }
    *out = (MusashiCpuExceptionToken){
        .owner = cpu, .identity = f->identity, .sequence = f->sequence
    };
    return 1;
}

static int has_token(const MusashiCpuStatus *cpu, const MusashiCpuExceptionToken *token)
{
    const MusashiCpuStatusSnapshot *f = &cpu->frame;
    return f->active && token && token->owner == cpu && token->identity == f->identity &&
           token->sequence == f->sequence;
}

int musashi_cpu_status_inspect(MusashiCpuStatus *cpu, const MusashiCpuExceptionToken *token,
                               MusashiCpuStatusSnapshot *out)
{
    if (!on_owning_thread(cpu))
        return 0;
    if (!out || !has_token(cpu, token))
        return refuse(cpu);
    *out = cpu->frame;
    return 1;
}

int musashi_cpu_status_accept_kernel_entry(MusashiCpuStatus *cpu,
                                           const MusashiCpuExceptionToken *token)
{
    if (!ready(cpu))
        return 0;
    if (!has_token(cpu, token) || cpu->frame.kernel_entered)
        return refuse(cpu);
    cpu->frame.kernel_entered = 1;
    return 1;
}

int musashi_cpu_status_apply_sys(MusashiCpuStatus *cpu, const MusashiCpuExceptionToken *token,
                                 int32_t *result)
{
    if (!ready(cpu))
        return 0;
    MusashiCpuStatusSnapshot *f = &cpu->frame;
    if (!has_token(cpu, token) || !f->kernel_entered || f->sys_applied)
        return refuse(cpu);

    const uint32_t return_enable = EXTERNAL_IRQ | PREVIOUS_INTERRUPTS_ON;
    switch (f->kind) {
    case MUSASHI_CPU_EXCEPTION_SYS1:
        if (!result)
            return refuse(cpu);
        *result = (f->saved_sr & return_enable) == return_enable;
        f->saved_sr &= ~return_enable;
        break;
    case MUSASHI_CPU_EXCEPTION_SYS2:
        f->saved_sr |= return_enable;
        break;
    default:
        return refuse(cpu);
    }
    f->sys_applied = 1;
    return 1;
}

int musashi_cpu_status_finish(MusashiCpuStatus *cpu, const MusashiCpuExceptionToken *token)
{
    if (!ready(cpu))
        return 0;
    MusashiCpuStatusSnapshot *f = &cpu->frame;
    if (!has_token(cpu, token) || !f->kernel_entered ||
        (f->kind != MUSASHI_CPU_EXCEPTION_IRQ && !f->sys_applied))
        return refuse(cpu);

    /* RFE copies the previous and old mode pairs down by one position.
     * The old pair remains in the top position of the mode stack. */
    f->sr = (f->saved_sr & ~15u) | ((f->saved_sr >> 2) & 15u);
    f->active = 0;
    return 1;
}

int musashi_cpu_status_fault(MusashiCpuStatus *cpu, const MusashiCpuExceptionToken *token)
{
    if (!on_owning_thread(cpu))
        return 0;
    int accepted = has_token(cpu, token);
    cpu->frame.faulted = 1;
    return accepted;
}
