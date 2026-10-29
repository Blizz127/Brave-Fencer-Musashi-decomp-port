/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Services the head of the 4-entry request queue: runs an active request's step/completion callbacks, or polls a busy one, and advances the queue head. */
#include "psx_types.h"

typedef struct Request {
    u8 active;
    u8 busy;
    u8 hold;
    u8 pad3;
    u8 id;
    u8 pad5[2];
    u8 suspended;
    u32 step; /* guest s32 (*)(Request *) */
    u8 pad0C[0x1C - 0x0C];
    u32 done; /* guest void (*)(s32, s32) */
    s32 done_arg;
    u32 poll; /* guest void (*)(Request *) */
    u8 pad28[0x30 - 0x28];
} Request;

extern u8 D_8006AEF5;
extern u32 D_8006AEF8;
extern u32 D_8006AEFC;
extern s32 D_80076114;
extern Request D_80076118[4];
extern s8 D_8007620C;

void func_8003500C(void) {
    Request *req;
    u8 id;

    if (D_8006AEF5 != 0) {
        return;
    }
    if (D_8006AEFC != D_8006AEF8) {
        req = &D_80076118[D_8006AEF8];
        if (req->active != 0) {
            D_80076114 = req->id;
            if (((s32 (*)(struct Request *))req->step)(req) == 0) {
                return;
            }
            id = req->id;
            D_80076114 = id;
            if (req->done != 0) {
                ((void (*)(s32, s32))req->done)(D_80076118[D_8006AEF8].done_arg, id);
            }
            req->active = 0;
            req->busy = 0;
            if (++D_8006AEF8 >= 4) {
                D_8006AEF8 = 0;
            }
            if (D_8006AEF8 == D_8006AEFC) {
                D_80076114 = 0;
            }
            return;
        }
        if (req->busy != 0 && req->suspended == 0) {
            if (req->poll != 0) {
                ((void (*)(struct Request *))req->poll)(req);
            }
            if (req->hold != 0) {
                return;
            }
        }
        req->busy = 0;
        if (++D_8006AEF8 >= 4) {
            D_8006AEF8 = 0;
        }
        if (D_8006AEF8 == D_8006AEFC) {
            D_80076114 = 0;
        } else {
            D_80076114 = 1;
        }
    } else {
        D_8007620C = 0;
    }
}
