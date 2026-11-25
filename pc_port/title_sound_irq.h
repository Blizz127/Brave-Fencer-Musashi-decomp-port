/* Mounted title and File 1 sequences. The callback uses the existing guest
 * CPU and clock. The intervals cover the observed kind-5 note, loop, wait
 * and last-channel release paths. */
static int irq_level_slot_pc(uint32_t pc) {
    static const struct { uint16_t start, end; } spans[] = {
        {0x000u, 0x058u}, {0x068u, 0x078u}, {0x07cu, 0x15cu},
        {0x190u, 0x1bcu},
        {0x1d8u, 0x380u}, {0x3c8u, 0x434u}, {0x484u, 0x49cu},
        {0x4b0u, 0x4bcu}, {0x4c0u, 0x518u}, {0x51cu, 0x5b4u},
        {0x61cu, 0x698u}, {0x6c8u, 0x75cu}, {0x764u, 0x7c4u},
        {0x7c8u, 0x7dcu}, {0x7e8u, 0x81cu}, {0x830u, 0x960u}, {0x988u, 0x998u}, {0xa14u, 0xa20u},
        {0xa28u, 0xa5cu},
    };
    uint32_t offset = pc - 0x8003388cu;
    for (size_t i = 0; i < sizeof spans / sizeof spans[0]; ++i) {
        if (offset < spans[i].start) return 0;
        if (offset < spans[i].end) return 1;
    }
    return 0;
}

/* Five remaining instructions of the resident File 1 end command. */
static int irq_file_end_pc(uint32_t pc) {
    return pc >= 0x80033d28u && pc <= 0x80033d38u && !(pc & 3u);
}

/* Only the observed computed dispatch, case 8 and downward ramp tail are
 * additional fetchable instructions. Their use requires the opening frame. */
static int irq_motion_eight_pc(uint32_t pc) {
    return (pc >= 0x8003171cu && pc <= 0x80031744u) ||
           (pc >= 0x80031898u && pc <= 0x800318bcu) ||
           (pc >= 0x80031938u && pc <= 0x80031978u);
}

static int irq_opening_motion_owner(const FormatterCpu *cpu);
static int irq_opening_motion_return(const FormatterCpu *cpu);

/* Four resident sound helpers called by the sequence slot's first note.
 * Each region is populated from the installed executable by irq_code. */
static int irq_level_leaf_pc(uint32_t pc) {
    return irq_motion_eight_pc(pc) || (pc >= 0x80030ca4u && pc <= 0x80030d34u) ||
           (pc >= 0x80030d6cu && pc <= 0x80030d7cu) ||
           (pc >= 0x800316f8u && pc <= 0x80031718u) ||
           (pc >= 0x80031980u && pc <= 0x80031984u) ||
           (pc >= 0x8002f064u && pc <= 0x8002f0ccu) ||
           (pc >= 0x8002eff8u && pc <= 0x8002f030u) ||
           (pc >= 0x8002f05cu && pc <= 0x8002f060u);
}

static uint32_t irq_level_leaf_target(uint32_t call) {
    switch (call) {
    case 0x80033d64u: return 0x80030ca4u;
    case 0x80033f9cu: return 0x800316f8u;
    case 0x800341c8u: return 0x8002f064u;
    case 0x800341d4u: return 0x8002eff8u;
    default: return 0;
    }
}

/* The parent selects the motion mask from the local note field and the
 * signed threshold. Both branches reach the same call and return address. */
static int irq_level_motion_mode(const FormatterCpu *cpu) {
    if (cpu->r[29] != 0x8006c9ccu || cpu->r[20] != cpu->r[29] + 16u) return -1;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    const uint8_t *field = musashi_boot_ram_span(ram, cpu->r[20] + 12u, 1);
    uint16_t threshold;
    if (!field || !musashi_boot_read16(ram, 0x800a4ef6u, &threshold)) return -1;
    return threshold < 0x8000u && *field < threshold;
}

static int irq_level_leaf_call(const FormatterCpu *cpu, uint32_t word, uint32_t target) {
    if (cpu->pc == 0x800341c8u) {
        int mode = irq_level_motion_mode(cpu);
        if (mode < 0 || cpu->r[2] != (unsigned)mode || cpu->r[4] != (unsigned)mode) return 0;
    }
    return (word >> 26) == 3u && target && target == irq_level_leaf_target(cpu->pc) &&
           cpu->level_slot && cpu->r[16] == cpu->level_slot && cpu->r[29] == 0x8006c9ccu;
}

/* The parent holds the chosen channel in S1 and the corresponding mixer
 * record in S2. Each eight-entry array has its own stride. */
static int irq_level_leaf_channel(const FormatterCpu *cpu, unsigned *index) {
    uint32_t channel = cpu->r[17];
    if (channel < 0x800a4988u) return 0;
    uint32_t offset = channel - 0x800a4988u;
    if (offset >= 8u * 0x54u || offset % 0x54u) return 0;
    *index = offset / 0x54u;
    return cpu->r[18] == 0x800a4c28u + *index * 0x48u;
}

static int irq_level_leaf_entry(const FormatterCpu *cpu, MusashiBootMemory *ram) {
    uint32_t call = cpu->r[31] - 8u, value, mask;
    unsigned index;
    if (!cpu->level_slot || cpu->r[16] != cpu->level_slot || cpu->r[29] != 0x8006c9ccu ||
        !irq_level_leaf_target(call) || cpu->pc != irq_level_leaf_target(call)) return 0;
    if (cpu->pc == 0x80030ca4u) {
        uint16_t priority;
        const uint8_t *channels = musashi_boot_ram_span(ram, 0x800a4988u, 8u * 0x54u);
        if (!channels || !musashi_boot_read16(ram, cpu->r[29] + 0x14u, &priority) ||
            cpu->r[4] != priority) return 0;
        /* A full pool enters a separate priority/voice-release path. */
        for (unsigned i = 0; i < 8u; ++i)
            if (!channels[i * 0x54u + 0x4eu]) return 1;
        return 0;
    }
    if (!irq_level_leaf_channel(cpu, &index) || cpu->r[21] != index) return 0;
    if (cpu->pc == 0x800316f8u)
        return cpu->r[4] == cpu->r[17] && musashi_boot_read32(ram, cpu->r[4] + 0x14u, &value) &&
               (!value || (value == 0x800c9a34u && irq_opening_motion_owner(cpu)));
    int mode = cpu->pc == 0x8002eff8u ? 1 : irq_level_motion_mode(cpu);
    return mode >= 0 && cpu->r[4] == (unsigned)mode &&
           musashi_boot_read32(ram, cpu->r[18], &value) &&
           musashi_boot_read32(ram, 0x80073180u + 4u * index, &mask) &&
           mask == (1u << (16u + index)) && cpu->r[5] == value && value == mask;
}

static int irq_level_leaf_return(const FormatterCpu *cpu, uint32_t target) {
    if (!cpu->level_slot || cpu->r[16] != cpu->level_slot || cpu->r[29] != 0x8006c9ccu ||
        cpu->r[31] != target) return 0;
    switch (cpu->pc) {
    case 0x80030d78u: {
        unsigned index = cpu->r[6];
        if (target != 0x80033d6cu || index >= 8u || cpu->r[2] != index + 1u ||
            cpu->r[5] != 0x800a4990u + index * 0x54u) return 0;
        const uint8_t *channels = musashi_boot_ram_span(cpu->irq_image->memory,
                                                       0x800a4988u, 8u * 0x54u);
        if (!channels || channels[index * 0x54u + 0x4eu]) return 0;
        for (unsigned i = 0; i < index; ++i)
            if (!channels[i * 0x54u + 0x4eu]) return 0;
        return 1;
    }
    case 0x80031980u:
        return target == 0x80033fa4u && (!cpu->opening_motion_eight || irq_opening_motion_return(cpu));
    case 0x8002f0c8u: {
        unsigned index;
        uint32_t enabled, disabled;
        int mode = irq_level_motion_mode(cpu);
        if (target != 0x800341d0u || mode < 0 || !irq_level_leaf_channel(cpu, &index) ||
            cpu->r[21] != index) return 0;
        uint32_t mask = 1u << (16u + index);
        return cpu->r[5] == mask && cpu->r[2] == ~mask &&
               musashi_boot_read32(cpu->irq_image->memory, 0x800c7d2cu, &enabled) &&
               musashi_boot_read32(cpu->irq_image->memory, 0x800a2ba0u, &disabled) &&
               (enabled & mask) == (mode ? mask : 0u) &&
               (disabled & mask) == (mode ? 0u : mask);
    }
    case 0x8002f05cu: return target == 0x800341dcu;
    default: return 0;
    }
}

static int irq_level_leaf_data(const FormatterCpu *cpu, uint32_t word, uint32_t address) {
    unsigned op = word >> 26;
    uint32_t pc = cpu->pc;
    if (!cpu->level_slot) return 0;
    if (pc >= 0x80030ca4u && pc < 0x80030d80u) {
        uint32_t frame = cpu->r[29];
        if (frame != 0x8006c9a4u || cpu->r[31] != 0x80033d6cu) return 0;
        if (pc == 0x80030ca8u)
            return op == 43u && address == frame + 32u && cpu->r[16] == cpu->level_slot;
        unsigned index = cpu->r[6];
        if (index >= 8u) return 0;
        uint32_t row = 0x800a4988u + index * 0x54u;
        if (cpu->r[16] != row || cpu->r[5] != row + 8u) return 0;
        switch (pc) {
        case 0x80030cc0u: return op == 43u && address == frame + 36u;
        case 0x80030cc4u:
        case 0x80030cecu: return op == 40u && address == frame + 26u;
        case 0x80030cc8u: return op == 36u && address == row + 0x4eu;
        case 0x80030cd8u: return op == 36u && address == frame + 26u;
        case 0x80030ce8u:
        case 0x80030d00u: {
            const uint8_t *active = musashi_boot_ram_span(cpu->irq_image->memory, row + 0x4eu, 1);
            return op == 37u && address == row + 8u && active && *active;
        }
        case 0x80030cf0u:
        case 0x80030d1cu: return op == 43u && address == frame + 16u;
        case 0x80030cf4u:
        case 0x80030d20u: return op == 43u && address == frame + 20u;
        case 0x80030cfcu:
        case 0x80030d18u: return op == 41u && address == frame + 24u;
        case 0x80030d04u: return op == 37u && address == frame + 24u;
        case 0x80030d6cu: return op == 35u && address == frame + 36u;
        case 0x80030d70u: return op == 35u && address == frame + 32u;
        default: return 0;
        }
    }
    unsigned channel_index;
    if (cpu->r[29] != 0x8006c9ccu || cpu->r[16] != cpu->level_slot ||
        !irq_level_leaf_channel(cpu, &channel_index) || cpu->r[21] != channel_index) return 0;
    if (pc >= 0x800316f8u && pc < 0x80031988u) {
        uint32_t channel = cpu->r[17], command;
        if (cpu->r[5] != channel) return 0;
        switch (pc) {
        case 0x80031700u: return op == 35u && address == channel + 0x14u &&
            musashi_boot_read32(cpu->irq_image->memory, address, &command) && !command;
        case 0x80031704u: return op == 35u && address == channel + 0x24u;
        case 0x8003170cu: return op == 40u && address == channel + 0x37u;
        case 0x80031710u: return op == 43u && address == channel + 0x18u;
        case 0x80031718u: return op == 43u && address == channel + 0x20u;
        default: return 0;
        }
    }
    if (pc >= 0x8002f070u && pc < 0x8002f0c8u &&
        (cpu->r[31] != 0x800341d0u || irq_level_motion_mode(cpu) != (pc < 0x8002f0a0u)))
        return 0;
    switch (pc) {
    case 0x8002f074u: return op == 35u && address == 0x800a2ba0u;
    case 0x8002f07cu: return op == 35u && address == 0x800c7d2cu;
    case 0x8002f08cu: return op == 43u && address == 0x800a2ba0u;
    case 0x8002f094u: return op == 43u && address == 0x800c7d2cu;
    case 0x8002f0a4u: return op == 35u && address == 0x800c7d2cu;
    case 0x8002f0acu: return op == 35u && address == 0x800a2ba0u;
    case 0x8002f0bcu: return op == 43u && address == 0x800c7d2cu;
    case 0x8002f0c4u: return op == 43u && address == 0x800a2ba0u;
    case 0x8002f008u: return op == 35u && address == 0x800a2b98u;
    case 0x8002f010u: return op == 35u && address == 0x800c7d20u;
    case 0x8002f020u: return op == 43u && address == 0x800a2b98u;
    case 0x8002f028u: return op == 43u && address == 0x800c7d20u;
    default: return 0;
    }
}

/* The levels callback visits eight resident sound slots. Its JAL sets A0
 * from the current slot and its delay instruction sets A1 from the index. */
static uint32_t irq_level_slot(const FormatterCpu *cpu, MusashiBootMemory *ram) {
    uint32_t index = cpu->r[17];
    uint16_t kind;
    if (index >= 8u) return 0;
    uint32_t slot = 0x800a46e8u + index * 0x54u;
    const uint8_t *paused = musashi_boot_ram_span(ram, 0x800a4f17u, 1);
    if (!paused || *paused || slot != cpu->r[4] || slot != cpu->r[18] ||
        !musashi_boot_read16(ram, slot, &kind) || (kind & 0x3fu) != 5u)
        return 0;
    return slot;
}

/* The title and File 1 use two resident, bank-zero request streams.
 * Each live stream must still match the copy taken when the EXE was mapped.
 * The next entry's data address bounds File 1. The entry table bounds the
 * last stream. The slot keeps the original start at +16 and advances +12. */
static uint32_t irq_level_stream_at(const FormatterCpu *cpu, uint32_t address) {
    uint32_t song, cursor, entry, limit, size;
    uint16_t bank;
    const uint8_t *mapped;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    if (!cpu->level_slot || cpu->r[16] != cpu->level_slot ||
        !musashi_boot_read16(ram, cpu->level_slot + 0x34u, &bank) || bank ||
        !musashi_boot_read32(ram, cpu->level_slot + 16u, &song) ||
        !musashi_boot_read32(ram, cpu->level_slot + 12u, &cursor)) return 0;
    if (song == 0x80068a20u) {
        entry = 0x80068a54u + 21u * 12u;
        size = sizeof cpu->irq_image->title_note;
        mapped = cpu->irq_image->title_note;
    } else if (song == 0x80068960u) {
        entry = 0x80068a54u + 18u * 12u;
        size = sizeof cpu->irq_image->file_note;
        mapped = cpu->irq_image->file_note;
        if (!cd_irq_ram_word(ram, entry + 12u, song + size)) return 0;
    } else return 0;
    limit = song + size;
    if (!cd_irq_ram_word(ram, entry, song) || cursor < song || cursor >= limit ||
        (cursor & 3u) || address < song || address > limit - 4u ||
        (address & 3u)) return 0;
    const uint8_t *live = musashi_boot_ram_span(ram, song, size);
    return live && !memcmp(live, mapped, size) ? song : 0;
}

/* The first levels call belongs to the current kind-5 File 1 slot. S0 and
 * S7 hold that slot; the levels iterator supplied S1/S2 and the saved link. */
static int irq_file_pan_frame(const FormatterCpu *cpu) {
    unsigned slot_index = cpu->r[30];
    uint32_t slot = cpu->level_slot;
    if (!cpu->irq_image || slot_index >= 8u || slot != 0x800a46e8u + slot_index * 0x54u ||
        cpu->r[16] != slot || cpu->r[17] != slot_index || cpu->r[18] != slot ||
        cpu->r[23] != slot || cpu->r[29] != 0x8006c9ccu) return 0;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    uint16_t kind;
    const uint8_t *paused = musashi_boot_ram_span(ram, 0x800a4f17u, 1);
    return paused && !*paused && musashi_boot_read16(ram, slot, &kind) && (kind & 63u) == 5u &&
           cd_irq_ram_word(ram, cpu->r[29] + 124u, 0x80031c90u) &&
           irq_level_stream_at(cpu, 0x80068960u) == 0x80068960u;
}

static int irq_file_pan_inputs(const FormatterCpu *cpu) {
    const MusashiGuestIrqImage *image = cpu->irq_image;
    MusashiBootMemory *ram = image->memory;
    const uint8_t *code = musashi_boot_ram_span(ram, 0x800336a8u, sizeof image->file_pan_code);
    const uint8_t *masks = musashi_boot_ram_span(ram, 0x80073140u, sizeof image->file_voice_masks);
    const uint8_t *gains = musashi_boot_ram_span(ram, 0x8006aa30u, sizeof image->file_gain_table);
    const uint8_t *pan = musashi_boot_ram_span(ram, 0x800731a0u, sizeof image->file_pan_table);
    const uint8_t *job = musashi_boot_ram_span(ram, cpu->level_slot, 0x54u);
    if (!code || !masks || !gains || !pan || !job || job[56] > 127u ||
        memcmp(code, image->file_pan_code, sizeof image->file_pan_code) ||
        memcmp(masks, image->file_voice_masks, sizeof image->file_voice_masks) ||
        memcmp(gains, image->file_gain_table, sizeof image->file_gain_table) ||
        memcmp(pan, image->file_pan_table, sizeof image->file_pan_table)) return 0;
    for (unsigned i = 0; i < 8u; ++i) {
        uint32_t voice = 0x800a4988u + i * 0x54u;
        uint16_t id;
        const uint8_t *pair;
        if (!job[58u + i]) continue;
        pair = musashi_boot_ram_span(ram, voice + 52u, 2);
        if (!pair || pair[0] > 127u || pair[1] > 127u ||
            !musashi_boot_read16(ram, voice + 10u, &id) || id >= 24u) return 0;
    }
    return 1;
}

static int irq_file_table_matches(MusashiBootMemory *ram, uint32_t address, uint32_t base,
                                  unsigned offset, const uint8_t *snapshot, unsigned width) {
    if (address != base + offset) return 0;
    const uint8_t *live = musashi_boot_ram_span(ram, address, width);
    return live && !memcmp(live, snapshot + offset, width);
}

/* Data permissions for the mounted leaf. T2 indexes eight voice records
 * and T3 walks their separate output records. The guest computes the mix. */
static int irq_file_pan_data(const FormatterCpu *cpu, unsigned op, uint32_t address) {
    uint32_t i = cpu->r[10], job = cpu->level_slot;
    if (!irq_file_pan_frame(cpu) || cpu->r[4] != job || cpu->r[31] != 0x80033a30u ||
        i >= 8u || cpu->r[9] != 0x800a4988u + i * 0x54u || cpu->r[11] != i * 0x48u)
        return 0;
    uint32_t voice = cpu->r[9], out = 0x800a4c28u + i * 0x48u;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    if (cpu->pc == 0x800336ccu) return op == 36u && address == job + 58u + i;
    const uint8_t *enabled = musashi_boot_ram_span(ram, job + 58u + i, 1);
    if (!enabled || !*enabled) return 0;
    switch (cpu->pc) {
    case 0x800336dcu: case 0x80033838u:
        return op == 37u && address == voice + 10u;
    case 0x800336f0u: {
        uint16_t id;
        return op == 35u && musashi_boot_read16(ram, voice + 10u, &id) &&
               id < 24u && irq_file_table_matches(ram, address, 0x80073140u, 4u * id,
                                                   cpu->irq_image->file_voice_masks, 4u);
    }
    case 0x800336f8u: return op == 43u && address == out;
    case 0x800336fcu: return op == 36u && address == voice + 52u;
    case 0x80033710u: {
        const uint8_t *gain = musashi_boot_ram_span(ram, voice + 52u, 1);
        return op == 37u && gain && *gain < 128u &&
               irq_file_table_matches(ram, address, 0x8006aa30u, 2u * *gain, cpu->irq_image->file_gain_table, 2u);
    }
    case 0x80033714u: return op == 33u && address == 0x800a4efau;
    case 0x80033724u: return op == 36u && address == job + 22u;
    case 0x80033734u: return op == 37u && address == job + 24u;
    case 0x80033750u:
        return op == 37u && cpu->r[3] <= 2u && address == job + 28u + (2u - cpu->r[3]) * 8u;
    case 0x80033770u: return op == 36u && address == voice + 53u;
    case 0x80033780u: return op == 36u && address == job + 56u;
    case 0x800337b8u: return op == 40u && address == voice + 82u && cpu->r[3] <= 127u;
    case 0x800337c0u: return op == 36u && address == 0x800a4f19u;
    case 0x800337d8u: case 0x800337f8u: {
        const uint8_t *p = musashi_boot_ram_span(ram, voice + 82u, 1);
        if (!p || !*p || *p > 127u) return 0;
        unsigned coefficient = cpu->pc == 0x800337d8u ? *p - 1u : 127u - *p;
        return op == 37u && irq_file_table_matches(ram, address, 0x800731a0u, 2u * coefficient,
                                                   cpu->irq_image->file_pan_table, 2u);
    }
    case 0x8003381cu:
        return op == 37u && irq_file_table_matches(ram, address, 0x800731a0u, 2u * 63u,
                                                   cpu->irq_image->file_pan_table, 2u);
    case 0x80033808u: case 0x80033830u: return op == 41u && address == out + 10u;
    case 0x80033834u: return op == 41u && address == out + 8u;
    case 0x8003383cu: return op == 36u && address == out + 68u;
    case 0x80033848u: return op == 43u && address == out + 64u;
    case 0x8003384cu: return op == 35u && address == out + 4u;
    case 0x8003385cu: case 0x80033864u: return op == 43u && address == out + 4u;
    case 0x8003386cu: return op == 40u && address == out + 68u;
    default: return 0;
    }
}

/* After the first idle voice, selector 7 polls the rest of the eight flags.
 * A1 may have advanced from the saved slot cursor across the preceding loop
 * command. An active voice uses the existing wait path. Only an exhausted
 * scan reaches the original halfword clear in the final jump's delay slot. */
static int irq_file_end_issue(const FormatterCpu *cpu, uint32_t word) {
    uint32_t slot = cpu->level_slot, index = cpu->r[30], lane = cpu->r[4];
    if (!cpu->irq_image || index >= 8u || slot != 0x800a46e8u + index * 0x54u ||
        cpu->r[29] != 0x8006c9ccu || lane == 0 || lane > 8u || cpu->r[3] != 7u)
        return 0;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    uint32_t song = 0x80068960u, last = song + sizeof cpu->irq_image->file_note - 4u;
    uint16_t kind;
    if (cpu->r[5] != last || cpu->r[6] >> 24 != 0x87u ||
        irq_level_stream_at(cpu, last) != song || !cd_irq_ram_word(ram, last, cpu->r[6]) ||
        !musashi_boot_read16(ram, slot, &kind) || (kind & 0x3fu) != 5u ||
        !cd_irq_ram_word(ram, cpu->r[29] + 124u, 0x80031c90u) ||
        !cd_irq_ram_word(ram, 0x80073120u + 7u * 4u, 0x80033d10u)) return 0;

    /* The branch delay has incremented the index. Use the returned flag
     * value, since a previously sampled flag can change between issues. */
    uint32_t pc = cpu->pc;
    int sequential = !cpu->delay_slot && !cpu->branch_pc && cpu->npc == pc + 4u;
    switch (pc) {
    case 0x80033d28u: return sequential && cpu->r[2] == 0;
    case 0x80033d2cu: return sequential && cpu->r[2] == (lane < 8u);
    case 0x80033d30u:
        return cpu->r[2] == (lane < 8u) && cpu->delay_slot &&
               cpu->branch_pc == 0x80033d2cu && cpu->npc == (lane < 8u ? 0x80033d18u : 0x80033d34u);
    case 0x80033d34u: return sequential && lane == 8u && cpu->r[2] == slot + lane;
    case 0x80033d38u:
        return lane == 8u && !cpu->r[0] && cpu->r[2] == slot + lane && cpu->delay_slot &&
               cpu->branch_pc == 0x80033d34u && cpu->npc == 0x800342b4u &&
               (word >> 26) == 41u && ((word >> 21) & 31u) == 16u &&
               ((word >> 16) & 31u) == 0u && (uint16_t)word == 0u;
    default: return 0;
    }
}

/* Bank one's last sequence row precedes the entry table. Its opening five
 * commands use the full and short note formats. Authenticate just that
 * prefix from the mounted disc and pair each operand with its load site. */
static const uint8_t *irq_opening_prefix(const FormatterCpu *cpu, uint32_t song) {
    static const uint8_t prefix_sha256[32] = {
        0x2c, 0x50, 0x67, 0x57, 0x35, 0x06, 0x11, 0x7c,
        0xdd, 0x3f, 0xa0, 0xe6, 0x90, 0xcc, 0x66, 0xee,
        0x01, 0x2c, 0x64, 0x65, 0x39, 0x8d, 0x63, 0x6c,
        0x1c, 0xfb, 0xa2, 0xef, 0xa4, 0x09, 0xbb, 0xaa
    };
    const uint32_t header = 0x800ca23cu, table = 0x800c9ac8u, prefix_size = 88u;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    uint16_t active, header_id, sound;
    if (cpu->level_slot != 0x800a46e8u || song != 0x800c9a50u ||
        !musashi_boot_read16(ram, cpu->level_slot + 4u, &sound) || sound != 0x0b5bu ||
        !musashi_boot_read16(ram, 0x800a4ef0u, &active) || active != 1u ||
        !cd_irq_ram_word(ram, 0x800a4ee8u, header) ||
        !musashi_boot_read16(ram, header, &header_id) || header_id != active ||
        !cd_irq_ram_word(ram, header + 24u, table) ||
        !cd_irq_ram_word(ram, table + 20u * 12u, song)) return NULL;
    const uint8_t *live = musashi_boot_ram_span(ram, song, prefix_size);
    uint8_t digest[32];
    return live && musashi_sha256_digest(live, prefix_size, digest) &&
           !memcmp(digest, prefix_sha256, sizeof digest) ? live : NULL;
}

static int irq_level_bank_one_note(const FormatterCpu *cpu, uint32_t word,
                                   uint32_t address, uint32_t song, uint32_t cursor) {
    /* The cursor stays at the command start while A1 advances. */
    unsigned command = cursor - song;
    int short_note = command == 60u;
    if ((command != 0u && command != 20u && command != 40u && !short_note && command != 68u) ||
        address != cpu->r[5] || address < cursor || address >= song + 88u ||
        !irq_opening_prefix(cpu, song)) return 0;

    unsigned op = word >> 26, offset = address - cursor;
    switch (cpu->pc) {
    case 0x80033ad0u: return op == 35u && offset == 0u;
    case 0x80033b18u: return !short_note && op == 35u && offset == 4u;
    case 0x80033b2cu: return !short_note && op == 35u && offset == 8u;
    case 0x80033b6cu: return !short_note && op == 37u && offset == 12u;
    case 0x80033b7cu: return !short_note && op == 35u && offset == 16u;
    case 0x80033ba0u: return short_note && op == 35u && offset == 4u;
    default: return 0;
    }
}

/* These are the typed sequence reads accepted for the opening bank. Check
 * them again after advancing devices, which can replace the mounted data. */
static int irq_opening_note_read(const FormatterCpu *cpu) {
    switch (cpu->pc) {
    case 0x80033ad0u: case 0x80033b18u: case 0x80033b2cu:
    case 0x80033b6cu: case 0x80033b7cu: case 0x80033ba0u: {
        uint16_t bank;
        return cpu->irq_image && cpu->level_slot == 0x800a46e8u &&
               musashi_boot_read16(cpu->irq_image->memory, cpu->level_slot + 52u, &bank) &&
               bank == 1u;
    }
    default: return 0;
    }
}

static int irq_level_note_read(const FormatterCpu *cpu, uint32_t word, uint32_t address) {
    if (address != cpu->r[5]) return 0;
    if (!irq_level_stream_at(cpu, address)) {
        uint16_t bank;
        uint32_t song, cursor;
        MusashiBootMemory *ram = cpu->irq_image->memory;
        if (!cpu->level_slot || cpu->r[16] != cpu->level_slot ||
            !musashi_boot_read16(ram, cpu->level_slot + 52u, &bank) || bank != 1u ||
            !musashi_boot_read32(ram, cpu->level_slot + 16u, &song) ||
            !musashi_boot_read32(ram, cpu->level_slot + 12u, &cursor)) return 0;
        return irq_level_bank_one_note(cpu, word, address, song, cursor);
    }
    unsigned op = word >> 26;
    switch (cpu->pc) {
    case 0x80033ad0u: case 0x80033b18u: case 0x80033b2cu:
    case 0x80033b7cu: case 0x80033ba0u:
        return op == 35u;
    case 0x80033b6cu:
        return op == 37u;
    default:
        return 0;
    }
}

/* The active File 1 ramp uses A2 for the current level and T0 for its
 * target. A0 indexes three eight-byte records in the caller's kind-5 slot.
 * The mapped guest instructions perform the arithmetic and the delay stores. */
static int irq_file_ramp_data(const FormatterCpu *cpu, unsigned op, uint32_t address) {
    uint32_t slot = cpu->level_slot, index = cpu->r[30], lane = cpu->r[4];
    if (index >= 8u || slot != 0x800a46e8u + index * 0x54u || lane >= 3u ||
        cpu->r[17] != index || cpu->r[18] != slot || cpu->r[23] != slot ||
        cpu->r[31] != 0x80031c90u || irq_level_stream_at(cpu, 0x80068960u) != 0x80068960u)
        return 0;
    uint32_t current = slot + 28u + lane * 8u;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    const uint8_t *enabled = musashi_boot_ram_span(ram, current + 6u, 1);
    const uint8_t *paused = musashi_boot_ram_span(ram, 0x800a4f17u, 1);
    uint16_t kind;
    if (cpu->r[6] != current || cpu->r[8] != current + 4u || !enabled || !*enabled ||
        !paused || *paused || !musashi_boot_read16(ram, slot, &kind) || (kind & 63u) != 5u)
        return 0;
    switch (cpu->pc) {
    case 0x8003392cu: case 0x8003397cu: case 0x800339acu:
        return op == 37u && address == current;
    case 0x80033954u: case 0x80033988u:
        return op == 37u && address == current + 2u;
    case 0x80033930u: case 0x80033978u: case 0x800339a8u:
        return op == 37u && address == current + 4u;
    case 0x80033944u:
        return op == 40u && address == current + 6u;
    case 0x80033960u: case 0x80033974u: case 0x800339a0u:
    case 0x800339a4u: case 0x800339c0u:
        return op == 41u && address == current;
    default: return 0;
    }
}

/* The dispatch moves the command number to V1, reads its address from
 * the installed table, and jumps through the loaded target. */
static uint32_t irq_level_case_target(uint32_t selector) {
    switch (selector) {
    case 0: return 0x80033b08u;
    case 1: return 0x80033b90u;
    case 2: return 0x80033bfcu;
    case 5: return 0x80033c54u;
    case 7: return 0x80033d10u;
    default: return 0;
    }
}

/* The full note copies its pan, track and instrument to both the stack
 * descriptor and the slot cache. The short note at +60 reuses the preceding
 * full note's cache. At the map lookup the cursor has already advanced. */
static const uint8_t *irq_opening_descriptor(const FormatterCpu *cpu) {
    if (!cpu->irq_image || !cpu->irq_image->memory) return NULL;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    uint32_t slot = cpu->level_slot, song, cursor;
    uint16_t kind, bank;
    if (slot != 0x800a46e8u || cpu->r[16] != slot || cpu->r[23] != slot ||
        cpu->r[21] >= 8u || cpu->r[29] != 0x8006c9ccu || cpu->r[20] != cpu->r[29] + 16u || !musashi_boot_read16(ram, slot, &kind) ||
        (kind & 63u) != 5u || !musashi_boot_read16(ram, slot + 52u, &bank) || bank != 1u ||
        !musashi_boot_read32(ram, slot + 16u, &song) ||
        !musashi_boot_read32(ram, slot + 12u, &cursor)) return NULL;
    unsigned offset;
    switch (cursor - song) {
    case 20u: offset = 0u; break;
    case 40u: offset = 20u; break;
    case 60u: case 68u: offset = 40u; break;
    case 88u: offset = 68u; break;
    default: return NULL;
    }
    const uint8_t *prefix = irq_opening_prefix(cpu, song);
    const uint8_t *desc = musashi_boot_ram_span(ram, cpu->r[20], 20u);
    const uint8_t *cache = musashi_boot_ram_span(ram, slot + 76u, 3u);
    if (!prefix || !desc || !cache || desc[9] != prefix[offset + 11u] ||
        desc[10] != prefix[offset + 10u] || desc[11] != prefix[offset + 9u] ||
        memcmp(cache, desc + 9u, 3u)) return NULL;
    return desc;
}

/* The normal track-4 map was populated by the last chunk of the installed
 * track. Its primary cache and the signed map agree on slot three. */
static int irq_opening_track4(MusashiBootMemory *ram) {
    uint16_t primary, mapped;
    const uint8_t *empty = musashi_boot_ram_span(ram, 0x800a4698u, 1u);
    return empty && !*empty && musashi_boot_read16(ram, 0x800a4688u, &primary) &&
           primary == 4u && musashi_boot_read16(ram, 0x800c5338u, &mapped) && mapped == 3u;
}

/* The third full opening note carries a pointer to its motion record at
 * descriptor+16. The normal caller writes that same pointer to channel+20.
 * Authenticate both the record and the registered stream before using it. */
static int irq_opening_motion_owner(const FormatterCpu *cpu) {
    static const uint8_t motion_digest[32] = {
        0xa9, 0xde, 0xd1, 0xe5, 0x31, 0x92, 0x23, 0xdb,
        0xe5, 0xf1, 0x03, 0x6f, 0x45, 0xe1, 0xc9, 0xd3,
        0x37, 0x4d, 0xf1, 0x49, 0x84, 0x61, 0x0f, 0x31,
        0x54, 0xd4, 0x94, 0xc2, 0x6c, 0xdf, 0x2b, 0x1e
    };
    const uint8_t *desc = irq_opening_descriptor(cpu);
    if (!desc || desc[10] != 4u || desc[11] != 9u || cpu->r[21] != 2u ||
        cpu->r[17] != 0x800a4a30u || cpu->r[18] != 0x800a4cb8u ||
        cpu->r[22] != 3u || cpu->r[31] != 0x80033fa4u) return 0;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    const uint8_t *motion = musashi_boot_ram_span(ram, 0x800c9a34u, 14u);
    uint16_t pitch, transpose;
    uint8_t digest[32];
    return irq_opening_track4(ram) && cd_irq_ram_word(ram, cpu->level_slot + 12u, 0x800c9a8cu) &&
           cd_irq_ram_word(ram, cpu->r[20] + 16u, 0x800c9a34u) &&
           cd_irq_ram_word(ram, cpu->r[17] + 20u, 0x800c9a34u) &&
           musashi_boot_read16(ram, 0x800c9a78u, &pitch) &&
           musashi_boot_read16(ram, cpu->level_slot + 82u, &transpose) && !transpose &&
           cd_irq_ram_word(ram, cpu->r[17] + 36u, pitch) &&
           cd_irq_ram_word(ram, 0x800730e4u, 0x80031898u) &&
           motion && musashi_sha256_digest(motion, 14u, digest) && !memcmp(digest, motion_digest, sizeof digest);
}

static int irq_opening_motion_return(const FormatterCpu *cpu) {
    if (!irq_opening_motion_owner(cpu) || cpu->r[3] != 1u || cpu->r[5] != cpu->r[17]) return 0;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    uint32_t ch = cpu->r[17], cmd = 0x800c9a34u, pitch;
    const uint8_t *flags = musashi_boot_ram_span(ram, ch + 55u, 2u);
    uint16_t amount, offset, count, reload, command_count, command_reload;
    return flags && flags[0] == 1u && !flags[1] && cd_irq_ram_word(ram, ch + 24u, 0u) &&
           musashi_boot_read32(ram, ch + 36u, &pitch) && cd_irq_ram_word(ram, ch + 32u, pitch) &&
           musashi_boot_read16(ram, cmd + 4u, &amount) && cpu->r[6] == amount && cpu->r[2] == 0u - amount &&
           musashi_boot_read16(ram, cmd + 6u, &offset) && cpu->r[4] == (uint16_t)(pitch + offset) &&
           cd_irq_ram_word(ram, ch + 40u, cpu->r[4]) && cd_irq_ram_word(ram, ch + 44u, cpu->r[2]) &&
           musashi_boot_read16(ram, cmd + 2u, &command_count) && musashi_boot_read16(ram, ch + 48u, &count) &&
           count == command_count && musashi_boot_read16(ram, cmd + 12u, &command_reload) &&
           musashi_boot_read16(ram, ch + 18u, &reload) && reload == command_reload;
}

/* Execute the mounted instructions using the normal guest clock. A2 holds
 * the record pointer until the common tail extracts a 16-bit ramp amount. */
static int irq_opening_motion_issue(const FormatterCpu *cpu, uint32_t word) {
    uint32_t pc = cpu->pc, ch = cpu->r[17], cmd = 0x800c9a34u;
    uint16_t amount;
    if (!irq_opening_motion_owner(cpu) ||
        (!irq_motion_eight_pc(pc) && !(pc >= 0x800316f8u && pc <= 0x80031718u) &&
         pc != 0x80031980u && pc != 0x80031984u) ||
        (pc != 0x800316f8u && cpu->r[5] != ch) ||
        (pc >= 0x80031714u && pc <= 0x80031938u && cpu->r[6] != cmd) ||
        (pc >= 0x8003193cu && (!musashi_boot_read16(cpu->irq_image->memory, cmd + 4u, &amount) ||
                                cpu->r[6] != amount))) return 0;
    if (pc == 0x80031980u || pc == 0x80031984u) return irq_opening_motion_return(cpu);
    unsigned op = word >> 26;
    if (op < 32u) return 1;
    uint32_t address = cpu->r[(word >> 21) & 31u] + (uint32_t)(int32_t)(int16_t)word;
    switch (pc) {
    case 0x80031700u: return op == 35u && address == ch + 20u;
    case 0x80031704u: case 0x80031944u: case 0x80031958u: return op == 35u && address == ch + 36u;
    case 0x8003170cu: case 0x8003196cu: return op == 40u && address == ch + 55u;
    case 0x80031710u: return op == 43u && address == ch + 24u;
    case 0x80031718u: return op == 43u && address == ch + 32u &&
        cd_irq_ram_word(cpu->irq_image->memory, ch + 36u, cpu->r[2]);
    case 0x8003171cu: return op == 37u && address == cmd;
    case 0x80031738u: return op == 35u && cpu->r[3] == 8u && address == 0x800730e4u;
    case 0x80031898u: return op == 40u && address == ch + 56u;
    case 0x8003189cu: return op == 37u && address == cmd + 12u;
    case 0x800318a0u: return op == 37u && address == cmd + 4u;
    case 0x800318a4u: return op == 37u && address == cmd + 6u;
    case 0x800318a8u: return op == 41u && address == ch + 18u;
    case 0x800318acu: return op == 35u && address == ch + 32u;
    case 0x800318b0u: return op == 37u && address == cmd + 2u;
    case 0x800318bcu: return op == 41u && address == ch + 48u;
    case 0x80031954u: return op == 36u && address == ch + 55u;
    case 0x8003195cu: return op == 43u && address == ch + 40u;
    case 0x80031978u: return op == 43u && address == ch + 44u && cpu->r[2] == 0u - cpu->r[6];
    default: return 0;
    }
}

/* Map publication precedes instrument installation. The opening prefix selects
 * these three instruments on track four. Verify the complete installed row. */
static int irq_opening_instrument(const FormatterCpu *cpu, const uint8_t *desc) {
    static const struct {
        uint8_t instrument;
        uint8_t digest[32];
    } installed[] = {
        { 9u, {
            0xfb, 0xa7, 0xed, 0x42, 0xf4, 0x2a, 0x0a, 0xa4,
            0x0a, 0x99, 0xba, 0x81, 0x06, 0x50, 0xa9, 0x04,
            0x80, 0x18, 0x6a, 0xfa, 0xf7, 0xcb, 0x17, 0x8c,
            0xec, 0xce, 0xa1, 0x88, 0x60, 0xa5, 0x7a, 0x2b
        } },
        { 10u, {
            0x45, 0x88, 0x5b, 0x68, 0x42, 0x72, 0xd9, 0xa8,
            0x2d, 0x53, 0x58, 0xdb, 0x1d, 0x1e, 0x80, 0x04,
            0x2c, 0xbf, 0x64, 0xee, 0x61, 0xf5, 0x8b, 0xd6,
            0xd6, 0x4c, 0x7c, 0x8f, 0x88, 0x94, 0xff, 0x33
        } },
        { 12u, {
            0xcd, 0x96, 0xd3, 0x2d, 0x44, 0x74, 0xdd, 0xa2,
            0x4e, 0x30, 0x9c, 0x58, 0xc8, 0x31, 0x01, 0x77,
            0x70, 0x95, 0xc6, 0xe1, 0x7a, 0x70, 0x59, 0xd8,
            0xef, 0x05, 0x77, 0xd6, 0xb0, 0x82, 0x71, 0x04
        } },
    };
    unsigned channel;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    if (!desc || desc[10] != 4u || cpu->r[22] != 3u ||
        !irq_level_leaf_channel(cpu, &channel) || cpu->r[21] != channel ||
        !irq_opening_track4(ram) || !cd_irq_ram_word(ram, 0x8006a97cu, 0x800a6180u)) return 0;
    for (size_t i = 0; i < sizeof installed / sizeof installed[0]; ++i) {
        if (installed[i].instrument == desc[11]) {
            const uint8_t *row = musashi_boot_ram_span(ram, 0x800a6180u + 12u * desc[11], 12u);
            uint8_t digest[32];
            return row && musashi_sha256_digest(row, 12u, digest) &&
                   !memcmp(digest, installed[i].digest, sizeof digest);
        }
    }
    return 0;
}

static int irq_opening_pan(const FormatterCpu *cpu, uint32_t word) {
    const uint8_t *desc = irq_opening_descriptor(cpu);
    if (cpu->r[31] != 0x80033fa4u || !irq_opening_instrument(cpu, desc)) return 0;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    const uint8_t *bias = musashi_boot_ram_span(ram, cpu->level_slot + 56u, 1u);
    const uint8_t *channel = musashi_boot_ram_span(ram, cpu->r[17], 0x54u);
    const uint8_t *stereo = musashi_boot_ram_span(ram, 0x800a4f19u, 1u);
    if (!bias || *bias > 127u || !stereo || *stereo > 1u || !channel || !desc[9] ||
        desc[9] > 127u || desc[9] != channel[53] || desc[9] != channel[82]) return 0;
    unsigned pan = desc[9];
    if (*bias) {
        unsigned sum = pan + *bias;
        pan = sum < 66u ? 1u : sum - 64u;
        if (pan > 127u) pan = 127u;
    }
    if (cpu->r[4] != pan || cpu->r[19] != pan ||
        (cpu->pc >= 0x80034108u && (*stereo != (cpu->pc < 0x80034158u)))) return 0;

    unsigned op = word >> 26;
    if (op < 32u) return 1;
    uint32_t address = cpu->r[(word >> 21) & 31u] + (uint32_t)(int32_t)(int16_t)word;
    unsigned index;
    switch (cpu->pc) {
    case 0x800340f8u: return op == 36u && address == 0x800a4f19u;
    case 0x80034110u: index = pan - 1u; break;
    case 0x80034134u: index = 127u - pan; break;
    case 0x8003415cu: index = 63u; break;
    case 0x80034144u: case 0x80034170u: return op == 41u && address == cpu->r[18] + 10u;
    case 0x80034154u: case 0x80034178u: return op == 41u && address == cpu->r[18] + 8u;
    default: return 0;
    }
    return op == 37u && irq_file_table_matches(ram, address, 0x800731a0u, 2u * index,
                                               cpu->irq_image->file_pan_table, 2u);
}

/* A note has selected an instrument before these reads. Validate the
 * installed bank pointer and the descriptor's instrument number together.
 * Each instrument record is twelve bytes. */
static int irq_level_instrument(const FormatterCpu *cpu, uint32_t *row) {
    MusashiBootMemory *ram = cpu->irq_image->memory;
    const uint8_t *instrument = musashi_boot_ram_span(ram, cpu->r[29] + 27u, 1);
    uint32_t bank = cpu->r[22], base;
    if (!instrument || cpu->r[20] != cpu->r[29] + 16u) return 0;
    if (bank == 0 && *instrument == 26u) base = 0x800a5f68u;
    else if (bank == 3u && (*instrument == 0u || *instrument == 3u)) base = 0x800a6180u;
    else if (bank == 3u && cpu->r[31] == 0x80033d6cu &&
             irq_opening_instrument(cpu, irq_opening_descriptor(cpu))) base = 0x800a6180u;
    else return 0;
    if (!cd_irq_ram_word(ram, 0x8006a970u + bank * 4u, base)) return 0;
    *row = base + 12u * *instrument;
    return 1;
}


/* A single sequence command refers to the selected slot, the chosen
 * channel/mixer pair, its local note descriptor, and a bank instrument row.
 * The issue word has already been compared to the mounted executable. */
static int irq_level_slot_data(const FormatterCpu *cpu, uint32_t word, uint32_t address) {
    uint32_t slot = cpu->level_slot, pc = cpu->pc;
    unsigned op = word >> 26, base = (word >> 21) & 31u;
    unsigned width = op == 32u || op == 36u || op == 40u ? 1u :
                     op == 33u || op == 37u || op == 41u ? 2u :
                     op == 35u || op == 43u ? 4u : 0u;
    if (!slot || !width || (address & (width - 1u)) || cpu->r[29] != 0x8006c9ccu) return 0;

    /* The opening sequence has a registered bank in slot zero. This load
     * compares its ID with the live registry before the guest rejoins the
     * common update. A different bank would take a separate shutdown path. */
    if (pc == 0x800338d4u) {
        uint16_t kind, bank, registry_id, header_id, sound;
        MusashiBootMemory *ram = cpu->irq_image->memory;
        return op == 33u && base == 2u && cpu->r[2] == 0x800a0000u &&
               address == 0x800a4ef0u && slot == 0x800a46e8u &&
               cpu->r[16] == slot && cpu->r[3] == 1u &&
               musashi_boot_read16(ram, slot, &kind) && (kind & 63u) == 5u &&
               musashi_boot_read16(ram, slot + 4u, &sound) && sound == 0x0b5bu &&
               musashi_boot_read16(ram, slot + 52u, &bank) && bank == 1u &&
               musashi_boot_read16(ram, address, &registry_id) && registry_id == bank &&
               cd_irq_ram_word(ram, 0x800a4ee8u, 0x800ca23cu) &&
               musashi_boot_read16(ram, 0x800ca23cu, &header_id) && header_id == bank &&
               cd_irq_ram_word(ram, slot + 16u, 0x800c9a50u);
    }
    if (pc >= 0x8003392cu && pc <= 0x800339c0u)
        return irq_file_ramp_data(cpu, op, address);
    uint32_t frame = cpu->r[29], width_end = address + width;
    if (width_end <= address) return 0;
    if (base == 29u && ((address >= frame + 88u && width_end <= frame + 128u) ||
                        (address >= frame + 16u && width_end <= frame + 36u))) return 1;
    if (base == 20u && cpu->r[20] == frame + 16u && op < 40u &&
        address >= frame + 16u && width_end <= frame + 36u) return 1;
    if (base == 16u && cpu->r[16] == slot && address >= slot && width_end <= slot + 0x54u)
        return 1;
    if (pc == 0x8003391cu || pc == 0x800340c4u) {
        if (cpu->r[4] >= 3u) return 0;
        uint32_t ramp = slot + 28u + cpu->r[4] * 8u;
        return pc == 0x8003391cu ? op == 36u && address == ramp + 6u :
                                    op == 37u && address == ramp;
    }
    unsigned channel;
    if (pc >= 0x80033df0u && irq_level_leaf_channel(cpu, &channel) && cpu->r[21] == channel) {
        if (base == 17u && address >= cpu->r[17] && width_end <= cpu->r[17] + 0x54u) return 1;
        if (base == 18u && address >= cpu->r[18] && width_end <= cpu->r[18] + 0x48u) return 1;
        if (pc == 0x80033fa0u) return op == 40u && address == slot + 58u + channel;
        if (pc == 0x80033fc4u) return op == 35u && address == 0x80073180u + 4u * channel;
    }
    if (base == 5u) return irq_level_note_read(cpu, word, address);
    if (pc == 0x80033a78u || pc == 0x80033d18u)
        return op == 36u && cpu->r[16] == slot && cpu->r[4] < 8u &&
               address == slot + 58u + cpu->r[4];
    if (op >= 40u) return 0;
    uint32_t row;
    switch (pc) {
    case 0x80033af8u:
        return op == 35u && irq_level_case_target(cpu->r[3]) &&
               address == 0x80073120u + 4u * cpu->r[3];
    case 0x80033d98u:
        if (cpu->r[19] == 4u) {
            const uint8_t *desc = irq_opening_descriptor(cpu);
            return desc && desc[10] == 4u && cpu->r[31] == 0x80033d6cu &&
                   op == 33u && address == 0x800c5338u &&
                   irq_opening_track4(cpu->irq_image->memory);
        }
        return op == 33u && (cpu->r[19] == 0u || cpu->r[19] == 0x57u) &&
               address == 0x800c5328u + 4u * cpu->r[19];
    case 0x80033e04u:
        return op == 35u && irq_level_instrument(cpu, &row) &&
               address == 0x8006a970u + 4u * cpu->r[22];
    case 0x80033e10u: case 0x80033e1cu: case 0x80033e28u: case 0x80033e34u: {
        if (!irq_level_instrument(cpu, &row) || cpu->r[3] != row) return 0;
        uint32_t offset = pc == 0x80033e1cu ? 4u : pc == 0x80033e28u ? 6u :
                          pc == 0x80033e34u ? 8u : 0u;
        return op == (offset ? 37u : 35u) && address == row + offset;
    }
    /* The interpolator tested its unsigned input against 0x5300 before
     * calculating A0 = 2 * (input >> 8). It reads adjacent halfwords. */
    case 0x80034004u: case 0x8003401cu:
        return op == 37u && !(cpu->r[4] & 1u) && cpu->r[4] <= 2u * 0x52u &&
               address == (pc == 0x80034004u ? 0x8006ab30u : 0x8006ab32u) + cpu->r[4];
    case 0x80034058u: return op == 36u && address == 0x800a4f1eu;
    case 0x80034084u: {
        if (!irq_level_leaf_channel(cpu, &channel) || cpu->r[21] != channel) return 0;
        const uint8_t *index = musashi_boot_ram_span(cpu->irq_image->memory, cpu->r[17] + 0x34u, 1);
        return op == 37u && index && *index < 128u && cpu->r[2] == 2u * *index &&
               address == 0x8006aa30u + cpu->r[2];
    }
    case 0x8003408cu: return op == 33u && address == 0x800a4efau;
    case 0x8003419cu: return op == 33u && address == 0x800a4ef6u;
    default: return 0;
    }
}

/* Eight software channel records share one resident iterator. Its indexed
 * mixer record uses a different stride; the hardware voice is a field. */
static uint32_t irq_channel_record(const FormatterCpu *cpu) {
    unsigned i = cpu->r[21];
    if (i >= 8u) return 0;
    uint32_t channel = 0x800a4988u + i * 0x54u;
    if (cpu->r[18] != channel || cpu->r[16] != channel + 10u ||
        cpu->r[19] != i * 0x48u || cpu->r[20] != 0x80073140u ||
        cpu->r[22] != 0x800a4efeu || cpu->r[23] != 0x800a4efau)
        return 0;
    return channel;
}

static int irq_channel_update_frame(const FormatterCpu *cpu, uint32_t channel) {
    uint32_t saved_return;
    return channel && cpu->irq_image && cpu->irq_image->memory &&
        cpu->r[29] == 0x8006ca3cu &&
        musashi_boot_read32(cpu->irq_image->memory, cpu->r[29]+52u, &saved_return) &&
        saved_return == 0x8002d014u;
}

/* The ordinary inactive-channel loop (31448 onward) retains its earlier
 * validation. Data instructions in the active portion always use this
 * function, including its refusal for an unowned access. */
static int irq_channel_active_data(const FormatterCpu *cpu, uint32_t word, uint32_t address) {
    uint32_t channel = irq_channel_record(cpu);
    if (!irq_channel_update_frame(cpu, channel)) return 0;
    uint32_t pc = cpu->pc, value = cpu->r[(word >> 16) & 31u];
    unsigned op = word >> 26;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    uint16_t voice, remaining;
    uint32_t count;
    const uint8_t *mode;

    switch (pc) {
    case 0x80030fecu:
        return op == 35u && address == channel+4u;
    case 0x80030ff8u:
        return op == 43u && address == channel+4u &&
            musashi_boot_read32(ram, address, &count) && value == count+1u;
    case 0x80030ffcu:
        return op == 35u && address == channel;
    case 0x80031010u:
        return op == 43u && address == channel &&
            musashi_boot_read32(ram,address,&count) && count && value == count-1u;
    case 0x80031040u:
        return op == 36u && address == channel+0x4eu;
    case 0x80031058u:
        mode = musashi_boot_ram_span(ram, channel+0x4eu, 1);
        return op == 40u && address == channel+0x4eu && mode && *mode >= 2u &&
            value == (uint32_t)*mode-1u;
    case 0x80031014u:
    case 0x8003105cu:
    case 0x8003109cu:
    case 0x80031400u:
        return op == 37u && address == channel+0x0au;
    case 0x80031068u:
        return op == 36u && musashi_boot_read16(ram, channel+10u, &voice) &&
            voice < 24u && address == 0x800a4efeu+voice;
    case 0x80031024u:
    case 0x800310acu:
    case 0x80031410u:
        return op == 35u && musashi_boot_read16(ram, channel+10u, &voice) &&
            voice < 24u && address == 0x80073140u+4u*voice;
    case 0x800310b4u:
    case 0x8003141cu:
        return op == 40u && address == channel+0x50u && value == 0u;
    case 0x800310b8u:
    case 0x80031420u:
        return op == 35u && address == channel+0x40u;
    case 0x800310c4u:
        return op == 40u && address == channel+0x4eu && value == 0;
    case 0x800310c8u:
    case 0x80031430u:
        return op == 36u && address == channel+0x51u;
    case 0x800310ccu:
    case 0x80031434u:
        return op == 35u && address == channel+0x44u;
    case 0x800310dcu:
    case 0x80031444u:
        return op == 43u && address == channel+0x40u && value == 0;
    case 0x800310e0u:
        return op == 36u && address == channel+0x50u;
    case 0x800310fcu:
        return op == 36u && address == channel+0x4cu;
    case 0x8003115cu:
        return op == 36u && address == channel+0x4fu;
    case 0x80031174u:
        return op == 36u && address == channel+0x37u;
    case 0x800311e8u:
        return op == 36u && address == 0x800a4f19u;
    case 0x800311f8u:
        return op == 36u && address == channel+0x53u;
    case 0x800313d8u:
        return op == 37u && address == channel+0x12u;
    case 0x800313e8u:
        return op == 41u && address == channel+0x12u &&
            musashi_boot_read16(ram,address,&remaining) && remaining && value == (uint32_t)remaining-1u;
    default:
        return 0;
    }
}

/* The game flushes its eight mixer records from the sound VBlank frame.
 * S2 is the record index. The first pass points S0 at the pending key-off
 * flag, the second points S1 at the record and S0 at its change flags. */
static int irq_title_mix_access(const FormatterCpu *cpu, unsigned op, uint32_t address) {
    if (cpu->r[29] != 0x8006ca54u) return 0;
    uint32_t pc = cpu->pc;
    if (pc == 0x8002d3bcu || pc == 0x8002d3c4u)
        return address == 0x800a2b98u && op == (pc == 0x8002d3bcu ? 37u : 43u);
    if (pc == 0x8002d3ecu || pc == 0x8002d3f4u)
        return address == 0x800a2ba0u && op == (pc == 0x8002d3ecu ? 37u : 43u);
    if (pc == 0x8002d460u || pc == 0x8002d468u)
        return address == 0x800c7d20u && op == (pc == 0x8002d460u ? 37u : 43u);
    if (pc == 0x8002d490u || pc == 0x8002d498u)
        return address == 0x800c7d2cu && op == (pc == 0x8002d490u ? 37u : 43u);

    uint32_t index = cpu->r[18];
    if (index >= 8u) return 0;
    uint32_t voice = 0x800a4c28u + index * 0x48u;
    if (pc == 0x8002d364u || pc == 0x8002d378u || pc == 0x8002d384u) {
        if (cpu->r[16] != voice + 0x45u) return 0;
        if (pc == 0x8002d364u) return op == 35u && address == voice + 4u;
        if (pc == 0x8002d378u) return op == 35u && address == voice + 0x40u;
        return op == 40u && address == voice + 0x45u;
    }
    if (cpu->r[17] != voice || cpu->r[16] != voice + 4u) return 0;
    if (pc == 0x8002d418u) return op == 35u && address == voice + 0x40u;
    if (pc == 0x8002d420u) return op == 40u && address == voice + 0x44u;
    return pc == 0x8002d424u && op == 43u && address == voice + 4u;
}

/* The channel caller owns S0, S2 and the channel index across both helpers.
 * Each leaf still retires its checked instructions on the active guest clock. */
static int irq_channel_leaf_pc(uint32_t pc) {
    if (pc & 3u) return 0;
    return (pc >= 0x800314dcu && pc <= 0x800314f0u) ||
           (pc >= 0x800316ecu && pc <= 0x800316f4u) ||
           (pc >= 0x800342e8u && pc <= 0x80034310u);
}

static int irq_channel_callback_site(uint32_t pc) {
    return pc == 0x800310d0u || pc == 0x80031438u;
}

static int irq_channel_steady(const FormatterCpu *cpu, uint32_t channel) {
    const uint8_t *flags = musashi_boot_ram_span(cpu->irq_image->memory, channel + 0x37u, 1);
    return flags && !(*flags & 1u);
}

/* The sequencer installs the callback, owning slot index and voice index in
 * the selected channel. A later note can replace these between clock ticks. */
static int irq_channel_callback_args(const FormatterCpu *cpu, uint32_t channel, int entry) {
    MusashiBootMemory *memory = cpu->irq_image->memory;
    uint32_t callback, slot;
    const uint8_t *voice = musashi_boot_ram_span(memory, channel + 0x51u, 1);
    return voice && *voice < 8u && cpu->r[4] == *voice &&
           musashi_boot_read32(memory, channel + 0x40u, &callback) && callback == 0x800342e8u &&
           musashi_boot_read32(memory, channel + 0x44u, &slot) && slot < 8u &&
           (!entry || cpu->r[5] == slot);
}

static int irq_channel_leaf_call(const FormatterCpu *cpu, uint32_t word,
                                 uint32_t target, uint32_t channel) {
    if (!channel || channel != irq_channel_record(cpu) || cpu->r[29] != 0x8006ca3cu) return 0;
    if (cpu->pc == 0x800313d0u)
        return word >> 26 == 3u && target == 0x800314dcu && irq_channel_steady(cpu, channel);
    return irq_channel_callback_site(cpu->pc) && word >> 26 == 0u &&
           (word & 63u) == 9u && ((word >> 21) & 31u) == 2u && ((word >> 11) & 31u) == 31u &&
           target == 0x800342e8u && cpu->r[2] == target && irq_channel_callback_args(cpu, channel, 0);
}

/* Checked after the branch and actual delay slot. The direct caller prepares
 * A0 in that delay. The indirect caller loads A1 immediately before JALR,
 * so the new slot value is required at entry, after the load delay. */
static int irq_channel_leaf_entry(const FormatterCpu *cpu, uint32_t channel) {
    if (!channel || channel != irq_channel_record(cpu) || cpu->r[29] != 0x8006ca3cu) return 0;
    if (cpu->pc == 0x800314dcu)
        return cpu->r[31] == 0x800313d8u && cpu->r[4] == channel && irq_channel_steady(cpu, channel);
    return cpu->pc == 0x800342e8u && irq_channel_callback_site(cpu->r[31] - 8u) &&
           irq_channel_callback_args(cpu, channel, 1);
}

static uint32_t irq_channel_release_byte(const FormatterCpu *cpu) {
    return 0x800a46e8u + 0x54u * cpu->r[5] + 0x3au + cpu->r[4];
}

static int irq_channel_leaf_return(const FormatterCpu *cpu, uint32_t target,
                                   uint32_t channel) {
    if (!channel || channel != irq_channel_record(cpu) || cpu->r[29] != 0x8006ca3cu ||
        cpu->r[31] != target) return 0;
    if (cpu->pc == 0x800316f0u)
        return target == 0x800313d8u && cpu->r[6] == channel && cpu->r[2] == 0;
    return cpu->pc == 0x8003430cu && irq_channel_callback_site(target - 8u) &&
           irq_channel_callback_args(cpu, channel, 1) && cpu->r[2] + 0x3au == irq_channel_release_byte(cpu);
}

static int irq_channel_leaf_data(const FormatterCpu *cpu, uint32_t word,
                                 uint32_t address, uint32_t channel) {
    if (!channel || channel != irq_channel_record(cpu) || cpu->r[29] != 0x8006ca3cu) return 0;
    if (cpu->pc == 0x800314e0u)
        return cpu->r[31] == 0x800313d8u && cpu->r[6] == channel && word >> 26 == 36u &&
               address == channel + 0x37u && irq_channel_steady(cpu, channel);
    return cpu->pc == 0x80034310u && cpu->delay_slot && cpu->branch_pc == 0x8003430cu &&
           irq_channel_callback_site(cpu->r[31] - 8u) && word >> 26 == 40u &&
           ((word >> 16) & 31u) == 0u && irq_channel_callback_args(cpu, channel, 1) &&
           address == irq_channel_release_byte(cpu);
}

/* The channel update calls the mask helper from three sites. The installed
 * note slot has a distinct call frame for its own invocation. */
static int irq_channel_mask_pc(uint32_t pc) {
    return !(pc & 3u) &&
           ((pc >= 0x8002eff8u && pc <= 0x8002f030u) ||
            (pc >= 0x8002f034u && pc <= 0x8002f060u));
}

static int irq_channel_mask_site(uint32_t pc) {
    return pc == 0x80031028u || pc == 0x800310b0u || pc == 0x80031418u;
}

static uint32_t irq_channel_voice_mask(const FormatterCpu *cpu, uint32_t channel) {
    uint16_t voice;
    uint32_t mask;
    if (!channel || irq_channel_record(cpu) != channel || cpu->r[29] != 0x8006ca3cu ||
        cpu->r[20] != 0x80073140u ||
        !musashi_boot_read16(cpu->irq_image->memory, channel + 0x0au, &voice) ||
        voice != 16u + cpu->r[21] || voice >= 24u ||
        !musashi_boot_read32(cpu->irq_image->memory, 0x80073140u + 4u * voice, &mask) ||
        mask != (1u << voice)) return 0;
    return mask;
}

static int irq_channel_mask_call(const FormatterCpu *cpu, uint32_t word,
                                 uint32_t target, uint32_t channel) {
    uint32_t mask = irq_channel_voice_mask(cpu, channel);
    if (!mask || !irq_channel_mask_site(cpu->pc) || (word >> 26) != 3u ||
        target != 0x8002eff8u ||
        cpu->r[2] != 0x80073180u + 4u * cpu->r[21]) return 0;
    /* The countdown caller prepares A0 in its delay slot. Both it and the
     * release caller at 310b0 have a pending A1 load at JAL issue. */
    if (cpu->pc == 0x80031028u) {
        uint32_t countdown;
        return musashi_boot_read32(cpu->irq_image->memory, channel, &countdown) && !countdown;
    }
    return cpu->r[4] == 0 && (cpu->pc == 0x800310b0u || cpu->r[5] == mask);
}

static int irq_channel_mask_entry(const FormatterCpu *cpu, uint32_t channel) {
    uint32_t mask = irq_channel_voice_mask(cpu, channel);
    if (!mask || cpu->pc != 0x8002eff8u || !irq_channel_mask_site(cpu->r[31] - 8u) ||
        cpu->r[5] != mask || cpu->r[2] != 0x80073180u + 4u * cpu->r[21]) return 0;
    if (cpu->r[31] == 0x80031030u) {
        uint32_t countdown;
        return cpu->r[4] == 1u &&
               musashi_boot_read32(cpu->irq_image->memory, channel, &countdown) && !countdown;
    }
    const uint8_t *flag = musashi_boot_ram_span(cpu->irq_image->memory, channel + 0x50u, 1);
    return !cpu->r[4] && flag && !*flag;
}

static int irq_channel_mask_data(const FormatterCpu *cpu, uint32_t word,
                                 uint32_t address, uint32_t channel) {
    uint32_t before, mask = irq_channel_voice_mask(cpu, channel);
    if (!mask || !irq_channel_mask_site(cpu->r[31] - 8u) ||
        cpu->r[5] != mask || cpu->r[2] != ~mask) return 0;
    int enable = cpu->r[31] == 0x80031030u;
    uint32_t clear_addr = enable ? 0x800a2b98u : 0x800c7d20u;
    uint32_t set_addr = enable ? 0x800c7d20u : 0x800a2b98u;
    switch (cpu->pc) {
    case 0x8002f008u: case 0x8002f038u:
        return (cpu->pc == 0x8002f008u) == enable &&
               (word >> 26) == 35u && address == clear_addr;
    case 0x8002f010u: case 0x8002f040u:
        return (cpu->pc == 0x8002f010u) == enable &&
               (word >> 26) == 35u && address == set_addr;
    case 0x8002f020u: case 0x8002f050u:
        return (cpu->pc == 0x8002f020u) == enable &&
               (word >> 26) == 43u && address == clear_addr &&
               musashi_boot_read32(cpu->irq_image->memory, address, &before) &&
               cpu->r[3] == (before & ~mask);
    case 0x8002f028u: case 0x8002f058u:
        return (cpu->pc == 0x8002f028u) == enable &&
               (word >> 26) == 43u && address == set_addr &&
               musashi_boot_read32(cpu->irq_image->memory, address, &before) &&
               cpu->r[4] == (before | mask);
    default: return 0;
    }
}

static int irq_channel_mask_return(const FormatterCpu *cpu, uint32_t target,
                                   uint32_t channel) {
    uint32_t cleared, set, mask = irq_channel_voice_mask(cpu, channel);
    int enable = target == 0x80031030u;
    uint32_t clear_addr = enable ? 0x800a2b98u : 0x800c7d20u;
    uint32_t set_addr = enable ? 0x800c7d20u : 0x800a2b98u;
    return mask && cpu->pc == 0x8002f05cu && cpu->r[31] == target &&
           irq_channel_mask_site(target - 8u) && cpu->r[2] == ~mask &&
           cpu->r[5] == mask && !(cpu->r[3] & mask) && (cpu->r[4] & mask) &&
           musashi_boot_read32(cpu->irq_image->memory, clear_addr, &cleared) && cleared == cpu->r[3] &&
           musashi_boot_read32(cpu->irq_image->memory, set_addr, &set) && set == cpu->r[4];
}

/* The channel countdown dispatches the same game helper used while a note
 * is installed. This caller owns the record in S2 and supplies A0 in its
 * JAL delay. The earlier note caller has its own frame and permissions. */
static int irq_channel_command_ready(const FormatterCpu *cpu, uint32_t channel) {
    uint32_t command;
    return irq_channel_update_frame(cpu, channel) &&
        musashi_boot_read32(cpu->irq_image->memory, channel+0x14u, &command) && !command;
}

static int irq_channel_command_call(const FormatterCpu *cpu, uint32_t w, uint32_t target) {
    uint32_t channel = irq_channel_record(cpu), mask, remaining;
    uint16_t voice;
    if (cpu->pc != 0x80031030u || w >> 26 != 3u || target != 0x800316f8u ||
        cpu->r[31] != 0x80031030u || !irq_channel_command_ready(cpu, channel)) return 0;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    return musashi_boot_read32(ram, channel, &remaining) && !remaining &&
        musashi_boot_read16(ram, channel+10u, &voice) && voice < 24u &&
        musashi_boot_read32(ram, 0x80073140u+4u*voice, &mask) &&
        mask == (1u << voice) && cpu->r[4] == mask && cpu->r[5] == mask;
}

static int irq_channel_command_entry(const FormatterCpu *cpu) {
    uint32_t channel = irq_channel_record(cpu);
    return cpu->pc == 0x800316f8u && cpu->r[31] == 0x80031038u &&
        cpu->r[4] == channel && irq_channel_command_ready(cpu,channel);
}

static int irq_channel_command_data(const FormatterCpu *cpu, uint32_t w, uint32_t address) {
    uint32_t channel = irq_channel_record(cpu), note;
    unsigned op = w >> 26, rt = (w >> 16) & 31u;
    if (cpu->r[31] != 0x80031038u || cpu->r[5] != channel ||
        !irq_channel_command_ready(cpu,channel)) return 0;
    switch (cpu->pc) {
    case 0x80031700u: return op == 35u && address == channel+0x14u;
    case 0x80031704u: return op == 35u && address == channel+0x24u;
    case 0x8003170cu: return op == 40u && !rt && address == channel+0x37u;
    case 0x80031710u: return op == 43u && !rt && address == channel+0x18u;
    case 0x80031718u: return op == 43u && rt == 2u && address == channel+0x20u &&
        musashi_boot_read32(cpu->irq_image->memory,channel+0x24u,&note) && cpu->r[2] == note;
    default: return 0;
    }
}

static int irq_channel_command_return(const FormatterCpu *cpu, uint32_t target) {
    uint32_t channel = irq_channel_record(cpu), note, current, step;
    if (cpu->pc != 0x80031980u || target != 0x80031038u || cpu->r[31] != target ||
        cpu->r[4] != channel || cpu->r[5] != channel || cpu->r[6] || cpu->r[7] ||
        cpu->r[8] || !irq_channel_command_ready(cpu,channel)) return 0;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    const uint8_t *flags = musashi_boot_ram_span(ram,channel+0x37u,1);
    return flags && !*flags && musashi_boot_read32(ram,channel+0x18u,&step) && !step &&
        musashi_boot_read32(ram,channel+0x24u,&note) && cpu->r[2] == note &&
        musashi_boot_read32(ram,channel+0x20u,&current) && current == note;
}

/* A channel can register this second callback to finish a type-1 sound
 * record. Its field number comes from the channel and its row from the
 * adjacent registered argument; the function rewrites A1 with the row base. */
static int irq_channel_cond_release_pc(uint32_t pc) {
    return !(pc & 3u) && pc >= 0x80033324u && pc < 0x80033398u;
}

static uint32_t irq_channel_cond_release_row(const FormatterCpu *cpu, uint32_t channel) {
    uint32_t callback, row;
    if (channel != irq_channel_record(cpu) || !irq_channel_update_frame(cpu, channel)) return 0;
    MusashiBootMemory *ram = cpu->irq_image->memory;
    const uint8_t *field = musashi_boot_ram_span(ram, channel+0x51u, 1);
    if (!field || *field != cpu->r[21] || cpu->r[4] != *field ||
        !musashi_boot_read32(ram, channel+0x40u, &callback) || callback != 0x80033324u ||
        !musashi_boot_read32(ram, channel+0x44u, &row) || row >= 8u) return 0;
    return 0x800a46e8u + row*0x54u;
}

/* Several channels can share a type-1 sound. A release removes only its
 * own field. The final field takes the existing path that clears the row. */
static int irq_channel_remaining_fields(const uint8_t *row) {
    unsigned count = 0;
    for (unsigned field = 0; field < 8u; ++field) {
        uint8_t active = row[14u + field];
        if (active > 1u) return -1;
        count += active;
    }
    return (int)count;
}
static int irq_channel_cond_release_ready(const FormatterCpu *cpu, uint32_t slot) {
    const uint8_t *row = musashi_boot_ram_span(cpu->irq_image->memory, slot, 0x54u);
    return row && (row[0] & 63u) == 1u && row[10] != 5u &&
        !row[13] && row[14u+cpu->r[4]] == 1u &&
        (row[12] == 1u || (row[12] > 1u && irq_channel_remaining_fields(row) == row[12]));
}

/* A mode-3 key-off clears the channel before invoking its registered
 * callback. The timeout path calls the same callback from a different site. */
static int irq_channel_keyoff_release(const FormatterCpu *cpu, uint32_t channel) {
    MusashiBootMemory *ram = cpu->irq_image->memory;
    uint16_t voice;
    const uint8_t *state = musashi_boot_ram_span(ram, channel+0x4eu, 3);
    if (!state || state[0] || state[2] ||
        !musashi_boot_read16(ram, channel+10u, &voice) || voice >= 24u) return 0;
    const uint8_t *mode = musashi_boot_ram_span(ram, 0x800a4efeu+voice, 1);
    return mode && *mode == 3u;
}

static int irq_channel_cond_release_link(const FormatterCpu *cpu, uint32_t channel) {
    return cpu->r[31] == 0x80031440u ||
        (cpu->r[31] == 0x800310d8u && irq_channel_keyoff_release(cpu, channel));
}

static int irq_channel_cond_release_call(const FormatterCpu *cpu, uint32_t word,
                                         uint32_t target, uint32_t channel) {
    uint32_t slot = irq_channel_cond_release_row(cpu, channel);
    if (!slot) return 0;
    int timed_out = cpu->pc == 0x80031438u && cpu->r[31] == 0x80031420u;
    int keyed_off = cpu->pc == 0x800310d0u && cpu->r[31] == 0x800310b8u &&
        irq_channel_keyoff_release(cpu, channel);
    return (timed_out || keyed_off) && target == 0x80033324u &&
        cpu->r[2] == target && word >> 26 == 0u &&
        (word & 63u) == 9u && ((word >> 21) & 31u) == 2u && ((word >> 11) & 31u) == 31u &&
        irq_channel_cond_release_ready(cpu, slot);
}

static int irq_channel_cond_release_entry(const FormatterCpu *cpu, uint32_t channel) {
    uint32_t slot = irq_channel_cond_release_row(cpu, channel);
    return slot && cpu->pc == 0x80033324u && irq_channel_cond_release_link(cpu, channel) &&
        cpu->r[5] == (slot-0x800a46e8u)/0x54u && irq_channel_cond_release_ready(cpu, slot);
}

static int irq_channel_cond_release_data(const FormatterCpu *cpu, uint32_t word,
                                        uint32_t address, uint32_t channel) {
    uint32_t slot = irq_channel_cond_release_row(cpu, channel);
    if (!slot || !irq_channel_cond_release_link(cpu, channel) || cpu->r[5] != slot) return 0;
    const uint8_t *row = musashi_boot_ram_span(cpu->irq_image->memory, slot, 0x54u);
    if (!row || (row[0] & 63u) != 1u) return 0;
    uint32_t field = cpu->r[4], op = word >> 26;
    uint32_t value = cpu->r[(word >> 16) & 31u];
    switch (cpu->pc) {
    case 0x80033344u:
        return op == 37u && address == slot && irq_channel_cond_release_ready(cpu, slot);
    case 0x80033358u:
        return op == 40u && !value && address == slot+14u+field &&
            cpu->r[2] == slot+field && irq_channel_cond_release_ready(cpu, slot);
    case 0x8003335cu:
    case 0x80033368u:
        if (address != slot+12u || row[14u+field] || row[13] || !row[12] ||
            (row[12] > 1u && irq_channel_remaining_fields(row) != row[12] - 1)) return 0;
        if (cpu->pc == 0x8003335cu) return op == 37u;
        return op == 41u && value == (uint32_t)row[12] - 1u;
    case 0x80033378u:
        return op == 36u && address == slot+10u && !row[12] && !row[13] && row[10] != 5u;
    case 0x80033388u:
        return op == 40u && !value && address == slot+10u && !row[12] && !row[13] &&
            cpu->r[3] == row[10] && row[10] != 5u;
    case 0x8003338cu:
        return op == 41u && !value && address == slot && !row[10] && !row[12] &&
            !row[13] && !row[14u+field];
    default:
        return 0;
    }
}

static int irq_channel_cond_release_return(const FormatterCpu *cpu, uint32_t target,
                                          uint32_t channel) {
    uint32_t slot = irq_channel_cond_release_row(cpu, channel);
    if (!slot || cpu->pc != 0x80033390u || !irq_channel_cond_release_link(cpu, channel) ||
        cpu->r[31] != target ||
        cpu->r[2] != 5u || cpu->r[5] != slot) return 0;
    const uint8_t *row = musashi_boot_ram_span(cpu->irq_image->memory, slot, 0x54u);
    if (!row || row[13] || row[14u+cpu->r[4]]) return 0;
    if (row[12])
        return (row[0] & 63u) == 1u && row[10] != 5u && irq_channel_remaining_fields(row) == row[12];
    return !row[0] && !row[1] && !row[10];
}

/* The channel timer reaches zero before this call. The observed voice
 * releases either have no command or a resident kind-2 command whose switch
 * arm immediately returns. Neither path writes RAM or touches a device. */
static int irq_channel_expiry_pc(uint32_t pc) {
    return !(pc & 3u) && ((pc >= 0x80031988u && pc <= 0x800319c0u) ||
                           pc == 0x80031a90u || pc == 0x80031a94u);
}

static int irq_channel_expiry_command(const FormatterCpu *cpu, uint32_t channel,
                                      uint32_t *command) {
    uint16_t remaining, kind;
    uint32_t destination;
    if (!channel || channel != irq_channel_record(cpu) ||
        !irq_channel_update_frame(cpu, channel)) return 0;
    MusashiBootMemory *memory = cpu->irq_image->memory;
    if (!musashi_boot_read16(memory, channel + 0x12u, &remaining) || remaining ||
        !musashi_boot_read32(memory, channel + 0x14u, command)) return 0;
    return !*command ||
           (*command == 0x800682d8u && musashi_boot_read16(memory, *command, &kind) &&
            kind == 2u && musashi_boot_read32(memory, 0x800730fcu, &destination) &&
            destination == 0x80031a90u);
}

static int irq_channel_expiry_call(const FormatterCpu *cpu, uint32_t word,
                                   uint32_t target, uint32_t channel) {
    uint32_t command;
    return cpu->pc == 0x800313f8u && word >> 26 == 3u && target == 0x80031988u &&
           irq_channel_expiry_command(cpu, channel, &command);
}

static int irq_channel_expiry_entry(const FormatterCpu *cpu, uint32_t channel) {
    uint32_t command;
    return cpu->pc == 0x80031988u && cpu->r[31] == 0x80031400u &&
           cpu->r[4] == channel && irq_channel_expiry_command(cpu, channel, &command);
}

static int irq_channel_expiry_data(const FormatterCpu *cpu, uint32_t word,
                                   uint32_t address, uint32_t channel) {
    uint32_t command;
    if (cpu->r[31] != 0x80031400u || cpu->r[4] != channel ||
        !irq_channel_expiry_command(cpu, channel, &command)) return 0;
    uint32_t op = word >> 26;
    switch (cpu->pc) {
    case 0x80031988u: return op == 35u && address == channel + 0x14u;
    case 0x80031998u: return op == 37u && command && cpu->r[5] == command && address == command;
    case 0x800319b4u: return op == 35u && command && cpu->r[5] == command &&
                              cpu->r[3] == 2u && address == 0x800730fcu;
    default: return 0;
    }
}

static int irq_channel_expiry_switch(const FormatterCpu *cpu, uint32_t word,
                                     uint32_t target, uint32_t channel) {
    uint32_t command;
    return cpu->pc == 0x800319bcu && word >> 26 == 0u && (word & 63u) == 8u &&
           ((word >> 21) & 31u) == 2u && target == 0x80031a90u && cpu->r[2] == target &&
           cpu->r[31] == 0x80031400u && cpu->r[4] == channel && cpu->r[3] == 2u &&
           irq_channel_expiry_command(cpu, channel, &command) && command && cpu->r[5] == command;
}

static int irq_channel_expiry_return(const FormatterCpu *cpu, uint32_t target,
                                     uint32_t channel) {
    uint32_t command;
    return cpu->pc == 0x80031a90u && target == 0x80031400u && cpu->r[31] == target &&
           cpu->r[4] == channel && irq_channel_expiry_command(cpu, channel, &command) &&
           cpu->r[5] == command && (!command || (cpu->r[2] == 0x80031a90u && cpu->r[3] == 2u));
}

/* Validate the title mixer's arguments at the mounted API boundary. The
 * key/reverb calls consume bits from the software voice mask. The attribute
 * call consumes the active record whose release flag the JAL delay clears. */
static int irq_title_mix_entry(const FormatterCpu *cpu, MusashiBootMemory *memory,
                               uint32_t call, uint32_t target, int after_delay) {
    if (cpu->r[29] != 0x8006ca54u) return 0;
    if (after_delay && (cpu->pc != target || cpu->r[31] != call + 8u)) return 0;
    if (call == 0x8002d41cu) {
        uint32_t index = cpu->r[18], voice;
        if (index >= 8u || target != 0x8003b250u) return 0;
        uint32_t record = 0x800a4c28u + 0x48u * index;
        const uint8_t *release = musashi_boot_ram_span(memory, record + 0x44u, 1u);
        return cpu->r[17] == record && cpu->r[16] == record + 4u &&
               cpu->r[5] == record && release && (after_delay ? !*release : *release != 0) &&
               musashi_boot_read32(memory, record + 0x40u, &voice) &&
               voice == 16u + index && (!after_delay || cpu->r[4] == voice);
    }
    static const struct { uint32_t call, function, mask, on; } calls[] = {
        {0x8002d3b0u, 0x8003c23cu, 0x800a2b98u, 0u},
        {0x8002d3e0u, 0x8003be74u, 0x800a2ba0u, 0u},
        {0x8002d454u, 0x8003c23cu, 0x800c7d20u, 1u},
        {0x8002d484u, 0x8003be74u, 0x800c7d2cu, 1u}
    };
    for (size_t i = 0; i < sizeof calls / sizeof calls[0]; ++i) {
        if (calls[i].call != call) continue;
        uint32_t bits;
        return target == calls[i].function && cpu->r[5] &&
               musashi_boot_read32(memory, calls[i].mask, &bits) &&
               cpu->r[5] == (bits & 0x00ff0000u) &&
               (!after_delay || cpu->r[4] == calls[i].on);
    }
    return 0;
}

/* The title mixer invokes the same sound services as the foreground guest.
 * Dispatch only the physical direct callers exercised by its VBlank updates. */
static int irq_title_mix_destination(uint32_t caller, uint32_t callee) {
    switch (caller) {
    case 0x8002d3b0u: return callee == 0x8003c23cu;
    case 0x8002d3e0u: return callee == 0x8003be74u;
    case 0x8002d41cu: return callee == 0x8003b250u;
    case 0x8002d454u: return callee == 0x8003c23cu;
    case 0x8002d484u: return callee == 0x8003be74u;
    default: return 0;
    }
}
