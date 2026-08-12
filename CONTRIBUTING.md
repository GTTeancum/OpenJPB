# Contributing

OpenJPB is a reconstruction project, so changes need a clear provenance trail.

## Expectations

- Keep original game assets, executables, symbols, and raw decompiler exports
  out of commits.
- Prefer PDB names, executable traces, and source-backed evidence over visual
  guessing.
- Preserve retail behavior, including quirks, when tests prove it is
  observable.
- Keep platform-specific services behind narrow interfaces.
- Add or update focused tests for behavior changes.

## Build Before Submitting

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

If your change depends on installed retail assets, also run the relevant
`real_assets` tests locally. Do not commit those assets or generated captures.

## Commit Hygiene

- Keep generated files out of commits unless they are intentionally versioned
  inventory metadata.
- Avoid unrelated formatting churn.
- Document newly recovered evidence in `docs/` or `STATUS.md` when it changes
  reconstruction confidence.
