# AMD RX 6600 — Ocarina of Time: fidelity-first Vulkan investigation

Status: **UNPROVEN ROOT CAUSE / NO GAMEPLAY PASS**. Experimental Windows branch, based on PR #1 / `test/amd-hmma-pipeline-trace` at `0c95985`. No merge to `nx2` or PR #1 until real hardware validates. Run one independent variable per test.

## Non-negotiable rendering contract
**Do not improve FPS by skipping/dropping native draws, compute dispatches, shaders, scene geometry, particles, lighting, fog, shadows, reflections, post-processing, upscaler or other visual passes.** Do not reduce internal resolution, precision, effects, texture quality, shader instruction coverage or accuracy. Preserve the guest's intended output and ordering, including CPU/GPU memory visibility. Do not turn on Frizti's `EDEN_NX2_ASYNC_DISCARD` or `EDEN_NX2_REJECT_HASH` as production "fixes"; no blanket GPU/driver optimization disabling, no shader bypass, no unvalidated default-texture substitutions. A controlled temporary A/B *diagnostic* is acceptable only if explicitly labeled as fidelity-breaking and reverted; not in a release candidate. More FPS must come from **faster correct guest→SPIR-V→Vulkan translation, caching, fewer redundant pipeline/descriptor operations, proper concurrency and synchronization**.

## Input evidence (local user logs; anonymized facts, NOT raw logs)
- Files: `eden_log.txt.old(6).txt` (2,574 lines; final timestamp `[328.071469]`) and `eden_log(6).txt` (38 lines, startup-only). Both identify `test/amd-hmma-pipeline-trace-0e1a499b52`, Win11, i5-10400 (6C/12T), RX 6600 8GB, AMD proprietary Vulkan driver 2.0.353 / Vulkan 1.4.315, API-reported available VRAM 6.30 GiB.
- Game title `0400486024DEC000`; initialized with `Total Pipeline Count: 0` (cold disk cache).
- ~501 `Nx2Perf async_pipelines` lines; 15 `Runtime pipelines in the last 10 s` summaries.
- Long graphics pipeline on GPU thread: `pipeline=3401a1d16e8f702e`, vertex `9be7607dbc31836b`, fragment `db68c2e27d3bce2f`, **80,110.6 ms**. Other graphics worker/driver builds lasted ~89,192 ms; long pipeline timestamps can include waiting/synchronization, not necessarily solely SPIR-V generation.
- At ~327.3 s: **202 pipelines/last 10s**, 7,920.6 ms GPU-thread work, 9,380.7 ms draw wait (these durations overlap; not additive).
- 15 `shader_environment.cpp:393 assert index <= tic_limit`; 36 `format_lookup_table.cpp:280 assert texture format=0 ...`; 16 `Unmapped Device ReadBlock` in a short burst at ~113.6 s. Repetition may be non-fatal (ASSERT logs), and there is NO final stack/exception or `VK_ERROR_DEVICE_LOST` in the log. **Do not attribute crash without dump or targeted trace**.
- 11 distinct `Native compute shader rejected: ... still being built by the driver; its dispatches are skipped until it is ready`, followed by `Native QMD failed` and unsupported dispatch logs (22 occurrences of each category). Crucially includes `EFF3CCBE1438679D` at 43.304 s, independently identified as problematic AMD compiler case. **The current HMMA deferral is a visible-output regression risk by construction**.
- No `PIPE ...` trace lines because `EDEN_NX2_PIPE_TRACE` was not enabled in captured run.
- Settings include `Renderer.enable_compute_pipelines: false` but the Vulkan device code gates `has_broken_compute` with that setting: do not assume ALL native compute is disabled; inspect implementation first.
- Built-in Max Resolution/Dynamic FPS were disabled in game add-ons. Don't change for stability A/B; preserve 1x resolution.

## Sources / prior art to examine before fixes
1. Repo `AGENTS.md`: check upstream Djihads80/Eglantine current `nx2` / open & closed PRs **before writing**; don't blindly merge upstream into this isolated test branch or erase our Vulkan worker lifetime fixes. Source access in GitHub Actions may be blocked: use GitHub API / provide exact excerpts and report limitations.
2. Same symptoms in `eden-emulator/Issue-Reports#751` (out-of-range TIC index and invalid formats leading to a possible fatal `SurfaceFormat(PixelFormat::Invalid)`). Different title/card; **hypothesis, not proof**: https://github.com/eden-emulator/Issue-Reports/issues/751
3. AMD driver extreme stalls & hashes `EFF3CCBE1438679D` and `F9EA14A22394F5B1`, long first compile and correct but slow unoptimized output: https://github.com/zortos293/Lichii/issues/200
4. AMD shader correctness, narrowed noopt hash allowlist + fragment subgroup shuffle fallback (already possibly inherited in our build; confirm presence before port): https://github.com/zortos293/Lichii/pull/273
5. Misaligned 64/128-bit SSBO accesses and proper compute→graphics/indirect Vulkan barriers: https://github.com/zortos293/Lichii/pull/295
6. Texture/macro desync related report on same game: https://github.com/zortos293/Lichii/issues/183
7. Frizti v3 performance comparison; do NOT port native-draw discards or huge graphics cache by default (RX 6600 8 GB): https://github.com/Frizti/Lichii/releases/tag/frizti-v3
8. Khronos Vulkan pipeline cache guidance: https://github.com/KhronosGroup/Vulkan-Guide/blob/main/chapters/pipeline_cache.adoc

## Work packages (separate incremental commits; analyze and implement only validated changes)

### A. Protect texture descriptor provenance and identify actual crash (highest priority)
- Inspect `GenericEnvironment::ReadTextureInfo`, `TextureHeaderIndex` (Switch 2 byte-offset transform), TIC pool bounds, mapping, header version, constant buffer / texture handle lifetime, access ordering, invalidation and shader compilation concurrency.
- Current code `ASSERT(index <= tic_limit)` **does not stop**; it then performs `gpu_memory->ReadBlock` at the out-of-range descriptor address. `PixelFormatFromTextureInfo` logs a failure and returns `A8B8G8R8_UNORM`; `SurfaceFormat` has an `UNREACHABLE_MSG` for invalid enum. Determine which actual paths are reachable; don't claim the final abort without dump.
- Add **opt-in**, bounded diagnostics of raw/index/limit/TIC address/mapped/source stage/program and before/after synchronization, without uploading full game shaders, GPU memory or user data. If possible prevent demonstrably out-of-range reads correctly (no arbitrary clamp / bogus texture selected). Identify and fix the actual handle/descriptor translation or read-after-write ordering. If unresolvable from static review, leave feature guarded/off and report precise missing evidence instead of silently replacing formats.
- Add focused tests for `TextureHeaderIndex`, bounds/overflow/native vs legacy handle conditions and mapping validation; no fabricated visuals.

### B. Stop losing compute output while AMD driver compiles
- The current `deferred_workers` policy **skips the guest dispatch** while the pipeline is building; user explicitly forbids dropped effects/output. This is not acceptable as a "performance" optimization. Design a correctness-preserving policy: compile in background ahead of use then wait/retry when required; every guest dispatch must eventually execute **once in original order** with valid dependencies. If implementing full queued replay is too risky, prefer disabling deferral on this branch by default with **only explicit opt-in** to legacy skip behavior for testing; this may mean initial shader stall and is acceptable as a baseline. Do NOT erase cache/lifetime fixes.
- AMD specific `BuildsUnoptimized` path is already present in compute code; confirm it handles the two known AMD hashes, and DO NOT turn it off globally or replace shader output with black. Distinguish **driver optimization flag** from **guest shader correctness**.
- Add tests that exercise not-ready pipeline, completion, failure, reload and shutdown paths; no missed dispatches, no data races, no unbounded GPU thread lock or double dispatch.

### C. Performance from full-fidelity translation and Vulkan efficiency (after A/B)
- With `EDEN_NX2_PIPE_TRACE=1`, `EDEN_NX2_PIPELINE_DIFF=1` and bounded timing detail, isolate time in: guest code/Cbuf reads, SPIR-V IR conversion, SPIR-V emission, driver `vkCreate*Pipelines`, GPU-thread synchronization, ASTC GPU decode, disk cache load/reuse and visual correctness; do not attribute 80s all to the AMD compiler without trace.
- Analyze duplicate graphics state key variants and driver cache reuse, preload/async SPIR-V emission, batching valid descriptor updates / uniform bank uploads, lock contention, GPU cache barrier correctness, and page-granular readbacks. Propose small correctness-preserving, opt-in performance changes with before/after timing; no blanket caches requiring >6 GiB of available VRAM. **Never use draw/dispatch dropping to mask stalls.**
- Compare baseline and experiment at same game settings, shader cache state (cold and warm), scenes, CPU/GPU load, mean/1%-low FPS and visual checks. CI compilation and synthetic tests do NOT prove AMD gameplay performance.

## Build and review acceptance
- Windows x86_64 MinGW/clang PGO unique artifact; DLL bundling, AV1 packaged smoke test and update-checker disabled; no release/merge.
- Focused unit/regression tests, clang/GCC where relevant, Codex independent code review for memory safety, Vulkan ordering, data races, performance and fidelity before requesting owner test.
- Preserve PR #1 head and PR #5 affinity experiment untouched, keep new PR draft and merged `nx2` untouched; no paid API keys or commercial game dumps. Note GitHub Actions job may have broad write privileges despite prompt; only commit to this feature branch, never merge.
- Request **Windows crash minidump** from owner if needed (`%LOCALAPPDATA%\\CrashDumps\\lichii.exe.<pid>.dmp`) and correlate exception stack with matching artifact PDB. Do not upload dumps publicly without explicit owner approval.

## Controlled first A/B for owner when an artifact exists
1. With the **same** build and game data and no trace, default/fidelity mode with HMMA not skipped; observe whether it boots, and note wait time.
2. If the crash persists, launch the **same** binary with `EDEN_NX2_PIPE_TRACE=1`, `EDEN_NX2_RENDER_TRACE=1`, and **`EDEN_NX2_TIC_TRACE=1`** (required to capture out-of-range TIC index/raw handle/limit/mapping; `RENDER_TRACE` alone only reports zero-format descriptors). Capture fresh `eden_log.txt.old`, especially the first `Out-of-range shader TIC` and the last unmatched `PIPE ... begin` line. These diagnostic flags are for reproduction only, not performance benchmarking.
3. Report actual AMD driver, shader/pipeline hashes, hardware outcome and any visual differences. Keep the screenshots private unless explicitly shared.
