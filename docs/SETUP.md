# Setup for existing public r38 installations

To build a development player on Linux, use the current source instructions. The
November 21 package review found embedded game tables in historical Linux and
Windows players. [Package review and current build instructions](../README.md#player-downloads-and-source-builds)

The controls, save paths and launcher examples below describe Linux r38,
published on October 2, 2026, from runtime source
`61c60516d18bf6402e6dbd937d8c8d18ebd1af4b`. Its forest, village and
isolated inn save and cold Continue checks used null audio. The r44 Windows
build was tested under Wine. Both builds use a hybrid runtime.

## Linux prerequisites

- Linux x86_64 with glibc **2.38 or newer** (`ldd --version` shows the installed version).
- System `libstdc++.so.6` exporting **GLIBCXX_3.4.32** or newer.
- Working OpenGL drivers, an X11 or Wayland desktop, and audio support. The bundled SDL library links both X11 and Wayland library families; it also needs ALSA/PulseAudio, libsamplerate, DRM/GBM, libxkbcommon and libdecor. OpenSSL needs system libzstd. Missing dependencies are reported by the system loader.
- Your own legally obtained USA disc, **SLUS-00726**, dumped as `.cue`/`.bin` files.
- A keyboard or an SDL-compatible game controller. In Steam Game Mode, keep Steam Input enabled for the shortcut.

The package includes the required SDL2, OpenAL, libcrypto and zlib shared
libraries. Keep its `lib/` folder with the executable. No BIOS is needed.

## Run an existing Linux package

1. Open the directory holding your existing installation.
2. Keep your own `.cue` file and all referenced `.bin` tracks together in
   `~/Games/brave-fencer-musashi/disc/`, or in a `disc/` directory beside
   `launch.sh`. One-bin and per-track dumps are supported. A 2048-byte `.iso`
   is unsupported because it lacks the required CD-XA data. The normal disc
   route checks the combined raw track bytes and the USA boot executable;
   other regions and modified dumps are rejected.
3. Run the launcher:

   ```sh
   "$HOME/Games/bfm-port/bfm-r38-61c60516d/launch.sh"
   ```

If the disc is elsewhere, set its path explicitly:

```sh
BFM_DISC="/path/to/your/game.cue" "$HOME/Games/bfm-port/bfm-r38-61c60516d/launch.sh"
```

A desktop file picker is offered when no disc is found and `zenity` or
`kdialog` is available. Otherwise use `BFM_DISC`. The wrapper also searches
`BFM_DISC_DIR`, its adjacent `disc/` directory, the folder above, and the
existing Banshee library's `games/brave-fencer-musashi/disc/` folder. It
remembers the selected file in the configuration directory listed below.

For Steam Deck/Game Mode, add this `launch.sh` as a non-Steam game from
Desktop Mode. The launcher chooses the optimized x86-64-v3 executable when
the CPU supports it. `BFM_BASELINE=1 ./launch.sh` selects the baseline build.
Hardware performance and audio fidelity vary; the headless regression gate
is not an audio or handheld certification.

Install new versions in separate folders. Finish playing and quit normally
before backing up a card or choosing another build. Installing this package
does not require replacing your disc, saves or settings.

The supported raw disc stream has **416,021,760 bytes** and SHA-256
`0a53702937d74e20d99fee9a29a80f7e91da762e66958819d531173939a50879`.
For a per-track dump, this applies to the concatenated track bytes, not to
the cue text. A cue may differ in filenames without changing the disc data.

## Controls

The port follows the controller you use. Button names below use the SDL/Xbox
layout; other controllers may print different symbols.

| PlayStation | Controller | Keyboard |
|---|---|---|
| D-pad | D-pad or left stick | Arrows or WASD |
| Cross | A | C or K |
| Circle | B | V or I |
| Square | X | X or J |
| Triangle | Y | Z or U |
| Start | Start | Enter |
| Select | Back/Select | Space |
| L1 / R1 | LB / RB | Left Shift / Right Shift |
| L2 / R2 | LT / RT | Left Ctrl / Right Ctrl |

The packaged dev menu is enabled by default: **F8** or **Select+Start** on
one controller opens it. Menu navigation uses **Up/Down**, **Enter**, **Esc**,
or the controller's D-pad/left stick, **A**, **B**. The game keeps running
while this menu is open. See the [Dev Menu wiki](https://github.com/Blizz127/brave-fencer-musashi-decomp-port/wiki/Dev-Menu)
for all options, shortcuts and save precautions.

## Saves, settings and files on Linux

| File | Default location |
|---|---|
| Virtual memory card, port 0 | `~/.local/state/bfm-port/bfm_card0.mcd` |
| Last run log | `~/.local/state/bfm-port/last-run.log` |
| Local route journal | `~/.local/state/bfm-port/route.jsonl` |
| Remembered disc path | `~/.config/bfm-port/disc` |
| Disable-menu marker | `~/.config/bfm-port/dev-menu-off` |
| Locally extracted boot executable cache | `~/.cache/bfm-port/SLUS_007.26` |
| Dev-menu PNG screenshots | `~/Pictures/bfm-port/` |

`XDG_STATE_HOME`, `XDG_CONFIG_HOME` and `XDG_CACHE_HOME` replace the respective
base directories. An exported `XDG_PICTURES_DIR` selects the screenshots'
base directory. Files created from your disc, cards and screenshots remain
local; do not upload them to this repository or wiki.

The game uses a 128 KiB virtual card for its save and Continue menus. The
historical Linux r38 binary passed an isolated inn save followed by a cold
Continue. Other locations and later game states still need testing. The current
source build is being tested separately with a disposable card.

Developers can exercise the two card requests issued by the US game on an
occupied vertical-blank handler. From the repository root, point the test at
the executable extracted from your own disc. The test creates temporary cards
and checks the event results after the interrupted game code resumes.

```sh
MUSASHI_CODE_IMAGE=/path/to/SLUS_007.26 python3 -m unittest discover -s tests -p test_guest_irq_frame.py
```

Keep a backup of `bfm_card0.mcd`, especially before cheats or warps. To use a
separate card directory, start a later session with:

```sh
BFM_SAVE_DIR="$HOME/Games/bfm-test-saves" ./launch.sh
```

This directory contains a different card; an empty directory starts a fresh
one. Do not point simultaneous game processes at the same card. The override
has priority over `XDG_STATE_HOME` and the normal default.

**Configuration limitation:** source APIs refer to
`~/.config/bfm-port/config.ini` (or `BFM_PORT_CONFIG`), but r38 does not
reliably load that file in its production startup. Do not rely on it to move
saves or change graphics/audio settings. Use the documented launcher
environment variables; disc selection and the menu-off marker work through
the launcher. The release uses Original rendering; general configuration
and enhanced rendering options are not promised here.

## Windows x64 test build

The historical `bfm-r44-c427ee546-windows-x64.zip` is 3,980,113 bytes with
SHA-256 `c182fd27d0558389c643c42ccedbbb3d62287d350818873438a1f2ec64f87af5`.
The package review found both game tables in its player.

**Status:**
- This is a Windows 10/11 x64 build cross-compiled from unpublished port
  sources (windows-port `c427ee546`).
- Its native-lane function set is the same as Linux r41: 585 of 623
  candidate functions are admitted. Everything else runs on the built-in CPU
  interpreter, so the runtime is hybrid, as on Linux.
- It was validated **under Wine only**. It boots, plays the intro and reaches
  gameplay (rooms 3005 → 3008). It has **not** been run on real Windows
  hardware, and audio, visual, controller and save behaviour on Windows are
  unverified.

**Install and run:**
1. Optional: check the zip in PowerShell with
   `Get-FileHash bfm-r44-c427ee546-windows-x64.zip -Algorithm SHA256`.
2. Extract it to a folder you control, for example
   `%USERPROFILE%\Games\bfm-port`.
3. Put the `.cue`/`.bin` dump of your own USA SLUS-00726 disc in `disc\` next
   to `launch.cmd`, or in `%USERPROFILE%\Games\brave-fencer-musashi\disc`.
   Keep the cue and every referenced track together, with their original
   filenames.
   - If no disc is found, a file picker asks once and remembers the choice
     in `%APPDATA%\bfm-port\disc.txt`.
   - `BFM_DISC` (a `.cue` or `.bin` path) overrides the search.
   - A 2048-byte `.iso` does not work.
4. Run `launch.cmd`. No BIOS is needed.

**Files:**
- Memory card: `%APPDATA%\bfm-port\bfm_card0.mcd`.
- Log of the last run: `%APPDATA%\bfm-port\last-run.log`. It writes a
  PERF line every 10 s.
- The dev menu is enabled by default. Use F8 or Select+Start, and
  `BFM_DEV_MENU=0` turns it off. These controls come from the package README
  and have not been re-checked on real Windows.

**Known issues:**
- The attract demo stops at a known refusal at `800495ec`.
- 48 native functions use approximate cycle charges.
- Interrupts can arrive late at native/interpreted call edges (IRQ-GAP-D011).
- Whole-game completion is unverified.

## Cross-build the current Windows x64 player on Linux

The public player compiles admitted game routines to x86-64 Windows code and runs other paths through its MIPS interpreter. Use the source checkout, pinned PsyCross dependency and USA disc preparation described in the repository README. The original disc stays on your machine. The build host needs Zig 0.13.0, CMake, Ninja, Clang, Python and GNU binutils.

Set `win_deps` to an absolute directory for the Windows development SDKs. Verify the downloaded archives before extracting them.

| SDK archive | SHA-256 | Extract under |
| --- | --- | --- |
| [SDL2 2.32.10 MinGW development archive](https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-devel-2.32.10-mingw.tar.gz) | `83a5d74012311edc3c0d40ea6faecbe57ad692aa033fa5dc273cc937e3938ff2` | `$win_deps/sdl2` |
| [OpenAL Soft 1.25.1 binaries](https://openal-soft.org/openal-binaries/openal-soft-1.25.1-bin.zip) | `0915af01928b17e46f7c59ffc44a7413486bbef8424a3465109f489a80cb68a2` | `$win_deps/openal` |

Run these commands from the source root. `SOURCE_DATE_EPOCH` gives repeated links of the same sources and inputs a stable PE timestamp.

```sh
export SOURCE_DATE_EPOCH="$(git log -1 --format=%ct)"
cmake -S . -B build-windows -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/zig-windows.cmake" \
  -DMUSASHI_WINDOWS_DEPS="$win_deps" \
  -DMUSASHI_NATIVE_LANE=ON -DMUSASHI_LANE_OWN_CODE_ONLY=ON \
  -DMUSASHI_LANE_OVERLAY_MEMBERS= -DMUSASHI_LANE_JOBS=1
cmake --build build-windows --parallel 1 --target musashi_native_boot
cp "$win_deps/sdl2/SDL2-2.32.10/x86_64-w64-mingw32/bin/SDL2.dll" build-windows/
cp "$win_deps/openal/openal-soft-1.25.1-bin/bin/Win64/soft_oal.dll" build-windows/OpenAL32.dll
```

Keep the two DLLs beside `musashi_native_boot.exe` on the Windows machine. Launch it with your own USA disc image from PowerShell.

```powershell
.\musashi_native_boot.exe --disc "C:\Games\Musashi\disc.cue"
```

The native-lane differential probe can also be built as `musashi_native_lane_probe` in the same build directory. Pass the path to your extracted `SLUS_007.26` when running its executable. Its output distinguishes equal, different, failed and non-comparable cases. The Windows testing reported for this source port uses Wine.

## Troubleshooting

| Symptom | Action |
|---|---|
| No disc found / no file picker | Keep cue and tracks together; set `BFM_DISC` to the cue's absolute path. |
| Wrong disc / boot executable rejected | Use an unmodified USA SLUS-00726 dump. Check the log and referenced track filenames; an ISO or different region is unsupported. |
| `GLIBC_2.38` or `GLIBCXX_3.4.32` missing | Use a system meeting the libc/C++ runtime requirements; the baseline CPU option does not lower these library requirements. |
| Missing shared library | Keep `lib/` beside the executable and start with `launch.sh`; check desktop graphics/audio dependencies. |
| Controller ignored in Game Mode | Enable Steam Input for this shortcut and press a button on the controller you want to use. Include controller details from the log in a bug report. |
| Dev menu will not open | Use F8 or Back/Select+Start on the same controller; check `BFM_DEV_MENU`, the menu-off marker and `BFM_CHEATS=0`. F1 is help, not the open key. |
| Save directory unavailable / unreadable card | Check the logged card directory and permissions. Back up the card before investigation; the runtime refuses to replace an unreadable existing card. |
| Game stops on unsupported code | Record the build, room/actions and refusal address. Attract-mode refusal `800495ec` is known; whole-game coverage remains incomplete. |
| Windows build will not start | Run `launch.cmd` from the extracted folder, keep `SDL2.dll` and `OpenAL32.dll` beside the `.exe`, and check `%APPDATA%\bfm-port\last-run.log`. The Windows build is Wine-validated only; include your Windows version in reports. |

For reports, include the release tag and relevant lines from `last-run.log`
after reviewing it for personal paths or other details you do not want to
share. Keep disc files, extracted data, memory cards and game screenshots
out of the public repository and wiki.
