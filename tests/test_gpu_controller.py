"""Checked initial GP0/GP1 frontend with an explicit renderer backend."""
import ctypes as C
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
GPUREAD = 0x1F801810
GPUSTAT = 0x1F801814
UINT32 = C.c_uint32

RESET = C.CFUNCTYPE(C.c_int, C.c_void_p)
DRAW_MODE = C.CFUNCTYPE(C.c_int, C.c_void_p, UINT32)
DISPLAY_ENABLE = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_int)
CLEAR_FIFO = C.CFUNCTYPE(C.c_int, C.c_void_p)
READY = C.CFUNCTYPE(C.c_int, C.c_void_p)
ENVIRONMENT = C.CFUNCTYPE(C.c_int, C.c_void_p, UINT32)
DISPLAY = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_void_p)


class DisplayState(C.Structure):
    _fields_ = [
        ("origin", UINT32), ("horizontal", UINT32), ("vertical", UINT32),
        ("mode", UINT32), ("x", C.c_uint16), ("y", C.c_uint16),
        ("width", C.c_uint16), ("height", C.c_uint16),
        ("h_start", C.c_uint16), ("h_end", C.c_uint16),
        ("v_start", C.c_uint16), ("v_end", C.c_uint16),
        ("dot_divisor", C.c_uint8),
    ]


class Fill(C.Structure):
    _fields_ = [("command", UINT32), ("xy", UINT32), ("wh", UINT32),
                ("x", C.c_uint16), ("y", C.c_uint16),
                ("width", C.c_uint16), ("height", C.c_uint16),
                ("color", C.c_uint16)]


FILL = C.CFUNCTYPE(C.c_int, C.c_void_p, C.POINTER(Fill))
STORE = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint16, C.c_uint16, C.c_uint16)
READ = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_uint16, C.c_uint16, C.POINTER(C.c_uint16))


class Backend(C.Structure):
    _fields_ = [("userdata", C.c_void_p), ("reset", RESET),
                ("draw_mode", DRAW_MODE), ("display_enable", DISPLAY_ENABLE),
                ("clear_fifo", CLEAR_FIFO), ("ready", READY),
                ("environment", ENVIRONMENT), ("display", DISPLAY),
                ("fill_vram", FILL), ("store_vram", STORE), ("read_vram", READ)]


class Controller(C.Structure):
    _fields_ = [("backend", Backend), ("read_latch", UINT32),
                ("draw_mode", UINT32), ("dma_direction", UINT32),
                ("display_disabled", C.c_int), ("faulted", C.c_int),
                ("executing", C.c_int), ("vblank_parity", UINT32),
                ("texture_window", UINT32), ("drawing_area_start", UINT32),
                ("drawing_area_end", UINT32), ("drawing_offset", UINT32),
                ("mask_flags", UINT32), ("accepted_gp0_words", C.c_uint64),
                ("display", DisplayState), ("fill_command", UINT32), ("fill_xy", UINT32),
                ("fill_words", C.c_uint8), ("completed_fills", C.c_uint64),
                ("filled_pixels", C.c_uint64), ("last_fill", Fill),
                ("store_command", UINT32), ("store_xy", UINT32),
                ("store_wh", UINT32), ("store_x", C.c_uint16),
                ("store_y", C.c_uint16), ("store_w", C.c_uint16),
                ("store_h", C.c_uint16), ("store_px", C.c_uint16),
                ("store_py", C.c_uint16), ("store_remaining", UINT32),
                ("store_phase", C.c_uint8), ("stored_pixels", C.c_uint64),
                ("copy_command", UINT32), ("copy_src", UINT32),
                ("copy_dst", UINT32), ("copy_words", C.c_uint8),
                ("copied_pixels", C.c_uint64), ("completed_copies", C.c_uint64),
                ("prim_needed", C.c_uint8), ("prim_got", C.c_uint8),
                ("prim_data", UINT32 * 12),
                ("tap", C.c_void_p), ("tap_userdata", C.c_void_p),
                ("raster_disabled", C.c_int), ("drawn_pixels", C.c_uint64),
                ("semi", C.c_uint8), ("semi_mode", C.c_uint8),
                ("semi_textured", C.c_uint8),
                ("poly_active", C.c_uint8), ("poly_gouraud", C.c_uint8),
                ("poly_words", UINT32), ("poly_vertices", UINT32),
                ("poly_command", UINT32), ("poly_color", UINT32),
                ("poly_prev_color", UINT32),
                ("poly_x", C.c_int32), ("poly_y", C.c_int32)]


class GpuControllerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        temp = tempfile.TemporaryDirectory(prefix="musashi-gpu-controller-")
        cls.addClassCleanup(temp.cleanup)
        library = Path(temp.name) / "gpu_controller.so"
        subprocess.run([
            "cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-shared", "-fPIC",
            "-I", str(ROOT / "pc_port/include"),
            str(ROOT / "pc_port/gpu_controller.c"), "-o", str(library),
        ], check=True, stdin=subprocess.DEVNULL, timeout=30)
        cls.lib = C.CDLL(str(library))
        cls.lib.musashi_gpu_controller_init.argtypes = [
            C.POINTER(Controller), C.POINTER(Backend)]
        cls.lib.musashi_gpu_controller_read32.argtypes = [
            C.POINTER(Controller), UINT32, C.POINTER(UINT32)]
        cls.lib.musashi_gpu_controller_write32.argtypes = [
            C.POINTER(Controller), UINT32, UINT32]
        cls.lib.musashi_gpu_controller_vblank.argtypes = [
            C.POINTER(Controller)]
        cls.lib.musashi_gpu_controller_get_store_image.argtypes = [
            C.POINTER(Controller),
            C.POINTER(C.c_uint16), C.POINTER(C.c_uint16),
            C.POINTER(C.c_uint16), C.POINTER(C.c_uint16),
            C.POINTER(UINT32),
        ]
        cls.lib.musashi_gpu_controller_get_store_image.restype = C.c_int
        cls.lib.musashi_gpu_controller_store_image_phase.argtypes = [
            C.POINTER(Controller)]
        cls.lib.musashi_gpu_controller_store_image_phase.restype = C.c_int
        cls.lib.musashi_gpu_controller_store_image_remaining_words.argtypes = [
            C.POINTER(Controller)]
        cls.lib.musashi_gpu_controller_store_image_remaining_words.restype = UINT32
        cls.lib.musashi_gpu_controller_read_store_image_buffer.argtypes = [
            C.POINTER(Controller),
            C.POINTER(UINT32), UINT32, C.POINTER(UINT32),
        ]
        cls.lib.musashi_gpu_controller_read_store_image_buffer.restype = C.c_int

    def setUp(self):
        self.controller = Controller()
        self.ready_value = 1
        self.calls = []

        def reset(_):
            self.calls.append(("reset",))
            return 1

        def draw_mode(_, word):
            self.calls.append(("draw_mode", word))
            return 1

        def display_enable(_, enabled):
            self.calls.append(("display_enable", enabled))
            return 1

        def clear_fifo(_):
            self.calls.append(("clear_fifo",))
            return 1

        def ready(_):
            self.calls.append(("ready",))
            return self.ready_value

        def environment(_, word):
            self.calls.append(("environment", word))
            return 1

        def display(_, candidate):
            state = C.cast(candidate, C.POINTER(DisplayState)).contents
            self.calls.append(("display", state.mode, state.width, state.height))
            return 1

        self.callbacks = (RESET(reset), DRAW_MODE(draw_mode),
                          DISPLAY_ENABLE(display_enable), CLEAR_FIFO(clear_fifo),
                          READY(ready), ENVIRONMENT(environment), DISPLAY(display))
        self.backend = Backend(None, *self.callbacks)
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(self.controller), C.byref(self.backend)), 1)

    def test_gp0_09_is_one_ready_gated_noop_between_environment_commands(self):
        self.assertEqual(self.write(0xE1000125), 1)
        before = self.controller.accepted_gp0_words
        self.ready_value = 0
        self.assertEqual(self.write(0x09543210), 0)
        self.assertEqual(self.controller.accepted_gp0_words, before)
        self.ready_value = 1
        self.assertEqual(self.write(0x09543210), 1)
        self.assertEqual(self.controller.accepted_gp0_words, before + 1)
        self.assertEqual(self.controller.draw_mode, 0x125)
        self.assertEqual(self.controller.drawn_pixels, 0)
        self.assertEqual(self.write(0xE200101E), 1)
        self.assertEqual(self.controller.texture_window, 0x101E)

    def test_fill_partial_packet_and_real_descriptor(self):
        fills = []
        def accept(_, candidate):
            f = candidate.contents
            fills.append((f.command, f.xy, f.wh, f.x, f.y,
                          f.width, f.height, f.color))
            return 1
        fill = FILL(accept)
        self.controller.backend.fill_vram = fill
        self.assertEqual(self.write(0x02ff00ff), 1)
        self.assertEqual(self.controller.fill_words, 1)
        self.assertEqual(self.controller.accepted_gp0_words, 1)
        self.assertEqual(fills, [])
        self.assertEqual(self.read()[1] & (1 << 26), 0)
        self.assertNotEqual(self.read()[1] & (1 << 28), 0)
        self.assertEqual(self.write(0xe60003ff), 1)  # data, not E6
        self.assertEqual(self.controller.mask_flags, 0)
        self.assertEqual(self.controller.fill_words, 2)
        self.assertEqual(fills, [])
        self.assertEqual(self.write(0x01ff0011), 1)
        self.assertEqual(fills, [(0x02ff00ff, 0xe60003ff, 0x01ff0011,
                                 1008, 0, 32, 511, 0x7c1f)])
        self.assertEqual(self.controller.fill_words, 0)
        self.assertEqual(self.controller.completed_fills, 1)
        self.assertEqual(self.controller.filled_pixels, 32 * 511)
        self.assertEqual(self.controller.accepted_gp0_words, 3)

    def test_fill_reset_refusal_and_accepted_reentry(self):
        for prefix in (1, 2):
            for reset_word in (0, 0x01000000):
                self.setUp()
                calls = []
                fill = FILL(lambda _, f: calls.append(f.contents.color) or 1)
                self.controller.backend.fill_vram = fill
                self.assertEqual(self.write(0x02ffffff), 1)
                if prefix == 2:
                    self.assertEqual(self.write(0), 1)
                self.assertEqual(self.write(reset_word, GPUSTAT), 1)
                self.assertEqual(self.controller.fill_words, 0)
                self.assertEqual(self.controller.accepted_gp0_words, prefix)
                self.assertEqual(calls, [])
                self.assertEqual(self.write(0xe6000003), 1)
                self.assertEqual(self.controller.mask_flags, 3)
        for accept in (False, True):
            self.setUp()
            calls = []
            def backend(_, f):
                calls.append(f.contents.color)
                if accept:
                    self.assertEqual(self.write(0xe1000000), 0)
                return int(accept)
            fill = FILL(backend)
            self.controller.backend.fill_vram = fill
            self.assertEqual(self.write(0x020000ff), 1)
            self.assertEqual(self.write(0), 1)
            self.assertEqual(self.write(0x00010010), 0)
            self.assertEqual(calls, [31])
            self.assertEqual(self.controller.faulted, 1)
            self.assertEqual(self.controller.accepted_gp0_words, 3 if accept else 2)
            self.assertEqual(self.controller.completed_fills, int(accept))
            self.assertEqual(self.controller.fill_words, 0 if accept else 2)

    def test_fill_busy_counter_and_zero_extent(self):
        calls = []
        fill = FILL(lambda _, f: calls.append(f.contents.color) or 1)
        self.controller.backend.fill_vram = fill
        self.assertEqual(self.write(0x0200ff00), 1)
        self.ready_value = 0
        self.assertEqual(self.write(0xe6000003), 0)
        self.assertEqual(self.controller.fill_words, 1)
        self.assertEqual(self.controller.faulted, 0)
        self.ready_value = 1
        self.assertEqual(self.write(0), 1)
        self.controller.completed_fills = (1 << 64) - 1
        self.assertEqual(self.write(0x00010010), 0)
        self.assertEqual(calls, [])
        self.assertEqual(self.controller.fill_words, 2)
        self.controller.completed_fills = 0
        self.assertEqual(self.write(0x02000400), 1)  # both effective sizeszero
        self.assertEqual(calls, [0x3e0])
        self.assertEqual(self.controller.filled_pixels, 0)
        self.assertEqual(self.controller.completed_fills, 1)
        self.assertEqual(self.controller.accepted_gp0_words, 3)

    def read(self, address=GPUSTAT):
        value = UINT32(0xDEADBEEF)
        accepted = self.lib.musashi_gpu_controller_read32(
            C.byref(self.controller), address, C.byref(value))
        return accepted, value.value

    def write(self, value, address=GPUREAD):
        return self.lib.musashi_gpu_controller_write32(
            C.byref(self.controller), address, value)

    def vblank(self, controller=None):
        target = self.controller if controller is None else controller
        return self.lib.musashi_gpu_controller_vblank(C.byref(target))

    def get_store_image(self, controller=None):
        target = self.controller if controller is None else controller
        x = C.c_uint16()
        y = C.c_uint16()
        w = C.c_uint16()
        h = C.c_uint16()
        total = UINT32()
        ok = self.lib.musashi_gpu_controller_get_store_image(
            C.byref(target), C.byref(x), C.byref(y), C.byref(w), C.byref(h), C.byref(total))
        return ok, x.value, y.value, w.value, h.value, total.value

    def store_image_phase(self, controller=None):
        target = self.controller if controller is None else controller
        return self.lib.musashi_gpu_controller_store_image_phase(C.byref(target))

    def store_image_remaining_words(self, controller=None):
        target = self.controller if controller is None else controller
        return self.lib.musashi_gpu_controller_store_image_remaining_words(C.byref(target))

    def test_init_requires_reset_acceptance_and_starts_guest_reset_state(self):
        self.assertEqual(self.controller.read_latch, 0x400)
        self.assertEqual(self.controller.draw_mode, 0)
        self.assertEqual(self.controller.display_disabled, 1)
        self.assertEqual(self.controller.dma_direction, 0)
        self.assertEqual(self.controller.faulted, 0)
        self.assertEqual(self.controller.vblank_parity, 0)
        self.assertEqual(self.calls, [("reset",)])

        failed = Controller()
        self.ready_value = 0
        self.callbacks[0]  # keep callback tuple alive while replacing backend
        refused = RESET(lambda _: 0)
        bad_backend = Backend(None, refused, self.callbacks[1],
                              self.callbacks[2], self.callbacks[3], self.callbacks[4],
                              self.callbacks[5])
        before = bytes(failed)
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(failed), C.byref(bad_backend)), 0)
        self.assertEqual(bytes(failed), before)
        self.assertEqual(failed.faulted, 0)
        self.assertEqual(failed.executing, 0)
        self.assertEqual(failed.accepted_gp0_words, 0)

    def test_gp1_display_05_to_08_commit_ordered_descriptor_and_status(self):
        self.assertEqual(self.controller.display.origin, 0)
        self.assertEqual(self.controller.display.horizontal, 0x00C00200)
        self.assertEqual(self.controller.display.vertical, 0x00040010)
        self.assertEqual(self.controller.display.width, 256)
        self.assertEqual(self.controller.display.height, 240)
        self.assertEqual(self.controller.display.dot_divisor, 10)
        self.assertEqual(self.write(0x05000000, GPUSTAT), 1)
        self.assertEqual(self.write(0x06C60260, GPUSTAT), 1)
        self.assertEqual(self.controller.display.width, 256)
        self.assertEqual(self.write(0x07040010, GPUSTAT), 1)
        self.assertEqual(self.write(0x08000001, GPUSTAT), 1)
        self.assertEqual(self.controller.display.mode, 1)
        self.assertEqual(self.controller.display.width, 320)
        self.assertEqual(self.controller.display.dot_divisor, 8)
        self.assertEqual(self.calls[-1], ("display", 1, 320, 240))
        accepted, status = self.read(GPUSTAT)
        self.assertEqual(accepted, 1)
        self.assertEqual((status >> 17) & 3, 1)
        self.assertEqual(status & (0xf << 19), 0)

    def test_gp1_display_rejects_unsupported_range_or_mode_transactionally(self):
        before = bytes(self.controller)
        self.assertEqual(self.write(0x07040020, GPUSTAT), 0)
        self.assertEqual(bytes(self.controller), before)
        self.assertEqual(self.write(0x08000008, GPUSTAT), 0)
        self.assertEqual(bytes(self.controller), before)

    def test_interlaced_480_line_scanout_covers_both_fields(self):
        self.assertEqual(self.write(0x06C60260, GPUSTAT), 1)
        for mode, height in ((0x03, 240), (0x07, 240), (0x23, 240), (0x27, 480), (0x01, 240)):
            self.assertEqual(self.write(0x08000000 | mode, GPUSTAT), 1)
            self.assertEqual(self.controller.display.height, height, hex(mode))
            self.assertEqual(self.calls[-1][3], height)

    def test_gp1_display_mode_map_and_status_bits_for_all_byte_values(self):
        supported = {}
        for mode in range(256):
            if mode & 0x98:
                continue
            if (mode & 0x40) and (mode & 3):
                continue
            if mode & 0x40:
                supported[mode] = 7
            else:
                supported[mode] = {0: 10, 1: 8, 2: 5, 3: 4}[mode & 3]
        for mode in range(256):
            before = bytes(self.controller)
            accepted = self.write(0x08000000 | mode, GPUSTAT)
            if mode not in supported:
                self.assertEqual(accepted, 0, hex(mode))
                self.assertEqual(bytes(self.controller), before)
                continue
            self.assertEqual(accepted, 1, hex(mode))
            self.assertEqual(self.controller.display.mode, mode)
            self.assertEqual(self.controller.display.dot_divisor, supported[mode])
            _, status = self.read(GPUSTAT)
            self.assertEqual((status >> 17) & 3, mode & 3)
            self.assertEqual((status >> 19) & 0xf, (mode >> 2) & 0xf)
            self.assertEqual((status >> 16) & 1, (mode >> 6) & 1)

    def test_gp1_display_origin_masks_upper_payload_bits(self):
        self.assertEqual(self.write(0x05081234, GPUSTAT), 1)
        self.assertEqual(self.controller.display.origin, 0x1234)
        self.assertEqual(self.controller.display.x, 0x234)
        self.assertEqual(self.controller.display.y, 0x4)

    def test_gp1_display_horizontal_rounding_and_bounds_refuse_atomically(self):
        self.assertEqual(self.write(0x06C60260, GPUSTAT), 1)
        self.assertEqual(self.write(0x06020002, GPUSTAT), 1)
        self.assertEqual(self.controller.display.width, 4)
        before = bytes(self.controller)
        for payload in (0x00020200, 0x00200200):
            self.assertEqual(self.write(0x06000000 | payload, GPUSTAT), 0)
            self.assertEqual(bytes(self.controller), before)
        self.assertEqual(self.write(0x060C0260, GPUSTAT), 0)
        self.assertEqual(bytes(self.controller), before)

    def test_gp1_display_clear_fifo_preserves_and_reset_restores_descriptor(self):
        for value in (0x05000123, 0x06C60260, 0x07040010, 0x08000001):
            self.assertEqual(self.write(value, GPUSTAT), 1)
        before = bytes(self.controller.display)
        self.assertEqual(self.write(0x01000000, GPUSTAT), 1)
        self.assertEqual(bytes(self.controller.display), before)
        self.assertEqual(self.write(0x00000000, GPUSTAT), 1)
        self.assertEqual(self.controller.display.origin, 0)
        self.assertEqual(self.controller.display.horizontal, 0x00C00200)
        self.assertEqual(self.controller.display.width, 256)

    def test_gp1_display_missing_capability_refuses_without_mutation(self):
        fresh = Controller()
        backend = Backend(None, self.callbacks[0], self.callbacks[1],
                          self.callbacks[2], self.callbacks[3], self.callbacks[4],
                          self.callbacks[5])
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(fresh), C.byref(backend)), 1)
        before = bytes(fresh)
        self.assertEqual(self.lib.musashi_gpu_controller_write32(
            C.byref(fresh), GPUSTAT, 0x05000123), 0)
        self.assertEqual(bytes(fresh), before)

    def test_gp1_display_provider_refusal_preserves_frontend_descriptor(self):
        fresh = Controller()

        def refuse(_, candidate):
            return 0

        refused = DISPLAY(refuse)
        backend = Backend(None, self.callbacks[0], self.callbacks[1],
                          self.callbacks[2], self.callbacks[3], self.callbacks[4],
                          self.callbacks[5], refused)
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(fresh), C.byref(backend)), 1)
        before = bytes(fresh.display)
        self.assertEqual(self.lib.musashi_gpu_controller_write32(
            C.byref(fresh), GPUSTAT, 0x05000123), 0)
        self.assertEqual(bytes(fresh.display), before)
        self.assertEqual(fresh.faulted, 1)

    def test_gp1_display_ready_reentry_refuses_before_provider(self):
        fresh = Controller()
        nested = []

        def reentrant_ready(_):
            nested.append(self.lib.musashi_gpu_controller_write32(
                C.byref(fresh), GPUSTAT, 0x05000123))
            return 1

        callback = READY(reentrant_ready)
        backend = Backend(None, self.callbacks[0], self.callbacks[1],
                          self.callbacks[2], self.callbacks[3], callback,
                          self.callbacks[5], self.callbacks[6])
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(fresh), C.byref(backend)), 1)
        before = bytes(fresh.display)
        self.assertEqual(self.lib.musashi_gpu_controller_write32(
            C.byref(fresh), GPUSTAT, 0x05000123), 0)
        self.assertEqual(nested, [0])
        self.assertEqual(bytes(fresh.display), before)
        self.assertEqual(fresh.faulted, 1)

    def test_gp1_display_provider_reentry_retains_accepted_descriptor_and_fault(self):
        fresh = Controller()
        nested = []

        def reentrant_display(_, candidate):
            nested.append(self.lib.musashi_gpu_controller_write32(
                C.byref(fresh), GPUSTAT, 0x05000123))
            return 1

        callback = DISPLAY(reentrant_display)
        backend = Backend(None, self.callbacks[0], self.callbacks[1],
                          self.callbacks[2], self.callbacks[3], self.callbacks[4],
                          self.callbacks[5], callback)
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(fresh), C.byref(backend)), 1)
        self.assertEqual(self.lib.musashi_gpu_controller_write32(
            C.byref(fresh), GPUSTAT, 0x05000123), 0)
        self.assertEqual(nested, [0])
        self.assertEqual(fresh.display.origin, 0x123)
        self.assertEqual(fresh.faulted, 1)

    def test_constructor_reset_reentry_refuses_publication(self):
        fresh = Controller()
        holder = {}
        nested = []

        def nested_reset(_):
            nested.append(self.lib.musashi_gpu_controller_init(
                C.byref(fresh), C.byref(holder["backend"])))
            return 1

        reset_callback = RESET(nested_reset)
        holder["backend"] = Backend(None, reset_callback, self.callbacks[1],
                                     self.callbacks[2], self.callbacks[3],
                                     self.callbacks[4], self.callbacks[5])
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(fresh), C.byref(holder["backend"])), 0)
        self.assertEqual(nested, [0])
        self.assertEqual(fresh.faulted, 1)
        self.assertEqual(fresh.executing, 0)
        self.assertEqual(fresh.accepted_gp0_words, 0)

    def test_gpu_read_latch_and_info_index_seven_leave_400_unchanged(self):
        self.assertEqual(self.read(GPUREAD), (1, 0x400))
        self.assertEqual(self.write(0x10000007, GPUSTAT), 1)
        self.assertEqual(self.calls, [("reset",)])
        self.assertEqual(self.read(GPUREAD), (1, 0x400))
        self.assertEqual(self.calls[-1], ("reset",))

    def test_gp0_nop_does_not_call_renderer_or_reset(self):
        before = bytes(self.controller)
        self.assertEqual(self.write(0x0007924C), 1)
        self.assertNotEqual(bytes(self.controller), before)
        self.assertEqual(self.controller.accepted_gp0_words, 1)
        self.assertEqual(self.calls, [("reset",), ("ready",)])

    def test_gp0_draw_mode_uses_backend_and_maps_only_supported_status_bits(self):
        word = 0xE1F23ABC
        self.assertEqual(self.write(word), 1)
        self.assertEqual(self.calls[-2:], [("ready",), ("draw_mode", word)])
        self.assertEqual(self.controller.draw_mode, word & 0x3FFF)
        accepted, status = self.read()
        self.assertEqual(accepted, 1)
        self.assertEqual(status & 0x7FF, word & 0x7FF)
        self.assertEqual(status & 0x8000, 0x8000 if word & 0x800 else 0)
        self.assertEqual(status & 0x1000, 0)

    def test_environment_words_commit_raw_latches_and_status_mask_bits(self):
        words = (0xE2123456, 0xE30ABCDE, 0xE41ABCDE,
                 0xE5123456, 0xE6000003)
        for word in words:
            self.assertEqual(self.write(word), 1)
        self.assertEqual(self.controller.texture_window, words[0] & 0xFFFFF)
        self.assertEqual(self.controller.drawing_area_start, words[1] & 0xFFFFF)
        self.assertEqual(self.controller.drawing_area_end, words[2] & 0xFFFFF)
        self.assertEqual(self.controller.drawing_offset, words[3] & 0x3FFFFF)
        self.assertEqual(self.controller.mask_flags, 3)
        self.assertEqual(self.controller.accepted_gp0_words, 5)
        self.assertEqual(self.write(0x10000002, GPUSTAT), 1)
        self.assertEqual(self.read(GPUREAD), (1, words[0] & 0xFFFFF))
        self.assertEqual(self.write(0x10000005, GPUSTAT), 1)
        self.assertEqual(self.read(GPUREAD), (1, words[3] & 0x3FFFFF))
        _, status = self.read()
        self.assertEqual(status & ((1 << 11) | (1 << 12)),
                         (1 << 11) | (1 << 12))

    def test_e2_preserves_noncontiguous_masks_and_offsets_outside_mask(self):
        raw = ((0b10101) | (0b01010 << 5) |
               (0b11100 << 10) | (0b00011 << 15))
        self.assertEqual(self.write(0xE2000000 | raw), 1)
        self.assertEqual(self.controller.texture_window, raw)
        self.assertEqual(self.write(0x10000002, GPUSTAT), 1)
        self.assertEqual(self.read(GPUREAD), (1, raw))

    def test_e5_exercises_every_independent_signed_11_bit_component(self):
        for x in range(2048):
            self.assertEqual(self.write(0xE5000000 | x), 1)
            self.assertEqual(self.controller.drawing_offset, x)
        for y in range(2048):
            self.assertEqual(self.write(0xE5000000 | (y << 11)), 1)
            self.assertEqual(self.controller.drawing_offset, y << 11)
        self.assertEqual((2047 if 2047 < 1024 else 2047 - 2048), -1)
        self.assertEqual((1023 if 1023 < 1024 else 1023 - 2048), 1023)

    def test_e6_all_force_and_check_flag_combinations_map_gpu_status(self):
        for flags in range(4):
            self.assertEqual(self.write(0xE6000000 | flags), 1)
            _, status = self.read()
            self.assertEqual((status >> 11) & 3, flags)

    def test_e3_e4_raw_queries_keep_twenty_bits_and_latch_until_next_query(self):
        start = 0x80000 | (7 << 10) | 2
        end = (511 << 10) | 1023
        self.assertEqual(self.write(0xE3000000 | start), 1)
        self.assertEqual(self.write(0xE4000000 | end), 1)
        self.assertEqual(self.write(0x10000003, GPUSTAT), 1)
        self.assertEqual(self.read(GPUREAD), (1, start))
        self.assertEqual(self.write(0xE3000000 | 0x123), 1)
        self.assertEqual(self.read(GPUREAD), (1, start))
        self.assertEqual(self.write(0x10000004, GPUSTAT), 1)
        self.assertEqual(self.read(GPUREAD), (1, end))

    def test_gp1_reset_preserves_accepted_count_but_clears_environment(self):
        self.assertEqual(self.write(0xE2123456), 1)
        self.assertEqual(self.write(0x00000000, GPUSTAT), 1)
        self.assertEqual(self.controller.accepted_gp0_words, 1)
        self.assertEqual(self.controller.texture_window, 0)
        self.assertEqual(self.controller.mask_flags, 0)

    def test_environment_without_backend_callback_refuses_without_mutation(self):
        fresh = Controller()
        backend = Backend(None, self.callbacks[0], self.callbacks[1],
                          self.callbacks[2], self.callbacks[3], self.callbacks[4],
                          ENVIRONMENT())
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(fresh), C.byref(backend)), 1)
        before = bytes(fresh)
        self.assertEqual(self.lib.musashi_gpu_controller_write32(
            C.byref(fresh), GPUREAD, 0xE2000000), 0)
        self.assertEqual(bytes(fresh), before)

    def test_environment_backend_refusal_is_sticky_and_preserves_raw_register(self):
        fresh = Controller()

        def refuse_environment(_, word):
            self.calls.append(("environment_refused", word))
            return 0

        callback = ENVIRONMENT(refuse_environment)
        backend = Backend(None, self.callbacks[0], self.callbacks[1],
                          self.callbacks[2], self.callbacks[3], self.callbacks[4],
                          callback)
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(fresh), C.byref(backend)), 1)
        self.assertEqual(self.lib.musashi_gpu_controller_write32(
            C.byref(fresh), GPUREAD, 0xE2000001), 0)
        self.assertEqual(fresh.faulted, 1)
        self.assertEqual(fresh.texture_window, 0)
        self.assertEqual(fresh.accepted_gp0_words, 0)

    def test_environment_backend_reentry_commits_word_but_faults_outer_call(self):
        fresh = Controller()
        nested = []

        def environment(_, word):
            nested.append(self.lib.musashi_gpu_controller_write32(
                C.byref(fresh), GPUREAD, 0xE1000000))
            return 1

        callback = ENVIRONMENT(environment)
        backend = Backend(None, self.callbacks[0], self.callbacks[1],
                          self.callbacks[2], self.callbacks[3], self.callbacks[4],
                          callback)
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(fresh), C.byref(backend)), 1)
        self.assertEqual(self.lib.musashi_gpu_controller_write32(
            C.byref(fresh), GPUREAD, 0xE2000001), 0)
        self.assertEqual(nested, [0])
        self.assertEqual(fresh.faulted, 1)
        self.assertEqual(fresh.texture_window, 1)
        self.assertEqual(fresh.accepted_gp0_words, 1)

    def test_ready_reentry_refuses_without_accepting_gp0_word(self):
        fresh = Controller()
        nested = []

        def ready_reentrant(_):
            nested.append(self.lib.musashi_gpu_controller_write32(
                C.byref(fresh), GPUREAD, 0xE1000000))
            return 1

        callback = READY(ready_reentrant)
        backend = Backend(None, self.callbacks[0], self.callbacks[1],
                          self.callbacks[2], self.callbacks[3], callback,
                          self.callbacks[5])
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(fresh), C.byref(backend)), 1)
        self.assertEqual(self.lib.musashi_gpu_controller_write32(
            C.byref(fresh), GPUREAD, 0xE1000000), 0)
        self.assertEqual(nested, [0])
        self.assertEqual(fresh.faulted, 1)
        self.assertEqual(fresh.accepted_gp0_words, 0)

    def test_gp0_counter_overflow_refuses_before_backend_callback(self):
        self.controller.accepted_gp0_words = (1 << 64) - 1
        before_calls = list(self.calls)
        before = bytes(self.controller)
        self.assertEqual(self.write(0xE1000000), 0)
        self.assertEqual(self.calls, before_calls)
        self.assertEqual(bytes(self.controller), before)

    def test_busy_gp0_refuses_without_command_effects_and_ready_bits_clear(self):
        self.ready_value = 0
        before = bytes(self.controller)
        self.assertEqual(self.write(0xE1001234), 0)
        self.assertEqual(bytes(self.controller), before)
        accepted, status = self.read()
        self.assertEqual(accepted, 1)
        self.assertEqual(status & ((1 << 26) | (1 << 28)), 0)

    def test_status_maps_display_dma_and_backend_ready_state(self):
        self.assertEqual(self.write(0x03000000, GPUSTAT), 1)
        self.assertEqual(self.controller.display_disabled, 0)
        self.assertEqual(self.write(0x04000002, GPUSTAT), 1)
        _, status = self.read()
        self.assertEqual(status & (1 << 23), 0)
        self.assertEqual((status >> 29) & 3, 2)
        self.assertEqual(status & (1 << 25), 1 << 25)
        self.ready_value = 0
        _, status = self.read()
        self.assertEqual(status & ((1 << 26) | (1 << 28) | (1 << 25)), 0)
        self.assertEqual(self.write(0x03000001, GPUSTAT), 1)
        self.assertEqual(self.controller.display_disabled, 1)
        self.assertEqual(self.calls[-1], ("display_enable", 0))

    def test_gp1_reset_fifo_ack_display_and_dma_controls_are_checked(self):
        self.controller.read_latch = 99
        self.controller.draw_mode = 0x3FFF
        self.controller.display_disabled = 0
        self.controller.dma_direction = 3
        self.assertEqual(self.write(0x00000000, GPUSTAT), 1)
        self.assertEqual(self.controller.read_latch, 0x400)
        self.assertEqual(self.controller.draw_mode, 0)
        self.assertEqual(self.controller.display_disabled, 1)
        self.assertEqual(self.controller.dma_direction, 0)
        self.assertEqual(self.write(0x00000000, GPUSTAT), 1)
        self.assertEqual(self.write(0x00000000, GPUSTAT), 1)
        self.assertEqual(self.write(0x00000000, GPUSTAT), 1)

    def test_vblank_toggles_gpu_status_odd_even_bit_once_per_edge(self):
        self.assertEqual(self.read()[1] & (1 << 31), 0)
        self.assertEqual(self.vblank(), 1)
        self.assertEqual(self.controller.vblank_parity, 1)
        self.assertEqual(self.read()[1] & (1 << 31), 1 << 31)
        self.assertEqual(self.vblank(), 1)
        self.assertEqual(self.controller.vblank_parity, 0)
        self.assertEqual(self.read()[1] & (1 << 31), 0)

    def test_two_coalesced_source_edges_toggle_parity_twice(self):
        self.assertEqual(self.vblank(), 1)
        self.assertEqual(self.vblank(), 1)
        self.assertEqual(self.controller.vblank_parity, 0)
        self.assertEqual(self.read()[1] & (1 << 31), 0)

    def test_vblank_does_not_require_ready_backend_for_blank_edge(self):
        self.ready_value = 0
        self.assertEqual(self.calls, [("reset",)])
        self.assertEqual(self.vblank(), 1)
        self.assertEqual(self.calls, [("reset",), ("ready",)])
        accepted, status = self.read()
        self.assertEqual(accepted, 1)
        self.assertEqual(status & (1 << 31), 1 << 31)
        self.assertEqual(status & ((1 << 26) | (1 << 28) | (1 << 25)), 0)

    def test_gp1_reset_clears_vblank_parity_to_source_reset_value(self):
        self.assertEqual(self.vblank(), 1)
        self.assertEqual(self.write(0x00000000, GPUSTAT), 1)
        self.assertEqual(self.controller.vblank_parity, 0)
        accepted, status = self.read()
        self.assertEqual(accepted, 1)
        self.assertEqual(status, 0x14802000)

    def test_vblank_refuses_invalid_owner_without_moving_prior_parity(self):
        self.controller.vblank_parity = 1
        self.controller.faulted = 1
        self.assertEqual(self.vblank(), 0)
        self.assertEqual(self.controller.vblank_parity, 1)
        self.controller.faulted = 0
        self.controller.executing = 1
        self.assertEqual(self.vblank(), 0)
        self.assertEqual(self.controller.vblank_parity, 1)

        fresh = Controller()
        self.assertEqual(self.vblank(fresh), 0)
        self.assertEqual(fresh.vblank_parity, 0)

        def refuse_ready(_):
            self.calls.append(("ready_refused",))
            return -1

        refused_ready = READY(refuse_ready)
        backend = Backend(None, self.callbacks[0], self.callbacks[1],
                          self.callbacks[2], self.callbacks[3], refused_ready)
        refused = Controller()
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(refused), C.byref(backend)), 1)
        refused.vblank_parity = 1
        self.assertEqual(self.lib.musashi_gpu_controller_vblank(
            C.byref(refused)), 0)
        self.assertEqual(refused.faulted, 1)
        self.assertEqual(refused.vblank_parity, 1)
        output = UINT32(0xDEADBEEF)
        self.assertEqual(self.lib.musashi_gpu_controller_read32(
            C.byref(refused), GPUSTAT, C.byref(output)), 0)
        self.assertEqual(refused.faulted, 1)
        self.assertEqual(output.value, 0xDEADBEEF)
        self.assertEqual(refused.vblank_parity, 1)

    def test_backend_refusal_faults_controller_and_blocks_later_access(self):
        def refuse_draw(_, word):
            self.calls.append(("draw_refused", word))
            return 0

        refusal = DRAW_MODE(refuse_draw)
        backend = Backend(None, self.callbacks[0], refusal, self.callbacks[2],
                          self.callbacks[3], self.callbacks[4])
        fresh = Controller()
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(fresh), C.byref(backend)), 1)
        self.assertEqual(self.lib.musashi_gpu_controller_write32(
            C.byref(fresh), GPUREAD, 0xE1000000), 0)
        self.assertEqual(fresh.faulted, 1)
        output = UINT32(0xDEADBEEF)
        self.assertEqual(self.lib.musashi_gpu_controller_read32(
            C.byref(fresh), GPUSTAT, C.byref(output)), 0)
        self.assertEqual(output.value, 0xDEADBEEF)

    def test_unsupported_commands_and_addresses_refuse_without_effects(self):
        before = bytes(self.controller)
        for value, address in ((0x1F000000, GPUREAD),
                               (0x09000000, GPUSTAT)):
            self.assertEqual(self.write(value, address), 0)
            self.assertEqual(bytes(self.controller), before)
        self.assertEqual(self.write(0x06000001, GPUSTAT), 0)
        self.assertEqual(bytes(self.controller), before)
        output = UINT32(0xDEADBEEF)
        self.assertEqual(self.lib.musashi_gpu_controller_read32(
            C.byref(self.controller), 0x1F801818, C.byref(output)), 0)
        self.assertEqual(output.value, 0xDEADBEEF)
        self.assertEqual(self.write(0, 0x1F801818), 0)

    def test_uninitialized_controller_refuses_without_calling_unbound_backend(self):
        fresh = Controller()
        output = UINT32(0xDEADBEEF)
        self.assertEqual(self.lib.musashi_gpu_controller_read32(
            C.byref(fresh), GPUSTAT, C.byref(output)), 0)
        self.assertEqual(output.value, 0xDEADBEEF)
        self.assertEqual(self.lib.musashi_gpu_controller_write32(
            C.byref(fresh), GPUSTAT, 0x02000000), 0)

    def test_backend_callbacks_cannot_reenter_writes_and_see_ready_during_execution(self):
        fresh = Controller()
        nested = []

        def nested_draw(_, word):
            fresh.dma_direction = 1
            nested.append(self.lib.musashi_gpu_controller_write32(
                C.byref(fresh), GPUREAD, word))
            output = UINT32(0xDEADBEEF)
            self.assertEqual(self.lib.musashi_gpu_controller_read32(
                C.byref(fresh), GPUSTAT, C.byref(output)), 0)
            self.assertEqual(output.value, 0xDEADBEEF)
            return 1

        callback = DRAW_MODE(nested_draw)
        backend = Backend(None, self.callbacks[0], callback, self.callbacks[2],
                          self.callbacks[3], self.callbacks[4], self.callbacks[5])
        self.assertEqual(self.lib.musashi_gpu_controller_init(
            C.byref(fresh), C.byref(backend)), 1)
        self.assertEqual(self.lib.musashi_gpu_controller_write32(
            C.byref(fresh), GPUREAD, 0xE1001234), 0)
        self.assertEqual(nested, [0])
        self.assertEqual(fresh.faulted, 1)


    def test_gp0_clear_cache_is_nop_like_ready_word(self):
        before_calls = list(self.calls)
        self.assertEqual(self.write(0x01000000), 1)
        self.assertEqual(self.controller.accepted_gp0_words, 1)
        self.assertEqual(self.controller.store_phase, 0)
        self.assertEqual(self.calls, before_calls + [("ready",)])

    def test_gp0_cpu_to_vram_writes_each_texel_through_store_vram(self):
        stored = []

        def accept(_, x, y, pixel):
            stored.append((x, y, pixel))
            return 1

        self.store_cb = STORE(accept)
        self.controller.backend.store_vram = self.store_cb
        self.assertEqual(self.write(0xA0000000), 1)
        self.assertEqual(self.controller.store_phase, 1)
        self.assertEqual(self.write(0x00020010), 1)  # y=2 x=16
        self.assertEqual(self.write(0x00020002), 1)  # h=2 w=2
        self.assertEqual(self.controller.store_remaining, 4)
        self.assertEqual(stored, [])
        self.assertEqual(self.write(0x22221111), 1)
        self.assertEqual(stored, [(16, 2, 0x1111), (17, 2, 0x2222)])
        self.assertEqual(self.write(0x44443333), 1)
        self.assertEqual(stored, [
            (16, 2, 0x1111), (17, 2, 0x2222),
            (16, 3, 0x3333), (17, 3, 0x4444)])
        self.assertEqual(self.controller.store_phase, 0)
        self.assertEqual(self.controller.stored_pixels, 4)
        self.assertEqual(self.controller.accepted_gp0_words, 5)

    def test_gp0_cpu_to_vram_payload_is_not_parsed_as_fill(self):
        stored = []

        def accept(_, x, y, pixel):
            stored.append((x, y, pixel))
            return 1

        self.store_cb = STORE(accept)
        self.controller.backend.store_vram = self.store_cb
        self.assertEqual(self.write(0xA0000000), 1)
        self.assertEqual(self.write(0), 1)
        self.assertEqual(self.write(0x00020002), 1)
        self.assertEqual(self.write(0x02000200), 1)
        self.assertEqual(stored, [(0, 0, 0x0200), (1, 0, 0x0200)])
        self.assertEqual(self.controller.fill_words, 0)
        self.assertEqual(self.controller.store_remaining, 2)
        self.assertEqual(self.write(0xA000A000), 1)
        self.assertEqual(stored, [
            (0, 0, 0x0200), (1, 0, 0x0200),
            (0, 1, 0xA000), (1, 1, 0xA000)])
        self.assertEqual(self.controller.store_phase, 0)

    def test_gp0_poly_f4_rasterizes_through_store_vram(self):
        stored = []

        def accept(_, x, y, pixel):
            stored.append((x, y, pixel))
            return 1

        self.store_cb = STORE(accept)
        self.controller.backend.store_vram = self.store_cb
        # 0x2A is semi-transparent: black back buffer + additive (E1 abr=1).
        self.read_cb = READ(lambda _, x, y, out: out.__setitem__(0, 0) or 1)
        self.controller.backend.read_vram = self.read_cb
        self.assertEqual(self.write(0xE1000020), 1)
        self.assertEqual(self.write(0xE3000000), 1)
        self.assertEqual(self.write(0xE403C13F), 1)  # 319,240
        self.assertEqual(self.write(0x2AFFFFFF), 1)
        self.assertEqual(self.controller.prim_needed, 5)
        self.assertEqual(self.write(0x00000000), 1)
        self.assertEqual(self.write(0x00000008), 1)
        self.assertEqual(self.write(0x00080000), 1)
        self.assertEqual(self.write(0x00080008), 1)
        self.assertEqual(self.controller.prim_needed, 0)
        self.assertGreater(len(stored), 0)
        self.assertTrue(all(p == 0x7FFF for _, _, p in stored))
        self.assertEqual(self.controller.drawn_pixels, len(stored))

    def test_gp0_poly_f3_flat_triangle(self):
        for opcode in (0x20, 0x21, 0x22, 0x23):
            self.setUp()
            stored = []
            self.store_cb = STORE(lambda _, x, y, p: stored.append((x, y, p)) or 1)
            self.controller.backend.store_vram = self.store_cb
            # Semi-transparent variants (command bit 1) read the back buffer.
            # With a black back buffer and additive blending (E1 abr=1),
            # B+F == F, so the colours below hold for every variant.
            back_reads = []
            self.read_cb = READ(lambda _, x, y, out: back_reads.append((x, y))
                                or out.__setitem__(0, 0) or 1)
            self.controller.backend.read_vram = self.read_cb
            if opcode & 2:
                self.assertEqual(self.write(0xE1000020), 1)
            self.assertEqual(self.write(0xE3000000), 1)
            self.assertEqual(self.write(0xE403C13F), 1)  # 319, 240
            cmd = (opcode << 24) | 0x000000F8  # pure red (r=248 -> 31 -> 0x001F)
            self.assertEqual(self.write(cmd), 1)
            self.assertEqual(self.controller.prim_needed, 4)
            self.assertEqual(self.controller.prim_got, 1)
            self.assertEqual(self.write(0x00000000), 1)  # v0 (0, 0)
            self.assertEqual(self.write(0x00000008), 1)  # v1 (8, 0)
            self.assertEqual(self.write(0x00080000), 1)  # v2 (0, 8)
            self.assertEqual(self.controller.prim_needed, 0)
            self.assertGreater(len(stored), 0)
            self.assertTrue(all(p == 0x001F for _, _, p in stored))
            self.assertEqual(self.controller.drawn_pixels, len(stored))

            self.assertEqual(bool(back_reads), bool(opcode & 2))
    def _polyline(self, words):
        stored = []
        self.store_cb = STORE(lambda _, x, y, p: stored.append((x, y, p)) or 1)
        self.controller.backend.store_vram = self.store_cb
        self.assertEqual(self.write(0xE3000000), 1)
        self.assertEqual(self.write(0xE403C13F), 1)
        for w in words:
            self.assertEqual(self.write(w), 1, hex(w))
        return stored

    def _geometry_probe(self, area=(4, 4, 11, 11), offset=(0, 0)):
        writes, reads = {}, []

        def store(_, x, y, pixel):
            writes[x, y] = pixel
            return 1

        def read(_, x, y, out):
            reads.append((x, y))
            out[0] = 0x3DEF
            return 1

        self.store_cb, self.read_cb = STORE(store), READ(read)
        self.controller.backend.store_vram = self.store_cb
        self.controller.backend.read_vram = self.read_cb
        left, top, right, bottom = area
        for word in (0xE3000000 | (top << 10) | left,
                     0xE4000000 | (bottom << 10) | right,
                     0xE5000000 | ((offset[1] & 0x7FF) << 11) | (offset[0] & 0x7FF)):
            self.assertEqual(self.write(word), 1)
        return writes, reads

    def _polygon(self, opcode, positions, high_bits=0, color=0x808080, tpage=0x106):
        words = [(opcode << 24) | color]
        for i, (x, y) in enumerate(positions):
            if i and opcode & 0x10:
                words.append(color)
            words.append(((y & 0x7FF) << 16) | (x & 0x7FF) | high_bits)
            if opcode & 4:
                words.append(tpage << 16 if i == 1 else 0)
        for word in words:
            self.assertEqual(self.write(word), 1, hex(word))

    def test_texture_coordinates_on_clipped_scanlines(self):
        # Independent software GPU readback. Adjacent samples straddle texel
        # boundaries with positive, negative and oblique screen-space slopes.
        cases = [
            (((0, 0), (997, 0), (0, 23)), ((10, 13), (25, 87), (19, 87)),
             498, 0, ((17, 50), (17, 50), (18, 50))),
            (((0, 28), (991, 28), (7, 51)), ((81, 99), (16, 71), (72, 13)),
             525, 28, ((47, 84), (47, 84), (46, 84))),
            (((69, 161), (989, 140), (10, 160)), ((75, 19), (85, 29), (54, 46)),
             310, 155, ((73, 27), (73, 27), (74, 27))),
        ]
        for points, uv, left, y, expected in cases:
            for opcode in (0x25, 0x35):
                with self.subTest(points=points, opcode=opcode):
                    writes, reads = self._geometry_probe(area=(left, y, left + 2, y))
                    words = [(opcode << 24) | 0x808080]
                    for i, ((px, py), (u, v)) in enumerate(zip(points, uv)):
                        if i and opcode & 0x10:
                            words.append(0x808080)
                        words.extend((px | (py << 16),
                                      u | (v << 8) | ((0x11a << 16) if i == 1 else 0)))
                    for word in words:
                        self.assertEqual(self.write(word), 1)
                    self.assertEqual(reads, [(640 + u, 256 + v) for u, v in expected])
                    self.assertEqual(set(writes), {(left + i, y) for i in range(3)})

    def test_horizontal_texture_ramps_keep_signed_fractional_steps(self):
        # A one-texel change over 180 pixels has a signed step of 22/4096.
        # Both directions cross the half-texel boundary between x=93 and 94.
        for opcode in (0x25, 0x2D, 0x35, 0x3D):
            with self.subTest(opcode=opcode):
                _, reads = self._geometry_probe(area=(92, 4, 95, 4))
                points = [(0, 0), (180, 0), (180, 8)]
                if opcode & 8:
                    points = [(0, 0), (180, 0), (0, 8), (180, 8)]
                words = [(opcode << 24) | 0x808080]
                for i, (x, y) in enumerate(points):
                    if i and opcode & 0x10:
                        words.append(0x808080)
                    u, v = (21, 50) if x else (20, 51)
                    page = 0x11A if i == 1 else 0
                    words.extend((x | (y << 16), u | (v << 8) | (page << 16)))
                for word in words:
                    self.assertEqual(self.write(word), 1, hex(word))
                self.assertEqual(reads, [(660, 307), (660, 307), (661, 306), (661, 306)])

    def test_texture_triangle_collapsing_at_middle_scanline(self):
        for opcode in (0x25, 0x35):
            with self.subTest(opcode=opcode):
                writes, reads = self._geometry_probe(area=(0, 0, 512, 129))
                self._polygon(opcode, ((0, 0), (127, 32), (512, 129)))
                self.assertEqual(reads, [])
                self.assertEqual(writes, {})

    def test_polygon_extent_is_checked_before_drawing_area_clip(self):
        # Only a small interior window is observed. The long edges straddle it.
        for opcode in (0x20, 0x25, 0x30, 0x35):
            for width, height, visible in ((1023, 511, True), (1024, 511, False),
                                           (1023, 512, False)):
                with self.subTest(opcode=opcode, width=width, height=height):
                    writes, reads = self._geometry_probe()
                    self.assertEqual(self.write(0xE1000000), 1)
                    self._polygon(opcode, [(-400, -100), (width - 400, -100),
                                           (-400, height - 100)], high_bits=0x9800C000)
                    expected = {(x, y) for x in range(4, 12) for y in range(4, 12)}
                    self.assertEqual(set(writes), expected if visible else set())
                    self.assertEqual(bool(reads), visible and bool(opcode & 4))
                    if opcode & 4:
                        self.assertEqual(self.controller.draw_mode & 0x1FF, 0x106)

    def test_integer_samples_on_slanted_triangle_edges(self):
        # Rows read back by the SCPH-5501 gpuraster 01TRI probe. Sampling
        # at half-pixel centers would add a pixel on the empty top row.
        vertices = [(1, 2), (11, 5), (4, 11)]
        rows = [(2, 4), (2, 7), (2, 10), (3, 9),
                (3, 8), (3, 7), (4, 6), (4, 5)]
        expected = {(x, y) for y, (first, last) in enumerate(rows, 3)
                    for x in range(first, last + 1)}
        for order in (vertices, vertices[::-1]):
            writes, _ = self._geometry_probe(area=(0, 0, 14, 14))
            self._polygon(0x20, order)
            self.assertEqual(set(writes), expected)

    def test_split_square_samples_each_pixel_once(self):
        a, b, c, d = (0, 0), (10, 0), (0, 10), (10, 10)
        expected_points = {(x, y) for x in range(10) for y in range(10)}
        for attributes in (0, 4, 16, 20):
            for semi in (0, 2):
                for packets in (([a, b, c, d],), ([a, b, c], [c, d, b]),
                                ([a, d, b], [d, a, c])):
                    with self.subTest(attributes=attributes, semi=semi, packets=packets):
                        pixels = {}
                        writes = []

                        def store(_, x, y, sample):
                            writes.append((x, y))
                            pixels[x, y] = sample
                            return 1

                        def read(_, x, y, out):
                            # Direct-colour texture page 6, UV=(0,0), STP set.
                            out[0] = 0x8001 if (x, y) == (384, 0) else pixels.get((x, y), 0)
                            return 1

                        self.store_cb, self.read_cb = STORE(store), READ(read)
                        self.controller.backend.store_vram = self.store_cb
                        self.controller.backend.read_vram = self.read_cb
                        self.assertEqual(self.write(0xE3000000), 1)
                        self.assertEqual(self.write(0xE4004010), 1)
                        self.assertEqual(self.write(0xE1000020), 1)  # B+F
                        textured = bool(attributes & 4)
                        for vertices in packets:
                            opcode = 0x20 | attributes | semi | (8 if len(vertices) == 4 else 0)
                            self._polygon(opcode, vertices, color=0x808080 if textured else 8,
                                          tpage=0x126)
                        self.assertEqual(set(writes), expected_points)
                        self.assertEqual(len(writes), 100)
                        self.assertEqual(set(pixels.values()), {0x8001 if textured else 1})

    def test_blended_quad_matches_hardware_color_samples(self):
        # The SCPH-5501 gpuraster 09STM readback used these four backgrounds
        # and a 5-bit foreground of 16 per channel in additive mode.
        back = [0x0000, 0x2108, 0x5EF7, 0x7FFF]
        for opcode in (0x2A, 0x3A, 0x2F):
            with self.subTest(opcode=opcode):
                pixels = {(x, 0): value for x, value in enumerate(back)}

                def read(_, x, y, out):
                    out[0] = 0xC210 if (x, y) == (384, 0) else pixels.get((x, y), 0)
                    return 1

                def store(_, x, y, color):
                    pixels[x, y] = color
                    return 1

                self.store_cb, self.read_cb = STORE(store), READ(read)
                self.controller.backend.store_vram = self.store_cb
                self.controller.backend.read_vram = self.read_cb
                for word in (0xE3000000, 0xE4000404, 0xE1000020):
                    self.assertEqual(self.write(word), 1)
                self._polygon(opcode, [(0, 0), (4, 0), (0, 1), (4, 1)], tpage=0x126)
                expected = [0x4210, 0x6318, 0x7FFF, 0x7FFF]
                self.assertEqual(set(pixels), {(x, 0) for x in range(4)})
                self.assertEqual([pixels[x, 0] for x in range(4)],
                                 [p | (0x8000 if opcode & 4 else 0) for p in expected])

    def test_quad_checks_only_edges_of_its_two_triangles(self):
        # B and C form the common edge. Either distant outer vertex may
        # invalidate only its own triangle, on the opposite side of that edge.
        b, c = (16, 0), (16, 20)
        for opcode in (0x28, 0x2D, 0x38, 0x3D):
            for a, d, left, right in (((-1009, 0), (32, 20), False, True),
                                      ((0, 0), (-1009, 20), True, False),
                                      ((-600, 0), (600, 20), True, True)):
                with self.subTest(opcode=opcode, a=a, d=d):
                    writes, _ = self._geometry_probe(area=(0, 0, 32, 20))
                    self._polygon(opcode, [a, b, c, d])
                    self.assertEqual((12, 5) in writes, left)
                    self.assertEqual((20, 15) in writes, right)

    def test_drawing_offset_keeps_edges_continuous_across_signed_limit(self):
        # X becomes 1008..1040. Its right vertices cross +1023 but the
        # triangle still covers an interior sample at the right edge of VRAM.
        writes, _ = self._geometry_probe(area=(1014, 6, 1015, 7), offset=(512, 0))
        self._polygon(0x20, [(496, 0), (528, 0), (496, 32)])
        self.assertEqual(set(writes), {(x, y) for x in (1014, 1015) for y in (6, 7)})
        # The signed sum -2048+64 must land at 64 in both axes. Exercise
        # a triangle, a line, a textured point sprite and a filled rectangle.
        def position(x, y):
            return ((y & 0x7FF) << 16) | (x & 0x7FF) | 0xA800B000

        for opcode in (0x20, 0x40, 0x68, 0x6D):
            with self.subTest(opcode=opcode):
                writes, _ = self._geometry_probe(area=(64, 64, 64, 64), offset=(-1024, -1024))
                if opcode == 0x20:
                    self._polygon(opcode, [(-962, -962), (-954, -962), (-962, -954)])
                else:
                    self.assertEqual(self.write((opcode << 24) | 0x808080), 1)
                    self.assertEqual(self.write(position(-960, -960)), 1)
                    if opcode == 0x40:
                        self.assertEqual(self.write(position(-957, -960)), 1)
                    elif opcode == 0x6D:
                        self.assertEqual(self.write(0), 1)
                self.assertEqual(set(writes), {(64, 64)})

    def test_gp0_flat_polyline_until_terminator(self):
        # 0x48: colour, then vertices until a 0x5xxx5xxx word.
        stored = self._polyline([0x480000F8, 0x00000000, 0x00000008,
                                 0x00080008, 0x55555555])
        self.assertFalse(self.controller.poly_active)
        pts = {(x, y) for x, y, _ in stored}
        self.assertIn((0, 0), pts); self.assertIn((8, 0), pts); self.assertIn((8, 8), pts)
        self.assertTrue(all(p == 0x001F for _, _, p in stored))
        # The next word is a command again, not a vertex.
        self.assertEqual(self.write(0xE1000000), 1)
        self.assertEqual(self.controller.draw_mode, 0)

    def test_gp0_gouraud_polyline_terminator_in_colour_slot(self):
        # 0x58: c0|cmd, v0, c1, v1, c2, v2, terminator (colour position).
        # A vertex that looks like a terminator is still a vertex.
        # The low eleven bits of each component give the position (0,0).
        stored = self._polyline([0x580000F8, 0x00000000, 0x0000F800, 0x50005000,
                                 0x00F80000, 0x00100010, 0x000000F8, 0x00100000,
                                 0x50005000])
        self.assertFalse(self.controller.poly_active)
        expected = {(i, i) for i in range(17)} | {(x, 16) for x in range(17)}
        self.assertEqual({(x, y) for x, y, _ in stored}, expected)
        self.assertEqual(self.write(0xE1000000), 1)

    def test_gp0_line_clipped_to_drawing_area(self):
        # E3/E4 = (0,0)-(4,4): the (0,2)-(10,2) line keeps x 0..4 only.
        stored = []
        self.store_cb = STORE(lambda _, x, y, p: stored.append((x, y, p)) or 1)
        self.controller.backend.store_vram = self.store_cb
        self.assertEqual(self.write(0xE3000000), 1)
        self.assertEqual(self.write(0xE4001004), 1)
        for w in (0x400000F8, 0x00020000, 0x0002000A):
            self.assertEqual(self.write(w), 1)
        self.assertEqual({(x, y) for x, y, _ in stored}, {(x, 2) for x in range(5)})

    def test_gp0_polyline_reset_by_gp1(self):
        self._polyline([0x480000F8, 0x00000000])
        self.assertTrue(self.controller.poly_active)
        self.assertEqual(self.write(0x01000000, GPUSTAT), 1)
        self.assertFalse(self.controller.poly_active)

    def test_gp0_poly_f4_flat_quad_five_words(self):
        for opcode in (0x28, 0x29, 0x2A, 0x2B):
            self.setUp()
            stored = []
            self.store_cb = STORE(lambda _, x, y, p: stored.append((x, y, p)) or 1)
            self.controller.backend.store_vram = self.store_cb
            # Semi-transparent variants (command bit 1) read the back buffer.
            # With a black back buffer and additive blending (E1 abr=1),
            # B+F == F, so the colours below hold for every variant.
            back_reads = []
            self.read_cb = READ(lambda _, x, y, out: back_reads.append((x, y))
                                or out.__setitem__(0, 0) or 1)
            self.controller.backend.read_vram = self.read_cb
            if opcode & 2:
                self.assertEqual(self.write(0xE1000020), 1)
            self.assertEqual(self.write(0xE3000000), 1)
            self.assertEqual(self.write(0xE403C13F), 1)  # 319, 240
            cmd = (opcode << 24) | 0x0000F800  # pure green (g=248 -> 31 -> 0x03E0)
            self.assertEqual(self.write(cmd), 1)
            self.assertEqual(self.controller.prim_needed, 5)
            self.assertEqual(self.controller.prim_got, 1)
            self.assertEqual(self.write(0x00000000), 1)  # v0 (0, 0)
            self.assertEqual(self.write(0x00000008), 1)  # v1 (8, 0)
            self.assertEqual(self.write(0x00080000), 1)  # v2 (0, 8)
            self.assertEqual(self.write(0x00080008), 1)  # v3 (8, 8)
            self.assertEqual(self.controller.prim_needed, 0)
            self.assertGreater(len(stored), 0)
            self.assertTrue(all(p == 0x03E0 for _, _, p in stored))
            self.assertEqual(self.controller.drawn_pixels, len(stored))

            self.assertEqual(bool(back_reads), bool(opcode & 2))
    def test_gp0_poly_g3_gouraud_triangle(self):
        for opcode in (0x30, 0x31, 0x32, 0x33):
            self.setUp()
            stored = {}
            self.store_cb = STORE(lambda _, x, y, p: stored.__setitem__((x, y), p) or 1)
            self.controller.backend.store_vram = self.store_cb
            # Semi-transparent variants (command bit 1) read the back buffer.
            # With a black back buffer and additive blending (E1 abr=1),
            # B+F == F, so the colours below hold for every variant.
            back_reads = []
            self.read_cb = READ(lambda _, x, y, out: back_reads.append((x, y))
                                or out.__setitem__(0, 0) or 1)
            self.controller.backend.read_vram = self.read_cb
            if opcode & 2:
                self.assertEqual(self.write(0xE1000020), 1)
            self.assertEqual(self.write(0xE3000000), 1)
            self.assertEqual(self.write(0xE403C13F), 1)  # 319, 240
            cmd = (opcode << 24) | 0x000000F8  # v0 red (r=248 -> 31)
            self.assertEqual(self.write(cmd), 1)
            self.assertEqual(self.controller.prim_needed, 6)
            self.assertEqual(self.controller.prim_got, 1)
            self.assertEqual(self.write(0x00000000), 1)  # v0 (0, 0)
            self.assertEqual(self.write(0x0000F800), 1)  # c1 green (g=248 -> 31)
            self.assertEqual(self.write(0x00000010), 1)  # v1 (16, 0)
            self.assertEqual(self.write(0x00F80000), 1)  # c2 blue (b=248 -> 31)
            self.assertEqual(self.write(0x00100000), 1)  # v2 (0, 16)
            self.assertEqual(self.controller.prim_needed, 0)
            self.assertGreater(len(stored), 0)
            self.assertEqual(stored.get((0, 0)), 0x001F)  # pure red
            self.assertNotIn((16, 0), stored)
            self.assertNotIn((0, 16), stored)
            self.assertEqual(stored[(0, 8)], 0x3C0F)  # halfway from red to blue
            mid = stored.get((8, 0))
            self.assertIsNotNone(mid)
            self.assertGreater(mid & 0x1F, 0)
            self.assertGreater((mid >> 5) & 0x1F, 0)
            self.assertEqual((mid >> 10) & 0x1F, 0)

            self.assertEqual(bool(back_reads), bool(opcode & 2))
    def test_undithered_horizontal_color_rounds_each_component(self):
        left, right, width = (214, 13, 76), (9, 244, 76), 37
        stored = {}
        self.store_cb = STORE(lambda _, x, y, p: stored.__setitem__((x, y), p) or 1)
        self.controller.backend.store_vram = self.store_cb
        self.assertEqual(self.write(0xE3000000), 1)
        self.assertEqual(self.write(0xE403C13F), 1)
        for i, (x, y, rgb) in enumerate(((11, 3, left), (11 + width, 3, right),
                                         (11, 7, left), (11 + width, 7, right))):
            packed = sum(c << (8 * k) for k, c in enumerate(rgb))
            self.assertEqual(self.write(packed | (0x38000000 if i == 0 else 0)), 1)
            self.assertEqual(self.write(x | (y << 16)), 1)

        steps = [(1 if b >= a else -1) * (abs(b - a) * 4096 // width)
                 for a, b in zip(left, right)]
        for dx in range(width):
            rgb = [(a * 4096 + dx * step + 2048) // 4096 for a, step in zip(left, steps)]
            expected = sum((c >> 3) << (5 * k) for k, c in enumerate(rgb))
            self.assertEqual(stored[11 + dx, 5], expected)

    def test_gp0_poly_g4_gouraud_quad(self):
        for opcode in (0x38, 0x39, 0x3A, 0x3B):
            self.setUp()
            stored = {}
            self.store_cb = STORE(lambda _, x, y, p: stored.__setitem__((x, y), p) or 1)
            self.controller.backend.store_vram = self.store_cb
            # Semi-transparent variants (command bit 1) read the back buffer.
            # With a black back buffer and additive blending (E1 abr=1),
            # B+F == F, so the colours below hold for every variant.
            back_reads = []
            self.read_cb = READ(lambda _, x, y, out: back_reads.append((x, y))
                                or out.__setitem__(0, 0) or 1)
            self.controller.backend.read_vram = self.read_cb
            if opcode & 2:
                self.assertEqual(self.write(0xE1000020), 1)
            self.assertEqual(self.write(0xE3000000), 1)
            self.assertEqual(self.write(0xE403C13F), 1)  # 319, 240
            cmd = (opcode << 24) | 0x000000F8  # v0 red
            self.assertEqual(self.write(cmd), 1)
            self.assertEqual(self.controller.prim_needed, 8)
            self.assertEqual(self.controller.prim_got, 1)
            self.assertEqual(self.write(0x00000000), 1)  # v0 (0, 0)
            self.assertEqual(self.write(0x0000F800), 1)  # c1 green
            self.assertEqual(self.write(0x00000010), 1)  # v1 (16, 0)
            self.assertEqual(self.write(0x00F80000), 1)  # c2 blue
            self.assertEqual(self.write(0x00100000), 1)  # v2 (0, 16)
            self.assertEqual(self.write(0x00F8F8F8), 1)  # c3 white
            self.assertEqual(self.write(0x00100010), 1)  # v3 (16, 16)
            self.assertEqual(self.controller.prim_needed, 0)
            self.assertGreater(len(stored), 0)
            self.assertEqual(stored.get((0, 0)), 0x001F)    # red
            self.assertNotIn((16, 0), stored)
            self.assertEqual(stored[(8, 0)], 0x01EF)  # halfway from red to green
            self.assertNotIn((0, 16), stored)
            self.assertEqual(stored[(0, 8)], 0x3C0F)  # halfway from red to blue
            self.assertNotIn((16, 16), stored)
            self.assertEqual(stored[(12, 12)], 0x5EEF)  # towards white in the second triangle

            self.assertEqual(bool(back_reads), bool(opcode & 2))

    def test_polygon_page_selects_the_next_sprite_texture(self):
        pixels = {(341, 9): 0x001F, (405, 9): 0x7C00}

        def read_pixel(_, x, y, out):
            out[0] = pixels.get((x, y), 0)
            return 1

        def write_pixel(_, x, y, value):
            pixels[x, y] = value
            return 1

        self.read_cb = READ(read_pixel)
        self.store_cb = STORE(write_pixel)
        self.controller.backend.read_vram = self.read_cb
        self.controller.backend.store_vram = self.store_cb
        uv = (9 << 8) | 21
        words = [
            0xE3000000, 0xE407FFFF, 0xE5000000, 0xE2000000, 0xE1000705,
            0x65808080, (300 << 16) | 128, uv, 0x00010001,
            0x25808080, (320 << 16) | 128, uv, (320 << 16) | 130,
            (0x126 << 16) | uv, (322 << 16) | 128, uv,
            0x65808080, (300 << 16) | 129, uv, 0x00010001,
        ]
        for word in words:
            self.assertEqual(self.write(word), 1)
        # Independent synthetic GPUREAD and GPUSTAT observations.
        self.assertEqual(pixels[128, 300] | (pixels[129, 300] << 16), 0x7C00001F)
        self.assertEqual(self.read()[1] & 0x1FF, 0x126)

    def test_polygon_page_commits_only_after_a_complete_accepted_packet(self):
        for opcode, length, uv1 in ((0x24, 7, 4), (0x2F, 9, 4),
                                    (0x36, 9, 5), (0x3D, 12, 5)):
            with self.subTest(opcode=hex(opcode)):
                self.controller.raster_disabled = 1
                self.assertEqual(self.write(0xE1001005), 1)
                words = [opcode << 24] + [0] * (length - 1)
                words[uv1] = 0xF1260000
                for word in words[:-1]:
                    self.assertEqual(self.write(word), 1)
                self.assertEqual(self.controller.draw_mode, 0x1005)
                self.assertEqual(self.write(words[-1]), 1)
                self.assertEqual(self.controller.draw_mode, 0x1126)
        # An untextured Gouraud triangle has no texture page.
        for word in (0x30808080, 0, 0, 0, 0x01420000, 0x01110000):
            self.assertEqual(self.write(word), 1)
        self.assertEqual(self.controller.draw_mode, 0x1126)
        # A FIFO reset discards an incomplete packet including its UV1 word.
        for word in (0x25808080, 0, 0, 2, 0x01040000):
            self.assertEqual(self.write(word), 1)
        self.assertEqual(self.write(0x01000000, GPUSTAT), 1)
        self.assertEqual(self.controller.draw_mode, 0x1126)
        # The CPU renderer refuses this triangle because it has no VRAM reader.
        self.controller.raster_disabled = 0
        for word in (0xE407FFFF, 0x25808080, 0, 0, 2, 0x01040000, 2 << 16):
            self.assertEqual(self.write(word), 1)
        self.assertEqual(self.write(0), 0)
        self.assertEqual(self.controller.draw_mode, 0x1126)
        self.assertEqual(self.write(0xE1000105), 1)
        self.assertEqual(self.read()[1] & 0x1FF, 0x105)

    def test_texture_window_selects_bits_before_texel_and_palette_lookup(self):
        pixels = {}

        def read_pixel(_, x, y, out):
            out[0] = pixels.get((x, y), 0)
            return 1

        def write_pixel(_, x, y, value):
            pixels[x, y] = value
            return 1

        self.read_cb = READ(read_pixel)
        self.store_cb = STORE(write_pixel)
        self.controller.backend.read_vram = self.read_cb
        self.controller.backend.store_vram = self.store_cb

        def packet(*words):
            for word in words:
                self.assertEqual(self.write(word), 1)

        def sprite(x, u, v, clut=0):
            packet(0x65808080, 300 << 16 | x, clut << 16 | v << 8 | u, 0x00010001)

        packet(0xE3000000, 0xE407FFFF, 0xE5000000, 0xE1000105)
        pixels.update({(341, 9): 0x001F, (357, 9): 0x7C00,
                       (493, 213): 0x4210, (357, 149): 0x03E0})
        # Readbacks were checked with the same GP0 commands in a synthetic PS-X executable.
        packet(0xE2000000)
        sprite(128, 21, 9)
        packet(0xE200101E)
        sprite(129, 21, 9)
        packet(0xE2000000)
        sprite(130, 0xAD, 0xD5)
        packet(0xE2011155)
        sprite(131, 0xAD, 0xD5)
        self.assertEqual([pixels.get((x, 300)) for x in range(128, 132)],
                         [0x001F, 0x7C00, 0x4210, 0x03E0])

        # For U=37 the four-bit path selects the second nibble of word 9,
        # and the eight-bit path selects the high byte of word 18.
        palette = (42 << 6) | 2
        pixels.update({(329, 9): 0x0040, (338, 9): 0xC700,
                       (32 + 4, 42): 0x1234, (32 + 199, 42): 0x5678})
        packet(0xE200101E, 0xE1000005)
        sprite(132, 21, 9, palette)
        packet(0xE1000085)
        sprite(133, 21, 9, palette)
        self.assertEqual([pixels.get((x, 300)) for x in (132, 133)], [0x1234, 0x5678])

    def test_gp0_poly_gt3_textured_gouraud_triangle(self):
        for opcode in (0x34, 0x35):
            self.setUp()
            stored = []
            self.store_cb = STORE(lambda _, x, y, p: stored.append((x, y, p)) or 1)
            self.read_cb = READ(lambda _, x, y, out: out.__setitem__(0, 0x7FFF) or 1)
            self.controller.backend.store_vram = self.store_cb
            self.controller.backend.read_vram = self.read_cb
            self.assertEqual(self.write(0xE3000000), 1)
            self.assertEqual(self.write(0xE403C13F), 1)
            tpage = 0x0100
            cmd = (opcode << 24) | 0x00808080
            self.assertEqual(self.write(cmd), 1)
            self.assertEqual(self.controller.prim_needed, 9)
            self.assertEqual(self.write(0x00000000), 1)  # v0 (0, 0)
            self.assertEqual(self.write(0x00000000), 1)  # uv0=(0,0), clut=0
            self.assertEqual(self.write(0x00808080), 1)  # c1
            self.assertEqual(self.write(0x00000008), 1)  # v1 (8, 0)
            self.assertEqual(self.write((tpage << 16) | 0x0008), 1)  # uv1=(8,0), tpage
            self.assertEqual(self.write(0x00808080), 1)  # c2
            self.assertEqual(self.write(0x00080000), 1)  # v2 (0, 8)
            self.assertEqual(self.write(0x00000800), 1)  # uv2=(0,8)
            self.assertEqual(self.controller.prim_needed, 0)
            self.assertGreater(len(stored), 0)

    def test_gp0_poly_gt4_textured_gouraud_quad(self):
        for opcode in (0x3C, 0x3D):
            self.setUp()
            stored = []
            self.store_cb = STORE(lambda _, x, y, p: stored.append((x, y, p)) or 1)
            self.read_cb = READ(lambda _, x, y, out: out.__setitem__(0, 0x7FFF) or 1)
            self.controller.backend.store_vram = self.store_cb
            self.controller.backend.read_vram = self.read_cb
            self.assertEqual(self.write(0xE3000000), 1)
            self.assertEqual(self.write(0xE403C13F), 1)
            tpage = 0x0100
            cmd = (opcode << 24) | 0x00808080
            self.assertEqual(self.write(cmd), 1)
            self.assertEqual(self.controller.prim_needed, 12)
            self.assertEqual(self.write(0x00000000), 1)  # v0 (0, 0)
            self.assertEqual(self.write(0x00000000), 1)  # uv0=(0,0), clut=0
            self.assertEqual(self.write(0x00808080), 1)  # c1
            self.assertEqual(self.write(0x00000008), 1)  # v1 (8, 0)
            self.assertEqual(self.write((tpage << 16) | 0x0008), 1)  # uv1=(8,0), tpage
            self.assertEqual(self.write(0x00808080), 1)  # c2
            self.assertEqual(self.write(0x00080000), 1)  # v2 (0, 8)
            self.assertEqual(self.write(0x00000800), 1)  # uv2=(0,8)
            self.assertEqual(self.write(0x00808080), 1)  # c3
            self.assertEqual(self.write(0x00080008), 1)  # v3 (8, 8)
            self.assertEqual(self.write(0x00000808), 1)  # uv3=(8,8)
            self.assertEqual(self.controller.prim_needed, 0)
            self.assertGreater(len(stored), 0)

    def test_variable_rectangle_dimensions_ignore_upper_bits(self):
        # GP0 rendering rectangles have a different size encoding from VRAM transfers.
        for opcode in (0x60, 0x65):
            for width, height, visible_w, visible_h in (
                    (1024, 2, 0, 2), (1025, 2, 1, 2), (1039, 2, 15, 2),
                    (2, 512, 2, 0), (2, 513, 2, 1), (2, 1027, 2, 3)):
                with self.subTest(opcode=opcode, width=width, height=height):
                    writes, reads = self._geometry_probe(area=(0, 0, 31, 31))
                    self.assertEqual(self.write(0xE1000106), 1)
                    self.assertEqual(self.write((opcode << 24) | 0x808080), 1)
                    self.assertEqual(self.write((4 << 16) | 4), 1)
                    if opcode & 4:
                        self.assertEqual(self.write(0), 1)
                    self.assertEqual(self.write((height << 16) | width), 1)
                    expected = {(4 + x, 4 + y) for x in range(visible_w)
                                for y in range(visible_h)}
                    self.assertEqual(set(writes), expected)
                    self.assertEqual(bool(reads), bool(expected) and bool(opcode & 4))

    def test_gp0_cpu_to_vram_without_backend_refuses(self):
        before = bytes(self.controller)
        self.assertEqual(self.write(0xA0000000), 0)
        self.assertEqual(bytes(self.controller), before)

    def test_gp0_store_image_packet_parsing_and_header_length(self):
        self.read_cb = READ(lambda _, x, y, out: out.__setitem__(0, 0x1234) or 1)
        self.controller.backend.read_vram = self.read_cb
        self.assertEqual(self.store_image_phase(), 0)

        # Word 0: Command 0xC0000000
        self.assertEqual(self.write(0xC0000000), 1)
        self.assertEqual(self.controller.faulted, 0)
        self.assertEqual(self.store_image_phase(), 1)
        self.assertEqual(self.controller.accepted_gp0_words, 1)
        self.assertEqual(self.read()[1] & (1 << 26), 0)
        self.assertEqual(self.read()[1] & (1 << 27), 0)

        # Word 1: VRAM X=16, Y=2
        self.assertEqual(self.write(0x00020010), 1)
        self.assertEqual(self.controller.faulted, 0)
        self.assertEqual(self.store_image_phase(), 2)
        self.assertEqual(self.controller.accepted_gp0_words, 2)
        self.assertEqual(self.read()[1] & (1 << 26), 0)

        # Word 2: Width=2, Height=2 (total 4 halfwords = 2 32-bit words)
        self.assertEqual(self.write(0x00020002), 1)
        self.assertEqual(self.controller.faulted, 0)
        self.assertEqual(self.store_image_phase(), 3)
        self.assertEqual(self.controller.accepted_gp0_words, 3)
        self.assertNotEqual(self.read()[1] & (1 << 26), 0)
        self.assertNotEqual(self.read()[1] & (1 << 27), 0)

        # Verify next word is treated as a new command (NOP)
        self.assertEqual(self.write(0x00000000), 1)
        self.assertEqual(self.controller.accepted_gp0_words, 4)
        self.assertEqual(self.controller.faulted, 0)

    def test_gp0_store_image_transfer_parameters(self):
        self.read_cb = READ(lambda _, x, y, out: out.__setitem__(0, 0x5555) or 1)
        self.controller.backend.read_vram = self.read_cb

        # Test parameter extraction for (x=100, y=50, w=16, h=8)
        self.assertEqual(self.write(0xC0000000), 1)
        self.assertEqual(self.write((50 << 16) | 100), 1)
        self.assertEqual(self.write((8 << 16) | 16), 1)
        self.assertEqual(self.controller.faulted, 0)

        ok, x, y, w, h, total = self.get_store_image()
        self.assertEqual(ok, 1)
        self.assertEqual(x, 100)
        self.assertEqual(y, 50)
        self.assertEqual(w, 16)
        self.assertEqual(h, 8)
        self.assertEqual(total, 128)
        self.assertEqual(self.store_image_remaining_words(), 64)

        # Test odd halfword count (w=3, h=3 -> 9 halfwords -> 5 32-bit words)
        self.assertEqual(self.write(0xC0000000), 1)
        self.assertEqual(self.write((20 << 16) | 10), 1)
        self.assertEqual(self.write((3 << 16) | 3), 1)
        self.assertEqual(self.controller.faulted, 0)

        ok, x, y, w, h, total = self.get_store_image()
        self.assertEqual(ok, 1)
        self.assertEqual(x, 10)
        self.assertEqual(y, 20)
        self.assertEqual(w, 3)
        self.assertEqual(h, 3)
        self.assertEqual(total, 9)
        self.assertEqual(self.store_image_remaining_words(), 5)

        # Test opcodes in range 0xC0..0xDF
        for op in (0xC0, 0xC1, 0xC8, 0xD0, 0xDF):
            self.assertEqual(self.write((op << 24) | 0x123456), 1)
            self.assertEqual(self.write(0x00010002), 1)
            self.assertEqual(self.write(0x00010002), 1)
            self.assertEqual(self.controller.faulted, 0)
            ok, x, y, w, h, total = self.get_store_image()
            self.assertEqual(ok, 1)
            self.assertEqual(w, 2)
            self.assertEqual(h, 1)
            self.assertEqual(total, 2)

    def test_gp0_store_image_roundtrip_with_load_image(self):
        vram = {}

        def mock_store(_, x, y, pixel):
            vram[(x, y)] = pixel
            return 1

        def mock_read(_, x, y, out):
            out[0] = vram.get((x, y), 0)
            return 1

        self.store_cb = STORE(mock_store)
        self.read_cb = READ(mock_read)
        self.controller.backend.store_vram = self.store_cb
        self.controller.backend.read_vram = self.read_cb

        # 1. Load a 4x4 image at (32, 48) with unique colors
        test_pixels = [0x1000 + i * 0x111 for i in range(16)]
        self.assertEqual(self.write(0xA0000000), 1)
        self.assertEqual(self.write((48 << 16) | 32), 1)
        self.assertEqual(self.write((4 << 16) | 4), 1)
        for i in range(0, 16, 2):
            word = test_pixels[i] | (test_pixels[i + 1] << 16)
            self.assertEqual(self.write(word), 1)
        self.assertEqual(self.controller.faulted, 0)
        self.assertEqual(len(vram), 16)

        # 2. StoreImage at (32, 48) with size 4x4
        self.assertEqual(self.write(0xC0000000), 1)
        self.assertEqual(self.write((48 << 16) | 32), 1)
        self.assertEqual(self.write((4 << 16) | 4), 1)
        self.assertEqual(self.controller.faulted, 0)
        self.assertEqual(self.store_image_remaining_words(), 8)

        # 3. Read back 8 words from GPUREAD and verify round-trip pixels
        read_pixels = []
        for _ in range(8):
            ok, val = self.read(GPUREAD)
            self.assertEqual(ok, 1)
            read_pixels.append(val & 0xFFFF)
            read_pixels.append((val >> 16) & 0xFFFF)
        self.assertEqual(read_pixels, test_pixels)
        self.assertEqual(self.controller.faulted, 0)
        self.assertEqual(self.store_image_remaining_words(), 0)

        # 4. Reading past end returns latched last word
        expected_last_word = test_pixels[14] | (test_pixels[15] << 16)
        ok, val = self.read(GPUREAD)
        self.assertEqual(ok, 1)
        self.assertEqual(val, expected_last_word)

    def test_gp0_store_image_roundtrip_with_fill(self):
        vram = {}

        def mock_fill(_, f):
            candidate = f.contents
            for r in range(candidate.height):
                for c in range(candidate.width):
                    vram[(candidate.x + c, candidate.y + r)] = candidate.color
            return 1

        def mock_read(_, x, y, out):
            out[0] = vram.get((x, y), 0)
            return 1

        self.fill_cb = FILL(mock_fill)
        self.read_cb = READ(mock_read)
        self.controller.backend.fill_vram = self.fill_cb
        self.controller.backend.read_vram = self.read_cb

        # Fill 16x16 with color
        self.assertEqual(self.write(0x02ff0000), 1)
        self.assertEqual(self.write(0x00000000), 1)
        self.assertEqual(self.write((16 << 16) | 16), 1)
        self.assertEqual(self.controller.faulted, 0)

        # StoreImage over filled area
        self.assertEqual(self.write(0xC0000000), 1)
        self.assertEqual(self.write(0x00000000), 1)
        self.assertEqual(self.write((16 << 16) | 16), 1)
        self.assertEqual(self.controller.faulted, 0)

        # Read back 128 words (256 halfwords)
        for _ in range(128):
            ok, val = self.read(GPUREAD)
            self.assertEqual(ok, 1)
            p0 = val & 0xFFFF
            p1 = (val >> 16) & 0xFFFF
            self.assertEqual(p0, 0x7C00)
            self.assertEqual(p1, 0x7C00)
        self.assertEqual(self.controller.faulted, 0)

    def test_gp0_store_image_boundary_handling(self):
        vram = {}
        for cy in range(508, 512):
            for cx in range(1020, 1024):
                vram[(cx, cy)] = 0x1234

        accessed = []

        def mock_read(_, x, y, out):
            accessed.append((x, y))
            out[0] = vram.get((x, y), 0)
            return 1

        self.read_cb = READ(mock_read)
        self.controller.backend.read_vram = self.read_cb

        # StoreImage spanning past bottom-right: x=1020, y=510, w=8, h=4
        self.assertEqual(self.write(0xC0000000), 1)
        self.assertEqual(self.write((510 << 16) | 1020), 1)
        self.assertEqual(self.write((4 << 16) | 8), 1)
        self.assertEqual(self.controller.faulted, 0)

        for ax, ay in accessed:
            self.assertLess(ax, 1024)
            self.assertLess(ay, 512)

        pixels = []
        for _ in range(16):
            ok, val = self.read(GPUREAD)
            self.assertEqual(ok, 1)
            pixels.append(val & 0xFFFF)
            pixels.append((val >> 16) & 0xFFFF)

        for r in range(4):
            for c in range(8):
                idx = r * 8 + c
                if r < 2 and c < 4:
                    self.assertEqual(pixels[idx], 0x1234)
                else:
                    self.assertEqual(pixels[idx], 0)
        self.assertEqual(self.controller.faulted, 0)

    def test_gp0_store_image_backend_refusal_and_missing_callback(self):
        before = bytes(self.controller)
        self.assertEqual(self.write(0xC0000000), 0)
        self.assertEqual(bytes(self.controller), before)

        self.read_cb = READ(lambda *_: 0)
        self.controller.backend.read_vram = self.read_cb
        self.assertEqual(self.write(0xC0000000), 1)
        self.assertEqual(self.write(0x00000000), 1)
        self.assertEqual(self.write(0x00010001), 0)
        self.assertEqual(self.controller.faulted, 1)

        self.setUp()
        self.read_cb = READ(lambda _, x, y, out: out.__setitem__(0, 1) or 1)
        self.controller.backend.read_vram = self.read_cb
        self.assertEqual(self.write(0xC0000000), 1)
        self.assertEqual(self.write(0x00000000), 1)
        self.ready_value = 0
        self.assertEqual(self.write(0x00010001), 0)
        self.assertEqual(self.controller.faulted, 0)

    def test_gp0_store_image_gp1_reset_and_clear_fifo(self):
        self.read_cb = READ(lambda _, x, y, out: out.__setitem__(0, 0xABCD) or 1)
        self.controller.backend.read_vram = self.read_cb

        self.assertEqual(self.write(0xC0000000), 1)
        self.assertEqual(self.write(0x00000000), 1)
        self.assertEqual(self.write(0x00020002), 1)
        self.assertEqual(self.store_image_remaining_words(), 2)

        self.assertEqual(self.write(0x00000000, GPUSTAT), 1)
        self.assertEqual(self.store_image_phase(), 0)
        self.assertEqual(self.store_image_remaining_words(), 0)
        self.assertEqual(self.controller.read_latch, 0x400)

        self.assertEqual(self.write(0xC0000000), 1)
        self.assertEqual(self.store_image_phase(), 1)
        self.assertEqual(self.write(0x01000000, GPUSTAT), 1)
        self.assertEqual(self.store_image_phase(), 0)

    def test_gp0_store_image_dimension_limits(self):
        self.read_cb = READ(lambda _, x, y, out: out.__setitem__(0, 1) or 1)
        self.controller.backend.read_vram = self.read_cb

        self.assertEqual(self.write(0xC0000000), 1)
        self.assertEqual(self.write(0x00000000), 1)
        self.assertEqual(self.write((1 << 16) | 1025), 0)
        self.assertEqual(self.controller.faulted, 0)

        self.assertEqual(self.write((513 << 16) | 1), 0)
        self.assertEqual(self.controller.faulted, 0)

    def test_gp0_store_image_dma_direction_status(self):
        self.read_cb = READ(lambda _, x, y, out: out.__setitem__(0, 0x42) or 1)
        self.controller.backend.read_vram = self.read_cb

        self.assertEqual(self.write(0x04000003, GPUSTAT), 1)
        self.assertEqual(self.read()[1] & (1 << 25), 0)

        self.assertEqual(self.write(0xC0000000), 1)
        self.assertEqual(self.write(0x00000000), 1)
        self.assertEqual(self.write(0x00010004), 1)

        status = self.read()[1]
        self.assertNotEqual(status & (1 << 25), 0)
        self.assertNotEqual(status & (1 << 27), 0)

        self.read(GPUREAD)
        self.read(GPUREAD)

        status = self.read()[1]
        self.assertEqual(status & (1 << 25), 0)
        self.assertEqual(status & (1 << 27), 0)


if __name__ == "__main__":
    unittest.main()
