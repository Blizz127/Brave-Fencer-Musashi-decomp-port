"""Full first-PVD source fixture; no real PsyCross worker or cold boot claim."""
from pathlib import Path
import subprocess
import sys
import tempfile
import os
import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).parent))
from test_audio_sdl import _sdl_compile_flags


def _compile_and_run(mutation=None, *, compiler="cc", extra_flags=()):
    cue, binary = ROOT / 'extracted/disc/disc.cue', ROOT / 'extracted/disc/disc.bin'
    guest_exe = ROOT / 'extracted/disc/files/SLUS_007.26'
    if not cue.exists() or not binary.exists() or not guest_exe.exists():
        pytest.skip('licensed pinned media unavailable')
    flags, _ = _sdl_compile_flags()
    assert flags is not None, "SDL2 SDK metadata unavailable"
    with tempfile.TemporaryDirectory(prefix='musashi-pvd-') as tmp:
        base=Path(tmp)
        exe=base/'probe'
        sources = {name: ROOT/'pc_port'/f'{name}.c' for name in (
            'mips_formatter','boot_memory','cd_controller','disc_media','bios_events',
            'bios_kernel','cd_dma3','dma_controller','scanline_timer','gpu_controller',
            'irq_controller','irq_scheduler','cpu_status')}
        if mutation:
            name, needle, replacement = mutation
            original = sources[name].read_text()
            assert original.count(needle) == 1
            sources[name] = base/f'{name}_mutant.c'
            sources[name].write_text(original.replace(needle,replacement))
        compiled = subprocess.run([compiler,'-std=c99','-O2','-Wall','-Wextra','-Werror',
            *extra_flags,
            '-Wno-parentheses','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
            '-DMUSASHI_NATIVE_IRQ_DISPATCH=1','-DMUSASHI_NATIVE_CALLBACK_HANDLER=1',
            '-DMUSASHI_NATIVE_DMA_REGISTRATION=1','-DMUSASHI_NATIVE_DMA_IRQ_HANDLER=1',
            '-I',str(ROOT/'include'),'-I',str(ROOT/'pc_port/include'),'-I',str(ROOT/'pc_port'),
            str(ROOT/'tests/pvd_irq_probe.c'),
            *map(str,sources.values()),
            *[str(ROOT/'src/main'/f'{name}.c') for name in
              ('8001096c',)],str(ROOT/'pc_port/irq_direct_compat.c'),
            '-lcrypto',*flags,'-o',str(exe)],capture_output=True,text=True,timeout=30)
        assert compiled.returncode == 0, compiled.stderr
        return subprocess.run([str(exe),str(cue),str(binary),str(guest_exe)],capture_output=True,text=True,timeout=30)


def test_pvd_irq():
    result = _compile_and_run()
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'PVD_IRQ_PASS fixture_only=1' in result.stdout


@pytest.mark.parametrize('mutation', [
    ('cd_dma3', 'memcpy(d->memory->bytes + d->state.madr, staging, bytes);',
     '(void)staging; /* mutant: consume without copying */'),
    ('mips_formatter', '!selected.deliver_event(selected.userdata,cpu.r[4],cpu.r[5])',
     '!1 /* mutant: fabricated service acceptance */'),
    ('mips_formatter', 'accepted = instruction == MIPS_JR(31) && cpu.r[31] == expected;',
     'expected = cpu.r[31]; accepted = instruction == MIPS_JR(31);'),
    ('mips_formatter', '!cd_irq_ram_word(memory,slots[i][0],slots[i][1])',
     '0 && !cd_irq_ram_word(memory,slots[i][0],slots[i][1])'),
], ids=['missing_ram_copy','fake_event_success','wrong_saved_return','missing_dma_pointer_guard'])
def test_pvd_irq_mutants_rejected(mutation):
    result = _compile_and_run(mutation)
    assert result.returncode != 0, result.stdout + result.stderr
