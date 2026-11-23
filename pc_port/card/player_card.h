#ifndef MUSASHI_PLAYER_CARD_H
#define MUSASHI_PLAYER_CARD_H

#include "card_vector.h"
#include "musashi_bios_events.h"
#include "musashi_boot_memory.h"

typedef struct CsStorage CsStorage;

typedef struct MusashiPlayerCard {
    CsStorage *storage;
    CardService *service;
    CsVector vector;
    MusashiBootMemory *memory;
    MusashiBiosEvents *events;
    MusashiBootMemory *event_memory;
    uint64_t last_vblank;
    int slot0_mounted;
    int file_device_installed;
    uint64_t metadata_open_id, metadata_read_id;
    uint8_t metadata_digest[32];
    uint64_t payload_open_id;
    uint8_t payload_digest[32];
} MusashiPlayerCard;

/* The player owns the first slot. A missing directory leaves the device
 * disconnected. Only an exclusively attached, missing image can be created. */
int musashi_player_card_open(MusashiPlayerCard *, MusashiBootMemory *,
                             MusashiBiosEvents *, const char *directory);
int musashi_player_card_open_selected(MusashiPlayerCard *, MusashiBootMemory *,
                                      MusashiBiosEvents *, const char *directory, int require_existing);
/* Publish the host backup-unit driver to the guest's device search after
 * InitCARD and StartCARD. Publication is independent of inserted media. */
int musashi_player_card_init_files(MusashiPlayerCard *);
/* On graceful exit, complete accepted requests while RAM and storage are
 * present. An interruption may close directly, preserving only durable writes. */
/* Descriptor identity for the observed busy-5 metadata seek, read and completion.
 * No transfer, seek or event delivery. Zero means the current phase cannot run. */
uint64_t musashi_player_card_metadata_descriptor(const MusashiPlayerCard *,
                             const MusashiBootMemory *, const MusashiBiosEvents *,
                             int32_t fd, uint32_t position, uint32_t count);
/* Verify the selected save file (1 through 4), its open handle and the expected payload transfer phase. */
uint64_t musashi_player_card_payload_descriptor(const MusashiPlayerCard *,
                             const MusashiBootMemory *, const MusashiBiosEvents *,
                             int32_t fd, unsigned file, uint32_t position, uint32_t count);
/* Return this mounted instance's ID for the exact completed metadata read.
 * NULL output validates without copying. On refusal the output is untouched.
 * Compare the ID earned by the IRQ with the live pre-close result. */
uint64_t musashi_player_card_copy_completed_metadata(const MusashiPlayerCard *,
                             const MusashiBootMemory *, const MusashiBiosEvents *,
                             int32_t fd, uint64_t open_id, uint8_t out[640]);
int musashi_player_card_finish(MusashiPlayerCard *);
void musashi_player_card_close(MusashiPlayerCard *);
void musashi_player_card_pulse(MusashiPlayerCard *, uint64_t source_vblank);
int musashi_player_card_vector(MusashiPlayerCard *, uint32_t vector, uint32_t selector,
                               const uint32_t args[4], uint32_t *v0, uint32_t *file_error);
#endif
