# Notice

## Ownership

*Brave Fencer Musashi* (PlayStation, 1998; USA release SLUS-00726) and all of
its code, data, artwork, music, sound, text and trademarks are the property of
Square Enix Co., Ltd. (originally developed and published by Square Co., Ltd.).
This project is not affiliated with, endorsed by or sponsored by Square Enix or
Sony Interactive Entertainment. The PlayStation hardware, BIOS and Psy-Q SDK
are the property of Sony Interactive Entertainment (formerly Sony Computer
Entertainment). "PlayStation" is a trademark of Sony Interactive
Entertainment.

## What this repository contains

The project publishes independently recovered game functions, the native PC port and its host/platform
layer, analysis and build tools, scripts, tests, split/symbol configuration
(addresses, sizes, names and hashes only), provenance metadata and
documentation. The MIT license in [LICENSE](LICENSE) applies to that original
work only.

This repository does **not** contain:

- any game content: no disc image, no executable, no file extracted from the
  disc, no disassembly, no audio, textures, movies, models, text or other
  assets, and no tables of retail bytes or words;
- any BIOS image or bytes from one;
- any Sony Psy-Q SDK code, headers, libraries or tools, and no decompiled Sony
  library code;
- any game code derived from the upstream
  decompilation [Druthulu/BFM-decomp](https://github.com/Druthulu/BFM-decomp).
  Its tools and documentation are AGPL-3.0, and its decompiled game code
  (`src/`) asserts no license, so it is not licensed for redistribution.
  None of it is included. It is credited in [CREDITS.md](CREDITS.md) as the
  reference this work was checked against. The symbol and type names that
  its research established, and which appear in `config/` and in comments,
  are used with credit to Druthulu/BFM-decomp. The owner is re-deriving the
  game code independently, from the game's own instructions checked against
  the retail bytes;
- prebuilt third-party binaries or third-party source trees.

[EXCLUDED.md](EXCLUDED.md) describes the public source boundary.
[SCANS.md](SCANS.md) records the dated checks on source snapshots.
The [README](README.md) identifies the status of downloadable builds.

## Your own disc

Playing the game requires your own legally obtained copy of *Brave Fencer Musashi* (USA, SLUS-00726). The port reads required guest code and assets from that disc. Its tools
can verify the disc identity using SHA-256. Do not share disc images, extracted
files, BIOS images, save files or screenshots of game content through this
repository.

## Purpose

The port and its tooling are made for preservation, research and
interoperability: keeping the game playable on current hardware for people who
own it.

## Third-party components

Third-party components keep their own licenses and are fetched by the user,
not redistributed here; see [CREDITS.md](CREDITS.md).
