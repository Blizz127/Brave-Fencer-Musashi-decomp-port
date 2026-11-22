#ifndef MUSASHI_MAIN12_H
#define MUSASHI_MAIN12_H

#include "musashi_disc_media.h"
#include "musashi_guest_irq.h"

typedef struct MusashiMain12Owner MusashiMain12Owner;

/* Both the mapped EXE and the immutable mounted disc have passed their
 * release identity checks. The serialized machine owns the returned handle
 * and closes it before its RAM, image or disc. Remapping revokes the handle. */
MusashiMain12Owner *musashi_main12_open(MusashiGuestIrqImage *image,
    MusashiBootMemory *memory, const MusashiDiscMedia *media);
void musashi_main12_close(MusashiMain12Owner *owner);

#endif
