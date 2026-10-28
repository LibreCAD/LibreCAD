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

Updated: 2026-10-28. Project: **LibreCAD**, not LibreCAD_3.
Status: **S0/W0-W2 partially implemented; helper/client, dialog/sheet/grip/Update routing and typed Produce qualified; creation/cache/trim/import integration remains open**.
The user explicitly confirmed that the shipped native geometry worker is required,
in addition to the Node developer validation runner. W0-W2 are mandatory.
The prior temporary checkout and test artifacts disappeared after interruption;
its observations are not durable acceptance. Retest affected changes here.
Implementation owner: `/Users/dli/.codex/worktrees/offset-2961-runtime/LibreCAD`,
branch `codex/offset-2961-native-worker`, initially pinned to
`df0c891ad5a0bb5b0d0e864f4cd9cdd8036d5f83`. Build/evidence root:
`/Users/dli/dev/LibreCAD/tmp/offset-2961-native-20261026`.
Latest upstream integration: merge `c2100eaff` includes freshly fetched
`origin/master` at `fb45503b8c10541958983230c0e5a633e83cb215`.
The W15 ledger below records the merged source, conflict resolutions and selected
Release qualification; earlier tracking-pin audits remain historical.
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
| Native geometry-worker path | Real six-operation protocol4 helper/client, typed source-copy production, same-producer ordinary fallback, plus dialog, sheet, grip and Update GUI routing committed/qualified | W0/W1, these callers and the isolated real-geometry-entry timeout lane are recorded. Preview-cache portability, creation/repeat routing, trim, import and packaged/final acceptance remain mandatory W0-W2 work. |
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
| S0 | Pin/reconcile candidate, main and feature; baseline/discovery; fix DWG enum classification and add R2007 execution | - | recovered; parser and ordinary R2007 slices qualified below; geometry/type/target qualification remains | `[curve-offset]`, `[lc-offsetcurve]`, `[entity-coverage]`, actual target/version inventory |
| A-core | Atomic shared admissions, frozen results, sticky status and ownership/lifetime | S0 | candidate `0eb2a0cfa`; audit/retest | `[lc-offsetcurve][budget-core]` |
| A-strict | Strict-query callback/record/order/depth/exception accounting | A-core | candidate `dc016f6e2`; audit/retest | `lc_parametriccurveintersection_tests.cpp`, named strict-budget cases |
| A-reference | Critical/provider/event accounting and atomic paired graph publication | A-strict | candidate `5c12fbeef`; audit/retest | `[lc-offsetcurve][reference-accounting]` |
| A-cache | Bounds/local cache, inherited depth and failed-stage snapshots | A-reference | candidate `dbf895638`; audit/retest | `[lc-offsetcurve][cache-budget-accounting]` |
| B | Immutable occurrence/edge-side terminals and issuer lifetime | A-cache | candidate `9bf0546b6`; audit/retest | geometry terminal/occurrence case inventory |
| C-search | Exhaustive certified cuts, owned independent laps, bounded equivalent-state memoization | B | candidate `b6814fb3a`; audit/retest | `lc_offsetpathsearch_tests.cpp`, public cut-search cases |
| C-check | Full public checker, directed coverage, incidence, seam/closure and typed refusal | C-search | candidate `d4a0b081a` plus later search; audit/retest | `[lc-offsetcurve][cache-topology-chain]` |
| W0 | Separate validation runner from product runtime; freeze native-worker schema, caps, deadlines and ownership | C-check | wire/request/reply and initial deadline/authority policy implemented; finish caller/fallback/import ownership freeze before routing | operation/cap/deadline/call-site table |
| W1 | Bounded headless geometry helper and parent transport, registered/packaged in both builds | W0 | helper/client and both build registrations qualified; full app/deployed-package gates remain | `[lc-offsetcurve][worker-protocol]`, `[worker-kernel]`, `[worker-process]`, `[worker-client]` |
| W2 | Deadline/cancel/crash/reap, validated result, UI caller adoption and timeout regression | W1 | fault/edit, dialog/sheet/grip/Update and geometry-entry timeout receipts qualified; creation/trim/import and final gates open | `[lc-offsetcurve][worker-client]`, `[worker-fault]`, `[worker-geometry-timeout]`, `[edit][preparation]`; remaining integration and runner regression |
| E1 | Guarded entity/factories, RTTI, stamp, clone, downgrade and transforms | C-check | candidate `92758859c` and later fixes; audit/retest | `[lc-offsetcurve][native]`, `[entity-coverage]` |
| U1 | Type/INSERT/selection/SVG/menu/info dispatch and safe properties | E1 | candidate `49c9b328b`, `f0fb28fab`; audit/retest | native type/name/dispatch TUs, `[entity-coverage]` |
| E2 | Cached grips/snaps, through proposal and transactional mutators | E1 | candidate `4300c1031`, `47e14c74b`; audit/retest | native edit/grip/transform TUs |
| G | Eligible creation, repeated offset and bounded ordinary fallback/reporting | E2,U1,W2 | candidate `a37982bd6`, `bf7b512cf`; shared preparation `ef40eecad` and typed Produce `558c3bba2` qualified; cache/GUI integration open | creation/repeat/modification TUs and existing Offset tests |
| F1 | Shared typed schema, raw-carrier eligibility, ownership and byte limits | E1 | candidate `305042903`; audit/retest | `lc_offsetcurvexdata_tests.cpp`, `lc_offsetcurvecarrier_tests.cpp` |
| F2 | Actual DXF/DWG SPLINE writer/adoption and transactional errors | F1,G | candidate `32597762b`, `7e46b2de8`; audit/retest | offset adapter/persistence/hostile-file TUs |
| F3 | Version/read/write/conversion, clipboard, independent fixtures and output inspection | F2 | candidate `79ae1930b`; fixtures present, rerun scope required | `[lc-offsetcurve][g-matrix]`, explicit per-row evidence |
| U2 | Single/multi properties, busy state, async preparation and stale-result protection | E2,U1,W2 | dialog `705a0135b` and sheet `ac7c7646f` isolated/qualified; final R requalification remains open | sheet/dialog/batch-failure TUs |
| U3 | Grip drag, cancel, ownership, undo/redo and no late publication | E2,U2 | worker continuation `0e80b8540` qualified Release/ASan; final R remains | `[lc-offsetcurve][h-drag]` |
| U4 | Ellipse-only persisted option, preview/commit parity and messages | G,F3,U2,U3 | candidate `3d7195631`; audit/retest | option-enable TU and visible workflows |
| O1 | Transactional Update from legal live ellipse; no-op/refusal counts | U4 | worker batch `075038088` qualified Release/ASan; final R remains | `lc_action_offsetcurve_source_tests.cpp` |
| O2 | Restore Original, same-container dedup, attributes and all entry points | O1 | candidate `0d3fed653`, `4a231d243` plus final fixes; audit/retest | Restore/source-UI TUs |
| T1 | Pure bounded trim prep and exact stored-cache fragments | C-check,E1,F2 | candidate `04c712069`; audit/retest | `[lc-offsetcurve][trim-kernel]`, standalone trim fault target |
| T2 | Trim/Trim Two, owner mapping, common prep and atomic undo | T1,U1,W2 | candidate `49bb7755f` plus final fixes; prepared worker consumption `3e4401990` qualified; async action routing remains open | `[lc-offsetcurve][trim-action]` |
| T3 | Plain-fragment metadata and trim/downgrade save/reopen/conversion | T2,F3,O2 | candidate `905158174`; audit/retest | `[lc-offsetcurve][trim-files]` |
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
| Produce5 | `[source,context,bounds,coordinate,fallback-or-null,distance,copies,asOffsetCurve,preview,batchLimits]`; shared ellipse/raw-native-cache copy series | Complete source diagnostics, exact per-copy output groups and definition/proposal binding. Parent uses private completion, preserves attribute policy and measures its own display expansion. Preview-cache portability and GUI routing remain open. |

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
   adoption work and per-copy binding. Before creation/repeat GUI routing, implement
   preview-cache portability and qualify budget/side/eviction parity. Native
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

All product executions are **not run for this plan revision**. Historical logs
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
build-offset/librecad_tests --list-tests "[lc-offsetcurve]"
build-offset/librecad_tests --list-tests "[curve-offset]"
build-offset/librecad_tests --list-tests "[entity-coverage]"
QT_QPA_PLATFORM=offscreen build-offset/librecad_tests "[lc-offsetcurve]" --order decl --reporter junit --out "$RECEIPTS/offset.xml"
QT_QPA_PLATFORM=offscreen build-offset/librecad_tests "[curve-offset]" --order decl --reporter junit --out "$RECEIPTS/adjacent.xml"
QT_QPA_PLATFORM=offscreen build-offset/librecad_tests "[entity-coverage]" --order decl --reporter junit --out "$RECEIPTS/types.xml"
```

These are future commands, not executed evidence. The receipt directory is unique;
also capture dirty diffs, toolchain/build identities and the actual commands.
S0 records actual discovery, including any zero future selector;
after registration, every required new slice selector must discover and execute
nonzero cases with its planned assertions. Catch2 registered cases, generated
assertions and JUnit entries are different counts. Use a structured XML parser.

The listed micro/fault targets are present on the pinned candidate, not the
older attached tree. Build failures caused by using that older tree are not a
reason to delete the gates. Discover/run each filter micro binary separately,
with its actual registered selectors and a unique report; include the grouped
DXF/DWG fast lanes too. Run both fault executables as standalone processes with
bounded capture, not Catch filters: global allocation overrides must never be
linked into the normal application/Catch target. Retain observed fault counts,
exit statuses, diagnostic prefixes and later recovery checks, not invented
expected assertion counts. At R also build the CMake application explicitly.

Current additional native targets are `librecad_offset_worker_wire_tests`,
`librecad_offset_worker_request_tests` and the explicit opt-in
`librecad_offset_worker_probe_tests` (`[worker-probe]`). The first two selectors
are `[lc-offsetcurve][worker-protocol][wire]` and
`[lc-offsetcurve][worker-protocol][request]`; reject zero discovery. The probe
is hidden from ordinary suites and reports measurements/refusals, not GUI timing
acceptance. After W1 adds its named native helper/lifecycle targets, add them to
both build and execution manifests and test the packaged helper launch. Merely
building the application or optional Node validation scripts cannot cover them.

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
- [ ] Validation recipe completeness: discover/execute `[offsetcurve]` as well
  as `[lc-offsetcurve]`, and list the implemented worker/micro/fault targets.

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

Immediate next slice: the complete selector/target recipe, with fresh broad
Release discovery/execution and explicit skipped external-tool accounting. Test
and commit the slice, immediately checkpoint this plan, and push to draft PR #3193.

Remaining creation/repeat steps after these review follow-ups:
1. Shared owned numeric preparation is implemented/qualified at `ef40eecad`.
   Target-relative source accounting is recorded above; retain upstream #3140.
2. Typed child Produce is implemented/qualified at `558c3bba2`, including private
   parent materialization, diagnostics, copy-series rollback and remaining limits.
   An original-definition rebase followed by the existing stored-cache fallback
   intentionally uses DIFFERENT inputs/remaining allowances; do not replace that
   fallback with ordinary geometry from the rebase invocation.
3. Preserve budget-sensitive preview-cache reuse, zero new producer effort on
   hits and its256-entry clearing policy across worker lifetimes. Qualify parity
   before GUI routing. Leave unrelated ordinary source tools unchanged.
4. Route preview/commit through owned continuations, complete context/source/undo/
   selection/UCS seals, cancellation/death handling and one atomic publication.
   Parent materialization must use the correct existing local attribute policy
   and charge actual display expansion before settling output reservations.

Then continue stored-cache/conic trim and unpublished import ownership in4.2 order.
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
- [ ] S0: finish the source/target reconciliation on the durable tree. Fresh
  baseline, ordinary DWG2007 correction, evidence links and broad offset/failure
  executions and full path/link classification are recorded above. The target-relative
  addendum retains already-upstream #3140; final source/runtime reconciliation remains.
  Do not redo implemented geometry as missing work.
- [ ] A-core through C-check: review/requalify existing code and patch only proven
  accounting, ownership, occurrence or global-topology gaps. No duplicate checker.
- [ ] W0-W2: typed Produce is qualified; finish cache portability, import ownership and remaining GUI
  adoption/failure acceptance. Reuse the qualified geometry-entry timeout/helper/client/
  reply modules; do not redo them as absent. Keep Node runner scope separate.
- [ ] E/U/G/F/O/T: requalify and integrate the existing entity, UI, creation,
  DXF/DWG, source and trim implementation with any new worker/publication changes.
- [ ] R: finish current-source Release/ASan/qmake6/failure/version/visible rows,
  review scoped commits, preserve reproducible evidence and close every required gap.

**Next concrete action on implementation resume:** preview-cache portability/parity,
then creation/repeat GUI routing in the remaining
4.2 caller adapters. The ordinary DWG2007 correction and fresh broad
baseline are already recorded; do not restart them as missing implementation.
Nothing is automatically copied into the dirty main tree. Native caller routing,
the remaining lifecycle/packaging gates and final UI/file acceptance are open.

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
