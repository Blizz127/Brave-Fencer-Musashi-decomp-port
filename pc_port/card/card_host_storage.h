#ifndef CARD_HOST_STORAGE_H
#define CARD_HOST_STORAGE_H

#include "card_service.h"
#include "bfm_plat_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CsStorage CsStorage;

/* Select the platform save directory before acquiring either slot. Serialize
   all calls within a process. Use the bridge as sole owner of each acquired
   platform session. A forked child must acquire its own sessions. Detach
   both slots before changing the save directory. */
CsStorage *cs_storage_create(void);
void cs_storage_destroy(CsStorage *storage);
/* BFM_PLAT_OK means the exclusive session is held. The backing image may
   still be missing. Reattaching the same slot on the same handle is safe. */
int cs_storage_attach(CsStorage *storage, unsigned slot);
void cs_storage_detach(CsStorage *storage, unsigned slot);

/* CsHost callbacks. The context is a CsStorage*. Each image argument points
   to CS_IMAGE_BYTES bytes, with NULL expected meaning create-if-absent. */
CsMedia cs_storage_snapshot(void *context, unsigned slot, uint8_t *destination);
int cs_storage_commit(void *context, unsigned slot, const uint8_t *expected,
                      const uint8_t *replacement);

#ifdef __cplusplus
}
#endif
#endif
