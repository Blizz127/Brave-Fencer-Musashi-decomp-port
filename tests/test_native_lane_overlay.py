"""Overlay entries in the native lane: differential plus the residency
guard's negative test (tests/native_lane_overlay_probe.c).

Needs a lane build with overlay members (MUSASHI_LANE_BUILD = its build
directory, holding musashi_native_lane_overlay_probe) and the user's own
extracted disc (the EXE and the member file); skipped otherwise. The member
payload is loaded as the game's loader leaves it: from the member's header
subsegment (or first code subsegment) to its end marker, at the yaml's
vram mapping.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
MEMBERS = [m for m in os.environ.get("MUSASHI_LANE_OVERLAY_MEMBERS", "main_0003").split(",") if m]


def load_plan(member, root=ROOT):
    text = (root / "config" / f"overlay_{member}.yaml").read_text(errors="replace")
    seg = re.search(r"-\s*name:\s*(\w+)\s*\n\s*type:\s*code\s*\n\s*start:\s*(0x[0-9A-Fa-f]+)"
                    r"\s*\n\s*vram:\s*(0x[0-9A-Fa-f]+)", text)
    target = re.search(r"target_path:\s*(\S+)", text).group(1)
    subs = [(int(m.group(1), 16), m.group(2), m.group(3)) for m in
            re.finditer(r"^\s*-\s*\[\s*(0x[0-9A-Fa-f]+)\s*(?:,\s*(\w+)\s*(?:,\s*([\w.]+))?)?\s*\]",
                        text, flags=re.M)]
    start, vram = int(seg.group(2), 16), int(seg.group(3), 16)
    end = subs[-1][0]
    hdr = [o for o, k, n in subs if n and n.endswith("_hdr")]
    code = [o for o, k, n in subs if k in ("asm", "c", "hasm")]
    return dict(file=root / target, vram=vram, start=start,
                load_off=hdr[0] if hdr else code[0], load_end=end)


class OverlayLane(unittest.TestCase):
    def test_members(self):
        build = os.environ.get("MUSASHI_LANE_BUILD")
        probe = Path(build) / "musashi_native_lane_overlay_probe" if build else None
        exe = ROOT / "extracted/disc/files/SLUS_007.26"
        if not probe or not probe.exists() or not exe.exists():
            raise unittest.SkipTest("needs MUSASHI_LANE_BUILD with the overlay probe and a local extract")
        for member in MEMBERS:
            plan = load_plan(member)
            if not plan["file"].exists():
                raise unittest.SkipTest(f"{member}: member file not extracted")
            r = subprocess.run([str(probe), str(exe), member, str(plan["file"]), f"{plan['vram']:x}",
                                f"{plan['start']:x}", f"{plan['load_off']:x}", f"{plan['load_end']:x}"],
                               capture_output=True, text=True, timeout=3600)
            summary = [l for l in r.stdout.splitlines() if "member=" in l]
            self.assertEqual(r.returncode, 0, (summary, r.stderr[-2000:]))
            self.assertIn("different=0", summary[0])
            self.assertIn("negative_ran_native=0", summary[0])


class OverlayAdmission(unittest.TestCase):
    @unittest.skipUnless(shutil.which("clang"), "requires clang")
    def test_shop_messages_require_a_bus_aware_lookup(self):
        shop = "sc03_0001"
        other = "sc99_9999"
        lookup, latch, caller = 0x8018C89C, 0x8018C8D4, 0x8018C918
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "tools").mkdir()
            (root / "config").mkdir()
            for tool in ("native_lane_gen.py", "psyq_manifest.py"):
                shutil.copy(ROOT / "tools" / tool, root / "tools" / tool)
            shutil.copy(ROOT / "config/psyq_ranges.txt", root / "config")

            # The leaves are safe arithmetic in both members. Only the admission
            # decision for these two identities should send the caller to guest code.
            for pc in (lookup, latch):
                (root / f"{pc:x}.c").write_text(
                    f"unsigned func_{pc:08X}(unsigned x) {{ return x + 1u; }}\n")
            (root / f"{caller:x}.c").write_text(
                "extern unsigned func_8018C89C(unsigned), func_8018C8D4(unsigned);\n"
                "unsigned func_8018C918(unsigned x) { return func_8018C8D4(func_8018C89C(x)); }\n")

            rows = []
            for member, pcs in ((shop, (lookup, latch, caller)), (other, (lookup, latch))):
                data = member.encode().ljust(256, b".")
                (root / member).write_bytes(data)
                (root / "config" / f"overlay_{member}.yaml").write_text(
                    f"sha1: {hashlib.sha1(data).hexdigest()}\ntarget_path: {member}\n"
                    f"- name: {member}\n  type: code\n  start: 0x0\n  vram: {lookup:#x}\n"
                    "  subsegments:\n    - [0x0, c]\n- [0x100]\n")
                rows.extend(dict(region=member, name=f"func_{pc:08X}", vram=pc, size=4,
                                 recovery="c", source=f"{pc:x}.c") for pc in pcs)
            registry = root / "rows.json"
            registry.write_text(json.dumps({"matches": rows}))
            only = root / "only.txt"
            only.write_text(f"{lookup:x}\n{latch:x}\n{caller:x}\n")
            out = root / "generated"
            subprocess.run([sys.executable, root / "tools/native_lane_gen.py", "--out", out,
                            "--registry", registry, "--only", only, "--own-code-only", "--jobs", "1",
                            "--overlay-members", f"{shop},{other}"], check=True, capture_output=True)
            report = json.loads((out / "report.json").read_text())
            held = {(r["member"], r["vram"], r["rule"]) for r in report["functions"]
                    if not r["admitted"]}
            self.assertEqual(held, {(shop, lookup, "memory"), (shop, latch, "memory")})
            self.assertEqual(report["admitted"], 3)
            self.assertEqual(report["call_stubs"]["interpreter_thunk"], 2)


class LoadPlan(unittest.TestCase):
    def test_header_to_end(self):
        with tempfile.TemporaryDirectory() as d:
            (Path(d) / "config").mkdir()
            (Path(d) / "config" / "overlay_main_0003.yaml").write_text(
                "  target_path: extracted/overlays/main/0003.bin\nsegments:\n  - name: main_0003\n"
                "    type: code\n    start: 0x0\n    vram: 0x8007CDF8\n    subsegments:\n"
                "      - [0x0, data, main_0003_container]\n      - [0x52000, data, main_0003_hdr]\n"
                "      - [0x520D8, asm, main_0003]\n      - [0x5677C, data, main_0003_tail]\n"
                "  - [0x7D000]\n")
            plan = load_plan("main_0003", Path(d))
        self.assertEqual((plan["vram"], plan["load_off"], plan["load_end"]), (0x8007CDF8, 0x52000, 0x7D000))
        self.assertEqual(plan["vram"] + plan["load_off"] - plan["start"], 0x800CEDF8)


if __name__ == "__main__":
    unittest.main()
