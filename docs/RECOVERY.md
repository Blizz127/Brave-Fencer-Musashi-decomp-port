# Candidate verification

A candidate is a draft C implementation identified by its region and function
address. A C89 syntax check catches missing declarations and type errors.
Matching also requires building the draft with the historical compiler,
linking it at the correct address and comparing the resulting bytes with
the corresponding code from a contributor's own disc.

## Reusing drafts

`tools/batch_match.py` can use an existing directory of drafts named
`func_XXXXXXXX.c`. With `--candidate-dir` it tries the available draft
first and generates a new draft with m2c when the file is missing. Add
`--banked-only` to test only files already present. The original draft files
remain available after the comparison.

Check the planned candidates before running a bounded comparison. For
example, with a main disassembly, toolchain and registry set up locally,
supply your draft directory as `CANDIDATE_DIR` and your registry path as
`REGISTRY`.

```sh
python3 tools/batch_match.py --asm asm/main.s --region main \
  --registry "$REGISTRY" --candidate-dir "$CANDIDATE_DIR" \
  --banked-only --dry-run --limit 50
```

For a multi-batch run, save the planned list first and resume after the
last address actually attempted, including failed candidates. Regenerating
a limited list after registering matches moves previously untested addresses
into the vacated positions.

The sanitizer used for m2c output is implemented in `tools/batch_match.py`
and covered by `tests/test_batch_match.py`. See [Matching](MATCHING.md)
for linking, validation and historical toolchain details.
