/* SPDX-License-Identifier: MIT */
/* Include after the real opening/formatter/CPU bus helpers and the host bus
 * adapter. The caller binds context to the active opening run and validates
 * that run's complete identity in the common scheduler on each clock side. */
#ifndef BFM_MIPS_MOUNTED_OPENING_SCALAR_INC
#define BFM_MIPS_MOUNTED_OPENING_SCALAR_INC

static inline bool
bfm_opening_scalar_resolve(void *context, MusashiBootMemory *memory, const FormatterCpu *cpu,
                           const struct bfm_host_bus_request *request,
                           struct bfm_host_bus_grant *grant)
{
    uint32_t fetched;
    uint32_t mounted;
    struct bfm_scalar_memory access;
    if (!memory || !cpu || !context || context != cpu->opening_pitch || !request || !grant ||
        !opening_pitch_owner(memory, cpu) || !opening_pitch_word(memory, cpu, &mounted) ||
        !formatter_fetch(memory, cpu, &fetched) || mounted != fetched ||
        !opening_pitch_issuer(memory, cpu, mounted) || !opening_pitch_access(memory, mounted, cpu) ||
        !bfm_decode_scalar_memory(mounted, cpu->r, &access) ||
        access.address != request->effective_address || access.address != request->bus_address ||
        access.width != request->width || access.store != request->writing ||
        request->byte_enables != (1u << access.width) - 1u ||
        (access.address & (access.width - 1u)) != 0)
        return false;

    /* The canonical view only covers the cached main-RAM array. Require
     * the actual CPU view for all requested bytes to agree with that view.
     * This probe never calls a read/write or grants the rest of the array. */
    uint8_t *canonical = musashi_boot_ram_span(memory, access.address, access.width);
    if (!canonical || canonical != cpu_ram_span(memory, cpu, access.address, access.width))
        return false;
    const uint32_t physical = (uint32_t)(canonical - memory->bytes);
    *grant = (struct bfm_host_bus_grant){
        .region = memory->bytes, .provider = context, .physical_address = physical,
        .first = physical, .last = physical + access.width - 1u
    };
    return true;
}
#endif
