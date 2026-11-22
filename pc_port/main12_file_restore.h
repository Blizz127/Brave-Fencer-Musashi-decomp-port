/* A closed, verified read of the selected file owns the restored guest
 * bytes until the game's foreground callback has applied the save. */
static int main12_restore_matches(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                  const MusashiMain12Owner *owner)
{
    if (!owner || owner != cpu->main12_callback_frame || !owner->restore.closed ||
        !cpu->cd_device || owner->restore.files != cpu->cd_device ||
        owner->menu_metadata.file_userdata != cpu->cd_device->userdata ||
        owner->restore.events != cpu->cd_device->metadata_events ||
        owner->restore.open_id == owner->menu_metadata.open_id) return 0;
    const MusashiGuestIrqCard card = {ram, cpu->cd_device, cpu->event_device, owner->restore.events};
    const uint8_t *live = musashi_boot_ram_span(ram, 0x80075cc0u, sizeof owner->restore.bytes);
    return owner->display.confirm_phase == 2u && main12_event_leases(&card, owner->restore.allocation) &&
           live && !memcmp(live, owner->restore.bytes, sizeof owner->restore.bytes);
}

static int main12_restore_pc(MusashiBootMemory *ram, const FormatterCpu *cpu,
                             const MusashiMain12Owner *owner)
{
    uint32_t pc = cpu->pc;
    if (!owner || !owner->restore.result) return 0;
    int selection = (pc >= 0x8017cf68u && pc < 0x8017d004u) ||
                    (pc >= 0x8017d040u && pc < 0x8017d064u);
    int apply = ((pc >= 0x8017df40u && pc < 0x8017e0bcu) || pc == 0x8017e224u || pc == 0x8017e228u) &&
                owner->restore.copied == MAIN12_RESTORE_SIZE && cpu->r[31] == 0x8017cfb8u;
    return (selection || apply) && main12_restore_matches(ram, cpu, owner);
}

static int main12_store_payload(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                MusashiMain12Owner *owner, uint32_t address, uint32_t value)
{
    uint32_t offset = address - MAIN12_RESTORE_BUFFER;
    if (!main12_restore_matches(ram, cpu, owner) || !owner->restore.result ||
        owner->restore.copied != offset || offset > MAIN12_RESTORE_SIZE - 4u ||
        cpu->r[31] != 0x8017cfb0u || !owner->resident_matches ||
        !owner->resident_matches(owner, 0x8002b00cu, 0x58u) || !main12_current(owner, ram))
        return 0;
    const uint8_t *bytes = owner->restore.bytes + offset;
    uint32_t expected = (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
                        (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
    uint32_t site = offset < 720u ? 0x8002b02cu + (offset & 15u) :
                    offset == 720u ? 0x8002b054u : offset == 724u ? 0x8002b058u : 0x8002b060u;
    return cpu->pc == site && value == expected;
}
