# How startup loads the MAIN overlays

The September 4, 2026 investigation traced the USA disc's startup loader through the executable and the extracted `MAIN.CD` members. Its evidence consists of file layout, instruction analysis and compiler comparisons. The order in which a running game visits the conditional callbacks was still open at that point.

## From archive member to guest address

The executable reads `LIST.CD` and constructs a table of 447 file descriptors across eight archives. The first 49 entries describe `MAIN.CD`. All 49 sector offsets and byte lengths agree with the archive's own header. The loader adds each nonzero relative sector offset to the archive's disc location and retains the individual file lengths.

The type-1 PAC transfer skips its first 2,048-byte sector. It rounds the remaining body length up to a whole word and transfers at most 512 words per call. Treating the generic 16-byte PAC header as the entire prefix gives the wrong guest addresses.

| MAIN member | Offset where the transferred body starts | Guest destination |
| --- | --- | --- |
| 0010 | `0x20000` | `0x800CEDF8` |
| 0012 | `0x28000` | `0x80128158` |

These mappings follow the destination arguments and file descriptors that the corresponding callers pass to the streaming routine. A successful member-0012 load advances a selector to its subsequent entry. Code from different members can occupy the same guest address, so identifying a function at runtime also requires identifying the resident member.

## Callback order

When the startup guard is nonzero, startup invokes a wrapper in member 0010 at `0x800D25FC`. The wrapper reads its callback from shared guest memory. Four writers were identified in member 0012. One conditional path installs a callback before startup reads the guard in that same iteration. Another installs a callback during dispatch after the guard read, allowing the new value to affect a later iteration.

One installed callback reads an unsigned halfword index, calls through an indexed table and increments a separate halfword counter. Its first target advances the guard from five to six while retaining the installed callback. A host implementation needs distinct storage for the guard, callback, index and counter.

## Check the file layout

From the repository root, run the archive extractor on your USA disc files. The output directory must be new. The manifest records the offset, length and digest of each extracted member.

```sh
python3 tools/extract_cd.py extracted/disc/files/MAIN.CD \
  --output extracted/overlays/main --manifest extracted/main-members.json
```

The archive layout and extraction procedure are described in the [asset documentation](ASSETS.md).
