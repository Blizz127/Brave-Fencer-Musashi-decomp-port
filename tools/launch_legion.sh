#!/bin/sh
# Brave Fencer Musashi native port. Plays from your own disc image (.cue/.bin
# of the USA disc, SLUS-00726); nothing from the game is in this package.
# The disc is found in this order: $BFM_DISC (a .cue or .bin), the one you
# picked last time, $BFM_DISC_DIR, disc/ next to the launcher, the game or
# library disc directory under ~/Games, then a desktop file picker.
set -u
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CONF=${XDG_CONFIG_HOME:-$HOME/.config}/bfm-port
STATE=${XDG_STATE_HOME:-$HOME/.local/state}/bfm-port
mkdir -p "$CONF" "$STATE"
LOG="$STATE/last-run.log"

say() {  # message box when a desktop tool exists, always stderr
    echo "bfm: $1" >&2
    if command -v zenity >/dev/null 2>&1; then zenity --error --title="Brave Fencer Musashi" --text="$1" 2>/dev/null
    elif command -v kdialog >/dev/null 2>&1; then kdialog --title "Brave Fencer Musashi" --error "$1" 2>/dev/null
    fi
}

find_in() {  # first .cue, else first .bin, in a folder
    [ -d "$1" ] || return 1
    for f in "$1"/*.cue "$1"/*.CUE "$1"/*.bin "$1"/*.BIN; do
        [ -f "$f" ] && { echo "$f"; return 0; }
    done
    return 1
}

DISC=${BFM_DISC:-}
[ -z "$DISC" ] && [ -f "$CONF/disc" ] && DISC=$(cat "$CONF/disc")
[ -n "$DISC" ] && [ ! -f "$DISC" ] && DISC=
if [ -z "$DISC" ]; then
    for d in "${BFM_DISC_DIR:-}" "$HERE/disc" "$HOME/Games/brave-fencer-musashi/disc" \
             "$HOME"/Games/*/games/brave-fencer-musashi/disc; do
        DISC=$(find_in "$d") && break
        DISC=
    done
fi
if [ -z "$DISC" ]; then
    if command -v zenity >/dev/null 2>&1; then
        DISC=$(zenity --file-selection --title="Choose your Brave Fencer Musashi (USA) .cue or .bin" \
               --file-filter="Disc image | *.cue *.CUE *.bin *.BIN" 2>/dev/null) || DISC=
    elif command -v kdialog >/dev/null 2>&1; then
        DISC=$(kdialog --title "Choose your Brave Fencer Musashi (USA) .cue or .bin" \
               --getopenfilename "$HOME" "*.cue *.CUE *.bin *.BIN" 2>/dev/null) || DISC=
    fi
fi
if [ -z "$DISC" ]; then
    say "No disc image found. Put your Brave Fencer Musashi (USA) .cue and .bin in ~/Games/brave-fencer-musashi/disc/ and start again."
    exit 66
fi
printf '%s\n' "$DISC" > "$CONF/disc"

export LD_LIBRARY_PATH="$HERE/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
# Playable window, real-time guest clock, errors shown on screen.
export MUSASHI_INTERACTIVE=${MUSASHI_INTERACTIVE:-1}
export MUSASHI_GUEST_CLOCK=${MUSASHI_GUEST_CLOCK:-1}
export MUSASHI_HOLD_WINDOW_MS=${MUSASHI_HOLD_WINDOW_MS:-3000}
"$HERE/musashi_native_boot" --disc "$DISC" "$@" > "$LOG" 2>&1
status=$?
# A refused image is not worth remembering.
[ "$status" = 2 ] && grep -q "could not start from your disc image" "$LOG" && rm -f "$CONF/disc"
exit "$status"
