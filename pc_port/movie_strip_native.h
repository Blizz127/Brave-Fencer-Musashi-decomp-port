/* The game callback between the MDEC transfer and GPU upload on the
 * publisher movie. Both hardware service calls keep their mounted guest
 * implementations. The IRQ dispatcher has already authenticated the movie,
 * the registered callback and both devices before calling this entry. */
#include "movie_strip_clock.h"

static int movie_call_service(FormatterCpu *cpu, MusashiBootMemory *memory,
    const MusashiGuestIrqDraw *draw, const MovieOutput *slice,
    MusashiGuestIrqStop *stop, uint32_t service, uint32_t continuation) {
    cpu->r[31] = continuation;
    cpu->pc = service;
    cpu->npc = service + 4u;
    if (!movie_output_execute(cpu, memory, draw, slice, stop)) return 0;
    return cpu->pc == continuation && cpu->npc == continuation + 4u &&
        !cpu->delay_slot && !cpu->merge_pending && !cpu->gte_load_pending;
}

static int movie_strip_native(FormatterCpu *cpu, MusashiBootMemory *memory,
    const MusashiGuestIrqDraw *draw, const MovieOutput *slice, MusashiGuestIrqStop *stop) {
    const uint32_t buffers = 0x800ec9b0u, destinations = 0x800ec9bcu;
    const uint32_t index_address = 0x800ec9ccu, rectangle = 0x800ec9d0u;
    const uint32_t ready = 0x800ec9d8u, trim_first = 0x800ec9f0u;
    const uint32_t caller_sp = cpu->r[29], caller_ra = cpu->r[31];
    const uint32_t frame = caller_sp - 40u;
    uint32_t cd_mode, buffer, index, first, dest_xy, dest_wh;
    if (cpu->pc != 0x800d24d0u || cpu->npc != cpu->pc + 4u ||
        cpu->delay_slot || cpu->merge_pending || cpu->gte_load_pending ||
        (caller_sp & 3u) || caller_sp < 0x8006c9acu + 40u || caller_sp > 0x8006cabcu ||
        !musashi_boot_read32(memory, 0x800ec9dcu, &cd_mode) ||
        !musashi_boot_read32(memory, buffers + 8u, &buffer) || buffer > 1 ||
        !musashi_boot_read32(memory, index_address, &index) || index > 1 ||
        !musashi_boot_read32(memory, trim_first, &first) ||
        !musashi_boot_read32(memory, destinations + index * 8u, &dest_xy) ||
        !musashi_boot_read32(memory, destinations + index * 8u + 4u, &dest_wh))
        return 0;

    /* In the current public movie IRQ, a nonzero CD mode reaches a read of
     * pending CD work outside the admitted callback data contract. */
    if (cd_mode) return 0;
    const int32_t width = (int16_t)slice->wh;
    const int32_t height = (int16_t)(slice->wh >> 16);
    if (width <= 0 || height <= 0) return 0;
    const int32_t dest_width = (int16_t)dest_wh;
    const int32_t remainder = first ? dest_width % width : 0;
    /* The mounted callback may clear the first-strip flag on a shortened
     * first advance. The guard only admits this flag's end-of-frame store. */
    if (remainder) return 0;
    const uint16_t next_x = (uint16_t)((uint16_t)slice->xy + width);
    const int last = (int16_t)next_x >= (int16_t)dest_xy + dest_width;
    uint32_t next_dest = 0;
    if (last && !musashi_boot_read32(memory, destinations + (index ^ 1u) * 8u, &next_dest))
        return 0;

    if (!movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_ENTER) ||
        !musashi_boot_write32(memory, frame + 36u, caller_ra) ||
        !musashi_boot_write32(memory, frame + 32u, cpu->r[16]) ||
        !movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_SNAPSHOT) ||
        !musashi_boot_write32(memory, frame + 16u, slice->xy) ||
        !musashi_boot_write32(memory, frame + 20u, slice->wh) ||
        !movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_FLIP_BUFFER) ||
        !musashi_boot_write32(memory, buffers + 8u, buffer ^ 1u))
        return 0;
    cpu->r[29] = frame;
    cpu->r[16] = buffer;
    cpu->merge_reg = 3;
    cpu->merge_value = slice->wh;
    cpu->merge_next_pc = 0x800d2524u;
    if (first) {
        if (!movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_DIVIDE) ||
            !movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_CHECK_DIVISOR) ||
            !movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_CHECK_REMAINDER))
            return 0;
        cpu->lo = (uint32_t)(dest_width / width);
        cpu->hi = (uint32_t)remainder;
    }
    if (!movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_ADVANCE_WIDTH) ||
        !musashi_boot_write16(memory, rectangle, next_x) ||
        !movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_CHECK_PAGE))
        return 0;
    cpu->r[7] = rectangle;
    if (!last) {
        const uint32_t words_times_two = (uint32_t)(((height - 1) / 16 + 1) * (width * 16));
        cpu->lo = words_times_two;
        cpu->hi = 0;
        cpu->r[1] = 0x800f0000u + index * 8u;
        cpu->r[2] = (uint32_t)((height - 1) / 16 + 1);
        cpu->r[3] = (uint32_t)(width * 16);
        cpu->r[4] = slice->next;
        cpu->r[5] = words_times_two / 2u;
        cpu->r[6] = (uint32_t)(height - 1);
        cpu->r[8] = words_times_two;
        if (!movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_PREPARE_REARM) ||
            !movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_CALL_REARM) ||
            !movie_call_service(cpu, memory, draw, slice, stop, 0x800d2c88u, 0x800d2688u) ||
            !movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_RETURN_REARM) || cpu->r[16] != buffer)
            return 0;
    } else {
        if (!movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_FINISH_PAGE) ||
            !musashi_boot_write32(memory, index_address, index ^ 1u) ||
            !musashi_boot_write32(memory, ready, 1u) ||
            !musashi_boot_write32(memory, rectangle, next_dest) ||
            !musashi_boot_write32(memory, trim_first, 1u))
            return 0;
        cpu->r[3] = next_dest & 0xffffu;
    }
    cpu->r[1] = 0x800f0000u + buffer * 4u;
    cpu->r[2] = buffer * 4u;
    cpu->r[4] = frame + 16u;
    cpu->r[5] = slice->current;
    if (!movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_CALL_UPLOAD) ||
        !movie_call_service(cpu, memory, draw, slice, stop, 0x800599b8u, 0x800d26f0u) ||
        !movie_strip_charge(cpu->clock, stop, MOVIE_STRIP_RETURN_UPLOAD) ||
        !musashi_boot_read32(memory, frame + 36u, &cpu->r[31]) ||
        !musashi_boot_read32(memory, frame + 32u, &cpu->r[16]))
        return 0;
    cpu->r[29] = caller_sp;
    if (!formatter_return(cpu)) return 0;
    if (getenv("MUSASHI_TRACE_MOVIE_STRIPS"))
        fprintf(stderr, "native_boot: MOVIE_STRIP_NATIVE completed pc=800d24d0 x=%u y=%u w=%d h=%d source=%08x next=%08x last=%d ra=%08x\n",
            (unsigned)(uint16_t)slice->xy, (unsigned)(slice->xy >> 16), (int)width, (int)height,
            slice->current, slice->next, last, cpu->pc);
    return 1;
}
