#!/usr/bin/env python3
"""Generate the narrowly adapted PsyCross GTE TU without editing its checkout."""
import argparse
from pathlib import Path


# The NCLIP command already clears FLAG at entry to GTE_operator. Clearing
# it again after the area calculation discards the MAC0 overflow flags.
NCLIP_END = "#endif\n\t\tC2_FLAG = 0;\n\t\treturn 1;\n\n\tcase 0x0c:"
NCLIP_KEEP_FLAGS = NCLIP_END.replace("\t\tC2_FLAG = 0;\n", "")


def prepare(source: str) -> str:
    # Signed int32 translations times 4096 fit int64. Left-shifting a
    # negative signed value is undefined C++; multiplication is defined.
    for selector in (1, 2, 3):
        original = f"(long long)CV{selector}(cv) << 12"
        if source.count(original) != 2:
            raise ValueError(f"expected exactly two {original!r} occurrences")
    for selector in (1, 2, 3):
        source = source.replace(f"(long long)CV{selector}(cv) << 12",
                                f"(long long)CV{selector}(cv) * 4096LL")
    reset = "C2_FLAG = 0;\n\n\tswitch (GTE_FUNCT(gteop(op)))"
    if source.count(reset) != 1 or source.count(NCLIP_END) != 1:
        raise ValueError("expected pinned GTE command and NCLIP flag resets")
    return source.replace(NCLIP_END, NCLIP_KEEP_FLAGS)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        generated = prepare(args.source.read_text())
    except ValueError as error:
        parser.error(str(error))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if not args.output.exists() or args.output.read_text() != generated:
        args.output.write_text(generated)


if __name__ == "__main__":
    main()
