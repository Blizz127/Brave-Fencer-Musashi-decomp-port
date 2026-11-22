#include "musashi_sound_record_clear.h"

int musashi_sound_clear_record_flags(MusashiBootMemory *memory,
                                     uint32_t count_table, uint32_t handle_table)
{
    for (unsigned group = 0; group < 4; ++group) {
        const uint32_t count_address = count_table + 4u * group;
        uint32_t count, entry;
        if (!musashi_boot_read32(memory, count_address, &count) ||
            !musashi_boot_read32(memory, handle_table + 4u * group, &entry))
            return 0;
        for (uint32_t i = 0; count <= INT32_MAX && i < count; ++i, entry += 12u) {
            uint8_t *flag = musashi_boot_ram_span(memory, entry + 10u, 1);
            if (!flag)
                return 0;
            *flag = 0;
            /* Counts and handles can overlap the records they describe. */
            if (!musashi_boot_read32(memory, count_address, &count))
                return 0;
        }
    }
    return 1;
}
