#!/bin/sh
# Package musashi_native_boot for the owner's launcher (Linux x86_64, Legion Go).
#
#   tools/package_legion.sh --binary BUILD/musashi_native_boot --run N \
#       --out DIR --status TEXT [--known-issue TEXT ...] \
#       --exe YOUR/SLUS_007.26 [--extracted YOUR/extracted/disc] \
#       [--verified-path TEXT ...] [--candidate-receipt FILE]
#
# An optional JSON playtest receipt binds source_commit, source_branch,
# build_type, player_sha256 and helper_sha256 to the tested pair. The default
# receipt is FILE.receipt.json beside --binary FILE. Record psycross_revision
# and psycross_patch_sha256 for verified component attribution. Without a
# receipt the archive keeps its development label and records unknown source.
# Distribute the matching OpenAL source and checksums with public downloads.
#
# Stages bfm-r<N>-<commit>/ (binary, runtime libraries, launch.sh, README,
# licenses, manifest.json with the size and SHA-256 of every file), runs
# tools/retail_guard.py over it and writes bfm-r<N>-<commit>.tar.gz beside it.
# No disc image, BIOS or game data is copied; --exe/--extracted feed the
# guard only. The game is read at run time from BFM_DISC_DIR (default
# ~/Games/brave-fencer-musashi/disc).

set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
binary=""
run=""
out=""
status=""
issues=""
receipt=""
verified=""
nl='
'

# Keep the original guard flags and their paths as individual arguments.
remaining=$#
while [ "$remaining" -gt 0 ]; do
    [ "$remaining" -ge 2 ] || { echo "package_legion: missing option value" >&2; exit 2; }
    remaining=$((remaining - 2))
    case "$1" in
        --binary) binary=$2; shift 2 ;;
        --run) run=$2; shift 2 ;;
        --out) out=$2; shift 2 ;;
        --status) status=$2; shift 2 ;;
        --known-issue) issues="$issues$2$nl"; shift 2 ;;
        --verified-path) verified="$verified$2$nl"; shift 2 ;;
        --candidate-receipt) receipt=$2; shift 2 ;;
        --exe|--extracted)
            if [ "$1" = --exe ]; then
                [ -f "$2" ] && [ -s "$2" ] || { echo "package_legion: missing or empty executable reference" >&2; exit 2; }
            else
                [ -d "$2" ] && [ -n "$(find "$2" -type f -size +0c -print -quit)" ] || {
                    echo "package_legion: missing or empty extracted reference" >&2; exit 2; }
            fi
            set -- "$@" "$1" "$2"; shift 2 ;;
        *) echo "package_legion: unknown option $1" >&2; exit 2 ;;
    esac
done
[ -f "$binary" ] && [ -n "$run" ] && [ -n "$out" ] && [ -n "$status" ] || {
    echo "usage: $0 --binary FILE --run N --out DIR --status TEXT [--known-issue TEXT ...]" >&2; exit 2; }

case "$run" in .*|-*|*[!a-zA-Z0-9._-]*) echo "package_legion: invalid run identifier" >&2; exit 2 ;; esac
mkdir -p "$out"
work=$(mktemp -d "$out/.bfm-stage.XXXXXXXX")
trap 'rm -rf -- "$work"' EXIT
stage="$work/stage"
mkdir -p "$stage/lib" "$stage/licenses"
metadata=$(python3 "$root/tools/legion_metadata.py" "$root" "$binary" "$stage" "$receipt")
commit=$(printf '%s\n' "$metadata" | python3 -c 'import json, sys; m=json.load(sys.stdin); print((m["source_commit"] or m["packaging_checkout_commit"])[:9])')
name="bfm-r$run-$commit"
if [ -e "$out/$name" ] || [ -e "$out/$name.tar.gz" ]; then
    echo "package_legion: $name already exists" >&2
    exit 2
fi
mv "$stage" "$work/$name"
stage="$work/$name"

cp "$root/tools/launch_legion.sh" "$stage/launch.sh"
chmod 755 "$stage/launch.sh"

{
    echo "Brave Fencer Musashi native port $name (Linux x86_64, Legion Go)"
    echo
    echo "Runtime: hybrid, with a MIPS CPU interpreter."
    echo "Status: $status"
    [ -n "$issues" ] && { echo; echo "Known issues:"; printf '%s' "$issues" | sed 's/^/  - /'; }
    cat <<'TXT'

Your disc (none of the game is included). Put the .cue and .bin dump of your
Brave Fencer Musashi (USA, SLUS-00726) disc in ~/Games/brave-fencer-musashi/disc/
or in disc/ beside launch.sh. A game library under ~/Games is also searched
if it contains games/brave-fencer-musashi/disc/. To use a different folder,
set BFM_DISC_DIR. A file picked on first start is remembered in
~/.config/bfm-port/disc. One-bin and per-track dumps
both work. A 2048-byte .iso does not (it lacks the CD-XA data).
No BIOS is needed: the port provides its own kernel services.

Controls: an SDL game controller or keyboard. The last controller used stays active.
  Controller: A Cross, B Circle, X Square, Y Triangle, Start, Back Select,
  LB/RB L1/R1, d-pad or left stick.
  Keyboard: arrows or WASD d-pad; C Cross, X Square, V Circle, Z Triangle
  (or K/J/I/U); Return Start, Space Select; left/right Shift L1/R1,
  left/right Ctrl L2/R2.

The host_requirements section of manifest.json lists the system libraries and
minimum imported glibc, libstdc++ and libgcc symbol versions for this build.
Use the display and audio client libraries listed for the bundled SDL build
and working system OpenGL and audio drivers. The launcher passes extra
arguments to the port.
For borderless desktop fullscreen, run ./launch.sh --fullscreen. The CPU
renderer fits the 4:3 picture inside the screen. Use --windowed for a
1280x960 window. Without either flag the player reads BFM_FULLSCREEN, then
[video] fullscreen in the port config.ini. Windowed is the default.
Log of the last run: ~/.local/state/bfm-port/last-run.log.
Dev menu (off by default): BFM_DEV_MENU=1 ./launch.sh, then F1 or L3+R3.
TXT
} > "$stage/README.txt"

python3 - "$stage" "$name" "$metadata" "$status" "$issues" "$verified" <<'PY'
import hashlib, json, os, sys
stage, name, metadata, status, issues, verified = sys.argv[1:]
files = []
for dirpath, _, names in os.walk(stage):
    for n in names:
        p = os.path.join(dirpath, n)
        rel = os.path.relpath(p, stage)
        if rel == "manifest.json":
            continue
        files.append({"path": rel, "size_bytes": os.path.getsize(p),
                      "sha256": hashlib.sha256(open(p, "rb").read()).hexdigest()})
files.sort(key=lambda f: f["path"])
disc = "~/Games/brave-fencer-musashi/disc"
manifest = {
    **json.loads(metadata),
    "package": name, "build_id": name, "target": "Linux x86_64 / Legion Go",
    "verified_paths": [v for v in verified.splitlines() if v],
    "device_checks": {check: "NOT RUN" for check in
                      ("controller", "borderless_fullscreen", "text_field_keyboard")},
    "playback": status,
    "known_issues": [i for i in issues.splitlines() if i],
    "disc_dir_default": disc, "disc_dir_env": "BFM_DISC_DIR", "disc_env": "BFM_DISC",
    "disc_search_order": ["BFM_DISC", "remembered selection", "BFM_DISC_DIR",
                          "./disc", disc, "~/Games/*/games/brave-fencer-musashi/disc",
                          "desktop file picker"],
    "disc_inputs": ["your own .cue/.bin of Brave Fencer Musashi (USA, SLUS-00726)"],
    "excluded": ["disc image and all other game data", "BIOS"],
    "launch": "./launch.sh [native port options]",
    "files": files,
}
json.dump(manifest, open(os.path.join(stage, "manifest.json"), "w"), indent=2)
open(os.path.join(stage, "manifest.json"), "a").write("\n")
PY

python3 "$root/tools/retail_guard.py" --stage "$stage" "$@" || {
    echo "package_legion: retail guard failed; no tarball written" >&2; exit 1; }
tar --owner=0 --group=0 --numeric-owner --mode=u+rwX,go+rX,go-w -C "$work" -czf "$work/$name.tar.gz" "$name"
archive="$out/$name.tar.gz"
archive_id=$(stat -c '%d:%i' "$work/$name.tar.gz")
mv -nT "$work/$name.tar.gz" "$archive"
[ ! -e "$work/$name.tar.gz" ] || { echo "package_legion: archive already exists" >&2; exit 2; }
if ! mv -nT "$stage" "$out/$name" || [ -e "$stage" ]; then
    [ "$(stat -c '%d:%i' "$archive")" = "$archive_id" ] && rm -f -- "$archive"
    exit 1
fi
(cd "$out" && sha256sum "$name.tar.gz" && stat -c '%s bytes' "$name.tar.gz")
