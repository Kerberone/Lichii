# Frizti v3 → Lichii AMD experimental ports

Source release: https://github.com/Frizti/Lichii/releases/tag/frizti-v3 (tag commit `72e5ad6c22ea5b159b44a184d4dea6b669df0ede`).

Target baseline: `test/amd-hmma-pipeline-trace` / PR #1. This experiment MUST NOT modify `nx2` or PR #1 until independently built and tested on Windows with AMD RX 6600 8 GB / Intel i5-10400.

## Already present or different in our baseline
- Pipeline background prebuild and cache-v52 migration are already present, via a different `background_pipelines` design. Frizti's `deferred_graphics` queue implementation cannot be blindly cherry-picked.
- Our AMD deferred HMMA worker, wait-before-replacing Vulkan cache, duplicate compute-key lifetime fixes, pipeline trace, Windows PGO, AV1 DLL packaging and AV1 smoke test must remain intact.
- Dynamic FPS already exists on `nx2` lineage; avoid a second bundled patch.

## First port: Windows GPU/guest thread placement (opt-in)
Source: https://github.com/Frizti/Lichii/commit/bcb570768ce80e7c3435ab179751c6ce59c03d7c

- Port ONLY the `PinCurrentThread` capability and call sites, **NOT** the `SEM_FAST` default change, live switch table or unrelated DMA changes from the source commit.
- Windows-only, OFF by default; enable only with `EDEN_NX2_PIN_THREADS=1`. 0 / unset means current behavior.
- Pin GPU front and GPU back to separate physical cores (when safely available). Keep guest cores off both physical cores including SMT siblings. Validate processor group, CPU topology, core count and nonzero masks; degrade silently to default affinity when not supported; avoid pinning onto a mask outside the process allowed affinity. Avoid new permanent global scheduling changes.
- Keep source and behavior on other platforms unchanged; no unconditional pinning on 4-core or 6-core CPUs.
- Add concise non-sensitive diagnostics when opt-in is enabled, and a focused test if feasible. Do not change any unrelated scheduling, semaphore or memory logic.

Potential second phase **only after first port has a passing build and review**:
1. Benchmark DMA snapshot synchronization https://github.com/Frizti/Lichii/commit/8ac3666f7b514561b20cc9ad1be140d9184dd097 (large, risky; do not port at the same time).
2. Compare smoother host pacing and refresh behavior; inspect https://github.com/Frizti/Lichii/commit/53ba1b2c7a1 and https://github.com/Frizti/Lichii/commit/72e5ad6c22e (avoid falsely raising FPS through interpolation).
3. Investigate pipeline retention and async native draw skipping at https://github.com/Frizti/Lichii/commit/7b5194987b2f698502757b103db9bafd806c20b3. Keep asynchronous native draw discard OFF until visual regressions are checked. An 8,000-pipeline floor (up to ~3.6 GB driver memory) is **not** a safe unqualified default on RX 6600 8 GB.
4. Do not blindly copy Frizti's own Vulkan cache files or cache formats; migration and duplicate cache risks.

## Required build / comparison
- Update the experimental Windows PGO workflow only on this feature branch to build and upload a separate ZIP for the experiment (unique ID, no release, no default-branch changes), retaining DLLs and the AV1 packaged smoke test.
- Compare baseline build-4 against the experimental binary on the **same scene**, 1x resolution and shader/cache state, with GPU Mode Fast. Record FPS and 1% lows, stutter, black screens, driver loss and any white flashes. Toggle `EDEN_NX2_PIN_THREADS=0` vs `=1`.
- Do not claim 120 FPS: Frizti v3 claims ~60 FPS for Kokiri on Ryzen 7 7800X3D / RTX 4080, not measured on our AMD setup.
- Keep the PR in draft until code review and build checks pass; NO AUTO-MERGE, NO paid OpenAI/Anthropic API keys, NO use or distribution of commercial game data.
