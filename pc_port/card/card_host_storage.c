#include "card_host_storage.h"

#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/types.h>
#include <unistd.h>
#endif

#if BFM_MEMCARD_SIZE != CS_IMAGE_BYTES || BFM_MEMCARD_SLOTS != 2
#error "Card service and platform storage geometry must agree"
#endif

struct CsStorage {
    uint8_t image[CS_IMAGE_BYTES];
};

/* The platform shares acquisitions between callers in the same process.
   Track the bridge handle that owns each acquisition so another handle
   cannot treat it as a separate writer or release its lock. */
static CsStorage **owner_slot(unsigned slot)
{
    static CsStorage *owners[BFM_MEMCARD_SLOTS];
#ifndef _WIN32
    static pid_t owner_process;
    pid_t current = getpid();
    if (current != owner_process) {
        unsigned i;
        /* fork copies these pointers. The parent's lock remains its own. */
        for (i = 0; i < BFM_MEMCARD_SLOTS; ++i)
            owners[i] = NULL;
        owner_process = current;
    }
#endif
    return &owners[slot];
}

CsStorage *cs_storage_create(void)
{
    return malloc(sizeof(CsStorage));
}

int cs_storage_attach(CsStorage *storage, unsigned slot)
{
    int result;
    CsStorage **owner;
    if (!storage || slot >= BFM_MEMCARD_SLOTS)
        return BFM_PLAT_INVALID;
    owner = owner_slot(slot);
    if (*owner == storage)
        return BFM_PLAT_OK;
    if (*owner)
        return BFM_PLAT_NOT_READY;
    result = bfm_plat_memcard_acquire(slot);
    if (result == BFM_PLAT_OK)
        *owner = storage;
    return result;
}

void cs_storage_detach(CsStorage *storage, unsigned slot)
{
    CsStorage **owner;
    if (!storage || slot >= BFM_MEMCARD_SLOTS)
        return;
    owner = owner_slot(slot);
    if (*owner == storage) {
        bfm_plat_memcard_release(slot);
        *owner = NULL;
    }
}

void cs_storage_destroy(CsStorage *storage)
{
    unsigned slot;
    for (slot = 0; slot < BFM_MEMCARD_SLOTS; ++slot)
        cs_storage_detach(storage, slot);
    free(storage);
}

static CsMedia load_current(CsStorage *storage, unsigned slot)
{
    int result;
    if (!storage || slot >= BFM_MEMCARD_SLOTS)
        return CS_MEDIA_ERROR;
    if (*owner_slot(slot) != storage)
        return CS_MEDIA_ABSENT;
    result = bfm_plat_memcard_load(slot, storage->image, sizeof(storage->image));
    if (result == BFM_PLAT_OK)
        return CS_MEDIA_OK;
    if (result == BFM_PLAT_NOT_FOUND)
        return CS_MEDIA_MISSING;
    return CS_MEDIA_ERROR;
}

CsMedia cs_storage_snapshot(void *context, unsigned slot, uint8_t *destination)
{
    CsStorage *storage = context;
    CsMedia media;
    if (!destination)
        return CS_MEDIA_ERROR;
    media = load_current(storage, slot);
    if (media == CS_MEDIA_OK)
        memcpy(destination, storage->image, sizeof(storage->image));
    return media;
}

int cs_storage_commit(void *context, unsigned slot, const uint8_t *expected,
                      const uint8_t *replacement)
{
    CsStorage *storage = context;
    CsMedia media;
    if (!replacement)
        return 0;
    media = load_current(storage, slot);
    if (expected) {
        if (media != CS_MEDIA_OK ||
            memcmp(storage->image, expected, sizeof(storage->image)) != 0)
            return 0;
    } else if (media != CS_MEDIA_MISSING) {
        return 0;
    }
    return bfm_plat_memcard_save(slot, replacement, CS_IMAGE_BYTES) == BFM_PLAT_OK;
}
