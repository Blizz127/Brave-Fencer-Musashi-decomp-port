"""An interrupt return requires the installed CPU, checked devices and live token."""
import ctypes
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class GuestIrqTests(unittest.TestCase):
    def test_legacy_direct_apis_refuse_without_side_effects(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'legacy.so'
            subprocess.run(['cc', '-std=c11', '-shared', '-fPIC', '-I', str(ROOT / 'pc_port/include'),
                            '-I', str(ROOT / 'include'), str(ROOT / 'pc_port/irq_direct_compat.c'),
                            '-o', str(path)], check=True, timeout=60)
            lib = ctypes.CDLL(str(path))
            memory = (ctypes.c_uint8 * (0x200000 + 1024))()
            ctypes.memset(memory, 0x5a, ctypes.sizeof(memory))
            original = bytes(memory)
            for symbol, argcount in (('800427f4', 2), ('80042ce8', 3), ('80042e08', 4)):
                with self.subTest(symbol=symbol):
                    call = getattr(lib, 'musashi_boot_call_' + symbol)
                    call.argtypes = [ctypes.c_void_p] * argcount
                    self.assertEqual(call(memory, *([None] * (argcount - 1))), 0)
                    self.assertEqual(bytes(memory), original)

    def test_consumed_real_irq_frame(self):
        path = Path(os.environ.get('MUSASHI_CODE_IMAGE',
                                   ROOT / 'extracted/disc/files/SLUS_007.26')).resolve()
        if not path.is_file():
            self.skipTest('Requires a local copy of the registered US disc executable')
        with tempfile.TemporaryDirectory() as temp:
            exe = Path(temp) / 'guest-irq'
            flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'sdl2'], text=True))
            port = ROOT / 'pc_port'
            sources = ('mips_formatter', 'boot_memory', 'irq_controller', 'dma_controller', 'cpu_status',
                       'irq_scheduler', 'spu_cd_audio', 'startup_bindings', 'event_bindings',
                       'bios_events', 'bios_exception', 'bios_kernel', 'callback_bindings',
                       'gpu_controller', 'gpu_dma2', 'scanline_timer', 'cd_dma3')
            card = ('player_card', 'card_vector', 'card_service', 'card_image', 'card_host_storage')
            subprocess.run(['cc', '-std=c11', '-O1', '-ffunction-sections', '-fdata-sections',
                            '-DMUSASHI_LINKED_BIOS_EXCEPTION=1', '-DMUSASHI_NATIVE_MEMSET_BINDING=1',
                            '-DMUSASHI_NATIVE_TABLE_CLEAR_BINDING=1', '-Wl,--gc-sections',
                            '-I', ROOT / 'include', '-I', port / 'include', '-I', port / 'platform',
                            '-I', ROOT / 'src/shared', ROOT / 'tests/guest_irq_probe.c',
                            *(port / (s + '.c') for s in sources),
                            *(port / 'card' / (s + '.c') for s in card),
                            port / 'platform/bfm_plat_storage.c',
                            *(ROOT / 'src/main' / (s + '.c') for s in ('80016714', '80029044', '8002906c')),
                            *flags, '-lcrypto', '-lm', '-o', exe], check=True, timeout=180)
            # Source and IRQ words must come from this machine's mapped user image.
            runtime_env = os.environ.copy()
            runtime_env.pop('MUSASHI_CODE_IMAGE', None)
            runtime_env.pop('MUSASHI_CODE_IMAGE_MUTANT', None)
            for mode in ('vblank', 'dma', 'both', 'code', 'record', 'wrongslot', 'callback', 'copy', 'owner',
                         'card', 'active', 'soundless', 'card-code', 'sound-code', 'slotswap', 'voice-range', 'title-fade', 'title-fade-code',
                         'countdown-positive', 'countdown-final', 'countdown-zero', 'countdown-wrap', 'countdown-code',
                         'fade-limit', 'fade-equal', 'fade-wrap', 'fade-limit-level-code', 'fade-limit-mode-code',
                         'fade-za-rise', 'fade-rise-clamp', 'fade-rise-exact', 'fade-rise-wrap',
                         'fade-equal-wrap', 'fade-rise-zero-step', 'fade-disabled', 'fade-step-on-read',
                         'fade-step-after-read', 'fade-clock-store-live', 'fade-clock-clamp-images',
                         'fade-clock-flags-return', 'fade-clock-step-refused',
                         'draw-complete', 'draw-timer', 'draw-lease', 'draw-owner', 'draw-clock',
                         'draw-image', 'draw-queue', 'draw-binding', 'draw-frame-slot',
                         'draw-foreign-gpu', 'draw-foreign-timer', 'draw-stale', 'draw-unbound',
                         'draw-replacement', 'draw-pending', 'draw-pending-next', 'draw-pending-wrap',
                         'draw-pending-operation', 'draw-pending-argument', 'draw-pending-unaligned',
                         'draw-pending-multiple', 'draw-frame-zero', 'draw-pending-frame-zero',
                         'draw-frame-out-of-range', 'draw-upload', 'draw-upload-active', 'draw-upload-foreign',
                         'draw-upload-other-irq', 'cd3-complete', 'cd3-next', 'cd3-slot',
                         'cd3-image', 'cd3-other', 'cd3-active', 'cd3-missing'):
                with self.subTest(mode=mode):
                    subprocess.run([exe, path, mode], env=runtime_env, check=True, timeout=10)
            for mode in ('complete', 'owner', 'handle', 'handler', 'event', 'file'):
                with self.subTest(card_request=mode):
                    directory = Path(temp) / mode
                    subprocess.run([exe, path, 'request-' + mode, directory], cwd=temp,
                                   env=runtime_env, check=True, timeout=10)
                    self.assertEqual((directory / 'bfm_card0.mcd').stat().st_size, 128 * 1024)
