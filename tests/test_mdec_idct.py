"""Independent cosine-basis check of the uploaded-scale integer IDCT."""
from pathlib import Path
import ctypes
import math
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]


def test_all_idct_basis_vectors():
    with tempfile.TemporaryDirectory() as tmp:
        p=Path(tmp)
        (p/'probe.c').write_text('''
#include "musashi_mdec_controller.h"
#include <string.h>
#include "musashi_mdec_decode.h"
void transform(int *b,const int16_t *s) { mdec_idct(b,s); }
''')
        subprocess.run(['cc','-std=c99','-shared','-fPIC','-O2','-I',str(ROOT/'pc_port/include'),str(p/'probe.c'),'-o',str(p/'probe.so')],check=True)
        lib=ctypes.CDLL(str(p/'probe.so'))
        lib.transform.argtypes=[ctypes.POINTER(ctypes.c_int),ctypes.POINTER(ctypes.c_int16)]
        # Uploaded PSX scale table, generated independently from cosine bases.
        basis=[[math.cos((2*x+1)*u*math.pi/16)*(1 if u==0 else math.sqrt(2)) for x in range(8)] for u in range(8)]
        scale=(ctypes.c_int16*64)(*[int(v*math.sqrt(2)*16384) for row in basis for v in row])
        for index in range(64):
            for amplitude in (-512,511):
                b=(ctypes.c_int*64)();b[index]=amplitude
                lib.transform(b,scale)
                # Two transposed passes produce the 2-D inverse DCT.
                for y in range(8):
                    for x in range(8):
                        expected=amplitude*basis[index%8][x]*basis[index//8][y]/8
                        assert abs(b[y*8+x]-expected)<2.1, (index,x,y,b[y*8+x],expected)


def test_rgb555_rounding_signed_and_mask(tmp_path):
    probe = tmp_path / 'colors.c'
    probe.write_text('''
#include "musashi_mdec_controller.h"
#include <assert.h>
#include <string.h>

static MusashiMdecController decoder;
static MusashiBootMemory ram;
static MusashiDmaController dma;

uint32_t first_pixel(uint32_t mode, int cr, int cb, int y, unsigned quant) {
    musashi_mdec_init(&decoder, &ram, &dma);
    memset(decoder.quant, quant, sizeof(decoder.quant));
    for (unsigned i = 0; i < 8; ++i) decoder.scale[i] = 0x5a82;
    assert(musashi_mdec_write32(&decoder, 0x1f801820u, mode | 6u));
    int coefficients[6] = {cr, cb, y, y, y, y};
    for (unsigned i = 0; i < 6; ++i) {
        uint32_t dc = 0x400u | ((unsigned)coefficients[i] & 0x3ffu);
        assert(musashi_mdec_write32(&decoder, 0x1f801820u, 0xfe000000u | dc));
    }
    uint32_t pixel = decoder.output[0] | (uint32_t)decoder.output[1] << 8;
    if (decoder.output_size == 768) pixel |= (uint32_t)decoder.output[2] << 16;
    else assert(decoder.output_size == 512);
    return pixel;
}
''')
    include = ROOT / 'pc_port/include'
    library = tmp_path / 'colors.so'
    subprocess.run(['cc', '-std=c11', '-shared', '-fPIC', '-O2', '-I', str(include),
                    str(probe), str(ROOT / 'pc_port/mdec_controller.c'),
                    str(ROOT / 'pc_port/dma_controller.c'), '-o', str(library)], check=True)
    render = ctypes.CDLL(str(library)).first_pixel
    render.argtypes = [ctypes.c_uint32, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                       ctypes.c_uint]
    render.restype = ctypes.c_uint32

    def pixel(mode, cr, cb, y, quant=8):
        return render(mode, cr, cb, y, quant)

    rounded_up = set()
    saturated = set()
    for y in range(-136, 137):
        cr, cb = (y % 13) - 6, (y % 23) - 11
        color = pixel(0x30000000, cr, cb, y)
        assert pixel(0x34000000, cr, cb, y) == color ^ 0x808080
        expected = 0
        for channel in range(3):
            value = (color >> (8 * channel)) & 255
            level, remainder = divmod(value, 8)
            if remainder >= 4 and level < 31:
                level += 1
                rounded_up.add(channel)
            if value >= 252:
                saturated.add(channel)
            expected |= level << (5 * channel)
        for flags, output_mask in [(0, 0), (0x04000000, 0x4210),
                                   (0x02000000, 0x8000), (0x06000000, 0xc210)]:
            assert pixel(0x38000000 | flags, cr, cb, y) == expected ^ output_mask
    assert rounded_up == saturated == {0, 1, 2}

    # Measured on SCPH-5501 using DC-only inputs and a uniform quantizer of 4.
    # The positive and negative excursions cross the converter's signed nine-bit range.
    for cr, cb, y, blue in [(-256, -256, -128, 255), (255, -256, -128, 255),
                            (-256, 255, 127, 0), (255, 255, 127, 0)]:
        assert pixel(0x30000000, cr, cb, y, 4) >> 16 == blue
        assert pixel(0x38000000, cr, cb, y, 4) >> 10 == blue >> 3
