# Port architecture standard (Brave Fencer Musashi)

This September 2026 design describes the port's preservation and customization
goals, including optional graphics settings, mods and cheats.

## Purpose: preservation

The port exists to preserve *Brave Fencer Musashi* as released: its
gameplay, logic, visuals and audio are exactly retail. Customization sits
on top of that and never replaces it. The rules:

1. **Retail by default.** With default settings, everything the game
   computes, shows and plays is what the North American PlayStation
   release does. There are two kinds of allowed difference:
   - the platform layer, which reproduces PS1 hardware (CPU, GTE, GPU,
     SPU, CD, pads, cards, timers) as faithfully as it can;
   - opt-in mods, cheats and enhancements (widescreen, HD, the gl
     renderer, fast-forward), all off by default.
2. **No fibs.** Nothing is described as "matches retail" or "1:1" without
   objective evidence, linked where the claim is made:
   - a byte-matched C path (a `provenance/matches.json` entry the build
     runs);
   - an emulator or video differential with identical output (frames,
     VRAM, audio samples, RAM at a checkpoint);
   - measured numbers (for example `tools/port_matched_ratio.py`).
   Without that, the word is **untested**.
3. **Every known gap is public.** Anything that is not yet 1:1 has an
   entry in [KNOWN-DIVERGENCES.md](KNOWN-DIVERGENCES.md) with its reason
   and status, until evidence closes it. That includes hand-written
   PsyQ-compat code, host workarounds that write guest RAM or inject input,
   and timing approximations.
4. **Host code doesn't steer the game.** A host workaround that writes
   guest state or injects input is a divergence (D-001, D-002), even when
   it only gets a test through. It must be opt-in, or the retail path must
   be made to work.
5. **Toward matched C, for game code.** Hand-written port C that shadows
   a registered, byte-matched *game* function is replaced by that
   function. Each swap is proven by the lane differential probe (0
   different) plus a headless boot, and noted in the commit.
6. **PsyQ stays platform code.** No decompiled Sony PsyQ code goes into
   the port, even when the registry has matched C for it; the PsyQ layer
   is PsyCross or the port's own code. Instead, each shim is proven
   faithful to the retail library code it replaces, using
   `tests/psyq_shim_probe.c`: the same inputs must give the same guest RAM,
   VRAM and return value. Any that differ are listed in
   KNOWN-DIVERGENCES.md.

The game itself (code, data, audio, video) comes from the user's own disc
at run time. The repository holds no retail bytes (see the retail guard,
`tools/check_port_retail.py`).

### Measuring it

`tools/port_matched_ratio.py` compares a trace recorded with the native lane off against the set of candidate host routines in the build report. It counts functions, calls and instructions that map to matched C, host shims, or guest execution. It also identifies shims that take precedence over a matched C entry. The resulting shares estimate how much of that trace the build could execute on the host. To measure actual execution, run the same route with the native lane enabled and record completed host calls and guest CPU activity.

```sh
# lane-ON build; the trace runs with the lane off so it records every
# call and instruction (tools/patches/trace-insns.patch adds
# MUSASHI_TRACE_INSNS for exact per-PC instruction counts)
MUSASHI_NATIVE_LANE=0 MUSASHI_PAUSE_AT_START_SCREEN=1 MUSASHI_TRACE_INSNS=run/trace.insns \
    tools/run_headless_boot.sh ... --out run      # (see "Headless live boot")
tools/port_matched_ratio.py --trace-counts run/trace.counts --trace-insns run/trace.insns \
    --lane-report BUILD/generated/native_lane/report.json --json ratio.json
```

Without `--trace-insns`, the instruction share is an estimate (calls ×
function size) and says so.

## Layers

```
 game C (matched decomp TUs under src/, native lane / hand ports in pc_port/)
   │  calls ONLY the bfm_plat_* interfaces below (no PsyCross, no host APIs)
 ┌─┴──────────────────────────────────────────────────────────────────────┐
 │ platform interfaces (pc_port/platform/bfm_plat*.h; umbrella bfm_plat.h)│
 │  renderer · audio · input · disc/storage · timing · mods · config      │
 └─┬──────────────────────────────────────────────────────────────────────┘
   │  one or more backends per interface, picked by name from the config
 backends: PsyCross (MIT, fetched, not committed) · in-house HLE (musashi_*
           device owners in pc_port/, null/image/scripted built-ins)
           · later: OpenGL/Vulkan renderer, other audio devices
```

Rules:
1. Game code never includes a PsyCross header, a host/OS header or a Sony
   Psy-Q header. Where the retail code called a Psy-Q library function
   (`LoadImage`, `PutDrawEnv`, `DrawOTag`, `CdRead`, `CdSearchFile`, `VSync`,
   `PadRead`, …), the port calls a `bfm_plat_*` function with the same
   meaning, and a backend implements it. `tests/test_bfm_plat.py` checks
   that the core `bfm_plat*` sources include no SDL/GL/AL/PsyCross header.
   Only `pc_port/platform/backends/` may include them.
2. Interfaces are defined by **meaning, not by PS1 registers**. The renderer
   takes draw/display environments, semantic primitives
   (`BfmPlatPrim`: flat/gouraud/textured tris and quads, lines, tiles,
   sprites, fills) and VRAM uploads, not GP0 words. That lets a modern
   renderer upscale, replace textures, widen the view or interpolate
   frames. The PS1-accurate path is one backend among several.
3. No Sony Psy-Q code or tables in any backend. Game data is never committed
   or shipped. It is read from the user's disc image at runtime.
4. Backends can be swapped without touching game code. Selecting an unknown
   renderer, audio or input backend falls back to the built-in `null`
   backend, so headless runs and tests always work. A disc backend has no
   fallback: a missing disc is a user-visible error.

Return convention: `BFM_PLAT_OK` (0), or a negative `BfmPlatResult`. The
older `musashi_*` device owners return 1 for success, and the backends
that wrap them translate.

## Interfaces

| Interface | Header | Covers | Backends now |
|---|---|---|---|
| renderer | `bfm_plat_renderer.h` | draw env / disp env, primitive lists, VRAM upload/download, present, output (window, internal scale, aspect, frame rate), overlay text | `null` (built-in: real 1024x512 VRAM, FILL, upload/download), `psycross` (`backends/renderer_psycross.c` → PsyCross libgpu) |
| audio | `bfm_plat_audio.h` | 44.1 kHz s16 stereo MAIN (SPU mix) and CD (XA/CD-DA) streams, volume, pause, status; drops PCM during fast-forward | `null` (built-in, counts frames), `sdl` (`backends/audio_sdl.c` → `pc_port/audio_sdl.c`) |
| input | `bfm_plat_input.h` | two pads as PS1 buttons (active-high) and analog, `[input]` bindings from config, port hotkeys (fast-forward, console, cheat menu, pause, screenshot, quit), console text/key queue, PS1 SIO wire bytes for the pad shim | `null` (built-in "scripted": state is set by code, for tests and the route autopilot), `sdl` (`backends/input_sdl.c`: keyboard + SDL GameController) |
| disc/storage | `bfm_plat_storage.h` | sector reads (2048 / 2324 / 2340 / 2352), MSF↔LBA, ISO9660 file lookup, whole-file reads through the mods file hook, memory cards (128 KiB `.mcd` per slot, atomic save) | `image` (built-in: user `.cue`/`.bin` raw or `.iso`), `pinned` (`backends/disc_pinned.c` → `pc_port/disc_media.c`, verified retail cue/bin by SHA-256) |
| timing | `bfm_plat_timing.h` | `VSync(n)` pacing at the NTSC field rate (1001/60000 s), resync after stalls, fast-forward speed (0 = uncapped), injectable clock | built-in host monotonic clock (POSIX `clock_gettime` / Win32 QPC) |
| mods | `bfm_plat_mods.h`, `bfm_plugin.h` | folder loader, event hooks, C plugins, script-runtime seam, hash-keyed asset replacement, texture decoders, texture dump, guest-memory binding | built-in (`.bfmi` decoder); `backends/texture_png.c` (`.png`, in-house on zlib) |
| console/cheats | `bfm_plat_console.h`, `bfm_plat_console_ui.h` | cheat registry (plugin callbacks, `cheats.ini` pokes), cheat-menu model, console commands, console text front end (scrollback, edit line, history, Tab completion) drawn via renderer `overlay_text` | built-in |
| game events | `bfm_plat_events.h` (events themselves in `bfm_plugin.h`) | event name catalog, guest-PC hook-site table for the native lane, `events` console command | built-in |
| config | `bfm_plat_config.h`, `bfm_plat_ini.h` | user INI file, CLI overrides, save-back, per-plugin sections | built-in |

The existing `pc_port/` device owners (`gpu_psycross.c`, `audio_sdl.c`,
`bios_input.c`, `disc_media.c`, `source_clock.c`, …) remain the
register-level HLE the native lane runs on. `bfm_plat` sits above them:
`audio_sdl.c` and `disc_media.c` are wrapped directly as backends. The
guest's own pad polling (`bios_input.c`, SIO) should read
`bfm_plat_input_pad()` + `bfm_plat_pad_to_wire()`. `source_clock.c` is a
modeled cycle count, not wall time, and stays separate from
`bfm_plat_timing`, which only paces presentation.

`bfm_plat_register_builtin_backends()` (`bfm_plat_backends.c`) registers
the backends compiled into a build: `-DBFM_PLAT_WITH_PSYCROSS`,
`-DBFM_PLAT_WITH_SDL_AUDIO`, `-DBFM_PLAT_WITH_SDL_INPUT`,
`-DBFM_PLAT_WITH_PINNED_DISC`. `-DBFM_PLAT_WITH_PNG` registers the PNG
texture decoder in `bfm_plat_init`. The default dependency-free build
registers none and runs on the built-ins.

### Input bindings

Bindings come from `[input]`. Each key names a target and lists host
inputs, comma-separated. A target with no key uses its default, and an
empty value unbinds it.

```ini
[input]
analog = 1                       ; report DualShock analog mode (sdl backend)
p1.cross = key:z, pad:a          ; defaults: p1 keyboard + pad 1, p2 pad 2 only
p1.l2 = key:q, pad:lefttrigger   ; axes: pad:lefttrigger, pad:-leftx, pad:+lefty
p2.start = pad:start
hotkey.fast_forward = key:tab    ; also console (`), cheat_menu (f1), pause,
hotkey.quit =                    ; screenshot (f12), quit (unbound)
```

Targets are `p1.`/`p2.` plus `select l3 r3 start up right down left l2 r2
l1 r1 triangle circle cross square`, and `hotkey.` plus `fast_forward
console cheat_menu pause screenshot quit`. The core parses these
(`bfm_plat_input_bindings_from_config`). A backend supplies only a resolver
from `key:NAME` / `pad:NAME` to its host codes, so a future non-SDL backend
gets the same config format. The `sdl` backend reads keyboard and
controller *state* after `SDL_PumpEvents`, and gets typed text and
controller hotplug through `SDL_AddEventWatch`. It never takes events off
the queue that PsyCross pumps. While the console is open, letter-key
bindings are ignored and pads report neutral to the game.

### Console front end

`bfm_plat_console_ui` is the text UI over `bfm_plat_console_exec`. The
console hotkey toggles it when `[debug] console = 1`. Text typed before it
opens is dropped. Input backends push text and editing keys
(`bfm_plat_input_push_text/_key`), and the frame loop drains them while
the console is open. It draws 12 scrollback lines plus the prompt through
`bfm_plat_renderer_overlay_text`, so any renderer that implements
`overlay_text` shows it. Keys: Enter, Backspace, Left/Right, Up/Down
(history), PageUp/PageDown, Tab (command completion), Esc.

### Lifecycle and the frame loop

```c
BfmPlatConfig cfg;
bfm_plat_config_defaults(&cfg);
char path[512];
if (bfm_plat_config_default_path(path, sizeof path) == BFM_PLAT_OK)
    bfm_plat_config_load_file(&cfg, path);          /* missing file is fine */
bfm_plat_config_apply_args(&cfg, argc, argv, used);
bfm_plat_init(&cfg);                  /* backends, disc, console, mods, BOOT */
bfm_plat_guest_memory_bind(&ram);     /* once guest RAM exists */
for (;;) {
    bfm_plat_frame_begin();           /* input poll + INPUT hook, hotkeys, FRAME_BEGIN */
    /* ... game frame: renderer / audio / disc calls ... */
    bfm_plat_frame_end(fields);       /* FRAME_END (cheats), cheat menu, present, VSync pacing */
}
bfm_plat_shutdown();
```

### Game-event hook points

The port emits events at selected guest function entries. The active overlay,
guest registers and guest RAM determine the event payload. A trace can reach
a shared guest address in several different overlays.

| Event | Guest entry | What the platform observes |
|---|---|---|
| `frame_begin` | `0x800189A8` | Pad polling starts the main frame. A resident modal loop also calls this site and can emit a begin without the corresponding end. |
| `frame_end` | `0x800184F0` | The main frame reaches its final game call before waiting for VSync. |
| `room_enter` | `0x80128158` | An active location overlay reaches scene phase zero after a phase or overlay change. The host obtains the area and room from the selected overlay name. Phase zero was observed as initialization in SC02 member 31. |
| `room_exit` | `0x80011B7C` | The low half of the first argument is mode 8 while a location overlay is selected. Other modes use this same function. Additional room traces are needed to establish how specific mode 8 is to exits. |
| `save`, `load` | `0x8002B0B4` | The card state at `0x800760AC` enters 13 for a write or 17 or 32 for a read. The second argument supplies the slot. A retry can produce another event. These hooks report a request inside the state machine. |
| `projection` | `0x8004923C`, `0x8004921C` | One entry supplies the unsigned 16-bit projection distance. The other supplies both signed 16-bit screen offsets. |
| `item_get` | `0x800D0F0C` | A nonzero item byte in the second argument identifies one inventory acquisition. Clearing a slot suppresses the event. |
| `item_use` | `0x800D128C` | The low byte of the first argument identifies the item whose effect is about to run. |

The frame-end memory observer emits `damage` and `bp_use` when it reads a
lower HP or BP value. The payload contains the previously observed value, the
new value and the current maximum. The original gauge routines have 141
identified overlay copies, in MAIN member 12 and 140 scene code members. Battle start and battle end still have
no established entry. The general player-active field and the save-slot
display do not establish combat boundaries.

Run the platform hook tests from the repository root.

```sh
python3 -m pytest tests/test_bfm_plat.py -k 'hook or gauge'
```

The host probes exercise registration, register and RAM inputs, payloads and
event suppression with synthetic state. With the matching USA disc extracted,
the tests also check the executable and overlay spans associated with the
identified hooks. Gameplay coverage depends on which scenes have actually
been exercised. The host event definitions and selection rules are in
[bfm_plat_events.h](../pc_port/platform/bfm_plat_events.h) and
[bfm_plat_hook_sites.c](../pc_port/platform/bfm_plat_hook_sites.c).

How the native lane uses them:
- `bfm_plat_guest_memory_bind` (RAM) and `bfm_plat_guest_regs_bind`
  (registers, for `$a0`/`$a1`). Then `bfm_plat_hooks_register_known()`.
- On overlay selection: `bfm_plat_hooks_set_overlay("SC02_031")`, or NULL
  for none. Sites at or above `0x80128158` (the location-overlay window)
  must name an overlay pattern (`"SC*"` or an exact name), because the same
  address is different code in each overlay. Registration refuses them
  otherwise.
- On entry to a guest function: `if (bfm_plat_hooks_has(pc))
  bfm_plat_hooks_at(pc);`.
- Frame sites are actions. The FRAME_BEGIN site calls `bfm_plat_frame_wait()`
  (input poll, hotkeys, and blocking while paused). The FRAME_END site calls
  `bfm_plat_frame_hook_end()` (pending screenshot, cheats, overlays). The
  game's own VSync → `bfm_plat_timing_vsync` does the pacing, and its own
  DrawOTag/present does the presenting.
- Direct emitters for hand-ported code: `bfm_plat_event_room_enter/exit`,
  `bfm_plat_event_battle`, `bfm_plat_event_item_get`, `bfm_plat_event_save`.

The `events` console command lists each event with its count and every
registered site with its overlay and hits.

### Live game state

The USA game snapshots live data into a 732-byte save record. Five fields
describe the layout used by the save and restore routines.

| Record offset | Live address | Contents |
|---|---|---|
| `0x24` | `0x80078E78` | 152-byte player and progress block |
| `0xBC` | `0x800A6588` | 64 halfword figure prices |
| `0x13C` | `0x800AE648` | 64 bytes of story flags |
| `0x17C` | `0x800BA1B8` | 256 byte-addressed script variables |
| `0x27C` | `0x800BA2B8` | 96 bytes used for NPC timestamps |

The save-slot display independently identifies several offsets in the progress
block. The exported guest address constants are in
[bfm_plat_events.h](../pc_port/platform/bfm_plat_events.h).

| Field | Live address or variable | Meaning and limits |
|---|---|---|
| Play time | `0x80078E7C` | Four bytes hold frames, seconds, minutes and hours. The clock uses 30 frames per second and caps at 99 hours, 59 minutes, 59 seconds. |
| Wallet | `0x80078E8C` | Signed 32-bit balance in stored units. A new game starts at 100. The shop rejects an unaffordable debit and caps increases at 99,999. The HUD adds a trailing zero. |
| First gauge | `0x80078EB2` maximum, `0x80078EB4` current | Unsigned maximum, signed current. Both start at 250 and the maximum caps at 500. The save display and refill behavior support the HP interpretation. |
| Second gauge | `0x80078EB6` maximum, `0x80078EB8` current | Same field sizes. Both start at 250 and the maximum caps at 1,634. The BP label is inferred from the second gauge's use. |
| Inventory | Script variables `0x2F` through `0x3A` | Twelve one-byte slots hold item IDs. Zero marks a vacancy. Each occupied slot has a corresponding acquisition-day halfword starting at variable `0x14`, two bytes apart. The game uses elapsed days for spoilage and maturation. |
| Toy figures | Script variables `0x63` through `0x8D` | Each byte records shop availability and ownership in bits 7 and 6. The low nibble gives the price trend. Shop prices use the separate halfword price table. |
| Day and time | `0x80078EAC` day, `0x80078EB0` minute, `0x80078EB1` hour | The day occupies two bytes. The slot display limits the day to 999 and the time to 23 hours, 59 minutes. |
| Location | `0x800B9A08` | Signed 16-bit current-location identifier. The location-change handler replaces this value. Its complete room mapping remains open. |
| NPC timestamps | `0x800BA2B8` | 24 four-byte entries. The halfword two bytes into an entry stores elapsed days converted to hours plus the current hour. The interpretation as time since a particular NPC encounter is provisional. |

The resident item-effect dispatcher handles IDs 1 through 119. The platform
does not require item names. The representation of equipment and key items
remains unverified.

The wallet and inventory tests exercise the independently recovered game
functions using synthetic balances, slots and dates. The public registries
can also be checked against a local USA disc extraction.
[Inventory test](../tests/test_resident_inventory.py) ·
[Wallet test](../tests/test_shop_wallet.py)

### Memory watches and derived events

`bfm_plat_watch.h`: a watch is a guest range (up to 64 bytes, addresses
normalised to physical RAM) plus a callback with the old and new bytes.
There are two feeds:
- **Write hook.** The native lane / interpreter store path
  calls `if (bfm_plat_watch_hit(addr, len)) { read old; store;
  bfm_plat_watch_on_write(addr, len, old, new); }`. `hit` is a min/max
  test plus a 4 KiB-page bitmap.
- **Poll** (works today). With `[mods] watch_poll = 1` (default),
  `bfm_plat_watch_poll()` runs at each FRAME_END (and at the FRAME_END hook
  site) and diffs each watch against its snapshot through the guest-memory
  binding. It sees at most one change per frame and doesn't run while
  paused.

Plugins get `host->watch_add/watch_remove` (plugin ABI 2, appended fields
only), and Lua gets `bfm.watch(addr, len, fn(addr, old, new))`. A mod's
watches are dropped when it unloads.

Derived events (`[mods] derived_events = 1`, default) come from data
changes, not from a code site, and carry `BFM_EVENT_FLAG_DERIVED`:

| Event | Source | Payload |
|---|---|---|
| `damage` (16) | gauge-1 current drops | `BfmEventStat {value, previous, max, flags}` |
| `bp_use` (17) | gauge-2 current drops | `BfmEventStat` |
| `money` (18) | money word changes | `BfmEventStat` (max 99999) |
| `item_get` (12) | a figure's owned bit sets | `BfmEventItem {item = figure index, count = 1, kind = BFM_ITEM_KIND_FIGURE, flags}` |
| `item_get` (12) | an inventory slot goes from 0 to an item | `BfmEventItem {item = id, count = 1, kind = BFM_ITEM_KIND_INVENTORY, flags}` |

When a code site (damage, bp_use, item_get) reports a change, the platform
suppresses the matching derived event for that change. Consumers get
exactly one event, with `flags = 0` from the code site or
`BFM_EVENT_FLAG_DERIVED` from the watch.

Caveats: a gauge drop is not always combat damage (scripted drains count
too). In poll mode several changes within one frame collapse into one
event. `battle_start`/`battle_end` are still unsited; a sustained
`damage` run is a hint, not a battle boundary.

### Cheat pack

`pc_port/mods/examples/cheat_pack/cheats.ini` holds `infinite_hp` and
`infinite_bp` (copy cheats: current = max each frame), `max_money` (poke
99999, the game's own cap) and `max_hp_gauge` (poke 500, the game's cap).
All are disabled by default. `cheats.ini` supports poke cheats
(`address`, `value`, `width`) and copy cheats (`address`, `copy_from`,
`width`).

### 60 fps (design note, not implemented)

Retail logic runs at 30 fps: `main` waits `VSync(*(s32 *)(p + 0xA3E8))`,
normally 2 fields per frame. The options:

1. **Interpolated presentation (recommended first).** Keep the 30 Hz logic
   tick. Present twice per tick, drawing the second frame from positions
   interpolated between the last two ticks.
   - What to interpolate. In order of payoff: the camera (the GTE
     rotation/translation the game loads each frame), per-object model
     matrices, and 2D sprite positions.
   - Hooks needed:
     - GTE matrix loads (the rotation/translation setters, the next libgte
       sites to identify after SetGeomScreen/SetGeomOffset)
     - a per-object identity (the OT/primitive's owner, or the object
       struct address)
     - the frame boundary: FRAME_BEGIN/FRAME_END, which already exist
   - The renderer needs primitives with a stable identity across frames,
     so it can match "the same quad" in ticks N and N+1. The semantic
     `BfmPlatPrim` list is where that belongs.
   - Risks:
     - No identity for a primitive: particles, and primitives rebuilt
       each frame, can't be matched, so they draw uninterpolated or are
       held.
     - Teleports and cuts: the camera snaps between scenes, so a large
       delta must disable interpolation for that frame.
     - Doubled GPU work.
     - Movie (MDEC) frames stay at their native rate.
   - Game logic, timing and RNG are untouched, so saves, scripts and
     physics stay retail.
2. **Doubled logic tick.** Run the game at 60 Hz (fields = 1) and halve
   every per-frame velocity, timer and animation step.
   - Risks: the game's timers, animation tables and physics constants
     assume 30 Hz. Every per-frame increment in hundreds of overlays would
     need patching (e.g. the play-time tick `func_80029444` counts 30
     frames per second). RNG advances twice as often, and scripted timing
     and the day/night clock drift. The patches would diverge from the
     matched decomp.
   - Hooks needed: essentially per-site patches, not platform hooks. Not
     recommended.
3. **Frame generation in the renderer.** Motion-vector or optical-flow
   extrapolation, with no game knowledge. Artifacts on the PS1's
   low-resolution affine textures; only as a later renderer option.

What to build first for option 1:
- identify the libgte matrix setters (SetRotMatrix/SetTransMatrix
  equivalents) as projection-like hook sites
- add a per-primitive owner tag to `BfmPlatPrim`
- give renderers `present_interpolated(alpha)` behind `[video]
  frame_rate = 60`, which already exists in the config

`frame_rate = 60` is accepted today but has no effect until this lands.

### Widescreen

Config: `[video] aspect = 4:3 | 16:9 | 16:10` and `widescreen_mode = hor+ |
anamorphic` (`--aspect`; `widescreen = 1` / `--widescreen` mean 16:9).
The maths is in `bfm_plat_widescreen.c`, in 16.16 fixed point so every
backend rounds the same way. The PS1 display is always shown at 4:3,
whatever its pixel width.

- **hor+**: projection unchanged. The renderer shows a wider window with
  unstretched pixels. Visible width in display pixels is
  `disp_w * A / (4/3)`, centred: 428 wide starting at x = -54 for a
  320-wide display at 16:9, and 384 starting at -32 at 16:10. The
  PsyCross backend does this: with PGXP it maps the display unstretched
  into a window of any aspect, so the backend sizes its window to the
  aspect (`bfm_plat_widescreen_window`: 1280x720 at 16:9). Geometry the
  GTE projects past the 4:3 edges then becomes visible. Objects the game
  culls at those edges will pop at the sides; that is the known cost.
- **anamorphic**: after perspective division, screen X is squeezed around
  OFX by `k = (4/3)/A` (0.75 at 16:9, 0.8333 at 16:10), and the output is
  stretched to A. The game's culling stays correct, but 2D HUD elements
  are stretched. A GTE backend applies it with
  `bfm_plat_widescreen_sx_current(sx)` on every projected SX (RTPS/RTPT).
  The renderer must stretch. PsyCross can't, so `set_output` refuses
  anamorphic there and `bfm_plat_init` falls back to hor+ with a log line.
- The game's projection is known to the platform: hook sites at libgte
  `SetGeomScreen`/`SetGeomOffset` (table above) record H/OFX/OFY
  (`bfm_plat_projection_get`) and emit `BFM_EVENT_PROJECTION`. Retail sets
  H = 1000 once at init (horizontal FOV 18.2° across 320 px; hor+ 16:9 shows
  24.2°). `bfm_plat_projection_hfov_mdeg` computes these.
- Tests: `test_widescreen_projection_maths` checks every aspect, display
  width (256-640) and a range of SX values against exact fractions, plus
  the config keys and the projection hook sites.

### gl renderer (HD / restyle)

`[backends] renderer = gl` selects a modern OpenGL 3.3 core backend
(`backends/renderer_gl.c`, built with `BFM_PLAT_WITH_GL`). `psycross` stays
the PS1 reference renderer. GL entry points come from
`SDL_GL_GetProcAddress`; there is no loader library, and only the Khronos
`GL/glcorearb.h` header is needed at compile time. There are three layers:

- `backends/gl/bfm_gl_core.c`: the CPU side, with no GL. It holds the
  VRAM mirror, texture-page/CLUT decode, the PS1 blend and dither
  formulas, the batcher, hor+ routing, the HD replacement registry and the
  24-bit display decode.
- `backends/gl/bfm_gl_exec.c`: runs the core's command list on any current
  GL 3.3 context, given a proc loader.
- `backends/renderer_gl.c`: the SDL window, the context and the
  `BfmPlatRendererBackend` vtable.

How it works:

- **VRAM.** A 1024x512 `R16UI` texture is fed from a CPU mirror by dirty
  box. The mirror backs LoadImage, StoreImage and MoveImage; MoveImage is
  StoreImage followed by LoadImage in the compat layer.
  - An upload flushes the queued primitives first, because they still
    sample the old texels. It then copies the rect into every render
    target it lands in, so frame-buffer uploads (backgrounds, loading
    screens) appear on screen.
  - A download reads rendered pixels back into the mirror first (the
    centre sample of each scaled pixel, STP = 0), but only where primitives
    drew. StoreImage of a frame buffer therefore sees the frame, while
    textures keep their STP bits.
- **Render targets.**
  - The *VRAM target* is RGBA8, `1024S x 512S` for internal scale S = 1..8
    (`video.internal_scale`).
  - Draw areas shaped like the display ("scene slots": the two frame
    buffers, least-recently-used) render into their own
    `(disp_w + 2m)S x disp_h S` targets instead, where m is the hor+
    margin.
  - Present shows the slot at the display origin, fitted to the window
    with black bars, or the display rect of the VRAM target when no slot
    matches.
- **Primitives.** Every `BfmPlatPrim` kind is covered:
  - flat, gouraud and textured polys;
  - lines, as one-pixel quads with inclusive endpoints;
  - tiles, sprites and fills.
  The draw-env offset, clip rect (as scissor) and texture window apply.
  Batches split only on target, blend mode, STP split, scissor, texture
  window or replacement texture.
- **Shader.** One shader decodes 4-, 8- and 15-bit pages through the CLUT
  exactly as `bfm_gl_sample` does, and texel 0x0000 is transparent.
  - Modulation is `(t * c) >> 7`, done in float; `RAW` skips it.
  - Interpolation is affine (`noperspective`), like the PS1.
  - Texels are sampled nearest at native texel size, so higher internal
    scales sharpen the geometry, not the art.
- **Semi-transparency.** The formulas are `bfm_gl_blend_state`'s:
  | Mode | Formula | Factors (src / dst) |
  |---|---|---|
  | 0 | B/2 + F/2 | 0.5 / 0.5 |
  | 1 | B + F | 1 / 1 |
  | 2 | B − F | 1 / 1, reverse subtract |
  | 3 | B + F/4 | 0.25 / 1 |
  - Opaque prims and modes 0, 1 and 3 use GL 3.3 core **dual-source
    blending**: the shader outputs `rgb * sf` and a second colour `df`,
    with blend `ONE, SRC1_COLOR`. Each fragment picks its own factors:
    opaque, or blended when the pixel is untextured, an STP texel or a
    replacement texel. Opaque and semi prims therefore share draws in PS1
    order, and textured semi prims need no second pass.
  - Mode 2 needs `FUNC_REVERSE_SUBTRACT`, so it gets its own draws.
    Textured mode 2 is drawn in two passes: texels without STP opaque,
    then STP texels subtracted.
  - Accuracy: modes 1 and 2 are bit-exact against the PS1 formula. Modes 0
    and 3 keep the fraction the PS1 truncates, so a channel can come out
    one 5-bit step higher. The target holds 8 bits per channel; this is
    intended on the HD path.
- **Dither.** The PS1 4x4 matrix is applied at native pixel granularity
  (`gl_FragCoord / S`), and only to primitives the PS1 dithers: gouraud or
  modulated-texture polys and lines, never rects. `video.dither = auto |
  on | off`; `auto` dithers at 1x only, so HD output stays smooth.
- **Widescreen.**
  - hor+: margin `m = -visible_x0` from `bfm_plat_widescreen_compute`.
    Scene-slot scissors are widened by m where the draw area touches the
    frame buffer's left or right edge. Geometry the GTE puts left of 0 or
    right of `disp_w` survives, and `clear_bg` fills clear the margins too.
  - anamorphic: m = 0, and the image is stretched to the output aspect.
- **HD replacement.** `upload_replacement` keeps the original pixels in the
  mirror (for StoreImage and for backends without HD) and registers an
  RGBA8 texture for the rect.
  - A textured primitive whose whole texel footprint lies inside a
    replaced rect samples that texture bilinear, in coordinates normalised
    to the rect. This works for 4-, 8- and 15-bit pages alike, and any
    image size or aspect is accepted.
  - Alpha < 0.5 is transparent, and vertex colour still modulates the
    image.
  - A plain upload overlapping the rect retires the replacement.
- **Mask bit** (GP0 E6). The decoder carries E6 as the prim flags
  `BFM_PRIM_FLAG_MASK_SET` and `_CHECK`.
  - Every render target's alpha channel is bit 15 of its pixels: texel
    STP | set-mask. Fills clear it, and uploads copy it in. Blending uses
    separate functions, so alpha is always the fragment's bit and never a
    blend.
  - Check-mask uses a stencil. A target gets one (depth24/stencil8) the
    first time it sees a check, and it is built once from alpha. A
    temporary copy is used for that, because GL 3.3 can't sample a bound
    target.
  - After that, every draw keeps the stencil in step. A command whose
    fragments can write both values runs two passes: written bit 0, then
    written bit 1, where the written bit is STP | set-mask. Each fragment
    is drawn once and writes its own bit. A test for "pixel bit 0" with
    REPLACE of the written bit is `EQUAL 0` or `GREATER 1`.
  - Only mask-check commands break batches, plus set-mask in textured
    mode-2 commands.
  - Readback returns bit 15 from alpha, so StoreImage sees mask bits.
  - LoadImage ignores the mask settings; on the PS1 transfers honour them.
- **Render feedback**: textures sampling pixels the GPU drew.
  - A 16x16-tile map in the batcher records which target holds each VRAM
    tile's newest pixels: the VRAM target, a scene slot, or the mirror.
  - For a textured prim whose texture or CLUT footprint covers rendered
    tiles:
    - *15-bit pages* without wrap or texture window, all from one target,
      get a `SNAPSHOT`. That is a GPU blit of the tile-aligned footprint
      at device resolution, sampled nearest. Render-to-texture effects
      keep their HD detail: at 3x, a triangle edge copied through a
      snapshot matches device pixel for device pixel. A snapshot is reused
      until something writes into its rect, and a list holds at most 32.
    - *CLUT pages*, texture windows, wraps and mixed owners get a
      `RESOLVE`: that tile run is read back into the mirror (all 16 bits)
      and the VRAM texture re-uploaded, mid-list. GPU-drawn index data and
      GPU-drawn CLUTs decode exactly.
  - StoreImage (and so MoveImage) resolves the rendered tiles it covers.
  - Fills and uploads mark fully covered tiles clean again.
- **24-bit display** (`DispEnv.rgb24`, used by movies): the frame is
  decoded from the mirror on the CPU and shown at 4:3.
- **Overlay text.** The built-in font is drawn in display pixels after the
  frame is composed, mapped onto the 4:3 area of the window. It never
  touches game VRAM.

Known limits:
- The mask settings don't apply to LoadImage/MoveImage transfers.
- A prim that samples the pixels it is drawing over sees them as they were
  before it started (the PS1 reads them as it writes).
- Replacements ignore the texture window and the CLUT, so palette swaps of
  one texture share its replacement. Semi-transparent replacements blend
  every texel.

Benchmark (`tools/gl_bench/run.sh [--prims N] [--frames N] [--scales 1,4]
[--feedback] [--mask] [--opaque]`) runs on OSMesa, headless.
- **Frame:** double-buffered 320x240 hor+ 16:9 with 4000 prims: half
  gouraud-textured 4-bit quads, plus 8-bit triangles, flat and gouraud
  polys, semi-transparent 15-bit quads, sprites, lines and semi tiles.
  That is about 0.25 Mpx of fill at 1x. It is heavy for a PS1 scene but
  within what the PS1 GPU drew at 30 fps.
- **Options:** `--feedback` adds 4 off-screen render-to-texture panels per
  frame; `--mask` puts set/check on 10% of the prims.
- **Columns:** CPU batching, GPU execute, present, fps, process CPU time,
  ns per rasterised pixel, and command/draw counts.

Results on llvmpipe (Mesa 25.0.7, LLVM 19, 12 cores), with the host at
load average 42–60 from other jobs. Wall-clock numbers there are
contention-bound, so compare them relatively:

| frame | scale | batch ms | gpu ms | present ms | fps | draws |
|---|---|---|---|---|---|---|
| default | 1x | 1.0 | 49 | 18 | 14.7 | 1 |
| default | 4x | 1.5 | 257 | 26 | 3.5 | 1 |
| --feedback | 1x | 1.0 | 43 | 17 | 16.2 | 6 |
| --feedback | 4x | 1.2 | 263 | 25 | 3.5 | 6 |
| --mask | 1x | 1.0 | 157 | 28 | 5.4 | 430 |
| --mask | 4x | 1.0 | 597 | 57 | 1.5 | 430 |

What the numbers say:
- Batching is about 0.25 µs per prim on the CPU, independent of scale.
- Dual-source blending took the default frame from 1,039 draws to 1, and
  from 188 to 49 ms GPU at 1x on the same host.
- Mask-check frames are about 3x the cost. Check prims break batches, and
  the stencil upkeep doubles fragment work.
- llvmpipe is a software floor: a desktop GPU does these frames in well
  under a millisecond. Use the bench to compare changes on one host.

Tests (`tests/test_bfm_gl.py`, `tests/bfm_gl_probe.c`,
`tests/bfm_gl_sdl_smoke.c`):
- **core** always runs, headless. It covers:
  - CLUT, sampler and texture window against hand-worked words;
  - modulation against `(t*c)>>7`;
  - the blend formulas against hand values;
  - the GL blend state, evaluated for every 5-bit pair of every mode,
    against the PS1 formula;
  - the dither matrix and clamping;
  - batching: merging, splitting, uv spans, line quads, fills into the
    mirror;
  - hor+ routing: slots, widened scissors, margins, LRU eviction, copies;
  - replacement lookup, coordinates and retirement;
  - VRAM move with overlap and wrap, and the 24-bit decode.
- **CPU reference rasteriser** (`bfm_gl_ref_draw`): the definition of
  exact for TILE/SPRITE/FILL. It covers clip, offset, window, CLUT,
  transparency, modulation, semi-transparency on untextured and STP
  pixels, mask check/set, and bit 15 of written pixels.
- **exec** runs the real executor on an OSMesa (llvmpipe) GL 3.3 core
  context, headless, compared pixel by pixel with the CPU reference. It
  covers:
  - mask set/check scenes, exact in all 16 bits at 1x and 3x. A mutation
    with the stencil test disabled fails them;
  - render feedback, exact in all 16 bits against the reference:
    - off-screen 15-bit render-to-texture through a snapshot;
    - GPU-drawn 4-bit index data plus a GPU-drawn CLUT through a resolve;
    - a hor+ frame buffer sampling itself;
    - the whole 1024x512 VRAM resolved and compared, at 1x and 3x;
  - HD detail preserved through a snapshot at 3x;
  - 3,072 texels through 4, 8 and 15 bit at 1x and 3x, all exact;
  - every blend mode with the STP split;
  - dither at 2x;
  - HD replacement;
  - hor+ margins and present letterboxing;
  - overlay placement;
  - 24-bit image present.
- **bench** builds `tools/gl_bench` at -O2 -Werror and runs 2 frames
  with feedback and mask.
- **sdl** drives the whole backend through `bfm_plat_renderer_*` on SDL's
  offscreen driver with Mesa EGL: double-buffered hor+ frames, StoreImage
  readback, HD replacement, 24-bit mode, and a scale change.

On a desktop the same code opens a real window; interactive use needs a
display. Without one, `bfm_plat_renderer_open("gl")` fails cleanly and
falls back to `null`. exec and sdl skip when their libraries are missing
(`BFM_HOST_DEPS`, `BFM_GL_INCLUDE` and `BFM_OSMESA` override the search).

### GPU bridge (native_boot → bfm_plat renderer)

`bfm_plat_gpu_bridge.{h,c}` speaks the PS1 GPU's port protocol in front
of the bfm_plat renderer. With it, native_boot, which emulates the GPU ports
in `gpu_controller.c`, can draw through the `psycross` or `gl` backend
instead of the controller's CPU rasteriser.

| Input | Output |
|---|---|
| GP0 draw commands (polys, lines incl. polylines, rects, fill) | assembled by word count, decoded by `bfm_psyq_decode_packet` (E1/E6 state on the prims), batched into `bfm_plat_renderer_submit` |
| GP0 E2–E5 | draw env: texture window, drawing area → clip, offset |
| GP0 A0 / C0 / 80 | `upload_vram` / `download_vram` (answered on GPUREAD) / download + upload. Rects wrapping at the VRAM edge are split |
| GP1 00/01/03/05/08/10 | reset, FIFO reset, display enable, display env (origin, 256–640 × 240/480, interlace, 24-bit), GPU info |
| vblank | flush + `present` + `begin_frame` |

- **Attach (tee):** `musashi_gpu_controller` gets a `tap` callback on
  every GP0/GP1 port write and a `raster_disabled` flag. FIFO, GPUSTAT, fills
  and VRAM transfers are unchanged, and only the CPU rasterisation stops.
  native_boot then presents through `bfm_gpu_bridge_vblank`.
- **Switch:** `MUSASHI_GPU=cpu|bfm_plat` (default `cpu`) and
  `MUSASHI_RENDERER` (default `psycross`, which draws into native_boot's own
  PsyCross context). The bfm_plat config key is `[video] gpu`.
- **GPU integration.** The bridge design adds three pieces:
  - the controller's tap and raster switch;
  - native_boot's switch, attach and `present_frame`;
  - CMake entries for `bfm_plat_gpu_bridge.c` and `psyq/psyq_psycross.c`.
  The renderer opens after PsyCross initialization. Opening it inside
  `plat_start` crashed in PsyCross `ResetGraph` during testing.
- **Tests:** `tests/test_bfm_gpu_bridge.py`.
  - Synthetic streams go through the bridge into a reference backend
    (`bfm_gl_ref_draw`), under ASan/UBSan: fill, opaque and semi tiles, the
    `0x2A` fade quad (abr 2), raw, neutral and dim `0x64/0x65` sprites, a
    4-bit CLUT sprite, a semi `0x67` sprite, a polyline, a VRAM copy, a
    moved and clipped drawing area, a wrapped upload, the texture window,
    C0 GPUREAD, GP1 info and vblank.
  - When a `gpu_controller.c` is available (`BFM_GPU_CONTROLLER_DIR`), the
    same streams also go through the controller and are compared region by
    region. With the blend correction, the controller matched the implemented
    regions. Before the correction, semi-transparent regions differed.
- **Known controller gaps,** reported rather than asserted:
  - raw-texture rects `0x65/67/75/77/7D/7F` are not drawn;
  - textured-rect modulation is missing;
  - polylines are unhandled, and the unhandled words desync the command
    stream.
- **Smoke:** the patched native-lane build engages the bridge
  (`GPU_PATH bfm_plat renderer=psycross`) and runs normally. Rendering it
  live needs a long boot run.

### First-run disc check

`bfm_plat_disc_check.h` is the single disc check. The native lane calls
it too, rather than keeping its own.
- **Source:** `[disc] path` or `--disc PATH`. When that's empty, the
  first run looks in `[disc] search` (default `disc`, next to the binary),
  preferring a `.cue`, then `.iso`/`.img`, then `.bin`.
- **Check:** the image opens through the `image` backend, SYSTEM.CNF's BOOT
  line names the executable, and that executable is read straight from the
  sectors (never through mod file replacement) and hashed with the in-house
  SHA-256. It must be `SLUS_007.26` with SHA-256
  `66371c3a7517e9eabd7cb6cf0c5abffe7296bd4bac29c85b8bf7bb9db349714a`.
- **Result:** `[disc] validate = 1` (the default) makes `bfm_plat_init`
  refuse a failing disc and log one user-facing message.

| Status | When | Message (short) |
|---|---|---|
| `no_path` | nothing configured or found | put your US dump in `disc/`, set `[disc] path`, or `--disc` |
| `unreadable` | missing file, size not a whole number of sectors, sector 16 not Mode 2 | the image is incomplete or not a dump |
| `missing_tracks` | a `.cue` names a file that isn't there (named in the detail) | keep the .cue next to all its .bin files |
| `unsupported_format` | `.chd` or an unknown extension | CHD isn't supported yet: `chdman extractcd` |
| `not_ps1` | no ISO9660 volume, no SYSTEM.CNF, no BOOT line | not a PlayStation disc |
| `wrong_region` | the BOOT names a Japanese/Asian or PAL serial | the port needs SLUS-00726 |
| `wrong_disc` | another US serial | a different game |
| `bad_dump` | SLUS_007.26 with a different hash | damaged or modified dump; re-dump |

CHD: libchdr (BSD-3-Clause) is the candidate reader. It needs zlib, LZMA,
zstd and FLAC decoders, so it's left as a later storage backend, and `.chd`
gets the conversion hint.

### Headless live boot

`tools/run_headless_boot.sh --binary BUILD/musashi_native_boot --disc
EXTRACTED/disc [--out DIR] [--timeout S] [--capture-every N]` boots the
real game (the user's own disc) with no display and no sound card.

Everything the run writes goes to `--out` (default `headless-out/`,
git-ignored), which must be outside the repository or git-ignored:
- `out.log`;
- `trace` and `trace.counts` (`MUSASHI_TRACE_FUNCS`);
- `frames.log`: one line per presented frame, with its non-black pixel
  count;
- `frame_*.png` and `vram_*.png`.

It ends by printing the stop line, any refusal, the boot markers
(`TITLE_LOAD`, `SCENE1_REACHED`, …), the first non-black frame and the top
of the call ranking.

| Setting | Why | Status |
|---|---|---|
| `SDL_VIDEODRIVER=offscreen`, Mesa EGL via a vendor JSON, `EGL_PLATFORM=surfaceless` | real GL 3.3 context without a display (llvmpipe) | environment |
| `LIBGL_ALWAYS_SOFTWARE` unset | setting it segfaults Mesa 25.0's EGL inside `SDL_CreateWindow` | Mesa bug; Mesa falls back to llvmpipe by itself |
| `MUSASHI_AUDIO=null` | explicit, logged null audio | used when the binary supports it. For older builds the script configures an ALSA null PCM through `ALSA_CONFIG_PATH` and `SDL_AUDIODRIVER=alsa` and logs the fallback |
| `MUSASHI_CODE_IMAGE=<EXE>` | instructions are fetched from the user's EXE | historical builds required this when guest memory was unbound at run entry. The affected boot stopped at `80010000` |
| `MUSASHI_GUEST_CLOCK=1` (default; `--host-clock` to compare) | guest time advances with executed cycles | on a loaded host the host-paced clock ends in an IRQ-dispatch refusal in the SPU poll loop (`8003b118`). Since 8edc1d55d it is printed: `IRQ_DISPATCH_REFUSED reason=source-overflow … hint=MUSASHI_GUEST_CLOCK=1` |
| `LD_PRELOAD` of Mesa `libEGL`/`libEGL_mesa`/`libgallium` | keeps Mesa mapped for PsyCross shutdown | for older builds where `GR_Shutdown` called GL after EGL had unloaded. The script uses `nm` to detect ordered shutdown (`musashi_psyx_shutdown`). Override with `--no-preload` or `--preload` |
| `tools/headless/bfm_swap_capture.c` (`LD_PRELOAD`, `BFM_CAPTURE_*`) | hooks `SDL_GL_SwapWindow` | observation only. It reads the back buffer before the real swap, and optionally PsyCross's `vram` array, found by `nm` offset. PNGs use `bfm_plat_png_write` |

Frames here are PsyCross presents, which only happen when native_boot
flushes the display. They are not guest VBlanks.

Diagnostics, none of which change the run:
- `BFM_CAPTURE_OT_HEADS` / `BFM_CAPTURE_OT_FRAMES`: display-list walks in
  guest RAM.
- `BFM_CAPTURE_GPU`: the controller's committed prims (`prims.log`), via a
  forwarding wrapper on its `store_vram` callback. The offsets come from
  `offsetof()` against the same build.
- `tools/headless/bfm_ot_replay.c`: replays a captured frame through
  `bfm_gl_ref_draw` and the gl renderer, plus an opaque variant.
- `tools/headless/gpu_semi_repro.c`: an 8-case semi-transparency repro
  against native-lane's `gpu_controller.c`, which draws all four semi
  modes as opaque. This caused the headless boot's white and grey fades:
  the game fades with a subtractive (abr 2) full-screen `0x2A` quad.

### Packaging (no game data)

`tools/package_port.sh --binary BIN --exe <your SLUS_007.26> --extracted
<your extracted disc> [--lib LIB ...]` stages:
- the binary
- the open-source libraries you pass
- license texts (LICENSE-NOTES, PsyCross, Lua, a THIRD-PARTY list)
- the docs, the plugin SDK headers, and the sample mods (as source)
- a README that tells users to supply their own US disc

It then runs `tools/retail_guard.py`, and writes the tarball only if the
guard passes. The guard fails on:
- disc-image, boot-executable, save-file or SYSTEM.CNF names
- any file identical (SHA-256) to the user's EXE or an extracted disc file
- any file containing 64 or more consecutive EXE bytes. The EXE is indexed
  with aligned 32-byte windows, so any 63-byte common run is caught. Each
  hit is extended to its full length, and low-entropy stretches such as
  padding are ignored.

The `--exe`/`--extracted` files stay on the user's machine; nothing from
them is copied. Tests: `tests/test_package_port.py` uses a synthetic random
executable, including the exact 64/63-byte threshold. In a historical
static-library build the guard found a 65-byte sequence of guest code addresses
matching the game's executable.

### Pause, screenshot, quit

- **Pause** (hotkey, `pause` command, `bfm_plat_pause`): `bfm_plat_frame_begin`
  returns `BFM_PLAT_PAUSED`. The caller skips its game logic and still calls
  `bfm_plat_frame_end`, which presents, draws "PAUSED" and paces but emits no
  FRAME_BEGIN/FRAME_END, so cheats and plugins don't tick. Audio is paused.
  The hook-site path blocks in `bfm_plat_frame_wait` instead, so the guest
  frame loop doesn't advance.
- **Screenshot** (hotkey, `screenshot [PATH.png]`): reads the current
  display area (last `set_disp_env`, 15- or 24-bit) back through
  `download_vram` and writes a PNG with the dependency-free encoder in
  `bfm_plat_image.c` (stored deflate, so zlib isn't needed). The shot is
  taken before overlays are drawn. Files go to `[video] screenshot_dir`
  (default `screenshots`) as `bfm_NNNNN.png`, at the native display
  resolution.
- **Quit** (hotkey, `quit`): sets `bfm_plat_quit_requested()` for the main
  loop.

## Mod hooks

- **Folders.** Every directory in `[mods] dirs` (';'-separated, default
  `mods` next to the binary) is scanned at startup. A mod is a folder with
  `mod.ini`:
  ```ini
  [mod]
  name = hello
  version = 1.0.0
  priority = 100        ; load order: ascending priority, then folder name
  enabled = 1
  plugin = hello_plugin ; <mod>/hello_plugin.so | .dll | .dylib
  script = main.lua     ; handed to a registered script runtime
  ```
  Folders without a manifest are skipped, and duplicate mod names are
  ignored. Plugin and script names must be plain file names inside the mod
  folder.
- **Event hooks.** `BfmEvent` (numbering is ABI): `BOOT`, `SHUTDOWN`,
  `FRAME_BEGIN`, `FRAME_END` (frame tick), `VSYNC`, `INPUT` (editable pads
  and hotkeys), `BEFORE_PRESENT`/`AFTER_PRESENT`, `ROOM_ENTER`/`ROOM_EXIT`,
  `BATTLE_START`/`BATTLE_END`, `ITEM_GET`, `SAVE`/`LOAD`. Hooks run in
  subscription order, which follows load order. Hooks made by mods are
  dropped at mods shutdown, so an unloaded plugin never stays subscribed.
- **C plugins** (`bfm_plugin.h`, ABI version 1). A plugin exports
  `bfm_plugin_init(const BfmPluginHost *, BfmPluginInfo *)` and optionally
  `bfm_plugin_shutdown()`. The host table offers log, subscribe/unsubscribe,
  `guest_read`/`guest_write`, `register_cheat`/`set_cheat`,
  `register_command` and `config_get`. `BfmPluginHost` only ever grows at
  the end, so plugins check `abi_version`/`size`. Plugins can also be linked
  into the binary and registered with
  `bfm_plat_mods_register_static_plugin` (for platforms without dlopen, and
  for tests). Sample: `pc_port/mods/examples/hello`.
- **Lua** (optional, `-DBFM_PLAT_WITH_LUA`, `backends/script_lua.c`).
  `tools/fetch_lua.sh` fetches Lua 5.4.9 into the ignored
  `tools/third_party/lua`, verified against lua.org's published SHA-256.
  Each mod whose manifest names a `.lua` script gets its own `lua_State`.
  - Sandbox: base (no dofile/loadfile/load/require), table, string, math,
    utf8 and coroutine. No io, os, package or debug. Text chunks only.
  - Every load and callback runs under a 5M-instruction budget, so a
    runaway script errors out instead of hanging the game.
  - The global `bfm` table offers the same services as `BfmPluginHost`:
    `bfm.on(event, fn)` / `bfm.off(id)` with payload tables (input is
    editable: `pads[i].buttons`, sticks, `hotkeys`); `bfm.read8/16/32`,
    `write8/16/32`, `read/write(addr, n|s)`; `bfm.cheat{...}` (poke or
    `apply`/`on_toggle`); `bfm.set_cheat`; `bfm.command(name, help, fn)`;
    `bfm.config`, `bfm.log`; `bfm.PAD_*` / `bfm.HOTKEY_*`.
  - Cheats and commands can be registered only while the main chunk runs.
    They belong to the mod and are dropped with it. Hooks made later are
    dropped at mods shutdown too.
  - Sample: `pc_port/mods/examples/lua_hello`.
  - Without the define, a mod naming a script logs "no script runtime".
  The same `BfmPlatScriptRuntime` seam takes other languages.
- **Asset replacement, hash-keyed, from mods only.** The key is 16 hex digits
  of FNV-1a 64 over the original data:
  - textures: `le16 width, le16 height, width*height le16 pixels` of each
    VRAM upload → `assets/textures/<hash>.<ext>`. The built-in decoder is
    `.bfmi` ("BFMI", le16 w, le16 h, RGBA8888). `.png` comes from
    `backends/texture_png.c` (in-house, on zlib's inflate: non-interlaced
    grey, RGB, palette 1-8 bit with tRNS, grey+alpha and RGBA, 8/16-bit,
    all filters, CRC-checked; Adam7 is refused). Other formats plug in
    through `bfm_plat_mods_register_decoder`. A replacement may be the same size
    or an integer multiple of it (HD). Backends with `upload_replacement`
    get the HD image. Otherwise a same-size image is folded back to 15-bit
    VRAM (opaque black keeps the STP bit), and anything else keeps the
    original.
  - disc files (`bfm_plat_disc_read_file`): whole-file hash →
    `assets/files/<hash>.<ext>`.
  - When two mods replace the same asset, the later one in load order wins.
    Mods contain only modder-made files. The port never ships disc data,
    and `[mods] dump_textures = 1` writes the user's own uploads to
    `dump_dir` on their machine only, so modders can find hashes. Those
    dumps must not be redistributed.
- **Cheat menu and console** (`bfm_plat_console.h`). Cheats come from
  plugins (callback) or from a mod's `cheats.ini` (poke: `address`,
  `value`, `width` 1/2/4, `enabled`). Enabled cheats run on every
  `FRAME_END`. The cheat menu is a model toggled by the cheat-menu hotkey
  and drawn through the renderer overlay. Console commands: `help`,
  `cheats`, `cheat NAME on|off|toggle`, `peek`, `poke`, `get`, `set`,
  `mods`, `ff [SPEED]`, `events`, plus any a plugin registers. The text
  front end is `bfm_plat_console_ui` (above).
- **Config file + command line** (`bfm_plat_config.h`). The path is
  `$BFM_PORT_CONFIG`, else `$XDG_CONFIG_HOME/bfm-port/config.ini`, else
  `~/.config/bfm-port/config.ini` (`%APPDATA%\bfm-port\config.ini` on
  Windows).
  ```ini
  [video]    widescreen, internal_scale (1-8), window_width, window_height,
             fullscreen, frame_rate (30|60), vsync
  [timing]   fast_forward_speed (0-64, 0 = uncapped), fast_forward_toggle
  [backends] renderer, audio, input, storage
  [disc]     path          ; the user's own image
  [storage]  save_dir
  [mods]     enabled, dirs, dump_textures, dump_dir
  [debug]    console, log_level
  [mod.<name>] ...         ; free-form, read by plugins via config_get
  ```
  CLI: `--config PATH` (applied first), `--set section.key=value`,
  `--widescreen`, `--fps N`, `--scale N`, `--renderer NAME`,
  `--audio NAME`, `--disc PATH`, `--no-mods`, `--fullscreen`. Out-of-range
  values are rejected, the previous value stays and `rejected_values`
  counts them. `bfm_plat_config_save_file` writes everything back,
  including plugin sections.
- **Test/harness aids** (route autopilot through the scripted input backend,
  injected clock) stay behind test code and are not part of the default mod
  state.

Widescreen and 60 fps need both halves. The config and renderer carry the
aspect and frame-rate target (`BfmPlatOutput`). The game-side projection
change (GTE H / screen-width constants) and frame interpolation are game
and renderer work still to be done; the flags alone only change the output
aspect.

## Backends and licenses

| Component | Where | License | Notes |
|---|---|---|---|
| bfm_plat core, built-in backends, mods loader, sample plugin | `pc_port/platform/`, `pc_port/mods/` | this repository's code (see `LICENSE-NOTES.md`) | original work, no Psy-Q material |
| PsyCross | `tools/third_party/psycross`, fetched by `tools/fetch_toolchains.sh` at `e56e4cde1c2b8a15e0d4e38b26cdd9202e0d17e6` from `github.com/OpenDriver2/PsyCross`; ignored by git | MIT, © 2020 REDRIVER2 Project | a reimplementation of the Psy-Q library API on SDL2/OpenAL/OpenGL. It is not Sony code. Bundles a glad-generated GL loader (`src/render/glad.c`; generated code under glad's permissive terms, Khronos GL registry Apache-2.0) |
| in-house HLE device owners | `pc_port/*.c` (`gpu_psycross.c`, `audio_sdl.c`, `disc_media.c`, `bios_*.c`, …) | this repository | the `sdl` and `pinned` backends wrap these |
| SDL2 (runtime dependency) | system / host-port-deps | zlib | not vendored |
| Khronos `GL/glcorearb.h`, `KHR/khrplatform.h` (`gl` renderer, compile time) | system / host-port-deps headers | MIT (© 2013–2020 / 2008–2018 The Khronos Group) | headers only, not vendored; GL entry points are loaded at runtime through SDL, with no loader library |
| OpenGL driver (`gl` renderer, runtime) | the user's system | the driver's own | dynamically loaded by SDL |
| Mesa OSMesa / EGL (llvmpipe) | host-port-deps | MIT | tests only: headless GL for `tests/test_bfm_gl.py`, dlopened; never shipped |
| OpenAL Soft (via PsyCross) | system | LGPL-2.1, dynamically linked | not vendored |
| OpenSSL libcrypto (`disc_media.c` SHA-256) | system | Apache-2.0 | `pinned` backend only |
| built-in 5x7 debug font (`bfm_plat_font.c`) | `pc_port/platform/` | this repository | original glyphs drawn for this port, not derived from PsyCross's `dbugfont` |
| Lua 5.4.9 | `tools/third_party/lua`, fetched by `tools/fetch_lua.sh` (sha256 `2335b6c5…6fb8e6`); ignored by git | MIT, © 1994–2026 Lua.org, PUC-Rio | optional script runtime (`BFM_PLAT_WITH_LUA`) |
| zlib (`texture_png.c` inflate) | system libz | zlib | PNG decoder only; the decoder itself is in-house, and no third-party PNG code is vendored |
| Game data | the user's own disc image | © the rights holders | never committed or shipped |

## Status

At this design checkpoint, `tests/test_bfm_plat.py` exercised the following
interfaces without SDL. The same tests passed under ASan and UBSan.
- All interface headers, dispatchers, and the built-in `null` / `image` /
  scripted-input / host-clock backends.
- Config file, CLI and save-back. Mods loader with dlopen and static
  plugins, the script seam, texture and file replacement, texture dump.
  Cheats (callback and poke), cheat-menu model, console. Frame loop with
  hotkeys and fast-forward.
- `gl` renderer (GL 3.3 core via SDL). It runs headless in tests: the
  OSMesa executor and the SDL offscreen/EGL smoke test.
- `psycross`, `sdl` (audio), `sdl` (input) and `pinned` (disc) backends,
  compile-checked only against their headers. The PNG decoder is fully
  tested (zlib is present).
- Config-driven input bindings, the console text front end, the
  event catalog and the guest-PC hook-site table.

At that checkpoint the following work remained open.

- The `psycross`, `sdl` and `pinned` backend runtime tests required
  SDL2, OpenAL and GL development libraries and CMake integration.
- The `psycross` renderer has no HD `upload_replacement` path, and
  `set_output` is a no-op. PsyCross owns window size. HD is on `gl`.
- `gl`: not yet run against the real game. That needs the native lane's
  libgpu shim routed through `bfm_plat_renderer_*`.
- The SDL input backend's rebinding at runtime (`set_bindings`) has no UI.
- Overlay text on `psycross` is compile-checked only. It draws the
  built-in 5x7 font (`bfm_plat_font.c`, original to this repo) as TILE runs
  over a semi-transparent backing, in the current draw environment's
  coordinates. It uses no VRAM texture, because PsyCross's `FntLoad` would
  overwrite game VRAM.
- Guest sites for battle start/end and item get (no evidence yet). Room
  exit is low-confidence. Registering the known sites and reporting the
  selected overlay from the native lane.
- Anamorphic widescreen needs the GTE backend to call
  `bfm_plat_widescreen_sx_current` and a stretching renderer (`gl`
  stretches). On PsyCross it falls back to hor+. The hor+ edge culling is not patched.
- 60 fps interpolation.
- Routing the game's Psy-Q-shaped call sites (libgpu/libcd/libetc shims in
  the native lane) through `bfm_plat_*`.

## Psy-Q compatibility layer

`pc_port/platform/psyq/` provides the Psy-Q to PsyCross/HLE layer used by
the native lane. The generated coverage report is
[PSYQ-COMPAT.md](PSYQ-COMPAT.md).

**Enumeration.** `tools/psyq_callsites.py --vendor <vendor/bfm-decomp>` works
as follows:
- It takes the Psy-Q segment ranges from the vendor splat config (the
  `snd*`, `lib*`, `apicard*` segments: `0x8003A444`–`0x80062998`).
- It collects every `jal`/`j` target in the local, gitignored splat
  disassembly: `asm/main.s` game code `0x80010000`–`0x8003A444` and every
  `asm/overlays/*/*.s`. That disassembly is generated from the user's disc
  and is never committed. The tool exits with a clear message when it is
  absent; `--asm DIR` points it elsewhere.
- It names targets from the vendor symbols, from libgte instruction shape,
  or from `INFERRED` in `tools/psyq_compat_status.py`. Each inferred name
  carries its call-context evidence, e.g. LoadImage/DrawOTag/PutDrawEnv
  from the main loop.

It writes `bfm_psyq_calls.json` (facts: addresses, names, counts),
`bfm_psyq_compat.def` (the X-macro table), and `docs/PSYQ-COMPAT.md`. The
scan of the disassembly found 212 entry points and 30,230 call sites (overlay
code shared by many overlays counts once per overlay); 117 of the entry points are named.
`--check` fails if the committed outputs are stale, and the tests run it
when the vendor tree is present.

**Interface for the native lane.**
```c
/* bfm_psyq_compat.def: BFM_PSYQ(pc, name, status, argc, returns, wrapper) */
#define BFM_PSYQ(pc, n, st, ac, ret, w) \
    static int lane_##n(MusashiNativeLaneCall *c) { return w(c->r); }
#include "psyq/bfm_psyq_compat.def"
#undef BFM_PSYQ
#define BFM_PSYQ(pc, n, st, ac, ret, w) {pc, 2 * 8, lane_##n, #n, ac, ret},
static const MusashiNativeLaneEntry psyq_entries[] = {
#include "psyq/bfm_psyq_compat.def"
};
```
- Wrappers are `int wrapper(uint32_t *r)` over the o32 register file:
  a0–a3 = `r[4..7]`, then words at guest `sp+0x10`; the result goes in
  `r[2]`. They return 1 when done and 0 to refuse. `STUB` entries (and
  `PSYCROSS` ones in a build without PsyCross) always refuse, so the whole
  table can be installed: a refusal makes the interpreter run the retail
  code from the user's disc.
- `bfm_psyq_set_ram(phys0, size)` sets the host address of guest physical
  0 (`(void *)0x80000000` under the lane's design (a)).
  `bfm_psyq_set_scratchpad(p)` sets the 1 KiB scratchpad; the game puts
  RECTs there. Guest pointers are masked to physical and bounds-checked.
- `bfm_psyq_call(pc, r)` is a lookup-and-call for routers that don't
  instantiate the table.

The [compatibility table](PSYQ-COMPAT.md) lists each address, its current host
implementation or guest status, and the observed call count.

**Semantics differences to know about:**
- **Pointer arguments** are guest addresses. The layer converts them. For
  structures (RECT, DRAWENV 0x5C bytes, DISPENV 0x14, CdlFILE, OT/prim
  packets) it reads and writes the guest layout field by field, never a
  host struct cast.
- **OT and primitives live in guest RAM**, with 24-bit links. PsyCross's
  own P_TAG uses host-width pointers (`USE_EXTENDED_PRIM_POINTERS`), so a
  guest OT can't be handed to PsyCross's `DrawOTag`. The HLE `DrawOTag`
  walks the guest OT and decodes each GP0 packet into `BfmPlatPrim`:
  polygons, lines and polylines, rectangles, VRAM fill, and texpage (E1)
  state. The result goes to `bfm_plat_renderer_submit`, so every renderer
  backend and texture replacement applies. Unknown commands are counted
  (`bfm_psyq_stats`). E2–E6 inside packets are ignored; the draw
  environment comes from PutDrawEnv.
- **GTE state (bridge):** there is one cop2 register file: PsyCross's
  process-wide `gteRegs` bank. The port's `gte_owner.c` leases it
  for the interpreter (its CTC2/CFC2/data/command paths run on that bank),
  and PsyCross's libgte functions work on the same bank. So no copying is
  needed; the bridge only enforces the owner's lease.
  - `bfm_psyq_set_gte_bridge({begin, end, user})`. `begin()` must confirm
    that the calling thread holds the lease, CP0 Status has CU2 set, and no
    cop2 operation is in progress, then mark the owner busy. `end(commands)`
    clears that and accounts for the commands issued.
  - Without a bridge, every GTE-state wrapper refuses and the interpreter
    runs the retail code. Reentry is refused too.
  - Pointer arguments are translated with size checks.
  - PsyCross's `long *` out-parameters (8 bytes on LP64) go through host
    temporaries and are stored back as 32-bit words.
  - `RotTransPers`/`3`/`4` don't call PsyCross's versions. With PGXP on,
    those store PGXP float bits as `sxy` for PsyCross's float-vertex
    renderer. Instead the wrappers run RTPS/RTPT on the bank and read SXY,
    IR0, SZ3 and FLAG back as integers.
- The matrix stack operations execute through the ordinary guest path.
- The GTE probe exercises the supported host projection and matrix calls
  with an explicit bridge. Random-number and string services execute the
  program on the supplied disc.
- **VSync:** mode > 1 waits n fields through `bfm_plat_timing_vsync`, 0
  waits one, 1 returns 0 (there's no hsync counter on the host), and < 0
  returns the platform's field counter.
- **Callbacks** (DrawSyncCallback, VSyncCallback, DMACallback…) call guest
  code through the port's IRQ HLE and `musashi_native_guest_call`.

The ordinary player uses the supplied program for coordinate-tree, sprite,
matrix construction, lighting and projection services listed in the table.

Additional guest services include the following.
  - GsSortFastBg / GsSortBg: GsBG cell-map walkers, the second with
    scaling and rotation
  - GsGetLw, GsGetLs and GsGetLws.
  - GsSetRefView2L
  - the TMD object preset and sort routines (GsLinkObject5, GsPresetObject,
    OBJT/OBJT2/PRESET2): large per-primitive GTE transform and lighting
    code, with 2 call sites each
  - GsInitGraph and the clip/offset setters: libgs globals, 1–4 call sites
- Display, ordering-table and image services have individual entries in the
  compatibility table.

**Next:**
- name the 96 unnamed entries (their Psy-Q objects are listed in the
  report)
- libgs: GsSortFastBg / GsSortBg, GsSetRefView2L
- HLE the libgte functions PsyCross lacks (Square0/12, VectorNormalSS,
  ApplyTransposeMatrixLV, csqrt…) on the same bank through the bridge
- SetDrawEnv (DR_ENV packets) and printf (guest varargs)

## CMake wiring

The following example records the design's target dependencies. The
repository's top-level CMake file defines the current build.

```cmake
# Core: C99, no dependencies. dlopen for native plugins.
add_library(bfm_plat STATIC
    pc_port/platform/bfm_plat.c          pc_port/platform/bfm_plat_backends.c
    pc_port/platform/bfm_plat_ini.c      pc_port/platform/bfm_plat_config.c
    pc_port/platform/bfm_plat_renderer.c pc_port/platform/bfm_plat_audio.c
    pc_port/platform/bfm_plat_input.c    pc_port/platform/bfm_plat_storage.c
    pc_port/platform/bfm_plat_timing.c   pc_port/platform/bfm_plat_mods.c
    pc_port/platform/bfm_plat_console.c  pc_port/platform/bfm_plat_console_ui.c
    pc_port/platform/bfm_plat_events.c   pc_port/platform/bfm_plat_hook_sites.c
    pc_port/platform/bfm_plat_image.c    pc_port/platform/bfm_plat_font.c
    pc_port/platform/bfm_plat_widescreen.c   pc_port/platform/bfm_plat_watch.c
    pc_port/platform/bfm_plat_disc_check.c   pc_port/platform/bfm_plat_sha256.c
    pc_port/platform/psyq/bfm_psyq_compat.c)
target_include_directories(bfm_plat PUBLIC pc_port/platform)
target_link_libraries(bfm_plat PUBLIC ${CMAKE_DL_LIBS} m)

if(MUSASHI_WITH_PSYCROSS)   # SDL2 / OpenAL / GL already found above
    target_sources(bfm_plat PRIVATE
        pc_port/platform/backends/renderer_psycross.c
        pc_port/platform/backends/audio_sdl.c
        pc_port/platform/backends/input_sdl.c)
    # renderer_psycross.c needs C11 (PsyCross headers use static_assert)
    set_source_files_properties(pc_port/platform/backends/renderer_psycross.c
        PROPERTIES COMPILE_OPTIONS "-std=gnu11;-include;assert.h")
    target_include_directories(bfm_plat PRIVATE
        ${MUSASHI_PSYCROSS_DIR}/include pc_port/include)
    target_compile_definitions(bfm_plat PRIVATE BFM_PLAT_WITH_PSYCROSS
        BFM_PLAT_WITH_SDL_AUDIO BFM_PLAT_WITH_SDL_INPUT)
    target_link_libraries(bfm_plat PRIVATE SDL2::SDL2 musashi_pc_port)
endif()

find_package(ZLIB)          # optional: .png texture replacements
if(ZLIB_FOUND)
    target_sources(bfm_plat PRIVATE pc_port/platform/backends/texture_png.c)
    target_compile_definitions(bfm_plat PRIVATE BFM_PLAT_WITH_PNG)
    target_link_libraries(bfm_plat PRIVATE ZLIB::ZLIB)
endif()
# Optional Lua (after tools/fetch_lua.sh): all of tools/third_party/lua/src/*.c
# except lua.c and luac.c, as a separate C target without -Werror.
set(BFM_LUA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools/third_party/lua/src)
if(EXISTS ${BFM_LUA_DIR}/lua.h)
    file(GLOB BFM_LUA_SOURCES ${BFM_LUA_DIR}/*.c)
    list(FILTER BFM_LUA_SOURCES EXCLUDE REGEX "/(lua|luac)\\.c$")
    add_library(bfm_lua STATIC ${BFM_LUA_SOURCES})
    target_include_directories(bfm_lua PUBLIC ${BFM_LUA_DIR})
    target_sources(bfm_plat PRIVATE pc_port/platform/backends/script_lua.c)
    target_compile_definitions(bfm_plat PRIVATE BFM_PLAT_WITH_LUA)
    target_link_libraries(bfm_plat PRIVATE bfm_lua)
endif()
# Optional: pc_port/platform/backends/disc_pinned.c + BFM_PLAT_WITH_PINNED_DISC
# (links disc_media.c / OpenSSL::Crypto, already in musashi_pc_port).

# Optional gl renderer: needs SDL2 and the Khronos GL headers only (entry
# points come from SDL_GL_GetProcAddress; do not link libGL or a loader).
find_path(BFM_GLCOREARB_DIR GL/glcorearb.h)
if(TARGET SDL2::SDL2 AND BFM_GLCOREARB_DIR)
    target_sources(bfm_plat PRIVATE
        pc_port/platform/backends/renderer_gl.c
        pc_port/platform/backends/gl/bfm_gl_core.c
        pc_port/platform/backends/gl/bfm_gl_exec.c)
    target_include_directories(bfm_plat PRIVATE ${BFM_GLCOREARB_DIR})
    target_compile_definitions(bfm_plat PRIVATE BFM_PLAT_WITH_GL)
    target_link_libraries(bfm_plat PRIVATE SDL2::SDL2)
endif()
```

Notes:
- The core list must match `pc_port/platform/bfm_plat*.c` exactly. A
  `file(GLOB ... pc_port/platform/bfm_plat*.c)` keeps it in step as files
  are added.
- `bfm_plat.c` must be compiled with the same `BFM_PLAT_WITH_PNG` define as
  `texture_png.c`. Keep the definitions on the target, not per source.
- `musashi_audio_sdl_*` comes from `pc_port/audio_sdl.c`. Link whichever
  target builds it (shown here as `musashi_pc_port`).
- Tests: `python3 -m pytest tests/test_bfm_plat.py`. It builds its own
  probes and doesn't need CMake.
- For the sample plugin to find nothing but the plugin SDK, export nothing
  from the executable. Plugins use only the `BfmPluginHost` table.

## Planned integration order

1. Add interfaces and built-in backends while the native lane continues
   using its `musashi_*` devices.
2. The native lane's Psy-Q-shaped shims call `bfm_plat_*`: VSync →
   `bfm_plat_timing_vsync`, CdRead/CdSearchFile → `bfm_plat_disc_*`, pad →
   `bfm_plat_input_*`, LoadImage/DrawOTag → renderer. It binds guest RAM
   with `bfm_plat_guest_memory_bind`.
3. SDL libs arrive: build and run the `psycross`/`sdl`/`pinned` backends
   (SDL input and the console front end are already written). Add
   `overlay_text` to the `psycross` renderer.
4. Room-enter and frame-tick events from dispatch, then asset replacement
   on real uploads, then the cheat console on the real game.
5. A modern renderer backend comes later and only needs the renderer
   interface (`upload_replacement` for HD textures, `set_output` for scale
   and aspect).
