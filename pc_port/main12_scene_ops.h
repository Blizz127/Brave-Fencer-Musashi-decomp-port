/* Per-instruction host effects for the MAIN/0012 scene setup and its child.
 * The caller owns the mounted-image, clock, control-flow and access checks. */
static int scene_overlay_load_half(MusashiBootMemory *ram, FormatterCpu *cpu, uint32_t address)
{
    uint16_t value;
    if (!cpu_read16(ram, cpu, address, &value)) return 0;
    cpu->r[2] = value;
    return 1;
}

static int scene_overlay_load_stack(MusashiBootMemory *ram, FormatterCpu *cpu,
                                    unsigned reg, unsigned offset)
{
    uint32_t value;
    if (!cpu_read32(ram, cpu, cpu->r[29] + offset, &value)) return 0;
    cpu->r[reg] = value;
    return 1;
}

static int scene_overlay_op(MusashiBootMemory *ram, FormatterCpu *cpu,
                            uint32_t *after, int *branch, uint32_t *call)
{
    uint32_t *r = cpu->r;
    switch (cpu->pc) {
    /* The outer routine saves RA and S0–S3, then clears the scene workspace. */
    case 0x801378f0u: r[29] -= 40u; break;
    case 0x801378f4u: r[4] = 0x80120000u; break;
    case 0x801378f8u: r[4] += 0x7520u; break;
    case 0x801378fcu: r[5] = 0xbfcu; break;
    case 0x80137900u: return cpu_write32(ram, cpu, r[29] + 32u, r[31]);
    case 0x80137904u: return cpu_write32(ram, cpu, r[29] + 28u, r[19]);
    case 0x80137908u: return cpu_write32(ram, cpu, r[29] + 24u, r[18]);
    case 0x8013790cu:
    case 0x8013a864u: return cpu_write32(ram, cpu, r[29] + 20u, r[17]);
    case 0x80137914u:
    case 0x8013a86cu: return cpu_write32(ram, cpu, r[29] + 16u, r[16]);

    case 0x80137910u:   /* Clear the workspace. */
    case 0x80137930u:   /* Clear the current scene record. */
    case 0x801379b0u:   /* Initialize the two child entries. */
    case 0x8013a880u:   /* Clear the current child entry. */
        r[31] = cpu->pc + 8u;
        *call = cpu->pc == 0x801379b0u ? SCENE_CHILD : SCENE_CLEAR;
        *after = *call;
        *branch = 1;
        break;

    /* Three 76-byte records receive four halfwords from eight-byte rows. */
    case 0x80137918u: r[19] = 0; break;
    case 0x8013791cu:
    case 0x8013a868u: r[17] = 0; break;
    case 0x80137920u: r[18] = 0x80120000u; break;
    case 0x80137924u: r[18] += 0x69f0u; break;
    case 0x80137928u: r[16] = 0; break;
    case 0x8013792cu: r[4] = r[18]; break;
    case 0x80137934u: r[5] = 76u; break;

    case 0x80137938u:
    case 0x80137950u:
    case 0x8013796cu:
    case 0x80137988u: r[1] = 0x80180000u; break;
    case 0x8013793cu:
    case 0x80137954u:
    case 0x80137970u:
    case 0x8013798cu: r[1] += r[17]; break;
    case 0x80137940u: return scene_overlay_load_half(ram, cpu, r[1] - 0x16ccu);
    case 0x80137958u: return scene_overlay_load_half(ram, cpu, r[1] - 0x16cau);
    case 0x80137974u: return scene_overlay_load_half(ram, cpu, r[1] - 0x16c8u);
    case 0x80137990u: return scene_overlay_load_half(ram, cpu, r[1] - 0x16c6u);

    case 0x80137944u:
    case 0x80137960u:
    case 0x8013797cu:
    case 0x80137998u: r[1] = 0x80120000u; break;
    case 0x80137948u:
    case 0x80137964u:
    case 0x80137980u:
    case 0x8013799cu: r[1] += r[16]; break;
    case 0x8013794cu: return cpu_write16(ram, cpu, r[1] + 0x6a28u, (uint16_t)r[2]);
    case 0x80137968u: return cpu_write16(ram, cpu, r[1] + 0x6a2au, (uint16_t)r[2]);
    case 0x80137984u: return cpu_write16(ram, cpu, r[1] + 0x6a2cu, (uint16_t)r[2]);
    case 0x801379a0u: return cpu_write16(ram, cpu, r[1] + 0x6a2eu, (uint16_t)r[2]);
    case 0x8013795cu: r[18] += 76u; break;
    case 0x80137978u: ++r[19]; break;
    case 0x80137994u: r[17] += 8u; break;
    case 0x801379a4u: r[2] = (int32_t)r[19] < 3; break;
    case 0x801379a8u:
        if (r[2]) *after = 0x8013792cu;
        *branch = 1;
        break;
    case 0x801379acu: r[16] += 76u; break;

    case 0x801379b8u: return scene_overlay_load_stack(ram, cpu, 31, 32);
    case 0x801379bcu: return scene_overlay_load_stack(ram, cpu, 19, 28);
    case 0x801379c0u: return scene_overlay_load_stack(ram, cpu, 18, 24);
    case 0x801379c4u:
    case 0x8013a89cu: return scene_overlay_load_stack(ram, cpu, 17, 20);
    case 0x801379c8u:
    case 0x8013a8a0u: return scene_overlay_load_stack(ram, cpu, 16, 16);
    case 0x801379ccu: r[29] += 40u; break;

    /* The child uses S0 as an advancing pointer and S1 as its loop index. */
    case 0x8013a860u: r[29] -= 32u; break;
    case 0x8013a870u: r[16] = 0x80120000u; break;
    case 0x8013a874u: r[16] -= 0x2580u; break;
    case 0x8013a878u: return cpu_write32(ram, cpu, r[29] + 24u, r[31]);
    case 0x8013a87cu: r[4] = r[16]; break;
    case 0x8013a884u: r[5] = 16u; break;
    case 0x8013a888u: ++r[17]; break;
    case 0x8013a88cu: r[2] = (int32_t)r[17] < 2; break;
    case 0x8013a890u:
        if (r[2]) *after = 0x8013a87cu;
        *branch = 1;
        break;
    case 0x8013a894u: r[16] += 16u; break;
    case 0x8013a898u: return scene_overlay_load_stack(ram, cpu, 31, 24);
    case 0x8013a8a4u: r[29] += 32u; break;

    case 0x801379d0u:
    case 0x8013a8a8u:
        *after = r[31];
        *branch = 1;
        break;
    case 0x801379b4u:
    case 0x801379d4u:
    case 0x8013a8acu:
        break;
    default:
        return 0;
    }
    return 1;
}
