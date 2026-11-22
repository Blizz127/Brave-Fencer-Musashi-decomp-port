/* Mounted overlay and resident code come from the existing owned-disc test.
 * The directory grant and asynchronous device below are synthetic. The three
 * registered interrupts and their guest calls use the normal IRQ executor. */
#include "irq_guest_fixture.h"

typedef struct {
    MusashiBiosEvents bank;
    unsigned seeks, reads, polls;
    uint32_t position;
    int pending;
} MetadataFile;

static const MusashiBiosEvents *metadata_poll_owner(void *userdata)
{
    return &((MetadataFile *)userdata)->bank;
}

static int metadata_poll(void *userdata, int32_t handle, int32_t *result)
{
    MetadataFile *file = userdata;
    ++file->polls;
    return musashi_bios_events_test(&file->bank, handle, result);
}

static uint64_t metadata_id(void *userdata, const MusashiBootMemory *ram,
    const MusashiBiosEvents *bank, int32_t fd, uint32_t position, uint32_t count)
{
    const MetadataFile *file = userdata;
    return ram == &machine && bank == &file->bank && fd == 2 && !file->pending &&
        position == file->position && count == (position == 512 ? 640u : 0u) ? 1u : 0u;
}

static int metadata_vector(void *userdata, uint32_t vector, uint32_t selector,
                           const uint32_t args[4], uint32_t *result)
{
    MetadataFile *file = userdata;
    CHECK(vector == 0xb0u && args[0] == 2u && !file->pending);
    if (selector == 0x33u) {
        CHECK(!file->position && args[1] == 512 && !args[2]);
        ++file->seeks;
        *result = file->position = 512;
    } else {
        CHECK(selector == 0x34u && file->position == 512 && args[1] == 0x80075a40u && args[2] == 640);
        ++file->reads;
        file->pending = 1;
        *result = 0;
    }
    return 1;
}

static void metadata_put(uint32_t address, uint32_t value)
{
    CHECK(musashi_boot_write32(&machine, address, value));
}

static unsigned metadata_irq(const MusashiGuestIrqCard *card, int expected)
{
    FixtureVblankBus pulse = {1u, 0};
    MusashiIrqDispatchDevice io = {.userdata=&pulse, .read16=fixture_vblank_read,
        .write16=fixture_vblank_write, .return_from_exception=fixture_vblank_return};
    MusashiGuestIrqServices service = {.card=card};
    MusashiGuestIrqStop stop;
    uint32_t installed[12];
    for (unsigned i = 0; i < 12; ++i)
        CHECK(musashi_boot_read32(&machine, 0x8006bb34u + 4u*i, &installed[i]));
    int result = musashi_boot_execute_guest_irq_with_services(&irq, &machine, 0x8006bb34u,
        installed, &io, &clock_source, NULL, &service, &stop);
    if (expected)
        CHECK(result == MUSASHI_IRQ_CONTEXT_RETURN_DELEGATED && stop.pc == 0xb0u && pulse.returns == 1);
    else
        CHECK(!result && stop.pc == 0x80062950u && !pulse.returns);
    CHECK(stop.instructions > 0);
    return stop.instructions;
}

static void check_metadata_irq(void)
{
    MetadataFile file = {0}, foreign = {0};
    MusashiCdDevice files = {.userdata=&file, .file_vector=metadata_vector,
        .metadata_events=&file.bank, .metadata_descriptor=metadata_id};
    MusashiEventDevice polls = {.userdata=&file, .b0_0b=metadata_poll, .registered_owner=metadata_poll_owner};
    MusashiGuestIrqCard card = {.memory=&machine, .files=&files, .events=&polls, .registered_events=&file.bank};
    seed_machine();
    install_by_guest();
    fixture_irq_vblank_setup(&machine);
    metadata_put(0x8006cbb4u, 0x800616d0u);
    metadata_put(0x80072a34u, 0);
    metadata_put(0x80078d78u, 0x800604c4u);
    metadata_put(0x80078c80u, 5);
    metadata_put(0x80078c88u, 0);
    metadata_put(0x80078d38u, 0);
    metadata_put(0x80078c90u, 2);
    metadata_put(0x80078c94u, 512);
    metadata_put(0x80078c98u, 640);
    metadata_put(0x80078c9cu, 0x80075a40u);
    for (unsigned i = 0; i < 8; ++i) {
        const uint32_t spec[] = {4, 0x8000, 0x100, 0x2000};
        int32_t handle, status;
        CHECK(musashi_bios_events_open(&file.bank, i < 4 ? 0xf4000001u : 0xf0000011u,
            spec[i % 4], 0x1000u, 0x800617ccu + 20*i, &handle));
        CHECK(musashi_bios_events_enable(&file.bank, handle, &status));
        metadata_put(0x80078c3cu + 4*i, (uint32_t)handle);
        metadata_put(0x80078c5cu + 4*i, i == 4);
    }
    uint8_t *row = musashi_boot_ram_span(&machine, 0x80076044u, sizeof owner->directory_row);
    CHECK(row);
    memset(row, 0, sizeof owner->directory_row);
    memcpy(row, "TEST", 4);
    memcpy(owner->directory_row, row, sizeof owner->directory_row);
    owner->directory_valid = 1;
    owner->directory_fd_token = 1;
    owner->metadata_read.files = &files;
    owner->metadata_read.events = &file.bank;

    CHECK(irq_metadata_entry(&irq, &card, 0));
    row[0] ^= 1u;
    CHECK(!irq_metadata_entry(&irq, &card, 0));
    /* A refused IRQ has already run its resident prologue. Isolate this
     * negative case from the independent successful route below. */
    memcpy(&before_step, &machine, sizeof machine);
    metadata_irq(&card, 0);
    memcpy(&machine, &before_step, sizeof machine);
    row[0] ^= 1u;
    MusashiCdDevice second = files; /* Same descriptor number in a different device. */
    card.files = &second;
    CHECK(!irq_metadata_entry(&irq, &card, 0));
    card.files = &files;
    polls.userdata = &foreign;
    CHECK(!irq_metadata_entry(&irq, &card, 0));
    polls.userdata = &file;
    uint8_t *distant = musashi_boot_ram_span(&machine, 0x80060610u, 1u);
    CHECK(distant);
    *distant ^= 1u;
    CHECK(!irq_metadata_entry(&irq, &card, 0));
    *distant ^= 1u;
    CHECK(!file.seeks && !file.reads && !file.polls && !owner->metadata_read.phase);

    unsigned seek_steps = metadata_irq(&card, 1);
    CHECK(file.seeks == 1 && !file.reads && !file.polls && owner->metadata_read.phase == 20);
    ++file.bank.allocation_serial[7];
    CHECK(!irq_metadata_entry(&irq, &card, 20));
    --file.bank.allocation_serial[7];
    metadata_put(0x80078c9cu, 0x80075a44u);
    CHECK(!irq_metadata_entry(&irq, &card, 20));
    metadata_put(0x80078c9cu, 0x80075a40u);
    unsigned read_steps = metadata_irq(&card, 1);
    CHECK(file.reads == 1 && file.polls == 8 && file.pending && owner->metadata_read.phase == 30);
    metadata_put(0x80078c5cu, 1);
    CHECK(!irq_metadata_entry(&irq, &card, 30)); /* The completion flag alone cannot make the request ready. */
    file.pending = 0;
    file.position = 1152;
    unsigned complete_steps = metadata_irq(&card, 1);
    CHECK(file.seeks == 1 && file.reads == 1 && file.polls == 12 && !owner->metadata_read.phase);
    CHECK(cd_irq_ram_word(&machine, 0x80078c80u, 0) && cd_irq_ram_word(&machine, 0x80078c88u, 1));
    CHECK(cd_irq_ram_word(&machine, 0x80072a34u, UINT32_MAX));
    printf("Metadata IRQ returned after %u/%u/%u guest steps with one seek, one read and 12 event polls.\n",
           seek_steps, read_steps, complete_steps);
}
