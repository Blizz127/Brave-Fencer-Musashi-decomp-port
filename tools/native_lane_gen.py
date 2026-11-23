#!/usr/bin/env python3
"""Build the native-lane table from byte-matched decomp C (docs/PC-PORT.md,
"Native lane").

For each main-executable registry entry with recovery "c" whose source has
a native word export (so the interpreter can always run it as a fallback),
compile the decomp C branch with clang for the 64-bit host, rewrite every
guest symbol to its guest address, and admit the function only when an
IR-level checker proves it pointer-width safe under the "guest RAM mapped
at host 0x80000000" model. Admitted functions become lane_func_<ADDR> in an
archive; a generated C file holds the register-ABI wrappers, interpreter
thunks for every unported callee, and the MusashiNativeLaneEntry table.

Rules (each rejection names the first rule that failed, in report.json):
  source  : no 'long' or 'sizeof' tokens (their widths differ on the host)
  compile : clang must accept the source (implicit declarations are fatal)
  symbols : guest data/function symbols must resolve into game RAM
            [0x80010000, 0x80200000); mutable data defined by the TU is
            refused (it would be a host copy, not the guest's)
  layout  : no struct/union/array type containing a pointer is used, and no
            pointer-element indexing (checked before optimization)
  memory  : no pointer-typed load/store to non-local memory; no atomics
  escape  : no host stack or host data address reaches guest memory, a
            guest callee, an integer, or the return value
  inttoptr: integer->pointer only from zero-extended 32-bit values or
            constants inside game RAM
  calls   : only direct calls to prototyped guest functions (lane_func_*),
            module-internal functions and memory intrinsics; no inline asm
  arith   : no variable or trapping divisors (x86 traps where MIPS does not),
            variable shift counts must be masked to 5 bits
  ub      : no undef/poison operands and no unreachable (UB the host
            optimizer could exploit differently from gcc-2.7.2)
  abi     : parameters/returns limited to 8/16/32-bit integers and pointers;
            no direct call passing fewer arguments than the callee reads
  memory  : (also) no volatile access, which in decomp C means a device
  irq     : no loop that neither stores nor calls (a native call runs
            without IRQ delivery, so a polling loop could spin forever)
  hooks   : the function's guest range contains no PC the port observes or
            intercepts (0x80xxxxxx literals on pc/ra/continuation/case lines
            in pc_port/*.c), since native execution would bypass the hook
"""
import argparse
import concurrent.futures
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from psyq_manifest import sdk_overlay

RAM_LO, RAM_HI = 0x80010000, 0x80200000
HOST_TARGET = "x86_64-unknown-linux-gnu"
# The lane objects' triple (--host-target). Every check runs on HOST_TARGET's
# IR whatever it is: the SysV signext/zeroext attributes there carry the C
# signedness of narrow arguments and returns, which the table, the call stubs
# and to_guest() need, and Win64 IR does not record. Only the object is
# compiled for this target (object_ir()).
OBJECT_TARGET = HOST_TARGET
def object_elf():
    """ELF objects are position independent; a Windows (COFF) one has no PIE
    model and instead needs unwind data (see build_one)."""
    return "windows" not in OBJECT_TARGET
def pie():
    return ["-fPIE"] if object_elf() else []



def load_matches(path=None):
    if path is None:
        path = ROOT / "provenance/matches.json"
        if not path.is_file():
            path = ROOT / "provenance/native-main.json"
    return json.loads(path.read_text())["matches"]


def host_long(text):
    """PS1 `long` is 32-bit: rewrite lone `long` / `long int` to `int` for the
    LP64 host, keeping `long long` (64-bit on both)."""
    text = re.sub(r"\blong\s+long\b", "\0LL\0", text)
    text = re.sub(r"\blong(\s+int)?\b", "int", text)
    return text.replace("\0LL\0", "long long")


def lower_arith(ir):
    """MIPS semantics for i32 shifts and divisions with a variable operand:
    shift counts are masked to 5 bits (SLLV/SRLV/SRAV use the low 5 bits;
    LLVM makes >= 32 poison), and sdiv/udiv/srem/urem call the guarded
    helpers (a trapping divisor aborts the native call instead of faulting
    the host or inventing a quotient). Constant operands are left alone."""
    out, n = [], 0
    used = set()
    for line in ir.splitlines():
        m = re.match(r"^(\s*)(%[\w.$-]+) = (shl|lshr|ashr)((?: nuw| nsw| exact)*) i32 (.+)$", line)
        if m:
            a, b = split_top(m.group(5))
            if not re.fullmatch(r"-?\d+", b.strip()):
                n += 1
                amt = f"%lane.amt{n}"
                out.append(f"{m.group(1)}{amt} = and i32 {b.strip()}, 31")
                line = f"{m.group(1)}{m.group(2)} = {m.group(3)} i32 {a.strip()}, {amt}"
        m = re.match(r"^(\s*)(%[\w.$-]+) = (sdiv|udiv|srem|urem)(?: exact)? i32 (.+)$", line)
        if m:
            a, b = split_top(m.group(4))
            if not re.fullmatch(r"-?\d+", b.strip()) or int(b) in (0, -1):
                fn = {"sdiv": "lane_sdiv32", "srem": "lane_srem32",
                      "udiv": "lane_udiv32", "urem": "lane_urem32"}[m.group(3)]
                used.add(fn)
                line = f"{m.group(1)}{m.group(2)} = call i32 @{fn}(i32 {a.strip()}, i32 {b.strip()})"
        out.append(line)
    for fn in sorted(used):
        out.append(f"declare i32 @{fn}(i32, i32)")
    return "\n".join(out) + "\n"


def split_operands(text):
    """'"=r"(a), "=r"(b)' -> [('=r', 'a'), ('=r', 'b')]"""
    out = []
    for part in split_top(text, ","):
        m = re.match(r'\s*"([^"]*)"\s*\((.*)\)\s*$', part, flags=re.S)
        if not part.strip():
            continue
        if not m:
            return None
        out.append((m.group(1), re.sub(r"\s+", "", m.group(2))))
    return out


def strip_empty_asm(text):
    """Drop empty-template asm statements that are identities on the host:
    no outputs, or every output tied to an input ("N") with the same
    expression; clobbers (incl. "memory") only steer MIPS scheduling. Returns
    (text, dropped) or None when an empty asm has an untied output (its value
    would be whatever the MIPS register held)."""
    pat = re.compile(r'\b(?:__asm__|__asm|asm)\s*(?:__volatile__|volatile)?\s*\(\s*""\s*')
    pos, pieces, dropped = 0, [], 0
    for m in pat.finditer(text):
        if m.start() < pos:
            continue
        depth, i = 1, text.index("(", m.start()) + 1
        while i < len(text) and depth:
            depth += {"(": 1, ")": -1}.get(text[i], 0)
            i += 1
        j = i
        while j < len(text) and text[j] in " \t\n":
            j += 1
        if j >= len(text) or text[j] != ";":
            return None
        inner = text[m.end():i - 1]
        sections = split_top(inner, ":")
        outs = split_operands(sections[1]) if len(sections) > 1 else []
        ins = split_operands(sections[2]) if len(sections) > 2 else []
        if outs is None or ins is None:
            return None
        for k, (cons, expr) in enumerate(outs):
            if not any(c == str(k) and e == expr for c, e in ins):
                return None
        pieces.append(text[pos:m.start()])
        pieces.append("/* empty asm removed for the host */")
        pos = j + 1
        dropped += 1
    pieces.append(text[pos:])
    return "".join(pieces), dropped


# Host intrinsics (mips_formatter.c formatter_call) whose matched C may run
# in the lane instead of the bespoke host implementation: their only port
# reference is the entry PC the intrinsic dispatch compares, and their file's
# only bespoke macro is a *_BINDING for that host implementation (compiled
# here without it, i.e. the matched body).
INTRINSIC_PCS = {0x80016714, 0x8001903C, 0x80029044, 0x8002906C, 0x80029094, 0x80029218}

CFLAGS = [
    f"--target={HOST_TARGET}", "-std=gnu89", "-fPIE", "-fwrapv",
    "-fno-strict-aliasing", "-fno-builtin", "-ffreestanding",
    "-ftrivial-auto-var-init=zero", "-fno-delete-null-pointer-checks",
    "-w", "-Werror=implicit-function-declaration", "-Werror=implicit-int",
    "-Wno-error=int-conversion", "-Wno-int-conversion",
    "-Wno-error=incompatible-pointer-types",
]


def object_ir(ir):
    """HOST_TARGET IR -> the module compiled for OBJECT_TARGET. Lane
    signatures are integers and pointers only (abi_type), and `long` is
    already int (host_long), so the one difference left is the SysV promise
    that a caller extends narrow arguments and returns. A Win64 caller (the
    table's dispatcher, compiled C) makes no such promise, so the callee must
    not assume it; the triple and layout come from --target."""
    if object_elf():
        return ir
    ir = re.sub(r"^target (?:datalayout|triple) = .*\n", "", ir, flags=re.M)
    return re.sub(r" (?:signext|zeroext)\b", "", ir)


SYM_RE = re.compile(r'@(?:"([^"]+)"|([A-Za-z_.$][\w.$]*))')
GLOBAL_DECL_RE = re.compile(r'^@(?:"([^"]+)"|([\w.$]+)) = (.*)$', re.M)
DEF_RE = re.compile(r"^\s*(%[\w.\"$]+) = (.*)$")


class Reject(Exception):
    def __init__(self, rule, detail):
        super().__init__(f"{rule}: {detail}")
        self.rule, self.detail = rule, detail


# Overlay members (MAIN.CD main_NNNN, scene scNN_NNNN) share load addresses,
# so their lane symbols carry the member: lane_main_0003__func_800D27A0.
QFUNC = r"(?:[a-z]+\d*_\d{4}__)?func_[0-9A-Fa-f]{8}"


def qualify(member, name):
    return f"{member}__{name}" if member else name


def fnv1a64(data):
    h = 0xCBF29CE484222325
    for b in data:
        h = ((h ^ b) * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return h


def member_info(member):
    """From config/overlay_<member>.yaml: the member file (the user's local
    extract), its vram/start mapping, and code ranges (asm/c subsegments).
    None when the config or the file is missing."""
    cfg = ROOT / "config" / f"overlay_{member}.yaml"
    if not cfg.exists():
        return None
    text = cfg.read_text(errors="replace")
    seg = re.search(r"-\s*name:\s*(\w+)\s*\n\s*type:\s*code\s*\n\s*start:\s*(0x[0-9A-Fa-f]+)"
                    r"\s*\n\s*vram:\s*(0x[0-9A-Fa-f]+)", text)
    tp = re.search(r"target_path:\s*(\S+)", text)
    if not seg or not tp:
        return None
    start, vram = int(seg.group(2), 16), int(seg.group(3), 16)
    subs = [(int(m.group(1), 16), m.group(2)) for m in
            re.finditer(r"^\s*-\s*\[\s*(0x[0-9A-Fa-f]+)\s*(?:,\s*(\w+))?", text, flags=re.M)]
    code = [(vram + o - start, vram + n[0] - start) for (o, k), n in zip(subs, subs[1:])
            if k in ("asm", "c", "hasm")]
    path = ROOT / tp.group(1)
    if not path.exists() or not code:
        return None
    data = path.read_bytes()
    digest = re.search(r"^sha1:\s*([0-9a-fA-F]{40})\s*$", text, flags=re.M)
    if not digest or hashlib.sha1(data).hexdigest() != digest.group(1).lower():
        raise ValueError(f"The extracted overlay does not match its configuration: {member}")
    return dict(member=member, path=path, vram=vram, start=start, code=code, data=data)


MAIN_EXE = ROOT / "extracted/disc/files/SLUS_007.26"
_MAIN_EXE_BYTES = None
IO_LO, IO_HI = 0x1F801000, 0x1F803000


def guest_word(addr, info=None):
    """Initial 32-bit word at a guest address, from the user's own local
    extract: the member file for overlay data, else the main EXE (loaded at
    0x80010000 after its 0x800-byte header). None when unknown."""
    global _MAIN_EXE_BYTES
    if info:
        off = addr - info["vram"] + info["start"]
        if 0 <= off <= len(info["data"]) - 4:
            return int.from_bytes(info["data"][off:off + 4], "little")
    if _MAIN_EXE_BYTES is None:
        try:
            data = MAIN_EXE.read_bytes()
        except FileNotFoundError:
            _MAIN_EXE_BYTES = b""
        else:
            if hashlib.sha256(data).hexdigest() != (
                    "66371c3a7517e9eabd7cb6cf0c5abffe7296bd4bac29c85b8bf7bb9db349714a"):
                raise ValueError("The extracted SLUS_007.26 does not match the USA executable")
            _MAIN_EXE_BYTES = data
    off = addr - 0x80010000 + 0x800
    if 0x800 <= off <= len(_MAIN_EXE_BYTES) - 4:
        return int.from_bytes(_MAIN_EXE_BYTES[off:off + 4], "little")
    return None


def check_device_pointers(ir, info=None):
    """The decompiled C often keeps device-register pointers in plain globals
    (`extern s32 *D_800DB674`, holding 1F8010F0) that no volatile marks. A
    native body dereferencing them would touch host memory at the MMIO
    address. Refuse a function that loads a guest global whose initial word
    (in the user's EXE or member file) is an I/O register address."""
    for m in re.finditer(r"load (?:i32|ptr), ptr inttoptr \(i64 (\d+) to ptr\)", ir):
        addr = int(m.group(1))
        word = guest_word(addr, info)
        if word is None and not _MAIN_EXE_BYTES:
            raise Reject("memory", f"no executable to check global at {addr:08x}")
        if word is not None and IO_LO <= word < IO_HI:
            raise Reject("memory", f"device pointer {addr:08x} -> {word:08x}")


def member_digest(info, pc, size):
    off = pc - info["vram"] + info["start"]
    if off < 0 or off + size > len(info["data"]):
        return None
    return fnv1a64(info["data"][off:off + size])


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def load_symbol_addrs(member=None):
    """name -> address from tracked config/*syms*.txt / symbol_addrs*.txt.
    With a member, that member's own symbol files come first."""
    table = {}
    paths = sorted((ROOT / "config").glob("*.txt"))
    if member:
        own = [p for p in paths if f".{member}." in p.name]
        paths = own + [p for p in paths if p not in own]
    for path in paths:
        if "sym" not in path.name:
            continue
        for line in path.read_text(errors="replace").splitlines():
            m = re.match(r"\s*([A-Za-z_]\w*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
            if m:
                table.setdefault(m.group(1), int(m.group(2), 16))
    return table


# Where a port reference to a guest PC sits decides what it means for a native
# body starting at that PC:
#  - formatter_admitted: the interpreter's word-source map, not a hook;
#  - cd_irq_call: CD IRQ frame call-edge checks; the lane is suspended in that
#    frame (musashi_boot_execute_cd_irq_with_services), so not a hook;
#  - the startup loop's observer list, native_boot's observe_entry and
#    spu_source_guard run at the top of the stepping loop, before formatter_step
#    tries the lane: a function's own entry PC stays observed. A body PC in
#    them would be skipped, so it is still a hook.
# Anything else (formatter_step intercepts, formatter_call, stops) is a hook.
IGNORED_HOOK_FUNCS = {"formatter_admitted", "cd_irq_call"}
ENTRY_OK_HOOK_FUNCS = {"run_startup_cpu_loop", "observe_entry", "spu_source_guard"}
# Host-sequenced whatever the context: main never returns (the game loop),
# and STARTUP_ENTRY is driven step by step by native_boot's startup.
ALWAYS_HOOKED = {0x80010178, 0x800141F0}


def hooked_pcs():
    """{guest PC: "hard" | "entry"} for PCs the port compares against (CPU pc /
    ra / checkpoint continuation tests and case lists, including enum names).
    "entry" PCs are observed only before the lane runs (see above). Word-table
    range bounds and data addresses are not hooks."""
    kinds = {}
    context = re.compile(r"\bpc\b|->pc|\.pc|r\[31\]|raw_cpu|_caller|\bcase ")
    named = re.compile(r"\b([A-Z][A-Z0-9_]*)\s*=\s*0x(80[0-9a-fA-F]{6})u?\b")
    func_head = re.compile(r"^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s\*]*?\b(\w+)\s*\([^;]*$")
    texts = [path.read_text(errors="replace") for path in (ROOT / "pc_port").glob("*.c")]
    enums = {}
    for text in texts:
        for m in named.finditer(text):
            enums.setdefault(m.group(1), int(m.group(2), 16))

    def add(pc, kind):
        if kinds.get(pc) != "hard":
            kinds[pc] = kind

    for text in texts:
        func = None
        for line in text.splitlines():
            fm = func_head.match(line)
            if fm and not line.startswith((" ", "\t")) and fm.group(1) not in ("if", "while", "for", "switch"):
                func = fm.group(1)
            if not context.search(line) or "Words" in line:
                continue
            if func in IGNORED_HOOK_FUNCS:
                continue
            kind = "entry" if func in ENTRY_OK_HOOK_FUNCS else "hard"
            pcs = [int(m.group(1), 16) for m in re.finditer(r"0x(80[0-9a-fA-F]{6})u?\b", line)]
            if not named.search(line):
                pcs += [enums[i] for i in re.findall(r"\b[A-Z][A-Z0-9_]*\b", line) if i in enums]
            for pc in pcs:
                add(pc, kind)
    for pc in ALWAYS_HOOKED:
        kinds[pc] = "hard"
    return kinds

def guest_address(name, symbols):
    plus = re.fullmatch(r"(.+?)\+(\d+|0x[0-9A-Fa-f]+)", name)
    if plus:  # asm-label aliases such as "D_80078E7C+1"
        base = guest_address(plus.group(1), symbols)
        return None if base is None else base + int(plus.group(2), 0)
    m = re.fullmatch(r"(?:D|func|jtbl|jpt)_([0-9A-Fa-f]{8})", name)
    if m:
        return int(m.group(1), 16)
    return symbols.get(name)


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


# ---- IR helpers ----------------------------------------------------------

def split_top(text, sep=","):
    parts, depth, cur = [], 0, []
    for ch in text:
        if ch in "([{<":
            depth += 1
        elif ch in ")]}>":
            depth -= 1
        if ch == sep and depth == 0:
            parts.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    if cur and "".join(cur).strip():
        parts.append("".join(cur).strip())
    return parts


def named_types(ir):
    types = {}
    for m in re.finditer(r"^(%[\w.\"$]+) = type (.*)$", ir, flags=re.M):
        types[m.group(1)] = m.group(2)
    return types


def type_has_ptr(ty, types, seen=None):
    seen = seen or set()
    if re.search(r"\bptr\b", ty):
        return True
    for name in re.findall(r"%[\w.\"$]+", ty):
        if name in types and name not in seen:
            seen.add(name)
            if type_has_ptr(types[name], types, seen):
                return True
    return False


def functions(ir):
    """Yield (header, [body lines]) for every define."""
    lines = ir.splitlines()
    i = 0
    while i < len(lines):
        if lines[i].startswith("define "):
            header, body = lines[i], []
            i += 1
            while i < len(lines) and lines[i] != "}":
                body.append(lines[i])
                i += 1
            yield header, body
        i += 1


INT_TYPES = {"i8", "i16", "i32"}


def parse_params(text):
    """'(i32 noundef signext %0, ptr %1)' -> [(type, attrs)]"""
    inner = text.strip()
    assert inner[0] == "(" and inner[-1] == ")"
    inner = inner[1:-1].strip()
    if not inner:
        return [], False
    params, variadic = [], False
    for part in split_top(inner):
        if part == "...":
            variadic = True
            continue
        tokens = part.split()
        params.append((tokens[0], set(tokens[1:])))
    return params, variadic


def abi_type(ty, attrs, what):
    if ty == "ptr" or ty in INT_TYPES:
        if attrs & {"byval", "sret", "inalloca", "preallocated"}:
            raise Reject("abi", f"{what} passes {ty} by value")
        return ty, ("signext" in attrs, "zeroext" in attrs)
    raise Reject("abi", f"{what} type {ty}")


def parse_signature(header_or_decl, name):
    """-> (ret_type, ret_ext, [(type, ext)])"""
    m = re.search(r"^(?:define|declare)\s+(.*?)@" + re.escape(name) + r"(\(.*\))", header_or_decl)
    if not m:
        raise Reject("abi", f"cannot parse signature of {name}")
    pre = m.group(1).split()
    # Return type is the last token that is a type; attributes precede it.
    ret = next((t for t in reversed(pre) if t == "void" or t == "ptr" or re.fullmatch(r"i\d+", t)), None)
    if ret is None:
        raise Reject("abi", f"return type of {name}")
    ret_attrs = set(pre)
    # Trim trailing function attributes / section after the parameter list.
    params_text = m.group(2)
    depth = 0
    for idx, ch in enumerate(params_text):
        depth += ch == "("
        depth -= ch == ")"
        if depth == 0:
            params_text = params_text[: idx + 1]
            break
    params, variadic = parse_params(params_text)
    if variadic:
        raise Reject("calls", f"{name} is variadic or unprototyped")
    if ret != "void":
        abi_type(ret, set(), f"{name} return")
    return (ret, ("signext" in ret_attrs, "zeroext" in ret_attrs),
            [abi_type(t, a, f"{name} param") for t, a in params])


# ---- Compile + check one source -------------------------------------------

MEMORY_FNS = ("memset", "memcpy", "memmove")
# Guarded division helpers (mips_formatter.c) that lower_arith() calls.
LANE_HELPERS = ("lane_sdiv32", "lane_srem32", "lane_udiv32", "lane_urem32")


def rewrite_symbols(ir, symbols, member=None, member_code=()):
    """Replace guest data symbols with absolute guest addresses. Functions
    this TU defines become lane_func_*; calls to guest functions it only
    declares become lane_call_func_* (resolved per signature later); any
    other use of a guest function is its guest code address."""
    decl_globals = {}
    for m in GLOBAL_DECL_RE.finditer(ir):
        decl_globals[m.group(1) or m.group(2)] = m.group(3)
    replace = {}
    for name, rest in decl_globals.items():
        if rest.startswith("external "):
            addr = guest_address(name, symbols)
            if addr is None:
                raise Reject("symbols", f"unresolved data symbol {name}")
            if not RAM_LO <= addr < RAM_HI:
                raise Reject("symbols", f"{name} at {addr:08x} is outside game RAM")
            replace[name] = f"inttoptr (i64 {addr} to ptr)"
        elif not rest.startswith(("private unnamed_addr constant", "internal unnamed_addr constant",
                                  "private constant", "internal constant")):
            raise Reject("symbols", f"TU defines data {name}")
    internal = set(re.findall(r"^define internal [^@]*@([\w.$]+)\(", ir, flags=re.M))
    defined = set(re.findall(r"^define [^@]*@([\w.$]+)\(", ir, flags=re.M)) - internal
    declared = set(re.findall(r"^declare [^@]*@([\w.$]+)\(", ir, flags=re.M))
    guest = set()
    for fname in defined | declared:
        if fname.startswith("llvm.") or fname in MEMORY_FNS:
            continue
        addr = guest_address(fname, symbols)
        if addr is None:
            raise Reject("symbols", f"unresolved function {fname}")
        if not RAM_LO <= addr < RAM_HI:
            raise Reject("symbols", f"function {fname} at {addr:08x} outside game RAM")
        guest.add(fname)
    out_lines = []
    for line in ir.splitlines():
        m = GLOBAL_DECL_RE.match(line)
        if m and (m.group(1) or m.group(2)) in replace:
            continue
        def sub(mm, line=line):
            name = mm.group(1) or mm.group(2)
            if name in replace:
                return replace[name]
            if name in guest:
                after = line[mm.end(): mm.end() + 1]
                is_def_or_decl = line.startswith(("define ", "declare "))
                is_call_target = after == "(" and re.search(r"\bcall\b", line[:mm.start()]) is not None
                if is_def_or_decl or is_call_target:
                    addr = guest_address(name, symbols)
                    local = member and any(lo <= addr < hi for lo, hi in member_code)
                    q = qualify(member, name) if (name in defined or local) else name
                    return ("@lane_" if name in defined else "@lane_call_") + q
                # Address taken: the guest sees its guest code address.
                return f"inttoptr (i64 {guest_address(name, symbols)} to ptr)"
            return mm.group(0)
        out_lines.append(SYM_RE.sub(sub, line))
    return "\n".join(out_lines) + "\n"


def narrow_pointer_cells(ir):
    """Guest memory holds 32-bit pointers. Before optimization, every
    pointer-typed load/store whose address is not a host local slot becomes
    a 32-bit access with zero extension / truncation, and every int->pointer
    conversion goes through the value's 32-bit guest form (C sign-extends an
    int converted to a pointer; the MIPS register is already the address).
    Aggregates holding pointers are refused by check_layout, so no offset or
    size baked into the IR depends on the pointer width."""
    out, counter = [], [0]

    def fresh(tag):
        counter[0] += 1
        return f"%lane.{tag}{counter[0]}"

    allocas = set()
    sext_defs = {}
    for line in ir.splitlines():
        if line.startswith("define "):
            allocas, sext_defs = set(), {}
        m = DEF_RE.match(line)
        if m:
            if m.group(2).startswith("alloca "):
                allocas.add(m.group(1))
            sm = re.match(r"sext (i\d+) (%[\w.\"$]+) to i64", m.group(2))
            if sm:
                sext_defs[m.group(1)] = (sm.group(1), sm.group(2))
        lm = re.match(r"^(\s*)(%[\w.\"$]+) = load (volatile )?ptr, (.*)$", line)
        if lm:
            parts = split_top(lm.group(4))
            addr = parts[0][len("ptr "):] if parts[0].startswith("ptr ") else None
            if addr is not None and addr not in allocas:
                ind, dst, vol = lm.group(1), lm.group(2), lm.group(3) or ""
                narrow, wide = fresh("n"), fresh("z")
                out.append(f"{ind}{narrow} = load {vol}i32, ptr {addr}, align 1")
                out.append(f"{ind}{wide} = zext i32 {narrow} to i64")
                out.append(f"{ind}{dst} = inttoptr i64 {wide} to ptr")
                continue
        st = re.match(r"^(\s*)store (volatile )?ptr (.*)$", line)
        if st:
            parts = split_top(st.group(3))
            addr = parts[1][len("ptr "):] if len(parts) > 1 and parts[1].startswith("ptr ") else None
            if addr is not None and addr not in allocas:
                ind, vol, value = st.group(1), st.group(2) or "", parts[0]
                narrow = fresh("t")
                out.append(f"{ind}{narrow} = ptrtoint ptr {value} to i32")
                out.append(f"{ind}store {vol}i32 {narrow}, ptr {addr}, align 1")
                continue
        im = re.match(r"^(\s*)(%[\w.\"$]+) = inttoptr i64 (%[\w.\"$]+) to ptr(.*)$", line)
        if im and im.group(3) in sext_defs:
            ind = im.group(1)
            width, src = sext_defs[im.group(3)]
            value = src
            if width != "i32":
                value = fresh("s")
                out.append(f"{ind}{value} = sext {width} {src} to i32")
            wide = fresh("z")
            out.append(f"{ind}{wide} = zext i32 {value} to i64")
            out.append(f"{ind}{im.group(2)} = inttoptr i64 {wide} to ptr{im.group(4)}")
            continue
        out.append(line)
    return "\n".join(out) + "\n"


def check_layout(ir):
    types = named_types(ir)
    ptr_types = {n for n, t in types.items() if type_has_ptr(t, types)}
    body = re.sub(r"^%[\w.\"$]+ = type .*$", "", ir, flags=re.M)
    for name in ptr_types:
        if re.search(re.escape(name) + r"\b", body):
            raise Reject("layout", f"uses pointer-bearing type {name}")
    if re.search(r"\[\d+ x ptr\]", body):
        raise Reject("layout", "array of pointers")
    for header, lines in functions(ir):
        for line in lines:
            if re.search(r"getelementptr (?:inbounds |nuw |nusw )*ptr,", line):
                raise Reject("layout", "pointer-element indexing")


def check_optimized(ir, module_internal):
    types = named_types(ir)
    private_globals = set(re.findall(r"^@([\w.$]+) = (?:private|internal)", ir, flags=re.M))
    for header, lines in functions(ir):
        defs = {}
        for line in lines:
            m = DEF_RE.match(line)
            if m:
                defs[m.group(1)] = m.group(2)
        tainted = set()
        for name, rhs in defs.items():
            if rhs.startswith("alloca "):
                tainted.add(name)
        changed = True
        while changed:
            changed = False
            for name, rhs in defs.items():
                if name in tainted:
                    continue
                op = rhs.split()[0] if rhs.split() else ""
                if op in ("getelementptr", "bitcast", "phi", "select", "addrspacecast", "freeze"):
                    if any(t in re.findall(r"%[\w.\"$]+", rhs) for t in tainted) or \
                            any("@" + g in rhs for g in private_globals):
                        tainted.add(name)
                        changed = True

        def refs_host(text):
            return any(re.search(re.escape(t) + r"\b", text) for t in tainted) or \
                any(re.search("@" + re.escape(g) + r"\b", text) for g in private_globals)

        for line in lines:
            s = line.strip()
            if not s or s.endswith(":") or s.startswith(";"):
                continue
            if re.search(r"\b(undef|poison)\b", s):
                raise Reject("ub", "undef/poison operand")
            if s == "unreachable":
                raise Reject("ub", "unreachable")
            m = DEF_RE.match(line)
            rhs = m.group(2) if m else s
            if re.search(r"\basm\b", rhs) and not empty_asm_identity(rhs):
                raise Reject("calls", "inline asm")
            if rhs.startswith(("atomicrmw", "cmpxchg", "fence")) or " atomic " in rhs:
                raise Reject("memory", "atomic operation")
            if re.match(r"(load|store) volatile ", rhs) or "i1 true)" in rhs and "llvm.mem" in rhs:
                # Decomp C marks device access volatile; a pointer cell such
                # as D_8006B560 holds an MMIO address the host cannot map.
                raise Reject("memory", "volatile access (device register)")
            if re.match(r"load (volatile )?", rhs):
                ty = split_top(re.sub(r"^load (volatile )?", "", rhs))[0]
                addr = split_top(re.sub(r"^load (volatile )?", "", rhs))[1]
                if type_has_ptr(ty, types) and not refs_host(addr):
                    raise Reject("memory", "pointer load from guest memory")
            if re.match(r"store (volatile )?", rhs):
                parts = split_top(re.sub(r"^store (volatile )?", "", rhs))
                value, addr = parts[0], parts[1]
                if type_has_ptr(value.split()[0], types) and not refs_host(addr):
                    raise Reject("memory", "pointer store to guest memory")
                if refs_host(value):
                    raise Reject("escape", "host address stored")
            if rhs.startswith("ptrtoint") and refs_host(rhs):
                raise Reject("escape", "host address converted to integer")
            if rhs.startswith("ret ") and refs_host(rhs):
                raise Reject("escape", "host address returned")
            if re.match(r"inttoptr ", rhs):
                src = split_top(rhs[len("inttoptr "):].split(" to ")[0])[0].split()
                val = src[-1]
                if val.startswith("%"):
                    d = defs.get(val, "")
                    if not (d.startswith("zext i32") or d.startswith("zext nneg i32") or
                            re.match(r"and i64 .*, 4294967295$", d)):
                        raise Reject("inttoptr", f"integer->pointer from '{d[:40]}'")
                elif re.fullmatch(r"-?\d+", val) and not RAM_LO <= int(val) < RAM_HI:
                    raise Reject("inttoptr", f"constant address {int(val):x}")
            for const in re.findall(r"inttoptr \(i64 (-?\d+) to ptr\)", rhs):
                if not RAM_LO <= int(const) < RAM_HI:
                    raise Reject("inttoptr", f"constant address {int(const) & 0xffffffffffffffff:x}")
            dm = re.match(r"(sdiv|udiv|srem|urem)(?: exact)? (i\d+) (.*)$", rhs)
            if dm:
                divisor = split_top(dm.group(3))[1]
                if not re.fullmatch(r"-?\d+", divisor) or int(divisor) in (0, -1):
                    raise Reject("arith", f"{dm.group(1)} by {divisor}")
            sm = re.match(r"(shl|lshr|ashr)(?: nuw| nsw| exact)* (i\d+) (.*)$", rhs)
            if sm:
                amount = split_top(sm.group(3))[1]
                if not re.fullmatch(r"\d+", amount):
                    d = defs.get(amount, "")
                    mm = re.match(r"and i\d+ (?:%[\w.\"$]+, (\d+)|(\d+), %[\w.\"$]+)$", d)
                    mask = int(mm.group(1) or mm.group(2)) if mm else None
                    if mask is None or mask > 31:
                        raise Reject("arith", "unmasked variable shift")
                elif int(amount) >= int(sm.group(2)[1:]):
                    raise Reject("arith", "oversized constant shift")
            cm = re.search(r"\b(?:tail |musttail |notail )?call\b(.*)$", rhs)
            if cm and empty_asm_identity(rhs):
                continue  # emits nothing; outputs are the tied inputs
            if cm:
                call = cm.group(1)
                target = re.search(r"(@[\w.$]+|%[\w.\"$]+)\(", call)
                if not target or target.group(1).startswith("%"):
                    raise Reject("calls", "indirect call")
                if "inttoptr" in call.split("(")[0]:
                    raise Reject("calls", "call through a guest address")
                name = target.group(1)[1:]
                args = call[target.end():]
                if name.startswith("llvm."):
                    if not re.match(r"llvm\.(lifetime|memset|memcpy|memmove|smax|smin|umax|umin|abs|"
                                    r"ctlz|cttz|ctpop|fshl|fshr|bswap|assume|experimental\.noalias|"
                                    r"sadd|uadd|ssub|usub|vector\.reduce)", name):
                        raise Reject("calls", f"intrinsic {name}")
                    continue
                if name in ("memset", "memcpy", "memmove") or name in LANE_HELPERS:
                    continue
                if name in module_internal:
                    continue
                if not (re.fullmatch(r"lane_(?:call_)?" + QFUNC + r"(?:__\w+)?", name) or
                        name.startswith("lane_icall__")):
                    raise Reject("calls", f"call to {name}")
                if refs_host(args):
                    raise Reject("escape", f"host address passed to {name}")


def loop_walks_memory(body):
    """True when some load in the loop reads an address that changes each
    iteration: its pointer derives (transitively, within the loop) from a phi
    defined in the loop. Such a loop (a table search, a string scan) ends on
    the data it reads; a polling loop re-reads the same address."""
    tainted = set()
    for l in body:
        m = re.match(r"\s*(%[\w.$-]+)\s*=\s*phi\b", l)
        if m:
            tainted.add(m.group(1))
    changed = True
    while changed:
        changed = False
        for l in body:
            m = re.match(r"\s*(%[\w.$-]+)\s*=\s*(.*)", l)
            if not m or m.group(1) in tainted:
                continue
            if any(re.search(re.escape(t) + r"\b", m.group(2)) for t in tainted):
                tainted.add(m.group(1))
                changed = True
    for l in body:
        m = re.search(r"=\s*load\b[^,]*,\s*ptr\s+(%[\w.$-]+)", l)
        if m and m.group(1) in tainted:
            return True
    return False


ICALL_RE = re.compile(r"^(\s*(?:%[\w.\"$]+ = )?)((?:tail |musttail |notail )?call )"
                      r"((?:[\w]+ )*?)(void|i8|i16|i32|ptr) (%[\w.\"$]+)\((.*)\)(\s*#\d+)?\s*$")
ARG_TYPES = ("i8", "i16", "i32", "ptr")


def rewrite_indirect_calls(ir):
    """Route calls through function pointers to the interpreter, as jalr does.

    A guest function pointer holds a guest address (a zero-extended 32-bit
    value). `call T %fp(args)` becomes `call T @lane_icall__<sig>(i32 addr,
    args)`, whose generated stub runs musashi_native_guest_call(addr, ...);
    the interpreter then dispatches the target (natively again when the lane
    admits it). Returns (ir, {symbol: (ret, rext, params)}). A call with an
    argument type the guest ABI cannot carry is left alone, and the checker
    refuses it as before."""
    out, icalls, n = [], {}, 0
    for line in ir.splitlines():
        m = ICALL_RE.match(line)
        if not m or " asm " in line:
            out.append(line)
            continue
        lead, call, rattrs, ret, fp, args, attrs = m.groups()
        params = []
        ok = True
        for a in split_top(args):
            toks = a.split()
            if not toks or toks[0] not in ARG_TYPES:
                ok = False
                break
            params.append((toks[0], ("signext" in toks[1:-1], "zeroext" in toks[1:-1])))
        if not ok or len(params) > 8:
            out.append(line)
            continue
        rext = ("signext" in rattrs.split(), "zeroext" in rattrs.split())
        sig = (ret, rext, tuple(params))
        sym = "lane_icall__" + sig_code((ret, rext, params))
        icalls[sym] = sig
        indent = re.match(r"^\s*", line).group(0) or "  "
        out.append(f"{indent}%icall.{n} = ptrtoint ptr {fp} to i64")
        out.append(f"{indent}%icall.{n}.t = trunc i64 %icall.{n} to i32")
        new_args = ", ".join([f"i32 %icall.{n}.t"] + split_top(args))
        out.append(f"{lead}{call}{rattrs}{ret} @{sym}({new_args}){attrs or ''}")
        n += 1
    for sym, (ret, rext, params) in sorted(icalls.items()):
        def t(ty, e):
            return ty + (" signext" if e[0] else " zeroext" if e[1] else "")
        plist = ", ".join(["i32"] + [t(ty, e) for ty, e in params])
        rdecl = ("signext " if rext[0] else "zeroext " if rext[1] else "") + ret
        out.append(f"declare {rdecl} @{sym}({plist})")
    return "\n".join(out) + "\n", icalls


def check_polling_loops(ir):
    """A native call runs to completion without IRQ delivery. A loop that
    neither stores nor calls can only make progress through state an IRQ or
    device changes (a VSync counter, a DMA flag), so it could spin forever
    natively. Refuse any such loop; counting loops that also store or call
    are unaffected."""
    for header, lines in functions(ir):
        labels = {}
        for idx, line in enumerate(lines):
            m = re.match(r"^([\w.$-]+):", line)
            if m:
                labels["%" + m.group(1)] = idx
        # The entry block is unlabeled; numbered blocks still get "N:" lines.
        for idx, line in enumerate(lines):
            if not line.strip().startswith("br "):
                continue
            for target in re.findall(r"label (%[\w.$\"-]+)", line):
                start = labels.get(target)
                if start is None or start > idx:
                    continue
                body = lines[start: idx + 1]
                if not any(re.search(r"\bstore\b", l) or
                           (re.search(r"\bcall\b", l) and "@llvm." not in l) for l in body) \
                        and not loop_walks_memory(body):
                    raise Reject("irq", "loop with no store or call (polling)")


def lane_names(ir):
    return set(re.findall(r"^define [^@]*@(lane_" + QFUNC + r")\(", ir, flags=re.M))


def sig_code(sig):
    ret, rext, params = sig
    def tok(t, e):
        return t + ("s" if e[0] else "z" if e[1] else "")
    return "r" + tok(ret, rext) + "".join("_" + tok(t, e) for t, e in params)


def resolve_calls(optir):
    """Give each lane_call_func_* a signature-specific symbol. Returns
    (ir, {symbol: (callee, sig or None, argc)}); sig None = unprototyped,
    called with at most argc register-width arguments."""
    stubs, renames = {}, {}
    for decl in [l for l in optir.splitlines() if l.startswith("declare ")]:
        m = re.search(r"@(lane_call_(" + QFUNC + r"))\(", decl)
        if not m:
            continue
        sym, callee = m.group(1), m.group(2)
        if re.search(r"\(\s*\.\.\.\s*\)", decl) or ", ...)" in decl:
            argc = 0
            ret = parse_return(decl, sym)
            for line in optir.splitlines():
                if line.startswith("declare ") or "@" + sym + "(" not in line:
                    continue
                inner = line.split("@" + sym + "(", 1)[1]
                depth, end = 1, 0
                for end, ch in enumerate(inner):
                    depth += ch == "("
                    depth -= ch == ")"
                    if depth == 0:
                        break
                args = split_top(inner[:end])
                for a in args:
                    ty = a.split()[0]
                    if ty not in ("i32", "ptr", "i8", "i16"):
                        raise Reject("abi", f"unprototyped {callee} gets {ty}")
                argc = max(argc, len(args))
            if argc > 6:
                raise Reject("abi", f"unprototyped {callee} with {argc} arguments")
            new = f"{sym}__va{argc}_{ret[0]}{'s' if ret[1][0] else ''}"
            stubs[new] = (callee, None, argc, ret)
        else:
            sig = parse_signature(decl, sym)
            new = f"{sym}__{sig_code(sig)}"
            stubs[new] = (callee, sig, len(sig[2]), None)
        renames[sym] = new
    for old, new in renames.items():
        optir = re.sub(r"@" + re.escape(old) + r"\b", "@" + new, optir)
    return optir, stubs


def parse_return(decl, name):
    m = re.search(r"^declare\s+(.*?)@" + re.escape(name) + r"\(", decl)
    pre = m.group(1).split() if m else []
    ret = next((t for t in reversed(pre) if t in ("void", "ptr") or re.fullmatch(r"i\d+", t)), None)
    if ret is None or (ret != "void" and ret not in ("ptr", "i8", "i16", "i32")):
        raise Reject("abi", f"return type of {name}")
    return ret, ("signext" in pre, "zeroext" in pre)


ASM_CALL_RE = re.compile(r'\basm\s+(?:sideeffect\s+)?(?:alignstack\s+)?(?:inteldialect\s+)?"([^"]*)",\s*"([^"]*)"')


def empty_asm_identity(rhs):
    """An empty asm template ("") emits no instruction. It is an identity
    when each output is tied to an input ("=r,0"); decomp C uses these as
    scheduling barriers for gcc-2.7.2 matching. An untied output would be
    an undefined value, so it is still refused."""
    m = ASM_CALL_RE.search(rhs)
    if not m or m.group(1).strip():
        return False
    cons = [c for c in m.group(2).split(",") if c]
    outs = [c for c in cons if c.startswith("=")]
    ins = [c for c in cons if not c.startswith(("=", "~"))]
    tied = {int(c) for c in ins if c.isdigit()}
    return all(i in tied for i in range(len(outs)))


# `register int x asm("$5")` pins a MIPS register for matching; C semantics
# are unchanged, and the host compiler rejects the MIPS name.
REG_PIN_RE = re.compile(r'(\bregister\b[^;={}]*?)\s*(?:__asm__|asm)\s*\(\s*"\$\w+"\s*\)')


def host_source(text):
    return REG_PIN_RE.sub(r"\1", text)


# Admissible by every rule, but held out until a differential covers them:
# with the probe's generic inputs none of their cases was comparable.
DENY = {
    0x80015498: "no comparable differential case yet (mirror faults on generic inputs)",
    0x80016638: "no comparable differential case yet (not comparable / mirror faults)",
    0x800167F0: "no comparable differential case yet (mirror faults on generic inputs)",
    0x8002FF0C: "no comparable differential case yet (interpreter refused every case)",
}

# The scene camera setup writes the shared actor and invokes guest callbacks.
# It and the facing routines require checked guest accesses and path timing.
# The three facing extents also occur in SC03/0108.
FACING_EXTENTS = {
    0x8017D01C: (184, 0x031C09BFB0E714BE),
    0x8017D484: (368, 0x0FBC2305FFE0240E),
    0x8017D5F4: (184, 0x7E49C97F155040C8),
    0x8017D6AC: (184, 0x67821A3E1D2AE386),
}


# These SC01/0000 player helpers access actor memory and globals, and their
# instruction-boundary timing is still interpreted. Some other members contain
# the same whole function at the same address; others use a different layout.
SC01_PLAYER_MOTION_EXTENTS = {
    0x801473DC: (16, 0xC43EE3DE99F5D03C),
    0x801473EC: (116, 0x21CB35CF151E9600),
    0x80147814: (76, 0x7581AF829A9891B6),
    0x80147948: (84, 0xF4F0A29E42172E90),
    0x801479BC: (84, 0xAC93D88A54D5E69B),
    0x80147B5C: (212, 0xA36525D925E9F0C9),
    0x80148038: (92, 0x605F22189CA0384F),
    0x801488A8: (116, 0x6AB40F4A792D7C96),
    0x8014891C: (100, 0xEB3B92195A88580D),
    0x80148980: (104, 0x067B31ACE1209C59),
    0x80148A48: (100, 0xADF7AAC6CEE6EBFE),
    0x80149744: (68, 0x1C428CFC35FA8A6C),
    0x80149884: (60, 0x8B07EBA43DB9CF50),
    0x80149CD4: (60, 0xEC44C5610B56AF22),
    0x80149FB0: (152, 0x3ECE2E2F5F3CE5F9),
    0x8016F1AC: (24, 0x2D530D164FD71FFB),
}


# The forest floor query and its explicitly identified variants need checked
# guest loads, exception handling and timing before player dispatch.
FOREST_FLOOR_EXTENTS = {
    0x80134510: {
        "sc03_0001": (232, 0xC9860184D2447326),
        "sc02_0039": (232, 0x595D4E3722DE9DFB),
        "sc01_0077": (232, 0xCB033B4706965762),
        "main_0012": (232, 0xD45CC4EC505088D6),
        "sc02_0035": (232, 0xB122B781A31FF157),
    },
    0x801345F8: {
        "sc02_0039": (424, 0x6230774866449889),
        "sc01_0077": (424, 0xF2C4C0EB8C6E5FAF),
        "main_0012": (424, 0x059F1C87841DD46E),
    },
    0x801347A0: {
        "sc02_0039": (648, 0xCB9978129245A5CE),
        "sc01_0077": (648, 0x0697C6B29C8DF08A),
        "main_0012": (648, 0xE8F7E0CB1848B05E),
    },
    0x80134A28: {
        "sc02_0039": (76, 0xA9B0AB85FCCB0E61),
        "sc01_0077": (76, 0xA9B0AB85FCCB0E61),
        "main_0012": (76, 0xA9B0AB85FCCB0E61),
    },
    0x80134A74: {
        "sc01_0077": (428, 0x21B8EFFE3D4C717E),
    },
    0x80134C20: {
        "sc01_0077": (920, 0x99ED4AE9F6310B7C),
    },
    0x80134FB8: {
        "sc01_0077": (76, 0xA9B0AB85FCCB0E61),
        "sc02_0035": (76, 0xA9B0AB85FCCB0E61),
    },
    0x80136A94: {
        "sc02_0039": (304, 0x1D6A0D3F1CAE4224),
        "sc01_0077": (304, 0x907C1DF3E1989D4C),
    },
    0x80135480: {
        "sc02_0039": (1032, 0x29D566DD2948D2A4),
        "sc01_0077": (1032, 0x5221F6D7176211A0),
        "main_0012": (1032, 0x8648A6C117ABCBC4),
    },
    0x80135888: {
        "sc01_0077": (452, 0x8EE787ADB445A8F7),
        "main_0012": (452, 0xEC327F42C79F1489),
    },
    0x80133784: {"sc02_0039": (812, 0x309DE57F9074D361), "sc01_0077": (812, 0xFE2D6B192DBC08CA)},
    0x801343C4: {"sc01_0077": (332, 0xB19C8B5B64754AC3)},
    0x80133AB0: {"sc02_0039": (548, 0xC6B065F5F4EE631A), "sc01_0077": (548, 0x9894996A69C12F1B)},
    0x80135004: {"sc01_0077": (356, 0x982B71CC5AB6E562)},
    0x80135A4C: {"sc01_0077": (724, 0xAC6B91F466F71566)},
    0x80135EB0: {"sc02_0039": (1156, 0x66571C1CA8B49E5B), "sc01_0077": (1156, 0x2453BFCF0858A8FC)},
    0x80136334: {"sc02_0039": (644, 0x47B856E198539C08), "sc01_0077": (644, 0x354456DF35EC82D4)},
    0x801365B8: {"sc02_0039": (620, 0x9C85FDEA0E49F65D), "sc01_0077": (620, 0xEE10B1998D349329)},
    0x80136824: {"sc02_0039": (624, 0xA6220B515717317C), "sc01_0077": (624, 0x441BAD6497EB45EC)},
    0x8014E5B4: {"sc01_0077": (236, 0x657C3615E0C9AA5A)},
    0x8014E83C: dict.fromkeys(("sc01_0077", "main_0012"), (248, 0x2B785897CD72D1DF)),
    0x8014E98C: dict.fromkeys(("sc01_0077", "main_0012"), (192, 0x3306C00B358039CB)),
    0x8014EA4C: dict.fromkeys(("sc01_0077", "main_0012"), (732, 0xD359EE3EC66A0766)),
    0x8016380C: {"main_0012": (148, 0x71854F0B6BBDC749)},
    0x801638A0: {"main_0012": (176, 0x6D0F1DABAABF3689)},
}


# The earlier Tower allocator used different C signatures for the same game routine.
TOWER_POOL_PRIOR_SOURCE = "37f8c8016ea1b697c77467c8ee51f6fe1f89930cfacf04a68d81bb7cd2b822e7"


# These scene routines need guest register, memory and event ordering. Some
# complete extents also appear at the same PC in another room.
FOREST_ATTACHMENT_EXTENTS = {
    0x80178438: (100, 0x1DD5EA346417021E),
    0x801853EC: (60, 0x46C2D13AD1E0AF2B),
    0x80184554: (64, 0xA9B66A3131FED9BC),
    0x80184594: (88, 0x705927075E43A722),
    0x801845EC: (176, 0x8D184634D85DA494),
    0x8018469C: (92, 0xE5698407F10D3677),
    0x801846F8: (20, 0x74E91EF7410A48D3),
    0x8018470C: (388, 0xC79193F42395FF44),
    0x80184890: (60, 0xEE6CFB631EED458E),
    0x801848CC: (60, 0x01269D53BE3A5566),
    0x80184A68: (132, 0x43509592C0856F97),
    0x80184AEC: (56, 0x1C46ECE17310E51F),
    0x80184B24: (144, 0xA5E1342C5F92A656),
    0x80184BB4: (88, 0xF0810B0DADC53C81),
    0x80184C0C: (192, 0x4EA25935271AEE70),
    0x80184CCC: (132, 0xC635AB3F2A9C82CF),
    0x80184D50: (392, 0xA4F5F95F9041CFDA),
}

SCENE_PHASE_EXTENTS = {
    # Twinpeak's action and queue operations need the live actor, services and
    # guest timing. The queue bytes alone cannot identify their scene.
    0x801466F0: {"sc03_0029": None},
    0x8014680C: {"sc03_0029": None},
    0x8015F89C: {"sc03_0029": None},
    # Jon selects a room script and may advance story state before returning.
    0x801EF558: {"sc03_0054": (384, 0x0A70742E7582B0D2)},
    # Village waypoint motion needs the live actor, its movement services and guest timing.
    0x80187A64: {"sc03_0001": (456, 0x467F92EE8AC88EA4)},
    0x80180994: {"sc03_0116": (60, 0xC1D82A67EF1E15BB)},
    0x801809D0: {"sc03_0116": (68, 0xB403F0AA729BC236)},
    0x80180FBC: {"sc03_0116": (60, 0x18F643941ADDD7FF)},
    0x80180FF8: {"sc03_0116": (48, 0x077E814D4392DF4D)},
    0x801814B0: {"sc03_0116": (60, 0x1D9923531F4B0FDF)},
    0x801814EC: {"sc03_0116": (100, 0xD0504353E6920C6D)},
    0x80181550: {"sc03_0116": (48, 0x866377F1CC79D587)},
    0x8018172C: {"sc03_0116": (172, 0xFE3397A86320CE36)},
    0x80181844: {"sc03_0116": (32, 0xB40100300F85BA40)},
    0x80181864: {"sc03_0116": (84, 0xAA89ABF31B6614FF)},
    0x80181FA8: {"sc03_0116": (260, 0xA4242333F7E649E4)},
    0x80182378: {"sc03_0116": (60, 0x4E8E9F64889C1744)},
    0x801825A8: {"sc03_0116": (176, 0x49CC1980C305434C)},
    0x8017E910: {"sc02_0039": (272, 0x55E6A69E5D457E0F)},
    0x8017E794: {"sc02_0039": (380, 0x67331A91CF80E51D)},
    0x8017EA20: {"sc02_0039": (532, 0x34F44B1AA9D02239)},
    0x8017EF5C: {"sc02_0039": (244, 0xFB88AECF43A44DD4)},
    0x8017EED8: {"sc02_0039": (132, 0x66C452168AE36CAB)},
    0x8017F050: {"sc02_0039": (96, 0x68F457D535D820EA)},
    **{pc: {"sc01_0077": extent} for pc, extent in FOREST_ATTACHMENT_EXTENTS.items()},
    0x8017D3C0: {"sc03_0109": (204, 0x4005979C36E4AA48)},
    0x8017F15C: {"sc03_0113": (160, 0xD98DDB81180D7632)},
    # The six control-reset helpers share their instructions with other room members.
    0x80180328: {"sc02_0005": (88, 0x6D2EC5AD6D71172C)},
    0x80175414: {"sc02_0005": None},
    0x8014C6AC: {"sc02_0005": None},
    0x8016F0E4: {"sc02_0005": None},
    0x8017196C: {"sc02_0005": None},
    0x801511A8: {"sc02_0005": None},
    0x80148634: {"sc02_0005": None},
    0x8017E758: {"sc02_0039": (60, 0x070CB4DE7F5952EF)},
    0x8017EE9C: {"sc02_0039": (60, 0xD615A064914C526A)},
    0x8017D2E0: {"sc03_0109": (164, 0xACBD125C7ED85BB9)},
    0x8017D384: {"sc03_0109": (60, 0xFAB1DF77A867F056)},
    0x8017F07C: {"sc03_0113": (164, 0xAD79B5CCBA7AF226)},
    0x8017F120: {"sc03_0113": (60, 0xBDA91C55D2623999)},
    0x8017E640: {"sc02_0039": (280, 0x5776B1218AB5E953)},
    # The five lower camera routines share their instructions with other room members.
    0x8017E26C: {"sc02_0005": (220, 0x474D323D3770728D)},
    0x80129FF4: {"sc02_0005": None},
    0x8012A018: {"sc02_0005": None},
    0x8012A048: {"sc02_0005": None},
    0x8012A094: {"sc02_0005": None},
    0x8012A100: {"sc02_0005": None},
    0x8012CC88: {"sc02_0039": None},
    0x8012CEB0: {"sc02_0039": None},
    # The request and control instructions also occur in other room members.
    0x8017FCFC: {"sc02_0005": (184, 0x46D58ECC4D744307)},
    0x801399F0: {"sc02_0005": None},
    0x80139914: {"sc02_0005": None},
    0x8013A8B0: {"sc02_0005": None},
    0x80147060: {"sc02_0005": None},
    0x8017FF8C: {"sc02_0005": (100, 0x7C0B44E8DBD7119A)},
    0x8017FFF0: {"sc02_0005": (112, 0x407B1C410C020ED8)},
    # Actor cleanup has the same instructions in other scene members.
    0x8012CAE4: {"sc02_0005": None},
    0x8014FA70: {"sc02_0039": None},
    # Tower resource instructions are shared by all scene members.
    0x8012913C: {"sc02_0005": None},
    0x801291C0: {"sc02_0005": None},
    0x801292C8: {"sc02_0005": None},
    0x8012C1B8: {"sc02_0005": None},
    0x80186F9C: {"sc02_0005": (212, 0xBAB48872018582AD)},
    0x8018675C: {"sc02_0005": (288, 0x5A8CADA4006224D0)},
    0x8018687C: {"sc02_0005": (176, 0x489F6184399FBA88)},
    0x80131F64: {"sc03_0116": (60, 0x7CEFDCDDB39E55A0)},
    0x80185E80: {"sc02_0005": (628, 0xB316C82C8AC22FD7)},
    0x801860F4: {"sc02_0005": (80, 0x1F90D84AC79281F0)},
    0x80186144: {"sc02_0005": (52, 0x2637A215A05B1B35)},
    0x8018AC48: {"sc02_0005": (108, 0x626C94365F200A02)},
    0x8018AD20: {"sc02_0005": (108, 0xB6FDD6C1F0BCA0E0)},
    0x80150B28: {"sc02_0039": (116, 0xDD4C9A52AE91D210)},
    0x80132144: {"sc03_0116": (108, 0x797E5A902D97DC4B)},
    # The timer instructions are identical across physical scene members.
    0x8012BEE8: {"sc02_0005": None},
    0x80181984: {"sc02_0005": (612, 0x52D30BEFD003B1E4)},
    0x801838A8: {"sc02_0005": (612, 0xBA2AE3DE730FC591)},
    0x80182810: {"sc02_0005": (200, 0xFA682CDB42DAA946)},
    0x801828D8: {"sc02_0005": (64, 0x7A4AAAC2D2825EAD)},
    0x8018791C: {"sc02_0005": (192, 0xBA3FB1822A4EF291)},
    0x80182918: {"sc02_0005": (128, 0x257219EAD0533937)},
    0x8018968C: {"sc02_0005": (616, 0x39BE58F0D86E1FB1)},
    0x80189AF4: {"sc02_0005": (60, 0x60B2BAB4787BE779)},
    0x8018B300: {"sc02_0005": (68, 0x5FA309D7618F30DC)},
    0x80186600: {"sc02_0005": (348, 0x4474F8D30C5571E7)},
    0x80186DE8: {"sc02_0005": (300, 0x4AE9ED75E2C5DDF0)},
    0x801877FC: {"sc02_0005": (196, 0xEF8D422E9712068A)},
    0x80182998: {"sc02_0005": (244, 0x2C9300B7B952078B)},
    0x8012AD44: {"sc02_0005": None},
    0x8015616C: {"sc01_0077": (516, 0xF6DC4115C9995E01)},
    0x80184C20: {"sc02_0005": (132, 0x81B3B7EB1443D22F)},
    0x80186F58: {"sc02_0005": (8, 0xD1C982C1E888FEE4)},
    0x8018AAAC: {"sc02_0005": (216, 0xBE13E267ABD55DC1)},
    0x8015BFF4: {"sc01_0077": (60, 0x4170BBA2D2061F12)},
    0x8015C128: {"sc01_0077": (516, 0x4A6E7C4C56E74170)},
    0x80186424: {"sc02_0005": (312, 0x1B47BE12370CCAFF)},
    0x80184EC0: {"sc02_0005": (416, 0xB67CEFACD4A0472F)},
    0x8018550C: {"sc02_0005": (472, 0xC421FCC0B6AAA08E)},
    0x8018874C: {"sc02_0005": (128, 0xB9965170B1A54304)},
    0x801887CC: {"sc02_0005": (100, 0xE8C405511881D60F)},
    # Animation refresh has the same instructions in all 138 scene images.
    0x8013240C: {"sc02_0005": None},
    0x80183FCC: {"sc02_0005": (284, 0x61085F35D31E12B2)},
    0x801840E8: {"sc02_0005": (116, 0x8BB69B208B13546C)},
    0x801831A4: {"sc02_0005": (92, 0x50D106B3DA5CA92D)},
    0x80183200: {"sc02_0005": (24, 0xB788597D0686DA02)},
    0x80183218: {"sc02_0005": (688, 0x495EED1701B033B3)},
    0x8018415C: {"sc02_0005": (40, 0x8162EF7435190B41)},
    0x801858C4: {"sc02_0005": (104, 0x811B40E8F6A8A3A4)},
    0x80186ADC: {"sc02_0005": (44, 0x1418CE2EA961A78A)},
    0x8018850C: {"sc02_0005": (176, 0x969ADEE8AE2B2AA7)},
    0x80184358: {"sc02_0005": (172, 0x74A29C913382F5DC)},
    0x80185A04: {"sc02_0005": (132, 0x6E755C8F78C97419)},
    0x80185A88: {"sc02_0005": (176, 0xA1A0DF0EE9365C4E)},
    0x801869E0: {"sc02_0005": (36, 0x7E4CA020F7DDF8E1)},
    0x80189CCC: {"sc02_0005": (60, 0x9FF3CF2DD97213CD)},
    0x8018BA38: {"sc02_0005": (172, 0x324B6C42F7ADDF10)},
    0x8018BAE4: {"sc02_0005": (116, 0x812998898392979A)},
    0x80187664: {"sc02_0005": (76, 0x015DFA2C7B78B104)},
    0x8018B16C: {"sc02_0005": (368, 0xE41CF2A7FF8FFCDC)},
    0x80185060: {"sc02_0005": (384, 0xFCE2C9E171C6A29D)},
    0x80186B08: {"sc02_0005": (176, 0xDF3C394285713854)},
    0x80187280: {"sc02_0005": (200, 0xA83FB48F6006C71F)},
    0x8016AA50: {"sc02_0005": (224, 0x5D09719A82C090FF)},
    0x80146A6C: {"sc02_0005": (72, 0x87DA5CB2EEA6CA90)},
    0x80146AFC: {"sc02_0005": (160, 0xAD5081E726E104B6)},
    0x80186A04: {"sc02_0005": (216, 0xC280AB6C8FDD078F)},
    0x8014C4AC: {"sc02_0005": (188, 0x03C2B0C590144408)},
    # The camera changes matrix state twice and passes a guest stack word to the transform.
    0x8012F14C: {"sc01_0077": (88, 0x128D65FE65F8BF49)},
    0x801853EC: {
        "sc02_0005": (160, 0x6FD1708294A2B1E9),
        "sc01_0077": FOREST_ATTACHMENT_EXTENTS[0x801853EC],
    },
    # Both passes share mutable guest records, callback tables and linked positions.
    0x80165CA0: {"sc02_0039": (396, 0xD0040599098500D1)},
    # Mine dispatchers read mutable callback targets and require an identified owner.
    0x8013BC7C: {"sc02_0039": (96, 0x4F93DE85924C7B19)},
    0x8013BCDC: {"sc02_0039": (88, 0x1CBB98A3C535F03A)},
    # Attachment callbacks and sequential guest writes can alter later slots and destinations.
    0x80129010: {"sc02_0039": (204, 0x4F2C769D8309384B)},
    # Fade state transitions update shared data before issuing screen callbacks.
    0x8013A448: {"sc02_0039": (124, 0x734318216920BE79)},
    0x8013A4C4: {"sc02_0039": (108, 0x86E68244540D595F)},
    # Mine actor and fade callbacks depend on guest state and frame timing.
    0x801379FC: {"sc02_0039": (388, 0x996017639B1A6852)},
    0x8013A380: {"sc02_0039": (200, 0x3158F085FABFA816)},
    # The controller can issue two draws and reread target state between callbacks.
    0x80165E90: {"sc02_0039": (276, 0x616A9BEC6ADB779B)},
    # Actor steps depend on the view query and may run repeatedly in the same caller.
    0x8015094C: {"sc02_0039": (292, 0xEB82E1745D606C6D)},
    # The external drawing services are represented by caller-specific observation adapters.
    0x8013A530: {"sc02_0039": (816, 0x643C6A867D5BB30E)},
    # Frame commands reference shared guest records and nested animation state.
    0x8012AAAC: {"sc02_0039": (500, 0x6DA323ED9D5C4F3F)},
    # Actor animation ticks can scan and apply frames during the same scheduler pass.
    0x8012A988: {"sc02_0039": (292, 0x28BE479E7A084CAA)},
    # Animation scans require a bounded frame reader for the owning actor.
    0x8012ACE0: {"sc02_0039": (100, 0x77902C59DFA7EBCF)},
    # Target indicators reuse a primitive and matrix across ten ordered submit callbacks.
    0x80167AE0: {"sc02_0039": (732, 0xEB04EFADECE21D52)},
    # The dialog updater reads and writes scene colors, with loads interleaved with stores.
    0x801299C8: {"sc02_0039": (632, 0xB666EDF924654ECB)},
    # The mine script reads duration after two observable guest stores.
    0x8013CA14: {"sc02_0039": (168, 0x135546466EC2D107)},
    # The per-frame update runs the same guest calls with a member-specific object.
    0x801287B8: {
        "main_0012": (248, 0x7003DEEF5601FA29),
        "sc01_0005": (248, 0x08B946C9DB1D0CE5),
        "sc01_0077": (248, 0x9A7880EE7ECA1D79),
        "sc02_0039": (248, 0xBEA43DF222B8BA28),
        "sc03_0001": (248, 0xFF9FB166A2180DC7),
    },
    # Both actor halfword updates load V0 and store in the return delay slot.
    # The counter returns the full sum, including 65536 when the halfword wraps.
    0x80146CA0: {"sc02_0035": (20, 0x3E4DBB3EB66FDFAF)},
    0x8014900C: dict.fromkeys(("main_0012", "sc02_0035"), (20, 0xE50FD3C212372182)),
    # These forest player states dispatch through member-specific game address tables.
    0x80146360: {"sc01_0077": (64, 0xD6FF4D78017740C9)},
    0x80159968: {"sc01_0077": (60, 0xB59A7B7CF404FBEB)},
    0x80159A20: {"sc01_0077": (232, 0xF35D76072A56E095)},
    0x80159C84: {"sc01_0077": (1348, 0x2C65A9449EB1F018)},
    0x8015AD3C: {"sc01_0077": (60, 0xAACEF9A05EB2078A)},
    0x8015B950: {"sc01_0077": (1084, 0x46A2E2A380C64CB4)},
    0x8015BDD0: dict.fromkeys(("main_0012", "sc01_0077"), (52, 0xB43708CB38721823)),
    0x8015BF48: dict.fromkeys(("main_0012", "sc01_0077"), (52, 0x35DA535E1DD56099)),
    0x8015C0C4: dict.fromkeys(("main_0012", "sc01_0077"), (100, 0xB01739AC8C0A156A)),
    # These object services call into guest code from the middle of the frame.
    0x8014A2E4: dict.fromkeys(("main_0012", "sc02_0039"), (156, 0x4DBDD625CB4098C8)),
    0x8014C278: dict.fromkeys(("main_0012", "sc02_0039"), (56, 0x93E8D33159724249)),
    0x8014FDF4: dict.fromkeys(("main_0012", "sc02_0039"), (108, 0xBE00661D1711B59B)),
    # The second floor probe calls the cell map while a guest return address is on the stack.
    0x80150150: dict.fromkeys(("main_0012", "sc01_0077", "sc02_0039"), (32, 0x7E508B16726EDF52)),
    0x80150460: dict.fromkeys(("main_0012", "sc02_0039"), (32, 0x7E508B16726EDF52)),
    0x801505FC: dict.fromkeys(("main_0012", "sc02_0039"), (168, 0x840FD970AD98C205)),
    # The pickup can call twice into item state and rereads its signed table index.
    0x801506A4: {"sc02_0039": (380, 0x7138AD8777ECFBE1)},
    0x801508B4: dict.fromkeys(("main_0012", "sc02_0039"), (68, 0x3C6364F78110B674)),
    0x80135D20: {
        "sc02_0039": (400, 0xFC11FE9020A17C22),
        "sc01_0077": (400, 0x5E2B50C772BBC3C2),
    },
    0x8016EE40: dict.fromkeys(
        ("sc02_0039", "sc01_0077", "sc02_0035", "sc03_0001", "main_0012"),
        (84, 0x0A847F9F21DC5E21)),
    0x800D1EBC: {"main_0010": (156, 0xE217FA964716E7AA)},
    0x8017CDD8: {"sc02_0039": (68, 0x30E5FDA816E1EC41)},
    0x8017CE1C: {"sc02_0039": (36, 0x0CAE45EE82867792)},
    0x8017CE40: {"sc02_0039": (64, 0x355752678B5D9877)},
    0x8017CEFC: {"sc02_0039": (32, 0x2C9907A07EA811E0)},
    0x8017CF58: {"sc02_0039": (116, 0xC884BB9EE9CCB8C7)},
    0x8017CFCC: {"sc02_0039": (40, 0x483A154A498A9B4A)},
    0x8017CFF4: {"sc02_0039": (40, 0xD306FAFD358F3CBE)},
}


# Earlier copies of the Tower callers retain their timing hold.
TOWER_EARLIER_CALLERS = {
    0x8012C354: {
        "1f493e9d4e1b3d830486396f67228a3b77ccbaf7da7d0ecbbd604887f0116e7d",
        "3ee21577767e61a084132b404d353cdf3f2efc30b3946297b90b7cb81d18a2f7",
        "43c4547365f3a1489458707ef6d693612ca115cb1e50b1c9a425743aac66ade9",
    },
    0x80182918: {
        "0f0ca5a402ac5b07d0d07835239b1e3dca84e7901c339ed3c38289178eb0899b",
    },
    0x80183FCC: {
        "d291052de8b96a64ce9299721645518e27e25d54ff747396369df812ace2f9b8",
    },
    0x80184EC0: {
        "ab241ef0b82bfbb75e909a39d97c8e48f54e684482951540e2e24c279d6c4fcf",
        "ae9b370a04dc19aa21be83b44411900ec71005d2af1be8616c13b659998b1fba",
    },
    0x8018968C: {
        "7eb67ab93e567687eb442044454d07fc609c5d3b1c34d4cd9fd00d6b3f6c9443",
        "d5900e57b02f7d861c5c700cc8c6a0ec3142a87c5bb4b2133b810873f834dc3f",
    },
    0x8018BA38: {
        "2e00200ca5822841766d39e5ccc038551eb2f348a6bdf929bf62683386064595",
        "4a59508f421e41922c07af3587d5b5720a6d029bc52f04b43d27a2fed827ad97",
        "e32056cefd34e8e4c45e272c30e5979b9360b9d1fa2326f2e47fbf37a84144e8",
    },
}


# Detached copies of the Tower introduction sources also require guest timing.
TOWER_INTRO_SOURCE_HASHES = {
    0x80180328: "7523e1c434e3836c07d8e1a0bd49c3f0b6ce510714a76c3c3a6abfda8228bfaa",
    0x80175414: "983d0490cf44043f9acb55e6b668fc000d8d8bb8a232a59148be19b5f1d3ee5c",
    0x8014C6AC: "e2ad7bc8a1b242aa883e84bb8a962df266977868245628b982b190e0c44af2d4",
    0x8016F0E4: "216ca316e0c7c7251965061445306fa712a5312219614a40e74308334dfbc51e",
    0x8017196C: "cbbd6fc52b0128d47c758ba5a850d6e54157ea931db64840c50cdb8231a980f1",
    0x801511A8: "334dd6eee0f81c9965797b8fdb8bb6dee184e58c9cd3776666fe46e0a2ecd644",
    0x80148634: "2166cb59c15b5fb12a7430e3ec6c5d7771a9f552f52e762813e9f27a17482658",
    0x8017E26C: "340937f93c7bf73860e0014d67ccacb4bb28e8ec4b51751f1b1e46ed6a691139",
    0x80129FF4: "314a917660a0829bb2b25a8dc636bf5a8cbf25f31ad04ad5e6bf57b143bc0553",
    0x8012A018: "f84ac1ab4216561965f662d0a6b757201ce62969c4c41ffcee41edcdad0b646d",
    0x8012A048: "b90d36e2fd2c20c0d30bc7655ec2a0271c7763abe95aa4520b01c6cd70bae4b9",
    0x8012A094: "c44a7c674de01826ddb3e3e289d6d714afb9cffafcf058d1096d3cad65fba5dd",
    0x8012A100: "7e176dd0a1fd7d0bc4848a4615e1b422d3d989ddedf15e8f0e5125c27803cbef",
    0x8017FCFC: "8074d448279c642cdc7a210fd31f9da7639da72835ce81524179498f764468db",
    0x801399F0: "ad9fb12d1be8b8f0ddcd3c5205141adbf7a352bb5c8a615de8788dc33d4f22cc",
    0x80139914: "ff6407e5b0fed4e54dfa0735e74428e103bc1503c8405b4a813b82e3946e0bcd",
    0x8013A8B0: "2d071880baf947d35578c2ff7e0d07d0153a6b99493345b11c4415fe6d65c67c",
    0x80147060: "4e3a609fc9a189be5a023fc989368806e0b6a153e80cc8191eabde8f573b7606",
    0x8017FF8C: "c8719c350874a9a81367d3bd62508d3595a56368601c288dcc45e25621c6c937",
    0x8017FFF0: "7621a18e7fc92be495fb76204810db643e9379099ba87e454668308648c92701",
}


def overlay_extent_source_only(entry, src, info, extents):
    pc = entry["vram"]
    variants = extents.get(pc)
    if not variants:
        return False
    if entry.get("region") in variants or (info and info["member"] in variants):
        return True
    if pc in FOREST_ATTACHMENT_EXTENTS and info and all(key in info for key in ("vram", "start", "data")):
        size, digest = FOREST_ATTACHMENT_EXTENTS[pc]
        if member_digest(info, pc, size) == digest:
            return True
    # Compare a mine entrance callback at its complete original extent even when the
    # supplied registry reports only a prefix or includes the following function.
    if pc in (0x8017D2E0, 0x8017D384, 0x8017F07C, 0x8017F120, 0x8017D3C0, 0x8017F15C) and info and all(key in info for key in ("vram", "start", "data")):
        if any(member_digest(info, pc, size) == digest for size, digest in variants.values()):
            return True
    # A member with a shared original body is identified only by its room or source.
    if pc in (0x8017EED8, 0x8017EF5C):
        size, fingerprint = variants["sc02_0039"]
        if info and info.get("data") is not None and "vram" in info and "start" in info:
            if member_digest(info, pc, size) == fingerprint:
                return True
    elif pc in (0x8017E758, 0x8017E794, 0x8017E910, 0x8017EA20, 0x8017EE9C, 0x8017F050):
        if info and info.get("data") is not None and all(key in info for key in ("vram", "start")):
            size, fingerprint = variants["sc02_0039"]
            if member_digest(info, pc, size) == fingerprint:
                return True
    elif pc in (0x80180994, 0x801809D0, 0x80180FBC, 0x80180FF8, 0x801814B0, 0x801814EC, 0x80181550, 0x8018172C, 0x80181844, 0x80181864, 0x80181FA8, 0x80182378, 0x801825A8):
        if info and info.get("data") is not None and "vram" in info and "start" in info:
            size, fingerprint = variants["sc03_0116"]
            if member_digest(info, pc, size) == fingerprint:
                return True
    elif pc in TOWER_INTRO_SOURCE_HASHES:
        if src.is_file() and hashlib.sha256(src.read_bytes()).hexdigest() == TOWER_INTRO_SOURCE_HASHES[pc]:
            return True
        # Custom registries can report another size. Identify the whole verified callback.
        extent = variants["sc02_0005"]
        if extent and info and all(key in info for key in ("vram", "start", "data")):
            size, digest = extent
            if member_digest(info, pc, size) == digest:
                return True
    elif info and any(variants.values()) and (entry["size"], member_digest(info, pc, entry["size"])) in variants.values():
        return True
    # The earlier source declared the terrain mode as a halfword.
    if pc == 0x8012CEB0 and hashlib.sha256(src.read_bytes()).hexdigest() == (
            "ef3dc9b0d0cb3acf3b6355373d1c8b5c159910ad42cdf4af9af10ad7beafbae0"):
        return True
    # The original mine emitter remains identifiable when its declared size is inaccurate.
    if (pc == 0x8017E640 and info and info.get("data") is not None and
            member_digest(info, pc, 280) == 0x5776B1218AB5E953):
        return True
    previous = TOWER_EARLIER_CALLERS.get(pc)
    if previous and hashlib.sha256(src.read_bytes()).hexdigest() in previous:
        return True
    # The first published forest grid query used unsigned formal coordinates.
    if pc == 0x80134A74 and hashlib.sha256(src.read_bytes()).hexdigest() == (
            "fe9e81f74f9e7bcf91978313554fa7061d2b19d74e7438334240edc82f6a9629"):
        return True
    for member in variants:
        known = ROOT / "src/overlays" / member / f"{pc:08x}.c"
        if src.resolve() == known or (known.is_file() and src.read_bytes() == known.read_bytes()):
            return True
    return False


# A scan of 96 slots must observe actor changes between successive guest loads.
# Earlier independent sources can be supplied to the generator without member data.
FOREST_ACTOR_EARLIER_SOURCES = {
    # Earlier forest sources use native-width slot comparisons or opaque scene callbacks.
    0x8014E98C: {"e553536d769677663fc542846a9246b969dea63116592025ef5e228ef2788f4d"},
    0x8014EA4C: {"95b8243f0e1262b8cceae7e23ca4756bdee24546ed273fca7a54c1a544cce109"},
    # This earlier sweep passed game addresses directly and skipped timed callbacks.
    0x80133784: {"6efbc131378b644fa5e4e98b800cc159e9c732d26d98a380ca87e22bc4e054a9"},
    0x8014E5B4: {
        "984d28309e384a1bbd6602aaf5e69a2d2b12e86138358e110d18aa62cb6d8652",
        "467fd42cda68349c509b667ee5ddddc4eeeb72bbd9278ff4adb3c575e8a94016",
        "50f6317326671bb5620ea61a6921b4ae2e9e3222a282366c9cbf26463102e7a2",
    },
    0x8014E83C: {
        "5a1363a246b5aa847ba8b4c51aba6a16571bd31382b243c4ba551f3e4ecba4ff",
        "0429456444e876365fe1c70afb5d7755eb2c2c90522c99a4982d57132af9f907",
        "10e4d54b8f6f354d30cf1d10b67852647cd97d1dbc352c28e5b55b3ba7d89056",
    },
}


def forest_floor_source_only(entry, src, info):
    if overlay_extent_source_only(entry, src, info, FOREST_FLOOR_EXTENTS):
        return True
    earlier = FOREST_ACTOR_EARLIER_SOURCES.get(entry["vram"])
    return earlier is not None and hashlib.sha256(src.read_bytes()).hexdigest() in earlier


# These forest action routines have byte-identical MAIN counterparts. They
# share code addresses while the running forest callbacks remain unqualified.
FOREST_ACTION_EARLIER_SOURCES = {
    0x8015AD78: "c69db65f195346d72eaca4ed9e7475cabb8345fe9282dbd27fda591c7064599d",
    0x8015ADB0: "4d683bf352164a9d211b42eb569d1788a495606afc075a57188808154dbc0589",
    0x8015BDD0: "defaa27d8e06c0bb2420db65ba384e8f59bcde6427f1f442fa0471e67d52e356",
    0x8015BF48: "cc4bc2cc555dbab8e0697c8aa89963a6112219673f3aa9e9e1803e0118f183af",
    0x8015C0C4: "0e438d7c3b4e43067b59b5da665b8adeefa90675ddd51846cd1748e171c76b7b",
    0x80161104: "692289b63197a663a1fd3048c171ea4dc342295c9d945474ea0d0f4e23981167",
    0x80161208: "9089ba26074b204806d2f8895c14e75fdb360b33b4e12dcc74ffc4f8eb2c53b5",
    0x801612B8: "7388a8f6c8aae5acf1b1d051a30796f83730471f713dcc100ba97a665c25483b",
    0x8016130C: "cbee9e8f11aea41396bcdf978e0937aa7db1c7bf96567e354e55bb72ef27af84",
}


def forest_action_source_only(entry, src, info):
    pc = entry["vram"]
    previous = FOREST_ACTION_EARLIER_SOURCES.get(pc)
    if previous is None:
        return False
    member = info["member"] if info else None
    if entry.get("region") == "sc01_0077" or member == "sc01_0077":
        return True
    forest = ROOT / f"src/overlays/sc01_0077/{pc:08x}.c"
    text = src.read_bytes()
    if src.resolve() == forest or (forest.is_file() and text == forest.read_bytes()):
        return True
    if hashlib.sha256(text).hexdigest() != previous:
        return False
    # The earlier 612B8 source is also the published MAIN implementation.
    if pc != 0x801612B8:
        return True
    return (member if member is not None else entry.get("region")) != "main_0012"


# Actor event tests call the game flag service, then fill one of two shared slot
# banks. Dispatch needs the real guest stack, memory ordering and event timing.
FOREST_EVENT_EXTENTS = {
    0x80146750: dict.fromkeys(("main_0012", "sc01_0077", "sc01_0005"), (188, 0x43E37B45848E8CD0)),
    0x8014680C: dict.fromkeys(("main_0012", "sc01_0077", "sc01_0000"), (188, 0xB522F1058C2D9B5F)),
    0x801498E0: dict.fromkeys(("sc01_0077", "main_0012", "sc01_0005"), (116, 0xF1ECAE0BE88F0991)),
    # The secondary request also calls the active field overlay and positional sound.
    0x80149954: dict.fromkeys(("main_0012", "sc01_0077"), (272, 0xF7C0F4613A875408)),
    0x80149AA8: dict.fromkeys(
        ("sc01_0077", "main_0012", "sc02_0035", "sc03_0001", "sc01_0005", "sc02_0031"),
        (44, 0x9855CACC76C548AD)),
    0x80149B54: dict.fromkeys(
        ("sc01_0077", "main_0012", "sc02_0035", "sc03_0001", "sc01_0005"),
        (88, 0x4B524B4DAA9C4848)),
    0x8014BEF8: dict.fromkeys(("sc01_0077", "main_0012"), (32, 0x6C7CD17EBBB3736A)),
}


# A supplied registry may reference a detached source with no member image or
# installed canonical C. Pin the typed sources and their earlier forms.
FOREST_EVENT_SOURCE_HASHES = {
    0x80146750: {
        "b99fbdf613593481a6fcc96bdeef1901edf6f0cfdd0063ff0d10136683a108e9",
        "b1411633fbec3d5518fe4bfc76f4dd5b2342371a3cc52a4c849cb0d870855c5d",
        "aba3bc1dd37589893b46ddf9ce195e106a960d23662f1e87998e3aa31e4e624e",
    },
    0x8014680C: {
        "bc1c6c8ee755b2c08d6188c24e81e0604c04e3e847a662d30e331a38bff0e68d",
        "081dd7635189bc386153916b89f2b73c6a6432943e99736ce3b0ec3ca2dce393",
        "b58b9e8e7cdecee41ca426e630a926060b795db13a7084932a964478faf3ecc3",
    },
    0x801498E0: {
        "6ea31a4287a152c69cb86aaa1b8485b43f5a605e6104fb592233497cdb0e20cf",
        "7f022c3cb05db879d3ce8fa0aafb48e2b06cbee5632bcdd56db8fe7a2da8a50b",
        "03c287fabb5fb626a804fd6162e8e35d44baad7ffbab56394171464d99dce3a1",
    },
    0x80149954: {
        "2823f787de7a201e743ae9fc89ec83635cb78fd6ebbfa2f9fbb8fd3792dfaa99",
        "71ba60eba7e368b1d9442ef0d21ebb3c5e1c67b67cf76a0fdd12923fe32691f0",
        "d1e47433da72bb4ba914a477d70797b76783e7dbf50dab478b5898c58d632068",
        "7bb9da4482e7b9ecc10403556dce1f943fe5526d891d86e40c39dac7cf83ae48",
    },
    0x80149AA8: {
        "a1d83d6898fa0b250683a1234803711dd77eadbc79f70eccbea2e037ee9b8ed9",
        "cb2bc6a23da2797780eac48fb1026a0bb0da0473efb05208ef5b90d0a1d36b9f",
        "d7f73b4ec55e1944d574102781944abc2c8ed731b4eb9fa65f9e24dc7195126f",
    },
    0x80149B54: {
        "2c3d37cccc087f8aa4009064ef5e40effa48d0e21ee5101ce11eed1ed2c3472f",
        "5d280b95fefddc0aa22df0bb5e872980ec772fcdd85c0c915089c40c7d37e174",
        "702f4cee930883e0af726b54d7fa237eeaed38f0f04315397ba4aeac51951a4d",
    },
    0x8014BEF8: {
        "8ea00334968e3a108b4e030c106a7d79a3e7c8a7619b1857111f2ae660968689",
        "83f23176510941418825a3f94c6e0ea739fdb2995c08ff4c6083839799395ff7",
        "05b45f349f7e57c82d88063b6e0eab6b601ff51b5d261233187687c79a8e9895",
    },
}


def forest_event_source_only(entry, src, info):
    if overlay_extent_source_only(entry, src, info, FOREST_EVENT_EXTENTS):
        return True
    known = FOREST_EVENT_SOURCE_HASHES.get(entry["vram"])
    return known is not None and hashlib.sha256(src.read_bytes()).hexdigest() in known


# The forest controller reads two guest callback tables. The position update
# and its orientation child also occur at the same extent in other members.
FOREST_GROUND_ATTACHMENT_EXTENTS = {
    0x801627E8: {"sc01_0077": (76, 0x11DB10D2EBC2E904)},
    0x80162834: {"sc01_0077": (60, 0xB5C00E87976B8C6E)},
    0x801628F4: dict.fromkeys(
        ("main_0012", "sc01_0077", "sc01_0005", "sc02_0035", "sc03_0001"),
        (116, 0xE8BEA49CBBDD2CBA)),
    0x80162968: dict.fromkeys(("main_0012", "sc01_0077"), (344, 0x78B2BD88E99ADACF)),
}

# Custom registries can supply the current or historical C separately.
FOREST_GROUND_ATTACHMENT_SOURCES = {
    0x801627E8: {
        "98cdc6ab80618064f5aee85ed3c51397caf4ad6ac58e8e940ea827dcd94ce635",
        "e628a398039451e58e609dffbb8ed0d5e6c680e9b1ea965b8db9a1edaec1c7df",
    },
    0x80162834: {
        "296a381a792d38f110fca48b6baf0774afa678af8dcae46887bf527abcf222a1",
        "dd17c1749955005676b34d2fa3fc6a37afcab23feb7e28f856b8e6c9518eab08",
    },
    0x801628F4: {
        "13fc104531ec53d04a85f3bbcc5b767f891095ab7fea7a7fd8469d28a91cff5c",
        "3341b8592c8feb1fb3a05067ac4d4f2955e0c99fe65dbfd48f675075e11a3ca9",
        "55393555d67c12a6fa7f5c04d1983d220a6ec7207868cd2fda9971e1bd07c7b0",
        "d1ddb83357fd957f63e6b07631ed8109d8c9918408e8b7cd316afb06fab162a0",
    },
    0x80162968: {
        "21d4f010c80bb023286b071be2742638cee720e1dfe4793a551635a31b453b1f",
        "70f84517d55de62f5ca9bc5c34128a7f42b0c723cfa6ce80b3f056dcc6a37c13",
        "3d358588c6fc0138dd30c7581078f5c3fc8455526e07ec0f5c47dca1e283f025",
        "aa45cfe58d61f388fdf7cf0f50f9d2fa95eaaa1e5908ea28679fa00df1f225b3",
    },
}


def forest_ground_attachment_source_only(entry, src, info):
    pc = entry["vram"]
    variants = FOREST_GROUND_ATTACHMENT_EXTENTS.get(pc)
    if not variants:
        return False
    # The verified game extent is independent of a supplied registry's size.
    size, _ = next(iter(variants.values()))
    if overlay_extent_source_only(dict(entry, size=size), src, info, FOREST_GROUND_ATTACHMENT_EXTENTS):
        return True
    return hashlib.sha256(src.read_bytes()).hexdigest() in FOREST_GROUND_ATTACHMENT_SOURCES[pc]


def mine_slot_update_source_only(entry, src, info):
    if entry["vram"] not in (0x8012A908, 0x80162B1C, 0x8017849C, 0x8017869C, 0x801787D4):
        return False
    return mine_member_source_only(entry, src, info)


# These two mine callers previously declared the spawned actor type locally.
MINE_SPAWN_EARLIER_SOURCES = {
    0x8017DE00: "52960ac86d06c2f3eb74c4038e487dfefec11a4e5e3818c5962e15a78b25ded3",
    0x8017DE84: "511580a1fff1d456aa1c3f69b4038ca170ac3873c0388f80c4bd343e4a3bafd4",
}


def mine_member_source_only(entry, src, info):
    pc = entry["vram"]
    # MAIN/0012 has a separately selected implementation at the same address and digest.
    if entry.get("region") == "sc02_0039" or (info and info["member"] == "sc02_0039"):
        return True
    earlier = MINE_SPAWN_EARLIER_SOURCES.get(pc)
    if earlier and hashlib.sha256(src.read_bytes()).hexdigest() == earlier:
        return True
    mine = ROOT / f"src/overlays/sc02_0039/{pc:08x}.c"
    return src.resolve() == mine or (mine.is_file() and src.read_bytes() == mine.read_bytes())


# This forest member rotates the local vector through guest matrix services.
SC01_FOREST_MOTION_EXTENTS = {
    # Player frame initialization orders controller, animation and scene callbacks.
    0x80146128: (568, 0x293FA1C5C6D94373),
    # Normal player action still uses the guest scene, animation and contact services.
    0x8015A3C8: (1972, 0x9ACBD2045A1A10EF),
    0x80147DC0: (132, 0x03C49C0F21FBAF45),
    0x80148634: (20, 0xDBD44BBDA70887AC),
    0x8016F14C: (96, 0x90B512EA84F799EB),
}


def scene_player_motion_source_only(entry, src, info):
    pc = entry["vram"]
    if pc in SC01_FOREST_MOTION_EXTENTS:
        extent = SC01_FOREST_MOTION_EXTENTS[pc]
        regions = ("sc01_0077",)
    else:
        extent = SC01_PLAYER_MOTION_EXTENTS.get(pc)
        if extent is None:
            return False
        regions = ("sc01_0000", "sc01_0077") if pc == 0x801473EC else ("sc01_0000",)
    if pc == 0x80146128 and info:
        member = info["member"]
        if member == entry.get("region") and member not in regions:
            # An independently retained implementation in another member keeps its existing policy.
            other = ROOT / "src/overlays" / member / f"{pc:08x}.c"
            if other.is_file() and src.read_bytes() == other.read_bytes():
                return False
    if entry.get("region") in regions or (info and info["member"] in regions):
        return True
    if info and entry["size"] == extent[0] and member_digest(info, pc, extent[0]) == extent[1]:
        return True
    for region in regions:
        source = ROOT / "src/overlays" / region / f"{pc:08x}.c"
        if src.resolve() == source or (source.is_file() and src.read_bytes() == source.read_bytes()):
            return True
    return False


# The two immediate halfwords locate a member's command switch table. The
# rest of each function agrees across all 107 members containing the routine.
FOREST_ANIMATION_EXTENTS = {
    0x8015444C: (1452, 0xA8, 0xB0, "1aeab53fb987f6287a82cdea2cbad93c788b21cd4d10eb075d43ea2988629754"),
    0x801549F8: (124, 0x18, 0x1C, "229f52f5ab5ebb7e65c3c045897b853d133cc2e34210650ba08d0499d3867717"),
}
# Historical MAIN source versions and an earlier forest placeholder can
# also be supplied by path without region metadata. These are Git blob IDs.
FOREST_ANIMATION_PRIOR_BLOBS = {
    "30defaf674ad689c6e7744432f24c442a2bbd69f",
    "7dfd371371c4b63382f77d879c77d6afc9d161f0",
    "40949f5d9dfe36e5e955816462f939b1a768427b",
}


def forest_animation_source_only(entry, src, info):
    pc = entry["vram"]
    extent = FOREST_ANIMATION_EXTENTS.get(pc)
    if extent is None:
        return False
    # Both named members contain this command family. The calling stack,
    # recursion and external services still need a timed player implementation.
    known = ("sc01_0077", "main_0012")
    if entry.get("region") in known or (info and info["member"] in known):
        return True
    canonical = ROOT / f"src/overlays/sc01_0077/{pc:08x}.c"
    if src.resolve() == canonical:
        return True
    if pc == 0x8015444C and src.resolve() in (
            ROOT / "src/overlays/main_0012/8015444c.c",
            ROOT / "src/overlays/sc01_0005/8015444c.c"):
        return True
    if info and all(key in info for key in ("vram", "start", "data")):
        size, high, low, digest = extent
        start = pc - info["vram"] + info["start"]
        if 0 <= start and start + size <= len(info["data"]):
            body = bytearray(info["data"][start:start + size])
            body[high:high + 2] = body[low:low + 2] = b"\0\0"
            if hashlib.sha256(body).hexdigest() == digest:
                return True
    contents = src.read_bytes()
    if canonical.is_file() and contents == canonical.read_bytes():
        return True
    if pc == 0x8015444C:
        header = f"blob {len(contents)}\0".encode("ascii")
        return hashlib.sha1(header + contents).hexdigest() in FOREST_ANIMATION_PRIOR_BLOBS
    return False


def check_sound_record_clear(entry, src, info):
    pc = entry["vram"]
    if pc != 0x8002CC4C:
        return
    member = info.get("member") if info else None
    region = entry.get("region") or member or "main"
    # The record routine or an independent row can cross adjacent code sections.
    code_end = pc
    if info and all(key in info for key in ("vram", "start", "data")) and pc >= info["vram"]:
        offset = pc - info["vram"] + info["start"]
        if 0 <= offset < len(info["data"]):
            for lo, hi in sorted(info.get("code") or ()):
                if lo > code_end:
                    break
                code_end = max(code_end, hi)
            code_end = min(code_end, pc + len(info["data"]) - offset)
    has_source = src.is_file()
    # Both counts and interrupt timing are observable during this particular loop.
    # The original extent is fixed even if a custom registry reports another size.
    if (region == "main" or member == "main" or
            (pc + 104 <= code_end and member_digest(info, pc, 104) == 0xd34ed2926778bca1) or
            (has_source and (src.resolve() == ROOT / "src/main/8002cc4c.c" or
                             hashlib.sha256(src.read_bytes()).hexdigest() in {
                                 "dab9b0a30d64d82146786921e5c9e152f0315a26469b72cfbadfdb2aa8559948",
                                 "433e0d6a8b1b3de89c0006011e131912a25594df092bedbc6e37022ae453d23a",
                             }))):
        raise Reject("timing", "sound record clearing requires per-store memory and event timing")
    if not has_source:
        raise Reject("source", "sound record candidate source is missing")
    # A declared foreign row must not become an unqualified resident registration.
    if region != member or not pc < pc + entry["size"] <= code_end:
        raise Reject("overlay", "sound record candidate requires its own mapped member code")


def build_one(entry, out, symbols, hooks, clang, info=None):
    vram, size, src = entry["vram"], entry["size"], ROOT / entry["source"]
    name = entry["name"]
    member = info.get("member") if info else None
    tag = f"{member}_{vram:08x}" if member else f"{vram:08x}"
    work = out / "ir"
    try:
        if sdk_overlay(entry.get("region"), vram, size) or sdk_overlay(member, vram, size):
            raise Reject("deny", "SDK library code in this overlay member")
        check_sound_record_clear(entry, src, info)
        # The matching menu-item C has no player timing/interrupt qualification.
        # Different members can independently implement a function at this PC.
        if (vram == 0x8017ceb8 or name.lower() == "func_8017ceb8") and (
                member == "main_0012" or
                src.resolve() == ROOT / "src/overlays/main_0012/8017ceb8.c" or
                hashlib.sha256(src.read_bytes()).hexdigest() ==
                "fe54e82ba5ccfd0720da3f72d022febe8510a568c65fede72a8650ed6cb39c2f"):
            raise Reject("deny", "MAIN member 0012 first card-menu item is source-only")
        # The File menu follows a table of guest addresses and writes each glyph
        # at its own bus issue. The matching C assumes the original address space.
        if (vram == 0x8017DCB0 or name.lower() == "func_8017dcb0") and (
                entry.get("region") == "main_0012" or member == "main_0012" or
                src.resolve() == ROOT / "src/overlays/main_0012/8017dcb0.c" or
                (src.is_file() and hashlib.sha256(src.read_bytes()).hexdigest() ==
                 "a986c6209f43cdd7f62d7d6794a8c6041b1454ae1fa259c935e2eb7cd9edcbc6") or
                (info and all(k in info for k in ("vram", "start", "data")) and
                 member_digest(info, 0x8017DCB0, 120) == 0x0E160E7268CEF357)):
            raise Reject("memory", "MAIN member 0012 glyph writes require timed guest memory accesses")
        # A single whole-function charge can miss a channel update during the scan.
        if member is None and vram in (0x800331D4, 0x8002F248, 0x8002DC68):
            raise Reject("timing", "sound channel lookup and its callers require event timing")
        # The distance caller uses a real guest stack frame and two distinct return PCs.
        if member is None and vram == 0x80013328:
            raise Reject("timing", "distance requires the nested guest call frames and event timing")
        # The resident flag read has an interruptible load and return delay slot.
        # Member aliases of the same code must use the same CPU fallback.
        if vram == 0x80029178 and (
                member is None or entry.get("region") == "main" or
                (size == 40 and member_digest(info, vram, size) == 0xe209fbd89c4aa62b) or
                src.resolve() == ROOT / "src/main/80029178.c" or
                hashlib.sha256(src.read_bytes()).hexdigest() ==
                "2fd11caf58aeb355272b2a004fb88769a8df9fc93eda9e54149f6f7388fd606d"):
            raise Reject("timing", "resident flag read requires guest load and interrupt timing")
        wrapped_angle = member is None and vram == 0x80012AB0
        if wrapped_angle and (size != 12 or hashlib.sha256(src.read_bytes()).hexdigest() !=
                "7e1d58c1eca9742e5c025641f349f594066b5cadd3b4eb1c4a64f8706bb7ace2"):
            raise Reject("source", "wrapped-angle step requires the verified clean source")
        # Use the forest's guest call sequence for each MoveImage record.
        # The SC02 and MAIN versions already share its bytes and address.
        if vram == 0x80146014 and (entry.get("region") == "sc01_0077" or
                member == "sc01_0077" or src.resolve() == ROOT / "src/overlays/sc01_0077/80146014.c"):
            raise Reject("timing", "forest image transfers require guest MoveImage timing")
        if (vram == 0x80165AC8 and
                (entry.get("region") == "sc01_0077" or member == "sc01_0077")):
            raise Reject("timing", "the forest ability callback requires the active code image and guest return timing")
        if (vram in (0x8017E868, 0x8017E8A4, 0x8017E924, 0x8017E974,
                     0x8017E9C0, 0x8017EA2C) and
                (entry.get("region") == "sc01_0077" or member == "sc01_0077")):
            raise Reject("timing", "the forest player scene callback requires the active stage table and guest return timing")
        if (vram in (0x8012C724, 0x8013E298, 0x8013E370, 0x8013E410, 0x8013E448) and
                (entry.get("region") == "sc01_0077" or member == "sc01_0077")):
            raise Reject("timing", "forest placement requires ordered actor allocation and callback timing")
        if (vram in (0x8017EA98, 0x8017EAC0, 0x8017EC40) and
                (entry.get("region") == "sc01_0077" or member == "sc01_0077")):
            raise Reject("timing", "forest settling effects require ordered state, random and particle calls")
        if (vram in (0x8017E1D4, 0x8017E460, 0x8017F240) and
                (entry.get("region") == "sc01_0077" or member == "sc01_0077")):
            raise Reject("timing", "forest tutorial requires ordered stage dispatch, countdown and actor allocation")
        if (vram == 0x8016345C and
                (entry.get("region") == "sc01_0077" or member == "sc01_0077")):
            raise Reject("timing", "forest projectile power lookup and contact query require guest call timing")
        if (vram in (0x80168AE4, 0x80168B70) and
                (entry.get("region") == "sc01_0077" or member == "sc01_0077")):
            raise Reject("timing", "forest impact emission and release require guest call timing")
        if forest_animation_source_only(entry, src, info):
            raise Reject("timing", "animation commands require checked scripts, recursive calls and timed services")
        if forest_floor_source_only(entry, src, info):
            raise Reject("memory", "floor collision requires checked guest memory and instruction timing")
        # Older forest source calls the guest phase address directly.
        if vram == 0x80159968 and hashlib.sha256(src.read_bytes()).hexdigest() == (
                "bcac09920911836169ac8b046074783fa94dde9ffcf4df65542f7bfbfedb00bc"):
            raise Reject("timing", "forest phase requires its guest callback and return timing")
        if vram in (0x8012BCCC, 0x8012C51C, 0x8012C658, 0x8012D5E4) and (
                entry.get("region") == "sc01_0077" or member == "sc01_0077" or
                src.resolve() == ROOT / f"src/overlays/sc01_0077/{vram:08x}.c"):
            raise Reject("timing", "forest actor queries and allocation require the active room state and guest timing")
        if vram in (0x8012C354, 0x8012C658, 0x8012C890):
            # The template and slot search have identical instructions in other rooms.
            # Only the Tower slot initializer can be identified by its code alone.
            tower = ROOT / "src/overlays/sc02_0005" / f"{vram:08x}.c"
            tower_initializer = (vram == 0x8012C890 and size == 596 and info and
                                 all(k in info for k in ("data", "start", "vram")) and
                                 member_digest(info, vram, size) == 0xF467CE302666990C)
            earlier_pool = (vram == 0x8012C658 and
                            hashlib.sha256(src.read_bytes()).hexdigest() == TOWER_POOL_PRIOR_SOURCE)
            earlier_template = (vram == 0x8012C354 and
                                hashlib.sha256(src.read_bytes()).hexdigest() in TOWER_EARLIER_CALLERS[vram])
            if (tower_initializer or earlier_pool or earlier_template or member == "sc02_0005" or
                    entry.get("region") == "sc02_0005" or
                    (tower.is_file() and (src.resolve() == tower or src.read_bytes() == tower.read_bytes()))):
                raise Reject("timing", "tower actor allocation requires its room state and guest call timing")
        if overlay_extent_source_only(entry, src, info, SCENE_PHASE_EXTENTS):
            raise Reject("timing", "scene operation requires instruction-level guest state and event timing")
        # This player wrapper shares its instructions with MAIN. Its terrain query identifies the mine.
        if (vram == 0x8014FA70 and entry["size"] == 336 and info and info.get("data") is not None and
                member_digest(info, vram, 336) == 0x219DBA359D04D973 and
                member_digest(info, 0x80133784, 812) == 0x309DE57F9074D361):
            raise Reject("timing", "mine terrain action requires its room query and event ordering")
        # The segment wrapper shares its instructions across rooms. Its terrain query identifies the mine.
        if (vram == 0x8012CEB0 and entry["size"] == 248 and info and info.get("data") is not None and
                member_digest(info, vram, 248) == 0x9B48077248B310BA and
                member_digest(info, 0x80133784, 812) == 0x309DE57F9074D361):
            raise Reject("timing", "mine vector terrain queries require ordered room contacts")
        # The actor and vector wrappers share instructions across rooms. Their terrain service identifies the mine.
        if (vram == 0x8012CC88 and entry["size"] == 420 and info and info.get("data") is not None and
                member_digest(info, vram, 420) == 0xA21742D4B95893BB and
                member_digest(info, 0x8012CEB0, 248) == 0x9B48077248B310BA and
                member_digest(info, 0x80133784, 812) == 0x309DE57F9074D361):
            raise Reject("timing", "mine actor terrain movement requires its ordered vector and terrain queries")
        if forest_action_source_only(entry, src, info):
            raise Reject("timing", "forest action requires ordered guest services and state updates")
        if forest_event_source_only(entry, src, info):
            raise Reject("timing", "actor event submission requires guest memory and event timing")
        if vram in (0x801627E8, 0x8016E95C, 0x801754A8) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine state dispatch requires current callback tables and timer ordering")
        if vram == 0x8012956C and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine backdrop submission requires current scratch, graphics services and timing")
        if vram in (0x8016E918, 0x8016EAC0, 0x8016EB3C, 0x8016EBA8) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine display movement requires its guest drawing and phase ordering")
        if vram in (0x8016E8F0, 0x8016E9EC, 0x8016EC0C) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine display text and sprites require their guest template, allocator and graphics ordering")
        if vram in (0x80173EF8, 0x80174158, 0x80174188, 0x801741A8) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine player award services require their original flag, sound and environment call ordering")
        if vram in (0x80173A28, 0x80173A60, 0x80173B4C) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine award controller setup requires the original actor lookup, model data and phase callback")
        if vram in (0x80173B88, 0x80173BC0, 0x80173C64, 0x80173CB4, 0x80173E1C, 0x801741EC, 0x80174274) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine player award phases require original actor movement, model, sound and event-object timing")
        if vram == 0x8015444C and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine model animation commands require scoped sequence, model and graphics service state")
        if vram in (0x80134310, 0x8013435C) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine terrain plane arithmetic requires original member and guest call timing")
        if vram in (0x80147060, 0x8014706C, 0x8014C118, 0x80171A1C, 0x80171A34, 0x80171CC4, 0x80174684) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine award actor services require original actor state and guest call timing")
        if vram in (0x80179B74, 0x8017A094) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine script queue services require the original member, shared payload and guest call timing")
        if vram in (0x801484B0, 0x801485B8) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine local pose reads require the original model state and guest call timing")
        if vram in (0x8012C750, 0x8013E4B4) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine initial placement needs the original actor pool, callbacks and guest call timing")
        if vram in (0x801473EC, 0x80147460) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine actor movement needs the original actor and guest call timing")
        if vram in (0x8012C1B8, 0x80131FDC, 0x8013221C) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine door setup requires the room callback table, model pool and original call timing")
        if vram == 0x80178608 and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine field reset requires the room actor pool, placement walker and guest call timing")
        if vram == 0x8012853C and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine field stage four requires the source room hook and resident substate timing")
        if forest_ground_attachment_source_only(entry, src, info):
            raise Reject("timing", "ground attachment requires checked guest callbacks, return values and timing")
        if vram in (0x8017DDB0, 0x8017DE00, 0x8017E25C, 0x8017E2F4, 0x8017E428, 0x8017E500) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine actor actions require original model services, callbacks and event timing")
        if vram in (0x801336E8, 0x80136BC4) and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine grid setup requires current room references and write timing")
        if vram == 0x801457A4 and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine descriptor lookup requires its room table and branch timing")
        if vram == 0x8012C890 and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine actor allocation requires its room setup handlers and timing")
        if vram == 0x8017DC7C and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine rotor update requires its original model and sound timing")
        if vram == 0x8017DE84 and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine actor setup requires its original model services and event timing")
        if vram == 0x8017DFE8 and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine collision actor state requires the original callback frame and timing")
        if vram == 0x80129CF8 and mine_member_source_only(entry, src, info):
            raise Reject("timing", "mine camera requires guest callback and view publication timing")
        if vram in (0x80129010, 0x8012A908, 0x8012A988, 0x8017849C, 0x8017869C, 0x801787D4) and (
                entry.get("region") == "sc01_0077" or member == "sc01_0077" or
                src.resolve() == ROOT / f"src/overlays/sc01_0077/{vram:08x}.c"):
            raise Reject("timing", "forest actor and effect updates require their room scheduler")
        if mine_slot_update_source_only(entry, src, info):
            raise Reject("timing", "mine slot updates require guest callback and memory timing")
        if scene_player_motion_source_only(entry, src, info):
            raise Reject("memory", "SC01 player motion requires checked guest memory and instruction timing")
        if vram in DENY:
            raise Reject("deny", DENY[vram])
        facing = FACING_EXTENTS.get(vram)
        if facing:
            source = ROOT / "src/overlays/sc02_0039" / f"{vram:08x}.c"
            if (src.resolve() == source or
                    (info is not None and size == facing[0] and member_digest(info, vram, size) == facing[1]) or
                    (info is None and source.is_file() and src.read_bytes() == source.read_bytes())):
                raise Reject("memory", "facing region requires checked guest accesses and path timing")
        if member == "sc03_0001" and vram in (0x8018C89C, 0x8018C8D4):
            raise Reject("memory", "signed menu table index can reach a guest RAM mirror")
        for pc, kind in hooks.items():
            if not vram <= pc < vram + size:
                continue
            if pc == vram and (vram in INTRINSIC_PCS or kind == "entry"):
                continue
            raise Reject("hooks", f"port references {pc:08x}")
        text = strip_comments(src.read_text(errors="replace"))
        body = text.split("#else", 1)[1] if "#else" in text else text
        # `long long` is 64-bit on the PS1 and the host; a lone `long` is
        # 32-bit on the PS1 and becomes `int` in the host copy (host_long).
        if re.search(r"\bsizeof\b", body):
            raise Reject("source", "uses 'sizeof'")
        o0 = work / f"{tag}.O0.ll"
        compile_src = src
        raw = src.read_text(errors="replace")
        host = host_long(host_source(raw))
        # `register T x __asm__("$5")` pins a local to a MIPS register for the
        # byte match only; it has no meaning on the host. Empty asm statements
        # that are identities go too; any other inline asm is still refused
        # (rule "calls").
        stripped = strip_empty_asm(host)
        if stripped is None:
            raise Reject("calls", "empty asm with an untied output")
        host = stripped[0]
        if host != raw:
            compile_src = work / f"{tag}.host.c"
            compile_src.write_text(host)
        # Win64 longjmp unwinds through native-lane frames using their unwind tables.
        unwind = [] if object_elf() else ["-fasynchronous-unwind-tables"]
        r = run([clang, *CFLAGS, *unwind, "-O0", "-Xclang", "-disable-O0-optnone", "-S", "-emit-llvm",
                 "-I", str(ROOT / "include"), "-I", str(src.parent), str(compile_src), "-o", str(o0)])
        if r.returncode:
            raise Reject("compile", (r.stderr.strip().splitlines() or ["?"])[0][:160])
        ir = o0.read_text()
        check_layout(ir)
        rewritten = lower_arith(narrow_pointer_cells(rewrite_symbols(
            ir, symbols, member, info["code"] if info else ())))
        sub = work / f"{tag}.sub.ll"
        sub.write_text(rewritten)
        opt = work / f"{tag}.opt.ll"
        # No auto-vectorisation or unrolling: their splat and remainder idioms
        # (insertelement poison, phi poison) trip
        # the undef/poison check, and lane functions gain nothing from it.
        r = run([clang, f"--target={HOST_TARGET}", "-O2", "-fno-vectorize", "-fno-slp-vectorize", "-fno-unroll-loops",
                 "-S", "-emit-llvm", "-w", str(sub), "-o", str(opt)])
        if r.returncode:
            raise Reject("compile", "optimize: " + (r.stderr.strip().splitlines() or ["?"])[0][:160])
        optir, icalls = rewrite_indirect_calls(opt.read_text())
        internal = set(re.findall(r"^define internal [^@]*@([\w.$]+)\(", optir, flags=re.M))
        check_optimized(optir, internal)
        check_device_pointers(optir, info)
        check_polling_loops(optir)
        defs = lane_names(optir)
        own = f"lane_{qualify(member, name)}"
        if own not in defs:
            raise Reject("symbols", f"{name} not defined by {entry['source']}")
        sigs = {}
        for d in defs:
            header = next(h for h, _ in functions(optir) if re.search("@" + d + r"\(", h))
            sigs[d] = parse_signature(header, d)
        optir, stubs = resolve_calls(optir)
        final = work / f"{tag}.lane.ll"
        final.write_text(object_ir(optir))
        if not object_elf():
            # clang emits unwind data for each framed function only under
            # uwtable (frameless ones never sit below a call and need none).
            groups = dict(re.findall(r"^attributes (#\d+) = \{(.*)\}$", optir, flags=re.M))
            for header, _ in functions(optir):
                refs = re.findall(r"#\d+", header.split(")")[-1])
                if not any("uwtable" in groups.get(g, "") for g in refs):
                    raise Reject("compile", "function without uwtable (no Win64 unwind data)")
        obj = work / f"{tag}.o"
        r = run([clang, f"--target={OBJECT_TARGET}", "-O2", "-fno-vectorize", "-fno-slp-vectorize", "-fno-unroll-loops", "-c", *pie(), "-w", str(final), "-o", str(obj)])
        if r.returncode:
            raise Reject("compile", "object: " + (r.stderr.strip().splitlines() or ["?"])[0][:160])
        undefined = run(["nm", "-u", str(obj)]).stdout.split()
        for u in undefined:
            if u != "U" and not re.fullmatch(r"lane_(?:call_)?" + QFUNC + r"(?:__\w+)?", u) \
                    and not u.startswith("lane_icall__") \
                    and u not in MEMORY_FNS and u not in LANE_HELPERS:
                raise Reject("symbols", f"object still references {u}")
        digest = member_digest(info, vram, size) if info else None
        if info and digest is None:
            raise Reject("overlay", "extent outside the member file")
        result = dict(name=name, qname=qualify(member, name), member=member, vram=vram, size=size,
                      source=entry["source"], admitted=True, obj=str(obj), defs=sigs, stubs=stubs,
                      icalls=icalls, digest=digest)
        if wrapped_angle:
            result["step_handler"] = "musashi_native_lane_wrapped_angle"
        return result
    except Reject as e:
        return dict(name=name, qname=qualify(member, name), member=member, vram=vram, size=size,
                    source=entry["source"], admitted=False, rule=e.rule, detail=e.detail)


def psyq_hooked(hooks, registry=None):
    """PsyQ compat-table entry points (pc_port/platform/psyq/bfm_psyq_compat.def)
    whose body contains a PC the port sequences or observes: the lane must
    not run those natively either. A body without a registry size ends at
    the next known function start."""
    defs = ROOT / "pc_port/platform/psyq/bfm_psyq_compat.def"
    if not defs.exists():
        return []
    pcs = [int(m.group(1), 16) for m in
           re.finditer(r"^BFM_PSYQ\((0x[0-9A-Fa-f]+),", defs.read_text(), flags=re.M)]
    if registry is None:
        registry = load_matches()
    sizes = {m["vram"]: m.get("size") for m in registry if m["region"] == "main"}
    starts = sorted(set(sizes) | set(pcs))
    blocked = []
    for pc in sorted(set(pcs)):
        size = sizes.get(pc)
        if size:
            end = pc + size
        else:
            later = [s for s in starts if s > pc]
            end = later[0] if later else pc + 4
        if any(pc <= h < end for h in hooks):
            blocked.append(pc)
    return blocked


# ---- Emit wrappers, stubs and the table ------------------------------------

def c_type(ty, ext):
    if ty == "ptr":
        return "void *"
    signed = ext[0]
    bits = int(ty[1:])
    return f"{'int' if signed else 'uint'}{bits}_t"


def to_guest(expr, ty, ext):
    """Host value -> the 32-bit guest register the MIPS ABI would carry."""
    if ty == "ptr":
        return f"(uint32_t)(uintptr_t)({expr})"
    if ext[0]:
        return f"(uint32_t)(int32_t)({expr})"
    return f"(uint32_t)({expr})"


def from_guest(expr, ty, ext):
    if ty == "ptr":
        return f"(void *)(uintptr_t)(uint32_t)({expr})"
    return f"({c_type(ty, ext)})({expr})"


# These main-executable helpers all take a signed byte offset into the same
# script-variable table. Their matched C dereferences the mapped host address.
SCRIPT_VAR_WIDTH = {
    "lane_func_800291A0": 1, "lane_func_800291B4": 1,
    "lane_func_800291C8": 2, "lane_func_800291DC": 2,
    "lane_func_800291F0": 4, "lane_func_80029204": 4,
}


def script_var_guard(target, index):
    width = SCRIPT_VAR_WIDTH.get(target)
    if width:
        return f"musashi_native_lane_script_var_direct({index}, {width}u)"
    return None


def emit_table(admitted, out):
    defs = {}
    for a in admitted:
        if not a.get("step_handler"):
            defs.update(a["defs"])
    stubs = {}
    for a in admitted:
        stubs.update(a["stubs"])
    lines = [
        "/* Generated by tools/native_lane_gen.py; do not edit. */",
        "#include <stdint.h>",
        "#include <stddef.h>",
        '#include "musashi_native_lane.h"',
        "",
    ]
    for d, (ret, rext, params) in sorted(defs.items()):
        proto = ", ".join(c_type(t, e) for t, e in params) or "void"
        lines.append(f"extern {'void' if ret == 'void' else c_type(ret, rext)} {d}({proto});")
    lines += ["", "/* Guest calls from native code. Each value crosses as the 32-bit",
              " * register the MIPS ABI carries, so caller and callee prototypes may",
              " * disagree exactly as they can on the PS1. An admitted callee is called",
              " * directly; any other runs on the interpreter (same CPU, same loop). */"]
    direct = thunk = 0
    ovl = {"lane_" + a["qname"]: a for a in admitted if a.get("member")}
    for sym, (callee, sig, argc, va_ret) in sorted(stubs.items()):
        addr = callee.split("func_")[-1]
        if sig is None:
            ret, rext = va_ret
            params = [("i32", (False, False))] * argc
        else:
            ret, rext, params = sig
        rtype = "void" if ret == "void" else c_type(ret, rext)
        plist = ", ".join(f"{c_type(t, e)} a{i}" for i, (t, e) in enumerate(params)) or "void"
        regs = [to_guest(f"a{i}", t, e) for i, (t, e) in enumerate(params)]
        lines.append(f"{rtype} {sym}({plist}) {{")
        target = "lane_" + callee
        guard = script_var_guard(target, regs[0] if regs else "0u")
        if target in ovl:
            # Another function of the same member: native only while its own
            # bytes are resident too (checked per call), else the interpreter.
            o = ovl[target]
            guard = (f"musashi_native_lane_overlay_resident(0x{o['vram']:08x}u, {o['size']}u, "
                     f"0x{o['digest']:016x}ull)")
        if target in defs and guard:
            lines.append(f"    if (!{guard}) {{")
            if params:
                lines.append(f"        const uint32_t args[{len(params)}] = {{{', '.join(regs)}}};")
                tcall = f"musashi_native_guest_call(0x{addr}u, {len(params)}u, args, NULL)"
            else:
                tcall = f"musashi_native_guest_call(0x{addr}u, 0u, NULL, NULL)"
            lines.append(f"        {'(void)' + tcall if ret == 'void' else 'return ' + from_guest(tcall, ret, rext)};")
            if ret == "void":
                lines.append("        return;")
            lines.append("    }")
        if target in defs:
            direct += 1
            cret, crext, cparams = defs[target]
            args = ", ".join(from_guest(regs[i] if i < len(regs) else "0u", t, e)
                             for i, (t, e) in enumerate(cparams))
            call = f"{target}({args})"
            for i in range(len(cparams), len(params)):
                lines.append(f"    (void)a{i}; /* the callee takes fewer arguments */")
            if ret == "void":
                lines.append(f"    {call};")
            elif cret == "void":
                lines.append(f"    {call};")
                lines.append(f"    return {from_guest('0u', ret, rext)};")
            else:
                lines.append(f"    return {from_guest(to_guest(call, cret, crext), ret, rext)};")
        else:
            thunk += 1
            if params:
                lines.append(f"    const uint32_t args[{len(params)}] = {{{', '.join(regs)}}};")
                call = f"musashi_native_guest_call(0x{addr}u, {len(params)}u, args, NULL)"
            else:
                call = f"musashi_native_guest_call(0x{addr}u, 0u, NULL, NULL)"
            if ret == "void":
                lines.append(f"    (void){call};")
            else:
                lines.append(f"    return {from_guest(call, ret, rext)};")
        lines.append("}")
    icalls = {}
    for a in admitted:
        icalls.update(a.get("icalls", {}))
    if icalls:
        lines += ["", "/* Calls through guest function pointers: the pointer holds the guest",
                  " * address, and the interpreter dispatches it exactly as jalr does. */"]
    for sym, (ret, rext, params) in sorted(icalls.items()):
        rtype = "void" if ret == "void" else c_type(ret, rext)
        plist = ", ".join(["uint32_t target"] + [f"{c_type(t, e)} a{i}" for i, (t, e) in enumerate(params)])
        regs = [to_guest(f"a{i}", t, e) for i, (t, e) in enumerate(params)]
        lines.append(f"{rtype} {sym}({plist}) {{")
        if params:
            lines.append(f"    const uint32_t args[{len(params)}] = {{{', '.join(regs)}}};")
            call = f"musashi_native_guest_call(target, {len(params)}u, args, NULL)"
        else:
            call = "musashi_native_guest_call(target, 0u, NULL, NULL)"
        if ret == "void":
            lines.append(f"    (void){call};")
        else:
            lines.append(f"    return {from_guest(call, ret, rext)};")
        lines.append("}")
    lines.append("")
    entries, ovl_entries = [], []
    for a in sorted(admitted, key=lambda a: (a["vram"], a.get("member") or "")):
        d = "lane_" + a["qname"]
        wrap = f"wrap_{a['member']}_{a['vram']:08x}" if a.get("member") else f"wrap_{a['vram']:08x}"
        ret, rext, params = a["defs"][d]
        if a.get("step_handler"):
            entries.append(f'    {{0x{a["vram"]:08x}u, 0u, {a["step_handler"]}, "{a["qname"]}", '
                           f'{len(params)}u, 1u}},')
            continue
        args = ", ".join(from_guest(f"musashi_native_lane_arg(call, {i}u)", t, e)
                         for i, (t, e) in enumerate(params))
        lines.append(f"static int {wrap}(MusashiNativeLaneCall *call) {{")
        guard = script_var_guard(d, "musashi_native_lane_arg(call, 0u)")
        if guard:
            lines.append(f"    if (!{guard}) return 0;")
        if not params and ret == "void":
            lines.append("    (void)call;")
        if ret == "void":
            lines.append(f"    {d}({args});")
        else:
            lines.append(f"    call->r[2] = {to_guest(f'{d}({args})', ret, rext)};")
        lines.append("    return 1;")
        lines.append("}")
        cost = 2 * (a["size"] // 4)
        row = (f"{{0x{a['vram']:08x}u, {cost}u, {wrap}, \"{a['qname']}\", "
               f"{len(params)}u, {0 if ret == 'void' else 1}u}}")
        if a.get("member"):
            ovl_entries.append(f"    {{{row}, {a['size']}u, 0x{a['digest']:016x}ull, \"{a['member']}\"}},")
        else:
            entries.append(f"    {row},")
    lines.append("")
    lines.append("const MusashiNativeLaneEntry musashi_native_lane_generated[] = {")
    lines.extend(entries or ["    {0u, 0u, NULL, NULL, 0u, 0u},"])
    lines.append("};")
    lines.append(f"const size_t musashi_native_lane_generated_count = {len(entries)}u;")
    lines.append("")
    lines.append("/* Overlay members: run only while guest RAM over the extent hashes to the")
    lines.append(" * member's digest (FNV-1a 64 of the user's own extracted member file). */")
    lines.append("const MusashiNativeLaneOverlayEntry musashi_native_lane_overlay[] = {")
    lines.extend(ovl_entries or ["    {{0u, 0u, NULL, NULL, 0u, 0u}, 0u, 0u, NULL},"])
    lines.append("};")
    lines.append(f"const size_t musashi_native_lane_overlay_count = {len(ovl_entries)}u;")
    (out / "native_lane_table.c").write_text("\n".join(lines) + "\n")
    return direct, thunk


# --own-code-only: a registry entry is third-party-derived when its origin or
# its source's header names the upstream decomp (Druthulu/BFM-decomp, whose
# src/ carries no license) or another project's PsyQ C. Such functions stay
# interpreted from the player's disc, so the binary compiles none of them.
THIRD_PARTY_MARK = re.compile(r"vendor|Druthulu|BFM-decomp|psyq|Sony|parasite|xenogears", re.I)


def third_party_derived(m):
    if THIRD_PARTY_MARK.search(m.get("origin") or ""):
        return True
    src = ROOT / m["source"]
    return src.exists() and bool(THIRD_PARTY_MARK.search(src.read_text(errors="replace")[:2000]))


def candidates(only=None, excluded=None, registry=None):
    """Game-code candidates. Sony PsyQ library ranges (config/psyq_ranges.txt)
    never enter the lane: their entry points belong to PsyCross/HLE."""
    sys.path.insert(0, str(ROOT / "tools"))
    import psyq_manifest
    psyq = psyq_manifest.ranges()
    if registry is None:
        registry = load_matches()
    picked = []
    for m in registry:
        if m["region"] != "main" or m.get("recovery") != "c":
            continue
        # Historical source attribution for these resident routines is unresolved.
        if m["vram"] in (0x8001282C, 0x800128EC):
            continue
        if psyq_manifest.psyq_segment(m["vram"], psyq):
            if excluded is not None:
                excluded.append(m["vram"])
            continue
        src = ROOT / m["source"]
        if not src.exists():
            continue
        text = src.read_text(errors="replace")
        other = set(re.findall(r"MUSASHI_NATIVE_[A-Z0-9_]+", text))
        if other and not (m["vram"] in INTRINSIC_PCS and
                          all(o.endswith("_BINDING") for o in other)):
            continue  # bespoke binding already owns this file
        if only and m["vram"] not in only:
            continue
        picked.append(m)
    return picked


def overlay_candidates(members, only=None, registry=None):
    if registry is None:
        registry = load_matches()
    return [m for m in registry if m["region"] in members and m.get("recovery") == "c"
            and not sdk_overlay(m["region"], m["vram"], m.get("size", 1))
            and (ROOT / m["source"]).exists() and (not only or m["vram"] in only)]


def main():
    global OBJECT_TARGET
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("--clang", default=shutil.which("clang") or "clang")
    ap.add_argument("--ar", default=shutil.which("ar") or "ar")
    ap.add_argument("--host-target", default=OBJECT_TARGET,
                    help="clang target triple of the lane objects (e.g. x86_64-windows-gnu); "
                         "the checks always run on the x86-64 SysV IR")
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--only", help="file of guest PCs (hex, one per line) to consider, e.g. a trace")
    ap.add_argument("--exclude", help="file of guest PCs (hex) never to admit")
    ap.add_argument("--own-code-only", action="store_true",
                    help="compile only owner-authored matched C (see third_party_derived)")
    ap.add_argument("--overlay-members", default="",
                    help="comma-separated overlay members to compile (e.g. main_0003); each needs "
                         "config/overlay_<m>.yaml and its extracted member file for the digests")
    ap.add_argument("--registry", type=Path, help="matching registry to use for candidate and hook extents")
    args = ap.parse_args()
    OBJECT_TARGET = args.host_target
    registry = load_matches(args.registry)
    out = args.out
    (out / "ir").mkdir(parents=True, exist_ok=True)
    def pcs(path):
        return {int(t.split()[0], 16) for t in Path(path).read_text().splitlines()
                if t.strip() and not t.startswith("#")}
    only = pcs(args.only) if args.only else None
    exclude = pcs(args.exclude) if args.exclude else set()
    symbols = load_symbol_addrs()
    hooks = hooked_pcs()
    psyq_excluded = []
    todo = [m for m in candidates(only, psyq_excluded, registry) if m["vram"] not in exclude]
    if args.own_code_only:
        kept = [m for m in todo if not third_party_derived(m)]
        print(f"native_lane_gen: own-code-only: {len(todo) - len(kept)} third-party-derived "
              f"candidates left to the interpreter")
        todo = kept
    infos, missing = {}, []
    for mem in [m for m in args.overlay_members.split(",") if m]:
        info = member_info(mem)
        if info is None:
            missing.append(mem)  # no config or no local member file: stays interpreted
        else:
            infos[mem] = info
    ovl_todo = [m for m in overlay_candidates(set(infos), only, registry) if m["vram"] not in exclude]
    if args.own_code_only:
        ovl_kept = [m for m in ovl_todo if not third_party_derived(m)]
        print(f"native_lane_gen: own-code-only: {len(ovl_todo) - len(ovl_kept)} third-party-derived "
              f"overlay candidates left to the interpreter")
        ovl_todo = ovl_kept
    msyms = {mem: load_symbol_addrs(mem) for mem in infos}
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        results = list(pool.map(lambda m: build_one(m, out, symbols, hooks, args.clang), todo))
        results += list(pool.map(lambda m: build_one(m, out, msyms[m["region"]], hooks, args.clang,
                                                     infos[m["region"]]), ovl_todo))
    admitted = [r for r in results if r["admitted"]]
    # A direct native call cannot reproduce a MIPS caller that passes fewer
    # arguments than the callee reads: the callee sees whatever the caller
    # left in a1-a3. Refuse such callers (to a fixpoint, since a refused
    # caller becomes an interpreted callee for others).
    while True:
        arity = {"lane_" + a["qname"]: len(a["defs"]["lane_" + a["qname"]][2]) for a in admitted}
        drop = set()
        for a in admitted:
            for sym, (callee, sig, argc, _) in a["stubs"].items():
                if "lane_" + callee in arity and argc < arity["lane_" + callee]:
                    drop.add(a["qname"])
                    a.update(admitted=False, rule="abi",
                             detail=f"passes {argc} of {arity['lane_' + callee]} arguments to {callee}")
        if not drop:
            break
        admitted = [a for a in admitted if a["qname"] not in drop]
    # A native caller of an admitted callee links to it directly; otherwise
    # the thunk runs the guest words. Duplicate definitions cannot occur:
    # each TU defines only its own registry function under lane_func_*.
    direct, thunk = emit_table(admitted, out)
    blocked = psyq_hooked(hooks, registry)
    with open(out / "native_lane_table.c", "a") as table:
        table.write("\n/* PsyQ compat entries whose body holds a port-sequenced PC. */\n")
        table.write("const uint32_t musashi_native_lane_hooked_psyq[] = {\n")
        table.write("".join(f"    0x{pc:08x}u,\n" for pc in blocked) or "    0u,\n")
        table.write("};\n")
        table.write(f"const size_t musashi_native_lane_hooked_psyq_count = {len(blocked)}u;\n")
    lib = out / "libmusashi_native_lane.a"
    if lib.exists():
        lib.unlink()
    objs = [r["obj"] for r in admitted]
    if objs:
        r = run([args.ar, "rcs", str(lib), *objs])
        if r.returncode:
            print(r.stderr, file=sys.stderr)
            return 1
    else:
        stub = out / "ir" / "empty.c"
        stub.write_text("typedef int musashi_native_lane_empty;\n")
        run([args.clang, f"--target={OBJECT_TARGET}", "-c", str(stub), "-o", str(out / "ir" / "empty.o")])
        run([args.ar, "rcs", str(lib), str(out / "ir" / "empty.o")])
    rules = {}
    for r in results:
        if not r["admitted"]:
            rules[r["rule"]] = rules.get(r["rule"], 0) + 1
    report = dict(candidates=len(results), admitted=len(admitted), rejected_by_rule=rules,
                  psyq_excluded=len(psyq_excluded),
                  psyq_hooked=[f"{pc:08x}" for pc in blocked],
                  call_stubs=dict(direct=direct, interpreter_thunk=thunk),
                  overlay=dict(members=sorted(infos), missing=missing,
                               candidates=len(ovl_todo),
                               admitted=sum(1 for a in admitted if a.get("member"))),
                  functions=[{k: (f"{v:016x}" if k == "digest" and v is not None else v)
                              for k, v in r.items() if k not in ("obj", "defs", "stubs")}
                             for r in sorted(results, key=lambda r: (r["vram"], r.get("member") or ""))])
    (out / "report.json").write_text(json.dumps(report, indent=1) + "\n")
    print(f"native lane: {len(admitted)}/{len(results)} game functions admitted "
          f"({len(psyq_excluded)} PsyQ-range candidates excluded); rejected {rules}; "
          f"call stubs direct={direct} thunk={thunk}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
