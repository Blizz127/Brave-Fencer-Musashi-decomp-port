/* The normal checkpoint runs before each game instruction. The caller begins
 * only after the original CRT call and its delay slot retire in this root. */
enum { CRT_GAME_CALL = 0x80010094u, GAME_MAIN_ENTRY = 0x80010178u,
       GAME_CTOR_CALL = 0x80010190u, GAME_CTOR = 0x800100a0u,
       GAME_CTOR_RETURN = 0x80010108u, GAME_CTOR_END = GAME_CTOR_RETURN+8u,
       GAME_MAIN_RESUME = GAME_CTOR_CALL+8u, GAME_EXCHANGE_CALL = 0x800101acu,
       GAME_EXCHANGE = 0x80043060u, POST_EXCHANGE = GAME_EXCHANGE_CALL+8u,
       GRAPHICS_CALL = 0x800101c0u, GRAPHICS_ENTRY = 0x800141f0u,
       GRAPHICS_LIBRARY_CALL = GRAPHICS_ENTRY+8u, GRAPHICS_LIBRARY = 0x80059234u };

static int game_main_owner(LaneRunFrame *frame, MusashiBootMemory *memory,
                           const FormatterCpu *cpu)
{
    GameMainEntry *entry = &frame->main_entry;
    return frame == g_lane_run_frames && !frame->prev && frame->entry_stop && frame->cpu == cpu &&
        entry->memory == memory && g_fetch_memory == memory && musashi_native_lane_usable(memory) &&
        cpu->clock == entry->clock && cpu->clock->advance == entry->charge.advance &&
        cpu->clock->userdata == entry->charge.userdata && cpu->callback_device == entry->callback &&
        cpu->callback_device->checkpoint == entry->checkpoint &&
        cpu->callback_device->userdata == entry->worker && !cpu->r[0] && !cpu->bios_frame &&
        !cpu->cd_irq_frame && !cpu->registration_stack && !cpu->dma_registration &&
        !cpu->irq_image && !cpu->cd_parent_image && !cpu->merge_pending && !cpu->gte_load_pending;
}

static void game_main_capture(LaneRunFrame *frame, MusashiBootMemory *memory, FormatterCpu *cpu)
{
    static const uint8_t expected_payload[32] = {
        0xbe,0x19,0x7b,0xd5,0x92,0xbe,0x13,0x24,0x00,0x94,0xbf,0x49,0xd0,0x66,0xe8,0xf3,
        0xac,0xd8,0x24,0x5f,0xbe,0xb9,0xec,0x81,0x0e,0x30,0x0a,0x37,0xea,0x0f,0x44,0xa4
    };
    GameMainEntry *entry = &frame->main_entry;
    const size_t size = CODE_IMAGE_END - CODE_IMAGE_BASE;
    const char *path = getenv("MUSASHI_CODE_IMAGE");
    const char *mutant = getenv("MUSASHI_CODE_IMAGE_MUTANT");
    const uint8_t *ram, *cache;
    uint8_t digest[32];
    if (!musashi_native_lane_usable(memory) || !cpu->clock || !cpu->clock->advance ||
        !cpu->callback_device || !cpu->callback_device->checkpoint ||
        cpu->npc != CODE_IMAGE_BASE+4u || cpu->r[31] || cpu->delay_slot || cpu->branch_pc ||
        (mutant && mutant[0] == '1') || !intrinsic_span(cpu,CRT_GAME_CALL,sizeof entry->call) ||
        !intrinsic_span(cpu,GAME_MAIN_ENTRY,sizeof entry->game)) return;
    ram = musashi_boot_ram_span(memory,CODE_IMAGE_BASE,size);
    cache = code_image();
    if (!ram || !musashi_sha256_digest(ram,size,digest) ||
        memcmp(digest,expected_payload,sizeof digest) || (path && *path && !cache) ||
        (cache && memcmp(cache+CODE_IMAGE_TEXT,ram,size))) return;
    /* A pre-existing compiled provider retains its entire call. */
    for (unsigned offset=0; offset<sizeof entry->game; offset+=4u)
        if (lane_find(GAME_MAIN_ENTRY+offset)) return;
    memcpy(entry->call,ram+(CRT_GAME_CALL-CODE_IMAGE_BASE),sizeof entry->call);
    memcpy(entry->game,ram+(GAME_MAIN_ENTRY-CODE_IMAGE_BASE),sizeof entry->game);
    entry->memory = memory;
    entry->clock = cpu->clock;
    entry->charge = *cpu->clock;
    entry->callback = cpu->callback_device;
    entry->checkpoint = cpu->callback_device->checkpoint;
    entry->worker = cpu->callback_device->userdata;
    entry->next = CRT_GAME_CALL;
    if (!intrinsic_span(cpu,GAME_CTOR,sizeof entry->constructor) ||
        !intrinsic_span(cpu,GAME_MAIN_RESUME,sizeof entry->resume)) return;
    for (unsigned off=0; off<sizeof entry->constructor; off+=4u)
        if (lane_find(GAME_CTOR+off)) return;
    for (unsigned off=0; off<sizeof entry->resume; off+=4u)
        if (lane_find(GAME_MAIN_RESUME+off)) return;
    memcpy(entry->constructor,ram+(GAME_CTOR-CODE_IMAGE_BASE),sizeof entry->constructor);
    memcpy(entry->resume,ram+(GAME_MAIN_RESUME-CODE_IMAGE_BASE),sizeof entry->resume);
    entry->follow_constructor = 1;
    if (!intrinsic_span(cpu,POST_EXCHANGE,sizeof entry->post_exchange) ||
        !intrinsic_span(cpu,GRAPHICS_ENTRY,sizeof entry->graphics)) return;
    for (unsigned off=0; off<sizeof entry->post_exchange; off+=4u)
        if (lane_find(POST_EXCHANGE+off)) return;
    for (unsigned off=0; off<sizeof entry->graphics; off+=4u)
        if (lane_find(GRAPHICS_ENTRY+off)) return;
    for (unsigned off=0; off<STARTUP_EXCHANGE_SIZE; off+=4u)
        if (lane_find(GAME_EXCHANGE+off)) return;
    memcpy(entry->post_exchange,ram+(POST_EXCHANGE-CODE_IMAGE_BASE),sizeof entry->post_exchange);
    memcpy(entry->graphics,ram+(GRAPHICS_ENTRY-CODE_IMAGE_BASE),sizeof entry->graphics);
    entry->follow_graphics = 1;
}

static void game_main_crt_retired(LaneRunFrame *frame, MusashiBootMemory *memory,
                                  const FormatterCpu *cpu, uint32_t issued)
{
    if (!frame || frame->main_entry.next != issued) return;
    GameMainEntry *entry = &frame->main_entry;
    const int call = issued == CRT_GAME_CALL;
    const uint8_t *ram = musashi_boot_ram_span(memory,CRT_GAME_CALL,sizeof entry->call);
    if (!game_main_owner(frame,memory,cpu) || !intrinsic_span(cpu,CRT_GAME_CALL,sizeof entry->call) ||
        !ram || memcmp(ram,entry->call,sizeof entry->call) || cpu->r[31] != CRT_GAME_CALL+8u ||
        cpu->pc != (call ? CRT_GAME_CALL+4u : GAME_MAIN_ENTRY) ||
        cpu->npc != (call ? GAME_MAIN_ENTRY : GAME_MAIN_ENTRY+4u) ||
        cpu->delay_slot != (unsigned)call || cpu->branch_pc != (call ? CRT_GAME_CALL : 0u))
        entry->next = 0;
    else entry->next = cpu->pc;
}

static int game_main_frame(LaneRunFrame *frame, MusashiBootMemory *memory,
                           const FormatterCpu *cpu, uint32_t pc)
{
    const int delay = pc == GAME_CTOR_CALL+4u;
    const uint32_t saved_sp = 0x801ffff8u, stack = 0x801fffc0u;
    return game_main_owner(frame,memory,cpu) && cpu->pc == pc && frame->main_entry.next == pc &&
        cpu->npc == (delay ? GAME_CTOR : pc+4u) && cpu->delay_slot == (unsigned)delay &&
        cpu->branch_pc == (delay ? GAME_CTOR_CALL : 0u) &&
        cpu->r[31] == (delay ? GAME_CTOR_CALL+8u : CRT_GAME_CALL+8u) &&
        cpu->r[29] == (pc == GAME_MAIN_ENTRY ? saved_sp : stack) &&
        cpu->r[30] == (pc < GAME_CTOR_CALL ? saved_sp : stack) &&
        intrinsic_span(cpu,GAME_MAIN_ENTRY,sizeof frame->main_entry.game) && intrinsic_span(cpu,stack,56u);
}

/* Only the ordinary instruction path can attest a constructor retirement.
 * Its code and branches come from the current user's mapped image. A call
 * outside this observed interval leaves the following game caller on the CPU. */
static void game_main_constructor_retired(LaneRunFrame *frame, MusashiBootMemory *memory,
                                          const FormatterCpu *cpu, uint32_t issued)
{
    if (!frame || !frame->main_entry.follow_constructor || !frame->main_entry.next) return;
    GameMainEntry *entry = &frame->main_entry;
    const uint8_t *live = musashi_boot_ram_span(memory,GAME_CTOR,sizeof entry->constructor);
    int owned = game_main_owner(frame,memory,cpu) && entry->next == issued &&
        !lane_find(issued) && intrinsic_span(cpu,GAME_CTOR,sizeof entry->constructor) &&
        live && !memcmp(live,entry->constructor,sizeof entry->constructor);
    if (issued == GAME_CTOR_RETURN)
        owned = owned && cpu->pc == GAME_CTOR_RETURN+4u && cpu->npc == GAME_MAIN_RESUME &&
            cpu->r[31] == GAME_MAIN_RESUME && cpu->delay_slot && cpu->branch_pc == issued;
    else if (issued == GAME_CTOR_RETURN+4u)
        owned = owned && cpu->pc == GAME_MAIN_RESUME && cpu->npc == GAME_MAIN_RESUME+4u &&
            !cpu->delay_slot && !cpu->branch_pc && cpu->r[31] == GAME_MAIN_RESUME &&
            cpu->r[29] == 0x801fffc0u && cpu->r[30] == cpu->r[29];
    else
        owned = owned && (uint32_t)(cpu->pc-GAME_CTOR) < sizeof entry->constructor &&
            (uint32_t)(cpu->npc-GAME_CTOR) < sizeof entry->constructor;
    entry->next = owned ? cpu->pc : 0u;
}

static int game_main_resumed_frame(LaneRunFrame *frame, MusashiBootMemory *memory,
                                   const FormatterCpu *cpu, uint32_t pc)
{
    const int delay = pc == GAME_EXCHANGE_CALL+4u;
    return game_main_owner(frame,memory,cpu) && frame->main_entry.next == pc && cpu->pc == pc &&
        cpu->npc == (delay ? GAME_EXCHANGE : pc+4u) && cpu->delay_slot == (unsigned)delay &&
        cpu->branch_pc == (delay ? GAME_EXCHANGE_CALL : 0u) &&
        cpu->r[31] == (delay ? GAME_EXCHANGE_CALL+8u : GAME_MAIN_RESUME) &&
        cpu->r[29] == 0x801fffc0u && cpu->r[30] == cpu->r[29];
}

static int game_main_resumed_issue(LaneRunFrame *frame, MusashiBootMemory *memory, FormatterCpu *cpu)
{
    GameMainEntry *entry = &frame->main_entry;
    const uint32_t pc = cpu->pc;
    const uint8_t *live = musashi_boot_ram_span(memory,GAME_MAIN_RESUME,sizeof entry->resume);
    if (!entry->follow_constructor || !game_main_resumed_frame(frame,memory,cpu,pc) ||
        !intrinsic_span(cpu,GAME_MAIN_RESUME,sizeof entry->resume) || !live ||
        memcmp(live,entry->resume,sizeof entry->resume) || lane_find(pc)) {
        entry->next = 0;
        return 0;
    }
    if (!entry->charge.advance(entry->charge.userdata,2u) ||
        !game_main_resumed_frame(frame,memory,cpu,pc) ||
        memcmp(live,entry->resume,sizeof entry->resume) || lane_find(pc)) {
        entry->next = 0;
        return -1;
    }
    switch ((pc-GAME_MAIN_RESUME)/4u) {
    case 0: cpu->r[16] = 0x800c0000u; break;
    case 1: cpu->r[16] -= 0x5ee8u; break;
    case 2: cpu->r[17] = 0x800b0000u; break;
    case 3: cpu->r[17] -= 0x09d0u; break;
    case 4: cpu->r[4] = 0; break;
    case 5:
        cpu->r[31] = pc+8u;
        if (g_trace_state >= 0) musashi_trace_function_entry(GAME_EXCHANGE,pc,'j');
        if (g_entry_hook) g_entry_pending = GAME_EXCHANGE;
        break;
    }
    cpu->pc = cpu->npc;
    cpu->npc = pc == GAME_EXCHANGE_CALL ? GAME_EXCHANGE : cpu->pc+4u;
    cpu->delay_slot = pc == GAME_EXCHANGE_CALL;
    cpu->branch_pc = cpu->delay_slot ? pc : 0u;
    entry->next = cpu->pc == GAME_EXCHANGE && !entry->follow_graphics ? 0u : cpu->pc;
    return 1;
}

static int game_main_issue(LaneRunFrame *frame, MusashiBootMemory *memory, FormatterCpu *cpu)
{
    GameMainEntry *entry = &frame->main_entry;
    const uint32_t pc = cpu->pc;
    if ((uint32_t)(pc-GAME_MAIN_RESUME) < sizeof entry->resume)
        return game_main_resumed_issue(frame,memory,cpu);
    const unsigned issue = (pc-GAME_MAIN_ENTRY)/4u;
    const uint8_t *ram = musashi_boot_ram_span(memory,GAME_MAIN_ENTRY,sizeof entry->game);
    if (issue >= 8u || !game_main_frame(frame,memory,cpu,pc) || !ram ||
        memcmp(ram,entry->game,sizeof entry->game) || lane_find(pc)) {
        entry->next = 0;
        return 0;
    }
    const int store = issue >= 1u && issue <= 4u;
    if (!entry->charge.advance(entry->charge.userdata,store ? 3u : 2u) ||
        !game_main_frame(frame,memory,cpu,pc)) {
        entry->next = 0;
        return -1;
    }
    if (store) {
        static const unsigned saved[] = {31,30,17,16};
        if (!musashi_boot_write32(memory,cpu->r[29]+56u-4u*issue,cpu->r[saved[issue-1u]])) {
            entry->next = 0;
            return -1;
        }
    } else if (issue == 0u) cpu->r[29] -= 56u;
    else if (issue == 5u) cpu->r[30] = cpu->r[29];
    else if (pc == GAME_CTOR_CALL) {
        cpu->r[31] = pc+8u;
        if (g_trace_state >= 0) musashi_trace_function_entry(GAME_CTOR,pc,'j');
        if (g_entry_hook) g_entry_pending = GAME_CTOR;
    }
    cpu->pc = cpu->npc;
    cpu->npc = pc == GAME_CTOR_CALL ? GAME_CTOR : cpu->pc+4u;
    cpu->delay_slot = pc == GAME_CTOR_CALL;
    cpu->branch_pc = cpu->delay_slot ? pc : 0u;
    entry->next = issue == 7u && !entry->follow_constructor ? 0u : cpu->pc;
    return 1;
}
