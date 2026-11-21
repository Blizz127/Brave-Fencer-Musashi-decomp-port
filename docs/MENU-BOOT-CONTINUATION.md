# Opening movie, title screen and scene initialization

This record covers the September 2026 development build of the hybrid PC port. It follows the USA disc from the opening movie into scene initialization. The port used a CPU interpreter alongside compiled host code. Loading a recognized instruction range and rebuilding a function as matching C were separate checks.

## Opening route

On September 7, a comparison against the USA disc established which overlay belonged at the opening. The game's loader had placed member 4 of `MAIN.CD` at `0x800CEDF8`. An earlier test had substituted member 7 and supplied a Start press, invalidating that test as evidence of the opening route. The corrected run kept the member the game loaded and left Start released.

By September 8 the port displayed the movie in a centered window, returned from movie playback and displayed the title prompt without a forced input or state change. A paused preview held the rendered prompt for inspection. Physical audio timing and input-to-display latency were outside this check.

The video decoder tests exercised all 64 inverse-transform basis positions at positive and negative amplitudes. An audio test compared four XA sectors with FFmpeg's 37,800 Hz decoded samples. The public component tests describe both comparisons. [Video transform test](../tests/test_mdec_idct.py) · [XA audio test](../tests/test_xa_decode.py)

## From the title screen to scene initialization

With the preview pause disabled, the idle route advanced through the title fade and reached the executable dispatcher at `0x80011380`. Later, the scene selector compared 178 instruction words in guest RAM with the decoded entry of SC02 member 31. The game's dispatch table then selected `0x80128420`. After the GPU backend handled the scene's VRAM copy, execution reached `0x8014C968`.

A separate CD interrupt fixture checked every payload in a 40-sector transfer to the audio device. With callback renewal disabled or the permitted programmed I/O work reduced to zero, the fixture reproduced the corresponding budget failures. This measured the port's transfer and interrupt behavior under those conditions. [CD interrupt streaming tests](../tests/test_cd_irq_stream.py)

## Last observed stopping point

The scene allocated and initialized its first record. Its entry loop selected further records in order. The last recorded execution stopped at the fifth record's dispatcher, `0x80181868`, after the preceding record skipped a conditional table walk. At that point the CPU, CD interrupt, source clock and sound processor reported no faults. A subsequent rebuild of the dispatch function matched all 15 instruction words. Execution beyond that address, player control in the starting level and an interpreter-free playthrough remained unverified in this record.

The title-screen preview requires a configured build and a local extraction of the USA disc. Its launcher disables automatic Start and pauses at the rendered title screen. [Title preview launcher](../tools/run_native_menu_preview.sh)
