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
See `git log origin/test/amd-hmma-pipeline-trace -1`. Updated in each phase below.

## Plan / status
| # | Phase | Status |
|---|---|---|
| 0 | Journal + branch | done |
| 1 | Verify PR #149 fence fix (ceed7ca) already in build 3 | todo |
| 2 | Port 2f067c1 (deferred AMD software HMMA compute pipelines) | todo |
| 3 | Port pipeline trace 2e46a01 + 31390c3 (`EDEN_NX2_PIPE_TRACE=1`) | todo |
| 4 | Workflow: run on work branch, artifact `amd-hmma-trace-build-4` | todo |
| 5 | Linux compile check + Windows CI run | todo |
| 6 | PR to base (no merge) | todo |

## Not to integrate (per owner)
Whole PR #223, PR #216, resolution experiments, global GPU-only memory changes, global shader
disabling, old pipeline managers, full AMD pipeline serialization (tried and reverted in #223).

## Completed changes
(none yet)

## Tests
(none yet)

## Hypotheses
- Unproven: 0 FPS stalls come from AMD driver compiling large software-HMMA compute shaders on the GPU thread.
- Unproven: abrupt exit happens inside a driver pipeline build call (trace will show last `begin` without `end`).

## WIP not ready
(none)

## Next action
Phase 1.
