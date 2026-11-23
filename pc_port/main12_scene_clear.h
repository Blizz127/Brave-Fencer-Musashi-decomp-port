/* The six scene clears use aligned addresses and lengths divisible by four. */
static int scene_clear_op(MusashiBootMemory *ram, FormatterCpu *cpu,
                          uint32_t *after, int *branch, uint32_t *call)
{
    uint32_t *r = cpu->r;
    (void)call; /* The resident leaf makes no calls. */
    switch (cpu->pc) {
    case 0x80016714u: r[2] = (int32_t)r[5] < 4; break;
    case 0x80016718u:
        if (!r[2]) *after = 0x80016744u;
        *branch = 1;
        break;
    case 0x8001671cu:
    case 0x80016770u:
    case 0x80016794u: r[2] = UINT32_MAX; break;
    case 0x80016744u: r[2] = r[4] & 3u; break;
    case 0x80016748u:
        if (!r[2]) *after = 0x80016768u;
        *branch = 1;
        break;
    case 0x8001674cu: r[3] = r[4]; break;

    case 0x80016768u: r[3] = (uint32_t)((int32_t)r[5] >> 2); break;
    case 0x8001676cu:
    case 0x80016780u: --r[3]; break;
    case 0x80016774u:
        if (r[3] == r[2]) *after = 0x8001678cu;
        *branch = 1;
        break;
    case 0x8001677cu: return cpu_write32(ram, cpu, r[4], 0);
    case 0x80016784u:
        if (r[3] != r[2]) *after = 0x8001677cu;
        *branch = 1;
        break;
    case 0x80016788u: r[4] += 4u; break;

    case 0x8001678cu: r[5] &= 3u; break;
    case 0x80016790u: --r[5]; break;
    case 0x80016798u:
        if (r[5] == r[2]) *after = 0x800167b0u;
        *branch = 1;
        break;
    case 0x800167b0u:
        *after = r[31];
        *branch = 1;
        break;
    case 0x80016778u:
    case 0x8001679cu:
    case 0x800167b4u:
        break;
    default:
        return 0;
    }
    return 1;
}
