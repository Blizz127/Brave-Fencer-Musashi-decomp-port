# Brave Fencer Musashi for Windows x64 and Linux x86-64

**Unpublished test candidate `f8eb164fa`** · November 23, 2026 UTC

These Windows and Linux builds report the game address, the failing interrupt when available, and the launcher's log path. The Linux player now opens the diagnostic in a UTF-8 process when its ordinary locale cannot display the path. On hosts with no supported UTF-8 locale, the dialog can spell non-ASCII filename bytes as hexadecimal escapes. On Windows the launcher chooses the log filename and redirects output to it. The player reads that filename through the Windows wide-character interface when it displays an error.

## Packages

| Platform | Local archive | Bytes | Local SHA-256 |
|---|---|---:|---|
| Windows x64 | `bfm-windows-x64-f8eb164fab.zip` | 3,863,647 | `c7137ceeb048e423e81b5d503db9ff9b956a845dcf19b2891bf3bca835b000b2` |
| Linux x86-64 | `bfm-r106-clean-qa-f8eb164fa.tar.gz` | 4,696,399 | `7a64b79f0dd7935f44c151bb1d35d8e5b9943cbb0e51d786a6f889218df7552e` |

Source commit `f8eb164fab163bee18e14f6485b181c316dfdedf`, tree `dc1eff21553988529085108b4528b230c86648cf`. An anonymous download of the public source reproduced all 3,833 files and their executable modes. The 2,570,860-byte source tarball has SHA-256 `5b36bc582781ec1c97e10be596c9fbc9aa28d0666c06d10f2ba76eceab4bd590`. Both local archives were read back. The public binary gate calls for playable startup and an inn save followed by a successful cold Continue. The Windows archive has 30 files and Linux has 27. Both exact archives passed the local retail, BIOS, excluded-source and private-material checks. Both File 1 tests stopped before the playable bedroom. New Game on both platforms stopped after loading, before the opening moon or the first controllable forest section. The public source is available now. The two candidate binaries have no GitHub Release tag or anonymous download checksums. [Browse the tested source](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/f8eb164fab163bee18e14f6485b181c316dfdedf)

## Gameplay and native execution

| Build or test | Observed result |
|---|---|
| Current candidate, Linux | Cold Continue opened the earned File 1 and displayed “Loaded data!” before exiting ahead of the Chapter 2 bedroom. Under X11, software rendering and the C locale, the real dialog displayed the entire 146-character log path, including the accented directory and `last-run.log`. It agreed with both open output files. The dialog correctly identified interrupt callback `0x8003392C` and interrupted address `0x80042524`. Dismissing it returned exit code 2. The same File 1 restored the bedroom in the original US game. The source save and disposable copy kept their original hashes. On a blank card, New Game accepted Start, showed “Now Loading” and went black. It stopped at interrupt callback `0x8002CFAC` with interrupted address `0x80042558`, before the opening moon or the forest. The original US game showed the moon and entered forest room 3008. |
| Current candidate, Windows | Under Wine the packaged x64 player read a separately earned File 1 and showed “Loaded data!” before stopping in room 3005, ahead of the Chapter 2 bedroom. Its error dialog showed callback `0x8003392C`, interrupted address `0x80042514` and the complete accented Windows log path. The log on disk matched the path in the dialog. Pressing Return closed the player with exit code 2. The original US game restored the bedroom and raised Musashi to his feet. Both copies of the test save retained their original hashes. On a separate profile with a blank card, New Game showed “Now Loading” and exited before the opening moon or forest. Its dialog showed interrupt callback `0x8002CFAC` and interrupted address `0x80042558`. On this window width the callback wrapped onto the second line. Both addresses were directly legible there and matched the log. The blank card remained unchanged and the process returned exit code 2. Physical Windows hardware is untested. |
| Previous paired candidate `97a342d8` | Both platforms opened the earned File 1 and reported “Loaded data!” before exiting ahead of the Chapter 2 bedroom. Linux truncated the displayed path at É. Windows under Wine displayed the whole accented path. The original US game reached the bedroom with this save. |
| Previous paired New Game run `c0b07365` | Both platforms exited before the opening moon. Linux with two occupied slots exited before the file picker. |
| Separate private Linux R117 | An earned File 4 crossed the copper doorway where R106 had stopped, reached the windmill, freed Jon, and received his request for four trees. Hotelo saved the four-tree objective in File 4 at Sunday 05:05, 61 HP, 16 BP and 90 Drans. The other three slots retained their original contents. After closing the player, a fresh cold Continue restored File 4 outside the inn with the same stats and quest flags. Opening the menu showed all three lines of Jon’s four-tree request. The same restored player then spoke to Guard Lumpwood, waited for him to clear the path and crossed into Twinpeak Mountain. On a separate File 4 run, charged Fusion hit the Magician, both Shrink pages completed and normal movement resumed. |
| Separate private R106 | Hotelo saved and restored the two-rescue four-trees objective. A later ordinary playthrough felled the waterfall tree, collected the first log and freed Knight Lardwick. The same player crossed the rotating platforms, freed Carpenter Carvey and collected the second log. The US game displayed the same two-logs-remaining message. Musashi then crossed the wall bars and climbed the waterfall-side pole. The third tree gave the “Almost there!” message. The waterfall tree then gave the fourth log and the message to head back to Jon. The pause inventory showed all four logs. Musashi returned to Jon on the waterfall deck with all four logs. Jon asked if he had brought them. Musashi confirmed that he had and asked for the secret. The original US game showed the same exchange. Jon accepted the logs and built the raft at the waterfall. Story progress advanced from 70 to 80 before he sent Musashi on his way. Musashi then boarded the raft and saw the steering and jump instructions. On a later attempt Musashi cleared the river and reached the shore steps alive. The same player then found the Bracelet, saved the appraisal objective with Hotelo and restored File 4 on a fresh launch. Conner identified the Bracelet as the L-Brace, and Musashi equipped it. |
| Separate private R103 | From a blank card, the player defeated Steam Knight, earned File 1, woke in Chapter 2, reached the inn, saved File 2 and cold-restored outside. |
| Separate private staged R118 | Its new Twinpeak initializer executed zero native stores on the saved-game route. The Magician shrank Musashi. On a separate private R118b playthrough the player hit the Magician with Fusion, earned Shrink, cast it with Circle, spent 10 BP (147 to 137) and regained movement. The US game spent 143 to 133 on the same File 4. In private R118b, the pictured Magician narrowed from 31 to 16 pixels before regrowing to 36 pixels against the same bank. Terrain and the full-sized Musashi stayed in view. On the original the cliff hid the recipient after impact. A 142-second side-by-side recording documented the cast, target regrowth and return to control. The completed R118b run spent 15 native clocks in the tutorial wrapper, then declined the next step at a device deadline. The new initializer never ran and issued zero stores. A second Fusion hit restored 10 BP while Shrink stayed equipped. Private R117 remained served. |
| Separate private Linux R119 | An ordinary File 4 run reached Twinpeak, earned Shrink with a charged Fusion throw, completed the tutorial, cast with Circle (147 to 137 BP) and regained control. Three parts of the contact check ran as host code. Their 48, 95 and 12 local clock units matched the US game. The enclosing phase took 19,248 units here and 20,217 in the original. The private camera showed the Magician shrink and regrow. On the US disc the target went behind the cliff. At that checkpoint the owner’s private launcher served Linux R119. An independent download reproduced all 6,000,210 package bytes, and the launcher successfully detected and installed R119 over R117. This downloaded copy cold-restored File 4 outside the inn with 150 HP, 155 BP and 830 Drans. It reached the Magician at Twinpeak, completed the charged Fusion and Shrink tutorial, and cast Shrink with Circle. BP fell from 147 to 137 while HP stayed at 150. The Magician visibly shrank while Musashi stayed full-size. The Magician later regrew. A subsequent movement input moved Musashi again. Normal quit preserved the earned save. The comparison film shows the full 112-second sequence. The precise projectile impact frame is unresolved. The original US game spent 143 to 133 BP on the matching cast. The 10-point cost is separate from an earlier four-point drain in the original. |
| Separate private Linux R120 | On an earned File 4, Musashi gained Shrink and cast it with Circle. The staged Linux player spent 10 BP (147 to 137) in 22 game clocks using host code. The US disc spent 10 (143 to 133) in the same 22 clocks. The Magician shrank and regrew on Linux, and Musashi moved in both directions afterward. A cliff obscured the final size of the target on the disc. The owner launcher subsequently served the same Linux archive. On a fresh run of the downloaded build, a Circle press produced a gold projectile. At video frame 20,576, the host routine spent 10 BP (111 to 101) in 22 game clocks. The HUD showed 101 shortly afterward. The Magician was outside the casting view. A hit, shrink or regrowth, movement after the cast, and a repeat Fusion reward remain unverified on this run. This private build uses a hybrid runtime. |
| Steam handheld device | Controller, borderless fullscreen and automatic on-screen keyboard remain **NOT RUN** on the owner's physical hardware. |

The public players use a MIPS interpreter alongside compiled host routines. The Linux release build registers 204 resident functions and zero scene-overlay entries for native dispatch. The 940 matching source functions and native dispatch count measure different things. An interpreter-free played chapter with sound, timing and saved progress remains open. The private builds use historical sources excluded from public packages.

### Historical release review

The latest published Linux build is R55 and the latest published Windows build is R48. On November 23, 2026 UTC, anonymous downloads returned the full files and matched the SHA-256 recorded by GitHub. These hashes identify the existing files. Those releases remain under review: historical players contain an original 36-byte dispatch table, and the project subsequently removed SDK-derived routines and scene address tables from the public source. [Historical player audit](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/blob/1ee5ec4815260aca4d8777eb6112cbfe442bd439/README.md#player-downloads-and-source-builds)

| Existing release asset | Bytes | Anonymous download SHA-256 |
|---|---:|---|
| `bfm-r55-ba90b87ad-linux-x86_64.tar.gz` | 8,420,178 | `757e9bc95919ea6e45cb3e3fb83fa3b206e35f81301d530b2e9803328d137b60` |
| `bfm-r48-355ec7668-windows-x64.zip` | 3,983,449 | `38a577f7da0dfa7bf4b1fb067b8743a3d012e0bb8fb8c1e29a0fd7f094f2726c` |

## Changes since public Linux R55

- Cold Continue now reads the mounted scene state and memory-card directory. The native room-slot reset clears 128 object types. Later foreground steps implement scene setup and the room latch while preserving interrupt delivery. The public player can still stop during the transition to the saved room.
- The clean-room player obtains dispatch and draw data from the user’s disc. The public source removed embedded scene address tables and SDK-derived implementations. Region and source checks distinguish overlays that reuse the same guest addresses.
- The resident game master fade advances through compiled host operations. Other interrupt-sensitive sound and game paths retain guest execution. The 204 resident rows describe selected dispatch entries, not the fraction of any chapter executed natively.
- Matching game C includes the opening forest and camera, Steam Knight actions, village characters, the forest rescue BP award, the cemetery slab and Jon’s Key, and Twinpeak actor actions. Three matches in the candidate find a tutorial slot, initialize it and return its handle. Together they match 288 bytes. Five additional matching routines cover Twinpeak message initialization, text-rectangle setup, the 648-halfword buffer clear, a contact query and its continuation. They match a further 604 bytes. At the candidate revision the public scene registry covered 631 regional matches and 98,616 bytes. All eight of these Twinpeak routines run on the guest CPU in the candidate.
- The ordinary launcher reports the failed interrupt and interrupted address when available. The Linux and Windows launchers pass their actual log filename through to the dialog. Windows reads the Unicode filename directly. On Linux, the X11 error dialog can use a UTF-8 locale in a separate process while the game retains its original locale.

## Source and playtest updates after the candidate build

The Windows and Linux archives identify the November 23 build from `f8eb164f`. Subsequent source checkpoints now cover 654 regional matches across 19 members, totaling 101,224 bytes. The Fores and Steamwood matches described here have no native dispatch in the public player. The public archives still stop before the opening moon on New Game and before the Chapter 2 bedroom on Continue.

- Two coordinate checks select a contact test for an actor. The original US game returned zero from the first check and `0x2000` from the second during a successful Fusion encounter. The pair accounts for 268 bytes. [Coordinate checks](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/782fe6f3aeac936ba83a8fd6138d20dc31e75ca9)
- The 56-byte debit subtracts the equipped ability’s cost. On the original File 4, a periodic deduction first took Musashi from 147 to 143 BP. Casting Shrink then took him to 133 BP in 22 game clocks. [Shrink debit](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/292737ec392eba20e15a6be73ac41eb726766469)
- A successful repeat Fusion hit displayed a green 10 and raised Musashi from 125 to 135 BP. The credit and its caller account for 60 and 76 regional bytes. The 60-byte body also occurs in the forest. The original trace followed the credit back to its caller. Testing has yet to observe the subsequent service return or a live award that hits the BP cap. [Repeat Fusion credit](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/36cb9e48c4c960753d7be144fdeb63e52b57f288)
- The 64-byte paid-cast routine and its 24-byte lookup select the equipped ability’s cost before calling the debit. In the original US game, Shrink selected ability 13 and returned a cost of 10. This match covers the Twinpeak table in SC03 member 0029. [Paid cast and cost lookup](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/6cf4715b36c3116e2acca2fcbfdaadb0de0d668d)
- Three Twinpeak inventory routines reuse the existing mine C for 828 matching bytes. They add no native dispatch entries. One path finds an empty inventory slot and stores the item and its acquisition day. Identifying the handler used for the Bracelet pickup still requires a trace of that interaction in the US game.
- Four functions in SC03 member 0056 start Fores’s first or repeat dialogue and handle its completion. The first conversation selects actor phase 10. Once the script reports completion, the original code sets story progress to 120 and story flag 237. The repeat conversation selects phase 14. All four complete C functions match the original 244 bytes. The script and dialogue text come from the player’s own disc. [Fores’s dialogue callbacks](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/e0e01a5986dcc2bf85d44ce9db0cde58569c0298)
- Two 60-byte functions in village member 0002 query interaction request 3. When it succeeds, the first passes the actor to the initial cottage conversation and the other passes it to the repeat conversation. The earned File 4 playthrough exercised the first call. Verification of the repeat call used controlled inputs.

On the separate private Linux R106 playthrough, the same character who delivered the four logs boarded Jon’s raft. After it broke, Musashi spoke, the screen faded and the game automatically restarted him on the river. On a later attempt he steered past the central boulder, collected Drans and reached the second waterfall before losing the last raft log. On the successful attempt, Musashi passed the final sweep with 135 HP and 2,060 Drans. The game loaded the landing in room 3025, entrance 2, at story progress 80. The raft carried him to shore. On the steps the HUD showed 139 HP, 57 BP and 2,060 Drans. Musashi then opened the chest on the ledge. The pickup message named the Bracelet and said it needed appraisal.

Back at the inn, Hotelo saved File 4 on Day 20, Saturday at 22:58, with 142/150 HP, 21/170 BP, 2,060 Drans and four of 35 Binchos. The write changed 88 bytes in File 4, including 29 in its summary and 59 in its save data. The other three files kept their original contents. A fresh launch loaded File 4 and restored the village outside the inn at video tick 2,038 (room 1010, entrance 10, story progress 90). The player still had 2,060 Drans, and the menu still directed Musashi to get the Bracelet appraised. A cold boot of the US disc in PCSX-Redux loaded a copy of the same File 4. Both games showed 146/150 HP, 17/170 BP and 2,060 Drans in the village on Day 20, Saturday at 23:00. The US status menu showed the appraisal objective, and its item menu described the selected Bracelet as unappraised. This US test ended on the selected item.

After a night at the inn, the R106 player took the Bracelet to Conner. He identified it as the L-Brace and told Musashi he could climb certain walls. The game confirmed that Musashi equipped it and could now climb special walls. Story progress reached 100. The HUD showed 150/150 HP, 142/170 BP and 1,850 Drans before and after the appraisal. The public Windows and Linux candidates still have the New Game and Continue outcomes recorded above.

The original US game subsequently loaded a copy of the same day-20 File 4 and completed Conner’s appraisal. It showed the L-Brace name, climbing explanation and equipment confirmation. At shop entry and again when story progress reached 100, all 64 story-flag bytes and 256 script variables agreed with R106. Both retained 1,850 Drans.

Private R106 then accepted the Steamwood assignment and saved again at Hotelo. A fresh Continue restored day 21, story progress 110 and the L-Brace. Musashi climbed the cliff and reached Fores at his fenced cottage. Pressing Square stopped R106 at `0x801CBB54` before the first dialogue. The game had read the three sectors containing his actor module. Its dialogue callbacks sat just outside the area recognized by that player.

Private Linux R123 then cold-loaded that File 4 at story progress 110. A player climbed to the cottage and spoke to Fores. He explained the Steamwood emergency, gave Musashi the Manual and directed him to the top of the cliff in Steamwood Forest. The dialogue closed, story progress advanced to 120, and story flag 237 was the only changed flag. The timer showed 23:55 beside Fores. Moving left took Musashi off the grass and onto the lower path at 23:33, and the pause menu displayed the new objective. The original US reference also shows the Manual award. This R123 playthrough used guest CPU execution for the conversation. Completing the plant and saving that result remain open.

Private Linux R124 implements the four dialogue callbacks as host code. Focused tests exercised all 61 instruction positions across these callbacks with guest instruction decoding disabled in the tested bodies. External game services still use the hybrid runtime. R125 also runs the six-instruction cost lookup for a witnessed Twinpeak Shrink cast as host code. Replaying the original cast through the compiled lookup returned 10 in 13 game clocks. The following debit took BP from 143 to 133 in 22 clocks. These focused tests cover the stated calls. Ordinary R124 and R125 playthroughs through these changes remain unverified here. The Windows and Linux archives listed above predate these private builds.

### Steamwood practice valve and private R123

The village initializer for Fores matches 280 original bytes. The earned File 4 entered his cottage area at story progress 110 and ran this initializer. Controlled comparisons checked 33 combinations of story progress and the game’s readiness check with two host compilers. They covered his actor phase, child actors and ordered service calls. A new visit at progress 120 remains unverified in this comparison.

Six functions in SC03 member 0011 match 588 original bytes. They initialize the pressure controls, advance and wrap the needle, check its position, and update the valve meter. A press in the inclusive 68–80 window adds the current selector’s increment, capped at 96. A miss subtracts 16 with a floor of zero. Four of the six original bodies also occur in other rooms and are counted here once each. The gauge tables, animation data and scene services come from the user’s disc.

On the owner-private Linux R123 playthrough, Musashi followed the Manual objective into Steamwood Forest, climbed to the plant and operated the practice gauge. The meter was photographed empty and half full. After a successful input the plant door opened, and File 4 entered the red-lit interior. The recorded call reached the meter increment. The exact needle position at that press was not recorded. The original US reference shows the same outdoor practice puzzle before the interior valves. This playthrough has yet to establish completion of all eight numbered valves or a subsequent save.

A separate R123 File 4 reached the Twinpeak Magician. The first Fusion contact taught Shrink. A paid cast spent ten BP. A later full-charge contact in that same visit displayed a green 10, raised BP from 109 to 119 in 29 game clocks and returned control. The US game awarded ten BP, from 125 to 135, in the same 29 clocks. R123 ran the reward return as host code and used guest CPU execution for the Fores conversation and practice gauge.

The private launcher served R123 from source `1babffad28d26c9d0cde6bc88e7e189cc328bf9d`. An independent download reproduced the 6,003,529-byte Linux archive, SHA-256 `1bbece9c9b0ff616f2d5194ff48b8d0026e992e1798b06056ac4fdc08ca0e6bc`. The playthroughs above used the staged copy of those same bytes. The public Windows and Linux candidates and their checksums at the top of these notes are separate.

## Supply your own disc

Use a personal dump of your legally owned **Brave Fencer Musashi US PlayStation disc (SLUS-00726)**. Keep the `.cue` and every `.bin` it references together, with filenames matching the cue sheet. Single-BIN and multiple-track dumps are supported. A plain 2048-byte-sector ISO lacks the required CD-XA data. No BIOS is needed. The player supplies its own kernel services.

1. **Windows x64.** Extract the entire ZIP to a writable folder. Keep `SDL2.dll` and `OpenAL32.dll` beside `musashi_native_boot.exe`. Run `launch.cmd` and select the cue sheet. You can also place the disc in a `disc` folder beside the launcher or set `BFM_DISC` to the full cue path. The launcher remembers the selection in `%APPDATA%\bfm-port\disc.txt` and writes `last-run.log` in that folder. The display path uses OpenGL 3.3. The observed Windows tests ran under Wine.
2. **Linux x86-64.** Extract the entire tarball. Put your disc in `disc/` beside `launch.sh`, or in `~/Games/brave-fencer-musashi/disc/`, then run `./launch.sh`. Alternatively, run `BFM_DISC="/path/to/disc.cue" ./launch.sh`. The log is `~/.local/state/bfm-port/last-run.log` under the default settings. The archive's `manifest.json` lists the required host libraries. Its imported symbol versions include glibc 2.38 and GLIBCXX 3.4.32. Working display and audio drivers are required.
3. **Controls.** The arrow keys move, Return presses Start, C presses Cross, X presses Square, V presses Circle and Z presses Triangle.

## Components and corresponding source

The local audit compared each compressed archive, its contents and the supported decoded material with 902 original-disc, BIOS and other reference files. Neither compressed archive matched a protected sequence. All 575 Linux member occurrences across 24 ranges were attributed to their compiled sources. Four occurred in the executable, including a standard hexadecimal alphabet and the compiler’s surrounding zero padding. The Windows ZIP had 801 member occurrences across 23 ranges, all attributed. The audit accounted for all 679,936 bytes in the Windows executable’s instruction section and checked all 451 files in its OpenAL source archive. No unaccounted original data, prohibited implementation, save, credential or private note was found.

Windows includes the x64 port, SDL2, OpenAL, their notices, and the OpenAL Soft source archive under `licenses/openal-soft-source/`. Linux lists the packaged libraries and their exact file hashes in `manifest.json`. The runtime provides its own kernel services. Both players read the original game from the disc image supplied by the player.

The matching Linux OpenAL source is available from the project’s existing public release. All four files were downloaded anonymously and checked against their recorded SHA-256 digests. [openal-soft_1.25.1-2.debian.tar.xz](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/releases/download/bfm-r55-ba90b87ad/openal-soft_1.25.1-2.debian.tar.xz) · [openal-soft_1.25.1-2.dsc](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/releases/download/bfm-r55-ba90b87ad/openal-soft_1.25.1-2.dsc) · [openal-soft_1.25.1.orig.tar.gz](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/releases/download/bfm-r55-ba90b87ad/openal-soft_1.25.1.orig.tar.gz) · [SHA256SUMS-openal-soft-source](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/releases/download/bfm-r55-ba90b87ad/SHA256SUMS-openal-soft-source)

## Full source changelog

The public R48 Windows and R55 Linux release tags both point to `ad71556b44506599fb08e654e28651d81eba7206`. The following list contains all 299 commits after that public source base through this candidate. The historical binary filenames identify their respective builds.

- Source update 2026-10-29: 1,340 owner clean-room decompiled functions. e0b18220472a8ade8cae7229fa419e5164071db2
- Merge source update 2026-10-29 (1,340 owner clean-room functions). 713295a38606bc500003f7e24011fcf33a4466d1
- Source update 2026-10-30: 18 more owner clean-room decompiled functions. 5cd48bc13842d87ef2ed0a5ad4175a4b93499da0
- Merge source update 2026-10-30 (18 owner clean-room functions). cc7b14cdb8f26f3609e9ae535d01c31aa2c58433
- Port-source update 2026-10-30: host platform layer. 45626d1c738662cdd435a9c7bc5431234581edcb
- Merge port-source update 2026-10-30 (host platform layer). 5fc1cffb57051a65944910eb0307f39d42eb8538
- Match 155 scene-object routines across six USA overlays. 180007f62c90ea9bee9bc71c6972d6e39ed2230f
- Build and exercise native angle and movement routines from public source. b2cc54a2abb7372c040cf6e199be0b776efd6091
- Read verified LIST dispatch targets from the supplied disc. 6a79e717fe87c267d9832461601a4b60bd57a0d0
- Match the opening player-state callback and direction timer. 9a275090dee6553f20ac52fadfd2d0c8b628e5e3
- Apply PS1 texture-window masks in the CPU rasterizer. 085a746054118edab4913f0fa66c7d0d1e5ca8a0
- Build and verify the native function library from the public registry. 016cec09c7623ce58764108b040b7f2a5314e388
- Carry accepted polygon texture pages into following GPU primitives. b8aa0c3a230f22a59bfe3ae43d6e6b6b9f629f15
- Match and test the timed player event state transition. 4d1b91ff4e374852669ebb5ddf9402e0e10616b9
- Share active SDL gamepads between gameplay and menus. 3cf8709f86480ebf9394bbeffd94d1e47efad7ea
- Rebuild and exercise the opening camera registration and scene counter. ecd1dcad82a7b8062a370a0368b8e3e722711f45
- Compute triangle orientation on the shared native geometry unit. 08906cd55c2b925eb0d008ad399fa98fe014448f
- Match resident inventory operations and test item acquisition and removal. dc57a6d1b45e484513a2bf0db6db2ba6c4941b60
- Accept the one-word GPU no-op used by the inn save menu. f00fea5f1accccbd0e1b53ca417a3ff109cbbad3
- Present the GPU display page independently of the drawing rectangle. d45d362e645a573391654642e4ee2b5b07039e90
- Honor borderless desktop fullscreen and preserve the original picture aspect. 7fd45d214bc65171ae850368e411bca0c8bacfe5
- Verify Linux package provenance and discover discs in local game libraries. 756c2f73f7d30923fabc53ff5776a2244ce5ddc0
- Rebuild the opening player event through its camera and release states. fd7dc148f2855dc48af5d41b80bf9a20b76f4589
- Rebuild the shop wallet and verify debit, cap and overflow behavior. 2da109f2ec473455a6a4cf1797739dfc33cb98a4
- Replace historical work notes and upstream test excerpts with reproducible public checks. d6d99065042cb99779c47954e263359e3630df60
- Recover the native wallet digit formatter and both HUD packet routines. 350cb3d61570854337d57dfd14e429ecccf5910f
- Recover the room facing selector and its signed triangle tests. 960bb68aef3e591b086abc86a87f7290598fcdbf
- Check script-variable bus ownership before native loads and stores. 76eedac6970be2d76cb0d70d02fa1af680c20cc8
- Recover the two title menu counters and verify their wraparound. 88dee3329728fc7c4d14e0dbee1d765c09fe23cb
- Build the Windows player and verify multi-track paths on both hosts. 29f2d55c2d8a1c42140a682880cc5f9ff40a0de3
- Recover the bakery purchase message lookup and selection. a0f1771f73328796bf0bbb28ad350ee27214a0ad
- Verify the card-menu item function and defer native execution at interrupt boundaries. 6d39578f22e04ac9374fd639a84bf57767e4f8a5
- Declare the streaming executable digest before Linux boot uses it. a06f06c977ff6a0111562bff22bb07fe590fdf4c
- Keep memory-card writes bound to the owning save session. 35de4f634261f37c60cc9d7d22bceae757169e16
- Check repeated reference spans and decoded source literals before packaging. 7688df2ae0d88379ad43b55e13a943fb9fdc1ee2
- Match the three forest bridge trunks through their fall and settling. 92b608720efbbbd5c43e105c3ecf693b809ac2d3
- Link current Linux and Windows source builds from the download section. 9eeb860d5dc82d621c67ff4ca37dc5a3e1b0c9bd
- Use exact tick offsets when the source already runs at the device clock rate. 0a010dbc77762b0dad487393081539e1e5ab8b5c
- Run device routines from the player disc and restore interrupt-driven card I/O. a4389bcef57e0a31a0bcebd10345c317a3d1c872
- Cache fixed resident address ranges while fetching live instruction words. c036e819d35a08f7c9dd4829ca018c2cb9d3c825
- Match the camera update and defer facing routines that require the guest bus. ed04764682febeb5f542a71eb89f2e5004c90e52
- Resolve Linux player libraries relative to the executable after relocation. 450cf6d14bfebd24c502aabb4ed21486f617ee4d
- Run the matched wrapped-angle subtraction at each of its three guest boundaries. d9ce63b7bfdf61e15c5bc87361adcdd08931a0fd
- Match room camera initialization and callback selection with one shared actor. 6ccdc19c2b0c5b70823a8889abb6b54549b1b878
- Match sixteen field movement and controller routines in their owning scene member. d3f07b0654ff1a20236381d68bca77193e7621ca
- Record per-player host library and ABI requirements in Linux packages. 4a1755f412782106c1d15e27c0b38d7edeb0a777
- Find Windows discs in literal package and profile paths and preserve valid selections. 6bd1aa6983581fcac10943b644c334e68f872a39
- Decode GPU drawing coordinates and reject oversized triangles before clipping. ecc861401fed234131723f09c53d9b0665d524a4
- Recover the forest floor query, cell walk, face selection and plane distance. 01cd11072cbf0f901e29f3a17dc2797179df6680
- Match measured GPU polygon edge coverage and rectangle size fields. 3cdb3a00ddd069904a80573d460146d9b1d08ea1
- Match the forest contact search and the plane intersection input to the normalizer. 5d7b22e564cbdd7d51c1e2cbd73e2ec4106399ab
- Match the forest contact transform and keep it out of unchecked native dispatch. aae402146231cc18fd0ac29f109b3d99e9729925
- Match scene countdown and fade transitions and guard their overlay selection. b5f4910ba34f8516941a7e270f6f40832f3c09c7
- Match the five-way forest contact dispatcher and its classifier. ee5c3b2daf36a816495eade51db4a2a5efafcebf
- Match the paired phase dispatch for the return side of a scene fade. 89f3831aa61b221b943e0ae81061655563fc7dd6
- Match forest linked-box collision and all three axis tests. d5ce271fbbec3a4410d1c5d3e316b8e93795ba46
- Recover the room draw dispatcher and preserve its three guest arguments. c8e6bdaaa0091fe2895216ffe657648a8ecb9712
- Match the sound voice release routine and model callback ordering. ab40672a33d1f37177306d2c46019833e611bc67
- Match room 3050 frame dispatch and honor its scene-specific state. b19a6a3b73ecc714cf5c377755ed5306bbefaa7d
- Check card VBlank selection, cancellation and batched completions. aa328be4aaaca5ea17b0bb0eb65ae7218ffcf2c3
- Match mine contact probes and correct the three-coordinate distance interface. e69cd3244a5c01628f02476dcd95906bb48cdedc
- Match four mine floor probes and preserve their host pointers. 17cc4a9628e0dc64830689f9c482e6778c657b89
- Re-derive mine model surface and floor map queries. b0f270aa0be7bbd1d46e24203402ff4ffb0548b2
- Preserve actor handles across the two forest collision searches. 494aa466cb7459217c7b027bfd2a293e55874e83
- Initialize interrupt auto-ack words at the selected game entry. 0f5bd9966e15dc9ba802e280c622b937172ad7a1
- Match the moving forest segment collision query. 924361dcd1b47d610b9115e26f9d1a434a99b6ba
- Match mine floor face intersection and dot product. 7d1ff0a0e45a37398cf487239862053a89acbb58
- Match linked sound events and preserve channel search timing. 59c17fab62e625df594925d2f1e2f666b2894cd3
- Match the mine model contact transform. 83e6221748d2d33595c0e45a58427d7892c4a7e7
- Read the first matching card directory entry through game callbacks. ff9ab165d575a9d22a06329cdc16124e93350d42
- Match the forest slope classifier against its shared source. 545a7b29aff33ec200ce5d46cfe82fa7bae3a7e9
- Match the mine model surface classifier. 2738c4641231ed415e23f84875caf4e11da67a27
- Match the forest model collision query. e34513256463f4e0bfdc4baa7669cc669e5c6b07
- Match the four mine box contact routines. 3f2c14a498a05f85b2c81978636355e6ef66dcde
- Read optional card transfer input as binary on Windows. a6f7428c66fb1e895a0bb4543f3c57d902c62865
- Match the forest model grid traversal. 3dd9420e9bc979e4e03142668ca3ca491dcd880a
- Match the mine per-frame duration script. 6567fec85626161092c6c7c709de4fc11160a7fc
- Read and close the authenticated memory card metadata record. e4b1060ea47b8bc071b2cbf645a1e58147713cb6
- Match the twenty mine slot position updates. d01b3e75c1303952859725dd9f664de8afa87b31
- Match the mine actor position scheduler. 40dab98cdc44906cbe873f4c824caddcd3b29b29
- Honor the game sound fade threshold inside an interrupt. 7026224cba634727a674b679724106efefc514eb
- Match forest movement and two-cell ground queries. e8b355162a62f77fc01685688ddc7dd1c92469e1
- Use the checked mine memory interface on 64-bit MIPS hosts. 1f3a0400b72e3b75974c205dd0b2828cf0e95e19
- Match the forest actor movement query. f43a93c0ce176699dcea98fb5c13a8a784db9ae0
- Return through the live GPU DMA interrupt callback. a2349a39e6ea819131f344cc0cc8fb0fc4713bf8
- Execute the startup exchange on the full player clock. 27a3142feb57e8438425a9ccf65166210409f501
- Match the mine actor scale and update prepasses. 4aee641a6320cd4479573b2655846516ce226c96
- Match the mine dialog band color update. 9370f452ad3e50d6f5461fac5b620dc87a4bad0b
- Match the conditional mine pickup handler. f347962407ca5acb697457d14ccbde46b3698b23
- Match the forest floor query and its contact result. 7d22b245630046a1e5593d39c5f5d80fbbdae5bb
- Match the mine target indicator draw list. d40671f9144b93daf9fe571ed36cb8667d6977ce
- Resolve forest model and grid addresses on host builds. 2b2dcc5e88973ebab64e261abc69a657e28b2b08
- Match the forest player update and action flag dispatch. 5e050a0b962f2858636e510c8a41e5df9a0a61a9
- Report the startup exchange under its actual execution context. 83ad88296ef4e24520a6147a6387bd1d0df0cd74
- Match the mine animation command scan. 4f92b5d0ecb541872b7b12043dcbc731bf3fbf09
- Match the per-actor mine animation update. 0fe255435977d4752d408c63a0790e86f9ca067e
- Match the forest player action selected by the dispatcher. e72cb44a020a4c7e76f66cdad36036dcb369a2a9
- Run the first game main call on the active native lane. 75ece15bfb06abb36b3e72ae0bcf0773f097de36
- Match the two forest actor collision tests. e746e0316d608f4eb28af5bfb932172a980ef948
- Match the mine animation command dispatcher. 6b518b285e281f3b5d85128825610a58c4bd39cc
- Match seven forest player action and ground query routines. b98812691f3214dccadeed6d02bc5ed6647d383a
- Initialize the registered game main test provider by field. 60122b83dd1978a9d8074fabd86e1851e968174d
- Preserve the guest actor handle in the forest event callback. 61e381cfb6ae8e94b14d6a31d3308afc33175591
- Match the mine screen effect with a four-byte guest handle. 21f0c117db2557f7ea63c12d251104002a49c0c9
- Follow the mounted save file through its first metadata read. 67f0ebf1416d706cd906283b9678c6fd8c2c5fbe
- Match the mine actor step toward the focus point. 6d1716eb7789e040b3b2a48aa9c9083a051c0acd
- Match the forest player phase and three transition handlers. 8fad6747f12ba2d50b81133e37e3c42cf82f0228
- Keep two actor counters on the guest CPU until timing matches. 08bb71cd4e3368a17bab5cb5d76f765fda436b93
- Run seven game main instructions after the original constructor returns. 576afe1df3c3de905a78ed3a5868e924a4473cd4
- Test two GPU DMA completions and their interrupt acknowledgments. 92d60b4846a7272c77e71f672f3217b15b021236
- Withdraw SDK-derived routines and direct affected methods to guest execution. c532cb8adfdb0e8b07a7ec1097b220a9e6d85956
- Match the forest actor event checks and shared event allocator. e6197eaa154c300c93777c92207e62293d143a77
- Match the mine controller target update from its observed frame service. d570408657191d8a3fefce6e7f74f974d462feb0
- Run the startup graphics caller through its first library call. 1cbbfba6b566875464f304a908274b9161f15e18
- Keep the forest flag read interruptible at its original instruction boundaries. 078954b24aeeeedbdf9f71a45cf6eab67a267427
- Match the mine actor update and its two fade requests. 7b84c4bfdde552a8d6548e95fad106ea9a76b237
- Report the clock actually driving the device epoch. f2305668002aeb4140ef18f83730bab687bff8f5
- Match the forest animation command reader and its selection routine. 6a7f0b8258aa54cca86cf666be5cacd5b1796a0e
- Match the mine fade progression and its completion conditions. 14e503d4c55803a524732791222ceeabe296d7d5
- Match the forest secondary event and its positional sound call. c070e41a4d1425286a09ff4193bf6620536d6b9c
- Match the mine attachment list and its per-slot position updates. 7136ad50e65f7d6430fd75344fb0c3b0dc8a7d42
- Remove scene address tables from the public interpreter. 1649350f9e0c6724e26b67593dccee16f4575ccc
- Use the project include directory when testing copied formatter sources. 6ad71f2ff297ea4d281ddc1349e3f219de618927
- Always report the final GPU DMA counters on shutdown. fa983b2ad61b59288eb728a8e989ad4b41457be1
- Reconstruct three forest actor ground attachment routines. 4b64e33f1d14bfe55b29dd28a34931d40fd0f597
- Reconstruct two mine frame callback dispatchers. 83aa3f0e8a9e12d15b437d19d89caa7b2a02b9fd
- Match the game graphics startup wrapper. cc5688bbf1fb69fc61c3cbefae8db4823b9ff8a1
- Match the mine frame actor link update. 359bd354518a172a3cec9cd30c013eeb06e39269
- Run the game caller through the CD library call after graphics returns. ad60d6d1db774ab71ccca807948d416e1fd18e54
- Resolve forest ground orientation links in the host source. e1c5e33b9f92c7fe6620d0bf6094dd9e21bfe3a9
- Match the mine camera update and its live owner selection. a6da90874424975ff100301caf455b90d4414c39
- Match the game sound startup sequence. 496030f709532efc891de02f0c0c7f6750f5cc7a
- Match the sound record clear and preserve its per-record reads. 51152e5d86845b7727f41d55f0e1ee60fd2e6669
- Match the game disc directory against original startup and archive entries. 357016e4d055363f498c62cb56374134723dd5dc
- Match the eleven-state game disc reader. fcfe6145325940a208e6f4fae3c4a2eed76281f3
- Match the mine collision actor state dispatch. e61d1e64cc26a6c5bfe189d8f7fad1e78847837a
- Preserve the full word returned by the forest flag reader. b61b26a56294ccfb9ec9427e0e099926974ce24d
- Match the mine collision actor initializer. 4f30668430837f4830f9629b7c99738f2729310a
- Match the game sector callback and its two dispatch tables. dc4c865f327935800e95c3280eb711ea8e0dccd2
- Match the mine actor visibility, motion, and collision sweeps. 37e7440c6f6b4f3ad01447b2f6148a0c76c38fa5
- Match mine rotor creation and state dispatch. 7d713ff80584973fb7225e902693d2ddb3a70d10
- Match the mine rotor update and sound selection. 9c12c3cc211d9e67f4ecf1e70bf5a1104b959982
- Match the Steam Knight attachment damage callback. d0010efa680dbe4d107a25c729996c49df7bb679
- Model signed forest velocity updates with defined 32-bit wrapping. 8f389c5b853b17ca13af890aeaf30e79f4ab047d
- Match game sound-stream submission and cancellation. 7bfa7b947a628f164d64a48c43a39dad07f9ec74
- Match the forest camera vector projection wrapper. 3a23533a27e6b9b628711b1714a484bf852f4767
- Match the Steam Knight contact that records incoming damage. 558683a151e8ee56663fdc8370a3f48a430a4d7d
- Match the mine actor pool slot initializer. d58661472a7814e9735d139143253546a86e5b1e
- Match five forest actor activation functions. 24f8b77fb3c861c3a5d9e4147bc7b5bfb131790f
- Match the tower boss damage reaction and verify its callers. 51eab8f8f88eb18edf8dbafa4a3dd8076d445a61
- Match the SC02/0039 mine room selector with its MAIN/0012 counterpart. 538869590a9c75fc80258489f39cf2cefe577ede
- Match the forest player model velocity transform. 1ceee70d4f7f9955a01e8780be5ddb1f31c0e273
- Match the forest image move list to its verified sibling. b1231ba260e009395e20c068b5a6bf2152f3b48c
- Match the forest bridge settling effect and its particle updates. 725299abb0efc2f48e3fcc6c5e6616a978da9183
- Match the tower effect request and its allocation of game slots. 7d75490acfbb297514319d4cb001d34c2485396f
- Match the forest player callback and its input clearing behavior. 7ad56ac5bf071ecebcb8cd09b2f9ea3b2277900c
- Match the forest tutorial actor dispatcher and soldier creation. 58e102f1ef22ead2893cebe18e13b837549139a5
- Match the tower impact feedback and its effect request. 1447910a55fde10ff55c7440e7317558f3d62f4f
- Match the forest actor ability-release callback. 39efcf47900f636c269d016db4d370f314af6a27
- Match the game track-start state machine and share its sound-bank layout. 1bcacc2d437a91abfba65e18681d87d9ab5dad9d
- Match the Steam Knight outer armor break and its particle requests. 9fb32313132567c41982154d98c5533a9cfaa8a4
- Match mine room grid setup and its address relocation. 1f15cbeb5988082aed9447508e72fcce2e014506
- Match the forest bridge player callback and its yaw adjustment. 76818c9143340d2f2f5c951f86a1882f71ed49b3
- Match the MAIN/0010 room stream selector. 7b6bbfddafee30613dfb809d670a4eaeb6d84a67
- Validate complete sound-memory DMA transfers. 7f879647015018949e886e4aa6bcbb29ea0f3177
- Advance ADPCM voice history after each decoded sample. 1eed040a19130fa82ac4fbd9aaa08fc33e6d869c
- Match the mine frame actor and field HUD routines. 8d43d1a3c1bf8989ef3272dcb3e2865192b02604
- Match the Steam Knight particle setup and movement routines. 3c197f04f97480eb55bea918bee7ffb34f94b183
- Match the mine backdrop painter against its loaded scene. f0cab3df1eb970848561cfb1ad812a1b3405aca5
- Match the forest bridge tutorial stage transitions. 471c884d31a61d24f46650d5352b36e73b411451
- Match the Steam Knight fragment actor update and removal. fa24b26899966f667f1c87e87ef55715ca0a7ad3
- Match the mine stat gain display timer and movement phases. 2c297b6e1c56f742316367795e56ed14869fffa3
- Independently match both music track row handlers. 62cac687a6be5bc1d7b05b7c116ef8bc2dd145b4
- Match the forest projectile target queries. aca6519e7f10a9896c58706d1d0f13689bd2db33
- Match the Steam Knight core contact and bounded damage routines. cca464adb4f267b35763533b75a3d2b99344aa51
- Match the mine numeric HUD setup and drawing routines. 1cf8c907d3bacf589794a2737929f24b35ca6ef3
- Match the forest player movement action with its controller and model state. b2c5a778632a5c4189d4431b921a012d786f2722
- Match the mine stat award actor and its three service wrappers. c406867414bd3ad4add33cce4dc348fdbad0eb5a
- Match the forest impact actor initialization, update, and expiry. 11931b4cd46696af05ca32c142b631aa100f9e3d
- Match tower actor allocation and preserve room-specific selection. bbca2aab63465f8b343a602cd1f32e1853cbd9db
- Recover mine award setup and verify packed numerals across every column. 54f17c9b02cdaafddc914e574edfad86b55cd5a5
- Match the seven tower body and marker routines. bb20c8ba38c9f746d7064b4f4f2ac71d4a9d10c3
- Match the forest player frame initializer and preserve ordinary dispatch. bdfcbbac5e50bbb2b2050e1bb974d8ddf8e7cc53
- Match the remaining seven mine award controller phases. ba9b45d5de0c5798be65aa61d2c4d2274d198464
- Match tower animation refresh and two opening routines. 08e81e1f12998ce988fc4fdec2415363a2110dc1
- Match the mine model sequence evaluator and its regional table. 18c04a2224d3a40d5605fd9dd9986e362dfd43d2
- Verify C recovery against assembly emitted in the target procedure. cb3358755401acdd34db4bc00d37d3a0f5e8240e
- Match four tower armor actor routines and preserve their room selection. ff8c75122ed51b9778275e091526ce0f92b295f0
- Match mine terrain plane distance and edge clearance tests. ced4629e6c319ba859b2c0c872479676f17624e0
- Recover mine award actor lookup, phases and movement services. 6590c7e110e1c6fe8db2f3ef74495f6413e33b76
- Match the Steam Knight moving armor attachment tracker. ab83adf5fe7550bea8132ee6231b7ad32618052a
- Match the forest kind-four player selector and contact action. dcbd7bbbb60e54847f2308eb641829ee64cb5b30
- Report coverage from split public match registries. 34b2fc91ff3acbd540dca0ddefbc356e328b977d
- Match the mine script action queue and stat award request. ceae97d8a86bcf90ec1059f6525e0a3e5b59beb3
- Match the Steam Knight auxiliary reset and body phase services. 09f327afc305d6b33c6abdb707f721f3eecffc84
- Read bounded ranges of files on the mounted game disc. 8cf8cd8399db1a8a54023568096292e8957c9f2d
- Read the forest player mesh from a mounted game disc. 43003f0dd35d34b7fb86355a0e3faeb3aa552ea7
- Match the forest attack strength calculation. 4f3e69f899347fad37d2eb8a88c63ef86a5a79db
- Match five Steam Knight body turning and phase routines. 0eefda43a45ccab6a9c497b71165c296417a4479
- Interpolate polygon texture coordinates from the quantized left edge. 9472890be8811b57a0d9ccb2d65e7b46b37f1d8d
- Use measured 12-bit steps for horizontal texture ramps. 90f2fa35a66b453b51a13d7e0751698fa045f073
- Match the mine award model and signed local pose reads. 9a2b6af76fde9bb8305326b423dfda6aea41724b
- Match Steam Knight joint setup, dispatch and allocation with typed model records. 15ad96932bc6f3f8ebdbe50204eb8ca5ebcc4b0c
- Match the initial mine placement list and free actor selection. 0cc137f6e0d0128230c44f4c0c291109ca11de6c
- Round horizontal undithered Gouraud colors at the measured precision. 77ca2f929b0fd9e64cd5ad2b0264858bcc6fffc0
- Match Steam Knight state transitions and preserve the signed collision result. 6f8fda751fc54447e06a24cfbfbf785807238471
- Match the mine actor world velocity and vertical step. 9b64b9ceaea3eeebee7fc7b657b523abd8c85b38
- Match the mine door phase and model allocation. 31ce4f9e8a68b5fbe2808f957d19b1d88597b872
- Match the mine field actor pool reset. 6a003678dbef9a00e27016afafa6dcd92b47e9c0
- Match the fourth-stage mine field initializer in SC02/0039. 612ccc05cb24cdcdaed05421a631ee01bc27e545
- Match Tower body action dispatch and shared duration timer. e9753b1d3ba6c666d64b3174d73ac2228e72f806
- Match the mine door actor initializer in SC03 member 0116. 64ad2e406a2d52348c85fc601b0d519bab78a980
- Correct opaque flat-line coverage for shallow GPU segments. 8f4a9d295a432a5b9a8f5fd1bb86efbf7b0aa4b0
- Match the room 3050 actor action dispatcher and pending movement. 45be0b40a38241c230cc97a2abcc599e54d14bc7
- Match the Steam Knight action evaluator and four decision routines. 3656cb5f1372d0e01c243f757719f88c8659ea5d
- Match the room 304C door actor phase dispatcher. 2e152be37b412a6eee2a03e15dfc717c8773e194
- Match two Steam Knight action predicates and retain their callers. 726cca34090f8e0f6361e5dfb0555214d5ddf8e5
- Match the Steam Knight joint effect and its typed actor allocation. f09a09b48b8fa4e9d86203c82e6984619ce71630
- Match the mine grid cell walk and its ordered face visits. d789de43c2b976941f8699fbad325e3f7f9cf220
- Match four Steam Knight resource routines and type their callers and test fixture. 7d40c0504983edadba945ffe7a04fded801dc632
- Match the mine terrain flag translation from its owned overlay. 351c8b20c64510f1dd466878083f2443b07aff75
- Match the Steam Knight actor cleanup and preserve its return type. 65363a4529936102dbb702d973be0f4ddbd56e7d
- Match the mine terrain response routine and its cell-walker call. 1b9495952d2c7156d2fca17c6ddd25bc7c39bbec
- Match the two Steam Knight intro countdown callbacks. d334a239379d6e1c8f099b370f2dda32890b2fbb
- Service pending GPU DMA and bind draw completion to the active graphics frame. 59ed3754137ecf55df044ef185868a1333b13905
- Recognize completed GPU block uploads on the active draw interrupt. 047f04e7b897261ca18c67bdb5a9e5112d81de28
- Match the Steam Knight turn callback and its player request helpers. 7d3a3e85f21500010e977d91aa5ce5bb192d3675
- Recover mine actor step and its full-width terrain vector service. cd312eb3edf5fe52d31ccf7b5030e2779c2919b3
- Service the audio channel countdown from its mapped VBlank state. 3564df9e009f8a61a80360232dbd637cdd121d0c
- Match six Tower camera routines and verify their transition caller. 1fb95c2b1380d78f2748913073049cd6b962e108
- Recover the mine child spawn with the shared actor allocation declaration. aa8c885c501879c93d1d19a229ffd37891d2b50e
- Match camera setup and dispatch in two mine rooms. 8d796410fddb6d544f3f4cf53d4fb1820bd350b4
- Match the mine emitter and child stage selectors. 279108c38b951fe0e59342c3223ceeffab734098
- Deliver completed CD movie buffers through the registered callback. bc9fdf97db8a7e46df5c169e09a3712745915837
- Match the Steam Knight handover and its actor reset helpers. 1d15fbb47865b502b19d57292bcdd70938ae0570
- Match the mode-zero camera updates in two mine rooms. 6f9d7d0f811676859fff406320175911e5aae685
- Run the mounted movie output callback on completed native MDEC and GPU transfers. 8368a725e136256147cc249d0efa4c233ed97281
- Round native MDEC color output to the nearest five-bit level. e31823f3c066376c47f4eeaf572d6cd7a9736909
- Register six existing forest scheduler matches in their SC01 overlay. 366fc26516941082218d03610b70e45f557adbb5
- Match the forest enemy and its child actor in the SC01 overlay. 1ea3117804712051e45fbc5990ed0959caa2849b
- Saturate decoded colors after signed nine-bit conversion. 8e6503d04324457d7a9dd2a9e6cb7d6e7afa849e
- Retain fixed-point transform precision through MDEC pixel packing. 1b02587e038895ae2a7e14571882c6da1de232b8
- Match mine emitter initialization and child model feedback. bc9aace983b99865c9b34680d3e12393bd0da78e
- Match four forest attack routines and align the sound command interface. b392f05e70791ef829ac352c20daac2109b793d5
- Match the mine child model segment query. 1e2ecb6c466833caca8a182555fe9a9273b3d8ef
- Match the forest actor diagnostic selected by kind 218. c4b67103479bc3f4d4fa5a5991e521a398221624
- Match the mine child actor setup and model allocation. 0bec1dd40b8a77ddb5ddf85d1eeb6a628bc76bc2
- Register four forest routines and match the child allocator caller type. cc7c0437879aa1986c8dbd644812b380dd6d72a8
- Match the mine emitter selection and preserve the resident audio ABI. 4bb427c05d571b438edd586d1fde9f678c900972
- Match the mine emitter projection wrapper. 1851a4208138dd453b6942b2b882984eb1587d21
- Match the mine room 304C light actor state selector. 8472d6f2f297e9b77c16de0de23e4cade932b88c
- Match the mine light actor first update with the resident light setter. 676d4e7fc978cb77f655c7ed9954e45dfcacaa99
- Match both mine room 304C type 334 actor update stages. fc8208941b2f5787f5eec30a95cace773e0c66cb
- Match the paired mine model actor selector and setup. 874c39497c45a8cec5e2e7742d4eb32dd3ca32a0
- Match the mine stage gate for actor type 33A. 68de17b0085e66ba67ded448db22b8347971d403
- Match the 304C mine event dispatch and first phase. 84b0c0e0298ab48d4f9f1cf908aaa2392ef62715
- Match the first phase of mine actor type 333. 3970a02c86004101aeb539b739437325e1347491
- Match the forest class 133 dispatcher alongside the Steam Knight overlay. 8d18f8a235a405313c25d1d4dbef6f7ef4fa9007
- Match the sixth phase of mine actor type 33A. 393c6fe6a64e581ab9272896bb681ec20b015a81
- Match the mine ring timer reset and stage update. f1a5501a940377c4675e23fc4830679de013a609
- Find configured memory cards and retain existing Continue saves. 272f24da11dd7e3f303579164f71960b3ea28e50
- Retire buffered movie DMA after its decoded strip reaches memory. df65a330a3b05b0b4e3230f00bacb58f630e81f4
- Run the opening movie output callback as native host code. 3d09f1430af266216f1a8264b02c432aea9be489
- Match the village porch quest actors and resident event-flag setter. a9c96fa6792a1833bf920f012e17213518d2ea9b
- Run title sequencer and mixer updates during Continue navigation. fcc711a7f4efbb4a52996f4e65577052649c0fdc
- Match the scheduled village field actor transition. d272bf44d59c9550deeacdce9aaa55b9c001b0c4
- Handle the mode-3 title sound callback when Continue opens. 614a69b4cb4afe81a021f528820d324a5e320871
- Transfer buffered GPU image words to RAM on DMA channel 2. 9a405b7aacbaed9bd04bd1aeac8669ad434fc8f6
- Match the five-phase village actor update. a2220e49336b3096e1a6ab5ee914e9d50f4a6f4d
- Match the game disc audio playback report callback. 18696c57bda68b12683a17698675b71d349030d8
- Match the saved-file status panel builder. b6f6a6792f29c040ff43af7e7cbcc89d727924dd
- Match the saved-file decimal formatter. 971d1565aff22e89fd7a4151071a433445f6f749
- Match the village actor waypoint advance. 35e1edebff425f40e7695d9cfd0bd8a11cf77340
- Verify the existing File menu glyph match and retain timed guest writes. f4e4df3f475360f9e3b0da270657dcd62e15d1f8
- Match the village floor selector and retain guest memory timing. 2482f23988c75e79dee9aceb0805565757cc520c
- Restore the File 1 card payload and match the village guard at Twinpeak. 83012998d13810c7aafcb8ee15fa8e2b1e780c44
- Match Restaurant’s Macho and the daytime Twinpeak gate. 3b569ed7014a67edec55ffaae0a2fd8df7f716a1
- Restore the selected save through the mounted copy loop. 04b64f5831b096c14e70e7544bfd19b0eed079d6
- Match the forest Bincho Field and Guard Lumpwood rescue. b9e02afea744fc111f9d53a2f6c82fcb61ce5367
- Match the village stocks actor and its story branch selection. c1cce445c71c54f0be5918861d07cb01ece3880b
- Match the Twinpeak Fusion throw and request queue. 5d87c58fbb74dddb9e16b292ded14062f9e7ccf6
- Dismiss the restored file in the mounted load-menu callback. 964bca20be3ed1a1cd81f7bb0622f57b1c6bae3a
- Read and display the mounted save rows across all four slots. 7a88d44bb4fec1e92c17ca2966ff13832f11a23a
- Follow the completed save transaction into the mounted room initializer. ff943a85984865df4162a224142bc834273d0dc3
- Keep the SC01 opening scene on its own fetch path at the file callback address. c5f6e2f0f1b86b7b6a46a6faa57cfee06fefce6d
- Match the room change and object reset after loading File 1. 48b8a3f98fc4622cc2df79f04d70911c2fdf4f88
- Match File 1 room setup and three Twinpeak actor routines. 8e634d47d0e33f037cae489a53a4a70618ee31c2
- Match the cemetery script that awards Jon's Key. 097968f288985c50bd37117ae71ef66d6a499d02
- Restore the earned File 1 room through the owned scene callbacks. 9a45e85397060740386922d649aa6aa65a0b2135
- Match the cemetery opening and Twinpeak contact poll. da3739b78e46fa5c8c8868ba9f381c3ade499a1c
- Run the registered game master fade through native clocked operations. 88ffbfa6af49ede809561905a8b9deecb73f6819
- Match the forest rescue BP award and refill. ea5767f90f02be97f41d3e298f3fad80935caa1a
- Match the village actor callback after Jon’s key is used. d9ac021f5ad826dc24e3f2d2fde3f62144b4f674
- Run the mounted File 1 room latch in native foreground steps. df0199542ebd403cb3f3449f452ea20d425c4862
- Match Twinpeak Fusion contact setup and handle opening. acd19a08abb209efd175417308d393f378a155eb
- Match the initial Twinpeak Fusion action and document Windows and Linux testing. 654b5bf333037d96e6915d147c7fc54050d30e5c
- Match three Twinpeak tutorial handle routines and record the two-slot Continue result. 1ee5ec4815260aca4d8777eb6112cbfe442bd439
- Match Twinpeak tutorial setup and contact decisions, and document both player builds. 43000299588d50905e7324768155dcb9995568cd
- Show the full launcher log path in Linux and Windows error dialogs. f8eb164fab163bee18e14f6485b181c316dfdedf

### Subsequent public source checkpoints

These source commits came after the Windows and Linux candidate archives.

- Match Twinpeak coordinate checks and document both public player candidates. 782fe6f3aeac936ba83a8fd6138d20dc31e75ca9
- Match the Twinpeak BP debit and document the four-log raft quest. 292737ec392eba20e15a6be73ac41eb726766469
- Match repeat Fusion BP credit and document the river checkpoint. 36cb9e48c4c960753d7be144fdeb63e52b57f288
- Match the Twinpeak cast cost and document the completed river course. 6cf4715b36c3116e2acca2fcbfdaadb0de0d668d
- Document the earned Bracelet save and the served Shrink cast. c5c5e8b69e35015426a2979615f7e1c92d0c4f5e
- Register Twinpeak inventory matches and document the L-Brace appraisal. 043573bc39f83bd7db349c600aed5ce374d2911e
- Recover Fores’s cottage conversations and document the File 4 playtest. e0e01a5986dcc2bf85d44ce9db0cde58569c0298
- Match the two village interaction checks for Fores and record the Steamwood Manual playthrough. d3f30461859c49ab7eba3b00575c5f581b8098f2
