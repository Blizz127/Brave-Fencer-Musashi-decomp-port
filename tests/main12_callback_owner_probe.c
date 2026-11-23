/* Controlled guest-CPU entry and ownership checks for the first cold callback.
 * Instructions and initial state are read only from the tester's USA disc. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "musashi_main12.h"
#include "musashi_disc_media.h"
#include "musashi_guest_irq.h"
#include "musashi_gte_owner.h"
/* During an authenticated slot reset instruction, observe the formatter's
 * scalar requests in the slot bank. In particular, an SH still requests
 * its halfword when the addressed slot is already inactive. */
#include "musashi_boot_memory.h"
static struct {
    MusashiBootMemory *memory;
    uint32_t first, limit, address;
    size_t width;
    unsigned count;
    uint32_t witness_address;
    uint8_t *witness;
} slot_ram_access;

static uint8_t *slot_probe_ram_span(MusashiBootMemory *memory, uint32_t address, size_t size)
{
    uint8_t *span = musashi_boot_ram_span(memory, address, size);
    if (memory == slot_ram_access.memory && size <= 4u &&
        address >= slot_ram_access.first && address < slot_ram_access.limit) {
        ++slot_ram_access.count;
        slot_ram_access.address = address;
        slot_ram_access.width = size;
        if (span && slot_ram_access.witness && size == sizeof(uint16_t) &&
            address == slot_ram_access.witness_address)
            return slot_ram_access.witness;
    }
    return span;
}

#define musashi_boot_ram_span slot_probe_ram_span
#include "../pc_port/mips_formatter.c"
#undef musashi_boot_ram_span

#include "musashi_sha256_digest.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "main12 test line %d: %s\n", __LINE__, #expr); exit(1); \
} } while (0)

enum {
    EXE_SIZE = 0x65000, MAIN_SIZE = 5988352, MEMBER_SIZE = 548864,
    TRANSFER_OFFSET = 0x28000, TRANSFER_SIZE = 0x5db28
};
static const uint32_t load_address = 0x80128158u;
static const uint32_t installer = 0x8017bf38u;
static const uint32_t callback_pc = 0x8017c080u;
static const uint32_t handler_pc = 0x8017c0dcu;
static const uint32_t dispatch_slot = 0x8011db24u;
static const uint32_t return_pc = 0x80011000u;
static const char member_hash[] =
    "7bf686da174e6bd98cab8e742af629ba4168cff76e3be4e367cb69f783af070c";
static const char initial_hash[] =
    "e50f2d180b3282cefa29f9f69fd1b3febaf8734e8070efc337a03891478f2067";
static const char installed_hash[] =
    "b811d5e11f6729666e37fc8489ec1b330c3433ce93e55e564da14c336dde30d1";

static MusashiBootMemory static_machine, other_machine, before_step;
static MusashiBootMemory *test_machine = &static_machine;
#define machine (*test_machine)
static int mapped_window;
static MusashiGuestIrqImage irq, other_irq;
static MusashiMain12Owner *owner;
static MusashiCallbackDevice callbacks;
static MusashiDiscMedia *media;
static unsigned char program[EXE_SIZE], archive[MAIN_SIZE];
static const unsigned char *transfer;
static unsigned clock_calls;
static uint64_t cycles;

static void check_digest(const void *data, size_t size, const char *expected)
{
    unsigned char digest[32];
    char hex[65];
    CHECK(musashi_sha256_digest(data, size, digest));
    for (unsigned i = 0; i < sizeof digest; ++i)
        snprintf(hex + i * 2, 3, "%02x", digest[i]);
    CHECK(strcmp(hex, expected) == 0);
}
static uint32_t little32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void load_media(const char *path, int check_stdin)
{
    char why[160];
    size_t actual;
    media = musashi_disc_media_open_image(path, why, sizeof why);
    if (!media) {
        fprintf(stderr, "Cannot open supported disc: %s\n", why);
        exit(1);
    }
    CHECK(musashi_disc_media_read_root_file(media, "SLUS_007.26", program, sizeof program, &actual));
    CHECK(actual == sizeof program);
    CHECK(musashi_disc_media_read_root_file(media, "MAIN.CD", archive, sizeof archive, &actual));
    CHECK(actual == sizeof archive && little32(archive) == 49);
    const unsigned char *row = archive + 8 + 12 * 8;
    uint32_t sector = little32(row);
    CHECK(little32(row + 4) == MEMBER_SIZE && sector <= (sizeof archive - MEMBER_SIZE) / 2048u);
    const unsigned char *member = archive + sector * 2048u;
    check_digest(member, MEMBER_SIZE, member_hash);
    CHECK(TRANSFER_OFFSET + TRANSFER_SIZE <= MEMBER_SIZE);
    transfer = member + TRANSFER_OFFSET;
    check_digest(transfer, TRANSFER_SIZE, initial_hash);
    if (check_stdin) {
#ifdef _WIN32
        CHECK(_setmode(_fileno(stdin), _O_BINARY) != -1);
#endif
        unsigned char block[4096];
        for (size_t off = 0; off < TRANSFER_SIZE;) {
            size_t count = TRANSFER_SIZE - off;
            if (count > sizeof block) count = sizeof block;
            CHECK(fread(block, 1, count, stdin) == count && !memcmp(block, transfer + off, count));
            off += count;
        }
        CHECK(fgetc(stdin) == EOF && !ferror(stdin));
    }
}
static int advance(void *user, uint32_t amount)
{
    (void)user;
    ++clock_calls; cycles += amount;
    return 1;
}
static MusashiExecutionClock clock_source = {NULL, advance};
static FormatterCpu entry(uint32_t pc)
{
    FormatterCpu cpu = {0};
    cpu.pc = pc; cpu.npc = pc + 4u;
    cpu.r[29] = 0x801ff800u; cpu.r[31] = return_pc;
    cpu.clock = &clock_source; cpu.callback_device = &callbacks;
    return cpu;
}
static uint8_t *span(MusashiBootMemory *host)
{
    uint8_t *result = musashi_boot_ram_span(host, load_address, TRANSFER_SIZE);
    CHECK(result);
    return result;
}
static void mount_machine(MusashiBootMemory *host, MusashiGuestIrqImage *identity)
{
    memset(host, 0, sizeof *host);
    CHECK(musashi_boot_map_irq_exe(identity, host, program, sizeof program));
    memcpy(span(host), transfer, TRANSFER_SIZE);
}
static void install_by_guest(void)
{
    FormatterCpu cpu = entry(installer);
    uint32_t slot;
    for (unsigned i = 0; i < 93; ++i) {
        CHECK(cpu.pc != return_pc);
        CHECK(formatter_step(&machine, &cpu));
    }
    CHECK(cpu.pc == return_pc && cpu.npc == return_pc + 4u);
    CHECK(musashi_boot_read32(&machine, dispatch_slot, &slot) && slot == callback_pc);
    check_digest(span(&machine), TRANSFER_SIZE, installed_hash);
}
static void expect_fetch(MusashiBootMemory *host)
{
    FormatterCpu cpu = entry(callback_pc);
    uint32_t word, live_word;
    CHECK(formatter_fetch(host, &cpu, &word));
    CHECK(musashi_boot_read32(host, callback_pc, &live_word) && word == live_word);
}
static void seed_machine(void)
{
    callbacks.main12_owner = NULL;
    if (owner) musashi_main12_close(owner);
    owner = NULL;
    mount_machine(&machine, &irq);
    owner = musashi_main12_open(&irq, &machine, media);
    CHECK(owner);
    callbacks.main12_owner = owner;
}
static void expect_refused(MusashiBootMemory *host, FormatterCpu *cpu)
{
    FormatterCpu saved;
    memcpy(&saved, cpu, sizeof saved);
    memcpy(&before_step, host, sizeof before_step);
    uint32_t word = 0x831267a5u;
    unsigned before_calls = clock_calls;
    uint64_t before_cycles = cycles;
    CHECK(!formatter_fetch(host, cpu, &word) && word == 0x831267a5u);
    /* A real step selects this memory for SOURCE context, including refusal. */
    CHECK(!formatter_step(host, cpu));
    MusashiCpuContext context;
    unsigned char initial[sizeof context];
    memset(&context, 0x69, sizeof context);
    memcpy(initial, &context, sizeof initial);
    CHECK(!musashi_boot_cpu_context(cpu, MUSASHI_CPU_CONTEXT_SOURCE, &context));
    CHECK(!memcmp(initial, &context, sizeof context));
    CHECK(!memcmp(&saved, cpu, sizeof saved));
    CHECK(!memcmp(&before_step, host, sizeof before_step));
    CHECK(clock_calls == before_calls && cycles == before_cycles);
}
static void expect_refused_at(MusashiBootMemory *host, uint32_t pc)
{
    FormatterCpu cpu = entry(pc);
    expect_refused(host, &cpu);
}
static void check_guest_path(void)
{
    seed_machine();
    expect_refused_at(&machine, callback_pc);
    install_by_guest();
    /* First callback context, directly after the installer and before a
     * callback instruction or cached callback fetch can establish trust. */
    FormatterCpu cpu = entry(callback_pc);
    for (unsigned i = 0; i < 11; ++i) {
        MusashiCpuContext context;
        CHECK(cpu.pc != handler_pc && musashi_boot_cpu_context(&cpu, MUSASHI_CPU_CONTEXT_SOURCE, &context));
        CHECK(context.pc == cpu.pc && context.instruction_valid && context.gpr == cpu.r);
        CHECK(formatter_step(&machine, &cpu));
    }
    CHECK(cpu.pc == handler_pc && cpu.main12_callback_frame == owner);
    FormatterCpu detached = entry(handler_pc);
    expect_refused(&machine, &detached);
    /* A different CPU cannot inherit a running callback merely by selecting
     * its handler. All mirrors other than the actual entered one refuse. */
    for (unsigned segment = 0; segment < 3; ++segment) {
        const uint32_t segments[] = {0, 0x80000000u, 0xa0000000u};
        for (unsigned mirror = 0; mirror < 4; ++mirror) {
            FormatterCpu alias = cpu;
            alias.pc = segments[segment] + mirror * MUSASHI_RAM_SIZE +
                (handler_pc & (MUSASHI_RAM_SIZE - 1u));
            if (alias.pc == handler_pc) continue;
            alias.npc = alias.pc + 4u;
            expect_refused(&machine, &alias);
        }
    }
    for (unsigned i = 0; i < 4; ++i) CHECK(formatter_step(&machine, &cpu));
    CHECK(cpu.pc == 0x800d24a0u && cpu.r[31] == 0x8017c0ecu && cpu.r[4] == 5u);
    /* No MAIN10 was loaded into this directed startup fixture. */
    expect_refused(&machine, &cpu);
}
static void check_active_menu_owner(void)
{
    seed_machine();
    install_by_guest();
    FormatterCpu cpu = entry(callback_pc);
    for (unsigned i = 0; i < 11; ++i) CHECK(formatter_step(&machine, &cpu));
    CHECK(cpu.pc == handler_pc && cpu.main12_callback_frame == owner);

    /* Another scene may own these addresses when this callback is inactive.
     * Its decoder cannot fill a hole in the running mounted callback. */
    FormatterCpu unobserved = cpu;
    unobserved.pc = 0x8017c1a0u;
    unobserved.npc = unobserved.pc + 4u;
    expect_refused(&machine, &unobserved);
    uint32_t word = 0;
    CHECK(formatter_fetch(&machine, &cpu, &word));

    FormatterCpu service = cpu;
    service.pc = MAIN12_SERVICE;
    service.npc = service.pc + 4u;
    service.r[6] = MAIN12_MAILBOX;
    service.r[31] = MAIN12_SERVICE_RETURN;
    CHECK(formatter_fetch(&machine, &service, &word));
    ++service.r[6];
    expect_refused(&machine, &service);
    --service.r[6];
    ++service.r[31];
    expect_refused(&machine, &service);
    --service.r[31];
    uint8_t *far = musashi_boot_ram_span(&machine, MAIN12_SERVICE_END - 1u, 1);
    CHECK(far);
    *far ^= 1u;
    expect_refused(&machine, &service);
    *far ^= 1u;
    CHECK(formatter_fetch(&machine, &service, &word));

    /* The mailbox must match its earned shadow even if the whole callback
     * has just been confirmed through another fetch. */
    uint8_t *latch = musashi_boot_ram_span(&machine, MAIN12_MAILBOX + 15u, 1);
    CHECK(latch);
    *latch ^= 1u;
    expect_refused(&machine, &cpu);
    *latch ^= 1u;
    expect_refused(&machine, &cpu);
}

/* At these sites the instruction remains valid. The data-bus access or
 * pending load must refuse without retiring the instruction. */
static void expect_copy_access_refused(FormatterCpu *cpu)
{
    uint32_t word;
    CHECK(formatter_fetch(&machine, cpu, &word));
    FormatterCpu saved;
    memcpy(&saved, cpu, sizeof saved);
    memcpy(&before_step, &machine, sizeof machine);
    unsigned previous_calls = clock_calls;
    uint64_t previous_cycles = cycles;
    CHECK(!formatter_step(&machine, cpu));
    CHECK(!memcmp(&saved, cpu, sizeof saved) && !memcmp(&before_step, &machine, sizeof machine));
    CHECK(clock_calls == previous_calls && cycles == previous_cycles);
}

static void check_slot_zero_prefix_copy(uint32_t caller_return)
{
    const int opening = caller_return == 0x800602a4u;
    const uint32_t stack = opening ? 0x1f800338u : 0x1f8002c8u;
    const uint32_t destination = opening ? 0x80078ca0u : stack + 16u;
    const uint32_t source = 0x8007472cu;
    seed_machine();
    install_by_guest();
    FormatterCpu cpu = entry(callback_pc);
    CHECK(formatter_step(&machine, &cpu) && cpu.main12_callback_frame == owner);
    /* The surrounding directory/open service frames are supplied from the
     * observed callers. The guest CPU executes each call and its delay slot. */
    cpu.pc = MAIN12_SERVICE; cpu.npc = cpu.pc + 4u;
    cpu.r[6] = MAIN12_MAILBOX;
    cpu.r[31] = MAIN12_SERVICE_RETURN;
    CHECK(formatter_step(&machine, &cpu) && cpu.main12_card_service);
    cpu.pc = caller_return - 8u; cpu.npc = cpu.pc + 4u;
    cpu.r[29] = stack;
    cpu.r[31] = opening ? 0x8002b208u : 0x8002c148u;
    cpu.r[4] = 0;
    if (opening) {
        cpu.r[16] = destination;
        cpu.r[6] = 1;
        cpu.r[5] = 0;
    } else {
        cpu.r[5] = destination;
    }
    CHECK(formatter_step(&machine, &cpu) && formatter_step(&machine, &cpu));
    CHECK(cpu.pc == MAIN12_COPY && cpu.r[31] == caller_return &&
          cpu.r[29] == stack && !cpu.r[4] && cpu.r[5] == destination);

    uint8_t *dst_zone = opening ? musashi_boot_ram_span(&machine, destination - 4u, 16) :
        machine.scratchpad + (destination & 0x3ffu) - 4u;
    uint8_t *input = musashi_boot_ram_span(&machine, source, 8);
    uint8_t *far = musashi_boot_ram_span(&machine, MAIN12_COPY_END - 1u, 1);
    uint8_t *caller = musashi_boot_ram_span(&machine, caller_return - 1u, 1);
    CHECK(dst_zone && input && far && caller);
    /* Use several distinct redzone bytes to catch an oversized or shifted write. */
    for (unsigned i = 0; i < 16; ++i) dst_zone[i] = (uint8_t)(0xb0u + i);
    uint8_t expected[16];
    memcpy(expected, dst_zone, sizeof expected);
    memcpy(expected + 4, input, 6);
    FormatterCpu bad = cpu;
    bad.r[31] += 4u;
    expect_refused(&machine, &bad);
    bad = cpu; bad.r[29] += 4u;
    expect_refused(&machine, &bad);
    bad = cpu; ++bad.r[5];
    expect_refused(&machine, &bad);
    bad = cpu; ++bad.r[4];
    expect_refused(&machine, &bad);
    MusashiGuestIrqImage separate_irq;
    mount_machine(&other_machine, &separate_irq);
    MusashiMain12Owner *other_owner = musashi_main12_open(&separate_irq, &other_machine, media);
    CHECK(other_owner);
    callbacks.main12_owner = other_owner;
    expect_refused(&machine, &cpu);
    callbacks.main12_owner = owner;
    musashi_main12_close(other_owner);
    *caller ^= 1u;
    expect_refused(&machine, &cpu);
    *caller ^= 1u;

    FormatterCpu start = cpu;
    unsigned begin_calls = clock_calls;
    CHECK(formatter_step(&machine, &cpu) && cpu.pc == MAIN12_COPY + 4u);
    bad = cpu; bad.main12_copy_dest = 0;
    expect_refused(&machine, &bad);
    for (unsigned i = 1; i < 4; ++i) CHECK(formatter_step(&machine, &cpu));
    CHECK(cpu.pc == 0x80061780u && cpu.main12_copy_dest == destination);
    *far ^= 1u;
    expect_refused(&machine, &cpu);
    *far ^= 1u;
    input[5] ^= 1u;
    expect_copy_access_refused(&cpu);
    input[5] ^= 1u;
    bad = cpu; ++bad.r[5];
    expect_copy_access_refused(&bad);
    for (unsigned i = 4; i < 22; ++i) {
        if (cpu.pc == 0x80061788u) {
            CHECK(cpu.main12_copy_pending && cpu.merge_pending && cpu.merge_reg == 2);
            bad = cpu; bad.merge_reg = 24;
            expect_copy_access_refused(&bad);
        }
        if (cpu.pc == 0x80061790u) {
            bad = cpu; ++bad.r[7];
            expect_copy_access_refused(&bad);
        }
        CHECK(formatter_step(&machine, &cpu));
    }
    CHECK(cpu.pc == caller_return && cpu.npc == caller_return + 4u && !cpu.delay_slot);
    CHECK(!cpu.main12_copy_dest && !cpu.main12_copy_pending && !cpu.merge_pending);
    CHECK(cpu.main12_callback_frame == owner && cpu.main12_card_service);
    CHECK(clock_calls == begin_calls + 22u && !memcmp(expected, dst_zone, sizeof expected));
    uint32_t registers[32];
    memcpy(registers, start.r, sizeof registers);
    registers[2] = input[2]; registers[3] = input[3];
    registers[5] = source; registers[6] = input[5]; registers[7] = destination;
    CHECK(!memcmp(registers, cpu.r, sizeof registers));
}

static void check_identity(void)
{
    CHECK(!musashi_main12_open(&irq, &machine, media));
    CHECK(!musashi_main12_open(NULL, &machine, media));
    CHECK(!musashi_main12_open(&other_irq, &other_machine, media));
    mount_machine(&other_machine, &other_irq);
    CHECK(!musashi_main12_open(&other_irq, &other_machine, NULL));
    CHECK(!musashi_main12_open(&other_irq, NULL, media));
    CHECK(!musashi_main12_open(&other_irq, &machine, media));
    CHECK(!musashi_main12_open(&irq, &other_machine, media));
    /* Correct code, copied installer effects and the wrong guest machine. */
    memcpy(&other_machine, &machine, sizeof machine);
    expect_refused_at(&other_machine, callback_pc);
    expect_fetch(&machine);
    callbacks.main12_owner = NULL;
    expect_refused_at(&machine, callback_pc);
    MusashiMain12Owner *different_owner = musashi_main12_open(&other_irq, &other_machine, media);
    CHECK(different_owner);
    callbacks.main12_owner = different_owner;
    expect_refused_at(&machine, callback_pc);
    callbacks.main12_owner = owner;
    musashi_main12_close(different_owner);
    expect_fetch(&machine);
    expect_refused_at(&machine, callback_pc ^ 0x20000000u);
    expect_fetch(&machine);
}
static unsigned active_hook, fake_calls;
static void count_hook(void *state, uint32_t pc, const uint32_t *regs)
{
    (void)state; (void)pc; (void)regs;
    ++active_hook;
}
static int count_native(MusashiNativeLaneCall *arguments)
{
    (void)arguments;
    ++fake_calls;
    return 1;
}
static uint32_t mirror_pc(unsigned index)
{
    static const uint32_t segment[] = {0, 0x80000000u, 0xa0000000u};
    return segment[index / 4] + (index % 4) * MUSASHI_RAM_SIZE +
           (callback_pc & (MUSASHI_RAM_SIZE - 1u));
}
static void check_mirrors_and_hooks(int intact)
{
    MusashiNativeLaneEntry probes[13];
    if (mapped_window) {
        for (unsigned i = 0; i < 13; ++i) {
            uint32_t pc = i == 12 ? return_pc : mirror_pc(i);
            probes[i] = (MusashiNativeLaneEntry){pc, 2, count_native, "test collision", 0, 0};
        }
        CHECK(musashi_native_lane_install(probes, 13) == 13);
        FormatterCpu positive = entry(return_pc);
        fake_calls = 0;
        CHECK(lane_try(&machine, &positive) == 1 && fake_calls == 1);
    }
    musashi_formatter_set_entry_hook(count_hook, NULL);
    for (unsigned i = 0; i < 12; ++i) {
        uint32_t pc = mirror_pc(i);
        FormatterCpu cpu = entry(pc);
        unsigned previous = active_hook;
        /* A pending observation represents an ordinary call to this PC. */
        g_entry_pending = pc;
        if (intact && pc == callback_pc) {
            CHECK(formatter_step(&machine, &cpu));
            CHECK(active_hook == previous + 1u && cpu.pc == pc + 4u);
        } else {
            expect_refused(&machine, &cpu);
            CHECK(active_hook == previous);
        }
        if (mapped_window) CHECK(fake_calls == 1);
    }
    musashi_formatter_set_entry_hook(NULL, NULL);
    CHECK(musashi_native_lane_install(NULL, 0) == 0);
    expect_refused_at(&machine, 0x1f801810u);
}
static uint32_t owned_address;
static int bios_intersects(uint32_t address, size_t size)
{
    return address <= owned_address && owned_address - address < size;
}
static void check_bus_owner(void)
{
    MusashiBiosImageDevice bios = {0};
    FormatterCpu cpu = entry(callback_pc);
    uint32_t word;
    cpu.bios_image = &bios;
    expect_refused(&machine, &cpu);
    bios.intersects = bios_intersects;
    owned_address = load_address + TRANSFER_SIZE - 1;
    expect_refused(&machine, &cpu);
    owned_address = dispatch_slot + 3u;
    expect_refused(&machine, &cpu);
    owned_address = 0x00000c80u;
    CHECK(formatter_fetch(&machine, &cpu, &word));
    expect_fetch(&machine);
}
static void check_missing_history(void)
{
    /* Copy a real installed image and slot back over a fresh machine. */
    memcpy(&other_machine, &machine, sizeof machine);
    seed_machine();
    memcpy(&machine, &other_machine, sizeof machine);
    check_digest(span(&machine), TRANSFER_SIZE, installed_hash);
    expect_refused_at(&machine, callback_pc);
    seed_machine(); install_by_guest(); expect_fetch(&machine);
}
static void check_cache_first(unsigned operation)
{
    seed_machine(); install_by_guest();
    FormatterCpu cpu = entry(callback_pc), old;
    uint32_t word;
    MusashiCpuContext context;
    CHECK(formatter_fetch(&machine, &cpu, &word));
    CHECK(musashi_boot_cpu_context(&cpu, MUSASHI_CPU_CONTEXT_SOURCE, &context));
    unsigned char *tail = span(&machine) + (handler_pc - load_address) - 1u;
    *tail ^= 1u;
    memcpy(&before_step, &machine, sizeof machine);
    memcpy(&old, &cpu, sizeof old);
    unsigned prior_calls = clock_calls;
    uint64_t prior_cycles = cycles;
    if (operation == 0) {
        word = 0x5a711a5u;
        CHECK(!formatter_fetch(&machine, &cpu, &word) && word == 0x5a711a5u);
    } else if (operation == 1) {
        unsigned char bytes[sizeof context];
        memset(&context, 0x77, sizeof context);
        memcpy(bytes, &context, sizeof bytes);
        CHECK(!musashi_boot_cpu_context(&cpu, MUSASHI_CPU_CONTEXT_SOURCE, &context));
        CHECK(!memcmp(bytes, &context, sizeof bytes));
    } else {
        CHECK(!formatter_step(&machine, &cpu));
    }
    CHECK(!memcmp(&old, &cpu, sizeof old) && !memcmp(&before_step, &machine, sizeof machine));
    CHECK(clock_calls == prior_calls && cycles == prior_cycles);
    *tail ^= 1u;
    expect_refused(&machine, &cpu);
}

static void check_revocation(void)
{
    unsigned char *bytes = span(&machine);
    /* Warm an interior word before changing a different byte of the callback. */
    FormatterCpu interior = entry(handler_pc - 4u);
    uint32_t word;
    CHECK(formatter_fetch(&machine, &interior, &word));
    size_t pos = callback_pc - load_address + 91;
    bytes[pos] ^= 1;
    check_mirrors_and_hooks(0);
    expect_refused_at(&machine, callback_pc);
    bytes[pos] ^= 1;
    expect_refused_at(&machine, callback_pc);

    seed_machine(); install_by_guest(); expect_fetch(&machine);
    bytes[TRANSFER_SIZE - 1] ^= 1;
    expect_refused_at(&machine, callback_pc);
    bytes[TRANSFER_SIZE - 1] ^= 1;
    expect_refused_at(&machine, callback_pc);

    seed_machine(); install_by_guest(); expect_fetch(&machine);
    CHECK(musashi_boot_write32(&machine, dispatch_slot, callback_pc + 4u));
    expect_refused_at(&machine, callback_pc);
    CHECK(musashi_boot_write32(&machine, dispatch_slot, callback_pc));
    expect_refused_at(&machine, callback_pc);

    seed_machine(); install_by_guest(); expect_fetch(&machine);
    CHECK(!musashi_boot_map_irq_exe(&irq, &machine, program, sizeof program - 1));
    expect_refused_at(&machine, callback_pc);

    seed_machine(); install_by_guest(); expect_fetch(&machine);
    CHECK(musashi_boot_map_irq_exe(&irq, &machine, program, sizeof program));
    expect_refused_at(&machine, callback_pc);
    callbacks.main12_owner = NULL;
    musashi_main12_close(owner);
    owner = NULL;
    expect_refused_at(&machine, callback_pc);
}

static void check_foreground_slot_reset(FormatterCpu *cpu);

#include "main12_metadata_irq.h"
#include "main12_slot_reset.h"

static uint64_t opening_thread(void *userdata)
{
    (void)userdata;
    return 1;
}
static int opening_gte_read(void *owner, const MusashiCpuContext *ctx, uint32_t index, uint32_t *value)
{
    return musashi_gte_owner_read_data(owner, ctx, index, value);
}
static int opening_gte_write(void *owner, const MusashiCpuContext *ctx, uint32_t index, uint32_t value)
{
    return musashi_gte_owner_write_data(owner, ctx, index, value);
}
static int opening_gte_command(void *owner, const MusashiCpuContext *ctx, uint32_t command)
{
    return musashi_gte_owner_command(owner, ctx, command);
}

/* The project PAC decoder supplies SC01.CD member zero from the same US disc. */
static void check_opening_callback_collision(void)
{
    enum { opening_size = 513935 };
    seed_machine();
    uint8_t *opening = musashi_boot_ram_span(&machine, load_address, opening_size);
    CHECK(opening);
#ifdef _WIN32
    CHECK(_setmode(_fileno(stdin), _O_BINARY) != -1);
#endif
    CHECK(fread(opening, 1, opening_size, stdin) == opening_size && fgetc(stdin) == EOF && !ferror(stdin));
    check_digest(opening, opening_size, "66a047a6242db0d212e27087b7576b19dbc12e399ae0d4f64f370e70bea35069");
    g_overlay_sc01_0000_words = sc01_entry_matches(&machine);
    CHECK(g_overlay_sc01_0000_words && g_sc01_code == kOverlaySc01_CodeWords);
    FormatterCpu cpu = entry(callback_pc);
    uint32_t actual, expected;
    for (uint32_t pc = callback_pc; pc < handler_pc; pc += 4u) {
        cpu.pc = pc;
        CHECK(formatter_fetch(&machine, &cpu, &actual));
        CHECK(musashi_boot_read32(&machine, pc, &expected) && actual == expected);
    }
    cpu.pc = callback_pc;
    static const uint32_t mutations[] = {0x80128158u, 0x8012841fu, 0x8017bf00u,
                                         0x8017bf10u, 0x8017c080u, 0x8017c0dbu};
    for (size_t i = 0; i < sizeof mutations / sizeof mutations[0]; ++i) {
        size_t off = mutations[i] - load_address;
        opening[off] ^= 1u;
        CHECK(!formatter_fetch(&machine, &cpu, &actual));
        opening[off] ^= 1u;
    }
    /* Even matching entry/caller/slot bytes cannot enroll a different image. */
    opening[0x3000] ^= 1u;
    CHECK(sc01_entry_matches(&machine) && !formatter_fetch(&machine, &cpu, &actual));
    opening[0x3000] ^= 1u;
    CHECK(sc01_entry_matches(&machine) && formatter_fetch(&machine, &cpu, &actual));
    const uint32_t *members[] = {kOverlaySc01Member1CodeWords, kOverlaySc01Member77CodeWords};
    for (size_t i = 0; i < sizeof members / sizeof members[0]; ++i) {
        g_sc01_code = members[i];
        CHECK(!formatter_fetch(&machine, &cpu, &actual));
    }
    g_sc01_code = kOverlaySc01_CodeWords;
    g_overlay_sc01_0000_words = 0;
    CHECK(!formatter_fetch(&machine, &cpu, &actual));
    g_overlay_sc01_0000_words = 1;
    cpu.main12_callback_frame = owner;
    CHECK(!formatter_fetch(&machine, &cpu, &actual));
    cpu.main12_callback_frame = NULL;
    cpu.main12_foreground_owner = owner;
    CHECK(!formatter_fetch(&machine, &cpu, &actual));
    cpu.main12_foreground_owner = NULL;
    cpu.pc = 0xa017c080u;
    CHECK(!formatter_fetch(&machine, &cpu, &actual));
    cpu.pc = callback_pc;
    cpu.r[2] = 0x1f8001f8u;
    cpu.r[3] = 0x1f800220u;
    cpu.r[4] = 0x1f800218u;
    cpu.r[5] = cpu.r[16] = 0x1f800258u;
    cpu.r[29] = 0x1f8001b8u;
    cpu.r[31] = 0x8017bf14u;
    MusashiCpuStatus *status = NULL;
    MusashiGteOwner gte = {0};
    MusashiCpuTransferDevice device = {.userdata = &gte, .read_data = opening_gte_read,
        .write_data = opening_gte_write, .command = opening_gte_command};
    CHECK(musashi_cpu_status_open(&status, opening_thread, NULL));
    CHECK(musashi_gte_owner_init(&gte, status, opening_thread, NULL, &cpu));
    CHECK(!formatter_step(&machine, &cpu));
    cpu.cpu_transfer = &device;
    CHECK(formatter_step(&machine, &cpu) && cpu.pc == callback_pc + 4u);
    CHECK(!cpu.main12_callback_frame && !cpu.main12_return_pc);
    CHECK(gte.command_count + gte.data_read_count + gte.data_write_count == 1u);
    CHECK(musashi_gte_owner_close(&gte) && musashi_cpu_status_close(status));
    g_overlay_sc01_0000_words = 0;
    puts("Opening scene owned all 23 overlapping fetches and retired the boundary without acquiring the file-menu callback.");
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : getenv("MUSASHI_TEST_DISC");
    int opening = argc == 3 && !strcmp(argv[2], "--verify-opening-stdin");
    if (argc > 3 || (argc == 3 && !opening && strcmp(argv[2], "--verify-transfer-stdin"))) return 1;
    if (!path || !*path) {
        puts("Provide a supported USA disc image to run the MAIN_0012 callback test.");
        return 77;
    }
    if (getenv("MUSASHI_TEST_LANE_COLLISION")) {
        int has_scratch_alias = 0;
        test_machine = musashi_native_lane_map_guest(sizeof *test_machine, &has_scratch_alias);
        CHECK(test_machine && musashi_native_lane_usable(test_machine));
        mapped_window = 1;
    }
    load_media(path, argc == 3 && !opening);
    if (opening) check_opening_callback_collision();
    check_mounted_slot_reset();
    check_metadata_irq();
    check_guest_path();
    check_active_menu_owner();
    check_slot_zero_prefix_copy(0x80060e10u);
    check_slot_zero_prefix_copy(0x800602a4u);
    seed_machine();
    install_by_guest(); check_identity(); check_missing_history();
    check_bus_owner(); check_mirrors_and_hooks(1); check_revocation();
    for (unsigned i = 0; i < 3; ++i) check_cache_first(i);
    callbacks.main12_owner = NULL;
    musashi_main12_close(owner); owner = NULL;
    musashi_disc_media_close(media);
    puts("Both slot-zero prefix calls returned after 22 guest steps with six bytes copied and intact redzones.");
    puts("93 installer steps, callback dispatch to MAIN10, and ownership controls passed.");
    printf("12 RAM mirrors and independent fetch/context/step revocation passed; native collision %s.\n",
           mapped_window ? "verified" : "not requested");
    return 0;
}
