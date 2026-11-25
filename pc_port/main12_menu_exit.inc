/* SPDX-License-Identifier: MIT */
#include "mips_scalar_memory.h"

/* The foreground dispatcher calls this callback with an enrolled stack and
 * link. Its stack frame stays live across a small result getter, the resident
 * setting/sound calls and room selection. Reconstruct only that ABI from the
 * authenticated caller. All CPU effects belong to normal instruction issue. */
typedef struct Main12MenuFrame {
    uint32_t sp, link, saved_link_address;
    int has_saved_link, has_result;
} Main12MenuFrame;

static uint32_t main12_menu_immediate(uint32_t word)
{
    return ((word & 0xffffu) ^ 0x8000u) - 0x8000u;
}

static uint32_t main12_menu_callee(uint32_t pc, uint32_t word)
{
    return ((pc + 4u) & 0xf0000000u) | ((word & 0x03ffffffu) << 2);
}

static int main12_exit_frame(MusashiBootMemory *ram, const FormatterCpu *cpu,
                             Main12MenuFrame *frame, int getter)
{
    frame->sp = cpu->main12_foreground_sp;
    frame->link = MAIN12_MENU_EXIT_RETURN; /* return to the invoking menu dispatcher */
    frame->has_saved_link = 0;
    frame->saved_link_address = 0;
    frame->has_result = 0;
    /* The load-success path of the callback is linear in its stack and link
     * effects. Its option branch only selects a command argument. A positive
     * result uses a different path and cannot enter under this owner. */
    uint32_t limit = getter ? MAIN12_MENU_RESULT_GET : cpu->pc;
    for (uint32_t p = MAIN12_MENU_EXIT; p < limit; p += 4u) {
        uint32_t ins;
        if (!musashi_boot_read32(ram, p, &ins)) return 0;
        unsigned op = ins >> 26;
        unsigned rs = (ins >> 21) & 31u;
        unsigned rt = (ins >> 16) & 31u;
        if (op == 9 && rs == 29 && rt == 29) {
            frame->sp += main12_menu_immediate(ins);
            if (frame->sp == cpu->main12_foreground_sp) frame->has_saved_link = 0;
        } else if ((op == 0x2b || op == 0x23) && rs == 29 && rt == 31) {
            uint32_t slot = frame->sp + main12_menu_immediate(ins);
            if (op == 0x2b) {
                frame->saved_link_address = slot;
                frame->has_saved_link = 1;
            } else {
                if (!frame->has_saved_link || slot != frame->saved_link_address)
                    return 0;
                /* The R3000 load can keep the previous link visible at the
                 * immediately following stack adjustment. It is dead there.
                 * The saved slot and actual return must still agree. */
                if (p + 4u != limit || cpu->r[31] != frame->link)
                    frame->link = MAIN12_MENU_EXIT_RETURN;
            }
        } else if (op == 0x29) {
            frame->has_result = 1;
        } else if (op == 3) {
            frame->link = p + 8u;
            if (getter && main12_menu_callee(p, ins) == MAIN12_MENU_RESULT_GET)
                break;
        }
    }
    if (frame->sp != cpu->r[29] || frame->link != cpu->r[31]) return 0;
    if ((getter || cpu->pc == MAIN12_MENU_EXIT || frame->sp != cpu->main12_foreground_sp) &&
        !main12_restore_stack_range(cpu->main12_foreground_sp - 24u, 24u)) return 0;
    if (frame->has_saved_link) {
        uint32_t saved;
        if (!main12_restore_stack_read(ram, cpu, frame->saved_link_address, &saved) ||
            saved != MAIN12_MENU_EXIT_RETURN) return 0;
    }
    if (!!frame->has_result != !!cpu->main12_foreground_owner->restore.foreground_result_written)
        return 0;
    uint16_t result;
    return !frame->has_result ||
           (musashi_boot_read16(ram, 0x801853f2u, &result) && result == UINT16_MAX);
}

static int main12_exit_flow(MusashiBootMemory *ram, const FormatterCpu *cpu,
                            uint32_t start, uint32_t word)
{
    if (cpu->delay_slot) {
        uint32_t p = cpu->branch_pc, branch;
        if (cpu->pc < start + 4u || p != cpu->pc - 4u ||
            !musashi_boot_read32(ram, p, &branch)) return 0;
        unsigned op = branch >> 26;
        unsigned rs = (branch >> 21) & 31u, rt = (branch >> 16) & 31u;
        uint32_t next = p + 8u;
        if (op == 3) next = main12_menu_callee(p, branch);
        else if (op == 1 && rt == 1) {
            if (!(cpu->r[rs] >> 31)) return 0; /* The retained result is negative. */
        } else if (op == 5) {
            if (cpu->r[rs] != cpu->r[rt]) next = p + 4u + (main12_menu_immediate(branch) << 2);
        } else if (op == 0 && (branch & 63u) == 8 && rs == 31) {
            next = cpu->r[31];
        } else return 0;
        return cpu->npc == next;
    }
    if (cpu->npc != cpu->pc + 4u) return 0;
    if (cpu->pc > start) {
        uint32_t predecessor;
        if (!musashi_boot_read32(ram, cpu->pc - 4u, &predecessor)) return 0;
        unsigned op = predecessor >> 26;
        if (op == 1 || op == 3 || op == 5 ||
            (op == 0 && (predecessor & 63u) == 8)) return 0;
    }
    /* BGEZ in this callback can skip the room transition only for a regular
     * nonnegative result. Its sign must agree with the bound negative latch. */
    if ((word >> 26) == 1 && ((word >> 16) & 31u) == 1)
        return !!(cpu->r[(word >> 21) & 31u] >> 31);
    return 1;
}

static int main12_menu_exit_word(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                 uint32_t *word)
{
    if (!ram || !cpu || !word || cpu->r[0] || (cpu->pc & 3u) ||
        cpu->pc != main12_physical_pc(cpu->pc) || cpu->merge_pending || cpu->gte_load_pending ||
        !main12_menu_exit_pc(cpu)) return 0;
    MusashiMain12Owner *owner = main12_owner_for(cpu, ram);
    uint32_t instruction;
    if (!owner || owner != cpu->main12_foreground_owner || !main12_current(owner, ram) ||
        !main12_menu_exit_owned(ram, cpu, owner) || !musashi_boot_read32(ram, cpu->pc, &instruction))
        return 0;
    if (cpu->main12_room_next && main12_room_address(cpu->pc)) {
        if (!owner->restore.foreground_result_written || !main12_room_word(ram, cpu, instruction))
            return 0;
    } else {
        int getter = cpu->pc >= MAIN12_MENU_RESULT_GET && cpu->pc < MAIN12_MENU_RESULT_GET + 16u;
        if (!getter && (cpu->pc < MAIN12_MENU_EXIT || cpu->pc >= MAIN12_MENU_RESULT_GET))
            return 0;
        Main12MenuFrame frame;
        if (!main12_exit_frame(ram, cpu, &frame, getter) ||
            !main12_exit_flow(ram, cpu, getter ? MAIN12_MENU_RESULT_GET : MAIN12_MENU_EXIT, instruction))
            return 0;
        struct bfm_scalar_memory access;
        if (bfm_decode_scalar_memory(instruction, cpu->r, &access)) {
            if (access.width == 4 && access.target == 31) {
                if (access.address != cpu->main12_foreground_sp - 8u ||
                    (access.store && access.store_value != MAIN12_MENU_EXIT_RETURN)) return 0;
            } else if (access.width == 2 && access.store && !getter) {
                if (access.address != 0x801853f2u || access.store_value != UINT32_MAX ||
                    frame.has_result) return 0;
            } else if (access.width == 2 && !access.store && getter) {
                if (access.address != 0x801857e8u) return 0;
            } else return 0;
        }
    }
    *word = instruction;
    return 1;
}
