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

Updated: 2026-10-26. Project: **LibreCAD**, not LibreCAD_3.
Status: **S0 partially implemented in the durable worktree; plan incomplete**.
The user explicitly confirmed that the shipped native geometry worker is required,
in addition to the Node developer validation runner. W0-W2 are mandatory.
The prior temporary checkout and test artifacts disappeared after interruption;
its observations are not durable acceptance. Retest affected changes here.
Implementation owner: `/Users/dli/.codex/worktrees/offset-2961-runtime/LibreCAD`,
branch `codex/offset-2961-native-worker`, initially pinned to
`df0c891ad5a0bb5b0d0e864f4cd9cdd8036d5f83`. Build/evidence root:
`/Users/dli/dev/LibreCAD/tmp/offset-2961-native-20261026`.
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
| Read-only candidate snapshot | `/private/tmp/lc2961-plan-refresh.NI3tkd/candidate`, detached at `df0c891ad5a0bb5b0d0e864f4cd9cdd8036d5f83` | Clean local shared clone used for this review; no dependency installation, builds or product edits. It is an inspection aid, not a new implementation owner. |
| Preserved draft | `attic/plan_offset_curve_2961_history_20261026.md` | Exact pre-consolidation working draft, including pre-existing uncommitted work. |

**Next implementation baseline:** an isolated clean checkout pinned to the later
candidate, after S0's scope/ancestry audit. Do not overwrite the dirty main tree,
reset the attached worktree, rewrite the existing candidate branch, or recreate
all implemented modules from the older feature snapshot. Carry this current plan
into the isolated implementation checkout without importing another person's index.
The candidate's committed roadmap and its `docs/offset_curve_2961_validation.md`
are historical evidence at the pinned SHA, not competing scheduling instructions.

The archived draft is 5,762,095 bytes, SHA-256
`2dc8b28eccb4092af8f2ffba05342abfa69aae93d3963b6b7d37e67b62d73ee0`.
Its historical section numbers, V455 tuple/ledger proposals, failed attempts and
receipts remain unchanged. They are historical evidence, not active routing or
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
| Native geometry-worker path | No dedicated product helper/transport in inspected CMake/qmake lists; edits call the engine/checker in process | W0-W2 remain an explicit architecture/readiness gap in this draft. Resolve this distinction before any whole-plan completion claim. |
| Tests/build targets | Main Catch target, filter micro targets and two standalone offset allocation-failure targets | Candidate present; source registration is not discovery/execution. Use the actual targets in section5.3, not just the old grouped fast lanes. |
| Surviving test defect | `spline_offset_representation_tests.cpp` lines205 and668 in candidate still use DWG enum ranges | `FormatDWG2007` is outside the range; those ordinary representation loops also omit it. Fix classification and add an executed R2007 regression in S0. The separate candidate matrix already lists all six DWG versions. |

### 2.1 Findings to Close, Not Repeat

| Priority | Finding | Bounded disposition |
|---|---|---|
| P1 | A plan based only on the older tree labels implemented candidate features missing | S0 audits ancestry, scope and source differences; select the candidate without wholesale changes to the main checkout. Preserve/review A-C, E/U/G/F/O/T code rather than rebuilding it. |
| P1 | Test-runner success could be mistaken for product worker isolation/responsiveness | W0 pins which runtime is being qualified; W1/W2 implement the native path promised here. If only the validation runner was intended, record an explicit scope correction before removing the native requirement; do not silently count X-H7 as it. |
| P2 | Ordinary representation tests misclassify DWG2007 and never execute that row | S0 adds explicit enum mapping, both affected round-trip paths and a physical `AC1021`/reader-routing assertion, plus non-DWG negatives. No unrelated codec rewrite. |
| P2 | Existing entity/filter/failure tests would be missed by a geometry-only or grouped-fast-lane run | S0 inventories actual declarations/targets; each slice runs its named cases, relevant filter micro targets and standalone fault processes. Zero matches or omitted targets cannot close a slice. |
| P2 | Historical evidence and temporary artifacts could be promoted into current or durable acceptance | Hashes of all 14 indexed receipts and all 52 pinned fixtures match in this review. Preserve source/runtime scopes; rerun affected lanes on the selected source and retain needed provenance outside disposable directories before delivery. |
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
| W0 | Separate validation runner from product runtime; freeze native-worker schema, caps, deadlines and ownership | C-check | unresolved product architecture; bounded probe required | operation/cap/deadline/call-site table |
| W1 | Bounded headless geometry helper and parent transport, registered/packaged in both builds | W0 | absent from inspected product; implement after W0 | proposed `[lc-offsetcurve][worker-protocol]` |
| W2 | Deadline/cancel/crash/reap, validated result, UI caller adoption and timeout regression | W1 | native path absent; Node X-H7 is separate existing work | proposed `[lc-offsetcurve][worker-lifecycle]`; retained runner regression |
| E1 | Guarded entity/factories, RTTI, stamp, clone, downgrade and transforms | C-check | candidate `92758859c` and later fixes; audit/retest | `[lc-offsetcurve][native]`, `[entity-coverage]` |
| U1 | Type/INSERT/selection/SVG/menu/info dispatch and safe properties | E1 | candidate `49c9b328b`, `f0fb28fab`; audit/retest | native type/name/dispatch TUs, `[entity-coverage]` |
| E2 | Cached grips/snaps, through proposal and transactional mutators | E1 | candidate `4300c1031`, `47e14c74b`; audit/retest | native edit/grip/transform TUs |
| G | Eligible creation, repeated offset and bounded ordinary fallback/reporting | E2,U1,W2 | candidate `a37982bd6`, `bf7b512cf`; worker integration open | creation/repeat/modification TUs and existing Offset tests |
| F1 | Shared typed schema, raw-carrier eligibility, ownership and byte limits | E1 | candidate `305042903`; audit/retest | `lc_offsetcurvexdata_tests.cpp`, `lc_offsetcurvecarrier_tests.cpp` |
| F2 | Actual DXF/DWG SPLINE writer/adoption and transactional errors | F1,G | candidate `32597762b`, `7e46b2de8`; audit/retest | offset adapter/persistence/hostile-file TUs |
| F3 | Version/read/write/conversion, clipboard, independent fixtures and output inspection | F2 | candidate `79ae1930b`; fixtures present, rerun scope required | `[lc-offsetcurve][g-matrix]`, explicit per-row evidence |
| U2 | Single/multi properties, busy state, async preparation and stale-result protection | E2,U1,W2 | candidate sheet/dialog and `f617fcf70`; compute isolation open | sheet/dialog/batch-failure TUs |
| U3 | Grip drag, cancel, ownership, undo/redo and no late publication | E2,U2 | candidate `f214b3408` plus final snapshots; audit/integrate worker | `[lc-offsetcurve][h-drag]` |
| U4 | Ellipse-only persisted option, preview/commit parity and messages | G,F3,U2,U3 | candidate `3d7195631`; audit/retest | option-enable TU and visible workflows |
| O1 | Transactional Update from legal live ellipse; no-op/refusal counts | U4 | candidate `379729bb3` and snapshot fixes; audit/retest | `lc_action_offsetcurve_source_tests.cpp` |
| O2 | Restore Original, same-container dedup, attributes and all entry points | O1 | candidate `0d3fed653`, `4a231d243` plus final fixes; audit/retest | Restore/source-UI TUs |
| T1 | Pure bounded trim prep and exact stored-cache fragments | C-check,E1,F2 | candidate `04c712069`; audit/retest | `[lc-offsetcurve][trim-kernel]`, standalone trim fault target |
| T2 | Trim/Trim Two, owner mapping, common prep and atomic undo | T1,U1,W2 | candidate `49bb7755f` plus final fixes; audit/integrate worker | `[lc-offsetcurve][trim-action]` |
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
   Keep one active roadmap; Git history and the existing attic archive are history,
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
clock source, cancellation and kill/reap bounds before W1 is ready; absence of
those values is a W0 blocker, not permission to wait indefinitely.

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
2. Freeze typed Generate, ValidateCache, TrimCache and TrimConic requests,
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

After W1 adds its named native helper/protocol/lifecycle targets, add them to
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

S0 overall, A-C requalification, W0-W2, integration and R remain open. Next:
finish S0 geometry/type qualification and W0's typed operation/deadline/ownership
freeze, then the helper and GUI lifecycle. Neither a framing codec nor the
existing developer runner constitutes a native geometry worker. Both shared
indexes retain their recorded hashes; all three code slices and their immediate
plan checkpoints belong only to `codex/offset-2961-native-worker`.

- [x] Read-only reconciliation: identify the later descendant candidate and its
  real entity/checker/UI/file/source/trim code instead of calling it all missing.
- [x] Verify 14 retained receipt hashes and 52 pinned fixture hashes. This is
  identity/provenance checking only, not new runtime or interoperability evidence.
- [ ] S0: audit scope and pin a clean candidate source; fresh baseline/discovery;
  fix/execute the ordinary DWG2007 regression and reconcile evidence links.
- [ ] A-core through C-check: review/requalify existing code and patch only proven
  accounting, ownership, occurrence or global-topology gaps. No duplicate checker.
- [ ] W0-W2: resolve the native runtime contract, freeze bounded design and
  implement/validate its missing product path; retain Node runner scope separately.
- [ ] E/U/G/F/O/T: requalify and integrate the existing entity, UI, creation,
  DXF/DWG, source and trim implementation with any new worker/publication changes.
- [ ] R: finish current-source Release/ASan/qmake6/failure/version/visible rows,
  review scoped commits, preserve reproducible evidence and close every required gap.

**Next concrete action on implementation resume:** S0 step1, then the candidate
baseline and minimal representation-test correction. The later branch is not
automatically accepted or copied into the dirty main tree. Native-worker deadlines,
platform policy and integration are real open work; independent fixtures are
available, while current-tool availability and output reinspection still need checks.

Completion requires every required deliverable and validation row on the selected
source, no unhandled type/lossy silent fallback, no pending P1/P2 finding, and an
accurate immediate post-commit roadmap. Earlier scoped COMPLETE labels, build
success, hash equality, Node runner reports or a narrowed test subset cannot close
this plan. Keep explicit unsupported platform/manual-tool limitations; do not
mark a mandatory row PASS merely because its prerequisite is unavailable.

The original plan-only audit did not implement product changes or commit the
existing staged draft. This execution now implements the roadmap in the isolated
worktree above. The prior archive and both shared indexes remain untouched.

## 8. Validation of This Plan Update

Document checks cover all 27 unique slices, known/ordered predecessors, no cycles,
and every slice reaching final R. Negative controls reject a duplicate slice,
missing trim slice, unknown predecessor, dependency cycle and reversed order.
The selected SHAs/ancestry, candidate source and target presence, real acceptance
tags, explicit version enums and surviving DWG2007 defect are checked against
source. Shell recipes are syntax-checked without running product commands.

All 14 artifact hashes from the candidate's committed evidence index and all 52
fixture hashes from its three manifests match. Source-blob receipts are checked
against their recorded commits, not promoted into current runtime PASS. The
existing archive/license and shared indexes are unchanged. The read-only candidate
snapshot stays clean; only the live plan is edited in this pass (the earlier
archive remains an existing untracked file). The machine-readable audit is
`/private/tmp/lc2961-plan-refresh.NI3tkd/report.json`.

The earlier document-only audit remains at
`/private/tmp/lc2961-plan-audit.gRHoJ7/report-final.json`; it qualified its own draft
structure, not this newer candidate reconciliation. Neither audit qualifies
geometry, native workers, current DXF/DWG interoperability, visible interaction
or a product milestone. Product builds/tests remain not run for this update.

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
