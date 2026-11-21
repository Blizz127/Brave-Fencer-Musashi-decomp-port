"""Instruction fetches must observe the live owner on every admitted address."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]

PROBE = r"""
#include <assert.h>
#include "mips_formatter.c"

static MusashiBootMemory memory;
static uint8_t image[CODE_IMAGE_SIZE];
static unsigned intersects_calls, reads, claim;
static int readable;
static uint32_t device_word;

static int intersects(uint32_t pc, size_t size) {
    assert(pc == 0x800424e4u && size == 4);
    ++intersects_calls;
    return claim;
}

static int read_word(void *arg, uint32_t pc, uint32_t *word) {
    assert(arg == &reads && pc == 0x800424e4u);
    ++reads;
    if (readable) *word = device_word;
    return readable;
}

int main(void) {
    const uint32_t pc = 0x800424e4u;
    MusashiBiosImageDevice owner = { .userdata = &reads, .intersects = intersects,
                                     .read32 = read_word };
    FormatterCpu cpu = {0};
    MusashiCpuContext context = {0};
    uint32_t word = 0;
    unsigned offset = CODE_IMAGE_TEXT + pc - CODE_IMAGE_BASE;
    cpu.pc = pc;
    cpu.npc = pc + 4u;
    g_code_image_state = -1;
    g_fetch_memory = &memory;

    assert(musashi_boot_write32(&memory, pc, 0x12345678u));
    assert(formatter_fetch(&memory, &cpu, &word) && word == 0x12345678u);
    assert(musashi_boot_write32(&memory, pc, 0x76543210u));
    assert(formatter_fetch(&memory, &cpu, &word) && word == 0x76543210u);
    assert(!formatter_fetch(NULL, &cpu, &word));
    assert(!formatter_fetch(&memory, NULL, &word));

    cpu.bios_image = &owner;
    readable = claim = 1;
    device_word = 0x123u;
    assert(formatter_fetch(&memory, &cpu, &word) && word == device_word);
    ++device_word;
    assert(musashi_boot_cpu_context(&cpu, MUSASHI_CPU_CONTEXT_SOURCE, &context));
    assert(context.instruction == device_word && intersects_calls == 2 && reads == 2);
    readable = 0;
    assert(!formatter_fetch(&memory, &cpu, &word) && word == 0x123u);
    owner.read32 = NULL;
    assert(!formatter_fetch(&memory, &cpu, &word));
    assert(intersects_calls == 4 && reads == 3);
    claim = 0;
    assert(formatter_fetch(&memory, &cpu, &word) && word == 0x76543210u);
    assert(intersects_calls == 6 && reads == 3);
    cpu.pc = pc + 2u;
    assert(!formatter_fetch(&memory, &cpu, &word));
    assert(intersects_calls == 6 && reads == 3);

    cpu.pc = pc;
    claim = 1;
    g_code_image = image;
    g_code_image_state = 1;
    image[offset] = 0x56;
    assert(formatter_fetch(&memory, &cpu, &word) && word == 0x56u);
    image[offset] = 0x78;
    assert(formatter_fetch(&memory, &cpu, &word) && word == 0x78u);
    assert(intersects_calls == 6 && reads == 3);
    g_code_image = NULL;
    g_code_image_state = -1;
    cpu.bios_image = NULL;

    cpu.pc = 0x800cedfcu;
    g_overlay_0010_words = 1;
    assert(formatter_fetch(&memory, &cpu, &word));
    g_overlay_0010_words = 0;
    g_overlay_0004_words = 1;
    assert(!formatter_fetch(&memory, &cpu, &word));
    g_overlay_0004_words = 0;
    g_overlay_0010_words = 1;
    assert(formatter_fetch(&memory, &cpu, &word));
    return 0;
}
"""


def test_fetch_observes_live_word_and_overlay(tmp_path):
    source = tmp_path / "fetch.c"
    binary = tmp_path / "fetch"
    source.write_text(PROBE)
    subprocess.run([
        "cc", "-std=c99", "-O2", "-ffunction-sections", "-fdata-sections",
        "-Wl,--gc-sections", "-I", str(ROOT / "include"), "-I", str(ROOT / "pc_port"),
        "-I", str(ROOT / "pc_port/include"), str(source), str(ROOT / "pc_port/boot_memory.c"),
        "-lcrypto", "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
