# brave-fencer-musashi-decomp-port

An experimental PC port of *Brave Fencer Musashi* for the USA PlayStation disc
(SLUS-00726). The player reads your own disc. Its hybrid runtime runs part of
the game as compiled host code and uses a CPU interpreter for the rest.

## Player downloads and source builds

**November 21, 2026. Player downloads are under review.**
The source build is available for development and testing.
We downloaded every historical player package from the public releases and
checked its published size and SHA-256. All 22 distinct Linux and Windows
executables contain a 36-byte game dispatch table. Nineteen also contain a
32-byte warp callback. Fourteen Linux executables include two complete original
diagnostic strings in their debug sections. The review covered 14 distinct
archives across 12 release tags.

The public source now reads the dispatch table from your disc. Its developer
menu has an empty warp list. A Linux player built at commit `7fd45d2` was
checked for both tables and both strings. The source also delegates interrupt
and device-library routines to the program on your disc. The current source
still needs an ordinary inn save and a successful Continue after restarting
before it can qualify as a replacement player download.

The Linux instructions cover disc verification, the build and the first
launch. Windows x64 users can cross-build the player on Linux with Zig and the
two pinned development SDKs. The Windows checks used Wine. Both builds run a
hybrid engine and require your own USA disc.
[Build on Linux](#build-from-source) · [Cross-build for Windows x64](docs/SETUP.md#cross-build-the-current-windows-x64-player-on-linux) · [Controls and save paths](docs/SETUP.md)

## About this project
This is a passion project. I'm working hard on it, but it's made for fun and for everyone's enjoyment — free, non-commercial, and made by a fan. If you enjoy it, that's the whole point.

## Milestones and progress

Public releases mark selected improvements; an entire milestone may still
be in progress. The [Milestones](https://github.com/Blizz127/brave-fencer-musashi-decomp-port/wiki/Milestones)
wiki page defines each milestone and records segment status and a dated
changelog.

**Next milestone: Chapter 1 playable end to end, with saving.**

### Verify the scene-object matches

The November 21 source update covers 231 functions across seven scene overlays,
totaling 16,748 bytes of matching executable code. In the opening scene, one
routine calls the handler selected by the player state byte. Another sets the
facing direction and counts frames. A separate scene event uses that timer
to advance its state on the 31st call from a fresh counter. The first opening handler
clears eight event flags and installs a camera callback. The next three handlers
switch the camera, wait on the direction timer, select animation records and
release the linked effect. Their no-disc test follows the normal and skip paths
with synthetic flags, animation responses and effect objects. The PC player
still uses a hybrid runtime.

In Chapter 1 room 3008, three trunks form the river crossing. Nine matched
functions cover the request to start the fall, actor initialization, standing,
rotation and settling. On the original USA disc, each trunk reached its settled
state 102 VSyncs after its fall began. The no-disc host test exercises all three
actors. Run it on Linux with `CC=gcc tests/run_forest_log_probes.sh`.

Room 3050 uses a facing selector and two signed-coordinate tests in scene
member SC02/0039. When a position falls inside the bounds, the predicates
use the signed area of three projected points. In an original-disc attract
run, 25 sampled selector calls agreed with the compiled C result, including
16 signed-area calls. A fourth matched scene function updates the camera
position and facing. Two other matches initialize the camera and select its
callback from the scene table. The camera update test links six original-matching smoothing
functions and supplies test matrix callbacks. The facing routines still need
the original memory access and timing behavior, so the native dispatcher
defers them in both scene members that share these bodies.
Run the synthetic cases with
`python3 -m unittest discover -s tests -p test_scene_geometry.py`.

A separate three-phase scene callback starts a fade, counts down ten updates,
issues two sound commands on expiry, and polls the fade result. Six matched
SC02/0039 functions cover this sequence, including dispatch and fade setup.
A seventh function in MAIN/0010 chooses which fade to poll. The no-disc host
test runs all three phases with synthetic scene state and checks every 16-bit
actor ID in the setup routine. The hybrid player retains guest execution for
these paths. Run `python3 -m unittest discover -s tests -p test_scene_phase.py`.

Room 3050 also has a matched 248-byte frame update. Retail execution
reached its return sequence during the idle demonstration. It calls
scene services in order, updates a frame counter and conditionally processes
the room's object block. A host test supplies the 19 service functions and
checks their order, the optional calls and counter wraparound. The player
continues to execute this scene update as guest code.

The same scene descriptors also contain a two-phase callback. Its setup starts
a second fade, resets its level and advances the actor phase. The next phase
polls the fade and returns whether its low 16 bits are nonzero. All four
matched routines retain guest execution in the player. The no-disc test checks
callback order, phase wraparound and the poll result. Run
`python3 -m unittest discover -s tests -p test_scene_fade_companion.py`.

Sixteen SC01/0000 functions read directional input, update actor velocity and
position, and check whether the current player state allows an action. The host
test covers digital button masks, analog stick thresholds, coordinate wraparound,
and movement limits. Fifteen of these functions also match the forest overlay
SC01/0077 byte for byte. The sixteenth reads a different scene table there.
The player continues to interpret this code. Run the component and native
selection tests with `python3 -m pytest -q tests/test_scene_player_motion.py`.

Four linked SC01/0077 floor-collision routines lower a point to the first
intersecting face in a grid cell and return its slope class. The component test
covers face order, plane boundaries, material selection and collision scratch
state on a late miss. The player interprets these routines. Run
`python3 -m pytest -q tests/test_forest_floor.py`.

Three additional forest routines test a moving segment against the faces in
its grid cell. An accepted contact calls the original VectorNormalSS service
before applying the final position adjustment. The public compatibility entry
for that service is a stub. The linked C test supplies a synthetic result and
checks the caller's correction, face material and scratch state.

The forest contact transform applies the actor's translation to the selected
contact and a second point. With rotation enabled it also transforms both
points and the saved normal through two resident geometry services. Its
component test checks in-place outputs, call order and 16-bit coordinate
wraparound. The player still interprets this routine.

The forest segment query now has matching C for its model selector and caller.
The selector can reject the segment or produce one of four model modes. The
caller searches a grid or a linked list and transforms the first hit. The
linked-box routine and its three face helpers also match. The linked test follows
a crossing from endpoint classification through the first accepted face. It
checks clipping, contact coordinates, normal direction and the box midpoint.
The player still interprets the collision path, including its divide traps,
resident normalizer and matrix services.
Run `python3 -m pytest -q tests/test_forest_contact_selection.py tests/test_forest_linked_box.py`.

Run the middle-state component test with a C compiler installed.

```sh
python3 -m unittest discover -s tests -p test_scene_opening_event.py
```

With the matching toolchain installed and your USA disc extracted as described
below, unpack the three scene archives into fresh directories. If you already
have the decoded members, start with the verification commands.

```sh
for scene in 01 02 03; do
    python3 tools/extract_cd.py "extracted/disc/files/SC${scene}.CD" --output "extracted/overlays/sc${scene}" || exit 1
done
python3 tools/decode_sc_overlays.py
python3 tools/verify_registry.py --registry provenance/scene-objects.json --jobs 1
python3 tools/check_no_retail_data.py --registry provenance/scene-objects.json --jobs 1
```

### Verify the inventory operations

Five resident functions find an empty slot, add and date an item, find the first
matching item, and remove it. The host test links these functions to the game's
script-variable accessors. It checks all 4,096 slot occupancy patterns and all
65,536 acquisition-day values. To run it without a disc, use
`python3 -m unittest discover -s tests -p test_resident_inventory.py`.

The shop wallet declines a debit if the result would be negative. It caps
positive balances at 99,999 stored units. The HUD displays a trailing zero.
Buying an item priced at 80 changes a stored balance of 100 to 92, displayed as
1,000 and 920. The wallet test checks arithmetic at the signed 32-bit limits
and verifies that a declined purchase preserves the balance. Run it with
`python3 -m unittest discover -s tests -p test_shop_wallet.py`.

Three scene routines pack the balance into decimal digits, build the seven
wallet sprites and refresh the five variable digits. The host packet builder takes the guest address explicitly so its GPU
links are independent of where the host allocates the buffer. Run
`python3 -m unittest discover -s tests -p test_shop_wallet_hud.py` to exercise
a purchase through all four wallet routines using synthetic packets. The decimal
conversion test covers every stored balance and signed shift boundaries. Run it
with `python3 -m unittest discover -s tests -p test_shop_wallet_bcd.py`.

Two bakery routines look up a message on the current menu page and store its
reference. The original purchase handler selects entry 1 after a purchase,
entry 2 for a full inventory and entry 3 when funds are insufficient. The
message contents come from the player's disc. Run the no-disc lookup test
with `python3 -m unittest discover -s tests -p test_shop_menu_messages.py`.
The hybrid player retains guest execution for these two routines because the
signed page index can reach mirrored guest RAM.

For the original code comparison, extract the resident archive from your USA
disc and verify its registry.

```sh
python3 tools/extract_cd.py extracted/disc/files/MAIN.CD --output extracted/overlays/main
python3 tools/verify_registry.py --registry provenance/resident-inventory.json --jobs 1
python3 tools/check_no_retail_data.py --registry provenance/resident-inventory.json --jobs 1
python3 tools/verify_registry.py --registry provenance/main0010-scene-fade.json --jobs 1
python3 tools/check_no_retail_data.py --registry provenance/main0010-scene-fade.json --jobs 1
```

### Verify the menu counter callbacks

Two callbacks in menu overlay MAIN/0012 increment the same 16-bit counter.
When the counter starts at 65,535, each callback stores zero and returns
65,536. The compiled C matches all 28 bytes of each original function.
The no-disc test checks both callbacks at every starting value and checks
that the neighboring halfwords keep their values. The player still uses its
existing hybrid dispatch for this menu.

```sh
python3 -m unittest discover -s tests -p test_menu_counter.py
python3 tools/verify_registry.py --registry provenance/native-main0012-menu-counter.json --jobs 1
python3 tools/check_no_retail_data.py --registry provenance/native-main0012-menu-counter.json --jobs 1
```

The two registry commands use MAIN member 0012 extracted from your USA disc
and the matching toolchain.

## How to play

Build the Linux player and launch it with the `.cue` from your own USA disc.
Keep every referenced `.bin` track beside the cue. The setup guide includes
instructions for backing up an existing memory card.
[Build from source](#build-from-source) · [Save locations](docs/SETUP.md#saves-settings-and-files-on-linux)

### Historical packaged controls, saves and dev menu

- Gameplay: D-pad/left stick or arrows/WASD; controller A/B/X/Y correspond to
  Cross/Circle/Square/Triangle, or keyboard C/V/X/Z. Enter is Start; Space is
  Select. [Full controls](docs/SETUP.md#controls).
- Save using the game's menus. The default Linux card is
  `~/.local/state/bfm-port/bfm_card0.mcd`; `BFM_SAVE_DIR` selects a separate
  card directory. The historical r38 binary passed an isolated inn save and
  cold Continue. Current source-build qualification is still in progress.
  [Save/config locations and limitations](docs/SETUP.md#saves-settings-and-files-on-linux).
- The packaged dev menu is enabled by default, with cheats initially off.
  Open with **F8** or **Back/Select+Start** on one controller. Navigate with
  Up/Down or D-pad/left stick; Enter/A selects, Esc/B goes back. **F1 is help.**
  The game keeps running while the menu is open.
- The menu offers 29 warp destinations, an empty Finish Area page, HP/BP
  refills, Drans/time actions, fast-forward and screenshots. See the
  [current Dev Menu wiki](https://github.com/Blizz127/brave-fencer-musashi-decomp-port/wiki/Dev-Menu)
  for exact options, effects and precautions.

The log is `~/.local/state/bfm-port/last-run.log` by default. See
[troubleshooting](docs/SETUP.md#troubleshooting). General `config.ini`
settings are not reliably loaded by r38; the setup guide lists working
launcher controls instead.

## Status

The historical r38 binary passed inn saving and cold Continue and fixed dark
forest frames. In that build, the forest, village and four-scene attract route passed
scoped checks. Attract still stops at known refusal `800495ec`; later
unsupported code may also stop. Automated checks used null audio, and do
not establish hardware audio fidelity or whole-game completion.

The goal remains a faithful, fully native port. Original rendering is the
release default. Cheats start off, even though the dev menu is enabled.
[Milestones](https://github.com/Blizz127/brave-fencer-musashi-decomp-port/wiki/Milestones)
and [known divergences](docs/KNOWN-DIVERGENCES.md) distinguish completed
checks from remaining work.

## Build from source

**Source snapshot limitation:** the public r38 tag points to sanitized
snapshot `ec620c78dd5f68a166f6565f7c2d8539e9c378a0`; its runtime binaries
were built from `61c60516d18bf6402e6dbd937d8c8d18ebd1af4b`. GitHub's automatic
source archives and the historical build instructions below do not
reproduce the newer r38 binary. The Windows x64 binary was cross-built
from unpublished port sources (windows-port `c427ee546`); this recipe does
not build it. The packaged menu and save instructions above describe the historical binaries.

The public build uses a curated main-executable registry at
`provenance/native-main.json`. It records 261 owner-authored C sources totaling
23,880 matched bytes. The registry includes three motion routines used by the
no-disc smoke test. The existing native-lane allowlist and compiler checks
decide which routines the player can run as host code. Other paths use the
interpreter. See [EXCLUDED.md](EXCLUDED.md) for the source inclusion criteria.

The model draw dispatcher now forwards the draw request, ordering-table context
and mode through both branches. Its 756-byte primitive-group handler and the
116-byte switch table generated by the compiler match the USA executable.
The original game was observed inside the handler and just after it returned
in room 3050. The host test uses generated model records and six test writers
to check primitive selection, cursor updates and changes to a group's count.
The public player still uses guest execution for this path.

The 512-byte sound-channel release routine also matches the USA executable.
The original game reached it during startup. A generated host test checks its
callback order, voice and slot state, and restoration of the busy flag.
Guest records use four-byte callback addresses. The Linux x64 host needs a
separate representation for its eight-byte callback pointers, and the player
retains guest execution for this routine. Audio output has not been compared.

Matched C is a reimplementation of the game's own code, written so that the
original compiler reproduces the original machine code. It is published for
study, interoperability and preservation, like other matching
decompilations; it does not grant any rights in *Brave Fencer Musashi*.

The r38 release binary is built with `-DMUSASHI_NATIVE_LANE=ON
-DMUSASHI_LANE_OWN_CODE_ONLY=ON`. With these options the native lane compiles
only owner-authored matched C listed in
[config/lane_own_code_allow.txt](config/lane_own_code_allow.txt) (guest
addresses only). Everything else runs interpreted from your own disc,
including every function whose recorded origin names the upstream
decompilation or Sony/Psy-Q code. The commands below describe the older
sanitized source snapshot. Its historical function counts do not describe
r38; the current release's admitted sources passed the no-m2c-macro guard.

A build needs the following:

1. **Linux build dependencies:** CMake, a C/C++ compiler and clang, Python 3,
   SDL2, OpenAL Soft, OpenGL, OpenSSL (libcrypto), and optionally zlib.
2. **PsyCross** (MIT). `./tools/fetch_toolchains.sh` fetches it at the pinned
   commit `e56e4cd` into `tools/third_party/psycross/`. The same script also
   fetches maspsx, m2c and the old-gcc compilers used by the matching tools.
   Then apply the port's local PsyCross edits (MIT):
   `git -C tools/third_party/psycross apply ../../patches/psycross-local.patch`
   (the patch is `tools/patches/psycross-local.patch`, made against
   `e56e4cde`). `./tools/fetch_lua.sh` fetches Lua for mod
   scripts. This step is optional.
3. **Prepare your USA disc for the player build.** The extracted executable
   supplies the initial values used to check guest globals. To reproduce the
   matching checks, generate its identity after extraction.

   ```sh
   python3 tools/register_retail.py "/path/to/Brave Fencer Musashi (USA).chd"
   python3 tools/extract_retail.py "/path/to/Brave Fencer Musashi (USA).chd"
   python3 tools/identify_retail.py extracted/disc/files
   python3 tools/verify_registry.py --registry provenance/native-main.json --jobs 1
   python3 tools/check_no_retail_data.py --registry provenance/native-main.json --jobs 1
   ```

4. **Build and launch the Linux player.** This configuration uses the published
   main-executable routines. The runtime reads game assets from your disc.

   ```sh
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
         -DMUSASHI_NATIVE_LANE=ON -DMUSASHI_LANE_OWN_CODE_ONLY=ON \
         -DMUSASHI_LANE_OVERLAY_MEMBERS=
   cmake --build build --parallel 4 --target musashi_native_boot
   build/musashi_native_boot --disc extracted/disc/disc.cue
   ```

To build the native function library on Linux without a disc or PsyCross, use
the following target. The generator omits functions whose 32-bit guest globals
need checking against the original executable. The player and the differential
runtime tests still require your disc.

```sh
cmake -S . -B build-source -DMUSASHI_WITH_PSYCROSS=OFF \
      -DMUSASHI_NATIVE_LANE=ON -DMUSASHI_LANE_OWN_CODE_ONLY=ON \
      -DMUSASHI_LANE_OVERLAY_MEMBERS=
cmake --build build-source --parallel 4 --target musashi_native_lane_gen
```

Full build instructions will be in BUILD.md (to be added). Also see
[docs/PC-PORT.md](docs/PC-PORT.md) and
[docs/ARCHITECTURE-PORT.md](docs/ARCHITECTURE-PORT.md).
`tools/package_legion.sh` builds the release bundle with `launch.sh`, and runs
the retail-data guard before it writes the tarball.

## License

- The owner's original work in this repository is under the **MIT License**
  ([LICENSE](LICENSE)). That license covers only this work.
- The upstream decompilation
  [Druthulu/BFM-decomp](https://github.com/Druthulu/BFM-decomp) is credited
  as the reference this project was checked against. Its symbol and type
  names are used in `config/`, with credit. None of its code is included (see
  [CREDITS.md](CREDITS.md)).
- Third-party libraries and tools keep their own licenses (see
  [CREDITS.md](CREDITS.md)).
- *Brave Fencer Musashi* is © Square Enix (originally Square Co., Ltd.). This
  project is not affiliated with or endorsed by Square Enix or Sony
  Interactive Entertainment. See [NOTICE.md](NOTICE.md).
