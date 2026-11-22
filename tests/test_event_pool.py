"""Check complete game event pools, including requests overlapping a destination slot."""
from hashlib import sha256
from pathlib import Path
import json
import random
import shutil
import struct
import subprocess
import tempfile

import pytest


HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
POOL = 0x8011F9D0
STRIDE = 104
SLOTS = 20
NO_ALIAS = 0xFFFFFFFF
PARTITIONS = ((8, 20), (0, 8))

# Source offset, slot offset, and width in the original store order.
FIELDS = (
    (8, 76, 4), (0, 0, 2), (2, 6, 2), (4, 10, 2),
    (6, 14, 2), (12, 80, 4), (16, 84, 4),
)
COMPILERS = {
    "gcc": ("gcc", ["-O3", "-flto", "-Werror=lto-type-mismatch"]),
    "clang": ("clang", ["-O3", "-flto"]),
    "ubsan": ("gcc", ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
}
SOURCES = (
    HERE / "event_pool_probe.c",
    ROOT / "src/overlays/main_0012/80146750.c",
    ROOT / "src/overlays/main_0012/8014680c.c",
)
HEADERS = (ROOT / "include/forest_event.h", ROOT / "include/scene_position_slot.h")


def generate():
    rng = random.Random(771249)
    for partition, (begin, end) in enumerate(PARTITIONS):
        for mask in range(1 << (end - begin)):
            pool = bytearray(rng.randbytes(STRIDE * SLOTS))
            for index in range(begin, end):
                occupied = mask & (1 << (index - begin))
                kind = rng.randrange(1, 65536) if occupied else 0
                struct.pack_into("<H", pool, index * STRIDE, kind)
            request = bytearray(rng.randbytes(20))
            if mask % 17 == 0:
                request[:2] = bytes(2)
            yield partition, NO_ALIAS, 1 + mask % 3, bytes(pool), bytes(request)

        for free in range(begin, end):
            for displacement in range(-20, 108, 4):
                address = free * STRIDE + displacement
                if not 0 <= address <= STRIDE * SLOTS - 20:
                    continue
                pool = bytearray(rng.randbytes(STRIDE * SLOTS))
                for index in range(begin, free):
                    struct.pack_into("<H", pool, index * STRIDE, index + 1)
                struct.pack_into("<H", pool, free * STRIDE, 0)
                yield partition, address, 2, bytes(pool), rng.randbytes(20)


def evaluate(case):
    partition, alias, times, initial, request = case
    pool = bytearray(initial)
    source = request if alias == NO_ALIAS else pool
    base = 0 if alias == NO_ALIAS else alias
    begin, end = PARTITIONS[partition]
    returns = bytearray()
    for _ in range(times):
        for index in range(begin, end):
            slot = index * STRIDE
            if struct.unpack_from("<H", pool, slot)[0]:
                continue
            for offset, target, width in FIELDS:
                src = base + offset
                dst = slot + target
                pool[dst:dst + width] = source[src:src + width]
            returns.extend(struct.pack("<I", POOL + slot))
            break
        else:
            returns.extend(bytes(4))
    return returns + pool + request


def make_inputs():
    cases = list(generate())
    inputs = bytearray()
    expected = bytearray()
    for case in cases:
        partition, alias, times, pool, request = case
        inputs.extend(struct.pack("<3I", partition, alias, times))
        inputs.extend(pool)
        inputs.extend(request)
        expected.extend(evaluate(case))
    return cases, inputs, expected


def check_compiler(name, types_include, inputs, expected):
    compiler, flags = COMPILERS[name]
    with tempfile.TemporaryDirectory() as temporary:
        binary = Path(temporary) / name
        subprocess.run([
            compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", *flags,
            "-I", ROOT / "include", "-I", types_include, *SOURCES, "-o", binary,
        ], check=True)
        result = subprocess.run([binary], input=inputs, capture_output=True, check=True)
        assert result.stdout == expected
        assert not result.stderr
        return {
            "output_sha256": sha256(result.stdout).hexdigest(),
            "elf_sha256": sha256(binary.read_bytes()).hexdigest(),
        }


@pytest.fixture(scope="module")
def pool_inputs():
    return make_inputs()


@pytest.mark.parametrize("compiler", COMPILERS)
def test_event_pool(compiler, pool_inputs, tmp_path):
    command, flags = COMPILERS[compiler]
    if not shutil.which(command):
        pytest.skip(f"{command} is unavailable")
    supported = subprocess.run([
        command, *flags, "-x", "c", "-", "-o", tmp_path / "check_flags",
    ], input=b"int main(void) { return 0; }", capture_output=True)
    if supported.returncode:
        pytest.skip(f"{command} cannot link with {' '.join(flags)}")
    _, inputs, expected = pool_inputs
    check_compiler(compiler, ROOT / "include", inputs, expected)


def main():
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--types-include", type=Path, default=ROOT / "include")
    args = parser.parse_args()
    cases, inputs, expected = make_inputs()
    builds = {
        name: check_compiler(name, args.types_include, inputs, expected)
        for name in COMPILERS
    }
    record = {
        "cases": len(cases),
        "calls": sum(case[2] for case in cases),
        "distinct_external_occupancy_patterns": sum(case[1] == NO_ALIAS for case in cases),
        "overlapping_input_cases": sum(case[1] != NO_ALIAS for case in cases),
        "builds": builds,
        "source_digests": {
            str(path.relative_to(ROOT)): sha256(path.read_bytes()).hexdigest()
            for path in (*SOURCES, *HEADERS, Path(__file__))
        },
    }
    if args.receipt:
        args.receipt.write_text(json.dumps(record, indent=2) + "\n")
    print(record["cases"], record["calls"], builds["gcc"]["output_sha256"])


if __name__ == "__main__":
    main()
