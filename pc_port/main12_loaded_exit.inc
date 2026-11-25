/* SPDX-License-Identifier: MIT */
/* Mounted US stage-advance and buffer-wait callbacks after a file load.
   Their parent tick and the teardown service have independent owners. */
enum {
    main12_tick_entry = 0x8017c080u,
    main12_stage_advance = 0x8017c348u, main12_stage_wait = 0x8017c364u,
    main12_stage_wait_end = 0x8017c3bcu, main12_stage_table = 0x801827c0u,
    main12_stage_cell = 0x80115112u, main12_wait_cell = 0x80115114u,
    main12_buffer_cell = 0x800b9a02u,
    main12_tick_child_return = 0x8017c0acu,
    main12_external_finish = 0x800d2624u
};

/* Find the lifetime of the one local return slot in the mounted function.
   The guest stack can be at either side of the prologue or epilogue. */
static int main12_handoff_frame(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                uint32_t begin, uint32_t end, uint32_t link, int called,
                                uint32_t *entry_sp, uint32_t *return_slot)
{
    uint32_t alloc = end, free_pc = end, spill = end, size = 0, slot_offset = 0;
    uint32_t live_link = link, alternate_link = link;
    for (uint32_t address = begin; address < end; address += 4u) {
        uint32_t w;
        if (!musashi_boot_read32(ram, address, &w)) return 0;
        unsigned op = w >> 26, base = w >> 21 & 31u, reg = w >> 16 & 31u;
        unsigned imm = w & 0xffffu;
        if (address < cpu->pc && called && (op == 3u || (op == 0u && (w & 63u) == 9u))) {
            live_link = alternate_link = address + 8u;
        } else if (address < cpu->pc && op == 35u && base == 29u && reg == 31u) {
            /* Delayed-load and immediate-write hosts agree at first use. */
            alternate_link = address + 4u == cpu->pc ? live_link : link;
            live_link = link;
        }
        if (op == 9u && base == 29u && reg == 29u) {
            if (imm & 0x8000u) {
                if (alloc != end) return 0;
                alloc = address; size = 0x10000u - imm;
            } else {
                if (!size || imm != size || free_pc != end) return 0;
                free_pc = address;
            }
        } else if (op == 43u && base == 29u && reg == 31u) {
            if (spill != end || !size || imm > size - 4u || (imm & 3u)) return 0;
            spill = address; slot_offset = imm;
        }
    }
    if (cpu->r[31] != live_link && cpu->r[31] != alternate_link) return 0;
    if (size < 4u || size > 128u || !(alloc < spill && spill < free_pc && free_pc < end)) return 0;
    uint32_t sp = cpu->r[29];
    if ((sp & 3u) || !((sp >= 0x80000000u && sp <= 0x80200000u) ||
                        (sp >= 0x1f800000u && sp <= 0x1f800400u))) return 0;
    *entry_sp = sp + ((cpu->pc > alloc && cpu->pc <= free_pc) ? size : 0u);
    *return_slot = *entry_sp - size + slot_offset;
    if (cpu->pc <= spill || cpu->pc > free_pc) return cpu->r[31] == link;
    uint32_t saved;
    return main12_restore_stack_read(ram, cpu, *return_slot, &saved) && saved == link;
}

/* The callback stamp names the mounted foreground dispatcher that enrolled
   this tick. Its caller's live link must trace back to a real resident call
   into that dispatcher. This is the same chain on main RAM or scratchpad. */
static int main12_handoff_caller(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                 const MusashiMain12Owner *owner, uint32_t root_sp)
{
    uint32_t prior_link, mounted, live;
    if (cpu->main12_return_pc != 0x800d2614u ||
        !main12_restore_stack_read(ram, cpu, root_sp + 16u, &prior_link) ||
        (prior_link & 3u) || prior_link < 0x80010008u || !owner->resident_matches ||
        !owner->resident_word || !owner->resident_matches(owner, prior_link - 8u, 8u) ||
        !owner->resident_word(owner, prior_link - 8u, &mounted) ||
        !musashi_boot_read32(ram, prior_link - 8u, &live) || live != mounted ||
        live >> 26 != 3u) return 0;
    uint32_t target = ((prior_link - 4u) & 0xf0000000u) | ((live & 0x03ffffffu) << 2);
    return target == 0x800d25fcu;
}

static int main12_handoff_control(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                  uint32_t origin, uint32_t begin, uint32_t end,
                                  uint32_t link, int buffer_ready,
                                  uint32_t *target, int slot, uint16_t wait_value, uint16_t buffer)
{
    uint32_t w;
    if (origin < begin || origin >= end || !musashi_boot_read32(ram, origin, &w)) return 0;
    unsigned op = w >> 26, rs = w >> 21 & 31u, rt = w >> 16 & 31u;
    uint32_t immediate_target = ((origin + 4u) & 0xf0000000u) | ((w & 0x03ffffffu) << 2);
    if (op == 3u) {
        *target = immediate_target;
        if (begin != main12_stage_wait || *target != main12_external_finish || !buffer_ready) return 0;
    } else if (op == 0u && (w & 63u) == 8u) {
        *target = cpu->r[rs];
        return rs == 31u && *target == link;
    } else if (op == 2u) {
        *target = immediate_target;
        return begin == main12_stage_wait && *target >= begin && *target < end;
    } else if (op == 5u) {
        if (begin != main12_stage_wait || cpu->r[rs] != wait_value ||
            cpu->r[rt] != ((unsigned)buffer ^ 1u)) return 0;
        uint32_t disp = ((w & 0xffffu) ^ 0x8000u) - 0x8000u;
        *target = origin + (buffer_ready ? 8u : 4u + 4u * disp);
        return *target >= begin && *target < end;
    } else return 0;
    return !slot || cpu->r[31] == origin + 8u;
}

static int main12_loaded_handoff(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                 const MusashiMain12Owner *owner)
{
    if (!ram || !cpu || !owner || cpu->main12_callback_frame != owner ||
        !cpu->main12_return_pc || !owner->restore.dismissed || !owner->restore.exit_written ||
        !owner->restore.result || !owner->restore.label || !owner->restore.consumed ||
        !owner->restore.request_cleared || owner->restore.copied != MAIN12_RESTORE_SIZE ||
        cpu->merge_pending || cpu->gte_load_pending || (cpu->pc & 3u)) return 0;
    uint32_t pc = cpu->pc, begin, end, entry_sp, slot_addr = 0;
    if (pc >= main12_stage_advance && pc < main12_stage_wait) {begin = main12_stage_advance; end = main12_stage_wait;}
    else if (pc >= main12_stage_wait && pc < main12_stage_wait_end) {begin = main12_stage_wait; end = main12_stage_wait_end;}
    else return 0;
    MusashiMain12Owner *active = main12_owner_for(cpu, ram);
    if (active != owner || !main12_current(active, ram) || !main12_restore_matches(ram, cpu, owner)) return 0;
    uint16_t stage, mode, timer, request, selected, result, buffer, wait;
    const uint8_t *activity = musashi_boot_ram_span(ram, 0x800b9a15u, 1u);
    uint32_t dispatch;
    if (!activity || *activity || !musashi_boot_read32(ram, 0x8011db24u, &dispatch) || dispatch != main12_tick_entry ||
        !musashi_boot_read16(ram, main12_stage_cell, &stage) || stage < 3u || stage > 5u ||
        !musashi_boot_read16(ram, 0x8011512cu, &mode) || mode != 4u ||
        !musashi_boot_read16(ram, 0x80115118u, &timer) || timer ||
        !musashi_boot_read16(ram, 0x80115126u, &request) || request ||
        !musashi_boot_read16(ram, 0x8011511au, &selected) || !selected || selected > 4u || selected != owner->display.selected_file ||
        !musashi_boot_read16(ram, 0x801857e8u, &result) || result != UINT16_MAX ||
        !musashi_boot_read16(ram, main12_buffer_cell, &buffer) ||
        !musashi_boot_read16(ram, main12_wait_cell, &wait)) return 0;
    /* The active restored row must still select this child's entry even in
       its epilogue after it has advanced the live stage. */
    unsigned entry_stage = begin == main12_stage_advance ? 3u : 4u;
    if (!musashi_boot_read32(ram, main12_stage_table + 4u * entry_stage, &dispatch) || dispatch != begin)
        return 0;
    int ready = ((unsigned)buffer ^ 1u) == wait;
    const uint32_t link = main12_tick_child_return;
    if (begin == main12_stage_advance) {
        entry_sp = cpu->r[29];
        if (stage != 3u || cpu->r[31] != link) return 0;
    } else if (!main12_handoff_frame(ram, cpu, begin, end, link,
                                     stage == 4u && ready,
                                     &entry_sp, &slot_addr)) return 0;
    uint32_t saved;
    if (!main12_handoff_caller(ram, cpu, owner, entry_sp + 24u) ||
        !main12_restore_stack_read(ram, cpu, entry_sp + 16u, &saved) || saved != cpu->main12_return_pc)
        return 0;
    if (begin == main12_stage_wait) {
        /* The increment commits before the same frame's epilogue. */
        uint32_t commit = end;
        for (uint32_t a = begin; a < end; a += 4u) {
            uint32_t w;
            if (!musashi_boot_read32(ram, a, &w)) return 0;
            if (w >> 26 == 41u) { if (commit != end) return 0; commit = a; }
        }
        if (commit == end || stage == 3u || (stage == 5u && pc <= commit) ||
            (stage == 4u && pc > commit && !ready)) return 0;
    }
    uint32_t target, w;
    if (!musashi_boot_read32(ram, pc, &w)) return 0;
    /* A slot must be stamped by its mounted predecessor even when its target
       happens to coincide with the ordinary sequential next PC. */
    uint32_t previous = 0;
    if (pc > begin && !musashi_boot_read32(ram, pc - 4u, &previous)) return 0;
    const unsigned preceding_op = previous >> 26;
    const int in_slot = (preceding_op >= 1u && preceding_op <= 7u) ||
        (preceding_op == 0u && ((previous & 63u) == 8u || (previous & 63u) == 9u));
    if ((cpu->delay_slot != 0) != in_slot) return 0;
    if (in_slot) {
        if (cpu->branch_pc != pc - 4u ||
            !main12_handoff_control(ram, cpu, cpu->branch_pc, begin, end, link, ready, &target, 1, wait, buffer) ||
            cpu->npc != target) return 0;
    } else if (cpu->branch_pc || cpu->npc != pc + 4u) return 0;
    unsigned op = w >> 26, rs = w >> 21 & 31u, rt = w >> 16 & 31u;
    uint32_t address = (rs ? cpu->r[rs] : 0u) + (((w & 0xffffu) ^ 0x8000u) - 0x8000u);
    if (op == 37u) {
        if (begin == main12_stage_advance) return address == main12_stage_cell;
        /* The buffer-wait callback loads each cell from its own page base. */
        if ((address & 0xffffu) != (w & 0xffffu)) return 0;
        return address == main12_stage_cell || address == main12_buffer_cell || address == main12_wait_cell;
    }
    if (op == 35u) return slot_addr && address == slot_addr && rs == 29u && rt == 31u;
    if (op == 43u) return slot_addr && rs == 29u && rt == 31u && address == slot_addr && cpu->r[31] == link;
    if (op == 41u) {
        uint16_t value = (uint16_t)(rt ? cpu->r[rt] : 0u);
        return address == main12_stage_cell && value == stage + 1u &&
            (begin == main12_stage_advance ? stage == 3u : stage == 4u && !ready);
    }
    if (op == 2u || op == 3u || op == 5u || (op == 0u && (w & 63u) == 8u))
        return !cpu->delay_slot && main12_handoff_control(ram, cpu, pc, begin, end, link, ready, &target, 0, wait, buffer);
    return op == 9u || op == 14u || op == 15u || w == 0u;
}

/* Recheck the resolved path after the selected-file display call from the
 * mounted handler. The temporary registers track only values needed by its
 * input filter. This check does not issue, retire, or change machine state. */
typedef struct Main12LoadedFilter {
    uint32_t r[32];
    uint32_t pc, next, branch;
    unsigned timer;
} Main12LoadedFilter;

static uint32_t main12_loaded_imm(uint32_t word)
{
    return ((word & 65535u) ^ 32768u) - 32768u;
}

static int main12_loaded_input_value(MusashiBootMemory *ram,
                                    const Main12LoadedFilter *path,
                                    uint32_t addr, unsigned width, uint32_t *out)
{
    if (width == 2) {
        if (addr == 0x80115118u) *out = path->timer;
        else if (addr == 0x8011512cu) *out = 4; /* loaded-file mode */
        else if (addr == 0x80115126u) *out = 0; /* load, not save */
        else return 0;
        return 1;
    }
    /* Both original button dispatch tables select the loaded-message row. */
    if (width != 4 || (addr != 0x80183f9cu + 4u * 4u &&
                       addr != 0x80183fd4u + 4u * 4u)) return 0;
    return musashi_boot_read32(ram, addr, out);
}

static int main12_loaded_filter_path(MusashiBootMemory *ram, const FormatterCpu *cpu,
        uint32_t filter_entry, uint32_t epilogue, unsigned pad, unsigned seed_timer,
        unsigned actual_timer, int dismissed)
{
    Main12LoadedFilter path = {0};
    path.pc = filter_entry;
    path.next = filter_entry + 4u;
    path.r[16] = pad;
    path.timer = seed_timer;
    int cleared = 0;
    int in_confirmation = 0, entering_confirmation = 0;
    int in_timeout = 0, candidate_in_block = 0;
    /* Ordinary input routing owns the scan for a button edge. This service
     * starts after the primary-confirm test chooses its message dispatch, or
     * at the original block that reads the expired countdown. Derive that
     * block from its incoming mounted branch and its timer access below. */
    /* No loop is required by the loaded-mode input filter. Refuse any other
     * continuation instead of traversing the rest of the menu handler. */
    for (unsigned credits = 0; credits < 96; ++credits) {
        if (path.pc < filter_entry || path.pc >= epilogue || (path.pc & 3u)) return 0;
        uint32_t word;
        if (!musashi_boot_read32(ram, path.pc, &word)) return 0;
        unsigned op = word >> 26, rs = word >> 21 & 31, rt = word >> 16 & 31;
        unsigned rd = word >> 11 & 31, fn = word & 63;
        uint32_t s = path.r[rs], t = path.r[rt], imm = main12_loaded_imm(word);
        uint32_t following = path.next + 4u, branch = 0, value = 0;
        unsigned dst = 0, use_s = 0, use_t = 0;
        if (op == 0) {
            if (fn == 0) { dst = rd; use_t = 1; value = t << (word >> 6 & 31); }
            else if (fn == 0x21) { dst = rd; use_s = use_t = 1; value = s + t; }
            else if (fn == 8) { use_s = 1; branch = path.pc; following = s; }
            else return 0;
        } else if (op == 2) {
            branch = path.pc;
            following = (path.next & 0xf0000000u) | ((word & 0x03ffffffu) << 2);
        } else if (op == 4 || op == 5) {
            use_s = use_t = 1; branch = path.pc;
            if ((s == t) == (op == 4)) following = path.pc + 4u + (imm << 2);
        } else if (op == 9 || op == 10 || op == 11 || op == 12 || op == 15) {
            use_s = op != 15; dst = rt;
            if (op == 9) value = s + imm;
            if (op == 10) value = (s ^ 0x80000000u) < (imm ^ 0x80000000u);
            if (op == 11) value = s < imm;
            if (op == 12) value = s & (word & 65535u);
            if (op == 15) value = word << 16;
        } else if (op == 0x21 || op == 0x25 || op == 0x23) {
            use_s = 1; dst = rt;
            if (!main12_loaded_input_value(ram, &path, s + imm, op == 0x23 ? 4u : 2u, &value)) return 0;
        } else if (op == 0x29) {
            use_s = use_t = 1;
            if (s + imm != 0x80115118u || t > 75u) return 0;
        } else return 0;
        /* Delay transferring authority for a block until it actually reads
         * the timer from the owned file state. This also certifies the base
         * calculation preceding that read without requiring an instruction
         * layout for the block. A branch and its delay slot end a block. */
        if (!in_timeout && !(pad & 0x50u) && !seed_timer &&
            (op == 0x25u || op == 0x21u) && s + imm == 0x80115118u) {
            in_timeout = 1;
            if (candidate_in_block) return 1;
        }
        if (path.pc == cpu->pc && path.next == cpu->npc && !!path.branch == !!cpu->delay_slot &&
            (!path.branch || cpu->branch_pc == path.branch) && actual_timer == path.timer &&
            cleared == !!dismissed && (!use_s || s == cpu->r[rs]) && (!use_t || t == cpu->r[rt])) {
            if (in_confirmation || in_timeout || (cleared && dismissed)) return 1;
            candidate_in_block = 1;
        }
        if (path.branch) candidate_in_block = 0;
        if (op == 0x29) {
            path.timer = t;
            /* Only the explicit zero store signals a dismissed file. The
             * ordinary counter's final decrement may also produce zero. */
            if (rt == 0) cleared = 1;
        }
        if (dst) path.r[dst] = value;
        if (entering_confirmation && path.branch) {
            in_confirmation = 1;
            entering_confirmation = 0;
        }
        if (!in_confirmation && !path.branch && (pad & 0x40u) && op == 4 && s && !t)
            entering_confirmation = 1;
        path.branch = branch;
        path.pc = path.next;
        path.next = following;
    }
    return 0;
}

static int main12_exit_stack_equals(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                    uint32_t where, uint32_t value)
{
    uint32_t saved;
    return main12_restore_stack_read(ram, cpu, where, &saved) && saved == value;
}

static uint32_t main12_exit_direct_return(MusashiBootMemory *ram, uint32_t begin,
                                         uint32_t end, uint32_t target)
{
    for (uint32_t pc = begin; pc < end; pc += 4u) {
        uint32_t op;
        if (!musashi_boot_read32(ram, pc, &op)) return 0;
        if (op >> 26 == 3 && (((pc + 4u) & 0xf0000000u) |
                ((op & 0x03ffffffu) << 2)) == target) return pc + 8u;
    }
    return 0;
}

static int main12_loaded_input(MusashiBootMemory *ram, const FormatterCpu *cpu,
        const MusashiMain12Owner *owner, unsigned timer, int dismissed, uint32_t input_return)
{
    const uint32_t filter_entry = main12_exit_direct_return(ram, 0x8017c3bcu,
            0x8017c3f0u, 0x8014168cu);
    const uint32_t epilogue = 0x8017ca00u;
    if (!filter_entry || cpu->pc < filter_entry || cpu->pc >= epilogue) return 0;
    uint32_t frame_sp = cpu->r[29];
    /* Include the input, file stage and menu caller in one backing. */
    if (!main12_restore_stack_range(frame_sp, 124u) || cpu->r[31] != filter_entry ||
        !main12_exit_stack_equals(ram, cpu, frame_sp + 40u, input_return) ||
        !main12_exit_stack_equals(ram, cpu, frame_sp + 76u, 0x8017c0acu) ||
        !main12_exit_stack_equals(ram, cpu, frame_sp + 96u, cpu->main12_return_pc) ||
        !main12_handoff_caller(ram, cpu, owner, frame_sp + 104u)) return 0;
    /* The mounted reader returned a 16-bit button value. Its caller retained
     * that result in S0 before calling the selected-row display. The stored
     * links above prove that display has returned in this same input frame. */
    if (cpu->r[16] > UINT16_MAX) return 0;
    if (main12_loaded_filter_path(ram, cpu, filter_entry, epilogue,
                                  cpu->r[16], timer, timer, dismissed)) return 1;
    return timer < 75u && main12_loaded_filter_path(ram, cpu, filter_entry, epilogue,
                cpu->r[16], timer + 1u, timer, dismissed);
}

static int main12_loaded_publish(MusashiBootMemory *ram, const FormatterCpu *cpu,
        const MusashiMain12Owner *owner, unsigned stage, unsigned active,
        unsigned result, uint32_t input_return)
{
    if (!owner->restore.dismissed || cpu->pc < input_return || cpu->pc >= 0x8017c348u)
        return 0;
    uint32_t word, frame_sp = cpu->r[29], expected_ra = input_return;
    int saved_result = 0, inactive = 0;
    uint32_t publish_begin = 0;
    uint32_t base[32] = {0}, known = 1u;
    for (uint32_t pc = input_return; pc < cpu->pc; pc += 4u) {
        uint32_t previous;
        if (!musashi_boot_read32(ram, pc, &previous)) return 0;
        unsigned op = previous >> 26, rs = previous >> 21 & 31u, rt = previous >> 16 & 31u;
        if (op == 3) expected_ra = pc + 8u;
        if (op == 4 && !publish_begin) publish_begin = pc + 8u;
        if (op == 0x23 && rs == 29 && rt == 31 &&
            (pc + 4u != cpu->pc || cpu->r[31] == 0x8017c0acu)) expected_ra = 0x8017c0acu;
        if (op == 9 && rs == 29 && rt == 29) frame_sp -= main12_loaded_imm(previous);
        if (op == 15u) { base[rt] = previous << 16; known |= 1u << rt; }
        else if (op == 9u && (known & (1u << rs))) {
            base[rt] = base[rs] + main12_loaded_imm(previous); known |= 1u << rt;
        } else if (op == 0x29u || op == 0x28u) {
            if (!(known & (1u << rs))) return 0;
            uint32_t address = base[rs] + main12_loaded_imm(previous);
            if (op == 0x29u && address == 0x801857e8u) saved_result = 1;
            else if (op == 0x28u && address == 0x800b9a15u) inactive = 1;
            else return 0; /* A completed stage update returns to the normal dispatcher. */
        }
    }
    /* The stage's renderer and result test are already owned. This service
     * handles the publication block reached only after the nonzero outcome. */
    if (!publish_begin || cpu->pc < publish_begin || cpu->r[31] != expected_ra || saved_result != !!owner->restore.exit_written ||
        result != (saved_result ? 0xffffu : 0u) || active != !inactive ||
        stage != 2u || !main12_restore_stack_range(frame_sp, 76u) ||
        !main12_exit_stack_equals(ram, cpu, frame_sp + 28u, 0x8017c0acu) ||
        !main12_exit_stack_equals(ram, cpu, frame_sp + 48u, cpu->main12_return_pc) ||
        !main12_handoff_caller(ram, cpu, owner, frame_sp + 56u) ||
        !musashi_boot_read32(ram, cpu->pc, &word)) return 0;
    uint32_t prior;
    if (!musashi_boot_read32(ram, cpu->pc - 4u, &prior)) return 0;
    unsigned pop = prior >> 26;
    int slot = pop == 3 || pop == 4 || (pop == 0 && (prior & 63u) == 8);
    if (slot != !!cpu->delay_slot || (slot && cpu->branch_pc != cpu->pc - 4u)) return 0;
    uint32_t next = cpu->pc + 4u;
    if (slot && pop == 3) next = (cpu->pc & 0xf0000000u) | ((prior & 0x03ffffffu) << 2);
    else if (slot && pop == 0) next = cpu->r[31];
    else if (slot && pop == 4 && !cpu->r[2]) return 0;
    if (cpu->npc != next) return 0;
    const unsigned op = word >> 26, rs = word >> 21 & 31u, rt = word >> 16 & 31u;
    if (op == 3 && cpu->r[2] != UINT32_MAX) return 0;
    if (op == 4 && cpu->r[2] == 0u) return 0;
    if (op == 0x23 || op == 0x25 || op == 0x28 || op == 0x29) {
        const uint32_t addr = cpu->r[rs] + main12_loaded_imm(word);
        if (op == 0x23) return rs == 29 &&
                ((rt == 31 && addr == frame_sp + 28u) || (rt == 16 && addr == frame_sp + 24u));
        if (op == 0x25) return addr == 0x80115112u;
        if (op == 0x28) return addr == 0x800b9a15u && !cpu->r[rt];
        return saved_result ? addr == 0x80115112u && cpu->r[rt] == 3u :
                       addr == 0x801857e8u && cpu->r[rt] == UINT32_MAX;
    }
    return 1;
}


static int main12_loaded_exit_pc(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                 const MusashiMain12Owner *owner)
{
    if (!ram || !cpu || !owner || cpu->r[0] || !cpu->main12_return_pc ||
        cpu->main12_callback_frame != owner || !owner->restore.result ||
        !owner->restore.label || !owner->restore.request_cleared || !owner->restore.consumed ||
        owner->restore.copied != MAIN12_RESTORE_SIZE || cpu->merge_pending ||
        cpu->gte_load_pending || (cpu->pc & 3u)) return 0;
    if (cpu->pc >= main12_stage_advance && cpu->pc < main12_stage_wait_end)
        return main12_loaded_handoff(ram, cpu, owner);
    unsigned input = cpu->pc >= 0x8017c3f0u && cpu->pc < 0x8017ca18u;
    unsigned publication = cpu->pc >= 0x8017c2fcu && cpu->pc < 0x8017c348u;
    if (!input && !publication) return 0;
    MusashiMain12Owner *bound = main12_owner_for(cpu, ram);
    if (bound != owner || !main12_current(bound, ram) || !main12_restore_matches(ram, cpu, owner))
        return 0;
    uint16_t mode, slot, operation, timer, stage, result;
    uint32_t callback;
    const uint8_t *active = musashi_boot_ram_span(ram, 0x800b9a15u, 1);
    if (!active || !musashi_boot_read32(ram, 0x8011db24u, &callback) ||
        callback != main12_tick_entry || !musashi_boot_read16(ram, 0x8011511au, &slot) ||
        !slot || slot > 4u || slot != owner->display.selected_file ||
        !musashi_boot_read16(ram, 0x80115126u, &operation) || operation ||
        !musashi_boot_read16(ram, 0x8011512cu, &mode) || mode != 4u ||
        !musashi_boot_read16(ram, 0x80115118u, &timer) || timer > 75u ||
        !musashi_boot_read16(ram, 0x80115112u, &stage) ||
        !musashi_boot_read16(ram, 0x801857e8u, &result)) return 0;
    uint32_t input_return = main12_exit_direct_return(ram, 0x8017c268u, 0x8017c348u, 0x8017c3bcu);
    if (!input_return) return 0;
    if (publication) return !timer && main12_loaded_publish(ram, cpu, owner, stage, *active, result, input_return);
    const uint32_t pad = cpu->r[16];
    if (stage != 2u || *active != 1u || owner->restore.exit_written ||
        result || pad > UINT16_MAX) return 0;
    /* Positive neutral countdown already belongs to the existing file grid. */
    if (!owner->restore.dismissed && (pad & 0x10u) && !(pad & 0x40u)) return 0;
    if (timer && !(pad & 0x40u)) return 0;
    return main12_loaded_input(ram, cpu, owner, timer, owner->restore.dismissed, input_return);
}
