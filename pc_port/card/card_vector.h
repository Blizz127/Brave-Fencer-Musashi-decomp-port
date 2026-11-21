#ifndef CARD_VECTOR_H
#define CARD_VECTOR_H

#include "card_service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    CardService *service;
    void *context;
    uint8_t *(*map)(void *, uint32_t, size_t, int);
    uint16_t file_handles;
    int owns_search;
} CsVector;

typedef struct {
    int recognized;       /* zero: leave the call entirely to the host */
    int write_v0;         /* zero: preserve the caller's return register */
    uint32_t v0;          /* guest 32-bit bit pattern */
    int file_error;       /* nonzero: also update the common file errno */
} CsVectorResult;

/* One dispatcher per guest, initialized before file calls. Guest file handles
   opened here stay under this dispatcher's ownership until close. Keep other
   devices' live descriptors reserved via cs_reserve_fd. Pass the same mapping
   callback/context used by the service. It is used to identify device names
   without sending unrelated file operations into the memory-card module. */
void cs_vector_init(CsVector *v, CardService *service, CsHost host);

/* vector is the BIOS vector identifier (0xA0 or 0xB0), not a guest PC.
   selector and a0..a3 are the integer ABI inputs. A recognized asynchronous
   call still requires cs_pump on the host scheduler. */
CsVectorResult cs_vector_dispatch(CsVector *v, uint32_t vector, uint32_t selector,
                                  uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3);
#ifdef __cplusplus
}
#endif
#endif
