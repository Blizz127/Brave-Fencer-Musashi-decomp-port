import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/overlays/sc03_0023/8017d828.c"
CC = os.environ.get("CC", "cc")


class CemeteryKeyTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which(CC), "requires a C compiler")
    def test_award_and_continuation_for_all_inventory_masks(self):
        options = [CC, "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2", "-I", ROOT / "include"]
        with tempfile.TemporaryDirectory() as directory:
            award = Path(directory) / "award.o"
            probe = Path(directory) / "cemetery-key"
            subprocess.run([*options, "-Dfunc_800D0EC4=probe_free", "-Dfunc_800D0F0C=probe_item",
                            "-Dfunc_800291A0=probe_variable", "-Dfunc_80029124=probe_flag",
                            "-c", SOURCE, "-o", award], check=True)
            inventory = ROOT / "src/overlays/main_0010"
            accessors = [ROOT / "src/main" / f"{address}.c"
                         for address in ("800291a0", "800291b4", "800291c8", "80029124")]
            subprocess.run([*options, award, inventory / "800d0ec4.c", inventory / "800d0f0c.c",
                            *accessors, ROOT / "tests/cemetery_key_probe.c", "-o", probe], check=True)
            subprocess.run([probe], check=True)

    def test_award_requires_the_cemetery_script_and_guest_timing(self):
        sys.path.insert(0, str(ROOT / "tools"))
        try:
            import native_lane_gen as lane
        finally:
            sys.path.pop(0)
        pc = 0x8017D828
        row = dict(vram=pc, region="sc03_0023", name="func_8017D828", size=92, source=str(SOURCE))
        with tempfile.TemporaryDirectory() as directory:
            scratch = Path(directory)
            (scratch / "ir").mkdir()
            self.assertEqual(lane.build_one(row, scratch, {}, {}, "clang")["rule"], "timing")
            alternate = scratch / "alternate.c"
            alternate.write_bytes(SOURCE.read_bytes())
            foreign = dict(row, region="sc03_0024", source=str(alternate), size=16)
            self.assertEqual(lane.build_one(foreign, scratch, {}, {}, "clang")["rule"], "timing")
            alternate.write_text("int func_8017D828(int x) { return x; }\n")
            # Field names mark the beginning, interior and end of a fabricated callback.
            fields = (b"free-slot-query|", b"inventory-item-write|", b"state-branch-marker|",
                      b"story-progress-write|", b"script-resume|")
            extent = b"".join(fields)
            self.assertEqual(len(extent), row["size"])
            member = dict(member="sc03_0023", vram=pc, start=4, code=[(pc, pc + len(extent))],
                          data=b"room" + extent + b"next")
            self.assertEqual(lane.build_one(foreign, scratch, {}, {}, "clang", member)["rule"], "timing")
            member["member"] = foreign["region"]
            self.assertFalse(lane.overlay_extent_source_only(foreign, alternate, member, lane.SCENE_PHASE_EXTENTS))

            fixture = {pc: {"sc03_0023": (len(extent), lane.fnv1a64(extent))}}
            with patch.dict(lane.SCENE_PHASE_EXTENTS, fixture):
                # The full callback identifies the match even when the row describes a prefix
                # or includes bytes from the next function. Adjacent bytes can vary.
                for adjacent in (b"next", b"stop"):
                    member["data"] = b"room" + extent + adjacent
                    for length in (len(fields[0]), len(extent), len(extent) + len(adjacent)):
                        with self.subTest(adjacent=adjacent, size=length):
                            foreign["size"] = length
                            self.assertEqual(lane.build_one(foreign, scratch, {}, {}, "clang", member)["rule"], "timing")

                foreign["size"] = len(fields[0])
                for changed in (extent.replace(b"free", b"used"), extent.replace(b"state", b"clock"),
                                extent.replace(b"resume", b"finish"), extent[:-1]):
                    with self.subTest(changed=changed):
                        member["data"] = b"room" + changed
                        self.assertFalse(lane.overlay_extent_source_only(foreign, alternate, member,
                                                                         lane.SCENE_PHASE_EXTENTS))


if __name__ == "__main__":
    unittest.main()
