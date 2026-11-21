"""Exercise the public staging guard with generated data and source files."""
import random
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


GUARD = Path(__file__).resolve().parents[1] / "tools" / "retail_guard.py"


class LiteralGuardTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        self.stage = root / "stage"
        self.stage.mkdir()
        self.reference = root / "reference.dat"
        rng = random.Random(64531)
        self.payload = bytes(rng.randrange(128, 256) for _ in range(61))
        self.reference.write_bytes(b"example prefix" + self.payload + b"end")

    def check(self, name, source, expected):
        (self.stage / name).write_text(source, encoding="utf-8")
        result = subprocess.run([sys.executable, str(GUARD), "--stage", str(self.stage),
                                 "--run-source", str(self.reference),
                                 "--run-bytes", "32"], capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        return result

    def test_python_escaped_and_concatenated_without_execution(self):
        parts = [r"b'" + "".join(fr"\x{value:02x}" for value in self.payload[i:i + 9]) + "'"
                 for i in range(0, len(self.payload), 9)]
        source = "raise RuntimeError('never execute staged code')\nrecord = (\n"
        source += "\n".join(parts) + "\n)\n"
        result = self.check("example.py", source, 1)
        self.assertIn("example.py:3: 61 decoded literal bytes", result.stdout)

    def test_c_hex_octal_comments_and_line_splices(self):
        parts = []
        for i in range(0, len(self.payload), 7):
            forms = [fr"\x{value:02x}" if j % 2 else fr"\{value:03o}"
                     for j, value in enumerate(self.payload[i:i + 7])]
            parts.append('"' + "".join(forms) + '"')
        source = "/* A comment with a quote: \" */\nconst char value[] =\n"
        source += " /* between literals */ \\\n".join(parts) + ";\n"
        result = self.check("example.c", source, 1)
        self.assertIn("example.c:3: 61 decoded literal bytes", result.stdout)

    def test_distinct_c_expressions_and_comments_do_not_join(self):
        parts = ['"' + "".join(fr"\{v:03o}" for v in self.payload[i:i + 8]) + '"'
                 for i in range(0, len(self.payload), 8)]
        source = "\n".join(f"const char s{i}[] = {part};" for i, part in enumerate(parts))
        source += "\n/* " + " ".join(parts) + " */\n"
        self.check("example.h", source, 0)

    def test_short_sources_and_separate_python_statements_pass(self):
        source = "\n".join(f"x{i} = {self.payload[i:i + 12]!r}"
                           for i in range(0, len(self.payload), 12))
        self.check("example.py", source, 0)

    def test_uncertain_python_parse_is_reported(self):
        result = self.check("broken.py", "example = (\n", 2)
        self.assertIn("broken.py: could not inspect source literals", result.stderr)


if __name__ == "__main__":
    unittest.main()
