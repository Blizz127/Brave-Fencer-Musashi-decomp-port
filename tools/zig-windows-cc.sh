#!/usr/bin/env bash
# CMake needs a separate executable name for each tool in the Zig cross toolchain.
set -eu
case "${0##*/}" in
    *-c++) compiler=c++ ;;
    *-cc) compiler=cc ;;
    *-ar) exec "${ZIG:-zig}" ar "$@" ;;
    *-ranlib) exec "${ZIG:-zig}" ranlib "$@" ;;
    *-rc) exec "${ZIG:-zig}" rc "$@" ;;
    *) echo "unrecognized Zig tool name: ${0##*/}" >&2; exit 2 ;;
esac

args=()
stdout_file=
while (( $# )); do
    case "$1" in
        -Wl,-v)
            # CMake's MinGW ABI detection asks GNU ld for its version; LLD's
            # COFF driver rejects this flag. The compiler also receives -v.
            shift ;;
        -o)
            if [[ "${2-}" == "-" ]]; then
                # Zig 0.13 treats this as a literal file when CMake probes the
                # C++ standard library. Give the preprocessor stdout instead.
                stdout_file=$(mktemp) || exit 1
                trap 'rm -f -- "$stdout_file"' EXIT
                args+=(-o "$stdout_file")
                shift 2
            else
                args+=("$1")
                shift
            fi ;;
        *)
            args+=("$1")
            shift ;;
    esac
done
if [[ -n "$stdout_file" ]]; then
    "${ZIG:-zig}" "$compiler" -target x86_64-windows-gnu "${args[@]}"
    cat "$stdout_file"
else
    exec "${ZIG:-zig}" "$compiler" -target x86_64-windows-gnu "${args[@]}"
fi
