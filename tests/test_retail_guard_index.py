"""Check the staged-file scanner with wholly generated reference data."""
import random
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import retail_guard


def has_common_run(source, staged, minimum):
    """Walk all alignments independently of the indexed search."""
    for offset in range(1 - len(source), len(staged)):
        length = 0
        values = set()
        for src in range(max(0, -offset), min(len(source), len(staged) - offset)):
            if source[src] != staged[src + offset]:
                length = 0
                values.clear()
                continue
            length += 1
            values.add(source[src])
            if length >= minimum and len(values) >= 8:
                return True
    return False


class RunIndexTest(unittest.TestCase):
    def check_scan(self, refs, staged, minimum):
        index = retail_guard.RunIndex(minimum)
        for name, source in refs.items():
            index.add(name, source)
        hits = list(index.scan(staged))
        self.assertEqual(bool(hits), any(has_common_run(s, staged, minimum) for s in refs.values()))
        last = 0
        for pos, name, src, length in hits:
            self.assertGreaterEqual(pos, last)
            self.assertLessEqual(pos + length, len(staged))
            self.assertGreaterEqual(src, 0)
            self.assertLessEqual(src + length, len(refs[name]))
            self.assertGreaterEqual(length, minimum)
            self.assertEqual(staged[pos:pos + length], refs[name][src:src + length])
            self.assertGreaterEqual(len(set(staged[pos:pos + length])), 8)
            last = pos + length

    def test_repeated_seeds_and_alignment(self):
        for limit in (8, 9, 15, 16, 31, 32, 63, 64):
            with self.subTest(limit=limit):
                width = limit // 2
                sequence = random.Random(limit).randbytes(limit + 1)
                decoy = b"".join(sequence[i:i + width] + bytes([241]) * width
                                 for i in range(0, len(sequence), width))
                # The first copy of every full aligned seed has the wrong neighbours.
                refs = {"first": decoy, "second": sequence}
                self.check_scan(refs, sequence, limit)
                self.check_scan({"one": decoy + bytes([247]) * width + sequence}, sequence, limit)
                for prefix in range(width):
                    fragment = sequence[:limit]
                    source = bytes([227]) * prefix + fragment + bytes([229]) * width
                    self.check_scan({"one": source}, fragment, limit)
                    self.check_scan({"one": source}, fragment[:-1], limit)

    def test_diversity_across_padding(self):
        rng = random.Random(195)
        marks = rng.sample(range(32, 224), 8)
        for padding in (bytes(96), (bytes([3, 5, 7]) * 160)):
            with self.subTest(padding=len(padding)):
                witness = bytes(marks[:4]) + padding + bytes(marks[4:])
                self.check_scan({"reference": witness}, witness, 64)
                self.check_scan({"reference": witness}, padding, 64)
                self.check_scan({"reference": padding}, witness, 64)

    def test_independent_generated_oracle(self):
        rng = random.Random(83711)
        for case in range(140):
            limit = rng.choice((8, 13, 16, 23, 32))
            pieces = [rng.randbytes(rng.randrange(3, 18)), bytes(rng.randrange(5, 16)),
                      bytes([11, 13]) * rng.randrange(3, 10), rng.randbytes(9)]
            ref = b"".join(rng.choices(pieces, k=7))
            if case % 3:
                start = rng.randrange(len(ref))
                sample = rng.randbytes(3) + ref[start:start + limit + rng.randrange(-2, 12)]
            else:
                sample = b"".join(rng.choices(pieces, k=3))
            self.check_scan({"reference": ref}, sample, limit)


if __name__ == "__main__":
    unittest.main()
