# Candidate 2754ada1 for Linux and Windows

Updated 2026-11-23 20:53 UTC. Both regular desktop archives were built from `2754ada1c1c57cde3730ce48e79e1b85790c099f`, tree `603abb5748fb05d76c0ce267fc53ab1918d8cadf`. The checksums identify local test packages. This candidate has no published Linux or Windows binary. [Candidate source](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/2754ada1c1c57cde3730ce48e79e1b85790c099f)

## What changed

The game reads separate controller words for newly pressed and held directions. On both player records, verified calls can now return those masks in compiled host code. A nonzero mask takes 41 game clocks. An eligible zero result on a digital or disconnected controller takes 54. Empty analog inputs, calls without a verified caller, and calls that would cross a device event use the guest CPU. First-controller button history was already running in host code on eligible frame polls.

The source used to build the tested archives registered 1,018 matching routines covering 144,984 original bytes. The two resident direction mappers account for 324 and 256 of those bytes. The two subsequent matching registrations accompanying this report are source-only and fall outside both tested archives. The game still uses a guest CPU interpreter for unconverted paths. Startup with the interpreter disabled has yet to reach the title screen. [Controller mapping source](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/2724353efc389a7ea5a81f76a84157d9f43e3e08)

## Downloads and verification

| Platform | Local candidate archive | Bytes | SHA-256 |
|---|---|---:|---|
| Linux x86-64 | `bfm-r106-clean-qa-2754ada1c.tar.gz` | 4,660,447 | `94b345abfc20d78a13104e752872b39b8547a3696cfa453343664ff83450ea8e` |
| Windows x64 | `bfm-windows-x64-2754ada1c1.zip` | 3,897,050 | `de4471d877a611f610f13bc07a010ee4855ebedae0fa19416712c10fdae7b44c` |

The packaged Linux executable has SHA-256 `1f1f5bef0e1fff09f0c3c8c89815cc5f94c9cf7b30ab06efe11401b661196455`. The Windows executable has SHA-256 `fa58312dad4fe92bf183d2d98f3f02670faa07d31f6dc600af78bb773ce9ca62`. Each archive and its expanded members were checked against 902 owned game and firmware references. The checks covered 27 Linux files and 30 Windows files.

Representing numeric C operands as 64-bit words produced a 55-byte sequence with 52 zeros and three ones. It matched five locations in three blocks of decoded game data. The same comparison found it 141 times in 62 files of the previously published parent, `2724353e`. No file in the `2754ada1` source archive or either packaged game executable contained the complete sequence as stored bytes. The OpenAL DLL in all four Windows packages checked had the same hash and contained the sequence once at the same offset. A separate eight-byte match in the compiled controller descriptor came from the previously published routine size and controller-field offset. A 12-byte match came from representing two routine sizes and a loop initializer together. At the 13 matching locations in extracted video streams, the bytes span a CD-sector boundary and are separated by framing bytes on the raw disc. The review cleared the three changed source files for publication.

## Ordinary desktop play

In the owned US game, New Game passed the moon sequence and reached the playable forest. Cold Continue on the same earned File 1 restored control in the Chapter 2 bedroom. Linux tests used the regular native executable on an isolated X11 software display. Windows tests used the installed x64 executable under Wine. Across all four ordinary tests, the new mappers recorded zero host completions.

| Platform and route | What appeared | Compiled controller work |
|---|---|---|
| Linux x86-64 · New Game | Push Start, New Game and Loading. Stopped before the moon. Foreground PC `8004250C`, refused callback `800338D0`. | 575 first-controller analog samples, 0 direction-mapper completions |
| Linux x86-64 · Cold File 1 | File 1, Yes and “Loaded data!” Stopped before the bedroom. Foreground PC `80042524`, refused callback `8003392C`. | 600 first-controller analog samples, 0 direction-mapper completions |
| Windows x64 under Wine · New Game | Push Start, New Game and Loading. Stopped before the moon. Foreground PC `80042560`, refused callback `800338D0`. | 568 first-controller analog samples, 0 direction-mapper completions |
| Windows x64 under Wine · Cold File 1 | Continued from File 1, confirmed, and displayed “Loaded data!” Stopped before the bedroom. Foreground PC `8004256C`, refused callback `8003392C`. | 621 first-controller analog samples, 0 direction-mapper completions |

The two Linux runs each matched the corresponding `395ab47a` run in all 2 MiB of terminal game RAM. New Game also matched all 1,188,327 stereo sample frames and cold Continue matched all 1,106,017. File 1 retained its original card hash. Windows New Game matched the earlier title, menu, loading and last black frames. The stop dialog differed by 108 pixels. The stereo capture between the sampled Loading and stop-dialog frames lasted 189.57 seconds on this Wine setup. Under Wine, the File 1 selection, confirmation, Loaded screen and last frame before the stop dialog matched the prior build. The stop-dialog capture differed by 973,325 pixels. The earned card retained its hash.

The first-controller latch counts processed analog packets. The new-press and held-direction counters record completed mapper returns. A missing activity row gives no count of the remaining guest calls. Controls ran through the regular desktop input paths.

**Earlier desktop comparison.** The original US game opens with the moon after New Game. Linux and Windows under Wine on `395ab47a` both reached Loading, then stopped before the moon. Linux and Windows under Wine both showed File 1, the confirmation and Loaded, then stopped before control returned in the bedroom. Across these four earlier ordinary runs, host code updated 2,363 first-controller analog samples at a cost of 543,490 game clocks. The guest processed the remaining analog work. On a separate four-slot Linux run, Continue stopped before the slot picker. The original opened that same card, selected File 4 and restored the town. The tested cards retained their hashes. These observations belong to the earlier executables.

**Focused checks on this build.** All 54 controlled input-path comparisons matched guest execution after device clocks settled. Host code handled 42 calls. Eight fell back at a device-event boundary and four lacked a verified caller. Four additional empty-analog inputs stayed on the guest. The immediate clocks differed by two ticks before synchronization.

## Separate earned gameplay

On private Linux R123, Musashi completed all eight Steamwood valves, won the Rootrick race, obtained the Earth Scroll and later saved File 4 at Hotelo at stage 135. That save had 14/175 HP. Later in the same session, two inn stays produced a separate 175/175 HP save.

The original US game and private R140 each awarded the Earth Scroll after the normal Rootrick route. On another saved route, the original and private R137 both rescued Knitter Lunchetta, raised maximum BP to 180, opened the four-switch gate and entered Hell’s Valley. All 320 recorded story bytes agreed at stages 150 and 160. During the Skullpion fight both runs dropped the overhead loads. The first comparison showed a full boss health bar after both drops. In a later original-game attempt, a pale boulder hit Skullpion, revealed the cyan core and stunned the boss. Striking the exposed core shortened the boss health bar. The private run has yet to reproduce that hit. The R137 attempt ended at zero player HP. Reloading the last saved File 4 restored its earlier stage 140.

In a separate private R143 Linux playthrough, the player earned the Mayor’s Berry, increased maximum HP to 175, refilled HP and saved File 4 at the inn. This route recorded 12 new-press and 19 held-direction returns in host code over 1,271 game clocks. The public desktop builds have yet to demonstrate these chapters.

## Using your own disc

Use your legally owned US Brave Fencer Musashi disc, serial SLUS-00726. Keep the CUE sheet and each referenced BIN track together. A data-only ISO omits the XA sectors used for audio. The player uses the supplied image at runtime. Retail files, original graphics and audio, console firmware, and saved games are excluded from the packages.

- **Linux** — Extract the full archive. From its launcher directory run `BFM_DISC="/path/to/your/disc.cue" ./launch.sh`.
- **Windows** — Extract the entire ZIP so the executable can find its DLLs. Run `launch.cmd` to choose the CUE file, or set `BFM_DISC` to its full path first.
- **Saved games** — Use a separate copy of your memory card when testing a candidate. Retain the original card and existing installation.

The older R48 Windows and Linux downloads and the R55 Linux download fail the current data-exclusion checks. Two original lookup tables were confirmed in their shipped player executables. These older packages cannot substitute for this candidate. Their existing public history was preserved.

## Device and release status

| Owner device check | Status |
|---|---|
| Default SDL gamepad or Steam Input in menus and gameplay | NOT RUN |
| Borderless fullscreen in Steam Game Mode under Gamescope | NOT RUN |
| On-screen keyboard opens automatically when each text field gains focus | NOT RUN |

Physical Windows hardware and the owner’s handheld remain untested on this build. The public binary gate still requires playable startup, an inn save and cold Continue, and package-data verification.

## Complete source changelog

The following 343 commits are the complete Git ancestry of the source used to build these two archives, including merges and documentation. Chronological commit subjects retain the measurements reported at their time. Current counts and known gameplay limits appear above.

- `20af675510bc` Initial commit
- `9e719c12a8d4` Brave Fencer Musashi native PC port: first public snapshot
- `902eb7ef3229` Add the owner-authored matched C the port compiles natively
- `ec620c78dd5f` README: Milestones and progress section linking the wiki
- `831e5e889827` docs: update r38 setup and Windows status
- `7da1b3c55860` docs: clarify Windows is a separate host-port project
- `3aff00271988` docs: Windows x64 test build available (bfm-r41-3634893fe release, Wine-validated only)
- `377c3860564c` README: put the r47 test prerelease (Linux) at the top
- `ad71556b4450` README: the r47 test prerelease now has the Windows x64 build
- `e0b18220472a` Source update 2026-10-29: 1,340 owner clean-room decompiled functions
- `713295a38606` Merge source update 2026-10-29 (1,340 owner clean-room functions)
- `5cd48bc13842` Source update 2026-10-30: 18 more owner clean-room decompiled functions
- `cc7b14cdb8f2` Merge source update 2026-10-30 (18 owner clean-room functions)
- `45626d1c7386` Port-source update 2026-10-30: host platform layer
- `5fc1cffb5705` Merge port-source update 2026-10-30 (host platform layer)
- `180007f62c90` Match 155 scene-object routines across six USA overlays
- `b2cc54a2abb7` Build and exercise native angle and movement routines from public source
- `6a79e717fe87` Read verified LIST dispatch targets from the supplied disc
- `9a275090dee6` Match the opening player-state callback and direction timer
- `085a74605411` Apply PS1 texture-window masks in the CPU rasterizer
- `016cec09c762` Build and verify the native function library from the public registry
- `b8aa0c3a230f` Carry accepted polygon texture pages into following GPU primitives
- `4d1b91ff4e37` Match and test the timed player event state transition
- `3cf8709f8648` Share active SDL gamepads between gameplay and menus
- `ecd1dcad82a7` Rebuild and exercise the opening camera registration and scene counter
- `08906cd55c2b` Compute triangle orientation on the shared native geometry unit
- `dc57a6d1b45e` Match resident inventory operations and test item acquisition and removal
- `f00fea5f1acc` Accept the one-word GPU no-op used by the inn save menu
- `d45d362e645a` Present the GPU display page independently of the drawing rectangle
- `7fd45d214bc6` Honor borderless desktop fullscreen and preserve the original picture aspect
- `756c2f73f7d3` Verify Linux package provenance and discover discs in local game libraries
- `fd7dc148f285` Rebuild the opening player event through its camera and release states
- `2da109f2ec47` Rebuild the shop wallet and verify debit, cap and overflow behavior
- `d6d99065042c` Replace historical work notes and upstream test excerpts with reproducible public checks
- `350cb3d61570` Recover the native wallet digit formatter and both HUD packet routines
- `960bb68aef3e` Recover the room facing selector and its signed triangle tests
- `76eedac6970b` Check script-variable bus ownership before native loads and stores
- `88dee3329728` Recover the two title menu counters and verify their wraparound
- `29f2d55c2d8a` Build the Windows player and verify multi-track paths on both hosts
- `a0f1771f7332` Recover the bakery purchase message lookup and selection
- `6d39578f22e0` Verify the card-menu item function and defer native execution at interrupt boundaries
- `a06f06c977ff` Declare the streaming executable digest before Linux boot uses it
- `35de4f634261` Keep memory-card writes bound to the owning save session
- `7688df2ae0d8` Check repeated reference spans and decoded source literals before packaging
- `92b608720efb` Match the three forest bridge trunks through their fall and settling
- `9eeb860d5dc8` Link current Linux and Windows source builds from the download section
- `0a010dbc7776` Use exact tick offsets when the source already runs at the device clock rate
- `a4389bcef57e` Run device routines from the player disc and restore interrupt-driven card I/O
- `c036e819d35a` Cache fixed resident address ranges while fetching live instruction words
- `ed04764682fe` Match the camera update and defer facing routines that require the guest bus
- `450cf6d14bfe` Resolve Linux player libraries relative to the executable after relocation
- `d9ce63b7bfdf` Run the matched wrapped-angle subtraction at each of its three guest boundaries
- `6ccdc19c2b0c` Match room camera initialization and callback selection with one shared actor
- `d3f07b0654ff` Match sixteen field movement and controller routines in their owning scene member
- `4a1755f41278` Record per-player host library and ABI requirements in Linux packages
- `6bd1aa698358` Find Windows discs in literal package and profile paths and preserve valid selections
- `ecc861401fed` Decode GPU drawing coordinates and reject oversized triangles before clipping
- `01cd11072cbf` Recover the forest floor query, cell walk, face selection and plane distance
- `3cdb3a00ddd0` Match measured GPU polygon edge coverage and rectangle size fields
- `5d7b22e564cb` Match the forest contact search and the plane intersection input to the normalizer
- `aae402146231` Match the forest contact transform and keep it out of unchecked native dispatch
- `b5f4910ba34f` Match scene countdown and fade transitions and guard their overlay selection
- `ee5c3b2daf36` Match the five-way forest contact dispatcher and its classifier
- `89f3831aa61b` Match the paired phase dispatch for the return side of a scene fade
- `d5ce271fbbec` Match forest linked-box collision and all three axis tests
- `c8e6bdaaa009` Recover the room draw dispatcher and preserve its three guest arguments
- `ab40672a33d1` Match the sound voice release routine and model callback ordering
- `b19a6a3b73ec` Match room 3050 frame dispatch and honor its scene-specific state
- `aa328be4aaac` Check card VBlank selection, cancellation and batched completions
- `e69cd3244a5c` Match mine contact probes and correct the three-coordinate distance interface
- `17cc4a9628e0` Match four mine floor probes and preserve their host pointers
- `b0f270aa0be7` Re-derive mine model surface and floor map queries
- `494aa466cb74` Preserve actor handles across the two forest collision searches
- `0f5bd9966e15` Initialize interrupt auto-ack words at the selected game entry
- `924361dcd1b4` Match the moving forest segment collision query
- `7d1ff0a0e45a` Match mine floor face intersection and dot product
- `59c17fab62e6` Match linked sound events and preserve channel search timing
- `83e6221748d2` Match the mine model contact transform
- `ff9ab165d575` Read the first matching card directory entry through game callbacks
- `545a7b29aff3` Match the forest slope classifier against its shared source
- `2738c4641231` Match the mine model surface classifier
- `e34513256463` Match the forest model collision query
- `3f2c14a498a0` Match the four mine box contact routines
- `a6f7428c66fb` Read optional card transfer input as binary on Windows
- `3dd9420e9bc9` Match the forest model grid traversal
- `6567fec85626` Match the mine per-frame duration script
- `e4b1060ea47b` Read and close the authenticated memory card metadata record
- `d01b3e75c130` Match the twenty mine slot position updates
- `40dab98cdc44` Match the mine actor position scheduler
- `7026224cba63` Honor the game sound fade threshold inside an interrupt
- `e8b355162a62` Match forest movement and two-cell ground queries
- `1f3a0400b72e` Use the checked mine memory interface on 64-bit MIPS hosts
- `f43a93c0ce17` Match the forest actor movement query
- `a2349a39e6ea` Return through the live GPU DMA interrupt callback
- `27a3142feb57` Execute the startup exchange on the full player clock
- `4aee641a6320` Match the mine actor scale and update prepasses
- `9370f452ad3e` Match the mine dialog band color update
- `f347962407ca` Match the conditional mine pickup handler
- `7d22b2456300` Match the forest floor query and its contact result
- `d40671f9144b` Match the mine target indicator draw list
- `2b2dcc5e8897` Resolve forest model and grid addresses on host builds
- `5e050a0b962f` Match the forest player update and action flag dispatch
- `83ad88296ef4` Report the startup exchange under its actual execution context
- `4f92b5d0ecb5` Match the mine animation command scan
- `0fe255435977` Match the per-actor mine animation update
- `e72cb44a020a` Match the forest player action selected by the dispatcher
- `75ece15bfb06` Run the first game main call on the active native lane
- `e746e0316d60` Match the two forest actor collision tests
- `6b518b285e28` Match the mine animation command dispatcher
- `b98812691f32` Match seven forest player action and ground query routines
- `60122b83dd19` Initialize the registered game main test provider by field
- `61e381cfb6ae` Preserve the guest actor handle in the forest event callback
- `21f0c117db25` Match the mine screen effect with a four-byte guest handle
- `67f0ebf1416d` Follow the mounted save file through its first metadata read
- `6d1716eb7789` Match the mine actor step toward the focus point
- `8fad6747f12b` Match the forest player phase and three transition handlers
- `08bb71cd4e33` Keep two actor counters on the guest CPU until timing matches
- `576afe1df3c3` Run seven game main instructions after the original constructor returns
- `92d60b4846a7` Test two GPU DMA completions and their interrupt acknowledgments
- `c532cb8adfdb` Withdraw SDK-derived routines and direct affected methods to guest execution
- `e6197eaa154c` Match the forest actor event checks and shared event allocator
- `d57040865719` Match the mine controller target update from its observed frame service
- `1cbbfba6b566` Run the startup graphics caller through its first library call
- `078954b24aee` Keep the forest flag read interruptible at its original instruction boundaries
- `7b84c4bfdde5` Match the mine actor update and its two fade requests
- `f2305668002a` Report the clock actually driving the device epoch
- `6a7f0b8258aa` Match the forest animation command reader and its selection routine
- `14e503d4c558` Match the mine fade progression and its completion conditions
- `c070e41a4d14` Match the forest secondary event and its positional sound call
- `7136ad50e65f` Match the mine attachment list and its per-slot position updates
- `1649350f9e0c` Remove scene address tables from the public interpreter
- `6ad71f2ff297` Use the project include directory when testing copied formatter sources
- `fa983b2ad61b` Always report the final GPU DMA counters on shutdown
- `4b64e33f1d14` Reconstruct three forest actor ground attachment routines
- `83aa3f0e8a9e` Reconstruct two mine frame callback dispatchers
- `cc5688bbf1fb` Match the game graphics startup wrapper
- `359bd354518a` Match the mine frame actor link update
- `ad60d6d1db77` Run the game caller through the CD library call after graphics returns
- `e1c5e33b9f92` Resolve forest ground orientation links in the host source
- `a6da90874424` Match the mine camera update and its live owner selection
- `496030f70953` Match the game sound startup sequence
- `51152e5d8684` Match the sound record clear and preserve its per-record reads
- `357016e4d055` Match the game disc directory against original startup and archive entries
- `fcfe61453259` Match the eleven-state game disc reader
- `e61d1e64cc26` Match the mine collision actor state dispatch
- `b61b26a56294` Preserve the full word returned by the forest flag reader
- `4f3066843083` Match the mine collision actor initializer
- `dc4c865f3279` Match the game sector callback and its two dispatch tables
- `37e7440c6f6b` Match the mine actor visibility, motion, and collision sweeps
- `7d713ff80584` Match mine rotor creation and state dispatch
- `9c12c3cc211d` Match the mine rotor update and sound selection
- `d0010efa680d` Match the Steam Knight attachment damage callback
- `8f389c5b853b` Model signed forest velocity updates with defined 32-bit wrapping
- `7bfa7b947a62` Match game sound-stream submission and cancellation
- `3a23533a27e6` Match the forest camera vector projection wrapper
- `558683a151e8` Match the Steam Knight contact that records incoming damage
- `d58661472a78` Match the mine actor pool slot initializer
- `24f8b77fb3c8` Match five forest actor activation functions
- `51eab8f8f88e` Match the tower boss damage reaction and verify its callers
- `538869590a9c` Match the SC02/0039 mine room selector with its MAIN/0012 counterpart
- `1ceee70d4f7f` Match the forest player model velocity transform
- `b1231ba260e0` Match the forest image move list to its verified sibling
- `725299abb0ef` Match the forest bridge settling effect and its particle updates
- `7d75490acfbb` Match the tower effect request and its allocation of game slots
- `7ad56ac5bf07` Match the forest player callback and its input clearing behavior
- `58e102f1ef22` Match the forest tutorial actor dispatcher and soldier creation
- `1447910a55fd` Match the tower impact feedback and its effect request
- `39efcf47900f` Match the forest actor ability-release callback
- `1bcacc2d437a` Match the game track-start state machine and share its sound-bank layout
- `9fb323131325` Match the Steam Knight outer armor break and its particle requests
- `1f15cbeb5988` Match mine room grid setup and its address relocation
- `76818c914334` Match the forest bridge player callback and its yaw adjustment
- `7b6bbfddafee` Match the MAIN/0010 room stream selector
- `7f8796470150` Validate complete sound-memory DMA transfers
- `1eed040a1913` Advance ADPCM voice history after each decoded sample
- `8d43d1a3c1bf` Match the mine frame actor and field HUD routines
- `3c197f04f974` Match the Steam Knight particle setup and movement routines
- `f0cab3df1eb9` Match the mine backdrop painter against its loaded scene
- `471c884d31a6` Match the forest bridge tutorial stage transitions
- `fa24b2689996` Match the Steam Knight fragment actor update and removal
- `2c297b6e1c56` Match the mine stat gain display timer and movement phases
- `62cac687a6be` Independently match both music track row handlers
- `aca6519e7f10` Match the forest projectile target queries
- `cca464adb4f2` Match the Steam Knight core contact and bounded damage routines
- `1cf8c907d3ba` Match the mine numeric HUD setup and drawing routines
- `b2c5a778632a` Match the forest player movement action with its controller and model state
- `c406867414bd` Match the mine stat award actor and its three service wrappers
- `11931b4cd466` Match the forest impact actor initialization, update, and expiry
- `bbca2aab6346` Match tower actor allocation and preserve room-specific selection
- `54f17c9b02cd` Recover mine award setup and verify packed numerals across every column
- `bb20c8ba38c9` Match the seven tower body and marker routines
- `bdfcbbac5e50` Match the forest player frame initializer and preserve ordinary dispatch
- `ba9b45d5de0c` Match the remaining seven mine award controller phases
- `08e81e1f1299` Match tower animation refresh and two opening routines
- `18c04a2224d3` Match the mine model sequence evaluator and its regional table
- `cb3358755401` Verify C recovery against assembly emitted in the target procedure
- `ff8c75122ed5` Match four tower armor actor routines and preserve their room selection
- `ced4629e6c31` Match mine terrain plane distance and edge clearance tests
- `6590c7e110e1` Recover mine award actor lookup, phases and movement services
- `ab83adf5fe75` Match the Steam Knight moving armor attachment tracker
- `dcbd7bbbb60e` Match the forest kind-four player selector and contact action
- `34b2fc91ff3a` Report coverage from split public match registries
- `ceae97d8a86b` Match the mine script action queue and stat award request
- `09f327afc305` Match the Steam Knight auxiliary reset and body phase services
- `8cf8cd8399db` Read bounded ranges of files on the mounted game disc
- `43003f0dd35d` Read the forest player mesh from a mounted game disc
- `4f3e69f89934` Match the forest attack strength calculation
- `0eefda43a45c` Match five Steam Knight body turning and phase routines
- `9472890be881` Interpolate polygon texture coordinates from the quantized left edge
- `90f2fa35a66b` Use measured 12-bit steps for horizontal texture ramps
- `9a2b6af76fde` Match the mine award model and signed local pose reads
- `15ad96932bc6` Match Steam Knight joint setup, dispatch and allocation with typed model records
- `0cc137f6e0d0` Match the initial mine placement list and free actor selection
- `77ca2f929b0f` Round horizontal undithered Gouraud colors at the measured precision
- `6f8fda751fc5` Match Steam Knight state transitions and preserve the signed collision result
- `9b64b9ceaea3` Match the mine actor world velocity and vertical step
- `31ce4f9e8a68` Match the mine door phase and model allocation
- `6a003678dbef` Match the mine field actor pool reset
- `612ccc05cb24` Match the fourth-stage mine field initializer in SC02/0039
- `e9753b1d3ba6` Match Tower body action dispatch and shared duration timer
- `64ad2e406a2d` Match the mine door actor initializer in SC03 member 0116
- `8f4a9d295a43` Correct opaque flat-line coverage for shallow GPU segments
- `45be0b40a382` Match the room 3050 actor action dispatcher and pending movement
- `3656cb5f1372` Match the Steam Knight action evaluator and four decision routines
- `2e152be37b41` Match the room 304C door actor phase dispatcher
- `726cca34090f` Match two Steam Knight action predicates and retain their callers
- `f09a09b48b8f` Match the Steam Knight joint effect and its typed actor allocation
- `d789de43c2b9` Match the mine grid cell walk and its ordered face visits
- `7d40c0504983` Match four Steam Knight resource routines and type their callers and test fixture
- `351c8b20c645` Match the mine terrain flag translation from its owned overlay
- `65363a452993` Match the Steam Knight actor cleanup and preserve its return type
- `1b9495952d2c` Match the mine terrain response routine and its cell-walker call
- `d334a239379d` Match the two Steam Knight intro countdown callbacks
- `59ed3754137e` Service pending GPU DMA and bind draw completion to the active graphics frame
- `047f04e7b897` Recognize completed GPU block uploads on the active draw interrupt
- `7d3a3e85f215` Match the Steam Knight turn callback and its player request helpers
- `cd312eb3edf5` Recover mine actor step and its full-width terrain vector service
- `3564df9e009f` Service the audio channel countdown from its mapped VBlank state
- `1fb95c2b1380` Match six Tower camera routines and verify their transition caller
- `aa8c885c5018` Recover the mine child spawn with the shared actor allocation declaration
- `8d796410fddb` Match camera setup and dispatch in two mine rooms
- `279108c38b95` Match the mine emitter and child stage selectors
- `bc9fdf97db8a` Deliver completed CD movie buffers through the registered callback
- `1d15fbb47865` Match the Steam Knight handover and its actor reset helpers
- `6f9d7d0f8116` Match the mode-zero camera updates in two mine rooms
- `8368a725e136` Run the mounted movie output callback on completed native MDEC and GPU transfers
- `e31823f3c066` Round native MDEC color output to the nearest five-bit level
- `366fc2651694` Register six existing forest scheduler matches in their SC01 overlay
- `1ea311780471` Match the forest enemy and its child actor in the SC01 overlay
- `8e6503d04324` Saturate decoded colors after signed nine-bit conversion
- `1b02587e0388` Retain fixed-point transform precision through MDEC pixel packing
- `bc9aace983b9` Match mine emitter initialization and child model feedback
- `b392f05e7079` Match four forest attack routines and align the sound command interface
- `1e2ecb6c4668` Match the mine child model segment query
- `c4b67103479b` Match the forest actor diagnostic selected by kind 218
- `0bec1dd40b8a` Match the mine child actor setup and model allocation
- `cc7c0437879a` Register four forest routines and match the child allocator caller type
- `4bb427c05d57` Match the mine emitter selection and preserve the resident audio ABI
- `1851a4208138` Match the mine emitter projection wrapper
- `8472d6f2f297` Match the mine room 304C light actor state selector
- `676d4e7fc978` Match the mine light actor first update with the resident light setter
- `fc8208941b2f` Match both mine room 304C type 334 actor update stages
- `874c39497c45` Match the paired mine model actor selector and setup
- `68de17b0085e` Match the mine stage gate for actor type 33A
- `84b0c0e0298a` Match the 304C mine event dispatch and first phase
- `3970a02c8600` Match the first phase of mine actor type 333
- `8d18f8a235a4` Match the forest class 133 dispatcher alongside the Steam Knight overlay
- `393c6fe6a64e` Match the sixth phase of mine actor type 33A
- `f1a5501a9403` Match the mine ring timer reset and stage update
- `272f24da11dd` Find configured memory cards and retain existing Continue saves
- `df65a330a3b0` Retire buffered movie DMA after its decoded strip reaches memory
- `3d09f1430af2` Run the opening movie output callback as native host code
- `a9c96fa6792a` Match the village porch quest actors and resident event-flag setter
- `fcc711a7f4ef` Run title sequencer and mixer updates during Continue navigation
- `d272bf44d59c` Match the scheduled village field actor transition
- `614a69b4cb4a` Handle the mode-3 title sound callback when Continue opens
- `9a405b7aacba` Transfer buffered GPU image words to RAM on DMA channel 2
- `a2220e49336b` Match the five-phase village actor update
- `18696c57bda6` Match the game disc audio playback report callback
- `b6f6a6792f29` Match the saved-file status panel builder
- `971d1565aff2` Match the saved-file decimal formatter
- `35e1edebff42` Match the village actor waypoint advance
- `f4e4df3f4753` Verify the existing File menu glyph match and retain timed guest writes
- `2482f23988c7` Match the village floor selector and retain guest memory timing
- `83012998d138` Restore the File 1 card payload and match the village guard at Twinpeak
- `3b569ed7014a` Match Restaurant’s Macho and the daytime Twinpeak gate
- `04b64f5831b0` Restore the selected save through the mounted copy loop
- `b9e02afea744` Match the forest Bincho Field and Guard Lumpwood rescue
- `c1cce445c71c` Match the village stocks actor and its story branch selection
- `5d87c58fbb74` Match the Twinpeak Fusion throw and request queue
- `964bca20be3e` Dismiss the restored file in the mounted load-menu callback
- `7a88d44bb4fe` Read and display the mounted save rows across all four slots
- `ff943a859848` Follow the completed save transaction into the mounted room initializer
- `c5f6e2f0f1b8` Keep the SC01 opening scene on its own fetch path at the file callback address
- `48b8a3f98fc4` Match the room change and object reset after loading File 1
- `8e634d47d0e3` Match File 1 room setup and three Twinpeak actor routines
- `097968f28898` Match the cemetery script that awards Jon's Key
- `9a45e8539706` Restore the earned File 1 room through the owned scene callbacks
- `da3739b78e46` Match the cemetery opening and Twinpeak contact poll
- `88ffbfa6af49` Run the registered game master fade through native clocked operations
- `ea5767f90f02` Match the forest rescue BP award and refill
- `d9ac021f5ad8` Match the village actor callback after Jon’s key is used
- `df0199542ebd` Run the mounted File 1 room latch in native foreground steps
- `acd19a08abb2` Match Twinpeak Fusion contact setup and handle opening
- `654b5bf33303` Match the initial Twinpeak Fusion action and document Windows and Linux testing
- `1ee5ec481526` Match three Twinpeak tutorial handle routines and record the two-slot Continue result
- `43000299588d` Match Twinpeak tutorial setup and contact decisions, and document both player builds
- `f8eb164fab16` Show the full launcher log path in Linux and Windows error dialogs
- `782fe6f3aeac` Match Twinpeak coordinate checks and document both public player candidates
- `292737ec392e` Match the Twinpeak BP debit and document the four-log raft quest
- `36cb9e48c4c9` Match repeat Fusion BP credit and document the river checkpoint
- `6cf4715b36c3` Match the Twinpeak cast cost and document the completed river course
- `c5c5e8b69e35` Document the earned Bracelet save and the served Shrink cast
- `043573bc39f8` Register Twinpeak inventory matches and document the L-Brace appraisal
- `e0e01a5986dc` Recover Fores’s cottage conversations and document the File 4 playtest
- `d3f30461859c` Match the two village interaction checks for Fores and record the Steamwood Manual playthrough
- `f960f7d2196b` Match the Steamwood practice valve and the Fores initializer above the village
- `f0d10e0fd4ff` Match the Steamwood countdown and the plant entrance door
- `36e596c789af` Consume the opening sound transfer once and report the Steamwood playthrough
- `011164b9fb6e` Match the first numbered Steamwood valve and document Windows and Linux playback
- `c695521e3a11` Run the opening track’s observed voice updates on its mounted clock
- `8d3a9b3623e7` Follow the opening note release and document both desktop runs
- `098b3d5af7b3` Run the opening player status query natively and document both real platform runs
- `2126b78c9c73` Match the indoor Steamwood pressure gauge and second numbered valve selector
- `6583f9825bad` Match the File 1 menu-exit callback and its signed status reader
- `47ed86153dd2` Run the opening sound sequence on the host and report all four player tests
- `f250d89811b1` Match the Steamwood timer display, failure recovery and remaining valve selectors
- `a863368eb9f8` Dispatch primary opening events on the host and document all four player results
- `6f5ffec0c3d8` Match the numbered Steamwood valve script and final cleanup
- `095ae49b9c9e` Document all four scene-dispatch playtests and the withheld Linux and Windows builds
- `b3a7b1c40c3c` Match six Steamwood repair and story transition functions
- `43cbe83cf5f6` Retire scene register loads and document four ordinary platform tests
- `94174dbc4bdc` Document Linux startup limits and two earned Steamwood rescue saves
- `0adc2f6ebbdc` Match the Steamwood roof-vent phase and verify the repaired File 4
- `b9978fe60b68` Document scene-counter starts, load timing and package checks for 94294368
- `5887c2a45a6f` Match the Governor’s two HP functions and verify both Wednesday saves
- `b2abb642435c` Record both Windows startup outcomes and verified File 4 progress
- `2e1980c85b52` Match the story-progress reader and writer and document both platform candidates
- `48f1b93b1bf4` Run the scene counter in host code inside a verified scene service
- `9b65a56858f3` Run matched story-progress reads and writes natively and document four startup tests
- `70a086b65605` Document story-progress calls and the four-slot Continue result on the two desktop builds
- `2724353efc38` Latch controller input in host code and verify both resident direction mappers
- `2754ada1c1c5` Return resident controller direction masks on verified quiet guest calls
