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
} MusashiPlayerCard;

/* The player owns the first slot. A missing directory leaves the device
 * disconnected. Only an exclusively attached, missing image can be created. */
int musashi_player_card_open(MusashiPlayerCard *, MusashiBootMemory *,
                             MusashiBiosEvents *, const char *directory);
/* On graceful exit, complete accepted requests while RAM and storage are
 * present. An interruption may close directly, preserving only durable writes. */
/* Descriptor identity for the observed busy-5 metadata seek, read and completion.
 * No transfer, seek or event delivery. Zero means the current phase cannot run. */
uint64_t musashi_player_card_metadata_descriptor(const MusashiPlayerCard *,
                             const MusashiBootMemory *, const MusashiBiosEvents *,
                             int32_t fd, uint32_t position, uint32_t count);
int musashi_player_card_finish(MusashiPlayerCard *);
void musashi_player_card_close(MusashiPlayerCard *);
void musashi_player_card_pulse(MusashiPlayerCard *, uint64_t source_vblank);
int musashi_player_card_vector(MusashiPlayerCard *, uint32_t vector, uint32_t selector,
                               const uint32_t args[4], uint32_t *v0, uint32_t *file_error);
#endif
