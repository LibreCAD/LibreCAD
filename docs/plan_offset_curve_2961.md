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
# Editable Offset Curves: Current Implementation Roadmap (#2961)

Updated: 2026-11-01. Project: **LibreCAD**, not LibreCAD_3.
Current status (W260; chronological evidence follows): planned product
implementation slices A-core through T3 and the local A-core through C-check
audit are present and locally qualified. This includes the shipped native
geometry worker, Modify Offset creation/repeat, native Trim preview/commit,
staged GUI drawing/template imports, and bounded worker validation for public
synchronous file/stream/byte filter imports. No additional product-code gap
has been identified in the latest audit. W218's exact product-source head
`71ecf102b36017d57bd30db39d0de630ac11bb75` passed Headless ON/OFF, CodeQL,
DWG/DXF Linux and Windows x64/ARM64, and all five-platform Pixi checks. Exact
head CI is re-run after pushed test/acceptance receipts; its live state is
tracked in the PR comment. At current PR head `e9fe078f4e31eb6917d4dfe3cf1be9e8204b6960`,
Headless ON/OFF and Text Foundations pass; CodeQL, DWG/DXF verification, and
five-platform Pixi builds are in progress (Security scan skipped by policy).
The W260 roadmap/refusal suite passes 99/99.

W253 records the approved FreeCAD first-run setup and a second, keyboard-driven
attempt to import the W171-derived DXF. The import dialog recognized about 233
entities, but FreeCAD exited before presenting a geometry viewport. The complete
offset validation Node suite passes 412/412; exact-head CI for `e246acd8` is
green. Independent contour-fidelity review therefore remains open.

W254 additionally tested the same unchanged DXF in Inkscape 1.4.2. The DXF
importer accepted it with a `$PDMODE` warning, but the drawing extent produced
only a tiny trace at a useful viewing scale. This is not contour-fidelity
evidence; the exact external visual gate remains open.

W255 visibly opened the preserved six-contour W193 acceptance DXF in the
current-source LibreCAD test copy and rendered two groups of three contours.
The W171-derived ODA DXF also opens in LibreCAD, but its one model-space SPLINE
is not a useful independent contour-comparison fixture. FreeCAD's approved
fused-shape retry still exits before a viewport. Neither attempt supplies the
independent W171 fidelity review or durable screenshot/DPR receipt.

W237 requalified the current test-only head with the full CMake application
suite: 5,155 cases, 5,080 passed, 75 skipped, and 10,348,250 assertions passed.
All nine registered CTest worker lanes passed as well.

W238 completed a fresh qmake6/Qt 6.12.0 macOS arm64 application, plugin and
helper build from this worktree. The worker-client suite passed against the
helper in the app bundle and after relocating the bundle; cross-platform and
full deployed-package acceptance remain distinct gates.

For W238 head `2c9b5bcb25e50865f301ee64c2ec41df7d9eb276`, exact-head GitHub
Headless ON and OFF both passed (run `41220922934`). W239 head
`a63a26eaf8ece575717c5c718e0ee07de96dccc7` also passed both modes (run
`41221162702`). Current W240 head CI status is tracked in the PR comment.

W241 rebuilt the exact-worktree ASan test target and reran stale-peer plus the
complete creation-worker group. The focused event test passed 38 assertions;
all 14 worker cases passed with 398 assertions (`detect_leaks=0`).

W243 requalified the exact current worktree with a full CMake Release build and
all 26 CTest entries; the application suite reports 5,083 cases, 5,057 passed,
26 skipped, and 10,347,377 assertions passed. The qmake6/Qt 6.12.0 application
build and bundled plus relocated worker-client suites also pass. The approved
LibreCAD test copy visibly renders both groups of three contours in W193. Its
binary predates the current qmake6 build, so its newly exported DXF is diagnostic
only; exact-source FreeCAD validation remains the W242 single-SPLINE result.

W244 integrates current `origin/master` `998e61dc0` through local merge commit
`c9fc81575`. Fresh CMake Release build and CTest pass 25/25, including the
merged dimension-clipboard lane. A fresh qmake6/Qt 6.12.0 app/helper rebuild
and bundled plus relocated worker-client runs pass. The bounded aggregate app
suite has two failing upstream dimension-label/property-copy cases (five
assertions); a clean build of exact `origin/master` reproduces the same cases
with the same RNG seed, so these are recorded as inherited baseline failures,
not attributed to offset code. PR conflict resolution and new-head CI are
recorded in W244-W245 below.

W242 fixed DXF output for the optional SPLINE/HELIX extrusion vector: the
value-initialized zero vector is now treated as unspecified instead of being
serialized as an invalid zero-length extrusion direction. The focused writer
regression and complete DXF fast suite pass. FreeCAD 1.1.4 imports a fresh
AC1032 file emitted by the exact-source application as one valid SPLINE edge,
without the prior zero-extrusion warning.

W220-W225 qualify selected pieces of the approved qmake6 app's GUI: normal
Offset Curve selection, compact options at one window size, locked-layer
selection refusal, and commit/Undo. W224's FreeCAD import recognized about 233
entities but produced no verified geometry viewport; FreeCAD first-start
settings remain untouched. W227's ASan publication filter passes changed-child
and freed-redo-subtree refusal cases. W233's deterministic Qt integration test
verifies the default right-release menu path; CUA's visible right-click remains
inconclusive. W234 verifies held-drag Escape cancellation through the real
QG_GraphicView event handler while the worker child is busy. W235 compares the
worker-generated preview and committed geometry through that same view route,
and confirms cache reuse and one-step history. W236 exercises stale-peer
rejection and recovery through the same view route. These native event tests
close their corresponding code-path gaps, not the remaining physical-app
evidence.

W246 confirms the approved exact-source LibreCAD test copy is responsive after
the Mac was unlocked. The saved acceptance DXF reopens and visibly renders five
left-hand and three right-hand contours. This adds no new preview/commit result;
W245's previously recorded preview, commit, Undo, Redo, and DXF round-trip
checks remain the interaction evidence. The normal-profile Offset distance was
restored to 0.5 and the app was left idle. The in-session CUA screenshot is not
a durable artifact: macOS `screencapture` captured the Codex desktop rather than
the controlled LibreCAD window, so the two misleading temporary captures were
removed. No screenshot or DPR claim is inferred from them.

Still open: physical stale-peer interaction; durable, separate visible
held-drag and preview/commit screenshots; independent FreeCAD rendering and
contour-fidelity review of the W171-derived fixture; durable screenshot/display-
DPR records; cross-platform and full deployed-package W0-W2 acceptance; review
approval; and final R integration. W226's older
qmake6 helper/client run passed 17 cases / 4581 assertions; W238 adds a fresh
current-worktree bundle and relocation check. W0-W2 and R remain acceptance
gates, not missing product features or substitutes for the native worker tests.

W216 integrates the current upstream tip `5a5fe7345fe7a7ec66892e879967673b6d7a859b`
through merge commit `c4eb33fd2038e81c55ddd79f437d29dc85f96d47`. The upstream
delta is the linetype dash-phase fix and its tests; the focused painter/render
CTest and the full qmake6 application/worker build pass on the merged source.
The PR head before this merge was
`5129ca3cb71b3446466141838ccc56b6669dbf81`, with Headless ON/OFF passing and
CodeQL, DWG/DXF, Windows, and Pixi checks still running. PR #3193 remains open,
draft, and review-required. The merge and roadmap receipt were pushed as
`2fe0a651b4eeff856a63f552629fb670abfee48f` against base
`5a5fe7345fe7a7ec66892e879967673b6d7a859b`; fresh Headless ON/OFF checks are
running and the remaining checks have not yet attached. Device Control approval
for the isolated test app was confirmed, but macOS remains locked, so the
W171-derived FreeCAD visual recheck is still open.

W213 requalifies the current source locally: the full Node suite passes 412/412,
the roadmap/refusal gate passes 99/99 (93 obligations, no pending declarations
or production mismatches), the CMake worker CTest set passes 9/9, and the
installed macOS worker-client suite passes 3,715 assertions in 17 cases. On
product-source head `440c2525058c26b4af4b4e0aece5b0d219ccd970`, Headless ON/OFF,
C++ analysis, Linux DWG/DXF, CodeQL analysis, and Ubuntu/macOS/Windows Pixi
builds pass. Windows ARM64 passes CTest 23/23, recovers across 66,594 allocation
failure points, and passes relocated worker-client tests (3,902 assertions in
17 cases). Windows x64 passes the same 23 CTests and 66,594-point recovery
sweep, with 4,140 assertions in 17 relocated worker-client cases. Its job took
5h38m55s, an unusually long but successful CI lane. At W213's GitHub status
capture, PR head `2d17b3ca060772b6393aa0def90d5d7738275602` showed
CodeQL, Text Foundations, DWG/DXF, and Pixi workflows pending. All changes from
tested product-source head `440c2525058c26b4af4b4e0aece5b0d219ccd970` through
that PR head were confined to this roadmap file. The CodeQL Security scan is
skipped by workflow policy. At that capture PR #3193 was open and draft, with
mergeability unknown and review required.

The approved LibreCAD test app was accessible for W214, which visibly confirms
the W193 drawing's six ellipse contours and idle/default state. At W214, normal
pointer clicks did not select a contour until the explicit Select Entities
control was enabled. W221 later confirms that a normal primary-pointer click
selects the native Offset Curve and exposes its grips/property row in the
approved qmake6 bundle. Right-click/Control-click on the selected entity and
right-click on blank canvas did not visibly open a menu; these probes do not
establish a product defect. Selection was cleared; no drawing edit or save
occurred. Earlier W193 interaction and W211/W163 FreeCAD rendering receipts
remain scoped results.
W171-derived external DXF/DWG rendering is still unverified: FreeCAD previously
recognized about 233 entities but produced no verified rendered view. The
screenshot was observed in-session but not retained, and display-DPR evidence
remains incomplete. The Mac relocked before the follow-up FreeCAD review.

W215 adds a broad current-source CMake Release qualification: all 26 registered
CTest entries passed, including the main application suite (5,050 passed, 26
explicitly skipped, 10,344,419 assertions passed). At the W215 PR-head snapshot,
Headless ON/OFF passed; CodeQL analysis, DWG/DXF verification, and Pixi remained
in progress. PR #3193 was open, draft, and mergeable, with review still required.

Acceptance remains open: remaining GUI refusal/interaction cases, context-menu
and entity-pick behavior, retained screenshot/display-DPR evidence, independent
CAD visual checks beyond W193/W163, signed/deployed package qualification,
broader W0-W2 platform acceptance,
reviewer approval, and final R integration. The mirrored external display
reports 1920x1080 at 75 Hz, but Qt DPR was not measured; in-session screenshots
were not retained. W203-W215 are scoped visible/test passes, not full acceptance.
Direct AC1021
ACadSharp parsing is also unqualified because that reader throws on negative
`ComprLen` before entity decoding; LibreDWG parses the current AC1021 matrix.

Earlier reader findings remain in their dated ledger below: ACadSharp's
`HasValidKnotCount` discrepancy on ODA-converted closed output is not treated as
proof of malformed DWG, and ODA conversion is not a substitute for LibreCAD's
direct DWG writer or external visual inspection.
The user explicitly confirmed that the shipped native geometry worker is required,
in addition to the Node developer validation runner. W0-W2 are mandatory.
Implementation owner: `/Users/dli/.codex/worktrees/offset-2961-runtime/LibreCAD`,
branch `codex/offset-2961-native-worker`, initially pinned to
`df0c891ad5a0bb5b0d0e864f4cd9cdd8036d5f83`. Build/evidence root:
`/Users/dli/dev/LibreCAD/tmp/offset-2961-native-20261026`.
Latest upstream integration: merge `64b57ac4e` includes freshly fetched
`origin/master` at `35e7def3d8365b3ef1bb2af54c6071ae3313691e`. W15 below is the
earlier integration checkpoint; W144 records this newer merge, its sole CMake
test-registration conflict resolution, and selected Release/ASan qualification.
The isolated qmake6 helper build is retained at
`tmp/offset-2961-qmake-native` inside this implementation worktree; it uses a
private SDK cache, not the shared checkout's stash.
Gibbs started the first bounded test correction before that interruption.
Shared source, attached draft/index and the earlier candidate branch are preserved.

## 1. Scope and Sources of Truth

Implement an editable ellipse-offset entity, its offset-specific worker path,
normal document interactions, DXF/DWG persistence, source editing and trimming.
Existing ordinary offsets of splines/parabolas/hyperbolas remain supported by
the existing engine; extending editable definitions to those sources is excluded.
Also excluded: automatic live source links, extending/filleting offsets,
LibreCAD_3, Vulkan/Metal/RHI, generic text/image/font rendering, Python bindings,
and unrelated build, CI, DWG-object or infrastructure work.

Only offset requirements may justify changes to shared dispatch, properties,
workers, filters or tests. Use the existing renderer to inspect offset geometry.
Gibbs remains the first implementation starter; that requirement was met before
the interrupted temporary run. Further agents must stay within concrete offset
subtasks and disjoint write scopes. No unrelated background work is authorized.

| Source | Observed identity | Meaning |
|---|---|---|
| Main checkout | `/Users/dli/dev/LibreCAD`, master `41678d97626c05ab652ceaa02cf0ac7f7f6205cf` | Dirty shared checkout; preserve all existing changes and its index. |
| Attached feature worktree | `/Users/dli/.codex/worktrees/offset-2961-v205-recovery/LibreCAD`, `codex/offset-curve-2961-recovery-v205`, HEAD `9309885cef66aef0cca0f3c8862e0c210518da3b` | Contains the extra geometry/checker groundwork and this plan; plan already had staged and unstaged changes. |
| Relationship | Merge base of those commits is `41678d97626c05ab652ceaa02cf0ac7f7f6205cf` | The feature groundwork is not present in the main checkout. Do not substitute one tree's tests for the other's. |
| Later implementation candidate | Local branch `codex/offset-2961-implementation`, HEAD `df0c891ad5a0bb5b0d0e864f4cd9cdd8036d5f83` | Descends from the attached feature HEAD. Contains the editable entity, public checker, file/UI/source/trim implementation and validation tooling. Not integrated into either checkout above. |
| Candidate product checkpoints | `b6814fb3a63dd66bb3aeae9e5b5e66668f8b8591` (complete cut search), `f617fcf70b46a0236a8725140aeb10d475c7d9ec` (final edit snapshots) | Substantive later code, not merely a plan claiming completion. Audit and reuse it before proposing replacement implementation. |
| Historical candidate inspection snapshot | `/private/tmp/lc2961-plan-refresh.NI3tkd/candidate`, detached at `df0c891ad5a0bb5b0d0e864f4cd9cdd8036d5f83` | Former read-only inspection aid; now absent. Use the surviving pinned local Git objects, not this disposable path. |
| Historical draft archive | `attic/plan_offset_curve_2961_history_20261026.md` | Earlier audit recorded an exact pre-consolidation draft, including uncommitted work. It is now absent from all three recorded worktrees and is not tracked; do not claim present preservation. |

**Selected implementation baseline:** the isolated
`/Users/dli/.codex/worktrees/offset-2961-runtime/LibreCAD` worktree on
`codex/offset-2961-native-worker`, descending from the later candidate. Current
slice SHAs/evidence and remaining S0 scope corrections are recorded below.
Do not overwrite the dirty main tree,
reset the attached worktree, rewrite the existing candidate branch, or recreate
all implemented modules from the older feature snapshot. Carry this current plan
into the isolated implementation checkout without importing another person's index.
The candidate's committed roadmap and its `docs/offset_curve_2961_validation.md`
are historical evidence at the pinned SHA, not competing scheduling instructions.

The earlier audit recorded the draft as 5,762,095 bytes, SHA-256
`2dc8b28eccb4092af8f2ffba05342abfa69aae93d3963b6b7d37e67b62d73ee0`.
The bytes are not currently available for rechecking that hash or reconstructing
uncommitted work. Its recorded section numbers, V455 proposals and receipt claims
are historical provenance, not active routing or
new PASS results. This document supersedes their scheduling instructions, not
the underlying geometric correctness requirements. Reuse detailed design material
only after checking it against the current contracts below and the selected code.
Do not create another chain of plan-only packet/reviewer revisions instead of
closing a concrete product prerequisite. No historical acceptance is transferred.

## 2. Verified Inventory and Review Findings

This inventory compares the older attached tree with the later candidate.
`candidate present` never means integrated, freshly tested, or fully correct.

| Area | Candidate observation | State / consequence |
|---|---|---|
| Ordinary offset engine | `lc_curveoffset`, `lc_offsetoutputbudget`, `RS_Modification::offsetWithOutcome` | Existing Direct/Trimmed/RegionBoundary behavior; reuse and preserve noneditable source types. |
| Reference, budgets and global checker | `lc_ellipseoffsetgeometry`, strict intersection code, `lc_offsetpathsearch`, public `checkCache` | Candidate extends local groundwork with terminals, complete cut search, memoization and chain proof. The older feature tree lacks this completion. Requalify rather than reimplement. |
| Editable entity and editing | `lc_offsetcurve`, `lc_offsetcurvegrips`, `lc_offsetcurvetransform`, full spline fingerprint | Candidate present: guarded adoption/create, edits, clones, cached snaps/grips and transform/refusal handling. Review call sites and retained proof validity. |
| Definition persistence | `lc_offsetcurvexdata`, `lc_offsetcurvecarrier`, `RS_FilterDXFRW` | Candidate present: schema, raw-carrier admission and actual SPLINE DXF/DWG paths. Same-implementation round trips alone remain insufficient. |
| Type/UI/source actions | Entity enum/coverage, property queue/dialog, default drag, Update/Restore actions | Candidate present. Property queue defers publication with a timer, but prepares geometry synchronously in `apply`; a deferred commit is not background computation. |
| Stored-cache Trim | `lc_offsetcachetrim`, contacts/matching helpers, Modify Trim action and file tests | Candidate present: pure preparation, exact fragments, two-operand publication and plain-fragment persistence. Recheck precision/ownership/undo guarantees. |
| Validation workers | `scripts/offset-curve-validation/`, X-H1 through X-H7 | Candidate has Node developer/test workers and recorded timeout fixes. These tools are not a shipped isolated geometry worker. Do not confuse their deadlines with GUI computation limits. |
| Native geometry-worker path | Real seven-operation protocol4 helper/client, cached source-copy production, same-producer ordinary fallback, plus dialog, sheet, grip, Update, Modify Offset, native Trim and GUI drawing-open/template routing committed/qualified locally | W16-W19 qualify cache portability and creation/repeat; W24-W26 qualify local Trim routing and scoped action faults; W31/W32 cover GUI drawing open/templates. W72 adds staged DXF/DWG plugin block import; W91 routes the legacy synchronous entry through that staged session (it still waits for compatibility) and preserves the async callback entry. W93 classifies hatch-pattern input as display-cache-only. W142 inventories every current in-repo import entry point and records that the remaining synchronous `RS_FileIO`/`LC_DocumentsStorage` APIs are compatibility-level calls with no current recognized DXF/DWG product caller; W143 reruns GUI open/template staging and ownership tests in Release/ASan. External API callers, cross-platform and packaged/final acceptance remain mandatory W0-W2 work. |
| Tests/build targets | Main Catch target, filter micro targets and two standalone offset allocation-failure targets | Candidate present; source registration is not discovery/execution. Use the actual targets in section5.3, not just the old grouped fast lanes. |
| Surviving test defect | `spline_offset_representation_tests.cpp` lines205 and668 in candidate still use DWG enum ranges | `FormatDWG2007` is outside the range; those ordinary representation loops also omit it. Fix classification and add an executed R2007 regression in S0. The separate candidate matrix already lists all six DWG versions. |

### 2.1 Findings to Close, Not Repeat

| Priority | Finding | Bounded disposition |
|---|---|---|
| P1 | A plan based only on the older tree labels implemented candidate features missing | S0 audits ancestry, scope and source differences; select the candidate without wholesale changes to the main checkout. Preserve/review A-C, E/U/G/F/O/T code rather than rebuilding it. |
| P1 | Test-runner success could be mistaken for product worker isolation/responsiveness | W0 pins which runtime is being qualified; W1/W2 implement the native path promised here. If only the validation runner was intended, record an explicit scope correction before removing the native requirement; do not silently count X-H7 as it. |
| P2 | Ordinary representation tests misclassify DWG2007 and never execute that row | S0 adds explicit enum mapping, both affected round-trip paths and a physical `AC1021`/reader-routing assertion, plus non-DWG negatives. No unrelated codec rewrite. |
| P2 | Existing entity/filter/failure tests would be missed by a geometry-only or grouped-fast-lane run | S0 inventories actual declarations/targets; each slice runs its named cases, relevant filter micro targets and standalone fault processes. Zero matches or omitted targets cannot close a slice. |
| P2 | Historical evidence and temporary artifacts could be promoted into current or durable acceptance | Earlier audit reported matching14 receipt/52 fixture hashes. The old14 receipt files are now absent; all52 committed fixtures still match. Use new scoped durable runtime receipts for current claims, never reconstruct old bytes or PASS results from hashes. |
| P2 | Prior visible UI results explicitly omit physical variants | R carries the unobserved rows from the candidate interactive record: context menus, compact options, held-drag cancellation, locked/stale peers and preview/commit comparisons. Automated tests do not turn those rows into physical PASS. |
| P2 | Candidate evidence links would become stale when this consolidated roadmap replaces its old plan | S0 retargets the two evidence documents without changing historical claims. R02 must point to `lc_ellipseoffsetgeometry.cpp`/`checkCache`, not just `lc_offsetresultcheck.cpp`, which checks legacy line/arc polylines. Validate files and heading anchors after consolidation. |

The candidate's R01-R15 index and its 14 retained receipt hashes were read and
checked, not independently re-executed. Its final C-search record reports 4786
full-Release cases and 1202 affected-ASan cases with explicit skips and host limits;
these are historical observations, not expected future counts. All 52 fixture
files (24 ACadSharp, 24 ODA, 4 hand-authored) match their committed manifests.
No blanket claim that external fixtures are unavailable remains justified.

Relevant existing tests: `lc_offsetcurve_geometry_tests.cpp`,
`lc_parametriccurveintersection_tests.cpp`, `lc_offsetoutputbudget_tests.cpp`,
`lc_entitytype_coverage_tests.cpp`, `rs_modification_offset_tests.cpp`,
`lc_action_modify_offset_tests.cpp`, `lc_action_modify_trim_tests.cpp`, and
`spline_offset_representation_tests.cpp`. Locate paths from the selected tree;
line numbers are navigation hints, never frozen test identities.

## 3. Non-Negotiable Contracts

### 3.1 Geometry, Cache Validation and Resource Accounting

1. Phase-1 editable sources are finite planar top-level `RS_Ellipse` objects,
   whole or arc. Distance is finite and positive. Normalize raw axes/sweeps before
   an `RS_Ellipse` constructor can erase tiny/full-turn distinctions. Preserve
   wrapped/reversed traversal, circle degeneracies and large-centre local detail.
2. A definition holds normalized basis, distance, toward-centre, cache reversal
   and saved relative tolerance. Original-whole, cache representation flags and
   geometric chain closure are separate facts. Do not infer one from another.
3. Admit only exact normalized degree-1 or degree-3 ClampedOpen cache layouts
   with complete unit weights (or genuine implied defaults). Preserve actual
   clamped knots/span multiplicities and controls; no repair, refit, coordinate
   projection, knot synthesis or rational normalization may establish editability.
   An exact `RS_Line` producer result may be converted to degree-1 cache data.
4. The public checker must prove finite valid layout, bounded proximity in both
   directions, correct visible reference coverage exactly once, orientation,
   occurrence/node incidence, source ends, cuts and seam/closure. A near-reference
   loop, repeated traversal, missing interval or wrong branch must fail. Local
   derivative/proximity success and dense sampling alone are insufficient.
   `distanceToOriginal` measures the source ellipse, not the offset set; it
   cannot certify the visible offset branch or replace the cache/reference metric.
5. Keep reference geometry independent of candidate cache subdivision. Distinct
   coordinate-near occurrences remain distinct until same-root identity is proved.
   Retain full root enclosures/fringes, one-sided ownership and endpoint provenance.
   Ambiguous order, tangency, visibility or proof conflict is a typed refusal.
6. Audit every data-dependent record, traversal, sort, proof pair and retry.
   Use checked arithmetic and irreversible shared counters. Multi-counter
   admission is atomic; failed entered work retains its genuine consumed prefix.
   Never double-debit snapshots, reset a latch with a fresh budget, or publish
   partial successful geometry after allocation, depth, cancellation or limit failure.
   Published/retained work snapshots are value copies. An immediate borrowed
   `WorkBudget::consumed()` view is not itself a defect, but must not escape a
   query lifetime or stand in for a frozen result; audit actual ownership before
   changing its API. Preserve the candidate's separate ordering allowance.
7. Preserve existing engine tolerance policy. Sampling may corroborate an interval
   proof, not replace it. Test exact limits and one less with independently
   computed work snapshots; no arbitrary larger limits to make cases pass.

### 3.2 Entity and Document Behavior

`LC_OffsetCurve : RS_Spline` owns its definition and validated ordinary cache.
Private construction follows full checker success; public raw data cannot create
an editable invalid entity. A complete cache fingerprint detects later mutation,
but is not geometric validation. Include controls, knots, weights, degree,
representation, fit data and closed/periodic flags, not only the existing length hash.

Creation, distance/side/through/source edits compute and validate temporary data
before replacing anything. Explicit edit failure preserves geometry, attributes,
selection, extensions and undo state. A no-op creates no undo record. Preserve
layer/pen/visibility, container ownership and supported foreign metadata.
Several/empty/unsupported ordinary producer results stay ordinary and are reported;
they never receive invented editable definitions.

Clone, draw, snapping, undo/redo and loading an already validated saved cache do
not rerun the offset generator. Move/rotate/uniform-scale/mirror/reverse transform
the basis and cache coherently. Document nonuniform scale/shear explicitly
substitutes plain spline geometry and removes stale owned metadata. A low-level
in-place unsupported transform must leave a safely noneditable object, not a false
definition. Generated children of INSERTs/dimensions are not replacement targets.

Grips default provisionally to distance grips at eligible axis vertices plus an
arc mid-sweep grip; centre is a snap. Keep this policy isolated for the unresolved
D0 discussion; do not record user approval. The new UI option defaults on only in
option loading for ellipse sources; constructor/data defaults stay off for existing
programmatic callers. Existing ordinary offset behavior remains unchanged.

### 3.3 DXF/DWG Schema and Safe Fallback

Use an ordinary SPLINE carrier, registered APPID `LibreCad`, and one typed sub-list:

```text
1001 LibreCad
1000 LC_OFFSET_CURVE
1002 {
1070 1
1000 ELLIPSE
1011 centre.x centre.y 0
1012 majorDisplacement.x majorDisplacement.y 0
1040 ratio
1040 angle1
1040 angle2
1070 flags
1041 distance
1040 relativeTolerance
[safe unknown tail]
1002 }
```

The ten fixed items have exact code/type/order semantics. Code 1012 is a vector,
not a second world point. Flags: bit0 original reversed, bit1 toward centre,
bit2 cache reversed, bit3 left-of-source, which must equal bit1 XOR bit0.
Preserve reserved bits4-14 on no-edit round trips; flags must be in `[0,32768)`.
The saved tolerance factor must be within 0.1-10 times the current build default.
Successful meaningful geometry edits clear safe unknown tail/reserved bits;
clones, refusals and no-ops preserve them. Regeneration stores its actual factor.

- One shared schema implementation serves DXF XDATA and DWG EED; no DWG custom
  class, new file container, or separate interpretation per format.
- APPID matching is case-insensitive without trimming. Preserve foreign groups,
  group spelling/order and unrelated `LibreCad` items; do not mutate shared variants.
- Parse with a bounded linear brace-aware scan. Duplicate known definitions,
  malformed discriminator, contradictory flags or unsafe raw geometry cannot adopt.
  Known-invalid ranges are removed only within their own application group.
  Unknown schema versions remain raw, even when nested tokens resemble schema1.
  Invalid known data takes precedence over a later apparently valid definition.
  Freeze the overall scan/item/byte allowance in F1, including malformed and
  foreign groups. If safe range ownership cannot be determined within it, refuse
  adoption/rewrite with a diagnostic; never strip an unexamined prefix or promise
  successful export of an unsafe partially scanned payload.
- Safe owned tail: at most64 items,4096 target-encoded bytes,depth8; only balanced
  typed 1000/1002,1010-1013,1040-1042,1070/1071 values. Do not retain owned binary
  or reference-valued extensions. Discard an unsafe unknown tail as a whole with
  a diagnostic, not a truncated brace list. Preserve unrelated foreign payloads.
- Before import repair, check raw `DRW_Spline` declared/actual counts, nonnull
  finite planar controls, degree/layout/knots, complete unit/default weights,
  normal semantics and absence of fit-only/repaired representation. Only actual
  SPLINE may adopt; HELIX and ordinary spline fallbacks must not gain editability.
  Accept only verified world-XY/unspecified normals; otherwise keep ordinary import.
- Adoption uses the full checker and preserves imported attributes. Failure keeps
  usable visible ordinary spline geometry and gives a bounded diagnostic.
- Serialize before choosing the format branch. Strip stale known markers when
  plain, inconsistent, trimmed or R12 geometry-only. Unsafe merge, invalid current
  definition, EED-size overflow or codec rejection fails the complete save through
  existing transaction/finalization mechanisms. Preserve an old target and clean
  staging files; logging a dropped offset is not a successful export.

### 3.4 Trim Contract

Trim uses the exact stored cache, not regenerated analytic offset geometry.
Resolve a selected display chord to its owning curve and a unique parameter
occurrence. Keep distinct roots with parameter provenance, including roots at
span joins; do not merge them by XY distance alone.

For an open curve, remove the unique clicked interval between cut boundaries
and retain its positive-length complements (one or two ordinary splines).
For a closed curve, require at least two usable distinct roots and retain the
cyclic complement as an open spline. Ambiguous click/root, unsupported cutter,
tangency/overlap without a certified decision, no intersection, degenerate result,
locked/hidden/non-document target or exhausted budget refuses atomically.

Clip degree-1/degree-3 spans with the existing native subdivision/knot operations;
do not refit or extend. Current editable caches have unit weights: this phase
does not quietly expand adoption to rational curves. Independently check cut
endpoints against cutter roots and preserved endpoints against original cache
endpoints; both obligations apply if they coincide. Retained parameter intervals
cover the expected complement exactly once, in directed order.

Trim converts affected targets to ordinary spline fragments and removes only
owned offset metadata. An offset used only as a limiting curve stays editable.
Trim Two prepares both sides before one undoable document batch; refusal changes
neither side. Preview and commit share preparation, with a clear loss-of-editability
notice. Undo restores the original editable entity; redo restores the same fragments.

## 4. Ordered Implementation Slices

This is the requirement/dependency map, not a claim that every row still needs
new code. `candidate` means source found on the pinned later branch, with
historical qualification only. All rows remain unaccepted for a newly selected
implementation source until their affected gates pass. S0 is the first next
action; reuse candidate code and patch only demonstrated gaps. W0-W2 cannot be
closed using the Node validation runner. Acceptance tags below exist on the
candidate except the explicitly proposed native-worker tags.

| ID | Owns / concrete deliverable | Depends on | Current state | Acceptance selector or artifact |
|---|---|---|---|---|
| S0 | Pin/reconcile candidate, main and feature; baseline/discovery; fix DWG enum classification and add R2007 execution | - | W153 closes the durable source/target reconciliation; parser/R2007, baseline/discovery, path/link and target inventories are recorded; later geometry and platform gates remain separate | `[curve-offset]`, `[lc-offsetcurve]`, `[entity-coverage]`, actual target/version inventory |
| A-core | Atomic shared admissions, frozen results, sticky status and ownership/lifetime | S0 | candidate `0eb2a0cfa`; `27f9c5239` fixes demonstrated compound admissions; `91dcde3c4` preserves ordered contact-control debit semantics; W107 requalifies budget-core primitives; W155 closes the local cross-stage counter-transfer/ownership audit; remote/final acceptance remains | `[lc-offsetcurve][budget-core]` |
| A-strict | Strict-query callback/record/order/depth/exception accounting | A-core | candidate `dc016f6e2`; W64 focused strict-budget Release/ASan requalification; W87 fixes depth-ceiling-based queue over-allocation; W111 current-source requalification; W155 verifies strict-kernel deltas enter the shared query ledger once; remote/final acceptance remains | `lc_parametriccurveintersection_tests.cpp`, named strict-budget cases |
| A-reference | Critical/provider/event accounting and atomic paired graph publication | A-strict | candidate `5c12fbeef`; W65/W112 focused Release/ASan requalification; W118/W119 sweep every allocation through six-pair/four-critical event and component publication paths with bounded ledgers and same-budget recovery; W120 runs all standalone default fault fixtures together in Release; W121 closes the local provider/contact/event/component/terminal/accounting matrix in Release/ASan; W155 verifies current-source budget/lifetime composition; remote platform execution remains | `[lc-offsetcurve][reference-accounting]`, `[reference-terminals]`, `[reference-components]`, `[reference-events]`, isolated reference fault selectors |
| A-cache | Bounds/local cache, inherited depth and failed-stage snapshots | A-reference | candidate `dbf895638`; W66/W113 budget requalification; W114 exhaustively injects allocations in `checkCache`, standalone bounds and local-topology APIs with bounded ledgers and same-budget recovery; local A-cache audit complete; W155 confirms the current-source shared-budget chain; cross-platform/final R gates remain | `[lc-offsetcurve][cache-budget-accounting]`, standalone cache-check fault target |
| B | Immutable occurrence/edge-side terminals and issuer lifetime | A-cache | candidate `9bf0546b6`; W97 current-source review and focused Release/ASan requalification found no gap; W155 confirms unchanged audited source and current full-geometry selector; final R acceptance remains | geometry terminal/occurrence case inventory |
| C-search | Exhaustive certified cuts, owned independent laps, bounded equivalent-state memoization | B | candidate `b6814fb3a`; W98 current-source review and focused Release/ASan requalification found no gap; W155 confirms unchanged audited source and current full-geometry selector; final R acceptance remains | `lc_offsetpathsearch_tests.cpp`, public cut-search cases |
| C-check | Full public checker, directed coverage, incidence, seam/closure and typed refusal | C-search | candidate `d4a0b081a`; W100 charges the final proof-publication slot; W103/W105/W108/W109/W115/W116 sweep every observed allocation failure over line, forward/reversed two-span cubic, forward/reversed whole-circle seam, both open and closed components of a producer-generated two-component trimmed path, and hidden-cusp endpoint refusal with bounded ledger and same-budget retry; W120 runs all standalone fault fixtures in one Release pass; W122 and W155 close the local topology/test inventory; remote platform/final gates remain | `[lc-offsetcurve][geometry][cache-topology-chain]`, standalone cache-check allocation-fault target |
| W0 | Separate validation runner from product runtime; freeze native-worker schema, caps, deadlines and ownership | C-check | wire/request/reply and initial deadline/authority policy implemented; current in-repo caller/fallback/import ownership inventory recorded; external public low-level API use and platform/deployment gates remain | operation/cap/deadline/call-site table |
| W1 | Bounded headless geometry helper and parent transport, registered/packaged in both builds | W0 | helper/client and both build registrations qualified; W154 adds component-only installed-helper execution to Linux/Windows CI; local macOS staged/relocated client passes; W213 Windows x64 and ARM64 installed-helper checks pass | `[lc-offsetcurve][worker-protocol]`, `[worker-kernel]`, `[worker-process]`, `[worker-client]` |
| W2 | Deadline/cancel/crash/reap, validated result, UI caller adoption and timeout regression | W1 | fault/edit, dialog/sheet/grip/Update, creation/repeat, native Trim preview/commit and scoped action faults and geometry-entry timeout receipts qualified locally; W81 adds Linux/Windows CMake worker CI; W82 removes PR path filtering and W83 adds scoped feature-branch push triggering; W213 Windows x64 and ARM64 pass; import, GUI, deployment and final gates remain open | `[lc-offsetcurve][worker-client]`, `[worker-fault]`, `[worker-geometry-timeout]`, `[edit][preparation]`, `[creation-worker]`; remaining integration and runner regression |
| E1 | Guarded entity/factories, RTTI, stamp, clone, downgrade and transforms | C-check | candidate `92758859c` and later fixes; W128 current-source review and focused Release/ASan requalification found no gap; remote/final R acceptance remains | `[lc-offsetcurve][native]`, `[entity-coverage]` |
| U1 | Type/INSERT/selection/SVG/menu/info dispatch and safe properties | E1 | candidate `49c9b328b`, `f0fb28fab`; W129 current-source dispatch review and focused Release/ASan coverage found no gap; separately gated legacy DXF test remains unavailable in this configuration; remote/final R acceptance remains | native type/name/dispatch TUs, `[entity-coverage]` |
| E2 | Cached grips/snaps, through proposal and transactional mutators | E1 | candidate `4300c1031`, `47e14c74b`; W131 current-source review and focused Release/ASan requalification found no gap; remote/final R acceptance remains | native edit/grip/transform TUs |
| G | Eligible creation, repeated offset and bounded ordinary fallback/reporting | E2,U1,W2 | shared preparation `ef40eecad`, typed Produce `558c3bba2`, W16/W17 cache and W18/W19 GUI/publication qualified through `f035d983e`; current-source W67a/W67b and W117 Release+ASan requalification; final platform/visible acceptance remains | creation/repeat/modification TUs, `[creation-worker]`, `[creation-publication]`, `[creation-sources]` and existing Offset tests |
| F1 | Shared typed schema, raw-carrier eligibility, ownership and byte limits | E1 | candidate `305042903`; W132 current-source review and Release/ASan hostile/adaptor coverage found no gap; remote/final R acceptance remains | `lc_offsetcurve_adapter_tests.cpp` (`[offset-adapter]`), `lc_offsetcurve_hostilefile_tests.cpp` (`[lc-offsetcurve][g-hostile-file]`), `lc_offsetcurve_eed_packet_tests.cpp` (`[offset-xdata]`), plus entity owned-tail cases |
| F2 | Actual DXF/DWG SPLINE writer/adoption and transactional errors | F1,G | candidate `32597762b`, `7e46b2de8`; W133 current-source writer/refusal matrix passed Release/ASan; read/adoption hostile evidence is W132; remote/final R acceptance remains | `lc_offsetcurve_adapter_tests.cpp` (`[offset-adapter][export]`, `[offset-adapter][dwg-export]`), `lc_offsetcurve_matrix_tests.cpp` (`[lc-offsetcurve][g-matrix][write]`), persistence save-refusal cases |
| F3 | Version/read/write/conversion, clipboard, independent fixtures and output inspection | F2 | candidate `79ae1930b`; W134 current full matrix passed Release/ASan; external CAD application validation and remote/final R acceptance remain | `[lc-offsetcurve][g-matrix]`, explicit per-row evidence |
| U2 | Single/multi properties, busy state, async preparation and stale-result protection | E2,U1,W2 | dialog `705a0135b` and sheet `ac7c7646f`; W135 current-source dialog/sheet/async/batch-failure matrix passed Release/ASan; visible/platform/final R gates remain | `lc_offsetcurve_propertiesdialog_tests.cpp`, `lc_offsetcurve_properties_tests.cpp`, `lc_offsetcurve_batch_failure_tests.cpp`, UI-helper tests |
| U3 | Grip drag, cancel, ownership, undo/redo and no late publication | E2,U2 | worker continuation `0e80b8540`; W136 current-source `[lc-offsetcurve][h-drag]` passed Release/ASan; visible/platform/final R gates remain | `[lc-offsetcurve][h-drag]` |
| U4 | Ellipse-only persisted option, preview/commit parity and messages | G,F3,U2,U3 | candidate `3d7195631`; W139 current-source option lifecycle/preview/commit selector passed Release/ASan; physical visible workflows remain | option-enable TU and visible workflows |
| O1 | Transactional Update from legal live ellipse; no-op/refusal counts | U4 | worker batch `075038088`; W140 current-source `[lc-offsetcurve][i-update]` passed Release/ASan; visible/platform/final R gates remain | `lc_action_offsetcurve_source_tests.cpp` |
| O2 | Restore Original, same-container dedup, attributes and all entry points | O1 | candidate `0d3fed653`, `4a231d243` plus final fixes; W141 action, UI dispatch, and R2018 DXF/DWG persistence matrix passed Release/ASan; visible/platform/final R gates remain | Restore/source-UI TUs |
| T1 | Pure bounded trim prep and exact stored-cache fragments | C-check,E1,F2 | candidate `04c712069`; W104 requalifies standalone trim allocation/cancellation failures locally and adds both trim/cache fault executables to Linux/Windows CI; broader audit remains | `[lc-offsetcurve][trim-kernel]`, standalone trim fault target |
| T2 | Trim/Trim Two, owner mapping, common prep and atomic undo | T1,U1,W2 | candidate `49bb7755f` plus final fixes; prepared worker consumption `3e4401990` qualified; W123 requalifies action-level asynchronous preview/commit routing, stale-input refusal, cancellation, timeout, process reaping and recovery in Release/ASan; final visible/platform acceptance remains | `[lc-offsetcurve][trim-action]` |
| T3 | Plain-fragment metadata and trim/downgrade save/reopen/conversion | T2,F3,O2 | candidate `905158174`; W124 requalifies the full local trim-file matrix in Release/ASan across DXF R12-R2018, DWG targets, binary DXF and conversion chains, second saves/reopens, and undo/redo; external-platform/final R acceptance remains | `[lc-offsetcurve][trim-files]` |
| R | Exact-source cross-feature, Release/ASan/qmake6/visible checks and delivery closeout | T3 | open; older scoped COMPLETE is not this plan's closure | all required rows, scoped receipts and exclusions |

### 4.0 First Runnable Slice: S0

1. Record main/feature/candidate SHAs, merge bases, dirty diffs and index hashes.
   The attached index is initially SHA256
   `7b342f9461c1b776745b945f81117bc6b2bb755dfa6e830557280721b1de2144`.
   Use a clean isolated candidate checkout on a new noncolliding `codex/` branch;
   never reset/rebase somebody else's active branch or stage their plan changes.
2. Classify the 287 candidate-changed paths against the offset requirement they
   serve, including shared codec and Save As changes. Reuse necessary support;
   exclude only demonstrated unrelated deltas through reviewed follow-up commits,
   not a bulk revert. Preserve old receipts/fixtures and the existing candidate.
3. Inventory actual CMake/qmake product sources, Catch declarations/tags, filter
   micro targets and standalone fault modules. Pin Qt6/compiler/Catch2 provider;
   use the exact local Catch2 source when available, with no dependency upgrades.
   Build/run the candidate baseline in a fresh directory before assigning PASS.
4. In its own test slice, replace both numeric DWG range tests with an explicit
   format switch/table and add DWG2007 to both ordinary representation paths.
   Add a regression that fails on the old classification: all six DWG targets,
   especially `FormatDWG2007`, route to the DWG reader; ASCII DXF never does.
   Verify the physical R2007 signature is `AC1021`, import succeeds, and controls,
   knots, weights, direction, layer/pen and bounded geometry are retained.
   Scope is this offset test TU unless a separate production defect is proved.
5. Discover then execute the new case plus ordinary representation/persistence,
   full native offset and entity-coverage selections. Run affected ASan and
   format lanes. Keep named baseline failures/skips; no count-based waiver.
   Commit the test slice, then a plan-only checkpoint with actual evidence.
6. When adopting this plan in the candidate, update its existing validation index
   and interactive record links to current sections or immutable historical Git
   checkpoints. Preserve receipt hashes and historical source scopes. Correct
   R02's public-checker source mapping and check every relative file/heading link.
   Keep one active roadmap; Git history and any actually available archive are history,
   not additional live plans. Do not edit unrelated planning documents.

Next, audit/requalify the existing A-C implementation in dependency order and
open only demonstrated correctness gaps. W0 is the next missing product-design
slice. Existing independent entity/file regression discovery may proceed while
that bounded probe runs; dependency gates govern final acceptance, not whether
already-written code is allowed to be inspected or tested.

### 4.1 Geometry Slice Cards: A-core through C-check

Primary owners: `librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.{h,cpp}`,
`lc_parametriccurveintersection.{h,cpp}`, relevant `lc_curveoffset` adapters, and
their existing tests. Engine code must not acquire UI/modification dependencies.

For each A/B/C review, first enumerate public entries, direct counter debits,
callback traversals, retained snapshots, nested searches and result publication.
Map each site to a unit/cap, admission point, owner, lifetime and exact positive/
one-less/failure assertion. Consolidate a site map once per affected code change;
do not add a generic audit framework or repeat plan-only reviewer loops.

Required independent cases: direct/cache-reversed orientation; wrapped and whole
seams; circles/near-circles; inward collapse/lens corners; cusps/stationary points;
close-but-distinct roots; transverse crossing and uncertain tangency; missing,
repeated, reordered or self-looping cache intervals; exact endpoint occurrences;
pre-spent/shared budgets; fresh/expired/replaced issuers; retries with inherited
depth; allocation failure and every entered failure path. Refused results contain
no usable partial proof/geometry. Global success requires analytic/interval
reference ownership and exact-once directed coverage, not candidate-derived samples.

### 4.2 Offset Worker Cards: W0 through W2

Scope is offset computation/validation only. Pure geometry tests may run in
process; expensive UI jobs must not mutate the document from a worker or keep
the GUI blocked in an unbounded wait. No shared generic renderer/agent runner.

Keep two tracks distinct. The candidate Node X-H1-X-H7 tooling is optional
developer validation infrastructure and has its own source/receipt/runtime
history. It does not run inside the application. This draft's native W0-W2
requirement is explicitly confirmed by the user; the candidate's 30ms property commit timer and busy
cursor do not satisfy it. Do not reimplement the completed validation pipeline
as a substitute for native work, or interpret a 300-second runner timeout as a
permitted UI pause. Do not remove native isolation or narrow this requirement
back to the developer runner.

W0 freezes the protocol before W1: typed versioned QtCore CBOR messages, explicit
finite doubles and integer counters, length-prefixed frames, allowed operation
set, one active job per editing context, request/source revision token and owned
definition/cache/result data. No QObject/entity pointers, filesystem instructions
or arbitrary commands cross the boundary. Validate sizes/types/depth/duplicates
before constructing large containers; reject unknown/malformed mandatory fields.

The preliminary `lc_offsetworkerwire` layer implements a 4-byte big-endian length
and exactly one CBOR payload per helper lifetime. Structural limits are 16MiB
per frame,262144 total values,65536 entries per array and depth32, counting the
root as depth1. Only fixed arrays, finite float64/int64, booleans and null are
allowed. Maps, strings/bytes, tags, indefinite containers, float16/32 and trailing
values refuse before decoding a value tree. This transport layer is not a
certificate, typed operation codec, helper process or completed W0/W1/W2.
Its limits may only be lowered. Preliminary implementation is independent of
geometry requalification; it does not waive W0's C-check predecessor or close it.

Also retain 64KiB bounded diagnostic capture and one queued replacement per
context. Derive smaller operation-specific per-array caps from existing geometry
budgets, not the frame size. W0 must freeze actual preview/commit deadlines,
clock source, cancellation and kill/reap bounds before enabling GUI routing;
the implemented policy below is not permission to wait indefinitely.

The current `LC_OffsetWorkerClient` policy is fixed: startup2000ms,
preview2000ms, commit30000ms, graceful termination100ms and an additional1000ms
kill/reap allowance. `QElapsedTimer` supplies monotonic elapsed time, starting
immediately before process setup; the total preview/commit deadline includes
startup, pipe I/O and reply admission. Expiration is inclusive. A5ms precise Qt
timer observes deadlines, including when a clean exit is observed too late;
admission checks the deadline again before issuing a completion. This is not an
OS scheduling guarantee when the parent event loop itself is unavailable.
SIGTERM is deferred until Qt confirms exec via `started()`; the kill deadline
still applies to Starting. A failed reap poisons this client, retains only a
disconnected process notifier until exit, reports `ReapFailed`, and issues no
completion or subsequent launch. Destruction uses a bounded kill/reap wait and
the same quarantine rule, not QProcess's unbounded destructor wait.

One active request and one replaceable queued request are owned per client.
Request IDs increase; stale/cancelled deferred deliveries cannot publish even
after a successful child exit. Only the fixed sibling `LibreCADOffsetWorker`
(`.exe` on Windows) and `--protocol=4` are launched. The helper has private Ini
settings and no document or window. CMake installs it beside the executable,
including `Contents/MacOS` on macOS; qmake builds it before the application.
The existing macOS packager already enumerates extra Mach-O executables.
This does not yet qualify a deployed/signed bundle on another machine.

W0's bounded deliverable contains an explicit operation/caller table for creation,
validation/adoption, distance/side/basis/through edits, grip preview/commit,
Update from Source and trim preparation. Audit file-load validation separately:
no asynchronous return may outlive an import callback or publish half a document.
Keep cheap cached draw/snap/clone operations in process. Use the existing pure
helpers and value records; move no QObject, live entity, codec callback or proof
issuer across a process boundary. Specify whether parent validation repeats a
bounded checker or validates an owned serializable result. Charge child work,
parent validation and retries explicitly; a received counter is not trusted proof,
and recreating a fresh allowance must not defeat the operation's limits.

Implementation order inside W0-W2:
1. Qualify bounded wire syntax/partial delivery independently (the new pure
   `librecad_offset_worker_wire_tests` target). Record negative controls and
   Release/ASan/qmake6 results. No helper or UI-isolation claim at this step.
2. Freeze typed Generate, ValidateCache, TrimCache, TrimConic and CheckThrough requests,
   complete cache/basis value layouts and status/work snapshots. Every fixed
   array must have an exact arity and admitted numeric/enum/count ranges; source,
   request-generation and undo tokens must bind replies to the retained parent
   inputs. Never use a received boolean as an adoption capability.
3. Run admitted and cap-boundary probes on the fresh geometry build. Freeze
   measured startup/preview/commit/kill/reap deadlines and steady-clock rules,
   plus a caller table including synchronous import lifetime. Preserve strict
   tolerances and all work caps; timeout refuses instead of extending them.
4. Implement the fixed-path QCoreApplication helper, build/package it in CMake
   and qmake6, then the asynchronous parent lifecycle and private trusted-result
   adoption path. No public constructor may accept a fabricated checked flag.
   Heavy generation/checking and trim preparation run in the built child; parent
   structural/transport work is separately bounded and accounted, with no hidden
   re-run of the expensive checker or reset of the child's geometry allowance.
5. Route each creation/property/dialog/grip/Update/trim caller through that
   lifecycle, retaining final-input seals and atomic publication. Keep cheap
   cache-only paths local. Exercise malformed/late/crashed/hung children,
   cancellation/destruction and subsequent recovery before UI qualification.

W0 exit: protocol version, field/type/enum/array table, fixed trusted helper path,
encoding/length arithmetic, numeric/counter caps, monotonic timing origin,
startup/preview/commit/termination bounds, packaging destinations and supported
platform behavior are recorded, with worst-admitted-case probes and refusal
behavior. Probe work is finite and cannot change geometry tolerances or hard work
caps merely to achieve a timing target. No unresolved choice is hidden in W1.
An early helper-launch prototype may supply W0's startup measurements; it does
not close W1 packaging, result-authority or lifecycle acceptance before the
remaining W0 choices are frozen.

The current request envelope is `[4,token,operation,body]`. Token contains
positive request ID and nonnegative generation/source-revision/undo-epoch int64
values. `lc_offsetworkerrequest.h` fixes every nested positional record, native
member order, enum and count admission. Request values do not confer editability.

| Operation | Body / callers | Remaining integration constraint |
|---|---|---|
| Generate0 | `[definition,runLimits,throughPoint-or-null]`; eligible creation/repeat, distance/side/basis edits, grip/dialog/sheet/Update preparation | Generate/check once in child. For Through, retain the actual picked point and run the existing final cache-containment proof with the remaining allowance. Derived distance alone is insufficient. |
| ValidateCache1 | `[rawCache,definition,worldTolerance,checkLimits]`; file/native cache admission | Validate original raw arrays before any repairing constructor can establish eligibility. Preserve callback lifetime and unpublished-document transaction. Do not rerun the generator. |
| TrimCache2 | `[cache,cutter,pick,trimLimits]`; stored-cache Trim and cache operand preparation | Exact stored cache only; parent attributes/owner/metadata stay outside child. Preserve all sections, roots and work records in the reply. |
| TrimConic3 | `[cache,conic,conicPick,cachePick-or-null,trimLimits]`; conic complement and Trim Two | One original contact session and shared budget; prepare both operands before one parent undo/publication. |
| CheckThrough4 | `[cache,definition,worldTolerance,point,checkLimits]`; unchanged-definition Through grip/dialog edits | Membership-only receipt; no generation, C rerun, returned curve or native admission. The exact raw input remains bound through parent completion. |
| Produce5 | `[source,context,bounds,coordinate,fallback-or-null,distance,copies,asOffsetCurve,preview,batchLimits]`; shared ellipse/raw-native-cache copy series | Complete source diagnostics, exact per-copy output groups and definition/proposal binding. Parent uses private completion, preserves attribute policy and measures its own display expansion. W16-W19 subsequently qualified portable preview caches and local creation/repeat routing; platform and deployed acceptance remain open. |
| CachedProduce6 | `[Produce5-body,ordinaryPreviewSnapshot]`; snapshot `[revision,otherEntries,cleared,entries]` | W17 qualifies pointer-free cache handoff across fresh processes. Response `[Produce5-payload,updatedSnapshot,reusedCopies]` binds sorted copy ordinals and their remaining output budgets. Cache hits consume no new producer effort; native adoption/rebase checks remain mandatory. Existing operation0-5 layouts are unchanged. |

Cutters are owned Segment/Arc/Circle/Ellipse/Spline values. Every spline member,
including latent fit/saved-open fields and flags, and every ellipse/arc render
cache field is transported without normalization. Caps are16384 controls/weights,
32768 knots/saved knots,4096 fit points; semantic layout, positive tolerance and
degree eligibility remain kernel decisions. All wire doubles are finite actual
float64, not coerced integers. Run/Check/Trim budgets may only lower existing
named caps, with zero preserved for a typed kernel refusal.

The matched-token reply layout and private native adoption authority are
implemented: the fixed built child runs the full checker, prepares cache
hints locally, and joins successfully; only the owning client may mint a private
prepared-result capability after strict bounded reply checks. Never expose a
public `checked=true` or raw-value factory. Topology issuer pointers never cross
the process boundary. Parent structural/transport work is separately bounded;
do not hide a second expensive C run or reset the child's work allowance.
Through checking must return its actual additional work, without repurposing
the existing C-only `checkWork` field. If crash/kill loses exact counters, record
that uncertainty and conservatively charge the reserved remainder for that
failed operation, never invent zero consumption or retry it with fresh limits.
Freeze/test no-op, exact-translation and cached clone paths separately so a new
worker does not force generator runs for existing cheap proof-preserving edits.

The current reply envelope is `[4,token,operation,failure,payload]`.
`lc_offsetworkerresult.h/.cpp` defines exact Native15/Curve6 and Trim8 records,
including all trim sections, endpoint/root domains and work diagnostics. Fatal
allocation/internal failures carry null payloads; their stderr prefix is only
the observed partial work, never a refund or final total. Through vertices/pairs
share C's remaining allowance, while depth is a maximum across successive
queries, not the sum of their depths. Every collection/scalar and aggregate is
admitted before copying native arrays. Limits remain unchanged; the synthetic
worst-shape codec test uses245943 values, not a raised frame cap or a geometry
certificate. `LC_OffsetWorkerCompletion` is move-only and privately constructed
by the owning client only after clean exit, exact token/operation join, bounded
reply admission and matching retained definition/raw validation inputs. Its
native materialization retains child-prepared hints and checks unchanged cache
representation; it does not rerun the expensive generator/C checker. Ordinary
wire values cannot call a public checked constructor. Failure or suppressed
delivery without a returned exact record keeps the full reservation charged.

Native15 appends ordinary status, ordinary values and original producer usage.
Only Generate0 native refusals after one successful producer run may carry
Complete/Unsupported; native success, ValidateCache and CheckThrough carry None.
Complete values are exact Segment or normalized raw degree1/3 ClampedOpen spline
records, including flags/knots/unit weights. Unknown output kinds or aggregate
span/control/knot overflow yield Unsupported with no usable prefix. Preserve the
producer's attempted usage: closed-chain seam subdivision and collinear compaction
can change stored span count, which has its own independently enforced cap.
Parent ordinary materialization grants no native eligibility, repeats no producer/C,
strips stale owned schema1 metadata, preserves foreign/local attributes, establishes
fresh graph identity and bounds actual display output. Through-point requests cannot
use this fallback to bypass their required membership proof.
Caller-level shared reservations/final snapshot checks must still be integrated;
the client itself never retries a failed geometry operation with fresh caps.

Finish W0-W2 in these bounded sub-slices before claiming GUI isolation:
1. Retain the now-implemented real parent fault-path tests in a separate owned test executable directory,
   leaving the production helper path fixed and never replacing the real app's
   helper. Exercise crash, truncated/duplicate/mismatched replies, excessive
   diagnostics, delayed exit and recovery; validate report syntax, not exit0 alone.
   Stateful injected-clock preview/commit/kill/reap transitions now execute with
   real children; retain the missed-kill-tick regression. The isolated
   geometry-entry timeout/reap lane is now qualified in `256c201a9` below. Its
   deliberately nonreturning real-numeric-work request is not a naturally hung
   primitive or OS-sandbox claim. The earlier1ms startup timeout and stalled
   transport fixture remain explicitly insufficient on their own.
2. Reuse the bounded `prepareDefinition` description now implemented: no-op/exact
   translation stay local; Generate descriptions run no producer/C stage and are
   not successful edit results. `Completion::apply` only updates a caller-owned
   clone and retains actual completed-stage counters, including late eligibility
   refusal. Extend preparation to preserve Through no-ops/membership without
   forcing an unnecessary generator run. Preserve final Through proof. Route
   dialog clones, sheet batches, grips and Update through the owning client,
   retaining context/source/undo seals, one batch publication and cancellation.
   Do not use nested GUI event loops as a substitute for asynchronous ownership.
3. Retain the now-qualified same-invocation ordinary fallback (W9) and complete
   typed Produce source series/private materialization (W12), including actual
   adoption work and per-copy binding. Preserve the W16-W19 preview-cache
   portability, budget/side/eviction parity and creation/repeat GUI routing. Native
   SeveralPieces/representation/C refusals must never trigger same-input regeneration
   with fresh limits. The existing original-definition rebase followed by a
   DIFFERENT actual stored-cache fallback must retain its remaining allowance.
4. Route stored-cache/conic trim through matched completions and the existing
   atomic parent staging/seals; diagnostic roots alone remain non-authoritative.
   Freeze import ownership separately: synchronous callbacks cannot return with
   outstanding work, publish half a document or resume against a deleted owner.
5. Qualify both full application builds, installed helper discovery, GUI workflows,
   failure injection and the complete file/version matrix. Only then close W1/W2.

W1/W2 launch only the built offset helper through a fixed executable/argument
contract. Handle partial I/O, short/oversized/truncated frames, invalid numeric
payloads, unsolicited/duplicate replies, worker exit/crash and stale generations.
Parent validates typed status/cache/budget snapshots before GUI-thread adoption.
Cancelled, superseded, timed-out or owner-destroyed jobs can never publish later.
Terminate/reap within the frozen bound and retain the previous document state.
Test helper launch and packaged path resolution separately from pure protocol tests.
Use QCoreApplication without creating a viewer/window. Existing engine linkage
may include QtGui/Widgets libraries; do not refactor the kernel merely to claim
QtCore-only linkage. Protocol data remains independent of live GUI/document objects.

Integrate the result at the existing transactional publication boundary, not by
calling mutating UI operations from a helper. Generation/document/source snapshot
and undo epoch must still match immediately before adoption, including after
successful virtual getters that can mutate another batch peer. Preserve the
candidate's final-input seals, queue/snapshot lifetime fixes and exact undo batch.
Cancellation, document/view destruction, worker rejection or a late result leaves
geometry, metadata, selection and undo unchanged. A new job must still succeed
after a real timeout/crash; no permanent poisoned queue or leaked child remains.

Preserve the candidate's X-H7 fixes `c1b2fa095`/`90714f758` and its two-run
qualification as scoped history, not native-worker acceptance. Reproduce any
runner regression only in its own bounded test child. Native W2 additionally
requires a deterministic fake-clock boundary test and a real hung geometry child
watchdog/reap test: termination, no stale reply, unchanged geometry/undo and later
successful job. Do not raise a timeout or disable a case to pass. Direct-child
termination is not an OS sandbox or a process-tree containment claim.

### 4.3 Entity, UI and Source-Action Cards

Reuse candidate `lc_offsetcurve.{h,cpp}` and its real public checker, not a
historical stub or a new parallel entity. Audit its existing enum/factory/coverage
row without renumbering persisted types. Retain clone/INSERT/file tests; no new
`knownGaps`/`notRun` exception may hide missing dispatch. The candidate already
has full property widgets, fingerprint and file support: do not reintroduce an
interim Edit Properties or serialization refusal merely to follow an obsolete
construction order. Preserve its ordinary-cache INSERT fallback where editability
cannot be safely transported. On genuinely unsupported edits, report/refuse or
explicitly downgrade under the existing contract, never silently drop an entity.
Requalify actual creation/import/export paths before enabling a new rollout; do
not reset users' already-persisted candidate options as part of reconciliation.

Use existing modification-batch/undo APIs. Extend every type-keyed clone/INSERT,
selection, snapping, property, file and Trim path needed by the new type. Preserve
the candidate's actual property widgets and refusal paths. U2 must cover
mixed selection and refusals; read-only multi-selection is not a silent shortcut.
Test source properties, not only control coordinates, after undo/redo/copy/paste.

Update from Source reads a legal live same-document ellipse, including a visible
locked source, but refuses generated children and illegal targets. Restore only
defined consistent originals; skip locked targets. Deduplicate against live
same-container ellipses, not deleted undo objects or other block definitions.
Use local scale plus ULP-aware equality, with separate exact-circle rules; do not
merge visibly distinct near-circles at large world coordinates. Preserve source
layer/pen and report updated/restored/refused/no-op counts. No stored source link.

### 4.4 File and Trim Cards

Reuse the candidate's codec-neutral `lc_offsetcurvexdata`, raw-carrier helper and
actual filter integration. Keep the schema independent of DWG-only APIs. Review
`RS_FilterDXFRW::addSpline`, `writeSpline`, `writeEntity` and writer finalization
together: helper-only round trips cannot qualify the application boundary.
Fix only demonstrated codec defects necessary for this schema;
unrelated DWG object work remains excluded. R12/hatch/ordinary downgrade removes
known schema1 editability while preserving supported foreign data.

Reuse candidate `lc_offsetcachetrim` in modification code and its owned prepared
results, contacts/matching helpers and existing cutter/ambiguity rules. Audit
actual intersection, parameter, exact subdivision and allocation paths before
changing the public interface. Do not cast a spline-derived container to
`RS_AtomicEntity`; dispatch before the generic atomic Trim path. Validate both
Trim Two operands before replacing either. Audit existing child-parent mapping and
respect the already-landed #3113 ownership protection; do not reimplement it.

## 5. Required Validation Matrix

These are acceptance requirements, not a blanket execution claim. Fresh results
are limited to the exact source pins and scoped ledgers below; historical logs
and verified fixture/receipt identities stay distinct from fresh product results.
Each future receipt identifies source SHA/dirty diff, target, selector and discovered
case names, command, toolchain, exit status, XML/assertions and relevant fixture hashes.
Never invent expected case counts before registration or count unmatched filters
as success. Tests excluded/compiled out or missing external tools are not PASS.

### 5.1 Numeric, Document, Worker and UI

| Lane | Required checks |
|---|---|
| Geometry | Full A/B/C positive/negative/limit corpus; independent formulas/reference intervals; forward and reverse coverage; no partial results. |
| Entity | Guarded construction/adoption, stale stamp, malformed cache, clone independence, transforms/refusals, no-op and engine-run invariants. |
| Document | Single/mixed sources, several/empty/unsupported results, hidden/locked/generated ownership, preview=commit, cancellation and exact one-step undo/redo. |
| Worker | Typed/size/depth/array limits, partial transport, malformed replies, timeout/crash/cancel/reap, stale ownership, later recovery and real helper launch. |
| Type sites | New enum factory row, clone, INSERT uniform/nonuniform expansion, selection/window/hover/properties/menu and SVG geometry paths. |
| Source actions | Shifted/rotated/reshaped/reversed originals; multi-edit failures/no-ops; restored original dedup, circles/near-circles, blocks/layers and save/reopen. |
| Trim | Open endpoint/middle/multiple cuts, closed cyclic cuts, reversed/multispan caches, root/endpoint provenance, cutter ambiguity, atomic Trim Two and metadata downgrade. |
| Visible UI | Existing renderer only: Offset option, grips/properties, preview, Update/Restore, Trim notice, layer/selection, save/reopen, undo/redo. Include previously unobserved context menus, compact options, held-drag Escape/away-back, locked/stale peers, preview/commit comparisons and Restore Undo/Redo Save/Open. Record display/Qt/DPR and images; headless tests alone do not qualify visible interaction. |
| Failure safety | Allocation injection and exact/one-less budgets; geometry/metadata/selection/undo and old target files remain unchanged on refusal. |

### 5.2 Explicit DXF/DWG Version and Direction Matrix

Never infer DWG membership from enum ranges: `FormatDWG2007` is declared later.

| Lane | Versions / enum mapping | Expected representation |
|---|---|---|
| Application ASCII DXF read and write | R14 `FormatDXFRW14`; R2000 `FormatDXFRW2000`; R2004 `FormatDXFRW2004`; R2007 `FormatDXFRW`; R2010 `FormatDXFRW2010`; R2013 `FormatDXFRW2013`; R2018 `FormatDXFRW2018` | Ordinary SPLINE plus typed schema1, restored editable only after strict raw/adoption checks. |
| R12 ASCII DXF read and write | `FormatDXFRW12` | Geometry-only POLYLINE within the declared approximation tolerance; no editable restoration or known owned marker. Failure to meet the bound fails export, not silent entity loss. |
| Application native DWG read and write | R2000 `FormatDWG`; R2004 `FormatDWG2004`; R2007 `FormatDWG2007`; R2010 `FormatDWG2010`; R2013 `FormatDWG2013`; R2018 `FormatDWG2018` | Ordinary SPLINE plus typed EED with resolvable APPID and full editable definition. |
| Legacy DWG input only | R13/R14 fixture versions through existing readers | Adopt only verified SPLINE/EED; no new write target promised. |
| Low-level binary DXF | R14 and R2018, read and write separately | Independently sourced input reaches normal callback; inspect encoded output separately. Application `fileExport` currently fixes `binary=false`. |
| Cross-format | DXF to DWG and DWG to DXF for each common R2000-R2018 version | Definition/cache, layer/pen and supported foreign XDATA survive; test both directions independently. |

Every editable row covers degree1 and degree3. R12 covers the corresponding
geometry downgrades. Record reader and writer evidence separately, plus conversion,
edit/save/reopen and clipboard cases. An unedited self-round-trip alone is insufficient.
Assert one case-insensitive APPID record for the emitted group spelling and
resolvable DWG EED linkage, not just presence of the marker's text in raw bytes.
Reuse the 52 verified candidate fixtures and their exact provenance/license/hash
manifests. Use independently authored positive/malformed carriers and independently
inspect current writer outputs, including LINE controls. Existing inputs do not
prove current output interoperability. Recheck availability/identity of external
tools before execution; if a tool needed for a particular output row is now
unavailable, only that row stays open with its dependency, not a blanket missing-
fixture claim. Never relabel binary DWG versions or repair fixtures to obtain PASS.

Negative carriers include duplicate/malformed/unknown schemas, nested foreign
markers, APPID case/whitespace, conflicting side bits, oversized strings/tails,
nonfinite or out-of-plane values, partial/nonunit weights, malformed knots/counts,
raw fit-only or repair-needed splines, HELIX and stale metadata after trim/edit.
Assert visible fallback geometry, retained foreign data, no false editability and
atomic output-file behavior. Keep `DWGSUPPORT` guards and real format dispatch.

### 5.3 Build, Discovery and Execution Recipes

Run from the selected clean source worktree, not the dirty main checkout. Record
the actual Qt6/toolchain paths; do not install/upgrade dependencies for this plan
or use artifacts built from another checkout. Cap total concurrent build jobs at8
and serialize test binaries within each build directory.

```bash
set -euo pipefail
RECEIPTS="$(mktemp -d "${TMPDIR:-/tmp}/lc2961-validation.XXXXXX")"
git rev-parse HEAD > "$RECEIPTS/source.txt"
cmake -S . -B build-offset -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build-offset --target librecad_tests librecad_dxf_fast_tests librecad_dwg_fast_tests librecad_offset_adapter_tests librecad_offset_persistence_tests librecad_offset_hostilefile_tests librecad_offset_matrix_tests librecad_xdata_appid_tests librecad_xdata_codec_tests librecad_eed_packet_tests librecad_offsetcurve_batch_failure_tests librecad_offset_cache_trim_failure_tests --parallel 4
selectors=("[lc-offsetcurve]" "[offsetcurve]" "[curve-offset]" "[entity-coverage]")
lanes=(native ui-source adjacent types)
for i in "${!selectors[@]}"; do
    QT_QPA_PLATFORM=offscreen build-offset/librecad_tests "${selectors[$i]}" --list-tests --reporter xml --out "$RECEIPTS/${lanes[$i]}-discovery.xml"
    QT_QPA_PLATFORM=offscreen build-offset/librecad_tests "${selectors[$i]}" --order decl --reporter junit --out "$RECEIPTS/${lanes[$i]}.xml"
done
python3 - "$RECEIPTS" <<'PY'
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
root = Path(sys.argv[1])
for lane in ("native", "ui-source", "adjacent", "types"):
    declared = [n.text for n in ET.parse(root / (lane + "-discovery.xml")).findall(".//TestCase/Name")]
    report = ET.parse(root / (lane + ".xml"))
    executed = [n.get("name") for n in report.findall(".//testcase")]
    assert declared and len(declared) == len(set(declared)), (lane, "empty/duplicate discovery")
    assert len(executed) == len(set(executed)), (lane, "duplicate execution")
    assert not set(declared) - set(executed), (lane, "missing execution")
    assert all(n in declared or any(n.startswith(d + "/") for d in declared) for n in executed), (lane, "undeclared case")
    suites = report.findall(".//testsuite")
    assert suites and all(int(s.get("errors", "0")) == int(s.get("failures", "0")) == 0 for s in suites), lane
    skipped = sum(int(s.get("skipped", "0")) for s in suites)
    print(lane, "declarations", len(declared), "JUnit cases", len(executed), "skipped", skipped)
    for case in report.findall(".//testcase"):
        if case.find("skipped") is not None:
            print("NOT QUALIFIED:", case.get("name"), ET.tostring(case.find("skipped"), encoding="unicode"))
PY
```

These are future commands, not executed evidence. The receipt directory is unique;
also capture dirty diffs, toolchain/build identities and the actual commands.
S0 records actual discovery, including any zero future selector;
after registration, every required new slice selector must discover and execute
nonzero cases with its planned assertions. Catch2 registered cases, generated
assertions and JUnit entries are different counts. Use a structured XML parser.
Both offset spellings are intentional: at the PR review baseline `[offsetcurve]`
adds168 declarations from sheet, dialog, option-enable, UI-helper, plugin and
modification tests which carry none of the other three selectors. Do not rename
tags or substitute `[lc-offsetcurve]` for this lane. Report overlaps without
double-counting them. A skipped external-tool row is not PASS even if Catch exits0.

The listed micro/fault targets are present on the pinned candidate, not the
older attached tree. Build failures caused by using that older tree are not a
reason to delete the gates. Discover/run each filter micro binary separately,
with its actual registered selectors and a unique report; include the grouped
DXF/DWG fast lanes too. Run both fault executables as standalone processes with
bounded capture, not Catch filters: global allocation overrides must never be
linked into the normal application/Catch target. Retain observed fault counts,
exit statuses, diagnostic prefixes and later recovery checks, not invented
expected assertion counts. At R also build the CMake application explicitly.

The native targets below are implemented, not future W1 registrations. Build
them explicitly: `EXCLUDE_FROM_ALL` means neither the app nor main test target
alone produces these binaries. Keep default CTest registration opt-in so a
normal build does not register absent executables. In this dedicated build:

```bash
cmake -S . -B build-offset -DLIBRECAD_REGISTER_OFFSET_WORKER_TESTS=ON
worker_targets=(librecad_offset_worker_wire_tests librecad_offset_worker_request_tests librecad_offset_worker_result_tests librecad_offset_worker_kernel_tests librecad_offset_worker_process_tests librecad_offset_worker_client_tests librecad_offset_worker_fault_tests librecad_offset_worker_geometry_timeout_tests librecad_offset_worker_edit_tests)
caller_targets=(librecad_offset_native_edit_tests librecad_offset_properties_dialog_tests librecad_offset_properties_sheet_tests librecad_offset_grip_tests librecad_offset_update_source_tests librecad_offset_trim_tests librecad_offset_source_preparation_tests librecad_offset_creation_publication_tests librecad_offset_creation_action_tests)
cmake --build build-offset --target librecad librecad_offset_worker librecad_offset_test_helper "${worker_targets[@]}" "${caller_targets[@]}" --parallel 4
ctest --test-dir build-offset -N -R '^offset_worker_'
ctest --test-dir build-offset -V --output-on-failure --output-junit "$RECEIPTS/native-ctest.xml" -R '^offset_worker_'
for target in librecad_offset_worker_edit_tests "${caller_targets[@]}"; do
    QT_QPA_PLATFORM=offscreen "build-offset/$target" --list-tests --reporter xml --out "$RECEIPTS/$target-discovery.xml"
    QT_QPA_PLATFORM=offscreen "build-offset/$target" --order decl --reporter junit --out "$RECEIPTS/$target.xml"
done
```

Require all nine named CTest entries: `offset_worker_wire`, `offset_worker_request`,
`offset_worker_result`, `offset_worker_kernel`, `offset_worker_process`,
`offset_worker_process_ordinary`, `offset_worker_client`, `offset_worker_fault`
and `offset_worker_geometry_timeout`. The ordinary process lane is a separate
execution, not filtered away. CTest resolves the platform/configuration-specific
client/fault/timeout executable paths and the real sibling helpers; never replace
the app helper with a fault fixture. Capture CTest's test inventory, commands,
exit status and per-test output. Also discover the eight worker binaries with
their actual `[lc-offsetcurve]` selector and compare executed identities (collect
JUnit reports with the CTest-recorded executable/selector for that purpose).
Apply the same nonzero/unique/SECTION checks above to every caller/edit micro
report; CTest success or building a target alone is not a discovery receipt.
Multi-config generators require explicit build/CTest configuration and adjusted
manual binary paths. On Windows include the executable suffix.

The separate `librecad_offset_worker_probe_tests` (`[worker-probe]`) remains an
explicit opt-in diagnostic, not GUI timing acceptance. Packaged helper discovery
still needs deployed execution; source-tree sibling success does not close it.
Merely building the application or running Node validation cannot cover native
worker execution, publication fault processes or those deployment gates.

Configure a separate ASan tree with C/C++ and linker sanitizer flags, matching
the selected source and toolchain. Run focused plus adjacent tags and failure
injection; report unsupported instrumentation/leak detection rather than hiding
it. Run the full CMake suite at integration points and final R; compare named
failures against S0 without rebaselining to hide regressions.

Build/run both fast-lane targets with their existing excludes
`~[.] ~[.slow] ~[slow] ~[corpus] ~[external] ~[external-reader] ~[fixture]`.
New required CI tests must not be hidden by those tags. File tests use
`test/lc_filtertestsupport.h`/`test/lc_sourcefonts.h` and only QCoreApplication
in fast lanes; action/UI tests stay in `librecad_tests`. Do not copy the old
file-local settings helpers or its broken DWG range detection.

Register product sources in both CMake and `librecad/src/src.pro`. Verify a fresh
qmake6 application build, not qmake5 and not a claim of qmake test registration:
resolve the installed Qt6 `qmake`, check its version, create an out-of-source
directory, run it on `librecad.pro`, then the generated make target. Audit helper
packaging for W2. Do not turn this feature into a general build-system rewrite.

### 5.4 Historical Candidate Evidence

The R01-R15 index in `offset_curve_2961_validation.md` and the interactive smoke
record describe their pinned historical source and execution scope. Old links to
the superseded per-slice plan sections route here after consolidation; the
original detailed roadmap remains available in Git at the candidate SHA above.
Their temporary artifact paths are historical locations, not guaranteed current
files. The interrupted temporary S0 run also lost its checkout and artifacts.
Neither old pass counts nor a remembered terminal result qualify this new tree.
Retain fresh reports under the durable evidence root named in the introduction,
and record the exact source and toolchain before accepting a slice.

## 6. Per-Slice Execution, Review, Commit and Self-Update

One owner operates a source/build set. The slice card contains: exact base and
allowed files; predecessors; public success/refusal/no-op/fallback contract;
numeric/resource/ownership/transaction invariants; named tests and independent
oracles; target registration and literal commands; known baseline failures; and
the concrete condition that would stop or split the work. No investigation is
hidden inside a supposedly ready implementation leaf.

1. Reconcile the card against current code. Close completeness, design correctness
   and readiness before coding; use a bounded probe slice for unresolved choices.
   Stop only the affected slice and descendants, not unrelated baseline auditing.
2. Implement the smallest buildable deliverable and its tests together. Complete
   registration/discovery, focused/adjacent tests, sanitizers and affected file/UI
   lanes. Keep failed attempts and do not weaken bounds/assertions to pass.
3. Freeze the tested source/diff. Review correctness, full requirement-to-test
   reachability and unexpected scope. Missing/skipped tests, unresolved defects,
   changed acceptance or stale source identity blocks commit.
4. Commit only the owned product/test changes. Then update this roadmap with the
   commit SHA, actual commands/results, limitations, closed findings and next
   slice readiness; make a separate plan-only follow-up commit when its index is
   isolated. Do not include another person's already-staged plan/source changes.
5. Verify the committed code matches the tested content and the plan accurately
   records it. Only then move to the next dependency. Recompute descendants after
   rebases, shared API changes or newly discovered failures. No inherited PASS
   from previous candidates, renderer work or narrative status alone.

Do not demand a new standalone packet/agent revision for each wording edit.
Technical evidence must remain reproducible and reviews must be substantive;
hashes prove identity, not geometry or correctness. External PR posting/push,
publishing/CI dispatch and D0 approval remain explicit user actions, not evidence
that local implementation is complete or a reason to silently bypass local tests.

## 7. Immediate Next Steps and Completion

### Current Execution Ledger

`3e3f158f631a1bc3007520afc2d824bbb2d847f0` closes the bounded S0 roadmap/runner
compatibility sub-slice: consolidated roadmap,117 checked links, correct public
checker reference and strict stable-marker/legacy H6 parsing. Gibbs' final
focused parser/accounting run passed6/6 against both the live roadmap and the
actual immutable candidate snapshot; final single-file H6 run passed99/99,
zero skipped/cancelled/pending, all71 original classifications and93 executable
obligations intact. The two previously failed unchanged genuine-preflight cases
passed2/2 on this source/runtime without increasing timeouts or dropping checks.
Node version: `v26.12.0`. These are developer-tool results, not native isolation.

Evidence is under the durable root in the introduction:

| Artifact | SHA256 |
|---|---|
| `S0-H6-final.log` | `0d9ad8ffebbd34d7e40851ecbb85f53146769d072462433407a7fefb418403b8` |
| `S0-node-genuine.log` | `7a2547e13f1070b47a6a3a41fd54a4aa91f00b62e1411111e17975242a476259` |
| `Gibbs-validation.txt` (commands, red controls, scope and review) | `59b8ff5c137e95c6aeadead8feba48b683864a88e8f44b9b6e3435dbb3f63fac` |

The immediate plan checkpoint for that commit is `311eb0ffd`. It is not
whole-roadmap completion.

`cbb9b581214d09e5120ab92fb6dcbcc17f827a45` closes the ordinary DWG2007
representation-test correction. Both ordinary round-trip paths use explicit
DWG enum cases and execute the R2007 row. The new physical-file control checks
`AC1021`, both ASCII DXF negatives, retained ordinary type, exact degree/knots/
weights, endpoints and independent Bezier samples/derivatives, layer and pen.
Fresh CMake Release built `librecad_tests`, `librecad_dxf_fast_tests` and
`librecad_dwg_fast_tests`. Final selected ordinary tests passed14/14 with16618
assertions. Removing only the DWG2007 switch case caused four reported failures
across the routing test and both ordinary R2007 file paths (exit42); the mapping
was restored, rebuilt and the full selected set passed again. Adjacent DXF:
625 cases,624 passed,1 optional external-SHAPE skip,75336 passed assertions.
Adjacent DWG:481 cases,480 passed,1 absent optional visualstyle fixture skip,
26805 passed assertions. No external/native-worker/whole-suite acceptance is
inferred from these selectors. Catch2 JUnit SECTION entries are not extra test
identities; case counts above come from the compact execution reports.

| Artifact | SHA256 |
|---|---|
| `S0-ordinary-restored.xml` | `3d4befeae74e4bf708d2fed204aa54216c08283ac1f019e5984efd910c8d0c8d` |
| `S0-ordinary-restored-counts.log` | `dc6eb23fd3667452023271f5e46e19ab6fae290514ee0f0b4f0f67abab259ba7` |
| `S0-ordinary-red.xml` | `e4a207068ea28d4f141e25b3817ede20b77e0e92e4ab253be3c6d1a1b4b86aef` |

The immediate ordinary-test plan checkpoint is `7363cd753`.

`7bfbe5f49811454d7bab5d29aa3c063648746bff` implements the preliminary native
wire sub-slice described in4.2. Final wire tests passed7/7 with1022 assertions
each in CMake Release, AddressSanitizer (`detect_leaks=0`, no leak-free claim)
and qmake6. Exact hard item/depth/array and lower frame boundaries, every split,
one-byte delivery, finite double/sign-bit fidelity, signed integer boundaries
and immediate overflows, malformed/extra/truncated frames and sticky refusal
lifecycle are executed. The retained first run correctly failed on CBOR -2^64;
the fixed range check refuses Qt's wrapped negative-magnitude accessor value.
Review P2s were closed by adding immediate-overflow/lifecycle cases and opt-in
CTest registration. Default configuration registers zero offset-worker tests;
`LIBRECAD_REGISTER_OFFSET_WORKER_TESTS=ON` plus building the named wire target
passes the actual CTest row1/1. Existing explicit-target CI is not made to run a
missing `EXCLUDE_FROM_ALL` executable.

Toolchain: CMake4.4.4, Qt6.12.0, installed Catch2 3.16.1, AppleClang21,
macOS SDK27.0. The initial qmake attempt encountered the existing shared SDK26.5
stash. A private `.qmake.cache`/stash selected SDK27.0 and the isolated wire
project built/passed. The shared stash was not removed or changed. This qmake
result qualifies the wire sources, not the full application/helper package.

| Artifact | SHA256 |
|---|---|
| `W0-wire-final.xml` | `81ff40f417d3f6d49d3224e1ac613e4d518e653950165eef63d8ee25d84d4aff` |
| `W0-wire-asan-final.xml` | `94ef9d803f6f685a8af95ab4a8684eeaaf115935744b0ebd9d95ea1369057bd1` |
| `W0-wire-qmake-final.xml` | `7a420e06969d0578fc376e79d6ece4bfa17c27ca99f216065041c7f25f05bf1a` |

The immediate wire plan checkpoint is `870903b52`.

`22a7ee970a656ed15bd62b756b0a62c330e95275` adds the typed request module and
bounded, opt-in framed calibration probes. Final request tests passed10/10 with
591570 assertions in both Release and ASan, including optional Through points.
The initial `[worker-request]` selector matched zero and failed; only the
discovered `[lc-offsetcurve][worker-protocol][request]` run receives credit.
The worker module is outside the geometry package to avoid adding a modification
dependency to the kernel. CMake and qmake source registrations are updated;
this is not a new full-application qmake qualification.

Framed probe runs passed2/2 with194 assertions in each configuration and emitted
13 actual measurement rows. Degree1/3 caches at1,64,256,1024 spans were admitted;
4096-span cases refused at the unchanged1048576-pair cap. Largest request frame:
577825 bytes/86079 decoded wire values. Largest observed Release row:600369us;
ASan:1193974us. These measured families are not a proof of universal worst-case
latency or a GUI deadline. The separate120-second probe watchdog is not a product
timeout. Startup/preview/commit/kill/reap policy remains to be frozen/tested.

Fresh broader Release and exact-bundled-Catch2 ASan offset selections each ran
1107 cases:1106 passed,1 named unavailable legacy-writer skip,4895381 passed
assertions. Release geometry/type selection passed387/387 with1577933 assertions.
The ASan provider is the exact local gitlink
`5aa8d113215bd9a97ecc1a2f3fc9506947a2fa57`, with no network install; Release
results above retain their installed-Catch2 provider identity. Standalone failure
executables passed in both configurations: batch preparation2 C++ new failures,
undo-entry failure1 and allocation-free handoff; trim1616 staging failures,
20 later cancellations,4 conic failures and4 conic cancellations. They do not
inject Qt/private malloc or prove generic post-publication allocation atomicity.

| Artifact | SHA256 |
|---|---|
| `W0-request-through.xml` | `aa3bfae521949a33e1f64bb5496b8fbf188f08209fcbcedea22a8b7410c34d09` |
| `W0-request-through-asan.xml` | `b5c066d0c337f691c607e07b992427ae06b2dcd3bcff173ad5d1a55f32a75e05` |
| `W0-probe-framed-runtime.log` | `befcee7e9f36f150a889e50baaac85a9475660645ea9813d7908f7810d12a97b` |
| `W0-probe-framed-asan-runtime.log` | `d5396ceecf432d28a534ef7a768938eb5642cfdd058704b403ed7b79eb6ca25c` |
| `S0-offset-all-asan.xml` | `f46b20db584093f4e436836b53041f5fb01e8af0ea0271165777804d3035e6d1` |
| `S0-offset-all.xml` | `7905daf7ff6b5e697fc49946628a999283650452272186fe7f60106ba2506883` |
| `S0-geometry.xml` | `0558640e6c0687e45accee90360c2f6f36bcb70b07e22ba6351668c2aef96425` |

`9655dd18bf9a847dcef7879ac9c3c131d0d594e4` adds the real headless helper,
strict owned replies, Through accounting and the asynchronous parent foundation
described in4.2. Release and exact-bundled-Catch2 ASan each pass25 native edit
cases/13404 assertions,17 reply cases/268869 assertions and6 kernel cases/527
assertions. The kernel selection includes eight actual reduced producer/check
budget refusals whose completed-stage records must still serialize unchanged.
Actual subprocess qualification passes all four operations, Through success/
refusal, original malformed validation/cutter payloads, EOF/partial delivery and
invalid/truncated/extra frame refusal in Release, ASan and the qmake6 helper.
Eight client cases pass in Release and ASan: private completion/native cache and
attribute retention, exact known geometric refusal, queue replacement, immediate
cancel, inclusive deadline predicate, lower-deadline timeout/recovery and owner/
deferred-delivery destruction/cancellation. Polling assertions vary with timing;
case identities, not those iteration counts, are the stable inventory.

The first kernel trim fixture hit the existing exact-axis LimitExceeded response;
the established off-axis admitted fixture passes without changing a geometry cap.
The initial client run returned parent exit0 but produced malformed XML because
a pre-exec child inherited Catch's signal handlers/report files. Those results
are NOT accepted. Guarding graceful signals until `started()` fixed the race;
fresh Release/ASan reports pass strict `xmllint --noout`, including the preserved
red control's `Extra content at the end of the document`. No test is disabled and
no timeout raised. Opt-in CTest now registers all six built worker lanes and
passes6/6 in3.83s; default registration remains off. These are foundation tests,
not UI routing, hung-geometry, deployed bundle or whole-roadmap acceptance.

The qmake helper is a genuine full source-list headless build, using separate
object/moc/uic products and the guarded GUI main; normal app source lists are
reused rather than independently drifted. Its actual binary passes the same
subprocess test TU via `LIBRECAD_OFFSET_EXTERNAL_TEST_HELPER`, a test-only CMake
input never consulted by the product client. Measured initial Release process
row395ms and subsequent46-50ms rows include launch+computation; earlier cap-boundary
probes remain scoped measurements, not universal timing proofs. ASan retains
`detect_leaks=0` and no leak-free claim. Both shared index hashes are unchanged.

| Artifact | SHA256 |
|---|---|
| `W1-through-final.xml` | `db580964fed72928e23ba6cf4ca09daf7a4a2d6f1a4f9baf3e863162e7e1b8cc` |
| `W1-through-asan.xml` | `a7f3d2369365eecbf8366bfe3e9dc1ff1ab8265fdfcd28ab4213d676daf79a48` |
| `W1-result.xml` | `4cabc84b5365a24670672c0678e58e472c0427749202d19fb37bb049331623f3` |
| `W1-result-asan.xml` | `5b22349f9c95b93b9beca489226c1d70c2bac5fc1dfa9127cc16d891691a170f` |
| `W1-kernel-limits.xml` | `91aff6a47fbaa1dec5850b9d8262c80a6e51fcf81865aa30af8a1b3843d5155f` |
| `W1-kernel-limits-asan.xml` | `d9857da49ba352370862b86cf3363967082331245fde8963cef56d524414518e` |
| `W1-client-final.xml` | `41065a313a74c756657c9f6d5ea1c66d1c682510539a02245c8316f333b759d9` |
| `W1-client-shipped-asan.xml` | `704aae7cac3ba791c07bb7abf9b9e0a2aa3c7c5ba7880a60b3e39fead9f900e2` |
| `W1-qmake-process.xml` | `5b6cc0fd948d16a47e5cde636a70db24ec690495c2272b89a5e7481bf4aad5b2` |
| `W1-ctest.log` | `5c45478e1b2a94cc9424ff7c77e34324eaacbf9407fc3b6822cee5362fe07aba` |

`5274355ef9ed44176fc8ac8e8110ffd9aa2ba762` is the next bounded foundation
slice. It extracts generation-free immutable definition preparation, reuses it
in the existing setter path, and adds private-completion application to owned
clones. Source/cache/attributes and exact-translation/no-op behavior are preserved;
invalid/stale sources refuse before job creation. A Generate description cannot
pretend that producer/checker stages ran. Native edit tests pass28/28 with15262
assertions in Release and ASan; client/application tests pass8/8 in both.

Seven isolated fault cases exercise20 failure modes, same-client queue recovery,
unknown-work charging, late clean exit, startup-independent stateful fake-clock
preview/commit/kill transitions and the inclusive quarantine boundary. A real
correctness gap was fixed: an event turn skipping the earlier kill tick must still
request kill before detaching/quarantining. The retained notifier is observed to
exit/clean up; `ReapFailed` is not mislabeled as a successful reap and the poisoned
context cannot launch another job. Fixture code never installs into or replaces
the real application helper and cannot fabricate native curve authority.

Release CTest passes7/7 (41.60s in this loaded run). The ASan fault suite's24s
aggregate watchdog failed while competing with the geometry suites, then passed
unchanged7/7 when executed alone. Keep process/client/fault CTest lanes serial,
and do not externally co-run proof-heavy sanitizer suites with them. No product
deadline, watchdog or geometry cap was raised. Current reports pass strict XML
syntax checks; polling assertion counts are timing-dependent. ASan continues with
leak detection disabled and no generic leak/Qt-allocation claim. This qualifies
transport fixtures, not a hung geometry kernel or a GUI transaction.

| Artifact | SHA256 |
|---|---|
| `W2-preparation.xml` | `defeba1b7cf9fd0dcfff86b864ec2ba7b52d919277453d53e2f267e398b6712e` |
| `W2-final-preparation-asan.xml` | `bfff4bb43e1a6f817ff6d9d706f6c5f869355f4fe216ea0913ffa2d0be1c95b2` |
| `W2-apply.xml` | `a2d401633021c1213a43cd1ba32cf241e7efbefa999f70e68ad3ad83ef33d346` |
| `W2-final-apply-asan.xml` | `761ce72a0a39da165c79b8f4265dee88b088316abb60354b1d3c74c15e6f2836` |
| `W2-clock.xml` | `999dc1d576cb0f0a20275ac88e6aec07e491b6e1eb0d645f3e99e428b359c247` |
| `W2-final-fault-serial-asan.xml` | `7b36d2ec37080021cbdd6981400b5ca5ffd0f5376a08d6a17d3bedacfbc99416` |
| `W2-final-ctest.log` | `c0e7203dac50817f8e6860f398ad2e39380eec2203bd23016b9a350a164fefd5` |

`705a0135b4727c494c5fce9d13b469d0efafddbe` routes the single-entity
properties dialog's owned clone through the real fixed-path helper. No-op/exact
translation remain local through an opaque source-bound preparation; a moved-from
translation cannot report a change. Generated replies retain actual child stages,
then recheck generation, source snapshot and document/UCS/outer action guard before
adoption. Rapid proposals coalesce; Accept/pickers are vetoed while pending;
rejection cancels without preventing reuse; final acceptance/destruction detach
before clone publication/release. Refusals restore cached text without revisiting
a revoked formatter context. Executing editor guards are copied and followed by
lifetime/generation checks; cancellation-signal reentry cannot overwrite a newer
binding. Ordinary entity widgets retain their existing setters.

Release and ASan each pass all55 dialog cases/25793 assertions, preserving all38
original case identities plus17 new cases. The10 opaque-preparation cases/90569
assertions also pass both builds. Reports pass strict XML validation. Retain the
earlier red moved-from test and four-failure dialog report: the former exposed an
authority-transfer bug; the latter included a synchronous picker expectation.
The final55-case runs use that original failing order/seed3812149849. No geometry
cap, product deadline or3500ms test watchdog was raised. ASan leak detection stays
disabled; this is not a generic leak/allocation-atomicity claim. The qmake6 full
application build was started and remains pending at this checkpoint; prior
helper-only evidence does not qualify the changed application or deployed bundle.

| Artifact | SHA256 |
|---|---|
| `W3-review-dialog-release.xml` | `f9fa4b50fb6826d6d0ad23b8c7f3ff85ecce3f5103476a25bea960eddaf78152` |
| `W3-review-dialog-asan.xml` | `c365d6a5889abbc1de57fb4dfc363a8a783e4c7014bc8f54cafa22e1d74e332b` |
| `W3-worker-edit.xml` | `7cd4561007539b6f5903872bcd7a86afbec7db9c44cf1b403d343d0ecb902f11` |
| `W3-worker-edit-asan.xml` | `8aa28a79a8fd2bc1aee48e1cad334478f1b1b1bf3a7be91543713b6899ca3b3d` |

`ac7c7646fcb1afff115807fdc7524672b24350f2` routes property-sheet geometry
proposals through one owning client, sequential peers and a shared deadline.
Attributes remain cheap. Accepted peers stay private until the batch completes;
prior accepted candidates survive an individual refusal. Publication seals the
source, selection, document/view, undo epoch and complete UCS/angle direction.
Same-context row refills retain value-owned work without retaining providers;
changed contexts and Escape cancel. Attempt receipts freeze into separately
preallocated storage, preserving actual child counters after late refusal without
allowing an unwinding callback to mutate published diagnostics. Deleted target
layers refuse before dereference/publication. Parent-rendered cache cost is
charged against the reservation: four32-leaf candidates fit a128-leaf batch,
and the fifth never starts the engine. No nested GUI wait or setter interception.

Final current-source Release and ASan runs each pass all41 sheet cases/9367
assertions and56 dialog cases/25937 assertions. All32 original sheet identities,
seven direct synchronous queue test bodies and38 original dialog identities are
retained. Reports pass strict XML syntax checks. The batch harness applies the
existing3500ms watchdog per distinct submitted peer and a total
`min(peerCount *3500ms,30000ms)` ceiling; progress callbacks cannot extend a
stalled peer. Product deadlines and geometry caps are unchanged. Historical red
reports remain: same-context refill cancellation exposed a product ownership bug;
duplicate fixture selection, stale display-child assumptions, paint-cache input
and a whole-batch3500ms harness bound were corrected explicitly. The ASan setting
remains `detect_leaks=0`, with no generic Qt allocation/leak claim.

The full qmake6 application build and subsequent incremental builds completed.
A forced current-source application subproject rebuild also exits0
(`W4-qmake-forced-app.log`). This is compilation evidence, not deployed-package,
visible-GUI, file-matrix or hung-geometry acceptance. One process micro test under
heavy concurrent build load hit its2500ms watchdog; it is not credited as a new
foundation pass. Final R still requires serial current-source requalification.

| Artifact | SHA256 |
|---|---|
| `W4-qualified-final-sheet-release.xml` | `4cb8ee7283bb20d5e501cb2c4fa1e11f180315da0f00d1dc9768aacc8d0ef23c` |
| `W4-qualified-final-sheet-asan.xml` | `4ae58befb50925bbee874c173ef738d65d3b28ab21c2a1f177cc7964216363cb` |
| `W4-qualified-final-dialog-release.xml` | `20280bd913b8c1daaa2486562ad3565857de45f9aa8b6098a511f5dcb38e7fe5` |
| `W4-qualified-final-dialog-asan.xml` | `ffc8b3a9f92fd9c5a383927cb3d630bc96d471dcced70832f01db4349691be21` |

`0d4f77b1ca49e4bb21d0d794c892594776ddc927` adds pure Through preparation
and protocol2 operation4, `CheckThrough [cache,definition,worldTolerance,point,check]`.
Exact stored endpoint no-ops remain local. An unchanged definition requests only
bounded raw-layout admission and membership, with actual costs in `throughWork`,
no producer/C stages, no curve and `changed=false`. A changed proposal uses the
existing Generate operation with its raw point. Membership cannot mint an entity;
private completion application requires the still-editable identical cache,
definition and tolerance. Existing synchronous setters reuse the pure proposal;
the original28 edit cases/15262 assertions remain green. Version1 envelopes and
invocation reject; frame/value/geometry caps and deadlines are unchanged.

Release and ASan each pass request11/596917, result18/269143, kernel8/1112 and
opaque-edit15/101553 (cases/assertions). The real process TU exercises all five
operations; client9 and fault7 cases pass both builds. Process/client/fault
assertion counts vary with event polling. Execution is serial; strict XML checks
pass. Retain the initial red kernel report (zero pair allowance refuses before
layout entry) and edit report (a centre point is a valid analytic proposal whose
disappearance is the producer's decision), both corrected test expectations.
ASan stays `detect_leaks=0`. No current-protocol qmake application/deployed-bundle,
genuine hung-geometry or final R claim follows from these foundation tests.

| Qualified W5 receipt | Release SHA256 | ASan SHA256 |
|---|---|---|
| request | `4a266867df72c5c44c8192c23071cdff795393902ad748adbfaffbf38d64e265` | `fafe53eaf2749d9b59a81bdacdf8f00a404d9fb2e7f47d319bb6cf8bf01f7ec0` |
| result | `60e53fbe1410ee683c95bc7faa398c6ce419b34f18b6fd76836abfff4c1306aa` | `a7fac6a2be4517d317e8c14559a1d60cae9cdc4b75a7f007c8b213dad6f16c95` |
| kernel | `0e5daf12a156384bf70e2db87fff8e9b9e4986d32f021669cf02b235c2d3def4` | `6bdc4090418214204e55177ef6d58ea9fdda39cf05bf5e0203715e4c6297d0d7` |
| edit | `8b71dc553b96b501016ecfdc8ce7a61de2158591498eecb112256513cb30331e` | `804d37a20ff4c3e6f92b725093b2b6fd8c2ace65cafa92ce08cdb72811af4e64` |
| native_edit | `5546d920b4f8464917df4c6fa3ec72f60de375badb497fa5f61a74e5f563fa18` | `0c8bfc9be4b55fdb9207e3e968b189bec6ee2065ddcb66ce7e03ab01679d389e` |
| process | `edb7ad75c719274853e8d30c213aae61e23c482d886cb121b3d5981aaacc06cb` | `ed26573f403957a03114a641f261c2c360373b79d2b1cbd3813d90163e8a2f53` |
| client | `e9cc200698cbffd4133cda2f8cbc180ff42dd746ce3f10554e0ae1fb5f41c237` | `3fadb24f72d6fe881d57f32ffa62d7f8d1178ee10572df2e8c14173a57e62586` |
| fault | `03485d128a2af3a30d25d03d247fe6a86e6b94f129c94b1bf7cd27bfb9484e31` | `66c99e012f18629d50ed029256983850fe62522ab7be23d9d103e4a1413600a4` |

Files are `W5-qualified-{receipt}-{release|asan}.xml` in the durable evidence root.
`0e80b8540a6771f18e422983ef322ff1e9803fee` routes native grip commits through
the fixed helper without a product nested event loop. Preview still draws only
bounded cached feedback; returning to the original cached grip stays a pre-clone
no-op. One action-owned continuation retains its private clone/input witness and
monotonic request identity. Repeated releases do not submit another job. Escape,
right cancel, init, suspend, finish and action/view/document death revoke it before
late delivery. A fresh gesture can recover while the cancelled child is reaped.
Completed child work is retained even if source/undo/context/UCS/layer checks
refuse publication. Parent display expansion must fit the original reservation.
All callbackful reads precede renewed membership and the final stored seal, then
one ordinary undo replacement transfers ownership. Attributes/foreign metadata,
cheap no-op redo and unchanged-cache membership are preserved.

The44 grip cases retain all36 original identities and add8 real-helper cases.
Release passes77934 assertions; ASan passes90582 (heartbeat/event polling makes
counts timing-dependent). Both complete suites pass strict XML checks; the
existing3500ms single-job harness bound and1ms heartbeat were not raised. Earlier
red receipts are retained: a radial decimal query projected to another distance,
the tangent-line fixture had no unique chosen grip, and a non-pixel-aligned mouse
query was rounded by device conversion. The final membership test uses a real
certified Through baseline and repeats its exact pixel-aligned interior query.
These are explicit fixture corrections, not relaxed geometry, grip matching or
producer/checker limits. ASan remains `detect_leaks=0` without a generic Qt OOM or
leak-free claim. No current-protocol qmake/deployed or visible-GUI claim yet.

| Artifact | SHA256 |
|---|---|
| `W6-qualified-grip-release.xml` | `cec8a0db22377fcb9b3cc2e479b60a1c875c66a9cc8ebfc37a388283e1cbd935` |
| `W6-qualified-grip-asan.xml` | `175183e17903026afdd881ede1617c55ca466cb9551736a7e3d75ceea35a2fe3` |

`075038088ea1ef87e8b594ad58f3a75a707b5025` routes the Update from Source
GUI entry through an owned sequential worker batch. It freezes the selected
inventory and picked ellipse values, preserves cheap no-op/exact translation,
retains actual child stages, and seals source/targets/selection/undo/view/UCS
before one undo handoff. The existing protected synchronous non-GUI batch callback
and its prefix/exception controls remain; GUI publication reuses its pure handoff
without invoking another setter/generator. Prepared counts observed by late
getters include genuinely received child candidates, then decrement on refusal;
they are not claims that document publication already happened.

The request retains a shared30s preparation deadline, four-source output and C
work allowances (depth is a maximum, not a sum), and conservative logical input
copy charges capped at4MiB per target/16MiB per request. These active-storage
charges bound cloning/witness amplification, including child metadata; they are
not exact allocator/RSS measurements. Full lost-work reservations stay charged.
Exact child C work can settle C allowances; output settles only after actual
parent display expansion fits. No automatic retry receives fresh allowances.
Cancellation freezes receipts in separate preallocated storage. Conflicting
picks do not replace the admitted source; reset, Escape, suspend, finish and
action/view/document death revoke pending work. Initial source callbacks cannot
revive a reset/deleted action admission. Private owners survive executing callbacks
and are retained until the document accepts the presealed lists.

Release passes34 cases/4438 assertions; ASan passes34/7102 with1ms heartbeat and
the existing3500ms per-submitted-peer/shared30000ms test ceilings. All26 original
case identities and protected handoff assertions survive, plus8 new real-helper,
context/lifetime, initial-reentry and quota cases. Strict XML checks pass. Retain
the earlier red ordering receipt: a successful child candidate was not counted
before late observed source reads; the fix preserves those reads and their final
stored closure, rather than weakening the test. Polling changes assertion counts.
ASan stays `detect_leaks=0`; no generic Qt allocation/leak guarantee is claimed.

| Artifact | SHA256 |
|---|---|
| `W7-qualified-update-release.xml` | `ae5295d2364b4ddc3a4639d1c3f2dfb67fabc1fac7f8de3680621d1b2f2fa1b1` |
| `W7-qualified-update-asan.xml` | `4e28682ab61fcc8dc8cadaf0ddc011cb5c4610d79ba8d05a99b835ea830b60c3` |

`256c201a9d505fc8c65124d7b3816c68ee457935` adds the isolated
`offset_worker_geometry_timeout` lane (`RUN_SERIAL`). A dedicated uninstalled
helper compiles the same main/kernel with a test-only Generate loop. It completes
an actual engine+C pass, emits one bounded observed-work witness, then deliberately
never returns while repeatedly doing real numeric work. After the witness, the
parent's private test clock advances to the unchanged30000ms commit boundary;
100ms termination/1000ms reap use real monotonic elapsed time. macOS observes
SIGTERM ignored only after actual entry, kill escalation, successful reap, a
responsive parent, no completion capability and full unknown-work reservation.
The same client rejects ID reuse and recovers through a genuine cache-validation
reply. This reproduces a deliberately nonreturning geometry request, not a
naturally hung numeric primitive, and establishes no OS sandbox/process-tree claim.

Release and ASan pass1 case each (1058/1926 timing-dependent assertions), with
strict XML checks. CTest discovers and passes exactly1/1 in both builds. The ASan
cache initially had worker test registration disabled and returned zero discovery;
that run receives no credit. Registration was explicitly enabled and rerun. Product
helper binaries in both builds contain no fixture marker; only the isolated test
helpers contain it. No product input, environment switch, path setter or command
option enables the compiled-out loop, and no application helper was replaced.
Retain the red recovery assertion: the normal `RS_DEBUG::setLevel(0)` startup line
is allowed, while stale geometry witness/failure prefixes from the killed job are
explicitly forbidden in the new reply. ASan remains `detect_leaks=0`.

| Artifact | SHA256 |
|---|---|
| `W8-qualified-timeout-release.xml` | `3a50afc68766544b9f2de1c20e5870fc2517b9420d55ae423fe06caa48cfe3ca` |
| `W8-qualified-timeout-asan.xml` | `b5d19a3fdc4113b30f6342f02114540f5a48239b8c7297088ba64be0b954923c` |

`1d17448e61f52b5faa41167bda1d1a638c88c81a` implements and qualifies the
same-invocation ordinary fallback foundation (W9). Core creation retains owned
producer output only on native refusal, reverses the complete directed output
when requested, and clears it on native success. Protocol3 carries all-or-Unsupported
ordinary output with strict two-pass shape/aggregate admission, actual producer
usage and no public native-proof bypass. Private completion materializes ordinary
geometry without regenerating/checking C, rejects Through fallback, strips stale
owned XDATA and keeps foreign metadata/local attributes with fresh identity.
Current helper/client/fault invocation and version-negative tests agree on3.

Both builds pass all13 complete micro suites with strict XML and no failures/errors.
Names are `W9-qualified-<suite>-{release,asan}.xml` under the durable evidence root
recorded above. Full micro discovery includes ordinary/Restore cases beyond the
older filtered sheet/Update runs; do not mistake those additional cases for renamed
or removed original tests. Polling assertions vary with scheduling.

| Suite | Cases | Release / ASan assertions |
|---|---:|---:|
| worker_request | 11 | 596917 / 596917 |
| worker_result | 22 | 467435 / 467435 |
| worker_kernel | 10 | 1557 / 1557 |
| native_edit | 29 | 15311 / 15311 |
| worker_edit | 15 | 101553 / 101553 |
| worker_process | 2 | 3348 / 3702 |
| worker_client | 10 | 9629 / 12470 |
| worker_fault | 7 | 25985 / 19501 |
| worker_geometry_timeout | 1 | 1594 / 2678 |
| properties_dialog | 56 | 25937 / 25937 |
| properties_sheet | 53 | 9367 / 9367 |
| grip | 44 | 77686 / 89838 |
| update_source | 36 | 3154 / 3688 |

| Suite | Release receipt SHA256 | ASan receipt SHA256 |
|---|---|---|
| worker_request | `f4d48e0137fedd4b1cd459235f2dc4a200d79d349877b0f9e393d25a197491b2` | `874c7ceed4937a6c6a07be05d0607e0960e3e21b0888fae3f9444f9ba81cda2e` |
| worker_result | `6e117414140c8a5f7d8b5a2d1e90e856e7ec175d202ace5f833181b079d8a781` | `cee222c28296f6a7d3325fcdabe35ec6c7152a38071487729a830c2f05d47945` |
| worker_kernel | `129ab8e6b98c28585313be44c306bf8957c4839b084767b99df8e4d5ff14e108` | `f6704dcfc0a1dd4825c4d2267093b5903ae20a6db341e8974c3123a7aa3e3dd0` |
| native_edit | `e1ee2b69507376511d35fc2da966acf206eb179df76fa977a19c2624f468bb92` | `2fae5707bf316297dc98ef8ca64f1fa1c42dcb79a027d2b2a00df52a1c62aa3b` |
| worker_edit | `5d4e81bd6a1c139f75f47d95e1c149c4a368b96bdf4eeb039abe33d484ca29d1` | `2d456f5c840fce8c7be576b8925d9f32f4e0cd46339db98c6366b463684ae1f8` |
| worker_process | `1a6df3d31d785b1932006b6f9feedbc7e26d7237a8189024254967232274e486` | `bc647a408ada892838a6c46f4c8708d5aa3a2df06b285a856ea1f836cb9f397f` |
| worker_client | `806c1792b7403b13ef9e4b43abc13c3aa72c8551c9c0b01b5a0c53f48a525432` | `779d3a8be05aedf5ce08618488088c42877a01aa915e58323f6ab717db114115` |
| worker_fault | `1d7fe84d88b527ad4997dec49fdf1ef265e24adb6b8d250ff3196ef22235b964` | `5070580d4d43b796a9d00640d2692ebb609ffa90b7b4b1c8ab20bfc8b9b843d1` |
| worker_geometry_timeout | `5df787ca2ac9e69e1869c6965851829f2a5c51c969431f7f0725edcd4ef8c2f4` | `08749f57cd1d7470019cb3c37c5696b6493be15c809f90172f9b856d6b01035e` |
| properties_dialog | `86ad02eedd967c1f8aa3f0fa807c8e41de6d5c6059eed2d29bf22bce5cef71ce` | `8acc83c87a03757ac597235d9f0080c804d3de36cbddac7f1c4c09b822aa49e5` |
| properties_sheet | `6c7ace8a3d56054dd7f26a70c74fbe4bbb2d33df314d02e97ef07211a5584200` | `b97486f165a132d32a8268c875456c48c69ced6d74f1f62a1c9f0dddcbc39f49` |
| grip | `b545c8c19a8d46df9b8deb58e6762199c3aa7c8b700b28aa14174d6ff1b5fc56` | `256a7b8d112344a568811896e1936d100cda52ce5c064653983d192828895574` |
| update_source | `00c8892e7a883d1926abb74645b2b32b0d704aa38323f964ab2c024b8baf8d10` | `a7db8805be31a33b4c619b24f313f9af7d0caf1e638921c2c3cba147f8ab6985` |

CTest discovers/passes9/9 in both builds, including wire, all five operations,
faults and real-geometry-entry kill/reap/recovery. New ordinary subprocess exchanges
have their own case/CTest row; the original case keeps all its assertions and
unchanged2500ms child/24000ms aggregate guards. Each CTest row keeps30s. The initial
aggregate watchdog red after adding exchanges is retained; no numeric/product
deadline was raised. Two loaded-host full ASan fault-watchdog reds are also retained;
subsequent unchanged full CTest and standalone runs both pass. No other host job
was stopped. Other retained red receipts cover the producer15/stored16 seam split,
line-fixture correction, and preservation of an empty LibreCad APPID group after
owned-definition stripping. These were not geometry tolerance relaxations. The
temporary-buffer lifetime assertion also no longer needs a C++20 structured-binding
capture extension. ASan remains `detect_leaks=0`; this is no leak-free claim.

| Artifact | SHA256 |
|---|---|
| `W9-ctest-release.xml` | `015a53dd1d1b69ded80a8b64e467e48ff173e11e4117c1f2e3628eab0505b8cf` |
| `W9-ctest-asan.xml` | `6a9efda7f63626f18217ac2e7abca5a2a881a25ba2006ca1dbc2bbdd5cf8d8f1` |

`ef40eecad63231bdb8dff70a9d2828fd96cea199` extracts the existing numeric
copy-series into shared `RS_Modification::prepareOffsetSource` (W10), with a fixed
value-owned top-level/native-definition context for detached inputs. The existing
synchronous API uses the same implementation with its original live eligibility
reads and early frozen copy count; no synthetic document or unchecked native source
is constructed. Side-once resolution, original-source distances, ordinary/native
attribute policy, rebase/cache remaining allowances, output accounting and preview
cache behavior stay unchanged. All native outputs still use real create/adopt/C.

The new public preparation entry clears provisional roots and committed usage on
refusal or exception, retaining actual attempted work. It performs no document/list/
undo publication. A dedicated micro target covers the three existing modification
offset TUs. All75 prior cases plus6 new parity/callback/exception cases pass in both
Release and ASan:81 cases/83207 assertions each. Existing case IDs/assertions remain.
Detached ellipse/native-off/preview, reversed/raw-cache repeat, undefined or nested
native sources, zero-sum fallback, cache hits, sample/pair limits and late-copy rollback
match the synchronous API. Additional consumer qualification passes Update36 cases
(3094/5386 assertions), sheet53/9367 and kernel10/1557 in both builds. Strict XML is
valid with zero errors/failures. ASan remains `detect_leaks=0`.

| Artifact | SHA256 |
|---|---|
| `W10-qualified-source_preparation-release.xml` | `0d47ce4d342f23298a37afb6747a2b2b8be7c67971487958a855d16d71f113f0` |
| `W10-qualified-source_preparation-asan.xml` | `71ab99ade69e858b2f4e3b0df1f68a63e8315acac77990a754760748ff5b9caa` |
| `W10-qualified-update_source-release.xml` | `989956d9c8c6eae4db45151a86a9f64bc7e3d9f357f6995067c8c70f2cb282a3` |
| `W10-qualified-update_source-asan.xml` | `e28d2d7d93bf47e71c1ceacd2443c476a2eb81cded65d82a5b0c8d51efae25d4` |
| `W10-qualified-properties_sheet-release.xml` | `edd350a9142058f5dc4ef1e5bff5090f5c29094a91653594284baf5884389714` |
| `W10-qualified-properties_sheet-asan.xml` | `659ed5a18c1be533acb2aa00d5bd77bffa7b1a0bb694f0516a72226d2eac70a7` |
| `W10-qualified-worker_kernel-release.xml` | `7687155e30e7f3075e773343ef4be942146e54654dc576252de10974e39d92ec` |
| `W10-qualified-worker_kernel-asan.xml` | `36b8dd4ecb615740213db3d2ee69d1ad92da726fa96c570a8ed3eadc59dc792b` |

The last complete CMake Release application relink passed on the clean W9 source
before W10; `W9-build-app-release.log` SHA256 is
`ab67d7ecac022f1d25e06db90cfecba2990fdd3018c0608f7b192930342dde65`.
That is not W10/W12 application/deployed/qmake6/visible qualification.

The bounded S0 audit now classifies every287 F..C path and identifies289 M..C paths,
using the surviving pinned Git objects. Its target is explicitly `eee8454fc` before
W9/W10, not a runtime PASS on current code. All117 evidence-document relative links
(50 heading links) resolve, including corrected R02/checkCache attribution. All52
committed independent fixtures still match. `W10-S0-reconciliation.md` in the durable
evidence root has SHA256
`24fb7623c0d306bf29f4ffad996a0d1caa0c6db128bb0b12c0da59faad8dcf4b`.

The target-relative S0 addendum corrects the earlier removal disposition: inherited
`b05fb541df651dd9cb55a222dee4563e6127c436` (#3140 shortcuts/translations) is
ALREADY an ancestor of local `origin/master` at
`f37ef6b10f96434f505715841cd3712513c9365d`, and is the feature/target merge base.
Retain its shortcut initializers, Chinese test and test-only translation setup.
Only the parent's uncommitted removal experiment was restored; no upstream fix
was reverted or removal committed. Its W11-scope-qualified-* receipts describe
the abandoned candidate, not retained-source qualification. At that audit pin,
W10 was the last product qualification on the identical retained source; W12
qualification on the later source is recorded below.

At feature HEAD `83ca73f99`, the actual merge-base diff has317 fully classified
paths (101 modified/216 added); HEAD...origin/master had391 feature-only/31
origin-only commits. At that pin the109 upstream-only changed paths had not been
merged, copied or reverted. This is a read-only local tracking-pin audit, not a network
freshness, mergeability or integrated-upstream build claim.
The later user-requested W15 merge below supersedes that unmerged status, not the
historical qualification boundaries or this audit's recorded classifications.
`W11-origin-scope-reconciliation.md` SHA256 is
`e59acbe4b576d6729a41f04bd30ee3392dbe116f69459ed4273d16b19a8e86be`.
It supersedes only W10's shortcut-removal disposition and preserves that report's
historical classifications/hash. The audit also confirms that the old14
disposable receipts, inspection clone, attic draft and
both old plan-only JSON reports are absent. Earlier hash checks remain historical
claims, not present availability or new PASS. Do not reconstruct lost bytes from
hashes. Current acceptance must use new scoped durable receipts; missing old evidence
does not waive any current runtime/manual gate. Both protected index hashes still match.

`558c3bba2a8742ae8a03bc3bca76311e74062573` qualifies typed Produce (W12).
Protocol4 appends operation5; operations0-4 retain their body layouts. Source values
are unrepaired ellipse or spline data plus captured bounds and explicit value-only
top-level/native-definition context. The child invokes shared preparation once,
including the distinct stored-cache fallback, side-once policy and stopped prefixes.
It returns every bounded diagnostic, exact copy grouping, actual adoption-call/C
work and typed output. Native outputs bind their copy ordinal and actual normalized
ellipse definition or successful rebase proposal. Unsupported shapes/aggregate caps
retain full diagnostics with no output prefix. Fatal/lost work remains unknown.

Private `Completion::produced` materializes only joined receipts, never reruns the
generator/C, strips stale owned metadata, creates fresh graph identities and grants
no document/delete/publication authority. Native-requested committed ellipse/cache
copies retain bounded local foreign metadata; ordinary/native-off/preview output
retains only the existing pen/layer policy. Actual parent display expansion is
independently admitted/measured before handoff. New curves do not inherit stale
owned extension tails. GUI routing and portable preview cache are NOT implemented
by this transport slice. All old test identities/assertions remain;12 new cases
cover transport, exact ordinal/mode/proposal binding and private materialization.

Fourteen full micro suites pass in both Release and ASan on the source-frozen W12
slice. Case/assertion counts R/A: request12/597055 both; result25/467560 both;
kernel14/2572 both; client13/6090/9071; process3/2495/2831; fault7/18867/21725;
real-geometry timeout1/1082/1890; source preparation81/83385 both;
worker edit15/101553 both; native edit29/15311 both; dialog56/25937 both;
sheet53/9367 both; grip44/77438/90830; Update36/3076/3754.
Strict XML has zero errors/failures/skips. Polling assertion totals vary with
scheduling. ASan remains `detect_leaks=0`; no leak-free claim. No product cap,
watchdog or deadline was increased. The first green Release fault receipt overlapped
the end of source preparation and is retained as `W12-overlap-fault-release.xml`;
the qualified fault receipt below is the later serial run, not that observation.

Artifacts in the durable evidence root use
`W12-qualified-<suite>-{release,asan}.xml`:

| Suite | Release SHA256 | ASan SHA256 |
|---|---|---|
| request | `6e9bc47885e7231809c24698bb4f51487d25a34be8157c4b64698330128b9271` | `7d07b82ce317fa87bf59346bae95900f96a407bb5fca9545d9aab732e90981d6` |
| result | `cf7986b0d4503ea8538a696e610e810eca37fb1fbe3477ab0ecf72fa76a44150` | `162a026134d7e99b005da41601568da765c1d9dcb779d6e2d4a0992939725f90` |
| kernel | `5416ec6be607ecf8949764cf7397bca5a3ea255dbf4bdcb833044bae0e5f3871` | `dcb1d098be8ffe301dcbea084f3553ea24e4fc5ece2a5d31827c08e181c07043` |
| client | `a9378f6428cc4676eff3fbe961985deb52c9d745c75f561ce00d2e6432270fad` | `2b81cdcd36b9f7d5ef968fca90dda10158b976a470636ee7f1bbdc6abaf0330c` |
| process | `b0710b75156268769b06e9095fd592dd01c6c294bdeaac6ccfb62935c3093220` | `7357c0d25bf4026264dc60a644feea473b01be2494ab0cea6c2ec6bf65ee931f` |
| fault | `8b3b63378c99f09d4c63ae26f950b5c2533c21b923bb9b2f7ca29ef94a31e240` | `b6fa98a11b160ab875b7d4d34a0fe786e04359ed24448efc399ea15af2e99258` |
| geometry_timeout | `f6f12d07ef62c753e061594595b3793b76a21ac561a04edce6c1ae4d9285b4f9` | `32a3c028b73bbabf2cc9b31bce4cc32f345fc915f29d99b3e2e649b0b98e3933` |
| source_preparation | `caefa0babf7dd609a18d6d48a363ea3141149bb5a247f0c285d8d80f1f3b7454` | `59513a493cdd9081b45dbabe05455765f787c53c612dd9ccf0f453b81a58668d` |
| worker_edit | `0be30e277e68119d9f2903908c20a1ddf4e80713923ca17435916254544c8e38` | `1187358466bc09c67663476fc240de14a288a2c8f486af5fe829e4608d88c5ae` |
| native_edit | `522b0a01c23ec934b3c68648deeb18a56ba0cffe35e6555aec860e4707dcf1da` | `d4526014b1aa3906fbc568e06ed5ebdcbaee626ea0218c0174a558686c12759e` |
| dialog | `cb35b27bf9d79ce116e2e84c6f5a9269e974a4a003e2fee09b957c32837e6099` | `a872673734402731669a3f4e61b22cdbad1b4540285f942b68b97ad326dd6221` |
| sheet | `959451611ad14dcada3e366b517bc0ef9de2a3d2d05559a2a21c11d4fb5095ee` | `7f0d38498539ecfccd2edd471b8da3705ad519c8dff281b8ecbd5a580ec736ac` |
| grip | `26169fba3db7c5f880812637d8f1e8d1d03f05cb3792c0bfb3a7bc3b408facf3` | `898d3530836dce2599b89b100acecef89e8166932a70bce700b4bcc04f5be766` |
| update_source | `2d9b347f7f6ed996c52e7b845108a0c598397bc663da05249f686b0d465b516f` | `7a0aee453e558b31b2efdb20cd5b0f3fbdd9a6d842095c4149af43515dff4456` |

The bounded C-check static audit maps78 work/callback/publication sites,276 named
test references and18 unchanged source/header/test files pinned to `06ffc4513`.
No proven P1/P2 defect was established. `W11-C-check-static-audit.md` SHA256 is
`c5a47554d38c02b9324ba6d4d34026871993285cb8a2b1014c5762b83ea15513`.
It is not runtime qualification or closure of A-C: a complete allocation-failure
sweep, independently summed whole-checker accounting oracle and complete analytic
review remain explicit coverage limits. Existing aggregate exact-cap replay and
callback-throw tests must not be relabeled independent whole-path proofs.

W13 requalifies all six existing geometry TUs through the complete CMake test
executable on product `558c3bba2` (roadmap HEAD `7fd2265a4`, then documentation-only
edits). Both complete test executables rebuild successfully. Exact
`[lc-offsetcurve][geometry]` discovery matches372 declarations in Release/ASan:
primary geometry261, intersections51, contact matching18, path search24,
cache contacts12 and topology budget6. Each run executes every declared identity
plus72 named section entries:444 unique JUnit cases and1576253 assertions,
zero errors/failures/skips. The identity-set check distinguishes declarations
from section cases; it does not misreport444 independent TEST_CASE declarations.
ASan remains `detect_leaks=0`. This runtime checkpoint does not close the static
A/B analytic review, allocation-failure sweep or independent whole-path accounting
oracle limitations above, or substitute for final source/UI/file qualification.

| W13 Artifact | SHA256 |
|---|---|
| `W13-build-full-tests-release.log` | `98b3608038d13ff6b210f8c330fa663cfd82597ad4dae6085890d47649e8f1f1` |
| `W13-build-full-tests-asan.log` | `fb3fe076c4f95c5b89bbea14775730e1b8ff3812eb06e0d9e67d640f338b1d46` |
| `W13-geometry-discovery-release.xml` | `124be6c26d982bc44aafad084ede851457a83796298f124f5a1d55e3eb8a559c` |
| `W13-geometry-discovery-asan.xml` | `b9d1c1a7c38e04b41388b0151c40cbcba066d48ff3fce2efa78ac990b357af8b` |
| `W13-qualified-geometry-release.xml` | `859bc28ee3e30ec16d2c7e308ab8bbe1da766bcf90b6475ef6f1a253e1dfcaaa` |
| `W13-qualified-geometry-asan.xml` | `61ec8ecda6ee0105c1278872bb1e45ec97c867561cfb4491db3c7044845bdb09` |

The read-only import caller design `W13-import-worker-design.md` has SHA256
`95c7c7f21cb5951d1226a4d8d8b6145b80d42a93a144a51703cca1d1c3b11060`.
It identifies actual raw SPLINE callbacks, recursive XREF clone/layer transfer,
all INSERT finalizers, whole-graphic storage/MDI publication, library/thumbnail
loads and direct plugin/pattern/sample escapes. It is supporting implementation
design, not another active roadmap or runtime PASS. Capture/telemetry foundation,
sequential owned admission and full publication/caller routing remain unimplemented.
Ordinary clipboard Paste is an in-memory copy, not an invented DXF loader; keep
it unchanged and prevent unpublished imports from becoming clipboard sources.
Do not add an unrelated plugin API or use nested event loops to simulate async
completion. Existing parser/ordinary finalization latency and bounded client
destructor/reap behavior remain separately stated limitations, not worker success.

`3e440199097bc0ce2778f08262af68d752cbe456` qualifies prepared trim consumption
(W14), without changing the existing synchronous numeric path. The shared wrapper
accepts only private joined completions, binds complete original request bytes,
raw caches/cutters, picks, all14 remaining limits and the caller-validated publication
epoch, and preserves the existing attribute/output/final-seal/atomic handoff path.
One paired op3 serves conic Trim Two; nonconic Trim Two consumes two op2 jobs using
the exact shared debited remainder. Missing, stale or mismatched receipts never
cause parent-side numeric fallback. Original pre-dispatch source/context seals and
dispatch reservation accounting still belong to the future asynchronous action.

Review reproduced a moved-from completion bug: after moving geometry away, the old
receipt could still materialize a successful empty copy series or ordinary result.
Custom noexcept moves now revoke the old joined state. All materializers and trim
payload consumption reject revoked owners; the moved-to owner retains the valid
receipt. `isJoined()` reports ownership/lifetime, not a geometric certificate.
The preserved red `W14-moved-red.xml` has five failed assertions (SHA256
`5a507c177df3511e0f8c6d10efc5aece11036cd051970b268a1eeaf8bf3f58b1`).

Release and ASan both pass the full98-case trim micro target (55 kernel +35 prior
action +8 new prepared cases),14-case client target and standalone trim failure
process. Trim assertions are59345/59964 and client assertions14813/17714 (R/A);
polling counts vary with scheduling. The standalone process verifies1616 observed
ordinary-new staging failures,20 late cancellations, four conic failures and four
conic cancellations, plus its three unchanged-input positive controls. This is not
an allocation sweep of the entire public C checker. ASan is `detect_leaks=0`.
Additional retained Release passes are kernel14/2572, source preparation81/83385
and fault7/28348. Strict XML has zero errors/failures/skips. No caps/deadlines or
watchdogs changed. One earlier fault run hit the unchanged aggregate24000ms watchdog
during concurrent external compilation; preserve its red receipt rather than
relabeling it PASS. A subsequent unchanged full run passed in20.664s. A separate
new-fixture red was corrected by actually replacing an insert-only user variable
(delete then set); final native-seal refusal assertions were not relaxed.

| W14 Artifact | SHA256 |
|---|---|
| `W14-qualified-client-release.xml` | `fedc3bb8e928ffb71c4f71095a5108d58eae7978276726d17726173243efe468` |
| `W14-qualified-client-asan.xml` | `17ef6ccc1309417a8b210919d8b53e134dc8cc46cbe403565702d90cebba710c` |
| `W14-qualified-trim-release.xml` | `ae23846a160c6ce894e4cc66672c6d4229c8f54d8b70af8f3f023879c6d5cab3` |
| `W14-qualified-trim-asan.xml` | `9f4551046ec354b839987bf60f7629e75f2208a4796ffcc06abd20b902a33c93` |
| `W14-trim_failure-release.log` / `W14-trim_failure-asan.log` | `4f9da7d2c23debf6b70aabf992d6eeb18499ed18f73612fbfa2ac1e83ddde5e6` (identical bytes) |
| `W14-qualified-kernel-release.xml` | `f6460ea12a606b9dbbf1ab8dbdddae229190f041fb24113aeb8d66e4a4e5e867` |
| `W14-qualified-source_preparation-release.xml` | `2950a0037072366fbb3e08b93dae5062cb24be31c0a2c4c7cfd99305070ae5ec` |
| `W14-qualified-fault-release.xml` | `72f78915b67432b12afa5df76bb56d96b14b1d4c147740a5a4d712c7da80623f` |
| `W14-fault-watchdog-red-release.xml` | `552cf9782dff93e30ad7ca6903c9a7d9967a722b24091ead891c9cc2ec74fa47` |
| `W14-trim-fixture-red.xml` | `2e88ec54cded58890d7c92280943e271765ba4c21789ef58dbee8dcd74d29bad` |

W14 does not implement asynchronous Trim action routing or close remaining creation,
preview-cache, import, packaging, current-source full GUI/file or final R gates.
The later user-requested upstream merge must have its own source pin and verification;
pre-merge receipts above do not qualify a different merged tree.

User-requested upstream update (W15):
`c2100eaff697f92937f170df087944964434dc14` merges freshly fetched
`origin/master` at `fb45503b8c10541958983230c0e5a633e83cb215` into
`7dbec029dad15bbb4bdfed5b0b0f5130206724d1`, preserving both parents and the
feature history. Immediately before merging there were398 feature-only and32
upstream-only commits. The merged source tree is
`0059193937519ffa7621b2fdd9578aac9fb3ea32`; this checkpoint changes documentation
only. The upstream update was explicitly requested, not unrelated feature work.
Both other checkouts and their protected indexes remain unchanged; nothing was
pushed or copied into the dirty main checkout.

Only two files required manual conflict resolution. `CMakeLists.txt` retains both
local offset/XDATA/coverage test registrations and the upstream modifier,
language, drawing-options and linetype tests. `LC_CopyUtils::copy()` retains the
local successful-copy result list and refusal accounting, then imports the
upstream referenced linetypes after `endCopy()`. Upstream copy/paste, document
linetype, shortcut/translation, font/license and packaging changes are retained;
the merge itself changes no offset-worker sources. No additional integration
code fixes were needed.

CMake Release successfully builds `librecad`, `librecad_tests`,
`librecad_offset_trim_tests`, `librecad_offset_worker_client_tests`,
`librecad_offset_source_preparation_tests` and `librecad_offset_test_helper`.
Fresh offscreen discovery/execution on that exact merged tree passes:

| Suite / selector | TEST_CASE declarations | JUnit cases | Assertions |
|---|---|---|---|
| copy: `[copy]~[.slow]` | 78 | 83 | 4597 |
| linetype: `[linetype]` | 109 | 133 | 3846 |
| geometry: `[lc-offsetcurve][geometry]` | 372 | 444 | 1576253 |
| clipboard: `[lc-offsetcurve][g-fixtures][clipboard]` | 1 | 1 | 1054 |
| trim: full micro target | 98 | 98 | 50254 |
| client: full micro target | 14 | 14 | 7715 |
| source_preparation: full micro target | 75 | 81 | 83385 |

Every discovered identity executes, there are no duplicate identities or missing
tests, and additional JUnit cases are named SECTION entries under discovered
declarations. In particular, source preparation has75 declarations plus six
section entries, not81 independent declarations. Every report has zero errors,
failures and skips; overlapping selectors are not summed as unique coverage.
Worker polling assertion totals remain scheduling-dependent.
`W15-<suite>-discovery-release.xml` and `W15-qualified-<suite>-release.xml` are
preserved in the evidence root. `W15-merge-evidence.sha256` binds all14 XML files
and the successful build log; its SHA256 is
`a2f12aa85d8cbd012e428c5b5ba412b5d3aab846a3a4a1670050fe4e8eb06e5a`.
`W15-merge-build-release.log` SHA256 is
`34e682d2bfabf1e653d1cd51d0ef4b7d51f7bf4d6421e287b2869d3d557eb0a1`.
Fresh H6 roadmap parser/inventory checks pass6/6 with no production mismatch or
pending declaration, preserving71 original identities and93 executable obligations.
These are document/tool checks, not completion of those runtime obligations.

This is selected Release/offscreen merge qualification, not execution of the
entire application test suite, slow copy corpus, or complete file/version matrix.
ASan, qmake6, deployed packaging and visible GUI acceptance were not rerun on the
merged tree; earlier receipts retain their original source limits. The merge
does not close native caller routing, source reconciliation, A-C review or final R.

PR #3193 review follow-ups (execute before the remaining creation/cache work):

- [x] Publication allocation safety: `d320a0eea` stages native property-sheet
  entity/selection/history/bounds changes and a complete undo cycle before final
  input seals. Commit performs no C++ allocation or callback; ownership transfers
  only after commit. Post-commit observer failure cannot erase the undo cycle.
  Generic modification/undo callers outside this native property publisher are
  unchanged. Listener registration now rolls back partial allocation failure.
- [x] Async source-action regressions: `ed9d23944` shares the existing test-only
  Update waiter with the registered R2018 DXF/DWG persistence and hidden-context
  tests. It retains3500ms per-peer and shared30000ms bounds, cancellation on
  assertion unwind, and every existing persistence/refusal/undo assertion.
  No production event loop or deadline changes.
- [x] Invalid-viewport grip regression: `0df6469db` retains invalid-coordinate
  and viewport-factor rejection while the cancelled-native-gesture release
  latch remains set. A redraw can retire the active grip but cannot route its
  next zero-factor move into the ordinary snapper. Matching release and a new
  drag recover; ordinary neutral snapping introduced upstream is unchanged.
- [x] Validation recipe completeness: `691ae7469` adds `[offsetcurve]` discovery/
  execution, XML identity/nonzero/SECTION checks and explicit skipped-row output.
  It inventories implemented worker/caller targets, opt-in CTest registration,
  all nine worker entries, actual helper/configuration paths and separate fault
  processes. It removes the obsolete claim that lifecycle targets await W1.

P1 fresh Release/ASan qualification on `d320a0eea`: the standalone fault target
passes35 fresh preparation allocation ordinals and113 redo-tail ordinals, with
zero C++ `new` calls during publication; former failure ordinals1-8, stale
selection, throwing observers, relative-zero undo and locked-layer selection
also pass. Both builds emit the same diagnostic. The initial census of100
allocations exceeded the existing64 guard; replacing per-child set allocations
with a sorted pointer vector reduced it to35 without raising guards or production
limits. A real undefined-native attribute-only sheet regression was fixed by
removing an unnecessary editable-definition prerequisite, not weakening tests.
The property-sheet micro target passes41 declarations/53 JUnit cases/9367
assertions in each build. Release `[tracked][listener]` passes35 declarations/
35 cases/150 assertions. Every report has zero errors, failures or skips.
ASan uses `detect_leaks=0`; this is not a Qt-private allocation or leak-free claim.
The Release app and main tests also build. Broader suites await the remaining
review fixes; these results do not close creation/cache/trim/import, final R,
qmake6/deployment, visible UI or full interoperability acceptance.

Evidence is in the build/evidence root above. The earlier red census and sheet
reports remain retained as red evidence. Final artifact SHA256 values:

| PR3193 P1 artifact | SHA256 |
|---|---|
| `PR3193-P1-qualified-fault-release.log` / `PR3193-P1-qualified-fault-asan.log` | `ca8ccb96179e1baed01a1c346e7d7b0444d7d7156870251b77e323590d1d57ed` (identical bytes) |
| `PR3193-P1-qualified-sheet-release.xml` | `fc8927d8a4225b4ad9138862295a5777dfb1b95a5484db149af02e02b68297de` |
| `PR3193-P1-listener-release.xml` | `f9058d314bad69c565ead06c4e0332e54416a7a764f73920ee693cfa3ad439b2` |
| `PR3193-P1-qualified-sheet-asan.xml` | `1aee2cd131c5c037a63507bc79ff4403e33ecbea30143060f1410ac70002e7f3` |

P2 fresh Release/ASan qualification on `ed9d23944`: the union of `[lc-offsetcurve]`
with `[i-update]`, `[i-restore]` or `[i-ui]` discovers81 declarations (34 Update,
35 Restore,12 UI) and executes83 JUnit cases in both builds, with14548/15346
assertions respectively. Release's standalone Update micro target also passes
34 declarations/36 cases/4930 assertions. Every discovered identity executes;
additional cases are named SECTION entries; every report has zero errors,
failures and skips. Worker polling assertion totals are scheduling-dependent.
ASan again uses `detect_leaks=0`. The main test executables and real helper build
in both configurations. Final artifacts in the same evidence root:

| PR3193 P2 artifact | SHA256 |
|---|---|
| `PR3193-P2-qualified-release.xml` | `4279e31d8a89602b2520f31a00fa0e5e6d85a4f9d1479a8a21d834cbe2773da1` |
| `PR3193-P2-qualified-asan.xml` | `b4ca23ee8217270ef96ea81cf17f8cce270b585d21ecb1b3b777f6d58926408c` |
| `PR3193-P2-qualified-update-release.xml` | `4edda5cb5360008682bf4096fa54ff74f12a90e3c12e030cfe5e266268e0de20` |

P3 fresh Release/ASan qualification on `0df6469db`: the union of
`[lc-offsetcurve][h-drag]`, `[snap][drag][issue3127]` and `[tracked][listener]`
discovers/executes81 declarations/cases in each build (44 native grip, two
upstream ordinary-snap and35 listener cases). Reports have126233/144585
assertions respectively and zero errors, failures or skips. Discovered/executed
identities match exactly. The strengthened regression checks cleanup immediately
after redraw and invalid motion, unchanged geometry/metadata/selection/history,
matching release and a subsequent successful gesture start. App/main/grip/helper
Release targets and main/grip/helper ASan targets build; ASan uses `detect_leaks=0`.
The pre-fix report retains its one stale-overlay assertion failure.

| PR3193 P3 artifact | SHA256 |
|---|---|
| `PR3193-P3-qualified-release.xml` | `9fcd85c5fd36d1b736c04c032d77fe6c12c8b426c83011807ecf7645f61061f5` |
| `PR3193-P3-qualified-asan.xml` | `dc29f269f0f7f544b303d53942e912ef6fe64ed8c0850c2e6356420207802a5e` |
| `PR3193-P3-viewport-red.xml` (red, not acceptance) | `065bd59e45eccab104b65b052a456d631913b0472d51900831e8ebfa66206ede` |

P4 fresh Release qualification for recipe `691ae7469`, using unchanged product
code from `0df6469db` (built at `6a1843f43` plus the recipe-only diff):

| Main selector | Declarations | JUnit cases | Assertions | Skips |
|---|---|---|---|---|
| `[lc-offsetcurve]` | 936 | 1241 | 3932765 | 1 |
| `[offsetcurve]` | 168 | 180 | 131414 | 0 |
| `[curve-offset]` | 218 | 274 | 1021409 | 0 |
| `[entity-coverage]` | 15 | 15 | 1680 | 0 |

All reports have zero errors/failures. Every discovered identity executes; named
SECTION entries account for extra JUnit cases. The168 `[offsetcurve]` declarations
are additional to all other selectors: sheet41, dialog56, enable17, UI helpers4,
plugin10 and modification20+20. No tests were retagged, removed or rebaselined.
The sole skipped case is `legacy DXF native dispatch writes visible cache
primitives when the real legacy exporter is supplied`: this build has no
`RS_FilterDXF`, `dl_dxf.h` or `FormatDXFOLD`. No surrogate writer or legacy-export
PASS is claimed. Ordinary/source support assertions remain covered by the real
`RS_FilterDXFRW` paths; full file/version interoperability remains open.

The app, helper/test helper, all nine worker/edit targets and all seven caller
micro targets build. Opt-in CTest discovers exactly the nine entries listed in5.3
and passes9/9 in35.84s, including separate ordinary fallback and isolated fault/
geometry-timeout execution. Replaying those exact nine CTest commands with Catch
discovery/JUnit reports also passes with no missing, duplicate, undeclared or
skipped cases. Separate edit/caller micro discovery/execution passes:

| Micro target suffix | Declarations | JUnit cases | Assertions |
|---|---|---|---|
| worker_edit | 15 | 15 | 101553 |
| native_edit | 29 | 29 | 15311 |
| properties_dialog | 56 | 56 | 25937 |
| properties_sheet | 41 | 53 | 9367 |
| grip | 44 | 44 | 86128 |
| update_source | 34 | 36 | 3406 |
| trim | 98 | 98 | 49882 |
| source_preparation | 75 | 81 | 83385 |

All micro reports have zero errors/failures/skips and complete identity coverage.
Scheduling-dependent worker polling assertions are not fixed expected totals;
overlapping main/micro selectors are not summed as unique coverage. Fresh standalone
publication allocation checks again pass35 preparation/113 redo-tail failures,
zero publication C++ `new` calls and all recovery cases. The separately serialized
trim allocation process passes4 directions,1616 staging allocation failures,
20 cancellation points and4+4 conic staging failure/cancellation cases. No global
allocation override was linked into Catch or the app. Both recipe Bash blocks
pass syntax checks; H6 parser/inventory passes6/6 with unchanged71 original/93
executable obligations, zero mismatch and zero pending declaration. These remain
document checks, not completion of all93 runtime obligations.

`PR3193-P4-evidence.sha256` in the evidence root binds74 current P4 XML, JSON and
log files, SHA256 `59eb22a0a48b7f0450cddc2263c1acaab3b3c4aa3e245b32e8cae302c5b55daf`.
The final standalone trim receipt is
`PR3193-P4-qualified-trim-fault-serial-release.log`; the earlier overlapping
process run is retained but is not the serialized acceptance receipt. Focused
ASan results are the P1-P3 receipts above, not a full ASan/main-suite or leak-free
claim. qmake6, deployment, visible UI, the full application suite and external
writer interoperability were not freshly qualified by this review follow-up.

All four PR review findings are closed within their stated scope. PR #3193 also
now documents ordinary ellipse/parabola/bounded-hyperbola/spline/spline-points
support versus ellipse-only editable definitions. Keep the PR draft: this does
not complete #2961 or the entire roadmap. The subsequent W16-W19 slices below
completed local preview-cache portability and creation/repeat routing; the
remaining 4.2 caller/import work and final acceptance are still open.

Creation/repeat continuation W16:
`7bfe70ac2` implements pointer-free, owned ordinary preview-cache snapshots.
Entries retain side/magnitude, original output budget, status, validation/error
metadata and ordinary LINE/raw normalized spline values. Restore binds them to
the helper-owned source, not a transported pointer. Zero new producer/sample/pair
effort on a hit and the existing budget-sensitive refusal policy are preserved.
An external-entry count preserves the action-wide256-entry clearing threshold,
including overwrite at capacity; revision-checked merge rejects stale updates
and propagates a helper's global clear to other sources. Cache node allocation
precedes clearing/overwriting. Snapshot scanning caps record/value/array copies;
it refuses native objects, malformed layouts and oversized latent arrays.
These are ordinary cache values, never native admission authority.

Fresh Release/ASan `librecad_offset_source_preparation_tests` discovery/execution
on W16 passes79 declarations/85 JUnit cases/83452 assertions in each configuration,
zero errors/failures/skips, complete identity coverage and only named SECTION
extras. Four new cases cover real source rebinding/zero-effort reuse, refusal
budget/side/distance rules, cross-source eviction and stale/malformed updates.
Both build systems register the new header; a fresh qmake6 app build is not
claimed. ASan remains `detect_leaks=0`. Evidence root is unchanged:

| W16 artifact | SHA256 |
|---|---|
| `W16-qualified-source-release.xml` | `bd1d8db09a9a59799a9f16a8dbf28cd8e02cf5901c6e1c90b74f91aa496c3186` |
| `W16-qualified-source-asan.xml` | `ad71e393aa98634aaa32b4df90be9679b2f556930193aeb5d5fa00c130f3212f` |

W16 completed the value/lifetime prerequisite only. The original continuation
split below is now implemented/locally qualified through W19b; retain its acceptance
contracts, with the ledgers and current remaining list below authoritative:

1. W17, cached Produce transport: append a new operation to protocol4, preserving
   existing operation0-5 body layouts. Carry a bounded ordinary snapshot into the
   existing shared Produce preparation and return its updated snapshot. Preserve
   side-once, per-copy effort/output limits, diagnostic prefixes and the distinct
   native-rebase/stored-cache fallback. Only a joined private completion may
   update the parent's cache. Admit all counts/scalars/geometry before native
   copying; raw cache values cannot mint native entities or bypass C. Test fresh
   real helper processes, opposite sides, hits under smaller effort/output budgets,
   refusal growth rules, eviction, stale merge, malformed/truncated frames and
   ordinary/native fallback parity. Oversized cache transport is an explicit
   refusal, never silent regeneration with fresh numeric limits.
2. W18, ordered preparation: retain owned per-source numeric results while the
   batch advances through worker and ordinary sources. Preserve original source
   order, deduplication, polyline segment runs, request-wide remaining display
   allowance, copy grouping and partial-series diagnostics. Rebuilding cheap
   dispatch must not rerun numeric preparation. Add allocation-safe creation
   publication at the existing prepared-model boundary; preserve the strict
   native replacement entry used by the property sheet. Stage complete entity,
   selection, undo/redo and relative-zero changes before ownership handoff.
3. W19, creation/repeat GUI continuations: integrate that preparation with Modify
   Offset preview/commit for eligible ellipses and native sources, without nested
   GUI event loops or changing ordinary-only tools. Use owned action continuations
   and fixed helper discovery. Seal source geometry/metadata/layer, document/view,
   selection, undo epoch, UCS, options and target attributes through final
   publication; cancel/reap superseded preview, Escape, suspension, context death,
   stale source and viewport changes. Preserve bounded detail previews, one-step
   undo/redo, current/source attribute policy, plain fallbacks and retry after
   failure. Test heartbeat, preview/cache parity, repeated offsets and mixed
   source batches plus allocation/cancellation/late-reply failure safety. Update
   existing async-facing tests without weakening their geometry/undo assertions.

Test/commit each slice, immediately checkpoint this file and push PR #3193.

Cached transport continuation W17:
`0552876f9` implements CachedProduce6 in the existing protocol4 helper, request/
reply admission and private completion. Source preparation is shared, not a second
generator: side resolves once and per-copy cache/rebase/adoption diagnostics are
retained. Sorted reuse ordinals bind to actual side/magnitude entries and remaining
budgets; output usage may include cached usage but may not invent producer runs or
sample/pair effort. Updated revisions bind to actual keep calls and global clears.
Native zero-sum repeat uses the distinct stored-cache fallback, while an altered
ordinary cached line cannot bypass ellipse adoption C. Only joined completions
can merge a current parent's cache; moved-from and stale-revision merges refuse.
All raw collections/scalars are bounded/scanned before native geometry copying;
combined serialized item/frame caps are unchanged. Oversized transport refuses,
not silently regenerates. Existing operation0-5 bodies/envelope remain unchanged.

Fresh full micro discovery/execution in Release and ASan passes with zero errors,
failures/skips, complete identities and only named SECTION extras:

| W17 suite | Declarations | JUnit cases | Assertions Release / ASan |
|---|---|---|---|
| request | 13 | 13 | 597166 / 597166 |
| result | 26 | 26 | 467578 / 467578 |
| kernel | 18 | 18 | 2728 / 2728 |
| real process | 4 | 4 | 2765 / 3215 |
| client | 15 | 15 | 7938 / 11171 |
| fault | 7 | 7 | 19583 / 25097 |
| geometry timeout | 1 | 1 | 1288 / 2272 |
| worker edit | 15 | 15 | 101553 / 101553 |
| source preparation | 79 | 85 | 83452 / 83452 |

Eight new cases cover request/reply corruption, real cubic value round trips,
fresh-process zero-effort reuse, opposite-side misses, refusal growth, cross-source
eviction, mandatory native C, zero-sum fallback and private/stale/moved completion
behavior. Existing generic fault/timeout tests also pass; these are not newly
cached-operation-specific crash proofs. Source-free local helper executables and
real client discovery build in both configurations; qmake6 app/deployment and
visible GUI acceptance are not claimed. ASan uses `detect_leaks=0`; polling totals
remain scheduling-dependent. Evidence root is unchanged; `W17-evidence.sha256`
binds54 qualified XML/log files and two final build logs, SHA256
`d83ba2aa4bbf74df5a21c1e48a8fc5327c3234b6f65268a2fa6e74b8ee6c145a`.
Development/probe reports are not final qualification.

W16/W17 completed preview-cache value/process portability. The subsequent W18/W19
slices below completed local ordered preparation/publication and GUI ownership
and cancellation. Retain ordinary-only behavior and all existing
option/geometry/selection/undo assertions during remaining integration.

Ordered preparation continuation W18a:
`c213a6c2a` adds `LC_OffsetBatchPreparation` around the existing modification
dispatcher. Owned per-source numeric rows survive asynchronous turns. Advancing
rebuilds only bounded dispatch/grouping and never regenerates an accepted row.
Ellipse/native rows wait for exact token/body-bound private completions; ordinary
prefixes remain private. Each helper receives the actual request allowance left
after earlier parent-materialized output. The final batch retains original source
order, deduplication, polyline runs/whole-parent aliases, copy groups, failure and
short-series diagnostics and source-removal policy. Nothing is published by this
class. Its final ownership release is allocation-free and limited to the staged
added set. The synchronous API still uses the same dispatcher with no continuation.
The caller must own source/context seals across turns; W19 will supply owned
numeric source graphs so live-document death cannot interrupt numeric staging.

Full Release/ASan micro discovery/execution passes with complete identities,
zero errors/failures/skips and only named SECTION extras:

| W18a suite | Declarations | JUnit cases | Assertions Release / ASan |
|---|---|---|---|
| source preparation | 82 | 88 | 83527 / 83527 |
| client | 16 | 16 | 8122 / 12666 |
| kernel | 18 | 18 | 2728 / 2728 |
| real process | 4 | 4 | 2756 / 3162 |

Four new cases verify ordinary no-rerun ownership, request exhaustion before
dispatch, polyline aliases/whole-source parity, and mixed ordered real-helper
completion with exact request-token rejection. Header registration is present in
CMake/qmake6; no fresh qmake6 application/deployment/visible claim. ASan uses
`detect_leaks=0`; process polling counts vary. `W18a-evidence.sha256` binds24
qualified XML/log files plus two final build logs, SHA256
`8d16113e9cf65b8d61a24081ae6c15c006dc616a3168a1301570519fa4d8a8f9`.
The initial test-probe signature build and zero-match pre-build reports are not
qualification; the final reports above discover and execute every named case.
The W17 ledger's new-case count is corrected to eight, matching its suite table.

W18 is complete at its preparation/publication boundary, not at the GUI caller.
W18b `328422713` adds `prepareCreation` to the existing prepared model transaction,
preserving the strict property replacement entry and original allocation guards.
It stages unequal add/remove lists, source/child selection, complete undo/redo,
relative zero and bounds before allocation-free ownership handoff. Parent-list
seals precede retained child reads, including obsolete redo subtrees. Enclosing
graphic destruction invalidates a transaction for a surviving block document.

Fresh Release/ASan discovery and execution in the same receipt root:
creation publication7 declarations/7 JUnit cases/100 assertions; property sheet41
declarations/53 JUnit cases/9367 assertions. All identities execute, zero failures,
errors or skips. Both standalone fault processes exit0 with35 normal and113 redo
replacement allocation failures (unchanged64/128 guards),38 add-only and42
source-removal creation failures; actual creation/replacement commit allocates
zero C++ new/new[]. Every refused preparation retains all owners and document
state; retry has one undo/redo cycle. Qt malloc/aligned-new injection is not
claimed. ASan uses `detect_leaks=0`.

| W18b qualified report | SHA256 |
| --- | --- |
| creation Release | `66683e7fc76157c1bac9f617e39be09442a3e7023069cab986f59e63672b9660` |
| creation ASan | `7495a80c7b5d3966f236319a30fdeb85be5a278350595d8b071d467f7fdbfd93` |
| sheet Release | `21aa963bf76b4461cd2fa3b3a32d76f03b6e8219c6b0a479f9f3ca5a9c7f5b44` |
| sheet ASan | `daf36cccb8428c4adcbd21e3f2972dd4db664a11a92444f4f84c72794f6e8a4f` |
| fault log, both builds | `0d18815831628849d5ff3620226586105c4aa5a203d16ce34bbaae15912e7e05` |

Files are `W18b-qualified-{creation,sheet}-{release,asan}.xml` and
`W18b-qualified-fault-{release,asan}.log`. Initial probe/build errors are retained
but are not acceptance receipts. Next W19: bounded owned source graphs and seals,
then Modify Offset preview/commit continuations; test/commit/checkpoint each slice.

W19a owned inputs `fd71915ab`: `LC_OffsetCreationSources` freezes bounded built-in
geometry, polyline/native parent-child identity, metadata values and independent
layer copies before ordered numeric work. It resolves live top-level/child
membership before retained-address reads and promotes native display children
once. Stored geometry, active metadata payloads, provenance, selected flags,
layer registry/visibility/lock and parent-list seals reject changed inputs.
Qualified built-in clones avoid root callbacks; child clones are admitted only
for owned built-in line/arc leaves. Oversized arrays/metadata, borrowed graphs,
unknown child implementations and stale sources refuse before cloning. This is
bounded creation input admission, not a new supported entity type or C proof.
Ordinary-only tool routing remains unchanged at this checkpoint.

Fresh Release/ASan `librecad_offset_creation_publication_tests` discovers/executes
11/11 cases,139 assertions, zero failures/errors/skips, including all7 W18b cases
and4 owned-source cases. Tests destroy the live document and continue reading
the owned numeric graph, reject geometry/active-XDATA/layer/provenance mutations,
preserve polyline aliases, localize only private output layers and refuse stale
or oversized inputs. `W19a-qualified-release.xml` SHA256
`294bbd3384b9862800989e75aebc4a995dd9c044682891ace81fc16b2f72adaa`;
ASan `W19a-qualified-asan.xml`
`a7d56b906df4e76d7b61b29fc361a0647438f95f89d5379f7f029b7e24b18dd3`.
ASan leak detection remains disabled. Source snapshots alone did not close GUI
routing or cancellation; W19b below supplies that caller integration. Final R
is still separate.

W19b creation/repeat GUI routing `f035d983e`:
Modify Offset now prepares eligible ellipse/native previews and committed copy
series through owned, ordered continuations and joined CachedProduce6 jobs.
There is no nested production event loop or synchronous geometry retry after
helper failure. Ordinary-only selections keep the existing dispatcher. Mixed
batches preserve order, grouping, source/target attributes, per-copy provenance,
partial failures, remaining output limits and the native rebase versus distinct
stored-cache fallback. Preview values remain ordinary; committed native outputs
still require the child checks. A click retains eligible hover-cache values.

Source geometry/active metadata/layers, document/view/context, selection, undo
epoch, options, target attributes, UCS, pan/zoom and relative zero are sealed
through allocation-safe model publication. Supersession, Escape, suspension,
right-click, stale inputs and context death cancel/reap without publishing a
prefix. Owned input copies survive source-document death; qualified built-in
descendant clones and latent spline tables/tails are bounded before copying.
Unknown/borrowed/oversized source graphs refuse explicitly, never regenerate
with fresh limits in the GUI. Membership admission is indexed/bounded rather
than a full document scan per source. Detailed/bounding-box previews transfer
ownership in dispatch order; two-click guides and one-cycle relative-zero undo
are preserved. Output ownership transfers before any publication notification.

Fresh Release application/full-test/helper builds and Release/ASan full-test,
helper and micro builds pass. Offscreen qualification, with complete discovered
identity execution and zero errors/failures/skips in these focused suites:

| W19b caller suite | Declarations | JUnit cases | Assertions, Release / ASan |
| --- | ---: | ---: | ---: |
| Creation action, options factory and existing offset matrix | 85 | 132 | 90217 / 90217 |
| Creation publication and owned-source admission | 13 | 13 | 191 / 191 |
| Complete source preparation | 82 | 88 | 83527 / 83527 |

The11 new GUI cases cover cache-hit/no-producer commit, native repeat/zero-sum
fallback, mixed copies/failed selection, current/source attributes, stale input/
option/selection/layer/UCS/pan/relative-zero rejection, cancellation, actual lowered-
deadline process timeout and later recovery, destruction, postcommit observer
death, two-click geometry and per-copy DWG identity policy. The existing hover
test now requires one reused copy/zero producer calls, preserving its native
geometry/check assertions. Production deadlines and original allocation guards
are unchanged. Both standalone fault processes exit0 with35 normal/113 redo and
38 add-only/42 source-removal failures; actual publication allocates zero C++
new/new[]. Qt malloc/aligned-new injection and a full GUI allocation census are
not claimed. ASan uses `detect_leaks=0`, not a leak-free claim.

Fresh serial broad Release runs also pass:

| Selector | Declarations | JUnit cases | Assertions | Skips |
| --- | ---: | ---: | ---: | ---: |
| `[lc-offsetcurve]` | 960 | 1267 | 3926535 | 1 |
| `[offsetcurve]` | 168 | 180 | 131450 | 0 |
| `[curve-offset]` | 218 | 274 | 1021413 | 0 |
| `[entity-coverage]` | 15 | 15 | 1680 | 0 |

The single legacy `RS_FilterDXF`/`dl_dxf.h`/`FormatDXFOLD` dispatch skip remains
NOT QUALIFIED; no surrogate writer or skipped PASS is substituted. Overlapping
selectors are not summed. Release native-worker CTest discovers/passes exactly
9/9 named entries. All9 separate ASan worker micro suites (wire, request, result,
kernel, process, client, edit, fault, geometry-timeout) discover/execute completely
with zero errors/failures/skips. This is focused ASan, not a full-ASan-suite claim.

In the same receipt root, acceptance uses `W19b-final3-{action,source}-*.xml`,
`W19b-final2-publication-*.xml`, `W19b-final3-native-*-release.xml`, serial
`W19b-final4-{ui-source,adjacent,types}-*.xml`, `W19b-final3-*-asan.xml` worker
reports, `W19b-final4-native-ctest-release.xml`, the two final3 publication-fault
logs and build4/build5 logs. `W19b-evidence.sha256` binds46 unique artifacts
(39 XML reports and7 build/fault/discovery logs), SHA256
`3b4db125beb436985e7075f37d560dbf00903cd65f192d6d08603041eff45a4c`;
all entries verify. Earlier probes, obsolete-cache expectation failures,
incorrect initial native-provenance assertions and overlapping preliminary
broad runs are retained, not acceptance evidence. Both protected indexes and
the main checkout HEAD retain their recorded identities.

W20 Trim request preparation `179a61654` adds a bounded geometry-only request
builder for the existing joined Trim consumer. It selects the same op2/op3
operand ordering and picks as the synchronous path, lowers the initial limits,
and derives a second nonconic Trim Two request only from the exact retained
first request and its successful joined completion. Request IDs must increase;
generation, source revision and undo epoch must match. This is not document
membership, source-lifetime or publication authority: the future action owns
and seals those inputs before and after every asynchronous turn. It does not
route preview/commit, retry numeric work or change the document.

Fresh Release/ASan `librecad_offset_trim_tests` discovery/execution:99/99
cases, zero failures/errors/skips, 51587/54128 assertions. Coverage includes
the existing conic operand/mode matrix with exact encoded-request parity,
real joined first and second jobs, mismatched requests, missing completions,
wrong ordering, and synchronous versus prepared output/work parity. ASan uses
`detect_leaks=0`. Reports in the existing receipt root are
`offset-W20-release.xml` SHA256
`7d13a531c5d8bc50ea24cc1a63425a593cd676b679454ca11aae6986465007f4` and
`offset-W20-asan.xml` SHA256
`8677010f25df958abff4ddd8799901c631b183dc1d177d539804380252c0e163`;
matching discovery and build logs use the same prefix. Next implement the
owned asynchronous Trim action controller, cancellation/context seals and
atomic publication; this prerequisite does not close that work.

W21 Trim worker sequencing `47661a831` retains the exact submitted requests
and joined completion owners across the existing one- or two-job query. A pending
job cannot be reissued, a mismatched completion cannot advance the sequence,
and nonconic Trim Two exposes no receipt before its second completion. An op3
conic pair or first-job refusal finishes after one job; a refusal grants no
second-job budget. The borrowed completed receipt is valid only while the owner
lives. This does not own live document entities or authorize publication: the
action must supply frozen, privately owned operands and independently seal its
document, selection, viewport, options and undo epoch through publication.

Fresh Release/ASan `librecad_offset_trim_tests` discovery/execution:103/103
cases, zero errors/failures/skips, 55918/56728 assertions. New real-helper cases
cover one-sided Trim, paired conics, nonconic two-job sequencing, rejection of
an unrelated completion without exposing a prefix, and a first-job refusal
without a second request. Existing synchronous/prepared output and work parity
remains intact. ASan uses `detect_leaks=0`. In the receipt root,
`offset-W21-final-release.xml` SHA256
`b5ee355a7f9cd6804bf3df54615ac01a6694deae78e2021a1bd2e12a4ffc96dc`;
`offset-W21-final-asan.xml` SHA256
`fb84c9f3977e226a3b15176dedf95a49c8843f76b6b0ea56f2a5d776eee4d8fe`.
Matching discovery/build logs use the same prefix. GUI callbacks, ownership
on cancellation/context death, atomic handoff and visible acceptance remain open.

W22 asynchronous Trim controller `c1710e254` owns bounded, detached built-in
operand copies and the W21 joined sequence under one elapsed deadline across all
helper jobs. It retains the receipt with its operands, reports worker status and
bounded diagnostics, suppresses callbacks on cancellation, and never retries
numeric work automatically. Calls and callbacks use the owning Qt thread. A
timeout requests cancellation immediately; the client may remain busy while it
reaps the helper. This controller neither owns a document nor publishes geometry.

Fresh Release/ASan Trim tests discover107 declarations and execute109 JUnit
cases, with56490/59434 assertions; worker-client tests discover and execute16
cases, with7722/11401 assertions. All have zero errors, failures or skips, and
discovery identities were checked against execution. The tests cover a running
helper's cancellation, lifecycle and retry, and a test-only event-loop stall that
forces the deadline; the latter is not a naturally hung geometry primitive. The
existing geometry-entry timeout lane is separate. ASan uses `detect_leaks=0`.
In the receipt root, the final JUnit SHA256 values are:

| W22 artifact | SHA256 |
| --- | --- |
| `offset-W22-owned-release.xml` | `5496c94cdb4f27755e3c7c1bff24b604766cb725afdb92968d32ecf7255cce1e` |
| `offset-W22-owned-asan.xml` | `4e7b7c89dc5ea048aa3f1ee0e047acaef0e861730d977f400855fffdcab45b56` |
| `offset-W22-owned-client-release.xml` | `8d5dbf9496b51bebf59804b1bba5fef0de008fb7283e48e7b8471020a4928993` |
| `offset-W22-owned-client-asan.xml` | `5009b95dc75fa94932d45b9ef551a6110de1c72b5837a1b237c710332bc96199` |

Matching discovery XML and build logs use the same prefixes. Asynchronous Trim
GUI routing and atomic document publication are not implemented by W22.

W23 detached Trim inputs `77d569295` extends the existing bounded creation-source
snapshot to seal a document-root target with a detached, action-owned limit. The
caller must retain and verify the exact limit object before matching it; ordinary
document-only snapshots retain their existing matching contract. Independently
owned worker copies have no parent or layer pointers, and extended-data values
are deeply copied with a bounded aggregate allowance. Neither this API nor W22
routes a GUI action or authorizes document publication.

Fresh Release/ASan source/publication tests discover16 declarations and execute17
JUnit cases with376 assertions each; Trim tests discover108 declarations and
execute110 cases with51038/54408 assertions. All have zero errors, failures or
skips and checked discovery identities. Coverage includes detached-limit changes,
replacement and admission refusal, metadata isolation, source/snapshot death,
and a real-helper detached-limit result compared with synchronous Trim geometry.
ASan uses `detect_leaks=0`. Final JUnit receipts in the same evidence root:

| W23 artifact | SHA256 |
| --- | --- |
| `offset-W23-final2-release.xml` | `8232f6ecf9047ef61e7e44d846ae83e874a8c77c09a056c9300248313d25d805` |
| `offset-W23-final2-asan.xml` | `f7d6b19885deed59a37ac20908a31befc17045bf5e0a2d02717f1485b2909c1c` |
| `offset-W23-trim-release.xml` | `7b79e2d0fc35a1651273b4f23df0c67479a1f0668a31fa7b2d9d1f6127608614` |
| `offset-W23-trim-asan.xml` | `f7658684c651a279ea663a1cefafeef73a0dc88b58e01fb085ff89c8083382c6` |

W24 native Trim commit routing `77c382799` sends native-involved action commits
through W22/W23 before opening an undo cycle. It checks document/source, picked
coordinates, selection, viewport/UCS, options and undo epoch across the callback,
and installs prepared one- or two-sided results in one publication transaction.
It preserves detached one-sided limits, ordinary-only synchronous Trim, provenance
policy and post-commit observer handling. Pending commits cancel on Escape,
right-click, suspend, action destruction or context death. No numeric fallback is
used for native commits. Refusal leaves the undo epoch and redo history unchanged.

Fresh Release/ASan Trim tests discover112 declarations and execute116 JUnit cases,
with55695/61560 assertions. Adjacent creation-action tests discover85 declarations
and execute132 cases with90217 assertions each; creation-publication tests discover16
declarations and execute17 cases with376 assertions each. Discovery identities were
checked, with zero errors/failures/skips. Coverage includes real-helper heartbeat,
stale geometry/selection/view/options/layer/extended data, cancellation and retry,
action/document destruction, mixed and detached limits, and one-step undo/redo.
ASan uses `detect_leaks=0`. Final JUnit receipts in the evidence root:

| W24 artifact | SHA256 |
| --- | --- |
| `offset-W24-final2-release.xml` | `e0f50b5148a4b2dde84683b0b1458d2a26186d1ceb4656638519df7841d5129e` |
| `offset-W24-final2-asan.xml` | `72abcb32c0b3ad3d79b8d6f6bb0fc7dd69ff181797095b448628a1b1c9f912b6` |
| `offset-W24-creation_action-release.xml` | `ebb8f62023eeede3e0d6e38d8932a9218cb1d364c9a7ed1313825bba3b10c469` |
| `offset-W24-creation_action-asan.xml` | `caa97c168ff7042674a4aaec46bfa188ece33cbc1ab5e46f04cac0571aeba6b3` |
| `offset-W24-creation_publication-release.xml` | `988978698cbd36bf16f2488f87d12eb31f05e94842c5bb904da908684d1fa073` |
| `offset-W24-creation_publication-asan.xml` | `efe3b0fa8bec5ad006f15d5fc699321d4786f46818fa4c32b3665c996f737628` |

At W24 native Trim preview was still synchronous. W25 below supersedes that
limitation; import ownership, qmake6, deployed/visible and final acceptance remain open.

W25 native Trim preview routing `b9f288033` removes the synchronous native query
from `previewTrim`, retaining ordinary-only behavior. Each native hover uses a
replaceable worker request with bounded document, source, selection, view, UCS,
options and display-setting seals. Context-target hover supports built-in nested
limits through a detached numeric copy and a live child/owner-chain seal; callers
must prove membership before reading the nested source. Custom overridden nested
geometry is refused rather than represented as a different built-in curve. Stale,
cancelled and failed jobs clear private/overlay preview content and cannot cancel a
newer request. The shared preview mouse-move wrapper now checks action lifetime
after its action-specific callback. Option callbacks are bracketed by source
snapshot and membership rechecks for both preview and commit.

Fresh Release/ASan suites have zero errors, failures or skips and checked
discovery identities. Trim discovers121 declarations and executes131 JUnit cases
(59490/67723 assertions); source/publication discovers18 and executes19 (414/414);
creation action discovers85 and executes132 (90217/90217); grip discovers44 and
executes44 (80176/91832); Update from Source discovers34 and executes36
(3016/3694). Coverage includes real-helper hover/commit parity, rapid hover
replacement, nested child/owner removal and mutation, inherited-layer changes,
worker-result refusal/retry, Escape, action/document destruction and option-callback
reentrancy. ASan uses `detect_leaks=0`. At W25 the action tests did not inject a helper
crash or a naturally hung numeric primitive; W26 below adds scoped action-level
faults, while worker-level geometry-entry evidence remains separate. Final JUnit receipts in the existing evidence root:

| W25 artifact | SHA256 |
| --- | --- |
| `offset-W25-final7-trim-release.xml` | `0e67d50ad0b0ffefbf7ccec72f44f88dd952b838e95bd58a68ab15025a16d677` |
| `offset-W25-final7-trim-asan.xml` | `1c4879915a8fb5978ebbad443212d6886e727359e368c75355b3963e46056256` |
| `offset-W25-final7-creation_publication-release.xml` | `666f5a1ddca83c3779f8a69307cfa3ebabfaa02da2961ac401f77bacc3894b41` |
| `offset-W25-final7-creation_publication-asan.xml` | `6a74431fe1f52478924c9070e5ae445f4a5e172db586a78a36c7e60feb2d87d3` |
| `offset-W25-final7-creation_action-release.xml` | `cfcd4ef027b63fff7f67633e4c831c0f26179827142bb169d147e7cc51242a2e` |
| `offset-W25-final7-creation_action-asan.xml` | `a6e88a2aac44e35ea0c47a108f7dc41516dcda009bd09d97d97fca4888d7ca35` |
| `offset-W25-final7-grip-release.xml` | `461641281c70eb4f762e3fa8f08784e54d1ee061657daa97ea992d741fb1819c` |
| `offset-W25-final7-grip-asan.xml` | `c29c1c634329c43706a8777c52a681ad476c47875e38fc2e943debc98ffe15c1` |
| `offset-W25-final7-update_source-release.xml` | `5ceeaeb2f5eb3a2c5a7c38e658d878317ad0d8ff4f0972571204efb4fdb49ada` |
| `offset-W25-final7-update_source-asan.xml` | `309d2aa4731a950334081e3fe33a23164b81f173e72bc5a71ac9cf4416e82f3a` |

W26 Trim action fault coverage `79ecceffd` uses a private one-shot test seam at
the real helper's started signal. For both preview and commit, the test kills the
action's own child or, on Unix, suspends it until a lowered test-only deadline.
It checks unchanged document/undo, action feedback where applicable, event-loop
heartbeat on timeout, actual PID disappearance after reap, and a successful retry.
No file/environment input selects this seam. The stopped process is deliberately
suspended before the request is processed, not a naturally hung geometry primitive;
the timeout lane is Unix-only and the existing geometry-entry fixture remains a
separate, non-OS-sandbox claim.

Fresh Release/ASan Trim tests discover123 declarations and execute133 JUnit cases
(60587/68218 assertions); worker-client tests discover and execute16 cases
(7368/10636); worker-fault tests discover and execute7 cases (16792/21533).
All have zero errors, failures or skips and checked discovery identities. ASan uses
`detect_leaks=0`. Final JUnit receipts in the existing evidence root:

| W26 artifact | SHA256 |
| --- | --- |
| `offset-W26-final2-trim-release.xml` | `e7485da4589923a1c1941f1473afae5f33881778629af409303d71bef3ce9d6f` |
| `offset-W26-final2-trim-asan.xml` | `2f9b11080b74f8de14e0498d4340b5bf255d3508d3be80fe52137193ea93cbbb` |
| `offset-W26-final2-worker_client-release.xml` | `a1a068df35f6adafc8985135a11a1cf3e3766c70b367a401ab0399849039919b` |
| `offset-W26-final2-worker_client-asan.xml` | `4111c7c52253323f3d078b9637431659b63f3f2f470e1dee400a99d73c352c96` |
| `offset-W26-final2-worker_fault-release.xml` | `df704c754215d737eec06266ed70e3bfd940a4698c710cdf62a1b83577b303b6` |
| `offset-W26-final2-worker_fault-asan.xml` | `37f7aa095241eaf371a1014ab2ce17f044d5b26381983b027b51034f32d354ad` |

Import ownership trace after W26: `RS_FilterDXFRW::addSpline` normally constructs
and validates an offset carrier synchronously using a transient `DRW_Spline` and
`m_currentContainer`; that container may be model space, a block, an XREF child,
or a scratch container. `RS_FileIO::fileImport` is synchronous and can explicitly
accept partial results; it is also used by console and direct-filter callers.
`LC_DocumentsStorage::loadGraphic` currently initializes the supplied graphic,
and `QC_ApplicationWindow::openFile` creates a view before invoking it. Therefore
neither returning a pending job from `addSpline` nor treating the current GUI
document as unpublished is safe. The import handoff must own parsed values and
attributes beyond reader lifetime, preserve entity/block order and partial-import
policy, defer publication until all admitted offset work is resolved, and retain
the exact raw-cache fallback/diagnostic contract. It must account separately for
GUI open/template, XREF, direct synchronous and console callers; do not silently
disable editable imports or introduce nested GUI event loops. Cancellation and
owner destruction must destroy the private document/jobs without late delivery.

W27 unpublished import session `bf8171912`: an opt-in session owns a private
graphic while the helper validates bounded offset carriers and replaces each
placeholder in order with a native entity or ordinary spline fallback. It keeps
bounded structured diagnostics, explicitly permits or rejects partial reads,
propagates staging through the tested one-level XREF clone path, refreshes inserts
after resolution, and refuses publication if placeholders remain or the helper
cannot be reaped. Cancellation and destruction suppress delivery. The deadline
begins after parsing; parsing itself remains synchronous on the owning Qt thread.
The session is not yet connected to GUI open/templates, existing direct `RS_FileIO`
or console callers; the caller must finalize document state before publication.
It introduces no nested GUI event loop and does not change existing synchronous
import semantics.

Release/ASan focused suites discover and execute every named case, with no errors,
failures or skips (extra Trim cases are named sections):

| W27 suite | Declarations | Cases | Assertions Release / ASan |
| --- | ---: | ---: | ---: |
| hostile file and staged import | 45 | 45 | 52746 / 53798 |
| persistence | 55 | 55 | 40876 / 40876 |
| worker client | 16 | 16 | 7413 / 10956 |
| Trim | 123 | 133 | 60638 / 68244 |

Polling counts vary; ASan used `detect_leaks=0`. Tests cover actual helper
validation, cancellation, retry, partial-read policy, invalid caches, ordered
replacement, one-level XREF cloning and owner destruction. The timeout fixture
suspends the helper before numeric work; it is not a naturally hung numerical
primitive or Windows qualification. Fresh qmake6 configuration and three scoped
object compilations passed; a full qmake6 application link is not claimed.
Evidence is in `tmp/offset-2961-native-20261026/` in the normal checkout;
the qualified XML files are `offset-W27-final-{suite}-{release,asan}.xml`:

| W27 suite | Release SHA256 | ASan SHA256 |
| --- | --- | --- |
| hostilefile | `6e5ec412fe0e0ce9075843e813029ad67dce89815f3bf7ea4f97a0c871aeaf91` | `22656429b1851e0819d4ed4c4143daaf64cf8652cbe8b9736ebc3739c2c02617` |
| persistence | `b8c2aae8416e85609d841e2c2f9bfa7b9821115e29f6c91bfb45cb30650f8478` | `5b50757c38656e52fe3fdd17f4648404f277c80546cad1f5b3468fbf06892144` |
| worker_client | `6fd7f752d7be76642b2fb3ff72d8a6700437e202c9f69a1cd251851183771d2b` | `c32c8dd415bfad4b02c6f48de1ef2155ab581e6adfd5fe29767caf5df674d51c` |
| trim | `ddca521366c1ba2f26b0d9aa099a39997a3c815ede4305a86fbb06bec257760d` | `a0b288d3f5e59992049a862860ab7b2f790f578e67171acb83c6541c68497db5` |

Current-source requalification at `bf8171912` rebuilt creation action,
publication, source preparation and worker-client targets in Release and ASan.
The previously completed preview-cache and creation/repeat routing remains in
place; no second implementation was necessary. The following offscreen suites
executed every freshly discovered Release identity in both configurations with
zero errors, failures or skips; ASan used `detect_leaks=0`:

| Suite | Declarations | Cases | Assertions per build |
| --- | ---: | ---: | ---: |
| creation action | 85 | 132 | 90217 |
| creation publication | 18 | 19 | 414 |
| source preparation | 82 | 88 | 83527 |

The six XML reports and three discovery files are in the same evidence root,
named `offset-current-{creation-action,publication,source}-{release,asan}.xml`
and `offset-current-{action,publication,source}-discovery.xml` respectively.
Report SHA256 values, Release then ASan:

| Suite | Release SHA256 | ASan SHA256 |
| --- | --- | --- |
| creation action | `1d30efbd3d46a6c0925b3a5fba4fb1f5996cb6f7d434d613ba01a7e3e3e00954` | `4382ff0dd7b02443ac88b62608f0e745e367f37589e10f395c3358e1386793ae` |
| creation publication | `e1dd6a5c93c633b9ab9d5a640806566deeec456521d5c934ef55770fdc7e03ee` | `96d4a1056f733ef573c99b6cfa4f3c8d4ad37f355f56bce57437baec0983f597` |
| source preparation | `78dcf4d47ddac6ad0e69275cc0544f2d6a9875107ae95c14e9facbbfb1dcc7b0` | `670fcb2bb03e537b967738d3edb774d56501e2359e6c22c22be2a9440dfdbd9b` |

W28 partial-import decision `3de2cb13b` prepares the unpublished session for
the existing GUI's explicit partial-read choice. A decision callback sees the
parser error and whether the private graphic has entities; acceptance continues
with the same parsed drawing and sets its format to unknown, preventing an
ordinary save from silently overwriting the incomplete input. Rejection publishes
no document. Cancellation or destruction during the callback suppresses delivery.
Successful imports initialize the format before parsing, matching `RS_FileIO`;
the filter may still narrow it to the actual version. This callback is not wired
to the GUI yet and does not change the policy of existing synchronous callers.

Release/ASan hostile-file suites each discover and execute 46 declarations with
zero errors, failures or skips (52730 / 53399 assertions); worker-client suites
execute all 16 previously discovered declarations (7271 / 10750 assertions).
Polling counts vary and ASan uses `detect_leaks=0`. The new sections cover accept
without reparsing, reject, cancellation and session destruction during the decision;
the existing fixture lanes compare successful format state with synchronous
import. These are offscreen tests, not GUI dialog or deployed-platform acceptance.
In the same evidence root, `offset-W28-{hostile,client}-{release,asan}.xml` have
the following SHA256 values:

| Suite | Release SHA256 | ASan SHA256 |
| --- | --- | --- |
| hostile | `48c092b785eb8f3618760086a9a3416bbf7e2152a56317ef169e1009c67e056c` | `69dd633ff097188d5a3d36e23866ef9f19c15b61c29f24cc871f30b27f5b09a0` |
| client | `78b14dbb9e19acb0f784053042cbbe799cc37719a6ebe113f9e25556d85de32a` | `e7de03ab9c409b87745beeb51f1ee5c9eb223219cb2f6d795434719d46fd0f0e` |

W29 shared open finalization `5fe8b65e3` extracts the existing successful-open
bookkeeping into `LC_DocumentsStorage::finalizeLoadedGraphic`, preserving the
order of `onLoadingCompleted`, autosave/filename setup, saved timestamp, and
re-dirtying repaired or recovery imports. Existing synchronous loading now uses
the same operation that a later private-document GUI handoff can call. Tests
cover clean, repaired, recovery and format-unknown partial graphics; this does
not connect the staged session to a window or implement template finalization.

Release and focused ASan full-test binaries rebuilt. Each `[documentsstorage]`
run executes all 29 discovered declarations as 52 JUnit cases, 1202 assertions,
zero failures/errors and one existing skip: the JWW Save As test is skipped
because JWW is advertised as exportable although nothing writes it. ASan uses
`detect_leaks=0`; no full-suite or visible-GUI claim is made. In the evidence
root, `offset-W29-storage-{release,asan}.xml` have SHA256 respectively
`52ebd0d11986696d27652468ba53f4408807213f0b39e81cddda174a1382db43` and
`d31d4c154576cbdd3722f1ea4505c98fcd666ad5667a0596117567db0805de2c`.

W30 asynchronous partial decision `b2de24054` adds a request/reply API for
GUI callers: the private import waits without a nested event loop until a reply
arrives on the session's owning thread. A reply is one-shot; duplicates, late
replies after cancellation/destruction, and replies from an earlier cancelled
import cannot affect a later import. Existing synchronous-decision overloads
remain supported. Accepted replies retain the same drawing and format-unknown
overwrite protection; a callback throwing after its reply cannot revoke it.
The validation deadline begins after the decision, not while the user is deciding.
This API is not yet connected to a GUI dialog or document publication.

Release/ASan full hostile-file suites each discover 48 declarations and execute
53 cases with zero errors, failures or skips (53377 / 54215 assertions). They
exercise delayed accept/reject, duplicate reply, cancellation, destruction,
retry with an old reply, and a throw after acceptance. Worker-client suites each
execute all 16 previously discovered declarations (7319 / 10736 assertions),
also without errors, failures or skips. Polling counts vary; ASan uses
`detect_leaks=0`. qmake6 rebuilt the two affected objects, not the full app.
Reports in the same evidence root are `offset-W30-{hostile,client}-{release,asan}.xml`:

| Suite | Release SHA256 | ASan SHA256 |
| --- | --- | --- |
| hostile | `b573da61e7cdd6d8b55c13612916345b002d07be4e5976d28dc468b0bba90ba0` | `0702591a0a8bddf7da80017c3038efd0b1df408d3c13414a47044f750e32e895` |
| client | `bcd6d4ffd7608f14dc963b130bbbb28ad73572397c4e37f00e79e8f62b59d4f6` | `16057201446687d91c2d74a04fa7610b0840c9e6c8b3011109cfe924ff3a50c1` |

W31 GUI drawing open `f38abfa7a` connects supported DXF/DWG opens to the
unpublished session. The owning MDI window is created only after validation and
document finalization; `std::unique_ptr` explicitly transfers graphic ownership.
Non-DXF/DWG opens remain on the existing synchronous path. A partial DWG with
entities gets a nonblocking Yes/No dialog; accepted partials retain the
format-unknown protection. Other parse failures get a nonblocking error dialog.
Downgraded offset carriers are reported in command history. Closing or destroying
the application cancels pending sessions, and the saved startup-active drawing
is restored after pending opens finish. Automatic zoom restores the pre-zoom
modified state so it does not erase recovery/repair dirty state. Parsing still
runs synchronously on the Qt thread; only native validation is asynchronous.

Fresh Release application/full-test/helper and focused Release/ASan binaries
built. The following offscreen reports have complete discovered-identity coverage
and zero errors/failures. `SECTION` cases are not additional declarations:

| Suite | Declarations | Cases | Assertions Release / ASan | Skips |
| --- | ---: | ---: | ---: | ---: |
| GUI offset import | 8 | 11 | 188 / 224 | 0 |
| dock-window | 33 | 36 | 413 / 450 | 7 |
| hostile file | 48 | 53 | 53442 / 54195 | 0 |
| documents storage | 29 | 52 | 1202 / 1202 | 1 |

GUI tests cover native binary DXF and standards-safe ODA DWG, the original
ACadSharp counted-NUL DWG remaining ordinary, ownership destruction, startup
active-file restoration, a malformed DXF dialog, recovery dirty state, helper
failure fallback/reporting, cancellation and application close with a stopped
real helper, and retry. The helper is suspended before numeric work; no naturally
hung primitive is claimed. Seven dock tests require a larger offscreen screen;
the storage skip is the existing unavailable JWW writer. ASan uses
`detect_leaks=0`. Four changed qmake6 objects compiled; full qmake linking and
visible/deployed or cross-platform acceptance are not claimed. The earlier red
DWG probe assumed the documented ACadSharp counted-NUL input should be native;
it is retained, and the final test explicitly checks ordinary fallback instead.

In the evidence root, reports `offset-W31-final4-{import,gui}-{release,asan}.xml`
and `offset-W31-final3-{hostile,storage}-{release,asan}.xml` have SHA256:

| Suite | Release SHA256 | ASan SHA256 |
| --- | --- | --- |
| GUI import | `f4be0e7eadbb57b7744e92e077e612ac1956b10239408fd7d6cf24328190e7f0` | `2ab8df6874f3633ff0e806036955b12637e25560bf1d5db69951d7a04beb8b4a` |
| dock-window | `38411dae426a3d625f680bfecfbe304413b8517c350c8a2d4827a0c054e43c2f` | `bc08d21b56fb977593beba9f55ce11a29023df9c3d3f302c51b911d334c2eb15` |
| hostile | `37b54e328b9fb85f0bbc1341445da418b9a17811f742fcf4019655e33313dd40` | `51c1a217d0b5c1b849d8540f2e6d1b23cc0d89e56b0e87185a0c8ce2e79d4c87` |
| storage | `57f1016995f5522432c84585bd04c87efc4b54aba3240d2fcc0950aab3fbfe1f` | `7ecea1be5dc10c38f387893d0eba3b75c25c3d736dd6ba2d50faa26dfff1d73a` |

W32 staged template creation `087aa2400` routes non-empty GUI templates through
the unpublished session and creates an owning MDI drawing only after validation.
Both binary DXF and DWG templates are accepted; their result is an untitled DXF
drawing with the existing unnamed autosave convention and no template filename
in the opened-files list. Shared template finalization explicitly clears an old
filename, including for a reused graphic, so it cannot accidentally save over
a prior document. Blank new drawings retain the immediate existing path. Errors
leave the current drawing in place and use the nonblocking session dialog;
helper downgrade reporting and cancellation reuse the drawing-open path. Parsing
itself remains synchronous on the Qt thread. Direct synchronous template callers
still use their existing API and are not worker-routed by this slice.

Fresh Release application/full-test/helper and focused Release/ASan builds pass.
All discovered test identities execute with zero errors/failures; extra cases
are named sections:

| Suite | Declarations | Cases | Assertions Release / ASan | Skips |
| --- | ---: | ---: | ---: | ---: |
| GUI template | 5 | 6 | 133 / 164 | 0 |
| dock-window | 38 | 42 | 546 / 614 | 7 |
| documents storage | 30 | 53 | 1207 / 1207 | 1 |
| hostile file | 48 | 53 | 53566 / 54348 | 0 |

The GUI cases exercise native binary DXF and ODA DWG templates, the actual
default-template action, untitled/format/autosave state, unchanged source bytes,
immediate blank creation, malformed input without publication, and cancellation/
reap of a stopped helper followed by successful retry. The helper is stopped
before numerical work. The seven GUI skips require a larger offscreen screen;
the storage skip is the existing JWW writer case. ASan uses `detect_leaks=0`.
Three qmake6 objects compile; full qmake linking and visible/deployed and
cross-platform acceptance are not claimed. Reports in the evidence root are
`offset-W32-{template,gui,storage,hostile}-{release,asan}.xml`:

| Suite | Release SHA256 | ASan SHA256 |
| --- | --- | --- |
| template | `4a77a44c93ac02e39fde8b94fdc295bbe2d37623d866b802c1e34daa3b567d1f` | `aa52e761044fc26a5cb92032cc38620b75036ef7d798260bd3d1ffd0342218dc` |
| dock-window | `b288bb0c4c70127b7be2b1adb96f83e502c1b5493f73970e0dfaed2dd08302b3` | `5e553ef6f7851bd35198bbbb4c7edc7f9e3ae6b0bc3cca1675d880b922c678b9` |
| storage | `aa1ab9ed68e10882a5e6ed2b3f719dbe8651e88a9a0a0b8a16202e4845c6da75` | `6958778a4c27f7a9ab042075c72c4385dc4607b0023bce8a1d57fb8293bf806f` |
| hostile | `4ddaca144384c9cf4647474b5e80a427efc216afd3b3eb84ce4bb2a25c2f3a2b` | `def852f9c101c847694abaa4c82241e184aaa246cca89c3a2a94f6b4300b648b` |

W33 bounded GUI import queue `33f882ebf` limits staged drawing/template
imports to four active sessions and 64 FIFO waiting requests. Further requests
are explicitly refused in command history; queued requests are announced.
Completion or failure schedules the next waiting requests without a nested event
loop. Cancellation/application close clears both active and queued work, and
startup-active restoration stays pending until both are empty. These bounds apply
to this GUI import queue, not all native workers in the application.

Fresh Release application/full-test/helper and focused Release/ASan builds pass.
All discovered identities execute with zero errors/failures; extra cases are
named sections:

| Suite | Declarations | Cases | Assertions Release / ASan | Skips |
| --- | ---: | ---: | ---: | ---: |
| GUI import queue | 3 | 3 | 49 / 56 | 0 |
| dock-window | 41 | 45 | 603 / 681 | 7 |
| documents storage | 30 | 53 | 1207 / 1207 | 1 |
| hostile file | 48 | 53 | 53913 / 54888 | 0 |

Tests cover mixed drawing/template requests, FIFO waiting order, the active cap,
startup-active restoration across the queue, overflow diagnostics, cancellation,
and a failed queued import followed by a successful one. The seven GUI skips
require a larger offscreen screen; the storage skip is the existing JWW case.
ASan uses `detect_leaks=0`. Two qmake6 objects compile, not a full link. The
first ASan queue probe aborted on the application singleton assertion while the
fixture replaced a window with six drawings still open. W33 final tests closed
those drawings normally before fixture replacement; W35 below reproduces and fixes
the forced-destruction path rather than treating that W33 result as proof.
The red probe is retained alongside the final evidence. Reports are
`offset-W33-{queue,gui,storage,hostile}-{release,asan}.xml` in the evidence root:

| Suite | Release SHA256 | ASan SHA256 |
| --- | --- | --- |
| queue | `36cb80fecc34b7aa82bd1f64269368b9a9569ff979d5c21ee506bbedee20915a` | `f6f21c81389a044999dd490b6e693703bfe9513a6910c70bd450a2369f541721` |
| dock-window | `b0b8b549b2078217198ba33a87e70d58ccf7dadd17ffd0b9e1821a53104b6f57` | `df776f6c8308b5e249b51249275dfd2dd3f50b34dd018068ed94de9adc79b198` |
| storage | `2f20b3a80a9a2049e96d1b29ec4dfe966d561ebf43b46d2267adbc2fedd74ab9` | `146400da206b4a9fecf04a8b330cd8ef18f198d4acf3c48e3454e047d21117e0` |
| hostile | `181f855144b9933c1c2ffeb6ac09e6464563353fc099393bd9844cd78e513333` | `522c0ea4da97ebe022a37f192989724ee47ed1877b3f7967a727bea31dfa7065` |

W34 staged-source checks `b9882d7a1` records a filesystem metadata stamp at
the start of parsing: canonical path, size, modification time, and metadata-change
and birth times when available. An observed change at the read boundary refuses
publication. A change or unverifiable stamp after parsing marks a GUI-opened
drawing modified and format-unknown, reports the reason, and requires Save As;
the pre-read modification time remains the ordinary Save baseline. Template
imports report an unverified source but remain untitled. Autosave now refuses a
path that identifies the current source, including existing hard/symbolic links
and a dangling symbolic link; unresolved link chains are refused conservatively.

This is **not a content-identity or atomic-write proof**. Indistinguishable or
unavailable filesystem metadata can miss a change, and an alias can be swapped
between the autosave check and write. A user can explicitly choose the source in
Save As and confirm overwrite. Stronger source identity and atomic destination
validation remain open; do not treat the metadata checks as a complete solution.

Fresh Release application/full-test/helper and focused Release/ASan builds pass.
All discovered identities execute with zero errors/failures; extra cases are
named sections:

| Suite | Declarations | Cases | Assertions Release / ASan | Skips |
| --- | ---: | ---: | ---: | ---: |
| GUI source change | 3 | 3 | 79 / 95 | 0 |
| dock-window | 44 | 48 | 728 / 878 | 7 |
| documents storage | 33 | 56 | 1241 / 1241 | 1 |
| hostile file | 48 | 53 | 53511 / 54340 | 0 |

Tests cover modification after parsing during a real helper, a same-size edit
with restored mtime detected by metadata-change time on this filesystem, the
read-boundary refusal using a private one-shot test hook, Save As rather than
ordinary Save, and autosave through identical, hard-link, symbolic-link and
dangling-link paths. The read-boundary hook is not a naturally concurrent parser
race; the metadata-time and link tests may skip on unsupported filesystems.
Seven GUI cases require a larger offscreen screen; the storage skip is the
existing JWW writer case. ASan uses `detect_leaks=0`. Four qmake6 objects compile,
not a full link. In the evidence root, reports are
`offset-W34-final2-{source,gui,storage,hostile}-{release,asan}.xml`:

| Suite | Release SHA256 | ASan SHA256 |
| --- | --- | --- |
| source | `6ac627b949c541986964950b9c4b6cde5b89ebebc0e10878cf31cba641a2d0e3` | `72f0bc7afb6abcebada37392f5e55b7ea854adcadebf2b8cd94d8ccaf6263f52` |
| dock-window | `f0512c80e1af8b16400df2564a37f17d9f696f6f66f0ac6957f95f38762bb835` | `1851cc4e46c02c87b8e5555e3493cf82f8a0c980268adcdd22b12f22673e2269` |
| storage | `c3e61b18784788bbf240dcf68acf2574e665a09984f450c951e7f95e0357fbf0` | `e81d2f2ead1a4c97855a055bd17d0dcc75ac74221c3e323e7ad57f9501d8e3c2` |
| hostile | `33303b5126423b4fb0fe9215cdd6e64b0e34839ac134dedb7e0b4dca0f588307` | `4898d6e343e9783b01baa52bd4803f659ceba83bffda63ab7b701589da04a277` |

W35 MDI teardown `d8d174365` reproduces the W33 assertion with six live imported
drawings and the original hide/deferred-delete sequence. LLDB showed that
destroying an MDI child emitted `QMdiArea::subWindowActivated`, which reached
`doWindowActivated`, the undo listener, and `getAppWindow()` after the singleton
had been reset. Application destruction now disconnects the MDI area's signals
to the application before cancelling imports and destroying child windows.
The focused test verifies destruction and replacement with live drawings, and
with a real helper stopped at startup: the helper is reaped and no drawing survives.
Normal close/ownership regressions also pass. This does not qualify every possible
teardown or naturally hung numerical primitive.

Fresh Release application/full-test/helper and focused Release/ASan builds pass.
All discovered identities execute with zero errors/failures; extra cases are
named sections:

| Suite | Declarations | Cases | Assertions Release / ASan | Skips |
| --- | ---: | ---: | ---: | ---: |
| forced destruction | 1 | 2 | 116 / 84 | 0 |
| dock-window | 45 | 50 | 758 / 861 | 7 |
| normal GUI close | 9 | 10 | 483 / 483 | 0 |
| documents storage | 33 | 56 | 1241 / 1241 | 1 |
| hostile file | 48 | 53 | 53794 / 54801 | 0 |

Polling counts vary. The GUI skips require a larger offscreen screen; the storage
skip is the existing JWW case. ASan uses `detect_leaks=0`. One qmake6 object
compiled, not a full link; cross-platform and visible acceptance remain open.
The failing W35 ASan reproduction is retained alongside the W33 red probe. In the
evidence root, reports are `offset-W35-{forced,gui,close,storage,hostile}-{release,asan}.xml`:

| Suite | Release SHA256 | ASan SHA256 |
| --- | --- | --- |
| forced | `7afa032ab72589e1360e4d1e892c08b9282f07d9a8399afd5e247f8846a1c779` | `2a5a927e743c0a7b1fd51fe67482abcfeba1cb53d63d391def04218d28480c79` |
| dock-window | `0dbdd233bbf1971792914455018ef3fa8b213bbc4daba6bd8a9205f15adba654` | `352f4ccc958db71b52d2f1ee0a53075e6f96cee45b405319cbc2640c1129ee84` |
| close | `ac9e9cbaaaaa487e87deee6cbede77213e4d0d4c5cc3d56853ecc659d4f9ffd7` | `358345da8447851b67e7476dcd38d3f93daa6c9a1d3dd1ed6ed2eb1bddd8d899` |
| storage | `92d1b755bdbafc24b50d9803cc9da610ad5287afad9201eb7c75edac8e7e9c66` | `05d345db92500cd7d5a72db4f53d5994e69d304550b7df28a9077a6c42646726` |
| hostile | `fd6efefb1d0ccdd80236b0fd02e1d37886ab4160549c83dba48f029517690aa1` | `2e576ba1079457dfd935576143797617c9ac080a30e3aced9d3127052a4801b4` |

Startup selection continuation W36 (`96f595cca`):
An observed switch to another MDI window while startup imports remain pending
now takes precedence over the saved startup filename. Import publication and
its restoration run under a lifetime-guarded activation scope, so they do not
overwrite the later selection; subsequent publications restore the selected
live window by identity. The selected pointer is checked against the live
window list before use, and cancellation/end of the queue clears the state.
The new regression first reproduced the lost selection, then passed using a
real helper stopped before numeric work, both the application window command
and Qt MDI activation paths, multiple publications, and closing the selected
window. These are offscreen programmatic activations, not visible mouse tests.

Fresh Release application, full-test executable, helper and focused GUI builds
passed. Release and ASan (`detect_leaks=0`) startup reports cover two declared
tests / eight JUnit entries with zero errors/failures/skips (176 / 207 assertions,
including polling). The broader dock-window suite has all 46 discovered
declarations / 57 entries, zero errors/failures and seven small-screen skips
(929 / 1015 assertions). Discovery identities were checked against both reports.
The changed qmake6 application-window object compiles, not a full qmake link.
The evidence root is unchanged:

| W36 artifact | SHA256 |
| --- | --- |
| `offset-W36-startup-release.xml` | `2b020aa4fb4a12dd469d880eda7d35448ebb3f6a264fc79547345a9af20917fa` |
| `offset-W36-startup-asan.xml` | `dc8a95d24ce9c3e7168f07fb508de758f1fa3f339d980711bc2648a0537688f5` |
| `offset-W36-gui-release.xml` | `c32dbe62edf316b804ca3358f73b1aa927e0af177090f262faabb672acaed933` |
| `offset-W36-gui-asan.xml` | `50244d6657c7e9a18d87fb32583c5d94156b9c5b2c9fdf0f7404ef3071c7cec5` |
| `offset-W36-gui-discovery.xml` | `bcd861ac8d9a8617ee69cf75605ed87857a6989ac8e589ee64fa3309d743a64b` |

This does not detect interaction with the already-active window or establish
priority for a new user-requested open; an activation from another non-import
source is treated as a later selection. Audit these cases, nested/reentrant
activation and actual visible keyboard/mouse behavior before declaring the
broader startup-intent contract complete.

Startup request ordering W37 (`8ef22d799`):
When a saved startup target is pending, each later accepted staged drawing or
template request receives a monotonic selection token, retained through the
bounded queue. A successful request becomes the selected window only if no
newer request or observed window switch has superseded it. Old startup imports
and older user requests cannot take focus back when they finish later; a live
window is still checked before restoration. Serial exhaustion clears the startup
restoration policy rather than wrapping to an old request. Failed or rejected
imports do not publish a drawing; this is not a claim about all failure-policy
or user-intent cases.

Release and ASan (`detect_leaks=0`) startup reports cover five declarations /
15 JUnit entries with zero errors/failures/skips (352 / 379 assertions, including
polling). Coverage includes new drawings and templates, a slow native request
overtaken by another request or observed switch, verified native admission,
queued request selection and a later switch while queued. A real helper is
stopped before numeric work in the out-of-order cases; the queue case uses
ordinary files and the real four-active limit. Earlier multi-suspended-helper
probes are not acceptance evidence because production timeouts could yield
fallback. Both final dock-window reports cover all 49 discovered declarations /
64 entries, zero errors/failures and seven small-screen skips (1016 / 1164
assertions). Fresh Release application, full-test executable and helper builds
passed; the changed qmake6 object compiled, not a full qmake link. The final
test-source change after that application build only extends tests; focused
test targets were rebuilt in both configurations. Evidence root is unchanged:

| W37 artifact | SHA256 |
| --- | --- |
| `offset-W37-final-startup-release.xml` | `a4a5fdcff857609aeaaa8d06d25ebf4d264af4cf241717c3161df933c0d5ad32` |
| `offset-W37-final-startup-asan.xml` | `f06a3d72d7d060935a8cc87ab713362ec241b655de1b5e9834babf0757a486b5` |
| `offset-W37-final-gui-release.xml` | `1cf87795385f51ec9b38fc6c31352a72a4807f62cd48d4ba011a1d32f6e78628` |
| `offset-W37-final-gui-asan.xml` | `105da3a96a7a4561529f6968f267809863e699bf21b5f66d5bde846aaf4e3a5b` |
| `offset-W37-gui-discovery.xml` | `797412160ec2fff6ac714f8db9a69671ed735b8289984d1a3bc091398ced2197` |

At W37 the source-protection audit found that an import snapshot must preserve
the original host path for XREF resolution, and source stamps alone do not bind
the bytes parsed. The output transaction then closed its exclusively created
descriptor and reopened the stage by pathname; W38 below fixes that reopen, but
not all publication and source-identity races.

Descriptor-backed output W38 (`0fb12e534`):
DWG/DXF output now writes through the descriptor returned by exclusive stage
creation. Its stream supports DXF position queries and patching; internal DWG
writers accept `std::ostream`. The stage's identity is checked after the write
and again after verification before publication, and cleanup avoids deliberately
removing a stage already observed to have been replaced. POSIX retains a
close-on-exec duplicate until publication/abort, preventing inode reuse during
those checks; descriptor exhaustion refuses publication. A private test hook
reproduced the old immediate truncation when the stage was swapped for a symlink
between exclusive creation and pathname reopen. The new code keeps the original
destination intact and refuses the substituted stage; a second test replaces
the stage during verification. These tests run on macOS, not Windows.

This is not an atomic identity-and-rename guarantee. The stage or its parent
may still change between a check and rename or cleanup; verification can observe
an altered or temporarily substituted stage; Windows closes its descriptor
before the pathname checks and has not been runtime-qualified. Autosave's
source/destination alias checks and the staged import's metadata-only source
stamp remain non-atomic. Preserve the open roadmap items for stable imported
bytes, XREF path semantics, anchored destination policy and adversarial races.

Fresh Release application, full-test, helper, DWG/DXF fast targets and
qmake6 libdxfrw archive builds passed. Release/ASan (`detect_leaks=0`) stage
tests pass all 14 declarations / 14 entries, 131 assertions, no skips. The full
offset hostile-file target passes all 48 declarations / 53 entries with no
skips (53804 / 54187 assertions, including polling). Additional Release runs
have zero errors/failures: `[dwg-write]` 685 declarations / 713 entries / 6
skips, DWG fast 1066 / 1152 / 37 skips, DXF fast 795 / 1090 / 4 skips, and
documents storage 33 / 56 / 1 skip. Discovery identities were checked; these
overlapping suites are not summed. Skipped external fixtures remain unqualified.
Full qmake application link, Windows tests, leak checking, visible acceptance and
the broader plan are not claimed. Evidence root is unchanged:

| W38 artifact | SHA256 |
| --- | --- |
| `offset-W38-final-stage-release.xml` | `663bef82bfabcca9952a97affe0e1ce97224033a58ffd2c74d6dd7c6309b7a85` |
| `offset-W38-final-stage-asan.xml` | `1d52f3520fe3f029debbef92429365219498243c0ba76362eee9af212435797a` |
| `offset-W38-final-stage-discovery.xml` | `10583c4785a23e94aad62bee28113cb10d54db37b0c425dbf4004289871b88fb` |
| `offset-W38-final-dwg-release.xml` | `5cc6b2174af519719ff7b249820886654825a7d4372c956d5cd71efc189ae9e8` |
| `offset-W38-final-dxf-release.xml` | `f8577e2c5a3214b1772f85f8d3a241b41b8f4f2501c606b26c49c6ad515c1af7` |
| `offset-W38-final-dwg-write-release.xml` | `2d5fc3842c3517f70707e23344550a081a18b39aad75db51acd9c94a45247dbe` |
| `offset-W38-final-hostile-release.xml` | `09ef6b4c97546e5cb77061d6d1d8625c9303779d810980636a701455e910b800` |
| `offset-W38-final-hostile-asan.xml` | `cad1bb61d2f464d72cae8ad0aada641d16d596fdb8da76cbba36554d921ca618` |
| `offset-W38-final-storage-release.xml` | `cbe71a7680c30f505c3d78e02ce8d92d2046fc203e143fe150f157c1afe144b3` |

Single-stream DXF read W39 (`dd862d48d`):
The DXF reader now detects ASCII, R12 binary or newer binary and parses through
the same open stream instead of closing and reopening the pathname after the
probe. A caller-owned seekable binary-stream entry point reads from its current
position and resets parser state between reads. ASCII CRLF and newline-less
records continue to be handled by the existing ASCII reader. A private test
hook reproduced a pathname swap after probing: the old code read the replacement
or selected the wrong binary parser. The new reader keeps the opened file;
tests cover both original and replacement formats, R12/R2018, nonzero stream
positions, and nonseekable input. The replacement test is POSIX-only and does
not establish Windows file-sharing behavior.

This is a prerequisite for, not an implementation of, an immutable snapshot:
the staged import still reads the source path, concurrent in-place writes can
change its bytes, and source metadata checks do not prove content identity.
XREF resolution still uses the original host path and nested references have
their own read/ownership questions. At W39 the DWG filter separately probed
the pathname for version metadata; W40 below removes that separate version
probe. Bound snapshot memory/I/O, cancellation, source-change detection,
external references and save policy remain open.

Fresh Release application, full-test executable, helper and qmake6 libdxfrw
archive builds pass; this is not a full qmake application link. Release and
ASan (`detect_leaks=0`) focused reader reports cover four declarations / four
entries, 94 assertions and no skips. Both complete DXF fast reports cover all
799 discovered declarations / 1094 entries, 84209 assertions, zero errors or
failures and four external-fixture skips. The complete hostile-file target
covers all 48 declarations / 53 entries with no skips (53503 / 54450 assertions,
including polling). Discovery identities were checked. Windows, visible UI,
leak checking and the broader roadmap are not qualified. Evidence root is
unchanged:

| W39 artifact | SHA256 |
| --- | --- |
| `offset-W39-final-reader-release.xml` | `b322743d02c555e5550c470471099bf70f23b35c575a936b16f77f4ed260c871` |
| `offset-W39-final-reader-asan.xml` | `5ee050d736eb499d5363c279680c5c70b64dd0ad9e1d9707f97f1d2aa6931fe1` |
| `offset-W39-final-dxf-release.xml` | `870cef12976b4e775fe5d072dd3b8ae8bd0284df5f08a08d354b968560af3c8c` |
| `offset-W39-final-dxf-asan.xml` | `8af4df182b0f81347696c1aa0b06fbc555e4cbaa0ddcadcfe74ca19fb9e49053` |
| `offset-W39-final-hostile-release.xml` | `67f98086caf7c53632fa3042b92fcb56bf7af89286385720c4352322b563dbbc` |
| `offset-W39-final-hostile-asan.xml` | `b8bf8859753bc9d86d88f940df69585ad34ef971a9f20f1d6561906b40224d43` |
| `offset-W39-reader-discovery.xml` | `ca26a168cb2a4cb64d0d4b7050b210c8223be43a85f8f292902c7620b2cc94ab` |
| `offset-W39-dxf-discovery.xml` | `b44f9015385b91af16058352d76cefa6bb6e60bf6585316166c8eeae3a7e55cb` |

Opened DWG version W40 (`85d90dd95`):
The DWG reader can synchronously report the version of the file or buffer it
actually opened, after its file header is accepted and before model callbacks.
The filter uses that version for offset XDATA context and its numeric import
guards, instead of separately reading the pathname's magic. For a recognized
DWG version, a conflicting header variable no longer overrides the physical
version; DXF header behavior remains unchanged. An observer exception fails
the read before model callbacks, and reentrant top-level reads, writes and
preview/test operations from the observer are refused without disturbing the
outer read. The observer/context are caller-owned and must not otherwise mutate
the reader.

The regression swaps an R2000 and R2018 fixture after the former filter probe;
both directions formerly supplied the wrong offset and numeric versions. The
new path supplies the version of the file actually opened. File/buffer callback
ordering, conflicting headers, observer failure/clearing/reentry and short input
are also covered. The swap case is POSIX-only. It does not prove that a swapped
source is rejected, or that bytes cannot change in place during parsing; stable
snapshot, XREF, source-save and atomic destination work remain open.

Fresh Release application, full-test, helper and DWG-fast builds pass. The
qmake6 libdxfrw archive and changed filter object compile, not a full qmake app
link. Release and ASan (`detect_leaks=0`) version tests cover three declarations /
three entries, 188 assertions and no skips; the complete hostile-file suite
covers 51 declarations / 56 entries, no skips (53667 / 54542 assertions,
including polling). Release `[dwg]` covers 706 declarations / 944 entries with
56 skips, and DWG fast covers 1066 / 1152 with 37 skips; both have zero errors
and failures. All discovered identities were checked, and overlapping suites
are not summed. Skipped fixtures/conditions, including pre-R13 external cases,
remain unqualified. Windows, visible UI, full ASan and leak checks are not
claimed. Evidence root is unchanged:

| W40 artifact | SHA256 |
| --- | --- |
| `offset-W40-version-release.xml` | `f13a126905d8af8c25c9fa164e9ccb3aef548fdc583087fca2f8a634c071e1d4` |
| `offset-W40-version-asan.xml` | `b4af384e417eac44520ff83a1fa106cf149c45bc7841d0870fe87c996caa1255` |
| `offset-W40-version-discovery.xml` | `83a7e684adb9a4c0faff613e06dce7585f18ff6bade8421c20a7ec8be2c2d638` |
| `offset-W40-hostile-release.xml` | `aa8c2e4d995f4db5531412d80e29ee9bb62bff184c79894f14706adbefd8b60d` |
| `offset-W40-hostile-asan.xml` | `a03d3bd6e8203184c5b058b82c0fcf88c813cf3d9d4834642d1be097085c8997` |
| `offset-W40-hostile-discovery.xml` | `843f76ab73e100d7b172e2a5533cb12131c62e6aca38082cae1716319e19a2f4` |
| `offset-W40-dwg-fast-release.xml` | `b609d0cad95cccba40c7ce9f549dd48561021b00dd39c1cf93bb29ec0cd21bc7` |
| `offset-W40-dwg-release.xml` | `d98c37c0b6f0c33fe887de0cca81dfe936be5e1c45ecc70c8a4da214d533012f` |
| `offset-W40-dwg-discovery.xml` | `704a1ca51c12003f7a15cb545b07ec6a06c34815fd13e3235ac71cb4416f52c4` |

W41 caller-owned import streams `3c7effca8`:
DWG readers now accept a caller-owned seekable binary stream from its current
position, including independent logical cursors over a shared stream. The filter
accepts either DWG or DXF bytes from a stream while retaining the logical source
filename for relative external-reference lookup. Tests cover nonzero origins,
file/buffer/stream version-observer ordering and reentry, forward-only and
end-seek failures, truncated input, all existing offset import lanes, and an XREF
resolved relative to a logical host pathname that does not exist on disk.

Release and ASan hostile-file runs pass all 54 declarations / 59 JUnit entries,
with no failures or skips and complete identity coverage. The DWG stream-buffer
test passes in both configurations. Release DWG fast passes 1067 declarations /
1153 entries with 37 existing skips; DXF fast passes 799 / 1094 with 4 existing
skips. All discovered identities are accounted for and neither suite has
failures. ASan uses `detect_leaks=0`; it is not a leak-free or full-suite claim.
The qmake6 libdxfrw archive and changed filter object compile, not the full app.
Local evidence in `/private/tmp` (temporary, not a durable external fixture):

| W41 report | SHA256 |
| --- | --- |
| `offset-W41-hostile-release.xml` | `9bd63b1908969a22364fba46375f11303610543693c50f48b32bea9b13ef5c4e` |
| `offset-W41-hostile-asan.xml` | `e62a7e10e91a766f0b3466087c9dbcf1580e74850c19b7cb1962903b5a5a9607` |
| `offset-W41-hostile-discovery.xml` | `99c4eef53c40479c8fc4f7245dfa9ff30568c0fad3c2529f1b8a8c73a39acde3` |
| `offset-W41-dwg-release.xml` | `395a0b53e8450d8e19320ad62d4609c164c0f1a92f194bc32f6977d880b6a9db` |
| `offset-W41-dwg-discovery.xml` | `3e6dbf0423791f169616de7eb1e019ca9a31e7837b33437e26f88e47329d5225` |
| `offset-W41-dxf-release.xml` | `1fb57b81306cef412316ef9c5d321f7243e903c00dcf79f39788f31a04f68048` |
| `offset-W41-dxf-discovery.xml` | `969a963b7c937cb98185b97a28ac95e1225f2c612d5b5c1bb835ce4fc8f60790` |
| `offset-W41-buffer-asan.xml` | `356cea5c4e60265010f5686d112766ebc7e3fbfaaeabaabe6e41d0ce0d04a9de` |

This is an API prerequisite, not a bounded immutable snapshot or GUI session
integration. In-place mutation, nested XREF consistency, source/save identity,
atomic publication, cross-platform qualification and remaining import callers
remain open. Parsing and nested XREF work are not made asynchronous by W41.

W42 staged host capture `664ed5e73` uses W41 for GUI staged imports. It reads
the opened host in 256 KiB event-loop chunks into an owned seekable buffer and
parses those captured bytes with the original logical path. Cancellation between
chunks suppresses delivery, and metadata changes observed during capture or
parsing refuse publication. The explicit per-host capture limit is 256 MiB; a
larger or growing input is refused with a diagnostic. Up to four active staged
imports can each hold a buffer, in addition to other parser and document memory.
Format selection for an unknown type uses the logical extension, avoiding a
separate content read before capture. An existing saved-layer GUI test now waits
for staged completion instead of assuming a single event-loop turn.

Release and ASan hostile-file runs pass all 56 declarations / 61 JUnit entries,
without failures or skips. They exercise all existing DXF/DWG staged lanes plus
yielding, cancellation and retry, in-place modification, pathname replacement,
and a sparse oversized file. Release offscreen `[dock-window]` passes 49
declarations / 64 entries; the ASan offset-import/saved-layer subset passes 20 /
34. All discovered identities are accounted for, without failures or skips.
The original GUI run exposed the saved-layer test's one-turn assumption; it was
updated without removing its assertions. ASan uses `detect_leaks=0`. The qmake6
session object compiles; neither a full qmake link nor visible UI was qualified.
The following reports are local temporary evidence in `/private/tmp`:

| W42 report | SHA256 |
| --- | --- |
| `offset-W42-hostile-release-qualified.xml` | `afc0f6bc4c5d8d553011602a936d979062804bc8a60b8963926762949788bf38` |
| `offset-W42-hostile-asan-qualified.xml` | `d9e2e556ae6fd210a0f78e5cbe76d025530f8e88bae2c1ed0a5217b8f2360625` |
| `offset-W42-hostile-discovery.xml` | `4750155c57b7b29536b30304cf2edf06f409b44afdc38d3b56354cb3e4c13d74` |
| `offset-W42-dock-release-qualified.xml` | `7561bbef7963e1c11e78e0e22add8d0eead6c82e8af51a0d711f0172cc9b0a4f` |
| `offset-W42-dock-asan-qualified.xml` | `60c4cc32f93fee79a12a811a8ad49795714fd0591588f8dd132c77a921a368d9` |
| `offset-W42-dock-discovery.xml` | `802ad74b8618dc8326a0b9257fc43f013cac7e1c5289ead95837f7c86e345458` |
| `offset-W42-dock-asan-discovery.xml` | `ebe000c59b0e71231f170c6fe39d9a5548a38fadc3a8304e3bd5d59043361678` |

The captured bytes are stable after capture, not necessarily an atomic version
of a concurrently modified file. W43 below subsequently binds the POSIX opened
descriptor to observed pathname identity and adds native metadata checks; those
checks can still miss changes. Nested XREFs still read their
own mutable paths. Parsing, each individual file read and XREF loading can block
the owning thread; capture cancellation is between reads. Source-save identity,
atomic publication, non-GUI callers, large-file policy and cross-platform
qualification remain open. Do not describe this as a complete immutable import
transaction or restart W42 as absent.

W43 POSIX host identity `8dd114db9` records regular-file device/inode, size,
modification and change timestamps in the staged source stamp. On POSIX it
verifies the opened descriptor against that stamp before capture and again after
capture and parsing, retains the descriptor through parsing, checks the captured
byte count, and includes native identity in subsequent pathname comparisons. A
regular source without verifiable POSIX identity is refused. Qt resources retain
their separate non-descriptor path. macOS and Linux use nanosecond timestamps;
other POSIX builds currently retain second-resolution native timestamps.

macOS tests cover replacement between pathname observation and open (refused
before parsing), descriptor mutation, same-size rewriting with restored mtime,
hard links, symlinks and a Qt-resource import. Release and ASan hostile-file
runs pass 59 declarations / 64 JUnit entries without skips; Release offscreen
`[dock-window]` passes 50 / 65 and the serial ASan import/template/saved-layer
subset passes 26 / 41. All discovered identities are accounted for without
failures or skips in these final reports. ASan uses `detect_leaks=0`; qmake6
compiled the session object, not a full app. An earlier ASan GUI selection run
had a native-template assertion failure while multiple suites ran concurrently;
the isolated template and full serial selection passed, but the initial failure
is retained and its cause is not established. Investigate reproducibility under
load before final acceptance. Local temporary evidence in `/private/tmp`:

| W43 report | SHA256 |
| --- | --- |
| `offset-W43-hostile-release-final.xml` | `3c5a7ddf138f970be4bf82787182b15d86df158ee0b2182b6b79a301fa783bab` |
| `offset-W43-hostile-asan-final.xml` | `9922c053a4f81a8786b84c8d1a2fec5ab4de34ec238e6a1780e330c34f6da3ff` |
| `offset-W43-hostile-discovery.xml` | `96886f04bc0c9aa68235815b5953077fa19dc14cc409e6a2aa7e0c0bf70f8279` |
| `offset-W43-dock-release.xml` | `1cc4fe4c7aa44c0c1f479bcf84cce2786355da344bab6ddf5bc00ed9be2c143d` |
| `offset-W43-dock-discovery.xml` | `e0b5335c5964c16e60402ed500177a2b422dfd5e9e2326ac703974c5f5d48d60` |
| `offset-W43-dock-asan.xml` (initial failure) | `b136bcc3142dd4d41da1f758b890f95a875281b3886c2e026d83e7d16fbfa583` |
| `offset-W43-dock-asan-serial.xml` | `3ada700f2618c1fc0ba91aa27ebda4c6c8bb024efb1dd86ae1d234d3e3d0fc82` |
| `offset-W43-dock-asan-discovery.xml` | `938fb1ec0baf5f71f18845fd3249dd2ad2aeeefac9608a6087a6405efd049095` |

Identity and timestamp comparisons are observations, not an atomic snapshot or
content hash: filesystems can have weak or altered metadata, a pathname can
change between checks, and the source can change after publication. Windows
parity is implemented below but is not runtime-qualified here.
Nested XREFs, save/check-to-use and atomic destination protection, the 256 MiB
host limit, other import callers and full platform/visible acceptance stay open.

W44 Windows staged source identity `2ab7f67ce` uses `QFile::handle()`'s CRT
descriptor and `_get_osfhandle` to compare volume serial/file index, size,
last-write time and change time from the open handle. Path checks open the
current path for attributes and compare the same identity; Windows timestamps
are normalized from 100 ns FILETIME units. The common capture flow now requires
this identity for regular files on Windows as on POSIX. Two Windows-guarded
tests cover same-handle mutation and a pathname replacement before open.

On this macOS host, Release and ASan hostile-file suites each pass all 59
discovered declarations / 64 entries with no failures or skips, and discovery
identity coverage is complete. The qmake6 session object compiles. These runs
exercise the POSIX branch only: no Windows SDK/compiler or Windows runtime was
available, so the Windows implementation and its two tests are not yet compiled
or executed. Do not close cross-platform acceptance based on this code-only
parity. Local temporary reports in `/private/tmp`:

| W44 report | SHA256 |
| --- | --- |
| `offset-W44-hostile-release-final.xml` | `c1d6c711bfc5895891e0a2efc2ec1baacc6505105435c5f9a629433cf30686e3` |
| `offset-W44-hostile-asan.xml` | `4460f59c2ab1b0ed884c38731ea6a8018a88f800df802fee9d25a3dc7b4cf4e8` |
| `offset-W44-hostile-discovery.xml` | `0223e0dc24c79edc576e82c3ce562460e0301e288e17b56ae7e95251a479912e` |

The Windows handle checks still depend on filesystem identity/timestamp
semantics and are not an atomic transaction. Nested XREFs, save/check-to-use,
destination publication, large-file policy and Windows runtime validation remain
open at W44; W46 below closes nested-XREF byte capture for staged imports only.

W45 current-source cache/create requalification at `838bfeca8` revisited the
requested preview-cache portability and creation/repeat routing. No product gap
was found: W16/W17 already provide owned value snapshots, bounded CBOR transport,
source rebinding and fresh-helper cache reuse; W18/W19 already route eligible
Modify Offset preview/commit/repeat through owned continuations and atomic model
publication. A fresh macOS Release CMake configure/build succeeded for
`librecad_offset_source_preparation_tests`,
`librecad_offset_worker_process_tests`, and
`librecad_offset_creation_action_tests`. Running each binary offscreen passed:

| W45 suite | Test cases | Assertions | Result |
| --- | ---: | ---: | --- |
| Source preparation/cache rebinding | 82 | 83527 | pass |
| Real helper-process/cache transport | 4 | 2724 | pass |
| Creation action/preview/repeat GUI routing | 85 | 90217 | pass |

The helper-process assertion count is scheduling-dependent; the prior interactive
run reported 2928 and the saved serial run below reports 2724. Both report all
four test cases passed. Saved logs under `/private/tmp`:

| W45 artifact | SHA256 |
| --- | --- |
| `offset-cache-portability-release.log` | `987d4d7d0890d73325a3714d6cc4d2de685dff1359a3a91b6ff967b479e31075` |
| `offset-cache-worker-process-release.log` | `196fffab5c7abbf426a64ac7b89d5a009f8a03fa1afcad43a1dde75c441b3974` |
| `offset-preview-create-action-release.log` | `d0a333f116a2d740ab50a5351945dd08d8a94dc5a0561d58954e158d068f9ec4` |

This is current-source macOS Release qualification only, not a new code change,
ASan run, cross-platform runtime result, visible interactive GUI session, or
deployed-helper proof. Keep W16-W19 implemented; do not restart them as missing.
Windows runtime validation and the separate W0-W2/import/final acceptance gates
remain open.

W46 staged nested-XREF snapshots `759bf4b7f` use the captured host's logical path
to resolve each referenced drawing, then read each distinct dependency into an
owned seekable byte buffer. DXF and DWG children parse from those captured bytes,
not a second pathname open; canonical-path duplicates reuse the same snapshot.
The staged import shares one 256 MiB host-plus-XREF byte budget and caps distinct
XREF files at 64. Missing, unreadable, changed-during-capture, changed-before-
publication, and over-budget dependencies fail closed. After parsing, byte
buffers are released while source stamps remain for final validation. Ordinary
non-staged imports retain their existing path-based XREF behavior.

A fresh macOS Release CMake build with `DWGSUPPORT` succeeded for
`librecad_offset_hostilefile_tests`. The fixed-path helper was staged beside the
test executable for this run because this CMake test target is emitted at the
build root while the product helper is inside the app bundle. The XREF-filtered
run passed 6 cases / 2908 assertions; the complete hostile-file suite passed
63 cases / 56954 assertions. The assertion count includes scheduling-dependent
worker-process checks. Saved logs under `/private/tmp`:

| W46 artifact | SHA256 |
| --- | --- |
| `offset-W46-xref-release.log` | `a855c2c05ac2401ae8ede45d0bd64368f4a99332243c9743f97f58e0cc7166a1` |
| `offset-W46-hostile-release.log` | `27f8fad958d779c361cf644e69c16c63defac9095ca23ca1462d47b6ceef0e2a` |

This does not make the entire dependency graph an atomic point-in-time snapshot:
each source is captured and checked independently, and identity/timestamp checks
are not a filesystem transaction. The recursive parse remains synchronous.
Direct synchronous/console imports still use the legacy path-based contract;
Windows runtime validation, save/check-to-use and destination publication remain
open. Do not claim cross-platform or ASan qualification from this Release run.

W47 source-save identity guard `14c7cc902` moves the existing import stamp into
shared `LC_FileIdentity` and carries it with `RS_Graphic`. Staged GUI import
keeps the identity captured before parsing; ordinary finalized imports capture
the current source, while new/template drawings clear it. Save detects a source
identity mismatch and routes through Save As. The final writer preflight also
rejects a destination that is the original normalized source path or native file
object when it no longer matches the imported snapshot, including forced Save
As to the old path, hard-link aliases, and a source path replaced by a symlink.
Autosave refuses a hard link to the imported file even after the source pathname
has been replaced. Successful saves refresh the stored identity.

This is still a check-before-use policy, not atomic destination publication: a
path can change after the final identity check and before the exporter opens or
replaces it. Do not describe this as eliminating the check-to-use race. Direct
console/synchronous XREF handling also remains path-based; W46 only snapshots
dependencies in staged GUI imports.

A fresh macOS Release CMake build of `librecad_tests` and a qmake6 `src.pro`
rebuild both succeeded. Offscreen Release results:

| W47 suite | Test cases | Assertions | Result |
| --- | ---: | ---: | --- |
| Document storage/import/save | 36 | 1303 | 35 passed, 1 existing JWW skip |
| Hostile offset-file/import coverage | 32 | 48849 | pass |

The document-storage cases include same-size content rewrite with restored
mtime, refusing the original path after the Save As prompt, replacement of the
source path with a symlink, hard-link autosave alias after path replacement,
source identity refresh after save, and template identity clearing. The hostile
suite requalifies staged import after extraction of the stamp implementation.
Saved logs under `/private/tmp`:

| W47 artifact | SHA256 |
| --- | --- |
| `offset-W47-documentsstorage.log` | `b0b868190bfb13bf3ed3d4a6cd6cc6deaf6eec0f9c2a2380ea3a69b8c1b465f7` |
| `offset-W47-hostile.log` | `aef67d6aec1547c26a3072d98be9342fd821882e4c0c3d2d3a71e29cd5a09691` |

No Windows runtime, sanitizer, or atomic export-race proof is claimed here.

Creation/repeat status after these review follow-ups:
1. Shared owned numeric preparation is implemented/qualified at `ef40eecad`.
   Target-relative source accounting is recorded above; retain upstream #3140.
2. Typed child Produce is implemented/qualified at `558c3bba2`, including private
   parent materialization, diagnostics, copy-series rollback and remaining limits.
   An original-definition rebase followed by the existing stored-cache fallback
   intentionally uses DIFFERENT inputs/remaining allowances; do not replace that
   fallback with ordinary geometry from the rebase invocation.
3. W16/W17 qualify owned cache snapshot/rebinding/merge, cross-source clearing
   and transport across fresh helpers. Retain this budget/side/effort parity during
   GUI integration; leave unrelated ordinary source tools unchanged.
4. W18/W19 qualify preview/commit continuations, complete input/context seals,
   cancellation/death handling, local attribute/provenance policy and one atomic
   publication. Preserve actual display-expansion charging and these regressions
   during remaining caller integration; this closes local creation/repeat routing,
   not platform/deployed/visible acceptance.

Current working-tree recheck: on `ba67b7599` with unrelated uncommitted DWG stream
changes present, the Release source-preparation, creation-action and creation-
publication targets rebuilt and passed offscreen. Discovery matched every test
identity, with no failures or skips: respectively 82 declarations / 88 JUnit
entries / 83527 assertions, 85 / 132 / 90217, and 18 / 19 / 414. Local temporary
JUnit reports `/private/tmp/offset-{source,creation,publication}-current.xml`
have SHA256 `31731e23e828adff980815d47c5f56a27edf9ef2311cc64e026f70a2bf14ea79`,
`bfdbafc0cb12c4df9a34756ac379132206aaaf0a2e3de28736cc327b2ef0d5cd`, and
`1eee2f42f8cdda06059d9294804b8fe539d9d4f838a0bae76db055e4da8490df` respectively.
This is not a clean-commit, ASan, visible-GUI, deployed, or cross-platform run;
the earlier W16-W19 qualifications and their limitations remain authoritative.

Then continue other import callers and cross-platform fault qualification in4.2 order.
Full current-source application/deployed builds, complete file/version matrix,
visible acceptance, source reconciliation/A-C audit and final R remain open.

S0 source/target reconciliation, A-C review, W0-W2 caller integration and R remain
open. Next execute the remaining bounded worker sub-slices in4.2, not a second
implementation of the completed Node runner or helper codecs. Both shared
indexes retain their recorded hashes; all implementation slices and their immediate
plan checkpoints belong only to `codex/offset-2961-native-worker`.

- [x] Read-only reconciliation: identify the later descendant candidate and its
  real entity/checker/UI/file/source/trim code instead of calling it all missing.
- [x] Verify52 committed fixture hashes and classify old14 receipt checks as historical
  and currently unavailable. This is provenance, not runtime/interoperability evidence.
- [x] S0: source/target reconciliation is complete on the durable tree. The selected
  implementation is the clean PR worktree, not the dirty shared master or the separate
  recovery worktree; `codex/offset-2961-implementation` is an ancestor of it. The exact
  product-source pin, PR base, preserved unrelated trees, and plan-only delta are in W153.
  The R2007 classification fix, baseline/discovery, 52-fixture provenance, and path/link
  inventory are recorded above. Do not redo implemented geometry as missing work.
- [x] A-core through C-check local review: W155 maps shared budget ownership and
  strict-kernel counter transfer across the public reference/cache/topology chain;
  the fresh full geometry selector passes 373 cases / 1,576,298 assertions. Existing
  W120 Release and W152 ASan fault sweeps cover 29,117 injection/recovery points, and
  the audited implementation/test files are unchanged from that source pin. No new
  local defect was found. Remote platform execution and final R acceptance remain open.
- [ ] W0-W2: typed/cached Produce, creation/repeat and local native Trim GUI routing and scoped action faults are qualified;
  a fresh macOS qmake6 full-app/bundle build, adjacent-helper worker smoke and paired
  cache/creation Release-ASan checks are qualified in W55-W57; asynchronous Trim test
  synchronization is corrected and requalified in W58-W61. W79 adds successful full CMake
  installation plus installed-directory worker-client and creation/repeat routing tests.
  W154 adds a component-only staged helper/client smoke to Linux/Windows CI.
  The local macOS staged/relocated client lane passes, while remote results are pending.
  W156 routes public synchronous `fileImport`, `fileImportStream` and
  `fileImportBytes` through deferred parsing and bounded sibling-worker validation,
  sharing import-budget state with XREF child filters and restoring deferred INSERT
  expansion. The caller remains synchronous through a bounded nested event loop;
  GUI open/templates retain their asynchronous staged session. Current-source
  Release and focused ASan direct-API selectors, broad Release format lanes, and
  qmake6 builds of both changed filter objects pass locally. W178 now adds a fresh
  current-source qmake6 full application/helper bundle build and a bundled sibling-
  worker client smoke (17 cases / 3,775 assertions). Finish Windows/native
  cross-platform failure coverage, deployed-helper acceptance, and final
  caller/adoption/failure acceptance. W170 passes the targeted local Debug,
  Release and ASan worker fixture suites. W171/W172 freshly qualify current-source
  DWG/DXF matrices with LibreDWG and ACadSharp. On code-equivalent head
  `c0438532b`, Windows ARM64 job `123816799173` passed all bounded tests and
  relocated-worker checks. The old workflow `40979668978` was canceled before
  Linux/x64 completed. Product source `7bd61bd24` remains under DWG/DXF workflow
  `40998099806`; W180 records all three platform tests active after successful
  builds. The W176-W180 roadmap-only commits are now on PR head
  `498a5deee690187a50ad776ad2e5df63fc51f1c5`; exact-head DWG/DXF dispatch
  `41003855350` is now active on the current head, and exact-head Pixi
  `41003830885` is active. The older DWG/DXF run was canceled during its test
  step by the concurrency handoff. Its partial logs show CTest 23/23 and
  `[sync-api]` / `[creation-worker]` passes (219 / 318 assertions) on all
  platforms, but no terminal workflow result. Prior Pixi runs `40979669043` and
  `40998099821` passed all five platforms. The CTest
  selector includes source identity. W175 aligns the
  Linux bounded lane with the Windows Release configuration without reducing
  the exhaustive test set; the next exact-head run must verify it. W177/W179
  independently convert all 34 current-source DXF matrix rows through ODA; see
  the detailed reader classifications below. Reuse the
  qualified geometry-entry
  W157 extends the DWG/DXF matrix to build and execute `[sync-api]` on Linux and
  Windows x64/ARM64; the prior code-equivalent ARM64 job passed, while the new
  exact-head Linux/x64/ARM64 run remains active. Reuse the qualified
  geometry-entry timeout/helper/client/reply
  modules; do not redo them as absent. Keep Node runner scope separate.
- [ ] E/U/G/F/O/T: requalify and integrate the existing entity, UI, creation,
  DXF/DWG, source and trim implementation with any new worker/publication changes.
  W171/W172 close fresh current-source external semantic-reader matrices for the
  direct DWG and DXF writer paths; ACadSharp's AC1021 pre-entity exception is
  recorded separately. Visible CAD interaction, full platform/package coverage,
  and final integrated acceptance remain open.
- [ ] R: finish current-source Release/ASan/qmake6/failure/version/visible rows,
  including Windows, visible GUI acceptance and remaining platform/package acceptance;
  W178 closes the fresh macOS qmake6 app/helper build row, but not these remaining gates. W79
  now covers a complete staged macOS CMake install. W149-W151 record broad local
  Release results and classify the five randomized alias assertions as the same
  inherited behavior on a fresh `origin/master` build; do not attribute them to
  the offset changes. W170 adds targeted current-source Debug/Release/ASan worker
  fixtures and a full 6,381-case headless Release pass; W171/W172 add fresh
  current-source cross-reader DWG/DXF matrix results; W177/W179 route all 34
  current-source DXF rows through ODA, and W178 adds a fresh current-source
  qmake6 app/helper build. The c043 workflow
  `40979668978` passed Windows ARM64 and was canceled before Linux/x64 completed;
  product-head DWG/DXF workflow `40998099806` was canceled in its test step
  after all builds succeeded. Its partial logs report CTest 23/23, `[sync-api]`
  219 assertions, and `[creation-worker]` 318 assertions on all platforms; the
  cache/fault and relocated-helper lane did not finish everywhere. Current
  PR-head exact DWG/DXF run `41003855350` and Pixi `41003830885` are active.
  Prior Pixi runs `40998099821` and `40979669043` passed on all five platforms. CodeQL
  `40973686197` passed on the preceding
  product-equivalent head `4df7229ca`; this test-only follow-up did not trigger a
  new CodeQL run. Review scoped commits, preserve reproducible evidence and close
  every required gap.

**Next concrete action on implementation resume:** follow all checks on the
current PR head through completion; address any Windows/Linux worker, direct-
import, or staged-helper failure; then close remaining W0-W2 deployment and
failure gates, requalify E/U/G/F/O/T on the exact source, and complete R's
visible-GUI, external-CAD, platform/package and final closeout rows. Current
AC1021 DWG output has passed direct semantic inspection with LibreDWG; the pinned
ACadSharp `ComprLen` exception remains a tool-specific limitation and must not
be represented as a LibreCAD failure or as direct ACadSharp acceptance.
The W145 Windows include correction has local validation only. W55 now records a fresh full macOS qmake6 app/bundle
build plus a worker-protocol smoke using the helper beside the bundled executable;
W79 separately records a complete staged macOS CMake install and offscreen
installed-directory tests. Neither establishes Windows behavior or visible GUI
acceptance. W46 already bounds and
snapshots nested XREF bytes for staged imports; W47 adds source identity
preflight but not atomic publication. W49a-W49c add a transaction guard, per-call
DXF/DWG APIs, and source-identity checks at filter publication boundaries.
These narrow but cannot eliminate the check-to-rename window. A deterministic
backup-restore failure test now passes on macOS; obtain Windows compile/runtime
evidence for native rename/share behavior. W51 captures drawing identity before
`LC_DocumentsStorage` starts its ordinary path-based import and preserves that
identity through finalization, so an identity observed only after a pathname
replacement is not accepted as the imported source. This is conservative if
the path changes before the reader opens it; the source-save guard then requires
Save As. W52 now binds direct parser bytes to the already-open file descriptor,
including console and recursive XREF imports through this filter. The read is
still synchronous. W53 revalidates all opened host/XREF identities after the
root import, including mutations to an earlier dependency while later XREFs
load; this is not an atomic multi-file snapshot. Obtain Windows compile/runtime
evidence for QFile handle identity and rename behavior. W54's current-source
serial ASan rerun did not reproduce the template assertion, but the concurrent
run cause remains unknown. Exercise a genuine partial DWG through the GUI
dialog. W35 fixes the
observed W33 teardown assertion; retain its
forced-destruction regression while auditing other close/dialog reentrancy.
W36/W37 handle observed switches and later staged requests; also audit
same-window interaction, reentrant/visible startup focus and failure behavior.
W31/W32 drawing-open/template publication and W33 GUI queue bounds are
implemented; do not repeat them as absent. Review the remaining cross-file
time-of-check limits without weakening editable imports.
Qualify remaining platform faults without weakening the existing worker tests.
W58 isolated two stale synchronous assertions in native Trim tests; W61 fixes
their event-pump/wait contract and requalifies the affected ASan lane. W59's
full randomized fast-lane run reports five assertions in two `[commands]` alias
cases. W150 passes the isolated selector in three seeds, and W151 reproduces
the same five assertion failures on a fresh `origin/master` build. Classify this
as inherited command-test ordering/state behavior; do not change unrelated
command behavior or waive the offset-specific gates.
W16-W19 cache portability and local creation/repeat GUI
routing are qualified; do not restart them as absent. The ordinary DWG2007 correction and fresh broad
baseline are already recorded; do not restart them as missing implementation.
Nothing is automatically copied into the dirty main tree. Other import callers,
remaining lifecycle/packaging gates and final UI/file acceptance
are open.

Completion requires every required deliverable and validation row on the selected
source, no unhandled type/lossy silent fallback, no pending P1/P2 finding, and an
accurate immediate post-commit roadmap. Earlier scoped COMPLETE labels, build
success, hash equality, Node runner reports or a narrowed test subset cannot close
this plan. Keep explicit unsupported platform/manual-tool limitations; do not
mark a mandatory row PASS merely because its prerequisite is unavailable.

The original plan-only audit did not implement product changes or commit the
existing staged draft. This execution now implements the roadmap in the isolated
worktree above. Both shared indexes remain untouched; the prior draft archive is
currently unavailable, as explicitly recorded in the current S0 audit.

## 8. Historical Plan-Only Audit

Document checks cover all 27 unique slices, known/ordered predecessors, no cycles,
and every slice reaching final R. Negative controls reject a duplicate slice,
missing trim slice, unknown predecessor, dependency cycle and reversed order.
The selected SHAs/ancestry, candidate source and target presence, real acceptance
tags, explicit version enums and surviving DWG2007 defect are checked against
source. Shell recipes are syntax-checked without running product commands.

That earlier pass reported matching14 receipt hashes and52 fixture hashes, with
source blobs checked against their recorded commits and no runtime PASS transfer.
Its clone, untracked attic draft and temporary receipt/report paths are now absent;
the original machine-readable report path was
`/private/tmp/lc2961-plan-refresh.NI3tkd/report.json`, also currently absent. Current
S0 checks use the surviving Git objects and52 committed fixtures instead. Preserve
the historical observation without asserting that unavailable bytes still exist.

The other earlier document-only audit's recorded path was
`/private/tmp/lc2961-plan-audit.gRHoJ7/report-final.json`, also currently absent;
it qualified its own draft
structure, not this newer candidate reconciliation. Neither audit qualifies
geometry, native workers, current DXF/DWG interoperability, visible interaction
or a product milestone. Those document-only audits did not run product builds or
tests. Subsequent executions and their exact scopes are in the current ledger.

## 9. Preserved Validation-Runner Contract

These original H6 expectations are copied verbatim from the candidate roadmap
at `df0c891ad5a0bb5b0d0e864f4cd9cdd8036d5f83`. Stable delimiters decouple
execution from heading numbering; they do not relax any original expectation.

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

## 10. W48 Preview Portability and Creation/Repeat Requalification

W48a `ea25396f4` adds an end-to-end regression for action-wide cache eviction
through a real fresh-helper CachedProduce request and parent merge. The parent
starts at the 256-entry boundary with one source entry and 255 entries belonging
to other sources; the worker computes a new entry, reports the global clear, and
the joined parent merge drops all stale-source entries while retaining the new
source result. This closes the process-boundary/global-clear test gap; it does
not change W16/W17 cache semantics or authorize raw previews as native curves.

Current-source macOS Release and ASan runs both discover and pass the two
`[preview-portable]` worker-client cases with no failures, errors or skips. The
new case is present in both runs and uses the built helper executable. ASan is
configured with address instrumentation and `detect_leaks=0`.

The Modify Offset creation/repeat path was requalified without duplicating its
W19 implementation. Release and ASan both discover and pass all 13
`[creation-worker]` cases with no failures, errors or skips. These exercise
worker-routed preview/commit/repeat, cache-hit reuse, native zero-sum fallback,
mixed copies, cancellation and publication invariants. This is offscreen GUI
test coverage, not a visible interactive acceptance run.

| W48 receipt | Test cases | JUnit `tests` | Result | SHA256 |
|---|---:|---:|---|---|
| Cache process Release | 2 | 709 | PASS | `bbbd2f838bdd6afc0efec5350e909ed36a4f70fbd17f8078107b7ebb1eb1a32d` |
| Cache process ASan | 2 | 1053 | PASS | `b40319d5179144bd4a942c182ae3c22034b7674493bd92947d7cf18162f4a62b` |
| Creation/repeat Release | 13 | 318 | PASS | `df93bf12c2dda78d5e72d2fdf7e401d01c29849ba3ce706d22ffb0ee838e52f7` |
| Creation/repeat ASan | 13 | 318 | PASS | `74ef453ed7100b26ee5e1c4537fb44473a84ae919e9d0acf31bdffc579d94395` |

Receipts are under `/private/tmp/offset-W48-*.xml`. ASan leak detection remains
disabled and is not a leak-free claim. This host validates macOS only; Windows
compile/runtime, installed-bundle execution, qmake6 deployment, and visible GUI
acceptance remain open. Continue with the existing W44 cross-platform gate and
the W47 destination-publication/source-path-race work; do not reopen W16-W19 as
missing implementation.

## 11. W49 Destination Publication Guard

W49a `844199ab5` adds an optional callback to
`DwgDxfOutputTransaction::commit` and `commitVerified`. Publication checks the
staged-file identity, calls the guard immediately before replacement, and
checks the staged identity again. A guard rejection or exception aborts and
removes only the private stage. The regression mutates the destination during
staged-file verification, then proves the publication guard rejects the stage,
preserves the external replacement, and leaves no temporary file.

This is transaction-level coverage only. It does not remove the filesystem
check-to-rename race, bind a pathname identity atomically to replacement, or
yet protect LibreCAD exports; those require the explicit libdxfrw writer APIs
and `RS_FilterDXFRW` integration in W49b/W49c.

| W49a validation | Result |
|---|---|
| `librecad_output_transaction_tests` | 15 cases, 137 assertions passed |
| `libdxfrw_verified_write_tests` | 4 cases, 299 assertions passed |
| Release build: `librecad_tests` and both focused test binaries | passed |
| `git diff --check` | passed |

The command `ctest -R ...` found no registered CTest cases in this build; the
two Catch2 executables were run directly. No ASan, Windows, or visible GUI run
is claimed.

W49b `5306c02bb` adds `DRW_OutputPublishGuard` and explicit `writeGuarded` plus
guarded `writeVerified` overloads to `dxfRW` and `dwgRW`; existing writer entry
points and `DRW_Interface`'s virtual layout remain unchanged. Both writers pass
the guard through to the staged transaction. Native fixtures exercise allowed
and rejected publication for AC1027/AC1032 DWG and ASCII/binary DXF, both with
and without the staged verifier. The verifier reparses the real staged file;
rejection preserves the original destination and leaves no stage behind.

| W49b validation | Result |
|---|---|
| `libdxfrw_verified_write_tests` | 5 cases, 468 assertions passed |
| Release build: `libdxfrw_verified_write_tests` | passed |
| `git diff --check` | passed |

No ASan, Windows, or visible GUI run is claimed. The W49c wiring and source
replacement regressions follow; the outer-DWG timing/backup-failure cases and
the final OS check-to-rename race remain explicitly open.

W49c `f69c034e7` captures the drawing's imported `LC_FileIdentity` at export
start, classifies the destination by normalized source path, canonical path, or
native file identity, and rejects a stale source before serialization. The
guard is passed into DXF publication and the inner DWG staged write; DWG also
rechecks immediately before moving the existing destination to backup. It does
not change `DRW_Interface`'s vtable. Tests replace the source from the DXF/DWG
finalization callback and prove publication refuses while preserving the
external bytes. Hard-link aliases are rejected at preflight without touching
either link.

| W49c validation | Result |
|---|---|
| `librecad_tests [source-identity]` | 3 cases, 115 assertions passed |
| `librecad_tests [export-commit]` | 5 cases, 86 assertions passed |
| `librecad_output_transaction_tests` | 15 cases, 137 assertions passed |
| `libdxfrw_verified_write_tests` | 5 cases, 468 assertions passed |
| Release build: `librecad_tests` | passed |
| `git diff --check` | passed |

W49d `b69f20671` adds a one-shot private fault hook immediately after a
successful low-level DWG stage publish and before the outer destination check.
The real export test replaces the source at that point, confirms the outer
check rejects before backup, preserves the external bytes, and leaves no stage
or backup artifact. It passes 1 case / 7 assertions in the Release
`librecad_tests [outer-publish]` run.

W49e `1ba774225` adds a second one-shot test hook after the existing target is
moved to backup and before the stage rename. Removing the private stage at that
point forces the actual rename failure path; the test confirms the old
destination is restored byte-for-byte and the backup/stage names are cleaned
up. Combined W49d/W49e Release tests pass 2 cases / 14 assertions.

The identity guard cannot atomically bind the final pathname check to
replacement. Backup restoration is fault-injected and verified on macOS, but
Windows compile/runtime and native rename/share behavior remain open. No ASan,
Windows, or visible GUI run is claimed.

## 12. W50 Current-Tree Cache and Creation Requalification

After W49d/W49e, rebuilt the current Release source and reran the portable
preview-cache and local Modify Offset creation/repeat lanes offscreen. The real
helper is used by the process/client and creation tests. All five JUnit reports
have zero errors, failures or skips:

| W50 suite | Testcases | JUnit `tests` | Result |
|---|---:|---:|---|
| Worker process cache portability | 1 | 186 | PASS |
| Worker client cache portability | 2 | 669 | PASS |
| Worker kernel cache ownership/merge | 4 | 156 | PASS |
| Worker result transport | 1 | 18 | PASS |
| GUI creation/repeat | 13 | 318 | PASS |

Receipts are under `/private/tmp/offset-W50-*.xml`:

| W50 artifact | SHA256 |
|---|---|
| `offset-W50-cache-process.xml` | `616cfd52d16121ff93b49c0add79acca9437d1f0024c291cfd6eecb969d03f32` |
| `offset-W50-cache-client.xml` | `4160d7b2bfc4824129c149b864c87e4bb54c05055f17ec61eca0f22d59dd0a87` |
| `offset-W50-cache-kernel.xml` | `e71893d01407f5bf1d121767bdb4ebb2e580f14a587a2106493c12f5660ef40f` |
| `offset-W50-cache-result.xml` | `8461f01de35536e8fd7218cea9df91c9ad9f9f906ea9ceda8e730179e200aeb5` |
| `offset-W50-creation-repeat.xml` | `d010dec1b99418c10a0af04cda67c29f0c26064244ceb4ca42032db3ed7e938e` |

This is current-source macOS Release/offscreen evidence, not a refreshed ASan,
visible interactive, deployed-bundle, or cross-platform run. Prior W48 ASan
receipts remain as recorded; W49 changed only publication/filter code, not the
preview-cache or creation/repeat implementation.

## 13. W51 Pre-Import Source Identity

Commit `d7bfcd33c` captures `LC_FileIdentity` at the start of
`LC_DocumentsStorage::loadGraphic`, before `RS_FileIO` begins parsing, and
retains that identity through `finalizeLoadedGraphic` when it still refers to
the requested logical path. Other callers that do not supply an earlier
identity keep the previous finalization behavior. If the path is replaced
during import, finalization no longer stamps the replacement as the source;
the existing save guard requires Save As rather than overwriting that
replacement. Capturing before open is intentionally conservative if a source
is replaced before the filter opens it.

The Release build incrementally rebuilt `librecad_tests`. Offscreen
`[documentsstorage][import]` passed 11 JUnit testcases / 153 assertions with
zero errors, failures or skips. The focused `[documentsstorage][import][source-change]`
run passed 7 testcases / 103 assertions. The new regression retains the
pre-import file identity across pathname replacement and verifies that a
subsequent save leaves replacement bytes intact and requests Save As.

JUnit receipt: `/private/tmp/offset-W51-documents-import.xml`, SHA256
`8d019add1baef0393755fbbe244b6aba7ccda8319d2a86d36983a2a1874eaf45`.
This is a caller-level source-stamping fix; descriptor-bound parsing is recorded
in W52 below. No ASan, Windows, or visible-GUI validation is claimed.

## 14. W52 Descriptor-Bound Direct Imports

Commit `df9bbff40` routes ordinary `RS_FilterDXFRW::fileImport` reads through a
seekable 64 KiB `QFile` stream buffer instead of reopening the pathname inside
libdxfrw. It captures file identity before open, verifies the native handle
against that identity where supported, and checks the opened handle again after
parsing. Read errors or detectable in-place changes fail with a source-changed
diagnostic. Path replacement cannot redirect the parser to the replacement
file after open; the DWG regression now verifies the original version was
parsed and the changed identity caused rejection. Direct synchronous/console
imports and recursive XREFs that use this filter inherit the per-file behavior.

Current macOS Release/offscreen validation, all zero errors/failures/skips:

| W52 suite | Testcases | JUnit `tests` | Result |
|---|---:|---:|---|
| Source identity and opened-path replacement | 4 | 124 | PASS |
| Document import/finalization | 11 | 153 | PASS |
| DXF round trips | 162 | 5880 | PASS |
| DWG round trips | 28 | 2658 | PASS |

JUnit receipts are in `/private/tmp`:

| W52 artifact | SHA256 |
|---|---|
| `offset-W52-source-identity.xml` | `351a264818067e3d28da739d1e87a024c3bb4d18c8aab0707d163ac25937614a` |
| `offset-W52-documents-import.xml` | `61f8b547f18b3b501634025f861702c1baef02a86fd56e3a68b6f6686012831f` |
| `offset-W52-dxf-roundtrip.xml` | `be61fd73575069d405c4f1d12f47ebf2a33aee37cecef561e8fc8d933d55d3a9` |
| `offset-W52-dwg-roundtrip.xml` | `53f26a7e83c34bc777171d46a7358c861163bb42c0450933c6f91a40d0ccc4ad` |

This is per-file descriptor binding, not an atomic snapshot spanning the host
drawing and its separately opened XREF dependencies. Parsing still runs on the
calling thread. Windows native handle/rename behavior, ASan, deployed packaging
and visible GUI/failure acceptance remain unqualified.

## 15. W53 Direct XREF Revalidation

Commit `b1e6145ae` shares a direct-import identity ledger with recursive XREF
filters. Each path is opened and parsed through W52's bounded stream, checked
against its opened descriptor after parsing, then all captured identities are
revalidated against their paths after the root import completes. A changed
dependency therefore rejects the host import even when it changed after its
own parse while a later XREF was loading. The behavior is per-file stability,
not an atomic snapshot of all files at one instant; parsing remains synchronous.

Current macOS Release/offscreen validation, zero errors/failures/skips:

| W53 suite | Testcases | JUnit `tests` | Result |
|---|---:|---:|---|
| Direct XREF stable/change/cross-change | 3 | 153 | PASS |
| Staged XREF snapshot and limit regressions | 13 | 2874 | PASS |

JUnit receipts are in `/private/tmp`:

| W53 artifact | SHA256 |
|---|---|
| `offset-W53-direct-xref.xml` | `9b7380d821a251ab6ca4b6c43eaaac1c8f1c812c4fcea794538dc17b76de59dd` |
| `offset-W53-staged-xref.xml` | `f8924e401e99ae14390ee6c002adb6b755304b32afa604c5c8f8c524fba32f5b` |

No Windows, ASan, visible GUI, or multi-file atomicity claim is made.

## 16. W54 Current-Source ASan Requalification

Rebuilt the ASan `librecad_tests` target against W52/W53. The historical
native-template assertion did not recur in the serial `[dock-window]` run or
the combined serial run covering dock/template, source identity, direct/staged
XREF and document-import paths. The combined run passed 93 JUnit testcases /
4538 assertions with zero errors or failures. Seven screen-size-dependent dock
layout cases were explicitly skipped. The import/source selection separately
passed 28 testcases / 3360 assertions with no skips. ASan used
`detect_leaks=0`; this is not a leak check. The earlier concurrent-run failure
remains unexplained because this requalification was serialized.

| W54 artifact | SHA256 |
|---|---|
| `offset-W53-dock-asan.xml` | `ad14ef8ae555ba85e0557a67957986dc94156e5d90ac471701926fddb80e004f` |
| `offset-W53-import-asan.xml` | `b66ad2cd31fb9cd34f0992ef06bbc9378ce0e8a46176ad5750c07bd2922cf0c3` |
| `offset-W53-serial-asan.xml` | `b6ba83d11a0ca6c849558a884afa24cb424cb09c4e2db53b066142f764a25d85` |

These are current-source macOS sanitizer results, not Windows, concurrent-load,
leak, visible-GUI, or full-suite acceptance. The original concurrent assertion
still needs a controlled reproduction/root-cause before it can be closed.

## 17. W55 Fresh qmake6 Bundle and Helper Smoke

Configured a fresh out-of-source build with qmake 3.1 / Qt 6.12.0 using
`qmake6 /Users/dli/.codex/worktrees/offset-2961-runtime/LibreCAD/librecad.pro`
in `/private/tmp/offset-W55-qmake6`, then completed the top-level `make -j4`.
The result is `/private/tmp/offset-W55-qmake6/LibreCAD.app`, containing both
`Contents/MacOS/LibreCAD` and the application-adjacent
`Contents/MacOS/LibreCADOffsetWorker`. This compiles and links the qmake6 app,
helper, and plugin targets; it is a local macOS build, not a Windows or release-
packaging claim.

For an application-directory helper smoke, ran the current-source W54 ASan
`librecad_tests` binary from the bundle's `Contents/MacOS` directory beside the
fresh qmake-built helper, with `ASAN_OPTIONS=detect_leaks=0`. The `[worker]`
selection completed 13 testcases with JUnit `tests=3780`, zero errors,
failures, or skips. The tests therefore resolved and exercised the actual
application-adjacent qmake helper through worker requests, rather than only
checking that the file exists.

| W55 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W55-bundled-worker.xml` | `39e567635eb9a00a7d20f9f3c310bb9aaefd04c70ef1956d831ed1c2a75cbe2b` |

The bundle smoke does not establish codesigning/notarization, distribution
installation, visible GUI interaction, Windows QFile/native rename semantics,
or multi-file atomicity. Those remain open acceptance items.

## 18. W56 Current ASan Cache and Creation Tests Against qmake Helper

Reused the W54 current-source ASan `librecad_tests` binary as the test runner,
placed it in the fresh W55 bundle's `Contents/MacOS` directory so production
application-directory discovery resolves the qmake-built
`LibreCADOffsetWorker` beside it. The test runner is ASan-instrumented; the
qmake helper is the ordinary Release binary. With `ASAN_OPTIONS=detect_leaks=0`,
the focused requalification passed:

| W56 suite | Testcases | JUnit `tests` | Errors / failures / skips | Result |
|---|---:|---:|---:|---|
| `[preview-portable]` | 4 | 67 | 0 / 0 / 0 | PASS |
| `[creation-worker]` | 13 | 318 | 0 / 0 / 0 | PASS |

Both suites exercise the real adjacent helper path; the creation tests cover
worker-routed preview, commit, repeat, cache reuse, fallback and publication
invariants. The cache portability tests cover decoding/rebinding to fresh
owned sources. These are ASan caller/test-runner checks against an uninstrumented
qmake helper, not ASan coverage of the helper process itself or leak validation.

| W56 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W55-preview-portable-asan.xml` | `04a7dd4c45b808d523f9f928484ff058cd4794fcbe5911f9d5d0dbe9ec40538c` |
| `/private/tmp/offset-W55-creation-worker-asan.xml` | `5fe52feffba9e98afff426f5f3c96f3b0454480fa741e9c85cf89ab9f6b6cdb3` |

Visible interactive GUI acceptance, Windows behavior, packaged installation,
and full-plan integration/failure acceptance remain open.

## 19. W57 Release Cache and Creation Tests Against qmake Helper

Repeated the W56 focused suites with the current CMake Release test runner,
also placed beside the fresh W55 bundle helper so `applicationDirPath()`
resolves the qmake-built `LibreCADOffsetWorker`. This pairs Release and ASan
test-runner coverage against the same helper binary and discovery location:

| W57 suite | Testcases | JUnit `tests` | Errors / failures / skips | Result |
|---|---:|---:|---:|---|
| `[preview-portable]` | 4 | 67 | 0 / 0 / 0 | PASS |
| `[creation-worker]` | 13 | 318 | 0 / 0 / 0 | PASS |

| W57 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W57-preview-portable-release.xml` | `48e2dff7de69234bf8001afde6b307d5498dfc0517fb049b3f38630c03c4141e` |
| `/private/tmp/offset-W57-creation-worker-release.xml` | `b6868514f6bdbc46363f66b616523bdee3973207c56795d4b0fee595c56c858d` |

The Release runner was built from the current worktree after W53; later W54-W56
changes were roadmap-only. XML receipts pass `xmllint --noout`. These offscreen
checks still do not substitute for visible interaction, Windows runtime, or
installed-package acceptance.

## 20. W58-W61 Broad Release and Async Trim Requalification

The first full Release fast-lane run (W58) executed 6,272 Catch testcases with
26 explicit skips. It found two failures in existing native Trim action tests:
they asserted immediately after routing through the asynchronous native worker.
Both failures reproduced when run alone, so they were not random-suite noise.
The test probe now pumps Qt events while `hasPendingNativeTrim()` remains true,
with a bounded 35-second wait before the preview-fragment and committed-line
assertions. Production Trim behavior is unchanged.

After the test fix, the same full Release fast-lane selection (W59) again
executed 6,272 cases with 26 skips. The two Trim failures are gone. The JUnit
suite reports `failures=5`; its failure nodes identify the two `[commands]`
cases "Line gap commands start Line Gap, not Break/Divide" and "Line gap
aliases survive saving and respect user overrides". Running `[commands]`
alone (W60) passes 11 cases / 69 JUnit tests with no failures or skips,
indicating an order-sensitive global alias-state issue in the broad process.
No command code was changed and those failures remain open for baseline/order
classification; they are not counted as an offset pass.

The affected Release cases pass after the bounded wait. A current-source ASan
rebuild then passed all five `[lc-offsetcurve][types][trim]` cases (7,176 JUnit
tests) with zero errors, failures or skips. ASan used `detect_leaks=0`; this is
not leak validation.

| W58-W61 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W58-release-fast-lane.xml` | `c700dea53e4c6eebaee0e75d70977c53411f44773bd6a7454eb4dfa3c4afd5b8` |
| `/private/tmp/offset-W59-release-fast-lane.xml` | `8f51e2da5d622180d31bafe1f2bdd71182b6e2cb4227d5b84a8791bce61afd2c` |
| `/private/tmp/offset-W60-commands.xml` | `21d0c647b90a23e8530c48b2f7a7424a923b86904b334a0a2c9eec5e95d614a8` |
| `/private/tmp/offset-W61-trim-types-asan.xml` | `a1665274ed4e2eb8b26b9545e33aba4284268b300114d03b0466dce1656443e7` |

All four reports pass strict `xmllint --noout`. W59 is explicitly not a clean
full-fast-lane PASS: close the order-sensitive command alias issue or compare
it against a fresh baseline before final R acceptance. Windows, visible GUI,
installed packaging and the remaining worker/import gates are still open.

## 21. W62 Isolated Command-Alias Order Check

To separate the W59 broad-process failures from the command tests themselves,
ran the `[commands]` selection in three fresh processes with Catch randomized
ordering and seeds 1, 2 and 3. Each run passed 11 testcases / 69 JUnit tests,
with zero errors, failures or skips. All three XML reports pass strict
`xmllint --noout`.

`librecad/src/cmd/rs_commands.cpp` and
`librecad/src/cmd/tests/rs_commands_tests.cpp` have no diff against
`origin/master`. This establishes that the tested command implementation and
test definitions are baseline-identical and that the isolated selection passes
these three orders. It does not establish whether the failures in W59's full
process are pre-existing or caused by interaction with other tests; the two
reported command-alias failures and the non-clean broad fast lane therefore
remain open for R acceptance.

| W62 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W62-commands-seed1.xml` | `5be463d531081975b243d6b3352b8b2a00246065973f4da5529e43fbf33a222a` |
| `/private/tmp/offset-W62-commands-seed2.xml` | `cd3b1561c4fd3224c83e452cf7440588b0fd107a57761b2f48d0f768c98e2062` |
| `/private/tmp/offset-W62-commands-seed3.xml` | `9b9eb1fb55794a33117b47f00287747f14a380995ec7465d2a521ca64b0ad141` |

## 22. W63 Shared-Budget Core Requalification

Re-ran the focused `[lc-offsetcurve][budget-core]` selection against the current
tree with the Release runner and the ASan runner. Both runs passed the two
sticky-refusal/shared-budget cases (3,942 JUnit assertions) with zero errors,
failures or skips; both reports pass strict `xmllint --noout`. These cases
exercise pre-spent exact-cap adoption, alias-safe counter snapshots and
terminal latches without refunding admitted work. This is focused requalification,
not completion of the required A-core source audit or A-strict/A-reference/A-cache.
ASan used `detect_leaks=0`, so it is not a leak check.

| W63 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W63-budget-core-release.xml` | `4ced959a310c99e22f06fe12a475b44ea95823b1d48a18cb4b41577010855084` |
| `/private/tmp/offset-W63-budget-core-asan.xml` | `dc5607f1c217a13fa714fa74844c214f53410bb14bb162d8f1d39d0e71c23cd7` |

## 23. W64 Strict-Query Budget Requalification

Reviewed and reran `[lc-offsetcurve][geometry][strict-budget]` with Release
and ASan runners. Both passed 20 testcases / 876 JUnit assertions with zero
errors, failures or skips. The source path was checked for the high-risk ordering
invariants: mandatory seed reservations precede segment callbacks; queue growth
is bounded before insertion; event records are charged before append; retained
session roots receive their separate metadata debit; and provider exceptions
invalidate retained proof credentials while the exit ledger preserves work.
No new code defect was demonstrated, so this slice is requalification rather
than an implementation change. A-core/A-strict full cross-entry and allocation-
failure audit remains open. ASan used `detect_leaks=0`, not leak validation.

| W64 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W64-strict-budget-release.xml` | `5c4098717c03d69f45a6f30a97b90daa1a4f68029c50f36fcb1200141989a5ae` |
| `/private/tmp/offset-W64-strict-budget-asan.xml` | `69d0485fa7d6e1ca3e3367ca6f0a07ce639557f79ff71fac0aa86da78bd20850` |

## 24. W65 Reference-Accounting Requalification

Re-ran `[lc-offsetcurve][geometry][reference-accounting]` with the current
Release and ASan runners. Both passed eight testcases / 2,873 JUnit assertions,
with zero errors, failures or skips. The checked cases include pre-spent and
one-short ordering allowances, fixed-scan debit-before-iteration, refusal
counter stability, and all-or-nothing retained occurrence inventory admission.
This does not close the separate paired-reference graph publication audit or
the full A-reference source review. ASan used `detect_leaks=0`; it is not a leak
check.

| W65 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W65-reference-accounting-release.xml` | `eb124f18364bc1f6a86185bf62357a4f32b8234f117176be0d063096a96139fc` |
| `/private/tmp/offset-W65-reference-accounting-asan.xml` | `060d4303d93901d01029d57c055341d3f9704375840987d7660f9662beca02a6` |

## 25. W66 Cache-Budget and Failed-Stage Requalification

Re-ran `[lc-offsetcurve][geometry][cache-budget-accounting]` with Release and
ASan runners. Both passed 51 testcases / 8,677 JUnit assertions with zero
errors, failures or skips. The source review confirmed retained critical
occurrences are staged into a separate vector and published only after every
occurrence maps and images successfully; entered depth remains charged on a
later failed stage, while previous snapshots stay unchanged. Shared budget
identity and sticky resource refusal remain enforced. This is focused A-cache
requalification, not completion of A-cache source audit or C-search/C-check.
ASan used `detect_leaks=0`, not leak validation.

| W66 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W66-cache-budget-release.xml` | `a496740a62bb4db96bdfa6931862a6e64929cbd1c7e41b1770389d6570939288` |
| `/private/tmp/offset-W66-cache-budget-asan.xml` | `8e377fca524fb73b55bade949b8c7b9e52adbedfc0615b82dca975457bcb044d` |

## 26. W67a Current-Source Preview-Cache Portability Requalification

Rebuilt and reran the value-cache and fresh-helper transport suites against the
current product tree in macOS Release and ASan configurations. Source preparation
passes 88 JUnit cases / 83,527 assertions; the real helper-process suite passes
4 cases / 2,696 Release assertions and 3,090 ASan assertions. All six runs have
zero errors, failures or skips. These cover owned pointer-free snapshots,
source rebinding, cache hit/refusal and budget behavior, and process-portable
cached Produce, including native adoption still requiring its normal checks.
ASan used `detect_leaks=0`; it is not a leak check. This is focused current-source
requalification, not cross-platform runtime, deployed-helper, or visible GUI
acceptance. The creation/repeat GUI action is a separate W67b slice.

| W67a artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W67-source-release.xml` | `d373871dbc1e6484a338123f208f90ed4e0d636304c591f6944de2df40a8c74d` |
| `/private/tmp/offset-W67-worker-process-release.xml` | `f1d122253ec9528a0e5ba947efe94315c8e857d6d5b6f997967523a7df76bfed` |
| `/private/tmp/offset-W67-source-asan.xml` | `6f808536a2abf8a663b16c29d8aebe7ae24f5edbbd4ffc7dda0bd0e1a7288762` |
| `/private/tmp/offset-W67-worker-process-asan.xml` | `d6684555cc075829ab117853e033bdddf26957d1008e12133b45de898459470d` |

## 27. W67b Current-Source Creation/Repeat Routing Requalification

Rebuilt and ran the complete offscreen Qt creation-action suite against the
current source in Release and ASan. Both pass all 132 JUnit cases / 90,217
assertions with zero errors, failures or skips. Coverage includes worker-backed
preview/cache reuse and one-cycle publication, native repeated offsets and the
zero-sum stored-cache fallback, ordered mixed sources, attributes and failed
selection, stale-input/context seals, cancellation and real helper timeout/retry,
two-click ellipse behavior, relative-zero undo, and DWG identity policy. This
confirms local GUI action routing under the offscreen harness; it is not a visible
interactive desktop run, installed-package validation, Windows runtime, or a
full application test. ASan used `detect_leaks=0`; it is not a leak check. W19b
remains the implementation checkpoint; W67b is current-source requalification.

| W67b artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W67-creation-action-release.xml` | `311e2fcfb0b8cd3d4406e4621dccb74b2a83aa5a5b0ea96ee398dc9c7b526dc6` |
| `/private/tmp/offset-W67-creation-action-asan.xml` | `c43baacbc8b6409c81cf145c6dcdb12931c73ce65bc15a08a67961a1b5ba17dc` |

## 28. W68 Library Block Insertion Staged Import Routing

DXF/DWG files selected through the block-library insertion action now use
`LC_OffsetImportSession` before preview or commit. The unpublished imported
graphic is action-owned, its source stamp is checked again before adoption,
and the previous import session is cancelled when the selection changes. Mouse
preview and coordinate commit remain disabled until import completes. Other
library formats retain the prior synchronous loader. The focused offscreen test
confirms pending imports are not published early and the inserted block retains
its native `EntityOffsetCurve`.

The complete offscreen creation-action target passes 133 JUnit cases and
90,232 assertions in both Release and ASan, with zero errors, failures or skips.
The focused library-insert case also passes in both configurations. ASan ran
with `detect_leaks=0`; this is not leak validation. This is local test-harness
coverage, not visible desktop, installed-package, or Windows runtime validation.
The full `librecad_tests` target also builds, and its focused library-insert run
passes in Release.
The library thumbnail renderer and synchronous plugin import API remain separate
call paths; CLI, hatch-pattern, and preview-only import callers also need explicit
classification before claiming all GUI/import routing is complete.

| W68 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W68-library-insert-release.xml` | `6810c430eafea142c8bf233f5c552af202885ca30e059868fd7be792012f2717` |
| `/private/tmp/offset-W68-library-insert-asan.xml` | `2420dfdb67968fbb5f1d6c78c95afc8ae431dc986036bceccf2d09c727bf5595` |
| `/private/tmp/offset-W68-creation-release.xml` | `a23fcc834b33969f4bd200d85137ed3971abbd4140ce2fc4def33d433e49c6b5` |
| `/private/tmp/offset-W68-creation-asan.xml` | `09d209d31a5fac1d54891fb643cf1648be346cbe95cc20da0fcc4caafc618804` |
| `/private/tmp/offset-W68-library-insert-full-release.xml` | `055d0b1153f4c5a2bdb3d9e4167659f262159a332d73196b34a6a81b288c77a5` |

## 29. W69 Library Import Cancellation and Retry

Added an action-level lifecycle regression for resetting the library insert
action while its staged import is still pending. It verifies no source is
adopted after cancellation, drains the event loop to catch late delivery, then
selects the file again and confirms successful recovery and insertion. This
checks the GUI action's cancellation/retry handoff in addition to the import
session's lower-level cancellation coverage.

The focused library-insert case passes Release and ASan (20 assertions each).
The complete creation-action target passes 133 JUnit cases / 90,237 assertions
in both configurations, with zero errors, failures or skips. ASan used
`detect_leaks=0`; it is not leak validation. These are offscreen macOS runs, not
visible desktop, installed-package, Windows runtime or full-suite qualification.

| W69 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W69-library-insert-release.xml` | `420fd262fb5b64df222b8828e392a26ec3ddfab387ee8b5fbcbf1d6f9349a04d` |
| `/private/tmp/offset-W69-library-insert-asan.xml` | `8d09d4ba7e82cac10ba0caf75f1d09eb3d6e51c563cd05fd19b7f64107864a44` |
| `/private/tmp/offset-W69-creation-release.xml` | `e606b0febe0fb4288013ba3a22b74c733742d4e24cdfa7113a03e8001c3c4fe4` |
| `/private/tmp/offset-W69-creation-asan.xml` | `354bbeaedd3cdbb549d9234f9d9de0074cd50d1f5c896bc06621bf49148a88da` |

## 30. W70 Staged Library Thumbnail Generation

Missing or stale library thumbnails no longer cause `updatePreview()` to parse
every drawing synchronously while populating the icon model. It shows the
placeholder immediately, queues one staged import at a time, renders only after
the source stamp and import outcome are accepted, then updates the persistent
model index. A directory change cancels the active request and invalidates old
indices, so a late thumbnail cannot replace the current selection. Existing
current thumbnails continue to load directly from cache.

The widget-level regression switches directories while the first session is
active, verifies its output is suppressed, then checks that the selected
directory produces a readable 64x64 PNG and updates the model icon. The focused
test passes with 23 assertions in both Release and ASan through both the
creation-action and full `librecad_tests` executables. The complete creation
action suite passes 134 JUnit cases / 90,260 assertions in both configurations.
The full LibreCAD application and both test targets link in Release and ASan.
ASan used `detect_leaks=0`; this is not leak validation.

The staged source capture yields and each request is independently validated,
but DXF parsing and image rendering still execute on the Qt thread. This reduces
model-population blocking and batches work between event-loop turns; it does not
make an individual parse nonblocking. This is offscreen macOS coverage, not
visible desktop, installed-package, Windows runtime, or full-suite acceptance.

| W70 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W70-thumbnail-release.xml` | `9467110aa5c3457e3f9080c821aac8e2bc57669443296205e13f4fc1514a6abc` |
| `/private/tmp/offset-W70-thumbnail-asan.xml` | `e99c9488c56b3ab356da8da0d9705618bc320d4c6683e8ad47e03f23b5f2df7a` |
| `/private/tmp/offset-W70-thumbnail-full-release.xml` | `cac101fc2ec0091060056537d05d6311a1799965b156aef8da2e2c80bc3add4d` |
| `/private/tmp/offset-W70-thumbnail-full-asan.xml` | `de97b270749cd8f78cabb1df043db6e37ea3fb341d00ff80c86f35ca988ecc11` |
| `/private/tmp/offset-W70-creation-release.xml` | `214472cc9af6ee3e15e7a3bdc4ac36869ead14e6f14590281ed1195c938246f9` |
| `/private/tmp/offset-W70-creation-asan.xml` | `bad218f3af3207266a376c59b36a201d23145d5491a0fad700d552d349010e4c` |

## 31. W71a Preview-Cache Portability Requalification

At current source `7a90b5884` (after W70), rebuilt and reran the real helper
process and worker-client portability selectors in Release and address-sanitized
builds. The helper test starts fresh processes, transports cached preview values,
and still performs native adoption checks; client cases cover source rebinding,
stale-revision rejection, and action-wide eviction/merge. All runs completed with
zero failures, errors, or skips. JUnit `tests` below is the assertion count, which
varies with process scheduling; each process report has one testcase and each
client report has two.

| W71a lane | Testcases / assertions | Result | SHA256 |
|---|---:|---|---|
| Helper process Release | 1 / 209 | PASS | `59ef7145192eeeece9519ea7a2cab7fe01997873f581a014d93ffc35ac9d4c43` |
| Helper process ASan | 1 / 272 | PASS | `de968c4afb59722b36097bfed2cd9247964d75b4b42140afb18892950306c87f` |
| Worker client Release | 2 / 681 | PASS | `50f529e4fb423f0a44bf78bc7e19dc4fe6f022c9481759974f984c66dcd99016` |
| Worker client ASan | 2 / 1009 | PASS | `261d87b92eca752be05c16d7f73dd0da5124e6adce2bff42db26cdca4f81b9d9` |

Reports are `/private/tmp/offset-W71-cache-{process,client}-{release,asan}.xml`.
ASan used `detect_leaks=0`, so this is not leak validation. This confirms the
existing W16/W17 portable-cache implementation on the current W70 tree; no
product gap was found and no cache implementation was duplicated. Cross-platform,
installed-helper, and final acceptance gates remain open.

## 32. W71b Creation and Repeat GUI Routing Requalification

Against the same product source `7a90b5884`, rebuilt and ran the complete
`[creation-worker]` selector in `librecad_offset_creation_action_tests` in Release
and ASan. All 13 GUI-action cases passed with zero failures, errors, or skips
(318 JUnit assertions in each run). Coverage includes worker-routed preview and
commit, cache reuse at publication, native repeated copies and zero-sum stored
cache fallback, mixed-source ordering and attributes, cancellation, timeout
recovery, stale source/selection/layer/UCS rejection, and one-cycle undo/redo
publication. These are offscreen Qt action tests, not visible interactive
acceptance.

| W71b lane | Testcases / assertions | Result | SHA256 |
|---|---:|---|---|
| Creation/repeat Release | 13 / 318 | PASS | `431301b4020ce1dad423503772271d972cca5430335498be760da42dda975ce5` |
| Creation/repeat ASan | 13 / 318 | PASS | `846823c08358c784b41ad21b537d48f5f3c3d3dddc5ff4dc77c0205b7fb9c108` |

Reports are `/private/tmp/offset-W71-creation-{release,asan}.xml`. ASan used
`detect_leaks=0`; no leak-validation claim is made. This confirms W18/W19 local
creation/repeat routing at the W70 source; no product gap was found. Visible GUI,
Windows runtime, installed-helper, remaining import callers, and final acceptance
remain open.

## 33. W72 Staged Plugin Block-Import API

Committed as `5f754c29d`. Added `Document_Interface::addBlockFromDiskAsync`
as an additive callback API with a synchronous compatibility adapter. The
LibreCAD implementation stages supported DXF/DWG imports through
`LC_OffsetImportSession`, checks the source stamp and active view/document
identity before publication, and allows at most one active staged import per
view. W91 also routes the legacy synchronous `addBlockfromFromdisk` contract
through this staged path for supported formats; it waits for completion to
return the block name, so it remains blocking and new plugins should use the
async API. Non-DXF/DWG formats retain their existing synchronous importer. No
in-tree production call site currently invokes the async API directly, so it
remains an available nonblocking route for plugins rather than evidence that
all external plugin callers have migrated.

Focused `[plugins][offsetcurve][block-import]` tests pass in Release and ASan
(5 cases / 33 assertions in each report), covering legacy behavior, async
native-offset preservation, stale-document refusal, per-view worker limit and
view teardown. The broader `[plugins][offsetcurve]` selector passes in both
configurations (9 cases / 19,120 assertions). Full `LibreCAD` targets link in
CMake Release and ASan. ASan used `detect_leaks=0`, so these runs are not leak
validation. The top-level qmake6 clean build was attempted in
`/private/tmp/offset-W72-qmake6-full` and completed with exit status 0, producing
`LibreCAD.app` with the application and adjacent `LibreCADOffsetWorker`. A
protocol-4 Generate request sent to that bundled helper exited 0 and returned
one size-exact framed reply (530-byte payload, 534 bytes including the prefix).
This is a local macOS build/smoke, not installed-package or Windows acceptance.
Visible UI, installed-helper deployment, Windows runtime, remaining import
callers, and final acceptance remain open.

| W72 lane | Testcases / assertions | Result | SHA256 |
|---|---:|---|---|
| Plugin block import Release | 5 / 33 | PASS | `0cdae7f4e7148cc13de11ee8f7c88d5db0a55e588d2b57c40ca20ce414fdd9ef` |
| Plugin block import ASan | 5 / 33 | PASS | `3aad4fc80c506478419bd74bbaeafbbb48de3b4ea5a43dc353cba101e98c5d82` |
| Plugin offset selector Release | 9 / 19,120 | PASS | `53d8f8bb6d0a66bef1df8a2b2376bb060f1f423cc9d3471877b71f160318f77b` |
| Plugin offset selector ASan | 9 / 19,120 | PASS | `d361bb543333245c89fd240dc42d300e7530090f3d665435012be89f0775c66b` |

Reports are `/private/tmp/offset-W72-plugin-{import,offset}-{release,asan}.xml`.
The qmake6 build and helper smoke used the same W72 source commit.

## 34. W73 Current-Head Cache and Creation Requalification

At PR head `f5d9d9e48`, rebuilt the real helper-process, worker-client, and
creation-action test targets from the current source in CMake Release and ASan
configurations. The fresh helper-process lane exercises portable cached values
across child lifetimes and native-adoption checks; the client lane covers
joined-cache merge/revocation and action-wide eviction; the creation lane covers
the complete worker-routed Modify Offset preview/commit/repeat path. All six
runs pass with zero failures, errors, or skips:

| W73 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Helper process Release | 1 / 212 | PASS | `7e0a8163f678268418a9896925bded0e6035714b262e7206ef2a4ca27b975557` |
| Helper process ASan | 1 / 275 | PASS | `f3f206b22c0ee5f3821b3e7a4befb5ee70fb1828b158c8b7a58df570cc4e5b43` |
| Worker client Release | 2 / 699 | PASS | `e8ea40ac2486cdbf163b798dcaae09fc3f284e989345bd19b4e9237d387e2325` |
| Worker client ASan | 2 / 1005 | PASS | `75146490cbfc4613c0cf17776b776fc541ee72e6ab4f2a50df3c8925235667ed` |
| Creation/repeat Release | 13 / 318 | PASS | `d47068050770b70413c3b9f69edbebb8662c8e1da8f4908f93cfd6285b8ac534` |
| Creation/repeat ASan | 13 / 318 | PASS | `4279071a77d07f9d0396702fbc89476d7d465819ce23b34ff4461a2fdc67d156` |

Reports are `/private/tmp/offset-W73-{cache-process,cache-client,creation}-{release,asan}.xml`.
ASan used `detect_leaks=0`; these runs are not leak validation. This is current-
head macOS offscreen requalification, not Windows runtime, visible-GUI,
installed-helper, or final acceptance. No product gap was found, so W73 changes
the evidence checkpoint only; W16-W19 remain implemented and the broader W0-W2
and platform gates remain open.

## 35. W74 Console DXF/DWG Import Routing

Implemented in product commit `ce507b4a9464576e9cca2352f7d96e78d2c6ce90`.
Console conversion, DWG admission reporting, PDF import, and raster/vector
image-export import now obtain drawings through the staged import session for
DXF/DWG. The helper waits in a local Qt event loop, rejects source-identity
changes and failed imports, and accepts partial input only when at least one
entity was recovered. It reports partial recovery, downgraded offset carriers,
and suppressed diagnostics. Other file formats retain the existing importer.
Ownership is returned as `unique_ptr<RS_Graphic>` so callers do not publish a
partially loaded document after failure.

Added console regressions for ordinary DXF, native-offset binary R2018 DXF,
ODA R2018 native-offset DWG, malformed input, and missing files. The focused
console import and SVG/raster export suites pass in CMake Release and ASan; the
full top-level qmake6 build also completes and links the application plus
adjacent worker. ASan used `detect_leaks=0`, so this is not leak validation.
The import/session and rendering work still executes on the Qt thread; this
routes supported console callers through bounded worker validation but does
not make parsing or rendering itself nonblocking.

| W74 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Console import Release | 4 / 20 | PASS | `fc18893572de761276d2109236825aa05da66ed7484cb50edb5d18619789c7b8` |
| Console import ASan | 4 / 20 | PASS | `0c32c7630b50cde7691d72fad8dc698167785a3dbd5318ec5e04af97a8747ec3` |
| Console SVG/raster Release | 306 assertions | PASS | `82ca5921191aa30645053f1de70b1d4e4f0333953036ab6e4bfcf1acccf783d9` |
| Console SVG/raster ASan | 306 assertions | PASS | `6d67e4199eda41a31cd12730f568d8ed3c06184372e49ba60935113a99c4d2bb` |
| Top-level qmake6 build | application + helper | PASS | n/a |

Reports are `/private/tmp/offset-W74-console-{import,svg}-{release,asan}.xml`.
The remaining direct import sites were subsequently classified. W32/W88 route
user-facing drawing-open and template flows through owned admission; W74 stages
supported console DXF/DWG imports; W91 routes both async and legacy plugin
block-import APIs through the staged session; W93 and W95 make hatch-pattern
and dimension-style previews consume ordinary display cache only. The in-tree
calls into generic `LC_DocumentsStorage` synchronous loading are reached only
for unsupported formats after their caller's DXF/DWG gate. The public
low-level storage API itself remains synchronous and has not been proven safe
for direct external DXF/DWG callers; do not describe it as staged. This is not
a gap in W16-W19 preview-cache transport or creation/repeat GUI routing.
Visible desktop acceptance, installed-helper deployment, Windows runtime, and
final acceptance remain open.

## 36. W75 Staged Import Identity CI Lane

CI/test-registration slice committed as `1516dc230a3394480ff62bc480d13cecc303ba4e`.
The bounded DWG/DXF workflow now builds the console-import and staged-import
hostile-file targets on Linux and both Windows architectures. CTest registers a
focused source-identity selector that compiles and executes the platform-native
file-identity cases. This also repairs the W74 workflow target list: its newly
registered console-import CTest had not yet been included in the workflow's
build command.

On macOS, CMake Release build targets
`librecad_console_command_utils_tests` and `librecad_offset_hostilefile_tests`
build successfully. The workflow-matched CTest run passes both
`librecad_console_import_tests` and `librecad_offset_import_identity_tests`.
Direct JUnit reports show four console-import cases / 20 assertions and three
POSIX identity cases / 128 assertions, with zero errors, failures, or skips.
This host does not compile or execute the Windows-only branches; actual x64 and
ARM64 source-identity validation remains pending the pushed GitHub Actions run.

| W75 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Console import Release | 4 / 20 | PASS | `e287b7d89efdb43f368d7961efc44becd2421cf3c276f118212aa74c3fa582de` |
| POSIX source identity Release | 3 / 128 | PASS | `6350f6539f2743aa2f94327a98ae7d552aa0a82bcf9352f4124bbb84c92fbcbf` |
| Workflow-matched CTest | 2 / 2 tests | PASS | n/a |

Reports are `/private/tmp/offset-W75-{console-import,import-identity}-release.xml`.
No ASan or Windows result is claimed for this slice. The PR workflow must pass
before W44 Windows source-identity parity can be considered runtime-qualified;
other visible-GUI, installed-helper and final acceptance gates remain open.

## 37. W76 MSVC Property-Queue Portability Fix

Fixed the prior Windows CI failure in product commit
`fef374fb14a8cbe0efcbb3ad3838d622c4bf2b4c`. The MSVC log for earlier PR run
`40742822290` showed C2607 at the assertion that `std::map` move construction
is nonthrowing. The publication path now swaps nodes into an empty local map,
which preserves candidate addresses and the existing refusal rollback while
relying only on the map's nonthrowing `swap` guarantee already asserted by the
code. This avoids depending on standard-library-specific conditional move
traits.

On macOS CMake Release, the changed library and
`librecad_offset_properties_sheet_tests` build successfully. The full
`[gui][properties][offsetcurve]` selector passes 53 JUnit cases / 9,367
assertions with zero failures, errors, or skips.

| W76 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Property sheet Release | 53 / 9,367 | PASS | `56e317250d63809cd11d9c7a0966ed92d5bc318a64021cac5aec355ad6c40559` |

Report: `/private/tmp/offset-W76-properties-release.xml`. This host cannot
reproduce MSVC's move-trait result; the exact failing static assertion is
removed, but Windows x64/ARM64 compile and runtime evidence remains pending the
updated PR workflow. W75 identity-lane and other final acceptance gates remain
open.

## 38. W77a Current-Head Preview-Cache Portability

At product source `f6f4f369d37ae4675558d9437a3f94c50ce0f9c5`, rebuilt the real
worker-process and worker-client targets in isolated CMake Release and ASan
trees. The helper-process selector transports cached preview values across
fresh child processes and still applies native adoption checks. The client
selector verifies joined completion/cache merge, stale-revision rejection,
action-wide eviction and moved-from authority revocation. All discovered test
identities executed, including the named cache cases, with zero failures,
errors, skips, duplicate identities or undeclared executions.

| W77a lane | Discovered / executed JUnit cases | Assertions | Result | SHA256 |
|---|---:|---:|---|---|
| Helper process Release | 1 / 1 | 231 | PASS | `7c0ff967f7849b2ba4ac355ab4438f9da5016bf48a2264aad6cc6b459defffca` |
| Worker client Release | 2 / 2 | 825 | PASS | `fd15db36513ea7c3f8b689a8eed01800c00e6655a1ee04b1851b9fa832f206ab` |
| Helper process ASan | 1 / 1 | 304 | PASS | `e495d51bc49fc11bcdb8843135a8c583180415b7a5d854ee0cb539cdc1472c44` |
| Worker client ASan | 2 / 2 | 1219 | PASS | `78a3b2154961616c94dfcad3e17b100d9cf4ea903a425a7790baebecfaab20ff` |

Reports are `/private/tmp/offset-W77-cache-{process,client}-{release,asan}.xml`;
the matching Release discovery receipts are
`/private/tmp/offset-W77-cache-{process,client}-discovery.xml`. ASan used
`detect_leaks=0`, so this is not leak validation. This is current-head macOS
offscreen qualification; Windows runtime, installed-helper deployment, visible
GUI and final acceptance remain open. No product gap was found in this slice.

## 39. W77b Current-Head Creation and Repeat GUI Routing

At the same product source `f6f4f369d37ae4675558d9437a3f94c50ce0f9c5`, rebuilt
and ran the complete offscreen `[creation-worker]` selector in isolated CMake
Release and ASan trees. Coverage includes cached-preview reuse at publication,
native repeat and the zero-sum stored-cache fallback, mixed-source ordering and
attributes, stale input/selection/layer/UCS rejection, cancellation and actual
helper-timeout recovery, two-click relative-zero behavior, and atomic
undo/redo publication. Discovery reports 11 top-level test declarations; Catch2
executes 13 JUnit identities because the repeat declaration contains two
independent `SECTION` paths. Every declared identity is represented in both
configurations, the Release and ASan identity lists match, and there are no
failures, errors, skips, duplicates or undeclared executions.

| W77b lane | Declared / executed JUnit cases | Assertions | Result | SHA256 |
|---|---:|---:|---|---|
| Creation/repeat Release | 11 / 13 | 318 | PASS | `347cf215367a92a4e0f5c0be4e45e7a1f0423ab8d1e1a65f965f94f1ea905448` |
| Creation/repeat ASan | 11 / 13 | 318 | PASS | `a5255be684e36bddaf2ab30bf5d1857b3818a072bd8c26a213c979cc571c1166` |

Reports are `/private/tmp/offset-W77-creation-{release,asan}.xml`; Release
discovery is `/private/tmp/offset-W77-creation-discovery.xml`. ASan used
`detect_leaks=0`, so this is not leak validation. This is current-head macOS
offscreen caller qualification, not visible desktop, Windows runtime,
installed-helper deployment or final acceptance. No creation/repeat product gap
was found in this slice.

## 40. W78a Windows `max` Macro Portability

Manual workflow run `40840320048` against `c3972e4069d47902d620df10870f3a2bd92c59b5`
found a real MSVC compile failure in
`libraries/libdxfrw/src/intern/dwg_dxf_output_transaction.cpp`: Windows' `max`
macro expanded `std::numeric_limits<std::size_t>::max()` at line 118. Added the
same guarded `NOMINMAX` convention already used by the application and file
identity implementation before including `windows.h`.

The focused CMake Release hostile-output regression was rebuilt from the patched
source and passes one JUnit testcase / 692 assertions with zero errors,
failures or skips; strict `xmllint --noout` validation also passes. This Linux
result does not exercise the Windows preprocessor branch. The Windows x64 job in
run `40840320048` therefore remains a failed pre-fix result, and the new source
must be pushed and rerun before recording Windows compile/runtime qualification.
Windows ARM64 and the workflow's Linux fast lane have not yet completed.

| W78a artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W78-output-transaction-release.xml` | `6500f10caa7cd2d79027e2ebb3470feacdaf2f2feb82ff3229d9de0368cae3dc` |

## 41. W78b Current-Source qmake6 Bundle Requalification

At product commit `8e4213399cbe889b3ebc43f27c319d4671d4b177`, configured a fresh
out-of-tree macOS build with qmake 3.1 / Qt 6.12.0, completed the clean
`make -j4`, then completed a second incremental `make -j4` after the
`NOMINMAX` source change. Both commands exited 0; the incremental pass rebuilt
`dwg_dxf_output_transaction.cpp`, relinked `libdxfrw`, and relinked the app and
helper. The bundle contains both arm64 executables at
`/private/tmp/offset-W78-qmake6-current/LibreCAD.app/Contents/MacOS/`.

The user-facing app was not qualified interactively in this lane. Invoking
`LibreCAD --version` from the non-GUI command environment aborted in macOS
HIServices while registering the application, with pasteboard-service errors;
the crash stack is in AppKit/QCocoa platform initialization before LibreCAD's
window setup. The CUA attach did not produce a usable accessibility window, so
this is recorded as an environment-limited launch attempt, not a GUI pass or a
product defect. Prior W55 bundle/helper and W77 offscreen test evidence remains
separate. Actions run `40842072529` validates this exact product commit: Linux
`VerifyDwgDxfFast` passed; Windows x64 and ARM64 remain in progress. Do not claim
Windows qualification until both relevant jobs complete.

| W78b artifact | SHA256 |
|---|---|
| `LibreCAD` bundle executable | `70fd5cde119b1439c5699bf2036c2330694aa0b6a202045361a1fe4b39a4cec9` |
| `LibreCADOffsetWorker` bundle executable | `ede752059cd64b09c78eac6afc591812f8c575921b320b184e48c101af5ecaa0` |

## 42. W79 Staged CMake Install: Preview Portability and Creation/Repeat Routing

At product source commit `8e4213399cbe889b3ebc43f27c319d4671d4b177` (the
current checkout adds only roadmap records after that source), enabled
`LIBRECAD_REGISTER_OFFSET_WORKER_TESTS=ON` in the current-source macOS CMake
Release tree `/private/tmp/offset-W77-release`. Built the app, production
`LibreCADOffsetWorker`, worker-client tests, creation-action tests, `ttf2lff`,
icon engine and configured default targets, then completed the full CMake install
to `/private/tmp/offset-W79-stage`. The installed bundle contains app and helper
at adjacent paths under `bin/LibreCAD.app/Contents/MacOS/`; CMake install exited
0.

Ran both test executables from that staged executable directory with
`QT_QPA_PLATFORM=offscreen`. The worker-client selector exercises real helper
requests, including portable cached-preview production across a fresh helper
process. The creation-action selector exercises Modify Offset creation/repeat
GUI continuations and publication routing using the same installed-adjacent
helper. Both XML reports are well formed and report zero errors, failures and
skips. The action selector has 13 JUnit testcase elements / 318 assertions. The
worker-client selector has 17 testcase elements and a JUnit suite `tests` count
of 8271.

| W79 lane | Result | SHA256 |
|---|---|---|
| Installed worker-client, `/private/tmp/offset-W79-installed-worker.xml` | PASS; errors=0, failures=0, skipped=0 | `d82132c711584e77802e6aecefa713901751ac9ae510d3332404aca23344d230` |
| Installed creation/repeat, `/private/tmp/offset-W79-creation-worker.xml` | PASS; 13 cases / 318 assertions | `f492d3ffda97dc9c0e7e3875f7654f58e8b01699c79a663e843df560c8f2580d` |

This is staged-package offscreen coverage, not visible desktop interaction or
cross-platform acceptance. Actions run `40842072529` has passed Linux
`VerifyDwgDxfFast`; Windows x64 and ARM64 remain in progress. Do not close W0-W2
or final platform gates from this macOS result alone.

## 43. W80 Atomic Compound Budget Admission

At source commit `27f9c5239`, changed compound geometry-budget reservations to
use one atomic `adoptWork` call instead of sequentially debiting vertices,
pairs, or cache spans. The fix covers cache-layout inspection, reference
provider/event admission, and retained ambiguity records. Regressions assert
that a failed compound reservation retains no partial debit and that work from
already-entered stages remains an exact prefix. This closes that specific
accounting defect; the rest of A-core's shared-budget ownership, sticky-status,
and lifetime audit remains open.

| W80 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Budget-core Release, `/private/tmp/offset-W80-budget-core-release.xml` | 4 / 4278 | PASS; errors=0, failures=0, skipped=0 | `abe61497e7994511134e79a26f26f5aceac47bf31a719eb2a02676b962efc503` |
| Budget-core ASan, `/private/tmp/offset-W80-budget-core-asan.xml` | 4 / 4278 | PASS; errors=0, failures=0, skipped=0 | `472af545897ba877b3340d9b117592019bf837c5c4f56f9a2d5015ae5fc23aba` |

ASan used `detect_leaks=0`, so this is not leak validation. The source change
does not close A-core/A-strict/A-reference/A-cache as a whole or the roadmap's
cross-platform and final acceptance gates.

## 44. W81 Cross-Platform Worker CI Lane

At workflow commit `a44fff463`, enabled opt-in offset-worker test registration
for the bounded DWG/DXF CMake jobs on Linux and Windows, and added the helper,
wire/request/result/kernel/process/client/fault/geometry-timeout, and
creation-action targets to both builds. The jobs run the registered worker
CTest set and the offscreen `[lc-offsetcurve][creation-worker]` selector.

Local macOS CMake Release validation of the same target and execution set passed:

| W81 lane | Result | SHA256 |
|---|---|---|
| Worker CTest, `/private/tmp/offset-W81-worker-ctest.xml` | PASS; 9/9, failures=0 | `99876d3c4efadc9d3651a94f4c80071f8284fe828c2f5b1785e2bcaf61ed2feb` |
| Creation/repeat, `/private/tmp/offset-W81-creation-worker.xml` | PASS; 318 assertions, errors=0, failures=0, skipped=0 | `8764979364dfea0f7b140a5e47f7ba8687d5704a02fd32a5befb2dd99ebac9ce` |
| Workflow YAML parse and `git diff --check` | PASS | n/a |

This local run validates current CMake target names and commands on macOS; it
does not substitute for Linux/Windows runtime validation. The PR Actions run
for this workflow revision is pending and must pass before claiming those
platforms qualified.

## 45. W82 Reliable Large-PR Trigger

At workflow commit `c3e1b1eed`, removed only the `pull_request.paths` filter from
the DWG/DXF verification workflow; the narrower path filter for pushes to
`master` remains. Before this change, the Actions run inventory for PR head
`32ff9857b98c130123a03028a48bddff4f683b03` contained no DWG/DXF verification
run, despite the PR reporting 377 changed files and the workflow's current
source/test paths being in scope. Its latest DWG/DXF run was a manual dispatch
for older source `8e4213399cbe889b3ebc43f27c319d4671d4b177`. The unconditional
PR trigger alone did not schedule a verification run for the new head: the
Actions inventory after push showed only Pixi Builds for
`32ff9857b98c130123a03028a48bddff4f683b03`. W83 below adds an explicit
feature-branch push trigger; scheduling and Linux/Windows execution remain
pending until that run is observed.

| W82 validation | Result |
|---|---|
| `actionlint .github/workflows/dwg-dxf-verify.yml` | PASS |
| Ruby YAML parse | PASS |
| `git diff --check` | PASS |

## 46. W83 Feature-Branch Verification Trigger

At workflow commit `4345678fa`, added `codex/**` to the existing path-filtered
push branches. This gives the current feature branch an explicit push trigger
for the changed workflow file while retaining the narrow DWG/DXF path list for
master and feature-branch pushes. The concurrency key now uses the PR head
branch when available and otherwise the push ref name, so a push run and a PR
run for the same branch supersede one another rather than duplicating the full
matrix.

| W83 validation | Result |
|---|---|
| `actionlint .github/workflows/dwg-dxf-verify.yml` | PASS |
| Ruby YAML parse | PASS |
| `git diff --check` | PASS |

The feature-branch push scheduled run `40848505182` for head
`c48e69c50b382464185932bfe70f33ac85ebda26`. At the W84 checkpoint, Linux fast
and both Windows jobs had started, with dependency/setup steps still in
progress. Pushing W84 will supersede that older-head run; verify the replacement
run and its Linux and Windows conclusions before advancing W2 qualification.

## 47. W84/W85 Contact-Control Admission Review

W84 (`38ffc230f`) tentatively changed the initial contact-control vertex/pair
reservation to be atomic. A post-commit review found this contradicted the
pre-existing exact-prefix contract: the test comment explicitly states that the
contact-control item is admitted before its pair. This is an intentional
ordered work stage, not a compound reservation, so W84's claimed product fix
was withdrawn. W85 (`91dcde3c4`) restores the sequential debit order and retains
the `[budget-core]` test tag so this boundary is included in focused runs.

| W85 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Budget-core Release, `/private/tmp/offset-W85-budget-core-release.xml` | 5 / 4554 | PASS; errors=0, failures=0, skipped=0 | `e7cc30429b3f28b9e56f4b33859d68e9143839e3a0f80cf7e672992e1f964f89` |
| Budget-core ASan, `/private/tmp/offset-W85-budget-core-asan.xml` | 5 / 4554 | PASS; errors=0, failures=0, skipped=0 | `5bb575e2dd28bbe9f015b7746ca46495696fed51121ea8f89e3a14e9ff113891` |

ASan used `detect_leaks=0`. The earlier W84 reports (4,554 assertions each)
passed for the altered expectation but do not establish a defect; they are
superseded by W85's source-semantics review. At this checkpoint, PR Actions run
`40848899453` was building Linux on older head
`a4c62eb66b9dabf3b2b6d822a17c7f57e0fdc175`; both Windows jobs were still in Qt
setup. Pushing W85 supersedes that run, so inspect the replacement run before
claiming cross-platform qualification. The full A-core audit remains open.

## 48. W86 Relocated Install Preview and Creation/Repeat Routing

Copied the complete W79 staged CMake Release install to
`/private/tmp/offset W79 relocated`, preserving the app bundle and its adjacent
`LibreCADOffsetWorker`. Ran the installed test executables from the relocated
`Contents/MacOS` directory. The worker-client test asserts that production
`helperPath()` resolves to the executable's own directory; its real cached
preview/client cases then start the relocated helper. The offscreen Modify
Offset creation-worker selector likewise routes preview and creation/repeat
publication through that sibling helper. This checks path relocation and spaces
in the install path without relying on a build-tree helper or environment
override.

| W86 relocated-install lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Worker client, `/private/tmp/offset-W86-relocated-worker.xml` | 17 / 8209 | PASS; errors=0, failures=0, skipped=0 | `e8d82fc52987498ad8b18aadffed26b0c9f269ed9caa4021ac46189580a98a16` |
| Creation/repeat, `/private/tmp/offset-W86-relocated-creation.xml` | 13 / 318 | PASS; errors=0, failures=0, skipped=0 | `389f8250ac8fb46614c1f7e52b38e8d2954aff62889b9a4df69bba8d1564b453` |

The installed test executables and production helper were copied together; no
source change was needed. This is macOS offscreen package-relocation evidence,
not visible desktop interaction, Linux/Windows relocation, or final W0-W2
acceptance. The visible GUI check could not run because the Mac was locked at
this checkpoint. Recheck the latest PR Actions run after pushing this roadmap
record; cross-platform and final acceptance gates remain open.

## 49. W87 Strict Intersection Queue Allocation

Product/test commit `2a72977bf` removes strict preparation and pair-refinement
queue reservations sized from the configured maximum depth and overall work
ceilings. Queues now reserve the root or next pair of children only as that
work is admitted, while capacity reservation still occurs before the
corresponding item debit and queue mutation. Previously, an immediately
resolvable two-line crossing with `UINT_MAX` depth and `SIZE_MAX` pair/item
ceilings could request billions of unused queue slots before its first solve.
The regression verifies the same exact crossing completes with shallow
observed depth and bounded generated work under those maximal ceilings.

| W87 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Maximal-ceiling regression, Release | 1 / 22 | PASS; errors=0, failures=0, skipped=0 | `e6cb972eb29c2972adbc9e7b2775d58c9725fefd9f3655d272d5c3879122516d` |
| Maximal-ceiling regression, ASan | 1 / 22 | PASS; errors=0, failures=0, skipped=0 | `82e94103e861775844f43c0de49a6a20d16c18b132e304bee6b2950634109787` |
| Strict-budget selector, Release | 21 / 898 | PASS; errors=0, failures=0, skipped=0 | `e16c0462f7cac99e71d63c1cfb9fc8b8a592b1f799e4c268afb4060b006978aa` |
| Strict-budget selector, ASan | 21 / 898 | PASS; errors=0, failures=0, skipped=0 | `51a6311c1b70b55690282b38a0e7add3b87581d896ec62f100b9273a54dc7dc0` |

ASan used `detect_leaks=0`; this is not leak validation. Reports are in
`/private/tmp/offset-W87-strict-{queue,budget}-{release,asan}.xml`. W87 closes
this concrete allocation issue only; A-core/A-strict shared accounting,
provider callbacks, lifetime, and cross-entry audits remain open.

## 50. W88 Current-Source Preview, Creation/Repeat and Pattern Import

Product/test commit `b12e26d7f` makes `RS_Pattern::loadPattern()` honor the
`RS_FileIO::fileImport()` result. Pattern files are internal library input, so
the call now supplies a noninteractive error callback rather than attempting
to create a modal `QMessageBox` from headless or non-GUI callers. A malformed
DXF is rejected before any imported entities can become hatch preview cache;
valid files still populate ordinary line/arc/ellipse display geometry. The
new hostile-file case checks both outcomes and an empty cache after partial
import failure.

Rebuilt the current source in the W77 Release and ASan CMake trees, then
requalified fresh-process cache transport, worker-client preview-cache
rebinding/merge, and offscreen Modify Offset creation/repeat GUI publication.
The targeted current-source results are:

| W88 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Hatch-pattern partial/valid import, Release, `/private/tmp/offset-W88-pattern-release.xml` | 1 / 100 | PASS; errors=0, failures=0, skipped=0 | `333b93ba1d5d31a386b90915e028cb85eb6f11c4b21fd712ad445ccbaa23122d` |
| Hatch-pattern partial/valid import, ASan, `/private/tmp/offset-W88-pattern-asan.xml` | 1 / 100 | PASS; errors=0, failures=0, skipped=0 | `35fa7935866b339a672da829866190097e8233fa641f4447c4ac339eb1efe0f2` |
| Fresh-helper preview cache, Release, `/private/tmp/offset-W88-cache-process-release.xml` | 1 / 208 | PASS; errors=0, failures=0, skipped=0 | `a0af9f810d8db511b41cfcca33f769df7f2f9f8de7d35811903b29151759e44f` |
| Fresh-helper preview cache, ASan, `/private/tmp/offset-W88-cache-process-asan.xml` | 1 / 271 | PASS; errors=0, failures=0, skipped=0 | `51720188571385e5295b63b6f2b98865c02258b331fcf8ab161edc7844ea6316` |
| Worker-client cache portability, Release, `/private/tmp/offset-W88-cache-client-release.xml` | 2 / 715 | PASS; errors=0, failures=0, skipped=0 | `fcd1a7da8b61998ca1099dd35e74c86b93021cd7ac751259ef8de359d0841155` |
| Worker-client cache portability, ASan, `/private/tmp/offset-W88-cache-client-asan.xml` | 2 / 993 | PASS; errors=0, failures=0, skipped=0 | `bc7f247fe3333fbe8cddc5c8d23163607677ce796625cb9f1ac1024efa02eb7d` |
| Creation/repeat GUI routing, Release, `/private/tmp/offset-W88-creation-release.xml` | 13 / 318 | PASS; errors=0, failures=0, skipped=0 | `e25c4160e4f79c93c7a884de7843c2447468db19c2f1370db6924735dcaac351` |
| Creation/repeat GUI routing, ASan, `/private/tmp/offset-W88-creation-asan.xml` | 13 / 318 | PASS; errors=0, failures=0, skipped=0 | `11fa4876a53341c8f5ceca03d3ab10a24825f138400cd38d22aa2994016412ef` |

ASan used `detect_leaks=0`; these runs do not establish leak validation. This
is current-source macOS CMake/offscreen evidence, not visible desktop
interaction, Windows/Linux runtime qualification, relocated-install retesting
at this head, or final W0-W2 acceptance. Remaining import callers and the
broader budget/source audit are still open.

## 51. W89 Dimension-Style Preview Import Failure

Product/test commit `8b2e124bb` routes the bundled dimension-style sample
through `RS_FileIO`'s noninteractive error callback. On import refusal, the
preview graphic is reset before view creation and the original drawing's arrow
blocks are recopied. This prevents an internal resource error from opening a
modal dialog or displaying entities from a partially parsed sample. A private
sample-path overload permits the test to inject a truncated DXF without
changing the production resource path.

The malformed-sample regression watches for `QMessageBox`, uses a timed
safety-close if one unexpectedly appears, verifies no dialog was shown, checks
that no sample entities remain, and confirms the copied `ARROW` block survives.
The existing preview/navigation selector also exercises the normal bundled
sample and hatch preview paths.

| W89 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Malformed dimension sample, Release, `/private/tmp/offset-W89-dimstyle-release.xml` | 1 / 9 | PASS; errors=0, failures=0, skipped=0 | `2c3ae687dd81a8e428b926dc941e43b1393c5bb739b0504af0b1dfad83ee38b1` |
| Malformed dimension sample, ASan, `/private/tmp/offset-W89-dimstyle-asan.xml` | 1 / 9 | PASS; errors=0, failures=0, skipped=0 | `f9d827ab1b1dd4986eb5c4a6b07c44d145ea7d84ea169bdbc8cd7eb5afbd5d52` |
| Normal preview/navigation, Release, `/private/tmp/offset-W89-dimstyle-normal-release.xml` | 38 / 9625 | PASS; errors=0, failures=0, skipped=0 | `97ef7e463a7e3182f45cd1c2f19631e4c1ee3d39392c037c1b1e03fd713b1814` |
| Normal preview/navigation, ASan, `/private/tmp/offset-W89-dimstyle-normal-asan.xml` | 38 / 9625 | PASS; errors=0, failures=0, skipped=0 | `f679425c5d32c979ffb7748398f7855ad8a3f0b6a6b12f4094ec6022970b9ada` |

ASan used `detect_leaks=0`. This is current-source macOS CMake/offscreen
coverage, not visible dialog interaction or cross-platform/final acceptance.
Other storage/import callers and the broader budget/source audit remain open.

## 52. W90 Current-Head Preview Cache and Creation/Repeat

Rebuilt the process/client cache and Modify Offset creation-action targets from
the post-W89 source at `6014d7882`, then reran the focused portability and GUI
selectors in CMake Release and ASan. These are qualification-only results: no
new product gap was found in the already implemented W16-W19 cache transport,
merge/eviction, preview reuse, creation/repeat publication, or fallback paths.

| W90 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Fresh-helper preview cache, Release, `/private/tmp/offset-W90-cache-process-release.xml` | 1 / 208 | PASS; errors=0, failures=0, skipped=0 | `632f3a0de8df2b111938b0b13ac26c2695d8f92e399558d44dbcc3daddf6e1d9` |
| Fresh-helper preview cache, ASan, `/private/tmp/offset-W90-cache-process-asan.xml` | 1 / 264 | PASS; errors=0, failures=0, skipped=0 | `c3115265df2d66ce87b660ebfc007ddc3af990d1a0c798963e644227862a9e76` |
| Worker-client cache portability, Release, `/private/tmp/offset-W90-cache-client-release.xml` | 2 / 665 | PASS; errors=0, failures=0, skipped=0 | `095bf6fcf8b002199c272a1665287aad96754a442fb0f186a789e23257f4c9dc` |
| Worker-client cache portability, ASan, `/private/tmp/offset-W90-cache-client-asan.xml` | 2 / 1013 | PASS; errors=0, failures=0, skipped=0 | `29c0c43391c2bfebcd5f1d8cbb6e96b1c27c74e120223fe64176d969e5fb4215` |
| Creation/repeat GUI routing, Release, `/private/tmp/offset-W90-creation-release.xml` | 13 / 318 | PASS; errors=0, failures=0, skipped=0 | `850f18e782afb55ec9ea72ee81e3119ea22a33f82c942446316aea4010b9ebb0` |
| Creation/repeat GUI routing, ASan, `/private/tmp/offset-W90-creation-asan.xml` | 13 / 318 | PASS; errors=0, failures=0, skipped=0 | `a586eb6f63f72c84b53d4e2301ce68b9019e16fee56909a56f07004e73a0e277` |

ASan used `detect_leaks=0`; this does not establish leak validation. The GUI
selector ran with `QT_QPA_PLATFORM=offscreen`. This evidence does not close
visible desktop interaction, Windows/Linux runtime, installed/relocated helper
retesting at this head, remaining import ownership, the broader budget/source
audit, or final W0-W2 acceptance.

## 53. W91 Legacy Plugin Block-Import Worker Routing

Product/test commit `e06b1b676` closes the known native-worker bypass in
`Doc_plugin_interface::addBlockfromFromdisk`: when the detected format is a
supported DXF/DWG, the legacy synchronous API now delegates to the existing
staged plugin-import route and waits for its owned completion before returning
the block name. The session remains parented to the view, and the staged path
continues to validate the source stamp and confirm the same document/graphic
before publication. View destruction quits the compatibility wait without a
late block publication. Other-format imports keep their prior synchronous
path, with a `QPointer` view snapshot preventing publication if a modal import
error allowed the active context to change.

The regression asserts that the compatibility call processes event-loop
heartbeats while the native worker validates the imported offset, and that a
scheduled document switch returns no block and mutates neither drawing. The
existing async native-import, queue-limit, cancellation and stale-document
cases remain in the same selector.

| W91 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Plugin block import, Release, `/private/tmp/offset-W91-plugin-block-release.xml` | 6 / 38 | PASS; errors=0, failures=0, skipped=0 | `442c907dfb7e26f213e6b2bcc5f51189dc9cdf48bb4a5de3e0c9c8c88808e9ef` |
| Plugin block import, ASan, `/private/tmp/offset-W91-plugin-block-asan.xml` | 6 / 38 | PASS; errors=0, failures=0, skipped=0 | `2a093dc8c703e254d87a4eed666b70a829ba537350bbb1a05d17e2ebc130bbe9` |

ASan used `detect_leaks=0`; no leak-validation claim is made. This is a
macOS/offscreen compatibility-path check, not proof that the synchronous API
is responsive or that external plugins have migrated to the async entry point.
Windows/Linux runtime, installed-helper retesting at this head, remaining
import ownership, broader budget/source audit and final W0-W2 acceptance remain
open.

## 54. W92 Current-Head Preview Portability and Creation/Repeat

Rebuilt and requalified the existing W16-W19 portable preview-cache transport
and Modify Offset creation/repeat GUI continuation at `2477268af` in CMake
Release and ASan. The code review found no new product defect in these requested
paths: the helper receives bounded value snapshots rather than entity pointers;
joined completions bind cache updates to the expected request/revision; eligible
ellipse/native-source preview and commit route through owned asynchronous
continuations; repeated outputs preserve cache reuse, stored-cache zero-sum
fallback, ordering, attributes, selection and one-step undo/redo. Ordinary-only
selection keeps its established synchronous path. This checkpoint is
qualification-only; it does not claim visible desktop, installed-helper, or
cross-platform acceptance.

All focused selectors passed with zero errors, failures, or skips. The first
number is JUnit testcase records; the second is assertion count. ASan used
`detect_leaks=0`, so no leak-validation claim is made.

| W92 suite | Release cases / assertions | ASan cases / assertions | Result |
|---|---:|---:|---|
| Portable cache request codec | 1 / 111 | 1 / 111 | PASS |
| Portable cache result codec | 1 / 18 | 1 / 18 | PASS |
| Cached Produce kernel | 4 / 156 | 4 / 156 | PASS |
| Fresh-helper process transport | 1 / 216 | 1 / 271 | PASS |
| Worker-client cache merge/eviction | 2 / 697 | 2 / 981 | PASS |
| Creation/repeat GUI routing | 13 / 318 | 13 / 318 | PASS |

Reports and SHA256:

| W92 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W92-request-release.xml` | `1277aa854e70beaf792d9f2a174f81665f3571a3f246288eaa1d384ae895ba09` |
| `/private/tmp/offset-W92-request-asan.xml` | `2efaccd201851d6b68bcb1d263ab2ab5ca53a3b5a2848866534d5f5c33d65c99` |
| `/private/tmp/offset-W92-result-release.xml` | `875d5c07677542caf03476a5642b44486bf04754eb71d4aa22a5fb4926e52d41` |
| `/private/tmp/offset-W92-result-asan.xml` | `9f4bd2a9c56bc524457c89399963773abf831d4830793a02ab62ac4767c49d01` |
| `/private/tmp/offset-W92-kernel-release.xml` | `c7e542e3bf74befb880f445574857b12cf030621c877b5c4de85e5509bcc8d0e` |
| `/private/tmp/offset-W92-kernel-asan.xml` | `ae9cbe35a3c0d9aafedcfc5463588d82229c09992d286d14a4ed39c0e9a007e0` |
| `/private/tmp/offset-W92-process-release.xml` | `8f0c144dfea733f9dba391c13fa953af7719c73b730ec03058bdb668c0984570` |
| `/private/tmp/offset-W92-process-asan.xml` | `2bb42ad3521bd8806fbafef693ea40dbc13fcf88b5fd53096de395f870f6e858` |
| `/private/tmp/offset-W92-client-release.xml` | `d3a644f5095ac495ccb510aec64c1b7887a274a90c1cf4e6335d3cb747522f47` |
| `/private/tmp/offset-W92-client-asan.xml` | `0ad33611177a606c6e49a7c3a98a7dcd992f39984402482aafbcd847b5525007` |
| `/private/tmp/offset-W92-creation-release.xml` | `835dec0e892f74af2feb3a250f184feac0b0fcdeb4296b8abfcd3eeef9d87af6` |
| `/private/tmp/offset-W92-creation-asan.xml` | `e1cd8834475952a008dbf122d65d3e2a610fe84670767028ed4b2ee6912bb4e9` |

W92 is a verification checkpoint, not a new implementation slice. Continue with
the remaining budget and caller-policy audit; pattern-DXF display-cache import
is closed by W93. Then finish cross-platform/deployed and visible-GUI
acceptance.

## 55. W93 Hatch-Pattern Cache-Only Import Policy

Product/test commit `0bf107649` adds an explicit `RS_FileIO` offset-import
policy. Normal callers retain `ValidateNative`; internal pattern loading opts
into `DisplayCacheOnly`. For a recognized offset-carrier spline, the DXFRW
filter creates an ordinary `RS_Spline` from the validated display-cache
payload and strips the owned native marker without invoking synchronous native
admission. Malformed carrier cache still follows the established ordinary
spline fallback. Nested XREF filters inherit the caller policy. Hatch pattern
loading now keeps ordinary splines alongside lines, arcs, and ellipses while
continuing to reject partial DXF imports without a modal error callback.

This closes the identified hatch-pattern caller bypass: pattern preview is
display-only and no longer performs native offset validation on the GUI path.
It does not change default drawing import or make cache-only geometry eligible
for native offset operations.

| W93 lane | Result | SHA256 |
|---|---|---|
| Hostile-file/pattern selector, Release, `/private/tmp/offset-W93-hostile-release.xml` | 1 / 71 assertions; PASS, errors=0, failures=0, skipped=0 | `f92cbe916a3adbd739c0285b4494501c30239a1dc0ed4be50f8f14ad712850bd` |
| Hostile-file/pattern selector, ASan, `/private/tmp/offset-W93-hostile-asan.xml` | 1 / 71 assertions; PASS, errors=0, failures=0, skipped=0 | `d6f27b319ff9af37fb420711b8adb508c57dd9c3fe414ebb2deced42d1427078` |
| CMake Release app and hostile-file test targets | PASS | n/a |
| qmake6 full app and plugin build (`make -j4`) | PASS, exit 0 | n/a |

The hostile-file selector was run as `[lc-offsetcurve][hostile]`; it verifies
that the same fixture remains a native offset in ordinary drawing import, but
is imported as a plain spline cache in hatch-pattern loading, with no owned
marker. The intentionally missing partial-DXF fixture logs the expected import
failure. ASan used `detect_leaks=0`, so this is not leak validation. Build
warnings were pre-existing missing-field-initializer warnings. This is
macOS/offscreen and qmake6 build evidence, not visible desktop testing or
Windows/Linux runtime qualification. Remaining GUI/import caller audit,
broader source/budget audit, installed-helper portability, and final W0-W2
acceptance remain open.

## 56. W94 Current-Source Cache and Creation/Repeat Qualification

Rebuilt the Release process-cache, worker-client, creation-action and shared
`librecad_tests` targets after W93, then requalified the focused worker cache
portability and Modify Offset creation/repeat paths at source head
`124d3cbc1`. The ASan process-cache, worker-client, creation-action and hostile
file targets were also rebuilt. No new product defect was found in the portable
cache transport, revision-bound merge/eviction, preview-cache reuse, or
creation/repeat publication and fallback paths.

| W94 lane | Release cases / assertions | ASan cases / assertions | Result |
|---|---:|---:|---|
| Fresh-helper preview cache, `[preview-portable][worker-process]` | 1 / 212 | 1 / 274 | PASS |
| Worker-client cache merge/eviction, `[preview-portable][worker-client]` | 2 / 697 | 2 / 1019 | PASS |
| Modify Offset creation/repeat, `[creation-worker]` | 13 / 318 | 13 / 318 | PASS |
| Hatch-pattern hostile import, `[lc-offsetcurve][hostile]` | 1 / 71 | 1 / 71 | PASS |

All reports contain zero errors, failures or skips. ASan used
`detect_leaks=0`; this is not leak validation. The focused execution is
macOS/offscreen and does not replace visible desktop, Windows/Linux runtime,
installed-helper relocation, or final W0-W2 acceptance. The exact JUnit
artifacts are:

| W94 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W93-process-release.xml` | `84ea312a6f0e2242a09f50d9a32b1c76ebf55603cb45d912ababee6b90f06fb9` |
| `/private/tmp/offset-W93-process-asan.xml` | `a7a1e38367eb67595c688a13b82f41d851212342c34e0bbcf4b744acb71c85e4` |
| `/private/tmp/offset-W93-client-release.xml` | `d7e6655b0a122832b4a185aca7fcdfc5d3836a0fabd7c34e2fc3fd08f65b2e56` |
| `/private/tmp/offset-W93-client-asan.xml` | `fca4e5e8b7325fa74b829a75d338131d6e4c6ff8164af72e72584c1a5254bb6f` |
| `/private/tmp/offset-W93-creation-release.xml` | `472ac2d166ae06e3edb3996ffbbe6380b6cecd7fee0d29671f85db33d252d891` |
| `/private/tmp/offset-W93-creation-asan.xml` | `3bc7f42abc91f17c92179b0be70f5b6062844a7b104d1e9d00c8c80906bf5a31` |
| `/private/tmp/offset-W93-hostile-release.xml` | `f92cbe916a3adbd739c0285b4494501c30239a1dc0ed4be50f8f14ad712850bd` |
| `/private/tmp/offset-W93-hostile-asan.xml` | `d6f27b319ff9af37fb420711b8adb508c57dd9c3fe414ebb2deced42d1427078` |

W94 is verification-only. W93 remains the latest product change in the
preview-cache/creation thread; caller-policy follow-up continues below.

## 57. W95 Dimension-Style Preview Cache-Only Import

Product/test commit `1a6b422a3` makes the dimension-style sample viewer opt into
`RS_FileIO::OffsetCurveImportPolicy::DisplayCacheOnly`, matching hatch-pattern
preview behavior. An owned offset carrier therefore renders as an ordinary
marker-free spline in this visual-only context; the same fixture still imports
as an editable `LC_OffsetCurve` through the normal drawing-import policy. The
existing malformed-sample behavior remains non-modal and clears partial
content.

The direct-import inventory was reconciled against production callsites:

| Caller | Supported DXF/DWG policy | Current disposition |
|---|---|---|
| Main drawing open and template creation | staged session before document publication | W31/W32 |
| Library block insertion and thumbnail preview | staged session before preview/publication | W68-W70 |
| Console import/conversion | staged session for supported DXF/DWG; other formats use legacy fallback | W74 |
| Plugin block import | staged async entry; legacy sync compatibility entry waits on staged completion | W72/W91 |
| Hatch-pattern preview | ordinary display cache only | W93 |
| Dimension-style preview | ordinary display cache only | W95 |
| `LC_DocumentsStorage` low-level synchronous API | in-tree GUI callers reach it only for unsupported formats; direct external calls are not proven staged | compatibility exposure remains explicit |

| W95 lane | Release cases / assertions | ASan cases / assertions | Result |
|---|---:|---:|---|
| Native offset carrier remains editable in drawing import and becomes plain marker-free spline in dimension preview | 1 / 67 | 1 / 67 | PASS |
| Malformed dimension sample remains non-modal and clears partial content | 1 / 9 | 1 / 9 | PASS |
| Normal hatch/dimension preview and navigation selector | 38 / 9625 | 38 / 9625 | PASS |
| Hatch-pattern cache-only hostile import | 1 / 71 | 1 / 71 | PASS |
| CMake app and `librecad_tests` builds | PASS | PASS | n/a |
| qmake6 full app and plugin build (`make -j4`) | PASS, exit 0 | n/a | n/a |

All JUnit reports have zero errors, failures or skips. ASan used
`detect_leaks=0`; no leak-validation claim is made. Test reports and SHA256:

| W95 artifact | SHA256 |
|---|---|
| `/private/tmp/offset-W95-dimstyle-release.xml` | `f7bdeba3a4b021648c9c7c75d0dcdb38cc331c1583de610d46877ef7796ae337` |
| `/private/tmp/offset-W95-dimstyle-asan.xml` | `3c1896bec37d092d2ce266ec2958b344680cff1ae52651f7c58ffc49960960bb` |
| `/private/tmp/offset-W95-dimstyle-partial-release.xml` | `657523dcc7cbf8e502aaa77c52fa91da672f4f06f968ea654c2846e7b1ed28b2` |
| `/private/tmp/offset-W95-dimstyle-partial-asan.xml` | `212288e15a28fff308481cfe94fa7b7851fe508a81ba108d7ac5917d7aaf4882` |
| `/private/tmp/offset-W95-preview-release.xml` | `a6b485c46c76f8ad7d558f1ad4a7358177adbc9be6f9b39df89a8fb7adeffefc` |
| `/private/tmp/offset-W95-preview-asan.xml` | `3271709e925b7a539ad519f61734e43a08d5ae3eb043b895fec913dad96c4057` |
| `/private/tmp/offset-W95-pattern-release.xml` | `487fb2c77121a53ed5d6ebdeccaba4d0892227af2482662eb356f899e435ce8b` |
| `/private/tmp/offset-W95-pattern-asan.xml` | `68480628e648fe8e6e07ba2ac389848f4fc3773d6d75c3912fae45cd9b89fbcc` |

W95 closes the identified dimension-style preview caller bypass; it does not
convert the public low-level synchronous storage API into an async API,
establish that external callers use the staged route, or close remaining
source/budget, Windows/Linux runtime, installed-helper, visible-GUI, and final
W0-W2 acceptance.

## 58. W96 Current-Source A-Core and Offset-Suite Audit

Reviewed the shared `WorkBudget` and strict intersection accounting paths at
the W95 source head. The audit found no new admission atomicity, overflow,
sticky-refusal, retained-issuer lifetime, or entered-work reporting defect.
Existing A-core/A-strict/A-reference/A-cache audit obligations remain open;
this pass is requalification evidence, not closure of the broad accounting
review.

The focused `[budget-core]` selector and full `[lc-offsetcurve]` selector were
run from the current Release and ASan test binaries. Both full selectors have
1,354 JUnit entries (including Catch sections), zero errors/failures and one
known skip: the real legacy DXF exporter
dispatch test, because this build lacks `RS_FilterDXF`, `dl_dxf.h`, and
`FormatDXFOLD`. No surrogate exporter is used. The Release full suite reports
3,954,967 assertions; ASan reports 3,982,318 due to randomized/section paths.
ASan used `detect_leaks=0`, so it is not leak validation.

| W96 lane | Result | SHA256 |
|---|---|---|
| `[budget-core]`, Release; 15 JUnit entries, zero errors/failures/skips | PASS | `19843bfcc84ecca61850d945a2d2a90eb7d65ccbac8508bb109aafd90b5326ce` |
| `[budget-core]`, ASan; 15 JUnit entries, zero errors/failures/skips | PASS | `f5e736b6a82e5fa28147a7f76ab21123c9fa4b9b9b1f6f45c4ee6a9c68dbb8f6` |
| `[lc-offsetcurve]`, Release; 1,354 JUnit entries, 3,954,967 assertions, one known skip | PASS, skip retained | `a773eaa5ccc1fb09720609a555f0ca35b94e8115ec12abc61c951853a0c378c2` |
| `[lc-offsetcurve]`, ASan; 1,354 JUnit entries, 3,982,318 assertions, one known skip | PASS, skip retained | `f0a46ed5f57592189ff3ce5f98e4f90b3d471cfd4e3de510731b86c0caa811b1` |

W96 does not resolve the known legacy-export skip or complete the source/budget
audit. Cross-platform worker CI, public low-level storage compatibility,
visible-GUI acceptance, and final W0-W2 gates remain open.

## 59. W97 Current-Source Occurrence and Terminal Identity Audit

Reviewed B's issuer-bound critical occurrence handles and immutable reference
component occurrence/edge-side terminal tables at the W96 source head. Critical
handles compare shared-owner identity and index before access; moved, destroyed,
foreign, and expired issuers cannot authorize a snapshot or contraction.
Reference terminal lookup rejects out-of-range edges and invalid enum values;
published terminal records are tied to exact directed occurrence, node, edge,
side, and one provenance locator (free end or pair event). The audit found no
new occurrence/terminal identity defect. This qualifies B's focused review and
test gate, not the dependent C-search/C-check proof obligations or final R.

The focused union `[reference-terminals],[intersection-refine]` completed all
24 JUnit entries in Release and ASan, with 1,453 assertions and zero
errors/failures/skips in each. The full `[lc-offsetcurve]` W96 selectors also
passed in both configurations, retaining the one unavailable legacy-export
skip documented above. ASan used `detect_leaks=0`.

| W97 lane | Result | SHA256 |
|---|---|---|
| Occurrence/terminal identity union, Release; 24 JUnit entries / 1,453 assertions | PASS | `212c7aa2354201a1a5440455a37d03ea71d2a93eb3e51485e546d19a4aba175e` |
| Occurrence/terminal identity union, ASan; 24 JUnit entries / 1,453 assertions | PASS | `3e0f7e2391ae356162244180640edeb4ecdef5165e5906195fca1d69f561786c` |

No product edit was needed. C-search, C-check, broader source/budget audit,
cross-platform CI, public low-level storage compatibility, visible-GUI testing,
and final acceptance remain open.

## 60. W98 Current-Source Offset Path Search Audit

Reviewed C-search at the W97 source head. Input CSR validation checks size
arithmetic before `edgeCount + 1`, accounts row/transition and duplicate-key
comparisons, and refuses malformed graphs before callbacks. DFS route scratch
and each memo key/state/frame are bounded and admitted before publication.
Memo keys include seed, current directed edge, and every used-edge word;
structural suffix counts saturate at two but do not merge distinct ordered
prefixes or imply geometric acceptance. Closed cycles are emitted modulo
rotation from their least edge; open routes preserve both endpoint eligibility
and transition identity. Shared budget refusals stay sticky. No C-search defect
was found; C-check remains dependent and unaudited.

The `[offset-path-search]` selector completed all 30 JUnit entries in Release
and ASan, with 2,965 assertions and zero errors/failures/skips in each. ASan
used `detect_leaks=0`.

| W98 lane | Result | SHA256 |
|---|---|---|
| Offset path search, Release; 30 JUnit entries / 2,965 assertions | PASS | `7487b052c4ab2064bb9278ba41f50d4b929b8151b8a50d8f4d99c06ab73d0351` |
| Offset path search, ASan; 30 JUnit entries / 2,965 assertions | PASS | `fddf6efbabf9a38410259d872404672e1a18e54c958b30f586061294268cbdd9` |

No product edit was needed. C-check, broader source/budget audit,
cross-platform CI, public low-level storage compatibility, visible-GUI testing,
and final acceptance remain open.

## 61. W99 Current-Head Preview Cache and Creation/Repeat Qualification

Ran the incremental CMake Release and ASan `librecad_tests` builds and reran the
portable preview-cache plus creation/repeat worker selectors at the current
source head. The selected cases exercise fresh owned-source rebinding, refusal
and budget-sensitive cache reuse, cross-source eviction, stale/malformed
revision rejection, mixed-source creation ordering and attributes, native
publication, two-click ellipse creation, preview supersession, process timeout
recovery, cancellation/context destruction, and repeated native offsets with
stored-cache zero-sum fallback. No new defect was found in these paths. The
full qmake6 application/plugin build also completed successfully.

| W99 lane | Result | SHA256 |
|---|---|---|
| `[preview-portable],[creation-worker]`, Release; 17 JUnit cases / 385 assertions, zero errors/failures/skips | PASS | `b80613b94c2ab4dbf3e2837572e75d62f1f45a2f92e6d6f96714acae4a5ef83c` |
| `[preview-portable],[creation-worker]`, ASan; 17 JUnit cases / 385 assertions, zero errors/failures/skips | PASS | `795cea746f1f42c6c5c063837b202689fb57138cedadf7013ba20596521ba22b` |
| CMake Release and ASan test-target builds | PASS | n/a |
| qmake6 full application and plugin build (`make -j4`) | PASS, exit 0 | n/a |

Reports: `/private/tmp/offset-W99-preview-release.xml` and
`/private/tmp/offset-W99-preview-asan.xml`. ASan used
`detect_leaks=0`; no leak-validation claim is made. This is an offscreen
macOS regression pass, not visible-GUI or cross-platform runtime acceptance.
No product change was needed. The remaining W0-W2, import/caller, budget,
cross-platform and final acceptance work is still open.

## 62. W100 Cache-Checker Final Publication Allocation

Moved the final proof-output vertex debit ahead of allocation of the immutable
`CacheTopologyAnalysis`. Added a dedicated CMake-only failure executable; its
global `operator new` interposition is isolated from Catch and LibreCAD. The
test measures a successful check's allocation count, injects `std::bad_alloc`
at the final allocation, and verifies that the caller-owned `WorkBudget` has
the complete successful work ledger despite the exception. The standalone
fault executable reports 108 allocations and 140 charged vertices in both
Release and ASan. The focused public checker selector separately retains
exact-cap success, one-less refusal, sticky retry refusal, proof ownership,
directed coverage, contact, seam, closure and typed-refusal cases.

| W100 lane | Result | SHA256 |
|---|---|---|
| Standalone final-allocation injection, Release | PASS; full ledger retained (140 vertices) | n/a |
| Standalone final-allocation injection, ASan | PASS; full ledger retained (140 vertices) | n/a |
| `[cache-topology-chain]`, Release; 65 JUnit cases / 17,977 assertions | PASS; zero errors/failures/skips | `b9a74214f12ecc78b8187842484c58698a32fd154143f71467506484dccdc538` |
| `[cache-topology-chain]`, ASan; 65 JUnit cases / 17,977 assertions | PASS; zero errors/failures/skips | `578e3629177e76f830009d543f4833ed698a762fe108964a481176bce4224ce3` |
| qmake6 full app/plugin build (`make -j4`) | PASS, exit 0 | n/a |

Reports: `/private/tmp/offset-W100-cache-check-release.xml` and
`/private/tmp/offset-W100-cache-check-asan.xml`. ASan used
`detect_leaks=0`; no leak-validation claim is made. This fixes and directly
tests the output-publication accounting gap, but it does not close the full
C-check inventory, the broader A-C allocation/failure audit, or final worker
acceptance.

## 63. W101 Full Offset Selector After Cache-Checker Fix

Ran the full `[lc-offsetcurve]` selector against product commit `fccd107af`,
including the W100 final publication-debit change. Both test binaries link
after the updated geometry object. Each run completed all 1,354 JUnit entries
with zero errors and failures. The single retained skip is the real
legacy-DXF-exporter dispatch case because `RS_FilterDXF`, `dl_dxf.h`, and
`FormatDXFOLD` are absent in this build; no surrogate writer is used. The
selectors cover geometry, budgets, worker/UI, trim, persistence, hostile input
and entity coverage; this broad pass does not replace the remaining manual,
cross-platform, package or complete A-C static audit gates.

| W101 lane | JUnit entries / assertions | Result | SHA256 |
|---|---:|---|---|
| `[lc-offsetcurve]`, Release | 1,354 / 3,947,585 | PASS; 0 errors, 0 failures, 1 known skip | `ff9b6933b9c6d8a85ab7a255f357b993b8e712c1f57e6be6af4ffd1fb5154646` |
| `[lc-offsetcurve]`, ASan | 1,354 / 3,970,466 | PASS; 0 errors, 0 failures, 1 known skip | `de7c50f30c952d1b6d2d747615f5abea65633a17b4612739c60ba5a0b69e9ebe` |

Reports: `/private/tmp/offset-W101-offset-release.xml` and
`/private/tmp/offset-W101-offset-asan.xml`. ASan used `detect_leaks=0`; this
is not leak validation. The skipped real exporter remains an explicit gap.

## 64. W102 Preview-Cache Portability and Creation/Repeat Audit

Re-reviewed the owned preview snapshot representation, CBOR admission and
decode, fresh-helper restoration, revision-bound merge/eviction, and the Modify
Offset creation/repeat continuation at source head `9eb53537f`. The snapshot
contains only bounded ordinary line/spline values; it carries no entity
addresses, native-cache capability, or process-local ownership. Source rebinding
occurs only after decode/admission, and completion checks the captured revision
before merging. The GUI action routes eligible ellipse/native-offset work
through the worker and seals the captured source, options, selection, view/UCS,
layer, pen, and undo epoch before one-cycle publication. Cancellation, timeout,
supersession, context destruction, repeated native output, and zero-sum stored
cache fallback already have direct cases. No new product defect was found.

Rebuilt the Release and ASan test targets and reran
`[preview-portable],[creation-worker]` offscreen. Each run executed 17 JUnit
cases / 385 assertions with zero errors, failures, or skips. ASan used
`detect_leaks=0`. These are macOS offscreen checks; Windows runtime and visible
GUI acceptance remain dependent on the existing PR workflow and final W0-W2
gates. No product change was warranted, so this is a roadmap/evidence checkpoint
only.

| W102 lane | Result | SHA256 |
|---|---|---|
| Preview portability + creation/repeat, Release; 17 cases / 385 assertions | PASS | `a2afe3bbce77fac3e5b86d438c2018d1cdb233e6585c904e02697441116b2463` |
| Preview portability + creation/repeat, ASan; 17 cases / 385 assertions | PASS | `85a884d3dc4a567953d5d8ff370990cdc092f808b9a2ac40992369cc1bb01947` |

Reports: `/private/tmp/offset-W102-preview-creation-release.xml` and
`/private/tmp/offset-W102-preview-creation-asan.xml`.

## 65. W103 Exhaustive Cache-Checker Allocation Failure Sweep

Product/test commit `2e5db37ba` extends the isolated cache-check fault executable
from final-publication-only injection to every allocation observed during one
successful public `checkCache` call, including aligned allocations. For each
failure point, the test verifies `std::bad_alloc` is injected, the caller-owned
budget remains `Ok` with every work counter no greater than the successful
baseline, and retrying with that same budget returns a complete analysis whose
reported work equals the accumulated ledger. The fixture encountered 108
allocation points and 140 baseline vertices. The executable remains separate
from Catch/the application because it replaces global allocation operators.

The DWG/DXF verification workflow now explicitly builds and runs this standalone
target in its Linux and Windows x64/ARM64 lanes, so the platform-specific aligned
allocator implementation is exercised by PR CI. Local macOS Release and ASan
builds/runs both passed all 108 injections and recoveries. ASan used
`detect_leaks=0`; this is not leak validation. `actionlint` passed. The remote
Windows/Linux run is not yet available in the current check set; the broader
A-core through C-check site audit and final R gates remain open.

| W103 lane | Result |
|---|---|
| Standalone checker fault sweep, Release | PASS; 108 injection points, 108 same-budget recoveries |
| Standalone checker fault sweep, ASan | PASS; 108 injection points, 108 same-budget recoveries |
| `actionlint .github/workflows/dwg-dxf-verify.yml` | PASS |

This closes only the public checker allocation-failure sweep for the selected
fixture. It does not claim every geometry operation or every allocator call
site across A/B/C has been swept.

## 66. W104 Cross-Platform Standalone Fault Gates

Product/test commit `08a23f294` adds the existing isolated trim-staging fault
executable beside the cache-check fault executable in the bounded Linux and
Windows x64/ARM64 workflow. Both are built explicitly and run as standalone
processes, keeping global allocation replacement out of Catch and the app. The
trim test's diagnostic now prints its `size_t` spline degree with the matching
format, removing the compiler warning seen during the fresh local build.

Fresh local Release and ASan builds/runs of the trim executable passed: 1,616
injected staging-allocation failures, 20 mid-staging cancellations, four conic
allocation failures and four conic cancellations, with subsequent recovery.
`actionlint` and `git diff --check` passed. ASan used `detect_leaks=0`. The
workflow on this pushed head is pending; no remote Windows/Linux success is
claimed until its jobs finish. This adds platform execution for both global
allocator test processes but does not close the broader A-C/T fault inventory.

## 67. W105 Multi-Layout Cache-Checker Fault Sweep

Product/test commit `46a1a5438` extends the standalone public `checkCache`
allocation sweep to three successful inputs: the original one-span line cache,
a two-span cubic cache in stored order, and the same cubic cache with reversed
stored controls plus `cacheReversed`. For every allocation observed in each
baseline, the test injects failure, checks that the shared work ledger is
componentwise bounded by the successful baseline, and retries with the same
budget to obtain a complete analysis. Release and ASan each pass all 338
injection/recovery points (108 for the line case, 115 per cubic direction); the
successful ledgers are 140 and 155 vertices respectively. ASan used
`detect_leaks=0`.

This adds line/cubic, multispan and orientation coverage to the failure census.
It still does not fault-sweep every analytic regime, contact/seam/cusp topology,
or every A/B/C public operation; keep the broader site audit open.

## 68. W106 Current-Head Preview Portability and Creation/Repeat Requalification

Rebuilt the full test executable in the local Release and ASan configurations
and reran `[preview-portable],[creation-worker]` offscreen at the current source
head. Both runs passed 17 cases / 385 assertions with zero errors, failures or
skips. The coverage exercises bounded owned ordinary cache snapshots through
CBOR and fresh helper processes, cache rebinding/merge/eviction, and the GUI
worker path for preview, one-cycle publication, native repeated copies, and
zero-sum stored-cache fallback. Source review also confirmed the bounded Linux
and Windows workflow runs the creation-worker selector and registers the
worker-client/process portability suites. No implementation gap was found, so
this is an evidence-only checkpoint. It does not claim remote platform jobs or
visible GUI acceptance have completed; those remain final W0-W2 gates.

| W106 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Preview portability + creation/repeat, Release | 17 / 385 | PASS; 0 errors, 0 failures, 0 skips | `45114c048ed26bd6e06f0a013589dc526ba8ec2b5d102f7aecada3fc52430a25` |
| Preview portability + creation/repeat, ASan | 17 / 385 | PASS; 0 errors, 0 failures, 0 skips | `6b499a10965a9f688e168a380ece25637db5232c5f020c610c9dbba54106ad3c` |

Reports: `/private/tmp/offset-W106-preview-creation-release.xml` and
`/private/tmp/offset-W106-preview-creation-asan.xml`. ASan used
`detect_leaks=0`; this is not leak validation.

## 69. W107 Current-Source Budget-Core Requalification

Reran `[budget-core]` in the Release and ASan test executables at source head
`3260855ad`, before the following standalone C-check test-only commit. Both
runs passed 15 cases / 4,554 assertions with zero errors, failures or skips.
The source review covered `WorkBudget` copy prohibition, exact and one-less
admissions, sticky terminal status, zero and maximum-size limits, aliasing
`adoptWork(consumed())`, separate pair/ordering ledgers and combined-depth
overflow. No defect was found in these primitives. This is a focused
requalification only: shared-budget ownership across all A-core call sites,
allocation failure and A-strict/A-reference/A-cache audits remain open.

| W107 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Budget core, Release | 15 / 4,554 | PASS; 0 errors, 0 failures, 0 skips | `529c1a00598b87e8dd191072f3458dad01478ca3dbcec97fb7de5422ebd0d62c` |
| Budget core, ASan | 15 / 4,554 | PASS; 0 errors, 0 failures, 0 skips | `84e7ad2144b6bdfe669157430c572dfa2966541ea25fb1bfb2bc86c25dfab39d` |

Reports: `/private/tmp/offset-W105-budget-core-release.xml` and
`/private/tmp/offset-W106-budget-core-asan.xml`. ASan used `detect_leaks=0`.

## 70. W108 Whole-Circle Seam Cache-Checker Fault Sweep

Test commit `6fc36ce21` adds forward and reversed whole-circle offset-cache
fixtures to the isolated public `checkCache` allocation-failure executable. The
baseline explicitly asserts a closed result containing a seam join before
fault injection, so both fixtures cover the intended seam path. For every
allocation observed in each successful baseline, the harness injects
`std::bad_alloc`, verifies the caller budget remains componentwise bounded by
the successful work, then retries with that same budget and requires a complete
analysis with matching cumulative work. Each circle orientation exercises 459
allocation points and 701 vertices. Alongside the prior line and forward/reversed
two-span cubic fixtures, the total is 1,256 allocation-failure/recovery points.

| W108 lane | Result | SHA256 |
|---|---|---|
| Standalone cache-check fault sweep, Release | PASS; 1,256 injections and same-budget recoveries | `b53c651ebde5237c5f1f8bf76533ebf5a87bdc002f0ab22b9db8b50435d1cfb4` |
| Standalone cache-check fault sweep, ASan | PASS; 1,256 injections and same-budget recoveries | `b53c651ebde5237c5f1f8bf76533ebf5a87bdc002f0ab22b9db8b50435d1cfb4` |

Logs: `/private/tmp/offset-W108-checker-release.log` and
`/private/tmp/offset-W108-checker-asan.log`. ASan used `detect_leaks=0`. The
workflow already builds and executes this standalone target in Linux and
Windows x64/ARM64 lanes, but those remote jobs for the updated head are not yet
verified. C-check's remaining open allocation/topology inventory includes
cusps, additional construction-contact arrangements and the closed companion
from W109's two-component fixture; no claim of full C-check acceptance is made.

## 71. W109 Trimmed Contact-Path Cache-Checker Fault Sweep

Test commit `44f30fd6f` adds a producer-generated trimmed offset that yields two
components, then fault-sweeps the open component through public `checkCache`.
The fixture refuses setup unless the producer returns exactly two splines; the
checker baseline must publish a complete open path before the sweep starts.
Every observed allocation is injected once, bounded work is checked against the
successful baseline, and the same budget must recover a complete analysis with
an exact cumulative work ledger. The trimmed path contributes 6,031 injection
and recovery points over 1,243 vertices. Together with W103/W105/W108 fixtures,
Release and ASan pass 7,287 points. Runtime was approximately five minutes in
Release and eight minutes under ASan on local macOS; keep this cost visible when
evaluating CI placement, without dropping the full sweep from validation.

| W109 lane | Result | SHA256 |
|---|---|---|
| Standalone cache-check fault sweep, Release | PASS; 7,287 injections and same-budget recoveries | `f7dd22e678d3a04448669360054f24d2cf74fa98cdfc5889074af2cc9fbba8c0` |
| Standalone cache-check fault sweep, ASan | PASS; 7,287 injections and same-budget recoveries | `f7dd22e678d3a04448669360054f24d2cf74fa98cdfc5889074af2cc9fbba8c0` |

Logs: `/private/tmp/offset-W109-checker-release.log` and
`/private/tmp/offset-W109-checker-asan.log`. ASan used `detect_leaks=0`. The
cross-platform PR workflow already invokes this standalone executable; its
updated-head Linux and Windows runs remain unverified. This covers one open
component from the two-component producer result, not the closed companion or
all cusp/contact cases. C-check and final R remain open.

## 72. W110 Current-Head Preview Portability and Creation/Repeat Requalification

At source head `1ba916084`, reran `[preview-portable],[creation-worker]` in the
current local Release and ASan test executables with Qt offscreen. Both passed
17 cases / 385 assertions with zero errors, failures or skips. The selected
coverage includes bounded owned cache snapshots through CBOR and fresh helper
processes, revision-bound cache rebinding/merge and eviction, and Modify Offset
GUI preview followed by one-cycle publication, repeated native creation and
zero-sum stored-cache fallback. Source review found no current implementation
gap in these routes, so this is an evidence-only checkpoint rather than a code
change. It does not replace remote Windows/Linux worker jobs or visible GUI
acceptance; those remain open under W0-W2 and final R.

| W110 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Preview portability + creation/repeat, Release | 17 / 385 | PASS; 0 errors, 0 failures, 0 skips | `4e5044ea6e9926df74cb454289b2e18494f7cce3cc94b98eeecaeaf09f59abb8` |
| Preview portability + creation/repeat, ASan | 17 / 385 | PASS; 0 errors, 0 failures, 0 skips | `9e13746693083df218fe5196cd252f3cfbd2eee571c1d41a5361de533a87a3c3` |

Reports: `/private/tmp/offset-W110-preview-creation-release.xml` and
`/private/tmp/offset-W110-preview-creation-asan.xml`. ASan used
`detect_leaks=0`; this is not leak validation.

## 73. W111 Current-Source Strict-Query Budget Requalification

Rebuilt the current CMake Release and ASan test targets, then reran
`[strict-budget]` at source head `079c525ef`. Both lanes passed 21 cases / 898
assertions with zero errors, failures or skips. Source review followed strict
preflight and mutation ordering for callback dispatch, generated records,
candidate/pair admission, retained root contraction, and output scratch. It also
checked depth arithmetic at `UINT_MAX`, balanced-tree capacity multiplication,
`ptrdiff_t` iterator limits, and exit-ledger behavior when provider callbacks
throw. No new defect was demonstrated. This does not close A-strict's broader
cross-entry or allocation-failure audit, nor A-core/A-reference/A-cache.

| W111 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Strict-query budget, Release | 21 / 898 | PASS; 0 errors, 0 failures, 0 skips | `e6052e4ec03cba27dbc26efcc36a726e60d5684521d4e71a1756702dc451e382` |
| Strict-query budget, ASan | 21 / 898 | PASS; 0 errors, 0 failures, 0 skips | `3c1d039d5aa7f0bc16c669c01f1af19bd35e655902cff0df1fed9d36838c638d` |

Reports: `/private/tmp/offset-W111-strict-budget-release.xml` and
`/private/tmp/offset-W111-strict-budget-asan.xml`. ASan used `detect_leaks=0`;
this is not leak validation.

## 74. W112 Paired Reference-Graph Publication Review

At source head `e7c6f8c0a`, reviewed the reference event/component builder's
paired graph path and reran `[reference-accounting],[reference-terminals],
[reference-components]` in CMake Release and ASan. Both lanes passed 33 cases /
25,367 assertions with zero errors, failures or skips. The audit traced graph
records through private staging, checked reserve and budget admission before
append/resize, reviewed public occurrence and edge-terminal cross-links, and
confirmed the result exposes `analysis` only after event publication and full
component assembly succeed. No publication defect was demonstrated. Exhaustive
allocation-failure injection at each graph assembly stage and the broader
A-reference cross-entry audit remain open; this is not a closeout.

| W112 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Paired reference graph, Release | 33 / 25,367 | PASS; 0 errors, 0 failures, 0 skips | `b4ab6b892e23d47b916ccfac68bd8b5ee8748950bdf51942b5de93cc9c85bc4d` |
| Paired reference graph, ASan | 33 / 25,367 | PASS; 0 errors, 0 failures, 0 skips | `ca51098ff4a472a9c2840ab408604ca83a28cdb889ed5120a53aca35babf538d` |

Reports: `/private/tmp/offset-W112-reference-graph-release.xml` and
`/private/tmp/offset-W112-reference-graph-asan.xml`. ASan used
`detect_leaks=0`; this is not leak validation.

## 75. W113 Current-Source Cache-Budget Requalification

At source head `0579e3465`, reran `[cache-budget-accounting]` in the current
CMake Release and ASan test executables. Both passed 52 cases / 8,699 assertions
with zero errors, failures or skips. The source audit traced `CacheBoundsBuilder`
and `CacheLocalTopologyBuilder` collection growth, ensuring capacity is reserved
before debit and append, candidate/proof child admission, inherited depth, and
sticky budget refusal. Tests confirm the local stage retains a complete bounds
snapshot when its own later budget refuses. No new defect was demonstrated.
This does not close A-cache: standalone allocation-fault coverage for the
independent bounds/local-topology APIs and the broader source audit remain open.

| W113 lane | JUnit cases / assertions | Result | SHA256 |
|---|---:|---|---|
| Cache budget, Release | 52 / 8,699 | PASS; 0 errors, 0 failures, 0 skips | `57932c7f8e300700e4c66b4f04b81b53a0f8412977dc49a3e73236faa8410bad` |
| Cache budget, ASan | 52 / 8,699 | PASS; 0 errors, 0 failures, 0 skips | `3f9bf2fcc4c39e22c73f943728e6e30114d3dbb4817b4fa0b627d1db701e3695` |

Reports: `/private/tmp/offset-W113-cache-budget-release.xml` and
`/private/tmp/offset-W113-cache-budget-asan.xml`. ASan used `detect_leaks=0`;
this is not leak validation.

## 76. W114 Standalone Bounds and Local-Topology Allocation-Failure Sweep

Runs used the harness contents committed as `f6930eb3c` on source head
`338154f34`. That test commit extends the isolated global-allocation
interposition executable to sweep the independent `analyseCacheBounds` and
`analyseCacheTopologyLocally` entry points in addition to `checkCache`. Each
baseline must publish a complete analysis; every observed allocation is then
failed once with `std::bad_alloc`, the work ledger must remain no greater than
the baseline and the budget status must remain reusable, and retrying with that
same budget must publish a complete result with an exact accumulated ledger.
The standalone bounds fixture contributes 92 injection/recovery points and the
local-topology fixture 100. Together with the existing line, forward/reversed
cubic, forward/reversed circle seam, and trimmed-contact fixtures, Release and
ASan each pass all 7,479 allocation-failure/recovery points. This closes the
local A-cache allocation-failure gap for these public paths; remote platform
execution and final R acceptance remain open.

| W114 lane | Result | SHA256 |
|---|---|---|
| Cache analysis fault sweep, Release | PASS; 7,479 injections and same-budget recoveries | `f8791253a5f6b03a068761d7ce816105154bb4297eeb39e4cfce74fbbacaa961` |
| Cache analysis fault sweep, ASan | PASS; 7,479 injections and same-budget recoveries | `f8791253a5f6b03a068761d7ce816105154bb4297eeb39e4cfce74fbbacaa961` |

Logs: `/private/tmp/offset-W114-cache-fault-release.log` and
`/private/tmp/offset-W114-cache-fault-asan.log`. ASan used `detect_leaks=0`;
this is not leak validation.

## 77. W115 Closed Trimmed-Component Cache-Checker Fault Sweep

Test commit `47e2ec1cf` extends W109's producer-generated two-component
fixture to select and check the closed component as well as retaining the open
component in the default full run. A `--closed-trimmed` selector permits focused
local validation; CI's no-argument invocation still runs the complete fixture
set. The fixture requires exactly two generated splines and the successful
checker baseline must identify the selected component as closed. At the
checker-only `.1` world tolerance, the closed component contributes 5,497
observed allocation sites over 1,374 vertices. Every allocation was failed
once, bounded work was checked, and the same budget recovered a complete
analysis. The focused Release run took about four minutes and ASan about five
and a half minutes. This closes the selected closed-companion fault-sweep gap;
the default full executable was not rerun in this slice, and cusp, broader
topology/site audit, remote platform execution, and final R acceptance remain
open.

| W115 lane | Result | SHA256 |
|---|---|---|
| Closed trimmed-component fault sweep, Release | PASS; 5,497 injections and same-budget recoveries | `3a9f8effaabd3ef06b91c1b2e75b4b18fec9a806d063abe2f0e257a3fbcd5830` |
| Closed trimmed-component fault sweep, ASan | PASS; 5,497 injections and same-budget recoveries | `3a9f8effaabd3ef06b91c1b2e75b4b18fec9a806d063abe2f0e257a3fbcd5830` |

Logs: `/private/tmp/offset-W115-closed-trim-release.log` and
`/private/tmp/offset-W115-closed-trim-asan.log`. ASan used
`detect_leaks=0`; this is not leak validation.

## 78. W116 Hidden-Cusp Refusal Fault Sweep and C-Check Chain Requalification

Test commit `1e4c51870` adds a focused allocation-failure sweep for the existing
hidden-cusp endpoint case. The successful baseline is required to return the
typed `OffCurve` refusal without publishing an analysis. Every observed
allocation is failed once; work remains bounded, the budget stays reusable, and
same-budget retry must reproduce the same typed refusal and cumulative ledger.
The case has 6,095 allocation sites and passes in both Release and ASan.
The standalone executable accepts `--hidden-cusp-endpoint` for focused runs;
the default no-argument invocation includes the case in the full suite.

The current CMake Release and ASan builds also pass the complete
`[lc-offsetcurve][geometry][cache-topology-chain]` selector: 61 test cases and
17,977 assertions each. This requalifies the existing cache-chain behavior for
the current source; it is not the full offset selector or the full no-argument
allocation-failure executable. No checker product defect was demonstrated.
Broader cusp arrangements, cross-site topology inventory, full current-head
fault executable, remote platform jobs, and final R acceptance remain open.

| W116 lane | Result | SHA256 |
|---|---|---|
| Hidden-cusp refusal fault sweep, Release | PASS; 6,095 injections and same-budget recoveries | `03511ae6566d8023fa4b74f3b3e6e167d036753acc1957b881f1074b8973ce6e` |
| Hidden-cusp refusal fault sweep, ASan | PASS; 6,095 injections and same-budget recoveries | `03511ae6566d8023fa4b74f3b3e6e167d036753acc1957b881f1074b8973ce6e` |
| Cache-topology chain, Release | PASS; 61 cases / 17,977 assertions | `04b9698d9dc217d2d8ea265ba14c0f70ffe2f48d9adfd07fc44b7bcfcb154ac2` |
| Cache-topology chain, ASan | PASS; 61 cases / 17,977 assertions | `98d0e3c2ad26ae32a6db527cb9dfcb3c28a98bd0eaf3db202a575ec9163b9888` |

Logs: `/private/tmp/offset-W116-cusp-release.log`,
`/private/tmp/offset-W116-cusp-asan.log`,
`/private/tmp/offset-W116-chain-release.log`, and
`/private/tmp/offset-W116-chain-asan.log`. ASan used `detect_leaks=0`; this is
not leak validation.

## 79. W117 Current-Head Preview Portability and Creation/Repeat Requalification

At source head `f7aab74fa`, rebuilt the affected CMake targets and requalified
preview-cache value transport, request/result schemas, helper-process reuse,
worker-client cache merge/eviction, and Modify Offset creation/repeat routing.
The main `librecad_tests` selector `[preview-portable],[creation-worker]` passed
15 cases / 385 assertions in Release and ASan. The dedicated creation-action
target independently passed 11 cases / 318 assertions in each configuration.
The worker lanes also passed: kernel 4 / 156; request 1 / 111; result 1 / 18;
worker-client 2 cases (1,045 Release assertions, 1,867 ASan assertions); and
fresh-helper process cache portability 1 case (190 Release assertions, 216 ASan
assertions). All had zero failures/errors; ASan used `detect_leaks=0`. The
separate process test confirmed that cache hits survive a new helper process,
while native adoption remains checked. No product defect was demonstrated.

W110 recorded 17 cases / 385 assertions for the combined selector. The current
main executable enumerates 15 cases / 385 assertions, and the 17-case count is
not reproducible from that executable; W117 records the current split by target
instead of treating the older aggregate as verified. The relevant implementation
files have not changed since the prior preview/creation checkpoint. This closes
the requested current-head preview and creation/repeat requalification, not
packaged/visible acceptance or remaining W0-W2 platform gates.

| W117 lane | Result | SHA256 |
|---|---|---|
| Main preview + creation selector, Release | PASS; 15 cases / 385 assertions | `4a038bbdff22737e8c8caecbdb8a1eb0b456a4e90c4ba00d2a2d6f976f9229fe` |
| Main preview + creation selector, ASan | PASS; 15 cases / 385 assertions | `f709c245bbab2c7a0ee30579e1cdfd596b52686969134db34930a86e1e22cebe` |
| Dedicated creation action, Release / ASan | PASS; 11 cases / 318 assertions each | `185f929d48c27f8f0ebe8ed79604da2ee2e5b088db0765e2f1fe891ec3b204be` / `71840ac04dab122480cda3cf422c848c311b51fa2a56a704e6ebd6e7a01a899b` |
| Worker kernel portability, Release / ASan | PASS; 4 cases / 156 assertions each | `18320ed64823ac9da7caf14ce93066539f5d3f20cfd29f40d3896809bafc3cb4` / `8bc8565cb2ab40d6af12a632305b3f3d65f03e6e747abd42e3a597ead6a35cbf` |
| Worker request portability, Release / ASan | PASS; 1 case / 111 assertions each | `1cdaca981cd4542e4d86b6abfa11ef26200724f4362877549b3b8789e0823a15` / `c68ede3f51820953e8c0e36b6523dd9a898917e44d08a9a1fc5edd9f3e61a070` |
| Worker result portability, Release / ASan | PASS; 1 case / 18 assertions each | `00f322a3d493faf6c13aed5ddbcc9cc56a7cb54a599c33c02a9f54175c1382de` / `1c387c1ef216c2be61a4a1b6b41a3f2aaced981bba1a5f64e429530170be1af6` |
| Worker-client cache portability, Release / ASan | PASS; 2 cases / 1,045 and 1,867 assertions | `d3529fdd6ee1e10cb78f2db0a26ff936fb68108b5705a00a28c9bc4122a6f0b8` / `44b85134427b912ef86eb6565eb8a0761e42fe89e3d6a660d998f4d02f1902d2` |
| Fresh-helper process cache portability, Release / ASan | PASS; 1 case / 190 and 216 assertions | `29b7dc250a6676ed7aabd23a6dc98f64d92bca2bc36be61ab672ed67c65d1771` / `0cfb1fd63a7350ffa42dd75495e0787f9d7a41ae113b33c42c53918752586550` |

Logs: `/private/tmp/offset-W117-preview-creation-release.log`,
`/private/tmp/offset-W117-preview-creation-asan.log`, and the matching
`/private/tmp/offset-W117-{kernel,client,process,request,result,creation}-{release,asan}.log` files.

## 80. W118 Reference-Component Publication Allocation-Failure Sweep

Test commit `cb1d9b43f` adds the isolated `--reference-components` fault sweep
and includes it in the standalone executable's default full run. Its fixture is
the established partial-ellipse family with six pair events and four critical
records; the public component analysis must publish successfully before the
sweep begins. Every observed C++ allocation is then failed once, the cumulative
work ledger must remain bounded with budget status still usable, and retrying
with the same budget must publish the complete graph with an exact accumulated
ledger. Release and ASan each passed 6,501 injection/recovery points over 1,373
work vertices.

The current Release and ASan reference-accounting/terminal/component selector
also passes 25,367 assertions. Catch reports 28 test cases in each current
binary; W112 recorded 33 cases with the same assertions, and that historical
case count is not reproducible from the current selector. The no-argument
fault executable was not rerun as a whole after adding this fixture, and this
single rich graph does not close A-reference's broader cross-entry/site audit,
remote platform execution, or final R acceptance. No product defect was
demonstrated.

| W118 lane | Result | SHA256 |
|---|---|---|
| Six-pair/four-critical reference graph fault sweep, Release | PASS; 6,501 injections and same-budget recoveries | `9d88358cc0bde797fe12d166ccfb7fdd75e16a71f03c8d19f74b6b6f1dfd5f86` |
| Six-pair/four-critical reference graph fault sweep, ASan | PASS; 6,501 injections and same-budget recoveries | `9d88358cc0bde797fe12d166ccfb7fdd75e16a71f03c8d19f74b6b6f1dfd5f86` |
| Reference accounting/terminal/component selector, Release | PASS; 28 cases / 25,367 assertions | `e67b1e220024a7365c3e59698502d17aed99f76d4c545d212fd5caa8aba1897c` |
| Reference accounting/terminal/component selector, ASan | PASS; 28 cases / 25,367 assertions | `bb35978c1eb8e3e8b5b4251480d891080c2e3d600952a78f36512ec5c5b4584a` |

Logs: `/private/tmp/offset-W118-reference-fault-release.log`,
`/private/tmp/offset-W118-reference-fault-asan.log`,
`/private/tmp/offset-W118-reference-release.log`, and
`/private/tmp/offset-W118-reference-asan.log`. ASan used `detect_leaks=0`; this
is not leak validation.

## 81. W119 Reference-Event Publication Allocation-Failure Sweep

Test commit `24c409df9` adds the isolated `--reference-events` fault sweep and
includes it in the standalone executable's default full run. It uses the same
established partial-ellipse fixture as W118 and requires the public event
analysis to publish six pair events and four critical records before injecting
failures. Every observed allocation is failed once; each attempt must retain a
bounded ledger and permit a same-budget retry that publishes the complete event
set. Release and ASan each passed 3,545 injection/recovery points over 1,283
work vertices.

The focused semantic `[reference-events]` selector passed 15 cases and 5,653
assertions in both builds. This targeted event path complements W118's component
graph publication path; it does not close A-reference's broader fixture and
cross-entry/site audit, the no-argument full fault executable, remote platform
execution, or final R acceptance. No product defect was demonstrated.

| W119 lane | Result | SHA256 |
|---|---|---|
| Six-pair/four-critical reference event fault sweep, Release | PASS; 3,545 injections and same-budget recoveries | `0e2f8dc74631a68684641f475f497a20c574b1fb0e4510b40ce679682f080f4f` |
| Six-pair/four-critical reference event fault sweep, ASan | PASS; 3,545 injections and same-budget recoveries | `0e2f8dc74631a68684641f475f497a20c574b1fb0e4510b40ce679682f080f4f` |
| Reference event semantic selector, Release | PASS; 15 cases / 5,653 assertions | `83373e010e6d3d93d0216398a69844c456b8ca072821b3682973d993a23d2e93` |
| Reference event semantic selector, ASan | PASS; 15 cases / 5,653 assertions | `7818d9eda26811ed6b6027bc4f88065e41e0ff48081a23b91c3cecee53c5f404` |

Logs: `/private/tmp/offset-W119-events-fault-release.log`,
`/private/tmp/offset-W119-events-fault-asan.log`,
`/private/tmp/offset-W119-events-release.log`, and
`/private/tmp/offset-W119-events-asan.log`. ASan used `detect_leaks=0`; this is
not leak validation.

## 82. W120 Full Standalone Fault-Suite Requalification

After W118 and W119 added reference graph and event sweeps to the default
standalone allocation-failure executable, reran the no-argument executable at
the current source head. Release passed the entire fixture sequence: line,
forward/reversed two-span cubic, standalone bounds/local-topology stages,
forward/reversed whole-circle seams, open/closed two-component trimmed paths,
hidden-cusp refusal, and six-pair/four-critical reference graph and event
publication. All 29,117 allocation injection/recovery points completed, each
with its bounded ledger and same-budget recovery checks.

An aggregate no-argument ASan run was started but intentionally stopped during
the expensive trim-fixture phase (exit 130). It is not a pass and is not needed
to replace the focused ASan runs recorded in W114-W119; those remain the
sanitizer evidence for their individual slices. No product defect was found.
This closes the specific post-W119 aggregate Release omission, but not the
broader A-reference cross-entry/site audit, remote platform execution, or final
R acceptance.

| W120 lane | Result | SHA256 |
|---|---|---|
| Standalone default fault suite, Release | PASS; 29,117 injection/recovery points across all fixtures | `04df4901a1cc4e3dd6e27d6769ba8fee0e9fd144d030b3d56bfbcfc82f0ac076` |
| Standalone default fault suite, ASan | Intentionally stopped during trim fixture; exit 130; not a pass | Not recorded |

Log: `/private/tmp/offset-W120-reference-full-release.log`. ASan was invoked
with `detect_leaks=0`; no leak-validation claim is made.

## 83. W121 A-Reference Cross-Entry Matrix

Reviewed the public reference-provider, event, component, terminal, and
accounting entry points against their named tests, then ran their union at the
current source head. The selector covers the six common conic fixtures,
reversed/rotated/translated variants, contact proofs, event identity and
ordering, component visibility/incidence, terminal continuation, and budget
accounting. Both Release and ASan passed all 60 cases and 35,540 assertions.
This closes the local fixture/cross-entry audit for A-reference; it does not
close remote platform execution, the wider A-C budget audit, or final R
acceptance. ASan used `detect_leaks=0`; no leak-validation claim is made.

| W121 lane | Result | SHA256 |
|---|---|---|
| A-reference provider/contact/event/component/terminal/accounting matrix, Release | PASS; 60 cases / 35,540 assertions | `797d66309d388c4a779816e87d0a8310d3fc0be0ac5cc38feb4ee76ee3e714c7` |
| A-reference provider/contact/event/component/terminal/accounting matrix, ASan | PASS; 60 cases / 35,540 assertions | `c81c28a0cf66d9e8d14f0d4ce13557d15f2ef1e1ed3ea1898556d0bfe17f7d99` |

Logs: `/private/tmp/offset-W121-reference-matrix-release.log` and
`/private/tmp/offset-W121-reference-matrix-asan.log`.

## 84. W122 C-Check Topology and Test-Site Inventory

Ran the complete `[lc-offsetcurve][geometry]` selector at the current source
head after reviewing the cache-check public entry points and their topology,
refusal, budget, and proof-publication tags. Release and ASan each passed all
373 cases and 1,576,298 assertions. The full selector covers more than C-check,
but includes its complete local geometry/test inventory. Combined with W120's
29,117-point standalone allocation-failure run and the focused C-check fault
sweeps from W103/W105/W108/W109/W115/W116, no uncovered local topology or
allocation-failure site was demonstrated. A full aggregate ASan fault-sweep was
not completed (see W120); remote platforms and final R acceptance remain open.
ASan used `detect_leaks=0`; no leak-validation claim is made.

| W122 lane | Result | SHA256 |
|---|---|---|
| Full offset geometry selector, Release | PASS; 373 cases / 1,576,298 assertions | `2469898c236442abf8254e5c80b5b47d8cf3346fde8c431efaff2cafe6ed0119` |
| Full offset geometry selector, ASan | PASS; 373 cases / 1,576,298 assertions | `a01a7f6ff2b014b805241eec9744496a4bb8065b763a16865e3c3641ebdabf82` |

Logs: `/private/tmp/offset-W122-geometry-release.log` and
`/private/tmp/offset-W122-geometry-asan.log`. Both runs emitted six expected
`RS_Spline::changeType: insufficient points` diagnostics from negative-path
tests; Catch reported no failures.

## 85. W123 Asynchronous Trim/Trim Two Action Routing

Ran the complete `[lc-offsetcurve][trim-action]` selector offscreen at the
current source head in Release and ASan. Both passed 68 cases. The action suite
includes native-owner mapping, mixed and paired Trim Two in both pick orders,
preview/commit output parity, unchanged-source seals, one-step undo, cancellation,
stale input and viewport refusal, worker timeout/failure, killed/stopped helper
reaping, action/document lifetime, and successful retry. This confirms that the
native Trim and Trim Two user-action paths submit the owned async worker and
publish only its prepared result; no additional product change was indicated.

Assertion totals differ because the event-loop polling tests count completed
heartbeats: Release passed 20,781 assertions and ASan passed 28,280. Case counts
match, with zero reported errors/failures. ASan used `detect_leaks=0`; no
leak-validation claim is made. The result closes T2's local async-routing gate,
not final visible interaction, remote platforms, or final R acceptance.

| W123 lane | Result | SHA256 |
|---|---|---|
| Trim/Trim Two action selector, Release | PASS; 68 cases / 20,781 assertions | `82c03be818af94b639622a3567b3466a40cbf36e1a59e3721f7c764b7da8a162` |
| Trim/Trim Two action selector, ASan | PASS; 68 cases / 28,280 assertions | `6578c96ec265da9e00cb04be71690d3a09403cec7cdecbd6185c667b4a20c5eb` |

Logs: `/private/tmp/offset-W123-trim-action-release.log` and
`/private/tmp/offset-W123-trim-action-asan.log`.

## 86. W124 Trimmed-Fragment File and Conversion Matrix

Ran `[lc-offsetcurve][trim-files]` offscreen at the current source head. The
same 12 cases and 809,556 assertions passed in Release and ASan. The registered
matrix covers ASCII DXF R12/R14/R2000/R2004/R2007/R2010/R2013/R2018, every
supported DWG writer target, binary DXF output and input, DXF-to-DWG and
DWG-to-DXF conversion, chained modern-version conversions, second
save/reopen, trimmed plain-fragment metadata, and undo/redo restoring native
definitions only when undo restores the original entities. DWG support was
enabled in both tested builds. This requalifies the local T3 matrix, not
independent external-CAD validation, remote platforms, or final R acceptance.
ASan used `detect_leaks=0`; no leak-validation claim is made.

| W124 lane | Result | SHA256 |
|---|---|---|
| Trim file/conversion selector, Release | PASS; 12 cases / 809,556 assertions | `2d81b728ed05c1bf67e38c0c87abfb5261b6d7b5efe54d5854cd1a473d7ed124` |
| Trim file/conversion selector, ASan | PASS; 12 cases / 809,556 assertions | `5279c8f4bd9f30788cf483d31fa90cb9c2c281a7d08d56240fa06f089f6b4582` |

Logs: `/private/tmp/offset-W124-trim-files-release.log` and
`/private/tmp/offset-W124-trim-files-asan.log`.

## 87. W126 Preview-Cache Portability Requalification

At source revision `307cc4c88`, reviewed the owned preview snapshot, bounded
CBOR admission/decoding, cache revision merge, and fresh-helper transport, then
rebuilt and reran the preview/cache selectors. Snapshots contain owned ordinary
line/spline values only; they carry no process-local entity pointers and never
grant native-cache authority. The combined main selector passed 15 cases / 385
assertions in both builds. Dedicated worker-client tests passed 2 cases (677
Release assertions, 1,003 ASan assertions); the fresh-helper process test passed
1 case (191 Release assertions, 212 ASan assertions). No portability or cache
merge defect was demonstrated. ASan used `detect_leaks=0`, so this is not leak
validation. These local checks do not replace packaged-helper installation or
remote platform/final W0-W2 acceptance.

| W126 lane | Result | SHA256 |
|---|---|---|
| Main preview + creation selector, Release | PASS; 15 cases / 385 assertions | `1e6e70d5987a5252b9f1189031937bb89bd91102baf35afcff8e3129eebdb1e0` |
| Main preview + creation selector, ASan | PASS; 15 cases / 385 assertions | `4401b98758b765c1e293320ff04ce864a985e86b09eecac985e7feb0db0a7230` |
| Worker-client preview portability, Release / ASan | PASS; 2 cases / 677 and 1,003 assertions | `3f14f70bebd20754f403400811b01997340cd4206481b8892d35c9151502414d` / `cd1a4460aed54214943fd87aada0a13fe2b71513e96b50b392c6e75dba7df48c` |
| Fresh-helper process preview portability, Release / ASan | PASS; 1 case / 191 and 212 assertions | `73126d637d6c5bcd2bae8d3274290ae13502a976b177ab5e2ab9dc7cad9d26d6` / `db159af62b23bd55d672aca4b75b101c4d5da14fcbcbb2027e7a520fc45b87d2` |

Logs: `/private/tmp/offset-W126-preview-creation-{release,asan}.log`,
`/private/tmp/offset-W126-preview-client-{release,asan}.log`, and
`/private/tmp/offset-W126-preview-process-{release,asan}.log`.

Current-source requalification W137 (plan-only commits since product source
`307cc4c88`; current branch head `403edcb8a`): ran the preview kernel selector
in `librecad_offset_worker_kernel_tests`, the client selector in the
app-bundled `librecad_offset_worker_client_tests`, and the fresh-helper
selector in `librecad_offset_worker_process_tests`. Each dedicated selector
passed in Release and ASan. The umbrella `librecad_tests` command did not
contain the kernel/client tests and therefore is not counted for this receipt;
its matching creation cases are recorded separately under W127. No cache
portability, serialization, merge, or fresh-process defect was demonstrated.
ASan used `detect_leaks=0`.

| W137 lane | Result | SHA256 |
|---|---|---|
| Preview kernel portability, Release / ASan | PASS; 4 cases / 156 assertions each | `28e6e4c4e2fac3345f4a9e3bcece65f04089baf777e4cfbf33ef5648cab840f9` / `af42af51285879f7007990221c259f1cd041e7233fc912128397675137995b77` |
| Worker-client preview portability, Release / ASan | PASS; 2 cases / 677 and 1,011 assertions | `08886fd6eb0ab19e3ff5265acbea354c3b5f45e8f6509886af1048c30cf08f8b` / `91c6c7457b188312593281198bd6f91bb1622cb53297607c3b99408088893fb8` |
| Fresh-helper process preview portability, Release / ASan | PASS; 1 case / 188 and 216 assertions | `1dbfb4a416f8d366389e910d571fa182bad8346d7a00f0ecd7490bd9f1e6a7d9` / `b7c3993b06f7a740034bd79b61bda84a6a1bae52fbc9f69e9a90f8cc4bbb55ab` |

Logs: `/private/tmp/offset-W137-preview-{kernel,client,process}-{release,asan}.log`.

## 88. W127 Creation/Repeat GUI Routing Requalification

At product source revision `307cc4c88`, reran the dedicated
`[creation-worker]` action selector offscreen in Release and ASan. The 11 cases
/ 318 assertions in each configuration cover cache-hit preview followed by
one-cycle native publication, repeated native definitions and zero-sum stored
cache fallback, mixed-source ordering/attributes, stale-input refusal, cancel
and retry, preview supersession and timeout recovery, action/document lifetime,
two-click ellipse creation, and provenance policy. Source review confirmed the
GUI path captures bounded owned inputs, submits the worker, checks the live
action/document/view/selection/options/viewport seal before publication, and
commits only prepared results. No routing gap was demonstrated. ASan used
`detect_leaks=0`; this is not leak validation. Local offscreen coverage does not
close visible user interaction, packaged-helper behavior, remote platforms, or
final W0-W2 acceptance.

| W127 lane | Result | SHA256 |
|---|---|---|
| Creation/repeat GUI action selector, Release | PASS; 11 cases / 318 assertions | `bee0ace13e80d6d7110a38ae58d664b3d5bac54ef1c0cfd12dd19b88ce7d223b` |
| Creation/repeat GUI action selector, ASan | PASS; 11 cases / 318 assertions | `fac8abcdbe645d40af2be6c7d07ebcc118c5ec36840da30aa19d6e96af42e2a6` |

Logs: `/private/tmp/offset-W127-creation-release.log` and
`/private/tmp/offset-W127-creation-asan.log`.

Current-source requalification W138 at branch head `83ef4fd17` (no product
source changes since `307cc4c88`): ran `[creation-worker]` in the dedicated
`librecad_offset_creation_action_tests` executable offscreen in Release and
ASan. Both passed 11 cases / 318 assertions. This independently exercises
cache-hit preview to one-cycle native publication, repeated definitions and
zero-sum fallback, mixed-source ordering, stale-input refusal, cancellation,
timeout and retry, lifetime, two-click ellipse creation, and provenance. No
creation/repeat routing gap was demonstrated. ASan used `detect_leaks=0`; the
offscreen suite does not close physical GUI, packaged-helper, remote-platform,
or final W0-W2 acceptance.

| W138 lane | Result | SHA256 |
|---|---|---|
| Creation/repeat GUI action selector, Release | PASS; 11 cases / 318 assertions | `924067affc5cb330c3140460067adc2564df468b0bfb5d3d2a5a4d862ec902bc` |
| Creation/repeat GUI action selector, ASan | PASS; 11 cases / 318 assertions | `3bab496109fe0ca8059f4d600e9bb14a43902c85bef2a97c917da0ef06ea4839` |

Logs: `/private/tmp/offset-W138-creation-release.log` and
`/private/tmp/offset-W138-creation-asan.log`.

## 89. W128 Native Entity and Factory Requalification (E1)

At source revision `307cc4c88`, reviewed `LC_OffsetCurve` guarded adoption,
consistency stamping over mutable spline representation, clone ownership,
ordinary downgrade behavior, and exact-similarity transform publication. The
combined `[lc-offsetcurve][native],[entity-coverage]` selector passed 44 cases
/ 6,391 assertions in Release and ASan. Its coverage includes invalid/stale
cache refusal, fingerprint mutation, identity and attribute independence,
supported and refused transforms, factories, runtime types, clone preservation,
container expansion, and DXF/DWG round-trip dispositions. No entity/factory
defect was demonstrated. ASan used `detect_leaks=0`; no leak-validation claim is
made. The local slice does not close remote platforms or final R acceptance.

| W128 lane | Result | SHA256 |
|---|---|---|
| Native entity + entity coverage, Release | PASS; 44 cases / 6,391 assertions | `25253c90b8a1aaf64db629040b44aec2950e3a50e187ae5feb1d549e5e2948c2` |
| Native entity + entity coverage, ASan | PASS; 44 cases / 6,391 assertions | `6cf6eced5b007ab1a6dd2200fac46e73d5d773a74b369389a633019589683e41` |

Logs: `/private/tmp/offset-W128-e1-release.log` and
`/private/tmp/offset-W128-e1-asan.log`.

## 90. W129 UI, Selection and Consumer Dispatch Requalification (U1)

At source revision `e2e69ea5f`, reviewed `EntityOffsetCurve` dispatch sites for
type/name registration, INSERT expansion, selection ownership, MakerCAM SVG,
quick info and match properties, context menus, actions, and plugin consumers.
The broad `[lc-offsetcurve][types],[lc-offsetcurve][f-actions],[entity-coverage]`
selector passed 61,271 assertions in 87 cases in Release and ASan; one test was
skipped because this configuration does not build/register the legacy
`RS_FilterDXF` exporter (`dl_dxf.h` and old-format enums are absent). This skip
does not substitute a surrogate writer and is outside U1's SVG/UI dispatch
claim. The focused `[lc-offsetcurve][f-names]` suite passed 16 cases / 463
assertions, and the `[plugins][offsetcurve]` suite passed 16 cases / 19,125
assertions, in each configuration. These cover native type naming, selection
checkboxes and pick descriptors, hiding the control net from generic matchers,
unavailable-value refusal, quick info, owner-normalized popup actions, SVG
cache output, INSERT expansion, plugin spline aliases and staged import. No U1
dispatch gap was demonstrated. ASan used `detect_leaks=0`; no leak-validation
claim is made. Remote platforms and final R acceptance remain open.

| W129 lane | Result | SHA256 |
|---|---|---|
| Type/action/entity dispatch, Release / ASan | PASS; 87 passed, 1 legacy-DXF skip; 61,271 assertions each | `2cac14d8b021ca095dc06ebd9c1db339960cf32e7fba2761f1ab34fe2a12d0b5` / `3634af215d88510c6f321db563c6a1908c2afddf9a8befabc1d2e2b0f7ecd1e4` |
| Native naming/quick-info/menu dispatch, Release / ASan | PASS; 16 cases / 463 assertions each | `e58161ffd61261ec50f2597961d5fe285c55a074243554bf912cd63543a0bcb1` / `39afeeed3776749bcd20fb318e57e57b3c70920341eb7f25936e9f40dc74a8c6` |
| Plugin native consumers, Release / ASan | PASS; 16 cases / 19,125 assertions each | `98599640757961b9caa30054f6186a0c672c0c446ab1b84f5497a3c79dbcfccf` / `c5daaea2bae58dc5eecb9bfda8878762c8a0b724af2e65e8969531290ee641ce` |

Logs: `/private/tmp/offset-W129-u1-{release,asan}.log`,
`/private/tmp/offset-W130-u1-names-{release,asan}.log`, and
`/private/tmp/offset-W130-u1-plugin-{release,asan}.log`.

## 91. W131 Grip, Snap and Transactional Edit Requalification (E2)

At source revision `a9f1f6f82`, reviewed cached grip generation and matching,
read-only through-point proposals, prepared definition edits, and typed
transform staging. Mutators stage complete replacement geometry and metadata
before publication; refused/unchanged operations do not partially alter the
live entity. The selector `[lc-offsetcurve][grips],[lc-offsetcurve][edit],[lc-offsetcurve][transform-tool]` passed 78 cases / 1,197,358
assertions in Release and ASan. Coverage includes directed cache/source
orientation, visible grip order and seam policy, snap output initialization,
large-center ULP behavior, stale/undefined fallback, through-point interior and
endpoint witnesses, remaining-work accounting, no-op preservation, engine/C
failure transactionality, and exact/refused transforms. No E2 defect was
demonstrated. ASan used `detect_leaks=0`; no leak-validation claim is made.
Remote platforms and final R acceptance remain open.

| W131 lane | Result | SHA256 |
|---|---|---|
| Grip/snap/edit/transform selector, Release | PASS; 78 cases / 1,197,358 assertions | `a109482da0b21a82aa45b4b6e17e35a0af1e65b7df84b0140e87e654879a1aa0` |
| Grip/snap/edit/transform selector, ASan | PASS; 78 cases / 1,197,358 assertions | `c29f011af0a796957c86de8c66c8f3303831f561dc8e59dc3fe1e9ec5958600b` |

Logs: `/private/tmp/offset-W131-e2-release.log` and
`/private/tmp/offset-W131-e2-asan.log`.

## 92. W132 Typed Schema, Carrier and Ownership Requalification (F1)

At source revision `2fe9f5d3c`, audited the actual carrier and XDATA
implementations and corrected the F1 test inventory above: the previously named
standalone XDATA/carrier test files are absent; these cases live in adapter,
hostile-file, EED-packet, and entity owned-tail suites. The combined
`[offset-adapter],[lc-offsetcurve][g-hostile-file],[offset-xdata]` selector
passed 104 cases / 390,961 assertions in Release and ASan. It exercises exact
typed schema recognition, unsupported/raw carrier refusal without repair,
duplicate/malformed schema fallback, owned/reserved tails, clone/edit behavior,
byte and item caps, EED packet boundaries, malicious declared counts and
carrier geometry, and staged-import refusal/reporting. Source review found no
unbounded copy before carrier count admission and no native authority granted
by raw cache data without C-check success. No F1 defect was demonstrated. ASan
used `detect_leaks=0`; no leak-validation claim is made. Remote platforms and
final R acceptance remain open.

| W132 lane | Result | SHA256 |
|---|---|---|
| Typed schema/carrier/hostile-file selector, Release | PASS; 104 cases / 390,961 assertions | `b34ed12ed102ea7fda7a9583e1235eb45a1d3132841c144752ca3ce3fa9ade39` |
| Typed schema/carrier/hostile-file selector, ASan | PASS; 104 cases / 390,961 assertions | `8e48993fdb68633635921b70d7961f6a00197ce6d8114215e13687595de314c5` |

Logs: `/private/tmp/offset-W132-f1-release.log` and
`/private/tmp/offset-W132-f1-asan.log`.

## 93. W133 Actual DXF/DWG Writer and Refusal Requalification (F2)

At source revision `81bc797a3`, reviewed the application filter's native
SPLINE write path, schema merge, target codec preflight, R12 downgrade, and
error propagation. Ran explicit adapter export, DWG cross-format second-save,
matrix writer, and persistence save-refusal selectors in Release and ASan.
Both passed 15 cases / 24,307 assertions. This adds current-source write and
transactional-target evidence to W132's adapter/import and hostile-carrier
coverage; it does not claim independent external-CAD verification or close the
conversion/round-trip matrix in F3. ASan used `detect_leaks=0`; no
leak-validation claim is made. Remote platforms and final R acceptance remain
open.

| W133 lane | Result | SHA256 |
|---|---|---|
| DXF/DWG writer and save-refusal selector, Release | PASS; 15 cases / 24,307 assertions | `8d4767b5e9c791b4e851bcbc5bf1c6d28f5aa0aaeeb32e8b9a86bf4adf269aa3` |
| DXF/DWG writer and save-refusal selector, ASan | PASS; 15 cases / 24,307 assertions | `f07da0eb48758c4dfd5d785f472fb20647ced266211d61ae0b34d060592ad24d` |

Logs: `/private/tmp/offset-W133-f2-release.log` and
`/private/tmp/offset-W133-f2-asan.log`.

## 94. W134 Full Version, Fixture and Conversion Matrix (F3)

At source revision `563da4a26`, ran the complete `[lc-offsetcurve][g-matrix]`
selector in Release and ASan. Each passed 22 cases / 60,653 assertions. The
registered matrix checks committed independent fixture hashes, ASCII and binary
DXF adoption, independently generated ODA and ACadSharp DWG input, explicit
DXF/DWG writer versions, ordinary LINE controls, R12 downgrade, native edits,
cross-format conversion, alternating-version chains, and second-save/reopen
behavior. No local version or conversion gap was demonstrated. This is
application/test-reader evidence, not an external CAD application opening
claim. ASan used `detect_leaks=0`; no leak-validation claim is made. Remote
platforms and final R acceptance remain open.

| W134 lane | Result | SHA256 |
|---|---|---|
| Full DWG/DXF matrix, Release | PASS; 22 cases / 60,653 assertions | `b2d4a90d9a758f48f02ca87477ebac8b43133906c0b75fe88bc7ea2e5caf226a` |
| Full DWG/DXF matrix, ASan | PASS; 22 cases / 60,653 assertions | `680920509781592d28a4ab0008d27a8f15983af46465fb6f411c18a25fbc7b85` |

Logs: `/private/tmp/offset-W134-f3-release.log` and
`/private/tmp/offset-W134-f3-asan.log`.

## 95. W135 Dialog and Property-Sheet Async Requalification (U2)

At source revision `c60fef244`, reviewed single/multi selection property
editing, busy-state admission, asynchronous worker coalescing, source/context
revision guards, cancellation, and atomic publication. The combined native
dialog/property-sheet/cursor/refusal/limit selector passed 102 cases / 35,397
assertions in Release and ASan. Coverage includes single and mixed peers,
geometry plus attribute composition, malformed/refused edits, no-op behavior,
live Qt heartbeat, busy edit refusal, stale selection/view/document/undo/source
guards, cancellation/recovery, dialog accept/cancel and undo/redo. No U2 defect
was demonstrated. ASan used `detect_leaks=0`; no leak-validation claim is made.
This does not close physical visible interaction or remote/final R acceptance.

| W135 lane | Result | SHA256 |
|---|---|---|
| Dialog/sheet async property selector, Release | PASS; 102 cases / 35,397 assertions | `3e5e553a7acf6c1efeef1d6a14df3c4fe0e95456905292fc681b2b1a22734866` |
| Dialog/sheet async property selector, ASan | PASS; 102 cases / 35,397 assertions | `fadc1417d460288477212bf352b1dfc02d017e1e41e3195f78eb838d0f34aac9` |

Logs: `/private/tmp/offset-W135-u2-release.log` and
`/private/tmp/offset-W135-u2-asan.log`.

## 96. W136 Native Grip Drag and Late-Publication Requalification (U3)

At source revision `7a4250dcd`, ran the complete `[lc-offsetcurve][h-drag]`
selector offscreen in Release and ASan. Both passed 44 cases. The action suite
covers bounded drag preview, real asynchronous release, unchanged/no-op and
side-flip behavior, one atomic replacement undo cycle, cancellation and fresh
gesture recovery, undo/redo ABA, source deletion, document/view/UCS changes,
late lock/visibility/selection refusal, ownership, and worker/lifetime delivery
after teardown. Release passed 80,920 assertions; ASan passed 91,088 because
event-loop polling assertions differ. No U3 defect was demonstrated. ASan used
`detect_leaks=0`; no leak-validation claim is made. Automated offscreen results
do not close physical drag interaction or remote/final R acceptance.

| W136 lane | Result | SHA256 |
|---|---|---|
| Native drag selector, Release | PASS; 44 cases / 80,920 assertions | `378a4589402d8d224766453119a3bc4aaf7c3402789b500fd5e3ef321f2b1b3c` |
| Native drag selector, ASan | PASS; 44 cases / 91,088 assertions | `bd294c49a2dfc98ee37c74cd4b67eab5fcd9b5c15e52dc945eb450514052dc41` |

Logs: `/private/tmp/offset-W136-u3-release.log` and
`/private/tmp/offset-W136-u3-asan.log`.

## 97. W139 Persisted Editable-Offset Option Requalification (U4)

At branch head `b7400f94c` (plan-only commits since product source
`307cc4c88`), reviewed the action's interactive saved-options lifecycle, direct
API opt-in default, checkbox and compact property-sheet routing, ordinary
preview/native commit split, and guarded one-cycle ellipse publication. Ran
`[gui][offsetcurve][enable]` in the dedicated
`librecad_offset_creation_action_tests` executable offscreen. Release and ASan
each passed 17 cases / 318 assertions. Coverage includes default and persisted
on/off behavior, widget and compact controls, blocked signal restoration,
preview equality across option changes, native commit/undo/redo, plain fallback,
message accounting, and callback lifetime. No U4 product gap was demonstrated.
ASan used `detect_leaks=0`; visible GUI/platform/final R acceptance remains
open.

| W139 lane | Result | SHA256 |
|---|---|---|
| Editable-offset option lifecycle, Release | PASS; 17 cases / 318 assertions | `c39c0c68e46b5d5f34b14b605c056b254269a092b83e7daeeeb5770cb13e91b0` |
| Editable-offset option lifecycle, ASan | PASS; 17 cases / 318 assertions | `3ce882708fcb61aaab73a866a8253311380c991ed43e8d436f376a19cb780c3d` |

Logs: `/private/tmp/offset-W139-u4-release.log` and
`/private/tmp/offset-W139-u4-asan.log`.

## 98. W140 Update-from-Source Requalification (O1)

At branch head `399e2e19b` (plan-only commits since product source
`307cc4c88`), reviewed live-source picking and document/container ownership,
bounded private clone preparation, asynchronous worker receipts, late source
and target seals, atomic multi-target publication, exact no-op behavior,
refusal counts, and the absence of a persisted source link. Ran the full
`[lc-offsetcurve][i-update]` selector in the dedicated
`librecad_offset_update_source_tests` executable offscreen. Release passed 34
cases / 3,100 assertions; ASan passed 34 cases / 3,760 assertions (event-loop
polling assertions vary). No O1 defect was demonstrated. ASan used
`detect_leaks=0`; physical GUI, remote platform, and final R gates remain open.

| W140 lane | Result | SHA256 |
|---|---|---|
| Update-from-Source action, Release | PASS; 34 cases / 3,100 assertions | `eb78d62895799ebc114103481eca5327086d9dd3f786d8a8a975a7a613742402` |
| Update-from-Source action, ASan | PASS; 34 cases / 3,760 assertions | `16c198bfa10344262c9dccb8d83f16d6f5020fcde88ff30e974b706ae10145f8` |

Logs: `/private/tmp/offset-W140-o1-release.log` and
`/private/tmp/offset-W140-o1-asan.log`.

## 99. W141 Restore-Original and Entry-Point Requalification (O2)

At branch head `5acae18bd` (plan-only commits since product source
`307cc4c88`), reviewed Restore Original target eligibility, embedded-source
ownership, effective attributes, same-container live-ellipse deduplication,
local-scale/ULP comparison and explicit exact-circle rules, transaction and
undo, action-factory/menu/property routing, and DXF/DWG persistence. The
`[lc-offsetcurve][i-restore]` action selector passed 35 cases / 3,661 assertions
in Release and ASan. The `[lc-offsetcurve][i-ui]` selector passed 12 cases /
4,811 assertions in both builds, including registered action entry points and
R2018 DXF/DWG Update/Restore save-reopen. No O2 defect was demonstrated. ASan
used `detect_leaks=0`; physical visible interaction, remote platform, and final
R acceptance remain open.

| W141 lane | Result | SHA256 |
|---|---|---|
| Restore Original action matrix, Release | PASS; 35 cases / 3,661 assertions | `9053bfedeabfe1724a43f2b4d23d7c1d7938613b35571ce6332b0e42ae1a8edb` |
| Restore Original action matrix, ASan | PASS; 35 cases / 3,661 assertions | `ecf8ca5fcf6607fc1b7944a6a915bfe44322d90fefe2620416128003e0dc248d` |
| Source-action UI and DXF/DWG persistence, Release | PASS; 12 cases / 4,811 assertions | `57e6169009923c716c0bbbd997346438b334eace53eb81c2b6fe8bffa6315b28` |
| Source-action UI and DXF/DWG persistence, ASan | PASS; 12 cases / 4,811 assertions | `9e84bb20bfecefff8bc47cd83285bda69f2c5765d4ec03dc1b1ac118431fd60b` |

Logs: `/private/tmp/offset-W141-o2-action-{release,asan}.log` and
`/private/tmp/offset-W141-o2-ui-{release,asan}.log`.

## 100. W142 Import Caller and Ownership Inventory

At product source revision `307cc4c88` (subsequent changes in this worktree are
plan-only), audited current in-repository import entry points and their
ownership/publication contracts. Application Open and drawing-from-template
route recognized DXF/DWG through `LC_OffsetImportSession` before creating a
window. Console import/conversion uses the staged session and waits only at the
command boundary. Plugin block import stages recognized DXF/DWG in both the
callback API and the legacy synchronous API (the latter waits for completion
for API compatibility); block-library insertion and supported thumbnail
loading use the staged route. XREF child filters inherit deferred admission,
shared byte budgets and staged snapshot providers. Dimension-style and pattern
previews explicitly use display-cache-only admission.

The remaining `RS_FileIO` and `LC_DocumentsStorage` synchronous APIs operate on
caller-owned graphics and remain compatibility-level low-level entry points;
source search found no current product caller passing a recognized DXF/DWG
through them. Public external callers may still do so, so no claim is made that
arbitrary plugin/API invocation is process-isolated. Converting those APIs
requires a separate ownership/publication design rather than merging a private
staged graphic into a live `RS_Graphic` implicitly. This boundary is explicit
for W0/W2 acceptance.

The combined caller/lifecycle selector passed in Release and ASan. It covers
console import, plugin async and synchronous compatibility routes, thumbnail
and block-library paths, nested XREF handling, staged import worker lifecycle,
and failure/recovery. No missed current product caller or ownership defect was
demonstrated. ASan used `detect_leaks=0`.

| W142 lane | Result | SHA256 |
|---|---|---|
| Import caller/lifecycle Release | PASS; 38 cases / 3,055 assertions | `1bfa02d412d6d3800566ea02ed90cd57831b77ba3ed1d068331e6c8c872ff6ca` |
| Import caller/lifecycle ASan | PASS; 38 cases / 3,661 assertions | `3e4fa66dd9a952ed921941f52a5b1f3a29a1df46098c6d0d3b6e1a7fea6d903a` |

Logs: `/private/tmp/offset-W142-import-callers-release.log` and
`/private/tmp/offset-W142-import-callers-asan.log`.

## 101. W143 Staged GUI Open and Template Ownership

At product source revision `307cc4c88` (no product source changes since; the
intervening commits update this roadmap), reran the offscreen GUI import and
template selectors in Release and ASan. Source review traced `openFile()` and
`newDrawingFromTemplate()` through the staged session: recognized DXF/DWG is
parsed into private owned state, finalized only after successful completion and
source-stamp checks, and then moved into a newly owned document window. A
failed, cancelled, stale, or partial outcome is handled before publishing the
new document; the existing active drawing is not modified. Template imports
use the same staging boundary and template-specific finalization. No GUI
open/template ownership gap was demonstrated.

| W143 lane | Result | SHA256 |
|---|---|---|
| GUI drawing open/template, Release | PASS; 25 cases / 803 assertions | `c4076efd5beb6c777bc303e672070629f087544af47a9271fc984a7d966e4f2e` |
| GUI drawing open/template, ASan | PASS; 25 cases / 952 assertions | `949002db31343878e6bd7ece375eecec7c926acd9a43cfbdd83a4d665ac778fa` |

Logs: `/private/tmp/offset-W143-gui-import-release.log` and
`/private/tmp/offset-W143-gui-import-asan.log`. ASan used
`detect_leaks=0`; these offscreen checks do not close physical visible
interaction, external low-level API compatibility, cross-platform CI, or final
W0-W2 acceptance.

## 102. W144 Current-Master Integration and Requalification

Merged freshly fetched `origin/master` at
`35e7def3d8365b3ef1bb2af54c6071ae3313691e` into the PR branch with merge commit
`64b57ac4e`. The merge was five upstream commits ahead of the prior remote PR
head. The only content conflict was the root CMake test-registration list;
resolution retains both the offset trim action tests and upstream Line Join
tests. The merge also brings upstream hatch/painter rendering and export changes.
`git diff --cached --check` passed before the merge commit.

Rebuilt the application, focused tests, worker helper, caller/fault/timeout
targets in Release and RelWithDebInfo+ASan. Release worker CTest passed 9/9;
focused creation/repeat passed 28 cases / 636 assertions; import callers passed
38 / 4,321; GUI open/template passed 25 / 1,147; the geometry and persistence
matrix passed 22 / 60,653; upstream line-type rendering passed 16 / 2,383; and
upstream Line Join plus hatch/spline export passed 6 / 413. Direct ASan worker
executables passed: wire 7 / 1,022; request 13 / 597,166; result 26 / 467,578;
kernel 18 / 2,728; process 4 / 3,080; client 17 / 9,992; fault 7 / 21,406; and
geometry-timeout 1 / 1,798. The ASan creation/repeat suite passed 28 / 636, and
the combined import, GUI open/template and geometry-matrix selection passed 65
cases / 64,527 assertions. ASan used `detect_leaks=0`; no leak-validation claim
is made. CTest has no registered worker tests in this ASan configuration, so
the listed worker binaries were invoked directly. No offset regression was
demonstrated by these focused lanes.

This is selected source qualification, not the full suite, physical visible
interaction, public external low-level API coverage, remote-platform CI, or
final W0-W2 acceptance. Those remain open; CI for this pushed merge must be
checked separately.

## 103. W145 Windows MSVC Standard-Exception Include

The Windows Pixi build on merge commit `2400fe02c` failed while compiling
`lc_action_modify_trim.cpp`: MSVC could not resolve `std::length_error` or
`std::runtime_error`. Those standard exceptions are used explicitly in this
file, but only arrived transitively on the local Clang toolchain. Commit
`c944266ac` adds the direct `<stdexcept>` include; behavior and exception
handling are unchanged. The failure came from Windows Pixi run
`40894046529`, job `123518198970`. The distinct bounded DWG/DXF Windows lane was
still building at the time this correction was made.

The changed application and test targets rebuilt successfully in local Release
and RelWithDebInfo+ASan. The focused `[lc-offsetcurve][trim-action]` selector
passed 68 cases in both configurations: Release reported 20,743 assertions on
the captured rerun (an earlier execution reported 20,618); ASan reported
28,877 assertions with `detect_leaks=0`. Logs:
`/private/tmp/offset-windows-include-release.log` (SHA256
`8f72750561858a07ffe60b78be856914533789aa136b24f0cafd6c54b49ae800`) and
`/private/tmp/offset-windows-include-asan.log` (SHA256
`b9939d3ce20f28f82b7a9da8cab024577670b4110b52789bdfe61ad11e098c1f`).

This fixes the compile diagnostic locally but does not count as Windows
qualification. The new PR-head Windows build and the still-running Linux,
macOS Intel/ARM, CodeQL, and bounded DWG/DXF lanes must be checked before
closing the cross-platform gate.

## 104. W146 Current-Head Preview-Cache Portability Requalification

At source head `efac323c8`, rebuilt the dedicated kernel, worker-client,
fresh-helper process, and creation-action targets after the current-master
merge and W145 include fix. Ran the preview portability selectors directly in
Release and RelWithDebInfo+ASan. Kernel passed 4 cases / 156 assertions in both
configurations; client passed 2 / 1,161 in Release and 2 / 2,171 in ASan; the
fresh-helper process selector passed 1 / 188 in Release and 1 / 219 in ASan.
No pointer/ownership, serialization, rebinding, merge, eviction, or fresh
process portability defect was demonstrated. ASan used `detect_leaks=0`; this
does not qualify leak behavior.

| W146 lane | Result | SHA256 |
|---|---|---|
| Preview kernel portability, Release / ASan | PASS; 4 / 156 each | `5215b141d3b282bd9f4fcf32d00bb603c0278082ae79830b9dac92b59051658c` / `4e836b1461ec87c80a85b9e59a9f7b8ebcc51c66c79077b63eaf68c630857ded` |
| Worker-client preview portability, Release / ASan | PASS; 2 / 1,161 and 2 / 2,171 | `8509e4b536ef4ef819f3f7711a6a2219a5e3c6ee21367f95885ff6f7dfd5e615` / `d1192e3e50363899b408a4a5ffe45e460600a87c0a4093482164d6c0f4041e0c` |
| Fresh-helper process preview portability, Release / ASan | PASS; 1 / 188 and 1 / 219 | `8b95ea9e0a779d6efa2f5712419d491ef5f39eed88e5b77381c73230bf70c3f7` / `0119ded7979a8de9b716f9de89645c866f747e6ba97bdaf9e819c99f2016ffb3` |

Logs: `/private/tmp/offset-W146-preview-{kernel,client,process}-{release,asan}.log`.
These focused local checks do not close installed-helper deployment, physical
visible interaction, remote-platform CI, or final W0-W2 acceptance.

## 105. W147 Current-Head Creation/Repeat GUI Routing Requalification

At source head `733bb7a2d` (W146's roadmap-only commit; no product source
changes since the W144 current-master integration), ran the dedicated
`[creation-worker]` selector offscreen in Release and RelWithDebInfo+ASan. Both
passed 11 cases / 318 assertions. The selector covers cache-hit preview to
one-cycle native publication, repeated native definitions and zero-sum stored
cache fallback, mixed-source ordering and attributes, stale-input refusal,
cancel/retry, preview supersession and timeout recovery, action/document
lifetime, two-click ellipse creation, and provenance policy. No creation or
repeat routing regression was demonstrated. ASan used `detect_leaks=0`; this
does not qualify leak behavior. Offscreen checks do not close physical visible
interaction, installed-helper deployment, remote-platform CI, or final W0-W2
acceptance.

| W147 lane | Result | SHA256 |
|---|---|---|
| Creation/repeat GUI action selector, Release | PASS; 11 / 318 | `ad6eb77af54758d078ee5bb406b049003dd00ca94cd6c9cb705a65d72f6e60cf` |
| Creation/repeat GUI action selector, ASan | PASS; 11 / 318 | `1752444b033a90fa0ca92ebd854275550bf171c5231fa34e2508875f8dc84ae4` |

Logs: `/private/tmp/offset-W147-creation-{release,asan}.log`.

## 106. W148 Current-Source qmake6 Application and Helper Requalification

At source head `ed0210844` (no product-source changes since the W145
`<stdexcept>` fix), reconfigured the private out-of-source qmake6 directory
`/private/tmp/offset-W55-qmake6` with qmake 3.1 / Qt 6.12.0 and completed the
top-level `make -j4`. AppleClang 21 used the build directory's private macOS
27.0 SDK cache. The application and `LibreCADOffsetWorker` are arm64 Mach-O
files adjacent in `LibreCAD.app/Contents/MacOS`; plugin build steps also
completed. The build log contains toolchain/linker warnings but no compile or
link errors.

Placed the current-source CMake Release `librecad_tests` runner beside the
qmake-built helper and ran offscreen selectors through that directory, proving
application-path helper discovery rather than only helper existence. The
`[worker]` selector passed 15 cases / 3,960 assertions; `[preview-portable]`
passed 4 / 67; `[creation-worker]` passed 11 / 318. The helper itself is the
qmake build; the test runner is CMake-built. This is local macOS Release
qualification, not a qmake test-runner build, ASan helper run, deployed/signed
installation, physical visible interaction, or Windows/Linux runtime result.

| W148 artifact | Result / SHA256 |
|---|---|
| qmake configure, `/private/tmp/offset-W148-qmake-configure.log` | PASS / `62b41930ee476584072b146a292cfeb127071b15dd08f0641fe70883d6c76c08` |
| qmake app/helper/plugin build, `/private/tmp/offset-W148-qmake-build.log` | PASS / `e777682fc5c9bb8cb710d3a50cc8f5ad4e12b0cee0dc80ca675074853386980b` |
| Adjacent qmake helper worker smoke, `/private/tmp/offset-W148-qmake-worker.log` | PASS; 15 / 3,960 / `8f254c770faca6a03b88b52eef3ff8903ae0456dd213dd499e80fbdebf3866b3` |
| Adjacent qmake helper preview portability, `/private/tmp/offset-W148-qmake-preview.log` | PASS; 4 / 67 / `6486a29eda622eabbafaf81fa8c760a16b37b82ce86d1f9b59875dc4ad2079de` |
| Adjacent qmake helper creation/repeat, `/private/tmp/offset-W148-qmake-creation.log` | PASS; 11 / 318 / `ce115f914358ea251e85c1713b2f8f7cce41793a6a0cc7afc46e0bc3871b6745` |

Remote cross-platform CI, installed/deployed helper acceptance, visible UI,
external public synchronous API use, and final W0-W2/R gates remain open.

## 107. W149 Current-Head Broad Release Requalification

At source head `d739b5a33` (no product-source changes since W145), rebuilt the
application and the main/DXF/DWG test targets in the existing Release build.
The full declaration-order suite passed 5,054 of 5,129 cases, with 75 optional
fixture/environment skips and 10,377,016 passing assertions; there were no
failures. The DXF fast lane passed 798 of 801 cases, with three absent optional
SHAPE/ODA-output fixture skips and 84,477 assertions. The DWG fast lane passed
1,021 of 1,025 cases, with four absent optional associative/DataStorage/
visualstyle fixture skips and 362,995 assertions. These selectors exclude
slow, corpus and external groups as recorded in their commands; the run is not
a claim that skipped external fixture coverage passed.

| W149 lane | Result | SHA256 |
|---|---|---|
| Full main suite, declaration order | PASS; 5,054 / 5,129, 75 skipped; 10,377,016 assertions | `ddebbf9cbc017a7c3ff586d64bee736f5aeb749ce352f0488c83b00f24cc125b` |
| DXF fast lane | PASS; 798 / 801, 3 skipped; 84,477 assertions | `0b25bb8042ed4ddb83928b2fa6194edc7ddd1fae0513470e75c960b7db3f0440` |
| DWG fast lane | PASS; 1,021 / 1,025, 4 skipped; 362,995 assertions | `6b0a989e10c2d689d54a2c383ac11bd230d9e83421f0f22cee85bd99c01d9923` |

Logs: `/private/tmp/offset-W149-full-release.log`,
`/private/tmp/offset-W149-dxf-fast.log`, and
`/private/tmp/offset-W149-dwg-fast.log`. The separate randomized-order alias
test investigation is recorded in W150-W151. Remote CI, installed-helper
deployment, physical visible interaction, public external synchronous API
callers, and final W0-W2/R acceptance remain open.

## 108. W150 Randomized Command-Alias Isolation

At the same product-source head `d739b5a33`, ran the `[commands]` selector in
three independently randomized orders (seeds 1, 2 and 3). Each passed all 11
cases and 69 assertions. This confirms the focused alias suite itself is stable
across these isolated orders, but does not by itself establish that it is safe
after arbitrary earlier test mutations in a full process; W151 compares the
broad-order failures against an exact clean `origin/master` build.

| W150 lane | Result | SHA256 |
|---|---|---|
| `[commands]`, seed 1 | PASS; 11 / 69 | `862fe9042c5f1bf6102db673d9727b56a85bed09c70eaf7d525f5d2525e293ee` |
| `[commands]`, seed 2 | PASS; 11 / 69 | `abbc91641ca9b032f3acd1e28308a5060f65dc1760a14418ba0d40621f72fc74` |
| `[commands]`, seed 3 | PASS; 11 / 69 | `7299d526fca06cf4d757d866694ec7b7307c89c421aec9979e5e8416895b21a1` |

Logs: `/private/tmp/offset-W150-commands-seed{1,2,3}.log`. This is focused
Release evidence only; it does not close unrelated command-state behavior in
the complete randomized suite or any W0-W2 acceptance gate.

## 109. W151 Randomized Alias Failures Baseline Classification

Built a fresh detached checkout of exact `origin/master` revision
`35e7def3d8365b3ef1bb2af54c6071ae3313691e` with the local Release CMake
configuration, then ran the full suite with randomized order and seed 1. It
reported 3,766 cases (3,690 passed, two failed, 74 skipped) and five failed
assertions. The five failures are the same alias-test assertions seen in the
current PR-source full randomized run: `rs_commands_tests.cpp` lines 83-84 and
117, 120-121 (`gl` resolves to the pre-existing Break/Divide alias and the
temporary alias-file expectation fails). The source/test definitions for the
command module are unchanged from this upstream revision. The PR source's
declaration-order full suite passes; the isolated command selector passes for
seeds 1-3 (W150). Therefore classify these five failures as inherited broad
test-order/state behavior, not an offset feature regression. No unrelated
command code is changed or waived from offset-specific coverage.

| W151 lane | Result | SHA256 |
|---|---|---|
| Exact `origin/master` test-target build | PASS | `8056f4586ccd68af85cb53a91c9d9f76441f15b456e52216c327454f6a11de29` |
| Full randomized suite, seed 1 | Baseline reproduces the same five assertions; 3,766 cases / 74 skips | `5fe7309e1cf94c3e4513ec129c1fd407018da54f69af59265cbec57bd11d434e` |
| PR-source full randomized suite, seed 1 (comparison) | Same five assertions; 5,129 cases / 75 skips | `416eb8f46f820c0e184dd37ca1661fd1d70836ea7727c0f7bbead7e17d521915` |

Logs: `/private/tmp/offset-W151-master-build.log`,
`/private/tmp/offset-W151-master-random-release.log`, and
`/private/tmp/offset-W151-full-random-release.log`. This classification does
not close remote CI, platform/package deployment, visible UI, public external
synchronous API, or final W0-W2/R acceptance.

The PR is still open and draft at head `069fce4234cff0012af54798718b9ed09caf84ab`.
The GitHub status endpoint currently returns no contexts for that head. The
CodeQL, DWG/DXF verification, and Pixi Builds runs `40903562768`, `40903562797`,
and `40903562899` for earlier product head `d739b5a33` are completed with
`cancelled` conclusions. No passing remote platform result is available for
the current PR head; rerun/obtain the required checks before claiming remote
qualification.

## 110. W152 Full Aggregate Cache-Check Fault Sweep, ASan

Rebuilt and ran the standalone cache-check failure target from the current
worktree at PR head `0556b88c50b9f77f91d5a62e243c5d50e3cf3c06`. The configured
CMake tree is `RelWithDebInfo` with `-fsanitize=address -fno-omit-frame-pointer`;
`librecad_lib` was checked immediately before execution and Ninja reported no
stale work. The no-argument aggregate runner passed every allocation-injection
point, refusal-atomicity check, and same-budget retry across all fixtures: 29,117
injection/recovery points. This closes W120's previously interrupted aggregate
ASan lane; it does not establish leak freedom (`detect_leaks=0`) or close the
broader A-C review, platform, package, visible UI, external caller, or final R
gates.

| W152 lane | Result | SHA256 |
|---|---|---|
| `librecad_offset_cache_check_failure_tests` executable | PASS; ASan; 29,117 injection/recovery points | `68c0f7e6523cc4965f69b569fb78a6abeec868e15f590b7747ca45b2dfcb6e38` |

Command: `ASAN_OPTIONS=detect_leaks=0 /private/tmp/offset-W77-asan/librecad_offset_cache_check_failure_tests`
(exit 0). Fixture counts: single-line 108; two-span cubic forward/reversed 115
each; standalone cache bounds 92; standalone local topology 100; whole-circle
seam forward/reversed 459 each; open/closed two-component trimmed paths 6,031 /
5,497; hidden-cusp endpoint refusal 6,095; six-pair/four-critical reference
graph 6,501; reference events 3,545. Their sum matches the executable's final
`allocation_failure_sweep_all_fixtures_recovered=1 injection_points=29117`.
The updated roadmap's H6 inventory/parser suite also passed:
`node --test scripts/offset-curve-validation/test-refusal-inventory.js`, 99/99
tests, zero failures/cancellations/skips, 71 original classifications retained,
93 executable obligations, and no pending declarations or production mismatches.

The PR remains open and draft at head `0556b88c50b9f77f91d5a62e243c5d50e3cf3c06`.
At the latest check snapshot, Text foundations passed; DWG/DXF verification,
Pixi Builds (including Windows, Linux ARM, macOS arm64 and Intel), and CodeQL
were still in progress. Obtain their terminal results on the pushed head before
claiming remote/platform qualification.

## 111. W153 Durable Source/Target Reconciliation (S0)

Reconciled the live worktrees and PR after W152. Immediately before this
documentation slice, the selected implementation worktree was clean at
`a00692fb703d5745a912cfd69df148335fa44971` on
`codex/offset-2961-native-worker`; PR #3193 was open/draft at that head against
`master` commit `35e7def3d8365b3ef1bb2af54c6071ae3313691e`. The comparison from
product-source head `0556b88c50b9f77f91d5a62e243c5d50e3cf3c06` to that checkpoint
contains only `docs/plan_offset_curve_2961.md`. Thus W152's 29,117-point ASan
fault result is pinned to the same product code now carried by the PR. This
checkpoint changes only the roadmap; it does not change product sources.

| Tree/reference | Observed state | Disposition |
|---|---|---|
| Selected implementation | `/Users/dli/.codex/worktrees/offset-2961-runtime/LibreCAD`, branch `codex/offset-2961-native-worker`; clean at the pre-W153 checkpoint above | Sole implementation/edit target for this roadmap. |
| PR base | `origin/master` = `35e7def3d8365b3ef1bb2af54c6071ae3313691e` | Matches PR #3193 base SHA. |
| Earlier candidate branch | `codex/offset-2961-implementation` = `df0c891ad5a0bb5b0d0e864f4cd9cdd8036d5f83`; its merge-base with the selected worktree is that exact candidate commit | Candidate is integrated into the selected descendant; the old “not integrated” inventory is historical, not current state. |
| Shared main checkout | `/Users/dli/dev/LibreCAD`, `master` at `41678d97626c05ab652ceaa02cf0ac7f7f6205cf`, 204 ahead / 70 behind its configured upstream and with extensive unrelated modifications/untracked files | Read-only reconciliation only; no cleanup, staging, checkout, or edits. |
| v205 recovery worktree | `/Users/dli/.codex/worktrees/offset-2961-v205-recovery/LibreCAD`, `codex/offset-curve-2961-recovery-v205` at `9309885cef66aef0cca0f3c8862e0c210518da3b`; plan has staged and unstaged edits and an untracked history file | Preserved untouched; not the implementation tree or evidence source for W152. |

This closes S0's remaining tree/source ambiguity; it does not close A-core through
C-check's broader audit, W0-W2 external-caller/cross-platform/deployment gates,
E/U/G/F/O/T final integration, or R acceptance. For current PR head
`a00692fb703d5745a912cfd69df148335fa44971`, Text foundations run
`40913350566` passed. Pixi Builds rerun `40913341606` is in progress across
Windows, macOS arm64/Intel, and Linux x64/arm64. The canceled CodeQL run
`40913350472` and DWG/DXF verification run `40913350371` were explicitly
rerun and are queued. Obtain terminal results for all required jobs before
claiming remote/platform qualification; rerunning these workflows does not
close the separate packaged-helper or visible-interaction gates.

## 112. W154 Cross-Platform Installed-Helper Smoke

Product/test commit `3ef93f8ee6408a3600c8b41f37545b4c4f3acadf` gives the
production `librecad_offset_worker` a dedicated CMake install component,
`OffsetWorker`, without changing its install destination. The generated install
script includes this component for component-filtered installs and still
includes it during the normal unfiltered install. The DWG/DXF workflow now
installs only that production component into a temporary path containing spaces,
copies the real client test executable beside it, and runs the
`[lc-offsetcurve][worker-client]` suite there on Linux and Windows. Client
`helperPath()` must therefore discover and exercise the installed sibling,
not a build-tree helper. These new remote steps are pending until the pushed
revision's workflow run completes.

Fresh local macOS CMake Release configuration and build:
`cmake -S . -B /private/tmp/offset-W154-rel -G Ninja -DCMAKE_BUILD_TYPE=Release
-DBUILD_TESTS=ON -DLIBRECAD_ENABLE_OPENTYPE=OFF -DLIBRECAD_REGISTER_OFFSET_WORKER_TESTS=ON`
and `cmake --build /private/tmp/offset-W154-rel --target
librecad_offset_worker librecad_offset_worker_client_tests --parallel 4` both
passed. Installed `OffsetWorker` to `/private/tmp/offset-W154 relocated install`
and ran the client test from the installed app-bundle executable directory,
after copying only the test runner there. JUnit contains 17 testcase elements,
zero failures, errors or skips. The client suite checks the sibling path and
performs real protocol requests.

| W154 artifact | Result | SHA256 |
|---|---|---|
| Installed-helper client JUnit, `/private/tmp/offset-W154-installed-client.xml` | PASS; 17 cases; zero failures/errors/skips | `e903c0272b98f05755b658b704232320520d6bf6b1b40814517f5e05da82767b` |
| Built production helper, `/private/tmp/offset-W154-rel/LibreCAD.app/Contents/MacOS/LibreCADOffsetWorker` | Built and executed as staged sibling | `fd3ab786877a2ea03f94a1468f0d3c8aa7bb88de0ee52b5af3c26fb8ad01dc1e` |

`actionlint .github/workflows/dwg-dxf-verify.yml` and `git diff --check`
passed. This closes local staged helper/client validation and adds Linux/Windows
coverage; it does not yet close remote platform acceptance, the separate full
app/deployed-package checks, visible UI, external synchronous API use, or final
W0-W2/R gates. W155 below closes the local A-core through C-check audit only.

## 113. W155 Shared-Budget and Geometry Audit Requalification

Reviewed the current A-core through C-check ownership chain. `WorkBudget` keeps
separate cache-span, vertex, pair, depth and ordering ledgers; each debit is
sticky, and compound `adoptWork` checks capacity before mutating counters. The
strict intersection kernel receives only the remaining shared limits. Initial
query work and later contraction work are adopted exactly once, including the
entered prefix on exceptions. Critical refinement handles remain tied to the
issuing session and its original budget identity.

Reference construction passes the same budget through own/opposite critical
analysis, event isolation, component/fringe construction, contact analysis,
matching and path search. Cache validation passes one budget through layout,
reference, bounds, local topology and global topology; only the public overload
that accepts limits creates a budget, at the query boundary. Work snapshots stored
in result objects are values. Failure/refusal clears published analysis; path
search reports structural route counts as complete only on exhaustion, while
visitor stop and limit status do not claim global validity.

The audited source and test files have no diff from product source pin
`0556b88c50b9f77f91d5a62e243c5d50e3cf3c06`, which anchors the W120 Release and
W152 ASan fault-sweep evidence. Fresh current-tree CMake Release build and test:

```text
cmake --build /private/tmp/offset-W154-rel --target librecad_tests --parallel 4
QT_QPA_PLATFORM=offscreen /private/tmp/offset-W154-rel/librecad_tests '[lc-offsetcurve][geometry]' --reporter console
```

The selector passed 373 Catch cases and 1,576,298 assertions with no failures
or skips. The JUnit report has 445 case/section entries, zero failures/errors/
skips, and SHA256
`eac522623113d756cc1184b6db71268ecb4814fd9937a8ee988fc345cf5e736d` at
`/private/tmp/offset-W155-geometry.xml`. The built test executable SHA256 is
`c3921a5015ef005289195ca999d9df668fa9af22fff250dc8974105d83ddbcdb`.

The plan parser/inventory regression passed 99/99, with all 71 original
classifications and 93 executable obligations intact, zero pending declarations,
and no production mismatches. `git diff --check` passed.

No new local A-core through C-check defect was demonstrated, so no geometry
product edit was justified. This closes the local audit/requalification row,
not remote platform execution, external public synchronous API usage, or final
W0-W2/R acceptance. The PR-head DWG/DXF workflow remains responsible for the
new Linux/Windows installed-helper smoke.

## 114. W156 Synchronous Filter Import Worker Bridge

Commit `763d3f695` moves direct public synchronous `RS_FilterDXFRW` imports onto
the worker-backed carrier validation path. `fileImportImpl` defers offset
admission while the DXF/DWG parser builds the caller-owned graph, passes one
shared bounded import budget through XREF child filters, and validates admitted
placeholders through `LC_OffsetImportSession` before returning. The compatibility
API waits in a bounded nested Qt event loop that excludes user input; geometry
validation remains in the sibling process. On worker startup failure, timeout,
refusal, or missing Qt application, remaining internal placeholders are replaced
with ordinary `RS_Spline` fallbacks. Deferred INSERT expansion is restored after
validation. GUI drawing-open/template workflows continue to use the asynchronous
staged session. If budget allocation fails, parsing uses cache-only ordinary
spline import rather than in-process editable adoption.

The regression covers public file, caller-owned stream, and byte-array entry
points for both degree-1 and cubic DXF offset carriers. Native type, source
geometry, display cache, attributes, source handle, and owned/foreign XDATA are
checked after each import; an inline worker dependency ensures the test cannot
silently pass by downgrading every carrier. CMake test-helper dependencies cover
the adapter, persistence, hostile-file, matrix, and grouped DXF/DWG fast targets.

Fresh macOS CMake Release build and focused checks passed:

| Lane | Result |
|---|---|
| `[sync-api]` | 1 case, 219 assertions; file/stream/bytes x degree 1/cubic |
| Offset adapter | 21 cases, 9,690 assertions |
| Offset persistence | 55 cases, 40,876 assertions |
| Offset matrix | 22 cases, 60,653 assertions |
| Offset hostile-file | 67 cases, 55,972 assertions |
| DXF fast | 798 passed, 4 existing fixture skips; 84,477 assertions |
| DWG fast | 1,033 passed, 37 existing fixture skips; 363,699 assertions |

The `[sync-api]` selector also passed under ASan with 219 assertions
(`ASAN_OPTIONS=detect_leaks=0`). qmake6 compiled the changed
`rs_filterdxfrw.cpp` and `lc_offsetimportsession.cpp` objects. The full qmake6
top-level build was stopped after its stale build tree began recompiling a broad
set of unrelated library and UI objects; no full qmake link is claimed. The
roadmap parser/inventory regression passed 99/99, with all 71 original
classifications and 93 executable obligations intact, no pending declarations,
and no production mismatch. `git diff --check` passed.

Commits `763d3f695` and `1a7b3c933` are pushed. On PR head
`1a7b3c93348a13e126ae7aeab13653c76e8fd8e0`, Text Foundations passed; CodeQL,
DWG/DXF verification (Linux, Windows x64, and Windows ARM64), and Pixi builds
were still in progress when checked. These are pending, not yet qualification.

This closes the in-repository synchronous filter entry-point routing gap, not
Windows runtime/failure coverage, full deployed-package acceptance, visible UI,
external-CAD interoperability, or final W0-W2/E/U/G/F/O/T/R acceptance.

## 115. W157 Cross-Platform Synchronous Import Regression

Commit `311f75033` adds `librecad_offset_adapter_tests` to the DWG/DXF CI build
and runs its `[sync-api]` selector on Linux plus Windows x64 and ARM64. This
executes the public file, caller-owned stream, and byte-array worker bridge on
each platform rather than only compiling the filter through the grouped format
tests. It also validates the worker helper dependency added in W156. `actionlint`
and `git diff --check` pass locally. W157's new remote jobs remain pending until
the pushed PR-head workflows finish; earlier DWG/DXF runs do not qualify this
new selector.

## 116. W158 Current-Source qmake6 App and Helper Smoke

At source head `41d0089d8d583f5521f5069e18631445699b14d3`, rebuilt the private
out-of-source qmake6 application tree `tmp/offset-2961-qmake-native` against
this worktree. The generated Makefile selects `/opt/homebrew/bin/qmake6`; qmake
reports version 3.1 with Qt 6.12.0. The app and `LibreCADOffsetWorker` are
present as adjacent arm64 Mach-O executables in `LibreCAD.app/Contents/MacOS`.
The top-level `make -C tmp/offset-2961-qmake-native
-j"$(sysctl -n hw.ncpu)"` build completed, and a subsequent
`make -C tmp/offset-2961-qmake-native -j12` recheck exited 0 with no remaining
work.

Ran the adjacent qmake-built `librecad_offset_worker_client_tests` executable
against its sibling qmake helper from the app directory. The real helper-client
selector passed 17 cases / 4,185 assertions. This verifies qmake app/helper
discovery and client behavior, not installed/signed distribution, ASan qmake,
physical visible interaction, or remote platform runtime.

| W158 artifact | Result / SHA256 |
|---|---|
| qmake6 top-level incremental recheck, `/private/tmp/offset-W158-qmake-recheck.log` | PASS / `13eb1bdeda3218d37bf718a5ebf1c6e62a2d22fe00ca8040e9be66e68fadd12d` |
| qmake-built adjacent helper client, `/private/tmp/offset-W158-qmake-helper-client.log` | PASS; 17 / 4,185 / `4ac3ba83019ce574bdd27312f59dc71c45de9a436485f7ebf843b97f522d64be` |

## 117. W159 Current-Source Preview and Creation/Repeat Requalification

At source head `41d0089d8d583f5521f5069e18631445699b14d3`, rebuilt the Release
and RelWithDebInfo+ASan test/helper targets against this worktree. The Release
build recompiled the changed adapter test object; the ASan tree rebuilt the
current test executable and native helper. The current-source preview portability
and GUI creation/repeat selectors pass in both configurations:

| Lane | Release | ASan |
|---|---:|---:|
| GUI preview portability, `[preview-portable]` | 4 cases / 67 assertions | 4 / 67 |
| Worker kernel preview portability | 4 / 156 | 4 / 156 |
| Worker-client preview portability | 2 / 391 | 2 / 1,323 |
| Fresh-helper process preview portability | 1 / 160 | 1 / 171 |
| GUI creation/repeat, `[creation-worker]` | 11 / 318 | 11 / 318 |

Every selector exited 0 with no failure. Client/process assertion totals vary
with exercised polling and elapsed-time observations; case identities are the
stable coverage count. ASan used `detect_leaks=0`, so this is not a leak check.

| W159 artifact | SHA256 |
|---|---|
| GUI preview portability, Release / ASan | `0bc340e7a7cd1e50d371d8eb0b0e214812d1283b8af75b8a0a2c9578c161e08b` / `71533f5c27e6476bf446f8287af2d736f9850701cb0716d26017005467201a41` |
| GUI creation/repeat, Release / ASan | `29b1fdff4db10b276b637ad23108e48a855b6747c482d151c04d1c028e03da6b` / `0c11657c5c507cb983e45871da7086678f152125f5c2e0e7dad324400cc0ab96` |
| Worker kernel preview, Release / ASan | `2353fca52cfb848942656a559bee39525deeec762e3a17c53bc8eea6cafe351c` / `08217d9a17f350fdd9d960744f34a83a35f04975b5f90cb81453ea6368fd8803` |
| Worker-client preview, Release / ASan | `ee2545a062c1e97f5a9029527d1b5b868ee1ed3389330f33b895a9d72cb6efe7` / `bae54074655380ba6cd2d26a25e66e64ff24cd92d4227f052fc60473c28764bc` |
| Fresh-helper process preview, Release / ASan | `cc4e7064fc426adcc6747c26746f5128ce11b682230ee6b5661375b1fd1d0de6` / `5a88ea9b655415c1846e0297f02f5b0f0b057b3b19b06626ba5442b405d44fa5` |
| Roadmap parser/inventory, `/private/tmp/offset-W159-plan-inventory.log` | PASS; 99 / 99, 93 executable obligations, zero pending, no mismatches / `d9e643c38c5515968c442483b7e234eebc22fd95765a6197a17d12f94ffd7195` |

Logs are under `/private/tmp/offset-W159-{preview,creation}-{release,asan}.log`
and `/private/tmp/offset-W159-preview-{kernel,client,process}-{release,asan}.log`.
The roadmap parser/inventory regression passed 99/99; its inventory reports 93
obligations, all 93 executable, zero pending declarations, and no production
mismatches. `git diff --check` passed.
No preview-cache portability or creation/repeat routing defect was demonstrated.
This does not close visible GUI interaction, installed/deployed helper
acceptance, Windows runtime, external-CAD interoperability, or final W0-W2/R.
The W157 PR-head CI run was still active at this checkpoint; after this
roadmap-only commit, the pushed head must be rechecked for current CI status.

## 118. W160 Current-Source Staged Install Helper Requalification

At product source head `41d0089d8d583f5521f5069e18631445699b14d3`, installed
only the production `OffsetWorker` component from the current-source Release
and RelWithDebInfo+ASan CMake trees into separate temporary prefixes containing
spaces. Copied only the matching worker-client test executable beside the
installed helper, then ran the full `[lc-offsetcurve][worker-client]` suite from
each installed app-bundle directory. This verifies `helperPath()` discovery
after relocation rather than using either build-tree helper.

Both installed-client runs passed all 17 cases: Release 3,721 assertions and
ASan 5,150. ASan used `detect_leaks=0`. This closes the local current-source
staged helper/client route in both instrumented and production configurations;
it does not qualify signed packaging, a full app install, Windows/Linux runtime,
or remote CI.
The roadmap parser/inventory also passed 99/99 with 93 executable obligations,
zero pending declarations, and no production mismatches; `git diff --check` passed.

| W160 artifact | SHA256 |
|---|---|
| Release installed helper-client, `/private/tmp/offset-W159-installed-worker-client.log` | `2f010aea56f7f059f642d5b1dab4f85c6ecf626d9b4752bf3c3d02c5a61dd8fb` |
| Release installed helper, `/private/tmp/offset-W159 relocated install/bin/LibreCAD.app/Contents/MacOS/LibreCADOffsetWorker` | `a88af8f02000bc1e90f993d63f0114365e54d9c1a1e111c7ed665bda679b94b8` |
| ASan installed helper-client, `/private/tmp/offset-W159-installed-worker-client-asan.log` | `61bfbe8b05bc6c4b4f962bd23995987ea97ffe19a385f494b4168b2ab4043431` |
| ASan installed helper, `/private/tmp/offset-W159 ASan relocated install/bin/LibreCAD.app/Contents/MacOS/LibreCADOffsetWorker` | `c06edb294686e19f0f43c8ce61757bf6500b47abc8d46aaeb54563dc7d367b5e` |
| Roadmap parser/inventory, `/private/tmp/offset-W160-plan-inventory.log` | PASS; 99 / 99 / `dfddcd7598ec6945ea5d4daa4f877dc79d622354a686f5205a535f891d3b2e15` |

## 119. W161 ACadSharp AC1021 Failure Triage

Re-ran the pinned temporary ACadSharp inspector over the 176 retained current-
source G-matrix artifacts. It reported 163 pass and 13 failures. All 13 failing
files have the `AC1021` magic. Their exception stacks converge on
`ACadSharp.IO.DwgReader.readFileHeaderAC21` line 980, where the pinned ACadSharp
source explicitly throws `NotImplementedException` when `comprLen < 0` at lines
976-980. This is before entity/object decoding and therefore is not evidence of
a LibreCAD spline or entity failure. Do not waive direct AC1021 semantic-reader
acceptance based on this diagnosis.

As a bounded follow-up, ODAFileConverter 27.1.0.0 re-saved those exact 13 files
to ACAD2018 DWG with audit/repair disabled. It emitted 13 files; the pinned
ACadSharp inspector then read all 13 as AC1032. The eight offset-spline rows
retained the expected degree/control/knot layouts and LibreCAD schema marker;
the ordinary line row had one non-spline entity, and the four plot-only rows
had zero modelspace entities. This is conversion-path semantic corroboration,
not direct proof that ACadSharp accepts AC1021, nor a replacement for visible
CAD acceptance. W0-W2/E/U/G/F/O/T/R and the direct AC1021 external-reader gap
remain open.

| W161 artifact | SHA256 / result |
|---|---|
| Direct 176-file ACadSharp inspection, `/private/tmp/offset-W161-acadsharp-stack.log` | 163 pass, 13 AC1021 reader failures / `528c79dae49bc17cc44f040b9b4da8fc5a26dee55bb547eeaa1e49886044bfe3` |
| ODA-resaved-file ACadSharp inspection, `/private/tmp/offset-W161-acadsharp-oda.log` | 13 / 13 pass as AC1032 / `a3734d7b239800651c27fb56b7abd8d8e019e83773d8fc67fc6296da9a38c985` |
| Roadmap parser/inventory, `/private/tmp/offset-W161-plan-inventory-final.log` | PASS; 99 / 99, 93 executable obligations, zero pending/mismatches / `b5a3b7d111052e265bdd36b7b0868e7f3da88170f7554822ff4339fe0332df75` |
| ACadSharp source | commit `3a52a52047e84670759c1f9ad3916ec3ce85ff56`; `src/ACadSharp/IO/DwgReader.cs:976-980` |
| LibreCAD source under test | product code unchanged at `41d0089d8d583f5521f5069e18631445699b14d3`; plan-only head before W161 |

## 120. W162 LibreDWG Direct AC1021 Inspection

Ran LibreDWG `dwgread 0.14` on the same 13 original `AC1021` files that ACadSharp
could not open. All 13 JSON exports completed successfully. Inspected the parsed
JSON rather than treating process exit alone as semantic evidence: eight files
contain one SPLINE each, all eight retain `LC_OFFSET_CURVE` in EED, and their
degree/control/knot layouts are six `(3,4,8)` and two `(1,2,4)`. One file contains
the expected ordinary line entity; four plot-only files have no modelspace
entities. The direct LibreDWG results show this is not a general AC1021 read
failure, while ACadSharp's direct AC1021 gap remains specifically open.

W162 did not change product code. The ODA-resaved AC1032 to ACadSharp check from
W161 remains corroborative only; neither it nor LibreDWG JSON replaces the
remaining visible CAD application and end-to-end DWG acceptance rows.

| W162 artifact | SHA256 / result |
|---|---|
| Direct LibreDWG read list, `/private/tmp/offset-W161-libredwg-direct.log` | 13 / 13 process successes / `408a33a63e6fed5e66dd9720237b23e0fbff04d102996d9113584c85d56e3822` |
| LibreDWG JSON semantic summary, `/private/tmp/offset-W161-libredwg-semantic-summary.tsv` | eight SPLINEs retain schema marker and valid degree/control/knot cardinalities / `f7efdc1f0f118329f0ca943a420c3c86370b156ecb18b4e58ab135d539b6dfa1` |
| Roadmap parser/inventory, `/private/tmp/offset-W162-plan-inventory.log` | PASS; 99 / 99, 93 executable obligations, zero pending/mismatches / `e72525f4decfb8de0506aec9749dab1dff59389ba2ffc7393f87dcc130b7a0a7` |
| LibreDWG tool | `dwgread 0.14`; original files all have `AC1021` signature |

## 121. W163 Focused macOS qmake6 UI and DXF Save/Reopen Check

On the current PR-head qmake6 application, created a temporary ellipse in a
fresh drawing, preselected it, and ran Editable Offset Curve. The red dashed
preview appeared; committing produced a selectable editable offset. Its
property sheet exposed the expected distance and ellipse geometry. Undo removed
the offset and Redo restored it. Saved as DXF 2018 and reopened that output; the
visible curves returned, and the file contains `$ACADVER=AC1032` plus the
`LC_OFFSET_CURVE` schema marker. The saved distance was restored to 10 before
the DXF save.

This is one focused interaction receipt, not completion of the visible UI lane.
No screenshot was retained and display/DPR were not measured. Context menus,
compact options, drag/Escape behavior, locked/stale peers, Update/Restore, Trim,
layer/selection combinations, and Restore Undo/Redo Save/Open remain unqualified.
The follow-up isolated-app launch was denied by the UI tool, so no additional
interaction was attempted.

| W163 artifact | SHA256 / result |
|---|---|
| DXF 2018 save/reopen, `/private/tmp/offset-W160-G-matrix-outputs/offset-W163-ui-check.dxf` | visible reopen succeeded; `AC1032`, `LC_OFFSET_CURVE` present / `d58e9d13b280f8f3b2fcb46ef276a0b251955ad8441fde811edcbbdd61d317a9` |
| LibreCAD source under test | PR head `84fe54790859ce998cd022515d3d26a4167fd6cb`; qmake6 Qt 6.12.0 app |
| Roadmap parser/inventory test, `node --test scripts/offset-curve-validation/test-refusal-inventory.js` | PASS; 99 / 99; 93 executable obligations, zero pending declarations, no mismatches |
| Remaining visible UI acceptance | open; screenshot, display/DPR, and full interaction matrix not captured |

## 122. W164 Independent Reader Checks

Used ACadSharp commit `3a52a52047e84670759c1f9ad3916ec3ce85ff56` to parse the
actual W163 DXF 2018 save. ACadSharp returned three modelspace entities, two
splines carrying `LC_OFFSET_CURVE` XDATA, and valid knot counts for both (degree
3; 4 and 88 controls). This output was saved by the qmake6 product at source
`41d0089d8d583f5521f5069e18631445699b14d3` and is the current-source DXF
external-reader result.

Also directly read the retained W134 DWG writer matrix: four degree-1/edited and
degree-3/edited outputs for each of R2000, R2004, R2007, R2010, R2013 and R2018.
LibreDWG 0.14 accepted all 24 and its JSON retained one SPLINE, one owned marker,
and valid degree/control/knot cardinalities in every file. ACadSharp accepted
20/24. All four R2007/AC1021 inputs throw `NotImplementedException` in
`DwgReader.readFileHeaderAC21` at line 980 before entity decoding; the source
comment there explicitly identifies negative `ComprLen` as unimplemented. The
matrix files were generated at W134 source `563da4a26`, not the current product
source. These reads supplement historical matrix evidence and do not close
current-head DWG interoperability or direct AC1021 ACadSharp acceptance.

| W164 artifact | SHA256 / result |
|---|---|
| Current-source W163 DXF, `/private/tmp/offset-W160-G-matrix-outputs/offset-W163-ui-check.dxf` | ACadSharp 3/3 entities; 2/2 marker splines with valid knots / `d58e9d13b280f8f3b2fcb46ef276a0b251955ad8441fde811edcbbdd61d317a9` |
| Retained DWG matrix + LibreDWG, `/private/tmp/offset-W164-libredwg-dwg-writer-matrix.tsv` | 24/24 accepted with semantic spline checks / `1e449466b6e8b5761b54198e5759740553b6ca0fe16bea103db3293d6c8bdc2d` |
| Retained DWG matrix + ACadSharp, `/private/tmp/offset-W164-acadsharp-dwg-writer-matrix.tsv` | 20/24 accepted; four AC1021 header exceptions at `DwgReader.cs:980` / `3c47a6e768bfb3be0ef5ac31e1a025d2076fa19e0bd58b4feb8467b0f7316a80` |
| Source pins | LibreCAD DWG matrix `563da4a26`; W163 DXF product `41d0089d8d583f5521f5069e18631445699b14d3`; ACadSharp `3a52a52047e84670759c1f9ad3916ec3ce85ff56` |

## 123. W165 ODA Conversion of Current-Source DXF

Copied the W163 current-source DXF to an isolated temporary input directory and
ran ODA File Converter 27.1.0.0 to ACAD2018 DWG. It exited successfully and
produced a file with the physical `AC1032` signature. LibreDWG 0.14 parsed it
and recovered both SPLINEs and their `LC_OFFSET_CURVE` markers. Pinned ACadSharp
also parsed three modelspace entities and both marker splines.

There is one reader-validator discrepancy worth retaining: the 88-control
spline has 92 knots and coincident first/last controls in the input DXF, whose
group 70 is planar-only (`8`, not closed). ODA marks that spline closed in the
DWG, keeps the same control/knot arrays, and LibreDWG independently reports the
closed bit. ACadSharp's `HasValidKnotCount` property then returns false for that
spline; the 4-control open spline validates. ODA and LibreDWG agree on the DWG
closed flag, while ODA's conversion did not alter the count arrays. ODA spec
§20.4.40 documents the closed bit and the two counts independently and does not
state ACadSharp's closed-curve cardinality formula. Do not classify the output
as malformed based only on this validator; retain the caveat for future semantic
round-trip tests.

This verifies an external DXF reader/conversion path and independent parsing of
the converted DWG. It does not qualify LibreCAD's direct DWG writer or replace
visual inspection by an external CAD application.

| W165 artifact | SHA256 / result |
|---|---|
| ODA AC1032 DWG, `/private/tmp/offset-W164-oda/out/offset-W163-ui-check.dwg` | generated from current-source W163 DXF; AC1032 / `cd365d451e34973413e5ab2101a69bb101634e42c0572f336133ae52a4294a03` |
| LibreDWG semantic summary, `/private/tmp/offset-W165-oda-reader/libredwg.json-summary` | 2 SPLINEs; one marker each; degrees 3; arrays `(4,8)` and `(88,92)`; second closed / `e73731262e9942c143bd673aa8207a8ca52a10d13b411724259fcabfdcef7452` |
| ACadSharp read result, `/private/tmp/offset-W165-oda-reader/acadsharp.txt` | 3 entities; 2 marker splines; parser succeeds; `HasValidKnotCount` true/false for the open/closed splines / `0f382e6b9049a1d2d4c4127d5f452d2c7af73eae4a8118db43c2a4db74beb333` |
| Input DXF | `/private/tmp/offset-W160-G-matrix-outputs/offset-W163-ui-check.dxf`; `AC1032`; hash recorded in W163/W164 |
| DWG specification | `~/doc/dwg/dwg.pdf`, §20.4.40, printed page 142 |

## 124. W166 Current-Head DWG Writer Interoperability Matrix

Built the full CMake `librecad_tests` target from PR head
`3ecd80331b52eda2187a7f8960ae36287df42930` and ran the direct DWG writer and
edited-cache second-save matrix cases. Both passed locally. To preserve the
generated files for independent readers, the test-only temporary-directory
cleanup was bypassed locally and then restored. The DWG writer/product sources
match the PR head; W163-W166 changed only roadmap documentation. W167 below is
a validation-test refactor, not a product DWG code change.

LibreDWG 0.14 independently parsed all 24 current-head outputs: six physical
versions (AC1015, AC1018, AC1021, AC1024, AC1027, AC1032), degree-1 and
degree-3 initial writes, and their edited second-save outputs. Every file has
the expected physical version, exactly one SPLINE and one `LC_OFFSET_CURVE`
marker, an open-curve flag, and coherent degree/control/knot counts. Initial
degree-1 rows decode as 1/2/4 and degree-3 rows as 3/4/8. The edited rows
decode as cubic 3/4/8 splines, including rows whose source was degree 1; the
local test contract intentionally allows the edited native cache to choose
either supported spline representation.

Pinned ACadSharp independently parsed the 20 files outside AC1021 with one
marker spline, valid knot counts, and the expected geometry. Its four AC1021
exceptions (initial/edit, degree 1/3) occur in
`DwgReader.readFileHeaderAC21` at line 980 before entity decoding, on the
known negative-`ComprLen` implementation gap. This is not an entity-level
failure. Keep direct AC1021 ACadSharp acceptance open until that upstream
reader supports this header encoding or another independent reader qualifies
those exact files. This matrix is binary semantic-reader evidence, not visual
inspection in an external CAD application.

| W166 artifact | SHA256 / result |
|---|---|
| Current-source writer matrix, `/private/tmp/offset-W166-retained-testtmp/write.log` | PASS; 5,822 assertions / `07b4042f8929e629e9de0638b93ed5d9661198d4805720235b2630fd19d43ae4` |
| Current-source edited second-save matrix, `/private/tmp/offset-W166-retained-testtmp/edit.log` | PASS; 5,414 assertions / `ffceade833c88e9a8e7f160192751738a73d5997d403b29a56b369591e401b4f` |
| 24-file DWG SHA256 manifest, `/private/tmp/offset-W166-current-dwg-manifest.sha256` | 24 artifacts / `72643b0298b3f089442b32c645c6e31c41b356992fdb9130710c655ed8b18c90` |
| LibreDWG semantic matrix, `/private/tmp/offset-W166-libredwg-current/summary.tsv` | 24/24 parsed and validated / `9d3aa5463e33d40277819354bd71fb6a72d0a6bf6661b94e16a0a1d29f81d515` |
| ACadSharp semantic matrix, `/private/tmp/offset-W166-acadsharp-current/summary.tsv` | 20 parsed, 4 AC1021 header exceptions / `cc35e65eb5a0f54c1401073fefb29df36bd93121c7ee87599a31c3d12f0c6ded` |
| Reader/source pins | LibreCAD PR head `3ecd80331b52eda2187a7f8960ae36287df42930`; LibreDWG `0.14`; ACadSharp `3a52a52047e84670759c1f9ad3916ec3ce85ff56` |

The visible external-CAD acceptance remains open. CUA reported the Mac locked
and could not unlock it, so no attempt was made to bypass the visible UI tool;
the previously retained W163 focused UI check remains the only qualified
visible interaction receipt. Full drag/cancel, context-menu, compact-options,
locked/stale peers, Update/Restore, Trim, selection/layer combinations,
Undo/Redo/Save/Open, screenshot/DPR, and external-CAD visual checks remain
unqualified. W0-W2 platform/package gates and final R acceptance also remain
open.

## 125. W167 H6 Discovery Test Separation

The first full validation run found one failure: parser unit tests had been
added to `test-refusal-inventory.js`, which is also the H6 worker-discovery
module. Its unlabelled parser tests were interpreted as production H6 refusal
cases, but only the inventory sentinel is permitted to be unlabelled. Extracted
`parseRoadmapOriginalRows()` to `roadmap-inventory-parser.js`, moved its five
unit tests to `test-roadmap-inventory-parser.js`, and kept the H6 module's
unlabelled inventory sentinel first. The worker-discovered refusal inventory
remains at 93 executable obligations with zero pending declarations; no
discovery exception or extra refusal ID was added.

| W167 validation | Result |
|---|---|
| H6 refusal plus standalone roadmap-parser tests | PASS; 99 / 99; 93 executable; zero pending; zero mismatches |
| Full validation suite, `node --test scripts/offset-curve-validation/test-*.js` | PASS; 412 / 412 / `c400d17c6c766cc75a8620f9e9a5fcb2fb6329ea2fe0c4bea3bce569e0d8abe6` |
| Focused qualification-generation regression | PASS; reserved temp-root snapshot reached the real preflight boundary |
| Code commit | `1a0d3d1bfaf24f9cfb68ea5a80175575570170e1` |

This closes the local validation-test regression only. It does not close the
visible CAD, cross-platform CI, installed-package, external AC1021 reader, or
final W0-W2/R acceptance gates listed in W166.

## 126. W168 Windows Worker/Wire CI Remediation

PR-head Windows x64 job `123694069995` failed its worker and import lanes. The
first causes were `QCborStreamReader` depth/item traversal that did not enforce
the same structural limits on Windows, plus the worker's `QFile::open(stdin / stdout)`
wrappers failing on redirected helper pipes. Their failures cascaded into worker
client/fault/timeout and native-offset import assertions. A separate Windows-only
identity test wrote through the same still-open handle and assumed the filesystem
published metadata changes synchronously.

The committed remediation replaces pre-materialization wire traversal with a
bounded scanner for the protocol's fixed arrays and permitted scalar types. It
enforces frame, depth, per-array and total-item caps before calling
`QCborValue::fromCbor`, rejects unsupported/narrow/nonfinite values and trailing
bytes, and validates encoded payloads through the same wire grammar after a
bounded typed-tree precheck. The depth-boundary test uses explicit CBOR bytes at
exactly the admitted depth and one level beyond. The helper reads and writes
redirected process handles directly (`ReadFile`/`WriteFile` on Windows and
`read`/`write` on POSIX), avoiding CRT text-mode and Qt `FILE*` pipe behavior.
The Windows identity test keeps the stamped descriptor open while a distinct
writer handle changes the file and closes, then checks both descriptor and path
stamps.

Local toolchain: CMake 4.4.4, Qt 6.12.0, AppleClang 21, macOS arm64. The
isolated Debug build is `/private/tmp/librecad-offset-2961-ci`; the isolated
ASan wire build is `/private/tmp/librecad-offset-2961-wire-asan`.

```bash
cmake --build /private/tmp/librecad-offset-2961-ci --target librecad_offset_worker --parallel 12
cmake --build /private/tmp/librecad-offset-2961-ci --target librecad_offset_worker_process_tests librecad_offset_worker_client_tests librecad_offset_worker_fault_tests librecad_offset_worker_geometry_timeout_tests librecad_offset_hostilefile_tests librecad_console_command_utils_tests librecad_dxf_fast_tests --parallel 8
ctest --test-dir /private/tmp/librecad-offset-2961-ci -R '^(offset_worker_(wire|process|client|fault|geometry_timeout|process_ordinary)|librecad_(console_import_tests|offset_import_identity_tests|dxf_fast_tests))$' --output-on-failure --parallel 1
cmake --build /private/tmp/librecad-offset-2961-wire-asan --target librecad_offset_worker_wire_tests --parallel 4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /private/tmp/librecad-offset-2961-wire-asan -R '^offset_worker_wire$' --output-on-failure
```

| W168 validation | Result |
|---|---|
| Debug CMake wire selector | PASS; `offset_worker_wire` 1/1 |
| CMake ASan wire selector | PASS; `offset_worker_wire` 1/1 (`detect_leaks=0`) |
| CMake worker/helper and process/client/fault/geometry-timeout test builds | PASS |
| Focused CTest: wire, process, client, fault, geometry timeout, ordinary fallback, console import, source identity, DXF fast | PASS; 9/9, 85.73s |
| Post-freeze helper rebuild and process rerun | PASS; process plus ordinary fallback 2/2 |
| Product/test commit | `4adedea92e7262d55cd9296f43987e85533733e2` |
| Windows x64/ARM64 PR-head rerun | Pending; local macOS execution does not qualify Windows handle behavior |

W168 closes the local diagnosis/remediation slice only. Recheck the pushed
Windows x64 and ARM64 jobs, especially source identity and the prior worker
timeout/fault chain, before closing current-head cross-platform CI. Remaining
visible CAD workflow, deployed/package checks, direct AC1021 ACadSharp reading,
external-CAD visual checks, and final W0-W2/R acceptance are unchanged.

## 127. W169 Fresh qmake6 Full Build and Bundled Helper Exchange

At PR source `9d983e8e027a890930a25eacb3baa18b8b271655`, configured a new
out-of-source top-level qmake6 tree at
`/private/tmp/offset-2961-qmake-final-9d983e`. The toolchain is qmake6/Qt6.12.0,
AppleClang21, macOS SDK27.0, arm64. The fresh `qmake6 librecad.pro` plus
`make -j4` produced the complete application and its sibling
`LibreCADOffsetWorker` in `LibreCAD.app/Contents/MacOS`, along with the plugin
and tool targets. A subsequent top-level `make -j4` completed with exit0 and
reported all generated targets up to date.

Launched the helper from that new app bundle with a valid four-byte-length-
prefixed protocol-v4 Generate request for a small elliptical arc. The request
used token `[7,11,13,17]` and the full typed definition/run-limit layout. The
helper exited0; decoding its response confirmed protocol4, the exact token,
operation0, failure0, a 15-item typed payload and a 530-byte frame. A separate
empty-array request was rejected with exit2 and `invalid request`, as expected.
The isolated CMake Debug tree `/private/tmp/librecad-offset-2961-ci` also built
the complete `librecad` app target (161 build steps); an incremental rebuild
returned exit0 with `ninja: no work to do.` The resulting app binary was hashed.
Installed only the `OffsetWorker` component into
`/private/tmp/offset-2961 current installed`, copied the client test beside the
installed helper, and ran it from the relocated app-bundle directory. All 17
worker-client cases passed (11,059 assertions), exercising sibling discovery
with a path containing spaces.
This is local build and bundled-helper evidence, not a signed/deployed bundle on
another machine or a GUI interaction receipt.

| W169 artifact | SHA256 / result |
|---|---|
| qmake6 application binary | `/private/tmp/offset-2961-qmake-final-9d983e/LibreCAD.app/Contents/MacOS/LibreCAD` / `ead4a2a24abea643fead31136b0fedc49ce99d610406bd14cb50ee437bf893a1` |
| qmake6 worker binary | `/private/tmp/offset-2961-qmake-final-9d983e/LibreCAD.app/Contents/MacOS/LibreCADOffsetWorker` / `f468b857359de8b76e86357b8efda545d114b538c28928ef2983cf1739ea767c` |
| CMake Debug application binary | `/private/tmp/librecad-offset-2961-ci/LibreCAD.app/Contents/MacOS/LibreCAD` / `c66fc9a46f29be6a46207a72cb6b696e16eb8e1f5912bff0c3b4c7763df9371c` |
| Relocated installed helper-client | PASS; 17 cases, 11,059 assertions; component `OffsetWorker` from the current Debug tree; install prefix contains spaces |
| Valid helper exchange | protocol4; op0; failure0; 15 payload items; frame530 bytes |
| Empty-array negative control | helper exit2; `invalid request`; no response frame |
| Full validation-script suite | PASS; `node --test scripts/offset-curve-validation/test-*.js`; 412/412, 0 failures, 195.2s |
| Roadmap parser focused suite | PASS; 5/5; `git diff --check` clean |
| Native CTest worker/import/format selector | PASS; 9/9, 81.05s (`offset_worker_wire`, process/client/fault/timeout/ordinary, console import, source identity, DXF fast) |

The earlier workflow `40953169555` tested the same product source before a
documentation-only commit and was superseded by branch concurrency. Its
cancelled jobs are not acceptance results. Headless OFF and ON passed on the
exact current head in `40957917654`. Do not close W168/current-head CI until the
current run below completes successfully.

The plan-only follow-up `efe1a1bb007ca1977ceb2681a72c6db38eaf4ca4` triggered
exact-head workflow `40957917479`. At this checkpoint its Ubuntu bounded-lane
build is in progress; Windows x64 has completed dependency setup and is
configuring/building; ARM64 has completed Qt and Boost setup and is
configuring/building. No failure has surfaced. Exact-head Headless OFF and ON both passed
(`40957917654`). The earlier `40953169555` run tested the identical product
source before this documentation-only commit; its ARM job was
superseded/cancelled by branch concurrency and is not current-head acceptance.
CodeQL `40957917558` and Pixi cross-platform build checks `40957917529` remain
in progress without surfaced failures. `40957909720` is a duplicate Pixi run
triggered by branch activity; use the exact-head run IDs above for acceptance.
Use `40957917479` for the next format/helper CI checkpoint.

The desktop became unlocked, but CUA still returned
`Computer Use was not approved to use LibreCAD-PR3193-visible` after the user
confirmed approval. No alternate UI route was attempted. The full visible drag,
cancel, properties, creation/repeat, Update/Restore, Trim, save/reopen and
undo/redo matrix remains open, along with display/DPR screenshots and external
CAD visual inspection. Also still open: signed/deployed helper acceptance,
direct AC1021 parsing by pinned ACadSharp (its known pre-entity header exception),
external CAD acceptance not already covered by semantic readers, and final R/W0-W2
acceptance. W169 closes only fresh local qmake6 app/helper build and a bundled
helper wire exchange; it adds no product-source change.

## 128. W170 Windows ARM64 Qualification Fixture Remediation

Exact-head workflow `40960582569` exposed three failures in its Windows ARM64
bounded test lanes (`123751984392`): wire depth/item expectations, the worker
fault-helper lifecycle suite, and the geometry-entry timeout witness. The
production worker was not implicated by those diagnostics. The wire test had
relied on nested `QCborArray` initializers whose nesting was not made explicit;
it now builds nested arrays with `append`, asserts canonical bytes `81818101`,
and explicitly appends the deeper wrapper used by the depth-boundary case.
This makes the structural-limit test independent of initializer-list overload
resolution.

The isolated fault helper still used `QFile::open(stdin/stdout/stderr)`, the
same redirected-pipe path already replaced in the shipped helper by W168. It
now reads and writes native Windows standard handles (`ReadFile`/`WriteFile`)
and POSIX descriptors, preserving complete writes and retrying interrupted
POSIX syscalls. The geometry timeout test's diagnostic regex now accepts both
LF and CRLF. No product behavior changed in this slice.

| W170 validation | Result |
|---|---|
| Focused CMake Debug CTest: `offset_worker_wire`, `offset_worker_fault`, `offset_worker_geometry_timeout` | PASS; 3/3, 19.89s |
| Focused CMake Release CTest, same selectors | PASS; 3/3, 9.52s |
| Focused CMake ASan CTest, same selectors (`ASAN_OPTIONS=detect_leaks=0`) | PASS; 3/3, 12.21s |
| Fresh current-source headless Release application suite | PASS; 6,381 JUnit cases, 0 failures/errors, 75 optional-fixture/legacy-path skips |
| Roadmap parser/inventory regression | PASS; 5/5 |
| `git diff --check` | PASS |
| Code commit | `4e107daba` (`fix offset worker Windows test fixtures`) |
| Exact-head Windows x64/ARM64 and Linux rerun | Historical workflow `40971000486` was superseded; current exact-head run `40979668978` is tracked in W173 |
| Visible LibreCAD app control | Still unavailable: this task exposes no computer-use control tool. The user said they will approve the app; no alternate UI path used |

The code and plan commits are `4e107daba` and `091686cd7`; their exact-head
workflows were superseded by later pushes. W170 closes the local test-fixture
remediation only. Current cross-platform results are tracked in W173.
The full visible workflow, signed/deployed package acceptance, current-source
external-CAD checks (including AC1021's reader limitation), and final W0-W2/R
acceptance remain open.

## 131. W173 Windows Source Identity Fixture Determinism

Exact-head workflow `40973686209` exposed one Windows ARM64 failure in
`librecad_offset_import_identity_tests` after the CMake test lanes built
successfully. The Windows-only open-handle test rewrote one byte without
changing file size, then expected metadata captured from the open descriptor
and the path to differ. That assertion depends on observing a same-size
timestamp update and failed on ARM64; the production identity guard was not
changed.

The fixture now opens a separate append handle and writes one byte. This gives
the identity comparison a deterministic size change while retaining the
original stamped descriptor open and checking both descriptor and path
identity. POSIX same-size rewrite coverage remains in its dedicated test,
including restoring mtime and asserting that change-time detects the rewrite.

| W173 validation | Result |
|---|---|
| Local macOS CMake build of `librecad_tests` | PASS |
| Local `librecad_offset_import_identity_tests` | PASS; 1/1 |
| Windows ARM64 exact-head rerun | PASS; workflow job `123816799173`, source-identity CTest passed in 23/23 CTest tests |
| Windows x64 exact-head format lane | Workflow `40979668978`, build complete; combined bounded-test step active |
| Linux exact-head format lane | Workflow `40979668978`, build complete; combined bounded-test step active |
| Pixi cross-platform build | PASS; workflow `40979669043`, all five OS/architecture jobs |
| Roadmap inventory/parser | PASS; 5/5 after this checkpoint edit |

W173 is a test-fixture portability correction only. The exact-head Windows
ARM64 source-identity selector now passes; cross-platform identity acceptance
remains pending Windows x64 and Linux.
Visible LibreCAD UI acceptance remains open. The user said they will approve the
test app, but this task exposes no computer-use control tool; no alternate UI
path was used.

## 132. W174 Exact-Head Windows ARM64 Worker and Format Qualification

The Windows ARM64 job in exact-head workflow `40979668978` completed
successfully at `2026-10-30T02:45:48Z` (`123816799173`). Its configure/build,
combined test step, and installed-worker exercise all passed. The passing CTest
set contained 23/23 tests, including `librecad_offset_import_identity_tests`.
Additional direct selectors passed: synchronous import (`[sync-api]`, 219
assertions), Modify Offset creation/repeat (`[lc-offsetcurve][creation-worker]`,
318 assertions in 11 cases), and the standalone worker-client suite against the
relocated installed helper (17 cases, 3,799 assertions).

Both allocation-failure processes also completed successfully on ARM64. The
stored-cache trim sweep covered four directions, 2,120 staging allocation
failures, 20 cancellation points, four conic allocation failures, four conic
cancellation points, and the wrapper's last-successful-allocation cancellation.
The cache-check sweep recovered all 66,594 allocation-injection points across
its fixtures. This is full Release fault-sweep evidence on Windows ARM64; it is
not an ASan or leak-freedom claim.

| W174 lane | Result |
|---|---|
| Configure and bounded test-target build | PASS; Windows ARM64 |
| CTest | PASS; 23/23, including source identity |
| Synchronous import and creation/repeat selectors | PASS; 219 and 318 assertions |
| Cache trim allocation/cancellation sweep | PASS; 2,120 allocation failures, 20 cancellations, conic and wrapper tail checks |
| Cache-check allocation sweep | PASS; 66,594 injection points |
| Relocated installed-worker client | PASS; 17 cases, 3,799 assertions |
| Previous c043 Linux and Windows x64 jobs | Canceled when the corrected Release configuration was pushed; no result claimed |

The Windows ARM64 pass is code-equivalent evidence from c043; the new PR head
must pass its own three-platform workflow before cross-platform or final W0-W2/R
closure. Visible GUI and external-CAD visual checks remain open.

## 133. W175 Linux Bounded CI Release Configuration

The Linux CMake/Ninja configure in `.github/workflows/dwg-dxf-verify.yml` omitted
`CMAKE_BUILD_TYPE`, unlike the Windows matrix's explicit Release configuration.
The exact-head Windows ARM64 Release lane passed all 66,594 cache-check fault
injection points in about 86 minutes, while the Linux job's combined test step
had been active for more than two hours. This is consistent with the Linux
single-config build using an unoptimized default for the same exhaustive
geometry/fault-injection work; it is a runtime diagnosis, not evidence of a
Linux test failure.

Commit `2ecef4430` sets the Linux bounded lane to `CMAKE_BUILD_TYPE=Release`.
It retains every test target and injection point and matches the Windows build
configuration. `actionlint .github/workflows/dwg-dxf-verify.yml` and
`git diff --check` pass locally. Workflow `40979668978` was canceled after the
push before Linux/x64 completed; neither canceled lane is counted as a result.
The new exact-head workflow `40998099806` on `7bd61bd24` is active and contains
the Release configuration; verify its Linux lane from terminal logs.

| W175 check | Result |
|---|---|
| Workflow syntax | PASS; actionlint |
| Whitespace/error check | PASS; git diff --check |
| Exact-head Linux Release run | Active; workflow `40998099806` |

## 134. W176 Current-Source Validation-Tool Regression

Ran the complete optional Node validation-tool regression suite against product
source `7bd61bd24ba729d779822928d818c3a93cbc4938` (worktree head
`654d83fca68e1b111621e09e90166ad8c5050ffc`; the only local change relative to
the PR head is this roadmap checkpoint). Node `v26.12.0` on macOS arm64 passed
all 412 tests with zero failures, skips, cancellations, or TODO cases. The
roadmap-inventory parser also passed all five focused parser tests. The
`dwg-dxf-verify.yml` workflow passed `actionlint 1.7.12`, and `git diff --check`
was clean before this checkpoint edit.

The retained full-suite output is `/private/tmp/offset-W176-node-suite.log`
(695 lines; SHA256
`572458b6a9a7da6af33e4b50527f9703ba654f63693d9e3645c8825d83474cbe`).
This covers developer validation tooling only; it is not product/GUI or
cross-platform runtime acceptance. At capture time, exact-head DWG/DXF workflow
`40998099806` remained active on Linux Release and Windows x64/ARM64, and Pixi
workflow `40998099821` remained active on all five platforms. Preserve the
current head until these runs finish, then record terminal job outcomes before
deciding whether another push is warranted. Visible GUI/external-CAD review,
deployed-package acceptance, and final W0-W2/R closure remain open.

| W176 check | Result |
|---|---|
| Full validation-tool Node regression | PASS; 412/412, zero skipped; Node 26.12.0/macOS arm64 |
| Roadmap inventory parser | PASS; 5/5 |
| DWG/DXF workflow syntax | PASS; actionlint 1.7.12 |
| Exact-head Linux/Windows x64/ARM64 CI | Active; workflow `40998099806` |
| Exact-head Pixi matrix | Active; workflow `40998099821`, five platforms |

## 135. W177 Current-Source ODA Conversion Matrix

Used the current-source W172 modern ASCII DXF matrix from product head
`7bd61bd24ba729d779822928d818c3a93cbc4938`. It contains 28 initial and edited
degree-1/degree-3 SPLINE rows across R14, R2000, R2004, R2007, R2010, R2013 and
R2018. ODA File Converter `27.1.0.0` converted all 28 to ACAD2018 DWG with
recursive processing and audit/repair disabled, using its documented
command-line interface ([ODA File Converter](https://www.opendesign.com/guestfiles/oda_file_converter?language=en)).
The conversion exited 0 and produced 28 files; all 28 signatures/readback
headers identify `AC1032`.

LibreDWG `dwgread 0.14` parsed each converted DWG to JSON. The bounded semantic
check found exactly one SPLINE and one `LC_OFFSET_CURVE` EED marker in each of
the 28 files. ACadSharp commit
`3a52a52047e84670759c1f9ad3916ec3ce85ff56` independently read all 28: each had
one modelspace SPLINE, one native marker and a valid knot count. This checks an
external DXF-to-DWG conversion path and semantic readers; it does not establish
LibreCAD direct-DWG writer correctness, render fidelity, or visual CAD
acceptance. The input selection is modern ASCII DXF only; binary DXF and R12
geometry-only rows remain outside this ODA conversion slice.

| W177 artifact | Result / SHA256 |
|---|---|
| Current-source DXF input manifest, `/private/tmp/offset-W177-oda-output/dxf-input.sha256` | 28 files / `c171014fe11bf0175496b2b23431b6b8b5ff39c82ba618796c85f251435c3ad0` |
| ODA AC1032 output manifest, `/private/tmp/offset-W177-oda-output/oda-dwg.sha256` | 28 files / `8ea1fbd6e40ac6978bece37a05ac59685e1f01193c3d934f131d6f9faa88b250` |
| LibreDWG semantic summary, `/private/tmp/offset-W177-oda-output/libredwg-summary.tsv` | 28/28 AC1032; one SPLINE and marker each / `b42fa9f2036d14f2c5154f69b268bbefda8027cd1fdd7d9b217e019a67997063` |
| ACadSharp semantic summary, `/private/tmp/offset-W177-oda-output/acadsharp-summary.tsv` | 28/28 parsed; valid knots / `0e26d6df42a9a5e5cc744b7e8eabb167a99592d93e3097172560d76e52177933` |
| Converted DWG and per-file JSON/reader logs | `/private/tmp/offset-W177-oda-output/` |

This supplements W171/W172 reader checks but does not close the direct external
CAD display, visible GUI, deployed-package, exact-head CI, or final W0-W2/R
gates.

## 136. W178 Current-Source qmake6 App and Bundle Qualification

Configured a fresh out-of-source qmake6 tree at
`/private/tmp/offset-W178-qmake-current` from product source
`7bd61bd24ba729d779822928d818c3a93cbc4938`. qmake 3.1 with Qt 6.12.0, AppleClang
21, macOS SDK 27.0 and arm64 built the full application, `LibreCADOffsetWorker`,
plugins and tools with top-level `make -j4` (exit 0). A second top-level make
recheck also passed; the retained log reports generated targets up to date.
The worktree stayed clean. Compilation emitted missing-field-initializer
warnings in `lc_dwgadvancedmetadata.h`; links also warned that Homebrew QtSvg
and freetype were built for macOS 27.0 while these targets specify 14.4. No
compile or link error occurred.

Copied the CMake-built worker-client test executable from the CMake tree
configured against this worktree into the new qmake6 app bundle's
`Contents/MacOS` beside the qmake-built helper. Running it from that directory
passed all 17 worker-client test cases (3,775 assertions); this validates sibling
helper discovery and protocol exchange for the fresh qmake bundle. It is a local
bundle smoke, not a signed/deployed installation on another machine or a visible
GUI receipt.

| W178 artifact | Result / SHA256 |
|---|---|
| qmake6 application, `/private/tmp/offset-W178-qmake-current/LibreCAD.app/Contents/MacOS/LibreCAD` | PASS / `7bb1e9f0ae37f037592cf7248b1e07e3adfdcd8c9b5e60b397120a1a0600b7b4` |
| qmake6 worker, `/private/tmp/offset-W178-qmake-current/LibreCAD.app/Contents/MacOS/LibreCADOffsetWorker` | PASS / `b70df2b40abfaae7cba0d3267c86cf3b3c1d3ae96c2778aed636cc1690fd2e6d` |
| Top-level qmake6 recheck, `/private/tmp/offset-W178-qmake-recheck.log` | PASS; 42 lines / `a0382f5d4dc1c251c28c95ef859ea5253d5979611a0599f08a69308e3c1f60c8` |
| Bundled worker-client smoke, `/private/tmp/offset-W178-qmake-client.log` | PASS; 17 cases / 3,775 assertions / `1fb155a3743cc24573bf087e3954b524a9bd25934ff1318b0ae388fcfd04b921` |
| Worker-client executable | `/private/tmp/offset-W178-qmake-current/LibreCAD.app/Contents/MacOS/librecad_offset_worker_client_tests` / `be74f79d52fd2a4efaa65cf04b37cb670ceb0b49ba921b1373cc3925648dab93` |

W178 closes the current-source local qmake6 full-build and bundled-helper smoke
rows only. Windows/Linux CI, installed/deployed package acceptance, visible GUI
and external-CAD visual checks, and final W0-W2/R integration remain open.

| W178 remote snapshot | Result |
|---|---|
| DWG/DXF workflow `40998099806` | Active; Linux bounded tests running; Windows x64/ARM64 building |
| Pixi workflow `40998099821` | PASS; all five platform jobs completed successfully |
| CodeQL for `7bd61bd24` | No run/result yet |

## 137. W179 Current-Source Binary DXF and R12 ODA Conversion

Extended the W177 ODA check to the six W172 rows not covered there: four
binary-DXF codec rows (R14 and R2018, degree 1/3) and two R12
geometry-only POLYLINE downgrade rows. ODA File Converter `27.1.0.0`
converted all six inputs to ACAD2018 DWG; LibreDWG parsed all six with AC1032
headers. The four binary-DXF rows retained one native SPLINE, one
`LC_OFFSET_CURVE` marker, and valid ACadSharp knot layouts. Both R12 rows were
read as one geometry-only LWPOLYLINE without the native marker, as expected;
the ACadSharp harness's no-marker exit code is expected for that representation
and is not a parser failure.

Together W177 and W179 cover all 34 current-source W172 DXF files through ODA:
32 native SPLINE/marker rows and two deliberate R12 geometry-only rows, with no
conversion or DWG-read failures. This is DXF-to-DWG interoperability evidence,
not validation of LibreCAD's direct DWG writer or CAD visual fidelity.

| W179 artifact | Result / SHA256 |
|---|---|
| Input manifest, `/private/tmp/offset-W179-oda-output/dxf-input.sha256` | 6 files / `a3fc007f30839a5479d0d17bc8fe2e28c54c01fd89e28ce7db0ad2bdc0da0495` |
| ODA AC1032 output manifest, `/private/tmp/offset-W179-oda-output/oda-dwg.sha256` | 6 files / `2258b83bdbc48f8eaf1e692876ac3aa05090db6e8e8f7f39bc5faecc16c8d2e9` |
| LibreDWG summary, `/private/tmp/offset-W179-oda-output/reader-summary.tsv` | 6/6 AC1032 parsed; binary marker rows and expected R12 representation / `b1b10f9f76df588dbf5d1e7ba5fc9b1b8afe91735cf7672643c20d75cc7708a3` |
| ACadSharp detail, `/private/tmp/offset-W179-oda-output/acadsharp-detail.tsv` | Binary rows valid; R12 geometry-only / `add5f613af332619763160e1a64f6e9e1ccf8de4a01da2a30632ed769f1f90e7` |
| Converted DWGs and per-file reader logs | `/private/tmp/offset-W179-oda-output/` |

The exact PR-head DWG/DXF workflow `40998099806` is still active on Linux
bounded tests and Windows x64/ARM64 builds; Pixi `40998099821` passed all five
platforms. Keep the product head unchanged until DWG/DXF jobs finish, then
record terminal results before pushing accumulated roadmap-only commits. Visible
GUI/external-CAD review, direct-DWG visual acceptance, deployed-package
qualification and final W0-W2/R closure remain open.

## 138. W180 Exact-Head DWG/DXF CI Phase Transition

Rechecked workflow `40998099806` on product head
`7bd61bd24ba729d779822928d818c3a93cbc4938`. The Linux Release lane completed
configure/build and is running bounded tests (job `123876195069`). Windows x64
and ARM64 both completed configure/build successfully and entered the bounded
test step (jobs `123876195326` and `123876194781`). All three jobs remain active;
the Actions API and job-log endpoint have not supplied terminal output yet, so
no test or worker-install result is claimed for these lanes. Exact-head Pixi
workflow `40998099821` passed all five platform jobs.

No product code changed for this checkpoint. At W180, the intended sequence was
to hold the branch until the three jobs finished. W181 records the subsequent
roadmap-only push to publish these checkpoints; the original product-head run
was canceled during its test step, and the current-PR-head dispatch is now
active. Visible GUI/external-CAD visual acceptance, deployed-package
qualification and final W0-W2/R closure remain open.

## 139. W181 Roadmap Published and Exact-Head Requalification Started

Pushed the seven roadmap-only commits (`654d83fca` through `498a5deee`) to
PR #3193. Verified that the PR remains open/draft at head
`498a5deee690187a50ad776ad2e5df63fc51f1c5`; the delta from the previously
tested product head `7bd61bd24ba729d779822928d818c3a93cbc4938` is only
`docs/plan_offset_curve_2961.md`.

The product-head DWG/DXF run `40998099806` was canceled in its bounded-test
step after all three builds passed. Its retained logs show CTest 23/23,
`[sync-api]` 219 assertions, and `[creation-worker]` 318 assertions on Linux,
Windows x64 and ARM64. The Linux cache-trim failure sweep emitted recovery
markers through the `reference-six-pair-four-critical-graph` fixture (6,501
injection points), but cache-check was terminated; Windows cache/fault sweeps
were also interrupted, and no relocated-helper result exists for this run.

Exact-head dispatch `41003855350` is active on
`498a5deee690187a50ad776ad2e5df63fc51f1c5`; at capture, Linux was building,
Windows x64 was installing dependencies/configuring, and ARM64 was installing
Boost. No bounded tests had begun in this exact-head run. Pixi `41003830885` is
active on all five platforms. Record terminal results before declaring remote
qualification complete.

The roadmap parser remains 5/5. At this checkpoint, the exact-head CI runs have
not completed; record their terminal outcomes before claiming remote
qualification. Visible GUI/external-CAD visual acceptance, deployed-package
qualification, and final W0-W2/R integration remain open.

## 140. W182 Headless Raster Rendering Check

Used the freshly built qmake6 application from W178 in console `dxf2png` mode
with `QT_QPA_PLATFORM=offscreen` and a private HOME. The output route uses the
application's drawing viewport and painter. The bundled `dim_sample.dxf`
rendered at 1200x800 with its boundary/linework, arcs, circles, dimensions,
MTEXT, solids and polylines visible in the raster. The W172 current-source
AC1032 degree-3 spline DXF also rendered successfully. That regression fixture
has an intentionally very narrow ellipse (DXF extents approximately
0.0000625 by 0.0124998), so its raster is a thin vertical curve; this is
consistent with the fixture geometry, not evidence of a rendering failure.

An initial SVG-export look at that narrow fixture was not used as viewport
evidence: the SVG export path only emits a subset of entity types, and its
thin result is expected for this aspect ratio. A separate `dim_sample.dxf`
PNG was visually inspected and confirms a nonblank, legible multi-entity
render. This headless raster check is not a visible GUI interaction receipt,
does not measure display/DPR, and does not replace external-CAD visual review.

| W182 evidence | Result / SHA256 |
|---|---|
| qmake6 `dxf2png` render of bundled dimension sample, `/private/tmp/offset-W182-svg-render/dim_sample.png` | PASS; 1200x800; `c9253a3230c301636278ed7099c203a06251a66e3d6bdd9e8f46c4467782d948` |
| qmake6 `dxf2png` render of W172 AC1032 degree-3 output, `/private/tmp/offset-W182-svg-render/offset-matrix-write-dxf-R2018-d3_actual.png` | PASS; 1200x800; `90a0b3a61c4293ae20168aa6a3f92379e1a7eb4fd622708f2c924641d3595e1c` |
| Roadmap inventory parser | PASS; 5/5 tests |
| Exact-head DWG/DXF workflow `41003855350` | Still building on Linux, Windows x64 and Windows ARM64; no test result yet |
| Exact-head Pixi workflow `41003830885` | Still running on all five platforms |

The current exact-head CI and Pixi runs are not complete. Visible GUI acceptance,
display/DPR capture, external-CAD visual review, deployed-package qualification,
and final W0-W2/R integration remain open.

## 141. W183 Exact-Head Pixi Qualification

Exact-head Pixi workflow `41003830885` completed successfully on
`498a5deee690187a50ad776ad2e5df63fc51f1c5` on all five matrix platforms:
Ubuntu x64, Ubuntu ARM64, Windows, macOS ARM64, and macOS Intel. The separate
DWG/DXF verification workflow `41003855350` on the same head completed its
Linux, Windows x64, and Windows ARM64 builds and entered the bounded test step
on all three platforms. Capture terminal results for CTest, format selectors,
fault sweeps, and relocated-helper checks before calling that workflow passed.

| W183 check | Result |
|---|---|
| Exact-head Pixi workflow `41003830885` | PASS; all five platform jobs |
| Exact-head DWG/DXF workflow `41003855350` | Builds passed on Linux, Windows x64, ARM64; bounded tests active |
| Head tested | `498a5deee690187a50ad776ad2e5df63fc51f1c5` |

This is CI/build qualification only. Visible GUI interaction, display/DPR
capture, external-CAD visual review, deployed-package qualification, and final
W0-W2/R integration remain open.

## 142. W184 Exact-Head Linux DWG/DXF Qualification

Linux job `123894559150` in workflow `41003855350` completed successfully on
head `498a5deee690187a50ad776ad2e5df63fc51f1c5`. The bounded CTest suite passed
23/23. The direct synchronous filter API passed 219 assertions; creation-worker
coverage passed 318 assertions in 11 cases. The cache-check allocation-failure
sweep recovered across all fixtures and 29,117 injection points. The installed
and relocated native worker client passed 3,379 assertions in 17 cases.

| W184 Linux check | Result |
|---|---|
| CTest bounded suite | PASS; 23/23 |
| `[sync-api]` filter | PASS; 219 assertions |
| `[lc-offsetcurve][creation-worker]` | PASS; 318 assertions / 11 cases |
| Cache-check allocation recovery | PASS; all fixtures / 29,117 injection points |
| Relocated worker client | PASS; 3,379 assertions / 17 cases |
| Linux exact-head job | PASS; workflow `41003855350`, job `123894559150` |
| Windows x64 and ARM64 exact-head jobs | Bounded tests still active |

No product source changed in this qualification checkpoint. Windows results are
still required before cross-platform DWG/DXF qualification can be closed;
visible GUI, external-CAD visual, deployed-package, and final W0-W2/R gates
remain open.

## 143. W185 Exact-Head Windows ARM64 Qualification

Windows ARM64 job `123894559201` in workflow `41003855350` completed
successfully on head `498a5deee690187a50ad776ad2e5df63fc51f1c5`. CTest passed
23/23, the synchronous filter API passed 219 assertions, and creation-worker
tests passed 318 assertions in 11 cases. Cache-check allocation recovery
completed for all fixtures across 66,594 injection points. The installed and
relocated worker client passed 3,893 assertions in 17 cases. Windows x64
remains active in the same bounded test step; Linux passed in W184.

| W185 ARM64 check | Result |
|---|---|
| CTest bounded suite | PASS; 23/23 |
| `[sync-api]` filter | PASS; 219 assertions |
| `[lc-offsetcurve][creation-worker]` | PASS; 318 assertions / 11 cases |
| Cache-check allocation recovery | PASS; all fixtures / 66,594 injection points |
| Relocated worker client | PASS; 3,893 assertions / 17 cases |
| Windows ARM64 exact-head job | PASS; workflow `41003855350`, job `123894559201` |
| Windows x64 exact-head job | Bounded tests still active |

No product source changed in this qualification checkpoint. The Windows x64
terminal result is still needed for full DWG/DXF matrix qualification; visible
GUI/external-CAD visual review, deployed-package qualification, and final
W0-W2/R integration remain open.

## 144. W186 Exact-Head Windows x64 Qualification

Windows x64 job `123894558777` in workflow `41003855350` completed
successfully on head `498a5deee690187a50ad776ad2e5df63fc51f1c5`. CTest passed
23/23; the synchronous filter API passed 219 assertions; creation-worker tests
passed 318 assertions in 11 cases. Cache-check allocation recovery completed
for all fixtures across 66,594 injection points, and the installed/relocated
worker client passed 3,974 assertions in 17 cases. Together with W184 and W185,
the exact-head DWG/DXF verification workflow is green on Linux, Windows x64 and
Windows ARM64. W183 records the all-green five-platform Pixi matrix.

| W186 x64 check | Result |
|---|---|
| CTest bounded suite | PASS; 23/23 |
| `[sync-api]` filter | PASS; 219 assertions |
| `[lc-offsetcurve][creation-worker]` | PASS; 318 assertions / 11 cases |
| Cache-check allocation recovery | PASS; all fixtures / 66,594 injection points |
| Relocated worker client | PASS; 3,974 assertions / 17 cases |
| Windows x64 exact-head job | PASS; workflow `41003855350`, job `123894558777` |
| Linux and Windows ARM64 jobs | PASS; W184/W185 |

This closes the current exact-head bounded DWG/DXF/helper CI matrix. No product
source changed in W182-W186. Visible GUI interaction, display/DPR capture,
external-CAD visual review, deployed-package qualification and final W0-W2/R
integration remain acceptance gates; this session has no supported computer-use
control tool, so the user-approved test app could not be operated here.

## 129. W171 Current-Source DWG Matrix and Independent Readers

Regenerated the bounded DWG matrix from the current checkout's Release
`librecad_tests` binary into `/private/tmp/offset-W171-current-dwg`, then
preserved the result under
`/Users/dli/dev/LibreCAD/tmp/offset-2961-native-20261026/W171-current-dwg/`.
The 24 files span AC1015, AC1018, AC1021, AC1024, AC1027, and AC1032, with
initial degree-1/degree-3 and edited second-save rows. The writer selectors
passed with 5,822 and 5,414 assertions respectively.

LibreDWG `dwgread 0.14` parsed all 24 outputs and the JSON semantic validator
found exactly one SPLINE per drawing, the `LC_OFFSET_CURVE` marker, expected
physical version, open/nonperiodic flags, and expected NURBS degree/control/knot
counts (degree 1: 1/2/4; degree 3: 3/4/8). ACadSharp at
`3a52a52047e84670759c1f9ad3916ec3ce85ff56` parsed 20/24 with valid knot counts.
Its four AC1021 cases stop in `DwgReader.readFileHeaderAC21` at the known
negative-ComprLen `NotImplementedException`, before entity decoding. The same
four current-source AC1021 outputs pass the independent LibreDWG semantic
check; this closes the direct AC1021 acceptance row for LibreDWG, not the
ACadSharp-specific reader gap.

| W171 evidence | Result |
|---|---|
| Current-source DWG writer matrix | PASS; 24 files, six versions, initial and edited degree-1/degree-3 cases |
| LibreDWG semantic read | PASS; 24/24; SPLINE marker, versions and NURBS layouts validated |
| ACadSharp semantic read | PASS; 20/24; four AC1021 cases hit its pre-entity header limitation |
| DWG SHA manifest | `dwg-manifest.sha256`; SHA256 `0f33599eb00176fd48336eedcbb1bacbe9f00ac7b4d0e5946485783d8a574273` |
| LibreDWG JSONL summary | SHA256 `5ca6a4a5b7b8f97b38f852c87dade3aa916b57405ff751d3537511b15994fe96` |
| ACadSharp summary | SHA256 `5edd9bf46baa9e759898af0c065e7acf51d48a6a91d10c5f480ac5de959000e1` |
| Writer logs | Initial `3434eb96d64ed56910fec0c6d6685f6a37ae7fb63a17fcbd8566db8ed19c09a2`; edited `40fe5a38427830083efeaba7480dcffad97c7ae1fe00cd28a956e9f426f33e24` |

This replaces the pre-current-source W134 DWG evidence. It is independent
semantic-reader validation, not external CAD rendering or signed-package
acceptance. Exact-head platform CI and full W0-W2/R acceptance remain open.

## 130. W172 Current-Source DXF Matrix and ACadSharp

Regenerated DXF output from the current checkout's Release test binary into
`/private/tmp/offset-W172-current-dxf`, preserving the outputs and reader logs
under `/Users/dli/dev/LibreCAD/tmp/offset-2961-native-20261026/W172-current-dxf/`.
The matrix contains 34 files: 28 modern ASCII DXFs across R14 through R2018
(degree 1/3, initial and edited), four binary DXF codec rows (R14 and R2018,
degree 1/3), and two R12 geometry-only POLYLINE downgrades. The modern writer
and edited writer selectors passed with 8,180 and 6,620 assertions; the R12
selector passed.

ACadSharp at `3a52a52047e84670759c1f9ad3916ec3ce85ff56` parsed all 32 modern
ASCII/binary spline-marker rows, each with the marker preserved and a valid
knot layout. It parsed both R12 rows as one ordinary entity each with no native
SPLINE marker, matching the deliberate geometry-only compatibility
representation. The summary validator reports `PASS=32 R12_geometry_only=2
invalid=0`.

| W172 evidence | Result |
|---|---|
| Modern DXF writer matrix | PASS; 8,180 assertions |
| Edited DXF writer matrix | PASS; 6,620 assertions |
| R12 writer/read matrix | PASS; geometry-only representation |
| ACadSharp semantic read | PASS; 32/32 modern native spline rows; 2/2 R12 geometry-only rows |
| DXF SHA manifest | `dxf-manifest.sha256`; SHA256 `9fffd2ab828892146a4a68919097fdcf0e5be4ccaa18ab6740553acb4f90c40a` |
| ACadSharp summary | SHA256 `c2d19756eb141ab7aa46e2eecfd9eacf7bce429e6a4b5aa7ae68a1fd88dc523a` |
| Writer logs | Modern `09f67eb2644824c9008b023beb9aaff6a1c5c5067d87ee24029a6252d4783f13`; edited `7cb6bd9c6fa770c970f8b0ca9b5b00a6cd165f18b48d60c9a6098cf381c8ae68`; R12 `10f1c8015f6ebb182d003cc5d8b5f1caf436bc1e29bbb356c22a3ac0fe24e992` |

The exact-head workflow set for `c0438532b` is the remote qualification for the
Windows x64/ARM64 and Linux helper/format lanes. Workflow `40979668978` has
completed configuration and builds on all three platforms; each is still in the
combined bounded-test step, with terminal results pending. The source-identity
CTest selector is registered in all three lanes. Pixi `40979669043` passed on
all five OS/architecture jobs. CodeQL
`40973686197` passed on the preceding product-equivalent head `4df7229ca`; no
exact-`c0438532b` CodeQL run was
triggered for the fixture/documentation-only follow-up. Recheck active runs
before closing cross-platform acceptance. This W130 snapshot predates the
W187 computer-use attempt; it is retained as historical status, not the current
GUI acceptance result.

## 145. W187 Current-Source GUI Launch and macOS File-Dialog Blocker

Refreshed the already-approved isolated app at
`/private/tmp/LibreCAD-PR3193-visible.app` with the qmake6 application and
worker built for W178. The executable hashes match the retained W178 build.
W178's product source is `7bd61bd24ba729d779822928d818c3a93cbc4938`; the
product-source diff through the CI-qualified product head
`498a5deee690187a50ad776ad2e5df63fc51f1c5` is empty, and the later branch head
`9f83601dce2e7d576cd4cfd79653170d8e259a1e` changes documentation only. Thus the
binary is product-equivalent to the exact-head-tested implementation, though it
was not rebuilt from the documentation-only tip.

The isolated app launched and showed its main window at 1920x982 pixels. macOS
reported no usable display-pixel-ratio value through this path. Opening the
Qt file dialog caused the app to terminate while the computer-use accessibility
bridge traversed the dialog hierarchy. Two crash reports,
`~/Library/Logs/DiagnosticReports/LibreCAD-2026-10-30-103113.ips` and
`LibreCAD-2026-10-30-103225.ips`, identify
`-[QMacAccessibilityElement accessibilitySelectedChildren]` during
`QG_FileDialog::getOpenFile`. Bypassing that dialog with Finder's explicit
Open With chooser loaded the W171 AC1032/R2018 matrix drawing into this exact
test app; LibreCAD reported the AC1032/R2018 version and the narrow spline was
visible in the canvas. Selection/property inspection did not resolve the entity
in the property sheet, so no edit, undo/redo, or GUI save/reopen result is
claimed. Earlier W163 edit/save interactions remain tied to their older source
commit and are not upgraded by this launch.

| W187 artifact | Result / SHA256 |
|---|---|
| Isolated qmake6 app binary | `/private/tmp/LibreCAD-PR3193-visible.app/Contents/MacOS/LibreCAD`; matches W178 `7bb1e9f0ae37f037592cf7248b1e07e3adfdcd8c9b5e60b397120a1a0600b7b4` |
| Bundled worker | `/private/tmp/LibreCAD-PR3193-visible.app/Contents/MacOS/LibreCADOffsetWorker`; matches W178 `b70df2b40abfaae7cba0d3267c86cf3b3c1d3ae96c2778aed636cc1690fd2e6d` |
| Product source under test | W178 `7bd61bd24ba729d779822928d818c3a93cbc4938`; product files identical through qualified head `498a5deee690187a50ad776ad2e5df63fc51f1c5` |
| Visible app launch | PASS; main window observed at 1920x982; DPR unavailable |
| Direct Qt file dialog | BLOCKED by reproducible macOS Qt accessibility hierarchy crash |
| Finder Open With: AC1032 DWG load/render | PASS; `offset-matrix-write-dwg-R2018-d3_actual.dwg`; SHA256 `aed066088fab728aff6224481accdd9900ce0b08e2e63c96dd729d28a2838842` |
| Current-source selection/property edit and GUI save/reopen | OPEN; no successful selection or edit recorded |
| Screenshot/DPR receipt | OPEN; screenshot observed in-session but not retained as a file; DPR unavailable |
| Roadmap inventory check | PASS; 1/1 targeted parser test; 93 obligations reconcile; 0 pending declarations / mismatches |
| Repo state | No product source changes; documentation receipt only |

W187 does not close visible UI acceptance. Continue the selection/property,
edit, undo/redo, and GUI save/reopen checks through Finder-mediated input or a
supported Qt/macOS accessibility fix. Remaining gates are the full visible
interaction matrix, screenshot/DPR receipt, external-CAD visual inspection,
deployed-package acceptance, broader W0-W2 platform acceptance, and final R
integration.

## 146. W188 Full Roadmap Validation and Visible-App Stall

The current product sources remain unchanged from the exact-head-qualified
implementation; this checkpoint is documentation and validation evidence only.
After the user approved the isolated app, the visible check was resumed. The
app remained in LibreCAD's `Closing Drawing` state after a close request, and
the CUA screenshot endpoint reported that the screenshot was unavailable. The
selection/property pane still did not confirm a selected entity, so no GUI
edit, undo/redo, or save/reopen result is claimed. The isolated test process
was left untouched rather than terminating another LibreCAD instance; the
Activity Monitor search used to inspect it was cleared afterward.

The complete offset roadmap/H6 parser-inventory suite passes on this checkout:
94 tests, zero failures, zero skips. It reconciles 93 unique obligations (71
original, 93 executable) with zero pending declarations and zero production
mismatches. `git diff --check` also passes. This validates the roadmap inventory
and developer-runner refusal/evidence checks; it is not product GUI or geometry
acceptance.

| W188 check | Result |
|---|---|
| Full `test-refusal-inventory.js` suite | PASS; 94/94, zero failures/skips |
| Obligation inventory | PASS; 93 unique, 0 pending, 0 mismatches |
| Isolated app state | BLOCKED; remained at `Closing Drawing`, screenshot endpoint unavailable |
| Selection/edit/undo/redo/save/reopen | OPEN; no successful current-source interaction recorded |
| Product-source changes | None |

The W188 test-app stall was caused by a hidden exit prompt. Canceling the DWG
close preserved the imported source; the empty untitled document was discarded.
W189 below supersedes the W188 GUI blocker with a completed current-source
selection/edit/undo/redo/save/reopen check on a separate copy.

## 147. W189 Current-Source DWG GUI Edit and Save/Reopen

Continued in the approved isolated qmake6 app at
`/private/tmp/LibreCAD-PR3193-visible.app`. The AC1032/R2018 source fixture
`/private/tmp/offset-W171-current-dwg/offset-matrix-write-dwg-R2018-d3_actual.dwg`
was never overwritten; its SHA256 remains
`aed066088fab728aff6224481accdd9900ce0b08e2e63c96dd729d28a2838842`. A
separate working DWG was created with Save As before editing.

The property sheet identified one imported `Offset Curve`. Its distance read
0.2500. Editing it to 0.3750 reported `Offset Curve: 1 changed, 0 unchanged,
0 refused.` Undo restored 0.2500; Redo restored 0.3750. The edited copy was
saved, closed, reopened through the Open dialog, selected again, and its
property sheet still showed 0.3750. The GUI's Save As filter was
`DWG Drawing 2010 (*.dwg)`, so the round-trip copy is AC1024/R2010, not an
AC1032 GUI-save qualification. LibreDWG `dwgread -O JSON` reads the reopened
copy successfully.

The open/save dialogs were operated by screenshot and keyboard without asking
the CUA accessibility bridge to traverse their Qt accessibility tree. This
avoided the earlier dialog crash. Screenshot observations were 1920x982; the
DPR was unavailable and the screenshot was not retained as a standalone file.
The test copy's on-disk SHA256 is
`c423353091139502de186495c932a3a7a9970d2fdf3fbee722c64637921d99af`.

| W189 check | Result |
|---|---|
| AC1032 source load/render | PASS; current-source DWG visibly rendered |
| Entity selection/property inspection | PASS; one `Offset Curve`, initial distance 0.2500 |
| Property edit | PASS; distance 0.2500 to 0.3750, one entity changed |
| Undo / Redo | PASS; 0.2500 restored by Undo, 0.3750 restored by Redo |
| Save/reopen | PASS; saved separate copy and reopened it in LibreCAD |
| Reopened semantic value | PASS; property sheet shows distance 0.3750 |
| Saved DWG version | AC1024/R2010; the GUI's active Save As filter was R2010 |
| Independent DWG read | PASS; LibreDWG `dwgread -O JSON`, exit 0 |
| Original fixture integrity | PASS; original AC1032 source hash unchanged |
| Screenshot/DPR receipt | Partial; screenshot observed in-session at 1920x982, not retained; DPR unavailable |

This closes one visible current-source offset edit and R2010 save/reopen path,
not the full GUI matrix or AC1032 GUI-save path. External-CAD visual review,
deployed-package acceptance, retained screenshot/DPR evidence, broader W0-W2
platform acceptance, and final R integration remain open.

## 148. W190 High-Eccentricity Ellipse Property-Edit Budget

Current-source visible testing uncovered a property-sheet-only rejection that
the geometry and worker suites did not expose. A small edit from distance 1.000
to 1.001 on a valid high-eccentricity ellipse offset was refused because
`LC_OffsetCurveUI::sheetLimits()` divided four already bounded checker budgets
by eight. The preview limits and native worker deadline remain the outer
resource controls; applying this additional UI-only reduction made valid cached
geometry fail checker validation. The fix now gives the worker-backed property
edit the checker's full configured bounded allowance and adds a regression with
major radius 10, ratio 0.5, tolerance 1e-4, and a cache containing more than 32
control points.

Product/test slice `4f32a89c41be3ff55f8cb78c20a60dea29df8be2` is committed. The
focused cache-budget case passes 25 assertions; the full native offset property
suite passes 42 cases / 9,392 assertions. The qmake6 application and bundled
worker rebuilt successfully. On the isolated current-source GUI bundle, the
high-eccentricity ellipse fixture visibly accepted 1.000 to 1.001 and reported
`1 changed, 0 unchanged, 0 refused`; Undo and Redo both completed. A separate
DXF 2018 copy was saved, and its `LC_OFFSET_CURVE` XDATA contains distance
1.001. The source fixture was not overwritten (SHA256
`d58e9d13b280f8f3b2fcb46ef276a0b251955ad8441fde811edcbbdd61d317a9`; saved
copy SHA256
`410356d466e373cb90f28e669778340ac1722395ac9816404b2f1991a5a4af90`). The
saved DXF copy was not reopened in the GUI, so this is not a save/reopen claim.
The temporary DXF Open With association and macOS full keyboard-navigation
setting used during the check were restored afterward.

| W190 check | Result |
|---|---|
| Focused cache-budget regression | PASS; 25 assertions / 1 case |
| Native property-sheet suite | PASS; 9,392 assertions / 42 cases |
| qmake6 incremental app/worker build | PASS |
| Visible property edit | PASS; ellipse distance 1.000 to 1.001; 1 changed, 0 unchanged, 0 refused |
| Undo / Redo | PASS |
| Separate DXF 2018 output | PASS; `LC_OFFSET_CURVE` XDATA contains distance 1.001; copy not reopened |
| Original fixture integrity | PASS; original SHA256 unchanged |
| Product/test commit | `4f32a89c41be3ff55f8cb78c20a60dea29df8be2` |

Continue the open offset-curve acceptance matrix: reopen the W190 DXF copy and
independently inspect it; retain screenshot/DPR receipts; finish visible
creation/repeat, preview, cancellation, refusal, and multi-entity workflows;
qualify installed/deployed bundles and external-CAD output; complete the
remaining platform-specific W0-W2/R gates and final integration. W190 closes
only the valid cached-ellipse property-edit budget defect and its focused
current-source GUI scenario.

## 149. W191 W190 DXF Copy Reopen Confirmation

Continued W190 in the same isolated current-source qmake6 app. Opened the
separately saved DXF 2018 copy
`/private/tmp/offset-W160-G-matrix-outputs/offset-W190-ellipse-edit-copy.dxf`
and visibly rendered both ellipses. Selecting the edited native `Offset Curve`
in the reopened document showed `Distance 1.0010` in the property sheet,
confirming that the edited value survives a LibreCAD DXF save/reopen/import
cycle. Raw DXF inspection finds both the original fixture offset (0.25) and the
edited offset (1.001), consistent with the GUI selection. The original W190
fixture remained untouched. The screenshot was observed at 1920x982; DPR was
unavailable, and no standalone screenshot file was retained. The saved copy's
SHA256 is `410356d466e373cb90f28e669778340ac1722395ac9816404b2f1991a5a4af90`.

| W191 check | Result |
|---|---|
| Saved W190 DXF 2018 copy reopened in current-source app | PASS |
| Imported offset geometry rendered | PASS; both ellipse boundaries visible |
| Imported edited native offset property | PASS; property sheet shows distance 1.0010 |
| Raw DXF XDATA | PASS; source distance 0.25 and edited distance 1.001 are both present |
| Original fixture integrity | PASS; source SHA256 remains `d58e9d13b280f8f3b2fcb46ef276a0b251955ad8441fde811edcbbdd61d317a9` |
| Roadmap/parser validation | PASS; parser 5/5 and refusal-inventory suite 94/94; 93 obligations, 0 pending, 0 mismatches |
| Screenshot/DPR receipt | Partial; 1920x982 screenshot observed in-session, not retained; DPR unavailable |
| Product-source changes | None; documentation-only validation checkpoint |

W191 closes the W190 DXF reopen check, but not independent external-CAD
inspection of this copy. Retained screenshot/DPR receipts, visible
creation/repeat, preview, cancellation, refusal and multi-entity workflows,
installed/deployed-bundle qualification, external-CAD output, the remaining
platform-specific W0-W2 gates, and final R integration remain open.

## 150. W192 Visible Ellipse Creation and Offset Re-Entry

Using the isolated current-source qmake6 application, created two ellipses in
an unnamed disposable drawing through the GUI command field (`ea`), repeating
the ellipse-axis action without restarting the application. Started Modify
Offset through its GUI command route (`o`) twice, selecting one ellipse each
time. The `Editable Offset Curve` option was visibly enabled; fixed distance
was 1 and the source ellipses were retained. The canvas showed both source
ellipses and their two offset boundaries, and the property selector identified
the selected output as `Offset Curve (1)`. Saved only the disposable drawing
to `/private/tmp/offset-W192-create-repeat.dxf` using the DXF 2018 filter. A
group-code scan reports two source `ELLIPSE` records, two exported `SPLINE`
records, and two `LC_OFFSET_CURVE` XDATA markers. The file SHA256 is
`b70383658d096e54df8e038a43632a3fc73d1be1051e6276912ea1aec615dbff`.
The GUI observation was 1920x982; DPR was unavailable and no standalone
screenshot was retained. This is a current-source visual creation/re-entry
receipt, not external-CAD validation or a reopened-copy claim.

| W192 check | Result |
|---|---|
| Repeated ellipse-axis GUI action | PASS; two ellipses visibly created |
| Modify Offset GUI command re-entry | PASS; invoked twice, one source per invocation |
| Editable Offset Curve option | PASS; visibly checked for both invocations |
| Offset geometry and semantic type | PASS; two offset boundaries visible; selected property type `Offset Curve (1)` |
| DXF 2018 output | PASS; 2 `ELLIPSE`, 2 `SPLINE`, 2 `LC_OFFSET_CURVE` markers; SHA256 recorded above |
| Screenshot/DPR receipt | Partial; 1920x982 observed in-session, not retained; DPR unavailable |
| Product-source changes | None; documentation-only validation checkpoint |

W192 closes the focused ellipse create/repeat and second Modify Offset command
entry path. It does not close preview/cancel/refusal or multi-entity GUI
workflows, independent external-CAD inspection, installed/deployed-bundle
qualification, retained screenshot/DPR evidence, platform-specific W0-W2
gates, or final R integration. The W193 queue remains these open acceptance
items; do not infer full plan completion from the ellipse-only workflow.

## 151. W193 Multi-Entity Editable Offset GUI Workflow

Continued in the same isolated qmake6 GUI build with the W192 drawing open.
Started Modify Offset again, selected both source ellipses in one invocation,
kept `Editable Offset Curve` enabled, used a fixed distance of 0.5, and kept
the originals. The canvas visibly showed three boundaries for each ellipse:
the source, the earlier distance-1 offset, and the new distance-0.5 offset.
The property selector identified the newly selected results as
`Offset Curve (2)`, and the command returned to its default action after
completion. Saved this additional case separately as
`/private/tmp/offset-W193-multi-entity.dxf`; W192's artifact was not modified.

The DXF 2018 ENTITIES scan reports two `ELLIPSE` source entities, four
`SPLINE` carriers, and four `LC_OFFSET_CURVE` markers. SHA256 is
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.
Closed and reopened the separately saved W193 file in the same current-source
LibreCAD app. All six contours rendered again, and selecting a reopened
carrier produced the native property type `Offset Curve (1)`. The GUI view was
observed at 1920x982; DPR was unavailable and the screenshot was not retained
as a standalone file. This confirms LibreCAD's own write/reopen path, not an
independent CAD reader.

| W193 check | Result |
|---|---|
| One Modify Offset invocation over two selected ellipses | PASS |
| Two editable offset results visibly rendered; originals retained | PASS; three boundaries per source |
| Semantic property selector | PASS; `Offset Curve (2)` |
| Separate DXF 2018 output | PASS; 2 `ELLIPSE`, 4 `SPLINE`, 4 `LC_OFFSET_CURVE` markers |
| LibreCAD close/reopen and semantic import | PASS; six contours render; selected carrier is `Offset Curve (1)` |
| Artifact hash | PASS; `05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d` |
| Roadmap/parser validation | PASS; parser 5/5, refusal-inventory 94/94; 93 obligations, 0 pending, 0 mismatches |
| Screenshot/DPR receipt | Partial; 1920x982 observed in-session, not retained; DPR unavailable |
| Product-source changes | None; documentation-only validation checkpoint |

W193 closes the focused multi-entity create and LibreCAD reopen workflow only.
Preview/cancel and refusal paths, independent external-CAD inspection,
installed/deployed-bundle qualification, retained screenshot/DPR evidence,
remaining platform-specific W0-W2 gates, PR conflict resolution, and final R
integration remain open. The PR currently reports a conflict against master;
the checks connector returned no statuses for the observed head, so neither
condition is treated as a passing integration gate.

## 152. W194 Offset Preview and Cancellation Attempt

On the reopened W193 drawing in the isolated qmake6 app, started Modify Offset,
selected one existing offset carrier, and pressed Escape at the direction
prompt before choosing a side. LibreCAD returned to its default action with no
selection and no document edit; the saved W193 DXF remained byte-identical.

A second attempt used a held GUI click at a side point to capture the active
red offset preview. The preview was visible while the action still displayed
the direction prompt. The click released and completed the offset before the
subsequent Escape could be delivered; the document became undoable. Undo then
returned the app to its saved state and disabled Undo. The W193 on-disk artifact
remains unchanged at SHA256
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d` with two
`ELLIPSE`, four `SPLINE`, and four owned markers. This records visible preview
and successful post-commit recovery, **not** cancellation of an active preview.
The CUA bridge serialized the cancellation input after the held click released,
so an in-preview Escape remains unqualified.

Separately, the existing ACadSharp inspector built against revision
`3a52a52047e84670759c1f9ad3916ec3ce85ff56` parsed W193 with exit code 0:
six modelspace entities, four SPLINE carriers, and four owned markers. Each
carrier is degree 3 with 88 control points and 92 knots, and ACadSharp reports
valid knot cardinality. The product artifact was generated from the current
qmake6 source containing product fix `4f32a89c41be3ff55f8cb78c20a60dea29df8be2`.
This is independent parsing, not an external application's visual/render
check. Inspector and ACadSharp assembly SHA256 values are respectively
`de1f94078cba4f989e95ad60b2005083b1e116f1499758695cb6a1fafcf8e62e` and
`b10896a9f13149c6ebd2a901dd4574bf62be985707471f8ae5bf5fd97696f324`.

| W194 check | Result |
|---|---|
| Cancel Modify Offset before direction selection | PASS; returned to Default with no selection |
| Fixed-distance preview rendering | PASS; red preview visible during held click at 1920x982 |
| Cancel while preview is active | OPEN; Escape arrived after button release/operation completion |
| Undo after the held-click completion | PASS; Undo became disabled after restoring saved state |
| W193 output integrity | PASS; unchanged SHA256 and entity/marker counts |
| Independent ACadSharp DXF parse | PASS; 6 entities, 4/4 native SPLINE markers, all knot counts valid |
| Roadmap parser/inventory | PASS; parser 5/5; H6 refusal/inventory 94/94, 93 obligations, no pending or mismatches |
| Screenshot/DPR receipt | Partial; preview observed in-session, not retained; DPR unavailable |
| Product-source changes | None; documentation-only validation checkpoint |

W194 does not close preview cancellation or refusal. Continue with a reliable
in-preview cancellation input, explicit invalid/unsupported-input refusal,
external-CAD visual/render inspection, installed/deployed-bundle qualification,
retained screenshot/DPR receipts, remaining platform-specific W0-W2 gates,
PR conflict resolution, and final R integration.

## 153. W195 Latest-Master Integration and Local Qualification

Integrated current upstream `origin/master` at
`d66b7e1379990224a6a897105683463692b05549` into the PR branch. The only content
conflict was `.github/workflows/dwg-dxf-verify.yml`. Resolution keeps the PR
trigger unconditional because this large feature branch exceeds GitHub's
path-filter comparison window, retains upstream renderer paths for push
events, and preserves the offset-worker build, test, and relocation lanes.
`actionlint` passes on the resolved workflow. The merge tree has no remaining
conflicts. A whole-merge `git diff --check` reports trailing whitespace in Lao
translation lines inherited byte-for-byte from upstream; the resolved workflow
itself passes `git diff --check` and those unrelated translations were not
rewritten.

The CMake Release build was regenerated from the merged source and built the
DWG/DXF fast suites, renderer and SVG suites, offset adapter/action/cache suites,
all worker protocol/process/client/fault/timeout targets, and verification
targets successfully. The registered worker CTest set passes 9/9. Direct local
results: DWG fast tests 1033 passed with 37 external-fixture skips; DXF fast
tests 798 passed with 4 skips; painter/render tests 3096 assertions across 51
cases; SVG export tests 306 assertions across 9 cases; adapter sync API 219
assertions; creation/worker action 318 assertions across 11 cases; and worker
client 3807 assertions across 17 cases. The relocated OffsetWorker component
also passed the worker-client test from its installed directory (4165
assertions across 17 cases), confirming helper discovery from the staged
package path on macOS. Cache-trim failure injection passed. The cache-check
failure-injection executable was still running at this checkpoint; do not count
that test as passed until its exit is observed.

| W195 check | Result |
|---|---|
| Upstream integration | PASS; merged `d66b7e1`; only workflow conflict, resolved |
| Workflow syntax and required trigger/job coverage | PASS; `actionlint`; unconditional PR trigger, upstream renderer push paths, offset-worker lanes retained |
| Merged-source CMake build | PASS; all selected DWG/DXF, renderer, offset, worker, and verification targets built |
| Registered offset-worker CTest | PASS; 9/9 |
| DWG/DXF, painter, SVG, adapter, creation/action, worker-client suites | PASS; counts recorded above; documented external DWG fixture skips remain |
| Relocated installed-worker client test | PASS; 4165 assertions / 17 cases |
| Cache trim failure injection | PASS |
| Cache-check failure injection | INTERRUPTED after more than 12 minutes at full CPU to avoid blocking; no new pass claimed; the cache checker and geometry sources are unchanged from the earlier W100 Release/ASan qualification |
| qmake6 integration build | PASS; full current-source `make -j4` completed, including the LibreCAD app and adjacent `LibreCADOffsetWorker` |
| Whole merge whitespace check | WARN; only upstream Lao translation trailing whitespace; resolved workflow is clean |
| Exact-head GitHub matrix | IN PROGRESS on merge head `8b139998c`; Text Foundations passed; Pixi Builds, CodeQL and DWG/DXF verification are running |
| Visible UI, active-preview cancellation, refusal, external visual inspection, screenshot/DPR | OPEN; macOS lock prevents CUA access |
| Final PR integration | OPEN; merge conflict resolved and pushed; PR remains draft and requires approval; exact-head checks and final acceptance remain |

W195 closes the stale-base/workflow-conflict slice and qualifies the installed
worker locally. It does not close the active-preview cancellation, GUI refusal,
external visual review, retained screenshot/DPR, remote platform matrix, or
final PR acceptance gates.

## 154. W196 Merge-Head and PR Status Refresh

The W195 merge commit `8b139998c45344e00ebaf9450d93eaefe4ce87e3` is now pushed
to PR #3193. GitHub reports the PR open, draft, and mergeable with respect to
base `master` at `d66b7e1379990224a6a897105683463692b05549`; merge is still
blocked by the draft/review requirement and unfinished checks. The PR
description now includes a W195 summary with local evidence and explicit limits.

For exact merge head `8b139998c45344e00ebaf9450d93eaefe4ce87e3`, Text
Foundations run `41089738559` passed both headless OFF and ON; Pixi Builds
`41089738650` passed all five platform jobs; and CodeQL run `41089738642`
passed analysis (its security-scan subjob is skipped by workflow policy).
DWG/DXF verification `41089738574` passed the Linux fast lane, including the
relocated-worker smoke; the Windows x64 and ARM64 test lanes remain in
progress. These remote results are pinned to the merge head, not a later
roadmap-only checkpoint. Record terminal outcomes for the Windows jobs before
updating the exact-head matrix. Post-merge, the complete Node validation suite
passes 412/412; the refusal inventory passes 94/94 (93 obligations, zero
pending declarations, zero production mismatches), and its separate parser
tests pass 5/5. `actionlint .github/workflows/dwg-dxf-verify.yml` passes. A whole-repository
`actionlint` invocation remains non-clean because inherited
`.github/workflows/build-all.yml` contains ShellCheck warnings and a custom
`windows-11-vs2026-arm` label unknown to the locally installed actionlint; that
workflow is outside this offset slice and was not changed. The merge-source local results and the
cache-check interruption are recorded in W195. The full merged-source CMake
Release `librecad_tests` run has also completed: 6,380 cases, 10,345,807
reported assertions, zero failures/errors, and 75 skipped cases. Skips remain
unqualified and are not converted into passes; the exact JUnit receipt is
`/private/tmp/offset-W196-full-catch.xml` (SHA256
`049a099663d8ad3d79f07c6a4426a7d75c52fad98fb7a790f0f9db8d2f396bfb`). The
command was `QT_QPA_PLATFORM=offscreen
/private/tmp/offset-2961-r-release/librecad_tests --reporter junit --out
/private/tmp/offset-W196-full-catch.xml`; the merged-source build is at
`/private/tmp/offset-2961-r-release`.

The first full-selector CMake ASan run used Catch2's default randomized order
and did not pass: JUnit recorded three failing cases (five failure records),
one suite-level error and the existing legacy-DXF skip. The failed cases were
`F Trim context maps native owners and generated chords while preview prepares
plain fragments`, `Staged offset import requires explicit partial-read
acceptance`, and `G matrix alternating format chains preserve edits through
every modern version/original degree 1`. No AddressSanitizer diagnostic was
present in the retained log. Each case passed alone, and the three passed
together in one process (12,312 assertions; log SHA256
`438b258d3e8d3da8c7e44bb35833d9d50c2af0e551678fbfa7a8a70976529604`). An
initial four-shard diagnostic omitted a fixed order/seed; Catch2 warned that
the shards could overlap and leave gaps, so those reports are not qualification
evidence. The complete selector was then rerun as four deterministic
`--order lex` shards with `ASAN_OPTIONS=detect_leaks=0`. All 1,027 discovered
declarations / 1,354 JUnit cases passed: 3,999,717 assertions, zero failures
or errors, and one existing legacy-DXF exporter skip. This deterministic full
rerun includes all three formerly failing cases. The first failure remains
recorded as a non-reproduced broad-run anomaly; the skip remains unqualified,
and leak detection was disabled. Exact shard receipts:

| Lexical shard | Cases | Assertions | Failures/errors/skips | JUnit SHA256 |
|---|---:|---:|---|---|
| 0/4 | 275 | 879,709 | 0/0/0 | `a1788ba2901db1f866e2b4c11ef73fafbe0666fcccc3b744a46b392fd1d937bd` |
| 1/4 | 472 | 285,756 | 0/0/0 | `3530bab980b07d7dc8aef310824edd9e25d1d0733890661dd802361c1c290bc4` |
| 2/4 | 279 | 1,923,414 | 0/0/0 | `130259e88bcd7c734d9756434cbeb65be022c2b17bc1270e75ce1017698def31` |
| 3/4 | 328 | 910,838 | 0/0/1 | `350644ed8da58bb7b0e15bc01e887463f28b85e94404e09e1f22750268a28935` |

The original randomized-run JUnit/log receipts are
`/private/tmp/offset-W196-asan-native.xml` (SHA256
`dbf2d4662b4e8265bb5ec81241d045ae49b1dcb9bebfe1aa6cc7c84b85dbe105`) and
`/private/tmp/offset-W196-asan-native.log` (SHA256
`1991a7096a53e38ca623bcdd3eb0047c3f946924df432c3d1b1008a947e7f686`). The
ASan worker CTest also passes 9/9, including protocol, client, timeout and
process-lifecycle cases; all ASan runs used `detect_leaks=0`.

| W196 check | Result |
|---|---|
| Merge conflict and branch head | PASS; merge `8b139998c` pushed; PR reports mergeable against current `master` |
| PR description | PASS; W195 evidence and limitations visible in the updated description |
| Exact-head Text Foundations | PASS; headless OFF and ON both passed |
| Exact-head Pixi Builds | PASS; all five platform jobs passed |
| Exact-head CodeQL | PASS; analysis passed; Security scan skipped by policy |
| Exact-head DWG/DXF verification | PARTIAL; Linux fast lane and relocated helper passed; Windows x64/ARM64 lanes still running |
| Roadmap validation after merge | PASS; Node suite 412/412; refusal inventory 94/94 plus parser 5/5; scoped DWG/DXF actionlint passes. Whole-repository actionlint still reports inherited `build-all.yml` warnings/custom runner label. |
| Full merged-source CMake Release Catch suite | PASS; 6380 cases, 10,345,807 reported assertions, zero failures/errors, 75 skips (not qualified) |
| Current-source ASan worker/caller requalification | PASS with limits; worker CTest 9/9; deterministic four-shard full `[lc-offsetcurve]` selector 1027 declarations / 1354 cases / 3,999,717 assertions, zero failures/errors, one legacy-exporter skip. A prior randomized whole-process run failed at three cases but all passed in deterministic full coverage and focused reruns; leak detection disabled. |
| Required approval / draft state | OPEN; PR is draft and requires an approving review |
| Visible, external-CAD and deployed acceptance | OPEN; native macOS app controls remain unavailable while the Mac is locked |

W196 corrects integration status only and changes no product code. Keep the
roadmap implementation incomplete until the remaining W0-W2 platform/deployment
gates and R rows have evidence. Continue with locally executable, bounded
acceptance work while remote checks run; do not substitute a successful build
for GUI cancellation/refusal or independent CAD visual review.

## 155. W197 Current-Tree Recheck and Rollup Correction

The opening status summary was stale at W189 and incorrectly repeated older
cross-platform completion wording. It now reflects W196: the planned product
implementation is present, while exact-head CI, visible GUI, independent CAD
visual review, deployed-package qualification, reviewer approval, and final
W0-W2/R acceptance remain open. Historical observations remain in their dated
sections and are not promoted to current-head results.

On PR head `71ddb74ef14ab0540ba45c4b11fa1535e9c0ef83`, reran the full Node
validation suite: 412/412 passed in 190.2 seconds, with no failures, skips,
cancellations, or TODO cases. After the rollup edit, the focused roadmap parser
passed 5/5; `actionlint .github/workflows/dwg-dxf-verify.yml` and
`git diff --check` passed. No product source changed.

At the status snapshot, PR #3193 remained open, draft, and mergeable, with an
approving review required. Exact-head Text Foundations passed both headless
modes. Pixi Builds, CodeQL analysis, and DWG/DXF Linux/Windows x64/ARM64 checks
were still in progress; CodeQL Security scan was skipped by workflow policy.
Native macOS controls remained unavailable because CUA reported the host locked.
W197 is a roadmap and validation checkpoint only; it does not close any of those
external acceptance gates.

## 156. W198 Exact-Head qmake6 Rebuild and Worker Requalification

On exact worktree head `2c4f26b378f201135227403cbb0575e510f88550`, rebuilt the
qmake6 Release application and bundled worker with the worktree-local
out-of-source build at `tmp/offset-2961-qmake-native` (`qmake6` / Qt 6.12.0,
macOS arm64, `make -j4`). The build exited successfully. The unmodified build
executable SHA256 values are application
`442b28e47622ca5a14b81731875842440ee2da7132a7c55dca879c8e747fada0` and worker
`7f1d732ebb33cd4bfc24d8d2070b762958cbf5ce43bab4f15bff91e11ae8b760`. The
isolated app bundle was staged under `/private/tmp` and ad-hoc re-signed after
assigning a temporary bundle identifier. Its signed executable hashes differ
from the unmodified build artifacts, as expected after signing.

The freshly built worker was placed beside the existing worker-client test
runner in a temporary app bundle. The real client protocol suite passed 4,285
assertions across 17 cases against that worker. The current merged-source CMake
worker test set also passed 9/9. No tracked product source changed. CUA can see
the rebuilt LibreCAD process, but has not obtained control; macOS approval for
the refreshed executable/signature is still pending. No new GUI pass, image
receipt, or refusal/cancellation result is claimed from this build.

| W198 check | Result |
|---|---|
| Exact-head qmake6 Release application and worker build | PASS; `make -j4`, macOS arm64 |
| qmake application / worker SHA256 | PASS; `442b28e4...fada0` / `7f1d732e...e8b760` before staging/signing |
| Client protocol suite against fresh qmake worker | PASS; 4,285 assertions / 17 cases |
| CMake worker CTest | PASS; 9/9 |
| Node validation suite | PASS; 412/412 on this roadmap-only head |
| Refusal/inventory validation | PASS; 94/94, 93 obligations, 0 pending declarations, 0 production mismatches |
| Roadmap parser tests | PASS; 5/5 |
| Scoped DWG/DXF workflow actionlint and `git diff --check` | PASS |
| Exact-head GitHub checks | IN PROGRESS; Text Foundations passed both modes; Pixi, CodeQL analysis, and DWG/DXF Linux/Windows x64/ARM64 in progress; CodeQL Security scan skipped by policy |
| PR state | OPEN; draft, blocked pending required review and checks |
| Fresh visible UI acceptance | OPEN; waiting for macOS approval of the rebuilt test app; no pass claimed |
| Product-source changes | None; build and acceptance checkpoint |

W198 closes exact-head qmake build and fresh-worker protocol qualification only.
Keep visible active-preview cancellation/refusal, retained screenshot/display-DPR
evidence, external-CAD visual review, deployed-package qualification, remaining
W0-W2 platform acceptance, exact-head remote checks, reviewer approval and final
R integration open. After macOS approval, resume the GUI matrix on the rebuilt
bundle and record outcomes separately; do not infer GUI success from worker
tests or the older in-session app.

## 157. W199 Independent FreeCAD Visual Review of W193 DXF

Opened `/private/tmp/offset-W193-multi-entity.dxf` in FreeCAD 1.1.4 using its
DXF import path and the default individual-Part-shape import mode. FreeCAD
reported approximately 90 geometric entities and rendered both source ellipses
with all four offset contours (six visible loops total). The test DXF remained
byte-identical at SHA256
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`. Screenshot
was observed in-session at 1800x977; DPR was unavailable and no standalone
image receipt was retained. The source document was not saved from FreeCAD.

| W199 check | Result |
|---|---|
| Independent DXF import | PASS; FreeCAD 1.1.4 accepted the W193 DXF |
| Independent visual rendering | PASS; 2 ellipse sources and 4 offset contours visibly rendered |
| Source artifact integrity | PASS; SHA256 unchanged |
| Screenshot/DPR receipt | Partial; 1800x977 observed in-session, not retained; DPR unavailable |
| Semantic XDATA/editability validation | OPEN; FreeCAD was used as a visual CAD check only |
| LibreCAD fresh-build GUI checks | OPEN; macOS Device Control and Data Access approval remains pending |
| Product-source changes | None; external visual acceptance checkpoint |

W199 closes independent visual rendering for this one DXF artifact only. It does
not validate LibreCAD-specific XDATA semantics, native editability in FreeCAD,
external DWG/AC1032 rendering, active-preview cancellation/refusal in LibreCAD,
retained screenshot/DPR evidence, deployed bundles, remaining W0-W2 gates, or
final R integration. Do not treat a successful DXF display as a full feature or
roadmap PASS.

## 158. W200 ODA Conversion of the AC1032 Fixture

Copied the unchanged W171 AC1032 fixture
`/private/tmp/offset-W171-current-dwg/offset-matrix-write-dwg-R2018-d3_actual.dwg`
to an isolated temporary input directory and invoked the installed ODA File
Converter 27.1.0 in command-line mode to write ACAD2018 DXF into a separate
temporary output directory. The source and copied-input SHA256 values both
remain `aed066088fab728aff6224481accdd9900ce0b08e2e63c96dd729d28a2838842`.
ODA exited successfully; the output is DXF 2018 with `$ACADVER` `AC1032`, size
93,294 bytes, SHA256
`b696b00f19d19f70fad13e6472c9d8149e4e33a75c0f6f5e5f8b6476dd53bf46`.

This verifies ODA conversion of this fixture only. FreeCAD initially presented
its first-start configuration view; no profile changes were made. The converted
DXF was then selected through FreeCAD's Open dialog, whose DXF import summary
reported approximately 233 geometric entities. After choosing the default
Individual Part shapes import mode and confirming, FreeCAD exited before a
rendered view could be observed. Its macOS diagnostic report identifies
`EXC_BAD_ACCESS`/`SIGSEGV` in `QMacAccessibilityElement` deallocation while
servicing an accessibility `PerformAction` request on the main thread. This
localizes the observed failure to the accessibility/UI action path; it does not
establish whether DXF import completed or indicate a geometry-parser defect. No
visual result is claimed. The ODA converter's folder UI also reported a
concurrent user change; its current state was left untouched. These temporary
outputs are not product fixtures and are not committed.

| W200 check | Result |
|---|---|
| W171 AC1032 DWG input integrity | PASS; source and temporary copy hashes match |
| ODA File Converter 27.1.0 AC1032 DWG to 2018 DXF | PASS; exit 0, output header `AC1032` |
| Converted output integrity | PASS; SHA256 `b696b00f...d53bf46` |
| Independent visual inspection of converted DXF | OPEN; FreeCAD accessibility action crashed before a rendered view could be verified |
| LibreCAD rebuilt-bundle GUI acceptance | OPEN; test app is not yet listed in Device Control and Data Access |
| Roadmap parser | PASS; 5/5 |
| H6 refusal/inventory suite | PASS; 94/94, 93 obligations, zero pending declarations, zero mismatches |
| Scoped DWG/DXF workflow actionlint | PASS |
| `git diff --check` | PASS |
| Exact PR-head GitHub checks | PARTIAL; W199 Text Foundations ON/OFF passed; W200 Text Foundations passed; W200 remaining checks were still running when W201 advanced the head; CodeQL Security scan skipped by policy |
| PR state | OPEN; draft and review required |
| Product-source changes | None; external conversion/acceptance checkpoint |

W200 does not close external DWG visual review, LibreCAD-specific XDATA
semantics, active-preview cancellation/refusal, retained screenshot/DPR
evidence, deployed-package qualification, remaining W0-W2 platform gates,
reviewer approval, or final R integration. The exact-head workflow snapshot is
for `ad962708066fde6fedfb8b34f60ad506e71e9be1`; W200 is committed as
`003eeb18240f454a52ee78599722eddc8efde2be`, and its exact-head workflows were
still running when W201 began. W201 is committed as
`f54e7f760590f3aa55b5bf930a0bebe1950139c2`; its exact-head workflows are the
current running set recorded in W202.

## 159. W201 FreeCAD Import Attempt and W200 Head Status

Followed the failed W200 external visual path without changing FreeCAD's
first-start preferences. Its Open dialog selected the ODA-generated AC1032 DXF
and reported approximately 233 geometric entities. The default Individual Part
shapes option was selected; after confirming the import, FreeCAD exited before
a rendered view could be observed. This establishes neither a visual pass nor a
product defect. The converted DXF and
source DWG remain disposable temporary artifacts, and the original source hash
is unchanged.

At PR head `003eeb18240f454a52ee78599722eddc8efde2be`, Text Foundations, Pixi
Builds, CodeQL analysis, and DWG/DXF verification were still running. The CodeQL
Security scan was skipped by workflow policy. PR #3193 remains open, draft, and
review-required. macOS Device Control and Data Access still does not list the
rebuilt LibreCAD test bundle; visible current-source checks remain unqualified.

| W201 check | Result |
|---|---|
| FreeCAD DXF entity-count/import dialog | PASS; ODA DXF recognized, approximately 233 geometric entities |
| FreeCAD visual rendering of ODA-derived DXF | OPEN; no rendered view captured |
| FreeCAD profile changes | NONE |
| Rebuilt LibreCAD visible GUI matrix | OPEN; privacy approval not present |
| Exact-head CI for W200 | IN PROGRESS; all required workflows except policy-skipped Security scan |
| Product-source changes | None; external-application acceptance attempt |

## 160. W202 FreeCAD Accessibility Diagnostic

Inspected the FreeCAD diagnostic generated at the end of the W201 import-dialog
attempt. It records `EXC_BAD_ACCESS` / `SIGSEGV` in `objc_release` and
`QMacAccessibilityElement` deallocation, with the main thread servicing an
accessibility `PerformAction` request. This is evidence of an accessibility/UI
path crash during the import confirmation. It is not evidence that FreeCAD
finished importing the DXF, and it does not establish a fault in ODA conversion
or LibreCAD geometry. No screenshot or rendered result exists for this attempt;
keep independent visual validation open.

At PR head `f54e7f760590f3aa55b5bf930a0bebe1950139c2`, Text Foundations,
CodeQL analysis, and DWG/DXF verification were in progress, with the CodeQL
Security scan skipped by policy. Pixi Builds jobs were running or queued. PR
#3193 remains open, draft, and review-required. The rebuilt LibreCAD bundle is
still not listed in Device Control and Data Access.

| W202 check | Result |
|---|---|
| Crash report diagnosis | PASS; main-thread accessibility `PerformAction` path, not attributable to file parsing from available evidence |
| External visual acceptance | OPEN; no viewport rendered or screenshot captured |
| Exact-head CI | IN PROGRESS; head `f54e7f760` |
| Current-source LibreCAD GUI checks | OPEN; explicit macOS privacy approval still missing |
| Product-source changes | None; diagnostic/documentation checkpoint |

## 161. W203 Visible In-Preview Cancellation

After user approval made the isolated LibreCAD test bundle controllable, resumed
the W193 GUI check. Modify Offset was active on the existing W193 drawing. A
fixed-distance red preview was visibly rendered while LibreCAD was still asking
for the offset direction. Escape was delivered before a side-point click; the
application returned to `Current Action: Default`. The accessibility tree
reported `No Selection`, with Undo and Redo disabled, and the viewport again
showed the same two groups of three ellipse contours. No save was issued. This
closes the specific active-preview cancellation gap left open by W194.

The source DXF remains byte-identical to the recorded W193 artifact. The
screenshot was observed during this session but was not retained as a file, and
display/DPR was not measured. This does not verify refusal behavior, other GUI
interaction paths, or external CAD rendering of the W171-derived DWG output.

| W203 check | Result |
|---|---|
| Isolated LibreCAD test-bundle access | PASS; user approved, CUA control succeeded |
| Fixed-distance preview visibility | PASS; visible while direction prompt was active |
| Escape during active preview | PASS; returned to Default before direction selection |
| Post-cancel document state | PASS; no selection, Undo/Redo disabled, six contours still visible |
| W193 on-disk artifact | PASS; SHA256 unchanged at `05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d` |
| Retained screenshot / display-DPR receipt | OPEN; screenshot observed in-session only, DPR unavailable |
| GUI refusal and broader interaction matrix | OPEN |
| FreeCAD visual check of W171-derived DXF | OPEN; no rendered viewport verified after accessibility-path crash |
| Exact PR-head workflows | IN PROGRESS; head `f54e7f760`; Text Foundations ON/OFF passed, other checks running, Security scan skipped by policy |
| PR state | OPEN; draft, merge state blocked, approving review required |
| Product-source changes | None; visible-app validation/documentation checkpoint |

## 166. W208 FreeCAD Draft-Mode Visual Retry

In FreeCAD 1.1.4, reopened the ODA-converted W171 AC1032 DXF
`/private/tmp/offset-W200-oda-output/offset-matrix-write-dwg-R2018-d3_actual.dxf`.
The import dialog reported approximately 233 geometric entities. Selected
`Editable Draft objects` rather than the earlier Individual Part shapes mode;
the document opened with a `Layers` tree group, but the viewport remained blank
except for a vertical line near the canvas center. Neither Fit All nor the
`V`, `F` shortcut produced a usable rendered view. This is not a visual pass and
does not identify whether the issue lies in the converted geometry, importer,
view state, or FreeCAD's uncompleted first-start flow.

A Document Recovery prompt appeared during further navigation. It listed the
separate W193 file as not yet recovered. No recovery or Cleanup action was
started; Escape dismissed the prompt. FreeCAD preferences were not changed and
neither DXF was saved. The W171-derived converted DXF SHA256 remains
`b696b00f19d19f70fad13e6472c9d8149e4e33a75c0f6f5e5f8b6476dd53bf46`.

| W208 check | Result |
|---|---|
| FreeCAD recognition | PASS; import dialog reported approximately 233 entities |
| Draft-object import | PASS; document opened, but no usable contours rendered |
| Fit All / viewport | OPEN; no rendered view; central vertical line only |
| Recovery prompt | Dismissed without recovery or Cleanup |
| Preferences / source artifact | NONE changed; converted DXF hash unchanged; no save |
| Screenshot / display-DPR | OPEN; observed in-session only, DPR unavailable |
| Exact PR-head checks at capture (`4341e27e`) | Headless ON/OFF PASS; analysis, DWG/DXF Linux/Windows x64/ARM64, and Pixi pending; Security scan skipped by policy |
| PR state | OPEN; draft, merge state blocked, approving review required |
| Product-source changes | None; external visual-validation checkpoint |

## 165. W207 Editable Offset Creation and Property Undo/Redo

Used the approved isolated qmake6 bundle
`/private/tmp/LibreCAD-PR3193-visible.app`, built from W198 product head
`2c4f26b378f201135227403cbb0575e510f88550`. Product paths are unchanged through
PR head `b086c780e3aed99fe80eadc949e379d8e740ad9b`. The staged app and worker
SHA256 values are `df35f8adef5c2ef6ec16aeecd8cb1ecaa7401557da012b02ea09eec0a6903966`
and `ecc00959fdce4f105a6f6c909e4ab554f1f966f58579ad889fde9fb058605c5b`.

Opened W193 and used Tools > Modify > Offset. `Editable Offset Curve` was
checked, with fixed distance 0.5. Selecting an existing ellipse-derived offset
contour and choosing its outward side created a new `Offset Curve`; its
property sheet showed distance 1.5. Editing the new entity to 2.0, then 1.75,
was reported as one changed entity each time. Undo restored 2.0 and Redo
restored 1.75. Subsequent Undo operations restored the intermediate value and
removed the test-created offset, returning the canvas to the original six
contours. Closed the disposable drawing and chose No at the save prompt; no
drawing was saved. W193's on-disk SHA256 remains
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

This qualifies one nested editable-offset creation and property-history path.
It is not an independent plain-RS_Ellipse creation test (W192 covers that
separately), multi-selection, layer/attribute combinations, Trim, Update or
Restore workflows. Screenshots were observed in-session only; no standalone
image or display-DPR receipt was retained.

| W207 check | Result |
|---|---|
| qmake6 app/worker identity | PASS; bundle hashes recorded above; product paths match W198 code head through PR head `b086c780e` |
| Editable Offset creation | PASS; `Editable Offset Curve` enabled; new `Offset Curve` entity visible and selectable |
| Property edit | PASS; 1.5 to 2.0, then 1.75; each reported one changed entity |
| Property Undo/Redo | PASS; Undo restored 2.0; Redo restored 1.75 |
| Test cleanup | PASS; subsequent Undo restored prior value and removed test entity; drawing closed with No/save-discard |
| W193 artifact integrity | PASS; SHA256 unchanged |
| Screenshot / display-DPR | OPEN; observed in-session, not retained; DPR unavailable |
| Exact PR-head checks at capture (`b086c780e`) | Headless ON/OFF PASS; analysis, DWG/DXF Linux/Windows x64/ARM64, and Pixi pending; Security scan skipped by policy |
| PR state | OPEN; draft, merge state blocked, approving review required |
| Product-source changes | None; visible-app validation/documentation checkpoint |

## 162. W204 Visible Unsupported-Entity Refusal

Loaded the existing text-only DXF fixture
`librecad/src/lib/filters/tests/testdata/dxf/big5_traditional.dxf` in the
isolated current-source LibreCAD bundle. With its sole `TEXT` entity selected,
started Modify Offset and completed entity selection. The GUI reported `No
valid entities selected, select them first` rather than creating geometry. An
Escape returned the action to `Default`, cleared the selection, and left Undo
and Redo disabled. The fixture was not saved. Its SHA256 remains
`d44c3da8a87f100f63e044b72a28cab077932ae4e5771c5f8617e5cd4b243442`; the W193
source DXF also remains unchanged at
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

This closes one unsupported-entity refusal/recovery path only; it does not
qualify all offset refusal reasons or the full interaction matrix. The GUI
screenshots were observed in-session, not retained as standalone files, and
display/DPR remains unavailable.

| W204 check | Result |
|---|---|
| Text-only fixture and rendered `TEXT` input | PASS; one imported text entity selected |
| Unsupported selection refusal | PASS; explicit “No valid entities selected” report |
| Recovery and document state | PASS; Default action, no selection, Undo/Redo disabled; no save |
| Fixture integrity | PASS; input and W193 hashes unchanged |
| Other GUI refusal reasons / interaction matrix | OPEN |
| Screenshot/DPR receipt | OPEN; screenshot observed in-session only, DPR unavailable |
| Exact PR-head workflows | IN PROGRESS at `7828c9b21`; Text Foundations, CodeQL analysis, DWG/DXF, and Pixi jobs pending; Security scan skipped by policy |
| PR state | OPEN; draft, merge state blocked, approving review required |
| Product-source changes | None; visible-app validation/documentation checkpoint |

## 163. W205 FreeCAD Visual Retry

Relaunched FreeCAD 1.1.4 without completing or changing its first-start
preferences. Opened the ODA-converted W171 AC1032 DXF
`/private/tmp/offset-W200-oda-output/offset-matrix-write-dwg-R2018-d3_actual.dxf`.
Its DXF Import dialog again identified approximately 233 geometric entities
and had Individual Part shapes selected. Confirmed by keyboard Return. FreeCAD
created a document tab, but the viewport showed no usable drawing geometry; a
vertical line remained clipped at the edge, and both the Fit All shortcut and
toolbar command left the viewport blank. The app later returned to its
first-start view. No document was saved and no FreeCAD preferences were
changed. This is not a visual pass and does not identify whether the cause is
the importer, view state, or the uncompleted first-start flow.

| W205 check | Result |
|---|---|
| FreeCAD recognized converted DXF | PASS; import dialog reported approximately 233 geometric entities |
| FreeCAD visual rendering | OPEN; no contours verified; Fit All did not reveal geometry |
| FreeCAD profile/preferences | NONE changed |
| ODA conversion / LibreCAD source artifacts | Not modified or saved during this retry |
| Exact PR-head workflows | IN PROGRESS at `f7a02c22a`; Text Foundations, DWG/DXF, and Pixi jobs pending; CodeQL analysis not surfaced; Security scan skipped by policy |
| Retained screenshot / display-DPR | OPEN; observations were in-session only |
| PR state | OPEN; draft, merge state blocked, approving review required |
| Product-source changes | None; external-visual validation checkpoint |

## 164. W206 Restored LibreCAD Visibility

After the user approved macOS control access for the isolated test bundle,
rechecked its active W193 drawing. The screenshot visibly shows all six ellipse
contours. Accessibility state reports `Current Action: Default`, `No Selection`,
and zero selected entities. No command, save, preference change, or drawing edit
was performed during this recheck. The on-disk W193 DXF SHA256 remains
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

This re-verifies display and idle state only; W203 remains the evidence for
active-preview Escape cancellation and W204 for text-only refusal. The screenshot
was observed in-session but not retained as a standalone artifact, and display
DPR remains unavailable. The FreeCAD W171-derived DXF view is still open and
unverified.

| W206 check | Result |
|---|---|
| LibreCAD app access | PASS; isolated app controllable after user approval |
| W193 drawing visibility | PASS; six ellipse contours visible |
| Idle/editor state | PASS; Default action, no selection, zero selected entities |
| W193 artifact integrity | PASS; SHA256 unchanged |
| New interactive behavior or source changes | NONE; visual recheck only |
| Screenshot/DPR receipt | OPEN; screenshot observed in-session only, DPR unavailable |
| Exact PR-head workflows | NOT REPORTED for documentation head `417d6b4ec653be2cc3089eee996e9bc5708096fe` |
| PR state | OPEN, draft; approving review required |
| Product-source changes | None; visible-app validation/documentation checkpoint |

## 167. W209 Current-Source Multi-Copy Creation and Undo

After the user approved Device Control access for the isolated qmake6 app, used
`/private/tmp/LibreCAD-PR3193-visible.app`, built from W198 product head
`2c4f26b378f201135227403cbb0575e510f88550`. The bundled app and worker hashes
remain `df35f8adef5c2ef6ec16aeecd8cb1ecaa7401557da012b02ea09eec0a6903966` and
`ecc00959fdce4f105a6f6c909e4ab554f1f966f58579ad889fde9fb058605c5b`; the
offset action, options widget and worker source paths are unchanged from that
build through PR head `6e1dc8b3fd69ba40a81789a00959eb5db230fb07`.

Opened W193 with its six original ellipse contours, started Modify Offset, and
kept fixed distance `0.5` and `Editable Offset Curve` enabled. A normal visible
mouse click on `Multiple Copies` enabled the count control; set the count to 2,
selected the outer ellipse in the right-hand group, completed selection and
picked the outward side. Two additional offset contours appeared around that
source and two entities were selected. The command returned to its direction
prompt. Escape ended the command; one Undo removed both created copies and the
viewport returned to the original six contours. Closed the disposable drawing
and chose No at the save prompt. The W193 on-disk SHA256 remains
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

An accessibility-triggered checkbox press changed its reported checked state
without updating the dependent count control; the normal visible mouse click
correctly fired the option handler and enabled the count. This was an input-path
artifact, not a product defect. This pass verifies a two-copy ellipse creation
and single-step undo, but not every repeat count, source type, layer/attribute
combination, or save/reopen path. The screenshot was observed in-session only;
no standalone image or display/DPR receipt was retained.

| W209 check | Result |
|---|---|
| App and worker identity | PASS; hashes match W207; relevant product source is unchanged through `6e1dc8b3` |
| Multiple-copy option and count | PASS; visible mouse interaction enabled the control; count set to 2 |
| Creation/repeat | PASS; one ellipse source produced two visible offset contours; two created entities selected |
| Undo and cleanup | PASS; one Undo removed both outputs; drawing closed with No/save-discard |
| W193 artifact integrity | PASS; SHA256 unchanged |
| Full validation script suite | PASS; 412 / 412, zero failures, cancellations, skips or TODO cases; 232.145 s |
| Screenshot / display-DPR | OPEN; observed in-session only, no retained screenshot or DPR reading |
| Current PR-head checks | Headless ON/OFF PASS; Ubuntu x64/ARM and macOS latest have passing runs. Analyze, DWG/DXF fast, Windows x64/ARM, Windows Pixi and macOS Intel remain pending; Security scan skipped by workflow policy |
| PR state | OPEN; draft, review required, merge blocked |
| Product-source changes | None; visible-app validation/roadmap receipt only |

W209 closes one visible creation/repeat/undo scenario only. Screenshot/DPR,
other GUI interaction and refusal cases, external-CAD rendering, deployed
package, remaining cross-platform checks and final R integration remain open.

## 168. W210 Current-Source CMake Creation and Portable-Cache Requalification

Rebuilt from this worktree at code head `6e1dc8b3fd69ba40a81789a00959eb5db230fb07`
(the later `f3c6e6240` commit changes only this roadmap) using the existing
CMake Release tree `/private/tmp/offset-W166-full-build`. Its
`CMAKE_HOME_DIRECTORY` points at this exact worktree. The `librecad_tests`
target rebuilt successfully through all 497 Ninja steps; the separate
`librecad_offset_worker_process_tests` target built successfully in five steps.
Only pre-existing deprecation/nodiscard warnings were emitted.

Ran the focused action creation-worker selector, including preview reuse,
two-copy native repeat and fallback cases; the portable-cache protocol selector;
and the standalone real helper-process selector. The latter exercises actual
helper launches including operation 6. A first tag probe against the general
`librecad_tests` executable found no process cases because CMake registers those
tests in a dedicated executable; running the selector against the correct
target passed.

| W210 check | Result |
|---|---|
| Exact-worktree CMake Release rebuild | PASS; `librecad_tests` 497/497 build steps; helper-process test target built 5/5 |
| Creation-worker/action repeat | PASS; 11 cases, 318 assertions |
| Portable preview request encoding | PASS; 4 cases, 67 assertions |
| Built helper-process cache/repeat | PASS; 4 cases, 2,326 assertions |
| Updated roadmap parser/refusal suite | PASS; 99/99 with W209 and W210 roadmap entries present |
| Current PR head `f3c6e6240` | Headless ON/OFF PASS; Analyze, DWG/DXF, Windows x64/ARM and Pixi jobs still pending; Security scan skipped by workflow policy |
| PR state | OPEN; draft, review required |
| Product-source changes | None; CMake/test requalification and roadmap receipt |

W210 closes the local macOS CMake Release and real-helper requalification for
preview-cache portability and creation/repeat. It does not close Windows or
other platform runtime/package acceptance, remaining visible GUI interactions,
retained screenshot/DPR evidence, external-CAD visual inspection, or final R.

## 169. W211 FreeCAD Visual Review of Current-Source AC1032 DXF

Opened `/private/tmp/offset-W160-G-matrix-outputs/offset-W163-ui-check.dxf` in
FreeCAD 1.1.4. Its SHA256 is
`d58e9d13b280f8f3b2fcb46ef276a0b251955ad8441fde811edcbbdd61d317a9`. The file
is an AC1032/R2018 DXF generated by the current-source LibreCAD export path and
contains two `LC_OFFSET_CURVE` markers. FreeCAD's DXF import summary recognized
approximately 89 geometric entities. With `Individual Part shapes` selected,
the viewport visibly rendered both nested ellipse contours. The file was not
saved from FreeCAD, no FreeCAD preferences were changed, and no input fixture
was modified.

In the approved isolated LibreCAD app, reopened W193 and confirmed its six
ellipse contours remained visible. A temporary ellipse selection was cleared;
the app returned to Default with zero selected entities. Right-click on the
canvas, both before and after selection, and Control-click did not open a
context menu. No drawing edit was made and the W193 file hash remains
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

| W211 check | Result |
|---|---|
| Independent FreeCAD DXF import | PASS; FreeCAD 1.1.4 recognized approximately 89 geometric entities |
| Independent visual rendering | PASS; two nested ellipse contours visible in Individual Part shapes mode |
| AC1032 export identity | PASS; `$ACADVER=AC1032`, two `LC_OFFSET_CURVE` markers, W163 SHA256 recorded above |
| LibreCAD isolated app idle state | PASS; W193 visible, Default action, zero selected entities after cleanup |
| Context menu | OPEN; right-click and Control-click produced no menu |
| Source/fixture integrity | PASS; W193 hash unchanged; no source drawing saved or modified |
| Screenshot / display-DPR | OPEN; screenshot observed in-session only, not retained; DPR unavailable |
| FreeCAD profile | NONE changed |
| Roadmap parser/refusal suite | PASS; 99/99 with W211 present |
| Current PR head `0e6b8b1` | Headless OFF PASS; Headless ON, Analyze, DWG/DXF, Windows x64/ARM and Pixi jobs pending; Security scan skipped by workflow policy |
| PR state | OPEN; draft, review required, merge blocked |
| Product-source changes | None; external-CAD visual/UI validation and roadmap receipt only |

W211 closes one independent visual rendering check for the W163 AC1032 DXF.
It does not qualify DWG rendering, the W171-derived DXF, other entity/source
types, LibreCAD context menus, retained screenshot/DPR evidence, deployed
packages, remaining platform acceptance, or final R integration.

## 170. W212 Keyboard Context Menu and Screenshot Capture Retry

On the approved isolated LibreCAD app, selected an ellipse in W193 and pressed
Shift+F10. No context menu appeared; focus moved to the command input. Escape
restored the Default action with zero selected entities. Earlier right-click
and Control-click attempts likewise produced no menu. The source file remains
unchanged at SHA256
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

Tried macOS screenshot shortcuts and Preview's File > Take Screenshot > From
Window path to retain the W163 FreeCAD view. No screenshot file was produced.
The capture attempt surfaced FreeCAD's existing Document Recovery dialog for
W193; Cancel was selected without starting recovery or cleanup, after which the
computer-use bridge reported that FreeCAD had quit. No document or preference
was saved or changed. The earlier W211 visual observation remains valid, but
its screenshot and display-DPR evidence remain partial.

| W212 check | Result |
|---|---|
| Selected-entity Shift+F10 context menu | OPEN; no menu; focus moved to command input |
| Cleanup | PASS; Escape returned LibreCAD to Default with zero selected entities |
| W193 integrity | PASS; SHA256 unchanged; no drawing saved or edited |
| Retained screenshot | OPEN; macOS/Preview capture attempts produced no file |
| FreeCAD recovery data | UNTOUCHED; no recovery or cleanup started; app quit after Cancel |
| Screenshot / display-DPR | OPEN; no standalone image retained and DPR unavailable |
| Roadmap parser/refusal suite | PASS; 99/99 with W212 present |
| Current PR head `d3882fe` | Headless ON/OFF PASS; Analyze, DWG/DXF, Windows x64/ARM and Pixi pending; Security scan skipped by workflow policy |
| PR state | OPEN; draft, review required, merge blocked |
| Product-source changes | None; visible validation and roadmap receipt only |

W212 does not close context-menu, screenshot/DPR, broader GUI refusal and
interaction, external-CAD, deployed-package, W0-W2 platform or final R acceptance.

## 171. W213 Current-Source Validation Receipt

No product-source files changed after W212. The current PR head is
`440c2525058c26b4af4b4e0aece5b0d219ccd970`; the existing CMake Release tree
`/private/tmp/offset-W166-full-build` points to this exact worktree, and the
source diff from the last product-code head `6e1dc8b3` is empty. Rebuilt all
the worker executable plus all eight worker-test targets (41 Ninja steps), then
ran the complete worker CTest set successfully. A fresh temporary install was
qualified using the macOS bundle layout, with the worker client beside the
installed helper.

| W213 check | Result |
|---|---|
| Full offset developer-validation Node suite | PASS; 412/412, zero failures, skips, or todos |
| Roadmap parser and H6 refusal/inventory | PASS; 99/99; 93 obligations, zero pending declarations and zero production mismatches |
| qmake6 application/worker build | PASS; W198 build at `2c4f26b` is source-equivalent to product head `6e1dc8b`; no non-roadmap diff |
| Exact-worktree worker-target build | PASS; 41/41 build steps |
| Native worker CTest | PASS; 9/9 |
| Relocated macOS worker-client suite | PASS; 3,715 assertions in 17 cases |
| Diff / source scope | PASS; `git diff --check`; W213 is the only worktree change, no product-source changes |
| Windows ARM64 exact-head job | PASS; job `124319327955`; CTest 23/23, 66,594 allocation-failure injection points recovered, relocated client 3,902 assertions in 17 cases |
| Windows x64 exact-head job | PASS; job `124319328539`; CTest 23/23, 66,594 allocation-failure injection points recovered, relocated client 4,140 assertions in 17 cases; job duration 5h38m55s |
| Other exact-head GitHub checks | PASS; Headless ON/OFF, C++ analysis, Linux DWG/DXF, CodeQL analysis, and Ubuntu/macOS/Windows Pixi builds; CodeQL Security scan skipped by workflow policy |
| PR state | OPEN; draft, mergeable, review required |
| Current-source visible GUI recheck | OPEN; CUA still reports macOS locked despite app Device Control approval |

W213 closes the current local developer-validation and worker portability
receipts and records full Windows x64 and ARM64 passes. The x64 DWG/DXF lane
took 5h38m55s, including its exhaustive allocation-failure work; this is a
successful result but a material CI latency to monitor. W213 does not close Mac
GUI refusal/interaction coverage, context-menu behavior, retained
screenshot/display-DPR, independent DWG visual review, deployed-package
qualification, remaining W0-W2 platform acceptance, reviewer approval, or final
R integration.

## 172. W214 Approved-App GUI Recheck

After Device Control approval, CUA controlled the running test application
`org.librecad.librecad.pr3193current`, showing
`/private/tmp/offset-W193-multi-entity.dxf`. The canvas visibly rendered two
groups of three nested ellipse contours. The screenshot was 1920x982 pixels;
this is a capture size, not a measurement of Qt or display DPR.

| W214 check | Result |
|---|---|
| W193 ellipse rendering | PASS; six contours visible across two groups |
| Default/idle baseline | PASS; Selection Pointer, `Current Action: Default`, no selection |
| Right-click on visible ellipse | OPEN/NOT REPRODUCED; no context menu appeared |
| Normal pointer pick at two visible contour locations | OPEN/NOT REPRODUCED; selection count stayed zero |
| Explicit Select Entities mode and pick | PASS; one native Offset Curve selected; highlight and grips visible |
| Right-click on selected Offset Curve | OPEN/NOT REPRODUCED; no context menu appeared |
| Control-click on selected Offset Curve | OPEN/NOT REPRODUCED; no context menu appeared |
| Recovery / document state | PASS; exited Select Entities and Escape cleared selection; Default action, zero selected entities, Undo/Redo disabled; no edit or save |
| Screenshot / DPR retention | OPEN; image observed in-session but not retained; DPR not exposed/measured |
| Product-source changes | None; GUI validation and roadmap receipt only |

W214 closes only the approved-app access, this scoped render/idle-state check,
and explicit-mode entity selection. The normal-pointer pick and context-menu
probes remain unverified GUI behavior, not product failures or passes. Remaining
GUI refusal/interaction coverage, external-CAD rendering of the W171-derived
files, durable screenshot/DPR, deployed-package qualification, W0-W2 platform
acceptance, reviewer approval, and final R integration remain open.

## 173. W215 Full CMake Release and PR-Head Requalification

The worktree has no product-source changes relative to tested product head
`6e1dc8b3fd69ba40a81789a00959eb5db230fb07`; the only path changed since that
pin is `docs/plan_offset_curve_2961.md`. Using the exact-worktree CMake Release
tree `/private/tmp/offset-W166-full-build`, rebuilt the complete application
target (256 build steps) and all executable targets referenced by CTest (159
target build steps). CTest discovery resolved every one of the 26 registered
entries to an executable command.

| W215 check | Result |
|---|---|
| Full CMake Release application build | PASS; 256 build steps |
| Registered CTest executable targets | PASS; 159 build steps; all 26 commands resolve |
| Full CTest | PASS; 26/26; 336.45 seconds |
| Main `librecad_tests` suite | PASS; 5,050 passed, 26 explicitly skipped, 10,344,419 assertions passed; 268.00 seconds |
| Other CTest entries | PASS; worker wire/request/result/kernel/process/client/fault/timeout, focused text/filter/identity/render/console tests, DWG/DXF fast lanes, and required-tag gate |
| Exact PR head `0dfa6d685eabfcb115efa448edeab9dac4bc6d40` | Headless ON/OFF PASS; CodeQL analysis, DWG/DXF Linux/Windows x64/ARM64, and five-platform Pixi were IN PROGRESS at capture; Security scan skipped by policy |
| PR state at capture | OPEN; draft, mergeable, review required |
| External FreeCAD visual review | OPEN; Mac relocked before the W171-derived file could be opened |
| Product-source changes | None; full-build/test requalification and roadmap receipt only |

W215 supersedes W213's partial local CMake inventory: the full application and
all registered CTest programs were built and executed. It does not close
current cross-platform PR checks, external visual inspection of the W171-derived
DXF/DWG, retained screenshot/DPR evidence, deployed-package qualification,
remaining visible GUI refusal/interaction cases, reviewer approval, or final R
integration.

## 174. W216 Current-Master Integration and Painter Requalification

Fetched `origin/master` at
`5a5fe7345fe7a7ec66892e879967673b6d7a859b`. It contained one commit beyond the
previous PR base, #3212's contiguous-polyline dash-phase fix and painter tests.
The branch had no conflicting modifications in either touched path. Merged
master with commit `c4eb33fd2038e81c55ddd79f437d29dc85f96d47`; no conflict
resolution was needed. This advances the PR branch to the current upstream tip
without altering the offset-curve implementation.

| W216 check | Result |
|---|---|
| Current upstream integration | PASS; origin/master `5a5fe7345fe7a7ec66892e879967673b6d7a859b` merged cleanly |
| Affected build | PASS; `librecad_painter_tests` rebuilt, including `rs_painter.cpp` and `lc_linetype_render_tests.cpp` |
| Focused render CTest | PASS; `librecad_painter_tests`, 1/1 |
| qmake6 application/worker build | PASS; full retained qmake6 Release tree rebuilt `rs_painter.cpp` and linked app/helper successfully; no tracked build artifacts |
| Roadmap/refusal gate | PASS; 99/99; 93 obligations, zero pending declarations and zero production mismatches |
| PR checks before merge | Headless ON/OFF PASS; CodeQL, DWG/DXF, Windows x64/ARM64, and Pixi IN_PROGRESS; security scan skipped by policy |
| PR state before push | OPEN; draft; review required; mergeable |
| Pushed PR head | PASS; `2fe0a651b4eeff856a63f552629fb670abfee48f` on current base `5a5fe7345fe7a7ec66892e879967673b6d7a859b` |
| Fresh checks after push | Headless ON/OFF IN_PROGRESS; remaining workflows not yet attached at capture |
| Approved-app visible check | OPEN; Device Control approval confirmed, but CUA reports macOS locked |
| Product source scope | Upstream painter fix and tests only; offset-curve product code unchanged |

The merge and focused painter test close the current-master integration check,
and the updated branch is pushed. Fresh CI has started on the new head; its
eventual completion still needs observation. This does not close the W171-derived
external visual check, retained screenshot/display-DPR,
remaining GUI interaction/refusal cases, deployed-package and W0-W2 acceptance,
reviewer approval, or final R integration.

## 175. W217 Isolated Application-Data Test Directory

The exact-worktree CMake Release build `/private/tmp/offset-W166-full-build`
was rebuilt after W216 master integration. The first full CTest run exposed
two failures in the existing command-alias test: `gl` resolved to
`ActionModifyBreakDivide` instead of `ActionModifyLineGap`. Investigation
showed that `RS_Settings::OtherSettingsDir` fell back to the developer's real
LibreCAD test-profile directory, where a pre-existing `librecad.alias` maps
`gl` to `breakdivide`. No user profile or alias file was edited or removed.

Updated the shared test settings isolation to set `Paths/OtherSettingsDir`
inside the same private temporary root as QSettings. Added a command-test
assertion that confirms the application-data path is the isolated directory.
The focused `[commands]` suite then passed (11 cases, 70 assertions); its
diagnostics confirmed that alias lookup used the temporary directory. Rebuilt
the complete configured tree and reran all registered CTest entries.

| W217 check | Result |
|---|---|
| Full CMake Release incremental build | PASS; exact current worktree, no work remaining on final build check |
| Initial full CTest | Diagnosed environment contamination; command-alias assertions read a pre-existing user alias from the real app-data directory |
| Isolated command suite | PASS; 11 cases, 70 assertions |
| Full CTest after fix | PASS; 26/26, 331.40 seconds |
| Main `librecad_tests` suite | PASS; 266.96 seconds |
| User profile / alias data | UNTOUCHED; test harness now routes `OtherSettingsDir` into its temporary root |
| Product source scope | None; test isolation, one regression assertion, and this receipt |
| PR state | OPEN; draft and review required; new CI for the pending test-harness commit not yet started |

W217 closes the current local CMake test-isolation and complete CTest
qualification. It does not close visible GUI refusal/interaction, retained
screenshot/display-DPR, independent external-CAD review of the W171-derived
files, deployed-package or remaining W0-W2 acceptance, reviewer approval, or
final R integration.

## 176. W218 Exact-Source PR CI Qualification

Rechecked PR #3193 after the W217 test-isolation commit was pushed. Its exact
source head is `71ecf102b36017d57bd30db39d0de630ac11bb75` on base
`5a5fe7345fe7a7ec66892e879967673b6d7a859b`. All required attached source-head
checks completed successfully; the security-scan job was skipped by workflow
policy. The duplicate push-triggered Pixi workflow also passed on the same
source head.

| W218 check | Result |
|---|---|
| Headless ON/OFF | PASS; Text Foundations run `41182640361` |
| CodeQL analysis | PASS; run `41182640333`; security scan skipped by workflow policy |
| DWG/DXF Linux | PASS; run `41182640320`; bounded suite 23/23, offset selector 219 assertions, creation-worker selector 318 assertions, relocated worker-client 3,389 assertions across 17 cases |
| DWG/DXF Windows ARM64 | PASS; run `41182640320`; bounded suite 23/23, offset selector 219 assertions, creation-worker selector 318 assertions, relocated worker-client 3,969 assertions across 17 cases |
| DWG/DXF Windows x64 | PASS; run `41182640320`; bounded suite 23/23, offset selector 219 assertions, creation-worker selector 318 assertions, relocated worker-client 3,970 assertions across 17 cases |
| Pixi Builds | PASS; PR run `41182640166` and duplicate push run `41182637965` passed on Ubuntu x64, Ubuntu ARM64, Windows, macOS ARM64, and macOS Intel |
| PR state at source-head CI capture | OPEN; draft, mergeable, review required |
| Current-source visible GUI | OPEN; Device Control approval is recorded, but CUA still reports macOS locked |
| Product source scope | None; this is CI evidence and roadmap reconciliation only |
| Follow-up documentation head | `628477a13e44b5ebea051d9fd182397eb3ea3c5f`; no product-source changes; Pixi run `41206813474` queued |

W218 closes the exact-source remote CI qualification for the W217 source head.
It does not close the W171-derived independent CAD visual review, durable
screenshot/display-DPR evidence, remaining GUI refusal and interaction cases,
signed/deployed package qualification, outstanding W0-W2 external acceptance,
reviewer approval, or final R integration. Keep the PR draft until those gates
are explicitly resolved; do not treat CI success as visual or release
acceptance.

## 177. W219 Approved-App Multi-Copy Undo/Redo Recheck

After the user approved control access, used the isolated LibreCAD test app at
`/tmp/LibreCAD-PR3193-current.app` (bundle ID
`org.librecad.librecad.pr3193current`, version `2.2.2`, executable SHA256
`1267fb562519199156079642b8356ba202a143f5b875a57b59e647337dd81071`). Opened
the disposable W193 fixture, whose original SHA256 is
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

The drawing rendered six ellipse contours in two groups. Started Tools > Modify
> Offset with fixed distance `0.5`, two copies, editable ellipse offsets,
Keep Originals, and current attributes/layer enabled. Selected the outer
ellipse in the right group, completed selection, and chose the inward side.
Two additional contours appeared and the action remained available at its
direction prompt. Escape returned to `Default`; one Undo removed both copies,
Redo restored both, and a second Undo returned to the six original contours.
The final idle state had no selection and Undo disabled. Closed the drawing and
chose No at the save prompt. The fixture hash still matches its original value.

The in-session screenshot confirms the visible geometry and history transitions,
but no standalone screenshot or display-DPR evidence was retained. The app
bundle identity and executable hash were captured; its exact build commit and
worker hash were not independently established in this pass, so this is not an
exact-binary qualification. FreeCAD was left at its first-start screen without
changing preferences; independent external-CAD rendering remains open.

| W219 check | Result |
|---|---|
| LibreCAD drawing render | PASS; six original ellipse contours visible |
| Offset options and source selection | PASS; fixed `0.5`, two copies, editable mode; one outer ellipse selected |
| Multi-copy creation and repeat-ready state | PASS; two additional contours visible; action returned to direction prompt |
| Escape, Undo, Redo, cleanup | PASS; Escape returned to Default; Undo removed both, Redo restored both, final Undo restored original geometry |
| Disposable fixture integrity | PASS; SHA256 unchanged at `05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d` |
| App binary provenance | PARTIAL; isolated bundle ID/version and executable hash recorded, exact build commit/worker hash unverified |
| Screenshot / display-DPR | OPEN; observed in-session only, no retained screenshot; DPR unavailable |
| Independent FreeCAD rendering | OPEN; first-start screen left untouched; no geometry view verified |
| PR #3193 head `4700af06a` checks at capture | Headless ON/OFF and Ubuntu Pixi jobs PASS; remaining Pixi, CodeQL analysis, DWG/DXF fast and Windows jobs IN PROGRESS; Security scan skipped by policy |
| PR state | OPEN; draft, review required |
| Product-source changes | None; visible-app validation and roadmap receipt only |

W219 adds one approved-app multi-copy creation/undo/redo observation. It does
not close exact-binary GUI qualification, independent external-CAD rendering,
retained screenshot/DPR evidence, remaining GUI refusal/interaction cases,
deployed-package and W0-W2 acceptance, reviewer approval, or final R
integration.

## 178. W220 Approved qmake6 Bundle Multi-Copy and History Recheck

Repeated the W193 visible workflow in the specifically approved isolated app
`/private/tmp/LibreCAD-PR3193-visible.app` (LibreCAD 2.2.2.6-alpha). The staged
application and `LibreCADOffsetWorker` hashes match W207 exactly:
`df35f8adef5c2ef6ec16aeecd8cb1ecaa7401557da012b02ea09eec0a6903966` and
`ecc00959fdce4f105a6f6c909e4ab554f1f966f58579ad889fde9fb058605c5b`. This
qmake6 bundle was built from W198 product head
`2c4f26b378f201135227403cbb0575e510f88550`. The offset action, options widget,
and worker source paths are unchanged through this checkpoint; the bundle is
not claimed as an exact full-source build because later master integration
changed painter code.

Opened the disposable W193 fixture
`/private/tmp/offset-W193-multi-entity.dxf`, whose SHA256 was
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`. The
initial six ellipse contours were visible. Tools > Modify > Offset showed fixed
distance `0.5`, two copies, editable offset enabled, and Keep Originals/current
attributes/current layer enabled. Selected the outer ellipse in the right
group and clicked the exterior side; two additional contours appeared (five
in that group). Escape returned the action to Default. Undo removed both
copies, Redo restored them, and a final Undo restored the original six
contours. Closed the drawing with No at the save prompt. The fixture hash after
closing remains unchanged.

| W220 check | Result |
|---|---|
| Approved app identity | PASS; exact approved path and W207 app/worker hashes match |
| W193 render and offset options | PASS; six source ellipses visible; fixed `0.5`, two copies, editable mode and preserve-original options verified |
| Multi-copy creation and repeat-ready exit | PASS; selected one outer ellipse, created two additional contours, Escape returned to Default |
| Undo/Redo and document recovery | PASS; Undo removed both; Redo restored both; final Undo restored original contours; drawing discarded without saving |
| Fixture integrity | PASS; before/after SHA256 `05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d` |
| Build provenance | PASS for app/worker binary hashes and offset-related source paths; PARTIAL for whole application source because later painter changes are absent from the W198-built bundle |
| Screenshot / display-DPR | OPEN; visible state observed in-session, no durable screenshot retained and DPR unavailable |
| Independent FreeCAD rendering | OPEN; first-start profile remains untouched; no external geometry viewport verified |
| PR state/checks | To be refreshed after this roadmap commit |
| Product-source changes | None; approved-app validation and roadmap receipt only |

W220 supersedes W219's app-identity caveat for this bounded workflow: the
specifically approved qmake6 bundle and worker are now identified by their
retained hashes, and relevant offset code paths match the build source. It does
not make the bundle a whole-tree current-source build or close the external
CAD visual review, durable screenshot/DPR capture, other GUI refusal and
interaction cases, deployed-package and W0-W2 acceptance, reviewer approval,
or final R integration.

## 179. W221 Approved-App Pointer Selection and Context-Menu Probe

After the user approved Device Control access, reconnected to the same isolated
qmake6 bundle at `/private/tmp/LibreCAD-PR3193-visible.app`. The app/worker
hashes remain those recorded in W220. With the disposable W193 drawing still
open, selected the right-group outer ellipse with a normal primary
pointer click. The property selector changed to `Offset Curve (1)` and the
curve was highlighted with grips, confirming this selection path in the
approved bundle.

Secondary-clicks on the selected offset curve and on blank canvas produced no
visible context menu. A secondary-click attempt in the Layers panel returned an
accessibility capture error and showed no popup in the subsequent screenshot;
this does not establish whether that panel event was delivered. The source
event path has a context-menu invocation for non-default mouse releases, but
these observations are not sufficient to attribute the absent popup to product
behavior rather than the UI-driving path. Keep context-menu acceptance open.
Escape cleared the selection and the app returned to an idle/default state.
The W193 fixture SHA256 remains
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

| W221 check | Result |
|---|---|
| Approved app access | PASS; isolated qmake6 app reconnected after Device Control approval |
| Normal pointer entity selection | PASS; property sheet reports `Offset Curve (1)` and selected contour/grips are visible |
| Selected-entity and blank-canvas secondary-click | OPEN; no visible context menu in either case |
| Layers-panel secondary-click | INCONCLUSIVE; accessibility capture error and no visible popup |
| App/drawing cleanup | PASS; Escape cleared selection; no edit or save performed |
| W193 fixture integrity | PASS; SHA256 unchanged at `05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d` |
| Retained screenshot / display-DPR | OPEN; UI state was observed in-session but no durable screenshot or DPR record was captured |
| PR CI at observation | IN PROGRESS; Text Foundations Headless ON/OFF passed; CodeQL analysis, DWG/DXF verification, and Pixi platform jobs were still running; Security scan skipped by policy |
| Product-source changes | None; visible validation and roadmap receipt only |

W221 upgrades normal-pointer selection from W214's unverified state to a
bounded pass. It does not close context-menu acceptance, the remaining visible
refusal/interaction matrix, independent external-CAD rendering, screenshot/DPR
evidence, deployed-package and W0-W2 acceptance, reviewer approval, or final R
integration.

## 180. W222 Compact Offset Options and Commit/Undo Recheck

In the approved isolated qmake6 app from W220, opened Tools > Modify > Offset
on the disposable W193 drawing. At the captured 1920x975 app viewport, the
compact options strip displayed Fixed Distance `0.5`, Keep Originals, Multiple
Copies `2`, Use Current Attributes, Use Current Layer, and Editable Offset
Curve without visible clipping. The Action Options property pane exposed the
same settings. This is a scoped layout observation at one viewport size, not a
display-DPR or responsive-size qualification.

Selected an existing offset ellipse, completed selection, and used the
direction prompt. An offset output became visible after the action was exited;
Escape returned to Default and one Undo restored the original visible
three-contour groups. The W193 file was not saved. Its SHA256 remains
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.
Because the transient preview was not separately captured before commit, this
does not establish preview/commit geometric parity. The UI tool also does not
expose a held mouse-down / move / Escape sequence, so held-drag cancellation
remains open.

| W222 check | Result |
|---|---|
| Compact option visibility | PASS at the observed viewport; all configured option labels/values visible in the toolbar strip and action property pane |
| Offset commit and Undo | PASS; offset output became visible, then one Undo restored the baseline geometry |
| Escape and cleanup | PASS; action returned to Default; drawing was not saved |
| W193 fixture integrity | PASS; SHA256 unchanged at `05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d` |
| Preview/commit parity | OPEN; no stable separate preview snapshot before commit |
| Held-drag cancellation | OPEN; CUA interaction does not provide a held-button move/cancel sequence |
| Screenshot / display-DPR | OPEN; in-session screenshot only; Qt DPR not measured |
| Product-source changes | None; visible validation and roadmap receipt only |

W222 closes one compact-options layout observation and one commit/undo
recovery case only. Context menus, held-drag cancellation, preview/commit
parity, remaining refusal/interaction cases, independent external-CAD
rendering, durable screenshot/DPR evidence, deployed-package and W0-W2
acceptance, reviewer approval, and final R integration remain open.

## 181. W223 Locked-Layer Offset Selection Refusal

In the approved isolated qmake6 app and disposable W193 drawing, used the
Layers panel's Lock All control. With layer 0 visibly locked, started Tools >
Modify > Offset and clicked the right-group outer ellipse. The action remained
at `Select entities to offset`; Return did not advance to the direction prompt,
so no commit stage was reached. Escape returned to Default. Unlock All restored
the layer, and the drawing showed the original three-contour groups with no
selected entities. The file was not saved; SHA256 remains
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

| W223 check | Result |
|---|---|
| Locked-layer selection refusal | PASS; source did not advance from entity selection and no offset commit occurred |
| Lock-state cleanup | PASS; Unlock All restored the layer and idle/default action state |
| Drawing/fixture integrity | PASS; original visible geometry retained; file SHA256 unchanged |
| Product-source changes | None; visible validation and roadmap receipt only |

W223 closes one locked-layer selection refusal case. Stale-peer behavior,
held-drag cancellation, context menus, preview/commit parity, remaining GUI
refusal/interaction cases, independent external-CAD rendering, durable
screenshot/DPR evidence, deployed-package and W0-W2 acceptance, reviewer
approval, and final R integration remain open.

## 182. W224 FreeCAD DXF Visual Retry

Inspected the existing FreeCAD 1.1.4 first-start state without changing its
settings: no theme was selected; Language was English, Unit System was Standard
(mm, kg, s, degrees), and Navigation Style was CAD. Used its Open dialog to
load `/private/tmp/offset-W200-oda-output/offset-matrix-write-dwg-R2018-d3_actual.dxf`.
The DXF import dialog reported approximately 233 geometric entities and kept
its default Individual Part shapes selection; the `Do not show this dialog
again` option remained unchecked. After import, the document title and layer
container appeared, but the geometry viewport was blank. Clicking Fit All did
not produce a visible rendering. While inspecting the layer tree, the document
view disappeared and FreeCAD returned to the Start page. No file was saved and
no first-start preference was applied. This is an inconclusive rendering
attempt, not evidence that the DXF is visually correct or incorrect.

| W224 check | Result |
|---|---|
| DXF open/import | PARTIAL; FreeCAD accepted the file and reported approximately 233 geometric entities |
| Geometry viewport | OPEN/UNVERIFIED; blank after import and Fit All; no rendered geometry confirmed |
| FreeCAD first-start settings | PASS; theme/language/units/navigation left unchanged; import dialog preference left unchecked |
| File integrity | PASS; source DXF opened read-only and was not saved |
| Durable screenshot / display-DPR | OPEN; only in-session screenshot observation; Qt DPR not measured |
| Product-source changes | None; external visual investigation and roadmap receipt only |

W224 does not close the external-CAD rendering gate. The W171-derived DWG/DXF
viewport, saved screenshot/DPR evidence, remaining GUI interaction matrix,
deployed-package acceptance, reviewer approval, and final R integration remain
open.

## 183. W225 Approved-Bundle Offset Selection and Context Menu

Reconnected to the approved isolated qmake6 app at
`/private/tmp/LibreCAD-PR3193-visible.app`. Its W193 canvas visibly rendered
the two groups of three ellipse contours. A primary click on the right-hand
carrier changed the property selector to `Offset Curve (1)`. Secondary-clicking
that selected carrier left the selection and canvas unchanged; a screenshot
showed no context menu. An exploratory blank-canvas click/secondary-click also
showed no popup, but the existing selection remained, so this does not qualify
the unselected blank-canvas case. A measurement prompt surfaced during an
additional pointer probe; Escape cancelled it, and a second Escape cleared the
selection. The app returned to Default with no selected entities. No drawing
was saved. The fixture SHA256 remains
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

| W225 check | Result |
|---|---|
| Approved LibreCAD bundle | PASS; visible app and W193 document were accessible |
| W193 viewport | PASS; the ellipse contours rendered in the approved bundle |
| Offset carrier selection | PASS; property selector reports `Offset Curve (1)` |
| Selected-carrier secondary-click | OPEN; screenshot confirms no visible context menu; invocation-path/product behavior remains unclassified |
| Unselected blank-canvas secondary-click | OPEN; not isolated because the prior selection remained |
| Cleanup and file integrity | PASS; prompt cancelled, selection cleared, Default restored, no save, fixture hash unchanged |
| Screenshot / display-DPR | PARTIAL; screenshots observed in-session, not retained as durable evidence; DPR unavailable |
| Product-source changes | None; visible validation and roadmap receipt only |

W225 strengthens the approved-bundle selection evidence and captures the
selected-offset context-menu result, but does not close context-menu acceptance.
Preview/commit parity, held-drag cancellation, remaining GUI refusal/interaction
cases, independent external-CAD rendering of the W171-derived drawing, durable
screenshot/DPR evidence, deployed-package and W0-W2 acceptance, reviewer
approval, and final R integration remain open.

## 184. W226 qmake6 Native Helper/Client Rerun

Reran the workspace qmake6 bundle's
`librecad_offset_worker_client_tests` executable. All 17 test cases passed with
4581 assertions. The suite includes real helper completions and geometric
refusals, timeout/cancellation of real children, recovery, and cached Produce
reuse across a fresh helper process. The executable and adjacent helper hashes
are recorded below. The repository diff from the qualified product-source pin
`71ecf102b36017d57bd30db39d0de630ac11bb75` remains roadmap-only, but this
workspace build's exact build commit is not independently stamped into the
binary. Do not promote it to packaged/deployed or cross-platform acceptance.

Attempted to reconnect to FreeCAD for a follow-up external visual check; CUA
reported the Mac locked before exposing the app. No UI action, document change,
or preference change occurred. The user has been asked to unlock it; continue
the remote CI and source audit while awaiting that state change.

| W226 check | Result |
|---|---|
| qmake6 helper/client suite | PASS; 17 cases / 4581 assertions |
| `librecad_offset_worker_client_tests` SHA256 | `b39ebaa1a6795c816d15de7eae30ad4e5fd032b8b17614eca1ac381b46ace71e` |
| `LibreCADOffsetWorker` SHA256 | `10440841d4e03c2d9a764ecfc67f6bb855f008ee0ff8bcf2d1667b05a915b407` |
| `LibreCAD` SHA256 | `6219de40a5a3c3160eb56d86390fa616002347982ea064cd629e6d70d7e2f80f` |
| Product-source delta since qualified pin | PASS; only `docs/plan_offset_curve_2961.md` differs |
| Exact binary build provenance | OPEN; no embedded commit stamp; hashes identify tested artifacts |
| Visible FreeCAD/LibreCAD continuation | BLOCKED FOR THIS ATTEMPT; Mac locked before app access; no preferences or files changed |
| Current PR CI | IN PROGRESS at capture; exact-head statuses are tracked in the PR comment |

W226 strengthens local native worker/client evidence but does not close W0-W2
packaged, cross-platform or full GUI acceptance. FreeCAD visual rendering,
context-menu acceptance, preview/commit parity, held-drag cancellation,
durable screenshot/DPR evidence, deployed-package verification, required
review approval and final R integration remain open.

## 185. W227 ASan Stale Publication Refusal

Ran the current-worktree CMake build's `librecad_offset_creation_publication_tests`
ASan executable from `/private/tmp/librecad-offset-2961-wire-asan` with the
`[stale]` filter and `ASAN_OPTIONS=detect_leaks=0`. Its CMake cache identifies
this worktree as the source directory. Both matching cases passed with 18
assertions:

- Offset creation publication rejects changed child membership before reading retired addresses.
- Offset creation publication refuses changed redo subtrees without touching freed children.

The tested executable SHA256 is
`d2a649e495645556a92e639e7b08e9a6cf196753254d5f3d4e33f1fce6e1cae4`. The
worktree source diff since the qualified product-source pin is documentation-only;
the binary itself has no embedded commit stamp. These cases strengthen stale
publication/freed-address safety but do not qualify visible stale-peer GUI
behavior. The Mac remained locked for this pass, preventing further desktop
interaction; no document or preference was changed.

| W227 check | Result |
|---|---|
| ASan stale publication filter | PASS; 2 test cases / 18 assertions |
| Changed child membership | PASS; stale membership rejected before retired-address read |
| Freed redo subtree | PASS; redo refusal leaves freed children untouched |
| Product-source delta | PASS; documentation-only since qualified product pin |
| Exact build commit stamp | OPEN; CMake source path verified, executable has no commit stamp |
| Visible stale-peer interaction | OPEN; ASan tests do not replace physical GUI interaction |

W227 closes only the local stale-publication safety cases. Visible stale-peer
interaction, held-drag cancellation, preview/commit parity, FreeCAD rendering,
durable screenshot/DPR evidence, deployed-package and W0-W2 acceptance,
reviewer approval, and final R integration remain open.

## 186. W228 Isolated Context-Menu Probes

Resumed the user-approved isolated qmake6 LibreCAD test bundle at
`/private/tmp/LibreCAD-PR3193-visible.app`. The W193 drawing visibly rendered
both groups of three ellipse contours. Normal pointer mode did not select an
entity until `Select Entities` was enabled; clicking the right-group outer
contour then showed `Offset Curve (1)` in the property selector. A secondary
click on that selected carrier displayed no context menu. After restoring the
original `Select Entities`-off/no-selection state, a secondary click on blank
canvas also displayed no context menu. The app remained responsive and the
viewport was visually intact. No drawing save or preference change was made;
the source fixture SHA256 remains
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.
FreeCAD 1.1.4 is now accessible but remains on its first-start configuration
screen. Its theme, language, unit system, navigation style, and profile were
left unchanged, so no new external-CAD rendering attempt was made.

| W228 check | Result |
|---|---|
| Approved app access | PASS; test bundle opened and accepted UI input |
| W193 rendering | PASS; both groups of three ellipse contours visible |
| Offset Curve selection | PASS after enabling Select Entities; property selector reports `Offset Curve (1)` |
| Selected-carrier secondary click | OPEN/NOT REPRODUCED; no visible context menu |
| Unselected blank-canvas secondary click | OPEN/NOT REPRODUCED; isolated with no selection active |
| UI state cleanup | PASS; Select Entities restored off and property selector returned to No Selection |
| Fixture integrity | PASS; no save, SHA256 unchanged |
| Retained screenshot / display-DPR | OPEN; screenshots observed in-session only; no durable image or DPR measurement |
| FreeCAD external visual review | OPEN; app accessible, but first-start configuration left untouched and no geometry viewport rechecked |
| Product-source changes | None; visible-app validation and roadmap receipt only |

W228 removes the blank-canvas ambiguity from W225. W230's source trace confirms
that an idle unmodified right-button release should reach the default popup
unless a matching custom menu activator overrides it; the Offset Curve-specific
menu factory exists. The visible non-popup result is therefore an unresolved
GUI routing/settings discrepancy, not evidence that menu behavior is optional
or a confirmed product defect. Context-menu acceptance remains open pending an
isolated-profile event-routing check. Stale-peer
interaction, held-drag cancellation, preview/commit parity, independent
FreeCAD rendering, durable screenshot/DPR evidence, deployed-package and W0-W2
acceptance, reviewer approval, and final R integration remain open.

## 187. W229 Preview-Parity Interaction Limit

In the approved qmake6 test app and disposable W193 drawing, started Tools >
Modify > Offset, selected the left outer ellipse and advanced to `Specify
direction of offset, or enter distance <0.5>`. The screenshot at that prompt
showed the selected source but no offset preview. CUA exposes click and drag
operations but no hover-only pointer move, so moving to a direction point would
also deliver a button event and could commit the operation. Canceled with
Escape before choosing a direction. LibreCAD returned to `Current Action:
Default`, `No Selection`; the six original contours remained visible and no
save occurred. The on-disk W193 fixture hash was rechecked as
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

FreeCAD 1.1.4 was accessible, but remained on its first-start configuration
screen. Theme, language, unit system, navigation style and profile were left
unchanged; the external visual check was not retried.

| W229 check | Result |
|---|---|
| Modify Offset selection and direction prompt | PASS; selected a source ellipse and reached direction prompt |
| Preview capture | OPEN; no preview visible at prompt; hover-only movement unavailable through CUA |
| Cancellation and cleanup | PASS; Escape returned to Default and No Selection; no commit/save |
| W193 fixture integrity | PASS; on-disk SHA256 unchanged |
| FreeCAD external visual review | OPEN; app accessible, first-start settings left untouched |
| Exact-head PR checks | IN PROGRESS; Headless ON/OFF pass; Analyze, DWG/DXF and platform builds pending |
| Product-source changes | None; visible-app validation and roadmap receipt only |

W229 does not establish preview/commit parity. A future acceptance run needs a
safe pointer-move-only mechanism or a deterministic UI test that captures the
preview separately before committing. Held-drag cancellation, stale-peer GUI
interaction, independent FreeCAD rendering, durable screenshot/DPR evidence,
deployed-package and W0-W2 acceptance, review approval, and final R integration
remain open.

## 188. W230 Context-Menu Source Trace

Traced the no-menu W228 observation through the current source without changing
product code. When no current edit action is active, `QG_GraphicView` routes a
right-button release away from the default select action and calls
`invokeContextMenuForMouseEvent`. `LC_CreatorInvoker` recognizes an unmodified
right-button release as the default-menu event unless a matching custom menu
activator applies. `LC_MenuFactoryGraphicView` builds an Offset Curve-specific
Modify submenu, and existing UI tests exercise its source actions and native
owner normalization. Thus an idle right-click should normally present a popup,
but a matching custom activator/settings entry or native event-delivery path
could change that result. The approved bundle's visible no-menu observation
remains unexplained; it is neither a pass nor a confirmed product failure.

| W230 check | Result |
|---|---|
| Idle right-button routing source | PASS; default path invokes graphic-view popup when no edit action is active |
| Offset Curve popup actions | PASS; factory defines Offset Curve-specific Modify actions; focused unit tests exist |
| User custom-menu override possibility | OPEN; settings/event configuration not modified or isolated in this visible run |
| Visible popup result | OPEN; W228 did not display a menu on selected or blank canvas |
| Product-source changes | None; source review and roadmap refinement only |

Next acceptance step: reproduce through a test-only QSettings profile or an
event-pipeline test that asserts the idle right-button release reaches the menu
factory and that the Offset Curve popup is shown. Do not edit the normal user
profile to force a result. Keep context-menu integration open until that check
separates custom-menu configuration from product event routing.

## 189. W231 Context-Menu Event Probe Follow-Up

With the user-approved test bundle, confirmed that a left click selects the
Offset Curve owner and the property selector reports `Offset Curve (1)`. A
right click on that selected owner still displayed no popup. A blank-canvas
right click and a blank-canvas left double-click also displayed no popup. The
last left double-click is only a probe of the popup factory path; it does not
replace the required right-release interaction. The selected contour was
cleared by clicking empty canvas with selection enabled, then `Select Entities`
was restored to off. The property selector returned to `No Selection`, the
command prompt returned to `Enter Command`, and no drawing save occurred. The
W193 source fixture still hashes to
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

Read the test bundle's current settings without modifying them. The only
persisted menu activator is `NNNMDNE` (`Middle` + `Double-Click`, no entity),
which does not match the right-button release. This makes a custom right-click
activator an unlikely explanation in the observed profile, but the run still
does not isolate settings or prove which native Qt mouse event CUA delivered.
`QG_GraphicView::mouseReleaseEvent` should route an idle right-button release
to `invokeContextMenuForMouseEvent`; `LC_CreatorInvoker` should permit the
default menu when no matching activator applies; and the factory builds a
non-empty default menu. The missing popup remains an unresolved GUI event-path
discrepancy, not a pass and not a confirmed product defect.

| W231 check | Result |
|---|---|
| Left-click selection path | PASS; selected contour resolved to `Offset Curve (1)` |
| Selected-owner right-click popup | OPEN; no popup visible |
| Blank-canvas right-click popup | OPEN; no popup visible |
| Blank-canvas double-click popup probe | OPEN; no popup visible; not a substitute for right-release test |
| Menu activator settings | PARTIAL; observed profile has only a middle-button double-click activator; no setting changed |
| UI cleanup | PASS; `Select Entities` off, `No Selection`, command idle |
| Fixture integrity | PASS; no save, source hash unchanged |
| Product-source changes | None; visible probe and roadmap receipt only |

Next step remains a test-only QSettings-profile run or deterministic event-pipeline
test that records the actual `QMouseEvent` button/type/modifiers and verifies
the popup reaches `QMenu::exec`. Do not change the normal user's settings.
FreeCAD external rendering, preview/commit parity, held-drag cancellation,
stale-peer GUI behavior, retained screenshot/DPR evidence, deployed-package and
W0-W2 acceptance, reviewer approval, and final R integration remain open.

## 190. W232 Timed Escape Delivery Measurement

Repeated Modify Offset on the disposable W193 fixture and reached the direction
prompt after selecting one ellipse. Held a direction-point click for 3000 ms and
scheduled Escape 500 ms after the click began. The CUA call timestamps show
Escape requested 502 ms after start, the click resolved 3125 ms after start,
and the Escape call resolved 3165 ms after start. Thus the bridge serialized
the key input until about 40 ms after mouse release; it did not deliver Escape
while the button was held. LibreCAD returned to Default with No Selection, and
the screenshot showed the original six contours with Undo disabled. No save
occurred; W193 still hashes to
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

| W232 check | Result |
|---|---|
| Direction prompt reached | PASS; selected one W193 ellipse |
| Escape scheduling | PASS; requested at +502 ms during a 3000 ms click |
| Key delivery before release | FAIL/UNAVAILABLE; Escape resolved at +3165 ms, after click at +3125 ms |
| Persistent geometry | PASS; six original contours, Undo disabled |
| UI cleanup | PASS; Default, No Selection |
| Fixture integrity | PASS; no save, hash unchanged |
| Product-source changes | None; timed UI probe and roadmap receipt only |

W232 confirms that this CUA bridge cannot prove cancellation while a mouse
button is held. Keep held-drag cancellation open until a native event-pipeline
test or another input tool can interleave Escape before release. Preview/commit
parity, visible CUA context-menu input, stale-peer GUI behavior, independent
FreeCAD rendering, durable screenshot/DPR evidence, deployed-package and W0-W2
acceptance, reviewer approval, and final R integration remain open.

## 191. W233 Deterministic Context-Menu Event Route

After the user approved control of the isolated LibreCAD test bundle, repeated
an unmodified right click on a visible ellipse. The CUA screenshot still showed
no popup. This remains an inconclusive observation of the UI-driving path; it
does not override the native event test below. The disposable W193 fixture
remains unchanged at SHA256
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

Added a GUI regression test that creates an MDI drawing, confirms the default
menu factory returns multiple actions, sends a `QMouseEvent::MouseButtonRelease`
for unmodified `Qt::RightButton` through `QG_GraphicView`, observes the shown
`QMenu`, and closes it from the nested event loop. The exact-worktree CMake
Release build and focused test pass (8 assertions). Existing Offset Curve
source-popup tests pass (64 assertions in three cases), verifying that a child
display chord resolves to the native owner and dispatches the Offset Curve-
specific action. Together these tests qualify application event routing and
owner-context behavior without changing user settings. The external CUA click
itself remains unqualified.

| W233 check | Result |
|---|---|
| Default menu factory | PASS; menu exists with more than one action |
| Qt right-button release path | PASS; `QG_GraphicView` event displays the popup |
| Offset Curve owner popup actions | PASS; existing source UI test covers owner normalization and dispatch |
| Visible CUA right-click | INCONCLUSIVE; no popup shown by UI driver |
| Fixture integrity | PASS; W193 hash unchanged and no save occurred |
| Build/test | PASS; CMake Release target builds; `[context-menu]` passes 8 assertions; `[dock-window]` passes 51 cases / 1042 assertions |
| Source popup tests | PASS; 64 assertions across three cases |
| Roadmap/refusal audit | PASS; 94/94 tests, 93 obligations, zero pending declarations or production mismatches |
| Product behavior changes | None; deterministic GUI regression coverage and roadmap receipt only |

The context-menu application route is no longer a known implementation gap.
Keep visible native-input confirmation as a test limitation. Native held-drag
cancellation remained open at this point and is addressed by W234; visible
held-drag interaction is still unverified. Preview/commit parity, stale-peer GUI
behavior, independent FreeCAD rendering, durable screenshot/DPR evidence,
deployed-package and W0-W2 acceptance, reviewer approval, and final R
integration remain open.

## 192. W234 Native Held-Drag Escape Event Pipeline

Added an offset-action regression test using a real `QG_GraphicView`, its normal
event handler, a selected ellipse, and the offset creation worker. The test
places the reference point, presses at the direction point, moves with
`Qt::LeftButton` still held, waits until the worker child is busy, dispatches
Escape through the view before releasing the mouse, waits for worker shutdown,
then sends the delayed left-button release. The action finishes, the source
remains live, and no output entity is published. This closes the native
held-drag cancellation test gap; it does not claim visible CUA input proof.

| W234 check | Result |
|---|---|
| Active child during held-button move | PASS; worker busy before Escape |
| Escape through the view event handler before release | PASS; action finishes and preparation cancels |
| Delayed left release after cancellation | PASS; no offset output is published |
| Source and drawing integrity | PASS; selected ellipse remains live and entity count unchanged |
| Offset worker regression group | PASS; 12 cases / 334 assertions, including active-child cancellation |
| Focused event-pipeline test | PASS; 16 assertions |
| Visible app-control attempt | UNAVAILABLE; user approved the isolated bundle, but the Mac still reports locked |
| Refusal-inventory validation | PASS; 94/94 tests, 93 obligations, zero pending declarations or production mismatches |
| Implementation commit | `c8b2f2d5` (`test: cover held offset preview cancellation through view events`) |
| Product behavior changes | None; event-pipeline regression test and roadmap receipt only |

The CMake Release `librecad_tests` target rebuilt from this worktree. Visible
held-drag confirmation remains a UI-driver limitation.
Preview/commit parity, stale-peer GUI behavior, independent FreeCAD rendering,
durable screenshot/DPR evidence, deployed-package and W0-W2 acceptance,
reviewer approval, and final R integration remain open.

## 193. W235 Native View-Route Preview/Commit Parity

Added a second deterministic `QG_GraphicView` event-pipeline test. It starts
Modify Offset on a selected ellipse, sends the reference click and held-button
direction move through the real Qt view, waits for the worker preview, and
captures entity type and geometry data before releasing at the same point. The
commit is also routed through the view and worker. The comparator checks
endpoints, bounds, and shape-specific ellipse, spline, or polyline data when
those result types are emitted. It verifies that the committed batch reuses the
cached preview result, preserves the source, and adds exactly one undo cycle.
This proves native action/event-path preview parity for the tested request; it
does not provide a physical-app screenshot or display-DPR record.

| W235 check | Result |
|---|---|
| Worker preview captured before commit | PASS; view-routed held-button move and asynchronous completion |
| Geometry comparison | PASS; preview and committed shapes have matching types and captured geometry data |
| Cache behavior | PASS; commit reports zero engine reruns and one reused cache result |
| Document history and source | PASS; one undo cycle; original ellipse remains live |
| Focused parity test | PASS; 25 assertions |
| Offset creation-worker regression group | PASS; 13 cases / 359 assertions |
| Implementation commit | `d72d9dd23` (`test: verify offset preview parity through view events`) |
| Product behavior changes | None; event-pipeline regression test and roadmap update only |

Native preview/commit parity for the tested GUI request is now qualified.
Physical screenshot/DPR evidence remains open, along with physical stale-peer
interaction, independent FreeCAD rendering, deployed-package and W0-W2
acceptance, reviewer approval, and final R integration.

## 194. W236 Native View-Route Stale-Peer Rejection and Recovery

Added a `QG_GraphicView` event-pipeline regression test for source mutation
while the offset commit worker is active. It establishes a preview from real
press/move events, starts commit on release, mutates the selected ellipse
before the worker returns, and verifies the stale request publishes no
geometry or undo cycle, leaves the source live, clears preview state, and
reports changed inputs. The test then drives a fresh move/press/release through
the same view and confirms the new geometry commits in one undo cycle while
preserving the edited source. This qualifies the native event-path behavior;
physical app interaction and screenshot/DPR evidence remain separate gates.

| W236 check | Result |
|---|---|
| Stale-peer edit during active commit worker | PASS; geometry edit is refused without publishing output |
| Recovery after a fresh view preview | PASS; new request commits and preserves the edited source |
| Document history and feedback | PASS; stale request adds no undo; successful retry adds one; changed-input message shown |
| Focused stale-peer event-pipeline test | PASS; 39 assertions |
| Offset creation-worker regression group | PASS; 14 cases / 398 assertions |
| Implementation commit | `eee2cf419` (`test: cover stale offset peer through view events`) |
| Product behavior changes | None; event-pipeline regression test and roadmap receipt only |

The native stale-peer route is now covered. Physical visible stale-peer
interaction, held-drag and preview/commit screenshots, independent FreeCAD
rendering, durable screenshot/DPR evidence, deployed-package and W0-W2
acceptance, reviewer approval, and final R integration remain open.

## 195. W237 Full CMake Exact-Worktree Requalification

After the W236 event-pipeline test was pushed, rebuilt and ran the complete
CMake Release application test executable from the worktree. All 5,155 cases
completed: 5,080 passed, 75 were explicitly skipped, and all 10,348,250
assertions passed. The separate registered CTest suite passed all nine worker
lanes, including process, fault, geometry-timeout, and ordinary-process tests.
This is test-only requalification; no product behavior changed.

| W237 check | Result |
|---|---|
| Full application test suite | PASS; 5,155 cases, 5,080 passed, 75 skipped; 10,348,250 assertions passed |
| Registered CTest worker suite | PASS; 9/9 tests |
| Build | PASS; CMake Release `librecad_tests` target rebuilt |
| PR head during validation | `b68b2c9571ef0f7b61a0683ecbb0c049e28fd843` |
| Product behavior changes | None; validation and roadmap receipt only |

The physical GUI, external rendering, deployed-package, W0-W2, reviewer, and
final R integration gates remain open as recorded above.

## 196. W238 Fresh qmake6 Bundle and Relocated Helper Smoke

Configured and built the complete qmake6 application from the current worktree
in `/private/tmp/offset-2961-w237-qmake` using Qt 6.12.0 on macOS arm64. The
out-of-source `qmake6 .../librecad.pro` configuration and `make -j4` completed
with exit 0, producing LibreCAD, plugins, and the adjacent
`LibreCADOffsetWorker`. Rebuilt the CMake worker-client test target from this
worktree, copied only that runner into the isolated app bundle, and ran the
protocol suite against its sibling helper. Then relocated the complete bundle
with `ditto` and reran the suite from the new path; both runs passed. This
proves the current bundle-relative helper lookup and protocol path locally,
not GUI-visible use, signing/notarization, or platform-wide deployment.

| W238 check | Result |
|---|---|
| qmake6 / Qt toolchain | PASS; qmake 3.1, Qt 6.12.0, macOS arm64 |
| Full application/plugin/helper build | PASS; fresh out-of-source `make -j4`, exit 0 |
| In-bundle worker-client suite | PASS; 17 cases / 3,779 assertions |
| Relocated-bundle worker-client suite | PASS; 17 cases / 3,727 assertions |
| LibreCAD SHA-256 | `206c319ba7dfcf29dfe7467352d1bb29b736b1315505e83517e8fc1d5d53c454` |
| LibreCADOffsetWorker SHA-256 | `4ec36eede0e1e9caec5e0d620b1dc463e8d8e6f11cca589ffb6aeadfc8fa1b83` |
| Client-test runner SHA-256 | `5aad800d8ea2498653715cf46b07c24ba70005ed335a64221575ef553376516f` |
| Source head | `e42d20711ac5597853b8e23eb223b4c059f45f00` |
| Product behavior changes | None; exact-source build and worker acceptance only |

W238 closes the fresh local qmake6 bundle/helper smoke. Visible GUI, independent
FreeCAD rendering, durable screenshot/DPR evidence, cross-platform and full
deployed-package W0-W2 acceptance, review approval, and final R integration
remain open.

## 197. W239 Exact-Head Headless CI Receipt

The W238 qmake6 bundle and relocation validation were pushed at head
`2c9b5bcb25e50865f301ee64c2ec41df7d9eb276`. Its exact-head Text Foundations
workflow `41220922934` completed successfully on both required configurations:
Headless ON job `124595108527` passed in 2m33s and Headless OFF job
`124595108712` passed in 2m19s. GitHub emitted only the existing runner
annotation that checkout/upload-artifact actions target Node.js 20 while the
runner forces Node.js 24; the jobs themselves passed.

| W239 check | Result |
|---|---|
| Headless ON | PASS; run `41220922934`, job `124595108527` |
| Headless OFF | PASS; run `41220922934`, job `124595108712` |
| Tested source head | `2c9b5bcb25e50865f301ee64c2ec41df7d9eb276` |
| Product behavior changes | None; exact-head CI receipt only |

This receipt does not close physical GUI, external FreeCAD rendering,
cross-platform/full deployed-package W0-W2 acceptance, required review, or final
R integration. Fresh CI is required for the following roadmap commit.

## 198. W240 Exact-Head Headless Requalification

The W239 roadmap update was pushed at
`a63a26eaf8ece575717c5c718e0ee07de96dccc7`. Its exact-head Text Foundations
workflow `41221162702` passed both configurations: Headless ON job
`124595826786` completed in 2m21s and Headless OFF job `124595826617` completed
in 4m45s. Both configured, built, ran the foundations suite, and retained test
results successfully. The only annotation was the existing GitHub warning that
checkout/upload-artifact actions targeting Node.js 20 are forced onto Node.js
24 by the runner.

| W240 check | Result |
|---|---|
| Headless ON | PASS; run `41221162702`, job `124595826786` |
| Headless OFF | PASS; run `41221162702`, job `124595826617` |
| Tested source head | `a63a26eaf8ece575717c5c718e0ee07de96dccc7` |
| Product behavior changes | None; exact-head CI receipt only |

Physical GUI, external rendering, cross-platform/full deployed-package W0-W2,
required review and final R integration remain open. Fresh exact-head CI must
qualify the next roadmap commit.

## 199. W241 Exact-Worktree and External Import Requalification

The existing ASan CMake cache points to this implementation worktree. Rebuilt
`librecad_tests` and the sibling worker after adding W236's stale-peer
`QG_GraphicView` regression. The focused `[stale-peer]` case passed 38
assertions, and the full `[creation-worker]` selection passed 14 cases / 398
assertions. Both used `ASAN_OPTIONS=detect_leaks=0`; these results do not claim
leak-sanitizer coverage. Also ran FreeCAD 1.1.4's isolated DXF importer on
`/private/tmp/offset-W200-oda-output/offset-matrix-write-dwg-R2018-d3_actual.dxf`
(SHA256 `b696b00f19d19f70fad13e6472c9d8149e4e33a75c0f6f5e5f8b6476dd53bf46`).
It reported one SPLINE, no unsupported features, and produced one non-null valid
BSpline edge. Its bounds are 0.0000625 mm wide by 0.0124998 mm high, with edge
length 0.0125 mm. An isolated FreeCAD GUI run on the native macOS backend saved
`/private/tmp/offset-2961-freecad-viewport.png` (SHA256
`5c62da29cda1a0d78728c7084d0b4f03697b175f528361404d2252fd0f30df98`). The
canvas shows only a very thin vertical trace, so this is partial viewport
evidence, not a contour-fidelity pass. A separate Qt offscreen attempt could
not create the OpenGL context required by FreeCAD and produced no capture. The
GUI used a temporary user config; no drawing was saved. Product behavior and
implementation files are unchanged.

| W241 check | Result |
|---|---|
| ASan CMake source directory | PASS; `/Users/dli/.codex/worktrees/offset-2961-runtime/LibreCAD` |
| ASan test/helper rebuild | PASS; `librecad_tests` and `LibreCADOffsetWorker` |
| Focused stale-peer event test | PASS; 1 case / 38 assertions |
| Creation-worker regression group | PASS; 14 cases / 398 assertions |
| FreeCAD DXF import | PASS; one AC1032 SPLINE imported as a valid one-edge BSpline, no unsupported features |
| FreeCAD native viewport capture | PARTIAL; screenshot at `/private/tmp/offset-2961-freecad-viewport.png`, SHA256 `5c62da29cda1a0d78728c7084d0b4f03697b175f528361404d2252fd0f30df98`; only a very thin vertical trace is visible |
| FreeCAD contour/fidelity review | OPEN; trace is not sufficient to judge the expected curve contour |
| FreeCAD offscreen GUI attempt | BLOCKED; Qt offscreen plugin cannot create the required OpenGL context |
| FreeCAD profile | PASS; used an isolated temporary user config |
| Sanitizer scope | `detect_leaks=0`; no leak-detection claim |
| PR head during validation | `b1a16f53364545e89eddfabfed439f8fdb4a3728` |
| Product behavior changes | None; exact-worktree and independent-import checks only |

This does not close current-head remote CI, physical LibreCAD GUI interaction,
independent external contour/fidelity review, cross-platform/full deployed-
package W0-W2 acceptance, required review, or final R integration.

## 200. W242 DXF Spline Extrusion and External Import Fix

An external FreeCAD import of application-generated AC1032 DXF found that
`DRW_Spline` value-initializes `normalVec` to `(0,0,0)`. Although the DXF
extrusion vector is optional and defaults to +Z, the shared SPLINE/HELIX body
writer serialized that zero sentinel because it only omitted explicit +Z.
FreeCAD consequently rejected the spline with “zero-length extrusion
direction.” Commit `151853f9a` now omits both the unspecified zero sentinel and
explicit +Z, while preserving explicit non-default normals. Its regression
checks SPLINE default, SPLINE non-default, and HELIX default output.

The exact-worktree `librecad_dxf_fast_tests` target passed 803 cases (799
passed, 4 skipped) and all 84,484 assertions. The focused
`[dxf][spline][helix][writer]` selection passed 2 cases / 16 assertions. The
roadmap inventory parser passed all 5 tests and `git diff --check` passed.
FreeCAD 1.1.4 imported a fresh file produced by the exact-source application:
`/private/tmp/offset-2961-normal-fix-current/offset-matrix-write-dxf-R2018-d3_actual.dxf`
(SHA256 `c693321e30827281fbb71cd0d82d4b9a5eb7c326d4becba3f7738171009a9bd2`).
It found one SPLINE, no unsupported features, and one valid one-edge shape; the
zero-extrusion warning was absent. This validates file acceptance, not visual
contour fidelity. The earlier very thin viewport trace remains inconclusive.

| W242 check | Result |
|---|---|
| Focused SPLINE/HELIX writer regression | PASS; 2 cases / 16 assertions |
| Complete DXF fast suite | PASS; 803 cases, 799 passed, 4 skipped, 84,484 assertions |
| FreeCAD import of fresh application output | PASS; one SPLINE, one valid edge, no unsupported features, no zero-extrusion warning |
| Fresh DXF evidence | `/private/tmp/offset-2961-normal-fix-current/offset-matrix-write-dxf-R2018-d3_actual.dxf`; SHA256 `c693321e30827281fbb71cd0d82d4b9a5eb7c326d4becba3f7738171009a9bd2` |
| Code commit | `151853f9a` (`fix: omit zero spline extrusion in DXF`) |
| Roadmap receipt commit | PASS; `97323cf5fbf2491dc9d63b8b35b8968860217f4c` |
| PR head at the W242 validation capture | `b1a16f53364545e89eddfabfed439f8fdb4a3728`; later superseded by W243 head checks |
| LibreCAD visible GUI check at the W242 capture | BLOCKED at that capture; W243 later verified the unlocked test copy |
| External contour/fidelity review | OPEN; successful import does not establish visual fidelity |

At the W242 capture, code commit `151853f9a` was already in the PR history, but
the W242 roadmap receipt had not yet been pushed. The receipt is now included
in W243's exact PR-head status.
Physical GUI preview evidence, independent contour review, cross-platform/full
deployed-package W0-W2 acceptance, required review, and final R integration
remain open.

## 201. W243 Exact-Worktree Requalification and GUI Evidence

W243 rechecked exact worktree head `97323cf5fbf2491dc9d63b8b35b8968860217f4c`.
No product source was changed. The temporary FreeCAD audit macro used during
this pass was removed; the DXF exports and build trees remain under
`/private/tmp`.

The CMake Release rebuild completed, followed by all 26 registered CTest
entries. `librecad_tests` reported 5,083 cases, 5,057 passed, 26 skipped, and
10,347,377 assertions passed. The qmake6/Qt 6.12.0 build returned success with
no rebuild work pending; its exact-worktree app bundle ran 17 worker-client
cases with 4,829 assertions. A relocated copy ran 17 cases with 4,377
assertions. Relocation retained identical worker and test-binary hashes.

The approved visible LibreCAD test copy displayed the W193 content as two
groups of three concentric contours. Offset selection and cancellation left the
document geometry unchanged; a later preview probe did not produce a
verifiable pre-commit ghost image, so separate visible preview/commit evidence
remains open. No source fixture was overwritten. FreeCAD 1.1.4 had previously
visibly rendered the W193 input; the current GUI import attempt exited at the
import-dialog transition and is not counted as evidence.

The test copy exported
`/private/tmp/offset-W193-W243-current-export.dxf` (SHA256
`8add98d9ff457f88f82e413978f939e8a0676a6eb6650002cb3f3d403c52e459`). FreeCAD
1.1.4's headless importer read two ELLIPSE and four SPLINE entities and created
six valid one-edge shapes, but emitted eight zero-extrusion warnings. The
exporting test-copy executable hash (`df35f8adef5c2ef6ec16aeecd8cb1ecaa7401557da012b02ea09eec0a6903966`)
does not match the exact qmake6 bundle executable hash
(`587d0f0bf5514f1bc6daf389b4d1fbd55b4aba3415356671857dc3f767b19aad`); this is
therefore a stale-bundle diagnostic, not a regression result for W242. The
exact-source W242 SPLINE output was re-imported headlessly with no zero-length
extrusion warning and one valid edge; its very thin bounds remain insufficient
to establish contour fidelity. The W171-derived AC1032 DXF was also re-audited
headlessly at its recorded SHA256
`b696b00f19d19f70fad13e6472c9d8149e4e33a75c0f6f5e5f8b6476dd53bf46`: FreeCAD
again reports one valid SPLINE edge, with bounds only 0.0000625 mm wide by
0.0124998 mm high. This confirms file acceptance and explains why it cannot
serve as a useful contour-fidelity fixture; the external visual gate remains
open.

| W243 check | Result |
|---|---|
| Exact-worktree CMake Release build | PASS |
| Full CTest | PASS; 26/26 entries |
| Main application suite | PASS; 5,083 cases, 5,057 passed, 26 skipped, 10,347,377 assertions |
| qmake6/Qt 6.12.0 app build | PASS; no pending rebuild work |
| Bundled worker-client suite | PASS; 17 cases / 4,829 assertions |
| Relocated worker-client suite | PASS; 17 cases / 4,377 assertions; worker/test hashes unchanged |
| LibreCAD W193 visible geometry | PASS; two groups, three contours each |
| Visible preview-before-commit screenshot | OPEN; no verifiable ghost preview captured |
| Exact-source FreeCAD spline import | PASS; one valid edge, no extrusion warning; fidelity remains open |
| W171-derived FreeCAD re-audit | PARTIAL; one valid edge at unchanged recorded hash, but bounds are too thin to judge contour fidelity |
| PR #3193 exact head | `97323cf5fbf2491dc9d63b8b35b8968860217f4c`; Headless ON/OFF, CodeQL, Analyze, Fast DWG/DXF, Windows x64/ARM64, relocated-worker, and Pixi checks pass; Security scan skipped by workflow policy |
| W243 roadmap receipt | Committed and pushed as `949afcb60139c6f383b64fa9393a77eb2f958a95` |

Exact-head Windows x64 completed successfully at 2026-10-31 21:04:03Z, including
the relocated-worker step. The W243 plan inventory parser and `git diff --check`
passed after recording the result. The roadmap receipt was pushed to PR
#3193; resulting head `949afcb60139c6f383b64fa9393a77eb2f958a95` queued five
Pixi platform checks. The preceding product-code head's complete DWG/DXF,
Windows, Pixi, Headless and CodeQL checks are recorded above. W244 integrates
the newer master tip locally; its merge commit and roadmap receipt are the next
PR update. Physical stale-peer interaction, preview/commit parity screenshots,
W171-derived external-CAD fidelity, screenshot/DPR records, deployed W0-W2
acceptance, review approval, and final R integration remain open.

## 202. W244 Upstream Integration and Exact-Source Requalification

The PR's base had advanced from `5a5fe7345fe7a7ec66892e879967673b6d7a859b`
to `origin/master` `998e61dc0b994d3ecf3b5e6b529123c556dc3494`, leaving PR #3193
marked conflicting. The worktree was clean before integration. The upstream
merge is `c9fc81575e0e5949059b40c5bde0957fd72dac35`; all upstream product,
translation, test and workflow changes are retained. Three overlapping files
were manually resolved: the DWG/DXF workflow keeps the unconditional PR trigger
and offset worker lanes while adding upstream dimension paths/targets; CMake
keeps both offset and dimension test registrations; the dock-layout test source
keeps both staged-offset-import hooks and dimension-popup coverage. The resolved
workflow parses as YAML, the CMake project configures, and no conflict markers
remain.

The exact merged source was rebuilt in CMake Release at
`/private/tmp/offset-2961-r-release` with integration and offset-worker tests
enabled. The application and dimension-clipboard executables, DWG/DXF lanes,
and all declared native worker/fault targets built successfully. Serial CTest
passed all 25 entries in 38.33 seconds, including worker process/fault/timeout,
import identity, dimension clipboard, DWG/DXF, and translation-catalog tests.

The full bounded application suite, seed `254215480`, ran 5,099 cases: 5,071
passed, 26 skipped, and two failed (five assertions). Failures were in
`Each measured dimension exposes its generated label`
(`lc_dimension_clipboard_tests.cpp`, four assertions) and
`Dimension menu actions copy snapshots of the context entity`
(`lc_property_copy_tests.cpp`, one assertion). Both test sources are unchanged
from `origin/master`. To distinguish regression from order-sensitive baseline,
a clean detached worktree at exact `origin/master` `998e61dc0` was configured
and built independently in `/private/tmp/offset-plan-master-baseline`; the same
suite, environment and seed reproduced exactly those two failures and five
assertions (3,731 cases, 3,704 passed, 25 skipped). The dimension-clipboard CTest
lane itself passes on the merged tree. Do not hide these inherited aggregate
failures or expand offset scope to edit the upstream dimension tests.

The exact merged source was also rebuilt with qmake 3.1 / Qt 6.12.0 from
`/private/tmp/offset-2961-w237-qmake` using `qmake6 -o Makefile
/Users/dli/.codex/worktrees/offset-2961-runtime/LibreCAD/librecad.pro` and
`make -j4`; both returned success. The bundled worker-client selector passed
17 cases / 4,344 assertions, and a fresh bundle copy at
`/private/tmp/offset-W244-relocated.yPKzEj/LibreCAD.app` passed 17 / 3,913.
The app, helper and client executable hashes matched between original and
relocated bundles (app `9fc9b5a8e2efccf6a4693ff4d3219ae922c9da037a96da357febf9360ca84149`,
helper `a42cb47bcedc27ba8e7adbbf253e01f43d0692e7204c5f451c28f1b37268f1a7`,
client `5aad800d8ea2498653715cf46b07c24ba70005ed335a64221575ef553376516f`).

| W244 check | Result |
|---|---|
| Upstream integration | PASS locally; merge `c9fc81575`; PR conflict resolved in the local branch |
| CMake Release build | PASS; application, clipboard, DWG/DXF and offset worker targets |
| CTest | PASS; 25/25 |
| Full bounded app suite | BASELINE-LIMITED; two dimension tests fail identically on clean `origin/master` with the same seed; 26 skips on merged source |
| qmake6 / Qt 6.12.0 app/helper | PASS |
| Bundled worker client | PASS; 17 cases / 4,344 assertions |
| Relocated worker client | PASS; 17 cases / 3,913 assertions; app/helper/client hashes unchanged |
| Visible exact-source preview/commit and DPR evidence | OPEN; no verified ghost screenshot or durable display/DPR record |
| External W171 contour-fidelity review | OPEN; import acceptance is not visual fidelity |
| PR #3193 exact merged head | Not pushed yet; push merge plus this roadmap receipt, then re-check current-head CI |

Physical stale-peer interaction, separate held-drag/preview/commit evidence,
independent external contour fidelity, deployed/cross-platform W0-W2 acceptance,
review approval, and final R closeout remain open. The visible app control path
was responsive only for the approved stale test copy; the exact-source app
instance remained non-interactive through accessibility, so no new physical
preview claim is made.

## 203. W245 Pushed-Head and Validation-Tool Requalification

W244's merge and roadmap receipt were pushed to PR #3193 as exact head
`eed09cb2a8efd029582c98b067e0352ba58ca9cb`. GitHub now reports the PR as
mergeable against `origin/master` `998e61dc0b994d3ecf3b5e6b529123c556dc3494`.
The push-triggered DWG/DXF run was superseded by the PR-triggered run; the
duplicate canceled jobs are not treated as failures.

The complete documented Node validation-tool command,
`node --test scripts/offset-curve-validation/test-*.js`, passed all 412 tests
with zero failures, cancellations, skips or TODOs in 198.9 seconds. The run
includes the roadmap parser, the 93-obligation refusal inventory, real child
process/evidence publication controls, independent checker cases and worker
pipeline regressions. It validates the optional developer tooling, not native
GUI acceptance or external CAD rendering.

| W245 check | Result |
|---|---|
| Roadmap validation-tool suite | PASS; 412/412; 198.9 seconds |
| PR #3193 head | `eed09cb2a8efd029582c98b067e0352ba58ca9cb`; OPEN, draft, mergeable |
| Headless (ON/OFF) | PASS on exact PR head |
| DWG/DXF Linux and Windows x64/ARM64 | IN PROGRESS on exact PR head |
| CodeQL Analyze | IN PROGRESS; Security scan skipped by workflow policy |
| Pixi platform builds | IN PROGRESS on exact PR head |
| Visible exact-source GUI | OPEN; CUA still reports macOS locked after the user's unlock response |
| Product-source changes | None; merge integration and validation only |

The new head is independently built and tested locally in W244, and CI is
qualifying that same pushed head. Physical stale-peer interaction, separate
held-drag/preview/commit screenshots with display-DPR, external W171 contour
fidelity, deployed/cross-platform W0-W2 acceptance, reviewer approval, and final
R integration remain open. No visual evidence is inferred from the synthetic
tool suite or headless import results.

## 204. W246 Unlocked-Session GUI Recheck and Exact-Head CI

After the user confirmed the Mac was awake, the approved exact-source LibreCAD
test copy became responsive. The existing saved acceptance file
`/private/tmp/offset-2961-visible-acceptance.dxf` was reopened in the app and
visibly showed five left-hand contours and three right-hand contours, with no
selection and the command prompt idle. The test-only Offset distance had been
temporarily set to 1.5 during a preview probe; it was restored to the original
0.5 before leaving the tool canceled. No source drawing was overwritten and no
product source changed.

The app-specific CUA screenshot was visible in-session, but not persisted as a
file. Two attempted `screencapture` outputs were inspected and found to show the
Codex desktop rather than LibreCAD; both were removed. Therefore W246 does not
close the durable screenshot or display-DPR evidence gate. The prior W245
preview/commit/Undo/Redo and DXF round-trip evidence remains separately recorded
in the PR comment; this recheck only reconfirms the reopened drawing's contours.

Exact PR head `c54ecd29ed04fdb6d2bd76a0f017764f53aefeb7` completed its current
validation run successfully. The bounded Windows x64 job took 5h38m58s; Linux,
Windows ARM64, headless ON/OFF, C++ Analyze, and all five Pixi platform builds
also passed. CodeQL finished neutral/skipped under workflow policy, and the
Security scan was skipped by workflow policy. No PR head change was made during
the run.

| W246 check | Result |
|---|---|
| Exact PR head | `c54ecd29ed04fdb6d2bd76a0f017764f53aefeb7` |
| Headless ON/OFF | PASS |
| DWG/DXF Linux | PASS; 1h06m42s |
| DWG/DXF Windows ARM64 | PASS; 2h17m44s |
| DWG/DXF Windows x64 | PASS; 5h38m58s |
| C++ Analyze | PASS |
| Pixi builds | PASS; macOS Intel/latest, Ubuntu x64/ARM64, Windows |
| CodeQL / Security scan | Neutral/skipped / skipped by workflow policy |
| Visible saved-DXF reopen | PASS; five left and three right contours; app idle |
| Durable preview/commit screenshots and DPR | OPEN; capture mismatch confirmed, no durable app image |
| External W171 contour-fidelity review | OPEN; FreeCAD first-run confirmation remains unanswered |

The code head is fully green for this CI run. Acceptance still requires physical
stale-peer and held-drag/preview/commit interaction evidence, independent W171
contour-fidelity review, durable screenshot/display-DPR evidence, deployed and
cross-platform W0-W2 acceptance, reviewer approval, and final R integration.

## 205. W247 FreeCAD Import Attempt and Exact-Head CI Checkpoint

With user approval, FreeCAD 1.1.4 first-run defaults were accepted (English,
Standard mm/kg/s, CAD navigation; theme unchanged). The temporary acceptance
DXF `/private/tmp/offset-2961-visible-acceptance.dxf` was selected read-only;
FreeCAD's DXF import dialog estimated 93 geometric entities. Confirming import
terminated the application before a viewport could be inspected. A second
attempt using fused-shape import also terminated FreeCAD. The crash report at
`~/Library/Logs/DiagnosticReports/freecad-2026-11-01-010325.ips` records
`EXC_BAD_ACCESS` during `QMacAccessibilityElement` teardown. This is not a
geometry-rendering result and does not establish a defect in the DXF parser.
No drawing was saved; the temporary DXF was not edited.

At this checkpoint, PR head
`8b99aabf45e1d2b00e06380dce68dcb58ffe38f7` has passed Headless ON/OFF, C++
Analyze, Linux DWG/DXF, CodeQL analysis, and Pixi builds on macOS Intel/latest,
Ubuntu x64/ARM64, and Windows. The Security scan is skipped by workflow policy.
Windows x64 and ARM64 DWG/DXF jobs remain in their bounded test steps; neither
has completed. These checks belong to the documentation-only head `8b99aab`,
whose product source is unchanged from the separately green `c54ecd29` head.

| W247 check | Result |
|---|---|
| FreeCAD first-run defaults | PASS; user-approved defaults applied, theme unchanged |
| Acceptance DXF recognition | PARTIAL; import dialog estimated 93 geometric entities |
| FreeCAD visual viewport | OPEN; app terminated on both import confirmations |
| Crash evidence | `EXC_BAD_ACCESS` in `QMacAccessibilityElement` teardown; not evidence of geometry failure |
| Drawing/file integrity | PASS; no save, temporary DXF not edited |
| Headless ON/OFF, C++ Analyze, Linux DWG/DXF | PASS at `8b99aab` |
| Windows x64/ARM64 DWG/DXF | IN PROGRESS at `8b99aab`; bounded test steps |
| Pixi platforms | PASS at `8b99aab`; macOS Intel/latest, Ubuntu x64/ARM64, Windows |
| W171-derived external contour fidelity | OPEN; this was the W193-derived acceptance DXF, not a visual recheck of the W171 fixture |

The independent W171 visual check, durable app screenshot/display-DPR records,
physical stale-peer/held-drag/preview-commit evidence, deployed/cross-platform
W0-W2 acceptance, review approval, and final R integration remain open. No
product-code change is justified by this FreeCAD accessibility crash.

## 206. W248 W171-Derived FreeCAD Visual Retry

Retried the actual W171-derived ODA-converted AC1032 fixture in FreeCAD 1.1.4:
`/private/tmp/offset-W200-oda-output/offset-matrix-write-dwg-R2018-d3_actual.dxf`.
Its input hash matched the recorded SHA256
`b696b00f19d19f70fad13e6472c9d8149e4e33a75c0f6f5e5f8b6476dd53bf46`. The DXF
import dialog estimated 233 geometric entities. The dialog was set to Fused
Part shapes; confirming import terminated FreeCAD before a viewport appeared.
This repeats the FreeCAD failure on the W171-derived fixture itself, not only
the W193 acceptance drawing. It provides no visual contour-fidelity evidence
and does not distinguish an importer, rendering, or file-geometry cause. No
drawing was saved, and the source DXF hash remained unchanged.

At the time of this recheck, the pushed docs-only head was
`e15057e8a5c2f9706135f32db5015dff5edb0b9a`. Its Headless ON/OFF, C++ Analyze,
Linux and Windows DWG/DXF, and Pixi checks were pending; the Security scan was
skipped under workflow policy. Product source remains unchanged from the
separately green `c54ecd29` product head. A new exact-head run is required for
the W248 roadmap receipt.

| W248 check | Result |
|---|---|
| W171-derived file identity | PASS; recorded SHA256 matches `b696b00f...` before and after |
| FreeCAD DXF recognition | PARTIAL; import dialog estimated 233 geometric entities |
| FreeCAD rendered viewport | OPEN; app terminated on fused import confirmation |
| Drawing/file integrity | PASS; no save, source DXF unchanged |
| Independent W171 contour fidelity | OPEN; no viewport was produced |
| Exact-head CI at test time (`e15057e`) | IN PROGRESS; headless, analysis, DWG/DXF, Pixi pending; Security scan skipped |

Do not treat either the 93-entity W193 acceptance import or this 233-entity
W171 import-dialog estimate as a visual pass. Independent rendering, retained
screenshot/display-DPR evidence, physical GUI receipts, deployed/cross-platform
W0-W2 acceptance, review approval, and final R integration remain open.

## 207. W249 FreeCAD Retry and Local Validation

With user approval of the FreeCAD first-run defaults, retried the actual
W171-derived ODA-converted AC1032 file
`/private/tmp/offset-W200-oda-output/offset-matrix-write-dwg-R2018-d3_actual.dxf`
read-only. FreeCAD 1.1.4 estimated 233 geometric entities. Selected Individual
Part shapes and confirmed import; the application terminated before exposing a
viewport. The new report
`~/Library/Logs/DiagnosticReports/freecad-2026-11-01-011414.ips` records
`EXC_BAD_ACCESS` in `QMacAccessibilityElement` teardown. The input SHA256 still
matches `b696b00f19d19f70fad13e6472c9d8149e4e33a75c0f6f5e5f8b6476dd53bf46`.
No drawing was saved or modified. This is not visual-fidelity evidence and
does not establish a geometry/parser defect.

Local CTest passed all 25 registered tests against the current product source.
The full `node --test --test-reporter=dot scripts/offset-curve-validation/test-*.js`
suite also exited successfully. The dot reporter does not report a case count;
the latest explicitly counted full run remains W245's 412/412. After writing
this receipt, the focused roadmap parser/refusal suite passed 99/99 and reported
93 obligations, zero pending declarations, and zero production mismatches. The
current PR head `81fbcb96fc065edd9629c869e7a9bd80fbf84fcd` changes only this
roadmap file relative to product-source head
`c54ecd29ed04fdb6d2bd76a0f017764f53aefeb7`.
Headless ON/OFF passed on the current head; CodeQL analysis, DWG/DXF
verification (including Windows x64/ARM64), and all five Pixi platform jobs
were still in progress at this receipt. The Security scan is skipped under
workflow policy. The same facts and the W249 FreeCAD result were posted to the
existing PR validation comment.

| W249 check | Result |
|---|---|
| FreeCAD file recognition | PARTIAL; import dialog estimated 233 entities |
| Individual Part-shape import | OPEN; FreeCAD exited before viewport |
| Crash report | `EXC_BAD_ACCESS` in `QMacAccessibilityElement` teardown |
| DXF integrity | PASS; recorded SHA256 unchanged; no save |
| Local CTest | PASS; 25/25 |
| Full Node validation-tool suite | PASS; exit 0; dot reporter omitted case count |
| Roadmap parser/refusal suite | PASS; 99/99; 93 obligations, zero pending declarations/mismatches |
| Headless ON/OFF | PASS on `81fbcb96` |
| CodeQL, DWG/DXF, Pixi | IN PROGRESS on `81fbcb96`; Security scan skipped by policy |

The independent W171 contour-fidelity review, durable preview/commit and
held-drag screenshots with display-DPR, physical stale-peer interaction,
deployed/cross-platform W0-W2 acceptance, reviewer approval, and final R
integration remain open. FreeCAD's accessibility crash is not grounds for
changing product geometry code. Exact-source LibreCAD GUI control for the
temporary relocated bundle has been requested; no permission or app setting is
changed by this receipt.

## 208. W250 Bundled and Relocated Worker Requalification

Re-ran the native worker-client suite from both the qmake6/Qt 6.12.0 bundle
built from the current product source and its already-relocated copy. Running
`librecad_offset_worker_client_tests` from each bundle's `Contents/MacOS`
directory passed all 17 cases: 4,281 assertions from
`/private/tmp/offset-2961-w237-qmake/LibreCAD.app` and 3,815 assertions from
`/private/tmp/offset-W244-relocated.yPKzEj/LibreCAD.app`. The assertion total
varies with randomized geometry cases; both runs exited zero with no failures.

Recomputed executable SHA256 values are identical for the original and
relocated bundles: LibreCAD
`9fc9b5a8e2efccf6a4693ff4d3219ae922c9da037a96da357febf9360ca84149`, helper
`a42cb47bcedc27ba8e7adbbf253e01f43d0692e7204c5f451c28f1b37268f1a7`, and
worker-client test `5aad800d8ea2498653715cf46b07c24ba70005ed335a64221575ef553376516f`.
The qmake bundle corresponds to product-source head `c54ecd29`; current changes
since that source head are roadmap-only. This requalifies the local macOS
installed-helper path after W249. It does not establish GUI integration,
cross-platform full-application deployment, visible preview/commit behavior, or
the broader W0-W2 acceptance contract.

| W250 check | Result |
|---|---|
| Original app-bundle worker-client suite | PASS; 17 cases / 4,281 assertions |
| Relocated app-bundle worker-client suite | PASS; 17 cases / 3,815 assertions |
| Original vs relocated app/helper/test SHA256 | PASS; all three hashes identical |
| Roadmap parser/refusal suite after W250 receipt | PASS; 99/99; zero pending declarations/mismatches |
| Product source | PASS; both bundles match `c54ecd29`; no product source changed |
| Full deployed/cross-platform W0-W2 acceptance | OPEN; these are local macOS worker-client lanes only |
| PR head CI | IN PROGRESS on `81fbcb96`; do not supersede while Windows/Pixi jobs run |

The independent W171 contour-fidelity review, physical stale-peer and held-drag
interaction evidence, durable screenshots with display-DPR, full deployed and
cross-platform W0-W2 acceptance, reviewer approval, and final R integration
remain open.

## 209. W251 Exact-Source Console Raster Inspection

With the desktop locked, used the exact-source qmake6/Qt 6.12.0 macOS
application's existing `dxf2png` command to produce supplementary raster
renderings. The executable SHA256 is
`9fc9b5a8e2efccf6a4693ff4d3219ae922c9da037a96da357febf9360ca84149`, matching
the W244 bundle built from product-source head `c54ecd29`. It reported successful
AC1032/R2018 input and wrote both 1920x1080 images without modifying either DXF.

The W171-derived source
`/private/tmp/offset-W200-oda-output/offset-matrix-write-dwg-R2018-d3_actual.dxf`
retains SHA256 `b696b00f19d19f70fad13e6472c9d8149e4e33a75c0f6f5e5f8b6476dd53bf46`.
Its raster
`/private/tmp/offset-W250-w171-native-render.png` (SHA256
`470c7e8ad6cdc925860a1112c52110ef9693a594f0b0632d9e1005a0fb0a7361`)
shows the expected very thin near-vertical trace, consistent with the previously
recorded tiny DXF bounds. It remains unsuitable for judging contour fidelity.

The W193-derived acceptance DXF
`/private/tmp/offset-2961-visible-acceptance.dxf` retains SHA256
`51b82a318f2d6fbf25fdc2fc4e4e255410860a1f8f72927acc02287dd9bc77cb`. Its raster
`/private/tmp/offset-W250-acceptance-native-render.png` (SHA256
`2798aeb254b3f676744147b34e7928167a9140f1f50d3776fc60ac7b87d6df90`)
visibly contains the two separated groups of nested contours. This corroborates
the prior LibreCAD visible re-open result with a deterministic current-source
renderer and provides a retained raster for this acceptance fixture.

| W251 check | Result |
|---|---|
| Exact-source `dxf2png` for W171 AC1032 fixture | PASS; exit 0; AC1032/R2018; 1920x1080 |
| W171 visual inspection | PARTIAL; thin trace agrees with recorded bounds; not contour-fidelity evidence |
| Exact-source `dxf2png` for acceptance DXF | PASS; exit 0; 1920x1080; two nested-contour groups visible |
| Input DXF integrity | PASS; both recorded input hashes unchanged; no drawing saved |
| Retained native-render output | PASS; both PNGs retained under `/private/tmp` with SHA256 above |
| External CAD / desktop GUI / DPR acceptance | OPEN; console export is not an independent CAD or visible-session receipt |

This closes neither independent W171 contour review nor physical GUI evidence:
the raster uses LibreCAD's own DXF reader and renderer, does not capture a live
viewport, and has no Qt/display-DPR measurement. Physical stale-peer and
held-drag/preview/commit interaction evidence, full deployed/cross-platform
W0-W2 acceptance, reviewer approval, and final R integration remain open.

## 210. W252 FreeCAD First-Run Retry Blocked by Desktop Lock

The user approved accepting FreeCAD's displayed first-run defaults (English,
standard mm/kg/s, CAD navigation; leave the theme unchanged). Before applying
them, the native-app control surface again reported that macOS is locked and
cannot be unlocked automatically. A brief `QT_QPA_PLATFORM=offscreen` launch
probe started FreeCAD but did not expose an operable setup dialog or viewport;
it was interrupted and no FreeCAD process remains. No defaults were applied,
no drawing was saved, and no independent visual evidence was produced. This
does not change the previous W247/W248 FreeCAD import-crash findings or imply a
geometry defect.

The W171-derived DXF remains unchanged at SHA256
`b696b00f19d19f70fad13e6472c9d8149e4e33a75c0f6f5e5f8b6476dd53bf46`. The
current exact PR head is `5ee311a56d140eab434e7c02b89d69f2870381c6`; the
Text Foundations workflow and Headless OFF job passed, while Headless ON,
CodeQL, DWG/DXF verification (including Windows x64/ARM64), and Pixi platform
jobs remained in progress at this receipt. Security scan is skipped by
workflow policy. The PR is open, draft, mergeable, and still requires review.

| W252 check | Result |
|---|---|
| User approval for first-run defaults | PASS; defaults authorized, not applied |
| Native desktop control | BLOCKED; CUA reports macOS locked |
| FreeCAD setup dialog / viewport | OPEN; offscreen probe provided neither |
| Drawing integrity | PASS; no save; W171-derived DXF hash unchanged |
| Roadmap parser/refusal/evidence tests | PASS; Node test command exited 0 |
| Exact-head Text Foundations / Headless OFF | PASS on `5ee311a5` |
| Exact-head Headless ON, CodeQL, DWG/DXF, Pixi | IN PROGRESS at this receipt |
| Independent W171 contour-fidelity review | OPEN; no independent viewport |

Resume the independent FreeCAD visual check only when the desktop control
surface is available. Physical stale-peer and held-drag/preview/commit GUI
evidence with durable screenshots and display-DPR, full deployed/cross-platform
W0-W2 acceptance, human review approval, and final R integration remain open.

## 211. W253 FreeCAD Setup and Keyboard-Driven Import Retry

After the desktop became available, the user-approved FreeCAD 1.1.4 first-run
defaults were applied: English, Standard mm/kg/s, CAD navigation, and no theme
change. The actual W171-derived ODA DXF
`/private/tmp/offset-W200-oda-output/offset-matrix-write-dwg-R2018-d3_actual.dxf`
was opened read-only. FreeCAD's import dialog recognized approximately 233
geometric entities; the default Individual Part shapes mode was retained. To
avoid the accessibility-triggered dialog-dismissal path, the file was selected
and the import confirmed with keyboard input. FreeCAD still exited immediately
after confirmation, before any viewport appeared. CUA then reported no FreeCAD
window, reproducing the import-boundary failure without a visual result.

The earlier W247 import crash report records `EXC_BAD_ACCESS` in
`QMacAccessibilityElement` deallocation. The repeat attempt produced no new
geometry evidence and does not establish that this earlier stack is the cause
of the new exit. The input DXF remains unchanged (SHA256
`b696b00f19d19f70fad13e6472c9d8149e4e33a75c0f6f5e5f8b6476dd53bf46`); no
drawing was saved. No product change is justified by this external application's
failure.

The full offset validation Node suite passes 412/412 tests (exit 0; 353,722 ms).
At exact PR head `e246acd8e434c00487b39a57fa771ba6587cb5ec`, Text Foundations,
C++ Analyze, DWG/DXF verification (Linux fast lane and Windows x64/ARM64),
CodeQL analysis, and Pixi builds (Ubuntu x64/ARM64, Windows, macOS latest and
Intel) passed; the Security scan subjob is skipped by workflow policy. PR #3193
remains open, draft, mergeable, and review-required.

| W253 check | Result |
|---|---|
| User-approved FreeCAD first-run defaults | PASS; applied, theme unchanged |
| W171-derived DXF recognition | PARTIAL; FreeCAD reported about 233 entities |
| Keyboard-driven Individual Part shapes import | OPEN; FreeCAD exited before viewport |
| Drawing integrity | PASS; source hash unchanged; no drawing saved |
| Full offset validation Node suite | PASS; 412/412, zero failures |
| Exact-head CI for `e246acd8` | PASS; required build, test, analysis, and platform workflows green |
| Independent W171 contour-fidelity review | OPEN; no external viewport rendered |

Do not infer geometric failure or success from this FreeCAD termination. The
W171 contour-fidelity review, physical stale-peer and held-drag/preview/commit
evidence with durable screenshots and display-DPR, full deployed/cross-platform
W0-W2 acceptance, reviewer approval, and final R integration remain open.

## 212. W254 Inkscape DXF Import Attempt

As an alternate independent vector viewer after the W253 FreeCAD termination,
opened the unchanged W171-derived ODA DXF in Inkscape 1.4.2. Its DXF input
extension completed import and reported that `$PDMODE` is ignored and a point is
displayed normally. Zoom Drawing brought a filled, narrow shape into view only
at extreme magnification; reducing the zoom left a tiny trace. The page view
was blank, and no useful contour geometry could be distinguished. This confirms
only that the extension accepted the file; it is not CAD validation or a visual
fidelity pass.

The source DXF SHA256 remains
`b696b00f19d19f70fad13e6472c9d8149e4e33a75c0f6f5e5f8b6476dd53bf46`. No file
was saved, no persistent Inkscape preference was changed, and no screenshot or
display-DPR record was retained. The complete Node suite remains 412/412 and
the focused roadmap parser/refusal suite 99/99 on the product-equivalent tree;
the W253-only document edit is unchanged except for this additional evidence.

| W254 check | Result |
|---|---|
| Inkscape DXF import | PARTIAL; import completed with `$PDMODE` warning |
| Useful contour viewport | OPEN; only a tiny trace was visible at useful scale |
| Source integrity | PASS; original DXF hash unchanged, no drawing saved |
| Independent W171 contour-fidelity review | OPEN; no readable contour comparison |
| Durable screenshot / display-DPR | OPEN; not captured |

Retain the external fidelity gate. Do not interpret the scale/trace observed in
Inkscape as proof of geometry corruption or correctness. Physical stale-peer
and held-drag/preview/commit evidence, deployed/cross-platform W0-W2 acceptance,
reviewer approval, and final R integration also remain open.

## 213. W255 Current-Source LibreCAD Rendering and FreeCAD Retry

Used the rebuilt test-only LibreCAD copy at
`/private/tmp/LibreCAD-PR3193-visible.app`, whose product source is unchanged
from `c54ecd29` (the current roadmap head adds documentation only). The app
opened `/private/tmp/offset-W193-multi-entity.dxf`; Fit to View displayed both
groups of three contours in model space. The input SHA256 is
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`. The
screen was visibly inspected through CUA, but no durable screenshot or
display-DPR record was retained and the drawing was not saved.

The same test copy opened the W171-derived ODA output
`/private/tmp/offset-W200-oda-output/offset-matrix-write-dwg-R2018-d3_actual.dxf`.
Its ENTITIES section contains one SPLINE, which LibreCAD displayed after
Auto Zoom; this is an open single-curve fixture, not an independent contour
fidelity comparison. The source DXF SHA256 remains
`b696b00f19d19f70fad13e6472c9d8149e4e33a75c0f6f5e5f8b6476dd53bf46`, and no
drawing was saved.

With the user-approved FreeCAD 1.1.4 defaults now applied, retried the W171 DXF
in Fused Part shapes mode. FreeCAD still terminated at import confirmation
before presenting a viewport. This provides no geometry verdict and does not
replace the LibreCAD render check.

| W255 check | Result |
|---|---|
| Current-source LibreCAD W193 DXF import | PASS; file opened in model space |
| Six-contour rendering | PASS; both groups of three contours visibly rendered |
| W193 input integrity | PASS; recorded SHA256 matches, no save |
| LibreCAD W171-derived ODA DXF import | PASS; one model-space SPLINE displayed |
| Independent W171 contour-fidelity review | OPEN; no independent expected-contour comparison |
| FreeCAD fused-shape viewport | OPEN; application exited before viewport |
| Durable screenshot / display-DPR | OPEN; screen was observed but not retained |
| Exact-head CI at `048c8e1c` | IN PROGRESS at test time; Text Foundations passed, other workflows live |

The six-contour LibreCAD screenshot observation is product-app rendering, not
independent-reader fidelity. Preserve the W171 fidelity gate, durable screenshot
and DPR record, physical stale-peer/preview/commit evidence, deployed and
cross-platform W0-W2 acceptance, reviewer approval, and final R integration.

## 214. W256 Offset Commit and Undo in the Current-Source App

Continued in the approved test-only LibreCAD copy at
`/private/tmp/LibreCAD-PR3193-visible.app`, with
`/private/tmp/offset-W193-multi-entity.dxf` open. The fixture began with three
ellipses in each of two groups and its SHA256 remained
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.
Using Offset with fixed distance `0.5`, two copies, and Keep Originals enabled,
selected one ellipse and clicked the exterior side. The right group visibly
grew from three to five concentric ellipses; exiting the action and invoking
Undo restored both groups to three. Undo became disabled. The drawing was
closed with No at the save prompt, and no drawing was saved.

A separate transient-preview state was not captured reliably. The CUA `drag`
operation releases the button as part of the action and committed the offset;
the red in-command outline could not be distinguished conclusively from
selection highlighting. Therefore the held-drag and preview parity gate stays
open. The screenshots were observed inline but not retained as durable files,
and display DPR was not recorded. The worktree/product source was unchanged by
this GUI check. At exact PR head `84e553845`, both Text Foundations jobs passed;
the C++ analysis, DWG/DXF Linux and Windows jobs, and platform builds were still
pending, with Security scan skipped by policy.

| W256 check | Result |
|---|---|
| Current-source W193 load and rendering | PASS; two groups of three displayed |
| Offset GUI creation | PASS; one direction pick added two copies |
| Offset Undo | PASS; restored three ellipses per group, Undo disabled |
| Drawing integrity | PASS; W193 hash unchanged, closed with No, no save |
| Independent W171 contour-fidelity review | OPEN; no independent expected-contour comparison |
| Held-drag and transient-preview parity | OPEN; CUA drag releases and committed, preview state ambiguous |
| Durable screenshot and display-DPR record | OPEN; inline view only, neither retained |
| Exact-head CI at `84e553845` | IN PROGRESS; Text Foundations passed, other required jobs pending |

Continue to treat W193 GUI creation/undo as local interaction evidence only.
Physical stale-peer testing, true held-drag/preview/commit parity, deployed and
cross-platform W0-W2 acceptance, reviewer approval, and final R integration
remain open.

## 215. W257 Reapproved Rebuilt-App Offset Recheck

After the user reapproved the rebuilt test copy at
`/private/tmp/LibreCAD-PR3193-visible.app`, it launched and opened
`/private/tmp/offset-W193-multi-entity.dxf`. The two groups of three ellipses
rendered. With Offset set to distance `0.5`, two copies, and Keep Originals, a
direction pick added two exterior ellipses to the right-hand group. Canceling
the active command and invoking Undo restored the original three-per-group
layout; no Save command was issued. The input SHA256 remains
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

This recheck corroborates W256 on the newly approved rebuilt app. It did not
perform a second entity selection after the first creation, so GUI repeat-after-
creation is not claimed here. This run did not capture a frame during a held-
button move; physical transient-preview parity and stale-peer interaction remain
unverified. No durable screenshot or display-DPR measurement was retained. The
product source stayed unchanged. PR #3193 remains open, draft, and mergeable at exact head
`f1acb2867882fc0e30aa13e79a18f0312097e3a8`; Headless ON/OFF pass, while analysis,
DWG/DXF Linux and Windows, and Pixi jobs are still pending (Security scan is
skipped by policy).

| W257 check | Result |
|---|---|
| Reapproved rebuilt-app launch and W193 rendering | PASS; two groups of three displayed |
| Offset creation | PASS; two exterior copies visibly added |
| Undo and source integrity | PASS; original layout restored, input hash unchanged, no Save issued |
| GUI repeat-after-creation | OPEN; no second entity was selected in this run |
| Physical held preview and stale-peer interaction | OPEN; preview not established by the UI frame, stale-peer interaction untested |
| Durable screenshot / display-DPR | OPEN; neither retained |
| PR #3193 CI at `f1acb286` | IN PROGRESS; Headless ON/OFF passed, other required jobs pending |

## 216. W258 Transient Preview Probe

In the reapproved LibreCAD test copy, started Offset on the W193 fixture with
distance `0.5`, two copies, and Keep Originals. After selecting the left group's
outer ellipse, issued a drag from the curve toward the exterior while requesting
a screenshot concurrently. The captured frame showed the selected red ellipse
but no distinct offset preview contour, and did not establish transient-preview
parity. A subsequent side click visibly added two exterior copies; the action
returned to Default. Undo removed both and restored the original six-contour
layout. No Save command was issued; the source DXF hash remained
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`.

Successful creation completes the current action. This run did not start another
Offset invocation or perform another entity selection in that invocation, so it
does not close the GUI repeat-routing gate. No durable screenshot or display-DPR
record was retained. Product source stayed unchanged.

| W258 check | Result |
|---|---|
| In-session drag-time screenshot | INCONCLUSIVE; selected outline visible, no distinct preview contour established |
| Offset creation and Undo | PASS; two exterior copies added, then removed; original six contours restored |
| Source integrity | PASS; recorded W193 hash unchanged, no Save issued |
| GUI repeat-routing and physical stale-peer interaction | OPEN; not demonstrated in this run |
| Durable screenshot / display-DPR | OPEN; neither retained |
| Physical preview parity | OPEN; transient preview not established by the CUA frame |
| Exact-head CI after `4516f4d6` | IN PROGRESS; Headless ON/OFF passed, analysis, DWG/DXF and Pixi pending |

## 217. W259 Sequential GUI Repeat Check

On the reapproved test copy, ran Offset on a right-group ellipse with fixed
distance `0.5`, two copies, and Keep Originals. The right group visibly gained
two exterior contours. After the action returned to Default, reissued Offset,
selected a different ellipse in the left group, and reached the direction-pick
stage. Canceled before committing the second operation, then used Undo to
remove the first operation. The final visible drawing again showed two groups
of three ellipses. The input DXF hash remained
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`; no Save
command was issued.

This demonstrates sequential command re-entry and selection of a different
source after a completed offset. It does not demonstrate automatic same-action
continuation, a second committed operation, held-drag transient preview, or
preview/commit visual parity. No durable screenshot or display-DPR record was
retained. The product source stayed unchanged. At exact PR head
`8abe14730544ea29241d39b7e7f180e512552bed`, Headless OFF passed while Headless
ON, analysis, DWG/DXF and five-platform Pixi checks were pending; Security scan
was skipped by policy.

| W259 check | Result |
|---|---|
| Sequential Offset re-entry and different-source selection | PASS; fresh Offset invocation reached direction stage on a second source |
| Second operation / same-action continuation | OPEN; second operation canceled, no automatic continuation claimed |
| Undo and visible fixture restoration | PASS; first operation undone, two groups of three visible |
| Source integrity | PASS; recorded W193 hash unchanged, no Save issued |
| Roadmap inventory/refusal suite | PASS; 99 tests, 0 failures |
| Physical preview parity / stale-peer | OPEN; neither demonstrated |
| Durable screenshot / display-DPR | OPEN; neither retained |
| Exact-head CI at `8abe1473` | IN PROGRESS; Headless OFF passed, remaining required checks pending |

## 218. W260 Visible Transient Preview, Commit, and Undo

In the user-approved rebuilt test copy at
`/private/tmp/LibreCAD-PR3193-visible.app`, opened the unchanged
`/private/tmp/offset-W193-multi-entity.dxf`. The visible model contains two
groups of three ellipse contours. Started Offset with fixed distance `0.5`,
two copies, Keep Originals, and Editable Offset Curve enabled; selected the
left group's outer ellipse and advanced to the direction prompt. During a
drag-to-direction operation, requested a screenshot while the pointer move was
in flight. The captured frame showed a distinct red offset preview outside the
white source contours while the command remained at the direction stage. A
subsequent direction click committed two copies; the left group visibly gained
two contours and the action returned to Default. One Undo restored both groups
to their original three-contour state. No Save command was issued.

This is visible preview plus commit/Undo interaction evidence, but the inline
frames were not retained as durable files and display DPR was not measured.
The preview frame is not a pixel-level parity artifact, so exact preview versus
committed-geometry parity remains open. Physical stale-peer interaction was not
tested; W236's native event-pipeline stale-peer test remains the available
evidence. The separate W171 independent-reader contour-fidelity gate remains
open. The W193 source SHA256 is
`05aa253d513afc53d7e40bf51efaaf290e5a411cfbbcaaaa99df8386a4281e7d`; it was
rechecked after the UI interaction. Product source stayed unchanged.

| W260 check | Result |
|---|---|
| Transient Offset preview | PASS; distinct red preview visible before the direction click |
| Offset commit | PASS; two copies visibly added to the left group |
| Undo and fixture restoration | PASS; original two groups of three visibly restored |
| Source integrity | PASS; W193 SHA256 unchanged, no Save issued |
| Physical stale-peer interaction | OPEN; not exercised in the visible app |
| Exact preview/commit visual parity | OPEN; no durable/pixel-comparable captures |
| Durable screenshot / display-DPR | OPEN; inline view only, neither retained nor measured |
| Independent W171 contour-fidelity review | OPEN; no useful external contour comparison |
| Roadmap/refusal suite | PASS; 99 tests, 0 failures |
| PR #3193 CI at `e9fe078f4e31eb6917d4dfe3cf1be9e8204b6960` | IN PROGRESS at 2026-11-01 19:48 UTC; Headless ON/OFF and Text Foundations passed, CodeQL, DWG/DXF verification, and Pixi builds in progress; Security scan skipped by policy |
