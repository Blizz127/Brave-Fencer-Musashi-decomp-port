# Source provenance and matching style

This public project includes independently recovered game routines and
original host implementations. The publication record identifies the source
categories that qualify and credits research by other projects.
[Excluded sources](../EXCLUDED.md) · [Credits](../CREDITS.md)

The September 2026 work included a separate upstream-derived intake. Its
function counts describe that historical tree. The public snapshot excludes
those sources and tracks independently recovered functions separately.

## Matching sources

Place an address-named matching file under the appropriate region. For
example, a resident main-executable routine belongs in `src/main/`. Use a
lowercase VRAM address for the filename and `psx_types.h` for fixed-width
guest types. Describe any known function boundaries, observed field widths
and behavior. Keep unidentified structure fields explicit until their layout
is verified.

Verify each claimed match against the contributor's own disc with the
matching toolchain. Record its region, address, extent, source and compiler
options in the corresponding registry. Host execution is tested separately
with the PC compiler and the native code path that selects the function.
