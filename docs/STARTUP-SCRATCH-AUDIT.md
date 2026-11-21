# Startup scratch-stack audit

The September 4, 2026 audit examined the USA executable `SLUS_007.26`. At the calls under study, startup saves its ordinary stack pointer at `0x1F8003FC`, enters the callee with the stack pointer at `0x1F8003F8`, and restores the saved pointer on return. Nested calls or interrupts that reuse the saved-pointer slot require separate handling.

## Geometry records and stack space

Two dispatchers use overlapping views of a table in main RAM. The final two entries visible to one dispatcher are also the first two visible to the other. Both dispatchers check object state before calling a handler. The audit established the initial layout from the executable and left the table’s runtime mutability open.

The geometry routine at `0x80020248` counts a zero-terminated list. For up to seven entries it uses two scratchpad buffers with 32-byte records and returns a selector telling its caller where to find the results. At eight or more entries it selects main RAM. The routine writes results from the Geometry Transformation Engine (GTE) into the buffers, which subsequent calls consume. On the inspected call path, the stack after allocation lay above both scratch buffers. Other handlers, deeper calls and interrupts can change how much scratchpad space is available.

## Native probes and their limits

The matrix leaf at `0x80020DA4` masks three input angles to 12 bits and produces nine signed halfwords. The 18-byte rotation result and the 32-byte input transforms have different layouts. The GTE control loads read the input transforms. The published native probe tests rotation, translation, saturation and in-place updates. Its negative control deliberately writes a vector to the wrong GTE register and must fail. In a configured PsyCross build, run the two CTest cases with the commands below. [Probe source](../tests/startup_gte_probe.c) · [Transform layouts](../config/context/render_transform.c)

```sh
cmake --build build --target musashi_startup_gte_probe
ctest --test-dir build --output-on-failure -R '^startup_gte_(probe|wrong_register)$'
```

Using the table from the verified disc executable, the historical matrix candidate passed 24,576 single-axis cases, including a Clang undefined-behavior sanitizer run. It also agreed with a restricted scalar instruction model on 4,608 combined-angle cases. A deliberately inverted sign failed both comparisons. The candidate compiled to 44 matching words out of 100. CPU timing, multiply latency, interrupts, caches, hardware replay and execution of the full startup path remained outside these tests.
