#include <assert.h>
#include <stdint.h>

void func_800141F0(void);
void func_80016714_native(uint8_t *destination, int32_t size);

signed char D_800BA118[160];
static unsigned next_call;
static unsigned fill;

static void expect_call(unsigned index) {
    assert(index == next_call);
    ++next_call;
}

void func_80059234(int mode) {
    expect_call(0);
    assert(mode == 0);
}

void func_800594CC(int mode) {
    expect_call(1);
    assert(mode == 0);
    D_800BA118[0] = (signed char)fill;
}

void func_80059658(int mode) {
    expect_call(2);
    assert(mode == 0);
    D_800BA118[151] = (signed char)~fill;
}

void func_80016714(signed char *destination, int size) {
    expect_call(3);
    assert(destination == D_800BA118 && size == 152);
    assert((unsigned char)destination[0] == fill);
    assert((unsigned char)destination[151] == (unsigned char)~fill);
    func_80016714_native((uint8_t *)destination, size);
}

void func_80029044(void) {
    expect_call(4);
    for (unsigned i = 0; i < 152; ++i) assert(D_800BA118[i] == 0);
}

void func_8002906C(void) { expect_call(5); }
void func_80029094(void) { expect_call(6); }
void func_80029218(void) { expect_call(7); }
void func_8001903C(void) { expect_call(8); }
void func_80018FC8(void) { expect_call(9); }
void func_8001B324(void) { expect_call(10); }
void func_80028FDC(int value) { expect_call(11); assert(value == 7); }
void func_80029020(int value) { expect_call(12); assert(value == 7); }
void func_80028DBC(int value) { expect_call(13); assert(value == 0); }
void func_80028E00(int value) { expect_call(14); assert(value == 0); }

int main(void) {
    for (fill = 0; fill < 256; ++fill) {
        for (unsigned i = 0; i < sizeof D_800BA118; ++i)
            D_800BA118[i] = (signed char)(i + fill);
        next_call = 0;
        func_800141F0();
        assert(next_call == 15);
        for (unsigned i = 152; i < sizeof D_800BA118; ++i)
            assert((unsigned char)D_800BA118[i] == (unsigned char)(i + fill));
    }
    return 0;
}
