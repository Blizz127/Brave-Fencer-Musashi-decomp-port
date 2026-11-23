import ctypes
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc03_0054"
CALLBACK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint)
NAMES = ("progress", "command", "script", "set_progress", "set_flag", "flag", "query", "item")
UNCHANGED = 0x7563

def initial_actor():
    actor = bytearray((i * 29 + 11) & 255 for i in range(54))
    actor[52:54] = UNCHANGED.to_bytes(2, sys.byteorder)
    return actor

def build_probe(directory):
    library = Path(directory) / "jon.so"
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-flto", "-fPIC", "-shared",
                    "-Wall", "-Wextra", "-Werror", "-I", str(SCENE), str(SCENE / "801ef558.c"),
                    str(ROOT / "tests/jon_stocks_probe.c"), "-o", str(library)], check=True)
    host = ctypes.CDLL(str(library))
    host.jon_probe_run.argtypes = [ctypes.c_void_p, CALLBACK]
    host.jon_probe_run.restype = None
    return host

def observe(host, progress, flag=0, query=0, change_during_flag=False):
    actor = initial_actor()
    buffer = (ctypes.c_ubyte * len(actor)).from_buffer(actor)
    globals_ = [ctypes.c_ushort.in_dll(host, name) for name in ("bfm_village_jon_state0", "bfm_village_jon_state1")]
    for value in globals_:
        value.value = UNCHANGED
    events = []
    def service(number, first, second):
        nonlocal progress
        name = NAMES[number]
        events.append((name, first, second, int.from_bytes(actor[2:4], sys.byteorder),
                       int.from_bytes(actor[52:54], sys.byteorder), *(v.value for v in globals_)))
        if name == "progress":
            return progress
        if name == "set_progress":
            progress = first
        if name == "flag":
            if change_during_flag:
                progress = 1000
            return flag
        return query if name == "query" else 0
    host.jon_probe_run(buffer, CALLBACK(service))
    return bytes(actor), tuple(v.value for v in globals_), progress, events

class JonStocks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temp.cleanup)
        cls.host = build_probe(cls.temp.name)

    def test_story_branches_and_effect_order(self):
        read = ("progress", 0, 0)
        command = ("command", 18, 0)
        flag_check = ("flag", 228, 0)
        query_check = ("query", 0, 5)
        cases = [
            ((-2147483648, 30, 39), 0, 0, [read, command, ("script", 0, 0), ("set_progress", 40, 0), ("set_flag", 227, 1)], 0),
            ((40, 49), 1, 1, [read, command, ("script", 1, 0)], 1),
            ((50, 69, 70, 2147483647), -1, 0, [read, flag_check, command, ("script", 2, 0)], 5),
            ((50, 59), 256, 0, [read, flag_check, command, ("script", 3, 0)], 2),
            ((60, 69), 0, 0, [read, flag_check, query_check, command, ("script", 4, 0)], 2),
            ((60, 69), 0, -1, [read, flag_check, query_check, command, ("script", 5, 0), ("item", 77, 0)], 6),
            ((70, 2147483647), 0, 1, [read, flag_check], UNCHANGED),
        ]
        for stages, flag, query, calls, branch in cases:
            for stage in stages:
                with self.subTest(progress=stage, flag=flag, query=query):
                    actor, globals_, progress, events = observe(self.host, stage, flag, query)
                    want = initial_actor()
                    want[2:4] = (6).to_bytes(2, sys.byteorder)
                    want[52:54] = branch.to_bytes(2, sys.byteorder)
                    self.assertEqual(actor, want)
                    self.assertEqual(globals_, (0, 0))
                    self.assertEqual(progress, 40 if stage < 40 else stage)
                    self.assertEqual([e[:3] for e in events], calls)
                    self.assertTrue(all(e[3:] == (6, UNCHANGED, 0, 0) for e in events))

    def test_matching_source_stays_out_of_the_native_dispatcher(self):
        sys.path.insert(0, str(ROOT / "tools"))
        try:
            import native_lane_gen as lane
        finally:
            sys.path.pop(0)
        row = dict(region="sc03_0054", vram=0x801EF558, size=384)
        owned = SCENE / "801ef558.c"
        self.assertTrue(lane.overlay_extent_source_only(row, owned, None, lane.SCENE_PHASE_EXTENTS))
        with tempfile.TemporaryDirectory() as folder:
            independent = Path(folder) / "alternative.c"
            independent.write_text("int independently_selected(void) { return 1; }\n")
            other = dict(region="sc03_0077", vram=row["vram"], size=176)
            self.assertFalse(lane.overlay_extent_source_only(other, independent, None, lane.SCENE_PHASE_EXTENTS))

    def test_progress_is_sampled_before_the_flag_service(self):
        actor, _, progress, events = observe(self.host, 59, change_during_flag=True)
        self.assertEqual(progress, 1000)
        self.assertEqual(int.from_bytes(actor[52:54], sys.byteorder), 2)
        self.assertIn(("script", 3, 0), [e[:3] for e in events])

if __name__ == "__main__":
    unittest.main()
