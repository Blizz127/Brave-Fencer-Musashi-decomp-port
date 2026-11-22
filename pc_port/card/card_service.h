#ifndef CARD_SERVICE_H
#define CARD_SERVICE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CS_IMAGE_BYTES 131072u
#define CS_SECTOR_BYTES 128u
#define CS_DIRENT_BYTES 40u
#define CS_EVENT_HW UINT32_C(0xf0000011)
#define CS_EVENT_SW UINT32_C(0xf4000001)
#define CS_DONE 4u
#define CS_TIMEOUT 0x100u
#define CS_CHANGED 0x2000u
#define CS_ERROR 0x8000u
#define CS_PREVIOUS_WRITE 0x8001u

typedef struct CardService CardService;
typedef enum {
    CS_MEDIA_OK, CS_MEDIA_ABSENT, CS_MEDIA_MISSING, CS_MEDIA_ERROR,
    CS_MEDIA_UNFORMATTED  /* preparation result for a present all-zero image */
} CsMedia;

typedef struct {
    void *context;
    /* Map the entire guest byte interval or return NULL. The pointer stays
       valid until the next map call. Slots below are 0 or 1, not port IDs. */
    uint8_t *(*map)(void *, uint32_t address, size_t count, int writable);
    /* Read exactly one image. MISSING means an absent backing file inside
       an exclusively mounted session. ABSENT means no device. Return OK
       for any exact-length fully readable image including all-zero storage;
       UNFORMATTED is classified by cs_prepare after a successful snapshot. */
    CsMedia (*snapshot)(void *, unsigned slot, uint8_t *destination);
    /* Publish exactly one image under the session's exclusive writer lock.
       Before replacement verify 'expected' against current bytes, or verify
       absence when it is NULL. Failure must preserve durable prior bytes. */
    int (*commit)(void *, unsigned slot, const uint8_t *expected,
                  const uint8_t *replacement);
    /* Return nonzero only after accepting the event exactly once. Returning
       zero guarantees no delivery and permits retry on the next pump. */
    int (*event)(void *, uint32_t event_class, uint32_t spec);
} CsHost;

/* One serialized instance per pair of mounted ports. Drain pending commands
   before destruction or stopping the host. Guest callbacks may query status;
   the host must not re-enter other service calls from a host callback. */
CardService *cs_create(CsHost host);
void cs_destroy(CardService *s);
/* Verify an existing medium without changing its contents, or initialize
   absent backing storage through create-if-absent. CS_MEDIA_UNFORMATTED
   keeps an existing all-zero medium mounted for raw access and explicit
   format. Never turn a present medium or read error into missing storage. */
CsMedia cs_prepare(CardService *s, unsigned slot);
void cs_init_card(CardService *s);
void cs_start_card(CardService *s);
void cs_stop_card(CardService *s);
void cs_init_files(CardService *s);
/* Required for every detach, swap or reattach. Serialize with all I/O and
   notify on the old slot before detaching its storage or exposing replacement
   bytes. Stale descriptors must be closed; queued transfers complete as
   changed. Otherwise a still-live descriptor could write the new medium. */
void cs_media_changed(CardService *s, unsigned slot);

/* A host with other BIOS devices reserves each of their live descriptors
   here. Returns 0 on an invalid fd or a collision with a live card handle. */
int cs_reserve_fd(CardService *s, int32_t fd, int reserved);
int32_t cs_open(CardService *s, uint32_t path, uint32_t mode);
typedef struct {
    uint32_t position, destination, count;
} CsCompletedRead;

/* Pure inspection of one literal, async read-only handle. Both queues and
   their notifications must be idle. The entire row must match the latest
   result from this slot's current listing and this descriptor's file.
   A zero count checks just the cursor, including EOF. If supplied, completed
   must match the latest successful asynchronous file read through this open.
   A positive ID identifies this open until close or media change. IDs belong
   to this service instance and never repeat within it. */
uint64_t cs_async_read_descriptor(const CardService *s, const void *host_owner,
                                  unsigned slot, int32_t fd, const char *name,
                                  const uint8_t row[CS_DIRENT_BYTES], uint32_t file_size,
                                  uint32_t position, uint32_t count, const CsCompletedRead *completed);
int32_t cs_seek(CardService *s, int32_t fd, int32_t displacement, unsigned origin);
int32_t cs_read(CardService *s, int32_t fd, uint32_t destination, uint32_t count);
int32_t cs_write(CardService *s, int32_t fd, uint32_t source, uint32_t count);
int32_t cs_close(CardService *s, int32_t fd);
uint32_t cs_first(CardService *s, uint32_t pattern, uint32_t destination);
uint32_t cs_next(CardService *s, uint32_t destination);
int cs_erase(CardService *s, uint32_t path);
int cs_format(CardService *s, uint32_t device);
int cs_errno(const CardService *s);
int cs_fd_error(CardService *s, int32_t fd);

/* Port IDs 0x00 and 0x10 correspond to slots 0 and 1. Submission returns 1
   when accepted. Raw buffer validity is checked before queuing. Status/wait
   take a slot number. chan returns the port ID from the last completion. */
int cs_sector_read(CardService *s, unsigned port, unsigned sector, uint32_t destination);
int cs_sector_write(CardService *s, unsigned port, unsigned sector, uint32_t source);
int cs_card_info(CardService *s, unsigned port);
int cs_card_info_sub(CardService *s, unsigned port);
int cs_card_load(CardService *s, unsigned port);
int cs_write_test(CardService *s, unsigned port);
void cs_new_card(CardService *s);
int cs_card_status(const CardService *s, unsigned slot);
int cs_card_wait(CardService *s, unsigned slot);
unsigned cs_card_chan(const CardService *s);

/* Work and notifications run on a later pump. Bit 0 means a command ran,
   bit 1 means at least one event was accepted, bit 2 means delivery stalled.
   Retry a stall without repeating a completed command or accepted event.
   cs_idle returns 1 only once all operations and notifications have drained. */
unsigned cs_pump(CardService *s);
int cs_idle(const CardService *s);

#ifdef __cplusplus
}
#endif
#endif
