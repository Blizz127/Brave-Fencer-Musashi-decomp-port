#!/bin/sh
# Build the no-disc probes with a modern ELF C compiler on a little-endian host.
set -eu
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
fixture=$(mktemp -d)
trap 'rm -rf "$fixture"' EXIT
scene="$root/src/overlays/sc01_0077"
includes=${BFM_INCLUDE_DIR:-"$root/include"}
cc=${CC:-cc}
# Synthetic model references occupy actual low guest-width addresses. Typed
# scene fields can overlap. These flags apply only to these test processes.
set -- -std=c11 -O2 -fno-strict-aliasing -fno-pie -no-pie -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -I"$includes"
if "$cc" -dM -E - </dev/null | awk '/__clang__/ {c=1} END {exit !c}'; then
    set -- "$@" -flto
else
    set -- "$@" -flto -Werror=lto-type-mismatch
fi
"$cc" "$@" "$root/tests/forest_log_standing_probe.c" "$scene/8017f460.c" -o "$fixture/standing"
"$fixture/standing"
"$cc" "$@" "$root/tests/forest_log_init_probe.c" "$scene/80180590.c" -o "$fixture/init"
"$fixture/init"
"$cc" "$@" "$root/tests/forest_log_spawn_probe.c" -o "$fixture/spawn"
"$fixture/spawn"
"$cc" "$@" "$root/tests/forest_log_fall_probe.c" \
    "$scene/8012bee8.c" "$scene/8017f290.c" "$scene/8017f460.c" "$scene/8017f5bc.c" \
    "$scene/8017f6d8.c" "$scene/8018047c.c" "$scene/80180588.c" "$scene/80180590.c" -o "$fixture/fall"
"$fixture/fall"
echo "SC01/0077 controller, initialization, standing, fall, and settle probes passed."
