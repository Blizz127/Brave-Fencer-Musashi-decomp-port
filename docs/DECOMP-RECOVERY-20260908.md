# C recovery review, 2026-09-08

The historical review inspected six complete PS1 implementations, rebuilt with GCC 2.7.2 and ASPSX 2.56 at `-O2 -G0`. Each matched its registered range. Five entries audited existing implementations. `8012C194` was recovered during the review. The public source selection is documented in [EXCLUDED.md](../EXCLUDED.md).

The review checked C bodies, entry/return boundaries and in-range non-call branch destinations against the disassembly. Interior labels are included through the common return and delay slot. Native instruction export branches are not the implementation being classified. Historical symbol discovery coalesces some neighboring functions, so its range sizes were not used to override these inspected boundaries.

| Function | Bytes | Review | Source SHA-256 |
| --- | ---: | --- | --- |
| func_80059234 | 388 | Mode dispatch, graph environment initialization and reset callback; common return at 800593A0. | a0461b24462fb6ae31e48e6a340b3d12e0d9c9014b57fcf26de2f071439ac6be |
| func_8005BD7C | 348 | Mode-dependent device reset and clearing; common restore at 8005BEC4. | 60a3c46d600e93a8381185ab559be999a93fa59b1d65711ec62d432d668a4fac |
| func_8005BED8 | 328 | Blocking and polling queue/device completion; shared return at 8005C00C. | 6f15026523feb53f1c8271fcabd7d8b55ceadfe2edb7d18aec31e727b6b58986 |
| func_8005C054 | 364 | Timeout test, diagnostic calls and queue/device reset; common return at 8005C1B0. | c07648d521d8fc83aefdad4a48bed351d8b69ec552adab346ab6092ee52bda6d |
| func_8012C194 | 36 | New C wrapper calling 8001D074 with 0x3E and 0x7D; return at 8012C1B0. | 9f6b755a67088651510b370e3978780bc57735a42ec52635b69a08535081ebb4 |
| func_8017C120 | 300 | Overlay state initialization and two bounded loops; return at 8017C244. | daf4e95441999683e79cc504ea666b0ba7270f412904251f10687dec3ec49f4c |

Coverage is the union of reviewed address ranges in each retail region. Overlapping entries count once. The results here cover only the listed functions.

Three additional controller record accessors were newly recovered into C: `80014C28` (44 bytes), `80014CF8` (56 bytes), and `80014D30` (56 bytes). The first reads record byte zero, the others read at byte offsets 0x32 and 0x42 with a masked selector; all use a 0x4c-byte record stride. Both selectors are masked to eight bits. The latter two use empty register constraints to retain retail address evaluation order, with no emitted assembly instructions. Each full body ends at its return plus delay slot. Installed-source builds matched all 39 retail words, and the existing native word exports were checked unchanged.

New controller C recovery also covers `80014B10` (236 bytes: digital state passthrough and ID-gated axis-to-direction conversion) and `80014CAC` (76 bytes: OR two controller states and test the supplied mask). Both include their complete return/delay slots and all non-call branch targets. `80014B10` keeps the unmasked port live across the ID call using an empty register constraint. Installed-source builds matched all 78 retail words; native word exports were checked unchanged.
