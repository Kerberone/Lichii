# AMD OoT progress journal (build 4)

## Goal
Experimental Windows PGO build 4 for Ocarina of Time (0400486024DEC000) on AMD RX 6600
(driver 2.0.353, Vulkan 1.4.315): Vulkan pipeline stability and AMD shader compile stalls.
Keep everything build 3 already had (FFmpeg AV1 + DLL packaging + AV1 smoke test,
upstream AMD fixes #210/#206, nested config array fix, disabled update checker, MinGW PGO workflow).

## Branches
- Base (build 3, do not touch): `audit/windows-pgo-validated` @ `c6baa721880ff0801987d4e6c61f098ca96ef15c`
- Work branch: `test/amd-hmma-pipeline-trace` (created from the base commit)
- Upstream source of patches: `https://github.com/zortos293/Lichii` (PR #149, #223)

## Last published commit
`origin/test/amd-hmma-pipeline-trace` = `01dbfc5` (verified with `git fetch` + `rev-parse`).
Pushes were refused (403, no GitHub App write access) until the owner fixed permissions; the
bundle/patch backup of `01dbfc5` was handed over as session files meanwhile. Pushing works now.

## Plan / status
| # | Phase | Status |
|---|---|---|
| 0 | Journal + branch | done |
| 1 | Verify PR #149 fence fix (ceed7ca) already in build 3 | done (present) |
| 2 | Port 2f067c1 (deferred AMD software HMMA compute pipelines) | done, local `aedb97f`, not compiled yet |
| 3 | Port pipeline trace 2e46a01 + 31390c3 (`EDEN_NX2_PIPE_TRACE=1`) | done, local `1dd1879` (WIP until compiled) |
| 4 | Workflow: run on work branch, artifact `amd-hmma-trace-build-4` | done `656d449` |
| 5 | Linux compile check + Windows CI run | running (see Tests) |
| 6 | PR to base (no merge) | todo |

## Not to integrate (per owner)
Whole PR #223, PR #216, resolution experiments, global GPU-only memory changes, global shader
disabling, old pipeline managers, full AMD pipeline serialization (tried and reverted in #223).

## Completed changes
### Phase 1: PR #149 / ceed7ca (fence deadlock)
Already in build 3 (merged under other SHAs, so `git merge-base --is-ancestor` says no).
`FenceManager::WaitPendingFences(force)` calls `SubmitDeferredWork()` after `SignalFence` and
before blocking; `delay_fence` in `CommitAsyncFlushesHigh` also calls it. Vulkan override
`FenceManager::SubmitDeferredWork()` -> `scheduler.FlushDeferred()` (flushes only if
`flush_pending`). Test `src/tests/video_core/split_fence_wait.cpp` covers it. Not re-applied.

### Phase 2: 2f067c1 (deferred software HMMA compute pipelines)
- Cherry-picked; one conflict in the `PipelineCache` constructor init list: kept build 3's
  `num_pipeline_workers`, `emission_budget`, `background_pipelines`, added `deferred_workers(1, "VkDeferredPipelines")`
  after `workers` (matches header declaration order).
- Adaptation fix (bug in upstream): deferred pipelines get `statistics = nullptr`. During the disk
  load the statistics object is a local of `LoadDiskResources`, and nothing waits for the deferred
  worker, so `Collect()` could run on freed memory.
- Lifetime review: `deferred_workers` is declared after `compute_cache` and `vulkan_pipeline_cache`,
  so it is destroyed (jthreads joined) before them; queued but unstarted builds are dropped with the
  queue. Known risk: a driver compile already running at exit (AMD: up to minutes) delays shutdown
  until it returns; a driver call cannot be cancelled.
- Dispatches of a deferred pipeline: native mode throws `Shader::RuntimeError` ("still being built"
  / "driver failed"), caught in `KeplerCompute`, logged once per message, dispatch skipped. GPU
  thread never waits for it (Configure returns before the `RunBuild` steal path).
- Gate: `Device::DefersSoftwareMatrixPipelines()`: AMD proprietary + AMD open (amdvlk); not RADV.
  `EDEN_NX2_DEFER_MATRIX_PIPELINES=0` off, `=1` forced on any driver.
- Rendering impact: while the HMMA network (upscaler) builds, its dispatches are skipped, so its
  output is missing/stale until built. If the driver fails it (VK_ERROR_OUT_OF_HOST_MEMORY seen on
  RX 5500M/5700), it stays skipped.

### Phase 3: pipeline trace (2e46a01 + 31390c3, adapted)
Header `src/video_core/renderer_vulkan/pipeline_trace.h`. `EDEN_NX2_PIPE_TRACE=1` (any value but
empty/"0"). Line format `PIPE <phase> <graphics|compute> <begin|done|end|fail> key=/cs= [stages] ms= [error=]`.
Phases: `cache` (disk cache load, translation+emission+driver in one), `prebuild` (background
prebuild), `rt` (GPU-thread translation), `emit` (worker SPIR-V emission), `drv` (driver
vkCreate*Pipelines; compute line also says `deferred=`). Logged at Error level: file backend
flushes on Error, so lines survive a crash. Unset: no new lines, no behaviour change.

### Phase 4: workflow (`.github/workflows/release-binaries.yml`)
- Push trigger adds `test/amd-hmma-pipeline-trace` (keeps `audit/windows-pgo-validated`).
- `LICHII_BUILD_ID: amd-hmma-trace-build-4` -> `-DLICHII_BUILD_TAG` and artifact name
  `amd-hmma-trace-build-4-windows-x86_64-pgo-<sha>`. Version string in the emulator stays
  `<commit>-<branch>` (GenerateSCMRev only uses tags matching `build-N`).
- Artifact = the packaged `build/release/bin` folder; GitHub serves it as one ZIP on download
  (uploading a .zip would give a zip inside a zip, see comment in the workflow).
- Unchanged: MSYS2 clang + PGO profile, FFmpeg from MSYS2 (`-DYUZU_USE_BUNDLED_FFMPEG=OFF`),
  DLL dependency copy, `av1_decode_smoke` run against the packaged DLLs, update checker OFF, no release job.

## Tests
- Local Linux compile: NOT possible in the cloud session. CPM downloads of github.com
  release tarballs are refused by the session's network policy (HTTP 403); git fetch works.
- CI compile checks instead:
  - Windows PGO run 4 on `656d449`: https://github.com/Kerberone/Lichii/actions/runs/42215516056 (result: pending)
  - `build-check.yml` (Linux GCC 13, -Werror, unit tests) dispatched on the branch (result: pending)
- Patch backup restore (`git am` and bundle) verified identical to `01dbfc5`.

## Hypotheses
- Unproven: 0 FPS stalls come from AMD driver compiling large software-HMMA compute shaders on the GPU thread.
- Unproven: abrupt exit happens inside a driver pipeline build call (trace will show last `begin` without `end`).

## WIP not ready
- `1dd1879` trace: written, not yet compiled.

## Next action
Wait for both CI runs; fix any compile error; then PR to `audit/windows-pgo-validated` (no merge).
