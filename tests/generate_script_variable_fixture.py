"""Compile six public script-variable accessors and their synthetic native callers."""
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
# (callee PC, return type, second argument type for stores)
ACCESSORS = (
    (0x800291A0, "void", "int"), (0x800291B4, "unsigned char", None),
    (0x800291C8, "void", "short"), (0x800291DC, "short", None),
    (0x800291F0, "void", "int"), (0x80029204, "int", None),
)


def main(out, clang, ar, host_target=None):
    out.mkdir(parents=True, exist_ok=True)
    registry = json.loads((ROOT / "provenance/native-main.json").read_text())
    matched = {row["vram"]: row for row in registry["matches"] if row["region"] == "main"}
    rows = []
    for i, (pc, result, value_type) in enumerate(ACCESSORS):
        rows.append(matched[pc])
        name = f"func_{0x801FE400 + 0x20 * i:08X}"
        callee = matched[pc]["name"]
        source = out / f"{name}.c"
        params = "int index" + (f", {value_type} value" if value_type else "")
        call = f"{callee}(index, value);" if value_type else f"int loaded = {callee}(index);"
        ret = "" if value_type else "return loaded;"
        source.write_text(f"""extern unsigned char D_800B9000[];
extern {result} {callee}({params});
{result} {name}({params}) {{
    D_800B9000[0] = 1;
    {call}
    D_800B9000[1] = 1;
    {ret}
}}
""")
        rows.append(dict(name=name, region="main", vram=0x801FE400 + 0x20 * i,
                         size=4, source=str(source), recovery="c"))
    manifest = out / "fixture.json"
    manifest.write_text(json.dumps({"matches": rows}))
    subprocess.run([sys.executable, ROOT / "tools/native_lane_gen.py", "--registry", manifest,
                    "--own-code-only", "--out", out, "--clang", clang, "--ar", ar, "--jobs", "1",
                    *(["--host-target", host_target] if host_target else [])],
                   check=True)
    report = json.loads((out / "report.json").read_text())
    if report["admitted"] != len(rows) or report["call_stubs"]["direct"] != len(ACCESSORS):
        raise SystemExit("Expected all six accessors and six generated direct native callers.")


if __name__ == "__main__":
    main(Path(sys.argv[1]).resolve(), sys.argv[2], sys.argv[3],
         sys.argv[4] if len(sys.argv) > 4 else None)
