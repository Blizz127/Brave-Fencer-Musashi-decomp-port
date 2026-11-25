/* A closed, verified read of the selected file owns the restored guest
 * bytes until the game's foreground callback has applied the save. */
static int main12_restore_transaction(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                  const MusashiMain12Owner *owner)
{
    if (!owner || !owner->restore.closed ||
        (!owner->restore.request_cleared && !main12_confirmed_file(ram, owner)) ||
        !cpu->cd_device || owner->restore.files != cpu->cd_device ||
        owner->menu_metadata.file_userdata != cpu->cd_device->userdata ||
        owner->restore.events != cpu->cd_device->metadata_events ||
        owner->restore.open_id == owner->menu_metadata.open_id) return 0;
    const MusashiGuestIrqCard card = {ram, cpu->cd_device, cpu->event_device, owner->restore.events};
    const uint8_t *live = musashi_boot_ram_span(ram, 0x80075cc0u, sizeof owner->restore.bytes);
    return owner->display.confirm_phase == 2u && main12_event_leases(&card, owner->restore.allocation) &&
           live && !memcmp(live, owner->restore.bytes, sizeof owner->restore.bytes);
}

static int main12_restore_matches(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                  const MusashiMain12Owner *owner)
{
    return owner == cpu->main12_callback_frame && main12_restore_transaction(ram, cpu, owner);
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

#include "main12_restore_transfer.h"

/* Only this foreground call needs the original per-word writes. The usual
 * compiled struct copy remains available to independent callers. */
enum { MAIN12_PAYLOAD_CALL = 0x8017cfa8u, MAIN12_PAYLOAD_COPY = 0x8002b00cu,
       MAIN12_PAYLOAD_COPY_BYTES = 0x58u };

static int main12_payload_copy_pc(const FormatterCpu *cpu)
{
    return cpu->main12_payload_pc || (cpu->main12_callback_frame &&
        (cpu->pc == MAIN12_PAYLOAD_CALL || cpu->pc == MAIN12_PAYLOAD_CALL + 4u ||
         (cpu->r[31] == MAIN12_PAYLOAD_CALL + 8u &&
          cpu->pc - MAIN12_PAYLOAD_COPY < MAIN12_PAYLOAD_COPY_BYTES)));
}


/* The loaded message accepts Cross after the first five updates. With no
 * input, the same game path dismisses it when its original timer expires. */
#include "main12_loaded_exit.inc"

/* Once the loaded message closes, the foreground dispatcher invokes the
 * selected menu routine. Only the real dispatch delay can arm this frame. */
enum { MAIN12_MENU_EXIT = 0x8017bf70u, MAIN12_MENU_RESULT_GET = 0x8017bff8u,
       MAIN12_MENU_EXIT_END = 0x8017c008u, MAIN12_MENU_EXIT_RETURN = 0x8017bf10u,
       MAIN12_MENU_STATUS = 0x801853f2u };

static int main12_menu_exit_owned(MusashiBootMemory *ram, const FormatterCpu *cpu,
                                   MusashiMain12Owner *owner)
{
    uint16_t stage, timer, state, result;
    uint32_t target;
    const uint8_t *active = musashi_boot_ram_span(ram, 0x800b9a15u, 1u);
    return owner && !cpu->main12_callback_frame && owner->restore.exit_written &&
           owner->restore.dismissed && owner->restore.request_cleared &&
           owner->restore.consumed && owner->restore.copied == MAIN12_RESTORE_SIZE &&
           active && !*active && main12_restore_transaction(ram, cpu, owner) &&
           musashi_boot_read16(ram, 0x80115112u, &stage) && (stage == 4u || stage == 5u) &&
           musashi_boot_read16(ram, 0x80115118u, &timer) && !timer &&
           musashi_boot_read16(ram, MAIN12_FILE_RESULT, &result) && result == 0xffffu &&
           musashi_boot_read16(ram, 0x801853f0u, &state) && state == 1u &&
           musashi_boot_read32(ram, 0x80181d6cu + 4u * state, &target) && target == MAIN12_MENU_EXIT &&
           main12_current(owner, ram);
}

static int main12_menu_exit_delay(const FormatterCpu *cpu)
{
    return cpu->pc == MAIN12_MENU_EXIT_RETURN - 4u && cpu->delay_slot &&
           cpu->branch_pc == MAIN12_MENU_EXIT_RETURN - 8u && cpu->npc == MAIN12_MENU_EXIT &&
           cpu->r[2] == MAIN12_MENU_EXIT && cpu->r[31] == MAIN12_MENU_EXIT_RETURN;
}

#include "main12_room_setup.h"

static int main12_menu_exit_pc(const FormatterCpu *cpu)
{
    return cpu->main12_foreground_owner &&
           ((cpu->pc >= MAIN12_MENU_EXIT && cpu->pc < MAIN12_MENU_EXIT_END) ||
            (cpu->main12_room_next && main12_room_address(cpu->pc)));
}

#include "main12_menu_exit.inc"
