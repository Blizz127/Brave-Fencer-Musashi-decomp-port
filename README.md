# brave-fencer-musashi-decomp-port

An experimental PC port of *Brave Fencer Musashi* for the USA PlayStation disc
(SLUS-00726). The player reads your own disc. Its hybrid runtime runs part of
the game as compiled host code and uses a CPU interpreter for the rest.

With the native lane active, the game can set up its first stack frame and
call the constructor on the host. A cold-start test follows the original CRT
call through all eight game instructions. It checks the saved registers and
the 20 elapsed source cycles at constructor entry. The constructor still
runs on the guest CPU.

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

The November 22 source update covers 284 regional function matches across seven scene overlays,
totaling 36,420 bytes. Four mine routines scan floor probes and pass their position to the map lookup.
The mine collision code intersects the probe with a face and tests its side planes.
A mine actor scheduler updates eligible actors and copies their positions to attached objects.
Two earlier passes select actors for updates. A separate routine counts down a signed timer and adjusts the actor’s scale.
A separate mine routine saves the positions of 20 slots and updates each active slot’s target after its handler returns.
The mine model classifier selects translated, rotated or scaled contact coordinates.
In the opening scene, one
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

Three of the frame update’s object services select a floor probe, check
nearby contacts and find an action target. Seven routines have the same
compiled body in the mine and MAIN member 0012. Together they cover both
floor probes, the 96-entry object pool and the signed distance comparison.
The mine pickup handler marks the first item touched and dispatches its effect.
A host test exercises the scanner and handler with synthetic item values and callbacks.
The two resident distance routines build the coordinate differences, call the
math services and return the result. The hybrid player still uses guest code
for the nested calls. Run the host pointer and distance checks with
`python3 -m unittest discover -s tests -p 'test_scene_object_distance.py'`
and `python3 -m unittest discover -s tests -p 'test_scene_target_pointer.py'`.

The mine’s model surface query extends a vertical probe by 768 coordinate
units. Tagged geometry selects a linked list of collision boxes. Untagged
geometry selects the floor grid. A successful query transforms the contact
into the caller’s point. The source test follows both floor probes through
nine game routines with synthetic geometry. It supplies the
classifier, box and plane tests, coordinate transform and address mapping.
Run it with `python3 -m unittest discover -s tests -p test_mine_model_surface.py`.

The mine box routines intersect the moving segment with its three pairs of faces.
Host tests cover face selection, signed coordinates, overlapping contact outputs
and the partial write on an X-face division by zero. Run them with
`python3 -m unittest discover -s tests -p test_mine_box_faces.py`.

The model contact transform also matches the mine member. It adds the
model’s three coordinates to the selected point and its auxiliary point.
When rotation is requested, it calls the two matrix services before
translating the points. A host test exercises coordinate wraparound and the
contact reference changing between the two stages. Run it with
`python3 -m unittest discover -s tests -p test_mine_contact_transform.py`.

The mine frame update also advances a signed-value, byte-duration script.
It writes both outputs before it reads the duration from the next byte.

The mine dialog code scales three color channels for the selected backdrop band.
The five selector values cover a preset, two record palettes and two flat colors.

The mine target indicator builds ten colored quadrilaterals in three groups.
The game supplies its vertex tables and submits each primitive through the drawing interface.

The mine animation scanner finds the last frame by reading signed operations from eight-byte records.
Host tests cover every operation code and truncated frame lists.

The mine actor animation tick advances or reverses each eligible actor through its frames.
The host path checks the frame list again after the reverse scan.

The mine animation command routine applies frame values and delays and follows nested sequence commands.
Its portable version preserves return positions and rereads the delay after writing the target.

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

A second segment routine tests the starting cell, then tries the ending cell
when the segment crosses a 128-unit cell boundary and the first search misses.
It writes the projected contact and material to the endpoint on a hit. During
ordinary original play in room 3008, the starting-cell call was observed twice.
The forest floor test covers first hits, retries and overlapping output. The
player continues to execute this routine on the guest CPU.

The forest collision routine selects a linked box or a grid. A moving
endpoint can trigger up to six grid queries. After a hit it transforms the
contact back to the endpoint. The host tests resolve each box or grid through
a 32-bit game address, including a box link changed by a previous contact
and a grid mapping withdrawn between retries. The standard player executes
this routine on the guest CPU.

The grid query computes the movement vector, visits short and long faces in
cell order, and records the selected material. The standard player executes
the query on the guest CPU.

Two forest controllers use the grid query. One retries with the destination
cell after a miss across a 128-unit boundary. The other tests an endpoint
sweep and classifies the contact slope. Its distance mode calls the original
square-root service. Both controllers execute on the guest CPU in the standard
player.

A matching actor routine gathers the current coordinates, tests the segment
from the previous position and writes the corrected coordinates on a hit.
It classifies the slope using the contact normal. Its source is shared with
MAIN/0012. A no-disc test checks both signed slope thresholds and confirms
that the actor keeps its original coordinates on a miss. The standard player
uses the guest routine.

In the forest, the ground check scans up to 96 actor slots for object surfaces,
queries the room floor, then stores the selected ground point and normal.
Three of the four forest matches for this check reuse existing MAIN/0012 C.
The fourth recovers the forest model probe, including its calls to the box
and grid tests. A recording of ordinary forest play reached the room-floor
query twice. Separate offline tests compared this check with the original.
Another ordinary forest run recorded a successful return from the model-grid
query through its starting-cell caller. The query left the near point unchanged
and moved the far point from Y −248 to −258. A separate offline run using
the same inputs and an earlier capture of the room reproduced these values
with the compiled forest code.
The standard player still executes these routines on the guest CPU.

The forest player update selects its state and phase through two game tables.
One phase runs four shared updates and a handler for the current actor kind.
It then updates a flag using the kind, action and mode left by those calls.
A comparison using 72 offline inputs matched 38 flag sets, 19 flag clears
and two alternate returns with the original. The 19 clears used a controlled
kind-11 actor. Thirteen inputs reached a callback outside the comparison.
That group includes the player from a captured forest scene, which reached
the action handler at 80159C84. The no-disc test also checks callback order
and changes made by handlers. The standard player executes this path on the
guest CPU.

The forest action handler at 80159C84 checks for player transitions and advances
animation steps and counters. Its complete 1,348-byte function and both generated
switch tables match the original. A separate comparison covered every instruction
in the function with controlled callback results. In an offline replay using a
captured forest player, the comparison reached a collision routine that uses the
guest scratchpad stack. Changing one actor field in that capture produced a
transition from kind 0, action 1 to kind 2, action 0 in both versions before
the same boundary. The standard player executes this action on the guest CPU.

Two forest collision functions scan up to 96 actor slots twice. A contact can
push the actor sideways or snap its coordinates to a surface. The actor's
flags disabled the scan in a captured forest state. Offline tests using that
room's geometry and constructed inputs reached six hits in 440 original
contact calls. The compiled callers reproduced the coordinates when supplied
with the recorded contact results. The standard player runs both routines on
the guest CPU.

Another actor routine copies its coordinates into the endpoint before calling
the model-grid sweep in mode one. A hit writes the corrected coordinates
back to the actor and returns the sweep flags. The standard player runs the
guest routine.

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

Two forest actor searches scan 96 slots for collision flags and store the first
accepted actor's four-byte game address. One also copies an additional result
field. The host test exercises slot order, field overlap and the last slot.
Run `python3 -m pytest -q tests/test_forest_actor_contact.py`.

The player runs these searches on the guest CPU in the supported forest scene.
A timed test made slot 95 eligible during the loop. An optional whole-function
provider skipped that collision. The generator now holds both searches out of
custom overlay tables, including the matching MAIN/0012 provider. A custom
MAIN/0012 run can stop at the search entry where the public guest decoder
does not cover that member.

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

Eight sound routines in `provenance/sound-events.json` share the four-byte event
and 84-byte channel layouts. A comparison against the USA executable exercised
640 generated inputs and all 256 event selectors. Channel state, return values
and the 188 calls to the test voice backend matched. The slot search can see a
channel change partway through its eight records. The player keeps the search,
event handler and dispatcher on the guest CPU to preserve that timing. Run
`python3 -m pytest tests/test_sound_events.py` for the disc-free host checks.

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
