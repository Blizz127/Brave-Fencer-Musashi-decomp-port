# Native boot entry contract

The September 2026 startup experiments used the USA executable from the player's
disc. This record covers the executable entry, the interrupt bridge and a mounted
startup diagnostic. The PC player still has a hybrid runtime. Its current status
is in the [project README](../README.md).

## Executable entry and guest memory

The USA executable enters at `0x80010000`. Its startup code explicitly clears
guest RAM from `0x80074750` up to, and excluding, `0x800C7F08`. The executable
header specifies zero BSS length, so applying the header alone misses this
clear. The entry places the guest stack at `0x801FFFF8`, computes the heap,
saves the incoming return address and calls `0x80010178`. The incoming return
address is a parameter of the memory probe.

The loader checks the identity of the exact bytes it will copy before modifying
guest memory. The memory tests compare the whole 2 MiB image, preserve bytes
outside the specified writes and check that rejected inputs leave the memory
unchanged. To require the real executable in these tests, extract your USA disc
and follow the disc identification steps in the [project README](../README.md).
Run the test from the repository root.

```sh
MUSASHI_REQUIRE_RETAIL_BOOT_DATA=1 python3 -m unittest discover -s tests -p test_boot_memory.py
```

## Interrupt delivery

A display-backed test connected a PsyCross VBlank to the game's interrupt
dispatcher and tick callback. It observed the virtual interrupt, the tick
increment and the delegated BIOS exception return. The fixture supplied the
RAM and device state. A second test ran the callback initializer against the
disc executable so the initializer set the interrupt guard itself. The
platform scheduler owns the exception frame used on return. These tests cover
one interrupt path. [Scheduler probe](../tests/irq_scheduler_live_probe.c)

## Mounted startup on September 6

The mounted diagnostic queried the disc's four tracks and observed the sound
initializer return to the file-loading call. Its audio backend queued and
drained output. Audible game sound and hardware timing remained unverified.

The graphics diagnostic processed the game's black 640-by-480 clear and read
back all 307,200 pixels from VRAM with zero mismatches. It then completed the
two framebuffer environments and the second geometry initialization. The
recorded stop was the camera setup at `0x800144D4`. Scanout was still disabled,
and the diagnostic had not reached the menu. The later start-screen experiment
is recorded in [Native menu continuation](MENU-BOOT-CONTINUATION.md).
