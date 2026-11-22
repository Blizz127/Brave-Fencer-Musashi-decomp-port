/* The installed MAIN/0012 callback builds seven fields for each of four file
 * slots. The formatter executes its words from the mounted game, including
 * its resident string copy. The owned image tracks only retired guest stores. */
static int main12_display_word(const MusashiMain12Owner *owner, MusashiBootMemory *ram,
                               const FormatterCpu *cpu, uint32_t *word)
{
    const uint32_t pc = cpu->pc;
    if (pc == 0x8017d730u) {
        if (owner->display.started || !owner->row_pointer_written || cpu->delay_slot ||
            cpu->npc != pc + 4u || cpu->r[31] != 0x8017d270u || cpu->r[29] != 0x1f800380u ||
            cpu->r[4] != 0x80u || cpu->r[5] != 0x80075b40u ||
            cpu->r[6] != 0x80075cd0u || cpu->r[7] != 0x20u) return 0;
    } else {
        if (!owner->display.started || owner->display.complete) return 0;
        uint32_t sp = pc == 0x8017dc40u || pc == 0x8017dc44u ? 0x1f800380u : 0x1f800348u;
        if (cpu->r[29] != sp) return 0;
        if (pc == 0x8017dc40u || pc == 0x8017dc44u) {
            if (cpu->r[31] != 0x8017d270u) return 0;
        } else if (pc == 0x8017d7d0u) {
            uint32_t target;
            if (cpu->r[20] >= MAIN12_COLUMNS ||
                !musashi_boot_read32(ram, 0x80184094u + 4u * cpu->r[20], &target) ||
                cpu->r[2] != target) return 0;
        }
    }
    if (pc >= 0x8017d730u && pc < 0x8017dc48u) return musashi_boot_read32(ram, pc, word);
    uint32_t target;
    if (pc >= 0x8017dc48u && pc < 0x8017dcb0u) target = 0x8017dc48u;
    else if (pc >= 0x8017dcb0u && pc < 0x8017dd28u) target = 0x8017dcb0u;
    else if (pc >= 0x8005c540u && pc < 0x8005c584u &&
             (!cpu->bios_image || (cpu->bios_image->intersects &&
              !cpu->bios_image->intersects(0x8005c540u, 0x44u))) && owner->resident_matches &&
             owner->resident_matches(owner, 0x8005c540u, 0x44u)) target = 0x8005c540u;
    else return 0;
    uint32_t call = cpu->r[31] - 8u, instruction;
    return call >= 0x8017d730u && call < 0x8017dc48u && musashi_boot_read32(ram, call, &instruction) &&
        instruction == (0x0c000000u | ((target >> 2) & 0x03ffffffu)) && musashi_boot_read32(ram, pc, word);
}

static int main12_display_access(const FormatterCpu *cpu, unsigned opcode, uint32_t address)
{
    uint32_t pc = cpu->pc;
    unsigned size;
    switch (opcode) {
    case 32: case 36: case 40: size = 1; break;
    case 33: case 37: case 41: size = 2; break;
    case 35: case 43: size = 4; break;
    default: return 0;
    }
    if (address & (size - 1u)) return 0;
    if (opcode < 40)
        return address - MAIN12_BASE <= MAIN12_BYTES - size ||
            address - 0x80075a40u <= 640u - size || address - 0x1f800348u <= 56u - size;
    if (opcode == 43 && pc >= 0x8017d734u && pc <= 0x8017d768u)
        return address >= 0x1f800358u && address <= 0x1f800378u;
    uint32_t slot = cpu->r[21];
    if (slot >= MAIN12_ROW_COUNT) return 0;
    if (opcode == 43 && (pc == 0x8017d84cu || pc == 0x8017db70u))
        return address == MAIN12_ROW_DESCRIPTORS + slot * MAIN12_COLUMNS * 4u;
    uint32_t row_start = MAIN12_ROW_STRINGS + slot * MAIN12_COLUMNS * MAIN12_TEXT_BYTES;
    if (address - row_start > MAIN12_COLUMNS * MAIN12_TEXT_BYTES - size) return 0;
    if (opcode == 40)
        return pc == 0x8005c560u || pc == 0x8005c56cu ||
               pc == 0x8017dbd4u || pc == 0x8017dbd8u || pc == 0x8017dbdcu;
    return opcode == 41 && (pc == 0x8017d92cu || pc == 0x8017d93cu ||
                             pc == 0x8017d94cu || pc == 0x8017dd14u);
}

static void main12_begin_display(MusashiMain12Owner *owner, MusashiBootMemory *ram)
{
    memcpy(owner->display.descriptors, ram->bytes + (MAIN12_ROW_DESCRIPTORS - 0x80000000u),
           sizeof owner->display.descriptors);
    memcpy(owner->display.strings, ram->bytes + (MAIN12_ROW_STRINGS - 0x80000000u),
           sizeof owner->display.strings);
    owner->display.started = 1;
}

/* Called after a successful write to RAM on this guest instruction. Private
 * stack saves fall outside both output arrays. */
static void main12_remember_display_store(MusashiMain12Owner *owner, MusashiBootMemory *ram,
                                          uint32_t address, unsigned size)
{
    uint32_t offset = address - MAIN12_ROW_DESCRIPTORS;
    uint8_t *shadow = (uint8_t *)owner->display.descriptors;
    if (offset > sizeof owner->display.descriptors - size) {
        offset = address - MAIN12_ROW_STRINGS;
        if (offset > sizeof owner->display.strings - size) return;
        shadow = (uint8_t *)owner->display.strings;
    }
    memcpy(shadow + offset, ram->bytes + (address - 0x80000000u), size);
}
