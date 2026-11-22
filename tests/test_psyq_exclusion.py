"""No Sony PsyQ code in the port (config/psyq_ranges.txt, tools/psyq_manifest.py)."""
from pathlib import Path
import json
import tempfile
from unittest.mock import patch
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import psyq_manifest  # noqa: E402


IDENTIFIED_SOUND_ENTRIES = (
    0x8003C438, 0x8003D424, 0x8003E2E4, 0x8003FA54,
    0x80041354, 0x800413B8, 0x80041400, 0x80041448,
)


class PsyqExclusionTest(unittest.TestCase):
    def test_manifest_covers_the_libraries_and_not_game_code(self):
        table = psyq_manifest.ranges()
        self.assertEqual(psyq_manifest.psyq_segment(0x800425B0, table), "libetc")
        self.assertEqual(psyq_manifest.psyq_segment(0x8005C388, table), "libc2_1")
        self.assertEqual(psyq_manifest.psyq_segment(0x8003A444, table), "snd1")
        for start, end, expected, following in (
            (0x8003C438, 0x8003C498, "libspu_transfer", "snd2"),
            (0x8003D424, 0x8003D434, "libspu_reverb", "snd3"),
            (0x8003D630, 0x8003D650, "libsnd_quit", "snd4"),
            (0x8003DC90, 0x8003E248, "libsnd_crescendo", "snd5"),
            (0x8003E2E4, 0x8003E310, "libsnd_play", "snd6"),
            (0x8003FA54, 0x8003FA64, "libsnd_vm_doff", "snd12"),
            (0x80040DE8, 0x800414E4, "libsnd_vm_seq", "snd9"),
        ):
            self.assertEqual(psyq_manifest.psyq_segment(start, table), expected)
            self.assertEqual(psyq_manifest.psyq_segment(end - 1, table), expected)
            self.assertEqual(psyq_manifest.psyq_segment(end, table), following)
        for vram in (0x80010000, 0x8001903C, 0x80057928):
            self.assertIsNone(psyq_manifest.psyq_segment(vram, table), hex(vram))
        for lo, hi, _ in table:
            self.assertLess(lo, hi)

    def test_published_source_tree_contains_no_identified_sdk_routines(self):
        table = psyq_manifest.ranges()
        # A source package can contain functions outside every build target.
        if (ROOT / ".git").exists():
            names = subprocess.check_output(["git", "ls-files", "-z", "--", "src"], cwd=ROOT)
            paths = (ROOT / name for name in names.decode("utf-8", "surrogateescape").split("\0") if name)
        else:
            paths = (ROOT / "src").rglob("*")
        found = [path.relative_to(ROOT).as_posix() for path in paths
                 if path.suffix.lower() in {".c", ".s", ".asm"}
                 and psyq_manifest.source_segment(path.relative_to(ROOT), table)]
        self.assertEqual(sorted(found), [])

    def test_identified_sound_sources_are_excluded(self):
        matches = json.loads((ROOT / "provenance/native-main.json").read_text())["matches"]
        registered = {row["vram"] for row in matches}
        allowed = (ROOT / "config/lane_own_code_allow.txt").read_text().splitlines()
        for pc in IDENTIFIED_SOUND_ENTRIES:
            path = f"src/main/{pc:08x}.c"
            self.assertFalse((ROOT / path).exists())
            self.assertNotIn(pc, registered)
            self.assertNotIn(f"{pc:08x}", allowed)
            self.assertEqual(self.check(path).returncode, 1)

    def test_manifest_build_keeps_identified_sound_routines(self):
        with tempfile.TemporaryDirectory() as tmp:
            layout = Path(tmp) / "layout"
            layout.write_text("- [0x2cc20, c, snd1]\n"
                              "- [0x2cc38, c, sgap_test]\n"
                              "- [0x31ce4, c, snd2]\n"
                              "- [0x31d04]\n")
            with patch.object(psyq_manifest, "MANIFEST", Path(tmp) / "ranges"):
                psyq_manifest.build(layout)
                self.assertEqual(psyq_manifest.ranges(), [
                    (0x8003C420, 0x8003C438, "snd1"),
                    (0x8003C438, 0x8003C498, "libspu_transfer"),
                    (0x8003D424, 0x8003D434, "libspu_reverb"),
                    (0x8003D630, 0x8003D650, "libsnd_quit"),
                    (0x8003DC90, 0x8003E248, "libsnd_crescendo"),
                    (0x8003E2E4, 0x8003E310, "libsnd_play"),
                    (0x8003FA54, 0x8003FA64, "libsnd_vm_doff"),
                    (0x80040DE8, 0x800414E4, "libsnd_vm_seq"),
                    (0x800414E4, 0x80041504, "snd2"),
                ])

    def test_unresolved_resident_sources_stay_out_of_host_selection(self):
        import native_lane_gen
        pcs = (0x8001282C, 0x800128EC)
        rows = json.loads((ROOT / "provenance/native-main.json").read_text())["matches"]
        allowed = (ROOT / "config/lane_own_code_allow.txt").read_text().splitlines()
        for pc in pcs:
            self.assertFalse((ROOT / f"src/main/{pc:08x}.c").exists())
            self.assertNotIn(pc, {row["vram"] for row in rows})
            self.assertNotIn(f"{pc:08x}", allowed)
        probes = [dict(region="main", recovery="c", vram=pc) for pc in pcs]
        self.assertEqual(native_lane_gen.candidates(registry=probes), [])

    def check(self, *sources):
        return subprocess.run(
            [sys.executable, str(ROOT / "tools/psyq_manifest.py"), "check",
             "--allow", str(ROOT / "config/psyq_pending_replacement.txt"), *sources],
            cwd=ROOT, capture_output=True, text=True)

    def test_check_refuses_new_psyq_sources(self):
        run = self.check("src/main/8004239c.c")
        self.assertEqual(run.returncode, 1)
        self.assertIn("FORBIDDEN src/main/8004239c.c", run.stderr)

    def test_check_accepts_game_code_and_pending_bindings_only(self):
        self.assertEqual(self.check("src/main/8001903c.c").returncode, 0)
        # Replaced bindings never come back.
        for replaced in ("800425b0", "80042c64", "8005c388", "800427f4", "80042e08", "8005bd7c", "8005c1c0", "800426fc", "800616d0", "8006291c", "80062988"):
            self.assertEqual(self.check(f"src/main/{replaced}.c").returncode, 1, replaced)

    def test_no_psyq_binding_is_pending(self):
        pending = [l for l in (ROOT / "config/psyq_pending_replacement.txt").read_text().splitlines()
                   if l.strip() and not l.startswith("#")]
        self.assertEqual(pending, [])

    def test_native_lane_never_takes_psyq_candidates(self):
        import native_lane_gen
        excluded = []
        registry = native_lane_gen.load_matches()
        # Supply SDK addresses without code. The generator rejects them before lookup.
        sdk_entries = (0x800425B0, *IDENTIFIED_SOUND_ENTRIES)
        registry.extend(dict(vram=pc, region="main", recovery="c") for pc in sdk_entries)
        for m in native_lane_gen.candidates(excluded=excluded, registry=registry):
            self.assertIsNone(psyq_manifest.psyq_segment(m["vram"]), m["name"])
        self.assertTrue(set(sdk_entries).issubset(excluded))


if __name__ == "__main__":
    unittest.main()
