#ifndef MUSASHI_SOUND_RECORD_CLEAR_H
#define MUSASHI_SOUND_RECORD_CLEAR_H

#include "musashi_boot_memory.h"

/* Clear byte 10 of each 12-byte sound record. Both arguments are guest addresses
 * of four aligned words in this owner's RAM. The count is signed. A successful
 * clear returns 1. A refused access returns 0 with preceding stores retained. */
int musashi_sound_clear_record_flags(MusashiBootMemory *memory,
                                     uint32_t count_table, uint32_t handle_table);

#endif
