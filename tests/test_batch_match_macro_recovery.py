"""Check target-level inline assembly when a banked source looks like plain C."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import batch_match as batch
import build_candidate as build
import progress


class CompilerRecoveryTests(unittest.TestCase):
    def test_scope_requires_the_named_procedure(self):
        assembly = """.ent other
other:
#APP
 .set noreorder; nop
#NO_APP
.end other
.ent clean
clean:
#APP
# just a comment
#NO_APP
 jr $31
.end clean
"""
        self.assertTrue(progress.compiler_inline_asm(assembly, "other"))
        self.assertFalse(progress.compiler_inline_asm(assembly, "clean"))
        self.assertIsNone(progress.compiler_inline_asm(assembly, "absent"))
        self.assertIsNone(progress.compiler_inline_asm(".ent clean\nclean:\n", "clean"))

    def test_compiler_marks_only_asm_emitted_in_target(self):
        tool_root = Path(os.environ.get("PSYQ_TOOLCHAIN_ROOT", build.default_toolchain_root(ROOT)))
        toolchain = tool_root / build.resolve_toolchain("gcc-2.7.2-psx").name
        if not all((toolchain / name).is_file() for name in ("cpp", "cc1")):
            self.skipTest("requires the matching PS1 compiler")
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            (tmp / "ops.h").write_text(
                '#define HIDDEN() __asm__ volatile ("nop")\n'
                '#define PIN(x) __asm__ volatile ("" : "=r"(x) : "0"(x))\n'
                '#ifdef NEVER_DEFINED\n#define MAYBE() HIDDEN()\n#else\n#define MAYBE() ((void)0)\n#endif\n'
            )
            source = tmp / "subject.c"
            source.write_text(
                '#include "ops.h"\n'
                'int touched(int x) { HIDDEN(); return x + 5; }\n'
                'int clean(int x) { MAYBE(); PIN(x); return x + 7; }\n'
            )
            self.assertEqual(progress.classify_recovery(source.read_text()), "c")
            unit, assembly = tmp / "subject.i", tmp / "subject.s"
            with unit.open("w") as out:
                subprocess.run(build.cpp_command(toolchain / "cpp", source), stdout=out, check=True, timeout=20)
            subprocess.run(build.cc1_command(toolchain / "cc1", unit, assembly), check=True, timeout=20,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            output = assembly.read_text()
            self.assertTrue(progress.compiler_inline_asm(output, "touched"))
            self.assertFalse(progress.compiler_inline_asm(output, "clean"))

    def test_existing_and_banked_candidates_honor_the_build_finding(self):
        function = batch.FunctionRange("subject", 0x1000, 8)
        with tempfile.TemporaryDirectory() as td:
            repo = Path(td)
            scratch, bank = repo / "scratch", repo / "bank"
            scratch.mkdir()
            bank.mkdir()
            original = b'#include "ops.h"\nint subject(void) { HIDDEN(); return 1; }\n'
            source = bank / "subject.c"
            source.write_bytes(original)
            asm = repo / "disasm.s"
            asm.write_text("")
            def nonportable(fn, argv):
                self.assertIs(fn, build.main)
                self.assertIn("--check-c-recovery", argv)
                return build.NONPORTABLE_C
            with patch.object(batch, "_quiet", side_effect=nonportable):
                existing = batch.verify_existing(function, "test", {"kind": "executable"}, source, scratch)
                banked = batch.attempt(function, "test", {"kind": "executable"}, repo,
                                       asm, repo / "unused", scratch, bank)
            self.assertEqual(existing.status, "unconfirmed-c-recovery")
            self.assertEqual(banked.status, "unconfirmed-c-recovery")
            self.assertFalse(batch.source_path(repo, "test", function.vram).exists())
            self.assertEqual(source.read_bytes(), original)
