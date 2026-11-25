/* SPDX-License-Identifier: MIT */
#ifndef MUSASHI_MAIN12_RESTORE_TRANSFER_H
#define MUSASHI_MAIN12_RESTORE_TRANSFER_H

#include "mips_scalar_memory.h"

enum {
    MAIN12_FILE_APPLY_RETURN = 0x8017c2e4u,
    MAIN12_COPY_ROUTINE = 0x8002b00cu,
    MAIN12_COPY_ROUTINE_END = 0x8002b064u,
    MAIN12_COPY_CALL = 0x8017cfa8u,
    MAIN12_COPY_LINK = MAIN12_COPY_CALL + 8u
};

/* Keep the complete frame within a single guest stack backing. */
static int main12_restore_stack_range(uint32_t address, unsigned size)
{
    if ((address & 3u) || size < 4u || (size & 3u) ||
        address > UINT32_MAX - (size - 1u))
        return 0;
    uint32_t first = main12_physical_pc(address);
    if (first)
        return main12_physical_pc(address + size - 4u) == first + size - 4u;
    if (address >= 0xc0000000u || (address >= 0x20000000u && address < 0x80000000u))
        return 0;
    uint32_t offset = (address & 0x1fffffffu) - 0x1f800000u;
    return size <= 0x400u && offset <= 0x400u - size;
}

/* Both main RAM and the R3000A scratchpad host real game stack frames. The
 * caller supplies the saved slot of its enrolled frame. Inspect that mapping
 * directly so a forged stack address cannot read a device. */
static int main12_restore_stack_read(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                     uint32_t address, uint32_t *word)
{
    if (!ram || !cpu || !word || !main12_restore_stack_range(address, 4u))
        return 0;
    const uint8_t *mapped = cpu_ram_span(ram, cpu, address, 4u);
    if (!mapped)
        return 0;
    *word = (uint32_t)mapped[0] | (uint32_t)mapped[1] << 8 |
            (uint32_t)mapped[2] << 16 | (uint32_t)mapped[3] << 24;
    return 1;
}

/* The issuer consumes the real instruction. This scope grants access to the
 * selected file's snapshot and its consecutive destination words only. */
static int main12_copy_scope(MusashiBootMemory *ram, const FormatterCpu *cpu,
                             MusashiMain12Owner *owner)
{
    uint32_t saved_link;
    return owner && cpu && cpu->pc == main12_physical_pc(cpu->pc) &&
           !cpu->merge_pending && !cpu->gte_load_pending &&
           owner->restore.result && main12_current(owner, ram) &&
           main12_restore_matches(ram, cpu, owner) &&
           main12_restore_stack_range(cpu->r[29], 40u) &&
           main12_restore_stack_read(ram, cpu, cpu->r[29] + 36u, &saved_link) &&
           saved_link == MAIN12_FILE_APPLY_RETURN &&
           owner->resident_matches && owner->resident_word &&
           owner->resident_matches(owner, MAIN12_COPY_ROUTINE,
                                   MAIN12_COPY_ROUTINE_END - MAIN12_COPY_ROUTINE) &&
           !(owner->restore.copied & 3u) &&
           owner->restore.copied <= MAIN12_RESTORE_SIZE;
}

static int main12_copy_resident_word(MusashiBootMemory *ram,
                                     const MusashiMain12Owner *owner,
                                     uint32_t pc, uint32_t *word)
{
    uint32_t mounted;
    return pc >= MAIN12_COPY_ROUTINE && pc < MAIN12_COPY_ROUTINE_END &&
           !(pc & 3u) && owner->resident_word(owner, pc, &mounted) &&
           musashi_boot_read32(ram, pc, word) && *word == mounted;
}

/* Before the short tail, the original loop compares its read cursor with
 * the end of the complete groups. Its return leaves one final word for the
 * delay slot. Check those semantic boundaries at issue and again in the slot. */
static int main12_copy_edge(const FormatterCpu *cpu, const MusashiMain12Owner *owner,
                            uint32_t pc, uint32_t branch, uint32_t *target)
{
    unsigned rs = (branch >> 21) & 31u;
    unsigned rt = (branch >> 16) & 31u;
    if (branch >> 26 == 5u) {
        unsigned copied = owner->restore.copied;
        unsigned groups_end = MAIN12_RESTORE_SIZE & ~15u;
        if (!copied || (copied & 15u) || copied > groups_end ||
            cpu->r[rs] != MUSASHI_PAYLOAD_DEST + copied ||
            cpu->r[rt] != MUSASHI_PAYLOAD_DEST + groups_end)
            return 0;
        uint32_t words = ((branch & 0xffffu) ^ 0x8000u) - 0x8000u;
        *target = copied == groups_end ? pc + 8u : pc + 4u + words * 4u;
        return *target >= MAIN12_COPY_ROUTINE && *target < MAIN12_COPY_ROUTINE_END;
    }
    if ((branch >> 26) || (branch & 63u) != 8u || rs != 31u ||
        cpu->r[rs] != MAIN12_COPY_LINK || owner->restore.copied != MAIN12_RESTORE_SIZE - 4u)
        return 0;
    *target = cpu->r[rs];
    return 1;
}

/* A delay slot may complete only the edge selected by the genuine branch.
 * Both source GPRs remain live until the slot itself has retired. */
static int main12_copy_position(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                const MusashiMain12Owner *owner)
{
    if (!cpu->delay_slot) {
        if (cpu->npc != cpu->pc + 4u || cpu->pc == MAIN12_COPY_CALL + 4u)
            return 0;
        if (cpu->pc > MAIN12_COPY_ROUTINE && cpu->pc < MAIN12_COPY_ROUTINE_END) {
            uint32_t previous;
            if (!main12_copy_resident_word(ram, owner, cpu->pc - 4u, &previous))
                return 0;
            if (previous >> 26 == 5u || (!(previous >> 26) && (previous & 63u) == 8u))
                return 0;
        }
        return 1;
    }
    if (cpu->branch_pc != cpu->pc - 4u)
        return 0;
    if (cpu->branch_pc == MAIN12_COPY_CALL)
        return cpu->npc == MAIN12_COPY_ROUTINE && cpu->r[31] == MAIN12_COPY_LINK;
    uint32_t branch;
    if (!main12_copy_resident_word(ram, owner, cpu->branch_pc, &branch))
        return 0;
    uint32_t target;
    return main12_copy_edge(cpu, owner, cpu->branch_pc, branch, &target) &&
           cpu->npc == target;
}

static int main12_restored_word_matches(const MusashiMain12Owner *owner,
                                   uint32_t address, uint32_t value)
{
    unsigned offset = owner->restore.copied;
    if (offset > MAIN12_RESTORE_SIZE - 4u ||
        address != MAIN12_RESTORE_BUFFER + offset)
        return 0;
    const uint8_t *input = owner->restore.bytes + offset;
    uint32_t expected = (uint32_t)input[0] | (uint32_t)input[1] << 8 |
                        (uint32_t)input[2] << 16 | (uint32_t)input[3] << 24;
    return value == expected;
}

static int main12_store_payload(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                MusashiMain12Owner *owner, uint32_t address,
                                uint32_t value)
{
    uint32_t word;
    struct bfm_scalar_memory access;
    return main12_copy_scope(ram, cpu, owner) &&
           cpu->main12_payload_pc == cpu->pc && cpu->r[31] == MAIN12_COPY_LINK &&
           main12_copy_position(ram, cpu, owner) &&
           main12_copy_resident_word(ram, owner, cpu->pc, &word) &&
           bfm_decode_scalar_memory(word, cpu->r, &access) &&
           access.store && access.width == 4u && access.address == address &&
           access.store_value == value && main12_restored_word_matches(owner, address, value);
}

static int main12_payload_copy_word(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                    uint32_t *word)
{
    MusashiMain12Owner *owner = main12_owner_for(cpu, ram);
    if (!word || !main12_copy_scope(ram, cpu, owner) ||
        !main12_copy_position(ram, cpu, owner))
        return 0;
    if (cpu->main12_payload_pc && cpu->main12_payload_pc != cpu->pc)
        return 0;

    uint32_t mounted;
    if (cpu->pc >= MAIN12_COPY_CALL && cpu->pc < MAIN12_COPY_LINK) {
        if (!main12_restore_pc(ram, cpu, owner) ||
            owner->restore.copied || cpu->r[4] != MAIN12_RESTORE_BUFFER ||
            (cpu->pc == MAIN12_COPY_CALL && cpu->r[31] != MAIN12_FILE_APPLY_RETURN) ||
            !!cpu->delay_slot != (cpu->pc != MAIN12_COPY_CALL) ||
            !musashi_boot_read32(ram, cpu->pc, &mounted))
            return 0;
        /* main12_current authenticated the installed overlay. Resolve the
         * caller's target from that image before handing either word onward. */
        uint32_t call;
        if (!musashi_boot_read32(ram, MAIN12_COPY_CALL, &call) ||
            call >> 26 != 3u ||
            (((MAIN12_COPY_CALL + 4u) & 0xf0000000u) |
              ((call & 0x03ffffffu) << 2)) != MAIN12_COPY_ROUTINE)
            return 0;
    } else {
        if (cpu->main12_payload_pc != cpu->pc || cpu->r[31] != MAIN12_COPY_LINK ||
            !main12_copy_resident_word(ram, owner, cpu->pc, &mounted))
            return 0;
        if (cpu->pc == MAIN12_COPY_ROUTINE &&
            (cpu->r[4] != MAIN12_RESTORE_BUFFER || owner->restore.copied))
            return 0;

        if (mounted >> 26 == 5u || (!(mounted >> 26) && (mounted & 63u) == 8u)) {
            uint32_t target;
            if (!main12_copy_edge(cpu, owner, cpu->pc, mounted, &target))
                return 0;
        }
        struct bfm_scalar_memory access;
        if (bfm_decode_scalar_memory(mounted, cpu->r, &access)) {
            if (access.width != 4u || (access.address & 3u))
                return 0;
            if (access.store) {
                if (!main12_restored_word_matches(owner, access.address, access.store_value))
                    return 0;
            } else {
                /* The leaf starts each group with consecutive loads from the
                 * first uncommitted source word. The final group is shorter. */
                uint32_t start = MUSASHI_PAYLOAD_DEST + owner->restore.copied;
                unsigned base = (mounted >> 21) & 31u;
                if (cpu->r[base] != start || access.address - start >= 16u ||
                    access.address - MUSASHI_PAYLOAD_DEST > MAIN12_RESTORE_SIZE - 4u)
                    return 0;
            }
        }
    }
    *word = mounted;
    return 1;
}
#endif
