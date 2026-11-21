#!/usr/bin/env python3
"""Refuse to ship retail content.

Checks every file under a staging directory:
  names   no disc images, boot executables or memory-card saves
          (*.cue *.bin *.iso *.img *.chd *.ccd *.mds *.mdf *.mcd *.mcr *.srm,
          SLUS_*/SCUS_*/SLPS_*/SLES_* style names, SYSTEM.CNF)
  hashes  no file identical (SHA-256) to the user's EXE or to any file
          extracted from their disc (--exe, --extracted)
  runs    no file containing a run of >= --run-bytes (default 64) consecutive
          bytes of the EXE (or of --run-source files). Low-entropy stretches
          (padding, zero fill) are ignored, so common byte patterns don't trip it.
          Python literal values and ordinary C string literals are checked after
          escape processing and implicit concatenation, as well as raw file bytes.

The references stay on the user's machine; only hashes and windows are held
in memory. Exit status 0 = clean, 1 = retail content found, 2 = usage.
"""
from __future__ import annotations

import argparse
import ast
from bisect import bisect_right
import hashlib
import os
import re
import sys
import warnings
from pathlib import Path

BAD_EXT = {".cue", ".bin", ".iso", ".img", ".chd", ".ccd", ".mds", ".mdf",
           ".mcd", ".mcr", ".srm", ".gme", ".psv"}
BAD_NAME = re.compile(r"^(S[CL][UEP][SMS]|SLKA|SCPS|SLPM|PAPX)[_-]?\d{3}\.?\d{2}(;1)?$", re.I)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def low_entropy(window: bytes) -> bool:
    return len(set(window)) < 8


class RunIndex:
    """Verify exact byte matches using aligned seeds and their distinguishing context."""

    def __init__(self, run: int):
        self.run = max(8, run)
        self.w = self.run // 2
        self.windows = {}
        self.sources = []

    @staticmethod
    def _outside(data, start, end, alphabet):
        """Find the first value outside the seed alphabet on each side."""
        left, right = start, end
        step = min(64, end - start)
        while left:
            chunk = data[max(0, left - step):left]
            outside = chunk.rstrip(alphabet)
            left -= len(chunk) - len(outside)
            if outside:
                break
            step = min(65536, step * 2)
        step = min(64, end - start)
        while right < len(data):
            chunk = data[right:right + step]
            outside = chunk.lstrip(alphabet)
            right += len(chunk) - len(outside)
            if outside:
                break
            step = min(65536, step * 2)
        return left, right

    def _children(self, data, start, end, alphabet):
        if len(alphabet) < 8:
            left, right = self._outside(data, start, end, alphabet)
            if left:
                yield left - 1, end
            if right < len(data):
                yield start, right + 1
        else:
            extra = (self.run - (end - start) + 1) // 2
            if start >= extra:
                yield start - extra, end
            if end + extra <= len(data):
                yield start, end + extra

    def _key(self, data, start, end):
        # Sparse diverse matches may cover a long run of padding. Hold a view
        # of such a source span, sharing the bytes already kept for verification.
        return data[start:end] if end - start <= self.run else memoryview(data)[start:end]

    def _remember(self, sid, start, end):
        source = self.sources[sid][1]
        key = self._key(source, start, end)
        location = sid, start
        old = self.windows.setdefault(key, location)
        if old == location:
            return
        alphabet = bytes(set(key))
        if len(key) >= self.run and len(alphabet) >= 8:
            return  # Either location is already a complete qualifying witness.
        if old is not None:
            old_sid, old_start = old
            old_source = self.sources[old_sid][1]
            old_end = old_start + len(key)
            # Equal surrounding threshold windows with enough diversity on
            # both sides accept the same matches through this seed.
            extra = max(0, self.run - len(key))
            if (len(alphabet) >= 8 and start >= extra and old_start >= extra and
                    end + extra <= len(source) and old_end + extra <= len(old_source) and
                    source[start - extra:end + extra] == old_source[old_start - extra:old_end + extra]):
                return
            self.windows[key] = None
            for a, b in self._children(old_source, old_start, old_end, alphabet):
                self._remember(old_sid, a, b)
        for a, b in self._children(source, start, end, alphabet):
            self._remember(sid, a, b)

    def add(self, name: str, data: bytes) -> None:
        if len(data) < self.run or low_entropy(data):
            return
        sid = len(self.sources)
        self.sources.append((name, data))
        covered = {}
        for start in range(0, len(data) - self.w + 1, self.w):
            end = start + self.w
            alphabet = bytes(sorted(set(data[start:end])))
            if len(alphabet) >= 8:
                self._remember(sid, start, end)
                continue
            # A qualifying interval contains an aligned block. If that block
            # uses too few values, the interval must cross a region boundary.
            if start < covered.get(alphabet, 0):
                continue
            left, right = self._outside(data, start, end, alphabet)
            if left:
                self._remember(sid, left - 1, left - 1 + self.w)
            if right < len(data):
                self._remember(sid, right + 1 - self.w, right + 1)
            covered[alphabet] = right
            if len(covered) > 256:
                covered = {symbols: limit for symbols, limit in covered.items() if limit > start}

    def _match(self, data, start, end, lower, hit):
        if hit is None:
            # An exact match must cover the first new byte on either side, or
            # (when already diverse) at least half of the missing length on one
            # side. Every refinement therefore gains diversity or length.
            key = self._key(data, start, end)
            for a, b in self._children(data, start, end, bytes(set(key))):
                if a < lower:
                    continue
                child = self._key(data, a, b)
                if child in self.windows:
                    matched = self._match(data, a, b, lower, self.windows[child])
                    if matched:
                        return matched
            return None
        sid, off = hit
        name, source = self.sources[sid]
        tail = off + end - start
        while start > lower and off and data[start - 1] == source[off - 1]:
            start -= 1
            off -= 1
        while end < len(data) and tail < len(source) and data[end] == source[tail]:
            end += 1
            tail += 1
        if end - start >= self.run and not low_entropy(data[start:end]):
            return start, name, off, end - start
        return None

    def scan(self, data: bytes):
        """Yield (staged_offset, source_name, source_offset, run_length)."""
        if len(data) < self.run or low_entropy(data):
            return
        lower = 0
        absent = object()
        for pos in range(len(data) - self.w + 1):
            if pos < lower:
                continue
            end = pos + self.w
            hit = self.windows.get(data[pos:end], absent)
            if hit is not absent:
                match = self._match(data, pos, end, lower, hit)
                if match:
                    yield match
                    lower = match[0] + match[3]


C_TOKENS = re.compile(
    rb"""(?P<comment>/\*.*?(?:\*/|\Z)|//[^\n]*)|"""
    rb"""(?P<string>(?:(?<!\w)(?:u8|u|U|L))?"(?:\\.|[^"\\])*")|"""
    rb"""'(?:\\.|[^'\\])*'""", re.DOTALL)
C_ESCAPES = re.compile(rb"\\(?:[0-7]{1,3}|x[0-9a-fA-F]+|u[0-9a-fA-F]{4}|U[0-9a-fA-F]{8}|.)",
                       re.DOTALL)
C_SIMPLE = dict(zip(b"abfnrtv", b"\a\b\f\n\r\t\v"))


def c_escape(match: re.Match[bytes]) -> bytes:
    value = match.group()[1:]
    if value[0] in b"01234567":
        return bytes([int(value, 8) & 0xff])
    if value.startswith(b"x"):
        return bytes([int(value[1:], 16) & 0xff])
    if value[:1] in (b"u", b"U"):
        return chr(int(value[1:], 16)).encode("utf-8")
    if value[0] in C_SIMPLE:
        return bytes([C_SIMPLE[value[0]]])
    if value in (b"?", b"'", b'"', b"\\"):
        return value
    # Keep implementation-defined escapes in their source spelling.
    return match.group()


def source_literals(path: Path, data: bytes):
    """Yield (physical line, value) for static Python or narrow C literals."""
    if path.suffix.lower() == ".py":
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", SyntaxWarning)
            module = ast.parse(data, filename=str(path))
        for node in ast.walk(module):
            if isinstance(node, ast.Constant) and isinstance(node.value, (str, bytes)):
                value = node.value
                if isinstance(value, str):
                    value = value.encode("utf-8", "surrogatepass")
                yield node.lineno, value
    elif path.suffix.lower() in (".c", ".h"):
        # C splices physical lines before tokenizing, including inside comments.
        logical = bytearray()
        lines = []
        for line in data.splitlines(keepends=True):
            lines.append(len(logical))
            logical.extend(re.sub(rb"\\\r?\n\Z", b"", line))
        end = 0
        chunks = []
        first_line = 1
        for token in C_TOKENS.finditer(logical):
            kind = token.lastgroup
            narrow = kind == "string" and token.group().startswith((b'"', b'u8"'))
            if chunks and (logical[end:token.start()].strip() or (kind != "comment" and not narrow)):
                yield first_line, b"".join(chunks)
                chunks.clear()
            if narrow:
                if not chunks:
                    first_line = bisect_right(lines, token.start())
                spelling = token.group()
                body = spelling[spelling.index(b'"') + 1:-1]
                chunks.append(C_ESCAPES.sub(c_escape, body))
            end = token.end()
        if chunks:
            yield first_line, b"".join(chunks)

def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--stage", required=True, type=Path)
    ap.add_argument("--exe", type=Path, action="append", default=[],
                    help="the user's boot EXE (SLUS_007.26); repeatable")
    ap.add_argument("--extracted", type=Path, action="append", default=[],
                    help="directory of files extracted from the user's disc; repeatable")
    ap.add_argument("--run-source", type=Path, action="append", default=[],
                    help="extra files to run-check against (besides --exe)")
    ap.add_argument("--run-bytes", type=int, default=64)
    ap.add_argument("--allow-no-reference", action="store_true",
                    help="only run the name check when no retail reference is given")
    args = ap.parse_args(argv)

    if not args.stage.is_dir():
        print(f"retail_guard: {args.stage} is not a directory", file=sys.stderr)
        return 2
    has_references = bool(args.exe or args.extracted or args.run_source)
    if not has_references and not args.allow_no_reference:
        print("retail_guard: give --exe, --extracted or --run-source (the user's own files), "
              "or --allow-no-reference for a name-only check", file=sys.stderr)
        return 2

    ref_hashes: dict[str, str] = {}
    for exe in args.exe:
        ref_hashes[sha256(exe)] = str(exe)
    for d in args.extracted:
        for p in sorted(d.rglob("*")):
            if p.is_file() and p.stat().st_size > 0:
                ref_hashes.setdefault(sha256(p), str(p))
    index = RunIndex(args.run_bytes)
    for src in list(args.exe) + list(args.run_source):
        index.add(str(src), src.read_bytes())

    problems = []
    errors = []
    staged = sorted(p for p in args.stage.rglob("*") if p.is_file())
    for p in staged:
        rel = p.relative_to(args.stage)
        if p.suffix.lower() in BAD_EXT or BAD_NAME.match(p.name) or p.name.upper() == "SYSTEM.CNF":
            problems.append(f"{rel}: disc image / retail file name")
        if ref_hashes:
            h = sha256(p)
            if h in ref_hashes:
                problems.append(f"{rel}: identical to retail file {ref_hashes[h]}")
                continue
        if index.windows:
            data = p.read_bytes()
            match = next(index.scan(data), None)
            if match:
                off, src, soff, length = match
                problems.append(f"{rel}: {length} bytes at 0x{off:x} match {src} at 0x{soff:x}")
                continue
            try:
                for line, literal in source_literals(p, data):
                    if len(literal) < index.run:
                        continue
                    match = next(index.scan(literal), None)
                    if match:
                        off, src, soff, length = match
                        problems.append(f"{rel}:{line}: {length} decoded literal bytes at "
                                        f"0x{off:x} match {src} at 0x{soff:x}")
                        break
            except (SyntaxError, UnicodeError, ValueError) as exc:
                errors.append(f"{rel}: could not inspect source literals ({type(exc).__name__})")
    for line in problems:
        print("RETAIL CONTENT: " + line)
    for line in errors:
        print("retail_guard: " + line, file=sys.stderr)
    print(f"retail_guard: {len(staged)} files checked, {len(problems)} problem(s)"
          + (f", {len(errors)} scan error(s)" if errors else "")
          + ("" if has_references else " (name check only)"))
    return 2 if errors else int(bool(problems))


if __name__ == "__main__":
    sys.exit(main())
