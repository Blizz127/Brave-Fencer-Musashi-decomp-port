#include "musashi_cpu_status.h"
#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CPU status line %d: %s\n", __LINE__, #condition); \
        return 0; \
    } \
} while (0)

enum { PENDING = 1u << 10, ENABLE = 1, MODE_BITS = 63, SYSCALL_FUNCTION = 12 };
static const uint32_t cu2 = UINT32_C(1) << 30;

typedef struct Fixture {
    uint64_t thread;
    uint32_t registers[32];
    MusashiCpuStatus *status;
    MusashiCpuContext context;
    MusashiCpuExceptionToken token;
} Fixture;

static uint64_t current_thread(void *user)
{
    return *(const uint64_t *)user;
}

static int open_frame(Fixture *f, uint32_t status,
                      MusashiCpuContextProvenance provenance, int delay)
{
    memset(f, 0, sizeof(*f));
    f->thread = 13;
    for (unsigned i = 1; i < 32; ++i)
        f->registers[i] = (i + 61u) * 713u;
    f->context.identity = f;
    f->context.gpr = f->registers;
    f->context.provenance = provenance;
    if (provenance == MUSASHI_CPU_CONTEXT_SOURCE) {
        f->context.pc = 91524u;
        f->context.npc = f->context.pc + (delay ? 156u : 4u);
        f->context.branch_pc = delay ? f->context.pc - 4u : 0u;
        f->context.delay_slot = delay;
        f->context.instruction_valid = 1;
        /* The IRQ path only records the fetched word. */
        f->context.instruction = 613u * 91u;
    }
    return musashi_cpu_status_open_fixture(&f->status, current_thread, &f->thread, status);
}

/* Each level of the MIPS mode stack contains a pair of bits. */
static unsigned mode_pair(uint32_t status, unsigned place)
{
    return (status / place) % 4u;
}

static int round_trip(uint32_t initial, MusashiCpuExceptionKind kind,
                      MusashiCpuContextProvenance provenance, int delay)
{
    Fixture f;
    MusashiCpuStatusSnapshot state;
    uint32_t owned_copy[32], observed;
    int eligible;
    int32_t result = 1783;
    const int irq = kind == MUSASHI_CPU_EXCEPTION_IRQ;
    const int source = provenance == MUSASHI_CPU_CONTEXT_SOURCE;
    unsigned current = mode_pair(initial, 1), previous = mode_pair(initial, 4);
    uint32_t entered = (initial & ~(uint32_t)MODE_BITS) + 16u * previous + 4u * current;
    uint32_t to_restore = entered;
    REQUIRE(open_frame(&f, initial, provenance, delay));
    REQUIRE(musashi_cpu_status_read(f.status, &observed) && observed == initial);
    REQUIRE(musashi_cpu_status_irq_eligible(f.status, &eligible));
    REQUIRE(eligible == ((initial & (PENDING | ENABLE)) == (PENDING | ENABLE)));
    if (!irq) {
        f.registers[4] = (kind == MUSASHI_CPU_EXCEPTION_SYS1) ? 1u : 2u;
        if (source)
            f.context.instruction = SYSCALL_FUNCTION;
    }
    memcpy(owned_copy, f.registers, sizeof(owned_copy));
    if (current >= 2) {
        REQUIRE(!musashi_cpu_status_begin(f.status, kind, &f.context, irq ? PENDING : 0, &f.token));
        REQUIRE(musashi_cpu_status_close(f.status));
        return 1;
    }
    REQUIRE(musashi_cpu_status_begin(f.status, kind, &f.context, irq ? PENDING : 0, &f.token));
    REQUIRE(f.token.owner == f.status && f.token.identity == &f && f.token.sequence != 0);
    memset(f.registers, 0xcc, sizeof(f.registers));
    REQUIRE(musashi_cpu_status_inspect(f.status, &f.token, &state));
    REQUIRE(state.active && !state.faulted && !state.kernel_entered && !state.sys_applied);
    REQUIRE(state.kind == kind && state.provenance == provenance && state.identity == &f);
    REQUIRE(state.sr == entered && state.saved_sr == entered);
    REQUIRE(memcmp(owned_copy, state.gpr, sizeof(owned_copy)) == 0);
    REQUIRE(state.instruction_valid == source && state.delay_slot == delay);
    REQUIRE(state.instruction == f.context.instruction && state.pc == f.context.pc);
    REQUIRE(state.npc == f.context.npc && state.branch_pc == f.context.branch_pc);
    REQUIRE(state.cause_epc_valid == source);
    REQUIRE(state.epc == (delay ? f.context.branch_pc : f.context.pc));
    uint32_t cause = irq ? PENDING : 8u * 4u;
    if (delay)
        cause |= UINT32_C(1) << 31;
    REQUIRE(state.cause == (source ? cause : 0u));
    REQUIRE(musashi_cpu_status_accept_kernel_entry(f.status, &f.token));
    if (!irq) {
        REQUIRE(musashi_cpu_status_apply_sys(f.status, &f.token, &result));
        if (kind == MUSASHI_CPU_EXCEPTION_SYS1) {
            REQUIRE(result == eligible);
            to_restore &= ~(uint32_t)(PENDING | 4u);
        } else {
            REQUIRE(result == 1783);
            to_restore |= PENDING | 4u;
        }
        REQUIRE(musashi_cpu_status_inspect(f.status, &f.token, &state));
        REQUIRE(state.sys_applied && state.saved_sr == to_restore);
    }
    REQUIRE(musashi_cpu_status_finish(f.status, &f.token));
    REQUIRE(musashi_cpu_status_snapshot(f.status, &state));
    REQUIRE(!state.active && !state.faulted && state.sequence == f.token.sequence);
    uint32_t after = (to_restore & ~UINT32_C(15)) +
                     4u * mode_pair(to_restore, 16) + mode_pair(to_restore, 4);
    REQUIRE(state.sr == after);
    REQUIRE(musashi_cpu_status_close(f.status));
    return 1;
}

enum InvalidOperation {
    OTHER_IDENTITY, OTHER_OWNER, STALE_SEQUENCE, EARLY_FINISH, EARLY_APPLY,
    REPEATED_APPLY, IRQ_AS_SYS, EXPLICIT_FAULT
};

static int exception_refusal(enum InvalidOperation operation)
{
    Fixture f;
    MusashiCpuStatusSnapshot state;
    MusashiCpuExceptionToken key;
    MusashiCpuExceptionKind kind = operation == EARLY_APPLY || operation == REPEATED_APPLY
        ? MUSASHI_CPU_EXCEPTION_SYS2 : MUSASHI_CPU_EXCEPTION_IRQ;
    REQUIRE(open_frame(&f, PENDING | ENABLE, MUSASHI_CPU_CONTEXT_HOST_BIOS_SERVICE, 0));
    f.registers[4] = 2;
    REQUIRE(musashi_cpu_status_begin(f.status, kind, &f.context,
            kind == MUSASHI_CPU_EXCEPTION_IRQ ? PENDING : 0, &f.token));
    key = f.token;
    switch (operation) {
    case OTHER_IDENTITY: case OTHER_OWNER: case STALE_SEQUENCE:
        if (operation == OTHER_IDENTITY) key.identity = f.registers;
        if (operation == OTHER_OWNER) key.owner = NULL;
        if (operation == STALE_SEQUENCE) ++key.sequence;
        REQUIRE(!musashi_cpu_status_inspect(f.status, &key, &state));
        REQUIRE(!musashi_cpu_status_accept_kernel_entry(f.status, &key));
        break;
    case EARLY_FINISH:
        REQUIRE(!musashi_cpu_status_finish(f.status, &key));
        break;
    case EARLY_APPLY:
        REQUIRE(!musashi_cpu_status_apply_sys(f.status, &key, NULL));
        break;
    case REPEATED_APPLY:
        REQUIRE(musashi_cpu_status_accept_kernel_entry(f.status, &key));
        REQUIRE(musashi_cpu_status_apply_sys(f.status, &key, NULL));
        REQUIRE(!musashi_cpu_status_apply_sys(f.status, &key, NULL));
        break;
    case IRQ_AS_SYS:
        REQUIRE(musashi_cpu_status_accept_kernel_entry(f.status, &key));
        REQUIRE(!musashi_cpu_status_apply_sys(f.status, &key, NULL));
        break;
    case EXPLICIT_FAULT:
        REQUIRE(musashi_cpu_status_fault(f.status, &key));
        break;
    }
    REQUIRE(musashi_cpu_status_inspect(f.status, &f.token, &state));
    REQUIRE(state.active && state.faulted);
    REQUIRE(!musashi_cpu_status_finish(f.status, &f.token));
    REQUIRE(musashi_cpu_status_close(f.status));
    return 1;
}

enum InvalidInput {
    NO_IDENTITY, NO_REGISTERS, UNKNOWN_PROVENANCE, HOST_PC, HOST_NPC, HOST_WORD,
    HOST_VALID, HOST_DELAY, HOST_BRANCH, SOURCE_UNFETCHED, BAD_SYS_SELECTOR,
    BAD_SYS_WORD, IRQ_ABSENT, DEVICE_BIT, IRQ_MASKED
};

static int rejects_begin(enum InvalidInput input)
{
    Fixture f;
    MusashiCpuExceptionKind kind = MUSASHI_CPU_EXCEPTION_IRQ;
    uint32_t pending = PENDING;
    uint32_t status = input == IRQ_MASKED ? ENABLE : PENDING | ENABLE;
    int source = input >= SOURCE_UNFETCHED && input <= BAD_SYS_WORD;
    REQUIRE(open_frame(&f, status, source ? MUSASHI_CPU_CONTEXT_SOURCE :
                                       MUSASHI_CPU_CONTEXT_HOST_BIOS_SERVICE, 0));
    switch (input) {
    case NO_IDENTITY: f.context.identity = NULL; break;
    case NO_REGISTERS: f.context.gpr = NULL; break;
    case UNKNOWN_PROVENANCE: f.context.provenance = (MusashiCpuContextProvenance)7; break;
    case HOST_PC: f.context.pc = 840u; break;
    case HOST_NPC: f.context.npc = 484u; break;
    case HOST_WORD: f.context.instruction = 562u; break;
    case HOST_VALID: f.context.instruction_valid = 1; break;
    case HOST_DELAY: f.context.delay_slot = 1; break;
    case HOST_BRANCH: f.context.branch_pc = 320u; break;
    case SOURCE_UNFETCHED: f.context.instruction_valid = 0; break;
    case BAD_SYS_SELECTOR:
        kind = MUSASHI_CPU_EXCEPTION_SYS1;
        pending = 0;
        f.context.instruction = SYSCALL_FUNCTION;
        f.registers[4] = 2;
        break;
    case BAD_SYS_WORD:
        kind = MUSASHI_CPU_EXCEPTION_SYS2;
        pending = 0;
        f.registers[4] = 2;
        break;
    case IRQ_ABSENT: pending = 0; break;
    case DEVICE_BIT: pending = 1; break;
    case IRQ_MASKED: break;
    }
    MusashiCpuExceptionToken output = {NULL, &f.thread, 938u};
    REQUIRE(!musashi_cpu_status_begin(f.status, kind, &f.context, pending, &output));
    REQUIRE(output.owner == NULL && output.identity == &f.thread && output.sequence == 938u);
    MusashiCpuStatusSnapshot state;
    REQUIRE(musashi_cpu_status_snapshot(f.status, &state));
    REQUIRE(!state.active && state.sr == status);
    REQUIRE(musashi_cpu_status_close(f.status));
    return 1;
}

static int ownership_and_status_write(void)
{
    uint64_t thread = 52;
    uint32_t result;
    MusashiCpuStatus *normal = NULL;
    Fixture f;
    REQUIRE(musashi_cpu_status_open(&normal, current_thread, &thread));
    REQUIRE(musashi_cpu_status_read(normal, &result) && result == cu2);
    REQUIRE(musashi_cpu_status_set_enabled(normal, 1));
    REQUIRE(musashi_cpu_status_read(normal, &result) && result == (cu2 | PENDING | ENABLE));
    ++thread;
    result = 919u;
    REQUIRE(!musashi_cpu_status_read(normal, &result) && result == 919u);
    REQUIRE(!musashi_cpu_status_close(normal));
    --thread;
    REQUIRE(musashi_cpu_status_set_enabled(normal, 0));
    REQUIRE(musashi_cpu_status_read(normal, &result) && result == cu2);
    REQUIRE(musashi_cpu_status_close(normal));
    REQUIRE(open_frame(&f, 0u, MUSASHI_CPU_CONTEXT_SOURCE, 0));
    /* Assemble MTC0 of register 25 into Status using architectural fields. */
    f.context.instruction = (16u << 26) | (4u << 21) | (25u << 16) | (12u << 11);
    f.registers[25] = 0u;
    REQUIRE(musashi_cpu_status_write_source(f.status, &f.context, 0u));
    f.registers[25] = cu2;
    REQUIRE(musashi_cpu_status_write_source(f.status, &f.context, cu2));
    REQUIRE(musashi_cpu_status_read(f.status, &result) && result == cu2);
    f.registers[25] = 0;
    REQUIRE(!musashi_cpu_status_write_source(f.status, &f.context, 0u));
    MusashiCpuStatusSnapshot failed;
    REQUIRE(musashi_cpu_status_snapshot(f.status, &failed));
    REQUIRE(failed.faulted && failed.sr == cu2);
    REQUIRE(musashi_cpu_status_close(f.status));
    return 1;
}

/* Exercise Status through a fetched MTC0 with its actual live register. */
static const uint32_t mtc0_status = (16u << 26) | (4u << 21) | (11u << 16) | (12u << 11);

static int swap_cache_write(void)
{
    Fixture f;
    uint32_t observed;
    int eligible;
    uint32_t status = cu2 | PENDING | ENABLE;
    const uint32_t swap_cache = 1u << 17;
    REQUIRE(open_frame(&f, status, MUSASHI_CPU_CONTEXT_SOURCE, 0));
    f.context.instruction = mtc0_status;
    f.registers[11] = status | swap_cache;
    REQUIRE(musashi_cpu_status_write_source(f.status, &f.context, f.registers[11]));
    REQUIRE(musashi_cpu_status_read(f.status, &observed) && observed == (status | swap_cache));
    REQUIRE(musashi_cpu_status_write_source(f.status, &f.context, f.registers[11]));
    REQUIRE(musashi_cpu_status_irq_eligible(f.status, &eligible) && eligible);
    REQUIRE(musashi_cpu_status_begin(f.status, MUSASHI_CPU_EXCEPTION_IRQ, &f.context,
                                     PENDING, &f.token));
    REQUIRE(musashi_cpu_status_accept_kernel_entry(f.status, &f.token));
    REQUIRE(musashi_cpu_status_finish(f.status, &f.token));
    REQUIRE(musashi_cpu_status_read(f.status, &observed) && observed == (status | swap_cache));
    REQUIRE(musashi_cpu_status_close(f.status));
    return 1;
}

static int swap_cache_refusals(void)
{
    const uint32_t status = cu2 | PENDING | ENABLE;
    const uint32_t swap = 1u << 17, isolate = 1u << 16, user = 2u;
    const struct { uint32_t before, requested; } changes[] = {
        {status | swap, status},                     /* Clear SwC. */
        {status, (status | swap) ^ cu2},             /* Disable CU2. */
        {status ^ cu2, status | swap},               /* Enable both bits. */
        {status, (status | swap) ^ PENDING},         /* Change IRQ mask. */
        {status, status | swap | isolate},           /* Enter cache isolation. */
        {status | isolate, status | isolate | swap}, /* Already isolated. */
        {status | user, status | user | swap},       /* User-mode write. */
    };
    Fixture f;
    MusashiCpuStatusSnapshot snapshot;
    for (unsigned i = 0; i < sizeof(changes) / sizeof(changes[0]); ++i) {
        REQUIRE(open_frame(&f, changes[i].before, MUSASHI_CPU_CONTEXT_SOURCE, 0));
        f.context.instruction = mtc0_status;
        f.registers[11] = changes[i].requested;
        REQUIRE(!musashi_cpu_status_write_source(f.status, &f.context, changes[i].requested));
        REQUIRE(musashi_cpu_status_snapshot(f.status, &snapshot));
        REQUIRE(snapshot.sr == changes[i].before && snapshot.faulted);
        REQUIRE(musashi_cpu_status_close(f.status));
    }

    enum { OPERAND, UNFETCHED, DESTINATION, HOST_CONTEXT, ACTIVE_IRQ };
    for (int input = OPERAND; input <= ACTIVE_IRQ; ++input) {
        int host = input == HOST_CONTEXT;
        REQUIRE(open_frame(&f, status, host ? MUSASHI_CPU_CONTEXT_HOST_BIOS_SERVICE :
                                             MUSASHI_CPU_CONTEXT_SOURCE, 0));
        if (!host)
            f.context.instruction = mtc0_status;
        if (input == UNFETCHED)
            f.context.instruction_valid = 0;
        if (input == DESTINATION)
            f.context.instruction ^= 1u << 11;
        if (input == ACTIVE_IRQ)
            REQUIRE(musashi_cpu_status_begin(f.status, MUSASHI_CPU_EXCEPTION_IRQ, &f.context,
                                             PENDING, &f.token));
        MusashiCpuStatusSnapshot before;
        REQUIRE(musashi_cpu_status_snapshot(f.status, &before));
        uint32_t requested = before.sr | swap;
        f.registers[11] = input == OPERAND ? before.sr : requested;
        REQUIRE(!musashi_cpu_status_write_source(f.status, &f.context, requested));
        REQUIRE(musashi_cpu_status_snapshot(f.status, &snapshot));
        REQUIRE(snapshot.faulted && snapshot.sr == before.sr && snapshot.saved_sr == before.saved_sr);
        REQUIRE(snapshot.active == before.active && snapshot.sequence == before.sequence);
        REQUIRE(musashi_cpu_status_close(f.status));
    }
    return 1;
}

int main(void)
{
    for (unsigned pair_stack = 0; pair_stack <= MODE_BITS; ++pair_stack) {
        uint32_t seed = UINT32_C(0x25a00000) | pair_stack;
        for (unsigned mask = 0; mask <= 1; ++mask) {
            uint32_t status = seed | (mask ? PENDING : 0);
            for (unsigned kind = MUSASHI_CPU_EXCEPTION_IRQ; kind <= MUSASHI_CPU_EXCEPTION_SYS2; ++kind) {
                if (kind == MUSASHI_CPU_EXCEPTION_IRQ && (status & (PENDING | ENABLE)) != (PENDING | ENABLE))
                    continue;
                if (!round_trip(status, kind, MUSASHI_CPU_CONTEXT_HOST_BIOS_SERVICE, 0) ||
                    !round_trip(status, kind, MUSASHI_CPU_CONTEXT_SOURCE, 0) ||
                    !round_trip(status, kind, MUSASHI_CPU_CONTEXT_SOURCE, 1))
                    return 1;
            }
        }
    }
    for (int action = OTHER_IDENTITY; action <= EXPLICIT_FAULT; ++action)
        if (!exception_refusal(action)) return 1;
    for (int input = NO_IDENTITY; input <= IRQ_MASKED; ++input)
        if (!rejects_begin(input)) return 1;
    if (!ownership_and_status_write() || !swap_cache_write() || !swap_cache_refusals()) return 1;
    puts("CPU_STATUS_PASS synthetic_contexts=1");
    return 0;
}
