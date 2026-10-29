/* Owner re-derivation from retail assembly (clean input set). */
/* Queues a copy of a request into the 4-slot ring at D_80076118 and returns the slot number plus one (0 when full). */
#include "psx_types.h"

typedef struct {
    u8 active;
    u8 b1;
    u8 b2;
    u8 b3;
    u8 pad4[3];
    u8 b7;
    s32 data[8];
    s32 w28;
    s32 slot;
} QueueEntry;

typedef struct {
    u8 pad0[8];
    s32 data[8];
    s32 w28;
} QueueReq;

extern u8 D_8006AEF5;
extern s32 D_8006AEF8;
extern s32 D_8006AEFC;
extern QueueEntry D_80076118[];

s32 func_80034CF0(QueueReq *req)
{
    QueueEntry *e;
    s32 next;
    s32 cur;
    s32 head;
    s32 w;
    u8 lock;

    next = D_8006AEFC;
    lock = D_8006AEF5;
    head = D_8006AEF8;
    D_8006AEF5 = 1;
    next++;
    if (next >= 4) {
        next = 0;
    }
    if (next == head) {
        D_8006AEF5 = lock;
        return 0;
    }
    e = &D_80076118[D_8006AEFC];
    e->data[0] = req->data[0];
    e->data[1] = req->data[1];
    e->data[2] = req->data[2];
    e->data[3] = req->data[3];
    e->data[4] = req->data[4];
    e->data[5] = req->data[5];
    e->data[6] = req->data[6];
    e->data[7] = req->data[7];
    w = req->w28;
    e->active = 1;
    e->b1 = 0;
    e->b2 = 0;
    e->b3 = 0;
    e->b7 = 1;
    cur = D_8006AEFC;
    D_8006AEFC = next;
    D_8006AEF5 = lock;
    e->w28 = w;
    e->slot = cur;
    return cur + 1;
}
