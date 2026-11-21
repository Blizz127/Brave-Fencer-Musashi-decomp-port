# Retail startup observation

## Bounded VBlank request observation, 2026-09-05

`tools/retail_vblank_request_observe.lua` observed a normal interpreter disc boot.
After qualifying the initialized callback table, it accepted slot-seven entry only from
`ra=80042D38`. The first accepted callback had request index `FFFFFFFF` and
tick `94`. Across 600 accepted callbacks it observed no worker request call.
Exit **2** is the explicit observation-limit result, not a request-chain pass.
No guest input, state injection, or MMIO writes were supplied by the observer.

The final framebuffer is 320 by 240, RGB555, entirely black (all decoded channel
extrema are zero). This is a bounded retail observation; it proves neither a
retail menu nor native startup. The absence of requests in this window does
not establish that the queue is unused later. No request target is inferred.

An earlier run reached the same frame but exited 3 while formatting the pixel
format enum. The run above used `tonumber` to format it and reached the
observer's intended limit.

Next native dependency: the recovered parent IRQ dispatcher `800427F4` and
its BIOS context-return boundary. Direct invocation of the VBlank child does
not deliver an interrupt. Nonempty request/completion targets remain unbound
until actual code and execution evidence support them.

## Qualified VBlank callback observation, 2026-09-05

`tools/retail_vblank_observe.lua` completed a normal interpreter disc boot
with the AppRun and BIOS identified below. It exited zero after recording
the first nonempty handler's final return-transfer instruction.

The observer qualifies initialization through the guard and installed table
targets before accepting handler entry with `ra=800428DC`. This matches the
parent dispatcher's indirect call at `800428D4`. It reads only cached RAM and
CPU registers, supplies no input, and changes no guest state or MMIO.

| Observed visits | Entry tick | Return tick | Enabled bits | Calls |
| --- | --- | --- | --- | --- |
| 1, 2, 3 | `0`, `1`, `2` | `1`, `2`, `3` | `0009` | zero; all slots empty |
| 148 | `00000093` | `00000094` | `000D` | slot 7 → `800616D0` |

Visit 148's other seven slots were zero. The indirect-call breakpoint saw both
`v0` and the live slot equal `800616D0`, with tick `94`; the handler subsequently
reached `80042D58` with `ra=800428DC`. This establishes a concrete retail
callback target and its return, not which internal branch or request callback
it took. A prior capture timed out after encountering breakpoint exceptions.

The qualified run ended at that return-transfer instruction. It does not prove
native IRQ scheduling, game callback execution, menu pixels, or menu input.

## Earlier startup observations

A local PCSX-Redux interpreter run booted the registered Musashi disc through
the BIOS and delivered an execution breakpoint at `800141F0`. The observer
reported `MUSASHI_RETAIL_STARTUP_OBSERVED pc=800141f0 cycles=456705331ULL`
and the process exited with code zero. It did not load the EXE directly, set
the PC, inject a return value, or change guest RAM.

The PCSX-Redux AppRun used for these observations has SHA-256
`0621042a2e2d2b4df0f3fd863a11d51be16957303c12f32b44fec2e11e61fa78`.
The inspected PCSX-Redux source was revision
`3e10093ad9cc2fb445668928b3d0f87abeccd29c`. The binary's source
revision was never independently established.

The test firmware has SHA-256
`11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef`.
The runtime identifies it as **SCPH-7003 (US)**.

The runtime recognized `extracted/disc/disc.cue` as `MUSASHI`, disc ID
`SLUS00726`. It loaded the executable at `80010000` with a text size of
`0x64800` bytes. The initial stack pointer was `0x801FFFF0`. Verify the
media identity with the extraction and provenance manifests.

To repeat the observation with your own BIOS and disc, set
`PCSX_REDUX_APPRUN`, `BIOS_DIR` and `REPO` and run the following command.
The fresh portable profile gives the test its own memory cards.

```sh
profile=$(mktemp -d)
"$PCSX_REDUX_APPRUN" \
  --portable "$profile" \
  --cli --testmode --interpreter --debugger \
  --bios "$BIOS_DIR/scph5500.bin" \
  --iso "$REPO/extracted/disc/disc.cue" \
  --dofile "$REPO/tools/retail_startup_observe.lua" \
  --run
```

An earlier dynarec run booted the disc but did not deliver the breakpoint.
Use the interpreter for this observer. GPU unknown-command messages also
occurred during startup.

Acceptance: retail BIOS-to-startup execution **OBSERVED**. Retail menu pixels,
input, audio, native boot, launcher launch, and human gameplay remain
**NOT_OBSERVED/NOT_RUN**. No menu or device-parity claim follows from entry.

### Retry-exhaustion observation, 2026-09-05

`tools/retail_status_retry_observe.lua` read-only arms `80044198` and exits
only if `D_800763F4` has reached the retail `003C0000` retry threshold. A
fresh interpreter boot ran for 150 seconds after arming without reaching
the threshold. The live state at retry exhaustion and the subsequent
`8004424C` to `80044250` callback loop remain unverified.

The observer was then qualified to log the first `80044198` arrival. It saw
`pc=80044198`, `ra=80044188`, `retry=0`, and `tick=deadline=0` at cycle
`54688849`. The direct `80044190` call would link to `80044198`, so this is an
unqualified/shared arrival and was deliberately rejected as the retry frame.
The run was stopped after recording that fact; it does not establish menu or
CD-status behavior.

The observer now also arms the exact `80044734` call instruction before it
accepts an `80044124` entry. A fresh normal boot again saw only the unrelated
entry at cycle `54687805`; it did not execute `80044734` during the bounded
run. Thus the direct startup-dispatch status call remains **NOT_OBSERVED**;
the `80044124` hit alone is not authority for its live frame.

The observed unrelated `80044124` entry is reached by the `80043B9C`
CD-status worker. Its tracked 354-word assembly is now an exact native bridge
for the no-command return path only; the bridge is not a C decompilation and
does not establish menu progress. Formatter calls and jump-table routes still
stop explicitly pending their retail inputs and C recovery.

### Post-input startup boundary, 2026-09-05

`tools/retail_post_input_observe.lua` arms a read-only execution breakpoint at
`80043300`, but accepts it only with the direct startup caller return address
`800101EC`. An unfiltered normal interpreter boot first reached the same
routine at cycle `253221132` with return address `8003F8F8`; that is a shared
routine use, not the call after `80018918`. A fresh filtered run did not reach
the startup-specific return-address pair during its 55-second limit. This is
negative timing evidence only: it neither disproves later startup execution
nor observes the menu, controller input, or native port.

### Post-startup direct calls, 2026-09-05

`tools/retail_poststart_observe.lua` performed a fresh normal interpreter boot
with a fresh portable profile. It observed two direct calls from the startup
loop in order, `8002C8F4` at cycle `543853010`, then `8001971C` at cycle
`543996854`. The script exits at the second breakpoint, so the two routines
are reached by this retail boot. It does not observe menu pixels, input,
audio, overlay residency, native execution, or launcher behavior.

## Device-query observation

`tools/retail_device_observe.lua` observes normal interpreter boot through the
first mode-zero device query. It checks the order of startup, callback entry,
callback return, query entry, and query return. Intermediate breakpoints read
CPU registers only; the observer never reads MMIO or changes guest state.
To repeat this test, use the startup invocation above with the device observer.

Two runs exited zero after observing callback return `v0=8006BAFC` and query
input `a0=0`, returning `v0=0` at `8005C294`. The second run observed:

| PC | Observation before instruction execution |
| --- | --- |
| `8005C1F0` | Masked GPU information in `v0=00000400`; comparison constant `v1=2` |
| `8005C210` | `v0=00002000`, `v1=E1001000`, before the OR instruction |
| `8005C234` | GPU status read result `v0=14802000`, before masking with `1000` |
| `8005C294` | Return value `v0=0`; caller return address `8005BEC4` |

The instructions imply the draw-mode word becomes `E1003000`; that value
is derived from the observed operands, not a separately captured bus write.
The status bit `1000` is clear, explaining the zero return. The observer's
initial log label for `8005C210` was `draw_mode_write_value`; it was corrected
to `draw_mode_before_or` to make the pre-instruction timing explicit.

These are results of this emulator configuration, not a hardware GPU oracle
or permission to hardcode zero in the native query. Unknown GPU-command
messages still occur. CPU cycle counts differed by six between the two runs,
so deterministic timing has not been established. Native device behavior and
full menu execution remain unresolved.

The neighboring emulator source provides a plausible explanation for the
`400` information value: `GPU::init` in `src/core/gpu.cc` initializes
`m_dataRet=0x400`; `CtrlQuery::type` in `src/core/gpu.h` categorizes queries
6 and 7 as `Unknown`; `GPU::write1(CtrlQuery*)` has no update for that type.
This is consistent with a retained latch after the game's query 7, rather
than a hardware GPU identification response. Binary-to-source provenance is
not established, so this remains a source-supported explanation, not proof
of the packaged binary's implementation. Query 7 is an unresolved authority
boundary for native device emulation.

## Callback table observation

`tools/retail_callback_observe.lua` snapshots cached RAM at initializer entry
`80042718` and its return epilogue `800427E0`. Normal interpreter disc boot
completed both snapshots and exited zero. The table pointer remained
`8006CB64`, and the return value was `8006BAFC`.

| State | Entry | Return |
| --- | --- | --- |
| Guard at `8006BAFC` | `0000` | `0001` |
| Saved-stack word at `8006BB38` | `00000000` | `8006CB14` |
| Table slot `+04` | `00000000` | `80042F8C` |
| Table slot `+14` | `00000000` | `80042D60` |

Slots `+00/+08/+0C/+10/+18/+1C` remained
`80073524/800429DC/80042718/80042B30/80042BDC/8006BAFC` respectively.
Only cached RAM and registers were read; no device reads or guest writes were
introduced by the observer. These observations establish this boot's installed
state, not complete callback semantics or native compatibility.

The September analysis matched all 22 words (88 bytes) of the installer
`80042C90` that returns slot `+14`'s target.
Its MMIO write and remaining callback dependencies are not yet native-bound.

Two further routines matched in the same analysis.

- The registration wrapper `800425B0` (12 words) forwards both arguments
  through table slot `+08` to the observed target `800429DC`.
- The handler `80042CE8` (30 words) increments the counter at `8006CBB8`
  and reads eight callback slots in order, invoking nonzero entries.
  The reconstruction uses an unsigned counter for
  defined wrap and volatile access to reproduce the observed extra load.

The isolated host test executes the actual handler source and verifies counter
wrap before callbacks, null slots, exactly eight slots, and a callback replacing
a later slot during dispatch. Host pointers in that test are not a guest-memory
binding. Registration, interrupts, and native startup remain open.

An extended callback observer completed normal boot through the two startup
registration calls, exiting zero. Slot addresses are based at `8006BB00`:

| Index | Installed callback | Previous / return value | Cached enabled bits before → after |
| --- | --- | --- | --- |
| 0 | `80042CE8` | `00000000` | `0000 → 0001` |
| 3 | `80042E08` | `00000000` | `0001 → 0009` |

The enabled bits are the cached halfword at `8006BB2C`; the observer did not
read the hardware interrupt mask. Both slots contained the installed callback
at return, and the original callback-table/guard observations were reproduced.

The registration body spans `[800429DC,80042B30)` (340 bytes), crossing the
disassembly label `80042A40`. That interior label is not an independently
callable C routine. Execution falls through into it with live registers from
`800429DC`. A 348-byte reconstruction covered same-value and uninitialized-guard
returns, temporary interrupt masking, slot and bitmap changes, BIOS calls for
indices 0/4/5/6, and mask restoration. It did not match the 340-byte original.
