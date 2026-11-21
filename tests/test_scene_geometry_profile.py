import sys
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))

import build_candidate
import verify_registry
from retail_common import RetailError


class GeometryCompilerProfileTests(unittest.TestCase):
    def test_cc1_keeps_default_and_applies_opt_in_once(self):
        args = Path("cc1"), Path("source.i"), Path("output.s")
        default = build_candidate.cc1_command(*args)
        self.assertEqual(default, build_candidate.cc1_command(*args, no_schedule_insns2=False))
        self.assertNotIn("-fno-schedule-insns2", default)
        opted_in = build_candidate.cc1_command(*args, no_schedule_insns2=True)
        self.assertEqual(opted_in.count("-fno-schedule-insns2"), 1)

    def test_registry_accepts_only_a_boolean_override(self):
        entry = dict(name="test", region="main", vram=0x80012000, size=8,
                     source="src/main/test.c")
        targets = {"main": {"kind": "executable", "identity": "provenance/exe_identity.json"}}
        args = targets, Path("candidate.bin")
        default, _ = verify_registry.plan(entry, *args)
        self.assertNotIn("--no-schedule-insns2", default)
        for flag in (True, False):
            build, _ = verify_registry.plan(dict(entry, no_schedule_insns2=flag), *args)
            self.assertEqual(build.count("--no-schedule-insns2"), int(flag))
        for value in (1, "false", None):
            with self.subTest(value=value), self.assertRaises(RetailError):
                verify_registry.plan(dict(entry, no_schedule_insns2=value), *args)


if __name__ == "__main__":
    unittest.main()
