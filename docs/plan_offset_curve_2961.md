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

Updated: 2026-10-29. Project: **LibreCAD**, not LibreCAD_3.
Status: **S0/W0-W2 partially implemented; local creation/cache/trim, GUI drawing-open/template routing, staged plugin block import through both async and legacy sync APIs, W79's complete staged macOS CMake install, W86's relocated-install checks, and W88/W90/W92/W94/W99/W106/W110/W117/W126/W127 current-source preview-cache portability and creation/repeat routing are qualified in focused Release/ASan or offscreen lanes; W137 requalifies W126 preview-cache portability in the dedicated kernel/client/fresh-helper targets, W138 requalifies W127 Modify Offset creation/repeat GUI routing, W139 requalifies U4 persisted option/preview/commit behavior, and W140 requalifies O1 Update from Source; W88 rejects partial hatch-pattern imports, W89 makes dimension-style preview failures noninteractive, W93/W95 classify hatch/dimension previews as display-cache-only, and partial sample content is cleared; W80 fixes demonstrated compound admissions, W85 preserves the explicitly staged contact-control accounting after review corrected W84, W87 bounds strict-query queue allocation by admitted work, W107/W111/W112/W113 requalify budget-core, strict-query, paired-reference graph and cache-budget slices, W114 closes selected local A-cache allocation-failure coverage, W118/W119 add selected A-reference event and component publication allocation-failure coverage, W120 completes the standalone default fault suite in Release across all 29,117 injection/recovery points, W121 closes the local A-reference cross-entry matrix in Release/ASan, W122 closes the local C-check topology/test inventory in Release/ASan, W123 requalifies asynchronous native Trim/Trim Two action routing in Release/ASan, and W124 requalifies local trim file/conversion/undo-reopen coverage in Release/ASan; W96 requalifies A-core, W97 B occurrence/terminal identity, W98 C-search, W101 the full offset selector, W108/W109/W115/W116 expand selected C-check allocation-failure coverage after W100, W128 requalifies E1 guarded entity/factory/clone/transform behavior, W129 requalifies U1 type/INSERT/selection/SVG/menu/info/plugin dispatch, W131 requalifies E2 cached grips/snaps/through edits and transactional transforms, W132 requalifies F1 typed schema/carrier/ownership/resource-limit behavior, W133 requalifies F2 actual DXF/DWG writing and transactional refusal, W134 requalifies F3 independent fixture/version/write/conversion/reopen matrix, W135 requalifies U2 dialog/sheet async property publication, and W136 requalifies U3 async grip drag/cancel/ownership/history behavior in Release/ASan; W81-W83 provide bounded Linux/Windows worker CI with feature-branch triggering; broader A-C budget audit, public low-level storage compatibility, remote cross-platform CI and final acceptance remain open**.
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
| Native geometry-worker path | Real seven-operation protocol4 helper/client, cached source-copy production, same-producer ordinary fallback, plus dialog, sheet, grip, Update, Modify Offset, native Trim and GUI drawing-open/template routing committed/qualified locally | W16-W19 qualify cache portability and creation/repeat; W24-W26 qualify local Trim routing and scoped action faults; W31/W32 cover GUI drawing open/templates. W72 adds staged DXF/DWG plugin block import; W91 routes the legacy synchronous entry through that staged session (it still waits for compatibility) and preserves the async callback entry. W93 classifies hatch-pattern input as display-cache-only. Other import callers, cross-platform and packaged/final acceptance remain mandatory W0-W2 work. |
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
| A-core | Atomic shared admissions, frozen results, sticky status and ownership/lifetime | S0 | candidate `0eb2a0cfa`; `27f9c5239` fixes demonstrated compound admissions; `91dcde3c4` preserves ordered contact-control debit semantics; W107 requalifies budget-core primitives with no new gap; broader audit remains open | `[lc-offsetcurve][budget-core]` |
| A-strict | Strict-query callback/record/order/depth/exception accounting | A-core | candidate `dc016f6e2`; W64 focused strict-budget Release/ASan requalification; W87 fixes depth-ceiling-based queue over-allocation; W111 current-source requalification; broader audit remains open | `lc_parametriccurveintersection_tests.cpp`, named strict-budget cases |
| A-reference | Critical/provider/event accounting and atomic paired graph publication | A-strict | candidate `5c12fbeef`; W65/W112 focused Release/ASan requalification; W118/W119 sweep every allocation through six-pair/four-critical event and component publication paths with bounded ledgers and same-budget recovery; W120 runs all standalone default fault fixtures together in Release; W121 closes the local provider/contact/event/component/terminal/accounting matrix in Release/ASan; remote platform execution remains | `[lc-offsetcurve][reference-accounting]`, `[reference-terminals]`, `[reference-components]`, `[reference-events]`, isolated reference fault selectors |
| A-cache | Bounds/local cache, inherited depth and failed-stage snapshots | A-reference | candidate `dbf895638`; W66/W113 budget requalification; W114 exhaustively injects allocations in `checkCache`, standalone bounds and local-topology APIs with bounded ledgers and same-budget recovery; local A-cache audit complete; cross-platform/final R gates remain | `[lc-offsetcurve][cache-budget-accounting]`, standalone cache-check fault target |
| B | Immutable occurrence/edge-side terminals and issuer lifetime | A-cache | candidate `9bf0546b6`; W97 current-source review and focused Release/ASan requalification found no gap; final R acceptance remains | geometry terminal/occurrence case inventory |
| C-search | Exhaustive certified cuts, owned independent laps, bounded equivalent-state memoization | B | candidate `b6814fb3a`; W98 current-source review and focused Release/ASan requalification found no gap; final R acceptance remains | `lc_offsetpathsearch_tests.cpp`, public cut-search cases |
| C-check | Full public checker, directed coverage, incidence, seam/closure and typed refusal | C-search | candidate `d4a0b081a`; W100 charges the final proof-publication slot; W103/W105/W108/W109/W115/W116 sweep every observed allocation failure over line, forward/reversed two-span cubic, forward/reversed whole-circle seam, both open and closed components of a producer-generated two-component trimmed path, and hidden-cusp endpoint refusal with bounded ledger and same-budget retry; W120 runs all standalone fault fixtures in one Release pass; W122 closes the local topology/test inventory with the full geometry selector in Release/ASan; remote platform/final gates remain | `[lc-offsetcurve][geometry][cache-topology-chain]`, standalone cache-check allocation-fault target |
| W0 | Separate validation runner from product runtime; freeze native-worker schema, caps, deadlines and ownership | C-check | wire/request/reply and initial deadline/authority policy implemented; finish caller/fallback/import ownership freeze before routing | operation/cap/deadline/call-site table |
| W1 | Bounded headless geometry helper and parent transport, registered/packaged in both builds | W0 | helper/client and both build registrations qualified; full app/deployed-package gates remain | `[lc-offsetcurve][worker-protocol]`, `[worker-kernel]`, `[worker-process]`, `[worker-client]` |
| W2 | Deadline/cancel/crash/reap, validated result, UI caller adoption and timeout regression | W1 | fault/edit, dialog/sheet/grip/Update, creation/repeat, native Trim preview/commit and scoped action faults and geometry-entry timeout receipts qualified locally; W81 adds Linux/Windows CMake worker CI; W82 removes PR path filtering and W83 adds scoped feature-branch push triggering; current cross-platform run, import and final gates remain open | `[lc-offsetcurve][worker-client]`, `[worker-fault]`, `[worker-geometry-timeout]`, `[edit][preparation]`, `[creation-worker]`; remaining integration and runner regression |
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
| O2 | Restore Original, same-container dedup, attributes and all entry points | O1 | candidate `0d3fed653`, `4a231d243` plus final fixes; audit/retest | Restore/source-UI TUs |
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
- [ ] S0: finish the source/target reconciliation on the durable tree. Fresh
  baseline, ordinary DWG2007 correction, evidence links and broad offset/failure
  executions and full path/link classification are recorded above. The target-relative
  addendum retains already-upstream #3140; final source/runtime reconciliation remains.
  Do not redo implemented geometry as missing work.
- [ ] A-core through C-check: review/requalify existing code and patch only proven
  accounting, ownership, occurrence or global-topology gaps. No duplicate checker.
- [ ] W0-W2: typed/cached Produce, creation/repeat and local native Trim GUI routing and scoped action faults are qualified;
  a fresh macOS qmake6 full-app/bundle build, adjacent-helper worker smoke and paired
  cache/creation Release-ASan checks are qualified in W55-W57; asynchronous Trim test
  synchronization is corrected and requalified in W58-W61. W79 adds successful full CMake
  installation plus installed-directory worker-client and creation/repeat routing tests.
  Finish other import ownership, Windows/native cross-platform failure coverage and remaining
  caller/final adoption/failure acceptance. Reuse the qualified geometry-entry
  timeout/helper/client/reply modules; do not redo them as absent. Keep Node runner scope
  separate.
- [ ] E/U/G/F/O/T: requalify and integrate the existing entity, UI, creation,
  DXF/DWG, source and trim implementation with any new worker/publication changes.
- [ ] R: finish current-source Release/ASan/qmake6/failure/version/visible rows,
  including Windows, visible GUI acceptance and remaining platform/package acceptance; W79
  now covers a complete staged macOS CMake install; resolve
  or baseline-classify the two order-sensitive command-alias cases in W59. The command
  source/tests are unchanged from `origin/master`, and W62 isolated random-order seeds
  1-3 pass; this does not yet baseline the broad-process interaction. Review scoped
  commits, preserve reproducible evidence and close every required gap.

**Next concrete action on implementation resume:** follow Windows x64 and ARM64
jobs in Actions run `40842072529`, then address any build/runtime failures before
claiming platform parity. W55 now records a fresh full macOS qmake6 app/bundle
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
full randomized fast-lane run no longer fails those offset cases, but reports
five JUnit failure assertions in two `[commands]` alias cases; the isolated
`[commands]` selection passes. Preserve this as an unresolved broader-suite
order/state issue rather than changing unrelated command behavior or waiving the
fast-lane gate.
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
