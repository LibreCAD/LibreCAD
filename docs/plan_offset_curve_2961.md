<!--
********************************************************************************
This file is part of the LibreCAD project, a 2D CAD program

Copyright (C) 2026 LibreCAD.org
Copyright (C) 2026 Dongxu Li (github.com/dxli)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
USA.
********************************************************************************
-->
# Editable Offset Curves: Closeout Plan (#2961)

Updated: 2026-11-26. Scope: **LibreCAD** and [PR #3193](https://github.com/LibreCAD/LibreCAD/pull/3193).

## Baseline and Completeness

Audited PR head `6a5e702ec643173a715b6f39a4d33169b1b5b90e` against its `master` base
`954217a719f7fff1f54bdf4ee894a0e462e21934`, an ancestor of the head.
Exact-head CI finished **19 passed, 7 skipped, 0 failed**, including the
[Linux fast and both Windows DWG/DXF lanes](https://github.com/LibreCAD/LibreCAD/actions/runs/45802689061)
and [macOS packaging](https://github.com/LibreCAD/LibreCAD/actions/runs/45802689210/job/139849781506).
The PR is draft, review required. These results cover the jobs actually run,
not every test or geometry variant on every platform.

| Area | Status and remaining work |
| --- | --- |
| S0, A/B/C, E: reconciliation, geometry, checker, entity and transforms | Implemented. Keep the existing kernel, proofs, ownership and work accounting; reopen only for a concrete failure. |
| W0-W2, G, U: isolated worker, staged import, preview/create/repeat, grips and Properties | Implemented in both build paths. Relocated-helper CI is not signed deployment or visible GUI evidence. X-H1-H7 is the separate developer runner, not the shipped worker. |
| U4/O: source replacement, highlight, Update from Source and Restore Original | Implemented. Full offscreen Properties suite: 54,599 assertions / 112 cases in source-equivalent CMake Release and ASan/UBSan, leak detection off. Visible packaged-app acceptance is pending. |
| F/T: DXF/DWG, clipboard, Trim and Trim Two | Implemented. Scoped file, independent-reader and Trim receipts exist. The broad Release Trim inventory passed at its source pin; exhaustive sanitizer runs for individual profile families do not close the full sanitizer/order lane. |
| R: integrated acceptance and delivery | **Open.** No known missing product module; close the gates below or fix the specific defect they expose. Do not restart implemented phases. |

### Invariants

- One Properties selector, populated from `LC_OffsetSourceGeometry::SupportedKinds`:
  line, arc, circle, ellipse, spline, spline-points/Bezier, **distinct parabola**,
  bounded hyperbola and polyline. Each filter and **All supported types** browse
  candidates without changing the committed source/cache or its highlight.
  The edited offset cannot select itself, including via its dialog clone.
  A different native offset may be captured as a spline.
- Worker-validated replacement adopts the real typed source and moves the
  highlight to the exact selected entity. No persistent live source link.
  Cancel, no-op, invalid/stale input and failed or late work preserve geometry,
  metadata, selection and undo. Retain one-step undo/redo, atomic Trim Two,
  bounded computation and explicit ordinary fallback or refusal.
- Schema-1 ellipse compatibility and schema-2 typed SPLINE carriers retain
  registered APPID/EED and supported foreign metadata. ASCII DXF read/write:
  R14 and R2000/04/07/10/13/18; native DWG read/write: R2000/04/07/10/13/18;
  R13/R14 DWG fixture input only; R12 DXF geometry-only; binary DXF R14/R2018
  at codec level only. Keep degree 1/3, both conversion directions for common
  versions, second save/reopen, clipboard, malformed-carrier and refused-write
  coverage. Do not infer R2007 membership from an enum range.
- Qualify the recorded admitted variants through independent geometry checks,
  worker/cache, edits, Trim where eligible and applicable file lanes. Picker
  presence does not qualify arbitrary knots, conic domains or polyline topology.
  Record unqualified boundaries and explicit atomic refusals; no silent loss.
- Out of scope: live links, Extend/Fillet, LibreCAD_3, Vulkan/Metal or other new
  rendering, generic text/image work and unrelated codec/build refactors. The
  [pinned plan][history] retains the detailed geometry/transaction requirements.

## Execution Order

Start **1** now. Run **2** in parallel once desktop access is available; secure
the dependencies for **3 and 4** in parallel. **5** follows the acceptance work.

| Step | Readiness | Acceptance |
| --- | --- | --- |
| **1. Complete the automated matrix** | **Ready locally** with the existing CMake/Qt6 toolchain and real sibling worker. Missing nonlocal rows need their corresponding runner. | Match receipts by source, case/profile, platform and configuration. Run missing portable family/variant rows first, including thin-ellipse cusp and mixed-bulge regressions where the broad variants were not exercised. Finish broad Trim sanitizer and shared-process order checks. Require integrated-suite evidence on the final candidate. |
| **2. Run the packaged-app walkthrough** | **Blocked** on observable desktop control or a human operator. The fixed copy launched and sampled in the normal event loop, but desktop control could not attach. | Use the candidate artifact, disposable fixtures and an isolated or approved profile. Complete the walkthrough below; an older build, app launch or offscreen test does not close it. Do not re-sign a working artifact just to retry access. |
| **3. Validate independent CAD** | **Partial**: bounded ODA and LibreDWG evidence exists; independent native DWG viewing and AutoCAD acceptance need an appropriate app/operator. | Compare the generated corpus, including degree 1/3 carriers and LINE controls. Verify geometry, layer/pen and metadata across the applicable round trips. Observe native DWG geometry in an independent CAD viewport and record AutoCAD open/audit, sampled geometry and save/reopen; check retained offset metadata and native adoption back in LibreCAD. Do not expect AutoCAD to edit LibreCAD's native offset definition. |
| **4. Qualify deployed macOS** | **Blocked** on an authorized release owner, Developer ID/notarization and macOS 14.4 access. | CI uses an ad-hoc universal DMG on `macos-15-intel` with deployment target 14.4; that is not runtime on 14.4. Reuse `scripts/build-osx.sh` and `scripts/test-osx-{package,dmg-runtime}.sh`; retain artifact hash, architecture, signature/notarization/staple verification and actual OS. Check app/worker launch, a worker-backed edit and save/reopen on 14.4. Retain Linux/Windows relocation evidence separately. |
| **5. Review and close out** | **Depends on 1-4 and maintainer review**. D0 grip-policy approval is not recorded. | Resolve the provisional distance grips (eligible axis vertices and arc midpoint; centre snap). Recheck the final diff against current master, rerun affected integration checks, wait for exact-head CI and update the PR with evidence and limits. Leave release, review and merge decisions under maintainer control. |

### Automated Runbook

1. Pin a clean isolated checkout of the actual PR head; leave other worktrees and
   untracked fixtures alone. Record toolchain/build flags and discover case names
   with `ctest --test-dir "$build" -N` and Catch `--list-tests`. Reuse a matching
   build. Configure `BUILD_TESTS=ON` and
   `LIBRECAD_REGISTER_OFFSET_WORKER_TESTS=ON`; verify DWG-enabled targets.
   Use separate Release and ASan/UBSan trees, at most eight build jobs, and
   serialize runners that share an executable or fixture directory.
2. Build the affected targets, then run
   `ctest --test-dir "$build" --output-on-failure -R '^offset_worker_'`.
   Reuse [CI's recipes](../.github/workflows/dwg-dxf-verify.yml) for the fault,
   worker/client and relocated-helper lanes. On platforms with missing geometry
   evidence, start with the existing worker-kernel selectors `[ellipse][cusp]`
   and `[mixed-bulge][region-boundary][topology]`, then the related client cases
   identified in pinned §§2005-2009. For local GUI logic run
   `librecad_tests '[gui][dialog][offsetcurve]'` and
   `librecad_offset_creation_action_tests '[creation-worker]'` offscreen.
   The pinned §5 lists the broader required CMake selectors, filter and standalone
   targets and Qt6 qmake instructions. Verify discovery before counting passes.
3. Run `scripts/run_offset_trim_case_matrix.py` and
   `scripts/run_offset_trim_profile_matrix.py` with the current test binary,
   `--log-dir` and `--summary-json`. First run
   `PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s scripts -p test_run_offset_trim_profile_matrix.py -v`.
   The profile manifest hashes the test binary, checks it stayed unchanged and
   records bounds and hyperbola mode; also retain the worker identity and
   sanitizer options separately. Derive the profile ranges from
   the profile runner. Exclude an exact case from the case runner only when its
   full profile range is accounted for; compare the union by identity, not by
   count. Reuse applicable exhaustive parabola-tangent/periodic-spline receipts
   only after checking changes since their pins. Run affected selectors together
   in declaration and seeded random order to detect shared-process leaks that
   isolated cases miss. Record test-only deadlines and sanitizer options; do
   not raise production limits or waive unexplained failures.
4. Generate one fixture set for Steps 2 and 3: run the exact test
   `Native source picker filters and replaces every supported source family`
   with `LIBRECAD_DWG_FIXTURE_DIR` pointing at a new retained directory. Use the
   R2018 DXF/DWG for visible copies and the six emitted DWGs for ODA Audit to a
   separate R2010 ASCII DXF directory. Run `[oda][all-family]` in
   `librecad_offset_hostilefile_tests` with `LIBRECAD_ALL_FAMILY_ODA_DWG_DIR`
   and `LIBRECAD_ALL_FAMILY_ODA_DXF_DIR`. Check nonempty output per input,
   no error files, geometry and typed data, true color, reserved flags, owned
   tail and foreign XDATA. The prior comparison passed 20,575 assertions on
   six pairs; it is not visual or AutoCAD acceptance. Use the pinned §5.2
   corpus for version/degree/control rows not covered by this fixture.

#### Remaining Trim Matrix

1. The periodic-spline profile manifest is reconciled below. Its isolated
   profiles do not replace the shared-process run.
2. The six timed-out generators now have complete isolated profile receipts,
   including the typed-parabola profiles. The reconstructed-parabola, default
   hyperbola, typed-parabola, beyond-cusp and seam aggregates also passed
   separately below; rerun the high-degree spline as a bounded single-process
   case. Multi-case
   shared-process behavior remains open; do not infer it from isolated manifests.
3. Where an existing selector cannot isolate every tuple, add a test-only
   profile selector to that case and register it in the existing profile
   runner. Preserve the unpinned test, use stable complete tuple identities,
   reject invalid IDs, and test first/last IDs and invalid input. Do not count
   a skipped tuple or a process with no exercised assertions as a pass. Compare
   the union against the original generator including nested format loops;
   document any intentionally conditional rows and retain failing logs.
4. Run each partition under the recorded sanitizer settings with bounded
   per-profile processes and retain manifests. Then investigate the two
   intermittent Update failures and the aggregate/shared-process behavior on
   supported configurations; isolated passes and Release passes do not close
   either issue. Add a regression and fix the responsible owner only when a
   product defect is reproduced. Keep production deadlines unchanged.

### Visible Walkthrough

1. Open generated native DXF and DWG copies in the candidate app. Open offset
   Properties, confirm the existing source indication and **Select in drawing**,
   then cycle the nine family filters and **All supported types**. The committed
   highlight must persist and the edited offset must not appear as a candidate.
   Accept one replacement per family (including an actual parabola) and record
   highlight transfer, typed source, undo and redo. Check that a wrong-family
   candidate is filtered, a hidden or locked source cannot be selected in the
   drawing, and cancellation or a stale candidate cannot replace the source.
2. In the same session exercise Offset/create/repeat, compact options and
   context menus. Capture matched live preview and commit in the same viewport,
   with actual application-window DPR; include held-grip away/back and Escape,
   and a stale peer before publication. Check Update/Restore Original, Trim and
   Trim Two notices and atomic undo, and save/reopen disposable native DXF/DWG
   and R12 geometry-only copies. Include Restore Undo/Redo Save/Open. Retain
   screenshot pairs, Qt/display details, input hashes, observed geometry and
   editability. An absent frame or unobservable interaction is still open.

## Recent Evidence

- On 2026-11-26, the source-equivalent macOS CMake Debug ASan/UBSan build
  ran the shared-process selector `[trim-worker-preview],[trim-worker-commit],
  [trim-worker-action-fault]` in declaration order and random order with seed
  3193. Both passed **619 assertions / 17 cases** with
  `ASAN_OPTIONS=detect_leaks=0` and
  `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. The scratch changes match
  PR source except for comments and an equivalent local variable in
  `dwgbufferw.cpp`; this is not an exact-head build. This qualifies those cases,
  not leak detection, the broad Trim matrix, other platforms or visible GUI.
  Next: reconcile the remaining Trim case/profile inventory by identity.
- Source-equivalent macOS Debug ASan/UBSan per-case Trim run: **92/100
  passed**, six aggregate timeouts at 300 seconds, and two cases reported one
  assertion failure each. Four separately profiled cases were excluded. Manifest:
  `/private/tmp/offset-3193-trim-asan-current.json`; case logs:
  `/private/tmp/offset-3193-trim-asan-current-logs` (temporary, not durable).
  Timeouts: bounded hyperbola file lanes, typed parabola against conic cutters,
  transformed beyond-cusp parabola, high-origin closed-polyline seam, rational
  high-degree spline file lanes, and reconstructed parabola transformed frames.
  No sanitizer diagnostic was reported by the completed cases; do not call
  timed-out generators passed or raise production deadlines.
- The concave-polyline high-origin case failed once in the case matrix on an
  Update from Source assertion (R14, origin 1e9, angle 0, scenario 0). Test-only
  commit `fdaee7ac0` adds `LC_OFFSET_TRIM_CONCAVE_REPRO=1` and refusal context.
  That pinned tuple passed twice under ASan/UBSan and once in Release (68
  assertions each). The full generator failed on one diagnostic rerun and
  passed another (18,910 assertions); the truncated failed rerun did not retain
  its tuple. This is intermittent, not a proven fix. The closed convex-polyline
  matrix also failed an Update assertion for rounded profile 1 and format 8;
  its existing profile/format pin passed in isolation (228 assertions, ASan).
  Preserve both failures and investigate with diagnostic logs and stable worker
  status/PID before classifying cause. Subsequent diagnostic and Release runs
  are recorded below; the sanitizer failures remain open.
- A disposable scratch-only diagnostic rerun reproduced the closed rounded
  profile failure and the concave tuple failure. Both returned `ProcessFailed`,
  exit code 9, reaped, at 128 ms and 123 ms respectively; neither reported a
  worker deadline. The macOS crash report for PID 87124 identifies an ASan
  allocator unfair-lock corruption in the child-side fork hook before exec,
  reached through QtCore `forkfd_fork_fallback` / `QProcessPrivate::startProcess`.
  This identifies the observed concave failure's launch path, not its ultimate
  trigger or whether a non-sanitized build can exhibit it. PID 77890's crash
  report was not located, so do not assert the same stack for the convex case.
  The temporary diagnostic was removed and the sanitizer target rebuilt. Next:
  preserve the crash evidence, compare launch behavior on the supported Qt/macOS
  configurations and reproduce both full cases in Release; keep sanitizer and
  all timed-out cases open until accounted for. Do not hide or retry away the
  original failures or change production deadlines.
- The complete concave and closed-convex generators subsequently passed in
  the source-equivalent macOS CMake Release build: **18,910** and **6,270**
  assertions respectively, one case each, using the 30-second test-only wait.
  Receipts: `/private/tmp/offset-3193-concave-release.txt` and
  `/private/tmp/offset-3193-convex-release.txt` (temporary). These passes do
  not eliminate the ASan fork failure or qualify other platforms.
- With the reproduction/profile pins unset, both full generators also passed
  a later source-equivalent macOS Debug ASan/UBSan rerun with the same options
  and 30-second test-only worker wait: **18,910 concave assertions** and
  **6,270 convex assertions**, one case each, under a 600-second per-case
  bound. Temporary manifest and logs:
  `/private/tmp/offset-3193-flaky-generators-asan.json` and
  `/private/tmp/offset-3193-flaky-generators-asan-logs`. The two retained logs
  contain no sanitizer diagnostics. This is another intermittent observation,
  not a fix or a reclassification of the previously recorded failures.
- The typed-parabola aggregate that timed out was split with
  `run_offset_trim_profile_matrix.py`: **152/152 profiles, 19,152 assertions**
  passed on the source-equivalent macOS CMake Debug ASan/UBSan build. The runner
  used a 180-second per-profile bound and `--worker-timeout-ms 30000`; sanitizer
  options were `detect_leaks=0:quarantine_size_mb=16:thread_local_quarantine_size_kb=256:malloc_context_size=5`
  and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. Manifest and per-profile
  logs are in `/private/tmp/offset-3193-trim-parabola-asan.json` and
  `/private/tmp/offset-3193-trim-parabola-asan-logs` (temporary). This accounts
  for the 152 recorded profiles in isolated processes; it does not resolve the
  multi-case order, leak detection, other timed-out cases, or other platforms.
  The aggregate is recorded below.
- The unpinned typed-parabola aggregate subsequently passed in one
  source-equivalent macOS Debug ASan/UBSan test process with **19,001 assertions**
  in 325.549 seconds, using the same sanitizer options and 30-second test-only
  worker wait, with a 900-second test-only case bound. This matches the 19,152
  isolated assertions after removing one repeated guard for each extra process.
  The retained log has no sanitizer diagnostic; temporary receipt and log:
  `/private/tmp/offset-3193-typed-parabola-aggregate-asan.json` and
  `/private/tmp/offset-3193-typed-parabola-aggregate-asan-logs`. The prior
  300-second timeout remains historical evidence; multi-case order, leak
  detection and non-macOS behavior remain open.
- The separately excluded typed-ellipse generator passed **76/76 profiles,
  9,252 assertions** on the same source-equivalent macOS Debug sanitizer build
  and settings, using the `ellipse` profile runner and 180-second per-profile
  bound. Manifest and logs: `/private/tmp/offset-3193-trim-ellipse-asan.json`
  and `/private/tmp/offset-3193-trim-ellipse-asan-logs` (temporary). This is
  isolated profile evidence; shared-process order, leak detection, and other
  platforms remain separate gates. High-origin ellipse is recorded below.
- The separately excluded high-origin ellipse generator then passed **52/52
  profiles, 4,500 assertions** on the same source-equivalent macOS Debug
  sanitizer build and settings, using `high-origin-ellipse` and the same
  180-second per-profile bound. Manifest and logs:
  `/private/tmp/offset-3193-trim-high-ellipse-asan.json` and
  `/private/tmp/offset-3193-trim-high-ellipse-asan-logs` (temporary). This
  qualifies these isolated rows, not leak detection, prior shared-process
  failures or other platforms.
- The excluded endpoint-tangent parabola family also passed **223/223 profiles,
  269,518 assertions** on the same source-equivalent macOS Debug ASan/UBSan
  build and options, using `parabola-tangent` with the 180-second per-profile
  bound. Manifest and logs: `/private/tmp/offset-3193-trim-tangent-asan.json`
  and `/private/tmp/offset-3193-trim-tangent-asan-logs` (temporary). The
  one other timed-out aggregate case, prior shared-process failures, leak
  detection and non-macOS validation remain open. The periodic matrix and
  reconstructed, hyperbola, typed-parabola, beyond-cusp and seam aggregates are
  recorded above or below.
- The periodic-spline matrix passed **1,080/1,080 profiles and 145,371
  assertions** under the same source-equivalent macOS Debug ASan/UBSan build
  and options, with 180-second per-profile and 30-second test-only worker
  bounds. All IDs 0-1079 occur exactly once in requested, passed and result
  lists; all 1,080 logs were checked for sanitizer diagnostics. Temporary
  manifest and logs: `/private/tmp/offset-3193-trim-periodic-asan.json` and
  `/private/tmp/offset-3193-trim-periodic-asan-logs`. During and after the
  run, test binary SHA-256 was
  `00a025386467ebf513dd150b9811dc962d3aeb017e7989f7762665f01e7ea8b9`
  and worker SHA-256 was
  `50a0140d656600d0e844b0251d9fac3d82045f7dd969aff0550c1411dd3e3eb2`.
  The pre-rebuild binary predates the new test-only selectors; the periodic
  test body and relevant product source are equivalent, not exact-head.
  This does not qualify shared-process order, leak detection or other platforms.
- Test-only commit `19bafe101` adds a `closed-seam` profile selector for the
  high-origin closed-polyline Trim Two case. The 38 original rows, including
  one duplicate tuple, were compared in order against the prior source.
  Source-equivalent macOS CMake Release passed **38/38 isolated profiles and
  8,102 assertions**, and the unpinned aggregate passed **8,102 assertions**.
  Invalid, out-of-range and conflicting reproduction selectors were rejected;
  the existing unpinned reproduction passed. Receipts:
  `/private/tmp/offset-3193-closed-seam-release.json` and
  `/private/tmp/offset-3193-closed-seam-aggregate-release.json` (temporary).
  This accounts for the Release rows, not the earlier sanitizer timeout,
  leak detection or other platforms.
- The `closed-seam` selector subsequently passed **38/38 isolated profiles
  and 8,102 assertions** under source-equivalent macOS CMake Debug ASan/UBSan
  with `detect_leaks=0:quarantine_size_mb=16:thread_local_quarantine_size_kb=256:malloc_context_size=5`,
  `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`, a 180-second profile
  bound and 30-second test-only worker wait. Requested, passed and result IDs
  match 0-37; all 38 logs are free of sanitizer diagnostics. Manifest and logs:
  `/private/tmp/offset-3193-closed-seam-asan.json` and
  `/private/tmp/offset-3193-closed-seam-asan-logs` (temporary). The manifest
  records an unchanged test binary SHA-256
  `070a3128c904c57a276bce9623d60ffd9cd62c87593edad52e5a10c614e5d126`;
  worker SHA-256 was
  `50a0140d656600d0e844b0251d9fac3d82045f7dd969aff0550c1411dd3e3eb2`.
  The aggregate is recorded below; multi-case behavior, leak detection,
  earlier worker-launch failures and other platforms remain open.
- The unpinned closed-seam aggregate subsequently passed in one
  source-equivalent macOS Debug ASan/UBSan test process with **8,102 assertions**
  in 484.776 seconds, using the same sanitizer options and 30-second test-only
  worker wait, with a 900-second test-only case bound. The retained log has no
  sanitizer diagnostic; temporary receipt and log:
  `/private/tmp/offset-3193-closed-seam-aggregate-asan.json` and
  `/private/tmp/offset-3193-closed-seam-aggregate-asan-logs`. The prior
  300-second timeout remains historical evidence; multi-case order, leak
  detection and non-macOS behavior remain open.
- Test-only commit `eacd80642` adds the `reconstructed-parabola` frame
  selector, retaining each frame's format lanes. Source-equivalent macOS CMake
  Release passed **86/86 profiles, 7,790 assertions**, and the unpinned aggregate
  passed **7,620 assertions**. The difference is the two selector guards
  repeated in each isolated process. Invalid and out-of-range IDs were rejected;
  each profile exercised at least 38 assertions. Receipts:
  `/private/tmp/offset-3193-reconstructed-release.json` and
  `/private/tmp/offset-3193-reconstructed-aggregate-release.json` (temporary).
  The sanitizer aggregate is assessed below; non-macOS rows remain open.
- The `reconstructed-parabola` selector subsequently passed **86/86 isolated
  profiles and 7,790 assertions** under the same source-equivalent macOS
  Debug ASan/UBSan settings and bounds as the seam run. Requested, passed and
  result IDs match 0-85, the test-binary hash remained unchanged, and all 86
  logs were free of sanitizer diagnostics. Temporary receipt and logs:
  `/private/tmp/offset-3193-reconstructed-asan.json` and
  `/private/tmp/offset-3193-reconstructed-asan-logs`. The aggregate is recorded
  below; multi-case behavior, leak detection and other platforms remain open.
- The unpinned reconstructed-parabola aggregate subsequently passed in one
  source-equivalent macOS Debug ASan/UBSan test process with **7,620 assertions**
  in 311.625 seconds, using the same sanitizer options and 30-second test-only
  worker wait, with a 900-second test-only case bound. The retained log has no
  sanitizer diagnostic; receipt and log:
  `/private/tmp/offset-3193-reconstructed-aggregate-asan.json` and
  `/private/tmp/offset-3193-reconstructed-aggregate-asan-logs` (temporary).
  The prior 300-second timeout remains historical evidence; this result does
  not qualify multi-case order, leak detection or non-macOS behavior.
- Test-only commit `03d32a2ec` adds the `beyond-cusp` selector over the 144
  existing nested combinations, retaining their format loops. Source-equivalent
  macOS CMake Release passed **144/144 profiles, 13,596 assertions**, and the
  unpinned aggregate passed **13,024 assertions**; the difference is four
  per-process setup/count assertions repeated in isolated profiles. Invalid and
  out-of-range IDs were rejected, and each profile exercised at least 89
  assertions. Receipts: `/private/tmp/offset-3193-cusp-release.json` and
  `/private/tmp/offset-3193-cusp-aggregate-release.json` (temporary). This is
  not sanitizer, leak-detection or non-macOS evidence.
- The `beyond-cusp` selector subsequently passed **144/144 isolated profiles
  and 13,596 assertions** under the same source-equivalent macOS Debug
  ASan/UBSan settings and bounds as the seam run. Requested, passed and result
  IDs match 0-143, the test-binary hash remained unchanged, and all 144 logs
  were free of sanitizer diagnostics. Temporary receipt and logs:
  `/private/tmp/offset-3193-cusp-asan.json` and
  `/private/tmp/offset-3193-cusp-asan-logs`. The aggregate is recorded below;
  multi-case behavior, leak detection and other platforms remain open.
- The unpinned beyond-cusp aggregate subsequently passed in one
  source-equivalent macOS Debug ASan/UBSan test process with **13,024 assertions**
  in 495.552 seconds, using the same sanitizer options and 30-second test-only
  worker wait, with a 900-second test-only case bound. The retained log has no
  sanitizer diagnostic; temporary receipt and log:
  `/private/tmp/offset-3193-cusp-aggregate-asan.json` and
  `/private/tmp/offset-3193-cusp-aggregate-asan-logs`. The prior 300-second
  timeout remains historical evidence; multi-case order, leak detection and
  non-macOS behavior remain open.
- Test-only commit `b93601b3b` adds the `hyperbola` selector for 20
  branch/orientation/format groups while retaining the other Catch generators.
  With both high-origin environment flags unset, source-equivalent macOS CMake
  Release passed **20/20 profiles and 14,856 assertions**; the unpinned
  aggregate passed the same **14,856 assertions**. Invalid and out-of-range
  IDs were rejected. Receipts: `/private/tmp/offset-3193-hyperbola-release.json`
  and `/private/tmp/offset-3193-hyperbola-aggregate-release.json` (temporary).
  The sanitizer aggregate is assessed below; non-macOS rows remain open.
- The default `hyperbola` selector subsequently passed **20/20 isolated
  groups and 14,856 assertions** under the same source-equivalent macOS Debug
  ASan/UBSan settings and bounds as the seam run. Both high-origin flags were
  unset; requested, passed and result IDs match 0-19, the test-binary hash
  remained unchanged, and all 20 logs were free of sanitizer diagnostics.
  Temporary receipt and logs: `/private/tmp/offset-3193-hyperbola-asan.json`
  and `/private/tmp/offset-3193-hyperbola-asan-logs`. The aggregate is recorded
  below; multi-case behavior, leak detection and other platforms remain open.
- The unpinned default hyperbola aggregate subsequently passed in one
  source-equivalent macOS Debug ASan/UBSan test process with **14,856 assertions**
  in 308.937 seconds, with both high-origin flags unset, the same sanitizer
  options and 30-second test-only worker wait, and a 900-second test-only case
  bound. The retained log has no sanitizer diagnostic; receipt and log:
  `/private/tmp/offset-3193-hyperbola-aggregate-asan.json` and
  `/private/tmp/offset-3193-hyperbola-aggregate-asan-logs` (temporary). The
  prior 300-second timeout remains historical evidence; multi-case order,
  leak detection and non-macOS behavior remain open.
- The same hyperbola selector passed **20/20 profiles and 4,952 assertions**
  with `LC_OFFSET_HYPERBOLA_HIGH_ORIGIN=1` and the very-high flag unset; it
  passed **20/20 and 7,608 assertions** with
  `LC_OFFSET_HYPERBOLA_VERY_HIGH_ORIGIN=1` and the high flag unset. Both used
  the same source-equivalent macOS Release build and test-only 30-second worker
  wait. Temporary manifests: `/private/tmp/offset-3193-hyperbola-high-release.json`
  and `/private/tmp/offset-3193-hyperbola-very-high-release.json`. These are
  isolated Release profiles, not aggregate, sanitizer or non-macOS evidence.
- The hyperbola high-origin mode subsequently passed **20/20 isolated groups
  and 4,952 assertions** under the same source-equivalent macOS Debug ASan/UBSan
  settings and bounds as the seam run. Its manifest records
  `LC_OFFSET_HYPERBOLA_HIGH_ORIGIN=1` and the very-high flag unset; IDs 0-19
  are complete, the binary hash was unchanged, and all 20 logs were free of
  sanitizer diagnostics. Temporary receipt and logs:
  `/private/tmp/offset-3193-hyperbola-high-asan.json` and
  `/private/tmp/offset-3193-hyperbola-high-asan-logs`. This does not qualify
  aggregate, shared-process, leak-detection or non-macOS coverage.
- The hyperbola very-high-origin mode subsequently passed **20/20 isolated
  groups and 7,608 assertions** under the same source-equivalent macOS Debug
  ASan/UBSan settings and bounds as the seam run. Its manifest records
  `LC_OFFSET_HYPERBOLA_VERY_HIGH_ORIGIN=1` and the high flag unset; IDs 0-19
  are complete, the binary hash was unchanged, and all 20 logs were free of
  sanitizer diagnostics. Temporary receipt and logs:
  `/private/tmp/offset-3193-hyperbola-very-high-asan.json` and
  `/private/tmp/offset-3193-hyperbola-very-high-asan-logs`. Aggregate,
  shared-process, leak-detection and non-macOS coverage remain open.
- Test-only commit `63632a8d1` adds the `high-degree-spline` selector for 45
  degree/multiplicity/format groups, retaining transform, distance, knot and
  weight variants. Source-equivalent macOS CMake Release passed **45/45
  profiles and 27,312 assertions**; the unpinned aggregate passed the same
  **27,312 assertions**. Invalid and out-of-range IDs were rejected. Temporary
  receipts: `/private/tmp/offset-3193-high-degree-release.json` and
  `/private/tmp/offset-3193-high-degree-aggregate-release.json`. Aggregate
  sanitizer, shared-process ordering and non-macOS rows remain open.
- The `high-degree-spline` selector subsequently passed **45/45 isolated
  groups and 27,312 assertions** under the same source-equivalent macOS Debug
  ASan/UBSan settings and bounds as the seam run. Requested, passed and result
  IDs match 0-44, the test-binary hash remained unchanged, and all 45 logs
  were free of sanitizer diagnostics. Temporary receipt and logs:
  `/private/tmp/offset-3193-high-degree-asan.json` and
  `/private/tmp/offset-3193-high-degree-asan-logs`. Aggregate timeout,
  shared-process behavior, leak detection and other platforms remain open.
- Test-only commit `ddad0c653` makes profile timeouts use the case runner's
  process-group cleanup and records test-binary SHA-256, change detection,
  bounds and hyperbola mode in new manifests. Three focused Python tests and
  a real reconstructed-parabola profile passed on macOS. The process-group
  test is POSIX-only; it does not qualify Windows cleanup. Existing
  receipts, including the in-flight periodic run, predate this manifest format
  and must not be attributed its additional guarantees.
- Test-only commit `1259e2499` strengthens the cleanup check with a child-started
  marker before verifying that the timed-out descendant cannot finish. All
  three focused Python tests passed again on macOS; Windows remains unqualified.
- Test-only commit `6717ab3fa` streams the binary digest in 1 MiB blocks so a
  large sanitizer executable is not loaded into memory just to hash it. Four
  focused Python tests and a real Release profile smoke passed on macOS.

## Smallest Safe Fix

Start with **no production diff**. For a failed row: pin a reproducible case,
add one failing regression, patch the existing owner, and rerun focused plus
adjacent gates. Reuse source adapters, worker preparation, undo and codecs.
No second picker, new schema, per-family UI classes or general cleanup. Touch
both build registrations only if the source list changes.

Test and commit each product/test slice; then update this plan with its SHA,
commands/results, limitations and next ready row. Record receipt source and
fixture identity, tool/platform, exact case or profile IDs and durable artifact
links. Reuse a receipt only when its affected code, tests, inputs and environment
are demonstrably equivalent. Batch pushes where practical so required CI can
finish. Close only when the named gates pass and no offset P1/P2 remains;
missing tools, policy skips and blocked rows are not passes.

### 5.4 Historical Candidate Evidence

The [pinned detailed roadmap and ledger][history] preserves the specifications,
variant matrices and receipts without a second active plan. Local access:
`git show 6a5e702ec643173a715b6f39a4d33169b1b5b90e:docs/plan_offset_curve_2961.md`.
Use §§3-5 for contracts and registered gates, §662 for the family matrix,
§§2018-2026 for Trim isolation, and §§2034-2052 for package, ODA, picker and CI
receipts. Historic scheduling and pending-state prose are superseded here.
See [validation index](offset_curve_2961_validation.md) and
[interactive record](offset_curve_interactive_smoke.md).

[history]: https://github.com/LibreCAD/LibreCAD/blob/6a5e702ec643173a715b6f39a4d33169b1b5b90e/docs/plan_offset_curve_2961.md

## Preserved H6 Refusal Inventory

Used by the developer validation runner; not a new implementation phase.

<!-- lc2961-h6-original-inventory-v1:begin -->
| IDs | Single condition(s), in suffix order | Expected boundary |
|---|---|---|
| H6-CLI-01 | Missing --run | CLI / RUN_MODE_REQUIRED |
| H6-CLI-02 | Unknown option | CLI / ARG_UNKNOWN_OPTION |
| H6-CLI-03 | Duplicate --run | CLI / ARG_DUPLICATE_OPTION |
| H6-CLI-04 | Missing option value | CLI / ARG_MISSING_VALUE |
| H6-CLI-05,06,07 | Zero, one, three receipt paths | CLI / RECEIPT_COUNT |
| H6-CLI-08 | Repeated path | CLI / RECEIPT_PATH_DUPLICATE |
| H6-CLI-09 | Distinct paths with duplicate IDs through real CLI | RECEIPT_JOIN / RECEIPT_ID_DUPLICATE |
| H6-CLI-10 | Reversed expected pair through real CLI | RECEIPT_JOIN / RECEIPT_ORDER_MISMATCH |
| H6-CLI-11 | Relative receipt path | CLI / ARG_RELATIVE_PATH |
| H6-ID-01 | Missing file | RECEIPT_IO / RECEIPT_NOT_FOUND |
| H6-ID-02a..02f | Stale revisions V506,V507,V508,V509,V510,V511 | IDENTITY / REVISION_MISMATCH |
| H6-ID-03a..03c | Synthetic stale candidate tuples V509,V510,V511 | IDENTITY / CANDIDATE_MISMATCH |
| H6-ID-04 | Stale manifest tuple | IDENTITY / MANIFEST_MISMATCH |
| H6-ID-05 | Other candidate field mismatch | IDENTITY / CANDIDATE_MISMATCH |
| H6-ID-06 | Other manifest field mismatch | IDENTITY / MANIFEST_MISMATCH |
| H6-ID-07 | Environment digest mismatch | IDENTITY / ENVIRONMENT_MISMATCH |
| H6-ID-08 | Runtime mismatch | IDENTITY / NODE_MISMATCH |
| H6-ID-09 | Runner source mismatch | IDENTITY / RUNNER_MISMATCH |
| H6-ID-10 | Canonical cwd mismatch | IDENTITY / CWD_MISMATCH |
| H6-ID-11 | Expected output-root mismatch | IDENTITY / RUN_ROOT_MISMATCH |
| H6-TREE-01a..01b | Invalid UTF-8; malformed JSON | RECEIPT_PARSE / REPORT_PARSE_FAILED |
| H6-TREE-02a..02h | Duplicate/reordered/missing/extra key; BOM; CR; double LF; other noncanonical bytes | RECEIPT_CANONICAL / REPORT_NONCANONICAL |
| H6-TREE-03a..03d | Non-PASS; nonzero exit; signal; spawn-error shape | RECEIPT_RESULT / CHILD_NOT_PASS |
| H6-TREE-04a..04c | Zero/multiple/noncanonical stdout line | RECEIPT_STREAM / STDOUT_SHAPE |
| H6-TREE-05a..05c | Persisted genuine report bytes/hash/size binding substitution, H5 owned readback throws | READBACK / MANIFEST_MISMATCH |
| H6-TREE-05d..05f | Stdout/stderr sidecar; basename/ID, H3 receipt binding | RECEIPT_BINDING / FILE_BINDING_MISMATCH |
| H6-TREE-06a..06b | Wrong mode; nonregular file | FILESYSTEM / FILE_METADATA_MISMATCH |
| H6-TREE-07a..07b | Missing/extra tree entry | FILESYSTEM / RECEIPT_TREE_MEMBERSHIP |
| H6-TREE-08a..08c | Symlink; hardlink; alias | FILESYSTEM / PATH_NOT_REGULAR_SINGLE_LINK |
| H6-TREE-09a..09c | Overlap; ancestor/descendant; protected-path overlap | PATH_POLICY / PATH_POLICY_VIOLATION |
| H6-RACE-01 | Parent changes before exclusive creation | ROOT_PREFLIGHT / PARENT_IDENTITY_CHANGED |
| H6-RACE-02a..02b | Report/sidecar changes between validations | REVALIDATION / RECEIPT_CHANGED |
| H6-RACE-03 | Readback differs | REVALIDATION / READBACK_MISMATCH |
| H6-RACE-04 | Output root appears after final check | ROOT_CAS / ROOT_ALREADY_EXISTS |
| H6-JOIN-01 | Same receipt tree/path through exported transaction | CLI / RECEIPT_PATH_DUPLICATE |
| H6-JOIN-02 | Duplicate ID through exported transaction | RECEIPT_JOIN / RECEIPT_ID_DUPLICATE |
| H6-JOIN-03 | Pair order mismatch through exported transaction | RECEIPT_JOIN / RECEIPT_ORDER_MISMATCH |
| H6-JOIN-04 | Self-consistent H3 output digest/bytes disagree with actual persisted prefix; H4 owned join throws | JOIN / OUTPUT_MISMATCH |
<!-- lc2961-h6-original-inventory-v1:end -->
