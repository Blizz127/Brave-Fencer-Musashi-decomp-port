/* SPDX-License-Identifier: MIT */
/* Include in the same translation unit as the actual cpu_read/write helpers. */
#ifndef BFM_MIPS_MOUNTED_HOST_BUS_INC
#define BFM_MIPS_MOUNTED_HOST_BUS_INC

#include "mips_mounted_scalar.h"

struct bfm_host_bus_request {
    uint32_t effective_address; /* Original effective byte address. */
    uint32_t bus_address;       /* Guest alias presented to the CPU bus. */
    unsigned width;
    bool writing;
    unsigned byte_enables;      /* Bit 0 selects bus_address. */
};

struct bfm_host_bus_grant {
    const void *region;
    const void *provider;
    uint64_t revision;
    uint32_t physical_address;
    uint32_t first;
    uint32_t last; /* Inclusive physical span. */
    void *masked_context;
    int (*write_masked)(void *, MusashiBootMemory *, const FormatterCpu *,
                        const struct bfm_host_bus_request *, uint32_t physical,
                        uint32_t value);
};

struct bfm_mounted_host_bus {
    MusashiBootMemory *memory;
    const FormatterCpu *cpu;
    const void *family;
    void *context;
    /* Pure original-family admission; authorize *both* guest addresses, width,
     * direction and lanes, including their exact table/device/alias route. */
    bool (*resolve)(void *, MusashiBootMemory *, const FormatterCpu *,
                    const struct bfm_host_bus_request *, struct bfm_host_bus_grant *);
};

struct bfm_host_bus_slot {
    bool bound;
    bool consumed;
    const struct bfm_mounted_host_bus *host;
    struct bfm_mounted_host_bus owner;
    struct bfm_host_bus_request request;
    struct bfm_host_bus_grant grant;
};

/* Start with a zero-initialized slot local to one actual instruction; use
 * the same slot for every check, through the transaction and retirement.
 * Natural width callbacks receive physical_address; this adapter preserves
 * the original guest alias when calling cpu_read/write. */
static inline bool
bfm_host_bus_same_request(const struct bfm_host_bus_request *a,
                          const struct bfm_host_bus_request *b)
{
    return a->effective_address == b->effective_address && a->bus_address == b->bus_address &&
           a->width == b->width && a->writing == b->writing && a->byte_enables == b->byte_enables;
}

static inline bool
bfm_host_bus_same_owner(const struct bfm_mounted_host_bus *a, const struct bfm_mounted_host_bus *b)
{
    return a->memory == b->memory && a->cpu == b->cpu && a->family == b->family &&
           a->context == b->context && a->resolve == b->resolve;
}

static inline bool
bfm_host_bus_same_grant(const struct bfm_host_bus_grant *a, const struct bfm_host_bus_grant *b)
{
    return a->region == b->region && a->provider == b->provider && a->revision == b->revision &&
           a->physical_address == b->physical_address && a->first == b->first && a->last == b->last &&
           a->masked_context == b->masked_context && a->write_masked == b->write_masked;
}

/* Check both guest addresses before asking the real family. The physical
 * address and grant come exclusively from that family; no alias is inferred. */
static inline bool
bfm_host_bus_resolve(const struct bfm_mounted_host_bus *host, const struct bfm_host_bus_request *req,
                     struct bfm_host_bus_grant *grant)
{
    if (!host || !host->memory || !host->cpu || !host->family || !host->resolve ||
        (req->width != 1u && req->width != 2u && req->width != 4u))
        return false;
    const unsigned width = req->width;
    const unsigned full = (1u << width) - 1u;
    if ((req->bus_address & (width - 1u)) != 0 || !req->byte_enables ||
        (req->byte_enables & ~full) != 0 ||
        (req->byte_enables != full && (!req->writing || width != 4u)) ||
        (req->effective_address != req->bus_address &&
         (width != 4u || (req->effective_address & ~UINT32_C(3)) != req->bus_address)))
        return false;
    if (!host->resolve(host->context, host->memory, host->cpu, req, grant))
        return false;
    const uint32_t physical = grant->physical_address;
    return grant->region && grant->provider && (physical & (width - 1u)) == 0 &&
           physical >= grant->first && physical <= grant->last &&
           width - 1u <= grant->last - physical &&
           (req->byte_enables == full || grant->write_masked);
}

static inline bool
bfm_host_bus_live(const struct bfm_host_bus_slot *slot)
{
    struct bfm_host_bus_grant current = {0};
    return slot && slot->bound && !slot->consumed && slot->host &&
           bfm_host_bus_same_owner(&slot->owner, slot->host) &&
           bfm_host_bus_resolve(slot->host, &slot->request, &current) &&
           bfm_host_bus_same_grant(&slot->grant, &current);
}

static inline int
bfm_host_bus_admit(void *context, uint32_t physical, unsigned width, int writing)
{
    const struct bfm_host_bus_slot *slot = context;
    return slot && slot->grant.physical_address == physical &&
           slot->request.width == width && (int)slot->request.writing == writing &&
           bfm_host_bus_live(slot);
}

static inline int
bfm_host_bus_read(void *context, uint32_t physical, unsigned width, uint32_t *out)
{
    struct bfm_host_bus_slot *slot = context;
    if (!out || !bfm_host_bus_admit(slot, physical, width, 0))
        return 0;
    slot->consumed = true;
    const uint32_t guest = slot->request.bus_address;
    MusashiBootMemory *memory = slot->owner.memory;
    const FormatterCpu *cpu = slot->owner.cpu;
    uint8_t byte;
    uint16_t half;
    uint32_t word;
    switch (width) {
    case 1:
        if (!cpu_read8(memory, cpu, guest, &byte)) return 0;
        *out = byte;
        return 1;
    case 2:
        if (!cpu_read16(memory, cpu, guest, &half)) return 0;
        *out = half;
        return 1;
    case 4:
        if (!cpu_read32(memory, cpu, guest, &word)) return 0;
        *out = word;
        return 1;
    default:
        return 0;
    }
}

static inline int
bfm_host_bus_write(void *context, uint32_t physical, unsigned width, uint32_t value, unsigned lanes)
{
    struct bfm_host_bus_slot *slot = context;
    if (!slot || lanes != slot->request.byte_enables ||
        !bfm_host_bus_admit(slot, physical, width, 1))
        return 0;
    slot->consumed = true;
    MusashiBootMemory *memory = slot->owner.memory;
    const FormatterCpu *cpu = slot->owner.cpu;
    const uint32_t guest = slot->request.bus_address;
    if (lanes != ((1u << width) - 1u)) {
        /* The admitting provider owns byte strobes, atomicity and the real
         * owner's bookkeeping. This route performs no read-modify-write. */
        return slot->grant.write_masked(slot->grant.masked_context, memory, cpu,
                                        &slot->request, physical, value);
    }
    switch (width) {
    case 1: return cpu_write8(memory, cpu, guest, value);
    case 2: return cpu_write16(memory, cpu, guest, (uint16_t)value);
    case 4: return cpu_write32(memory, cpu, guest, value);
    default: return 0;
    }
}

static inline bool
bfm_host_bus_map(struct bfm_host_bus_slot *slot, const struct bfm_mounted_host_bus *host,
                 const struct bfm_host_bus_request *request, struct bfm_scalar_mapping *mapping)
{
    if (!slot || !host || !request || !mapping || slot->consumed)
        return false;
    if (!slot->bound) {
        struct bfm_host_bus_grant grant = {0};
        if (!bfm_host_bus_resolve(host, request, &grant))
            return false;
        slot->host = host;
        slot->owner = *host;
        slot->request = *request;
        slot->grant = grant;
        slot->bound = true;
    } else if (slot->host != host || !bfm_host_bus_same_request(&slot->request, request) ||
               !bfm_host_bus_live(slot)) {
        return false;
    }

    *mapping = (struct bfm_scalar_mapping){
        .region = slot->grant.region, .revision = slot->grant.revision,
        .physical_address = slot->grant.physical_address,
        .first = slot->grant.first, .last = slot->grant.last,
        .bus_context = slot, .admit = bfm_host_bus_admit,
        .read = bfm_host_bus_read, .write = bfm_host_bus_write
    };
    return true;
}
#endif
