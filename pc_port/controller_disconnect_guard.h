/* Identify the poll's mounted child. The formatter issues its actual words
 * and charges its device clock, one guest instruction at a time. */
enum {
    PAD_DISCONNECT = 0x80018f88u, PAD_DISCONNECT_LENGTH = 64u,
    PAD_DISCONNECT_CALL = 0x80018b04u, PAD_REPORT_OFFSET = 42u,
    PAD_REPORT_LENGTH = 6u
};

static int pad_disconnect_image(MusashiBootMemory *memory, const FormatterCpu *cpu,
                                const uint8_t *image, uint32_t pc, size_t size)
{
    const uint8_t *live = musashi_boot_ram_span(memory, pc, size);
    return live && intrinsic_span(cpu, pc, size) &&
        !memcmp(live, image + CODE_IMAGE_TEXT + (pc - CODE_IMAGE_BASE), size);
}

static int pad_disconnect_bound(LaneRunFrame *nested, MusashiBootMemory *memory,
                                const FormatterCpu *cpu)
{
    if (!frame_pad_child_bound(nested, memory, cpu) || !g_installed_frame_poll ||
        cpu->r[31] != PAD_DISCONNECT_CALL + 8u ||
        (cpu->r[4] != PAD_FIRST && cpu->r[4] != PAD_FIRST + PAD_RECORD_SIZE) ||
        lane_find(cpu->pc) || lane_ovl_first(cpu->pc) != g_lane_ovl_count)
        return 0;
    const uint8_t *image = code_image();
    return image && g_code_image_state == 1 &&
        pad_disconnect_image(memory, cpu, image, PAD_DISCONNECT_CALL, 8u) &&
        pad_disconnect_image(memory, cpu, image, PAD_DISCONNECT, PAD_DISCONNECT_LENGTH) &&
        intrinsic_span(cpu, cpu->r[4], PAD_RECORD_SIZE) &&
        musashi_boot_ram_span(memory, cpu->r[4], PAD_RECORD_SIZE);
}

/* Compute the destination from the fetched word and current bus operands.
 * A device checkpoint may update them before the actual byte transfer. */
static int pad_disconnect_data(const FormatterCpu *cpu, uint32_t word)
{
    unsigned op = word >> 26;
    if (op < 32u) return 1;
    uint32_t address = cpu->r[(word >> 21) & 31u] + (uint32_t)(int32_t)(int16_t)word;
    return op == 40u && address - (cpu->r[4] + PAD_REPORT_OFFSET) < PAD_REPORT_LENGTH;
}
