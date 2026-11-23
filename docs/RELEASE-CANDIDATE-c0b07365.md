# Brave Fencer Musashi for Windows x64 and Linux x86-64

**Candidate `c0b073650` · unpublished · November 23, 2026 UTC**

Both packages run the master sound fade as compiled host code. Both accepted New Game and then stopped on a black screen before the opening moon. Both loaded an earned File 1 and stopped before the bedroom appeared. Linux still displayed NOW LOADING. Windows under Wine displayed a black screen. These failures keep the packages in local testing. A public tag and anonymous binary-download checks remain pending.

## Downloads, verification and known issues

The local archives were built from `c0b073650260724f2f01e1eff8a5ba2229434f6a`, tree `cbe7d8682a1febf6163e72e25b7fc58d203d8376`. Anonymous GitHub binary download checks and a release tag are pending.

| Platform | Archive | Bytes | Local SHA-256 |
|---|---|---:|---|
| Windows x64 | `bfm-windows-x64-c0b0736502.zip` | 3,869,217 | `8edbd43605740bd0c2526f554dad52fe1b03cf9dcad5745f64b5e5ee8b23911f` |
| Linux x86-64 | `bfm-r106-clean-qa-c0b073650.tar.gz` | 4,702,521 | `f9c7db50ac0b4d1152ffa8461049d7847018ab8baeda3ba2b5b6068b67bac200` |

Compute the checksum with `Get-FileHash -Algorithm SHA256 .\bfm-windows-x64-c0b0736502.zip` in PowerShell or `sha256sum bfm-r106-clean-qa-c0b073650.tar.gz` on Linux. The R106 filename is the Linux packaging label.

| Area | What was tested | Result |
|---|---|---|
| Linux x86-64 on this archive | The regular player built and passed its mounted file-menu check. The registered interrupt suite passed all 92 cases. | The ordinary player loaded the earned File 1. The fade reached its target of 16,384 and cleared its active flag. Room 3001, entrance 0 stayed inactive at progress 2. NOW LOADING remained on screen at presentation 1579. The player stopped on interrupt delivery 8,812 at target `0x80033D28`. All twelve scene halfwords, the entire 2 MiB of main RAM, scratch RAM, CPU boundary and final image matched the parent run. The US original restored Musashi in the bedroom. The next row records a separate cold Continue with two occupied files. |
| Linux cold Continue with two occupied files | The card contained an earned Chapter 1 bedroom save in File 1 and a later save at Hotelo’s inn in File 2. The Linux player accepted Continue and showed the memory-card reading message. | The player exited with status 2 at `0x8017CE2C` before displaying either slot. The card stayed byte-identical. The original US game and private hybrid R100 restored File 2 outside the inn with 150 HP, 88 BP and 1,510 Drans. Windows has yet to be tested with this two-slot card. |
| Linux New Game on this archive | An ordinary player started with a blank card, pressed START at the title and confirmed New Game. The loading text appeared at presentation 1149. The original US game showed the opening moon. | The last lit frame was 1490. The following 178 presentations were black. The player stopped on interrupt delivery 8,997 at target `0x800338D0`, with the foreground at `0x8004250C` and story progress 0. All 177 retained images matched the predecessor at the same frame numbers. The card remained blank. |
| Windows x64 on this archive | The release executable was independently linked and all 92 registered interrupt checks passed under Wine. | The regular Windows executable ran under Wine 10 on a fresh copy of the earned File 1 card. After the third Cross, it stopped on interrupt delivery 8,879 at target `0x80033D28`. The last nine rendered frames were black. Room 3001, entrance 0 remained inactive in mode 4 at progress 2. All twelve scene values and five fade fields agreed with Linux. The original US disc restored the bedroom and played the wake-up sequence. A later directional-input test demonstrated free walking in the village. The source card stayed intact. A new save and restart, physical Windows hardware, controller and fullscreen remain unverified. |
| Windows x64 New Game on this archive | Under Wine 10, the regular executable created an empty card and accepted the two ordinary title and New Game inputs. | After loading, the last 64 captured game images were black. The Windows client exited with code 2 after 8,967 interrupt deliveries at target `0x800338D0`. Room 3005, entrance 0 remained inactive in mode 8 at story progress 0. The Linux archive stopped at the same target. The original US game reached controllable room 3008. |
| File 1, parent `412da90e` | Linux and Windows/Wine loaded an earned save, accepted the final Cross and restored all twelve scene fields. Three Windows links reproduced the archived executable. Five component checks passed, including six clears, 832 stores and 3,597 guest steps on both memory mappings. | Both stopped at `0x80033D28` with room 3001 inactive at progress 2. Linux reached mode 16 after 8,812 interrupts, with 783 loading-screen pixels lit at P1582. Windows reached mode 0 after 8,926 interrupts, with a black screen. The original entered the bedroom at progress 10. |
| File 1, `7e7554d85` | Linux and Windows/Wine passed the 128-slot reset and reached scene setup at `0x801378F0`, return `0x800D1D84`. | Both stopped with identical black 320×240 displays. Eleven CPU registers and seven room fields agreed. Windows counted 8,811 interrupts and Linux 8,672. Full RAM differed. |
| File 1, `88de76842` and earlier `e45d0e034` | Linux and Windows/Wine on `88de76842` read the occupied slot beside three empty slots. Two Wine runs on `e45d0e034` reproduced eleven CPU values and identical black displays. | After confirmation both builds selected room 3001/0 and stopped at reset `0x80129258` before the bedroom appeared. Earlier Wine was in mode 12 at progress 2. The card remained intact. |
| New Game, `412da90e` and `e45d0e034` | Linux on both builds and Windows/Wine on the earlier build accepted New Game on a blank card. | The players faded from loading to black and stopped at `0x800338D0` before the original opening moon. Foreground PC was `0x8004250C` on Linux and `0x80042558` on earlier Wine. Chapter 1 control and the first inn save remained out of reach. |
| Public sources `ff943a85` and `c5f6e2f0`, with a separate local File 1 comparison | The first public build and the local comparison loaded File 1 and stopped at `0x8002CE84`. Their complete RAM, scratchpad and final image matched. Public `c5f6e2f0` started New Game and stopped at `0x8002CFAC`. | File 1 did not reach the original bedroom. All 176 retained New Game images matched the earlier candidate, including 17 black frames during the original moon scene. |
| Four occupied files, anonymous source `9a45e853` | The regular Linux client was built directly from the anonymous download with the documented dependencies and a user-supplied disc. | It displayed Loading Data and stopped at `0x8017CE28` before the picker. The final image matched the preceding failure. All four files remained intact. |
| File 1, public Linux `88ffbfa6` and a separate local build with the same fade | Both loaded the earned save and accepted Cross. The fade advanced once, from 8,192 to 8,738 toward 16,384. | Both stopped at `0x8003392C` after passing `0x8002CE84`. Public Linux counted 8,669 deliveries with room 3005/0 in mode 12 at progress 2. The loading confirmation remained visible. Full RAM, scratchpad and final image matched. The original restored the bedroom. |
| Four occupied files, `412da90e` and earlier `cd089b62` | Linux on the parent and Linux/Wine on the earlier build selected Continue. On the parent, the reading box appeared at P1413, with the full message at P1420 and a matching last image at P1510. | The Linux parent stopped at `0x8017CE28` before the picker. The display count stayed fixed through P1514. Earlier Linux and Wine also stopped at the save icon. Private R110 loaded File 4 from the same card. |
| File 1, a follow-up local Linux build | During a cold Continue the outer room-load routine ran 28 positions in compiled host code and returned once. | The candidate again stopped at `0x80033D28` after 8,812 interrupts. The final image, 2 MiB of main RAM, scratchpad and twelve scene values matched the packaged candidate. At P1585 the screen still showed NOW LOADING. The earned save remained intact. |

Controller in Steam Game Mode, borderless fullscreen on the owner device and automatic on-screen keyboard on text entry are all **NOT RUN**. Windows hardware has yet to be tested.

## Native execution and decompilation

The package installs 204 of 247 resident dispatch candidates and zero generated overlay entries. The room reset, mounted scene setup and master sound fade run in host code outside that generated table. The 164-byte fade block was recovered by compiling previously published first-party game C. It follows the game’s unsigned 16-bit rise, fall, wrap and clamp rules, preserving the timing of individual device advances. These handlers preserve interrupt points and individual memory writes. The production game continues to use the guest MIPS interpreter elsewhere. A fully native chapter, soundtrack and save/reload playthrough remain outstanding.

## Changes since the last public binary release

R55 was a Linux prerelease. R48 was the last previous release with a Windows build. The R55 tag points at README commit `ad71556b44506599fb08e654e28651d81eba7206`. The R55 release notes identify the Linux binary’s source as port commit `ba90b87ad4`. The full history below compares the tagged **public source tree** to this candidate. R55's published 591-of-629 native count used a different set of functions, including ability-module entries. The two counts do not establish a native-coverage trend. R55's scripted opening and memory-card results also describe that older binary. [R55 release record](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/releases/tag/bfm-r55-ba90b87ad)

- **Clean-room source and execution.** The public history adds independent C reconstructions, compiles the native library from the public registry, and records which resident functions can safely run on the host. It withdraws SDK-derived routines and removes embedded scene address tables from the interpreter. Disc-backed services, interrupt-sensitive paths and unmatched code retain guest execution.
- **Graphics and cinematics.** Changes cover texture windows, draw/display pages, polygon edges, fixed-point color and texture interpolation, flat-line coverage, GPU image transfers and draw completion. The opening movie gained a native output callback with changes to MDEC color conversion, DMA timing and GPU upload. The earned-card tests establish which pictures were actually displayed.
- **Sound, disc and timing.** Changes cover mounted disc reads, streamed-audio callbacks, sound-bank and track selection, DMA transfers, ADPCM sample history, fades, key events and timer/interrupt timing. Four earlier commits preserve the restored File 1 through a fading sequence, service the sound transfer query, run the opening music sweep and refresh the selected sequence slot's voices when File 1 closes.
- **Menus, stores and memory cards.** Source changes cover shared SDL input for menus and gameplay, wallet arithmetic, shop purchase messages, inventory operations, saved-file number and glyph rendering, configured-card discovery, directory reads, the card metadata record and File 1 payload restoration. This candidate adds independent status, summary and selected-file handling for all four slots.
- **Forest, mines and tower.** Matched source includes forest ground and collision tests, bridge trunks and crossing stages, player movement and attacks, tutorial actor setup, Bincho Field and Guard Lumpwood. Mine work covers terrain and camera queries, door and light actors, model and animation dispatch, item awards and HUD numerals. Tower work covers Steam Knight armor, damage, particles, camera and state transitions. These are decompilation milestones. Chapter completion on the download remains unverified.
- **Village and story actors.** Source now covers the village porch and its event flag, waypoint updates, the guard at Twinpeak, Restaurant's Macho, the daytime gate, Jon's stocks actor and its story branch. The private gameplay table below identifies which of these changes players actually encountered.
- **Packaging and platform support.** Both platforms support multi-track disc paths. Linux gained relocatable library loading, package provenance, minimum library-version reporting and game-library disc discovery. Windows gained path handling for package and profile folders. Renderer changes preserve the original picture aspect in desktop fullscreen. Device testing remains open.

The preceding builds distinguish the opening cinematic from the file menu where their code shares an address, handle a mounted forest disc sound-release callback, and add the room-object reset. The parent recognizes a foreground menu that remains active through the reset and checks the two saved returns before resuming. The new scene setup clears six buffers, restores the room parameters and returns to the mounted caller. The parent Linux build displayed the subsequent loading screen. The SC02 scene uses its own implementation. The itemized history below covers the exact source of the two archives.

## Gameplay verified on separate private builds

These tests use distinct executables. The platform table above records exactly how far each local archive ran.

| Build | Ordinary gameplay observed |
|---|---|
| R100 | Musashi picked up Lumina, escaped across the rooftop, entered the castle and regained movement. Four portions of the captured stereo matched the owned disc’s chase music. The player then charged Lumina, spun through the ring of fire and resumed the throne-room conversation with Rootrick and the princess. Musashi broke the Steam Knight’s armor, depleted its palace health gauge, lifted it and threw it through the castle wall. In the avenue, he reduced its second gauge to 48/96 before losing his first life. On the next life he depleted the avenue gauge with 65 HP remaining and threw the boss into the town. In this third arena, he reduced the body gauge from 96 to 64. On a later life, he reached 53 of the original 96 displayed gauge pixels in town before he lost his remaining HP. On the fifth attempt in town, two hits lowered the displayed gauge from 64 to 12 of 96 pixels. Musashi had 75 HP after the second hit and later fell to 0 while the boss still had 12 pixels. The next life began in the palace at 150 HP. On the seventh run from the blank-card character, Musashi cleared the palace with 140 HP, the avenue with 100 HP and the final town arena with 70 HP. He lifted and threw the Steam Knight, watched the ending, accepted the save offer and used File 1. The screen displayed Saved data! Both game checksums and the Lumina equipment bit verified from the saved file. Slots 2–4 remained empty. The same character selected Resume, saw the Chapter 2 title, entered bedroom 3001/0 at story progress 10 and reached the original wake-up position on the bed. Guest CPU execution remained active during the route. |
| R101 | An ordinary character rescued Leno, delivered him to the village, and returned to the Mayor during daylight. The Mayor thanked Musashi and referred him to the man in the stocks. Five dialogue text masks matched the original US game. Private story progress advanced from 20 to 30. The player saved that progress normally at the inn, preserving the other three file slots. After buying RyeBread and obtaining WellH2O, he reached the occupied stocks. The first Square input there stopped this build before Jon spoke. |
| R102 and R103 | R102 independently reached the avenue after throwing the Steam Knight through the palace wall. R103 put out the ring of fire, depleted the palace boss gauge, and threw the Steam Knight through the wall with 20 HP remaining. |
| R106 | After Jon’s request, the player returned to Hotelo and saved over File 4. A cold launch restored the village exterior at quest progress 50, with 138/150 HP, 115/155 BP and 90 Drans. The other three files remained intact. On an ordinary playthrough from File 4, Musashi earned Hop at the upper sign, followed Leno from the green forest into the purple woods, and defended him from flying enemies. The six dog-led choices were left, left, north, east, north and north. Musashi and Leno reached the cemetery together. Musashi opened the northwest tomb, picked up Jon’s handmade key, and advanced story progress from 50 to 60. On the return trip he earned Hop again and crossed the first two thorn barriers, then lost his remaining HP at the southern thorns. The field Memory Box offered a paid continue and reported that it had no saved data. The earlier inn save remained intact. A second private playthrough on a second Linux system independently reached the six stones, opened the northwest tomb, and displayed the message confirming Jon’s handmade key. That character carried the key through the steam passage to the village. Jon declined the handover in daylight. On Sunday at 04:00, Musashi used the key at the stocks and Jon stood up and thanked him. The village advanced to story progress 70. The pause screen then asked Musashi to cut four trees and bring the logs to Jon. Musashi later died at the farm stream before reaching the inn. The field Memory Box offered a paid continue and reported that it had no data. The prior File 4 survived. On another visit Musashi banked the key at Hotelo at progress 60. The save showed 74 HP, 48 BP and 90 Drans. Its 74 changed bytes all belonged to File 4. Saving and cold-reloading the handover at story 70 remain unverified. On a separate route, the character rescued Hawker Steakwood with Lumina. Both the original game and the private build displayed his Minku advice and increased maximum BP from 155 to 160. The character saved normally over File 4 at the inn. A fresh launch restored the rescued Hawker and Guard, 150 HP, 144/160 BP, 10 Drans and the four purchased gels. The three other save files were byte-identical. |
| R108 | On an earned File 4, a charged Fusion attack in Twinpeak entered and returned from the compiled queue function in 95 guest cycles. A closer cast then hit the Magician, and the game displayed the Shrink award again. The character had already learned Shrink. |
| R110 | The player restored File 4, passed the farm, reached the west bank of forest room 3014/1 and earned Hop from the first Hopper. Musashi used Hop to cross both thorn rows. An armored enemy hit him, and one W-Gel restored his HP from 105 to 150. In a separate ordinary encounter, Musashi earned Satiate, hit the pink Hopper with a charged cast and earned Hop a second time. Both tutorial pages completed, and normal movement resumed at 150 HP and 146 BP. |

An earlier private R111 candidate compiles the charged Fusion throw’s preparation and return on the host CPU. Its remaining game services use guest execution. Both private Linux configurations built. On the ordinary private build, the character loaded File 4, hit the Twinpeak mage on the first charged cast, earned Shrink again, dismissed both tutorial pages and returned to the field. The new host throw handler recorded zero executions. The allocator declined its one call with 136 guest cycles available. This cast ran through the hybrid fallback.

The subsequent private Linux R111 build restored the same File 4, hit the Twinpeak Magician, displayed the Shrink award and both tutorial pages, and returned to field control. Its charged throw executed 26 compiled blocks over 587 guest-clock cycles. The queue allocated the first free slot in 95 cycles. The owner-only launcher served R111. The independently downloaded owner-only archive displayed all four file slots, accepted File 4, showed “Loaded” and restored the village. The downloaded copy also hit the Magician, awarded Shrink, displayed both tutorial pages, and restored field control. Its throw ran 29 compiled blocks over 613 guest clocks with 12 returns. Its allocator again finished in 95 clocks. The test preserved the earned File 4. This private build contains historical code excluded from the public distribution.

On a fresh private R102 download, the fourth town fight left 11 of 96 boss-gauge pixels at Musashi’s death. A separate R103 download left 12 of 96. The US reference completed the fight. On R103’s sixth attempt, Musashi cleared the palace arena with 145 HP and reached the exterior. The palace comparison showed hits of 53 and 51 on the opposite ankle and the core gauge falling to zero. On that sixth attempt, R103 also depleted the avenue gauge with 75 HP and reached the town arena. The Chapter 1 save described above was earned on R100.

The owner-only Linux launcher subsequently selected R112. A private download and all five launcher checks verified the cutover. On the staged package, the same earned File 4 restored the village. Musashi reached the Magician, used charged Fusion, received Shrink again and returned to field control after both tutorial pages. The post-contact poll ran 219 waits and one advance in 227 host blocks, totaling 13,924 guest-clock cycles. Fifteen deadlines declined native work. The advancing visit, including its delegated services, took 3,239 clocks compared with 2,483 on the US original. The original timing of each service inside that visit and the corresponding interval on R111 remain unmeasured. The pre-cut test retained all saved-card bytes. A fresh launcher download independently loaded File 4 and repeated the charged throw, Shrink award, both tutorial pages and return to play. It recorded 219 waits, one advance and the same 3,239-clock advancing visit. The full card was preserved.

Private R113 then ran the end of the charged Fusion animation in host code. The staged Linux build loaded File 4, hit the Magician, awarded Shrink again and returned to idle. It recorded nine continuing animations and one final return, with 23 host blocks, 524 guest-clock cycles and 13 calls to guest services. The final return took 16,697 clocks versus 17,328 in the original. A 12-frame directional press switched the actor from idle to its moving action, and releasing it restored idle. Two landmarks in the video did not resolve how far Musashi moved. The earlier polling phase recorded 224 native waits and no native advance on this cast. The normal quit preserved the card. The owner-only Linux launcher subsequently selected this exact R113 archive. The downloaded copy matched the staged package.

The downloaded R113 Linux launcher build independently loaded File 4, struck the Twinpeak Magician on its first charged cast, awarded Shrink and returned control after both tutorial pages. It ran ten continuing native animation calls and one final return to field control, matching the original call counts. During a 36-frame directional press, two stationary terrain features confirmed southeast movement over a 15-frame interval. Camera rotation prevented a measurement across the full press. The ordinary quit preserved the card. The private comparison includes 20 captioned pictures and a 94-second stereo film.

On private Linux R114 an earned File 4 showed the Guard rescued and Hawker still missing. Musashi broke the forest crystal with Lumina, heard Hawker’s Minku and Longevity Berry advice, and saw “Max BP increased by 5!” BP changed from 139/155 to 160/160. The US original went from 147/155 to 160/160. The post-rescue roster showed both characters, and closing the game preserved all four save slots. The compiled actor, maximum-BP update and refill each ran once and returned. Dialogue used the guest interpreter. The owner-only Linux launcher served R114. A full download matched the tested package. Private R115 ran charged Fusion contact preparation and handle opening in an ordinary Twinpeak encounter. Preparation took 88 guest clocks on both this build and the US original. Opening took 8,804 clocks on R115 and 11,695 on the original. R115 then displayed the Shrink award and both tutorial pages. The ordinary player returned to the field. R116 packages add the initial post-contact action setup. Both executable profiles in the R116 Linux package passed their local checks. Ordinary play on that package remains pending.

A separate interpreter-free Linux build binds the live CD service to its guest-visible table entry and reads it during an ordinary cold boot. The next verified revision executed the selected native syscall, entered its exception handler and saved eleven values through clock 954,214. It stopped at the following register transfer. The original still showed the boot logo. The native build displayed black and has yet to reach the title.

## Supply your own disc and start the player

Use a personal dump of your legally owned **Brave Fencer Musashi US PlayStation disc (SLUS-00726)**. Keep the `.cue` and every `.bin` it references together, with filenames matching the cue sheet. Single-BIN and multiple-track dumps are supported. A plain 2048-byte-sector ISO lacks the required CD-XA data. No BIOS is needed. The player supplies its own kernel services.

1. **Windows x64.** Extract the entire ZIP to a writable folder. Keep `SDL2.dll` and `OpenAL32.dll` beside `musashi_native_boot.exe`. Run `launch.cmd` and select the cue sheet. You can also place the disc in a `disc` folder beside the launcher or set `BFM_DISC` to the full cue path. The launcher remembers the selection in `%APPDATA%\bfm-port\disc.txt` and writes `last-run.log` in that folder. The display path uses OpenGL 3.3. The observed Windows tests ran under Wine.
2. **Linux x86-64.** Extract the entire tarball. Put your disc in `disc/` beside `launch.sh`, or in `~/Games/brave-fencer-musashi/disc/`, then run `./launch.sh`. Alternatively, run `BFM_DISC="/path/to/disc.cue" ./launch.sh`. The log is `~/.local/state/bfm-port/last-run.log` under the default settings. The archive's `manifest.json` lists the required host libraries. Its imported symbol versions include glibc 2.38 and GLIBCXX 3.4.32. Working display and audio drivers are required.
3. **Controls.** The arrow keys move, Return presses Start, C presses Cross, X presses Square, V presses Circle and Z presses Triangle.

## Why earlier binaries remain under review

The project checked 22 distinct older Linux and Windows players. All contained a 36-byte game dispatch table and nine original scene-address sequences. Nineteen also contained a warp callback, and 14 Linux players carried two original diagnostic strings in their debug sections. The public source removed the affected tables, reads the dispatch data from the user’s disc and leaves the developer warp list empty. A scene can stop if no supported execution path remains. The R106/R107 private gameplay builds also include 16 SDK source implementations excluded from public distribution, so those private results cannot qualify their executables for this release. [Project’s historical download audit](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/blob/c1cce445c71c54f0be5918861d07cb01ece3880b/README.md#player-downloads-and-source-builds)

## Archive contents and source origin

This Linux TAR contains 27 files. The Windows ZIP contains 30, including its corresponding OpenAL source. The Linux archive and its complete TAR stream were checked against 902 media and BIOS references. All 574 intersecting ranges were traced to SDL, OpenSSL, zlib, or compiler-linked formatting data in the player. No intersections crossed member boundaries. The Windows executable’s 693,760 bytes of machine code and 250 import slots were accounted for from the source and toolchain. For Windows, 23 physical ranges accounted for all 801 raw match occurrences. A scan of the outer ZIP, the complete nested OpenAL source archive, and decoded content covered 4,755 distinct streams. All 1,043 match occurrences were attributed to SDL2, OpenAL, compiled SHA code, or supplier and license text. The outer ZIP and compressed source returned zero matches. Two format strings in the stripped OpenAL DLL matched its official source. The attribution to a particular source file uses the adjacent tables. These counts include reference files with shared content. Both local packages passed the release script’s retail-data guard. The matching Linux OpenAL source and checksums have been staged beside the archives.

Linux uses Ubuntu's OpenAL Soft `1:1.25.1-2`. Its corresponding source consists of `openal-soft_1.25.1-2.dsc`, `openal-soft_1.25.1.orig.tar.gz` and `openal-soft_1.25.1-2.debian.tar.xz`. Their checksums are recorded in `SHA256SUMS-openal-soft-source`. Windows bundles its matching OpenAL Soft source and notices. The port's own original contributions and the third-party components are credited under their respective licenses. [Public source exclusions](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/blob/c1cce445c71c54f0be5918861d07cb01ece3880b/EXCLUDED.md) · [Project and library credits](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/blob/c1cce445c71c54f0be5918861d07cb01ece3880b/CREDITS.md)

The matching Linux OpenAL source is available from the project’s existing public release. All four files were downloaded anonymously and checked against their recorded SHA-256 digests. [openal-soft_1.25.1-2.debian.tar.xz](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/releases/download/bfm-r55-ba90b87ad/openal-soft_1.25.1-2.debian.tar.xz) · [openal-soft_1.25.1-2.dsc](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/releases/download/bfm-r55-ba90b87ad/openal-soft_1.25.1-2.dsc) · [openal-soft_1.25.1.orig.tar.gz](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/releases/download/bfm-r55-ba90b87ad/openal-soft_1.25.1.orig.tar.gz) · [SHA256SUMS-openal-soft-source](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/releases/download/bfm-r55-ba90b87ad/SHA256SUMS-openal-soft-source)

Public source now includes the room-change routine and the reset that clears the type field on 128 scene-object slots. Both functions were rebuilt from independent project C, matching 148 original instruction bytes. All 916 registered matches passed before publication. That revision uses guest execution for the transition. [Room change and reset source](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/48b8a3f98fc4622cc2df79f04d70911c2fdf4f88)

The next public source checkpoint adds five more matches totaling 600 bytes. Two initialize or clear room state, and three drive an actor’s action in Twinpeak. All 921 registered functions passed the matching and retail-data checks. The scene registry contains 612 functions across 16 overlay members and 96,176 matching bytes. The generated overlay dispatch list remains empty. [Public matching source checkpoint](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/8e634d47d0e33f037cae489a53a4a70618ee31c2)

The following public checkpoint matches the 92-byte cemetery script that awards Jon’s Key. The original script stores the item in the first available slot, sets the quest state and completion flag, advances the story to 60, and resumes the room script. The tests cover all 4,096 inventory occupancy patterns. That checkpoint verifies 922 registered functions and 96,268 scene bytes across 17 overlay members. A normal private game acquired the key; the new source match keeps the original guest timing. [Cemetery key source](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/097968f288985c50bd37117ae71ef66d6a499d02)

Public source `9a45e853` also brings the room-slot reset and mounted scene initialization to the public branch. The regular build passed four mounted File-menu and scene combinations. Tests exercised the direction of memory accesses, including a read substituted for a required zero write. All 922 registered functions passed the matching and retail-data checks again. An anonymous download reproduced all 3,790 source files. Restoring the bedroom in ordinary play on this exact revision remains unverified. The SHA-256 of the downloaded **source** archive is `1708aba47035820ad4cf8af3103d874423865282e86b02057a5e5f6d7edad61b`. [Room initialization source](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/9a45e85397060740386922d649aa6aa65a0b2135)

The public source subsequently gained four matches covering the cemetery tomb and Twinpeak contact poll, totaling 752 bytes. This raised the scene registry to 617 functions and 97,020 matched bytes across 17 overlay members. The public master fade then landed as 41 timed instruction positions in host code. All 926 registered functions passed the matching and retail-data checks. An anonymous download verified the fade checkpoint’s 3,799 files and modes, with source archive SHA-256 `48139200018e2402da82ec7486e05d833a5858e1fa5947a547d3dcfcb4355233`. Both published source commits and the two platform archives above have distinct Git identities. The SHA-256 in this paragraph applies to the source download. [Cemetery and Twinpeak changes](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/da3739b78e46fa5c8c8868ba9f381c3ade499a1c) · [Native master fade](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/88ffbfa6af49ede809561905a8b9deecb73f6819)

The public source also includes Hawker Steakwood’s maximum-BP reward and refill. The two functions match 56 and 60 original bytes. The playthrough from an earned private File 4 showed 111/155 BP becoming 160/160 after the rescue. The original game also finished at 160/160. Neither ordinary recording traced both function entries. This public revision still executes both routines on the guest CPU. All 928 registered matches and their retail-data checks passed. The anonymous source archive contained 3,803 files and had SHA-256 `684d940e286fc649077811e4b7b67a876b61b98bdc778ee79b17c856864d4123`. [Hawker’s forest rescue source](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/ea5767f90f02be97f41d3e298f3fad80935caa1a)

A later public source checkpoint matches the 80-byte actor callback that runs after Musashi uses the handmade key at Jon’s stocks. It calls two game services and replaces an actor part. During an ordinary private playthrough, Jon stood free, story progress reached 70 and the next objective requested four logs. That recording did not trace this specific callback. A different actor routine advances the story. The callback still runs on the guest CPU. A separate proposed 20-byte setter was withheld after the source-origin check found correspondence with excluded upstream code. The published checkpoint passed all 929 registered byte matches and retail-data checks. An anonymous download reproduced all 3,807 files and modes. Its 2,504,697-byte source archive has SHA-256 `989f065675e218fe4d56f278d6b4ec9eed6039ae18a4afece77360972673b876`. Both platform packages above predate this source checkpoint. [Jon’s post-message callback](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/d9ac021f5ad826dc24e3f2d2fde3f62144b4f674)

Public source `df019954` added native execution for the 28 instruction positions in the outer File 1 room-load handler. On an ordinary Linux File 1 load this revision stopped at interrupt target `0x8003392C` after 8,669 deliveries with **Loaded data!** still visible. The handler had zero native steps and zero returns. The complete main RAM, scratchpad, final CPU state, final image and card matched the earlier public comparison. A separate local revision reached all 28 positions and returned once before stopping later at `0x80033D28` with room 3001 still inactive. The US original restored the bedroom and played the scripted wake-up. In village room 1011/5, pressing Down and then Up moved Musashi and returned him to his starting position. All 929 registered matches passed for the public source. Its anonymous source archive reproduced all 3,809 files and modes. Source archive SHA-256 is `933853982ff588058e7cbf01942afbf6759c6c3c7348d14d51f11a7af96f6f58`. [File 1 room-load source](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/df0199542ebd403cb3f3449f452ea20d425c4862)

Public source `acd19a08` added two independently matched Twinpeak Fusion routines. The 172-byte routine prepares contact and performs cleanup on its abort path. The 88-byte routine opens the contact handle. All 931 registered functions matched and passed the retail-data check. The normal Linux binary had byte-identical executable code to the preceding public revision. Both public routines still run on the guest CPU. An anonymous download reproduced all 3,813 files and modes. Source archive SHA-256 is `e8058579fa5c80c7bb821db2ddbfc5232b913e19059e1225e3ae438178356311`. [Twinpeak contact source](https://github.com/Blizz127/Brave-Fencer-Musashi-decomp-port/commit/acd19a08abb209efd175417308d393f378a155eb)

<details>
<summary>Full source changelog · 301 commits, including three merges</summary>

- `e0b18220472a8ade8cae7229fa419e5164071db2` Source update 2026-10-29: 1,340 owner clean-room decompiled functions
- `713295a38606bc500003f7e24011fcf33a4466d1` Merge source update 2026-10-29 (1,340 owner clean-room functions)
- `5cd48bc13842d87ef2ed0a5ad4175a4b93499da0` Source update 2026-10-30: 18 more owner clean-room decompiled functions
- `cc7b14cdb8f26f3609e9ae535d01c31aa2c58433` Merge source update 2026-10-30 (18 owner clean-room functions)
- `45626d1c738662cdd435a9c7bc5431234581edcb` Port-source update 2026-10-30: host platform layer
- `5fc1cffb57051a65944910eb0307f39d42eb8538` Merge port-source update 2026-10-30 (host platform layer)
- `180007f62c90ea9bee9bc71c6972d6e39ed2230f` Match 155 scene-object routines across six USA overlays
- `b2cc54a2abb7372c040cf6e199be0b776efd6091` Build and exercise native angle and movement routines from public source
- `6a79e717fe87c267d9832461601a4b60bd57a0d0` Read verified LIST dispatch targets from the supplied disc
- `9a275090dee6553f20ac52fadfd2d0c8b628e5e3` Match the opening player-state callback and direction timer
- `085a746054118edab4913f0fa66c7d0d1e5ca8a0` Apply PS1 texture-window masks in the CPU rasterizer
- `016cec09c7623ce58764108b040b7f2a5314e388` Build and verify the native function library from the public registry
- `b8aa0c3a230f22a59bfe3ae43d6e6b6b9f629f15` Carry accepted polygon texture pages into following GPU primitives
- `4d1b91ff4e374852669ebb5ddf9402e0e10616b9` Match and test the timed player event state transition
- `3cf8709f86480ebf9394bbeffd94d1e47efad7ea` Share active SDL gamepads between gameplay and menus
- `ecd1dcad82a7b8062a370a0368b8e3e722711f45` Rebuild and exercise the opening camera registration and scene counter
- `08906cd55c2b925eb0d008ad399fa98fe014448f` Compute triangle orientation on the shared native geometry unit
- `dc57a6d1b45e484513a2bf0db6db2ba6c4941b60` Match resident inventory operations and test item acquisition and removal
- `f00fea5f1accccbd0e1b53ca417a3ff109cbbad3` Accept the one-word GPU no-op used by the inn save menu
- `d45d362e645a573391654642e4ee2b5b07039e90` Present the GPU display page independently of the drawing rectangle
- `7fd45d214bc65171ae850368e411bca0c8bacfe5` Honor borderless desktop fullscreen and preserve the original picture aspect
- `756c2f73f7d30923fabc53ff5776a2244ce5ddc0` Verify Linux package provenance and discover discs in local game libraries
- `fd7dc148f2855dc48af5d41b80bf9a20b76f4589` Rebuild the opening player event through its camera and release states
- `2da109f2ec473455a6a4cf1797739dfc33cb98a4` Rebuild the shop wallet and verify debit, cap and overflow behavior
- `d6d99065042cb99779c47954e263359e3630df60` Replace historical work notes and upstream test excerpts with reproducible public checks
- `350cb3d61570854337d57dfd14e429ecccf5910f` Recover the native wallet digit formatter and both HUD packet routines
- `960bb68aef3e591b086abc86a87f7290598fcdbf` Recover the room facing selector and its signed triangle tests
- `76eedac6970be2d76cb0d70d02fa1af680c20cc8` Check script-variable bus ownership before native loads and stores
- `88dee3329728fc7c4d14e0dbee1d765c09fe23cb` Recover the two title menu counters and verify their wraparound
- `29f2d55c2d8a1c42140a682880cc5f9ff40a0de3` Build the Windows player and verify multi-track paths on both hosts
- `a0f1771f73328796bf0bbb28ad350ee27214a0ad` Recover the bakery purchase message lookup and selection
- `6d39578f22e04ac9374fd639a84bf57767e4f8a5` Verify the card-menu item function and defer native execution at interrupt boundaries
- `a06f06c977ff6a0111562bff22bb07fe590fdf4c` Declare the streaming executable digest before Linux boot uses it
- `35de4f634261f37c60cc9d7d22bceae757169e16` Keep memory-card writes bound to the owning save session
- `7688df2ae0d88379ad43b55e13a943fb9fdc1ee2` Check repeated reference spans and decoded source literals before packaging
- `92b608720efbbbd5c43e105c3ecf693b809ac2d3` Match the three forest bridge trunks through their fall and settling
- `9eeb860d5dc82d621c67ff4ca37dc5a3e1b0c9bd` Link current Linux and Windows source builds from the download section
- `0a010dbc77762b0dad487393081539e1e5ab8b5c` Use exact tick offsets when the source already runs at the device clock rate
- `a4389bcef57e0a31a0bcebd10345c317a3d1c872` Run device routines from the player disc and restore interrupt-driven card I/O
- `c036e819d35a08f7c9dd4829ca018c2cb9d3c825` Cache fixed resident address ranges while fetching live instruction words
- `ed04764682febeb5f542a71eb89f2e5004c90e52` Match the camera update and defer facing routines that require the guest bus
- `450cf6d14bfebd24c502aabb4ed21486f617ee4d` Resolve Linux player libraries relative to the executable after relocation
- `d9ce63b7bfdf61e15c5bc87361adcdd08931a0fd` Run the matched wrapped-angle subtraction at each of its three guest boundaries
- `6ccdc19c2b0c5b70823a8889abb6b54549b1b878` Match room camera initialization and callback selection with one shared actor
- `d3f07b0654ff1a20236381d68bca77193e7621ca` Match sixteen field movement and controller routines in their owning scene member
- `4a1755f412782106c1d15e27c0b38d7edeb0a777` Record per-player host library and ABI requirements in Linux packages
- `6bd1aa6983581fcac10943b644c334e68f872a39` Find Windows discs in literal package and profile paths and preserve valid selections
- `ecc861401fed234131723f09c53d9b0665d524a4` Decode GPU drawing coordinates and reject oversized triangles before clipping
- `01cd11072cbf0f901e29f3a17dc2797179df6680` Recover the forest floor query, cell walk, face selection and plane distance
- `3cdb3a00ddd069904a80573d460146d9b1d08ea1` Match measured GPU polygon edge coverage and rectangle size fields
- `5d7b22e564cbdd7d51c1e2cbd73e2ec4106399ab` Match the forest contact search and the plane intersection input to the normalizer
- `aae402146231cc18fd0ac29f109b3d99e9729925` Match the forest contact transform and keep it out of unchecked native dispatch
- `b5f4910ba34f8516941a7e270f6f40832f3c09c7` Match scene countdown and fade transitions and guard their overlay selection
- `ee5c3b2daf36a816495eade51db4a2a5efafcebf` Match the five-way forest contact dispatcher and its classifier
- `89f3831aa61b221b943e0ae81061655563fc7dd6` Match the paired phase dispatch for the return side of a scene fade
- `d5ce271fbbec3a4410d1c5d3e316b8e93795ba46` Match forest linked-box collision and all three axis tests
- `c8e6bdaaa0091fe2895216ffe657648a8ecb9712` Recover the room draw dispatcher and preserve its three guest arguments
- `ab40672a33d1f37177306d2c46019833e611bc67` Match the sound voice release routine and model callback ordering
- `b19a6a3b73ecc714cf5c377755ed5306bbefaa7d` Match room 3050 frame dispatch and honor its scene-specific state
- `aa328be4aaaca5ea17b0bb0eb65ae7218ffcf2c3` Check card VBlank selection, cancellation and batched completions
- `e69cd3244a5c01628f02476dcd95906bb48cdedc` Match mine contact probes and correct the three-coordinate distance interface
- `17cc4a9628e0dc64830689f9c482e6778c657b89` Match four mine floor probes and preserve their host pointers
- `b0f270aa0be7bbd1d46e24203402ff4ffb0548b2` Re-derive mine model surface and floor map queries
- `494aa466cb7459217c7b027bfd2a293e55874e83` Preserve actor handles across the two forest collision searches
- `0f5bd9966e15dc9ba802e280c622b937172ad7a1` Initialize interrupt auto-ack words at the selected game entry
- `924361dcd1b47d610b9115e26f9d1a434a99b6ba` Match the moving forest segment collision query
- `7d1ff0a0e45a37398cf487239862053a89acbb58` Match mine floor face intersection and dot product
- `59c17fab62e625df594925d2f1e2f666b2894cd3` Match linked sound events and preserve channel search timing
- `83e6221748d2d33595c0e45a58427d7892c4a7e7` Match the mine model contact transform
- `ff9ab165d575a9d22a06329cdc16124e93350d42` Read the first matching card directory entry through game callbacks
- `545a7b29aff33ec200ce5d46cfe82fa7bae3a7e9` Match the forest slope classifier against its shared source
- `2738c4641231ed415e23f84875caf4e11da67a27` Match the mine model surface classifier
- `e34513256463f4e0bfdc4baa7669cc669e5c6b07` Match the forest model collision query
- `3f2c14a498a05f85b2c81978636355e6ef66dcde` Match the four mine box contact routines
- `a6f7428c66fb1e895a0bb4543f3c57d902c62865` Read optional card transfer input as binary on Windows
- `3dd9420e9bc979e4e03142668ca3ca491dcd880a` Match the forest model grid traversal
- `6567fec85626161092c6c7c709de4fc11160a7fc` Match the mine per-frame duration script
- `e4b1060ea47b8bc071b2cbf645a1e58147713cb6` Read and close the authenticated memory card metadata record
- `d01b3e75c1303952859725dd9f664de8afa87b31` Match the twenty mine slot position updates
- `40dab98cdc44906cbe873f4c824caddcd3b29b29` Match the mine actor position scheduler
- `7026224cba634727a674b679724106efefc514eb` Honor the game sound fade threshold inside an interrupt
- `e8b355162a62f77fc01685688ddc7dd1c92469e1` Match forest movement and two-cell ground queries
- `1f3a0400b72e3b75974c205dd0b2828cf0e95e19` Use the checked mine memory interface on 64-bit MIPS hosts
- `f43a93c0ce176699dcea98fb5c13a8a784db9ae0` Match the forest actor movement query
- `a2349a39e6ea819131f344cc0cc8fb0fc4713bf8` Return through the live GPU DMA interrupt callback
- `27a3142feb57e8438425a9ccf65166210409f501` Execute the startup exchange on the full player clock
- `4aee641a6320cd4479573b2655846516ce226c96` Match the mine actor scale and update prepasses
- `9370f452ad3e50d6f5461fac5b620dc87a4bad0b` Match the mine dialog band color update
- `f347962407ca5acb697457d14ccbde46b3698b23` Match the conditional mine pickup handler
- `7d22b245630046a1e5593d39c5f5d80fbbdae5bb` Match the forest floor query and its contact result
- `d40671f9144b93daf9fe571ed36cb8667d6977ce` Match the mine target indicator draw list
- `2b2dcc5e88973ebab64e261abc69a657e28b2b08` Resolve forest model and grid addresses on host builds
- `5e050a0b962f2858636e510c8a41e5df9a0a61a9` Match the forest player update and action flag dispatch
- `83ad88296ef4e24520a6147a6387bd1d0df0cd74` Report the startup exchange under its actual execution context
- `4f92b5d0ecb541872b7b12043dcbc731bf3fbf09` Match the mine animation command scan
- `0fe255435977d4752d408c63a0790e86f9ca067e` Match the per-actor mine animation update
- `e72cb44a020a4c7e76f66cdad36036dcb369a2a9` Match the forest player action selected by the dispatcher
- `75ece15bfb06abb36b3e72ae0bcf0773f097de36` Run the first game main call on the active native lane
- `e746e0316d608f4eb28af5bfb932172a980ef948` Match the two forest actor collision tests
- `6b518b285e281f3b5d85128825610a58c4bd39cc` Match the mine animation command dispatcher
- `b98812691f3214dccadeed6d02bc5ed6647d383a` Match seven forest player action and ground query routines
- `60122b83dd1978a9d8074fabd86e1851e968174d` Initialize the registered game main test provider by field
- `61e381cfb6ae8e94b14d6a31d3308afc33175591` Preserve the guest actor handle in the forest event callback
- `21f0c117db2557f7ea63c12d251104002a49c0c9` Match the mine screen effect with a four-byte guest handle
- `67f0ebf1416d706cd906283b9678c6fd8c2c5fbe` Follow the mounted save file through its first metadata read
- `6d1716eb7789e040b3b2a48aa9c9083a051c0acd` Match the mine actor step toward the focus point
- `8fad6747f12ba2d50b81133e37e3c42cf82f0228` Match the forest player phase and three transition handlers
- `08bb71cd4e3368a17bab5cb5d76f765fda436b93` Keep two actor counters on the guest CPU until timing matches
- `576afe1df3c3de905a78ed3a5868e924a4473cd4` Run seven game main instructions after the original constructor returns
- `92d60b4846a7272c77e71f672f3217b15b021236` Test two GPU DMA completions and their interrupt acknowledgments
- `c532cb8adfdb0e8b07a7ec1097b220a9e6d85956` Withdraw SDK-derived routines and direct affected methods to guest execution
- `e6197eaa154c300c93777c92207e62293d143a77` Match the forest actor event checks and shared event allocator
- `d570408657191d8a3fefce6e7f74f974d462feb0` Match the mine controller target update from its observed frame service
- `1cbbfba6b566875464f304a908274b9161f15e18` Run the startup graphics caller through its first library call
- `078954b24aeeeedbdf9f71a45cf6eab67a267427` Keep the forest flag read interruptible at its original instruction boundaries
- `7b84c4bfdde552a8d6548e95fad106ea9a76b237` Match the mine actor update and its two fade requests
- `f2305668002aeb4140ef18f83730bab687bff8f5` Report the clock actually driving the device epoch
- `6a7f0b8258aa54cca86cf666be5cacd5b1796a0e` Match the forest animation command reader and its selection routine
- `14e503d4c55803a524732791222ceeabe296d7d5` Match the mine fade progression and its completion conditions
- `c070e41a4d1425286a09ff4193bf6620536d6b9c` Match the forest secondary event and its positional sound call
- `7136ad50e65f7d6430fd75344fb0c3b0dc8a7d42` Match the mine attachment list and its per-slot position updates
- `1649350f9e0c6724e26b67593dccee16f4575ccc` Remove scene address tables from the public interpreter
- `6ad71f2ff297ea4d281ddc1349e3f219de618927` Use the project include directory when testing copied formatter sources
- `fa983b2ad61b59288eb728a8e989ad4b41457be1` Always report the final GPU DMA counters on shutdown
- `4b64e33f1d14bfe55b29dd28a34931d40fd0f597` Reconstruct three forest actor ground attachment routines
- `83aa3f0e8a9e12d15b437d19d89caa7b2a02b9fd` Reconstruct two mine frame callback dispatchers
- `cc5688bbf1fb69fc61c3cbefae8db4823b9ff8a1` Match the game graphics startup wrapper
- `359bd354518a172a3cec9cd30c013eeb06e39269` Match the mine frame actor link update
- `ad60d6d1db774ab71ccca807948d416e1fd18e54` Run the game caller through the CD library call after graphics returns
- `e1c5e33b9f92c7fe6620d0bf6094dd9e21bfe3a9` Resolve forest ground orientation links in the host source
- `a6da90874424975ff100301caf455b90d4414c39` Match the mine camera update and its live owner selection
- `496030f709532efc891de02f0c0c7f6750f5cc7a` Match the game sound startup sequence
- `51152e5d86845b7727f41d55f0e1ee60fd2e6669` Match the sound record clear and preserve its per-record reads
- `357016e4d055363f498c62cb56374134723dd5dc` Match the game disc directory against original startup and archive entries
- `fcfe6145325940a208e6f4fae3c4a2eed76281f3` Match the eleven-state game disc reader
- `e61d1e64cc26a6c5bfe189d8f7fad1e78847837a` Match the mine collision actor state dispatch
- `b61b26a56294ccfb9ec9427e0e099926974ce24d` Preserve the full word returned by the forest flag reader
- `4f30668430837f4830f9629b7c99738f2729310a` Match the mine collision actor initializer
- `dc4c865f327935800e95c3280eb711ea8e0dccd2` Match the game sector callback and its two dispatch tables
- `37e7440c6f6b4f3ad01447b2f6148a0c76c38fa5` Match the mine actor visibility, motion, and collision sweeps
- `7d713ff80584973fb7225e902693d2ddb3a70d10` Match mine rotor creation and state dispatch
- `9c12c3cc211d9e67f4ecf1e70bf5a1104b959982` Match the mine rotor update and sound selection
- `d0010efa680dbe4d107a25c729996c49df7bb679` Match the Steam Knight attachment damage callback
- `8f389c5b853b17ca13af890aeaf30e79f4ab047d` Model signed forest velocity updates with defined 32-bit wrapping
- `7bfa7b947a628f164d64a48c43a39dad07f9ec74` Match game sound-stream submission and cancellation
- `3a23533a27e6b9b628711b1714a484bf852f4767` Match the forest camera vector projection wrapper
- `558683a151e8ee56663fdc8370a3f48a430a4d7d` Match the Steam Knight contact that records incoming damage
- `d58661472a7814e9735d139143253546a86e5b1e` Match the mine actor pool slot initializer
- `24f8b77fb3c861c3a5d9e4147bc7b5bfb131790f` Match five forest actor activation functions
- `51eab8f8f88eb18edf8dbafa4a3dd8076d445a61` Match the tower boss damage reaction and verify its callers
- `538869590a9c75fc80258489f39cf2cefe577ede` Match the SC02/0039 mine room selector with its MAIN/0012 counterpart
- `1ceee70d4f7f9955a01e8780be5ddb1f31c0e273` Match the forest player model velocity transform
- `b1231ba260e009395e20c068b5a6bf2152f3b48c` Match the forest image move list to its verified sibling
- `725299abb0efc2f48e3fcc6c5e6616a978da9183` Match the forest bridge settling effect and its particle updates
- `7d75490acfbb297514319d4cb001d34c2485396f` Match the tower effect request and its allocation of game slots
- `7ad56ac5bf071ecebcb8cd09b2f9ea3b2277900c` Match the forest player callback and its input clearing behavior
- `58e102f1ef22ead2893cebe18e13b837549139a5` Match the forest tutorial actor dispatcher and soldier creation
- `1447910a55fde10ff55c7440e7317558f3d62f4f` Match the tower impact feedback and its effect request
- `39efcf47900f636c269d016db4d370f314af6a27` Match the forest actor ability-release callback
- `1bcacc2d437a91abfba65e18681d87d9ab5dad9d` Match the game track-start state machine and share its sound-bank layout
- `9fb32313132567c41982154d98c5533a9cfaa8a4` Match the Steam Knight outer armor break and its particle requests
- `1f15cbeb5988082aed9447508e72fcce2e014506` Match mine room grid setup and its address relocation
- `76818c9143340d2f2f5c951f86a1882f71ed49b3` Match the forest bridge player callback and its yaw adjustment
- `7b6bbfddafee30613dfb809d670a4eaeb6d84a67` Match the MAIN/0010 room stream selector
- `7f879647015018949e886e4aa6bcbb29ea0f3177` Validate complete sound-memory DMA transfers
- `1eed040a19130fa82ac4fbd9aaa08fc33e6d869c` Advance ADPCM voice history after each decoded sample
- `8d43d1a3c1bf8989ef3272dcb3e2865192b02604` Match the mine frame actor and field HUD routines
- `3c197f04f97480eb55bea918bee7ffb34f94b183` Match the Steam Knight particle setup and movement routines
- `f0cab3df1eb970848561cfb1ad812a1b3405aca5` Match the mine backdrop painter against its loaded scene
- `471c884d31a61d24f46650d5352b36e73b411451` Match the forest bridge tutorial stage transitions
- `fa24b26899966f667f1c87e87ef55715ca0a7ad3` Match the Steam Knight fragment actor update and removal
- `2c297b6e1c56f742316367795e56ed14869fffa3` Match the mine stat gain display timer and movement phases
- `62cac687a6be5bc1d7b05b7c116ef8bc2dd145b4` Independently match both music track row handlers
- `aca6519e7f10a9896c58706d1d0f13689bd2db33` Match the forest projectile target queries
- `cca464adb4f267b35763533b75a3d2b99344aa51` Match the Steam Knight core contact and bounded damage routines
- `1cf8c907d3bacf589794a2737929f24b35ca6ef3` Match the mine numeric HUD setup and drawing routines
- `b2c5a778632a5c4189d4431b921a012d786f2722` Match the forest player movement action with its controller and model state
- `c406867414bd3ad4add33cce4dc348fdbad0eb5a` Match the mine stat award actor and its three service wrappers
- `11931b4cd46696af05ca32c142b631aa100f9e3d` Match the forest impact actor initialization, update, and expiry
- `bbca2aab63465f8b343a602cd1f32e1853cbd9db` Match tower actor allocation and preserve room-specific selection
- `54f17c9b02cdaafddc914e574edfad86b55cd5a5` Recover mine award setup and verify packed numerals across every column
- `bb20c8ba38c9f746d7064b4f4f2ac71d4a9d10c3` Match the seven tower body and marker routines
- `bdfcbbac5e50bbb2b2050e1bb974d8ddf8e7cc53` Match the forest player frame initializer and preserve ordinary dispatch
- `ba9b45d5de0c5798be65aa61d2c4d2274d198464` Match the remaining seven mine award controller phases
- `08e81e1f12998ce988fc4fdec2415363a2110dc1` Match tower animation refresh and two opening routines
- `18c04a2224d3a40d5605fd9dd9986e362dfd43d2` Match the mine model sequence evaluator and its regional table
- `cb3358755401acdd34db4bc00d37d3a0f5e8240e` Verify C recovery against assembly emitted in the target procedure
- `ff8c75122ed51b9778275e091526ce0f92b295f0` Match four tower armor actor routines and preserve their room selection
- `ced4629e6c319ba859b2c0c872479676f17624e0` Match mine terrain plane distance and edge clearance tests
- `6590c7e110e1c6fe8db2f3ef74495f6413e33b76` Recover mine award actor lookup, phases and movement services
- `ab83adf5fe7550bea8132ee6231b7ad32618052a` Match the Steam Knight moving armor attachment tracker
- `dcbd7bbbb60e54847f2308eb641829ee64cb5b30` Match the forest kind-four player selector and contact action
- `34b2fc91ff3acbd540dca0ddefbc356e328b977d` Report coverage from split public match registries
- `ceae97d8a86bcf90ec1059f6525e0a3e5b59beb3` Match the mine script action queue and stat award request
- `09f327afc305d6b33c6abdb707f721f3eecffc84` Match the Steam Knight auxiliary reset and body phase services
- `8cf8cd8399db1a8a54023568096292e8957c9f2d` Read bounded ranges of files on the mounted game disc
- `43003f0dd35d34b7fb86355a0e3faeb3aa552ea7` Read the forest player mesh from a mounted game disc
- `4f3e69f899347fad37d2eb8a88c63ef86a5a79db` Match the forest attack strength calculation
- `0eefda43a45ccab6a9c497b71165c296417a4479` Match five Steam Knight body turning and phase routines
- `9472890be8811b57a0d9ccb2d65e7b46b37f1d8d` Interpolate polygon texture coordinates from the quantized left edge
- `90f2fa35a66b453b51a13d7e0751698fa045f073` Use measured 12-bit steps for horizontal texture ramps
- `9a2b6af76fde9bb8305326b423dfda6aea41724b` Match the mine award model and signed local pose reads
- `15ad96932bc6f3f8ebdbe50204eb8ca5ebcc4b0c` Match Steam Knight joint setup, dispatch and allocation with typed model records
- `0cc137f6e0d0128230c44f4c0c291109ca11de6c` Match the initial mine placement list and free actor selection
- `77ca2f929b0fd9e64cd5ad2b0264858bcc6fffc0` Round horizontal undithered Gouraud colors at the measured precision
- `6f8fda751fc54447e06a24cfbfbf785807238471` Match Steam Knight state transitions and preserve the signed collision result
- `9b64b9ceaea3eeebee7fc7b657b523abd8c85b38` Match the mine actor world velocity and vertical step
- `31ce4f9e8a68b5fbe2808f957d19b1d88597b872` Match the mine door phase and model allocation
- `6a003678dbef9a00e27016afafa6dcd92b47e9c0` Match the mine field actor pool reset
- `612ccc05cb24cdcdaed05421a631ee01bc27e545` Match the fourth-stage mine field initializer in SC02/0039
- `e9753b1d3ba6c666d64b3174d73ac2228e72f806` Match Tower body action dispatch and shared duration timer
- `64ad2e406a2d52348c85fc601b0d519bab78a980` Match the mine door actor initializer in SC03 member 0116
- `8f4a9d295a432a5b9a8f5fd1bb86efbf7b0aa4b0` Correct opaque flat-line coverage for shallow GPU segments
- `45be0b40a38241c230cc97a2abcc599e54d14bc7` Match the room 3050 actor action dispatcher and pending movement
- `3656cb5f1372d0e01c243f757719f88c8659ea5d` Match the Steam Knight action evaluator and four decision routines
- `2e152be37b412a6eee2a03e15dfc717c8773e194` Match the room 304C door actor phase dispatcher
- `726cca34090f8e0f6361e5dfb0555214d5ddf8e5` Match two Steam Knight action predicates and retain their callers
- `f09a09b48b8fa4e9d86203c82e6984619ce71630` Match the Steam Knight joint effect and its typed actor allocation
- `d789de43c2b976941f8699fbad325e3f7f9cf220` Match the mine grid cell walk and its ordered face visits
- `7d40c0504983edadba945ffe7a04fded801dc632` Match four Steam Knight resource routines and type their callers and test fixture
- `351c8b20c64510f1dd466878083f2443b07aff75` Match the mine terrain flag translation from its owned overlay
- `65363a4529936102dbb702d973be0f4ddbd56e7d` Match the Steam Knight actor cleanup and preserve its return type
- `1b9495952d2c7156d2fca17c6ddd25bc7c39bbec` Match the mine terrain response routine and its cell-walker call
- `d334a239379d6e1c8f099b370f2dda32890b2fbb` Match the two Steam Knight intro countdown callbacks
- `59ed3754137ecf55df044ef185868a1333b13905` Service pending GPU DMA and bind draw completion to the active graphics frame
- `047f04e7b897261ca18c67bdb5a9e5112d81de28` Recognize completed GPU block uploads on the active draw interrupt
- `7d3a3e85f21500010e977d91aa5ce5bb192d3675` Match the Steam Knight turn callback and its player request helpers
- `cd312eb3edf5fe52d31ccf7b5030e2779c2919b3` Recover mine actor step and its full-width terrain vector service
- `3564df9e009f8a61a80360232dbd637cdd121d0c` Service the audio channel countdown from its mapped VBlank state
- `1fb95c2b1380d78f2748913073049cd6b962e108` Match six Tower camera routines and verify their transition caller
- `aa8c885c501879c93d1d19a229ffd37891d2b50e` Recover the mine child spawn with the shared actor allocation declaration
- `8d796410fddb6d544f3f4cf53d4fb1820bd350b4` Match camera setup and dispatch in two mine rooms
- `279108c38b951fe0e59342c3223ceeffab734098` Match the mine emitter and child stage selectors
- `bc9fdf97db8a7e46df5c169e09a3712745915837` Deliver completed CD movie buffers through the registered callback
- `1d15fbb47865b502b19d57292bcdd70938ae0570` Match the Steam Knight handover and its actor reset helpers
- `6f9d7d0f811676859fff406320175911e5aae685` Match the mode-zero camera updates in two mine rooms
- `8368a725e136256147cc249d0efa4c233ed97281` Run the mounted movie output callback on completed native MDEC and GPU transfers
- `e31823f3c066376c47f4eeaf572d6cd7a9736909` Round native MDEC color output to the nearest five-bit level
- `366fc26516941082218d03610b70e45f557adbb5` Register six existing forest scheduler matches in their SC01 overlay
- `1ea3117804712051e45fbc5990ed0959caa2849b` Match the forest enemy and its child actor in the SC01 overlay
- `8e6503d04324457d7a9dd2a9e6cb7d6e7afa849e` Saturate decoded colors after signed nine-bit conversion
- `1b02587e038895ae2a7e14571882c6da1de232b8` Retain fixed-point transform precision through MDEC pixel packing
- `bc9aace983b99865c9b34680d3e12393bd0da78e` Match mine emitter initialization and child model feedback
- `b392f05e70791ef829ac352c20daac2109b793d5` Match four forest attack routines and align the sound command interface
- `1e2ecb6c466833caca8a182555fe9a9273b3d8ef` Match the mine child model segment query
- `c4b67103479bc3f4d4fa5a5991e521a398221624` Match the forest actor diagnostic selected by kind 218
- `0bec1dd40b8a77ddb5ddf85d1eeb6a628bc76bc2` Match the mine child actor setup and model allocation
- `cc7c0437879aa1986c8dbd644812b380dd6d72a8` Register four forest routines and match the child allocator caller type
- `4bb427c05d571b438edd586d1fde9f678c900972` Match the mine emitter selection and preserve the resident audio ABI
- `1851a4208138dd453b6942b2b882984eb1587d21` Match the mine emitter projection wrapper
- `8472d6f2f297e9b77c16de0de23e4cade932b88c` Match the mine room 304C light actor state selector
- `676d4e7fc978cb77f655c7ed9954e45dfcacaa99` Match the mine light actor first update with the resident light setter
- `fc8208941b2f5787f5eec30a95cace773e0c66cb` Match both mine room 304C type 334 actor update stages
- `874c39497c45a8cec5e2e7742d4eb32dd3ca32a0` Match the paired mine model actor selector and setup
- `68de17b0085e66ba67ded448db22b8347971d403` Match the mine stage gate for actor type 33A
- `84b0c0e0298ab48d4f9f1cf908aaa2392ef62715` Match the 304C mine event dispatch and first phase
- `3970a02c86004101aeb539b739437325e1347491` Match the first phase of mine actor type 333
- `8d18f8a235a405313c25d1d4dbef6f7ef4fa9007` Match the forest class 133 dispatcher alongside the Steam Knight overlay
- `393c6fe6a64e581ab9272896bb681ec20b015a81` Match the sixth phase of mine actor type 33A
- `f1a5501a940377c4675e23fc4830679de013a609` Match the mine ring timer reset and stage update
- `272f24da11dd7e3f303579164f71960b3ea28e50` Find configured memory cards and retain existing Continue saves
- `df65a330a3b05b0b4e3230f00bacb58f630e81f4` Retire buffered movie DMA after its decoded strip reaches memory
- `3d09f1430af266216f1a8264b02c432aea9be489` Run the opening movie output callback as native host code
- `a9c96fa6792a1833bf920f012e17213518d2ea9b` Match the village porch quest actors and resident event-flag setter
- `fcc711a7f4efbb4a52996f4e65577052649c0fdc` Run title sequencer and mixer updates during Continue navigation
- `d272bf44d59c9550deeacdce9aaa55b9c001b0c4` Match the scheduled village field actor transition
- `614a69b4cb4afe81a021f528820d324a5e320871` Handle the mode-3 title sound callback when Continue opens
- `9a405b7aacbaed9bd04bd1aeac8669ad434fc8f6` Transfer buffered GPU image words to RAM on DMA channel 2
- `a2220e49336b3096e1a6ab5ee914e9d50f4a6f4d` Match the five-phase village actor update
- `18696c57bda68b12683a17698675b71d349030d8` Match the game disc audio playback report callback
- `b6f6a6792f29c040ff43af7e7cbcc89d727924dd` Match the saved-file status panel builder
- `971d1565aff22e89fd7a4151071a433445f6f749` Match the saved-file decimal formatter
- `35e1edebff425f40e7695d9cfd0bd8a11cf77340` Match the village actor waypoint advance
- `f4e4df3f475360f9e3b0da270657dcd62e15d1f8` Verify the existing File menu glyph match and retain timed guest writes
- `2482f23988c75e79dee9aceb0805565757cc520c` Match the village floor selector and retain guest memory timing
- `83012998d13810c7aafcb8ee15fa8e2b1e780c44` Restore the File 1 card payload and match the village guard at Twinpeak
- `3b569ed7014a67edec55ffaae0a2fd8df7f716a1` Match Restaurant’s Macho and the daytime Twinpeak gate
- `04b64f5831b096c14e70e7544bfd19b0eed079d6` Restore the selected save through the mounted copy loop
- `b9e02afea744fc111f9d53a2f6c82fcb61ce5367` Match the forest Bincho Field and Guard Lumpwood rescue
- `c1cce445c71c54f0be5918861d07cb01ece3880b` Match the village stocks actor and its story branch selection
- `5efbbc65e04b209209dda5db73c3de2747d9816b` Retain the restored File 1 through its fading sequence track
- `2325387b63543f129d7140f417a1568fe5d7da64` Run the opening sound transfer query alongside the File 1 sequence track
- `db001802d0169fd3b0d86162626b3af5aa1515ec` Execute the opening music sweep through the mounted sound service
- `624fb4447b20562aa5ba811ae12a72b811c61486` Run the selected sequence-slot voice refresh when closing File 1
- `09467c781887aa03eac097b04a9360e7cab4d496` Verify the selected file result in the foreground menu
- `dfe27587cef663037832b42016a67d2e46d86e75` Retire a released opening note in the sampled low voice table
- `30cd1468d29299a43d759652e30fe5af87c45d98` Fit the initial player window to the available desktop
- `6ea641075cca378bbde59fc5d0e281a5a21366a9` Bind the confirmed save file to its mounted card payload
- `73b3bc63918023ee1a9e90434b1cdbaecbab7bef` Publish each verified memory-card slot status before the file picker
- `e2730472c29ef727e2f6662b7f896386012d0573` Follow the loaded file into the mounted room initialization routines
- `5bd282069d060ab455f58a373b5a0e7c5435859d` Honor a repeated opening note on a released sequencer voice
- `d73583e35c3813767ef8fa843019225db79f4fc7` Prepare each populated file row from its own validated save summary
- `42194e45c2529918677a2f14eea1548405599f15` Retain the file menu result when all saved rows are empty or damaged
- `23e05e056667ea9ba2d957a4fc161f727d2fd293` Validate the rooted room initializer halfword after File 1 restore
- `cd089b62ded4ca9bb87b8ba8d52c449b7d737c23` Validate the file-row call against the last checked save summary
- `15c0b5f70c35d280db42699fd68b932a29f53742` Run the LIST sound-release callback on its mounted guest frame
- `e45d0e0340a54ec0a5299f38afbaaeceac6277a4` Keep the SC01 opening scene on its own fetch path at the file callback address
- `88de7684234d63dc281dfba2cd550acf24930513` Retire the mounted room slot reset one operation at a time
- `7e7554d85ac91106eda34f00c1b0e98c05ed14e3` Resume the verified foreground menu through the room slot reset

- `412da90e2911877d5aa1436e5e4706fa1d16ce24` Retire the mounted File 1 scene setup and its nested clears
- `c0b073650260724f2f01e1eff8a5ba2229434f6a` Run the registered master sound fade natively during room loading

</details>
