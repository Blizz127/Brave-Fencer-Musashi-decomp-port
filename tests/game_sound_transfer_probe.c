#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

/* Keep the caller and provider definitions type-checked in one unit. */
int func_8002FC64(int bytes, void *data);
void func_8002FDC8(void);
#include "../src/main/8002fc64.c"
#include "../src/main/8002fdc8.c"

_Static_assert(UINT_MAX == UINT32_MAX && INT_MIN == -INT32_MAX - 1,
               "game cursor arithmetic uses 32-bit words");

unsigned char D_8006AEF4;
int D_800A46C8;

typedef struct {
    unsigned char flags;
    int cursor;
} StreamState;

enum Operation { POLL, SET_ADDRESS, SUBMIT, STOP };

typedef struct {
    enum Operation operation;
    StreamState before, after;
    int result;
} ServiceCall;

static const ServiceCall *calls;
static size_t count, used;
static void *expected_data;
static unsigned int expected_bytes;

static void check_state(StreamState expected)
{
    assert(D_8006AEF4 == expected.flags);
    assert(D_800A46C8 == expected.cursor);
}

static int serve(enum Operation operation)
{
    const ServiceCall *call;
    assert(used < count);
    call = &calls[used++];
    assert(call->operation == operation);
    check_state(call->before);
    D_8006AEF4 = call->after.flags;
    D_800A46C8 = call->after.cursor;
    return call->result;
}

int func_8003C4F0(int mode)
{
    assert(mode == 0);
    return serve(POLL);
}

int func_8003C498(unsigned int address)
{
    assert(address == (unsigned int)D_800A46C8);
    return serve(SET_ADDRESS);
}

unsigned int func_8003C438(void *data, unsigned int bytes)
{
    assert(data == expected_data && bytes == expected_bytes);
    return (unsigned int)serve(SUBMIT);
}

void func_80037D74(void)
{
    (void)serve(STOP);
}

static void start(const ServiceCall *script, size_t size)
{
    calls = script;
    count = size;
    used = 0;
    D_8006AEF4 = script[0].before.flags;
    D_800A46C8 = script[0].before.cursor;
}

static void transfer(const ServiceCall *script, size_t size, int bytes, void *data,
                     int result, StreamState final)
{
    start(script, size);
    expected_data = data;
    expected_bytes = (unsigned int)bytes;
    assert(func_8002FC64(bytes, data) == result);
    assert(used == count);
    check_state(final);
}

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

int main(void)
{
    unsigned char payload;
    void *wide_pointer = &payload;
#if UINTPTR_MAX > UINT32_MAX
    /* This opaque token is compared only. It is never dereferenced. */
    wide_pointer = (void *)(((uintptr_t)1 << 32) + 37);
#endif
    const ServiceCall poll_refused[] = {
        {POLL, {0x81, 17}, {0x20, -3}, 0},
        {STOP, {0x20, -3}, {0x42, 29}, 0}
    };
    transfer(poll_refused, COUNT(poll_refused), 8, &payload, 0, (StreamState){0x42, 29});

    const ServiceCall address_refused[] = {
        {SET_ADDRESS, {0x80, -7}, {0x01, 4}, 0},
        {STOP, {0x01, 4}, {0x08, 13}, 0}
    };
    transfer(address_refused, COUNT(address_refused), 3, NULL, 0, (StreamState){0x08, 13});

    const ServiceCall busy_then_ready[] = {
        {POLL, {0x05, 9}, {0x62, -11}, -2},
        {SET_ADDRESS, {0x62, -11}, {0x10, 6}, -1},
        {SUBMIT, {0x10, 6}, {0xa4, 0}, 0}
    };
    transfer(busy_then_ready, COUNT(busy_then_ready), -1, wide_pointer,
             1, (StreamState){0xa5, -1});

    const ServiceCall ready[] = {
        {SET_ADDRESS, {0x02, 13}, {0x40, 9}, 3},
        {SUBMIT, {0x40, 9}, {0x84, INT_MAX}, -1}
    };
    transfer(ready, COUNT(ready), 1, &payload, 1, (StreamState){0x85, INT_MIN});
    transfer(ready, COUNT(ready), 0, NULL, 1, (StreamState){0x85, INT_MAX});

    const ServiceCall stop[] = {
        {STOP, {0x91, 5}, {0x20, -5}, 0}
    };
    start(stop, COUNT(stop));
    func_8002FDC8();
    assert(used == count);
    check_state(stop[0].after);
    return 0;
}
