"""Use Clang's real MIPS targets to check how mine sources resolve game addresses."""
import re
import shutil
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ROOM = ROOT / "src/overlays/sc02_0039"

POINT = "bfm_model_probe_point"
BOX_FACE = {POINT, "bfm_model_box_invalid_point", "bfm_model_box_invalid_bounds",
            "bfm_model_box_divide_by_zero"}
HOST_CALLS = {
    "80135d20": {"bfm_model_probe_record", "bfm_model_probe_box", "bfm_model_probe_grid"},
    "80136a94": {POINT},
    "80135480": {POINT, "bfm_model_probe_box", "bfm_surface_zero_scale"},
    "80135eb0": BOX_FACE - {"bfm_model_box_divide_by_zero"},
    "80136334": BOX_FACE,
    "801365b8": BOX_FACE,
    "80136824": BOX_FACE,
    "8013ca14": {"bfm_mine_script_read"},
}


class MineGuestAbiTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.clang = shutil.which("clang")
        if not cls.clang:
            raise unittest.SkipTest("Clang is unavailable")
        targets = subprocess.check_output([cls.clang, "--print-targets"], text=True)
        if not all(re.search(rf"^\s*{name}\s+-", targets, re.MULTILINE)
                   for name in ("mipsel", "mips64el")):
            raise unittest.SkipTest("Clang has no little-endian MIPS targets")

    def test_resolver_calls_by_pointer_abi(self):
        for target, abi, pointer_bytes in [
            ("mipsel-linux-gnu", "32", 4),
            ("mips64el-linux-gnuabi64", "64", 8),
        ]:
            flags = [self.clang, "--target=" + target, "-mabi=" + abi, "-ffreestanding",
                     "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include"]
            subprocess.run([*flags, "-x", "c", "-fsyntax-only", "-"],
                           input=f'_Static_assert(sizeof(void *) == {pointer_bytes}, "wrong ABI");\n',
                           text=True, check=True)
            for name, expected in HOST_CALLS.items():
                with self.subTest(abi=abi, source=name):
                    ir = subprocess.check_output([*flags, "-S", "-emit-llvm", ROOM / (name + ".c"),
                                                  "-o", "-"], text=True)
                    calls = set(re.findall(r"\bcall\b[^\n]*?@(bfm_\w+)\(", ir))
                    self.assertEqual(calls, expected if pointer_bytes == 8 else set())


if __name__ == "__main__":
    unittest.main()
