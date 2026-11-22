#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

extern void func_80019AF8(int seek_only);

#define CELL(address) (*(uint32_t *)(uintptr_t)(address))
#define BASE 0x800a0000u
enum {
    STATUS = 0x800ae6e8u, STATE = 0x800ae6f0u, MODE = 0x800ae6f8u,
    RESULT = 0x800ae70cu, COUNT = 0x800ae71cu, TRACKS = 0x800ae724u,
    HANDLE = 0x800ae734u, REPLY = 0x800ae738u, PARAM = 0x800ae740u,
    CHUNKS = 0x800ae748u, TRANSFER = 0x800ae74cu, TIMER = 0x800ae750u,
    WAIT = 0x800ae754u, POSITION = 0x800ae7a4u, SIZE = 0x800ae7a8u,
    SECTOR = 0x800ae7acu, DONE = 0x800ae7b0u,
    TABLE_A = BASE + 256, TABLE_B = BASE + 512, POS_A = BASE + 768,
    POS_B = BASE + 784, POS_C = BASE + 800, CALLBACK = 0x8001a338u,
};

static int poll_result, failed_command, track_calls;
static char calls[16];
static unsigned call_count;

static void record(char call)
{
    assert(call_count + 1 < sizeof calls);
    calls[call_count++] = call;
    calls[call_count] = 0;
}

static void begin(unsigned state)
{
    memset((void *)(uintptr_t)BASE, 0, 65536);
    CELL(STATE) = state;
    CELL(DONE) = 27;
    poll_result = failed_command = track_calls = call_count = 0;
    calls[0] = 0;
}

void func_800435CC(int command, const void *parameter, unsigned char *reply)
{
    record('C');
    assert((uintptr_t)reply == REPLY);
    *reply = command == failed_command ? 16 : 0;
    switch (command) {
    case 14:
        assert((uintptr_t)parameter == PARAM && *(const unsigned char *)parameter == 160);
        break;
    case 21:
        assert((uintptr_t)parameter == POS_A);
        CELL(POSITION) = POS_B;
        break;
    case 9:
        assert(parameter == NULL);
        CELL(POSITION) = POS_C;
        break;
    default:
        assert(command == 6 && parameter == NULL);
    }
}

int func_8004355C(int mode, unsigned char *reply)
{
    record('P');
    assert(mode == 1 && (uintptr_t)reply == REPLY);
    if (!strcmp(calls, "CLP")) assert(CELL(STATE) == 36);
    return poll_result;
}

void func_800434BC(void)
{
    record('R');
    CELL(STATE) = 41;
}

unsigned func_800435B4(unsigned callback)
{
    record('H');
    if (CELL(STATE) == 5) assert(callback == CALLBACK);
    else assert(callback == 17);
    return 73;
}

int func_80043B1C(const void *position)
{
    record('L');
    assert((uintptr_t)position == POS_B);
    CELL(STATE) = 35;
    return 57;
}

int func_8002FD14(int id, int argument)
{
    static const int expected[][2] = {{11, 13}, {21, 23}, {31, 33}};
    record('T');
    assert(track_calls < 3);
    assert(id == expected[track_calls][0] && argument == expected[track_calls][1]);
    ++track_calls;
    CELL(TRACKS) = TABLE_B;
    CELL(COUNT) = track_calls == 1 ? 0 : 2;
    return track_calls == 1 ? 0 : -7;
}

void func_8001A0FC(void) { record('S'); }
int func_8001A114(void) { record('F'); return -5; }

int main(void)
{
    void *wanted = (void *)(uintptr_t)BASE;
    assert(mmap(wanted, 65536, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) == wanted);

    begin(UINT32_MAX);
    func_80019AF8(0);
    assert(!calls[0] && CELL(STATE) == UINT32_MAX && !CELL(DONE));

    begin(0);
    failed_command = 14;
    func_80019AF8(0);
    assert(!strcmp(calls, "C") && CELL(STATE) == 9 && !CELL(DONE));

    begin(1);
    poll_result = -11;
    func_80019AF8(0);
    assert(!strcmp(calls, "P") && CELL(STATUS) == (uint32_t)-11 && CELL(STATE) == 1);

    begin(2);
    CELL(TIMER) = INT32_MAX;
    func_80019AF8(0);
    assert(!calls[0] && CELL(TIMER) == (uint32_t)INT32_MIN && CELL(STATE) == 2);

    begin(3);
    poll_result = 2;
    CELL(POSITION) = POS_A;
    CELL(POS_C) = 97;
    func_80019AF8(-1);
    assert(!strcmp(calls, "CLPC") && CELL(STATE) == 8 && CELL(RESULT) == 97 && CELL(SECTOR) == 57);

    begin(4);
    poll_result = -3;
    CELL(WAIT) = 179;
    func_80019AF8(0);
    assert(!strcmp(calls, "P") && CELL(WAIT) == 180 && CELL(STATE) == 4 && !CELL(DONE));
    func_80019AF8(0);
    assert(!strcmp(calls, "PPR") && CELL(WAIT) == 181 && CELL(STATE) == 3);

    begin(5);
    CELL(SIZE) = INT32_MAX;
    func_80019AF8(0);
    assert(!strcmp(calls, "HC") && CELL(MODE) == 3 && CELL(STATE) == 6);
    assert(CELL(CHUNKS) == (uint32_t)-536870911 && CELL(HANDLE) == 73);

    begin(6);
    CELL(TIMER) = 299;
    CELL(HANDLE) = 17;
    func_80019AF8(0);
    assert(!strcmp(calls, "HR") && CELL(TIMER) == 300 && CELL(STATE) == 41);

    begin(7);
    CELL(TRACKS) = TABLE_A;
    CELL(COUNT) = 1;
    CELL(TABLE_A) = 11; CELL(TABLE_A + 4) = 13;
    CELL(TABLE_B) = 21; CELL(TABLE_B + 4) = 23;
    CELL(TABLE_B + 8) = 31; CELL(TABLE_B + 12) = 33;
    func_80019AF8(0);
    assert(!strcmp(calls, "TTT") && CELL(STATE) == 8);

    begin(8);
    poll_result = 2;
    func_80019AF8(0);
    assert(!strcmp(calls, "P") && CELL(STATE) == 0 && CELL(DONE) == 1);

    begin(9);
    func_80019AF8(0);
    assert(!strcmp(calls, "SF") && CELL(STATE) == 0 && CELL(STATUS) == (uint32_t)-5);

    return 0;
}
