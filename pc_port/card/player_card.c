#include "player_card.h"
#include "save_payload.h"
#include "card_host_storage.h"
#include "musashi_sha256_digest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *map_guest(void *context, uint32_t address, size_t size, int writable)
{
    MusashiPlayerCard *card = context;
    uint32_t physical = address & 0x1fffffffu;
    (void)writable;
    if (!address || !size || (address >= 0x20000000u && address < 0x80000000u) ||
        address >= 0xc0000000u) return NULL;
    if (physical < MUSASHI_RAM_SIZE && size <= MUSASHI_RAM_SIZE - physical)
        return card->memory->bytes + physical;
    if (physical >= 0x1f800000u && physical < 0x1f800000u + MUSASHI_SCRATCHPAD_SIZE) {
        uint32_t offset = physical - 0x1f800000u;
        if (size <= MUSASHI_SCRATCHPAD_SIZE - offset)
            return card->memory->scratchpad + offset;
    }
    return NULL;
}

static CsMedia snapshot(void *context, unsigned slot, uint8_t *destination)
{
    MusashiPlayerCard *card = context;
    return cs_storage_snapshot(card->storage, slot, destination);
}

/* Discovery saw an existing card. A disappearance before the lock, or later
 * during the session, must never turn that selection into a new blank. */
static CsMedia snapshot_existing(void *context, unsigned slot, uint8_t *destination)
{
    CsMedia media = snapshot(context, slot, destination);
    return media == CS_MEDIA_MISSING ? CS_MEDIA_ERROR : media;
}

static int commit_image(void *context, unsigned slot, const uint8_t *expected,
                        const uint8_t *replacement)
{
    MusashiPlayerCard *card = context;
    return cs_storage_commit(card->storage, slot, expected, replacement);
}

static int deliver(void *context, uint32_t group, uint32_t spec)
{
    MusashiPlayerCard *card = context;
    /* Only replay authenticated leaves from the mounted US program. Delivery
     * can visit several records. Stage their guest writes and status changes
     * together so an unsupported later callback can retry the entire event. */
    static const uint8_t expected[32] = {
        0x49, 0xef, 0xee, 0xd0, 0x8c, 0x06, 0xa6, 0x6f, 0xd2, 0xc4, 0x9b, 0x47, 0x65, 0x2f, 0xc2, 0x26,
        0x73, 0x90, 0xa5, 0x2e, 0xbe, 0x73, 0x38, 0x89, 0xe3, 0xfd, 0x8b, 0x23, 0x5c, 0x73, 0xaf, 0x97
    };
    uint8_t digest[32];
    MusashiBiosEvents staged;
    if (musashi_bios_events_deliver_poll(card->events, group, spec)) return 1;
    const uint8_t *code = musashi_boot_ram_span(card->memory, 0x800617ccu, 160);
    if (!code || !musashi_sha256_digest(code, 160, digest) ||
        memcmp(digest, expected, sizeof digest)) return 0;
    if (!card->event_memory) card->event_memory = malloc(sizeof *card->event_memory);
    if (!card->event_memory) return 0;
    staged = *card->events;
    memcpy(card->event_memory, card->memory, sizeof *card->memory);
    if (!musashi_bios_events_deliver(&staged, group, spec,
                                    musashi_boot_execute_bios_event, card->event_memory)) return 0;
    for (size_t offset = 0; offset < MUSASHI_RAM_SIZE; offset += 4096) {
        uint8_t *live = card->memory->bytes + offset;
        const uint8_t *updated = card->event_memory->bytes + offset;
        if (memcmp(live, updated, 4096)) memcpy(live, updated, 4096);
    }
    memcpy(card->memory->scratchpad, card->event_memory->scratchpad, MUSASHI_SCRATCHPAD_SIZE);
    *card->events = staged;
    return 1;
}

static const CsCompletedRead metadata_transfer = {512u, 0x80075a40u, 640u};

/* The guest file API searches an array of 80-byte drive records via two
 * words in kernel RAM. Reserve one record and its name in the host kernel's
 * low RAM, below the callback stack and executable. The guest may replace
 * the record's operation fields. Host file I/O still uses cs_vector_dispatch. */
enum {
    IO_DEVICE_TABLE = 0x150,
    BU_RECORD = 0x7000,
    DEVICE_RECORD_BYTES = 80,
    BU_NAME = BU_RECORD + DEVICE_RECORD_BYTES
};

static int owns_file_device(const MusashiPlayerCard *card)
{
    uint32_t table, size, name;
    return card->file_device_installed &&
        musashi_boot_read32(card->memory, 0x80000000u + IO_DEVICE_TABLE, &table) &&
        musashi_boot_read32(card->memory, 0x80000000u + IO_DEVICE_TABLE + 4, &size) &&
        musashi_boot_read32(card->memory, 0x80000000u + BU_RECORD, &name) &&
        table == BU_RECORD && size == DEVICE_RECORD_BYTES && name == BU_NAME &&
        memcmp(card->memory->bytes + BU_NAME, "bu", sizeof "bu") == 0;
}

int musashi_player_card_init_files(MusashiPlayerCard *card)
{
    if (!card || !card->service || !card->memory) return 0;
    if (card->file_device_installed)
        return owns_file_device(card) && cs_init_files(card->service);

    MusashiBootMemory *memory = card->memory;
    for (unsigned i = 0; i < 8; ++i)
        if (memory->bytes[IO_DEVICE_TABLE + i]) return 0;
    for (unsigned i = 0; i < DEVICE_RECORD_BYTES + sizeof "bu"; ++i)
        if (memory->bytes[BU_RECORD + i]) return 0;
    if (!cs_init_files(card->service)) return 0;

    memcpy(memory->bytes + BU_NAME, "bu", sizeof "bu");
    musashi_boot_write32(memory, 0x80000000u + BU_RECORD, BU_NAME);
    musashi_boot_write32(memory, 0x80000000u + IO_DEVICE_TABLE + 4, DEVICE_RECORD_BYTES);
    musashi_boot_write32(memory, 0x80000000u + IO_DEVICE_TABLE, BU_RECORD);
    card->file_device_installed = 1;
    return 1;
}

static void remember_read(void *context, unsigned slot, int32_t fd, uint64_t open_id,
                              const CsCompletedRead *transfer, const uint8_t *bytes)
{
    MusashiPlayerCard *card = context;
    if (slot != 0) return;
    uint32_t relative = transfer->position - MUSASHI_PAYLOAD_FIRST;
    if (relative < MUSASHI_SAVE_FILES * MUSASHI_PAYLOAD_BYTES && relative % MUSASHI_PAYLOAD_BYTES == 0 &&
        transfer->destination == MUSASHI_PAYLOAD_DEST && transfer->count == MUSASHI_PAYLOAD_BYTES) {
        card->payload_open_id = 0;
        if (open_id && musashi_sha256_digest(bytes, transfer->count, card->payload_digest))
            card->payload_open_id = open_id;
        return;
    }
    if (fd != 2 || transfer->position != metadata_transfer.position ||
        transfer->destination != metadata_transfer.destination ||
        transfer->count != metadata_transfer.count) return;
    /* Re-reading even identical bytes on the same fd earns a different ID.
     * Never recycle an ID. Ordinary card I/O still works after exhaustion. */
    card->metadata_open_id = 0;
    if (!open_id || card->metadata_read_id == UINT64_MAX) return;
    ++card->metadata_read_id;
    if (musashi_sha256_digest(bytes, transfer->count, card->metadata_digest))
        card->metadata_open_id = open_id;
}

int musashi_player_card_open(MusashiPlayerCard *card, MusashiBootMemory *memory,
                             MusashiBiosEvents *events, const char *directory)
{
    return musashi_player_card_open_selected(card, memory, events, directory, 0);
}

int musashi_player_card_open_selected(MusashiPlayerCard *card, MusashiBootMemory *memory,
                                      MusashiBiosEvents *events, const char *directory,
                                      int require_existing)
{
    CsHost host = {card, map_guest, require_existing ? snapshot_existing : snapshot,
                   commit_image, deliver};
    if (!card || !memory || !events || card->service || card->storage) return 0;
    card->memory = memory;
    card->events = events;
    card->storage = cs_storage_create();
    card->service = cs_create_with_read_observer(host, remember_read);
    if (!card->storage || !card->service) {
        musashi_player_card_close(card);
        return 0;
    }
    cs_vector_init(&card->vector, card->service, host);
    if (!directory) return 1;
    if (bfm_plat_memcard_set_dir(directory) != BFM_PLAT_OK ||
        cs_storage_attach(card->storage, 0) != BFM_PLAT_OK) {
        fputs("Memory card slot 1 unavailable: could not acquire its save directory.\n", stderr);
        return 1;
    }
    CsMedia media = cs_prepare(card->service, 0);
    card->slot0_mounted = media == CS_MEDIA_OK || media == CS_MEDIA_UNFORMATTED;
    if (!card->slot0_mounted) {
        cs_media_changed(card->service, 0);
        cs_storage_detach(card->storage, 0);
        fputs("Memory card slot 1 unavailable: failed to load or create a valid image.\n", stderr);
    }
    return 1;
}

static uint64_t selected_read_descriptor(const MusashiPlayerCard *card,
                     const MusashiBootMemory *memory, const MusashiBiosEvents *events,
                     int32_t fd, uint32_t position, uint32_t count, const CsCompletedRead *read)
{
    if (!card || !memory || !events || card->memory != memory || card->events != events ||
        !card->storage || !card->slot0_mounted) return 0;
    const uint32_t end = read->position + read->count;
    if (!((position == 0 && count == 0) || (position == read->position && count == read->count) ||
          (position == end && count == 0))) return 0;
    /* The selected path and the directory row are separate guest buffers.
     * The service verifies the latter against an actual listing and handle. */
    const uint8_t *path = memory->bytes + (0x80078ca0u - 0x80000000u);
    const uint8_t *row = memory->bytes + (0x80076044u - 0x80000000u);
    if (memcmp(path, "bu00:", 5) || memchr(path + 5, 0, 19) || path[24] ||
        memcmp(path + 5, row, 20)) return 0;
    return cs_async_read_descriptor(card->service, card, 0, fd, (const char *)path + 5,
                                    row, 8192, position, count,
                                    position == end ? read : NULL);
}

uint64_t musashi_player_card_metadata_descriptor(const MusashiPlayerCard *card,
                     const MusashiBootMemory *memory, const MusashiBiosEvents *events,
                     int32_t fd, uint32_t position, uint32_t count)
{
    if (fd != 2) return 0;
    return selected_read_descriptor(card, memory, events, fd, position, count, &metadata_transfer);
}

uint64_t musashi_player_card_payload_descriptor(const MusashiPlayerCard *card,
                     const MusashiBootMemory *memory, const MusashiBiosEvents *events,
                     int32_t fd, unsigned file, uint32_t position, uint32_t count)
{
    uint32_t offset = musashi_save_payload_offset(file);
    if (!offset) return 0;
    const CsCompletedRead payload_transfer = {offset, MUSASHI_PAYLOAD_DEST, MUSASHI_PAYLOAD_BYTES};
    uint64_t id = selected_read_descriptor(card, memory, events, fd, position, count, &payload_transfer);
    if (!id || position != payload_transfer.position + payload_transfer.count) return id;
    const uint8_t *bytes = memory->bytes + (payload_transfer.destination - 0x80000000u);
    uint8_t digest[32];
    if (id != card->payload_open_id || !musashi_sha256_digest(bytes, payload_transfer.count, digest) ||
        memcmp(digest, card->payload_digest, sizeof digest)) return 0;
    return id;
}

uint64_t musashi_player_card_copy_completed_metadata(const MusashiPlayerCard *card,
                     const MusashiBootMemory *memory, const MusashiBiosEvents *events,
                     int32_t fd, uint64_t open_id, uint8_t out[640])
{
    if (!card || !open_id || card->metadata_open_id != open_id ||
        musashi_player_card_metadata_descriptor(card, memory, events, fd, 1152u, 0) != open_id)
        return 0;
    const uint8_t *live = memory->bytes + (metadata_transfer.destination - 0x80000000u);
    uint8_t digest[32];
    if (!musashi_sha256_digest(live, metadata_transfer.count, digest) ||
        memcmp(digest, card->metadata_digest, sizeof digest)) return 0;
    if (out) memmove(out, live, metadata_transfer.count);
    return card->metadata_read_id;
}

int musashi_player_card_finish(MusashiPlayerCard *card)
{
    if (!card || !card->service || cs_idle(card->service)) return 1;
    /* Each slot owns at most one command. A single pump visits both and
     * delivers available notifications without rerunning a completed write. */
    (void)cs_pump(card->service);
    return cs_idle(card->service);
}

void musashi_player_card_close(MusashiPlayerCard *card)
{
    if (!card) return;
    if (owns_file_device(card)) {
        memset(card->memory->bytes + IO_DEVICE_TABLE, 0, 8);
        memset(card->memory->bytes + BU_RECORD, 0, DEVICE_RECORD_BYTES + sizeof "bu");
    }
    if (card->service) {
        if (!cs_idle(card->service))
            fputs("Memory card closed with a pending command or notification. Completed writes remain on disk.\n", stderr);
        cs_media_changed(card->service, 0);
        cs_destroy(card->service);
    }
    if (card->storage) cs_storage_destroy(card->storage);
    free(card->event_memory);
    *card = (MusashiPlayerCard){0};
}

void musashi_player_card_pulse(MusashiPlayerCard *card, uint64_t source_vblank)
{
    if (!card || !card->service || card->last_vblank == source_vblank) return;
    card->last_vblank = source_vblank;
    if (!cs_idle(card->service)) (void)cs_pump(card->service);
}

int musashi_player_card_vector(MusashiPlayerCard *card, uint32_t vector, uint32_t selector,
                               const uint32_t args[4], uint32_t *v0, uint32_t *file_error)
{
    CsVectorResult result;
    if (!args || !v0 || !file_error) return 0;
    if (vector == 0xb0 && selector == 0x54) {
        *v0 = *file_error;
        return 1;
    }
    if (!card || !card->service) return 0;
    /* Initialization also installs the port's existing controller and BIOS
     * state. Its owner updates the file service after accepting each call. */
    if ((vector == 0xb0 && (selector == 0x4a || selector == 0x4b)) ||
        (vector == 0xa0 && selector == 0x70)) return 0;
    result = cs_vector_dispatch(&card->vector, vector, selector,
                                args[0], args[1], args[2], args[3]);
    if (!result.recognized) return 0;
    if (result.file_error) *file_error = (uint32_t)result.file_error;
    if (result.write_v0) *v0 = result.v0;
    return 1;
}
