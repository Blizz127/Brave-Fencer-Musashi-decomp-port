import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc03_0023"
ENTRIES = ((0x8017E414, 272), (0x8017E524, 232), (0x8017EC30, 84))


class CemeteryOpeningTests(unittest.TestCase):
    def test_slab_motion_and_key_activation(self):
        with tempfile.TemporaryDirectory() as directory:
            probe = Path(directory) / "cemetery-opening"
            sources = [SCENE / f"{pc:08x}.c" for pc, _ in ENTRIES]
            subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra",
                            "-Werror", "-I", ROOT / "include", "-I", SCENE, *sources,
                            ROOT / "tests/cemetery_opening_probe.c", "-o", probe], check=True)
            subprocess.run([probe], check=True)

    def test_room_callbacks_require_guest_timing(self):
        sys.path.insert(0, str(ROOT / "tools"))
        try:
            import native_lane_gen as lane
        finally:
            sys.path.pop(0)
        with tempfile.TemporaryDirectory() as directory:
            scratch = Path(directory)
            (scratch / "ir").mkdir()
            alternate = scratch / "alternative.c"
            for pc, size in ENTRIES:
                with self.subTest(pc=hex(pc)):
                    source = SCENE / f"{pc:08x}.c"
                    row = dict(vram=pc, region="sc03_0023", name=f"func_{pc:08X}",
                               size=size, source=str(source))
                    self.assertEqual(lane.build_one(row, scratch, {}, {}, "clang")["rule"], "timing")
                    alternate.write_bytes(source.read_bytes())
                    foreign = dict(row, region="sc03_0024", size=size - 4, source=str(alternate))
                    self.assertEqual(lane.build_one(foreign, scratch, {}, {}, "clang")["rule"], "timing")
                    alternate.write_text("int separate_room(void) { return 1; }\n")
                    self.assertTrue(lane.overlay_extent_source_only(foreign, alternate, {"member": "sc03_0023"},
                                                                  lane.SCENE_PHASE_EXTENTS))
                    self.assertFalse(lane.overlay_extent_source_only(foreign, alternate, None,
                                                                   lane.SCENE_PHASE_EXTENTS))


if __name__ == "__main__":
    unittest.main()
