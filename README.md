# Brave Fencer Musashi PC port

This project reimplements *Brave Fencer Musashi* for the US PlayStation release (SLUS-00726). Supply your own disc image. Keep the CUE file and its referenced BIN tracks together with their original filenames.

## Run the desktop preview

Download the Windows x64 ZIP or Linux x86_64 archive from [GitHub Releases](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/releases/latest). Extract the whole archive and follow its README to select your US disc. The bundled Linux SDL2 library requires glibc 2.38 or newer. The executable also accepts `--disc "path/to/game.cue"`. Append `--windowed` to run in a window.

Press Enter or the controller’s Start button at **Push Start**. Select **Start** to enter the opening scene. The preview reads its title and opening artwork from the disc at runtime and runs the matching title-menu C function on the host CPU. New Game shows the original loading sprite, the opening moon, the publisher credit and the animated game title. The camera pans down to Allucaneet Palace, shows its location caption, then moves toward the gates. The exterior fades to black and the first interior staircase comes into view. Masonry falls as Steward Ribson and his companions descend the stairs. Ribson’s portrait and two dialogue pages appear from resources on your disc. The second page advances automatically. The player decodes palace voices from your disc and sends them to the selected audio output. Butler Livers and Princess Fillet can speak when playback reaches their cues.

Without a skip, the court gathers around the crystal. Fillet finishes her spoken question and Alchemist Leanman answers. Scribe Shanky then speaks, Leanman gives Fillet an instruction, and Fillet begins the ritual. The view centers on the crystal as the room darkens. Green rays and pulses move through the chamber. After the first white flash, crossing hoops widen around the crystal and the room fills with light again. As the light fades, Butler Livers announces the arrival of Musashi. The camera reveals Musashi asleep on the dais. After Scribe Shanky reacts, Butler and Fillet speak, the court calls out together, and Musashi wakes and answers. He turns in the air and lands facing the camera. At the doorway he turns to address the court. Fillet answers, Ribson intervenes, and Musashi folds his arms and tells the court his name. Ribson apologizes and asks for Musashi’s help. A courtier questions whether the task is too much for him, and Ribson defends the princess’s choice. Musashi asks why the court brought him there. Scribe Shanky walks around him while explaining the summoning and the danger from losing Bincho Energy. Musashi objects, and the view returns to the group. The chamber shakes. The court warns him about the search for Lumina and gives him a sword called Fusion. Musashi holds Fusion up as the court explains its Assimilate ability. He lowers the blade, turns to look at the sheath and asks about it. Fillet directs him to Spiral Tower to find Lumina. Musashi runs out of the chamber. The court finishes speaking, the doorway fades, and the Chapter 1 card appears. After the card fades, Musashi enters Giant Forest.

The rising ritual tone, the first flash and the smaller sound that follows use instruments from your disc. Music enters at the second flash. The player reads the score and instrument bank from your disc and plays the passage in stereo as the court explains its request. Reverb and the original mixer balance are still in progress. Giant Forest’s music begins during the black screen at the end of the Chapter 1 card. It starts slightly earlier if you skip from the exterior than if you leave from the court. The score plays under Musashi’s two lines and continues as you move. The introductory note plays once on entering the forest. When the phrase ends, the music returns to the first chord. This return was observed twice during an uninterrupted run of the original US game. The conversation continues even when the system has no audio output.

To reach Chapter 1 sooner, press Space, X or controller A during the opening. On either route, Musashi speaks two lines as he enters Giant Forest. When the two lines finish, the status panels slide into view. Use the arrow keys, D-pad or left stick to move across the entrance, slide along the angled doorway wall and climb the first ramp. At the raised obstacle on the landing, Musashi can slide along the slanted wall to the adjoining face. Press Space, X or controller A to jump. From that corner, hold Down and Right as you jump to land on top of the obstacle. Releasing the jump button early shortens the rise. Reverse direction to run back down the ramp, then release the controls to coast to a stop. Walking off the far rim reaches the preview boundary. The drop toward the creek, combat and further travel are still in progress. The introductory movie and the music before the summoning are still unavailable.

Continue can display the four files on your own raw 128 KiB memory card. Add `--card "/path/to/card.mcd"` when launching the executable. Use the arrows or D-pad to choose a file, confirm with Space or controller A, and back out with T or controller Y. The player reads the card without writing to it. An early-story File 1 can resume in Musashi’s castle bedroom. Confirm **Loaded data!** to watch him wake, step off the bed and look around. His first spoken line plays from your disc as the words appear in his balloon. He approaches the doorway and steps back as Steward Ribson and Butler Livers enter. Ribson greets him. The camera cuts to Musashi for his reply, then to Butler Livers as he introduces himself. The preview ends when the butler’s voiced balloon clears. Press Start to return to the title. Other saved scenes and writing progress are still unavailable.

To build the preview, install CMake 3.16 or newer and the SDL2 development files (2.0.18 or newer). From the repository root, run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target musashi_native_boot
```

On a single-configuration Linux build, the player is `build/preview/musashi_native_boot`.

## Earlier hybrid runtime

The earlier player ran part of the game as compiled host code and used a CPU interpreter for the remaining paths.

Startup, the first graphics calls, the mounted room reset, controller disconnect and several sound and scene handlers execute instructions supplied by your disc. The guest CPU advances the same device clock used for rendering and sound. An original US trace and a controlled startup run both measured 111 modeled clocks from main entry to the first graphics wrapper. The controlled test exercised the graphics return with supplied child results.

On Linux and on the Windows x64 player under Wine, build `286492fc` loaded File 1 and displayed **Loaded data!**. Confirming the message stopped at `8017C3FC`. The original US game restored Musashi in the Chapter 2 bedroom from the same save. An earlier build, `328f8faa`, reached **Now Loading** before stopping.

For New Game, build `7becc625` drew the complete opening title on Linux and under Wine, then stopped in the sound interrupt before the palace. The later Linux build `1406d60c` still stopped on the moon at `8003B110`. Playable New Game, an inn save and Continue from a fresh process remain unverified on this source line. Native Windows and handheld gameplay also remain unverified.

## Matching decompilation and historical runtime tests

The notes below describe the earlier hybrid runtime and the original US game. The compiled preview and its playable routes are described under **Run the desktop preview** above.

This source tree contains **1,124 matching entries totaling 170,196 bytes** of the US game. Counting each region and start address once gives **1,121 distinct ranges totaling 169,624 bytes**. Three entries in Giant Forest account for 976 bytes of the movement code observed during the ramp climb. They transform a position, commit individual coordinates and apply a contact probe. The latter two reuse C definitions that also match another overlay. Two other matches cover 68 bytes in the Giant Forest landing routine. One increments an actor counter. The other passes three vector components to an actor update. The room registry includes the 3,080-byte room-entry routine in two scene members, their shared scene command and music picker, and the music caller in the MAIN overlay. The 160-byte resident routine that advances the mode after a room request also matches the US disc. The desktop player still executes that routine on the guest CPU. [Room matching record](provenance/scene-room-entry.json) The resident sound mixer recalculates volume and pan for up to eight active sound slots. Its compiled C matches all 484 bytes of the US function. This source match adds no entry to the desktop player’s native dispatch table. [Sound matching record](provenance/sound-events.json)

The 208-byte slot search and 596-byte placement constructor match both US bedroom scene members. The search selects a free slot from a pool of 96 actors. The constructor fills that slot and calls its setup routine. During an ordinary File 1 load, the original returned from the first type-360 setup call into this constructor at `8012C9C4`. The desktop player has yet to demonstrate this return in ordinary play. [Room matching record](provenance/scene-room-entry.json)

For the first type-360 room actor, the C sources match the 156-byte dispatcher at `8017FF74`, its 256-byte initial state at `80180808` and the 36-byte allocation wrapper it calls. All three match in both US scene members. If the work allocation succeeds, the function uses the current phase to choose its setup and advances the actor state. The 548-byte fade routine at `8017FD50` also matches both members. It builds a shaded rectangle and links it between two draw-mode packets in the active ordering table. The 176-byte early-story handler at `80180010` redraws the fade during its countdown. When the game signals readiness, it sets story progress to 10, clears four event flags and advances the actor. The command runner repeats while the signed status is 3. The first command in the room launch sequence installs a callback and yields with status 5. A later 68-byte actor state decrements its timer. When the timer runs out, it sets story progress to 2 and requests room 3002. The matched request function stores the room descriptor’s address for the loader. The loader tries the pending request before the queued one, copies the selected 36-byte descriptor, and uses its tag to choose the room resource. The original US File 1 run reached the furnished bedroom. That hybrid build stopped before displaying the room. The compiled desktop preview now restores the early Chapter 1 bedroom. [Room matching record](provenance/scene-room-entry.json)

The US game's saved-room setup calls a routine that converts relative offsets in textured polygon packets into addresses. The newly reconstructed C matches all 616 bytes of that routine in two scene members. Six further functions on the same File 1 route match existing public C, including allocation of a scene record and clearing event flag 303. Together these seven routines account for 944 distinct bytes in each member. [Saved-room matching record](provenance/scene-room-entry.json)

The saved-room loader calls a scene list reader after two resets. Existing C for the reader and its caller also matches the two SC01 scene members, adding four verified region entries and 624 bytes. The matching check uses the original US disc image.

The field initializer and its event-table helper also match all 808 bytes in each of those scene members. On the File 1 route the initializer sets up the room record, calls the table helper, and returns to the scene command. In mode zero the helper tests five game event flags to choose which tables to install.

The preceding checkpoint added ten Giant Forest routines that set bits in the player status word and ten that read those bits. All 320 bytes match the US overlay. The setters preserve the other bits, and the queries return the masked bit in its original position. The desktop player continues to execute these routines on the guest CPU to preserve memory access timing. [Giant Forest matching record](provenance/scene-objects.json)

The previous three complete routines matched all 996 original bytes. They draw text and sprite slots, request streamed data and advance the loader. The HUD and two loader routines ran on earlier Linux New Game and Continue tests. The standard desktop build still uses the guest CPU for these calls.

The previous checkpoint covered 1,036 functions and 151,408 bytes. Four newly verified functions store the vertical sync result for the current display buffer, test controller and scene flags before a scene callback, flush queued VRAM transfers, and add two full-screen packets to the GPU ordering table. Their 1,020 bytes matched the complete US functions. Earlier ordinary Linux New Game and File 1 runs reached all four entries. The standard desktop build keeps these routines on the guest CPU.

The preceding four rendering routines added 1,376 matching bytes. They build a four-color quad, insert drawing commands into the ordering table, and calculate a fixed-point rotation matrix. The preceding checkpoint matched three controller routines for copying reports, resetting disconnected inputs and updating a frame counter. It also unified the two-port poller’s report type. Its Linux and Windows generators selected 204 of 248 routines for host execution.

**Desktop tests of `506bc389`.** New Game stopped at Loading before the moon on Linux and Windows x64 under Wine. Continue displayed “Loaded data!” on both platforms and stopped before the Chapter 2 bedroom. Both local archives are held because the decoder implementation reproduces a sequence of original game instructions. Physical Windows testing remains outstanding. [Gameplay results, local archive hashes, disc setup and full changelog](docs/DESKTOP-QA-506bc389.md)

**Desktop tests of `2754ada1`.** On Linux and on Windows x64 under Wine, New Game stopped at Loading before the opening moon. Continue read an earned File 1 and displayed Loaded on both platforms, then stopped before the Chapter 2 bedroom. The four runs recorded zero native direction-mapper returns. A focused controller test completed 42 native returns in 54 cases. Both desktop archives remain local test builds. [Four route results, local checksums and complete source history](docs/RELEASE-CANDIDATE-2754ada1.md)

[3104b26b Linux and Windows startup results, four-file Continue test and QA archive checksums](docs/RELEASE-CANDIDATE-3104b26b.md)

The earlier story-progress checkpoint verified two 16-byte routines. A cold File 4 load on private Linux build R137 entered the story writer at startup and the reader later in the session.

**November 23, 2026, 19:02 UTC.** Private build `27f862e0` completed 10 native scene-counter calls on Linux New Game and 11 on cold File 1. Windows x64 under Wine completed 11 and 12 on the same routes. The direct parent, `e8149f66`, credited zero on all four. Both players stopped before the opening moon on New Game and before the Chapter 2 bedroom on Continue.

[27f862e0 platform results, local checksums and full history](docs/RELEASE-CANDIDATE-27f862e0.md)

**November 23, 2026, 19:00 UTC.** In a separate private `3104b26b` Linux New Game test, the player completed 94 native story-progress reads and one write for 855 source clocks, with two timing refusals. All 2 MiB of final game RAM and all 1,188,327 stereo sample frames matched the played `27f862e0` run. The player stopped before the opening moon. This candidate has no published Linux or Windows binary. The historical release audit below documents the earlier downloads.

**November 23, 2026, 18:11 UTC.** Private build `6186e55c` completed 45 native scene-counter calls across four regular startup tests on Linux and Windows x64 under Wine. New Game stopped before the opening moon, and Continue stopped before the Chapter 2 bedroom. In two separate Linux tests, `e8149f66` reached the same stopping points with zero native scene-counter completions. The original US game and served Linux R137 also cold-loaded the same Earth Scroll save at story stage 135, restored all 320 story bytes and responded to Left. A later R123 inn save recorded 175/175 HP.

[6186e55c platform results, checksums and history](docs/RELEASE-CANDIDATE-6186e55c.md) · [e8149f66 platform results, checksums and history](docs/RELEASE-CANDIDATE-e8149f66.md)

**November 23, 2026, 17:36 UTC.** Private build `94294368` stopped before the opening moon on New Game and before the Chapter 2 bedroom on Continue, on both Linux and Windows x64 under Wine. In separate gameplay, the original US game and the private launcher's Linux R137 both cold-loaded R123's Wednesday 175/175 HP File 4 and restored control outside Hotelo. R137 also closed Steamwood valve 4, and R123 earned the Earth Scroll on Twinpeak Mountain.
[Windows startup results, File 4 comparisons and Steamwood gameplay](docs/RELEASE-CANDIDATE-94294368.md#november-23-2026-1736-utc-gameplay-update)

**November 23, 2026, 17:13 UTC.** Two Longevity Berry HP functions bring the
public matching registry from 1,012 to 1,014 entries. Both were already
counted in the private decompilation and both still execute on the guest CPU.
In separate Wednesday playthroughs, the original US game and private Linux
R123 each saved 175/175 HP in File 4 after the Mayor's reward.
[Matching functions and Wednesday save results](docs/RELEASE-CANDIDATE-94294368.md#wednesday-longevity-berry-follow-up)

**November 23, 2026, 17:08 UTC.** Private test build `94294368` completed
13 scene-counter calls in host code on each of two Linux starts. New Game
stopped before the opening moon. Continue read an earned File 1 and
stopped before the Chapter 2 bedroom. Under Wine, New Game was running
and the File 1 test was queued.
The changelog records the 323 commits behind the private build, its local
archive checksums and the guest CPU paths that still ran.
[Scene-counter changelog and platform results](docs/RELEASE-CANDIDATE-94294368.md)

**November 23, 2026, 16:41 UTC.** Linux candidate `e58d95ea` reached “Now Loading” on
New Game and stopped before the opening moon. Continue read an earned File 1,
displayed “Loaded data!” and stopped before the Chapter 2 bedroom. The
original US game reached the moon and the bedroom on those routes.
The Windows build of this candidate is packaged. Its gameplay results are
pending. The previous candidate stopped at both points on Linux and under
Wine. The changelog includes all 322 source commits, both local archive
hashes and instructions for supplying your own US disc.
[Candidate notes](docs/RELEASE-CANDIDATE-e58d95ea.md) · [Previous four startup tests](docs/RELEASE-CANDIDATE-72cdca11.md)

On the separate private Linux build R123, Musashi closed all eight numbered
Steamwood valves during one visit. The roof vented, the story advanced to
stage 130 and Fores thanked Musashi. The original US game completed the
same repair. Both players returned through Fores’s cottage and saved File 4
at Hotelo. The private character had rested before the attempt. A fresh
original game restored stage 130 and player control outside Hotelo. At the
verification cutoff, the R123 cold Continue test was in progress.
[Steamwood gameplay results](docs/RELEASE-CANDIDATE-e58d95ea.md#verification-and-known-issues)

An earlier nine-function checkpoint matched all 772 bytes of the indoor Steamwood
pressure-gauge actions and the second valve’s selector. It brought the scene registry
to 676 functions across the matched overlays. These nine entries document
the decompilation. Native execution of the indoor gauge remains in progress.

The File 1 menu-exit callback and its signed status reader now match the US
disc across all 152 bytes. Two option readers used by that callback return
the same zero-extended byte to both menu and resident callers. The Chapter 2
bedroom still requires a successful cold Continue in the release candidate.

The Steamwood timer checkpoint brought the total to 1,000 matched functions across 13 registries.
Thirteen new Steamwood functions match 1,084 bytes of the US game. These
cover the display for a numbered valve’s short timer, the cleanup and
restart script after it expires, and the selectors for valves 3 through 8.
A controlled comparison covered all 65,536 signed tick values. An ordinary
private Linux playthrough ran out of time after Valve 1, received the restart
dialogue and closed Valve 1 again.

Five further matches cover the script transitions after a numbered valve
and the cleanup at the final progress value. A successful check sets the
player’s result byte to 1. A failed check sets it to 2. The script reader
preserves the action flags, loads the next word and advances the byte
cursor, including its wrap from 255 to zero. The final branch clears a
story halfword and three 56-byte blocks. All 212 original bytes matched.
Across 5,633 controlled calls, the original and host C agreed on the whole
player structure, story writes and external call order. That checkpoint
brought the match count to 1,005. On an earlier private Linux attempt, valve 4 turned green and
its clock reset from 3 to 35 seconds. Valve 5’s gauge opened at 23 seconds
and that attempt ran out of time. A subsequent run closed six valves in order.

Six more C functions brought the verified total to 1,011 across 13 registries.
Inside Steamwood they select a callback from the supplied room data, check a
position, and handle descriptor completion, a periodic sequence and a script
request. The sixth advances the story stage and sets a countdown on the exit
route. All 432 bytes of their compiled game code matched. In 2,597 controlled
comparisons the C agreed with the original routines. The ordinary player
still executes these six through the guest CPU.

A seventh match covers the two timers in a roof-vent actor, bringing the total
to 1,012. All 168 compiled bytes matched the original US room. During the
eight-valve repair, private Linux R123 executed this routine 64 times on the
guest CPU and called its expiry service once. Cold Continue on the repaired
R123 File 4 showed Musashi outside Hotelo at stage 130.

Earlier candidates documented the truncated Linux log path and what happened
with several occupied saves. [Previous File 1 results](docs/RELEASE-CANDIDATE-97a342d8.md) · [Earlier paired tests](docs/RELEASE-CANDIDATE-c0b07365.md)

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
and device-library routines to the program on your disc.

A Linux player built from public revision `ff943a85` showed an earned Chapter 1
save in File 1, read its 768-byte payload, restored the 732-byte game record and
displayed **Loaded data!** After Cross, the interrupt dispatcher stopped at
sound callback `0x8002CE84` with the message still visible. The original US game
showed Musashi asleep in the bedroom after the same load.

New Game on public Linux revision `c5f6e2f0` reached **NOW LOADING** and
exited before the opening moon at interrupt callback `0x8002CFAC`. In a
separate pair of local Linux and Windows x64 builds at `e45d0e034`, New Game
exited at sound-sequence target `0x800338D0`. The Windows test ran under Wine
10. A later Linux and Windows pair at `88de76842` also tested an earned
File 1. Both players displayed **Loaded data!**, accepted Cross and selected
the bedroom (room 3001, entrance 0). Both stopped at the room-object reset,
`0x80129258`, with a black screen before the room became active. The Windows
run used Wine 10. The original US game restored the bedroom from this card.

The room-change routine, the 128-slot reset and the two following scene setup functions have matching C source. On the mounted File 1 path, the guest CPU latches the pending room mode, saves the previous mode and executes the reset services. The setup functions clear the room workspace and initialize records from scene data. Focused tests observed the room-latch return with two mounted overlay configurations. The ordinary File 1 test above stopped before the bedroom appeared.

A Linux player built from the anonymous source archive at `9a45e853` read a
card with four occupied files. After **Continue**, it displayed **Loading
Data** and stopped at `0x8017CE28` before the file picker appeared. All four
files stayed intact.

A Linux player at `88ffbfa6` read File 1, accepted the confirmation and
displayed **Loaded data!** and accepted a third Cross press. It then stopped at
interrupt target `0x8003392C` with the message still on screen. The master
volume had risen one step, from `0x2000` to `0x2222`. The saved room had
not become active.

A separate, unpublished pair of players at `c0b073650` tested a card with one
occupied file. Linux and Windows x64 under Wine 10 both read File 1, accepted
the confirmation, restored all twelve scene values and completed the master
sound fade. Both stopped at interrupt target `0x80033D28` before Musashi
could move. Linux still displayed **NOW LOADING**. Windows showed a black
screen. The original US disc restored Musashi in the bedroom. With an empty
card, both Linux and Windows under Wine accepted **New Game**, then stopped
on black before the opening moon at interrupt target `0x800338D0`. The
original game reached controllable forest room 3008. A cold reload following
a new save and testing on physical Windows hardware remain open.

An earlier hybrid Linux player reached the title after its host callback processed
1,760 strips across 88 movie frames. The first two frames matched the original
strip for strip. That path used guest CPU services for decoding and GPU uploads.
The separate clean-room replacement has been checked against one recorded frame.
Ordinary playback with that replacement is still unverified. The 564-byte output
callback appears in two overlapping disc members and counts once in the
[opening movie registry](provenance/opening-movie.json).

Download the source from GitHub’s **Code** menu and follow the Linux build,
disc verification and launch instructions below. Windows x64 users can
cross-build the player on Linux with Zig and the two pinned development SDKs.
The Windows checks used Wine. Both builds run native code alongside a guest CPU
interpreter and require your own USA disc (SLUS-00726).
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

The scene registry covers 723 regional function matches across 21 overlay members,
totaling 105,680 bytes. The recent Steamwood source includes Fores’s cottage
initializer and six functions that run the outdoor practice gauge. Four more C
functions reproduce 336 bytes of the US game. One records when the emergency
timer starts. The other three select the outdoor door’s current action, start
a delay and finish the transition. Inside the plant, nine more functions match
844 bytes of the numbered-valve code. They select valve 1, check the order,
start the separate Valve Timer and reset progress when time runs out. These
functions use the guest CPU in the public player. Public playtests have yet to reach the cottage or the plant.

In the US game and private Linux R123, Musashi loaded an earned File 4,
received the Manual from Fores and regained movement with the timer running.
The US reference shows the outdoor practice gauge opening the door. In a
staged R123 test Musashi filled the gauge, entered the plant and reached
the first catwalk with 00:04 left. Steamwood exploded when time ran out.
The Memory Box offered recovery at a cost of half his Drans, then reported
that it contained no data. Loading saved data restored day-21 File 4 outside
the inn. In later tests, the original US game and private R123 both showed
valve 1 turn green and Musashi say “Yes!!” The original showed the Valve Timer
counting from 60 to 58 as he moved away. On R123 the valve timer expired,
then the facility clock ran out. Loading File 4 at the inn and paying for a
rest restored his BP. On the next attempt he closed valves 1 and 2. The Valve
Timer then showed 53 with 05:00 left on the facility clock.

Two Twinpeak routines compare an actor’s current and saved coordinates to choose
a contact check. Together they match 268 bytes in SC03 member 0029. On a recorded
US Fusion encounter, the first check returned zero and the second returned
`0x2000`. Musashi then advanced to the next phase. Both routines still execute
on the guest CPU in the public player.

The 56-byte Twinpeak routine that spends BP also matches the US game. On an
ordinary Shrink cast, the original charged 10 BP, taking Musashi from 143 to
133. A separate periodic debit had already taken him from 147 to 143. The
public player continues to execute this regional routine on the guest CPU.

The cast routine and its ability-cost lookup match a further 64 and 24 bytes
in Twinpeak. On the US disc, ability 13 selected a cost of 10 before the
debit. Other rooms can place their cost tables at different addresses. Both
Twinpeak routines still run on the guest CPU in the public player.

On the same US save, a second successful Fusion hit on the Twinpeak Magician
displayed a green 10 and raised BP from 125 to 135. Two regional matches cover
the 60-byte credit and its 76-byte caller. The credit also occurs in the forest
overlay. It writes the 16-bit sum, then writes the maximum if the sum exceeds
it. For a nonzero award the caller applies the credit and requests a second
service. The original trace observed the credit return. The later service and
its return remain unverified. Both routines run on the guest CPU in the public
player.

The cemetery award in SC03 member 0023 stores Jon’s Key, advances story progress
to 60, sets the completion flag and resumes the room script. The field player
executes this callback on the guest CPU. Run `python3 tests/test_cemetery_key.py`
to check the order of the award and its inventory effects. Three earlier callbacks
handle selecting the slab, moving it for 40 updates, and starting the key script
when Musashi reaches it. The opening flag is set on the final movement update.
Run `python3 tests/test_cemetery_opening.py` to exercise those callbacks. They
also execute on the guest CPU in the public player.

Seven Twinpeak actor routines in SC03 member 0029 match 960 bytes from the US disc.
They start action 22, dispatch its phases, check the initial actor status, prepare contact, open its handle,
poll it and process animation events and cleanup. During a Shrink encounter
on the original game, the actor advanced from phase one to two in 88 clocks
and from two to three in 11,695. It waited in phase three for 289 calls and
advanced on the next call. The public player executes these routines on the
guest CPU. The abort path has host-test coverage. Run
`python3 -m pytest -q tests/test_twinpeak_action22.py` to check the normal
and abort paths, service call order, actor fields and the optional queue request.
The 248-byte initial status callback can continue the current action or switch
the actor to action 3 or 4. In one recorded original encounter, it returned
to action 22, phase one, after 20,217 clocks including its service calls. The
recording did not capture the status value. Host tests exercise all three outcomes.

Three more Twinpeak routines match 288 bytes. They find a free tutorial slot,
initialize it and return a handle to the actor. In an original-game Shrink
encounter, the tutorial took the first available slot. Both pages advanced
and Musashi regained control. Host tests cover a full pool, selection of the
second slot and replacement of an active handle. The public player executes
these routines on the guest CPU.

The tutorial defaults routine and its two helpers match another 288 bytes.
On the original game, Musashi’s Shrink tutorial initialized the slot, called a
graphics service with a 56 × 48 rectangle and cleared 648 halfwords. The full
initialization took 9,962 guest clocks, including 3,955 in the external
graphics service and 5,842 in the clear. The public player runs all three
routines on the guest CPU.

Two more Twinpeak routines match 316 bytes. During a Shrink encounter, the
original game combined its actor status queries and returned `0x2000` after
15,955 guest clocks. The following action took 4,218 clocks. Its source
selects either an actor callback or a sequence of event and state changes.
That recording did not identify which branch ran. The host fixture covers
both branches, including callbacks that change the actor. The public player
continues to execute these routines on the guest CPU.

Jon’s stocks initializer selects a dialogue script using story progress, an event flag and a game query. The host test covers seven outcomes, including the high-progress path that keeps the previous selection. The public player excludes this initializer from native dispatch. The callback following the key-use message also has matching C source. It calls the first actor service, updates the resulting actor part and calls the next service. The callback retains guest timing. On an ordinary private playthrough, Musashi used the key, Jon stood free and the four-log objective appeared at story progress 70. A separate actor routine advances the story.

The forest attack routine computes strength from the selected attack record, applies player bonuses and can adjust the result for the actor. The playable build runs this routine on the guest CPU. The forest player frame initializer updates the model, controller, animation and coordinate cache. The playable build runs this handler on the guest CPU. The normal forest player action applies speed limits, selects a movement animation and checks ground contact. The playable build runs this action on the guest CPU. The kind-four forest action selects its update from the active scene, rotates player input by the model yaw and chooses a transition after checking contact. This action also runs on the guest CPU during play. The Steam Knight damage routine subtracts the incoming hit from the actor’s HP and selects its reaction. The introduction turns Musashi toward the boss, checks and closes his request, and restores player control before phase 14. The phase-14 camera callback sets its position and focus, copies its target state, and selects the camera mode. Seven phase-22 functions cover the handover to the player, including movement and input reset. Their ordinary execution still uses the guest CPU. Two later countdowns enter phases 19 and 20. These routines still run on the guest CPU during play. Four Tower routines now cover model reservation and the allocation, lookup and release of room records. The Tower's actor cleanup sets the room flag, clears a model of type one or two if present, and clears the actor record. Gameplay runs this routine on the guest CPU. The joint effect routine requests an actor using the joint index and display flags, and attaches a successful allocation to the linked actor when required. Two matched Tower routines clamp the Steam Knight’s movement and check its route markers. The game still runs them on the guest CPU. Five matched Tower routines select the body’s next scripted action using player position, available slots and the current marker. Gameplay runs these routines on the guest CPU. Two matched Tower routines advance the third body action and tick its duration. Gameplay runs these routines on the guest CPU. Five matching body routines cover direction changes, state-two transitions and movement cue requests. These routines still run through the guest CPU in gameplay. The joint-effect actor reserves a slot, initializes its resources, and disposes of incomplete setups. The body updates its turning and joints, enters its armor phase, and resets the actor substate when it changes states. The body can clear an auxiliary countdown, reset and reattach the auxiliary actor, and release its active effects. The contact routine records the hit type, damage, and position on eligible actors. A surviving attachment can queue its reaction ID in a four-entry list. When the outer armor breaks, it enables the paired inner armor and removes its reaction ID if present. The break particles initialize a model and animation, then update their rotation and check their height after moving. The break also requests six particles and sets the initial position and velocity of each particle it can allocate. The armor initializes its template and model, then updates active and broken parts using their linked body flags. Its tracking routine adjusts the player’s position when an attached armor part moves. The Tower actor allocator finds an empty slot, initializes its record, and applies its template health and model. The core reacts to closed hits and queues accepted damage against the parent according to the current round. Source-only matches now cover the body update, its pending damage and phase changes, and the marker that opens the core. The Tower animation refresh, core model setup and opening sound are also matched. Hit feedback can allocate the first free effect slot from a pool of 30. The feedback caller uses the impact coordinates for nonnegative damage and the player position after cleanup for negative damage. Four mine routines scan floor probes and pass their position to the map lookup.
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
Both entrances to room 304C select the same event handler. It dispatches on the event's phase byte. The first phase calls a room routine, requests mode zero, then advances the phase if both calls return. The playable build still uses the guest CPU for these callbacks.
Two ring routines for actor type 0x33A in room 304C reset its timer and update the ring intensity. The update clamps the signed timer to 0 through 255 after subtracting four on odd frames. It writes both ring control words before reloading and advancing the timer. Both routines run on the guest CPU in gameplay.
Phase six of actor type 0x33A in room 304C selects a ring record from the placement parameter, raises the actor's Z coordinate and requests a ring of radius 128. After the ring calls return, the routine sets the effect flags and advances the phase. Gameplay runs this routine on the guest CPU.
The eleventh placement in room 304C is a type-0x333 actor. Its constructor checks an event flag and requests a model. A set flag or an unavailable model sends the actor to cleanup. On success it binds the model, sets the contact shape and attached vector, and advances the actor phase. Gameplay runs this constructor on the guest CPU.
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

A 60-byte forest routine selects the setup or update callback for an attack effect.
The regular player executes this routine on the guest CPU.

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

### Freeing Guard Lumpwood from the forest Bincho Field

Musashi can break the crystal holding Guard Lumpwood with a Lumina strike.
Sixteen matching C functions cover the crystal's eleven states, the glow and
shards, and the callbacks that award five maximum BP, refill current BP and
mark the Guard as rescued. The functions reproduce 3,704 bytes of the USA
forest overlay. The public player executes the crystal actor through the
guest CPU.

Two private hybrid runs reached this crystal from the inn on File 3. Both
showed the field shatter, the named Guard emerge, and maximum BP rise from
150 to 155, with current BP refilled to 155. In one run the first swing
missed. Musashi turned back toward the intact crystal and broke it on the
second swing. The Guard then appeared as rescued on the roster.

Hawker Steakwood's Bincho Field uses the same reward. Two more matching C
functions (116 bytes) add to maximum BP, cap it at 1,634, then refill current
BP up to that maximum. The public player still executes them on the guest CPU.
In a private hybrid File 4 playthrough, Musashi broke Hawker's crystal with
Lumina. Hawker gave his Minku advice, BP went from 111/155 to 160/160, and
his name appeared in the rescue list back in the village. A recording of the
original game showed the same advice and its maximum BP rose from 155 to 160.

### Guard Lumpwood at the Twinpeak gate

Six matching functions cover the rescued guard at the north end of the village.
They check his rescue flag, initialize his model and interaction callback, and
start a conversation when Musashi is in range. The guard selects one script for
the first conversation and another for later conversations with the same actor.
After the script finishes, he returns to his waiting state.

The C reproduces all 604 bytes of these functions in the USA disc overlay. In
a private hybrid File 3 playthrough, the rescued guard granted passage, stepped
aside and let Musashi enter Twinpeak Mountain during daylight. The public
player still runs the guard through the guest CPU.

### Restaurant’s Macho at the Twinpeak gate

Before Musashi rescues Guard Lumpwood, Restaurant’s Macho can occupy the
same village gate. Twelve matching C functions cover Macho’s seven actor
states, including his walk onto and off the post, conversation range, refusal
dialogue and return to duty. His shift begins at 09:00 and ends at 18:00.
Speaking to him sets a conversation flag and advances his dialogue counter.

In a private hybrid playthrough, Macho refused passage during the day, left
his post at 18:00, and Musashi walked through to Twinpeak Mountain. An
original USA disc run from the same inn save also reached the mountain through
the unstaffed gate at 20:00. These functions reproduce 1,520 consecutive bytes
of the USA village overlay. The public player executes Macho through the guest
CPU.

### Verify the Mayor’s porch routines

All 15 functions in the town’s porch actor module match the USA disc, covering
1,488 bytes. They set up village actors, handle dialogue choices, update quest
flags and deliver three cues to a running actor. The resident flag setter
returns the complete updated flag byte. The module references the dialogue and
event records on the supplied disc.

A separate 92-byte routine in the main village overlay sets a field actor to
state 27 and calls four game services. An original-disc playthrough observed
this transition at 10:00 after the player accepted the missing-dog quest.
The 424-byte update at `801859AC` starts and advances paths and selects the
actor’s next action. The C rebuild also matches both switch tables on disc.
Its 456-byte waypoint advance at `80187A64` turns the actor toward the next
point, applies movement and checks the distance before advancing the path
index. The advance routine uses the console’s 32-bit model and route pointers.
The public player executes both routines on the guest CPU.

An original-disc File 3 playthrough recorded 922 calls to the waypoint
actor’s floor wrapper. The wrapper calls the 232-byte selector at `80134510`,
which uses the village’s active floor map and scratch vectors to adjust the
probe height and classify the slope on contact. The public player runs this
floor query on the guest CPU.

In a separate private hybrid build, an ordinary File 2 playthrough reached the
Mayor, accepted the dog quest, received the Twinpeak Mountain directions and
returned to player control. The porch actor’s entry routine ran once as compiled
host code and made six calls back into the game. The public player has yet to
reach this encounter in an observed run. This checkpoint adds matching source.

After extracting SC03 as described above and generating the executable identity,
run both checks for each registry. The porch registry contains 15 functions and
the resident event-flag registry contains one.

```sh
python3 tools/verify_registry.py --registry provenance/village-porch.json --jobs 1
python3 tools/check_no_retail_data.py --registry provenance/village-porch.json --jobs 1
python3 tools/verify_registry.py --registry provenance/resident-event-flags.json --jobs 1
python3 tools/check_no_retail_data.py --registry provenance/resident-event-flags.json --jobs 1
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
`provenance/native-main.json`. It records 288 match entries totaling
37,036 bytes. The registry includes three motion routines used by the
no-disc smoke test. The existing native-lane allowlist and compiler checks
decide which routines the player can run as host code. Other paths use the
interpreter. See [EXCLUDED.md](EXCLUDED.md) for the source inclusion criteria.

The actor work-record reset at `8001C9D0` writes 38 of the record's 132
bytes and leaves 200 in the return register. Its 16 stores take effect at
different times. The general player uses guest execution for this routine
so a device event can observe a partially initialized record.

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

The 156-byte disc audio callback in `provenance/game-track-rows.json` counts
the first ten playback notifications. On later absolute-time reports, a
packed-decimal track number different from the requested track moves playback
from phase 12 to phase 13. The public player runs this callback on the guest CPU.

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
