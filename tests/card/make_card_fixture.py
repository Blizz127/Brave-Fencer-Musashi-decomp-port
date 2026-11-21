"""Build an invented raw card and independently retain one file's logical bytes."""
from functools import reduce
from operator import xor
from pathlib import Path
from struct import pack_into

FRAME = 128
BLOCK = 8192
PRIMARY = b"SYNTHETIC-ENTRY-0001"
assert len(PRIMARY) == 20
card = bytearray((i * 37 + i // 101 + 13) % 256 for i in range(16 * BLOCK))


def write_record(frame, state, allocated, following, name=b""):
    base = frame * FRAME
    pack_into("<IIH", card, base, state, allocated, following)
    card[base + 10:base + 31] = name.ljust(21, b"\0")
    finish_record(frame)


def finish_record(frame):
    base = frame * FRAME
    card[base + 127] = reduce(xor, card[base:base + 127])


card[:2] = b"MC"
finish_record(0)
for slot in range(1, 16):
    write_record(slot, 160, 0, 65535)

# One deleted tail has obsolete linkage. Its block can still be reclaimed.
write_record(1, 163, 0, 9)
write_record(2, 81, BLOCK, 65535, b"SECOND")
write_record(3, 81, 3 * BLOCK, 13, PRIMARY)
write_record(14, 82, 0, 4)
write_record(5, 83, 0, 65535)
write_record(6, 162, 0, 11)

expected = b"".join(card[i * BLOCK:(i + 1) * BLOCK] for i in (3, 14, 5))
# Cover the header, two directory entries, and two payload sectors.
mapping = [0, 3, 14 * 64, 64, 1]
for idx in range(20):
    sector = mapping[idx] if idx < len(mapping) else 0xffffffff
    pack_into("<I", card, (16 + idx) * FRAME, sector)
    finish_record(16 + idx)
    if idx < len(mapping):
        begin = sector * FRAME
        replacement = (36 + idx) * FRAME
        card[replacement:replacement + FRAME] = card[begin:begin + FRAME]
        card[begin:begin + FRAME] = bytes([0xd0 + idx]) * FRAME

Path("synthetic-card.bin").write_bytes(card)
Path("synthetic-file.bin").write_bytes(expected)
