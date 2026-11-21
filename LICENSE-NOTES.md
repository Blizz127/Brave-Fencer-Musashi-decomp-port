# License and provenance notes

The game, its executable, and all disc content are copyrighted by their
respective rightsholders. The public source includes independently written game and port code,
tooling and documentation. The project license covers the project’s own
work. Game content remains the property of its rightsholders.

Contributors must provide their own legally obtained copy of the game. Retail
payloads are ignored by `.gitignore` and must remain outside version control.
Only deterministic metadata such as media size, SHA-256, observed boot path,
and executable identity may be recorded. Do not commit extracted files,
archives, screenshots containing copyrighted assets, or generated object files.

The eventual Linux and Windows ports must be built from the decompilation and
must preserve the provenance boundary: retail bytes are used as an oracle for
matching and validation, not copied into the distributed port.
