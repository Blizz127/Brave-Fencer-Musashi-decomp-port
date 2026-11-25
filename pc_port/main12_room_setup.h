/* The loaded file returns through MAIN/0012's foreground dispatcher. Its
 * room table selector and state initializer are called in the same frame. */
enum {
    MAIN12_ROOM_SELECT = 0x80145934u, MAIN12_ROOM_SELECT_END = 0x80145a2cu,
    MAIN12_ROOM_INIT = 0x80145b24u, MAIN12_ROOM_INIT_END = 0x80145bf8u,
    MAIN12_ROOM_RETURN = 0x8017bfe0u, MAIN12_INIT_RETURN = 0x80145a1cu
};

static int main12_room_address(uint32_t pc)
{
    return (pc >= MAIN12_ROOM_SELECT && pc < MAIN12_ROOM_SELECT_END) ||
           (pc >= MAIN12_ROOM_INIT && pc < MAIN12_ROOM_INIT_END);
}

#include "main12_room_access.h"

static int main12_room_word(MusashiBootMemory *ram, const FormatterCpu *cpu, uint32_t word)
{
    if (!ram || !cpu || !cpu->main12_foreground_owner ||
        cpu->main12_foreground_owner != main12_owner_for(cpu, ram) ||
        !cpu->main12_foreground_owner->restore.foreground_result_written ||
        cpu->merge_pending || cpu->gte_load_pending ||
        !main12_room_address(cpu->pc) || cpu->main12_room_next != cpu->pc ||
        !room_ram_value(ram, cpu->pc, word)) return 0;

    const int inner = cpu->pc >= MAIN12_ROOM_INIT;
    const uint32_t entry = inner ? MAIN12_ROOM_INIT : MAIN12_ROOM_SELECT;
    const uint32_t caller = inner ? MAIN12_INIT_RETURN : MAIN12_ROOM_RETURN;
    const uint32_t end = inner ? MAIN12_ROOM_INIT_END : MAIN12_ROOM_SELECT_END;
    uint32_t top = cpu->main12_foreground_sp;
    if (top < 80 || (top & 3) || !musashi_boot_ram_span(ram, top - 80, 80))
        return 0;
    uint32_t menu_frame = top - 24, selector_frame = menu_frame - 24;
    uint32_t frame = inner ? selector_frame - 32 : selector_frame;
    uint32_t previous_sp = inner ? selector_frame : menu_frame;

    if (!room_ram_value(ram, menu_frame + 16, MAIN12_MENU_EXIT_RETURN)) return 0;
    if (inner && !room_ram_value(ram, selector_frame + 16, MAIN12_ROOM_RETURN)) return 0;
    if (cpu->main12_room_resuming && (cpu->delay_slot || cpu->r[31] != cpu->pc)) return 0;

    uint32_t branch = 0;
    unsigned op = word >> 26;
    int control = room_is_control(word);
    if (cpu->pc != entry && !musashi_boot_read32(ram, cpu->pc - 4, &branch)) return 0;
    /* Neither mounted routine has an alternate entry into a transfer's delay slot. */
    if (!!cpu->delay_slot != room_is_control(branch)) return 0;
    if (cpu->delay_slot) {
        if (cpu->branch_pc != cpu->pc - 4 || control ||
            !room_control(ram, cpu, cpu->branch_pc, branch, word, entry, end, caller, 1))
            return 0;
    } else {
        if (cpu->npc != cpu->pc + 4) return 0;
        if (control) {
            uint32_t slot;
            if (!musashi_boot_read32(ram, cpu->pc + 4, &slot) ||
                !room_control(ram, cpu, cpu->pc, word, slot, entry, end, caller, 0)) return 0;
        }
    }

    uint32_t transfer = cpu->delay_slot ? branch : word;
    int at_return = (transfer >> 26) == 0 && (transfer & 63) == 8;
    if (cpu->r[29] != (cpu->pc == entry || at_return ? previous_sp : frame)) return 0;
    if (!room_saved_register(ram, cpu, entry, frame, 31, inner ? 24 : 16, caller)) return 0;
    if (inner && (!room_saved_register(ram, cpu, entry, frame, 16, 16, cpu->main12_room_s0) ||
                  !room_saved_register(ram, cpu, entry, frame, 17, 20, cpu->main12_room_s1))) return 0;

    unsigned base = (word >> 21) & 31, reg = (word >> 16) & 31;
    uint32_t address = cpu->r[base] + (int16_t)word;
    if (op == 35 || op == 43) { /* The only word objects here are ABI save slots. */
        uint32_t offset;
        uint32_t incoming;
        if (reg == 31) {
            offset = inner ? 24 : 16;
            incoming = caller;
        } else if (inner && (reg == 16 || reg == 17)) {
            offset = reg == 16 ? 16 : 20;
            incoming = reg == 16 ? cpu->main12_room_s0 : cpu->main12_room_s1;
        } else {
            return 0;
        }
        return base == 29 && address == frame + offset &&
               (op == 43 ? cpu->r[reg] == incoming : room_ram_value(ram, address, incoming));
    }
    if (op == 41) { /* Destination-room halfword of the live game state. */
        return inner && cpu->r[17] == 0x800af630u && address == 0x800b9a0au &&
               cpu->r[reg] == 0x3000u && musashi_boot_ram_span(ram, address, 2);
    }
    if (op == 36 || op == 40) { /* Progress-dependent latch, read or set. */
        if (!inner || address != 0x80078ec0u) return 0;
        const uint8_t *latch = musashi_boot_ram_span(ram, address, 1);
        return latch && (op == 36 || (cpu->r[reg] == 1 && (*latch & 0x7f) != 0));
    }
    if (control) return 1;
    if (!op) return word == 0 || (word & 63) == 33;
    return op == 9 || op == 10 || op == 12 || op == 15;
}

static void main12_room_retired(FormatterCpu *cpu, uint32_t issued, uint32_t next)
{
    if (issued == MAIN12_ROOM_RETURN - 4u && next == MAIN12_ROOM_SELECT) {
        cpu->main12_room_next = next;
        cpu->main12_room_resuming = 0;
        return;
    }
    if (!cpu->main12_room_next || !main12_room_address(issued)) return;
    if (issued == 0x80145a28u && next == MAIN12_ROOM_RETURN) {
        cpu->main12_room_next = 0;
        cpu->main12_room_resuming = 0;
        return;
    }
    if (issued == MAIN12_INIT_RETURN - 4u && next == MAIN12_ROOM_INIT) {
        cpu->main12_room_s0 = cpu->r[16];
        cpu->main12_room_s1 = cpu->r[17];
    }
    /* A call outside these two mounted routines keeps its normal admission.
     * The next room instruction must be that call's own return address. */
    cpu->main12_room_resuming = !main12_room_address(next);
    cpu->main12_room_next = cpu->main12_room_resuming ? issued + 4u : next;
}
