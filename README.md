# brave-fencer-musashi-decomp-port

An experimental PC port of *Brave Fencer Musashi* for the USA PlayStation disc
(SLUS-00726). The player reads your own disc. Its hybrid runtime runs part of
the game as compiled host code and uses a CPU interpreter for the rest.

With the native lane active, the game can set up its first stack frame and
call the constructor on the host. A cold-start test follows the original CRT
call through all eight game instructions. It checks the saved registers and
the 20 elapsed source cycles at constructor entry. The constructor runs on
the guest CPU. After its observed return, the game sets up two pointers and
calls the startup exchange in seven more host instructions. Both original
constructor paths reached that call in 14 cycles from the return. The startup
exchange itself still runs on the guest CPU. Nine compiled game instructions
then store the startup setting and enter the first graphics library call.
After the graphics wrapper returns on its original stack, three host
instructions clear the argument and call the CD library. The original game
took six cycles between that return and CD entry. A cold caller test checks
the same interval with test providers for the graphics and subsystem calls.

## Player downloads and source builds

**November 22, 2026. Player downloads are under review.**
Historical player binaries also contain routines removed during the SDK
source review. This correction is available as source for development and testing.
We downloaded every historical player package from the public releases and
checked its published size and SHA-256. All 22 distinct Linux and Windows
executables contain a 36-byte game dispatch table. Nineteen also contain a
32-byte warp callback. Fourteen Linux executables include two complete original
diagnostic strings in their debug sections. The review covered 15 package attachments across 12 release tags, including
14 distinct archives. All 22 distinct players also contain nine original scene
address sequences in the start or end fields of their execution-range tables.
This source revision removes the three affected tables. An affected scene can
stop when it reaches an address with no remaining execution path.

The public source now reads the dispatch table from your disc. Its developer
menu has an empty warp list. A Linux player built at commit `7fd45d2` was
checked for both tables and both strings. The source also delegates interrupt
and device-library routines to the program on your disc. In a cold-start test from commit `c532cb8`, the Linux player read the disc
and stopped at `8002D08C` before it displayed the menu. A replacement
download still needs to reach ordinary gameplay, save at an inn, and
successfully Continue after restarting.

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

The November 22 source update covers 557 regional function matches across eleven scene overlays,
totaling 86,932 bytes. The forest attack routine computes strength from the selected attack record, applies player bonuses and can adjust the result for the actor. The playable build runs this routine on the guest CPU. The forest player frame initializer updates the model, controller, animation and coordinate cache. The playable build runs this handler on the guest CPU. The normal forest player action applies speed limits, selects a movement animation and checks ground contact. The playable build runs this action on the guest CPU. The kind-four forest action selects its update from the active scene, rotates player input by the model yaw and chooses a transition after checking contact. This action also runs on the guest CPU during play. The Steam Knight damage routine subtracts the incoming hit from the actor’s HP and selects its reaction. The introduction turns Musashi toward the boss, checks and closes his request, and restores player control before phase 14. The phase-14 camera callback sets its position and focus, copies its target state, and selects the camera mode. Seven phase-22 functions cover the handover to the player, including movement and input reset. Their ordinary execution still uses the guest CPU. Two later countdowns enter phases 19 and 20. These routines still run on the guest CPU during play. Four Tower routines now cover model reservation and the allocation, lookup and release of room records. The Tower's actor cleanup sets the room flag, clears a model of type one or two if present, and clears the actor record. Gameplay runs this routine on the guest CPU. The joint effect routine requests an actor using the joint index and display flags, and attaches a successful allocation to the linked actor when required. Two matched Tower routines clamp the Steam Knight’s movement and check its route markers. The game still runs them on the guest CPU. Five matched Tower routines select the body’s next scripted action using player position, available slots and the current marker. Gameplay runs these routines on the guest CPU. Two matched Tower routines advance the third body action and tick its duration. Gameplay runs these routines on the guest CPU. Five matching body routines cover direction changes, state-two transitions and movement cue requests. These routines still run through the guest CPU in gameplay. The joint-effect actor reserves a slot, initializes its resources, and disposes of incomplete setups. The body updates its turning and joints, enters its armor phase, and resets the actor substate when it changes states. The body can clear an auxiliary countdown, reset and reattach the auxiliary actor, and release its active effects. The contact routine records the hit type, damage, and position on eligible actors. A surviving attachment can queue its reaction ID in a four-entry list. When the outer armor breaks, it enables the paired inner armor and removes its reaction ID if present. The break particles initialize a model and animation, then update their rotation and check their height after moving. The break also requests six particles and sets the initial position and velocity of each particle it can allocate. The armor initializes its template and model, then updates active and broken parts using their linked body flags. Its tracking routine adjusts the player’s position when an attached armor part moves. The Tower actor allocator finds an empty slot, initializes its record, and applies its template health and model. The core reacts to closed hits and queues accepted damage against the parent according to the current round. Source-only matches now cover the body update, its pending damage and phase changes, and the marker that opens the core. The Tower animation refresh, core model setup and opening sound are also matched. Hit feedback can allocate the first free effect slot from a pool of 30. The feedback caller uses the impact coordinates for nonnegative damage and the player position after cleanup for negative damage. Four mine routines scan floor probes and pass their position to the map lookup.
The mine collision code intersects the probe with a face and tests its side planes.
A mine actor dispatches its setup or update callback according to its current state. Its setup creates the parent model or links a child to its collision box. Its update controls model visibility, damps the angle, and checks up to five segments across the box. The parent rotor changes its model angle and sound volume as the player approaches.
A mine rotor runs its state action and synchronizes its model. Its initializer allocates the primary model and optional accessory, then issues four child spawn requests.
The forest camera sets the rotation and translation matrices before transforming a short vector. A separate forest routine submits rectangular VRAM copies from a terminated list.
The mine actor pool initializes a slot from its placement, assigns its owner and runs its setup handler. A rejected setup releases the slot and its model.
The mine room selector looks up a descriptor in the room table supplied by the loaded member.
Four mine stat-gain displays can be armed by signed input and move through entering, holding and exiting phases.
The mine player controller also handles stat-award events that can restore HP and BP and request up to four gain displays.
The recovered player setup installs the award controller and selects its next callback from the current phase.
The eight recovered phases move the player, run the event, animate the award and turn the player back to the saved heading.
The mine model animation reader selects poses from the mounted sequence and handles 27 commands, including loops, flags and model updates.
The scene action queue appends commands at their original widths and retains one current award operand.
Seven player services select a live actor, install a callback, update actor phases and coordinate scripted movement.
The mine field’s fourth setup stage runs its room hook before advancing to the actor reset.
The mine field reset clears 96 actor slots and a 1,024-byte work area before placing the room actors.
The mine door placement handler selects the callback for its phase, searches for a free model and enables contact after successful setup.
One mine camera constrains the horizontal target according to the player’s height. The other can toggle its target depth from the player control flags. Both pass a copy of the anchor to their room-specific camera calculation. These two handlers still run on the guest CPU in gameplay.
Two mine entrances set different camera depths and pitches. Each setup registers its own camera update, copies the initial fields and calls the handler selected by the camera mode. The original entrances select mode zero. Gameplay still runs these four functions on the guest CPU.
The mine player reads the action from the terrain query and applies the corrected position on contact. If the player reaches action 29 before its event flag is set, the routine also saves the surface normal. Gameplay still runs this function on the guest CPU.
The mine checks the terrain faces listed for the player's previous cell in stored order. The face result and the collision state determine when scanning ends and which material is retained. Gameplay still runs this function on the guest CPU.
The mine tests movement from the previous position against the terrain grid and repeats the query after a collision.
The mine checks the movement segment once for odd query modes.
The mine emitter requests a model at startup and releases the actor when the model pool is full. On success it binds the model, clears the sequence index and variant, copies the starting delay into its timer, and advances its state. Gameplay still runs this function on the guest CPU.
When its timer expires, the mine parent actor reads a child subtype and delay from its sequence. It advances the sequence even when its range or distance prevents it from requesting a type-1A9 child. Gameplay still runs this function on the guest CPU.
The mine child selects a model query record with a signed index, synchronizes its actor and tests the selected record. On success it copies two 16-bit fields from that record into the room feedback state. The actor synchronization service remains external. The guest CPU runs this function during gameplay.
The mine child sets its segment endpoints from the width in its model record. It tests each pair of offsets from the record and returns at the first contact. The segment-contact service remains external. Gameplay still runs this query on the guest CPU.
The mine child’s first stage acquires a model, selects its scale and heading from its variant and parent, and sets up its starting position and contact shape. Model, transform and random-number services remain external. The guest CPU runs this stage during gameplay.
The mine emitter scans the room’s 96 actor slots for emitters, uses their projected positions to select its nearest match, and starts or stops a positional sound. Audio remains external. The guest CPU runs this scan during gameplay.
The emitter’s projection wrapper copies its actor coordinates and model heading into the transform operation, restores the room view, and saves depth and horizontal position. A negative projection flag sets horizontal position to –999. Matrix and projection services remain external. The guest CPU runs this wrapper during gameplay.
The mine emitter and its child each select an initialization or update callback using the actor's current stage. Gameplay runs both selectors on the guest CPU.
The mine actor adds acceleration to velocity, advances its position, and checks the offset movement against terrain. It preserves each coordinate fraction and clears vertical velocity when the terrain query returns flag 0x2000 or 0x4000. Gameplay still runs this function on the guest CPU. For even modes, a nonzero first result triggers a vertical query that can update the height and contact flags. Gameplay still runs this function on the guest CPU. It returns the adjusted endpoint, contact flags and surface material to the player. Gameplay still runs this function on the guest CPU.
The mine player dispatches eligible actions through its room-specific table when the pending transition permits them. It clears the action after handling it. Gameplay still runs this dispatcher on the guest CPU.
The 108-byte constructor for the two type-0x15d door placements in room 304C releases the actor when the model pool is full. On success it initializes the model, sets the room-specific contact shape and advances the phase. Gameplay runs this constructor on the guest CPU. The room 304C door handler uses the actor phase to select model setup or the idle callback. Both placements run this handler again during the field actor update.
The third placement in room 304C is a type-0x252 actor. Its handler selects one of six callbacks using the actor’s phase. Gameplay runs this handler on the guest CPU.
The actor’s first lighting phase copies its room light, varies its direction with the actor’s angle, advances the angle and submits light 2. Gameplay runs this phase on the guest CPU.
The fourth placement in room 304C is a type-0x334 actor. Its handler selects a callback by phase. The initial callback clears the actor counter, sets a room flag and advances the phase. Gameplay runs both routines on the guest CPU.
The fifth and sixth placements in room 304C use the type-0x33a phase table. Their first callback requests a model from slots 62 through 124. On success it initializes the model, sets the actor coordinates and advances the phase. When the pool is full it releases the actor. Gameplay runs both routines on the guest CPU.
In phase one, the type-0x33a actor checks its placement parameter. A nonzero value selects phase six. A zero value leaves the actor in phase one. Gameplay runs this check on the guest CPU.
Two mine actor functions apply world velocity and a separate vertical step to fixed-point positions.
The mine reads its initial placement lists in order and scans the 96 actor slots from either end to find space.
The mine player model supplies two local points from six signed components in the current animation frame.
Two terrain helpers evaluate a face plane and test edge clearance against its vertical normal.
The mine clears these displays at startup, formats each stat gain in its room text, and submits its backdrop and glyphs in order.
The mine backdrop updates two records and submits their gradients or scratch primitives.
The mine dispatches its optional state, active four-record queue and timer-dependent current mode each frame.
The mine room setup installs its floor grid and resolves the six section offsets on first use.
The mine camera update selects a camera, updates seven view components and publishes its guest address.
A host test supplies the rendering callbacks and checks the matrix copy.
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

The 644-byte MAIN/0010 room selector chooses a resource row and track, submits
a load when the row is present, and commits a changed group on success.
The C source matches the original game. A host test covers group changes,
track reuse, room overrides, and callbacks that change the selected room.
Run `python3 -m unittest discover -s tests -p test_room_stream_selector.py`.

Two additional routines in shared MAIN/0010 format a decimal value and copy its
glyph pairs into the mine display text. The numeral table comes from the user’s
original game. The source matches 260 original bytes. Run
`python3 -m pytest -q tests/test_main0010_numbers.py` to test padding, field
widths and overlapping glyph buffers.
The hybrid player executes this overlay routine on the guest CPU.

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

A mine screen effect scales its work coordinates and requests drawing operations for six modes.
The host fixture resolves the effect through its 32-bit game address and records the outgoing calls.

A mine actor checks its position against the current view and can step four units toward the focus on either axis.
The original clipping call remains external.

The mine controller checks two independent visibility flags and the active target before updating and drawing paired points.
Host tests cover service order and the target indicator. The scene continues to use the guest CPU for this controller.

The forest attachment controller selects a callback, copies the ground position and updates a model flag.
Host tests cover both callback tables, signed coordinates and overlapping records. The orientation routine reads the ground normal and three game addresses to set rotation and scale.
Its host tests supply angle and matrix service results and check the order of writes. The player still runs this update and its orientation routine on the guest CPU.

The mine updates its three visual actors, checking the current focus again after each callback, then advances two fade slots.
Host tests follow the regional screen effect from the fade dispatcher. Both scheduler routines still use the guest CPU during gameplay.

The opening fade increases both scale factors until the horizontal factor reaches 4096. The closing fade reduces both factors, then clears the slot and its effect address when the signed horizontal factor falls to 1024 or below.
Host tests run both transitions through the regional fade dispatcher and screen effect.

The mine also visits 128 attachment slots. After each active slot runs its type handler, the game copies three position coordinates to the currently attached object. Each write can change the address used by the next coordinate.
The host tests use generated attachment tables and callback responses. The production scene still runs this loop on the guest CPU.

Two mine frame services select callbacks from a record index and a pending function address. The pending service reads its address twice when enabled.
Host tests check the selected values and record argument. The frame still uses the guest CPU for callback execution.

The mine checks 30 actor slots and runs their active type handlers before synchronizing linked positions. Depending on the target kind it also updates wide coordinates, a status bit, or a nested object. The source tests exercise overlapping records and callback changes, while the production scene still executes the original instructions.

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
SC01/0077 byte for byte. The forest velocity integrator adds all three components with 32-bit wraparound and returns the updated movement flags when a component is nonzero. The sixteenth reads a different scene table there. The forest player builds a rotation matrix from the current model and uses it to transform the local velocity through the original matrix services. A scene callback can clear the player’s digital inputs and center both stick axes before it runs.
The player continues to interpret this code. Run the component and native
selection tests with `python3 -m pytest -q tests/test_scene_player_motion.py`.

The forest player selects its ability cleanup handler from the loaded scene. This callback still runs on the guest CPU.

The forest bridge script turns Musashi toward its target heading. Once he faces
the target, the next update advances the script. The standard player still runs
these two routines on the guest CPU.

Four more bridge tutorial routines match the forest scene. They check the
script clock, set Musashi's direction and input, and clear his scene callback
when the sequence finishes. The standard build runs them on the guest CPU.

Four routines previously matched in MAIN/0012 also match 448 bytes in the forest overlay.
They measure distance to the player, find available actor slots, and update a player contact value.
The four forest entries continue to use the guest CPU in the regular player.

The forest actor table routes kind 218 to a diagnostic that sets up a text label and the actor number.
This 100-byte routine uses a forest-specific label. The current player runs it on the guest CPU.

Four forest attack routines match 676 bytes of the North American forest overlay.
They select close attacks, advance the swing timer and request contact and effects.
The playable build continues to run these routines on the guest CPU.

Eleven forest functions initialize and update a placed actor and its attached model.
These source matches cover 1,408 bytes, including the parent state dispatcher,
animation changes and attachment lifetime. They add no native wrappers.

Six existing actor and effect routines also match the forest overlay. They select
active slots, advance animations and fades, and dispatch effects. The six regional
matches cover 1,264 bytes. These registrations add no native wrappers.

Five existing game routines also match forest placement activation in SC01/0077.
They scan the placement list, find a free actor slot, and use the scene predicate
or distance bounds to decide which placements to activate. The player executes
these routines on the guest CPU in this scene.

When a forest bridge log settles, it spawns effect 37. The effect starts a
16-tick countdown. On each update it issues four requests and checks whether
to release its actor. The standard player executes these three routines on the
guest CPU. Run the host component test with
`python3 -m pytest -q tests/test_forest_settle_effect.py`.

The forest tutorial requests the Gunshot soldier when its countdown expires.
The game records the request even if no actor slot is available. The standard
player executes the three matched script routines on the guest CPU. Run the
host component test with
`python3 -m pytest -q tests/test_forest_tutorial_soldier.py`.

The forest Gunshot query copies the scene offset record, obtains attack power,
then passes both to the contact check. The standard hybrid player runs both
matched routines on the guest CPU. Test the host components with
`python3 -m pytest -q tests/test_forest_projectile_contact.py`.

The forest projectile impact uses three matched routines to dispatch the effect,
spawn streaks and an attached sprite, and release the parent when its countdown
expires. The standard hybrid player executes these routines on the guest CPU.
Run the host checks with `python3 -m pytest -q tests/test_forest_impact_effect.py`.

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

The kind-two player update uses all 16 bits of the action index and returns
the selected callback's result. Action zero combines two contact results to
choose a transition. When that check permits the next update, the code caps
a player field at 24, calls four services in order and sets a game flag.
An original forest input recording observed kind two during six frames of
westward movement. The recording did not sample the action index. The
standard player executes these seven routines on the guest CPU.
Run the no-disc test with `python3 -m pytest -q tests/test_forest_kind2_action.py`.

Kind two, action two checks motion and input conditions before selecting another
player action. Its 1,084 bytes of game code and seven-entry switch table match
the forest overlay. Replaying six recorded forest positions with action two
selected reached the same guest collision routine in both implementations.
The recording captured the player kind and phase. Its action index is unknown.
Three related contact routines match existing MAIN game code. In a controlled
replay, a kind-two contact entered kind four, set the next action to one and
wrote the motion components from the signed floor normal. The standard player
executes these four routines on the guest CPU.

The forest player can request an event when either of two actor checks succeeds.
The first checks the actor's flags and the second can query game flag 128.
The request carries the actor's four-byte game address. The allocator searches
slots 8 through 19 and uses the first free one. An offline replay on the original
forest state reached both checks. With controlled input it also reached the
allocator, including the full-pool case. The issuer returns one for an accepted
request even when the pool is full. The standard player uses guest code here.
Run `python3 -m pytest -q tests/test_forest_actor_event.py tests/test_event_pool.py`
to check the linked game logic using synthetic actors and event slots.

The forest player reads animation frames and commands from a word stream. Two
new matches recover its command interpreter and the helper that counts frames
before a script cursor. Two offline replays selected a new animation using the
captured player's original active slot. The original and compiled routines read
the same commands, called the flag-clear service and wrote the same frame
address into the model. The comparison used checked host versions of the
animation setters. The standard player executes both routines on the guest CPU.

Case four of the forest player's action switch can request a type-18 event.
Its 272 bytes match the existing MAIN/0012 game function. In 19 offline cases
using original game code, six returned zero and thirteen reached a sound or
field service where the comparison stopped. Twelve additional cases used modeled
service responses to test the request and allocator. A completed type-18 event
and its sound have yet to be observed in gameplay. The player still executes
this path on the guest CPU. Run
`python3 -m pytest -q tests/test_forest_secondary_event.py` for the portable
tests covering the slot allocator and projected positions at the edge of the screen.

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
`provenance/native-main.json`. It records 265 owner-authored C sources totaling
29,036 matched bytes. The registry includes three motion routines used by the
no-disc smoke test. The existing native-lane allowlist and compiler checks
decide which routines the player can run as host code. Other paths use the
interpreter. See [EXCLUDED.md](EXCLUDED.md) for the source inclusion criteria.

The 72-byte graphics startup wrapper calls three graphics services, clears its
152-byte state and runs subsystem initialization. A host test checks call order
and the state clear. The player continues to run the underlying graphics
library through the guest CPU.

The game disc reader advances requests through 11 states. Its 1,540-byte
state machine matches the original instructions and jump table. A Linux host
test covers seeks, retries, transfers and error recovery with synthetic drive
responses. The ordinary player still executes the routine on the guest CPU.

Two track-row procedures reproduce 349 instructions from the USA executable.
A host test exercises ten-row batches, sound-service errors and buffer bounds.
Run `python3 -m pytest tests/test_game_track_rows.py` for the synthetic checks.
The standalone test supplies the sound-data windows and audio-service results.

The 664-byte track-start routine opens a sound bank and advances its pending
request. Host tests cover bank reuse, failure returns and changes made by sound
services between calls. The player still runs this routine on the guest CPU.

Two game sound-transfer routines match all 52 original instructions. Host tests
cover successful writes, refused transfers and 32-bit cursor wrapping. The
ordinary player continues to execute both game routines on the guest CPU.

The 1,696-byte sector callback matches all 424 instructions and both dispatch
tables. It reads sector headers, advances the current transfer and sends
graphics or audio data to external services. Linux tests cover partial reads,
track selection, address wrapping and transfer errors. The ordinary player
still executes this callback on the guest CPU.

The 532-byte disc directory initializer locates the game files and builds their
archive entries. On the original USA disc it made 521 service calls. A host
test reproduced their arguments, and a separate native disc-reader test matched
all 447 resulting archive entries. The player runs this initializer on the
guest CPU.

The 856-byte sound startup routine initializes the game’s sound records and
calls 14 services. An original-disc run captured each call and return through
the return to the game caller. A Linux host test checks service order and
shared storage using generated inputs. The player uses the guest CPU on this
path. Its host calling interfaces and audible output still need verification.

The 104-byte sound record clear matches the USA executable. It clears one flag
per record and rereads the current group’s count after each store. A standalone
host test covers overlapping count and handle tables in game RAM. The player
keeps this routine on the guest CPU for instruction and interrupt timing.

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
