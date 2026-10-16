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

# Offset Curve (`LC_OffsetCurve`) Implementation Plan — #2961, phases 0–2

Plan **v29**, 2026-10-16 (immediate post-slice reconciliation for product commit `ef3ba756b52796c02b8490caedcaa48b8db69605`). This file is the implementation source of truth.
Every implementation slice ends with a verified commit followed by a committed reconciliation of this plan (§1.4).
Design baseline reviewed: `41678d976` (#3121). Fetched execution base: **b05fb541df651dd9cb55a222dee4563e6127c436**
("Keep default shortcuts independent of translations (#3140)").
Status: **IMPLEMENTATION INCOMPLETE — `.b-check-bounds` is implemented at `ce1ed51924d20a708e8bec059f7f9dd46235a232`; its direct parent is the v24 endpoint-contract checkpoint `a0ce67a3b8746770ce49fb1ff76616c9da3d23fe`. The v25 plan-only reconciliation is verified at `4ca63677c27043004aae45e0fe54d42db48664a5`, and the v26 topology activation checkpoint is `fd4697059572af28c08ecbb532e687af1603ee1c`. The local-topology implementation is committed as `ef3ba756b52796c02b8490caedcaa48b8db69605`, a three-path direct child of v26 with tested file digests. Focused/bounds/adjacent Release and ASan, full Release, DXF/DWG fast Release and ASan, qmake6 app build, skip-identity comparisons and independent implementation review pass as recorded below. This v29 roadmap is the immediate one-file reconciliation for that product commit; the next leaf remains waiting until the direct-child, path-scope, digest and worktree postconditions are verified. The v27 DXF/DWG clarification remains a hard phase-1 read/write gate; new-entity application file support is still incomplete. Entity integration, persistence, UI and phase-2 work remain incomplete.**
Authoritative execution plan: `/Users/dli/dev/LibreCAD/.claude/worktrees/lc-offset-curve-2961/docs/plan_offset_curve_2961.md`.
Execution branch: `codex/offset-curve-2961`. The research checkout retains the v5 seed only; it is not a second live plan.

**Required file support — hard phase-1 gate.** Implement and validate both application-level reading and writing for
DXF and DWG; supporting only one direction, generic SPLINE geometry, or codec-level payloads is not sufficient. A
valid editable offset curve must retain its ordinary SPLINE cache, typed ellipse-offset definition, and editability
after save/open in the same format and conversion in either direction. The application paths are ASCII DXF and native
DWG: independently prove the reader and writer for every applicable version in §3.10, with independently authored
reader fixtures, actual writer-payload inspection, exact-version checks, and edited second-generation reopen. This is
14 application version rows (eight DXF rows, including R12's geometry-only behavior, and six writable DWG rows);
legacy DWG R13/R14 add reader-only rows. Also test both DXF→DWG and DWG→DXF conversion. Binary DXF codec read/write
at R14 and R2018 is an additional low-level obligation; it does not substitute for ASCII DXF application support or
promise that the application export interface emits binary DXF. No 1E-3 or phase-1 release gate may pass with a
missing read or write receipt, a same-writer fixture standing in for independent reader evidence, or a new-type
`knownGap`/`notRun`. R12 explicitly exports/imports geometry without restoring editability. The owning steps are
1E-1 through 1E-3; see the normative matrix and acceptance rules in §3.10.

**How this plan was checked.** v1 was anchored at 7ecebc8fc; v2 had six design/API reviews at 7c4a93fa8. v3 reconciled
the codebase against local `origin/master` at 41678d976. v4 adds the requested DXF/DWG read/write contract and checks
its version matrix against the actual format enum, export mapping and codec version guards. Before implementation,
fetch the intended upstream branch and repeat S0 against that exact commit: verify symbols/type-site census, re-run
the guard compile, validate the new class declaration against current APIs, and review modification, file-format,
UI and phase-2 call sites. v5 reviews completeness, correctness, slice readiness and the commit/reconciliation loop.
v6 checks the executable sub-plans against b05fb541d, tightens numeric/topology and metadata contracts, and makes the
commit/reconciliation evidence auditable. v7 reconciles the tree at `41d83689e`, adds requirement identities and
executable leaf recipes, separates completeness review from correctness review, and corrects raw-import and test
selection hazards. The initial v7 review at `1b9b3cda9` changed documentation only. Subsequent executed slices and
their verification are recorded below; prepared or prototype code is not done merely because it exists. v8 reviews
the current tree at `290d2658fdc2d9bb55da6205ae7c3afaaade6a4a`, reconciles depth-accounting evidence and separates
trusted pair refinement, critical-occurrence refinement and event integration. It strengthens the five requested
tracks: completeness, correctness, executable readiness, verified slice commits and post-commit self-update.
Prototype compilation and measurements remain historical evidence. v9 reconciles the committed event-isolation
slice, captures its final native test receipts and review limitation, and makes `.b-ref-components` the next current
leaf. v10 resolves the remaining component design questions from independent readiness/correctness reviews, adds a
file-scoped executable card and failure policy, and rechecks the five governance tracks: completeness, adversarial
correctness, sub-plan readiness, implementation commits, and post-commit self-update. v11 reconciles the actual
worktree at `c3f091c3f`: the three component-analysis product/test paths have uncommitted changes, and focused
Release attempt 6 is red. It records the blocker and removes the stale "ready" handoff; no product status is
promoted. Future activation cards become active only after predecessor checkpoints are committed, the card is
revalidated against actual HEAD, and the owner receives that exact SHA.
v12 adds an explicit audit for isolated retained event points, which the current edge-only graph representation must
not silently drop; an unsupported singleton requires no-partial refusal or a modeled point component. v13 separates
plan correctness from implementation correctness, makes the completeness audit verify the plan against current code
and format contracts, strengthens executable-readiness and commit/checkpoint postconditions, and reconciles the active
component card with the five paths currently modified. Historical attempt 6 remains preserved but is explicitly
stale for the live source tree. It also narrows the isolated-node audit to own-role events; opposite/cutter-only events
are not vertices of the published own-span graph.
v14 orders the gates explicitly: completeness and plan correctness are proven before implementation; readiness is a
separate go/no-go decision; implementation correctness and final completeness are rechecked on one frozen tree; each
executable leaf gets one implementation commit; and product work pauses until that commit has a separate, audited
plan-only checkpoint. The active in-progress card remains subject to final review/validation; no missing historical
preflight is retroactively represented as having occurred.
v15 makes DXF and DWG read and write independently auditable: ASCII DXF, binary DXF codec, and native DWG lanes are
separated; every application format row has distinct reader and writer expectations; checked-in reader fixtures
cannot be replaced by same-run self-round trips; and the two cross-format conversion directions remain separate
acceptance obligations.
v16 reconciles the component-graph implementation commit `9682d5cac65f30cfd86aa10556678ceb88b1448b` against its
attempt-8 frozen source and JUnit receipts; that plan-only checkpoint is committed at `c6e32c17f`. v17 rechecks the
current code and roadmap contracts. It found duplicated stale ownership of `isEngineLayout`/layout tests in the
already completed `.a` section and `.b-check-layout`, and found that `LC_CurveOffset` has a degree-1/line output
  branch whose reachability and R05 disposition for the ellipse-offset domain had not been resolved. v17 moves future
  cache-check work to the `.b-check` children, makes producer shape a bounded entry probe, and strengthens the evidence needed
for each completeness, correctness, readiness, commit and post-commit update gate. Product code is unchanged.
v18 closes the resulting contract question against actual producer source: the all-straight output branch is intentional
and emits `RS_Line` for one open span or a degree-1 `ClampedOpen RS_Spline` otherwise, so no reachability proof or
plain-fallback exception is needed. Since R05/D16 require editable adoption for a valid one-piece result, the persistent
curve boundary will normalize an `RS_Line` exactly to a degree-1 spline cache; the shared cache layout/checker supports
degree 1 and degree 3 only. The plan now carries degree-specific controls, knots, span extraction, bounds, schema-import
eligibility, writer/readback assertions and the R12 geometry-only case through their owning leaves. The checker's legacy
`maxCubicPieces` name is renamed to `maxCacheSpans` in its own API only; the separate curve-offset producer budget keeps
its cubic-approximation terminology. The existing five gate records remain distinct and the next leaf stays gated on
post-checkpoint source/API verification. No product code is changed.
v19 closes that readiness gate at source HEAD `127231e41`: the complete `createEntities` -> `chainEntity` output path,
degree-1/degree-3 spline data, checker budget callers, absence of a persistent `LC_OffsetCurve` implementation and
existing CMake test membership were rechecked. The direct materialization selector passed 5 cases/226 assertions.
The ready card now records its preflight completeness/plan-correctness result, explicit paths/selectors/cost/evidence/
recovery fields and frozen source base; implementation correctness remains pending until the product diff exists. The
card stays `ready` only while no product source/API changes from that base. No product edit is part of this update.
v20 reconciles the layout-only implementation committed at `37462efc8` against its actual parent `cad1c5b4d`, frozen
three-file diff and SHA-256 identities. The final tree passes 16 focused cases/3,311 assertions in Release and ASan;
94 adjacent cases/754,288 assertions in each; full Release at 3,874 cases, 3,800 passed and 74 unchanged skips;
and DXF/DWG fast lanes in Release and ASan with their established 3/4 skips. Independent read-only review found no
actionable issue. The layout card is complete, but `.b-check-bounds` remains waiting for a fresh post-checkpoint
completeness, plan-correctness and readiness probe. This update contains documentation only and does not claim any
remaining geometry, topology, persistence or UI feature as implemented.
v21 re-probes `.b-check-bounds` at committed source base `6f25975f6ad02ba7cbb15982152905ff6f948fb0`. It replaces
unspecified “parameter-aware matching” with a pairwise set-Hausdorff certificate, local de Casteljau and exact-offset
chord-error bounds, exact-D visible-span clearance, explicit root-fringe/node treatment, a complete pair-witness
relation, explicit caller-owned budget/result-owned geometry lifetimes, and exact error/limit tests. Independent reviews
identified endpoint/fringe and budget-lifetime gaps, then a topological counterexample: a cubic can have the same image
as a line while reversing internally. The card now keeps bounds strictly geometric and expands `.b-check-topology` to
prove derivative-sign monotonicity/injectivity, exact occurrence/edge-end incidence, and an order-preserving,
non-reusing full-domain assignment; it also requires topology to refine coarse witnesses even when setwise distance
passes. The fresh existing layout selector passes 16 cases/3,311 assertions in Release and ASan; code-file digests are
unchanged from v20. The new bounds selector has not been registered and its required count remains a pre-implementation
gate. Independent mathematical and completeness reviews are closed. Findings on clearance consistency, strict
directional endpoint separation, degree-1/constant-span handling, empty-witness refusal, cross-stage budget checks and
incidence identity are incorporated into the proof and test contract. Activation checkpoint `138694560926cbb8e4cb34198fae83dcb6bb4c9a` and its post-commit checks passed; the card is ready at unchanged product base `6f25975f6`. No product code changed.
v22 deepens the five-gate execution contract and reconciles implementation evidence without promoting the uncommitted
bounds diff. Independent source/math review established that a successful interior `SimpleCusp` is necessarily a
zero-degree hidden node: the nonzero cubic term in squared source distance makes the cusp point strictly closer than
the offset distance on one source side, and continuity hides both adjacent offset neighborhoods. Endpoint-coincident
cusp roots are not visible incidences and the current uncertain-membership path refuses them. The card requires a
rigorous representability proof for binary64 public angles and adjacent-double boundary tests; failure to find a
fixture is not proof. It also requires hidden-cusp/no-witness assertions and separate crossing, trim-end,
stationary/free-end and seam identity checks. The 24-case focused Release result is preliminary evidence only; all
final gates and commits remain pending.

v23 applies the independent final-test review to the live `.b-check-bounds` card. The production bounds for a
noncircular reference ellipse previously had only point-sample checks; the generic phrase “interval containment”
did not make a whole-domain oracle an executable requirement. Before the test diff can freeze again, add an
independent outward interval or analytic range oracle that encloses the noncircular reference curve over each full
tested subdomain and encloses its deviation from the endpoint chord over that same domain. Cover wrapped/reversed,
near-cusp and extreme-scale cases; subdivisions must cover the entire domain with outward bounds. Sampling stays a
secondary diagnostic and cannot establish either enclosure. This is a test-only refinement: do not change production
API or the three-file ownership. The currently passing 25-case/12,756-assertion focused runs are preliminary because
this oracle gap remains open. After the separate v23 plan-only amendment checkpoint is verified, Gibbs may resume
only this owned test path; re-freeze and repeat all reviews and required validation before the product commit.

v24 applies the next independent review to the v23 oracle. In addition to enclosing the noncircular curve over
each complete parameter range, the test must compare independent outward endpoint enclosures at both ends against
all four production chord fields (`startX`, `startY`, `endX`, `endY`). These are separately consumed by the chord
Hausdorff calculation; checking only the whole-range `x/y` boxes and deviation could miss an endpoint-specific
underbound. The assertions are test-only and remain in the existing test file. Readiness stays waiting until the
plan-only amendment is verified, the assertions pass for every wrapped/reversed, near-cusp and extreme-scale range,
and independent review confirms all endpoint enclosures are outward.

The v27 and v28 drafts were not committed and are folded into v29. v29 makes the five requested execution gates
independently recordable, distinguishes completeness and planned-contract correctness from implementation correctness,
records readiness invalidation conditions, and reconciles the active topology slice to its rebuilt focused/event tests,
independent review and final validation receipts. It retains the hard DXF/DWG read/write gate. The plan-only commit
must follow the validated product commit and may not be mixed into it.

Appendix E lists what changed and why. This plan remains a proposal for a new persistent `LC_OffsetCurve`; the
current engine's internal `LC_CurveOffset` is not that entity. **Line numbers and type-site counts are historical
search aids only: recensus at S0 and read current code before editing.**

**Terms.**
- *Original*: the copy of the ellipse that a curve embeds.
- *Source*: an ellipse entity in the drawing.
- *Cache*: the curve's spline data, which is the offset engine's output.
- *Whole*: the original is a whole ellipse (angle1 = angle2 = 0), never decided by `isClosed()`.
- *Top-level*: the entity's parent is a document (a drawing, or a block open for editing).

**Supporting material** (the local research folder `~/.claude/plans/offset-curve-type-2961/`, not in the repository):
- `offset-curve-report.md`: the research (why, measurements, prior art).
- `design/recommended.md` and `redteam/*.md`: the design and its corrections.
- `evidence/digest/*.md`: per-topic evidence.
- `prototypes/`: scratch code that ran against master beccd8c14. Use it as reference, never paste it:
  - `redteam_semantics_work/s2class.h`: the prototype class.
  - `redteam_semantics_work/rt_check.cpp`: the closed-form check (`REPL`).
  - `redteam_integrity_work/rt_offsetcurve.{h,cpp}` and `patched/hunks.diff`: the file round trip.
  - `type_shape_operation_matrix/patched/sites.diff` and `tsom_probe.cpp`: the type sites and parity rows.
  - `design_entity_probe/dep.cpp`: offset-again chains and arc grips.
- `prototypes/plan_review_2026-10-14/`:
  - `corrected.cpp`: the class declaration of §3.2, compiled clean with `-Wall -Wextra -Woverloaded-virtual`.
  - `guard_skeleton.cpp`: the 0B guard, compiled clean.
  - `flags.rsp`: the compile flags used for those checks.

These artifacts are optional background. A required slice must be implementable from this plan and the checked-out
source/tests; missing private prototypes are not a blocker. Recreate a probe from its stated geometry and current APIs
instead of copying an old patch or making the implementation depend on an untracked home-directory file.

---

## 0. Status (source of truth for the plan-implementer)

Execution states: `pending`, `in progress`, `validated`, `committed — reconciliation pending`, `partial — <child ids>`,
`done — <implementation sha>, <evidence id>, <date>`, `needs revision`, `needs decision`, `blocked`, and
`deferred — user publication action`. Readiness is separate (§0.3): `ready`, `conditional`, or `waiting`.
A commit alone does not mean done; verification, review and the post-commit plan checkpoint are required.

| Step | Title | PR | Depends on | Size (LOC, product + tests) | Status |
|---|---|---|---|---|---|
| S0 | Worktree, tracked plan, build baseline and evidence (documentation commit) | — | — | 0 product | done — de30f1d45, S0 reports/card, 2026-10-15 |
| 0A | Trim Two must not replace an owned child | — | — | — | done on baseline by #3113 (`609ca81ae`); verify current tests at S0, then add the new-type mapping/test in 1D |
| 0B-1 | Guard skeleton: `rowOf` table, pragmas, `static_assert`, kinds-only walk; registration; CI path filter; shadow-`rs.h` checks | B | S0 | 250 | done — d6de53d3c, 0B-1 evidence, 2026-10-15 |
| 0B-2 | Entity factory header; clone and INSERT checks; pin tests | B | 0B-1 | 500 | done — 5bfb3c8f0, 0B-2 reports/card, 2026-10-15 |
| 0B-3 | DXF 2018 and DWG 2018 round trips, `reloadsAs`, `knownGaps`/`notRun` | B | 0B-2 | 200 | done — 0e3c6b342, 0B-3 reports/card; one pinned baseline generated-MLEADER DWG gap, 2026-10-15 |
| 1A-1 | Closed forms, normalization, tolerance helpers and cache validation | C | S0 | 900 | partial — .a/.b-budget/.b-progress/.b-ref-frames/.b-ref-events-core/.b-ref-rate/.b-ref-events-critical/.b-ref-events-accounting/.b-ref-events-refine/.b-ref-critical-refine/.b-ref-providers/.b-ref-contacts/.b-ref-events-isolate/.b-ref-components/.b-check-layout/.b-check-bounds done; .b-check-topology-local/.b-check-topology-chain/.c pending |
| 1A-1.b-ref-components | Exact visibility and incidence-preserving components | C | .b-ref-events-isolate | 900 | done — `9682d5cac65f30cfd86aa10556678ceb88b1448b`, attempt-8 evidence, 2026-10-16 |
| 1A-2 | Enumerator; class core and validated adoption factory; clone, `toPlainSpline`, normalisation, stamp, transforms; `writeEntity` case; INSERT capability; 0B row; interim Edit Properties guard | C | 0B-3, 1A-1 | 900 | pending |
| 1A-3 | Grips, `moveRef`, snaps | C | 1A-2 | 550 | pending |
| 1B | Run limits, refusal enum, `adopt`/`create`, mutators, engine-run counter | C | 1A-3 | 900 | pending |
| 1C-1 | `asOffsetCurve`; wrap on Offset; kept-plain outcome; Explode; non-uniform scale substitution (document, plugin) | C | 1B | 600 | pending |
| 1C-2 | Offset again | C | 1C-1 | 600 | pending |
| 1D | Remaining type sites | C | 1A-2 | 600 | pending |
| 1E-1 | Shared schema; DXF XDATA and DWG EED read/write; import adoption; 0B row update | C | 1B, 1C-1, 1D | 1000 | pending |
| 1E-2 | DXF/DWG metadata safety, geometry fallback, fixtures, copy and paste | C | 1E-1 | 700 | pending |
| 1E-3 | Complete version matrix; bidirectional conversion; edit/save/reopen; file verification | C | 1E-2, 0B-3 | 500 | pending |
| 1G-1 | Type name and match descriptor, Quick Info, context menus, Select Window, menu activator | D | 1C-1, 1D | 600 | pending |
| 1G-2 | Refusal texts, busy cursor, property provider, refusal-aware property sheet | D | 1G-1 | 700 | pending |
| 1G-3 | Edit Properties case and widget (replaces the interim guard) | D | 1G-2 | 700 | pending |
| 1H | Grip drag in the default action | D | 1G-2 | 500 | pending |
| 1F | Option "Editable offset curve" (default on) and messages | D | 1C-2, 1G-3, 1H | 350 | pending |
| 1I-a | Release checks run by the agent | D | 1E-3, 1F | 0 | pending |
| 1I-u | User: push, CI, post on #2961, open PRs B–D | — | 1I-a | — | deferred — user action |
| 2A | Action "Update from Source" | E | 1I-a | 500 | pending |
| 2B | Action "Restore Original" | E | 2A | 300 | pending |
| 2C | Registration, menus, links | E | 2A, 2B | 200 | pending |
| 2D-a | Release checks run by the agent | E | 2C | 0 | pending |
| 2D-u | User: push, CI, PR E | — | 2D-a | — | deferred — user action |

The LOC figures are legacy rough estimates, not readiness evidence. Re-estimate after S0 and after the strengthened
geometry/file work is qualified; v2's calendar estimates are obsolete. Split a large step into named coherent children
before implementation rather than forcing work into an arbitrary line or session budget.

### 0.1 Upstream reconciliation (design at 41678d976; execution at b05fb541d)

These changes are already part of the reviewed baseline; do not implement them again. The new entity and its tests
must integrate with their current behavior:

| Commit | Landed behavior | Plan consequence |
|---|---|---|
| #3117 `cc21a1d85` | Elliptic-arc offset trimming now finds all corners and retains visible portions; coverage includes multi-chain arcs and near-critical whole ellipses. | Replace old topology/failure-band expectations in 1B/1C-2. Re-run geometric acceptance measurements at S0; do not pin engine status or spline control counts. |
| #3112 `feb2df528` | Elliptic-arc nearest-point search considers interior critical points and endpoints. | Remove the old nearest-point defect claim. §3.5 still needs its own distance-to-offset calculation; source-ellipse nearest-point distance is not the validation metric. |
| #3113 `609ca81ae` | Trim preserves source layer and refuses replacing generated/owned parts; Trim Two and spline chord mapping have tests. | 0A is complete. 1D must add `EntityOffsetCurve` to the landed single-Trim child-to-parent mapping and test the new type against Trim and Trim Two. |
| #3119 `b335200dd` | DXF XDATA application names are registered, and polyline XDATA is preserved. | Remove APPID registration as a planned production change. Keep an APPID regression assertion for the new `LibreCad` group. |
| #3116 `fe2e5150f` | DWG EED writing accepts binary XDATA and preserves 16-bit group-code payloads. | Include binary XDATA in DXF→DWG round trips; retain a focused regression because the offset metadata shares the XDATA pipeline. |
| #3114 `be8e340b1` | DWG save preserves supported XDATA layer references and repairs free draw-order keys. | Preserve foreign application groups byte/value-wise through offset metadata merge; only filter items inside the owned schema sub-list. Keep reference-bearing foreign XDATA in the round-trip fixture. |
| #3109 `920cf21a5` | Leaders update after document loading completes. | Coverage rows involving `DimLeader` must exercise `onLoadingCompleted()` and record any remaining gap rather than assuming import leaves it empty. |
| #3121 `41678d976` | Native DWG output received interoperability fixes, including SHAPE-related paths. | Treat this as the DWG writer baseline; run existing DWG smoke/verification lanes for the new ordinary-SPLINE carrier. No SHAPE-specific implementation belongs in this plan. |

Execution reconciliation on 2026-10-15: fetching upstream added only #3140 after the reviewed snapshot. Its changes
touch `CMakeLists.txt`, `lc_actionfactory.cpp` and `lc_docklayout_tests.cpp`; the enum and file codecs are unchanged.
The clean execution checkout is at b05fb541d; use its current source-registration/action-factory positions. After a
necessary rebase, rebuild, repeat affected baseline checks, and rerun the last finished step's focused tag.

### 0.2 Release gates (the user's, not the agent's)

- **PR C must include the new-type Trim mapping and regression tests** (1D); PR D inherits that prerequisite. The upstream ownership guard is in place,
  but single Trim's spline-chord-to-parent mapping is type-keyed and does not automatically recognize the new subtype.
- **PR C can merge alone.** 1A-2's interim Edit Properties guard (U2) means a curve that arrives from a file does not
  leave a stuck action. 1G-3 removes the guard. PR C must include the complete DXF/DWG file contract and passing
  1E-1 through 1E-3 checks, with distinct evidence for DXF read, DXF write, DWG read and DWG write; the new RTTI
  writer case or a same-implementation round trip alone does not complete file support.
- **Before PR D**, resolve grip decision D0 and post the two-sentence answer to sand1024's DXF question from #2523 on
  #2961. Do not treat the provisional grip default as user-approved.

### 0.3 Sub-plans and readiness

Implementation proceeds in this default order:
`S0 → 0B-1 → 0B-2 → 0B-3 → 1A-1 → 1A-2 → 1A-3 → 1B → 1C-1 → 1C-2 → 1D → 1E-1 → 1E-2 → 1E-3 →
1G-1 → 1G-2 → 1G-3 → 1H → 1F → 1I-a → 2A → 2B → 2C → 2D-a`.
0A is an upstream prerequisite verified during S0. User publication rows do not block local implementation or phase 2.
Use an already documented default, including D0, while a publication discussion remains pending.

**A slice is ready only when:** its required predecessors are done; its current symbols/files and baseline are known;
the requirement coverage is complete; the plan's contract and repository/spec assumptions have been checked and are
internally consistent; its behavioral contract and refusal/fallback policy are explicit; its validation commands,
expected test families and nonzero case-count checks are recorded; and no unresolved design choice changes its success
criteria. Existing exact counts come from S0; count new cases at registration and record the result before commit
instead of inventing totals before writing tests. Readiness is not inferred from a LOC estimate or an old prototype.
Recheck it after every commit, rebase or overlapping change.

**Readiness decision order (mandatory for every new leaf):**

1. Freeze the candidate base and inspect current declarations, definitions, callers, tests, build registrations,
   format/version guards and relevant specifications. Record the exact source revision and file paths. Search results,
   old cards, prototypes and green builds are discovery evidence, not proof of the contract.
2. Close **completeness** before coding: trace every applicable R01-R15 row through the complete behavior path, from
   public entry point and producer/input representation through consumers, persistence/conversion variants, build
   targets and registered positive, boundary, refusal, no-op and fallback tests. Name each exclusion and its owning
   leaf. Show that every promised result is actually asserted by a reachable test; a selector count or coverage
   percentage alone is not a trace.
3. Close **plan correctness** before coding: check each proposed API, invariant, source/spec/version claim, numeric
   threshold, resource debit, dependency edge, fallback and test oracle against current code and authoritative
   evidence. Check that adjacent sections and all format rows agree, and that a DAG audit finds no missing node, stale
   predecessor, cycle or aggregate mistaken for an executable leaf. Correct the roadmap first when facts conflict.
4. Define the executable sub-plan: explicit file ownership; an ordered, bounded task list; one observable success and
   refusal contract; resource, lifetime, transaction and compatibility invariants; literal build/test commands;
   selectors with a nonzero post-registration case count; independent oracles; failure/recovery policy; evidence
   paths; and one buildable commit boundary. Any unresolved investigation must state its bounded probe, outcomes,
   decision rule, stop condition and how each outcome changes scope. No required design decision may be deferred into
   implementation.
5. Record `ready` only when steps 1-4 close and all real predecessors plus their plan checkpoints are committed at
   the actual base. Otherwise record `conditional` with the exact closure test or `waiting` with the exact predecessor.
   Recompute readiness after every commit, rebase, API/toolchain change or overlapping edit. If a probe changes the
   contract, return to steps 2-5 and revise affected descendants before product edits continue.

After implementation, repeat completeness against the actual diff, recheck plan correctness wherever observed behavior
or evidence touched an assumption, and run implementation correctness review on one frozen source identity. A
preflight review cannot approve code that did not yet exist, and a final review cannot repair an incomplete plan by
silently redefining acceptance. Each review record must say `pass`, `fail`, or `not applicable` with justification;
"reviewed", a green build, and an unexplained assertion total are not gate results.

Readiness is a gate over the current tree, not a label attached to a plan row. A leaf with edited product files is
`in progress`, even when the previous plan checkpoint called it `ready`; a failed required test makes it not
validation-ready; a passing build alone cannot advance it. Before implementation, the activation card must name the
actual HEAD and scoped diff, one owner per writable path, concrete positive and negative outcomes, current test
registration, resource/ownership/error invariants, and the exact next action for each known blocker. If a new
counterexample changes the contract, pause implementation at a safe boundary and revise this card plus affected
downstream contracts before continuing. Do not silently reduce acceptance criteria, recategorize required success as
optional, or start a dependent leaf to work around a blocker.

Before editing a slice, put its concrete card below the relevant step heading, using this template. Keep the card in
this file; build logs and machine-generated artifacts live under ignored `build/evidence/<slice-id>/`.

```yaml
slice: <step-or-child-id>
state: pending|in progress|validated|committed — reconciliation pending|done
readiness: ready|conditional|waiting
readiness_decision: <ordered gate results; reason for ready/conditional/waiting; exact closure test if conditional>
completeness_gate: PASS|FAIL
plan_correctness_gate: PASS|FAIL
implementation_correctness_gate: PASS|FAIL|PENDING
readiness_gate: READY|CONDITIONAL|WAITING
implementation_commit_gate: PASS|PENDING|BLOCKED
self_update_gate: PASS|PENDING|BLOCKED
base_sha: <actual-checkout-sha>
owner: <one-agent-and-nonoverlapping-file-scope>
dependencies: <done-ids-and-implementation-shas>
requirements: <R01-R15-identities-from-section-0.4-and-step-specific-contracts>
api_preflight: <current-symbols-and-invariants-confirmed-in-the-scoped-source>
contract: <observable-success-and-refusal-behavior>
invariants: <proof-lifetime-resource-transaction-properties-that-must-not-change>
blockers: <exact unresolved prerequisite or none; conditional readiness must name its closure test>
files: <explicit-product-test-build-documentation-paths>
tasks: <ordered-small-changes-with-one-reviewable-purpose>
validation: <commands-case-counts-oracles-and-adjacent-regressions>
selectors: <registered-test-identities-and-nonzero-counts-in-each-required-target>
cost_model: <charged-units-admission-order-depth-history-and-limit-boundary-tests>
evidence: <tested-tree-command-exit-count-skips-and-report-identities>
frozen_identity: <base-sha-plus-scoped-diff-and-file-digests-matching-final-validation>
recovery: <how-to-preserve-and-resume-uncommitted-work>
completeness_review: <preflight trace and exclusions; then final diff-to-requirement/caller/format/build/test/evidence reconciliation; reviewer/tree identities and gaps>
plan_correctness_review: <preflight current API/spec/source validation and contract/dependency consistency; final recheck after implementation evidence; findings and resolution>
correctness_review: <independent reviewer/tree identity; implementation proof/invariant/adversarial challenge; independent oracle; findings and retests>
implementation_sha: pending
product_commit_audit: <exact committed path list, tested-tree/digest match, hook result, actual implementation SHA>
plan_checkpoint_state: pending|committed — postconditions verified
checkpoint_postconditions: <plan-only path scope; product SHA ancestor; digest match; diff-check; status accounted; next card remains gated>
next: <candidate-next-slice-and-entry-check>
```

The detailed step sections remain the technical specification. The following sub-plans define the commit boundaries
and the evidence needed to activate them. Only the explicitly recorded committed leaves are done; prepared code is
not validation evidence. Aggregates are never executable cards or extra implementation commits.

| Step | Initial readiness / entry check | Commit-sized sub-plan and exit evidence |
|---|---|---|
| S0 | Done at de30f1d45 | Clean execution worktree, tracked roadmap, toolchain/builds/baseline reports, format matrix and census recorded below; post-commit checkpoint records this SHA. |
| 0A | Existing upstream code | Verify #3113 ancestry and current trim tests. Record evidence in S0; only the new-type mapping belongs to 1D. |
| 0B-1 | Done at d6de53d3c | Exhaustive 50/27/23 census and both negative shadow compiles passed; Release tag/lanes verified. |
| 0B-2 | Done at 5bfb3c8f0 | 27 valid fixtures, clone/sole+mixed INSERT and both pins; 7 cases/910 assertions pass full/DXF-fast/ASan; no gaps. |
| 0B-3 | Done at 0e3c6b342 | Real 27-type DXF/DWG walks and both DXF pins; 10 cases/1335 assertions full/DXF-fast/ASan, full suite/lanes pass. Generated-MLEADER DWG fallback is pinned, no notRun rows. |
| 1A-1.a | Done at a4690db08 | Normalization/local-frame/closed forms, robust source distance and tolerance helpers; 17 cases/2538 assertions pass Release/ASan, existing regressions and both fast lanes pass. |
| 1A-1.b | Aggregate; budget/progress/reference/layout/bounds done, local/global topology pending | Parent-only progress; layout and bounds commits add no topology claim. Local-topology analysis is explicitly diagnostic only; no public `checkCache` success exists until the ordered-chain child passes. No separate aggregate implementation commit. |
| 1A-1.b-check-topology-local | Done — implementation `ef3ba756b52796c02b8490caedcaa48b8db69605` plus immediate v29 plan-only reconciliation; post-commit audit is required before successor work | Add bounded local candidate/reference monotonicity certificates, preserve exact edge/occurrence/node identity, and refine only through the shared geometric pair-proof helper. Expose local proof status only; no public cache-validity result. Three geometry files, 20 focused cases, independent rational derivative oracle, all required gates and two scoped commits. |
| 1A-1.b-check-topology-chain | Waiting for a fresh actual-source/API/completeness/readiness probe after the local implementation and v29 checkpoint | Consume the committed local proof relation; add one-component ordered chain, bidirectional coverage, closure/event transitions, bounded uniqueness decision and public `checkCache` status mapping. No implementation begins from this card until its later readiness audit closes. |
| 1A-1.b-budget | Done at 10c6bff46 | Checked shared work, bounded two-axis/range indices, nine budget cases Release/ASan, all offset/full/fast-lane gates pass; findings/evidence below. |
| 1A-1.b-progress | Done at fb70681e2 | Reachable hang fixed; four watchdog-protected cases/80 assertions Release/ASan, old probe terminates, broad gates pass. |
| 1A-1.b-ref | Aggregate; foundations/refinements/providers/contacts/isolation done | Complete — `.b-ref-components` committed at `9682d5cac65f30cfd86aa10556678ceb88b1448b` with attempt-8 evidence. Bounds/topology checking and parent `.b` remain incomplete. |
| 1A-1.b-ref-frames | Done at 26d107ccf | Seven new cases, independent 113-bit enclosures, shared budget and all Release/ASan/full/fast-lane gates recorded below. |
| 1A-1.b-ref-events | Aggregate; isolation and components committed | Complete with `.b-ref-components` at `9682d5cac65f30cfd86aa10556678ceb88b1448b` and its separate plan checkpoint; no separate aggregate implementation commit or partial event-success API. |
| 1A-1.b-ref-events-core | Done at 834a46a88 | Opt-in strict proofs/accounting; 14 registered cases, Release/ASan/probes/full/fast-lane evidence below. No ellipse graph/adoption claim. |
| 1A-1.b-ref-rate | Done at bcd6c5a4f | Six cases/505703 assertions Release/ASan; continuous exact-sweep rate and all jet/products, independent oracle and broad/fast checks pass. |
| 1A-1.b-ref-events-critical | Done at 943406abe | Nine cases/268244 assertions Release/ASan; immutable adapter, exact cusp/axis classification, proven regions and joint join/seam exclusions; all broad/fast gates pass. |
| 1A-1.b-ref-events-accounting | Done at 290d2658f | Five cases/153 assertions Release/ASan; actual depth on every exit, unchanged decisions/counters; old core/broad/fast gates pass. |
| 1A-1.b-ref-events-refine | Done at f4f87f48e | Fourteen cases/490 assertions Release/ASan; owned immutable provider, opaque handles, retained proof/history, native broad/fast gates pass. |
| 1A-1.b-ref-critical-refine | Done at72eb5f953 | Native red1/11 retained; final Release/ASan21/16424, original V6/P23/deepest0, legacy/core/broad/full/fast gates pass. Retained exact critical state and bounded contraction, one-sided join repair with whole-box/cap tests. |
| 1A-1.b-ref-providers | Done at9c27e2980 | Owned immutable offset/cutter composite, true-pi bounds/injectivity, both-hook remapping and exact collapsed-opposite exclusion;16/299294 Release/ASan plus critical/core/broad/full/fast gates pass. No event graph. |
| 1A-1.b-ref-contacts | Done at92ca5e445 | Additive contact factory, immutable whole-box support/radial/critical-gap proofs and diagnostic intervals;17/4383 Release/ASan plus old97/586799 and all broad/full/fast gates pass, legacy behavior retained. |
| 1A-1.b-ref-events-isolate | Done at dff307668 | Owned combined-event result; strict anchored roots, critical/free/seam occurrences, proved role/parameter order, exact-only same-role identity, tol/16 rounded world-node proof, and cumulative shared-budget refusal. At this checkpoint, multi-pair shared-own success was not separately fixture-pinned; `.b-ref-components` later added the successful shared-occurrence proof. Tangent fixture remains a bounded no-partial refusal at the strict-query depth cap, not an identity/order success. |
| 1A-1.b-check | Aggregate; `.b-ref`, layout and bounds complete; topology children pending | `.b-check-bounds` is committed at `ce1ed5192`; v25 checkpoint `4ca63677c` is verified. v26 splits topology into local injectivity certificates and the global ordered-coverage/public-validator leaf. Only `.b-check-topology-local` is next; `.b-check-topology-chain` waits for the local implementation commit, its separate plan checkpoint and a fresh actual-source/API probe. Bounds and local witnesses never imply complete topology or public validity. |
| 1A-1.c | Waiting for .b | Add adversarial and current-engine corpus checks, failure budgets and performance measurements. Record acceptance/refusal results; endpoints and midpoint checks alone cannot close this parent step. |
| 1A-2 | Waiting for 0B-3 and all 1A-1 children | Add type, core class, complete consistency stamp, transforms, clone/INSERT/write dispatch and interim UI guard. Compile CMake and update qmake lists; run copy/ownership and enum coverage. |
| 1A-3 | Waiting for 1A-2 | Implement definition-backed grips/snaps with runtime consistency checks and separate cache closure. Test closed results from arc sources and stale cache fallback. |
| 1B | Waiting for 1A-3 | Implement input/refusal checks, one-run mutation, staged cache adoption and resource limits. Verify release-mode refusals preserve state; remove regeneration stubs. |
| 1C-1 | Waiting for 1B | Wrap eligible tool output; preserve plain-result diagnostics, Explode and unequal-scale substitutions. Test preview/commit parity, layer/pen, ownership and undo. |
| 1C-2 | Waiting for 1C-1 | Implement non-nesting offset-again and bounded rebase/fallback. Required zero-sum fallback and near-critical success tests replace the optional old failure-band search. |
| 1D | Waiting for 1A-2; run after 1C-2 by default | Resolve every current type site, including Trim's chord mapping. Commit the disposition census and parity tests; verify owned chords are never replaced or double-deleted. |
| 1E-1.a | Waiting for 1B, 1C-1 and 1D | Implement bounded typed schema parsing/merging, major-axis vector representation and stale-schema removal. Test parser behavior before changing filter callbacks. |
| 1E-1.b | Waiting for .a | Preflight complete spline/metadata before every writer return; integrate both writers and R12 downgrade; inspect actual payloads and existing transactional failure propagation. |
| 1E-1.c | Waiting for .b | Integrate common import adoption and codec regressions; verify both formats restore editable curves without regeneration. Update both 0B round-trip expectations. |
| 1E-2 | Aggregate; waiting for all 1E-1 children | Execute `.safety → .fixtures → .copy` (§0.6), with malformed/raw carrier, unknown-tail, binary/provenance and copy/paste tests. |
| 1E-3 | Aggregate; waiting for 1E-2 and 0B-3 | Execute `.matrix → .conversion` (§0.6); all 14 application rows, legacy DWG R13/R14 input-only, independent degree-1/degree-3 reader fixtures, binary DXF, both conversion directions and edited reopen; nonzero DWG coverage. |
| 1G-1 | Waiting for 1C-1 and 1D | Commit type name and match descriptor together, then associated information/menu/selection cases. Check registry completeness and spline-tool exclusion. |
| 1G-2.a | Waiting for 1G-1 | Add Changed/Unchanged/Refused outcomes and one-original/latest-pending-clone transactions to generic properties, with rapid-edit/ownership regression tests. Existing provider defaults remain compatible. |
| 1G-2.b | Waiting for .a | Add offset provider, messages and busy-cursor guard; use runtime validity checks and existing UI conventions. |
| 1G-2.c | Waiting for .b | Exercise both selection/refusal orders, mixed success, no-op edits, delayed commit, cancel and undo/redo; broaden property-sheet regression coverage. |
| 1G-3 | Waiting for all 1G-2 children | Add dialog/widget and replace the interim guard. Verify success, refusal, cancellation and clone lifetime through the real action. |
| 1H | Waiting for 1G-2 | Integrate grip preview and commit with no engine run during mouse motion. Test both completion modes, cancellation, refusals and cleanup. |
| 1F | Waiting for 1C-2, 1G-3 and 1H | Expose the default-on option and kept-plain messages; preserve constructor/test defaults and option-off behavior. |
| 1I-a | Waiting for 1E-3 and 1F | Audit phase-1 completeness; run required release/build/UI checks; commit final tests/docs or a validation-only checkpoint when no code changes are needed. |
| 2A | Waiting for 1I-a | Add Update from Source with document ownership, consistency, no-op and one-undo-step checks. Reject generated source children. |
| 2B | Waiting for 2A | Add Restore Original using normalized geometry and same-container live-entity deduplication. Verify no duplicate across a multi-curve request. |
| 2C | Waiting for 2A and 2B | Register actions, context entries and property links; confirm factory construction and empty/wrong selection handling. |
| 2D-a | Waiting for 2C | Verify source actions, persistence in both formats, undo/redo and final release evidence; commit and reconcile the final roadmap. |

Predeclared leaf children are real commit slices; the parent is `partial` until every required child is done.
`1A-1.b` is the aggregate of `.b-budget`, `.b-progress`, `.b-ref` and `.b-check`, not an extra implementation commit. The children of
`.b-ref`, `.b-check`, 1E-2 and 1E-3 are specified in §0.6. Any other step
may be split before editing by adding named children, dependencies and validation to its card. Keep coherent changes
such as a new type name and its required registry entry in one buildable commit.

### 0.4 Completeness and correctness coverage

| ID | Requirement | Owning steps | Required oracle |
|---|---|---|---|
| R01 | Finite 2D domain and unambiguous whole/arc normalization | 1A-1, 1B | Tiny/zero/full-turn, circle, extreme ratio/scale and non-finite cases; no accidental whole ellipse |
| R02 | Bounded geometry and topology validation | 1A-1, 1E-1 | Conservative bounds, hostile controls, complete ordered component coverage, explicit limit/ambiguity refusal |
| R03 | Runtime consistency and safe edit state | 1A-2, 1B, 1G, 1H | Mutated mode/flags/knots/controls cannot retain active definition; refused edits preserve geometry/history |
| R04 | Clone, transforms, ownership and INSERT | 1A-2, 1C, 1D | Independent children, attributes, reflection/scale parity, unsupported transform demotion, ASan |
| R05 | Creation, repeated offset and fallback | 1B, 1C, 1F | Option-off parity, one-piece adoption, multi-piece/zero-sum fallback, bounded attempts and diagnostics |
| R06 | Complete type integration | 0B, 1D, 1G | Current site census and dispositions; no silent save/drop/INSERT gap for the new type |
| R07 | DXF and DWG application read/write | 1E-1 through 1E-3 | Separate application reader and writer receipts for all 14 version rows; independent ASCII-DXF and native-DWG reader fixtures; binary DXF codec read/write at R14/R2018 as a separate low-level lane; both conversion directions and edited reopen; R12 and legacy-DWG reader-only exceptions are explicit |
| R08 | Metadata compatibility and resource limits | 1E-1, 1E-2 | APPID, typed tails/bytes/depth, invalid/unknown schema policy and stale metadata removal |
| R09 | UI edit and selection behavior | 1G, 1H, 1F | Dialog/sheet/drag success/refusal/cancel/no-op, mixed selection and one-step undo |
| R10 | Source actions | 2A through 2D | Live document source, no stored dependency, restore/deduplication and both-format persistence |
| R11 | Build and regression portability | 0B, 1I, 2D | CMake/qmake6, focused/adjacent tests, GCC/MSVC coverage where CI is available |
| R12 | Responsiveness and bounded work | 1A-1, 1B, 1C-2, 1H | Shared nonoverflowing work caps, measured costs; no motion-time regeneration/validation |
| R13 | Durable implementation progress | Every slice | Reviewed implementation commit, tested-tree evidence, plan checkpoint and next ready card |
| R14 | Raw file geometry is not laundered into editability | 1E-1.c, 1E-2.safety | Raw Z/normal/null/count/knot/weight/fit checks before XY projection or geometry repair |
| R15 | Save refusal preserves the target and document | 1E-1.b, 1E-2.safety | Existing-target bytes unchanged, absent target still absent, no leaked stage or mutated document |

Before phase 1 or the whole plan is marked complete, audit this matrix against the committed tree. A product stub,
unimplemented required callback, skipped format, missing qmake entry, unresolved correctness finding or missing
post-commit reconciliation leaves the corresponding row incomplete. Separate user publication tasks and explicitly
out-of-scope features from implementation completion.

For every slice, completeness review must produce a trace from each applicable `Rxx` to: concrete implementation
paths and call sites; every relevant representation/format/build target; tests for success, refusal, no-op and fallback;
and a durable evidence identity. A requirement with no owning leaf or with only an unregistered/unreachable test is
uncovered. A feature is not complete because a placeholder, diagnostic counter or raw-preservation path exists when
the requirement promises semantic behavior. Re-scan the current type/caller/serialization surface when a new subtype
or persistence path changes shared dispatch.

### 0.5 Current readiness and implementation handoff

S0 is complete at `de30f1d45`. Release/ASan configurations, all three target builds, Release full
and fast-lane baselines, and focused ASan trim/ellipse checks passed, with existing skips recorded in its card.
0B-1 is committed at d6de53d3c, 0B-2 at 5bfb3c8f0, 1A-1.a at a4690db08, and `.b-budget` at 10c6bff46; their
evidence is below. Budget verification was isolated from 0B-3 and includes Release/ASan, full suite and both fast
lanes. 0B-3 is now committed at 0e3c6b342 after exact restoration and complete verification. Its old
`build/evidence/0B-3/pending-roundtrip.patch` is historical evidence, **not a patch to apply again**. All 27 DXF rows
retain type; 26 DWG rows do, with generated MLEADER's documented LEADER+MTEXT fallback pinned explicitly.
`.b-progress` is now committed at fb70681e2, with its confirmed hang safely refusing and ordinary rounded growth
preserved. `.b-ref-frames` is committed at 26d107ccf with conservative point/jet bounds and shared checked budgets.
`.b-ref-events-core` is committed at 834a46a88 with exhaustive proof-only mode and unchanged default solver behavior.
Its C1 review exposed the frame's endpoint-only seam alias; `.b-ref-rate` is now committed at bcd6c5a4f with a continuous
real-period/input-endpoint mapping, rate/angle accessors and qualified jet/product factors. `.b-ref-events-critical`
is now committed at 943406abe with exact thresholds/membership, complete critical preparation and cached truthful
injectivity/exclusion proofs. Read-only preflight found two real core prerequisites for isolation: observed adaptive
depth was absent from stats, and physical-node contraction is private. Accounting is now committed at290d2658f:
three observed depth fields and max helper, every exit covered without solver/counter changes. Owned-session
refinement is now verified/committed atf4f87f48e: immutable provider/options, issuer-bound handles, frozen original
provenance, cached images, persistent root history and bounded contraction; fourteen cases/490 assertions pass
Release/ASan and all required broad/full/fast gates. Critical refinement is now verified/committed at72eb5f953:
factory scalar/angle provenance and immutable raw geometry survive, opaque occurrence handles share bounded
convergent scalar/inverse/image refinement, issuing-budget identity/history cannot reset, and depth reflects entered
work. Native red V6/P23/deepest1 now passes with deepest0; final21 cases/16424 assertions pass Release/ASan and all
broad/full/fast gates. Native validation also exposed and repaired a legacy one-sided join-progress refusal, with
whole-box/cap tests and failed evidence preserved. Both refinement dependencies for isolation are now satisfied;
its own API/proof/cost preflight exposed the coherent owned-provider prerequisite, now verified/committed at9c27e2980:
immutable own/opposite/cutter geometry, full true-pi jets, both strict-hook maps and support exclusions, exact qualified
collapsed-opposite absence with contiguous typed roles. Sixteen cases/299294 assertions pass Release/ASan, old critical/
core and all broad/full/fast gates pass; tested digests match the three-path commit. Construction contacts then verified/
committed at `92ca5e445`: immutable whole-box endpoint proofs over both roles/ends, near-threshold exact F/critical-gap
handling, actual radial chain depth and direct critical work/history. Native 17/4383 Release/ASan plus old 97/586799
and all broad/full/fast gates pass, with no old assertion/cap change. Complete event isolation is now verified and
committed at `dff307668`; the final 15/5616 focused, geometry 154/1,464,224, full Release 3,842/7,321,304 with 74
unchanged skips, and DXF/DWG fast-lane Release+ASan receipts are recorded below. Independent rereview corrected the
tangent stress test's claim: it proves bounded empty refusal at depth 20, not successful ordering or identity. A
successful multi-pair shared-own-occurrence proof and component-incidence tests are now present in the committed
`.b-ref-components` slice at `9682d5cac`; this does not complete cache layout, distance bounds, topology validation or
the `.b-check` aggregate. The earlier six-query/coarse-node probe is historical and is superseded only for event
isolation, not for component-span or cache correctness. Successful interior-fallback execution remains
source-qualified, not natively exercised; the negative base-refusal test and bounded NO_MATCH search are distinguished
in the receipt.
The degree-1/degree-3 layout leaf is complete at `37462efc8` and reconciled at `6f25975f6`. The `.b-check-bounds`
product leaf is complete at `ce1ed51924d20a708e8bec059f7f9dd46235a232`, with exact parent `a0ce67a3b8746770ce49fb1ff76616c9da3d23fe`; its final source/test digests, independent reviews and Release/ASan/full/fast-lane evidence are recorded in the completed card below. The v25 plan-only reconciliation is verified at `4ca63677c27043004aae45e0fe54d42db48664a5`, its direct child. The v26 plan-only topology activation is verified at `fd4697059572af28c08ecbb532e687af1603ee1c`, a direct child of v25; local-topology implementation began from that committed product base. This v29 checkpoint reconciles the active implementation after the original red cases and invalid event fixture were repaired, and separates requirement/plan correctness from implementation correctness with explicit gate records. Current passing validation and reviewer outcomes are recorded in the active card. This bounded slice proves only local candidate/reference monotonicity and endpoint pairing; it does not establish global topology, public cache validity or later entity/file/UI behavior.
The coordinator owns the plan,
shared build directories and commits. Assign explicit file ownership before any parallel work; never run concurrent
Git/index operations or concurrent builds that mutate the same build tree.

There are five mandatory gates, with two separate judgments inside Gate 1: (1) **completeness** and **plan-contract
correctness** (§0.4), (2) **implementation correctness** on the actual diff (§0.7 and §§3.3-3.10), (3) an executable
sub-plan and current go/no-go readiness decision (§0.3/§0.6), (4) exactly one verified implementation commit after
each executable slice (§1.4 steps 1-3), and (5) immediate self-update in a separate plan-only commit after that product
commit (§1.4 steps 4-6). “Correctness” is intentionally not one blended review: Gate 1 asks whether the promised
contract and its oracle are true and complete for this checkout; Gate 2 asks whether the frozen implementation actually
satisfies that contract. Each judgment records `PASS` or `FAIL`, the reviewer and source identity, findings, evidence,
and any excluded behavior with its owning leaf. `N/A` is not a substitute for evidence; it requires a reason and a
verified scope boundary.

The order is strict: close Gates 1 and 3 before editing; after implementation, re-close completeness and plan-contract
correctness on the actual diff, then close Gate 2 on one immutable tested tree; commit only when all applicable gates
pass; and perform Gate 5 before opening or editing the next leaf. Every checkpoint rechecks all five against current
source and Git state; a prior receipt is evidence, not a permanent waiver. Product closure still requires each owning
slice's evidence.

Apply these tracks as a strict per-slice cycle:

| Gate | Required work and auditable pass record | Failure effect |
|---|---|---|
| 1. Completeness and plan-contract correctness | Record two independent results, each `PASS` or `FAIL`. **Completeness:** trace each applicable R01-R15 and any newly discovered obligation from request/source through caller, representation, producer/consumer, persistence/version/conversion path, build target, reachable positive/boundary/refusal/no-op/fallback tests and durable evidence. Map each claimed outcome to a concrete assertion, registered selector, fixture/oracle and evidence receipt; name exclusions and the owning leaf. Rescan shared dispatch/type sites when scope changes. **Plan correctness:** verify APIs, authoritative source/spec/version claims, preconditions/postconditions, numeric thresholds, arithmetic/proof steps, resource debit order, fallback/refusal policy, lifetime/transaction semantics, test-oracle independence, file/version matrix, cross-references, owners and acyclic dependency graph against current code and evidence. Record inspected revisions/files, findings, resolutions and exclusions. A green requirement table without reachable evidence is not a pass. | Repair coverage or correct the contract before implementation. Any unowned requirement, unsupported claim, contradictory/unachievable acceptance, invalid oracle, unreachable test or stale dependency keeps Gate 1 failed, its parent partial and the leaf not ready. |
| 2. Implementation correctness | On one immutable frozen diff, a reviewer distinct from the implementer should challenge every accepted and refused path: preconditions; full-domain proof/conservative enclosure; finite/overflow and outward-rounding behavior; numerical thresholds; resource admission/accounting; identity/order/closure; ownership/lifetime/transactions; atomic refusal; compatibility; and serialized bytes plus reload semantics when in scope. Independently establish at least one required positive and one adversarial negative; verify tests reject both false acceptance and unconditional refusal. Label proof, independent computation, sample, fixture oracle and bounded refusal distinctly. Record reviewer role, base/tree/file digests, counterexamples, findings, disposition and exact fix/retest evidence. If independent review is unavailable, record a separately executed self-review as such; never call it independent. Any edit invalidates the frozen review and affected test receipts. | Any required behavior defect, missing positive, unexamined refusal, invalid/self-referential oracle, source drift or unresolved finding fails Gate 2. Never weaken a required positive into a refusal to get green. |
| 3. Sub-plan and readiness | One executable leaf only. Its card names actual source and plan-checkpoint SHAs, committed predecessors, exclusive owner/path scope, verified APIs/spec facts, observable success/refusal and invariants, ordered bounded tasks/probes with decision and stop rules, literal build/test commands and targets, selectors with pre/post-registration nonzero counts, independent oracle, evidence destinations, recovery, resource/runtime envelope and one commit boundary. Resolve before `ready` every uncertainty that could change API, files, semantics, oracle, testability, resource cap or commit scope. Record a dated `READY`, `CONDITIONAL` or `WAITING` decision against actual `HEAD`; readiness expires on any base/source/API/toolchain/owner/scope change and must be re-probed. | An uncommitted predecessor means `waiting`; `conditional` is allowed only for a bounded named closure test with an explicit result-to-decision rule. A stale base, changed source/API/toolchain/scope, unresolved design choice, red required check or incomplete Gate 1 invalidates readiness. LOC, prototypes, compilation and passing subsets never establish it. |
| 4. Commit after each slice | Freeze the tested identity (base SHA, exact scoped diff, file digests, toolchain and durable reports); serialize shared-build validation; close final Gate 1 and Gate 2 on those bytes; inspect the pre-existing index and unrelated work; then make exactly one implementation commit for one named leaf. Stage explicit owned paths only; verify staged names and staged digests before commit. After hooks, verify actual parent, scope, commit SHA and committed bytes against the tested tree; isolate-validate any hook-modified bytes. Preserve failed attempts and all unrelated state. | Red/stale/mixed/unreviewed evidence, a changed frozen digest, any unowned staged path, or a commit that includes plan/next-slice work blocks closure. Fix the same leaf; never commit a passing subset, aggregate, or successor work. |
| 5. Self-update after commit | Before another product edit, immediately reread the actual product commit, diff, reports and worktree. Reconcile implementation SHA/file digests; all five gate outcomes; review findings; leaf/parent/requirement/format status; dependency DAG; risks and follow-ups; evidence ledger; queue; and successor readiness against actual source and predecessor/checkpoint SHAs. Update status/history/version and the next card, rerun whitespace/structure/hash/path/ancestry checks, then commit only this roadmap as a separate direct-child checkpoint. Verify one-file scope, product digest equality, product ancestry, checkpoint parent and all remaining worktree state. Record the product SHA, never the checkpoint's own SHA. | Stop product work if any reconciliation item or postcondition is missing. Do not mark `done`, activate a successor, or reuse stale readiness before this checkpoint is durable and verified. |

**Leaf state transitions (mandatory).** Readiness and implementation state are separate fields. `pending`/`waiting` may become
`ready` only after Gate 1 and Gate 3 pass at the actual base and every predecessor's required implementation commit
and post-commit checkpoint are verified (a documentation-only predecessor has its single scoped commit). The first product
edit changes state to `in progress` and consumes that readiness receipt. `validated` requires a frozen tree, final Gate
1 reconciliation, Gate 2 pass, and every required command/selector receipt. The single implementation commit changes
state to `committed — reconciliation pending`; only the successful separate Gate 5 checkpoint makes the leaf `done`.
Any new counterexample, API/toolchain/base change or post-freeze edit reopens the affected review and invalidates its
downstream readiness. A review finding may reopen a previously completed requirement; preserve the old commit/evidence
and add a new owning fix leaf rather than rewriting history or hiding the finding.

**Required per-leaf gate record.** The activation and completion cards must contain explicit fields, not infer them from
prose: `completeness_gate: PASS|FAIL`, `plan_correctness_gate: PASS|FAIL`, `implementation_correctness_gate:
PASS|FAIL|PENDING`, `readiness_gate: READY|CONDITIONAL|WAITING`, `implementation_commit_gate: PASS|PENDING|BLOCKED`,
and `self_update_gate: PASS|PENDING|BLOCKED`. Each field links the exact reviewer/source identity, test or commit
receipt, and unresolved item; `PENDING` is required before the work occurs and is never treated as success. Gate 1 may
close before edits, but it must be rechecked on the final implementation diff. Gate 2 cannot pass against an uncommitted
or subsequently modified tree. Gates 4 and 5 cannot pass in the same commit.

This is a repeatable loop, not one final plan audit: newly discovered behavior can reopen a completed requirement and
must create/revise an owning leaf. Completeness asks whether every promised behavior has an owner and a testable path;
plan correctness asks whether those promises and tests are true for this checkout; implementation correctness asks
whether the committed code actually meets the correct contract. These are three separate recorded judgments even
though the first two share Gate 1. A review may be performed by a separate agent; if unavailable, the implementation
author performs a distinct read-only pass and labels it self-review rather than independent review. Neither a passing
coverage table nor a plausible proof substitutes for another gate. The product commit and reconciliation checkpoint
are separate commits; the checkpoint records the product SHA, never its own SHA. A documentation-only plan review may
make one plan-only commit, but it must not fabricate a product commit or mark unfinished implementation done.

Before promoting a leaf card, check these concrete prerequisites:

| Card | Fact that must be established before editing | Evidence required before exit |
|---|---|---|
| 0B-1 | Current enum is contiguous; each of 50 values has a disposition; current CMake/CI locations confirmed | 50/27/23 kinds census, both negative shadow compiles, full and fast-lane tag counts |
| 1A-1.a | Raw sweep rules and finite/local coordinate domain are fixed | Boundary tests on both sides of each threshold and independent closed-form comparisons |
| 1A-1.b-budget | Done; original candidate-allocation/refinement accounting mismatch resolved | Nine cap/index cases, exposed consumed work, all offset/ASan/full/fast-lane gates recorded at 10c6bff46 |
| 1A-1.b-progress | Done at fb70681e2 | Underflow/non-finite/rounded-growth cases, old probe and all required broad checks passed |
| 1A-1.b-ref | Foundations/refinements/providers/contacts/event isolation/components done | `.b-ref-components` committed at `9682d5cac65f30cfd86aa10556678ceb88b1448b`; attempt-8 receipts are source matched. The v16 plan checkpoint is committed at `c6e32c17f`. The aggregate is complete; `.b-check` is separate and incomplete. Attempt 6 is historical red and attempt 7 pre-fix green. |
| 1A-1.b-check-layout | Done at `37462efc8`, reconciled at `6f25975f6`; bounds successor committed at `ce1ed5192`, v25 reconciliation verified at `4ca63677c` | Layout and bounds have scoped implementation/review/evidence receipts. The local-topology product commit is `ef3ba756b`; its v29 checkpoint remains pending. No public validity claim follows from layout, bounds or local proofs alone. |
| 1A-1.b-check | Layout/reference/bounds producers are committed; local topology is the next leaf and global chain is waiting | Local monotonicity/endpoint proofs with shared-budget refinement; later global coverage, unique order, exact event transitions and public reason mapping. No child alone closes R02 or exposes public cache validity. |
| 1A-2 | Guard and all geometry children are done; no raw constructor bypass exists | Current-header compile, complete stamp mutation cases, transform overflow and ownership checks |
| 1E-1.a | Typed schema grammar, malformed/unknown ownership and flag consistency are settled | Codec-independent exact-code/type tests, duplicate/malformed cases and bounded tails |
| 1E-1.b/.c | Writer placement and common DXF/DWG adoption callbacks confirmed | Real codec payloads, save failure propagation, no regeneration and attribute preservation |
| 1G-2.a | Existing provider/delegate and document Undo/Redo contracts are understood before generic edits | Old-provider defaults, rapid pending edits, before/after history hooks, refusal/no-op tests and ASan; full regression suite for shared history changes |
| 2A/2B | Live-document ownership, normalized equality and undo rules are fixed | Generated-child refusal, circle/arc equality, local-scale tolerance and one-step undo |

**Completed implementation card — `1A-1.b-check-layout` (product commit `37462efc8`; immediately reconciled by this v20 plan-only checkpoint).**

```yaml
slice: 1A-1.b-check-layout
state: done — product implementation committed at 37462efc8; this v20 checkpoint immediately reconciles it
readiness: ready before edits; all required implementation/validation gates passed
readiness_decision: Gate 1 completeness and plan-correctness passed at the v19 activation base `127231e41e20c4b434c0800bbde98dcb16f55ac3`; the product source was unchanged through the plan-only activation commit `cad1c5b4d`. The producer's line/degree-1/degree-3 outputs, exact layouts, 1B line-normalization boundary, target registration, bounded admission, refusal policy and test obligations were resolved before coding. Final implementation completeness, plan correctness and implementation correctness were rechecked against the frozen three-path diff; independent reviewer Anscombe found no actionable issues. Readiness for `.b-check-bounds` is separate and remains waiting pending its own refreshed probe.
candidate_code_base_sha: 127231e41e20c4b434c0800bbde98dcb16f55ac3
activation_base_sha: cad1c5b4d (plan-only readiness activation; identical product-source base 127231e41e20c4b434c0800bbde98dcb16f55ac3)
base_sha: cad1c5b4d
owner: Gibbs implemented the scoped slice; coordinator owned frozen-tree review, builds, evidence, and Git
dependencies: 1A-1.b-ref-components at 9682d5cac65f30cfd86aa10556678ceb88b1448b; v19 plan activation checkpoint at cad1c5b4d
requirements: R01, R02, R12, R13
source_facts_at_candidate_base: At lc_curveoffset.cpp:3109-3158, chainEntity emits RS_Line for one open straight chain; other all-straight chains become degree-1 ClampedOpen RS_Spline with endpoint multiplicity 2/internal 1; non-straight chains become degree-3 with endpoint multiplicity 4/internal 3; both spline paths use unit weights. materializeBranches at :5518-5553 calls chainEntity for every output branch; createEntities returns these entities. RS_Spline::setDegree at rs_spline.cpp:538-540 supports 1-3, while raw RS_SplineData::degree is unconstrained; validate at :1617-1680 checks general array/weight/knot/clamping rules but not this cache's finite planar unit-weight/exact-group/degree contract. This is the historical candidate-base fact. The implemented checker now uses `maxCacheSpans`/`cacheSpans` only; generic `remainingPieces`/`consumePieces` methods remain, and the distinct engine producer budget keeps its `cubicPieces` terminology. The persistent LC_OffsetCurve class is not yet present and its exact line normalizer remains owned by 1B.
contract: Layout-only classification and bounded admission of a normalized degree-1 or degree-3 ClampedOpen cache representation. Degree-1 and degree-3 are the only accepted cache degrees. An `RS_Line` result is converted exactly to a degree-1 spline at the persistent-curve engine-result normalization boundary owned by 1B; this checker consumes normalized `RS_SplineData` and does not perform entity conversion. This is not geometric validation and must not expose a successful `checkCache` result.
invariants: No refit/regeneration; no projection of nonzero Z; no padding/repair of missing controls, knots or weights; shared-budget debits happen once; no partial span output on refusal; layout does not infer closure/topology from flags alone.
blockers: None remain for this completed layout leaf. The 1B line normalizer is a planned downstream caller, not an implementation dependency. The next `.b-check-bounds` leaf must independently revalidate actual HEAD and its source/API facts after the v20 checkpoint; any product source/API change reopens that leaf's readiness.
files: Product/tests only lc_ellipseoffsetgeometry.h, lc_ellipseoffsetgeometry.cpp and tests/lc_offsetcurve_geometry_tests.cpp. Read-only preflight: lc_curveoffset.cpp, rs_spline.h/.cpp, CMakeLists.txt and librecad/src/src.pro. If evidence requires a producer change or additional product path, revise/split this card before coding.
selectors: Add `[lc-offsetcurve][geometry][cache-layout]` cases to the already registered `lc_offsetcurve_geometry_tests.cpp`; require and record at least 10 enumerated cases in Release. Run the identical selector in ASan. Existing `[reference-components]`, `[reference-events]`, `[intersection-budget]`, `[curve-offset][topology]` and `[curve-offset][ellipse]` are adjacent regression selectors.
cost_model: One layout admission charges `n` cache spans and every inspected control exactly once. Before either debit, check both remaining dimensions; if either is insufficient, latch `LimitExceeded` and consume neither. After admission, all debits are irreversible. No allocation proportional to untrusted counts before checked formulas, caps and array-size agreement; count helpers are nonallocating.
entry_probe:
  - Closed at 127231e41: inspected `chainEntity` (:3109-3158), `materializeBranches` (:5518-5553), `createEntities` (:5549-5564), `RS_Spline::setDegree` (:538-540), `validate` (:1617-1680), `LC_OffsetCheckLimits` and all budget field users; `LC_OffsetCurve` implementation is absent. Existing `lc_curveoffset_tests` selector `[curve-offset][direct][materialize]` passed 5 cases/226 assertions and exercises straight-line and cubic materialization. Source routing shows ellipse results use the same `chainEntity`; no claim that an ellipse fixture takes the straight branch is needed.
  - The exact persistent class/API does not exist yet: 1B owns `RS_Line` -> degree-1 normalization and the lossless adoption test; this layout child owns only normalized `RS_SplineData`. The future adapter must run before this layout validator, but its implementation does not block this prerequisite.
  - Decision rule: every emitted shape has an editable path: line -> exact degree-1 ClampedOpen data `[P0,P1]`, knots `[0,0,1,1]`, weights `[1,1]`; degree-1 spline -> validated degree-1 layout; degree-3 spline -> validated cubic layout. Unsupported/malformed layouts refuse adoption; do not downgrade a valid supported degree to plain. If a producer shape outside these forms is found, revise the persistent-cache representation and dependent cards before readiness.
  - Confirm raw-to-normalized import: absent raw weights may be materialized only after raw-carrier eligibility in 1E-1.c; normalized cache data has exactly one complete weight per control. Degree-specific raw control/knot counts are validated before XY projection or repair.
  - Record inspected revision/files, producer result shapes, normalization location, limits and any mismatch. If producer/caller behavior changed since `22317685c6a0fdba3b341cc138115750e6b0a5b6`, update this plan and affected descendants before editing.
api_preflight: The candidate-base API was inspected at 127231e41; the final committed API is `LC_OffsetCheckLimits::maxCacheSpans`, `CheckWork::cacheSpans`, `WorkBudget::remainingPieces/consumePieces`, and `inspectCacheLayout`. Only the checker-specific cap/counter were renamed; producer budget names remain untouched. Degree 1 and 3 are accepted; degree 2/>3 refused. `m_closedFlag`/`m_periodicFlag` do not change layout counts and later topology checks endpoint meaning. All old checker field references in owned production/tests were reconciled; no additional production/test/build-list caller was omitted.
readiness_probe_result: Entry readiness passed at 127231e41 and was refreshed at activation HEAD `cad1c5b4d`: `LC_CurveOffset::createEntities` emits only `chainEntity` results; supported shapes are `RS_Line`, degree-1 ClampedOpen `RS_Spline` and degree-3 ClampedOpen `RS_Spline`. The layout inspector receives normalized `RS_SplineData` only; line conversion is isolated to future 1B. The geometry test translation unit is already registered in the CMake `librecad_tests` target; no build-list edit was needed. Sixteen registered tagged cases now pass in Release/ASan. This records readiness for the completed leaf, not readiness for `.b-check-bounds`.
subplan:
  - stage: Close entry probe and freeze contract
    state: done before edits; source review repeated against frozen implementation
    work: Source probe closed at 127231e41: verify producer shape, current absence of persistent-curve caller, planned R05 one-piece adoption semantics, 1B line-normalization boundary, flags and exact degree-specific knot/data forms against current source plus roadmap contract.
    exit: Every producer shape has a lossless editable representation or an identified contract conflict that is resolved by revising dependent plan sections before readiness; no valid line or supported spline degree silently falls back to plain geometry.
  - stage: Implement bounded layout inspection
    state: done in product commit 37462efc8
    work: Rename only the checker limit/work counter from `maxCubicPieces`/`cubicPieces` to `maxCacheSpans`/`cacheSpans`; leave the distinct curve-offset producer budget names untouched. After minimum-size checks derive `n` without underflow: degree 1 uses `controlCount-1`; degree 3 first requires `(controlCount-1)%3==0`, then uses `(controlCount-1)/3`. Checked helpers verify exact degree-specific knot counts before any derived allocation: degree 1 has `n+1` controls and `n+3` knots, two equal endpoint knots and single internal knots; degree 3 has `3n+1` controls and `3n+5` knots, four equal endpoint knots and triple internal knots. Cap spans by `maxCacheSpans` and controls by `maxVertices` before scanning/materializing. Preflight both remaining budget dimensions before debiting either; if either is insufficient, latch `LimitExceeded` with neither debit applied. Once admitted, debit `n` spans and all inspected controls once; subsequent failures retain those spent debits. Factor each count formula into a nonallocating helper for size_t-boundary tests. Require ClampedOpen, empty fitPoints, default/empty saved-open payload, finite XY with Z exactly zero, complete finite weights exactly 1, finite knots with exact repeated groups and strictly increasing representatives. Do not use an absolute tolerance that merges narrow valid intervals. Permit finite positive affine knot reparameterization; degree-1 span `k` uses controls `[k,k+1]`, cubic span `k` uses `[3k..3k+3]`; return each actual knot interval. Do not reject `m_closedFlag`/`m_periodicFlag` at this layer.
    exit: Return only layout status and span count; every scan/extraction is bounded and no incomplete geometric-validity API is published.
  - stage: Add focused cases
    state: done; 16 registered cases / 3,311 assertions in both configurations
    work: Exercise normalized line data, one- and multi-span degree-1 data, actual degree-3 producer data, and malformed `RS_SplineData`: both knot domains and affine rescaling; synthetic size_t-boundary arithmetic for both degree formulas without giant allocations; short/mismatched arrays; wrong degree/type; malformed endpoint/internal groups; fit/saved-open payload; nonfinite/Z control; missing/partial/nonunit weights; file closure flags; exact/one-less/pre-spent/sticky span and vertex budgets. Assert refusal publishes no extracted span; failed combined preflight consumes neither dimension, while failures after admission preserve all spent debits.
    exit: At least 10 registered `[lc-offsetcurve][geometry][cache-layout]` cases execute; degree-1 and degree-3 positive/refusal layouts and each documented disposition have non-vacuous assertions; observed case count is recorded.
  - stage: Freeze and verify
    state: done; frozen product commit 37462efc8 matches all tested source digests
    work: Freeze the three owned paths and digests; run focused Release/ASan, adjacent reference/intersection/engine regressions, full Release and both fast suites in Release/ASan serially per build directory; close separate completeness, plan-correctness and implementation-correctness reviews.
    exit: Passed. No failures; full-suite 74 skips and fast-lane 3/4 skips match recorded baseline identities. Independent review has no actionable findings.
commit_boundary: Exactly one layout-only product commit. No distance bounds, topology/public checkCache, producer edits or plan reconciliation are bundled.
validation:
  - `cmake --build build --target librecad_tests --parallel 4` (pass)
  - `cmake --build build-asan --target librecad_tests --parallel 4` (pass; linker emitted the existing compact-unwind size warning)
  - Build `librecad_tests`, `librecad_dxf_fast_tests`, and `librecad_dwg_fast_tests` in both trees; all targets pass.
  - `build/librecad_tests --list-tests "[lc-offsetcurve][geometry][cache-layout]"`: 16 matching cases.
  - Focused Release/ASan selector: 16 cases, 3,311 assertions, 0 failures/skips in each.
  - Adjacent reference/intersection/engine selectors: 94 cases, 754,288 assertions, 0 failures/skips in Release and ASan.
  - Full Release: 3,874 cases, 3,800 passed, 74 unchanged skips; 7,346,172 assertions passed.
  - DXF fast lane: Release/ASan each 39,932 assertions, 3 expected baseline skips; DWG fast lane: Release/ASan each 34,952 assertions, 4 expected baseline skips; zero errors/failures.
  - JUnit receipts are the `*-attempt-1.junit.xml` files under `build/evidence/1A-1.b-check-layout/`; reports list zero errors/failures and preserve the respective skip identities.
evidence: `build/evidence/1A-1.b-check-layout/` contains focused, adjacent, full Release, DXF-fast and DWG-fast JUnit reports for Release/ASan where required. The focused report contains all 16 tagged test cases. Reports are generated and ignored under the local build tree; they are not part of either commit.
frozen_identity: Product commit `37462efc8` has parent `cad1c5b4d`; scoped product diff SHA-256 is `7ccaf6becfc135cc8ae91d88e2b945f27fcb8c677cd1f6cf02cc21774e009c2c`, with 468 insertions/21 deletions across exactly the three owned files. File SHA-256: header `5e9b69dd05df19e554b4d39692c31a547ef19bed0da8d2099e939f8c240b5b70`; implementation `cb548c8bd48aef7bab2f79ed555e5b4f7fee5e16debd7e44556cae25d6aad307`; tests `4ff049d28c128fa76a1c8473430395a2815d80ea7ddf62fb5ae5d7399b48e340`. Final working-file hashes match the committed files.
recovery: No failed slice attempt. Preserve all JUnit receipts and the ignored/untracked `build-asan/`; stage only the three card-owned source/test paths. Any future correction reopens this card with new evidence and a new product commit/checkpoint pair.
completeness_review: Closed. Preflight and final diff-to-requirement/caller/build/test reconciliation cover R01/R02/R12/R13, all producer output forms, normalized degrees, 1B line-normalization ownership, import descendants, all three owned paths and 16 actually registered cases. No additional caller or build registration is required. Reviewer: coordinator plus read-only independent review by Anscombe; no actionable finding.
plan_correctness_review: Closed. The candidate-base producer facts, degree-specific formulas/groups, layout-vs-geometry boundary, affine parameter domains, generic work-budget API, combined admission and closure-flag responsibility match current source and the final implementation. Current checker references consistently use `maxCacheSpans/cacheSpans`; separate producer cubic terminology remains intact. No unresolved assumption or finding.
correctness_review: Closed on frozen three-path diff. Independently challenged overflow-safe count arithmetic, exact group sizing, allocation/admission order, finite planar/unit-weight checks, degree-specific control slicing and actual knot intervals, all-or-none budget preflight, no partial spans on refusal, and boundary of the line conversion. Tests and build evidence cover both accepted degrees. Independent reviewer Anscombe reported no actionable findings; no source changes followed review.
implementation_sha: 37462efc8
product_commit_audit: Pass. `37462efc8` contains only the three owned product/test paths and 468 insertions/21 deletions; its three committed SHA-256 values equal the frozen pre-validation values above. No build/evidence, plan, or unrelated file was staged. `build-asan/` remains untracked and preserved.
plan_checkpoint_state: This v20 plan-only reconciliation directly follows product commit `37462efc8` and owns only `docs/plan_offset_curve_2961.md`; its own commit SHA is intentionally not stored recursively. No next product edit is authorized by this completed card.
checkpoint_postconditions: On checkpoint commit, verify its parent is `37462efc8`, product file digests remain unchanged, and the commit contains only this plan file. Then refresh actual HEAD and source digests; `.b-check-bounds` remains `waiting` until its own Gate-1 completeness/plan-correctness and executable-readiness review passes.
next: The next action after this checkpoint is a re-probe of `.b-check-bounds` against actual HEAD: refine its subplan for line bounds and cubic de Casteljau bounds/distance clearances, lock independent oracles and error budgets, verify selector/file ownership, and record separate completeness, plan-correctness and readiness decisions. Only then have Gibbs start implementation. Commit that leaf and immediately self-update the roadmap before any successor.
```

**Completed implementation card — `1A-1.b-check-bounds` (v25 post-commit reconciliation; implementation `ce1ed51924d20a708e8bec059f7f9dd46235a232`).**

```yaml
state: done — implementation `ce1ed51924d20a708e8bec059f7f9dd46235a232`; this v25 roadmap-only reconciliation is the required separate post-commit checkpoint
readiness: ready at implementation base; all implementation gates are now closed
readiness_decision: The leaf was activated only after the v24 endpoint-contract checkpoint `a0ce67a3b8746770ce49fb1ff76616c9da3d23fe`. Final completeness traced R02/R05/R12/R13 through layout, visible-reference alternatives, crossing/cusp/trim/seam incidence, complete-interval and endpoint oracles, shared-budget accounting, no-partial refusal and topology handoff. Plan correctness closed the v22 cusp/trim semantics, v23 exact-rational noncircular interval oracle and v24 four-endpoint mapping. Independent implementation review on the frozen three-file tree found no actionable issue. Focused and adjacent Release/ASan, full Release, and DXF/DWG fast Release/ASan pass; baseline skip identities match. No public topology/cache-validity claim is made by this bounds leaf.
base_sha: a0ce67a3b8746770ce49fb1ff76616c9da3d23fe
original_product_source_base_sha: 6f25975f6ad02ba7cbb15982152905ff6f948fb0 (product sources unchanged across the v21-v24 plan-only amendments)
activation_checkpoint_sha: 138694560926cbb8e4cb34198fae83dcb6bb4c9a
contract_amendment_checkpoint_sha: v22 `350abaa82eddb97fa11cee3a7f3a4bdd3fb05217`
oracle_amendment_checkpoint_sha: v23 `1c767f1726fb5a08d58a8c7e5ffa187ce3d601b2`; parent is v22 `350abaa82eddb97fa11cee3a7f3a4bdd3fb05217`
endpoint_amendment_checkpoint_sha: v24 `a0ce67a3b8746770ce49fb1ff76616c9da3d23fe`; parent is v23 `1c767f1726fb5a08d58a8c7e5ffa187ce3d601b2`
owner: Gibbs owns implementation edits to the three product/test paths; coordinator owns review, shared builds, evidence and Git. Anscombe is read-only for the plan/review gates. No concurrent product writer or build-directory writer.
dependencies: 1A-1.b-ref-components at 9682d5cac65f30cfd86aa10556678ceb88b1448b; 1A-1.b-check-layout at 37462efc8; v20 reconciliation at 6f25975f6ad02ba7cbb15982152905ff6f948fb0; v24 endpoint-contract checkpoint at a0ce67a3b8746770ce49fb1ff76616c9da3d23fe.
requirements: R02 (bounded geometric correctness), R05 (refusal must not adopt unchecked geometry), R12 (shared deterministic limits), R13 (reproducible validation/evidence).
api_preflight: At base 6f25975f6, `CacheLayoutResult` provides owned degree-1/degree-3 controls and actual knot intervals; `ReferenceComponentAnalysis` owns its `ReferenceEventAnalysis`, visible `ReferenceEdge`/`ReferenceSpan` graph, `ReferenceIncidence`, and full `ReferenceFringe` boxes; `ReferenceFrame::boundOffsetJet(a,b)` returns conservative centre-relative, uniformly scaled local jets; `pointToFrame` encloses world-double inputs; `WorkBudget` is irreversible and exposes remaining vertices/pairs plus depth. `ReferenceSpan.interior` excludes root isolating intervals, visible `clearance` is exact `abs(signedDistance)` on the open span and its one-sided closure, and only edges in the visible component graph may witness geometry. The CMake `librecad_tests` target already compiles `lc_offsetcurve_geometry_tests.cpp`; the Release binary enumerates 16 `[cache-layout]` and 16 `[reference-components]` cases. No `checkCache` or persistent entity API exists yet.
contract: Add a proof-only bounded relation for normalized cache subcurves and exact visible-reference fragments. For every candidate/reference fragment pair, either certify a pair witness or prove separation beyond the geometry threshold; an unresolved pair refuses the complete call. Every candidate parameter domain must be covered by at least one certified visible-edge/node witness. If every visible alternative for any candidate domain is proved separated, refuse as a geometric mismatch (mapped to `OffCurve` by the later public checker); never interpret an empty witness relation as vacuous success. An empty/refused reference analysis preserves its upstream failure disposition and cannot be reclassified as successful empty geometry. Return outward geometry-Hausdorff upper bound and candidate source-clearance lower bound, plus cache dyadic identity and immutable reference edge/subinterval or event-node/occurrence identity. `Ok` means the complete bounded relation and candidate-to-reference witness coverage were proved; it does not mean one reference component, reverse coverage, injectivity, ordered assignment, editability or public `checkCache` validity.
invariants: No parameter map, nearest-foot identity, rounded-node substitution or sampled acceptance is assumed. Compare specified closed subcurve sets using symmetric chord-segment Hausdorff bounds. Use local frame units consistently: `eQ=M2*h^2/8` where `M2` is an outward local bound on `||Q''||` and `h` is the reference traversal-parameter width, never a cache knot width; candidate cubic deviation uses de Casteljau residual controls. Require `E_geom.hi <= ((2*tol)/scale).lo` and independently `(L-E_geom).lo >= ((abs(distance)-2.5*tol)/scale).hi`, with interval operations and endpoint comparisons outward. For an event-node witness, source clearance is `D_local - E_candidateNode`; the fringe-to-node enclosure radius contributes to geometry/reverse-fringe coverage, not to this candidate-to-node clearance term. Root fringes are explicit; bind each visible occurrence fringe to its exact node and inherit clearance only from closure of an incident visible edge. Never cross the whole-ellipse seam in a jet call. `WorkBudget` is caller-owned: the top-level cache-check orchestration retains the same instance across layout, reference construction, bounds and later topology; the bounds result owns the immutable reference analysis/witness IDs and reports a work snapshot, not a budget copy. Every expansion/query/output record is charged before allocation; work is never refunded. Any unresolved potentially-relevant pair, nonfinite bound or cap refusal returns no partial witness relation.
blockers: None remain for this completed leaf. Any later product/API change that alters the bounds proof or its caller invalidates the corresponding evidence and requires a scoped re-probe/review. The v25 checkpoint is now verified; topology readiness and its split local/chain cards are recorded in v26 below.
files: Write only `librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.h`, `librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.cpp`, and `librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_geometry_tests.cpp`. Read-only checks: `lc_curvejet.h`, `lc_interval.h`, `rs_spline.h/.cpp`, offset reference/component implementations, `CMakeLists.txt`, and `src.pro`. CMake registration already exists; no build-list edit unless the source audit proves otherwise. Gibbs starts implementation after activation is ready.
tasks:
  - stage: Reconfirm entry contract and freeze tests/API
    work: Re-read actual base, §3.5 and all relevant callers; build and enumerate existing layout/reference selectors; verify source span visibility, exact-D clearance, endpoint-root incidence/fringe ownership, local units, depth semantics, and all three file owners. Record baseline receipts and hashes. If any contract changes, revise the card first.
    exit: All claims trace to current symbols and mathematics; baseline selectors are nonempty and green; reviewer has no unresolved blocker; no unowned caller/build path.
  - stage: Add immutable result and one-ledger orchestration
    work: Add a move-only proof result owning the complete `CacheLayoutResult`, `ReferenceComponentAnalysis` and all witness records, but not `WorkBudget`. The caller/top-level checker creates and retains one budget and passes that same instance through layout, component construction, bounds and the later topology stage; no stage creates/resets a replacement. Validate/inspect cache layout first (bounded rejection before expensive reference work), then build components and compare using the same caller budget; return its cumulative snapshot. Owning the layout and immutable reference graph lets topology refine even a geometry-passing coarse witness without borrowed cache/graph lifetime. Expose no public validity result. Use diagnostic indices only while the owned analysis remains alive.
    exit: Invalid layout spends no reference work; reference refusal spends no pair work; every exit reports the same irreversible ledger; no partially published witness vector on failure.
  - stage: Implement certified pair bounds and witness enumeration
    work: Iterate candidate-span/reference-fragment Cartesian pairs lazily. Reference fragments are each visible edge's event-free `interior` plus each visible edge's own start/end occurrence fringe as a separate node-anchored fragment; do not generate a fragment for hidden or cutter-only geometry. On each candidate dyadic leaf, use interval-local de Casteljau controls. Degree 1 has zero chord error; degree 3 uses the two Bernstein residual-control norm upper bound. On each reference interior subinterval strictly inside one visible edge, enclose endpoints and `Q''` via `boundOffsetJet`; compute local `M2*h^2/8`, with `h` from that reference traversal interval. Bound symmetric Hausdorff between the two chord segments by both directed endpoint-to-segment bounds with outward projection/clamping; use a conservative endpoint-ball bound for degenerate chords. Sum all error intervals. For each root fringe, associate the exact occurrence, its node and incident visible edge; compute a max-separation bound between the full `ReferenceFringe` local box and `ReferenceNode` local box. Bound candidate-to-node and node-to-candidate using the candidate chord/residual, add fringe separation for fringe coverage, and use only candidate-to-node error in `D_local-E_candidateNode` clearance. Do not substitute a rounded node or assume the hidden side of the root box is visible. Keep every certified edge/node alternative for topology. For an interior fragment, prune only if the outward AABB distance lower bound exceeds the geometry-threshold upper bound; for a fringe fragment, prune only if the candidate/node lower bound minus the full fringe-to-node error exceeds that threshold. Otherwise split the larger-error/size side deterministically. If pair upper bound passes, record it; if lower bound proves separation, discard it; otherwise split until one applies. Search domains never cross a visible-edge boundary or `T -> 0` seam.
    exit: Every candidate dyadic domain has at least one visible edge/node witness; every cross-pair was either certified, conservatively separated, or the entire call refused as unresolved/limited. Every witness independently satisfies geometry and source-clearance inequalities; ambiguous near pairs are never omitted; all IDs refer to the owned immutable graph. No topology/public-validity claim is made.
  - stage: Add proof and refusal tests
    work: Register `[lc-offsetcurve][geometry][cache-bounds]` cases for generated degree-1/degree-3 positives; exact straight/circle chord and analytic sagitta; independent high-precision interval containment; **a noncircular reference-ellipse oracle that outwardly encloses the full Q(t) coordinate range and full Q(t)-linear-endpoint-chord deviation on every tested subdomain, with complete interval coverage for wrapped/reversed, near-cusp and extreme-scale cases; also compare independent start/end x/y endpoint enclosures to production `startX`, `startY`, `endX`, `endY` fields for every such range**; endpoint-and-midpoint adversarial cubic with large between-sample deviation; restricted-source clearance; reversed cache geometry; edge endpoint plus full root fringe; hidden-span/cutter-only exclusion; multiple components (proof relation only, no topology claim); near cusp; extreme centre/radius scale; affine/narrow knots; and exact, one-less, pre-spent, sticky vertex/pair/depth caps. For the noncircular oracle, use an independent outward interval enclosure or analytic range proof, not `boundReferenceChord`, the production `LC_Interval` result under test, or finite point samples as acceptance evidence. Compare each `CurveChordBounds` endpoint interval against the corresponding whole-precision point enclosure, not merely against the whole-curve box. If subdivision is used, prove every child is outwardly enclosed and the children cover the entire closed parent domain without gaps; include endpoint and wrapped/seam semantics explicitly. A directed-rounding interval library may be used only after confirming its rounding policy and dependency are available in the test target; otherwise use an independently justified analytic/Taylor interval oracle. Keep dense multiprecision point samples as supplemental checks. Explicitly assert that all visible alternatives being proved separated, while only hidden/cutter geometry is nearby, refuses as geometric mismatch (`OffCurve` at the later public boundary) and publishes no partial relation (no vacuous-empty success). Exercise one `WorkBudget` instance through layout, reference analysis and bounds, checking cumulative counters, no reference work after layout refusal, no pair work after reference refusal, and no partial result after bounds exhaustion. At a shared physical crossing, assert each visible incoming/outgoing edge retains exact edge, occurrence and end-role identity; for each admitted interior `SimpleCusp`, assert `row.cusp && row.hiddenPoint`, zero visible degree, and no witness for its occurrence/node. Add a trim-boundary fixture asserting each visible incident edge's exact edge, occurrence, endpoint role and trim-node identity. Keep visible endpoint identity assertions on applicable stationary/free endpoints and both seam incidences. Prove whether exact endpoint/cusp equality is representable by finite binary64 angle fields. If exact equality is impossible, record a rigorous argument (a simple cusp has algebraic `sin^2(theta)`, while sine of a nonzero algebraic/dyadic-radian angle is transcendental; axis cases are not simple cusps), then test the closest representable endpoint angles bracketing the certified cusp root and assert correct strict classification or conservative refusal with no false endpoint incidence. If equality is representable, test that exact case refuses without bounds analysis/witness publication. Distinguish mathematical certificates from dense-sample cross-checks.
    exit: At least 12 actually registered selector cases execute in Release and ASan with non-vacuous success and failure assertions; replace the minimum with the measured case count before validation.
  - stage: Freeze, independently review, validate and commit product slice
    work: Freeze exactly the three owned paths and file/scoped diff hashes; independent reviewer challenges chord Hausdorff proof, interval projection degeneracy, event endpoint/fringe clearance, relation completeness, overflow/finite handling, budget admission/lifetime and false-accept counterexamples. Run focused, adjacent, full and both fast lanes serially per build tree; retain every attempt.
    exit: All required checks pass on the frozen tree; baseline skip identities match; reviewer has no actionable finding; product commit contains only the three owned files and exactly matches validated bytes.
commit_boundary: One bounds-only product commit. No topology/public `checkCache`, entity, persistence, format, engine-producer or plan changes in this product commit. Immediately after it, reread the commit and self-update only this roadmap in a separate checkpoint commit before any next product edit.
selectors: `[lc-offsetcurve][geometry][cache-bounds]`: 25 listed test cases/46,104 assertions Release and ASan. Adjacent `[cache-layout]`, `[reference-components]`, `[reference-events]`, `[intersection-budget]`, `[curve-offset][topology]`, and `[curve-offset][ellipse]`: `--list-tests` enumerates 110 test functions; saved Release/ASan JUnit receipts contain 145 invocations/757,599 assertions, with zero skips/failures.
cost_model: Layout spends `maxCacheSpans` and controls through `inspectCacheLayout`. Each candidate/reference or candidate/node proof comparison consumes one `distancePairs` debit before examination; each candidate/reference interval subdivision, retained witness, and pending/work record consumes `vertices` before allocation. Both dimensions are preflighted before a record needing both is created; no refunds. Search depth is the checked sum of candidate and reference subdivision depth; event-node comparison adds candidate depth only. `checkDepth` is recorded on entry. Initial Cartesian products are iterated lazily, not allocated. AABB pruning is conservative only. Exact, one-less per dimension, pre-spent and sticky retry tests must return `LimitExceeded` with no witness list; a cross-stage test passes the identical `WorkBudget` through layout, reference analysis and bounds and asserts the cumulative snapshot and stage-stop counters. Result work is the final shared budget snapshot, not summed stage snapshots.
correctness_oracle: Independent 113-bit/Boost multiprecision evaluation of exact circle/ellipse offset and candidate Bezier values; analytic circle sagitta for circle chord tests; independent outward whole-interval range and endpoint-chord-deviation proof for noncircular reference ellipses, including separate endpoint containment checks for `startX/startY/endX/endY`; exact-rational blossom/residual-control checks for candidate cubics. The noncircular oracle must enclose every point of each entire closed parameter subdomain, its deviation bound must enclose Q(t) minus the linear endpoint interpolant for the same full domain, and the returned chord endpoint fields must contain the corresponding independent Q(a)/Q(b) enclosures. Dense independent geometry sampling is a diagnostic cross-check only, never acceptance evidence. Actual engine-generated caches are compatibility positives, not the proof oracle.
entry_evidence: Candidate code base and full SHA are `6f25975f6ad02ba7cbb15982152905ff6f948fb0`; Release enumerates 16 `[cache-layout]` and 16 `[reference-components]` cases. Fresh `[cache-layout]` Release and ASan runs each pass 16 cases/3,311 assertions, zero skips/failures; receipts are `build/evidence/1A-1.b-check-bounds/baseline-cache-layout-{release,asan}-attempt-1.junit.xml`. Product-source SHA-256 values equal v20: header `5e9b69dd05df19e554b4d39692c31a547ef19bed0da8d2099e939f8c240b5b70`, implementation `cb548c8bd48aef7bab2f79ed555e5b4f7fee5e16debd7e44556cae25d6aad307`, tests `4ff049d28c128fa76a1c8473430395a2815d80ea7ddf62fb5ae5d7399b48e340`. Test target registration confirmed at top-level `CMakeLists.txt:2612`.
validation: |
  mkdir -p build/evidence/1A-1.b-check-bounds
  cmake --build build --target librecad_tests librecad_dxf_fast_tests librecad_dwg_fast_tests --parallel 4
  cmake --build build-asan --target librecad_tests librecad_dxf_fast_tests librecad_dwg_fast_tests --parallel 4
  build/librecad_tests --list-tests "[lc-offsetcurve][geometry][cache-bounds]"
  QT_QPA_PLATFORM=offscreen build/librecad_tests "[lc-offsetcurve][geometry][cache-bounds]" --order decl --reporter junit --out build/evidence/1A-1.b-check-bounds/focused-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[lc-offsetcurve][geometry][cache-bounds]" --order decl --reporter junit --out build/evidence/1A-1.b-check-bounds/focused-asan-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_tests "[lc-offsetcurve][geometry][cache-layout],[lc-offsetcurve][geometry][reference-components],[lc-offsetcurve][geometry][reference-events],[intersection-budget],[curve-offset][topology],[curve-offset][ellipse]" --order decl --reporter junit --out build/evidence/1A-1.b-check-bounds/adjacent-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[lc-offsetcurve][geometry][cache-layout],[lc-offsetcurve][geometry][reference-components],[lc-offsetcurve][geometry][reference-events],[intersection-budget],[curve-offset][topology],[curve-offset][ellipse]" --order decl --reporter junit --out build/evidence/1A-1.b-check-bounds/adjacent-asan-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_tests --order decl --reporter junit --out build/evidence/1A-1.b-check-bounds/full-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_dxf_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/1A-1.b-check-bounds/dxf-fast-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_dwg_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/1A-1.b-check-bounds/dwg-fast-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_dxf_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/1A-1.b-check-bounds/dxf-fast-asan-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_dwg_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/1A-1.b-check-bounds/dwg-fast-asan-attempt-N.junit.xml
  # Replace N with a fresh attempt number; serialize commands within each build directory. Parse every JUnit report,
  # record process exits/case/assertion/skip identities, compare expected skips to the v20 baseline, run git diff --check,
  # hash the frozen scoped diff/files, and ensure product bytes do not change after review or the final test.
evidence: `build/evidence/1A-1.b-check-bounds/`. Final receipts: focused Release/ASan attempt 10 (25 listed test cases/46,104 assertions, 0 skips/failures); adjacent Release/ASan attempt 9 (145 JUnit invocations/757,599 assertions, 0 skips/failures; the selector lists 110 test functions); full Release attempt 9 (4,793 cases/7,392,350 assertions, 74 skips, 0 failures); DXF fast Release/ASan attempt 9 (1,028/39,932, 3 skips each); DWG fast Release/ASan attempt 9 (1,084/34,952, 4 skips each). All skip identities match their recorded baselines. Final independent oracle/reviewer receipt is retained with the slice evidence.
recovery: Preserve failed patches, reports and worktree. Never stage/delete `build-asan/` or other unrelated files. Fix and rerun the same leaf with a new attempt number; if API/source/scope changes, reset only the plan decision to waiting and re-probe before continuing.
completeness_review: Pass on frozen implementation `ce1ed51924d20a708e8bec059f7f9dd46235a232` (product files unchanged from v24 checkpoint through product parent). Coordinator plus independent Anscombe review traced R02/R05/R12/R13 through every owned path, registered test and refusal path. The v22 cusp/trim/crossing/seam/stationary incidences, v23 whole-domain Q and residual oracle, v24 endpoint-field assertions, shared budget/no-partial guarantees and topology handoff are implemented and evidenced. Component count/order, injectivity, reverse coverage and public reason mapping remain explicitly owned by `.b-check-topology`/later leaves, so the parent aggregate is not complete.
plan_correctness_review: Pass. v22's hidden simple-cusp/endpoint representability contract, v23's independent exact-rational directed interval oracle (Machin pi bracket, Taylor remainder and 128-cell closed-domain coverage), and v24's mapping of each Q(a)/Q(b) coordinate enclosure to the corresponding `startX/startY/endX/endY` output were checked against the final test source and current API. No unresolved acceptance contradiction or test-oracle gap remains for this bounds-only leaf; proof of topology is not inferred from setwise proximity.
correctness_review: Pass on the frozen three-path product tree `ce1ed51924d20a708e8bec059f7f9dd46235a232`; independent Anscombe final review found no actionable issue. Review covered accepted/refused paths, outward interval arithmetic and complete coverage, four endpoint-field comparisons, interval finite/rounding behavior, conservative pair pruning, candidate/source clearance, shared-budget ordering, lifetime and no-partial refusal. No product edits followed review; the measured Release/ASan and broad-suite results below are on the committed bytes.
implementation_sha: ce1ed51924d20a708e8bec059f7f9dd46235a232
product_commit_audit: Pass. Commit `ce1ed51924d20a708e8bec059f7f9dd46235a232` is a direct child of v24 checkpoint `a0ce67a3b8746770ce49fb1ff76616c9da3d23fe`; exactly three owned paths, 1,485 insertions and no other changes. Committed SHA-256: header `3b52881bcd18f72a646b66a94ed8fc63666e203b850c259b018fca08a86aa931`, implementation `dcb1744fd067caf631038db2821004a55af050eacfad2bbc69e63800b25f6558`, tests `a782408d3574ccc4c156dbc90536d7cecec81509e3e7823946c17b3679c46701`. Index check and commit succeeded; no plan/evidence files were staged. Existing untracked `build-asan/` is preserved.
plan_checkpoint_state: v25 is verified at `4ca63677c27043004aae45e0fe54d42db48664a5`, a plan-only direct child of product commit `ce1ed51924d20a708e8bec059f7f9dd46235a232`; it contains only `docs/plan_offset_curve_2961.md`.
checkpoint_postconditions: Pass at v25 SHA `4ca63677c27043004aae45e0fe54d42db48664a5`: parent is exactly `ce1ed51924d20a708e8bec059f7f9dd46235a232`; the only path is this roadmap; diff check passes; all three product SHA-256 values equal the committed card; branch/worktree check preserves the only unrelated untracked `build-asan/`. The refreshed actual product/API base is `4ca63677c27043004aae45e0fe54d42db48664a5`. The v25 self-hash was omitted in v25; it is recorded here after commit and is no longer recursive.
next: v26 closes Gate 1 and records ready status for `.b-check-topology-local` at unchanged product base `4ca63677c27043004aae45e0fe54d42db48664a5`. Commit this roadmap update as a plan-only direct child, verify its path/parent/product-digest/worktree postconditions, then Gibbs may start only the local topology leaf. The global chain/public `checkCache` leaf remains waiting for the local implementation commit plus its separate checkpoint and a fresh source/API/readiness probe. Each product leaf still receives exactly one implementation commit and immediate plan-only reconciliation.
```

**Active implementation card — `1A-1.b-check-topology-local` (v29 immediate post-commit reconciliation).**

```yaml
slice: 1A-1.b-check-topology-local
state: done — product implementation `ef3ba756b52796c02b8490caedcaa48b8db69605` plus this immediate v29 plan-only reconciliation; post-commit checkpoint audit is required before successor work
readiness: ready at activation, consumed by the first product edit; this receipt does not authorize the next leaf
candidate_product_base_sha: v26 checkpoint `fd4697059572af28c08ecbb532e687af1603ee1c`; product files are unchanged at the base
activation_checkpoint: v26 plan-only commit `fd4697059572af28c08ecbb532e687af1603ee1c` is the direct child of `4ca63677c27043004aae45e0fe54d42db48664a5`; implementation began against that committed product base. The v29 roadmap is excluded from the implementation commit and is its immediate separate one-file child.
owner: Gibbs owns product edits after activation; coordinator owns plan, one build at a time per build tree, independent review, evidence and all commits
requirements: R02, R05, R12, R13
completeness_gate: PASS — final trace is on the three-path scoped implementation/test diff. The 20-case registered selector reaches success, monotonicity/refusal, exact endpoint pairing, exact event-root incidence, reversed orientation, seam, malformed/extreme and budget boundaries. CMake test registration and both CMake fast targets are verified; qmake6 is an application build only. This leaf does not claim global coverage, public validity, file/entity/UI behavior, or reverse reference coverage; those remain owned by `.b-check-topology-chain` and later leaves.
plan_correctness_gate: PASS — coordinator rechecked actual symbols and callers, derivative/projection proof conditions, endpoint orientation, shared budget, conservative failure statuses, test-oracle independence, exact source scope and the downstream DAG. The internal candidate-cell chord axis cannot be used unless its lower length bound is positive; the root fringe remains one-sided identity only. `TooNear` remains absent because no clearance upper-bound proof exists.
implementation_correctness_gate: PASS — independent Gibbs review of the current three-path implementation diff returned no actionable correctness findings. Review and final validation were performed with no subsequent product edits; product digests below identify the reviewed/tested bytes.
readiness_gate: READY at v26 activation; consumed by implementation. A new Gate 1/Gate 3 probe is mandatory before `.b-check-topology-chain`.
implementation_commit_gate: PASS — product commit `ef3ba756b52796c02b8490caedcaa48b8db69605` has parent `fd4697059572af28c08ecbb532e687af1603ee1c`, exactly the three owned geometry/header/test paths, and staged/committed content matching the tested hashes and binary-diff digest.
self_update_gate: PASS — this v29 is the immediate separate one-file plan reconciliation after the product commit; its direct-child/path/digest/worktree postconditions are checked immediately after commit, and no successor work starts before they pass. The v29 commit SHA is intentionally not stored here.
completeness_review: PASS at activation `4ca63677c27043004aae45e0fe54d42db48664a5`; final PASS against the current three-path diff. Trace local topology through `CacheBoundsWitness` alternatives, candidate layout spans, exact edge/occurrence/node IDs, root-fringe roles, the public geometry namespace, future cache adoption and the CMake test target. This leaf owns only local injectivity/monotonicity and endpoint-pair proofs; it must not claim complete reference coverage or public cache validity. Global ordered assignment, component count, closure flags and `checkCache` are assigned to `.b-check-topology-chain`. No current production caller of `analyseCacheBounds` or `checkCache` exists; the API remains geometry-layer groundwork for 1B/1E callers.
plan_correctness_review: PASS by coordinator at activation and final diff. Checked degree-1/cubic derivative formulas, outward projection tests, immutable witness identity, fringe-only endpoint semantics, common shared budget, scope split, reason labels, reflection API contract and exact dyadic representability. For reference direction, a strict positive lower bound on the projected derivative over the entire interval directly proves strict forward scalar monotonicity; if the derivative enclosure includes zero, require a nonnegative derivative enclosure plus strict outward endpoint separation. The symmetric backward proof follows by sign reversal. This avoids refusing a sound strict-derivative proof solely because rounded endpoint enclosures overlap; endpoint separation alone is never used without a nonnegative derivative proof. The reflected-domain helper validates exact dyadic metadata and rejects unsafe/malformed domains atomically; it is not a certificate constructor. `TooNear` remains removed because current certificates do not prove a clearance upper bound.
readiness_decision: Gates 1 and 3 passed at v26 before edits. Readiness was consumed on the first product edit. After implementation, final completeness and plan correctness pass, Gibbs returned no actionable correctness finding, every required validation lane passes on the frozen product diff, and the path/hash/index audit verified commit `ef3ba756b52796c02b8490caedcaa48b8db69605`. This immediate v29 reconciliation closes the leaf only after its own post-commit path/parent/digest/worktree audit passes.
current_blockers: No unresolved product-correctness blocker. Gate 5 requires the immediate post-commit audit of this one-file v29 checkpoint; until it passes, do not activate `.b-check-topology-chain`. Preserve the pre-existing untracked `build-asan/` directory and all failed historical reports.
source_facts: At source base `4ca63677c27043004aae45e0fe54d42db48664a5`, `WorkBudget` is noncopyable/irreversible and exposes `CheckWork{cacheSpans,vertices,distancePairs,deepest}`; `CacheBoundsAnalysis` owns normalized spans, immutable `ReferenceComponentAnalysis`, and setwise witness alternatives but not the budget or the acceptance thresholds needed to rerun a subpair. `CacheBoundsBuilder` currently contains the only pair geometry/clearance implementation. `ReferenceFrame::boundOffsetJet` supplies whole-interval offset jets; `ReferenceComponent.edges` are source-parameter ordered and explicitly do not imply crossing-successor pairing. `ReferenceFringe.parameter` is the full event-root enclosure; the exact edge start/end occurrence and node IDs identify its one-sided role. `RS_SplineData` requires ClampedOpen for normalized caches; `m_closedFlag` and `m_periodicFlag` are round-trip metadata handled only by the later chain leaf.
api_contract: In namespace `LC_EllipseOffsetGeometry`, add `CacheLocalTopologyStatus {Ok, InvalidInput, BoundsRefusal, Topology, Ambiguous, LimitExceeded}`, move-only `CacheLocalTopologyResult`, immutable `CacheLocalTopologyAnalysis` and `analyseCacheTopologyLocally(cache, definition, tol, budget)`. Preserve the originating bounds/reference statuses and cumulative `CheckWork`. `Ok` means a complete bounded set of locally monotone candidate/reference alternatives exists and every candidate positive-length domain has at least one such alternative; it does not establish reverse coverage, one component, global order, closure, or validity. The analysis owns the bounds result and local witness records, never the caller's budget. Extract one internal pair-proof helper from `CacheBoundsBuilder`, used both by initial bounds enumeration and any topology refinement, and retain checked geometry/clearance thresholds in the owned analysis. Do not duplicate the bounds arithmetic or retry with a fresh budget. The top-level caller applies `towardCentre` once to the signed offset; `cacheReversed` reverses whole candidate span order and each local control/domain direction once, while reference source parameter remains increasing. Every local cell is paired start-to-start and end-to-end after this single normalization; reference intervals always advance in increasing source parameter. Never swap endpoints or select a fresh orientation for an individual cell to rescue a candidate reversal. For a start/end fringe witness, normalize which candidate endpoint is bound to its node only after applying that whole-cache reversal; preserve the original edge-end role in the record.
proof_contract: For each selected interior pair, re-prove geometry and clearance on the exact subdomains. The cell's normalized candidate start must pair to the selected reference interval's lower-parameter endpoint, and its normalized candidate end to the upper-parameter endpoint; bound both corresponding endpoint distances against the same geometry threshold. These endpoint roles are invariant across every subdivision of a cache span. Pick a deterministic finite nonzero local chord from the oriented candidate endpoint enclosures and prove its squared length has positive lower bound. Degree 1 requires a strictly positive interval projected derivative. Cubic derivative controls are `3(P1-P0)`, `3(P2-P1)`, `3(P3-P2)`; their projected intervals must all have lower endpoint `>= 0`, with at least one `> 0`. For the reference interval, a strictly positive lower bound on the outward projected derivative proves strict forward scalar monotonicity directly; otherwise require a nonnegative derivative enclosure and strict outward endpoint separation (`end.lo > start.hi`). Use the symmetric negative-derivative/start-after-end condition for backward direction. Endpoint separation alone is insufficient. The candidate and reference therefore share the same forward cell orientation after `cacheReversed` is applied once; no locally reversed witness may be retained even if it has the same set image. A symmetric setwise pass alone does not pair boundaries. Refine/re-prove both sides when any condition is undecided. Root-fringe records are exact one-sided endpoint/incidence certificates only, never positive-length candidate/reference coverage. Use the published root enclosure and edge role as-is; if a sign cannot be proved on that enclosure, return conservative `Topology`/`Ambiguous`, not an invented root or sampled pass. No partial analysis escapes failure.
files: Product/test writes are exactly `librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.h`, `librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.cpp`, and `librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_geometry_tests.cpp`. Read-only preflight already confirms CMake registers the test source at `CMakeLists.txt:2612`; `librecad/src/src.pro` does not register this CMake-only test file. No build-list change is expected. Any newly discovered caller or need for another source file resets readiness and requires a revised card before edits.
selectors: `[lc-offsetcurve][geometry][cache-topology-local]` selects 20 registered cases (verified by `--list-tests`). Final focused Release attempt 4 and ASan attempt 2 each pass 20 cases/6,823 assertions, 0 skips/failures. Bounds-regression Release attempt 3 and ASan attempt 2 each pass 25 cases/46,104 assertions. Adjacent Release/ASan attempt 2 each pass 145 JUnit invocations/757,599 assertions, no skips. Historical red attempt 1 remains preserved and is not current evidence. The minimum case count is exceeded; named cases cover exact event-root identity, reversed orientation, seam direction, local folds, endpoint-role mismatch, atomic resource boundaries and reflection metadata.
correctness_oracle: Match the actual folded test fixture exactly: source radius 1000, outward offset 1, symmetric sweep `[-delta,delta]` with `delta=1/200000`, recentered so the cache chord is vertical through `y=0`, `tol=0.005`, and cubic scalar controls `[0,1,-1/4,1]` scaled by `dy=2*1001*sin(delta)`. The exact derivative roots are `(9-sqrt(5))/19` and `(9+sqrt(5))/19`; exact-rational brackets around both roots prove a forward/reverse/forward derivative sign pattern, while the cubic remains in scalar image `[0,1]`. Independently compare its Bernstein controls with the straight chord's `[0,1/3,2/3,1]`: their maximum coordinate-control difference is `(11/12)*dy < (11/12)*(1001/100000) < 0.00918`. The reference-circle chord sagitta is bounded by `1001*delta^2/2 < 1.26e-8`, so the sum is `<0.009181 < 2*tol`; radial clearance is at least `1001*cos(delta)-1000 > 0.998 > 1-2.5*tol`. The runtime fixture explicitly requires bounds to accept this geometry before local topology refuses its internal reversal; these analytic rational bounds are independent of the production topology result. Also retain the rounded `[0,4/3,-1/3,1]` discriminator. Dense samples are diagnostic only. Positive cases use exact derivative sign/control proofs, analytic circle bounds or already-established bounds oracles, not the topology implementation as its own oracle.
cost_model: Keep the exact `WorkBudget` instance alive from `analyseCacheBounds` through local topology. `distancePairs` debit before every pair bound, endpoint-pair comparison, derivative interval query, relation check or root/sign subdivision; `vertices` debit before each pending cell, retained local witness, index or output record; `checkDepth` records the checked sum of candidate/reference refinement depths. Preflight combined record costs before allocation; checked arithmetic and no refunds. Never materialize a Cartesian product or enumerate paths exponentially. A resource refusal latches `LimitExceeded`, reports the same cumulative `CheckWork`, and publishes no partial local analysis.
subplan:
  - stage: Verify v26 activation and freeze inputs
    work: Confirm the v26 plan-only commit is the direct child of `4ca63677c`, contains only the roadmap, and leaves the three product hashes below unchanged. Re-read exact current source/API and actual Git status; preserve `build-asan/`. Assign Gibbs the three product/test paths and coordinator the builds/index. If any input differs, stop and re-probe before edits.
    exit: Source base, plan parent, paths, APIs and ignored/untracked state are recorded; no competing writer/build exists.
  - stage: Extract reusable pair certification
    work: Factor the existing chord, geometry-threshold and source-clearance comparison into one internal helper shared by `CacheBoundsBuilder` and local topology refinement. Store required immutable threshold/distance data with `CacheBoundsAnalysis`; preserve public bounds statuses, witnesses, endpoint outputs and debit order. Add focused assertions that existing bounds classifications and no-partial behavior are unchanged.
    exit: Existing cache-bounds focused tests pass; no copied/second pair formula, reset budget, changed clearance policy or altered public bound success contract.
  - stage: Emit local monotone alternatives
    work: Add the move-only local result/analysis/witness record and bounded local projection proof. Split only exact candidate/reference domains through the shared pair helper. Bind every normalized candidate subinterval start/end to the reference interval lower/upper endpoint, respectively; preserve these endpoint roles through all subdivisions and never reverse a cell independently. Record forward orientation, geometry/endpoints/clearance, edge/occurrence/node identity and root-fringe role. Require at least one positive-length local witness for every candidate domain; treat fringe witnesses as endpoint certificates only. Return no public cache-validity result.
    exit: Complete local alternative relation is immutable and owns its bounds analysis; every candidate domain has a locally injective witness in the one normalized forward direction, or the whole result refuses atomically. Same-image internal reversal refuses even if each geometric subpiece independently has a setwise witness.
  - stage: Add adversarial, event and budget tests
    work: Cover degree-1 and generated cubic positives, stationary endpoint, both `cacheReversed` directions, exact-binary folded same-image cubic, rounded legacy discriminator, constant span, candidate-start/reference-start and candidate-end/reference-end mismatch, a subdivision that would pass only by flipping local orientation, event-root fringe identity/sign refusal, hidden cusp exclusion, affine/narrow knots, extreme finite coordinates, exact/one-less/pre-spent/sticky pair/vertex/depth caps, and same-ledger cross-stage accounting. Retain previous cache-bounds/layout/reference assertions.
    exit: At least 12 new registered cases run; exact rational oracle proves the folded counterexample is rejected despite any individually setwise-matching subpieces; all positive local outcomes have independent forward-derivative/endpoint checks and failures publish no partial result.
  - stage: Freeze, review, build, validate and commit
    work: Freeze the three owned paths and hashes; close final Gate 1/2 reviews on those bytes. Run focused/adjacent/full/fast CMake matrix and qmake6 application build serially per build directory; parse JUnit case/assertion/failure/skip identities; compare skip identities with baselines; preserve all attempts.
    exit: All checks pass on the frozen tree; reviewer has no actionable issue; tested hashes equal exactly one local-topology implementation commit containing only the three owned files.
commit_boundary: Exactly one `.b-check-topology-local` product commit with subject `Prove local offset-cache topology`. Stage only the three owned paths, run `git diff --cached --check`, verify `git diff --cached --name-only` exactly matches the card, and compare staged hashes to the tested freeze before committing. Afterward verify commit parent/path/hash and hooks. Do not add the global chain, public `checkCache`, entity integration, persistence, format or UI changes. If local proof cannot be delivered coherently in these three paths, preserve evidence and revise into explicit children before any partial product commit.
validation: |
  mkdir -p build/evidence/1A-1.b-check-topology-local
  cmake --build build --target librecad_tests librecad_dxf_fast_tests librecad_dwg_fast_tests --parallel 4
  cmake --build build-asan --target librecad_tests librecad_dxf_fast_tests librecad_dwg_fast_tests --parallel 4
  build/librecad_tests --list-tests "[lc-offsetcurve][geometry][cache-topology-local]"
  QT_QPA_PLATFORM=offscreen build/librecad_tests "[lc-offsetcurve][geometry][cache-topology-local]" --order decl --reporter junit --out build/evidence/1A-1.b-check-topology-local/focused-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[lc-offsetcurve][geometry][cache-topology-local]" --order decl --reporter junit --out build/evidence/1A-1.b-check-topology-local/focused-asan-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_tests "[lc-offsetcurve][geometry][cache-bounds]" --order decl --reporter junit --out build/evidence/1A-1.b-check-topology-local/bounds-regression-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[lc-offsetcurve][geometry][cache-bounds]" --order decl --reporter junit --out build/evidence/1A-1.b-check-topology-local/bounds-regression-asan-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_tests "[lc-offsetcurve][geometry][cache-layout],[lc-offsetcurve][geometry][reference-components],[lc-offsetcurve][geometry][reference-events],[intersection-budget],[curve-offset][topology],[curve-offset][ellipse]" --order decl --reporter junit --out build/evidence/1A-1.b-check-topology-local/adjacent-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[lc-offsetcurve][geometry][cache-layout],[lc-offsetcurve][geometry][reference-components],[lc-offsetcurve][geometry][reference-events],[intersection-budget],[curve-offset][topology],[curve-offset][ellipse]" --order decl --reporter junit --out build/evidence/1A-1.b-check-topology-local/adjacent-asan-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_tests --order decl --reporter junit --out build/evidence/1A-1.b-check-topology-local/full-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_dxf_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/1A-1.b-check-topology-local/dxf-fast-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_dwg_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/1A-1.b-check-topology-local/dwg-fast-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_dxf_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/1A-1.b-check-topology-local/dxf-fast-asan-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_dwg_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/1A-1.b-check-topology-local/dwg-fast-asan-attempt-N.junit.xml
  WT=/Users/dli/dev/LibreCAD/.claude/worktrees/lc-offset-curve-2961
  mkdir -p /private/tmp/lc-offset-curve-2961-qmake6
  cd /private/tmp/lc-offset-curve-2961-qmake6
  /opt/homebrew/bin/qmake6 -r CONFIG+=sdk_no_version_check "$WT/librecad.pro"
  make -j4
  # Parse all JUnit results; expected unchanged skips are full Release 74, DXF fast 3 and DWG fast 4.
  # No qmake test registration is expected: this source is CMake-test-only; qmake6 is an app compile gate.
  # Record exact commands/exit codes, selector counts, skip identities, hashes and diff-check result.
evidence: `build/evidence/1A-1.b-check-topology-local/`; reports are ignored under build output and are not committed. Final focused Release attempt 4 and ASan attempt 2 pass 20 cases/6,823 assertions each; cache-bounds Release attempt 3 and ASan attempt 2 pass 25/46,104; adjacent Release/ASan attempt 2 pass 145/757,599; full Release attempt 3 passes 4,813 cases/7,399,173 assertions with 74 baseline-identical skips; DXF fast Release/ASan attempt 2 pass 1,028/39,932 with 3 baseline-identical skips; DWG fast Release/ASan attempt 2 pass 1,084/34,952 with 4 baseline-identical skips. Every report has zero failures/errors. Exact skip-name sets were compared with `.b-check-bounds` attempt-9 baselines and are identical. qmake6 recursive project generation and `make -j4` both exit 0; transcript is `qmake6-build-attempt-2.log`. The red focused attempt 1, earlier invalid fixture and superseded diagnostics remain preserved as history, not final evidence.
baseline_evidence: Product SHA-256 at `4ca63677c27043004aae45e0fe54d42db48664a5`: header `3b52881bcd18f72a646b66a94ed8fc63666e203b850c259b018fca08a86aa931`; implementation `dcb1744fd067caf631038db2821004a55af050eacfad2bbc69e63800b25f6558`; tests `a782408d3574ccc4c156dbc90536d7cecec81509e3e7823946c17b3679c46701`. CMake registration confirmed at line 2612. Existing baselines: cache-bounds 25 listed test cases/46,104 assertions; adjacent 110 listed test functions and 145 JUnit invocations/757,599 assertions; full Release 4,793/7,392,350 with 74 skips; DXF fast Release/ASan 1,028/39,932 with 3 skips; DWG fast Release/ASan 1,084/34,952 with 4 skips. Existing skip identities are preserved in v25 evidence. No new-selector baseline is claimed.
frozen_identity: Base `fd4697059572af28c08ecbb532e687af1603ee1c`; scoped binary diff SHA-256 `eae944d5e6402b2a3f55c69d4f6b8adcabe785adc2f42b6e8a347290d539447c`. Header SHA-256 `0887c44a8c042505e1a4b8299d9ed133dadc0cc4a522a7183215225629769eba`; implementation `1ce0bd467d9512044cc2f039cbfd6fa8dfd2073b963eba5acd014a47c7e94069`; test `021e753b1216a99d0f0588e26ab4d96034840bc4f6546d066f37beb1a7ab4a6e`. These product bytes were unchanged across final CMake validation and the independent review; recompute and compare staged/committed hashes immediately before/after commit.
implementation_correctness_review: PASS — independent Gibbs review of the current three-path diff: “No actionable correctness findings in the reviewed current diff. No files edited.” The review challenged local monotonicity, endpoint orientation, reflected dyadic mapping, event fringe, seam handling, bounded work and atomic refusal. No product source/test edit followed that review; reviewed/tested source digests are recorded below.
implementation_sha: `ef3ba756b52796c02b8490caedcaa48b8db69605`
product_commit_audit: PASS — commit `ef3ba756b52796c02b8490caedcaa48b8db69605` is a direct child of activation checkpoint `fd4697059572af28c08ecbb532e687af1603ee1c`; it contains exactly the three owned product/test paths, 1,500 insertions and 91 deletions. Product diff SHA-256 is `eae944d5e6402b2a3f55c69d4f6b8adcabe785adc2f42b6e8a347290d539447c`. Committed SHA-256: header `0887c44a8c042505e1a4b8299d9ed133dadc0cc4a522a7183215225629769eba`, implementation `1ce0bd467d9512044cc2f039cbfd6fa8dfd2073b963eba5acd014a47c7e94069`, test `021e753b1216a99d0f0588e26ab4d96034840bc4f6546d066f37beb1a7ab4a6e`. Pre-existing index was empty; only explicit paths were staged; `git diff --cached --check` passed; the plan and `build-asan/` were not committed.
plan_checkpoint_state: This v29 roadmap is the immediate plan-only checkpoint after product commit `ef3ba756b52796c02b8490caedcaa48b8db69605`; do not store its own commit SHA in this file.
checkpoint_postconditions: Immediately after committing, verify v29 parent is exactly `ef3ba756b52796c02b8490caedcaa48b8db69605`, changed path is exactly `docs/plan_offset_curve_2961.md`, product hashes remain unchanged, diff-check passes, requirements/status/evidence/DAG/next readiness are reconciled, and full worktree status preserves `build-asan/`. Stop if any condition fails; do not start the successor until all pass.
recovery: Preserve all patches, exploratory red output, JUnit attempts and ignored build outputs; do not reset/delete/stage unrelated files. If any post-freeze product edit is needed, return the leaf to `in progress`, invalidate review and affected receipts, and rerun the required matrix with new attempt numbers. No product edit may begin after `ef3ba756b` until v29 is committed and its direct-child/path/digest postconditions pass.
next: After the v29 plan-only checkpoint is verified, re-read actual HEAD/API and conduct a new Gate-1 and Gate-3 probe for `.b-check-topology-chain`. It adds the global ordered path, reverse coverage, exact event transitions, cycle/closure semantics and public `checkCache`; no stale local card authorizes it.
```

**Waiting implementation card — `1A-1.b-check-topology-chain` (must be activated by a later checkpoint).**

```yaml
slice: 1A-1.b-check-topology-chain
state: waiting — local implementation and v29 checkpoint are predecessor commits; no fresh activation probe has been run
readiness: waiting; inspect actual source/API and both predecessor commits, then close Gate 1 completeness/plan correctness plus Gate 3 before edits
dependencies: `.b-check-topology-local` implementation commit and its immediate plan-only checkpoint
requirements: R02, R05, R12, R13
contract: Require exactly one retained nondegenerate component; use exact source-ordered edge/occurrence incidences, consume all local alternatives without reuse, prove a unique ordered path/cycle over both complete domains, and only then expose public `checkCache`. Node-fringe proofs are endpoint certificates, not positive-length coverage. Open components match exact first/last occurrences. Whole-source cycles use the one `ReferenceSeam.start/end` transition; closed trimmed components require a proven first/last event-node transition. `cacheReversed` reverses the candidate sequence once and never changes the signed offset. Check `m_closedFlag`/`m_periodicFlag` consistency without inferring periodic derivative continuity from ClampedOpen controls. Multiple paths are ambiguous unless they have identical edge/occurrence sequence modulo the single allowed cycle cut; crossing branches are not interchangeable.
entry_probe: Inspect actual whole-source and trimmed-cycle engine outputs and the first/last edge/occurrence ordering at current HEAD. Determine whether the producer's start is canonically anchored to `ReferenceSeam.start` or the earliest source-ordered retained start. If so, enforce that exact anchor. If the producer can legitimately rotate the cycle, define and test a bounded cut-alignment proof before changing the contract; otherwise conservatively refuse noncanonical rotations. Recheck how `cacheReversed`, closed/periodic flags and file-loaded control order affect that anchor. The probe must close before readiness is marked.
algorithm: Build a bounded DAG keyed by exact candidate domain boundaries and reference edge/parameter or event-incidence boundaries. A transition consumes one locally certified witness and advances both orders. Require complete candidate and reference interior coverage plus every one-sided fringe endpoint certificate; no branch switching, repeated edge, gap, extra loop or component subset. Count paths with saturation at two, not enumeration. Canonicalize a cycle only at its exact seam/trim occurrence; quotient duplicate subdivisions only when the full edge/occurrence sequence is identical. Do not accept arbitrary phase rotations unless the entry probe adds a bounded, independently tested cut-alignment algorithm. Unresolved cycle phase/identity refuses rather than sampling or greedily selecting a nearest branch.
failure_mapping: Invalid arguments -> `InvalidInput`; layout -> `Layout`; `abs(distance) <= tol` -> `TooSmall`; proven absence of any geometric witness -> `OffCurve`; event/component-analysis refusal -> `ReferenceUnresolved`; unresolved interval proof -> `Ambiguous`; missing, extra, non-unique or incompatible ordered topology/closure -> `Topology`; exhausted work -> `LimitExceeded`. No `TooNear` claim without a clearance upper-bound proof. Return the same final cumulative `CheckWork` and no partial success.
tests: Exact one-component positive, disconnected components, proper subset/missing tail, extra loop/retrace, branch switch at shared crossing, event-fringe identity, trim/lens join, whole seam, closed trimmed cycle, open endpoint mismatch, both cacheReversed values, flag inconsistencies, equivalent-vs-non-equivalent cycle assignments, ambiguous relation, and all work caps. Independent reference/component path oracle must be derived from fixture event parameters/incidences, not the selected production path.
files: Re-probe; expected geometry header/implementation/test translation unit, but any new source/registration expands scope and requires readiness amendment.
commit_boundary: One global-chain/public-validator product commit only after complete final reviews and all required tests. Immediately follow with a separate roadmap-only plan checkpoint; no entity, file, UI or persistence integration bundled.
```

**Completed implementation card — `1A-1.b-ref-components`** (implementation `9682d5cac65f30cfd86aa10556678ceb88b1448b`;
this v16 plan-only checkpoint is the separate reconciliation commit). Product dependency base is `dff307668`; initial
edits began at `c3f091c3f6f2367dae13f631b1e72487528672d2`. Frozen final validation used HEAD
`d896a19160ef9fc9fca0d9ba94632d49ab2470f7` plus the five-path diff. The implementation commit is exactly that
validated source; no product edit followed validation. `.b-check-layout` remains waiting for a fresh activation check
after this checkpoint commit.

```yaml
slice: 1A-1.b-ref-components
state: done — implementation `9682d5cac65f30cfd86aa10556678ceb88b1448b`, attempt-8 evidence, 2026-10-16
readiness: ready — all implementation gates passed before the product commit
readiness_decision: Final completeness, plan-correctness and implementation-correctness gates closed on the frozen tree; no required finding remains. Historical pre-implementation reviews were not recorded and are not retroactively claimed. The product commit matches the tested five-path tree; this plan-only checkpoint reconciles it. The next leaf remains waiting until this checkpoint is committed and HEAD is re-read.
product_base_sha: dff307668
product_edit_base_sha: c3f091c3f6f2367dae13f631b1e72487528672d2
validation_base_sha: d896a19160ef9fc9fca0d9ba94632d49ab2470f7
current_product_sha256: geometry.cpp=e8cbb9daa03db672dad21fafd5b299d94eb201c40be097f572efb9f2816254ad; geometry.h=2b7e0c0c45c886f937a3aed778a3407a76a5448de17893a9dfc56fde7334c547; intersection.cpp=cf2b3e177c5e234992df606900860107b43dfc49580d367b438440a06e38bd79; intersection.h=1ddb4389e780be8d4ac901cabbdbd0b22399a109930953901e493d8cc3a25b41; tests=83f26f06afe063bc0d8b76e2188221cf06f0182c9a9235ac3d0edf9c19c1a038
frozen_diff: five product/test paths; 1430 insertions, 12 deletions relative to validation_base_sha; no CMake/qmake6 registration changes required
owner: Gibbs completed the five-file product handoff; coordinator now owns independent review, validation and Git. Any required product fix stays in this slice and invalidates affected evidence/digests.
coordinator: plan, validation, independent review, Git and evidence; do not overlap product edits or builds in the same build directory
dependencies: 1A-1.b-ref-events-isolate at dff307668 plus its checkpoint at 17810a8d7
requirements: R02, R12, R13
completeness_review: Closed against the frozen diff: all five owned paths, graph contract, success/refusal/no-partial tests, actual CMake and qmake6 source/test membership, and focused selector coverage reconciled. No missing build registration or required contract path found.
plan_correctness_review: Closed against current source and plan: private issuer-bound builder, shared budget and exact-D visibility contracts match the implementation; engine compatibility remains downstream; no new API/build dependency or unresolved assumption remains for this leaf.
correctness_review: Gibbs independently found the non-finite-distance rational-conversion P1. The early input guard and invalid-input assertions were added, then independently re-reviewed; no unresolved finding remains. Review is read-only against the final frozen five-path tree.
api_preflight: ReferenceEventAnalysis is immutable/factory-owned; it publishes root-enclosure intervals, ordered own occurrences, node IDs and retained strict sessions, but not the temporary occurrence-to-root handles needed for further contraction. Component preparation must remain inside the private builder transaction or preserve an equivalent issuer-bound map.
contract: Produce an immutable exact-mathematical-D reference component graph; classify all event-free own spans and retain every connected component, with no engine materialization or partial success.
invariants: Keep event refinement issuer-bound; use the same caller WorkBudget and one preparation/query history; contract complete root fringes to the existing node-error allocation; preserve every crossing incidence and both seam occurrence IDs; do not use diagnostic IDs as solver handles.
files: librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.h; librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.cpp; librecad/src/lib/engine/document/entities/lc_parametriccurveintersection.h; librecad/src/lib/engine/document/entities/lc_parametriccurveintersection.cpp; librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_geometry_tests.cpp. Explain why each helper change belongs to component analysis; recheck CMake/qmake6 target membership and update registrations only if required.
subplan:
  - stage: Reconcile current worktree
    work: Preserve attempts 1-6; inspect the complete five-path diff against `c3f091c3f`; assign one writer; do not reset/replay. Reconcile the aggregate oracle against the current tests so each expected flag has a fixture that can produce it; attempt 6's impossible stationary expectation is historical and must be checked for correction, not presumed fixed.
    exit: Enumerate and record the actual registered focused case count; the six-fixture topology/flag oracle has no impossible aggregate expectation; focused Release rerun is green on a recorded source identity.
  - stage: Prove shared-root success
    work: Construct and independently derive a deterministic input where one own occurrence participates in multiple pair records. Diagnose the current depth-20 refusal from root enclosures/proof bounds; repair the proof/search or fixture geometry without increasing caps or merging by proximity.
    exit: Native success publishes every pair incidence at the shared physical node, agrees with an independent high-precision oracle, and remains within the caller's one shared budget. A bounded refusal does not satisfy this exit.
  - stage: Certify spans and fringes
    work: Continue the builder while issuer-bound handles are live (or retain equivalent owner-bound handles); contract each complete root enclosure; cover each event-free open core and both symbolic one-sided fringes without gaps/overlap; classify exact-D visibility with conservative whole-interval proofs.
    exit: Whole-source isolated equality refuses with no partial graph; exact and adjacent thresholds behave as specified; restricted arcs do not use the supporting whole ellipse; near-foot narrow visibility either proves correctly or refuses conservatively.
  - stage: Certify exact axis-to-parameter mapping
    work: For the major-radius analytic contact specialization, prove the linear-fractional axis mapping encloses the true parameter over the pi bracket, with denominator nonzero and sign consistent with sweep direction. Reconcile it with `CriticalBuilder::mapAngle`; cover wrapped near-zero sweeps, reversed sweeps, exact endpoint axes and interior axes. If the bracket cannot prove the mapping, return unavailable/refuse rather than publish an inferred identity.
    exit: Each claimed axis enclosure contains an independent high-precision parameter and agrees with the source-domain membership/orientation; no false Present/Absent classification or axis identity occurs at wrapped, reversed or near-degenerate inputs.
  - stage: Assemble exact graph
    work: Create one edge per retained event-free own span and preserve every endpoint incidence. Map physical nodes only with certified identity/world enclosure; preserve crossing/cusp/tangency/construction/free-end roles and two seam occurrences on one physical seam node; derive weakly connected components without guessed successor pairing. Audit nodes that have an own-role occurrence but zero retained own-span incidence: prove hidden/non-retained, publish a modeled singleton point component, or refuse the entire graph with no partial analysis. Opposite/cutter-only nodes are excluded from this graph audit.
    exit: Six fixture outcomes plus rotated/reversed/translated, crossing, cusp, tangency, seam, free-end and branch cases match independently enumerated expected incidences and component membership. Tests non-vacuously exercise a successful hidden own-role zero-degree node, or prove this outcome unreachable over the accepted domain; every zero-degree own-role event has an explicit disposition. Retained isolated equality at restricted-source and endpoint-cap cases is preserved or refused atomically; non-own-only event nodes do not cause false refusal.
  - stage: Prove bounded refusal atomicity
    work: Charge records before allocation and comparisons before examination against the same irreversible WorkBudget; propagate actual depth and final cumulative work; keep result staging private until all checks succeed.
    exit: Exact/one-less per dimension, pre-spent and sticky-retry caps refuse with no analysis; successful work equals the budget's final snapshot and does not double-count event work.
  - stage: Freeze, review and accept
    work: Hash all five owned paths and base/diff; run every validation command below serially per build directory; record completeness, plan-correctness and implementation-correctness reviews against that frozen identity; fix findings in this slice and rerun affected gates.
    exit: All required reports pass with nonzero cases, expected skips only, no sanitizer diagnostics, all three review records closed, and tested digests exactly equal the intended implementation commit.
commit_boundary: These are ordered sub-stages of one leaf and one product commit; do not commit a passing subset. If a stage proves independently shippable only after current-code review, first revise the roadmap into buildable child leaves and dependencies, then continue without discarding existing work.
selectors: Add a nonzero `[lc-offsetcurve][geometry][reference-components]` selector in the existing test translation unit; preserve `[reference-events]`, `[reference-contacts]`, `[intersection-budget]`, `[curve-offset][topology]` and `[curve-offset][ellipse]` identities.
validation: |
  mkdir -p build/evidence/1A-1.b-ref-components
  cmake --build build --target librecad_tests librecad_dxf_fast_tests librecad_dwg_fast_tests -j8
  cmake --build build-asan --target librecad_tests librecad_dxf_fast_tests librecad_dwg_fast_tests -j8
  build/librecad_tests --list-tests "[lc-offsetcurve][geometry][reference-components]"
  QT_QPA_PLATFORM=offscreen build/librecad_tests "[lc-offsetcurve][geometry][reference-components]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-components/release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[lc-offsetcurve][geometry][reference-components]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-components/asan-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_tests "[lc-offsetcurve][geometry][reference-events],[lc-offsetcurve][geometry][reference-contacts],[intersection-budget]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-components/reference-regression-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[lc-offsetcurve][geometry][reference-events],[lc-offsetcurve][geometry][reference-contacts],[intersection-budget]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-components/reference-regression-asan-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_tests "[curve-offset][topology],[curve-offset][ellipse]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-components/engine-regression-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[curve-offset][topology],[curve-offset][ellipse]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-components/engine-regression-asan-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_tests --order decl --reporter junit --out build/evidence/1A-1.b-ref-components/full-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_dxf_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/1A-1.b-ref-components/dxf-fast-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen build/librecad_dwg_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/1A-1.b-ref-components/dwg-fast-release-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_dxf_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/1A-1.b-ref-components/dxf-fast-asan-attempt-N.junit.xml
  QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_dwg_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/1A-1.b-ref-components/dwg-fast-asan-attempt-N.junit.xml
  Replace N with a fresh attempt number. Serialize binaries within each build directory. Record exit codes before parsing XML, registered cases/assertions/skips, stable failure identities, source/test digests and evidence paths. Unmatched selectors, empty feature counts, unexpected skips or sanitizer diagnostics fail the gate. Run the independent high-precision oracle as a separately identified test/probe; it does not replace interval proofs.
cost_model: Charge occurrence/fringe/span/node/edge/adjacency records before allocation; charge every proof comparison/subdivision and intersection pair before examination; propagate actual depth. The result work equals the same budget's final consumed snapshot, not event snapshot plus component delta. Exact, one-less per dimension, pre-spent and sticky-retry limits must publish no analysis on refusal.
correctness_oracle: Independent high-precision restricted-source distance checks and interval containment checks; engine fixtures establish compatibility only, never certify the exact predicate. A sample can select a sign only after complete event proof establishes constancy over the whole span.
frozen_identity: Frozen at edit base `c3f091c3f` and validation HEAD `d896a19160ef9fc9fca0d9ba94632d49ab2470f7`; the five-path diff is 1,430 insertions/12 deletions and is committed unchanged at `9682d5cac65f30cfd86aa10556678ceb88b1448b`. SHA-256 digests above match the committed files.
completeness_review: Trace R02/R12/R13, all event roles, both endpoints/seam, six expected fixtures, multiplicity, build target membership, graph-to-checker contract and every refusal/no-partial path; verify every new helper dependency is in scope and no format/build integration is silently required. Enumerate actual selector cases on the frozen tree, distinguish JUnit assertions from test-case count, and map each flag assertion to a fixture that can exhibit it.
plan_correctness_review: Re-read current event/intersection APIs and callers; independently check that the exact-D component contract, own-role zero-edge disposition (excluding opposite/cutter-only nodes), axis parameter mapping/orientation contract, dependencies, budget/refusal rules and downstream checker expectations agree with current code and the plan's mathematical definitions. Record source paths/revision and every assumption confirmed or corrected; any unresolved contract contradiction blocks readiness.
correctness_review: Independently inspect exact-D and restricted-sweep predicates, complete root-fringe coverage, physical node identity, crossing/cusp/tangency incidence, seam closure, total budget charging, axis-to-parameter enclosures, hidden-event witness bounds and engine-margin separation; review a successful multi-pair component and adversarial/isolated equality refusals against an independent oracle. Required shared-own success, zero-degree witness coverage or isolated-node coverage unresolved means correctness gate fails.
evidence: Preserve attempts 1-6 as historical; attempt 6 lists 10 cases and three failures across two cases, including the now-corrected aggregate stationary expectation and shared-own depth-20 refusal. Attempt 7 passed before the finite-distance fix. Final attempt 8 reports: focused Release/ASan, 16 JUnit testcase entries each, 21,557 assertions, zero failures/skips; reference regression 48 entries/11,559 assertions and engine regression 98 entries/727,115 assertions in both builds; full Release 4,752 entries/7,342,935 assertions, zero failures/errors and 74 unchanged skips; DXF fast Release/ASan 1,028 entries/39,932 assertions with 3 expected optional external-fixture skips; DWG fast Release/ASan 1,084 entries/34,952 assertions with 4 expected missing-optional-fixture skips. All XML parsed, exits were zero, skip identities match prior baselines, and focused selector enumerates 16 cases. Reports are under `build/evidence/1A-1.b-ref-components/*attempt-8.junit.xml`; no test process remains running.
implementation_sha: 9682d5cac65f30cfd86aa10556678ceb88b1448b
blockers: None for the completed component slice. `.b-check-layout` waits only for a fresh post-checkpoint activation check; the three DXF and four DWG optional-fixture skips have unchanged baseline identities and are not failures of this geometry slice.
commit: Product/test commit `9682d5cac65f30cfd86aa10556678ceb88b1448b`; this separate plan-only commit records its SHA, evidence, parent status and next-leaf gate. No next geometry product change is bundled.
recovery: Preserve `build-asan/` and unrelated changes. If this checkpoint is interrupted, finish plan reconciliation before more product work; do not reapply the committed component implementation.
readiness: ready — all implementation gates passed before commit; the product slice is complete. The next product leaf needs a fresh HEAD/source check.
next: `1A-1.b-check-layout`; re-read actual HEAD after this checkpoint and activate only if its entry contract still matches.
```

These are entry/exit gates, not permission requests. Resolve an algorithm/API mismatch in the owning card and revise
its downstream contracts before coding; do not silently narrow required scope. The default order remains §0.3, with
`.b-budget`, `.b-progress`, `.b-ref` then `.b-check` replacing its aggregate `.b`. Ready leaves may proceed independently only when predecessors,
file ownership, validation ownership and commit ordering permit it.

### 0.6 Executable leaf recipes

These recipes refine, not replace, the normative contracts. Paths below are relative to
`librecad/src/lib/engine/document/entities/` unless a full repository-relative path is given. Before editing, expand
the selected recipe into the §0.3 card with actual HEAD, file ownership, API observations and literal commands.
Only satisfied dependencies promote a card to `ready`; future recipes are `waiting`, not ready by assertion.

| Leaf / dependencies | Ordered implementation tasks and scope | Required exit evidence |
|---|---|---|
| 1A-1.b-budget / .a | Done at 10c6bff46; shared checked cap, bounded spatial/range indices and conservative same-branch pruning, verified separately from 0B-3. | Nine registered `[intersection-budget]` cases, Release/ASan/full/fast-lane evidence below; no partial roots on refusal. R02/R12/R13. |
| 1A-1.b-progress / .b-budget | Done at fb70681e2; finite/progress checks and derived 2099-step bound preserve positive rounded growth and refuse invalid arithmetic. | Four cases/80 assertions Release/ASan, standalone before/after probe and full offset/ASan/full-suite/fast-lane gates pass. R02/R12. |
| 0B-3 / 0B-2 | Done at 0e3c6b342; actual version/load-completion walks, measured/pinned generated-MLEADER fallback, no fixture or production changes. | Both 27-type walks and both DXF pins; full/DXF-fast/ASan coverage, full suite/lanes pass; every baseline testcase/skip retained. R06/R11/R13. |
| 1A-1.b-ref-frames / .b-progress | Done at 26d107ccf; immutable scaled frame, source/offset jets and point/interval enclosures, checked conversions and shared latched budget. | Seven new cases / 359105 assertions Release/ASan; independent 113-bit oracle, all geometry/offset/full/fast-lane checks pass. R01/R02/R12. |
| 1A-1.b-ref-events / .b-ref-frames | Aggregate complete at component checkpoint `9682d5cac65f30cfd86aa10556678ceb88b1448b`; all required children committed. | No separate aggregate implementation commit or partial event-success API; next work belongs to `.b-check-layout`. R02/R12/R13. |
| 1A-1.b-ref-events-core / .b-ref-frames | Done at 834a46a88; strict interval/contact/injectivity proofs, bounded spatial/range work and global occurrence ordering. Defaults unchanged. | 14 registered cases /16 JUnit entries /634 assertions Release/ASan, independent probes and broad/fast gates pass; failures/resolutions below. R02/R12. |
| 1A-1.b-ref-rate / .b-ref-events-core | Done at bcd6c5a4f; continuous ideal-angle rate, unwrapped angle/rate access and all jet/product factors; no adapter/topology yet. | Six cases/505703 assertions Release/ASan, independent 113-bit oracle, full Release/combined ASan/both fast lanes pass; failure correction and hashes below. R01/R02/R12. |
| 1A-1.b-ref-events-critical / .b-ref-rate | Done at 943406abe; geometry header/cpp/tests: shared F/F' bounds, factory-owned adapter, exact thresholds/cusps and gap-free proven regions/joint exclusions. | Nine cases/268244 assertions Release/ASan; independent proofs/oracles, all broad/fast gates, failures/fixes/hashes below. R01/R02/R12. |
| 1A-1.b-ref-events-accounting / .b-ref-events-critical | Done at290d2658f; three entered adaptive-depth fields and max helper, retained on every exit; decisions/counters unchanged. | Five cases/153 assertions Release/ASan, old strict/budget union32/2194 and all broad/fast gates pass; new test expectation corrected from legacy trace, no production change. R02/R12/R13. |
| 1A-1.b-ref-events-refine / .b-ref-events-accounting | Done atf4f87f48e; intersection header/cpp/existing tests: owned immutable provider/options, opaque handles, original-proof-preserving contraction and persistent history/work/latches. | Fourteen cases/490 assertions Release/ASan; old core37/2347 and all broad/full/fast gates; source/commit digests match. R02/R12/R13. |
| 1A-1.b-ref-critical-refine / .b-ref-events-refine | Done at72eb5f953; geometry header/cpp/tests: factory-issued canonical polynomial/angle state, occurrence handles, convergent contraction/images, observed-depth repair and bounded one-sided join progress. | Native red retained; final21/16424 Release/ASan, legacy11/268911, core51/2837 and all broad/full/fast gates pass, tested/committed hashes match. R01/R02/R12/R13 sub-contract only. |
| 1A-1.b-ref-providers / both refinement children | Done at9c27e2980, geometry header/cpp/tests: owned immutable offset/cutter geometry, both-hook maps, support exclusion and exact collapsed-opposite omission; logical costs/debits qualified. |16/299294 Release/ASan, critical30/284668,core51/2837, broad/full/fast gates pass; no old body/assertion/cap changed, committed digests equal tested source. R01/R02/R12/R13 provider sub-contract only. |
| 1A-1.b-ref-contacts / .b-ref-providers | Done at92ca5e445; immutable complete-box construction contact proofs, exact F/critical-free-gap qualification, bounded radial neighborhoods, unchanged legacy factory. |17/4383 both builds,old97/586799,broad/full/fast gates pass; all digests/identities retained. Native successful interior fallback not claimed. R01/R02/R12/R13 contact sub-contract only. |
| 1A-1.b-ref-events-isolate / .b-ref-contacts | Done at dff307668; geometry header/cpp/tests expose an owned immutable complete-event result, retained strict session/providers, diagnostic IDs, role/parameter ordering, exact-only same-role identity, rounded world-node enclosure and one irreversible work ledger. | 15 focused cases/5616 assertions Release+ASan; 154 geometry cases/1464224 assertions Release+ASan; full Release 3842 cases/7321304 assertions/74 unchanged skips; DXF and DWG fast lanes pass Release+ASan. Construction/free-end and stationary/free-end identities pass. Tangent stress reaches strict-query depth20 before ordering; it asserts bounded empty refusal only, not identity resolution. At this checkpoint, multi-pair shared-own success was not separately pinned; the following `.b-ref-components` slice added that proof. R02/R12. |
| 1A-1.b-ref-components / .b-ref-events | Done at `9682d5cac65f30cfd86aa10556678ceb88b1448b`; exact-D visibility, complete event fringes, incidence-preserving components and atomic budget refusal are implemented. Keep engine rho/marginal policy out of this exact reference; qualify compatibility in `.c`/`.b-check`. | Attempt-8 focused Release/ASan, reference and engine regressions, full Release, and DXF/DWG fast lanes pass; the finite-distance conversion P1 was independently re-reviewed after its fix. Attempt 6 remains historical red and attempt 7 pre-fix green. Frozen/committed hashes and receipts are in §0.5. `.b-check-layout` is the next leaf after activation review. R02/R12/R13. |
| 1A-1.b-check-layout / .b-ref-components | Done at `37462efc8`; bounded degree-1/degree-3 ClampedOpen layout inspection, checked formulas, exact knot-group/finite planar/unit-weight validation, actual parameter-interval span extraction, atomic shared-budget admission, and checker-only cap rename. No public geometry/topology-validity claim. | 16 registered cases/3,311 assertions pass Release/ASan; adjacent 94/754,288 both builds; full Release 3,874 cases/7,346,172 assertions/74 unchanged skips; DXF/DWG fast Release/ASan pass with 3/4 expected skips. Independent review clear; frozen hashes and reports in §0.5. Bounds is also complete at `ce1ed5192`; the local-topology successor is gated by v26 reconciliation, and global chain remains a later child. R01/R02/R12/R13. |
| 1A-1.b-check-bounds / .b-check-layout | Done at `ce1ed5192`: proof-only bounded cache/reference subcurve comparisons using exact degree-1 chords, interval de Casteljau cubic error, exact-offset jet interpolation, outward chord Hausdorff bounds, restricted-source clearance and complete event-node/fringe witnesses. Preserve all edge/node alternatives; no parameter correspondence, injectivity or public-validity claim. | 25 focused test cases/46,104 assertions Release/ASan; adjacent selector lists 110 test functions and its JUnit receipts record 145 invocations/757,599 assertions; full Release 4,793/7,392,350 with 74 baseline-identical skips; DXF/DWG fast Release/ASan with baseline-identical 3/4 skips. The independent rational interval oracle encloses complete noncircular Q ranges, chord residuals and all four returned endpoint fields. Exact/one-less/pre-spent/sticky caps refuse without partial witnesses. R02/R12/R13. |
| 1A-1.b-check-topology-local / .b-check-bounds | Validated from v26 product base `fd4697059`; product commit and v29 checkpoint pending | Shared geometry/clearance pair proof, strict local injectivity, exact endpoint pairing, genuine root-fringe incidence certificates, whole-cache reversal and one retained budget. Same-image folded cubic, constant span and malformed/cap cases refuse atomically. This leaf exposes no global/public cache validity; exact gates and receipts are in its active card. R02/R05/R12/R13. |
| 1A-1.b-check-topology-chain / .b-check-topology-local | Waiting for local product commit/checkpoint and fresh readiness review | Prove one component, exact ordered non-reusing chain, complete two-way coverage, event transitions, open/cyclic closure and reason mapping; only this leaf exposes `checkCache`. Proper subset, missing tail, extra loop, crossing branch switch, wrong reversal, ambiguous cycle and cap exhaustion refuse atomically. R02/R05/R12/R13. |
| 1A-1.c / all .b children | Qualify defaults and tolerance-floor policy against current engine/adversarial corpus; record seeds, statuses/work/cost and resolve required-success disagreements. Retain the source-qualified interior-fallback native reachability coverage limit; investigate natural cases or prove limits without a test backdoor. | Existing offset/topology regressions, independent geometry oracle, every cap exit, near-critical success, file-rounding margins; no timing assertion or unearned fallback-coverage claim. R01/R02/R12. |
| 1E-2.safety / all 1E-1 children | Add schema/raw-carrier hostile cases, unknown-tail/flags edit policy, geometry demotion and late-failure save tests. Scope schema/filter fixes only to demonstrated violations. | Both formats, second save/reload, foreign data unchanged, raw 3D/repair inputs never editable; transaction checks for existing/absent target and document state. R07/R08/R14/R15. |
| 1E-2.fixtures / .safety | Commit independently authored input fixtures for every schema-capable application reader version, each covering degree-1 and degree-3 cache carriers; include input-only legacy DWG R13/R14, binary DXF R14/R2018 and R12 geometry-only input. Record reproducible provenance, producer/version and expected definitions. | Every reader version consumes a checked-in fixture not emitted by LibreCAD's writer under test; both degrees are asserted. Separately inspect writer output; immutable hashes/provenance, fast-lane registration, full and both fast lanes. Missing external fixtures leave their reader rows explicitly incomplete. Same-run writer/reader payloads alone do not close reader coverage. R07/R11/R14. |
| 1E-2.copy / .fixtures | Test same/cross-unit paste, block/library insert, Save Block As, clone-tail alias safety and hatch-edge downgrade. | Geometry/distance scaling, owner/layer/pen preservation, correct editable-vs-plain outcome, no stale owned definition; Release/ASan ownership checks. R04/R07/R08. |
| 1E-3.matrix / all 1E-2 children | Execute explicit 14-row reader and writer matrix with exact-version fixtures; include editable degree-1 and degree-3 carriers, plus separate input-only legacy DWG R13/R14 receipts; second-generation reload, binary R14/R2018 codec and no-DWGSUPPORT schema compile. | Separate per-version read/write receipts and degree/control/knot/weight/definition/payload oracles; all six DWG writer rows executed, legacy readers separately pass, R12 degree-1/degree-3 geometry downgrades to POLYLINE without schema; full suite/fast lanes; no silent valid-file demotion. R07/R11/R14. |
| 1E-3.conversion / .matrix | Run both fan-out conversion directions for degree-1 and degree-3 caches, repeated alternating chains and edited/refused reopen. Run available independent readers; distinguish unavailable from passing. | Counts for every conversion leg and degree, no duplicate schema/drift/stale definition, mutation remains usable after reload; committed external-tool results or explicit unavailability. R07/R08/R15. |

Other executable leaves retain their ordered tasks and files in §§5-6. For 1A-2, the enum, dispatch, INSERT
capability, guard row and interim action guard must land together as one buildable slice. For 1G-2.a, Changed /
Unchanged / Refused, pending-clone ownership, timer generation and before/after history hooks form one coherent
generic infrastructure slice with its regression tests; do not commit the unsafe queue first. Split either only by
documenting independently safe, buildable intermediate behavior before editing. Registration and tests accompany
each product slice, not a later catch-all test phase.

**Immediate implementation-resume queue:** `.b-ref-events-isolate` is committed at `dff307668`; `.b-ref-components`
is committed at `9682d5cac65f30cfd86aa10556678ceb88b1448b`; layout is committed at `37462efc8` and reconciled at
`6f25975f6`. `.b-check-bounds` is committed at `ce1ed51924d20a708e8bec059f7f9dd46235a232`, parented by the v24
endpoint-contract checkpoint `a0ce67a3b8746770ce49fb1ff76616c9da3d23fe`; its v25 plan-only reconciliation is verified
at `4ca63677c27043004aae45e0fe54d42db48664a5`, and the v26 plan-only topology activation is `fd4697059572af28c08ecbb532e687af1603ee1c`.
The product worktree is now at implementation commit `ef3ba756b52796c02b8490caedcaa48b8db69605`; only this v29
roadmap is modified, and the pre-existing untracked `build-asan/` is preserved. The first focused attempt and earlier
invalid event fixture remain preserved as historical red evidence. The current 20-case Release/ASan selector, bounds
and adjacent regressions, full Release, both DXF/DWG fast lanes in Release/ASan, qmake6 application build and
independent review pass; exact reports and digests are in the active card. This roadmap is the immediate one-file
direct-child checkpoint for the product SHA; verify its committed parent, path scope, digests and worktree before any
next action. `.b-check-topology-chain` remains
waiting for a fresh actual-source/API/completeness/readiness probe; this checkpoint alone does not activate it. The bounds witness relation does not prove topology
or authorize the public checker. Do not repeat or amend earlier component/layout/bounds slices.
Do not restart or reapply contacts/providers/
refinements/isolation. Budget, guard, progress and frames are complete at 10c6bff46 / 0e3c6b342 / fb70681e2 /
26d107ccf; strict core, rate, critical preparation and observed-depth accounting are complete at 834a46a88 /
bcd6c5a4f / 943406abe / 290d2658f; trusted pair refinement is complete at f4f87f48e and critical refinement at
72eb5f953. Failed and final evidence remain under `build/evidence/<slice-id>/`. Keep each build directory
single-writer and use actual HEAD/files if the current source changes.

The v8 review was plan-only. Its implementation continuation completed core refinement from2e75f4c7a and critical
refinement from334354633, after literal API/cost probes, native red reproduction, independent reviews and all gates.
Gibbs's read-only isolation readiness review identified the additive provider prerequisite, now implemented and
natively verified/committed at9c27e2980. Its whole/restricted visibility-algebra review also qualified a bounded future
components proof option (§3.5.1), not a product implementation. Construction-contact child is now implemented and
natively verified/committed at92ca5e445. Historical notes below retain the evidence and limitations at each earlier
checkpoint; they are not the current queue. The current cache-validation children remain waiting until their exact
predecessors, producer contract and public API handoffs are verified, not merely because a recipe is written.

### 0.7 Review judgments within the five-gate cycle

The operational gates and their order are defined in §0.5. This section makes the three review judgments independently
auditable: completeness and plan correctness are separate results inside operational Gate 1; implementation correctness
is operational Gate 2. Readiness, product commit and self-reconciliation are operational Gates 3-5, not substitutes for
any review judgment. A single undifferentiated "reviewed" note does not satisfy them.

1. **Completeness review (before and after implementation).** Before coding, map each applicable R01-R15 requirement
   to implementation sites, callers, representations, format/version paths, build lists, test registration, success,
   refusal/no-op/fallback cases and durable evidence. Name excluded scope and its owning leaf. After coding, reconcile
   that map against the actual diff and verify no caller, codec branch, build target, ownership path or promised test
   was missed. A placeholder, skipped required format, unreachable test or absent evidence leaves the requirement
   open, even when compilation passes.
2. **Plan-correctness review (before coding, then rechecked).** Verify every requirement and acceptance rule against
   current source, supported DXF/DWG versions, repository APIs, specifications/compatibility evidence and the dependency
   graph. Challenge assumptions against declarations, callers, actual codec behavior and available independent
   readers. Check that tasks are feasible, boundaries are buildable, tests can distinguish success from refusal, and
   contracts do not contradict each other, promise an impossible API outcome or omit a required fallback. Resolve
   contradictions in the roadmap before declaring the leaf ready; after implementation, recheck any rule affected by
   observed behavior or test evidence.
3. **Implementation-correctness review (after implementation).** Independently challenge the frozen diff and algorithm:
   raw-vs-normalized input, finite intermediates, conservative error direction, ambiguity, whole-component coverage,
   counters before allocation, lifetime/history, refusal transaction and saved payload. Trace success and adversarial
   refusal through the real caller. Use an independent falsifying oracle, not the function under test's own output.

The completeness and plan-correctness results jointly decide whether Gate 1 is closed and work may become ready; the
independent implementation-correctness result plus final diff reconciliation are required before Gate 4 can commit.
Record each result separately, including reviewer, repository revision, frozen source identity when applicable, scope,
findings, resolutions, residual risk and retests. Keep plan-correctness and implementation-correctness distinct even if
one reviewer performs both passes. A second agent may perform the implementation review; when unavailable, perform a
separate deliberate pass after implementation, not merely another green test run. A new finding gets an explicit
fix/retest and can reopen a previously done requirement. The checkpoint must reflect that reopening; it must never
rewrite a failure as a baseline exception. Gate 5 does not retroactively close missing review evidence: it records the
actual status, preserves the finding and leaves the owning leaf incomplete until fixed and re-reviewed.

Each finding records severity, current source anchor, failing invariant, counterexample/oracle, owning fix slice and
required retest. The recorded review names the reviewer, tested tree SHA/digests, scope inspected, findings and their
disposition. Classify evidence explicitly as source inspection, declaration compilation, native reproduction or
verified regression; none substitutes for another. For numerical/geometry work, state why each enclosure or error
bound is conservative, how all domains/branches are covered, and which independent oracle can falsify it; sampled
oracles are regression alarms, never a replacement for a completeness proof. For serialization, inspect actual
bytes/records and reload semantics, not only writer success. A residual risk that violates a required success/refusal
contract blocks readiness or completion. An independent reviewer must read changed behavior and negative paths, not
only the implementation summary. Documentation status, test-registration counts and graph consistency are necessary
checks, not proofs of numerical correctness or runtime feature coverage.

For topology graphs built only from retained own spans, separately audit every own-role event occurrence whose node
has zero retained incidence. Opposite/cutter-only event nodes are not graph vertices and must not trigger refusal.
For each own-role zero-degree node, prove it is non-retained, publish an explicit singleton component if the public
contract supports that representation, or refuse the complete analysis with a named ambiguity reason. Never let an
edge-only connected-components pass silently discard a geometrically retained isolated point. Include source-end/cap
tangency, pair tangency and exact equality cases, including restricted-source arcs not covered by a whole-source
special case.

### 0.8 Immediate validation commands

Run from the authoritative worktree, after freezing and isolating the selected slice as §1.4 requires. Create its
ignored evidence directory first. Use new attempt-specific report names instead of overwriting a failed run. The
following commands are reproducible recipes, not evidence by themselves. The budget slice has completed its
final runs recorded below, as have 0B-3, `.b-progress`, `.b-ref-frames`, strict core, rate, critical, accounting, refinements, providers and contacts;
do not rerun them merely to preflight `.b-ref-events-isolate`. Choose fresh report names
if reproducing a run:

```bash
cmake --build build --target librecad_tests librecad_dxf_fast_tests librecad_dwg_fast_tests --parallel 4
cmake --build build-asan --target librecad_tests librecad_dxf_fast_tests librecad_dwg_fast_tests --parallel 4

# 1A-1.b-budget: nine registered cases, including the existing depth-limit case.
build/librecad_tests --list-tests "[intersection-budget]"
QT_QPA_PLATFORM=offscreen build/librecad_tests "[intersection-budget]" --order decl --reporter junit --out build/evidence/1A-1.b-budget/release-attempt-1.junit.xml
QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[intersection-budget]" --order decl --reporter junit --out build/evidence/1A-1.b-budget/asan-attempt-1.junit.xml
QT_QPA_PLATFORM=offscreen build/librecad_tests "[curve-offset]" --order decl --reporter junit --out build/evidence/1A-1.b-budget/offset-attempt-1.junit.xml
QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[curve-offset][topology],[curve-offset][ellipse]" --order decl --reporter junit --out build/evidence/1A-1.b-budget/adjacent-asan-attempt-1.junit.xml
QT_QPA_PLATFORM=offscreen build/librecad_tests --order decl --reporter junit --out build/evidence/1A-1.b-budget/full-attempt-1.junit.xml

# 0B-3: three new registered cases; 27 rows per format walk and both pin branches.
build/librecad_tests --list-tests "[entity-coverage][roundtrip]"
QT_QPA_PLATFORM=offscreen build/librecad_tests "[entity-coverage]" --order decl --reporter junit --out build/evidence/0B-3/release-attempt-1.junit.xml
QT_QPA_PLATFORM=offscreen build/librecad_dxf_fast_tests "[entity-coverage]" --order decl --reporter junit --out build/evidence/0B-3/dxf-fast-coverage-attempt-1.junit.xml
QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[entity-coverage]" --order decl --reporter junit --out build/evidence/0B-3/asan-attempt-1.junit.xml
QT_QPA_PLATFORM=offscreen build/librecad_dxf_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/0B-3/dxf-ci-attempt-1.junit.xml
QT_QPA_PLATFORM=offscreen build/librecad_dwg_fast_tests '~[.]' '~[.slow]' '~[slow]' '~[corpus]' '~[external]' '~[external-reader]' '~[fixture]' --order decl --reporter junit --out build/evidence/0B-3/dwg-ci-attempt-1.junit.xml
QT_QPA_PLATFORM=offscreen build/librecad_tests --order decl --reporter junit --out build/evidence/0B-3/full-attempt-1.junit.xml
```

Serialize binaries within each build directory. Record the process exit before parsing the report; missing/truncated
reports, timeouts, sanitizer diagnostics, unexpected skips or unmatched selectors fail the check. Parse XML with an
XML parser and retain stable failure identities, not only totals. A focused test path being intentionally `notRun`
in the baseline guard is recorded as unverified, never included in the passed count. The new type may have no such
exception. Later cards expand this command pattern with their exact selectors/version rows and broad gates.

---

## 1. Ground rules

### 1.1 Workspace and builds

- **Worktree and branch.** At S0, inspect `git worktree list` and branch state. Reuse only a clean worktree based on
  the fetched target branch; otherwise create a dedicated `codex/offset-curve-2961` worktree. The original seed
  resides in a research worktree behind the execution baseline; do not use that checkout for product edits. Record the
  absolute worktree path and base SHA in the status table.
- **Build directories** inside it: `build` (Release) and `build-asan` (Debug + ASan).
  - Record the actual compiler, Qt, CMake and Ninja versions. The old review used Qt 6.12.0; it is not a requirement to
    upgrade the machine. Configure fresh directories for the selected checkout/toolchain.
  - Release: `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DQt6_DIR=/opt/homebrew/lib/cmake/Qt6`
  - ASan: same, plus `-DCMAKE_BUILD_TYPE=Debug`, and `-fsanitize=address -fno-omit-frame-pointer` in C, C++ and linker
    flags.
- **Targets.** Build them by name: `librecad_tests`, `librecad_dxf_fast_tests`, `librecad_dwg_fast_tests`. The `all`
  target builds no tests.
- **Shared machine.** Other sessions run on this machine: build with `-j8` at most, and never run two test binaries
  from the same build directory at once. Eight is the combined job cap, not eight per concurrent build. One owner
  controls shared build directories; freeze source changes during final evidence collection.

### 1.2 Tests

- **Run** as `QT_QPA_PLATFORM=offscreen build/librecad_tests "<tags>" --order decl --reporter compact`.
- **Regression gate.** No new failing test identity relative to S0. Record test name, section/assertion and failure
  reason using Catch2 XML/JUnit output and a structured parser; source line numbers are navigation only and may move.
  A verified fix may remove an old failure. Case-count reductions, new skips or changed baseline expectations require
  an explanation and review; never rebaseline merely to hide a failure.
- **Tags.**
  - Phases 1–2 use `[lc-offsetcurve]`, plus one area tag: `[geometry]`, `[class]`, `[grips]`, `[edit]`,
    `[modification]`, `[sites]`, `[files]`, `[ui]`, `[grip-drag]`, `[option]` or `[update-source]`.
  - Hidden GUI tests use `[.gui][lc-offsetcurve]`.
  - 0B uses `[entity-coverage]`.
  - Do not use `[offset-curve]` (easily confused with the separate existing `[curve-offset]` family), nor the existing
    `[trim]` or `[source]` for new areas.
- **Fast lanes.** `librecad_dxf_fast_tests` and `librecad_dwg_fast_tests` are processes with only a
  `QCoreApplication`. CI builds and runs them with ctest on Linux GCC and Windows MSVC x64 and arm64, with the excludes
  `"~[.]" "~[.slow]" "~[slow]" "~[corpus]" "~[external]" "~[external-reader]" "~[fixture]"` (`CMakeLists.txt:2541-2543`).
  - A step that adds a file to a fast-lane list also builds and runs both lanes with these excludes.
  - Never tag a CI test `[fixture]`, `[slow]` or `[external]`.
  - In a file that runs in a fast lane, never call `lc::test::application()` (it creates a `QApplication`) and never
    include `test/lc_actiontestsupport.h`.
- **Filter and file tests** use `test/lc_filtertestsupport.h`:
  - `lc::test::ensureSettings()` :59, `QuietLog` :82, `OutputDirectory` :99;
  - `importDrawing` :141, `exportDrawing` :165;
  - `dxfRecords` :226, `EditableDrawing` :323.

  Fonts come from `lc::test::SourceFonts` (`test/lc_sourcefonts.h:42`). Template: `Bench` in
  `lib/filters/tests/lc_attribute_export_tests.cpp:74-80`. Do not copy a file-local `ensureSettings()`, and do not
  reuse `roundTrip()` from `spline_offset_representation_tests.cpp`: its DWG range check misclassifies
  `FormatDWG2007`. Wrap import and export loops in `QuietLog`, because the filter turns libdxfrw tracing on per file.
- **Headless action tests** (`lc::test::ActionFixture`, `test/lc_actiontestsupport.h:102-116`) live in
  `librecad_tests` only.
- **No timing asserts.** Test binaries run at `RS_Debug::D_DEBUGGING`.
- **No empty test success.** Discover each selector in the exact target first (`--list-tests "<selector>"`), then
  run it and inspect the structured report. Record registered cases separately from assertions, generators and JUnit
  entries; Catch2's JUnit `tests` attribute is not the registered-case count. Require nonzero actual feature cases
  and the expected per-version/per-type paths. A missing hidden GUI tag, compiled-out DWG case or unmatched selector
  is a failed validation setup, not a green test run. Hidden GUI selection is `[.gui][lc-offsetcurve]`.

### 1.3 Code conventions

- **File names.** New files use the `lc_` prefix.
- **Build lists.** New product sources go into `CMakeLists.txt` (product list around :879-887) **and**
  `librecad/src/src.pro` (HEADERS around :450, SOURCES around :1028, FORMS around :2337-2350).
  - `.ui` files are not listed in `CMakeLists.txt` (AUTOUIC) but are listed in FORMS.
  - After editing a `.ui`, touch the `.cpp` that includes its `ui_*.h`, or its object is not rebuilt.
- **Names vs glibc 2.17.** Because of the Pixi Linux sysroot, no function or test helper may be named like a libc or
  POSIX symbol (`wait`, `group`, `near`, `open`, `y0`, `y1`, `yn`, `j0`, `j1`, `jn`, …). This applies to product
  code too.
- **Catch2.** Write `GENERATE(from_range(std::begin(a), std::end(a)))`, never `from_range(array)` (fails on Catch2 3.4).
- **Variants.** Write `DRW_Variant(1070, std::int32_t{...})`.
- **Formats.** Never test DWG formats with a range check.
- **Pens.** Copy whole pens: `setPen(src.getPen(false))`.
- **Messages.** Code in `lib/` builds no user-visible `QString`. It returns enums; the texts live in the UI layer (§3.1).

### 1.4 Commits and the plan file

Local commits after verified slices are part of implementation. S0 first copies this reviewed file into the selected
implementation worktree at `docs/plan_offset_curve_2961.md` and tracks it. That checkout's file becomes authoritative;
record its absolute path here. The research copy is retained as the starting snapshot, not maintained as a second live
ledger. Do not depend on a separate working plan under `~/.claude/plans/`.

**Commit boundary:** one named executable leaf is one product commit, containing its coherent implementation, tests,
and necessary registrations. Do not bundle an independent downstream leaf to save a commit. If a leaf cannot remain
buildable as written, revise it into smaller named children before editing. Each product commit is followed
immediately by exactly one plan-only reconciliation commit; do not defer reconciliation to a parent milestone or
combine it with the next product change. A leaf may contain multiple ordered internal tasks, but it has one acceptance
contract and one implementation commit. If part of the work is independently shippable, buildable and testable, split
it into named child leaves before beginning that work; do not commit a passing subset under the parent's identity.
Every implementation commit closes exactly one executable leaf (not an aggregate and not several leaves bundled
together). For an in-flight leaf, its product commit must be based on the committed plan checkpoint from which that
leaf was activated; uncommitted roadmap edits are excluded and reconciled only after the product commit. Do not start
or stage the next product leaf until the previous leaf's plan-only checkpoint is committed.

For every step or named child slice:

1. **Prepare.** Reconcile actual HEAD and relevant changes with the roadmap; complete its ready card. Record explicit
   file scope, contract, commands and expected test cases before implementation. Keep unrelated user edits untouched.
   Select an executable leaf, never an aggregate. Confirm every required predecessor's implementation and plan
   checkpoint in Git; declaration probes and prepared patches do not satisfy dependencies. Assign one product owner
   and one validation/Git owner. If no leaf is ready, repair the earliest unmet prerequisite without reducing scope.
2. **Verify and review.** Implement, run focused and required adjacent checks, inspect the full scoped diff, and resolve
   all three §0.7 review gates. Record actual exit codes, case counts, skips, artifact paths and the tested tree. Mark
   `validated` only while that evidence still matches the files being committed. Any required failure, unexpected
   skip, missing case, unresolved required finding at any severity or changed source digest keeps the slice unvalidated;
   preserve the run and fix/retest the same slice. Do not create a product commit to make progress bookkeeping easier.
3. **Commit the implementation.** Inspect the index; stage owned product/test/build files by explicit path, and commit
   one coherent slice with the repository's sentence-title style. For a documentation-only slice, commit its scoped
   documentation instead. Check `git show --stat HEAD` and capture `git rev-parse HEAD`. Hooks must pass; a failed commit
   leaves the slice uncommitted and must be repaired without `--no-verify` or silently widening scope.
   Verify that the committed product tree matches the tested one. If hooks changed code or unrelated unstaged source
   affected the build, validate the committed slice in isolation before marking it done; preserve the other edits.
   Prefer a clean temporary validation checkout of the candidate commit when feasible. If temporarily parking an
   owned patch is necessary, save its forward/reverse patch and hashes first, apply only the owned changes, and
   verify exact restoration. Never park another contributor's work or mistake a stale already-committed patch for
   pending work. A mixed-tree build is exploratory evidence, not proof that either isolated commit passes.
   A pre-existing staged change is not implicitly authorized for this commit. Use a clean index in the dedicated
   worktree or preserve its ownership and stop the commit operation until it can be isolated; never unstage another
   contributor's files as cleanup. Record the tested source-tree identity (base SHA plus scoped diff/digest for an
   uncommitted test run), not just the SHA from which editing began.
4. **Self-update after the commit.** With the real implementation SHA now known, reread the changed code and reconcile
   the plan: mark the child `committed — reconciliation pending`, update parent progress, record test/evidence
   identities and the date, correct invalid assumptions/anchors/estimates, add any discovered follow-up slice,
   and recalculate downstream readiness. Invalidate stale test evidence when its source/dependency/toolchain changes;
   carry it forward only with an explicit unaffected-scope explanation. Record the next slice's remaining API or
   mathematical prerequisite, not a promise to decide it during coding. Increment the plan version/date and append a
   concise execution-history entry; synchronize the header status, status table, parent aggregate, requirement matrix,
   executable recipe, immediate queue, and next card so there is one current account. Fill the
   next ready card or the precise unresolved prerequisite. Prepare its `done` record in the checkpoint; it becomes
   effective only when step 5 succeeds. Do not mark a parent done while required children remain.
5. **Commit the plan checkpoint.** Commit only the reconciled roadmap, for example
   `Record offset curve slice 1B validation and next steps`. This checkpoint records the implementation SHA, not its
  own SHA. It must be a single-parent direct child of the implementation commit, not merely a later descendant, and
  it does not trigger another checkpoint, which avoids recursive self-referential commits. Check its diff and the
  worktree status; the slice becomes `done` only when this reconciliation is durable. If this checkpoint
   fails, stop product work and repair it while preserving the implementation commit and all pending roadmap edits.
6. **Continue when implementation is authorized.** Start the next ready slice automatically during an implementation
   run. A successful commit is a checkpoint, not the end of that run. A newer plan-only, review-only, stop or pause
   request controls this transition; updating readiness does not itself authorize new product edits or publication.

**Mandatory post-commit reconciliation checklist (steps 4-5).** Verify the implementation SHA exists and contains the
intended paths; link it from the executed card/ledger; record commands, exit codes, case counts, skips, baseline
differences and each review outcome; update every affected parent and requirement-coverage row; correct the current
API/file/build anchors; record any newly discovered fix as a named leaf with dependencies; replace the next card's
stale base SHA with actual HEAD; and refresh the top-level status. Recompute readiness from actual predecessor
implementation and checkpoint SHAs, not from the prior queue. Check that the dependency graph is acyclic, every
waiting leaf names a real unfinished predecessor, every executable leaf has a recipe and acceptance evidence, and
every done parent has all required children done. Reconcile the status table, parent aggregates, R01-R15 coverage,
format matrix, executable recipe, immediate queue, evidence ledger, current card and version/history together; remove
stale `ready`, `done`, or validation claims. Run `git diff --check` and the available structural inventory/DAG checks;
if no automated checker exists, perform the same checks explicitly and record that limitation. Review the plan-only
staged diff before committing it. A plan edit made before the
implementation commit cannot stand in for this post-commit check. A checkpoint changes no product files and does not
require a redundant full test run. Its own SHA is discoverable from Git history and is not written into itself.

This reconciliation is a code-to-plan audit, not copy-forward administration: compare the committed diff with the
card's promised scope and actual callers; remove claims disproved by implementation or tests; add newly exposed
requirements/findings; and recompute readiness from actual predecessor implementation and checkpoint SHAs. The plan
checkpoint must leave no stale `ready` label, placeholder activation SHA, completed parent with unfinished children,
or next-step handoff based only on a passing subset.

**Checkpoint postconditions.** Inspect `git show --name-only HEAD` to confirm the checkpoint changes only this plan;
confirm `HEAD^` is exactly the implementation SHA (the checkpoint is its direct child) and committed file hashes match
the tested slice; inspect
`git status --short` and account for every remaining change. Keep a durable compact summary in this file (commands,
exit/count/skip results, findings and next entry check); ignored reports supplement it, not replace it. The next card
records the checkpoint's actual HEAD when activated, avoiding a self-referential placeholder SHA. A plan-only review
such as v7 needs one documentation commit, not a fictitious implementation commit or recursive checkpoint.

**Recovery by actual state, not elapsed time.** Before resuming, inspect HEAD, index, scoped diff and owned process
identifiers. A yielded build/test/agent wait is still running, not a failure: resume that process instead of starting
a competing run or parsing its partial report. If source changed during a final run, label the run exploratory and
freeze/retest the affected tree. With validated uncommitted files, recheck digests/index and finish step 3; with an
implementation commit but no plan checkpoint, finish steps 4-5; with a durable checkpoint, select the next leaf.
Preserve failed reports and pending edits. Neither a timeout nor a missing checkpoint authorizes restarting old
implementation, reverting another contributor's changes, increasing a cap or weakening an assertion.

Keep validation evidence for the entire slice; do not cherry-pick the final green run while hiding failed or skipped
required cases. Resolve the failure, record the cause/fix and rerun affected checks. If a verifier is unavailable,
record it as unavailable, never as passing. Required native tests/builds are gates; optional external tools and user
publication actions are separately labeled and cannot be used to claim those native gates passed.

If interrupted after step 3, record `committed — reconciliation pending` and recover by inspecting HEAD and the scoped
diff; finish steps 4–5 before more product work. Never reimplement or revert a successful slice because its checkpoint
is missing. If a checkpoint commit fails, preserve both the implementation commit and pending plan changes and repair
the checkpoint. A subsequent correctness discovery reopens the parent and creates a new fix slice; it does not justify
amending published history or concealing the finding.

Use this compact ledger under each executed card; expand it only with meaningful evidence. Its parent/next column
records the handoff at that historical checkpoint; §0.5/§0.6 give the current queue, so never reapply an old recovery
patch simply because an executed card once described it as pending:

| Slice | Implementation SHA | Tested tree / evidence | Review outcome | Parent / next readiness |
|---|---|---|---|---|
| S0 (documentation only) | de30f1d45f1337d55265c2f72413a9c976c18c40 | Product tree b05fb541d unchanged; all S0 builds/reports passed with existing skips documented | Coordinator/Gibbs review resolved; staged/committed paths only this roadmap | S0 done; 0B-1 ready for Gibbs; product steps pending |
| 0B-1 | d6de53d3cb80199b4148b0e6d3dbb28f2b392b8c | `build/evidence/0B-1/summary.md`, owned-file hashes, Release builds/tags/lanes, positive and two negative syntax controls | Coordinator reviewed all three committed paths; no unrelated geometry files staged | Guard partial (0B-1 done); 0B-2 ready; 1A-1.a independent/in progress |
| 1A-1.a | a4690db08b7220e93bbc8642d2c2b8b2e14fcf30 | Isolated Release/ASan geometry 17/2538; existing ellipse 48-case regressions; both Release fast lanes; CMake/qmake registrations | Gibbs's numeric findings fixed with regressions; committed paths match final tested files; 0B-2 patch safely preserved outside this commit | Geometry parent partial; .b-budget ready; 0B-2 final verification next |
| 0B-2 | 5bfb3c8f08462694796489166f0146dd51fc7ca0 | `build/evidence/0B-2/*-final.junit.xml`: full/DXF-fast/ASan coverage 7/910, both Release fast lanes pass | Fixture-order failure corrected rather than masked; 27 types pass clone and both INSERT shapes; no gaps/skips in feature tag | Guard partial; 0B-3 ready; .b-budget independent/in progress |
| 1A-1.b-budget | 10c6bff46976938492c5d1ad5b5e74001d6b2d03 | Isolated three-file tree, nine cases/1560 assertions Release/ASan, all offset/full/fast-lane gates; source hashes and failure history in card/summary | Gibbs/coordinator reviewed spatial/range coverage, branch isolation, arithmetic and boundary safety; no cap increase or geometry expectation change | Geometry partial; .b-progress ready after checkpoint; restore/verify 0B-3 next |
| 0B-3 | 0e3c6b342a0edefe06847c33d775f756c31fe674 | One owned test file, 10/1335 full/DXF-fast/ASan coverage, full Release/CI lanes pass; 27 rows per format and two DXF pins | Generated-MLEADER DWG fallback source-confirmed/pinned; no factory/production changes, no hidden export/import/geometry failure | Guard complete with one declared baseline gap; .b-progress ready next, 1A-2 still waits for complete geometry |
| 1A-1.b-progress | fb70681e2eecd7ba937c7942bed8de129425a3aa | Two owned cpp/test paths; four cases/80 assertions Release/ASan; fixed standalone probe; all offset/full/fast-lane checks pass | Coordinator reviewed finite arithmetic, binary64 growth bound, no partial roots and ordinary rounded-growth success | .b-ref-frames ready; geometry/reference/check parent still partial, no entity/UI/file completion claim |
| 1A-1.b-ref-frames | 26d107ccfc863e58b3b912ab7fbe591588814e2d | Three owned paths; seven new cases/359105 assertions Release/ASan; all geometry/offset/full/fast-lane checks pass, hashes/card below | Coordinator independently reviewed interval remainder, scaling, derivative/product identities, clearing and irreversible budgets | .b-ref-events-core ready; events split into three coherent children after current-code preflight; no component/adoption/entity completion claim |
| 1A-1.b-ref-events-core | 834a46a88e2b017dc473f75ff86ad64aad99e6fe | Three owned paths; 14 registered strict cases /16 entries /634 assertions Release/ASan; independent probes and all broad/fast gates pass | Default solver body unchanged; proof/domain/identity/accounting reviewed; two new-test contract mismatches resolved with independent evidence, failures retained | .b-ref-rate ready to repair continuous exact-sweep mapping before ellipse certification; all later reference/entity/file/UI phases remain partial |
| 1A-1.b-ref-rate | bcd6c5a4fe8a56d30b731f5e2e310e359b8ecfe5 | Three owned paths; six cases/505703 assertions Release/ASan; full Release/combined ASan/both fast lanes pass, all baseline identities retained | Independent rate/periodicity/chain-rule review; initial finite-precision oracle residue fixed without production widening, failed reports retained; committed hashes equal frozen tested source | .b-ref-events-critical ready with exact rational threshold/budget/hook recipe; all reference/component/check/entity/file/UI parents remain partial |
| 1A-1.b-ref-events-critical | 943406abe5cfff8b188947757f101791bc3da8ad | Three owned paths; nine cases/268244 assertions Release/ASan; geometry/full/combined ASan/both fast lanes pass with all baseline identities retained | Exact predicates/maps/projection/domain/accounting reviewed; static thin-complement and native adjacent-bracket refusals corrected without weakening tests/caps/tolerances; final hashes below | Core accounting ready; new named accounting/refinement prerequisites precede geometry isolation; all event/component/check/entity/file/UI parents remain partial |
| 1A-1.b-ref-events-accounting | 290d2658fdc2d9bb55da6205ae7c3afaaade6a4a | Three core paths; five cases/153 assertions Release/ASan; legacy union32/2194, geometry/full/ASan/both lanes pass, baseline identities retained | Every update/charge/failure/guard reviewed; new test corrected ordinary refusal depth8 versus strict3 from independent traces; no production decision/counter change, failed reports retained | Trusted core session card ready with compiled declaration/privacy probe; separate critical-refinement/depth timing prerequisite added before isolation; all parents remain partial |
| 1A-1.b-ref-events-refine | f4f87f48ed024589e31a9a9f1c543340ca1a6c49 | Three frozen/committed paths;14/490 Release/ASan, legacy37/2347, geometry324/2154626, ASan156/1863348, full4667/6995661/74 unchanged skips, both fast lanes pass | Independent proof/lifetime/history/cap review; retained malformed categories and exception-ledger cleanup fixed before native runs; no old test/cap weakened; all baseline identities retained | Core complete, all parents partial; critical API/cost preflight conditional, isolate waits that child; no new entity/file/UI completion |
| 1A-1.b-ref-critical-refine | 72eb5f953f5c88cf6e26505d658fef5237d562b3 | Three frozen/committed geometry paths;21/16424 Release/ASan, legacy11/268911, core51/2837, geometry345/2171050, ASan177/1879772, full4688/7012085/74 identical skips, both lanes pass | Independent exact-root/inverse/image/lifetime/history/accounting/domain review; native red/failed D100 evidence retained and fixed without weakening tests/caps; tested hashes match | Critical sub-contract complete, all parents partial; isolation conditional on its own literal API/proof/cost preflight, not waiting for completed refinements |
| 1A-1.b-ref-providers | 9c27e29807b59c70eb7799087eaf4aa0202b0618 | Three frozen/committed geometry paths;16/299294 Release/ASan,critical30/284668,core51/2837,geometry361/2470344,ASan193/2179066,full4704/7311379/74 identical skips,both lanes pass | Separate completeness/correctness review of exact source/cutter/proof maps/ownership/charges; no old body/assertion/cap change, no native failed attempt, all digests/identities retained | Provider sub-contract complete, all parents partial; isolation conditional own contact/order/fixture/API/cost preflight, no longer waiting for provider/refinement predecessors |
| 1A-1.b-ref-contacts | 92ca5e44508ad42b1873ca8b324e77399028af8b | Three tested/committed paths;17/4383 Release/ASan,old97/586799,geometry378/2474727,ASan210/2183449,full4721/7315762/74same skips,both fast lanes pass | Separate whole-domain/sign/inventory/history/charge review; genuine extra transverse contact retained, no old assertion/cap change, all digests retained; native fallback-success coverage limit explicit | Contact sub-contract done, all parents partial; isolation conditional own literal API/order/accuracy/cost gate, six strict-query families observed complete but not combined events/node accuracy |
| 1A-1.b-ref-events-isolate | dff307668 | Three committed product paths; focused 15/5616 Release+ASan; geometry 154/1,464,224 Release+ASan; full Release 3,842/7,321,304/74 skips; DXF 745/39,929/4 skips and DWG 1,048/35,646/36 skips, both lanes Release+ASan | Independent rereview corrected the tangent test's claim: strict-query depth-cap refusal happens before identity/order; the renamed test asserts only empty bounded refusal. Preparation-limit reason and unresolved-identity diagnostics corrected. Multi-pair shared-own success remains unpinned; exact endpoint/cusp cross-origin identity is covered. | Isolation done; v10 component card ready after its plan checkpoint. Cache checker/entity/formats/UI and all later parents remain incomplete |

**Git safety.** Stage files by name; inspect staged paths before every commit. Preserve unrelated staged/unstaged work
and untracked artifacts. Do not use `git add -A`, destructive checkout/reset, `--amend` or `--no-verify`. Local commit
authorization does not include pushing or merging; those remain the separate publication rows. No generated trailers
or attribution lines are needed.

After a necessary rebase, use `git range-diff`/scoped diffs to map rewritten implementation SHAs, verify their ancestry
and contents, update the ledger, and rerun affected checks. Evidence belongs to a tested tree, not just a title. Fetch
and rebase at S0 or when upstream drift materially affects the work; avoid gratuitous history rewrites between slices.

### 1.5 When a step meets something the plan did not foresee

1. Record the concrete observation, affected contract, reproduction and files under §7.
2. If the plan is wrong, mark `needs revision`, repair the relevant sub-plan and entry criteria, then continue the
   authorized work. If a change is larger than one coherent commit, name child slices and their tests before splitting.
3. Preserve in-progress edits. Inspect whether they are this slice's work, another user's work or generated output.
   Resume or narrowly revise owned edits; do not automatically revert a failed slice or discard unrelated changes.
4. Use `needs decision` only for a material choice with no accepted default or technical resolution. Apply documented
   defaults without repeatedly requesting permission. Work on independent ready slices while external input is needed.
5. A failed test is an investigation task. Record a genuine environment/input blocker precisely, including attempted
   remedies; it is not implementation completion. Do not skip required coverage, add a new-type known gap, or change
   the baseline simply to make a slice green.

Every step can become `partial` through named committed children. After repairing a plan or finishing another slice,
recheck waiting prerequisites and resume newly ready work. Preserve enough evidence to restart without relying on chat
history or uncommitted recollection.

### 1.6 Default "done when" (every step)

- Its scoped Release build and focused tests pass with nonzero expected case counts; required ASan and adjacent tests
  pass for ownership, generic UI, type-dispatch and shared file-path changes.
- Relevant CMake and qmake source/header/form registrations are included in the slice; the complete qmake6 build is
  gated at 1I-a and 2D-a.
- Broaden to the full suite for the completed guard (0B-3), new RTTI integration (1A-2), generic property behavior
  (1G-2), complete file matrix (1E-3), and release gates (1I-a, 2D-a). Compare stable failure identities with S0; do not
  repeat an unchanged broad run merely because a documentation checkpoint was committed.
- If a fast-lane list or shared codec behavior changed, both applicable lanes pass with CI excludes. Required feature
  checks have no silent skips or tolerated new-type gaps.
- The step's own exit criteria and scoped diff review pass. No unresolved correctness finding remains in its contract.
- The implementation commit exists, its actual SHA and evidence are recorded, and the self-updated plan checkpoint is
  committed. Parent and downstream readiness accurately reflect the tree.

### 1.7 Autonomous completion loop

Repeat: reconcile HEAD and plan → choose a ready card → implement its sub-plan → validate and review → commit the
slice → update and commit the roadmap → promote the next ready card. Continue until all required implementation
rows and the §0.4 coverage matrix are done. If no row is ready while work remains, resolve the dependency/design/test
issue or record the concrete external blocker; an empty ready queue is not a completed plan. Publication rows remain
separate. Execution has already completed the leaves listed in §0; do not restart them or the S0 baseline. A plan-only
review updates readiness without promoting prepared code or automatically committing it. On implementation resume,
read Git history, this ledger and the remaining diff first; finish an interrupted implementation's verification and
checkpoint before beginning the next leaf. Completion means all required phase-0/1/2 leaves and R01-R15 are verified
and reconciled, not that the current session or ready queue has ended.

**Per-slice operator checklist.** Follow in order; a failed gate returns to its owning task, not to S0:

1. Reconcile actual checkout and any live work; choose the earliest eligible leaf in §0.3/§0.6.
2. Fill its current-base card, ordered tasks, invariant/cost model and nonzero test selectors; Gibbs starts product work.
3. Implement only that coherent leaf with tests and necessary build registrations. Preserve prior semantics unless
   the card explicitly owns a demonstrated fix; document any newly discovered downstream contract change.
4. Freeze the scoped source; build named targets, discover selectors, run the required checks and record completed
   exits/reports. Correct failures, freeze again and rerun affected checks; never overwrite the failed attempt.
5. Perform the separate completeness and correctness reviews in §0.7, resolve findings and verify tested digests.
6. Stage explicit owned paths, inspect the index, commit the slice with hooks enabled and verify the committed tree.
7. Self-update actual SHA, evidence, findings, parent/requirement state and downstream readiness; commit only the plan.
8. Check checkpoint path scope, ancestry, digests and remaining status. During an authorized implementation run,
   activate the next leaf at this actual HEAD and repeat. On a plan-only request, leave the next card ready but inactive.

**Milestone closure, not just an empty queue.** Before 1I-a closes phase 1 and 2D-a closes the whole plan, enumerate
all required leaves and R01-R15 against the committed tree. Verify every leaf's implementation/checkpoint receipts,
all 14 format rows and conversion legs, raw-carrier and save-refusal tests, CMake/qmake6 builds, nonzero hidden GUI
cases, undo/redo and phase-2 actions. Reopen missing coverage as named leaves. Record independent-reader availability
separately; no self-reader pass implies interoperability. User publication rows stay deferred and never substitute
for a local implementation gate. Required behavior cannot be reclassified as out of scope to end the loop.

---

## 2. Decisions

### 2.1 Implementation decisions

| # | Decision | Taken |
|---|---|---|
| D0 | Grip set | Provisional default: (a) distance grips at offsets of original axis vertices lying on the curve, plus a mid-sweep grip on arcs; centre is a snap only. Not user-approved; resolve before PR D. |
| D1 | Own rtti | In PR C, gated by 0B. No class-sniffing guards in UI files, except the interim U2 guard. |
| D2 | Default of "Editable offset curve" | On for ellipse sources. Set only in `doLoadOptions()`; `RS_OffsetData` and the action constructor stay off, so tests that build the action keep today's behaviour. |
| D3 | How far "edited by the original" goes | The embedded original (phase 1), plus update by pick (phase 2). No stored link (§8). |
| D8 | Two-piece arc results | Stay plain splines and are reported. Re-characterize against #3117-era engine at S0; exact topology and usable fixtures are geometry-based, not inferred from the old 194° band. |
| D9 | File items | Typed centre (1011), major-axis displacement (1012), distance (1041) and scalar fields under `LibreCad`; §3.10 defines the exact schema. |
| D10 | A schema-1 sub-list that fails the load check | Plain spline; the sub-list is removed, and the group too if left empty. Another schema number stays raw XDATA. |
| D11 | Ellipse results | None in this plan. |
| D12 | Tolerance | The engine's rule is unchanged. The relative factor is saved per curve; edits regenerate with the build default and store it. |
| D13 | Limits of an explicit edit | One curve: `LC_OffsetRunLimits{}` (the Offset tool's own). Several curves at once (property sheet, 2A): `LC_OffsetRunLimits::preview()`. |
| D14 | Draw > Spline > Explode on the type | Admitted, as for a spline. |
| D15 | Merge target | `master`; implementation work uses the dedicated feature branch from S0. |
| D16 | Sources in this release | A top-level `RS_Ellipse`, whole or arc, with one entity result passing bounded geometry/topology validation. A supported `RS_Line` is normalized exactly to a degree-1 spline cache; supported degree-1/degree-3 SPLINE results remain editable. Multi-entity results and unsupported/malformed representations stay plain and are reported. |
| D17 | DXF and DWG read/write | Required by the user. Both formats preserve the editable definition using an ordinary SPLINE plus registered `LibreCad` XDATA/EED, with the version contract in §3.10 and acceptance tests in 1E-3. |
| D18 | Unknown owned extensions after edits | Preserve schema-1 unknown tail/flags for no-edit round trips and clones; clear them on successful meaningful geometry changes, preserve them on refusal/no-op. Foreign groups remain raw. |
| N1 | Enumerator | `EntityOffsetCurve` right after `EntityParabola` (`rs.h:198`), value 34. It renumbers only 34–49 (overlays, previews, refs, snaps, DimArrowBlock), none persisted; Quick Selection persists ≤ 33. |
| N2 | Explode | Through Modify > Explode (`LC_ActionBlocksExplode`, filter `isContainer()`): one plain spline. The context-menu label stays "Explode". |
| N3 | Select Window | The type folds under the "Spline" checkbox. |
| N4 | Menu activator | New code `"OC"` (free). Custom "Spline" menus do not fire on offset curves. |
| N5 | Texts | Public statics on `LC_ActionModifyOffset` (§3.1), so they keep its translation context. |
| N6 | Lost definition | An in-place unequal scale or shear leaves an "undefined" curve that behaves as `RS_Spline` (§3.4). Document-level tools substitute the plain spline first, so only hatch-loop clones reach this state. |

### 2.2 Decisions for the user

Defaults apply unless the user changes them; a later change marks the affected steps `needs revision`.

| # | Question | Default |
|---|---|---|
| D0 | Grip set | Provisional option (a); ask on #2961 and record user's decision before PR D. |
| U1 | Write flag bit 4 "source kept at creation"? | No. Bits 4–14 are reserved; safe unknown bits survive no-edit round trips under D18, then clear on meaningful geometry edits. |
| U2 | Interim Edit Properties guard in 1A-2, so PR C can merge alone? | Yes. |
| U3 | General default-arm hardening in `LC_ActionModifyEntity`? Leader, Tolerance, Solid, ConstructionLine, Wipeout and MLeader leave a stuck action today. | Out of scope (Appendix C). |
| U4 | Command-line names for the phase-2 actions? | None. |
| U5 | Multi-curve property-sheet edits | The refusal-aware design of 1G-2, including generic `lib/properties` regression coverage, is required. Read-only multi-selection is a scope change requiring an explicit user decision, not an automatic shortcut. |

#### Decision needed

- D0: confirm or change the provisional grip set before PR D. Until then, implement using the stated default and keep the
  grip policy isolated in `rebuildGrips()` so it can be revised without changing geometry or serialization.

---

## 3. Normative design

### 3.1 New engine-level types and the texts

```cpp
// lib/engine/document/entities/lc_offsetoutputbudget.h (included by lc_curveoffset.h and rs_modification.h)
struct LC_OffsetRunLimits {
    LC_OffsetSourceBudget budget = makeDefaultOffsetSourceBudget();
    std::size_t maxSamples = 0;           // 0: the engine's default
    std::size_t maxIntersectionPairs = 0; // 0: the engine's default
    static LC_OffsetRunLimits preview();  // one eighth of each, the values of LC_OffsetBatchLimits::preview()
};

// lib/engine/document/entities/lc_offsetcurve.h
enum class LC_OffsetCurveRefusal { None, InvalidInput, Undefined, Inconsistent, TooSmall, NothingLeft, SeveralPieces,
                                   NotCacheRepresentable, CheckFailed, Engine };
struct LC_OffsetEditResult {
    LC_OffsetCurveRefusal refusal = LC_OffsetCurveRefusal::None;
    LC_CurveOffsetStatus engineStatus = LC_CurveOffsetStatus::Ok;
    LC_OffsetCheckResult::Reason checkReason = LC_OffsetCheckResult::Reason::None;
    bool ok() const { return refusal == LC_OffsetCurveRefusal::None; }
};
```

- **Why `LC_OffsetRunLimits`.** `LC_OffsetBatchLimits` lives in `lib/modification/rs_modification.h:147-163`; its own
  comment says it belongs to the modification layer. No `lib/engine` entity includes a modification header.
- **Applying run limits** mirrors `rs_modification.cpp:1533-1541`: the budget comes from the limits, and
  `options.maxSamples` / `options.maxIntersectionPairs` are clamped when the limits' values are non-zero. Optionally
  add `LC_CurveOffset::applyRunLimits(options, limits)`; do not refactor `offsetWithOutcome` to use it.
- **N5 texts** are public statics on `LC_ActionModifyOffset` (`lc_action_modify_offset.h:76-81` are protected statics
  today):
  - `static QString offsetFailureReason(LC_OffsetFailureDetail, LC_CurveOffsetStatus)`; the existing outcome overload
    forwards to it;
  - `static QString offsetCurveRefusal(const LC_OffsetEditResult&)`;
  - `static QString offsetCurveKeptPlain(LC_OffsetCurveRefusal reason, int count)`.

  Callers in `ui/` and `actions/` include `lc_action_modify_offset.h`.

### 3.2 The class

New `lib/engine/document/entities/lc_offsetcurve.{h,cpp}`. The old declaration prototype was syntax-checked against
7c4a93fa8 and predates v5's consistency/validation additions. Compile the current declaration and real headers during
1A-2; the private `corrected.cpp` is historical design evidence, not a passing check for this version.

```cpp
class LC_OffsetCurve : public RS_Spline {
public:
    using Definition = LC_EllipseOffsetDefinition;  // lc_ellipseoffsetgeometry.h (1A-1):
    //   struct LC_EllipseOffsetDefinition { RS_EllipseData basis; double distance = 0.; bool towardCentre = false;
    //                                       bool cacheReversed = false; double relTolerance = 0.; };
    LC_OffsetCurve(const LC_OffsetCurve&) = default;       // clones an already established state
    static std::unique_ptr<LC_OffsetCurve> adopt(const RS_Entity& engineResult, Definition def, LC_OffsetEditResult* why);
    static std::unique_ptr<LC_OffsetCurve> create(const RS_Ellipse& source, double distance, bool towardCentre,
                                                  const LC_OffsetRunLimits& limits, LC_OffsetEditResult* why);
    static bool normaliseBasis(RS_EllipseData& basis);   // §3.3; false: unusable
    static bool isUniformScale(const RS_Vector& factor); // |fx| == |fy| within LC_InsertTransform::UniformScaleTolerance
    static std::size_t engineRuns();                     // relaxed std::atomic, counted in runEngine() (tests)

    RS2::EntityType rtti() const override { return RS2::EntityOffsetCurve; }
    RS_Entity* clone() const override;                   // new LC_OffsetCurve(*this): always this class
    const Definition& definition() const;
    bool isDefined() const;
    bool isConsistent() const;                           // cacheFingerprint(getData()) == m_stamp
    bool hasEditableDefinition() const;                  // isDefined() && isConsistent()
    bool isWhole() const;                                // the original is whole; never branch on isClosed()
    bool cacheFormsClosedChain() const;                  // validated result closure, separate from original kind
    std::unique_ptr<RS_Spline> toPlainSpline() const;    // std::make_unique<RS_Spline>(static_cast<const RS_Spline&>(*this))
    const std::vector<std::shared_ptr<DRW_Variant>>& unknownFileTail() const;
    void setUnknownFileTail(std::vector<std::shared_ptr<DRW_Variant>> tail);
    int unknownFileFlags() const;
    void setUnknownFileFlags(int bits);                  // flag bits 4-14 written by a newer LibreCAD

    LC_OffsetEditResult setDistance(double distance, const LC_OffsetRunLimits& limits);
    LC_OffsetEditResult setTowardCentre(bool towardCentre, const LC_OffsetRunLimits& limits);
    LC_OffsetEditResult setThrough(const RS_Vector& point, const LC_OffsetRunLimits& limits);
    LC_OffsetEditResult setBasis(const RS_EllipseData& basis, const LC_OffsetRunLimits& limits);
    bool throughParameters(const RS_Vector& point, double* distance, bool* towardCentre) const; // closed form, no engine

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    RS_Entity& shear(double k) override;
    void revertDirection() override;
    RS_VectorSolutions getRefPoints() const override;
    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;   // editable: inert; otherwise base (§3.6)
protected:
    RS_Vector doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const override;
    RS_Vector doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** centerEntity) const override;
    RS_Vector doGetNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;
    RS_Vector doGetNearestDist(double distance, const RS_Vector& coord, double* dist) const override;
private:
    struct ValidatedCacheTag {};
    LC_OffsetCurve(RS_EntityContainer* parent, const RS_SplineData& cache, const Definition& def, ValidatedCacheTag);
    LC_OffsetCurve(const RS_Spline& cache, const Definition& def, ValidatedCacheTag); // preserves imported attributes
    LC_OffsetEditResult runEngine(const Definition& next, const LC_OffsetRunLimits& limits, RS_SplineData* out) const;
    void adoptCache(const RS_SplineData& candidate, const Definition& next);
    static std::uint64_t cacheFingerprint(const RS_SplineData& cache);
    void clearUnknownDefinitionExtensions();             // after successful meaningful geometry changes only
    void rebuildGrips();
    Definition m_def;
    bool m_defined = true;
    bool m_cacheClosed = false;
    std::uint64_t m_stamp = 0;
    std::vector<RS_Vector> m_grips;   // in cache order, reversed with the cache
    std::vector<std::shared_ptr<DRW_Variant>> m_unknownFileTail;
    int m_unknownFileFlags = 0;
};
```

- **Copy.** The defaulted copy is used for `clone()`:
  - the container copy deep-clones and reparents the chords (`rs_entitycontainer.cpp:176-196`);
  - the `RS_Entity` copy copies `Impl`, including #3098's lineage (`rs_entity.cpp:109-114`).
- **Construction invariant.** Raw-cache constructors are private and require the validated tag. `adopt` validates the
  normalized definition and cache before invoking them; implement this factory in 1A-2, not as a success-shaped stub.
  Public raw data cannot create an editable invalid object. Use a member-owned `new`/`unique_ptr` construction where
  necessary: `std::make_unique` cannot invoke a private constructor merely because its caller is a member.
- **Fingerprint.** Make the existing geometry hash `public: static std::uint64_t geometryFingerprint(const RS_SplineData& data);` in
  `rs_spline.h`, with the body moved out of the anonymous namespace (`rs_spline.cpp:193-216`). `arcLengthTable()`
  keeps calling it unqualified at :879. A non-static member would hide the free function and break that call.
  The offset entity's `cacheFingerprint` also covers representation type, saved-open representation/knots, fit points
  and closed/periodic flags: the current length-cache hash omits them. Do not mistake a hash match for geometric
  validation; construction/adoption establishes that invariant, while the stamp detects later mutation.
- **The original is built from data:** `RS_Ellipse{nullptr, data}`, never `source.clone()`, which would carry parent,
  layer, pen, XDATA and DWG handles.
- **Side mapping:** `towardCentre = (side == Left) != basis.reversed` (`lc_curveoffset.cpp:473-474`).
- **Where entity regeneration runs:** only in `runEngine` for this entity's creation/mutators. The existing Offset
  tool and offset-again helper have their own engine entry points; `engineRuns()` counts this class's `runEngine`
  invocations, not every engine call in the process. Never regenerate in `clone()`, `update()`, painting, undo, redo, transforms,
  hover or INSERT expansion. The hover highlight clones the entity under the cursor whenever it changes and after
  every drawing redraw (`rs_actiondefault.cpp:97-100`, `:1107-1125`), so `clone()` must be cheap and must carry
  `m_grips`.
- **Calling through the derived type.** `curve.scale(c, 2.5)` does not compile (name hiding, as for `RS_Spline`):
  call through `RS_Entity&`, or pass `RS_Vector{k, k}`.

### 3.3 Normalising the original (the order matters)

Validate raw input before constructing a temporary ellipse: valid finite XY vectors/angles, nonzero finite axis length,
positive finite ratio and a representable 2D basis. Preserve the canonical whole form `(0, 0)`.

1. For a noncanonical arc, distinguish a genuine one-turn sweep (`abs(abs(angle2-angle1) - 2π)` within the documented
   angular tolerance) from a zero/tiny sweep. Convert only the genuine full turn to canonical whole. A remainder-only
   test confuses zero with 2π and must not be used. Refuse degenerate noncanonical sweeps and unsupported multiple
   turns instead of silently enlarging them to a whole ellipse. Test wrapped and reversed arcs separately.
   Use `1e-12·2π` as the full-turn comparison allowance and `RS_TOLERANCE_ANGLE` for the zero/tiny-sweep refusal;
   keep these separate and test both sides of each boundary.
2. Build `RS_Ellipse e{nullptr, basis}` using its setters/current canonicalization. For a classified full turn, call
   `e.setAngle1(0)` and `e.setAngle2(0)` before any axis swap. If `e.getRatio() > 1`, call `switchMajorMinor()`;
   return false on failure.
3. Copy `e.getData()` back after validating finite normalized results. Canonical whole input already normalized by the
   source API remains whole; raw imported metadata must be checked before its small angles can be canonicalized away.

Never write `angle1`, `angle2` or `isArc` of `RS_EllipseData` directly. Only the setters and `calculateBorders()`
refresh `isArc` and the render caches (`rs_ellipse.h:63-77`, `rs_ellipse.cpp:298-336`).

**Why this order.** `switchMajorMinor()` re-derives the arc angles from the end points (`rs_ellipse.cpp:581-603`).
For a full-turn arc both points coincide, so the sweep collapses: 6,901 of 10,000 emulated ratio-2 full-turn arcs
came out with a sweep of 1e-8 or less. The engine then refuses the source (`lc_curveoffset.cpp:384-390`).

**"Whole"** is `!RS_Ellipse{nullptr, basis}.isEllipticArc()`; angles below `RS_TOLERANCE_ANGLE` snap to 0
(`rs_ellipse.cpp:298-306`).

### 3.4 Stamp, consistency, undefined state

- **The stamp** is `cacheFingerprint(getData())`, including the representation/round-trip fields listed in §3.2.
  It is renewed only by the class's own transforms and
  mutators, after the base transform. It is never compared in `update()`, because every `RS_Spline` transform calls
  the virtual `update()` itself (`rs_spline.cpp:948-1051`); comparing there dropped the definition on every move.
- **Each transform** records `carry = m_defined && isConsistent()` **before** the base transform. It carries the
  original and the grips and restamps only if `carry`; otherwise a moved cache would make an edited cache consistent
  again.
- **Transform input:** reject non-finite move/scale/shear/rotation inputs, zero rotation vectors and degenerate mirror
  axes before calling the base transform; leave the entity unchanged. Finite singular scale or unequal scale and
  nonzero shear produce an undefined representation after the base transform. `shear(0)` is a no-op.
  Finite inputs can still overflow or collapse representable coordinates. Stage the transformed cache, original and
  grips before publishing: reject non-finite results, a lost finite axis or unrepresentable defining geometry without
  changing the live object. This is not an engine run. Test very large centres/factors, subnormal axes and cancellation;
  an overflow must not create a freshly stamped editable object. A representable singular scale is the deliberate
  geometry-only case above, not the same as arithmetic overflow.
- **An undefined curve behaves exactly like `RS_Spline`:**
  - every override forwards to `RS_Spline`, and `getRefPoints()` returns the control points;
  - mutators return `Undefined`;
  - the writer writes no sub-list;
  - UI rows are read-only;
  - 1H uses the generic drag path;
  - offset again treats it as a plain spline;
  - 2A and 2B skip it.

  Its `rtti()` stays `EntityOffsetCurve`, so casts keyed on rtti stay valid. This covers direct API transforms and
  hatch-loop clones (`rs_hatch.cpp:557-566`); document-level tools normally substitute a plain spline first.
- **Runtime guard:** mutators, property/dialog/drag paths and phase-2 actions use `hasEditableDefinition()` in release
  builds as well as debug builds. An inconsistent cache returns `Inconsistent` without regeneration or state change;
  export demotes it to an ordinary spline and removes stale owned schema. Definition-backed grips/snaps and UI values
  also fall back safely instead of using stale geometry. Debug assertions may supplement, not replace, these checks.
- **Closure:** `isWhole()` describes the embedded source; `m_cacheClosed` describes a validated result whose ends meet.
  A trimmed arc source can produce a closed result. Keep both facts separate in endpoint snaps, grip ordering and
  topology checks; the engine's closed result can use ClampedOpen data with coincident ends.
- **Future owned extensions:** preserve unknown schema-1 tail/flag bits for no-edit save/open and cloning. Clear those
  owned unknown extensions only after a successful meaningful geometry/definition change, because their semantics
  cannot be updated safely. Refusal and no-op edits preserve them. Foreign application groups and unknown schema
  versions retain their existing raw preservation policy (§3.10).

### 3.5 The validity check (one rule for creation, edit and load)

```cpp
struct LC_OffsetCheckLimits {
    std::size_t maxCacheSpans = 4096;
    std::size_t maxVertices = 65536;
    std::size_t maxDistancePairs = std::size_t{1} << 20;
    unsigned maxDepth = 20;
};
struct LC_OffsetCheckResult {
    enum class Reason { None, InvalidInput, Layout, TooSmall, ReferenceUnresolved,
                        OffCurve, Ambiguous, Topology, LimitExceeded };
    Reason reason = Reason::None;
    std::size_t span = 0;
    LC_EllipseOffsetGeometry::CheckWork work{};
};
namespace LC_EllipseOffsetGeometry {
LC_OffsetCheckResult checkCache(const RS_SplineData& cache, const LC_EllipseOffsetDefinition& definition,
                                double tol, const LC_OffsetCheckLimits& limits = {});
}
```

`tol` is the finite positive tolerance already requested by the owner (normally the value produced by
`LC_CurveOffset::toleranceFor(RS_Ellipse{nullptr, basis}, distance, factor)` in 1A-1). This geometry checker does not
call the engine or regenerate a cache. Validate `tol`, the positive finite distance magnitude, normalized basis copy,
and every nonzero limit before expensive work. Map `towardCentre` to the signed outward offset exactly once;
`cacheReversed` changes candidate traversal order only and never changes physical side or source traversal. Return the
final cumulative `CheckWork` snapshot from the same `WorkBudget` used by layout, reference analysis, bounds and
topology. `TooNear` is intentionally not a public reason: current bounds prove a clearance lower bound or refuse, but
do not prove an upper bound establishing that the source is actually too near. `Ambiguous` and `ReferenceUnresolved`
must not be mislabeled as geometric truth. Default limits are concrete starting bounds aligned with existing engine
limits; qualify their acceptance/cost in 1A-1.c before freezing them.

The old endpoints/midpoints validator is insufficient: a cubic may agree at those points and stray between them.
Use a bounded adaptive check with conservative error accounting; do not claim an exact algebraic identity.

1. **Validate domain and layout before work/allocation.** Require finite 2D controls/knots/weights and normalized
   source fields, valid positive limits, degree 1 or 3, ClampedOpen representation, and `n ≥ 1` spans. For degree 1,
   require `n+1` controls and `n+3` knots; for degree 3, require `3n+1` controls and `3n+5` knots. The normalized
   cache has exactly one finite unit weight per control; an absent raw file weight list is materialized only after the
   separate raw-carrier checks in 1E-1.c, never by this validator. Degree-1 knots have two equal initial values,
   `n-1` single internal groups and two equal final values. Cubic knots have four equal initial values, `n-1`
   internal groups of three, and four equal final values. In both layouts require exact repeated values after
   decoding and strictly increasing finite group representatives; do not use an absolute tolerance that merges
   narrow valid spans. A finite positive affine knot reparameterization is valid, and all later span evaluation must
   use its actual knot interval rather than assume integer parameters. Reject invalid count arithmetic, fit points,
   nondefault saved-open payloads, non-finite or nonzero-Z controls and non-finite/degenerate geometry.
   `m_closedFlag`/`m_periodicFlag` may be carried with ClampedOpen data and are not rejected by layout alone; the
   topology stage validates their endpoint/closure meaning. Degree-1 span `k` uses controls `[k,k+1]`; cubic span `k`
   uses `[3k .. 3k+3]` because every interior cubic knot has multiplicity three.
2. **Small distance:** `distance ≤ tol` gives `TooSmall`. The caller must distinguish invalid inputs, geometric
   refusal and a work limit; none may become successful adoption with an unchecked cache.
3. **Construct bounded reference geometry.** Use `analyseReferenceComponents` with the same normalized source sweep,
   signed distance and shared `WorkBudget`; its owned events/providers already supply exact visible intervals, event
   nodes and root fringes for reversed/wrapped arcs. Compare against `ReferenceFrame::boundOffsetJet`, not a materialized
   `createEntities` result, a fitted spline, stroke approximation or independently duplicated trim logic. Subdivide
   each visible edge's exact reference-parameter interval adaptively and bound its chord error from its local second
   derivative. Keep event-node/fringe pieces explicit and split at the whole-ellipse seam. This is bounded geometric
   analysis; it never refits or regenerates the saved cache. Unresolved visibility, event classification or finite
   interval bounds refuses adoption.
4. **Check every span, not three samples.** Work in the `ReferenceFrame`'s centre-relative, uniformly scaled local
   coordinates. Normalize each `CacheSpan` to a degree-1/degree-3 Bezier on `[0,1]`; recursively split its control
   net with interval de Casteljau, never with its possibly narrow/affine knot difference. For a cubic leaf with local
   controls `P0..P3`, its same-parameter deviation from its endpoint chord has the Bernstein difference controls
   `0`, `P1-(2P0/3+P3/3)`, `P2-(P0/3+2P3/3)`, `0`; the maximum norm of the two middle interval vectors is a certified
   upper bound by the convex-hull property. A line's bound is zero. For each visible `ReferenceEdge`, restrict to its
   `ReferenceSpan.interior` (the certified event-free open interval and its limiting endpoints), split intervals
   dyadically, and enclose exact offset endpoints and `Q''` with `ReferenceFrame::boundOffsetJet`. If `M2` encloses
   `sup ||Q''||` and `h` encloses the reference parameter width, linear-interpolation error is at most `M2*h^2/8`.
   The formula applies to the parameterized exact offset and needs no regular-speed assumption; non-finite or
   unbounded enclosures are unresolved, never success.

   Compare each specified candidate/reference **subcurve pair as sets**; do not assert an affine parameter map,
   nearest-foot identity or like-numbered endpoint match. Let `S_C` and `S_Q` be their closed chord segments and let
   `Hchord` be the maximum of both directed endpoint-to-opposite-segment bounds. Distance to a closed convex segment
   is convex along the other segment, so endpoint maxima bound the complete chord-to-chord Hausdorff distance. Form
   all projection, clamp, vector-norm and conversion calculations with outward intervals; if a chord length is not
   provably positive, use a conservative endpoint-ball bound or subdivide. In local frame units set
   `E = eC + eQ + Hchord`, where `eC` and `eQ` already include every interval-conversion/rounding enclosure. This
   triangle bound certifies symmetric Hausdorff distance between the two curve subpieces, independent of traversal
   parameterization or orientation. A pair is a geometry witness only if its interval upper bound proves
   `E <= (2*tol)/frame.scale()` against the threshold's interval lower bound. For the candidate-to-visible-reference
   direction, form `L = ReferenceSpan.clearance / frame.scale()` and the interval `clearanceLower = L-E`; pass only if
   `clearanceLower.lo()` is at least the upper endpoint of the interval
   `(abs(distance)-2.5*tol)/frame.scale()`. Likewise, prove the geometry threshold by comparing `E.hi()` with the
   lower endpoint of `(2*tol)/frame.scale()`. Source proof uses the exact signed-distance magnitude, not the engine's
   traversal-side signed value. The visible-span premise is checked from the referenced edge; never use hidden spans,
   the full ellipse in place of the restricted source, or a sample as proof.

   Root isolating intervals are not silently dropped or treated as wholly visible. The `ReferenceSpan.interior`
   covers the certified event-free interval and its limiting endpoints. Treat each endpoint fringe separately using
   the exact `ReferenceFringe` selected by the span's start/end occurrence and its `ReferenceNode`; never substitute
   the rounded node or a root midpoint for the exact event. Bound the full fringe-to-node image separation from both
   local interval boxes and include it in the fringe geometry/coverage error. A node certificate is eligible only when
   the node has an incident visible edge: the exact event point is then in that edge's closure, whose restricted-source
   clearance is `D`; charge candidate-to-node and fringe-to-node enclosure errors to their distinct proof terms as
   specified below. Cutter-only,
   hidden-point and hidden-span geometry cannot witness a retained cache. Keep each reference interval within one
   visible edge and within `[0, frame.parameterEnd()]`; split at the whole-ellipse seam and use the two explicit seam
   incidences, never an unwrapped `boundOffsetJet` query across `T -> 0`.
   Preserve every certified edge/node alternative needed by topology, with diagnostic indices into the exact immutable
   `ReferenceComponentAnalysis`; no pointers or IDs outlive that analysis. These witnesses are proof data for the next
   topology stage, not a successful cache or public `checkCache` result. Topology must prove complete bidirectional
   coverage and a consistent ordered assignment; it may request further pair proofs through the same bounded helper.
   Build a bounded relation of candidate leaf/reference subspan and candidate leaf/event-node witnesses. Charge and
   retain all certified alternatives required by topology; do not greedily keep only the first matching edge. Use
   conservative interval AABB separation only to prune a pair when its lower distance proves it cannot satisfy the
   threshold. Otherwise subdivide the candidate and/or reference side selected by a deterministic largest-error/size
   rule. A pair that remains possibly relevant but unresolved at depth or representability limits refuses the whole
   bounds result; it is not silently omitted from the relation. Witnesses contain cache span plus dyadic subspan,
   reference edge and exact subinterval (or event occurrence/node), outward error upper bounds and source-clearance
   lower bounds. IDs are indices into the exact immutable analysis owned by the result. No partial witness vector is
   published on any failure. Candidate nearest-point samples or golden-section searches may order probes but may not
   prove or prune acceptance.

Source clearance is not a second nearest-point search. For an exact visible interior fragment, its restricted-source
clearance is `D_local=abs(distance)/frame.scale()` and distance-to-source is 1-Lipschitz. Thus the same proved
candidate/reference error gives `clearance(candidate) >= D_local-E`; require this lower bound to meet
`(abs(distance)-2.5*tol)/frame.scale()` independently of the geometry threshold. At event nodes use the distinct
`D_local-E_candidateNode` rule above. This uses the exact-D component predicate already implemented by
`.b-ref-components`; any tolerance-retained or engine-marginal result must instead account for its explicit clearance
loss and may not inherit this exact-D claim.
5. **Prove local topology, then global coverage in two commit-sized leaves.** `.b-check-topology-local` consumes the
   complete geometric witness alternatives but does not publish cache validity. It adds a shared internal pair-proof
   session used by both initial bounds enumeration and topology refinement; store the immutable acceptance intervals
   needed by that session with `CacheBoundsAnalysis`, but never store/replace the caller's `WorkBudget`. A topology
   refinement must rerun the actual subpair geometry and clearance proof, not inherit a coarse parent Hausdorff result.
   Each local proof cell records candidate span/dyadic domain, visible reference edge and exact interval (or the
   occurrence/node plus start/end role for a fringe), projection direction and exact proof outcome. An interior cell
   also proves both corresponding candidate/reference endpoint distances with outward intervals; setwise Hausdorff
   closeness alone does not pair cell boundaries. A root-fringe/node witness is an endpoint/incidence certificate,
   never positive-length candidate or reference coverage: it cannot let a constant/tiny loop stand in for a whole
   visible edge. Keep every certified alternative needed downstream; IDs stay indices into the owned analysis. No
   pointer, root midpoint or rounded world coordinate is an event identity.

   On each proposed cell choose a finite nonzero local projection vector from the oriented candidate endpoint
   enclosures and prove its squared norm has a strictly positive lower bound. If it is zero/unresolved, subdivide and
   choose a new local vector; refuse at representability/depth limits. Candidate order is normalized once: with
   `cacheReversed=false`, traverse spans and each span's local parameter forward; with `true`, traverse spans backward
   and reverse each span's local parameter/control order. After this transform both candidate and reference are
   required to progress forward; never flip an individual cell. `towardCentre` affects only the signed offset used to
   build the reference, not cache order.

   For a degree-1 candidate require the outward interval for projected derivative to be strictly positive; a constant
   span is `Topology`. For a cubic, the derivative is the quadratic Bernstein curve with controls
   `3(P1-P0)`, `3(P2-P1)`, `3(P3-P2)`. Require every projected-control interval lower endpoint `>= 0` and at least one
   `> 0`; Bernstein positivity then proves strict increase on the open cell, allowing a stationary endpoint. For the
   exact reference, obtain `Q'(t)` and endpoint enclosures from `ReferenceFrame::boundOffsetJet`; require projected
   derivative lower bound `>= 0` over the complete exact interval and strict endpoint separation
   `endProjection.lo > startProjection.hi`. The strict endpoint condition rules out a flat subinterval. If either
   certificate is unresolved, subdivide and rerun both pair bounds and derivative bounds, even when the parent
   Hausdorff bound passed. Root/fringe intervals retain their published occurrence and one-sided edge role. Use only
   the published enclosure and exact event identity; do not invent/refine an unowned root. If its full enclosure cannot
   prove the required one-sided direction, refuse conservatively. Hidden simple-cusp points have no visible incident
   edge and cannot become endpoint witnesses. The local leaf returns only a local-proof analysis/status; it must not
   claim complete reference coverage, one component, or a successful `checkCache`.

   `.b-check-topology-chain` consumes that committed local relation and requires exactly one retained nondegenerate
   reference component. Interior cells tile each positive-length candidate and visible-edge parameter domain; exact
   endpoint cells bind the one-sided root fringe to a candidate endpoint and its occurrence/node role. The fringe's
   full isolating box is charged to geometric closure, but is not mistaken for a full interval of visible traversal.
   Use `ReferenceComponent.edges` in source-parameter order, but do not infer a successor from
   physical-node equality: each transition must match the exact outgoing/incoming edge IDs, occurrence IDs, node ID
   and incidence role. This preserves the source-ordered traversal through crossings and permits only a proved
   lens/trim transition. Build a bounded DAG of local proof cells; a transition is eligible only when candidate
   domains touch in the normalized traversal and reference domains touch in one visible edge or at an exact allowed
   event/seam incidence. Find an order-preserving, non-reusing path that covers every candidate span/domain and every
   reference visible interior exactly once; each one-sided fringe appears exactly once as its endpoint/incidence
   certificate, except for the single shared boundary at a join.
   Count paths with saturation at two rather than enumerating exponentially many assignments. A unique path passes;
   multiple paths pass only when they have the same exact edge/occurrence sequence modulo the one documented cycle
   cut. Distinct crossing branches or distinct event occurrences are never equivalent rotations. A whole-ellipse
   closure uses `ReferenceSeam.start/end` as its sole last-to-first transition. A closed trimmed component may close
   only when its first/last exact occurrences share the same node and the incidence graph proves that transition;
   an open component must preserve its exact first/last endpoint occurrences. `m_periodicFlag` without
   `m_closedFlag` refuses; either true flag requires exact closed candidate endpoint controls, while a false flag does
   not by itself make an otherwise valid geometric cycle open. Do not infer periodic derivative continuity from this
   clamped representation.

   The public `checkCache` success is created only after bounds, local proofs, unique global chain, both complete
   coverage directions and closure/flag checks pass. Map only proven dispositions: invalid arguments/limits to
   `InvalidInput`, layout errors to `Layout`, `abs(distance) <= tol` to `TooSmall`, a proved lack of geometric witness
   to `OffCurve`, reference-analysis refusal to `ReferenceUnresolved`, unresolved interval arithmetic to `Ambiguous`,
   non-unique/missing/invalid order or incidence to `Topology`, and any exhausted resource to `LimitExceeded`. Do not
   report `TooNear` unless a future separate upper-bound proof actually establishes it. Every failure is atomic and
   returns the final shared `CheckWork`; callers keep plain geometry or the previous editable object.

   Required adversarial discriminator: a cubic with controls `[0, 4/3, -1/3, 1]` on a line has image `[0,1]` but its
   projected derivative reverses internally. Its setwise bounds may pass, but local proof or the global chain must
   refuse it. Also test a genuinely monotone cubic with a stationary endpoint, exact event endpoint identity,
   crossing branch-switch, missing/repeated reference tails, whole-seam cycle, closed trimmed join, open-end mismatch,
   and both `cacheReversed` values. No success may result from a passing subset or an ambiguous cycle.
6. **Bound every path.** Check vertex/piece/pair/depth caps before expansion; non-finite intermediate bounds or exhausted
   work produce a refusal (`InvalidInput` or `LimitExceeded`), never an acceptance. Work counters support deterministic
   tests. Adaptive validation runs at adoption/edit/load, not painting or mouse motion. Use local coordinates and
   scale-aware error handling; extreme coordinates must not silently overflow a validity test.

**Acceptance arithmetic.** A point estimate from `distanceToOriginal()` is not a lower bound, and closeness to the
supporting whole ellipse does not prove visibility relative to a restricted arc. Source clearance uses conservative
bounds on the actual source sweep plus its endpoints; whole-ellipse distance may seed a search only. Account for
local/world conversion, chord and interval arithmetic errors in the stated `2*tol` / `d-2.5*tol` inequalities using
outward bounds. An invalid or unbounded interval cannot pass by an empty comparison. Check the complete finite
candidate before publishing it; a successful prefix or a close sample cannot rescue an undecided remainder.

For an exact visible interior reference fragment, the pairwise Hausdorff certificate supplies, for every candidate
point, an actual point on a restricted-source-visible reference subcurve within its proved error `E`; distance-to-source
is 1-Lipschitz, so interior clearance is at least `L-E`, where `L` is the certified restricted-source clearance in
local frame units. At a root fringe, use the exact visible edge endpoint/node as the candidate-to-source clearance
witness and subtract only the candidate-to-node enclosure error: `D_local-E_candidateNode`. Separately include the
complete fringe/node enclosure separation in the fringe geometry and reverse-coverage bound. Never add that fringe
separation a second time to candidate-to-node clearance, and never infer that a hidden side of the root interval has
clearance `D`. A future tolerance-retained reference with only clearance
`D-epsilon` must use `D-epsilon-E` and still meet `D-2.5*tol`. These local certificates do not replace complete events,
topology or ordered bidirectional coverage, and a sample/whole-ellipse estimate cannot establish the restricted-source
premise.

1A-1.c must test adversarial edits between the former sample points, reversed/missing components, NaNs, unusual knot
scales, near cusps, closed arc-source results and each work-limit exit. Compare with independently dense geometry as
a test oracle and record current-engine acceptance. Native files written from accepted curves must still pass after
DXF rounding and DWG readback; resolve any precision margin explicitly rather than widening tolerances to hide errors.

Historical `rt_check.cpp` results at beccd8c14 accepted 16,914 engine results but also 630 of 929 control edits of
4.5 tolerances. They demonstrate why the previous sample-only check cannot satisfy this readiness gate; its old
0.5 ms timing is not a performance promise for the revised algorithm.

#### 3.5.1 Exact-reference sub-plan and qualification gate

`1A-1.b-ref` owns a read-only reference analysis contract, not a call to the materializing offset engine. Its result
contains a status, ordered retained components, source-parameter intervals/direction, typed endpoint/node identities
(free source end, cusp, transverse corner or trim boundary), conservative interval bounds, and consumed work. The
public geometry checker consumes this result; keep the concrete adapter private to the geometry module unless a
small shared engine helper actually removes duplicated classification logic. No load path may refit saved geometry.

Implementation order for the `.b-ref` aggregate: frames, strict core, continuous-rate correction, critical preparation,
core depth accounting, trusted two-curve refinement, trusted critical-occurrence refinement, owned certified provider
preparation, construction contacts, exhaustive isolation, then components (§0.6). As of v11, all predecessors through
isolation are committed; `.b-ref-components` is the current in-progress uncommitted leaf, and its unresolved required
multi-pair success keeps cache checking and the aggregate open.
The event split was qualified against the committed frame and solver at 26d107ccf; strict core certificates are an
independently safe helper slice, not a completed exact-reference adapter or an adoption success:

1. Read `EllipseSource::make`/`OffsetCurves`, `nodeBranches` and `trimBranches` at the execution base. Record sweep,
   side, numeric-floor and visibility-tolerance conventions in the card. `LC_Interval` lives in `lib/math/`; use its
   outward bounds, `lc_curvejet.h` and `lc_parametriccurveintersection.h`, not a new ad-hoc sampling engine.
2. Construct exact signed-offset jets and bounded parameter intervals, splitting at curvature-zero/cusp events and
   isolating own-offset/opposite-offset intersections and open-end half-circle **cutters**. The cutters classify
   visibility; they are not output geometry appended to the offset. Qualify corner, tangent and overlapping events;
   unresolved events return a refusal. Do not reuse fitted cubic control points as an allegedly independent exact
   reference. A helper extraction must preserve existing engine behavior and its tests.
   Ordinary `LC_ParametricIntersection` values supply estimated parameters and a kind without a certification
   guarantee. The committed opt-in strict mode at834a46a88 additionally reports parameter-box enclosures and the proof
   category (interval-unique transverse root or qualified analytic endpoint/cusp/tangency). Reuse that strict path and
   retain ordinary defaults for existing callers. Trusted retained pair contraction is now committed atf4f87f48e;
   critical-occurrence contraction is committed at72eb5f953 and owned-provider preparation at9c27e2980;
   geometry-level complete event integration is committed at `dff307668`; retained visibility/component assembly
   remains unfinished.
   Depth-terminal numerical contact classification is not a
   certificate; in strict mode an unproven event returns ambiguity. Analytic whole-lens/cusp identities may certify
   their own occurrences with bounded arithmetic, but cannot certify unrelated arc/cutter events. Root estimates may
   seed isolation, never replace exhaustive interval exclusion of other candidate boxes. If enclosures cannot be
   ordered or distinguished within budget, refuse rather than sort approximate parameters into a fabricated graph.
   Audit every default tolerance heuristic when enabling strict mode: small-image leaf acceptance/bridge pruning,
   near-join exclusions and deduplication are not by themselves proofs that no other event exists. The new cached
   range index may supply conservative monotonicity bounds, but its default small-image shortcut cannot certify
   topology. Strict mode must prove absence/identity analytically or by interval enclosures, or return ambiguity;
   do not inherit an `Ok` result merely because a loop/contact is below the display tolerance.
   Ordinary Krawczyk estimates evaluation error as `64*eps*(|A(mid)|+|B(mid)|)`. That is not a strict certificate
   near cancellation such as `E+delta*N`: the result may be tiny while arithmetic error is governed by the source
   scale. Reference adapters must support outward point enclosures via `bound(segment,t,t,...)` / frame
   `boundJet(t,t)`. Strict mode forms `F(mid)` by subtracting those enclosures; floating jets are seeds/preconditioner
   estimates only. Missing/invalid/unbounded point enclosures refuse certification. Existing adapters/default
   behavior stay unchanged; do not replace this requirement with a larger heuristic epsilon.
3. Build the exact-D reference graph, independently of the current engine's tolerance heuristics.
   - The public result is immutable and factory-owned. It contains the complete event analysis, source-parameter
     spans in traversal order, retained-edge/node incidence, physical connected-component edge sets, endpoint kinds,
     conservative node/fringe bounds and the caller budget's final cumulative work. A component factory may extend the
     existing private event builder before publication or retain equivalent issuer-bound continuation state privately.
     It must not expose refinement handles, use diagnostic IDs as handles, rerun event isolation, or create a fresh
     budget from `ReferenceEventResult::work`.
   - `ReferenceOccurrence::parameter` is a certified root enclosure, not an exact scalar. For each own occurrence,
     use its original private pair/critical issuer to contract until the offset image of the **entire parameter
     enclosure** is within the node's already allocated world-error bound (including local-to-world conversion and
     rounded-node error). Treat that residual interval as a boundary fringe, not as an omitted gap. Partition the
     source domain into left fringe, event-free core and right fringe without overlap or uncovered parameter ranges.
     Prove the retained side of each fringe from its one-sided event classification. If the fringe cannot be bounded,
     oriented or contracted within the shared limits, return the precise ambiguity/limit reason and no analysis.
   - Between consecutive ordered own occurrences, classify the complete event-free core. A representative value may
     seed or select a sign only after the complete strict event set proves there is no visibility-boundary root in
     that core. Bound the exact restricted-source clearance over the whole core; midpoint, dense sampling, nearest-point
     projection and the engine's fitted controls are not a proof. Include curvature/cusp breaks, both open-source
     cutters, all own/opposite crossings and every critical occurrence in the partition. An unresolved threshold,
     root order or one-sided fringe state subdivides within budget or refuses.
   - Represent every retained core as a directed edge in increasing source traversal parameter, with its two proven
     physical endpoint node IDs and attached fringe bounds. Keep every incidence at transverse crossings and cusps;
     never choose a successor by array order, nearest tangent or an arbitrary crossing pairing. Physical connected
     components are weakly connected components of this embedded directed multigraph, with their edges reported in
     source-parameter order and their full node-incidence lists preserved. This is the component definition, not a
     claim that a branchy component is one cacheable path; `.b-check-topology` must refuse a candidate when it cannot
     prove complete ordered coverage of exactly one component. A cusp has separate one-sided incidences. A whole-source
     seam records an explicit T-to-0 adjacency while retaining distinct start/end occurrence IDs that share a proved
     node. Endpoint-circle cutters classify edges and are never output edges.
   - Merge node identities only when the existing strict identity proof establishes one physical node; proximity,
     overlapping root boxes or a shared rounded coordinate do not merge distinct events. When one own occurrence is
     shared by multiple pair events, retain every pair/incidence on that occurrence. Add a successful native fixture
     for this case; the tangent depth-cap test is only a no-partial refusal and does not satisfy it. If no successful
     case can be produced, do not claim this card ready: implement a conservatively complete refusal for that class,
     mark downstream required-success fixtures that need it unresolved, and add a named prerequisite/fix slice rather
     than inventing graph pairings.
   - The reference graph uses the exact physical distance `D=abs(signedDistance)`. It must not classify at the
     engine's `rho=D-(requestedGeometry+classification)`, and the engine's `rho<=numericFloor` bypass, evaluation
     recheck, marginal-size, dangling-fragment and tiny-fragment rules are not geometric proof. Keep these behaviors
     in a distinct engine-compatibility qualification (`.b-check`/`.c`). A cache is eligible only if it maps the full
     exact reference topology without a component-count/connectivity change and satisfies the existing complete
     geometry/clearance budgets (`2*tol` and `D-2.5*tol`, including all approximation/conversion/node errors). The
     `[rho,D)` band may explain boundary displacement within those budgets; it cannot silently create/delete a
     component or change incidence. If the engine's exact-component topology differs, keep plain geometry and record
     the disagreement; never redefine exact visibility from engine output. Preserve existing engine behavior and its
     tests; do not share/refactor `lc_curveoffset.cpp::visibility()` unless a separately reviewed slice proves the
     accounting and parity contract.
   - `distanceToOriginal()` in the committed helper measures against the whole ellipse and cannot replace clearance
     to the restricted source sweep. For inward offsets use the qualified identity below and prove phase membership,
     endpoint-minimum switching, and the squared predicate with outward/exact bounds over the entire parameter box.
     Outward offsets use the convex-support proof with the actual source parameter as a distance-D witness. All other
     cases require a complete restricted-source bound or refusal.
   A qualified bounded analytic option for ellipse visibility is the exact inward-offset identity, with a² the exact
   raw major-axis squared norm, physical D>0, h(theta)²=r²*cos(theta)²+sin(theta)² and Q=E(theta)-D*N(theta):
   `|Q-E(phi)|²-D² = 2*(1-cos(theta-phi))*[a²*h((theta+phi)/2)²-D*a*r/h(theta)]`.
   Theta MUST belong to the actual source sweep to supply its exact distance-D witness. The exact predicate is
   minimum distance to the restricted source >= D; equality is retained as a boundary node, not rounded into hidden
   or visible by an epsilon. For the whole source, visibility iff D<=a*r*h(theta). For a restricted ordered
   unwrapped real-angle source[A,B], minimize sin² on
   [(theta+A)/2,(theta+B)/2]: a contained integer REAL-pi multiple gives zero; otherwise minimum is one of the
   two endpoints. Reversal changes mapping, not the physical criterion. With Hmin=min_phi h((theta+phi)/2)²>0,
   the equivalent squared predicate is `a²*Hmin²*h(theta)² >= D²*r²`, avoiding square-root cancellation. Membership,
   endpoint-minimum switching and the predicate need outward/exact proof over ENTIRE parameter intervals, not
   approximate phases or midpoint signs. Unresolved equality subdivides within budget or refuses. Degenerate
   circles/stationary/multiple-witness cases still require proper event/topology treatment. Whole-source invisibility
   does not imply restricted-source invisibility. Every comparison/subdivision is charged before work/allocation.
   Coordinator algebra derivation and independent Gibbs review qualify this option; standalone 113-bit vector-versus-
   factor oracle (72,675 checks) compiled and ran successfully across rotations/scales/ratios/distances, but is not
   product interval/runtime proof
   (`build/evidence/1A-1.b-ref-events-isolate/clearance-identity-*`). Exact visibility may differ from the engine's
   rho=D-(requestedGeometry+classification) and evaluation/marginal rules; qualify required fixtures and preserve
   those allowances explicitly, never widen the `2*tol`/`D-2.5*tol` contracts or silently discard retained fragments.
4. Share one checked work-accounting object across event isolation, subdivision and all distance/intersection queries.
   `maxVertices` is the total generated reference/cache leaves, not a fresh allowance per span or nested query;
   do not refund replaced/discarded validation leaves to create unlimited work. Pair/depth/piece caps include
   failed/refined queries. Debit `LC_IntersectionResult::work.totalBoxPairs` on every status and pass only remaining
   allowance into each call. Strict queries additionally debit the generated-item statistic specified below (all
   preparation boxes/certificate records); do not debit its included retained leaves a second time. The legacy
   generatedLeaves statistic is not complete strict preparation/refinement accounting. Count the adapter's additional
   subdivisions/visibility boxes explicitly rather than pretending they are included in solver statistics.
   Bound fixed-iteration numerical helpers and check that adaptive steps actually progress. Check before allocation
   or increment and use overflow-safe arithmetic. Exhaustion returns `LimitExceeded` with no partially successful
   reference result.
5. Qualify these exact component outcomes before declaring the adapter done (angles are radians in `RS_EllipseData`;
   the signed distance is the `ReferenceFrame` input and must be cross-checked against `engineSignedDistance()`):
   - Whole outward: `basis(10, 0.5)`, signed distance `+3`; one closed retained component.
   - Whole inward, untrimmed: `basis(10, 0.5)`, signed distance `-1`; one closed retained component.
   - Whole inward near-critical lens: `basis(100, 0.6)`, signed distance `-36.036`; one closed component with the
     two certified lens crossings and no hidden wing.
   - Open retained arc: `basis(10, 0.5, pi/3, 2*pi/3)`, signed distance `-2`; one component with two open boundaries.
   - Closed result from an arc: `basis(100, 0.5, 10deg, 350deg)`, signed distance `-49.5`; one closed lens
     component.
   - Multi-component arc: `basis(100, 0.05, 15deg, 315deg)`, signed distance `-4`; exactly two components, one open
     offset-by-start component and one closed lens. The existing engine case is
     `lc_curveoffsetellipsetests.cpp`, "Trimmed elliptic arcs keep every part that is left".
   Requalify against the exact graph and an independent restricted-distance oracle, not engine spline counts or
   closest-point samples. Repeat the nontrivial families after rotation, reversal and a representable large-centre
   translation. Add a successful pair-multiplicity fixture in which one own occurrence participates in multiple
   pair events, exact threshold/equality and adjacent-representable threshold cases, distinct close nodes, cusp,
   free-end and seam incidence, plus exact/one-less/pre-spent/sticky-retry budget variants. Every fixture asserts
   complete edge coverage, component count, endpoint/node identity and that all retained fringes fit their node error.

`1A-1.b-check` subsequently owns the layout, bounds and topology leaves: conservative whole-span error bounds,
source clearance, bidirectional ordered coverage and candidate topology. Add explicit tests for a short sub-arc
taken from a valid cache, one disconnected
component taken from a multi-component result, a repeated traversal and a reversed traversal with/without the flag.
All must refuse for the specified reason rather than passing because their points lie near the correct locus.
`.c` records acceptance and refusal distributions, reproducible seeds and cost/work counters. A passing sample-only
prototype or a statement that an API will be chosen later does not make `.b-ref` or `.b-check` complete.

### 3.6 Grips, `moveRef`, snaps

- **`rebuildGrips()`**, called by the constructors and after every cache change:
  1. For each vertex parameter η ∈ {0, π/2, π, 3π/2} inside the sweep, take `O(η)`; keep it if it is within `2·tol` of
     the cache's Bezier geometry (an inward lens drops the major-axis vertices).
  2. An arc also gets `O(η_mid)` of its sweep midpoint, or, failing that, the cache point nearest to it.
  3. Deduplicate within `tol`. If nothing is left, use the cache's arc-length midpoint.
  4. Store the grips in cache order, and reverse them with the cache.

  Typical counts: 4 on a whole ellipse, 2 on a lens, up to 5 on an arc before deduplication.
- **`getRefPoints()`** returns cached grips only while `hasEditableDefinition()` holds; otherwise it delegates to the
  spline. Its consistency fingerprint is allowed, but it never rebuilds grips, runs adaptive validation or regenerates
  geometry during painting (`lc_graphicviewrenderer.cpp:342-358`).
- **`moveRef()`** does nothing while `hasEditableDefinition()` holds. It must still override `RS_Spline::moveRef`,
  which moves any control point within 1e-4 of the grip. For an undefined **or inconsistent** curve, delegate to the
  base spline so the generic control-point drag remains usable, without restoring editability or restamping stale
  definition data. Editable grip drag is the default action's job (1H); before 1H, it changes nothing.
- **`throughParameters(p)`:**
  - d′ is the distance from p to the **whole** original ellipse;
  - "towards the centre" means p lies inside the whole original (its implicit equation in the original's frame);
  - returns false if `d′ ≤ toleranceFor(original, d′, build default)`.
  - for an arc, require a usable occurrence inside its source/retained interval. After the single regeneration,
    `setThrough` verifies the requested point is on the resulting cache within tolerance; otherwise it refuses without
    changing the entity. A normal foot outside the arc or on a removed loop must not be presented as a through-point edit.
- **Grip identity in 1H.** The default action passes the grip value itself (`getNearestSelectedRefInfo`), so an exact
  match is the norm. Otherwise compare local deltas using a geometric tolerance plus a small explicit ULP allowance;
  never multiply `1e-9` by world-coordinate magnitude. If more than one grip qualifies, use a preserved grip index or
  refuse the ambiguous drag, not an arbitrary first match. A small ellipse near 1e12 must not treat distant grips as
  identical. Grip picking tolerance is separate from the engine's approximation tolerance.
- **Snaps** (all reached through the snapper, `rs_snapper.cpp:959-962`):

  | Snap | Whole original | Arc |
  |---|---|---|
  | `doGetNearestCenter` | the original's centre | the original's centre |
  | `doGetNearestEndpoint` | nearest grips/corner points, never the seam | base endpoints for open cache; grips/corners with no seam for a closed cache |
  | `doGetNearestMiddle` | the grips | the base implementation |
  | `doGetNearestDist` | invalid (`RS_Vector(false)`, `*dist = RS_MAXDOUBLE`) | the base implementation |

  - Always write `*dist`, and write `*entity` / `*centerEntity = this` whenever the result is valid (template
    `rs_ellipse.cpp:827-833`). The container's version leaves its out-pointer uninitialised
    (`rs_entitycontainer.cpp:1246-1258`).
  - An undefined or inconsistent curve uses the base implementations. An arc source does not force an endpoint snap
    on the seam of a closed result.

### 3.7 Transforms

- **`move`, both `rotate`s, `mirror`, uniform `scale`:** if `carry` (§3.4), apply the same transform to the original
  through a temporary `RS_Ellipse` and to `m_grips`; for a scale, multiply the distance by |k|; then normalise and
  restamp. `RS_Ellipse`'s `scale` and `mirror` re-derive the angles and toggle `reversed` correctly
  (`rs_ellipse.cpp:1204-1256`, `:1299-1324`).
  Validate transform inputs first. Clear owned unknown extensions only for an actual successful geometry change;
  preserve them for a no-op or refusal. Test factors on both sides of the uniform-scale classification tolerance.
- **`revertDirection()`:** base reversal (it also mirrors the knots); toggle `cacheReversed`; reverse `m_grips`;
  restamp and clear unknown owned extensions when the definition is editable; an inconsistent cache must not be
  laundered into consistency by reversal.
- **`stretch`** is inherited: it calls the virtual `move` when the curve lies wholly inside the window, and otherwise
  leaves the geometry unchanged (`rs_entitycontainer.cpp:1838-1854`).
- **Measured** on the prototype: 1,500 curves through six random transforms each still passed the check (worst 0.42 of
  its tolerance).

### 3.8 Engine runs: `runEngine`, `adopt`, `create`, mutators (1B)

- **`runEngine(next, limits, out)`:**
  - rejects invalid/non-finite definition fields, `d ≤ tol`, and invalid limits before constructing expensive geometry;
  - builds `RS_Ellipse basis{nullptr, next.basis}`;
  - side = `(next.towardCentre != next.basis.reversed) ? Left : Right`;
  - options = `makeOffsetOptions(basis, next.distance, toleranceFor(basis, next.distance, next.relTolerance))`,
    clamped by the limits as §3.1 says;
  - calls `createEntities(basis, makeSideRequest(side, next.distance), options, limits.budget)` and increments
    `engineRuns()`.

  Result mapping:

  | Engine outcome | Refusal |
  |---|---|
  | status ≠ Ok | `Engine` (with `engineStatus`) |
  | Ok with 0 entities (the engine's own `shrinksAway`, `lc_curveoffset.cpp:5141-5145`, `:5215-5221`) | `NothingLeft` |
  | more than one entity | `SeveralPieces` |
  | one result cannot be normalized losslessly to degree-1/degree-3 ClampedOpen cache data | `NotCacheRepresentable` |
  | §3.5 fails | `CheckFailed`, with its `checkReason` preserved |

  Input guards do not duplicate the engine's geometric "nothing left" decision. Keep that decision in the engine.
- **`adoptCache(candidate, next)`:** `getData() = candidate;` (closed flags as the engine made them: false);
  `m_def = next;` then `if (next.cacheReversed) RS_Spline::revertDirection(); else update();` (both rebuild the
  chords); then update validated closure, rebuild grips and set `m_stamp = cacheFingerprint(getData())`. The arc-length table is keyed by the
  fingerprint and refreshes itself (`rs_spline.cpp:878-889`).
- **`adopt(engineResult, def)`:** normalise `def.basis`; require exactly one result. Convert an `RS_Line` to exact
  degree-1 ClampedOpen cache data (`[start,end]`, knots `[0,0,1,1]`, unit weights); copy a supported degree-1 or
  degree-3 ClampedOpen `RS_Spline` without refitting. Reject other entity/data shapes with
  `NotCacheRepresentable`. Run §3.5 with `tol` from `def.relTolerance`; construct through the private validated-tag
  copy-from-spline constructor. Preserve common entity attributes (parent/owner, layer, pen, visibility and supported
  metadata) from `engineResult`, including the line path; geometry normalization must not reset drawing attributes.
  1C wraps engine results that already ran under the batch's budget this way, and 1E
  adopts loaded splines this way. **Neither ever calls `create()`.**
  The definition passed to the validator includes `cacheReversed`. A loaded cache is already in its saved direction;
  the copy-from-spline adoption constructor must not reverse it again. `adoptCache` applies reversal only to fresh
  engine output produced in the source's original direction.
- **R05 one-piece regression:** test the exact `RS_Line` normalizer directly and through an engine result whenever a
  checked-in/admitted ellipse case produces that entity. Assert editable `EntityOffsetCurve` construction, exact
  endpoint/interior line geometry, degree-1 controls/knots/unit weights, parent/layer/pen/visibility preservation and
  no second engine run during adoption. If the current producer corpus does not yield an `RS_Line`, the direct
  normalizer test remains mandatory; empirical unreachability cannot delete the supported producer branch. Multi-span
  degree-1 and cubic producer results receive separate adoption cases.
- **`create(source, d, toward, limits)`:**
  1. Build `def{normalised source data, d, toward, false, relativeOffsetTolerance(source)}`.
  2. `runEngine` produces and validates one staged cache, then construct through the private validated-tag path.
     Reuse internal construction code without calling the public `adopt` validator a second time. Public `adopt`
     always validates untrusted/loaded data; it must not gain a caller-controlled "already checked" bypass.
  3. Copy pen and layer from the real source: the engine copies them from the temporary
     (`lc_curveoffset.cpp:5322-5326`).
- **Mutators:**
  1. Check defined/consistent state at runtime; return `Undefined` or `Inconsistent` before touching any fields.
  2. Validate and normalize the proposed change. A true no-op returns success without regeneration or clearing extensions.
  3. Set `next = m_def` with all changed fields and the build-default factor (D12), then call `runEngine` once.
  4. Validate the complete candidate before `adoptCache`; on geometric/refusal results, no cache, definition, grips,
     stamp, attributes or unknown extension changes. Clear owned future extensions only after successful change.

  `setThrough(p)` builds one candidate containing both distance and side; it does not call the two public mutators
  sequentially. Keep the candidate off the live object until all checks pass. Do not turn an allocation exception
  during rebuild into a reported ordinary refusal after mutation; follow the application's error/exception policy and
  claim a stronger exception guarantee only if the actual data/child ownership swap supports it.
- **`setBasis` fast path.** It applies only when the new original is the old one moved, or rotated about the old
  centre and then moved:
  1. Derive the motion M from the centres and major-axis directions; try +majorP, and −majorP with the angles
     shifted by π.
  2. Accept M only if applying it through `RS_Ellipse::rotate` / `move` reproduces centre, majorP, ratio, angle1,
     angle2 and reversed within 1e-12 relative.
  3. Apply and validate M on a candidate, then publish it, clear owned unknown extensions and restamp. A failed
     fast-path check leaves the live object unchanged and can use one normal engine attempt.

  Anything else, reflections included, costs one engine run. An arc with the same axes and sweep but another start
  angle is not congruent: moving the cache there would give a wrong curve.

### 3.9 Where the type meets the rest of LibreCAD

- **Creation** (1C-1): see the wrap code shape there.
- **Offset again** (1C-2): `LC_OffsetCurveAgain` decides per copy whether to rebase on the original.
- **Explode, non-uniform scale, plugin scale** (1C-1); **INSERT** expansion with unequal factors (1A-2).
- **Copy, paste, blocks:**
  - inherited copy semantics;
  - paste scales uniformly (`lc_copyutils.cpp:266-270`), so the distance scales;
  - tests in 1E-2.
- **Trim on a chord of the curve:** single Trim must map the generated chord to its `LC_OffsetCurve` parent; Trim Two
  must respect #3113's landed guard and leave the owned child intact (1D, release gate §0.2).

### 3.10 File format (schema 1) and its rules (1E)

**Read/write contract.** The cache is an ordinary SPLINE. DXF stores the definition in XDATA, and DWG stores the same
typed values in EED associated with a registered `LibreCad` APPID. Other readers can display the cached spline without
understanding the private definition. Both LibreCAD readers must restore `LC_OffsetCurve` from valid schema-1 data;
both writers must serialize the current definition. No new DWG custom class or application-specific file container
is needed. Track each direction independently: writer success does not prove reader support, and an application
self-round trip does not independently qualify either direction.

The mandatory format lanes are:

| Lane | Read requirement | Write requirement | Scope |
|---|---|---|---|
| Application ASCII DXF | Read SPLINE/XDATA and adopt valid schema-1 as editable | Write current SPLINE/XDATA and registered APPID | All eight DXF rows below; R12 is geometry-only |
| Binary DXF codec | Decode schema payload and import it through the application adoption path | Encode the same schema payload and inspect the actual binary output | Low-level codec at R14 and R2018; the application export interface currently writes ASCII DXF only |
| Native DWG | Read SPLINE/EED and adopt valid schema-1 as editable | Write current SPLINE/EED with resolvable APPID linkage | Six writable DWG rows below; legacy R13/R14 is input-only |
| DXF ↔ DWG conversion | Read a source file and preserve its editable definition | Write the destination file with that definition intact | Both conversion directions; tested separately from same-format round trips |

The mandatory application-level version matrix at the reviewed baseline gives reader and writer expectations
separately. Each cell must have a test receipt; a combined save/open result cannot substitute for either direction.
For editable schema-1 round trips, exercise both supported cache degrees (1 and 3) in each required reader/writer
lane; a cubic-only fixture does not close the degree-1 line-normalization contract. R12 remains geometry-only and must
preserve degree-1/degree-3 cache geometry within the declared approximation tolerance without restoring editability.

| Format | File version | `RS2::FormatType` | Reader expectation | Writer expectation |
|---|---|---|---|---|
| DXF R12 | AC1009 | `FormatDXFRW12` | POLYLINE imports as ordinary geometry; no editable definition is inferred | POLYLINE geometry within export tolerance; no editable offset definition |
| DXF R14 | AC1014 | `FormatDXFRW14` | Independently authored degree-1/degree-3 SPLINE + schema XDATA fixtures restore editable curves | SPLINE + schema XDATA for both degrees |
| DXF R2000 | AC1015 | `FormatDXFRW2000` | Independently authored degree-1/degree-3 SPLINE + schema XDATA fixtures restore editable curves | SPLINE + schema XDATA for both degrees |
| DXF R2004 | AC1018 | `FormatDXFRW2004` | Independently authored degree-1/degree-3 SPLINE + schema XDATA fixtures restore editable curves | SPLINE + schema XDATA for both degrees |
| DXF R2007 | AC1021 | `FormatDXFRW` | Independently authored degree-1/degree-3 SPLINE + schema XDATA fixtures restore editable curves | SPLINE + schema XDATA for both degrees |
| DXF R2010 | AC1024 | `FormatDXFRW2010` | Independently authored degree-1/degree-3 SPLINE + schema XDATA fixtures restore editable curves | SPLINE + schema XDATA for both degrees |
| DXF R2013 | AC1027 | `FormatDXFRW2013` | Independently authored degree-1/degree-3 SPLINE + schema XDATA fixtures restore editable curves | SPLINE + schema XDATA for both degrees |
| DXF R2018 | AC1032 | `FormatDXFRW2018` | Independently authored degree-1/degree-3 SPLINE + schema XDATA fixtures restore editable curves | SPLINE + schema XDATA for both degrees |
| DWG R2000 | AC1015 | `FormatDWG` | Independently authored degree-1/degree-3 SPLINE + schema EED fixtures restore editable curves | SPLINE + schema EED for both degrees |
| DWG R2004 | AC1018 | `FormatDWG2004` | Independently authored degree-1/degree-3 SPLINE + schema EED fixtures restore editable curves | SPLINE + schema EED for both degrees |
| DWG R2007 | AC1021 | `FormatDWG2007` | Independently authored degree-1/degree-3 SPLINE + schema EED fixtures restore editable curves | SPLINE + schema EED for both degrees |
| DWG R2010 | AC1024 | `FormatDWG2010` | Independently authored degree-1/degree-3 SPLINE + schema EED fixtures restore editable curves | SPLINE + schema EED for both degrees |
| DWG R2013 | AC1027 | `FormatDWG2013` | Independently authored degree-1/degree-3 SPLINE + schema EED fixtures restore editable curves | SPLINE + schema EED for both degrees |
| DWG R2018 | AC1032 | `FormatDWG2018` | Independently authored degree-1/degree-3 SPLINE + schema EED fixtures restore editable curves | SPLINE + schema EED for both degrees |

At S0, reconcile this explicit table with `RS2::FormatType` and `RS_FilterDXFRW::fileExport`; never infer versions from
enum ranges. Guard DWG-only format references with `DWGSUPPORT`; the shared schema module uses ordinary DRW variants
and has no dependency on a DWG-only API. Standard CMake targets currently define `DWGSUPPORT`. The application
currently writes ASCII DXF. Binary DXF read/write at R14 and R2018 is a low-level codec obligation; binary input
fixtures must enter through the normal adoption callback and be independently sourced, while codec-generated output
is inspected separately. The current application export interface remains the entry point for ASCII DXF. Legacy
DWG R13/R14 remain input-only through the existing readers; the six DWG rows above define the required write targets.
Accepted legacy SPLINE/EED input still reaches the common adoption callback.

The schema below is identical in DXF XDATA and DWG EED. A valid schema restores the saved definition without rerunning
the offset engine. Missing or invalid schema leaves usable spline geometry. Unknown schema versions stay raw metadata
and do not confer editability. Save/open does not require the original source ellipse to remain in the document.

```text
1001 LibreCad
1000 LC_OFFSET_CURVE
1002 {
1070 1                       schema
1000 ELLIPSE                 kind of original
1011 x y z                   centre, world
1012 x y z                   major-axis displacement, world (= majorP; not an endpoint)
1040 ratio                   0 < ratio <= 1
1040 angle1                  normalised to [0, 2pi); a whole ellipse writes 0, 0
1040 angle2
1070 flags                   bit0 original reversed, bit1 towards the centre, bit2 cache reversed,
                             bit3 left of the original's direction (redundant, for other software);
                             bits 4-14 reserved (unknown bits of a newer writer are kept); always < 2^15
1041 distance
1040 relative tolerance factor
[trailing items of a newer writer, kept]
1002 }
```

Store the axis directly as displacement code 1012 rather than subtracting two large world-space points. This avoids
loss of a small axis at a large centre and gives the typed displacement its intended transform behavior. Schema 1
has not shipped; earlier private prototypes using a second 1011 endpoint must be regenerated. Their old field-code
layout fails strict schema-1 adoption and preserves plain spline geometry rather than being misinterpreted. The field
codes, types and order are part of the schema contract. This feature remains 2D: reject non-finite or nonzero out-of-plane
definition values and nonrepresentable geometry instead of silently claiming an editable 3D offset.

**Writing** is shared by both formats. Define `using XData = std::vector<std::shared_ptr<DRW_Variant>>` and
`std::optional<XData> forExport(const RS_Spline& s, XData stored, bool includeDefinition = true)`. The optional result
lets an unsafe metadata merge fail the export through `m_writeFailed`; it must not silently save a defined curve
without its definition. `includeDefinition = false` is used by the R12 downgrade. The writer:

1. Split `stored` at every 1001. Items before the first 1001 stay.
2. A `LibreCad` group is one whose 1001 text equals `LibreCad` ignoring case, **without trimming**. This matches the
   codec/APPID registration identity rather than the unrelated layer-extension normalization. `" LibreCad "` is a
   distinct application's identifier and must remain foreign data; never merge it into or strip its payload as ours.
3. Remove recognized old schema-1 offset sub-lists before adding the current definition, including when `s` has become
   a plain, undefined or inconsistent spline. Preserve unknown schema versions and unrelated XDATA. Remove a group
   header only if it is left empty. If remaining stored data prevents a safe merge, return `std::nullopt` and propagate
   failure; do not discard unrelated data or fabricate a successful semantic save.
4. When `includeDefinition` and `s` is a defined, consistent `LC_OffsetCurve` with valid fields (finite numbers;
   0 < ratio ≤ 1; distance > 0; factor within 0.1–10× the build default; flags in [0, 2^15)), merge the items of every
   `LibreCad` group into the first, keeping its header spelling. Append exactly one canonical sub-list with
   `m_unknownFileTail` and `m_unknownFileFlags`. Other applications' groups keep their relative order. Invalid current
   definition fields fail serialization instead of omitting the entity or reporting a semantic save as successful.
   Geometry-only fallback for undefined/inconsistent curves contains no stale schema-1 sub-list.
   The writer uses the same owned-tail type/byte/depth validation as the reader; public tail setters cannot bypass it.
5. Never edit a stored variant, because clones share them (`rs_entity.cpp:63-70`); reuse the `shared_ptr`s and make
   new ones for new items.

Call this before the DXF/DWG branch in `writeSpline`, then pass the resulting `DRW_Spline` to `dxfRW::writeSpline` or
`dwgRW::writeSpline`. Use it with `includeDefinition = false` for R12's POLYLINE before that branch returns. Trace
`DRW_Spline` encode/parse and the common EED read/write path during 1E-1; repair any demonstrated version-specific
loss in libdxfrw and cover it with a codec regression. A working DXF path alone does not establish DWG support.

Reasons:

- one bad item fails the **whole** save, in DXF (`libdxfrw.cpp` `writeExtData`) as in DWG;
- two `LibreCad` groups on one entity become two EED chunks with one APPID in DWG;
- a group that ends with an open brace fails the DWG save (`drw_entities.cpp:4542-4544`).

**Reading** is done by `LC_OffsetCurveXData::parseAndStrip(const std::vector<...>& stored)`, which returns
`Parsed{status (NoSubList | Schema1 | Invalid | Other), def, unknownTail, unknownFlags, remaining, changed, diagnostics}`.
Diagnostics distinguish malformed owned data, duplicates, inconsistent flags, unsafe tails and geometric-adoption
refusal. They are bounded enum/count data, not user-visible strings in the engine layer. Only `Schema1` may request
adoption; all other statuses preserve the visible ordinary spline.

1. In each `LibreCad` group, find `1000 LC_OFFSET_CURVE` followed by `1002 {` at brace depth 0, and identify its schema
   discriminator and matching `}` with a bounded, linear scan. Markers inside another list are not ours. A marker
   followed by a different token is unrelated data; do not delete arbitrary neighboring data to repair it.
2. A correctly typed first 1070 value other than 1 is an unknown schema: keep its complete raw range (`Other`), never
   inspect or strip nested apparent schema-1 markers. If its braces are malformed, preserve it with a diagnostic and
   let an unsafe subsequent save fail through the existing codec; raw preservation is not a promise to repair an
   unknown format. Missing/ill-typed discriminators are malformed owned data (`Invalid`), not a guessed schema 1.
3. For known schema 1 or malformed owned data, strip only the identified owned range. If its closing brace is missing,
   strip from that owned tag to the end of its application group, not into the next 1001 group. More than one
   schema-1 candidate, including a malformed one, makes adoption invalid; remove all known owned candidates and
   adopt none. Unknown-schema candidates stay raw and do not erase foreign application data.
4. For exactly one:
   - parse its ten items using exact code/type/order checks; read the major-axis displacement directly from 1012;
   - verify flag bit 3 equals `(towardCentre != basis.reversed)`; reject contradictory redundant side data instead of
     selecting whichever interpretation happens to fit the cache. Bits 4-14 are preserved reserved data, not behavior;
   - filter the trailing items: keep only 1000, 1002 (balanced), 1010–1013, 1040–1042, 1070 (int16) and 1071; drop
     anything else (1004 hex text, 1003/1005 references). Bound the owned tail by 64 items, 4096 encoded bytes and
     nesting depth 8, with per-item type/range/finite/string checks matching the codec. Account for target encodings,
     not just UTF-8 character count. If a whole balanced safe tail cannot be retained, discard that owned unknown tail
     and record the reason; do not truncate through braces or remove foreign application data;
   - remove the sub-list, and remove a `LibreCad` header left with no items.
5. Bounds:
   - factor within 0.1–10× the default;
   - distance finite and > 0;
   - ratio finite and > 0;
   - angles finite, classified through the raw `sweep`/`normaliseBasis` contract of §3.3 before construction; preserve
     its full-turn comparison allowance and directed wrapped-arc rules, rather than adding a conflicting strict-2π
     precheck or letting `RS_Ellipse` snap tiny noncanonical angles into whole geometry;
   - axis length > 0;
   - then `normaliseBasis`.

Check bounds before copying/reserving payloads. The current DWG EED chunk size is a uint16 payload length and the
encoder cannot split an open brace group; preflight the owned balanced sub-list and propagate codec overflow/type
failures. A small item count alone is not a byte-size bound. No-edit round trips preserve safe owned unknown fields;
successful geometry changes clear those fields/reserved bits under D18, while refusals/no-ops keep them unchanged.
Status precedence is `Invalid` (malformed/duplicate owned candidates), then a unique valid `Schema1`, then `Other`,
then `NoSubList`. An unknown version can coexist with a valid schema-1 list and stays in `remaining`; it does not
override that list's validated meaning or cause a second definition to be emitted. A malformed known candidate
cannot be rescued by silently selecting a later valid one.

**Adoption** happens only on `addSpline`'s main path, and only for `data->eType == DRW::SPLINE`. The loaded spline must
be degree 1 or 3, ClampedOpen (its closed and periodic flags may be set; ODA sets them), in the matching cache layout
of §3.5, with all weights exactly 1.0 (`makeSplineFromDrw` fills 1.0 when the file has none), and it must pass §3.5. Use
the public validated `LC_OffsetCurve::adopt(*spline, def, why)` factory; its private copy-from-spline constructor keeps
what `setEntityAttributes` set. `addHelix` strips but never adopts.

**Raw-carrier eligibility comes first.** Before `makeSplineFromDrw`, check the original `DRW_Spline` carrying a
schema-1 candidate: actual/vector declared counts agree, all controls exist and have finite XY with Z exactly zero,
the normal is finite and is world-XY or the codec's unspecified zero sentinel, degree-1 or degree-3 layout/knots are
present and valid under their respective control/knot count and multiplicity rules, unit weights are either genuinely
absent or complete (never a partial list padded into validity), and no fit-only or repaired
representation is needed. Do not trust only group-70 planar/rational flags. Keep these checks scoped to editable
offset adoption; ordinary unmarked spline import retains existing behavior.
`DRW_Spline::normalVec` is zero by default and the DWG parser resets it to zero; DWG control-scenario payloads do not
carry a DXF-style normal. Accept that unspecified value only with the complete raw 2D control check, and reject an
incompatible explicit plane. Requiring `(0,0,1)` from every DWG callback would wrongly reject every native round trip.

The current converter drops control Z, rounds knots to `tolknot`, synthesizes missing knots, fills missing weights,
and may clamp/reclassify geometry. None may manufacture eligibility. For an eligible marked carrier, construct the
candidate from the validated raw controls/knots without shape-changing rounding/repair (implied unit weights may be
materialized), then run the same full `adopt` check. If raw eligibility fails, strip only known owned schema and use
the existing plain import fallback with a bounded diagnostic. This does not add 3D support or require dropping the
visible ordinary projection. Test raw out-of-plane/invalid normal, null controls, declared-count mismatches, absent
or collapsing knots, partial/nonunit weights and fit/repaired inputs in both codecs; a repaired visible spline must
never acquire an editable definition that the original file geometry did not justify.

The same `addSpline` path must be reached by actual DXF and DWG imports. Preserve layer, pen, visibility, block owner
and supported foreign XDATA/EED when replacing the plain spline with the editable entity. Check a low-level callback
capture of each saved file as well as the document entity, so a round trip cannot pass solely because both ends of the
new adapter share the same metadata mistake.

**Other outputs.**
- R12 writes and reloads as a POLYLINE without any owned schema-1 sub-list; preserve supported foreign XDATA through
  the existing codec. Verify degree-1 and degree-3 geometry against the exported spline within the R12 approximation
  tolerance.
- Hatch boundary edges carry no XDATA: an offset-curve edge is written as a SPLINE edge and reloads as
  `LC_SplinePoints`, as every spline edge does.
- #3119 registers XDATA application names. Assert `LibreCad` appears exactly once under case-insensitive APPID lookup,
  matching the spelling used by the emitted group; use the existing registration mechanism.

**Existing publication boundary.** DXF `dxfRW::writeImpl` already writes through `DwgDxfOutputTransaction` and calls
`iface->finalizeDxfWrite()` before commit; `RS_FilterDXFRW::finalizeDxfWrite()` returns `!m_writeFailed`. DWG also has
its codec transaction, finalization and filter staging. Reuse these mechanisms. Ensure every offset serialization
failure latches the existing filter failure before finalization, including invalid control counts, failed R12
approximation, unsafe metadata and codec rejection. A writer's early return that merely logs a discarded curve is
not acceptable for this type. Do not add duplicate staging or promise filesystem durability beyond the existing
transaction contract. Tests must verify target bytes/absence, stage cleanup and unchanged document/metadata state.

---

## 4. Phase 0 — prerequisites

### S0. Worktree, tracked plan, builds and baseline (documentation slice)

**Execution card, 2026-10-15**
- State: done; implementation/documentation SHA `de30f1d45f1337d55265c2f72413a9c976c18c40`; this plan-only
  reconciliation records that SHA, never its own. Base `b05fb541df651dd9cb55a222dee4563e6127c436` fetched from origin.
- Worktree: `/Users/dli/dev/LibreCAD/.claude/worktrees/lc-offset-curve-2961`; branch `codex/offset-curve-2961`.
- Scope: this tracked-plan preparation and ignored build/evidence artifacts only; no product source edits.
- Toolchain: AppleClang 21.0.0 (`clang-2100.3.34.2`), Qt 6.12.0/qmake 3.1, CMake 4.4.4, Ninja 1.13.2, installed Catch2.
- Release configure succeeded with `BUILD_TESTS=ON`, Qt6_DIR `/opt/homebrew/lib/cmake/Qt6`; ASan Debug configure
  succeeded with address sanitizer/frame pointers. Both built all three required targets with four jobs each; exit 0.
- Gibbs completed the first-slice preflight: 50 contiguous enum values, 27 drawing rows and 23 exemptions. Current
  exempt-row wording must identify Unknown as the fallback sentinel and ConstructionLine as nonpersistent, not nonexistent.
- Evidence: `build/evidence/S0/build-release.log`, `configure-asan.log`, `build-asan.log`, and the reports below.
  All four test commands completed with exit 0; structured XML parsing found no failure/error elements.
- Upstream: all eight commits in §0.1 are in the fetched base's ancestry. The 14-row file matrix matches the actual
  `FormatType` enum and `fileExport` mappings. Current `EntitySpline` seed census: 55 textual occurrences across 37
  non-test files (includes comments; not a claim of 55 dispatch sites). Their dispositions remain explicit work in 1D.
  Focused ASan trim/ellipse checks include the current multi-component, closed-arc and near-critical engine fixtures;
  this qualifies engine baselines, not the not-yet-implemented reference validator.
- Plan audit: 27 status rows with known acyclic dependencies and matching headings, balanced code fences, 14 unique
  format rows (8 DXF/6 DWG), and 50 current enum values; leaf/reference/check cards reconciled. Gibbs's independent
  review findings are integrated into §7, including pending-clone ownership and phase-2 test ordering.
- Post-commit audit: `git show --stat` contains only this roadmap; tracked product sources remain identical to the
  tested b05fb541d tree. Gibbs's follow-up review found no remaining material contradiction in the revised property
  transaction contract. The index was clean before staging and after the documentation commit; untracked ASan build
  artifacts were preserved and not staged.
- Next: after this checkpoint commit, Gibbs starts the ready 0B-1 product card. No product completion is claimed.

| Baseline command (all use `QT_QPA_PLATFORM=offscreen`, `--order decl`, JUnit reporter) | Report under `build/evidence/S0/` | JUnit testcase entries | JUnit `tests` attribute | Failures/errors | Skips |
|---|---|---|---|---|---|
| `build/librecad_tests` (default visible suite) | `librecad.junit.xml` | 4564 | 5855820 | 0/0 | 74 |
| `build/librecad_dxf_fast_tests` + §1.2 CI exclusions | `dxf-fast.junit.xml` | 1018 | 38597 | 0/0 | 3 |
| `build/librecad_dwg_fast_tests` + §1.2 CI exclusions | `dwg-fast.junit.xml` | 1084 | 34952 | 0/0 | 4 |
| `ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests '[trim],[curve-offset][ellipse]'` | `asan-trim-ellipse.junit.xml` | 96 | 778725 | 0/0 | 0 |

JUnit's `tests` attribute is not presented as a count of registered Catch2 cases; generated/section entries and
assertion reporting differ. Keep both fields when comparing reports. The 74 full-suite skips are 73 optional/local
fixture or external-verifier inputs absent in the clean checkout, plus the existing JWW export limitation. Fast-lane
skips are existing missing optional fixtures. Their stable identities/reasons are in the XML; no offset feature test
exists or is hidden by these skips. Leak detection was disabled for the focused macOS ASan run; do not claim a LeakSanitizer
pass or a full-suite ASan pass. External CAD verification and non-macOS compilers have not been run in S0.

1. **Worktree.** Fetch the target branch, record its SHA, and inspect `git worktree list` plus `git status`. Reuse only
   a clean checkout at that SHA; otherwise create `/Users/dli/dev/LibreCAD/.claude/worktrees/lc-offset-curve-2961`
   on `codex/offset-curve-2961` from the fetched target. If either name is already occupied by a different checkout
   or branch, choose a unique suffix and record it. Never overwrite the existing research worktree or its untracked
   plan/build files. Copy this reviewed plan into that worktree at the same repository-relative path and track it.
   Record the actual base SHA, execution worktree and authoritative plan path in the S0 card.
2. **Build.** Configure `build` (Release) and `build-asan` (§1.1), then build `librecad_tests`,
   `librecad_dxf_fast_tests` and `librecad_dwg_fast_tests` in both.
3. **Baseline.** Create `build/evidence/S0/` and run
   `QT_QPA_PLATFORM=offscreen build/librecad_tests --order decl --reporter junit --out build/evidence/S0/librecad.junit.xml`.
   Record the real exit status and counts; run both fast lanes with the current CI excludes and separate reports.
   Read the structured output with an XML parser, recording stable test/section/assertion identities and reasons for
   any existing failures or skips. Review fixture/environment failures before treating them as a baseline exception.
   Record commands, toolchain, tested base SHA and artifact paths in the S0 card; do not commit generated logs.
4. **Baseline reconciliation.** Verify each commit in §0.1 is an ancestor of the fetched target. Re-read the changed
   trim, offset, XDATA and document-load paths; amend affected tests and anchors before implementation if upstream
   diverged from the reviewed snapshot. Reconcile the §3.10 file matrix against the current format enum, export
   mapping and codec version guards; record any added or removed targets before writing the 1E tests.
5. **Readiness.** Refresh the type-site census and qualify the #3117 multi-piece and near-critical fixtures. Record the
   current validation API/budget choices for 1A-1 and the missing facts its children will establish. Promote 0B-1 only
   after its card is concrete. Historical measurements are not copied into current evidence.
6. **Commit and reconcile.** Commit the prepared plan, then record that real SHA and baseline evidence in the S0
   checkpoint (§1.4). No product implementation is included in this slice. After a necessary rebase or toolchain
   change, repeat the affected baseline work and update evidence rather than retaining stale readiness.

### 0A. Trim child ownership prerequisite (landed; verify at S0)

Upstream #3113 (`609ca81ae`) added the ownership-aware replacement rule and Trim/Trim Two regression tests. Do not
reimplement that fix. At S0, run the focused trim tests and inspect the current action implementation. Phase 1D still
has a new type-specific responsibility: single Trim maps a clicked generated chord to its parent curve only for the
recognized spline RTTI values. Add `EntityOffsetCurve` there, and verify Trim Two continues to refuse replacing a
generated child without deleting it or damaging undo state. See the 1D tests and the PR C gate in §0.2.

### 0B. Guard test over every `RS2::EntityType`

**Goal.** A test that fails when an enumerator is neither handled by the writer, by INSERT expansion and by `clone()`,
nor declared "not applicable, because …". Today an unlisted type is silently not written while the export reports
success (`rs_filterdxfrw.cpp:30237-30238`, DXF and DWG), and empties every INSERT of a block that holds it
(`rs_insert.cpp:164`, `:201-203`, `:622-630`).

**What each mechanism catches** (verified on clang 21 and GCC 16.2):
- **`static_assert(countRows() == EntityDimArrowBlock + 1)`** catches an enumerator **inserted before**
  `EntityDimArrowBlock` (such as 1A-2's `EntityOffsetCurve`) on every compiler, with or without the pragma. It cannot
  see one **appended** after it.
- **The pragma-enforced default-less `switch`** catches both. GCC and clang make it an error with
  `#pragma GCC diagnostic error "-Wswitch"`. MSVC needs `#pragma warning(push)`, `#pragma warning(1 : 4062)`,
  `#pragma warning(error : 4062)`, `#pragma warning(pop)`, because C4062 is off by default and CI compiles at `/W1`.
  The MSVC form is unverified until CI shows a dummy enumerator rejected; the Linux GCC lane is the gate.
- **Optional, GCC/clang only:** a `__PRETTY_FUNCTION__` check that `EntityDimArrowBlock + 1` is not a named enumerator.
  It catches the append without any warning.

#### 0B-1. Guard skeleton, registration, CI filter, shadow checks

**Executed card: done at d6de53d3cb80199b4148b0e6d3dbb28f2b392b8c**
- Owner: Gibbs; coordinator owns this plan, shared builds and commits.
- Base: `de30f1d45f1337d55265c2f72413a9c976c18c40` plus this plan-only checkpoint; refresh actual HEAD at handoff.
- Dependency: S0 done at de30f1d45; this executed leaf's implementation SHA is d6de53d3c, recorded above.
- Contract: every current enumerator has an explicit drawing/exempt row; inserted/appended unknown enumerators fail
  compile; no initial known gaps are invented by this kinds-only slice.
- Files: new `librecad/src/lib/engine/document/entities/tests/lc_entitytype_coverage_tests.cpp`, `CMakeLists.txt`,
  `.github/workflows/dwg-dxf-verify.yml` only. No qmake product registration is needed for this test-only source.
- Tasks: implement 50-case exhaustive `rowOf`, compiler diagnostics, row integrity assertions and one kinds-only
  `[entity-coverage]` case; register in full/DXF fast targets; correct both CI path filters; perform shadow compiles.
- Validation: build all three Release targets; run `[entity-coverage]` in full and DXF fast targets (at least one case
  each, pin 50 total / 27 drawing / 23 exempt rows); run both fast lanes with §1.2 exclusions; capture both negative
  compile exit codes/diagnostics. No test uses a QApplication in the fast lane. MSVC negative guard remains CI-only.
- Recovery: preserve edited files and evidence; after a successful product commit, reconcile its actual SHA before
  starting 0B-2. The private skeleton is reference material only, not a required input.
- Review: current enum/CMake/CI locations confirmed by Gibbs; coordinator reviewed the scoped diff and confirmed the
  tested owned-file SHA-256 hashes before committing. This enum-only test depends on the unchanged real rs.h; the
  Release build log compiled only the two new guard objects and linked against the S0 library. Concurrent untracked
  geometry files/tolerance-helper preparation were not part of that build or commit.
- Evidence: `build/evidence/0B-1/summary.md` and logs. Three Release targets exit 0; `[entity-coverage]` passes one case /
  76 assertions in full and DXF fast targets. DXF CI lane: 732 passed / 3 existing skips; DWG CI lane: 1002 passed /
  4 existing skips, no new failures/skips. Positive syntax control exit 0; inserted shadow exit 1 with switch/count
  errors, appended shadow exit 1 with switch error only. Real rs.h unchanged. Clang 21 checked; GCC/MSVC remain CI-only.
- Next: 0B-2 factory and clone/INSERT checks; no full guard-completion claim until 0B-3 is done.

New `librecad/src/lib/engine/document/entities/tests/lc_entitytype_coverage_tests.cpp`, after the compiled skeleton
`prototypes/plan_review_2026-10-14/guard_skeleton.cpp`, condensed:

```cpp
namespace {
enum class Kind : unsigned char { NotAnEnumerator, DrawingEntity, NotApplicable };
enum Check : unsigned { DxfRoundTrip = 1u << 0, DwgRoundTrip = 1u << 1, InsertExpansion = 1u << 2, CloneClass = 1u << 3 };
struct Row { Kind kind; const char* reason; RS2::EntityType reloadsAs; unsigned knownGaps; unsigned notRun; const char* gapNote; };
// drawing(type), drawingReloadingAs(type), withGap(row, gaps, note), notApplicable(why), notAnEnumerator()
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(1 : 4062)
#pragma warning(error : 4062)
#endif
constexpr Row rowOf(const RS2::EntityType type) {
    switch (type) {   // no default: one case per enumerator, one per line
        case RS2::EntityLine: return drawing(type);
        // ... 27 drawing types; 23 not-applicable rows with reasons (see the skeleton file)
    }
    return notAnEnumerator();
}
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
constexpr unsigned countRows() { unsigned n = 0; while (rowOf(static_cast<RS2::EntityType>(n)).kind != Kind::NotAnEnumerator) ++n; return n; }
constexpr unsigned kRowCount = countRows();   // hoisted for MSVC's constexpr step limit
static_assert(kRowCount == static_cast<unsigned>(RS2::EntityDimArrowBlock) + 1u, "RS2::EntityType has a value without a row");
// rowsWellFormed(): a not-applicable row has a reason; a gap has a note; only drawing rows have gaps;
// knownGaps & notRun == 0; reloadsAs is a drawing row. static_assert(rowsWellFormed()).
} // namespace
```

- **The rows** (the reviewed commits through 41678d976 do not change the `EntityType` enum, but the writer, INSERT,
  clone and lifecycle behavior still must be rechecked at S0):
  - Expected initial drawing rows: Insert, Point, Line, Polyline, Arc, Circle, Ellipse, Hyperbola, Solid, MText, Text, the seven
    dimension types, Tolerance, DimLeader, Hatch, Image, Wipeout, MLeader, Spline, SplinePoints, Parabola.
  - 23 not applicable, each with a reason:
    - Unknown: fallback sentinel for unsupported/unclassified entities, not a persistent drawing type;
    - Vertex and OverlayBox: owned geometry/overlay values, not standalone persisted drawing records;
    - Container: a top-level group comes only from the JWW import; it is dropped on save and refused by `RS_Insert`;
    - Block: definitions are written by `writeBlocks`;
    - FontChar: its `clone()` is an `RS_Block` by design;
    - Graphic, Preview: documents;
    - Pattern;
    - ConstructionLine: a concrete nonpersistent construction entity with native INSERT support; XLINE/RAY import as
      `RS_Line`. Its INSERT behavior is pinned explicitly rather than mislabeled as a nonexistent class;
    - OverlayLine, the six `EntityRef*`, the five `EntitySnap*`: overlays;
    - DimArrowBlock: a dimension's child, written as its primitives.
  - Recompute the exact row count from the current enum in 0B-1; the old expected 27 drawing rows is a starting
    checklist, not a constant to preserve. Every not-applicable row needs a reason of more than 10 characters.
- **Registration:**
  - in `librecad_tests`, after `lc_insert_transform_tests.cpp` (`CMakeLists.txt:2621`);
  - in `LIBRECAD_DXF_FAST_TEST_SOURCES`, near the other filter/XDATA tests; verify the current list and avoid relying
    on a remembered insertion line;
  - **not** in the DWG fast list: the DXF lane defines `DWGSUPPORT`, so it covers the DWG checks too.
- **CI path filter.** In `.github/workflows/dwg-dxf-verify.yml`, add `'librecad/src/lib/engine/rs.h'` to both path
  lists (`:29-52`, `:55-78`), so a PR that only adds an enumerator runs the guard. Fix the stale
  `'librecad/src/lib/engine/document/entities/rs_entitycontainer.*'` entry (`:40`, `:66`): the file is in
  `document/container/`.
- **Done when (in addition to §1.6):** two `-fsyntax-only` compiles of the test file, each with a copy of `rs.h` in a
  scratch directory first on the include path. Take the command from `ninja -C build -t commands librecad_tests | grep
  lc_entitytype_coverage_tests` and add `-fsyntax-only -I<scratch>`:
  - a dummy enumerator after `EntityParabola`: expect the `-Wswitch` error and the count `static_assert`;
  - a dummy after `EntityDimArrowBlock`: expect the `-Wswitch` error only.

  Record both messages for the PR text. Never edit the real `rs.h` for this.

#### 0B-2. Entity factory, clone and INSERT checks, pins

**Executed card: done at 5bfb3c8f08462694796489166f0146dd51fc7ca0**
- Owner: Gibbs; dependency 0B-1 at d6de53d3c; refresh HEAD after this checkpoint.
- Scope: new `librecad/src/test/lc_testentityfactory.h` and the existing coverage-test source only. Coordinator owns
  CMake/qmake lists and shared builds while independently validating 1A-1.a; request the build handoff before running.
- Contract: 27 valid drawing fixtures, clone RTTI preservation and both sole/mixed INSERT expansion checks; explicit
  construction-line and generic-container pins. Observe baseline gaps rather than guessing or hiding fixture defects.
- Tasks: construct/update headless-safe entities; add check dispatch and explanatory baseline gaps only if reproduced;
  extend the guard walk and pins. No round-trip work yet; 0B-3 remains separate.
- Validation: Release full/DXF-fast `[entity-coverage]` nonzero cases; both fast CI lanes; ASan coverage/ownership tags.
  Record per-type fixture/check results and verify no QApplication is created in either fast lane.
- Review/evidence: the initial two mixed-block failures were intentional image/hatch prepending by `addEntity()`.
  Fixtures now explicitly append and assert source order. Final full, DXF-fast and ASan `[entity-coverage]` runs pass
  7 cases/910 assertions each, no skips/failures; all 27 fixtures, clones, sole/mixed INSERT shapes and both pins pass.
  Both Release CI lanes pass (DXF JUnit tests=39507, 3 existing skips; DWG tests=34952, 4 existing skips).
  Reports: `build/evidence/0B-2/coverage-{release,asan,dxf-fast}-final.junit.xml`, `dxf-fast-final.junit.xml`,
  `dwg-fast-final.junit.xml`; all exit 0. Failed first-run reports remain, not hidden. ASan uses `detect_leaks=0`.
- Commit/isolation: the restored patch was recompiled against committed a4690db08 geometry; only this header and
  coverage-test file were staged. Gibbs's later core edits were not compiled into these already-built executables or
  included in the commit. Scoped index/worktree and whitespace checks passed. No baseline gaps were added.
- Next: 0B-3 format round trips; the complete guard/full-suite gate is still pending until that slice finishes.

- **New header `librecad/src/test/lc_testentityfactory.h`** (header-only, `inline`; `librecad/src/test` is on the
  shared include path, `CMakeLists.txt:271`).
  - It may include engine and filter headers only. Not allowed: `ui/` headers (the DXF lane has no autogen include
    path), `lc_actiontestsupport.h`, or `lc::test::application()`.
  - Function: `inline std::unique_ptr<RS_Entity> lc::test::makeDrawingEntity(RS2::EntityType, RS_Graphic&, RS_EntityContainer* owner)`
    returns a valid, updated instance, or `nullptr` for a type it cannot make.
  - Its own switch over the type has a `default:`, so including files get no `-Wswitch` noise.
  - Preconditions: `lc::test::ensureSettings()` has been called, a `lc::test::SourceFonts` object is alive, and the
    graphic has run `initForNewDocument()`.
  - `EntityInsert` also adds an inner block holding one line.
  - Constructions to reuse:
    - polyline: `lc_copy_semantics_tests.cpp:160-168`;
    - spline: `lc_action_modify_offset_tests.cpp:130-136`;
    - hatch: `lc_copy_semantics_tests.cpp:170-181`;
    - image: `block_insert_wipeout_tests.cpp:538-542`, with an explicit size (no raster loads without a GUI application);
    - wipeout: `block_insert_wipeout_tests.cpp:579-589`;
    - MLeader: `dwg_write_smoke_tests.cpp:15929-15945`;
    - dimensions: `RS_DimensionData` plus the specific data, then `update()`. Radial, Diametric, Angular and Ordinate
      have never been built in any test: debug them here, before any file I/O.
  - Copy constructions only from those files: their fixtures create a `QApplication`.
- **Checks** (`bool runCheck(unsigned check, RS2::EntityType type, RS2::EntityType reloadsAs)`, with factories):
  - **CloneClass:** `std::unique_ptr<RS_Entity> c{e->clone()}; return c && c->rtti() == e->rtti();`
  - **InsertExpansion**, twice per row: a block {instance}, and a block {line, instance}. Insert it with
    `RS_InsertData(name, {10,5}, {2,2}, M_PI/6, 1, 1, {0,0})`, call `update()`, and expect 1 (or 2) children. The last
    child's rtti is the row type, except for `EntityInsert`, whose nested insert is flattened (`rs_insert.cpp:591-617`).
- **Pins** (same tag):
  - `RS_ConstructionLine`: INSERT expansion gives 1 child (`NativeOrthogonal`).
  - A block of {line, `RS_EntityContainer{line}`}: INSERT expansion gives 0 children.
  - The DXF halves of both pins come in 0B-3.

#### 0B-3. Round trips, `reloadsAs`, known gaps

**Executed card — done at 0e3c6b342a0edefe06847c33d775f756c31fe674, 2026-10-15**
- Owner: coordinator; dependency 0B-2 at 5bfb3c8f0; refresh HEAD after this checkpoint.
- Activated at `00ed2eb77cd8e72d2705396a0ba69e0e5eb13fec`; budget commit 10c6bff46 is already verified/reconciled.
  The saved round-trip patch was restored exactly before building; no uncommitted core changes affect this slice.
- Files: coverage-test source and factory header only if a demonstrated I/O fixture correction is needed. No new
  target registration; existing full/DXF-fast targets already contain the guard. Gibbs's core files remain disjoint.
- Contract/tasks: actual DXF R2018 and DWG R2018 export/import for every valid drawing fixture; run
  `RS_Graphic::onLoadingCompleted()`; assert one live top-level entity and measured reload type. Add both DXF pins.
  Validate fixtures before declaring format gaps; check native version, export/import success and diagnostics.
  Current callback/helpers and both format enum mappings are confirmed; use `QuietLog`, `SourceFonts` and
  process-isolated `OutputDirectory`. Do not introduce GUI initialization into the fast lane.
- Validation: focused full/DXF-fast and ASan coverage tags, both fast CI lanes, full visible Release suite against S0.
  Expected new case families: DXF and DWG walks plus two DXF pins, each with nonzero actual counts. Report per-type
  native reloads and only reproduced baseline gaps; no generic success/count result may hide a save failure.
- Recovery/exit: preserve any core work during isolated verification; explicit-path implementation commit followed by
  a roadmap checkpoint with actual SHA/known-gap evidence. This closes the guard, not the future new-type contract.
- Initial preparation: DXF/DWG walks and the two-path DXF pin test were uncommitted; all rows expected their own
  type and all gap masks were zero pending measurement. Three new registered cases were expected (10 coverage cases
  total with 0B-2); each walk must actually visit all 27 drawing rows. Release/ASan target builds passed on the mixed
  prepared tree; no runtime/gap outcome or isolated-commit verification is claimed yet.
- First actual probe: both Release and ASan execute all three cases and all 27 rows per format. DXF rows pass;
  only generated MLEADER's DWG row fails its native-type expectation, loading two usable entities (LEADER + MTEXT).
  Source inspection confirms the intentional generated fallback in `writeMLeader`: native output requires a nonzero
  imported source handle. This is a baseline gap, not an invalid fixture or parser failure. Pin export/import success,
  exact fallback types and usable geometry so the known-gap bit cannot conceal a different failure. No production
  MLEADER writer change belongs in this guard slice. Evidence: `probe-{release,asan}-1.junit.xml`, both exit 42
  before the measured gap is recorded; all other rows passed and no crash/hang/skips occurred.
- Final evidence: all three targets build in Release/ASan, exit 0. `[entity-coverage]` passes 10 cases / 1335 assertions
  with zero skips/failures in full Release, ASan and the DWG-enabled DXF-fast target. Every one of the 27 drawing rows
  executes through each format, plus both nonpersistent DXF pins; no `notRun` bits are set. All 27 DXF / 26 DWG rows
  retain runtime type. The sole baseline gap is generated MLEADER → LEADER+MTEXT in DWG, pinned to successful
  export/import, exact loaded types and usable geometry. A repaired native writer must update that expected gap.
  No source handle was fabricated to bypass the generated route and no production/factory code was changed.
- Broad evidence (`build/evidence/0B-3/*-final.junit.xml`, summary): full visible Release 4606 entries /
  JUnit tests=5,861,252, no failures/errors, the same 74 skips and every S0 testcase retained. DXF CI lane 1028 entries /
  tests=39,932 / 3 unchanged skips; DWG 1084 / 34,952 / 4 unchanged skips; all exit 0. ASan reports no sanitizer finding,
  with `detect_leaks=0` (no LSan claim). Failed initial probes remain; whitespace/scoped review passes.
- Commit audit: only `lc_entitytype_coverage_tests.cpp` was staged/committed; its SHA-256 is
  `2046e06f943fbe70d7ea7a60add4545fb3c97d9ecc601c73fb94e8b65b66edf7`, matching final builds/tests. Coordinator/Gibbs
  independently reviewed gap specificity, type/version checks and headless behavior. The old saved patch is obsolete
  for replay. This completes the pre-feature guard, not the persistent entity or its 14-version file contract.
- Next: `.b-progress` against the actual checkpoint HEAD. `1A-2` still waits for all geometry children; it adds a new
  gap-free row and raises the drawing count to 28. The generated-MLEADER baseline limitation belongs in PR B's text.

- **DXF and DWG round trip.** For each drawing row: a fresh graphic with the one entity; `lc::test::exportDrawing` and
  `importDrawing` with `FormatDXFRW2018` and `FormatDWG2018`, inside `QuietLog`; run the application's post-load
  completion hook before checking load-dependent entities; expect one live top-level
  entity whose `rtti() == reloadsAs`.
  - `reloadsAs` is the type itself unless the row says otherwise. Record what Hyperbola, Parabola and SplinePoints
    actually reload as: `handleQuadraticConicSpline` (`rs_filterdxfrw.cpp:8001-8030`) can turn a degree-2 spline back
    into a conic.
    If measured legitimate reload types differ by format, split the row's expectation into DXF/DWG fields with the
    same native-type default; do not label a supported conversion as a gap or force one format's expectation onto
    the other. The future offset-curve row must reload as itself in both formats after 1E-1.
  - Only standalone texts: a block attribute's text is written by its INSERT (`hasNoOwnRecord`, `:30129-30131`).
- **Pins, DXF half:** a ConstructionLine and a container both leave 0 entities after a round trip.
- **Known gaps.** `Row::knownGaps` is a bitmask of checks that fail at the base commit: the walk asserts `CHECK_FALSE`
  for each, so a later fix forces the row to be updated. `Row::notRun` lists checks that crash or hang; they are never
  run and are listed. `gapNote` holds the observed failure. The PR text lists every gap.
- **Candidate gaps to verify at S0** (do not predeclare failures; fixture validity and completion hooks matter):
  - writers that skip silently: Hyperbola needs `isValid()`, Parabola `getData().isValid()`, Spline at least
    degree + 1 control points, SplinePoints at least 3 points, Leader `count() > 0`;
  - candidates not previously exercised through a complete DWG writer path: Hyperbola, Parabola, DimLinear, DimRadial,
    DimDiametric, DimAngular, DimOrdinate, Tolerance and Leader. `DimLeader` requires
    `RS_Graphic::onLoadingCompleted()` after import.
- **Optional:** a hidden `[.entity-coverage-gaps]` case that `WARN`s each gap, for the PR text.
- **Done when:** the pre-feature walk asserts the recounted baseline row total (expected 27 drawing types at
  41678d976) and every non-gap check passes. 1A-2 adds the new row and updates the count.

---

## 5. Phase 1 — the type

### 1A-1. Closed forms, normalization and tolerance helpers

**Execution card: 1A-1.a, 2026-10-15**
- State: done at `a4690db08b7220e93bbc8642d2c2b8b2e14fcf30`; S0 done at de30f1d45.
- Owner: coordinator. Files: new `lc_ellipseoffsetgeometry.{h,cpp}` and
  `tests/lc_offsetcurve_geometry_tests.cpp` under the entity directory; appended tolerance helpers in
  `lc_curveoffset.{h,cpp}`; `librecad/src/src.pro` registrations. CMake registration waits until Gibbs releases his
  0B-1 build/CMake scope; no concurrent builds or changes to his coverage-test/workflow files.
- Contract: finite representable 2D normalized basis, canonical whole vs directed arc/full-turn classification,
  sweep membership, exact point/derivative/normal, signed offset, curvature/lens/inside helpers, and source-distance
  query with bounded exterior solving and native nearest-point seeding/validation. No cache fitting/adoption is added here.
- Tasks: implement checked local-frame arithmetic and raw normalization; expose geometry operations; append engine
  tolerance helpers without changing existing options; add boundary/random closed-form tests; register after handoff.
- Validation: Release/ASan `[lc-offsetcurve][geometry]` with nonzero case counts, existing `[curve-offset][ellipse]`
  regressions, tolerance equality and malformed/overflow cases. Ref/check/layout tests remain explicit later children.
- Findings: classify tiny sweeps using their directed wrapped sweep, not just `abs(angle2-angle1)`; a small negative
  delta can describe a nearly full counterclockwise arc. Axis switching is done in a local-centred temporary ellipse
  before restoring world centre, avoiding endpoint-minus-large-centre loss. Do not divide tiny vectors using
  `RS_Vector::operator/`, whose tolerance shortcut returns the undivided vector.
- Review: Gibbs found far-query false zeroes from the native distance sentinel, huge-angle fmod drift, intermediate
  curvature underflow, thin-query squared-distance ties, coordinate overflow and the limiting thin vertex. All are
  addressed in this module with regressions: trig-consistent reduction, exponent-scaled curvature, bounded Boost
  exterior solving/foot checks, quadrant/limiting thin handling and source/query scaling before unsafe arithmetic.
  No existing `RS_Ellipse` parser, renderer or nearest-point implementation was changed. A Boost const-call compiler
  error during review fixes was resolved; failed build logs remain in the evidence directory.
- Evidence: `build/evidence/1A-1.a/geometry-{release,asan}-final-isolated.junit.xml`: 17 cases, 2538 assertions, no
  failures/skips, both exit 0; all three targets built in Release/ASan. Existing ellipse regressions: 48 cases /
  722731 assertions pass Release and ASan (engine paths unchanged by the later helper-only fixes). Both Release fast
  lanes pass with the existing 3/4 optional-input skips; final lane reports are `dxf-fast-final.junit.xml` and
  `dwg-fast-final.junit.xml`. ASan uses `detect_leaks=0`; no LeakSanitizer claim. Full qmake6 build remains a release gate.
- Isolation/commit audit: the prepared 0B-2 test patch was preserved at `build/evidence/0B-2/pending-coverage.patch`
  and temporarily removed through an owned patch for validation. The real coverage source matched HEAD, and only
  these seven geometry/build paths were staged. Scoped index/worktree and whitespace checks passed before commit.
- Next at that checkpoint: restore and verify 0B-2 (now committed); current geometry continuation is `.b-budget`,
  `.b-progress`, the `.b-ref` children, the `.b-check` children, then `.c`. No validator, new
  persistent entity, UI or file-format completion is claimed by this leaf.

**Files.**
- New `lib/engine/document/entities/lc_ellipseoffsetgeometry.{h,cpp}`, containing the completed `.a` scope:
  - `struct LC_EllipseOffsetDefinition` (§3.2);
  - free geometry operations in namespace `LC_EllipseOffsetGeometry`; this module owns `normaliseBasis` from .a,
    so its tests do not depend on the not-yet-created `LC_OffsetCurve` class. The later class static forwards to it;
  - point, derivative and outward normal at η;
  - `exactOffsetPoint(basis, η, s)` (signed s; towards the centre is negative);
  - the sweep interval of §3.5;
  - distances from a point to the original and exact offset, minimum radius of curvature, lens corners and whole-ellipse inside test.
- Future cache-validation work in the same module is assigned only to `.b-check` children, not to completed `.a`:
  - `.b-check-layout`: checked degree-1/degree-3 cache-layout predicate, finite 2D/unit-weight preflight and
    degree-specific span extraction;
  - `.b-check-bounds`: conservative cache-to-reference/source distance bounds;
  - `.b-check-topology-local`: local injectivity, paired endpoints and exact fringe-incidence witnesses only;
  - `.b-check-topology-chain`: public `checkCache` result and complete component/topology/coverage decision;
  - 1H: `offsetPolyline(basis, d, toward, samples)` for preview only, not a validity/adoption oracle.
- Build lists for the geometry module were added in `.a`; later helper work uses the existing registered files unless
  source review proves a new file is needed.

The original v1 file list also named `isEngineLayout`, cache-Bezier distance and `checkCache` under `.a`; those items
were not delivered by the committed `.a` slice and are not complete. This v17 revision assigns them to `.b-check`
and removes the duplicate ownership. Do not interpret the historical `.a` implementation card as evidence for them.

- `lc_curveoffset.{h,cpp}` contains the completed `.a` tolerance helpers:
  - `double relativeOffsetTolerance(const RS_Entity& source)`: 1e-4 for ellipses (`kEllipseRelativeOffsetTolerance`),
    1e-6 otherwise;
  - `double toleranceFor(const RS_Entity& source, double distanceMagnitude, double relativeFactor)`: the formula of
    `lc_curveoffset.cpp:4962-4965` with the file-local `computeScale` (`:601-622`).

Do not name the helper parameter `requestedTolerance`: a parameter of that name in `makeDirectOptions` /
`makeOffsetOptions` would hide an unqualified call.

**Precondition:** use the S0 build and baseline; retain the #3117 arc behavior measured at S0.

Execute 1A-1.a, .b-budget, .b-progress, the .b-ref children, the .b-check children and .c in order (§0.6); each
executable leaf has its own compile/test and commit checkpoint. `.b`, `.b-ref` and `.b-check` are aggregates, not
additional implementation commits. All children must finish before 1A-2 adds the persistent type.

**Tests** — new `lib/engine/document/entities/tests/lc_offsetcurve_geometry_tests.cpp`, `[lc-offsetcurve][geometry]`,
registered in `librecad_tests` next to `lc_curveoffsetellipsetests.cpp`:
- **Closed forms against brute-force dense sampling** on random ellipses, including ratio 1: distances, b²/a, lens
  corners, minimum radius of curvature.
- **`toleranceFor`:** `makeOffsetOptions(e, d).tolerance.requestedGeometry == toleranceFor(e, d, relativeOffsetTolerance(e))`.
- **Domain and normalization:** canonical whole, genuine ±2π sweeps, zero/tiny noncanonical sweeps, multiple turns,
  wrapped/reversed arcs, circles, extreme valid scales/ratios, non-finite values and precision loss. Include a case
  whose small arc angles would be snapped away if raw validation happened after ellipse construction.
- Checker/layout, cache-corruption, adversarial-acceptance and checker-work tests belong to `.b-check-layout`,
  `.b-check-bounds`, both `.b-check-topology` children and `.c`; they were listed in the original `.a` design but were not part of
  the committed `.a` implementation. Do not count them as `.a` evidence.
- Current engine output across ratios 0.05–0.95, distances 1.01·tol … 3a, whole/arcs and both sides is qualified in
  `.c`; include closed results from arc sources and boundaries near cusp/trim events. One output entity alone does
  not prove eligibility.

#### 1A-1.b-budget. Bound shared intersection work before allocation

**Executed card — done at 10c6bff46976938492c5d1ad5b5e74001d6b2d03, 2026-10-15**
- Dependency: 1A-1.a at a4690db08. Files: `lc_parametriccurveintersection.{h,cpp}` and its existing tests only.
  Coordinate shared builds with the coordinator; no CMake/qmake registration changes are needed for existing files.
- Baseline finding: `Solver::candidatePairs()` built its whole vector before `solvePair()` enforced `maxBoxPairs`; the latter
  also increments before checking. The result exposes no consumed-work counters for the reference/check shared cap.
- Contract/tasks: check a combined pair-work allowance before broad candidate examination/allocation and before each
  refinement, without counter overflow; return `LimitExceeded` rather than using a partial candidate list. Expose
  consumed candidate/refinement/leaf counts on every result so reference analysis can debit one shared budget.
  Preserve existing zero-depth behavior and default intersection/ordering semantics; do not add a fitting path.
- Tests: deterministic dense-overlap input with a tiny cap refuses before quadratic allocation; consumed counts never
  exceed caps; an exact-used budget passes and one-less refuses; invalid/empty input returns zero/appropriate work;
  existing topology and all curve-offset tests remain valid. Run Release full suite, focused ASan topology/ownership
  checks, and ellipse regressions before committing this shared-engine change.
- Recovery/exit: preserve other slices, review scoped diff, commit this leaf, then update the roadmap with its real
  SHA/work evidence before promoting `.b-progress`. `.b` and 1A-1 remain partial until every required child is done.
- The committed patch exposes `LC_IntersectionWorkStats` and checks hierarchy/refinement work before increment/storage;
  nine `[intersection-budget]` cases are registered. Final isolated verification passed as recorded below.
  Contact-classification progress is not fixed by these pair counters; it belongs to
  the next leaf, not an implicit completion claim for this patch.
- Verification finding, 2026-10-15: the first isolated Release offset run and ASan topology/ellipse run refused valid
  baseline geometry with status 14. Convex-L reoffset spent all 1,048,576 examinations in the X-only broad phase
  (1,535 leaves, only one anchored branch, zero refinement); a near-critical whole ellipse spent 580,418 broad plus
  468,158 refinement examinations. Temporary diagnostics were removed after recording these measurements.
- Refined tasks: replace the X-only pair sweep with a bounded balanced two-axis box hierarchy, pruning aggregate
  disjoint/unanchored ranges while charging each nontrivial node-pair examination before expansion. Keep one shared
  cap, deterministic canonical leaf-pair ordering and no partial results; do not increase defaults or change the
  geometry expectations. Index storage is linear in capped leaves and traversal stack is bounded. Update work-stat
  documentation to distinguish hierarchy box visits from leaf-pair refinements. Add y-separated and anchored-cloud
  regressions plus brute-force small-input parity, then rerun focused/adjacent/full verification. Failed reports remain
  `offset-isolated-1.junit.xml` and `adjacent-asan-isolated-1.junit.xml`; no validated state is claimed yet.
- Second verification: all eight budget cases passed in Release/ASan (1,471 assertions each), and the convex-L
  baseline passed. The final near-collapsed circle (`a=b=100,d=99.99`) still exhausted the unchanged cap at 499,617
  hierarchy visits plus 548,959 refinements (1,832 leaves, one branch). Extend the existing conservative `bridged`
  monotonicity/small-image predicate to same-branch hierarchy ranges after charging the visit; this avoids repeating
  individual proofs without relaxing topology. Cross-branch nodes cannot use that proof. Add a same-branch pruning
  regression and rerun the untouched ellipse/crossing/corner tests. Preserve `regressions-isolated-2.junit.xml` and
  diagnostic logs; neither this partial pass nor the index implementation alone closes the slice.
- Final correction: wide recomputed curve bounds remained too loose near the collapsed circle. A conservative
  cached-leaf union proved the required monotone ranges; the final implementation replaces the diagnostic scan with
  a checked `2*N` parameter-order range index, queried in O(log N). The queried range includes every intervening
  original leaf, not only spatial descendants, and is used only for one common branch. Point-only outside spans
  are skipped by the shared range predicate; interior corner spans remain included. No tolerance, default cap or
  original geometry expectation was changed. Tests cover 255/256/257-leaf boundaries and preserve crossings/loops.
- Final evidence (`build/evidence/1A-1.b-budget/`, summary and `*-final.junit.xml`): all three targets built Release/
  ASan with exit 0; focused budget nine cases, 16 JUnit entries, 1560 assertions, zero failures/skips in both.
  Release `[curve-offset]`: 246 entries / 1,017,679 assertions; ASan topology/ellipse: 78 / 726,401; both exit 0,
  no skips/sanitizer findings. Full visible Release: 4603 entries / JUnit tests=5,860,827, no failures/errors, the same
  74 S0 skips and every baseline testcase entry retained (39 added). Both fast lanes pass with CI exclusions:
  DXF 1025 entries / tests=39,507 / 3 unchanged skips; DWG 1084 / 34,952 / 4 unchanged skips. All processes exit 0.
  ASan uses `detect_leaks=0`, no LSan claim. Exploratory runs 1-4 remain as failure evidence; run 3's overlapping
  Release processes are not final verification. Final binaries were serialized within each build directory.
- Commit/isolation audit: the 0B-3 test patch was saved and parked; its source matched HEAD for the final builds.
  Only the three owned core/header/test paths were staged/committed. SHA-256: cpp `a51476cd6ef0ed0bae5ac75358d3d86d46aa1daa2bb045db24d5b16845b8c0ec`,
  header `cbb6bad182d982f0737b9d1aa30c2cd7638ebcdd3702edec9a15ce4c7f8bfc35`,
  tests `87c60a0e37b5839c6c21310fdc823b1c04cbf393f655b47f42b3f4cef345fb23`; unchanged through verification/commit.
  Coordinator and Gibbs separately reviewed conservative coverage, safe indices/counters and corner/branch isolation.
- Next: restore 0B-3 exactly and verify it; `.b-progress` now has its ready dependency and confirmed reproduction.
  Parent `.b`/1A-1 remain partial: no reference validator, persistent offset entity, UI or file contract is complete.

#### 1A-1.b-progress. Guarantee contact-classifier termination

**Executed card — done at fb70681e2eecd7ba937c7942bed8de129425a3aa, 2026-10-15**
- Dependency: `.b-budget` at 10c6bff46976938492c5d1ad5b5e74001d6b2d03. Owner: Gibbs; coordinator owns builds/Git.
- Activated at `210d51a9ee63458eff76ac3219c021db17e00b78`; tracked product tree was clean at handoff.
  Code scope is the existing intersection cpp/tests, header only for necessary diagnostics; shared builds remain
  coordinator-owned and wait until the patch is frozen.
- Scope: `lc_parametriccurveintersection.cpp` and its existing tests; header only if additional bounded diagnostics
  are necessary. No geometry fitting, source adapter, registration or unrelated engine refactor.
- Finding: `classifyContact()` starts its unbounded doubling loop at `8*tolerance/db.magnitude()`. A zero initial
  step (underflow or infinite computed speed) stays zero forever, so pair caps cannot terminate that loop. This is a
  source-level P1 finding; reproduce the reachable conditions before choosing the narrow fix.
- Reproduction confirmed during budget verification: two finite crossing lines with derivatives `(1e200,0)` and
  `(1e200,1e200)`, parameter domain 0..1 and intersection at t=0.5; tolerance `1e-200`, `maxDepth=0`. The initial
  classification step is exactly zero, and the unmodified classifier times out after three seconds (exit 124).
  Probe/source and log are under `build/evidence/1A-1.b-progress/`; the subprocess was terminated by its timeout,
  not left running. This evidence qualifies the future regression; no fix is included in the budget leaf.
- Tasks: add a deterministic iteration bound and finite/progress checks; preserve ordinary shallow-contact behavior.
  A small positive step may initially round `u + direction*step` back to `u`; permit bounded step growth rather than
  misclassifying every such rounding case. A zero/non-finite step or exhausted classification allowance refuses
  without publishing partial roots. Audit adjacent midpoint/step arithmetic for the same termination invariant.
- Validation: explicit underflow, extreme derivative, parameter-rounding and boundary-contact cases, run under a
  process timeout; timeout is a failed test run, never a hidden skip. Focused Release/ASan plus existing topology,
  all curve-offset tests and full visible Release suite; compare stable failures with S0. Default depth semantics
  remain unchanged. Record actual finite-loop bound and reason mapping in the checkpoint.
- Exit: reviewed implementation commit and plan checkpoint; `.b-ref-frames` may then become ready. Pair budgets and
  fixed numerical-loop limits are complementary; neither substitutes for certified event ordering in `.b-ref-events`.
- Implemented: finite point/parameter/speed/step/contact checks, positive-step growth guard, and per-side
  `max_exponent - min_exponent + digits + 1` bound (2099 for binary64). Zero/non-finite arithmetic gives
  `AmbiguousTopology`; defensive loop exhaustion gives `LimitExceeded`. Positive steps that initially round `s`
  back to `u` keep growing; ordinary classification/depth policy remains. Repeated projection norms are cached.
- Evidence (`build/evidence/1A-1.b-progress/`, summary): all three targets build Release/ASan, exit 0. Four registered
  `[intersection-progress]` cases / 80 assertions pass both under 30-second process watchdogs, with no skips or
  sanitizer findings. The original standalone probe, recompiled after the fix, returns status 2 / zero roots / exit 0
  under the same three-second watchdog instead of the before-fix timeout/exit 124. Valid rounded growth passes at
  `1e-16` and `denorm_min`; the tests do not impose timing assertions or claim a configurable-cap exit.
- Broad gates: Release `[curve-offset]` 250 JUnit entries / 1,017,759 assertions; ASan topology/ellipse 82 / 726,481;
  full visible Release 4610 / JUnit tests=5,861,332, zero failures/errors, all S0 testcase entries and the same 74 skips.
  DXF CI lane 1028 / tests=39,932 / 3 unchanged skips; DWG 1084 / 34,952 / 4 unchanged skips. Every process exits 0.
  ASan uses `detect_leaks=0`, no LSan claim. Original hung subprocess was killed, and all final processes finished.
- Commit audit: only the intersection cpp/tests were staged; no header/build/guard/factory changes. SHA-256:
  cpp `e294634ad1c4aa220c6ff6e0ff6f537b18dd6ef038f45b9acdb4b41ccd6ce617`,
  tests `1358320dfe2789c91f7a2cd367c2b7ae27c53bd03bade7a1a898cbec0adab2d7`, matching the frozen tested tree.
  Coordinator independently reviewed the scoped diff, refusal paths and binary64 termination argument.
- Next: `.b-ref-frames` card below. The default solver still has approximate topology/evaluation heuristics; strict
  reference certification, components, cache validation and the persistent feature are not completed by this fix.

#### 1A-1.b-ref-frames. Local reference jets, bounds and shared budget

**Executed card — done at 26d107ccfc863e58b3b912ab7fbe591588814e2d, 2026-10-15**
- Handoff at e061a75116a7bfc42ede45f14100ee22c403bd74: tracked tree clean; only the pre-existing untracked
  `build-asan/` remained. Gibbs owns the three files
  below; coordinator owns documentation, build directories, validation evidence and Git. No concurrent writers or
  same-directory test processes are permitted. Freeze the patch before collecting final evidence.
- Owner: Gibbs for the geometry cpp/header/test scope; coordinator owns roadmap, builds and commits. Dependencies:
  `.a` a4690db08, `.b-budget` 10c6bff46, `.b-progress` fb70681e2, S0 baseline; no required predecessor is unfinished.
- Files: `librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.{h,cpp}` and
  `librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_geometry_tests.cpp` only. Existing registration
  suffices; do not alter the materializing engine, intersection core, reference graph or persistent entity yet.
- Source conventions confirmed: increasing adapter `t` runs 0..abs(sweep), `eta=start+sigma*t`, sigma=-1 when
  reversed. Whole start is pi/4, with pi/4+k*pi/2 source span boundaries; closing evaluation is canonicalized to
  opening. Outward-positive delta=-distance for inward definitions; engine left-normal distance is `-sigma*delta`.
  `cacheReversed` changes matching order later, never source traversal or physical side.
- Geometry API: add an immutable factory-validated `ReferenceFrame` in the existing namespace. Planned operations
  are `create(basis,signedDistance)`, `sourceJet(t,out)`, `offsetJet(t,out)`,
  `boundSourceJet(t0,t1,out)`, `boundOffsetJet(t0,t1,out)`, plus checked point-to-frame conversion and scale/domain
  accessors. Use existing `LC_CurveJet` / `LC_CurveJetBounds`; stage/clear outputs on failure. Coordinates are centre-
  relative and uniformly scaled; keep world centre out of intermediate geometry and enclose conversion error.
  A frame/bound success is not a reference-component or cache-adoption success.
- Jets: use exact ellipse/outward-offset identities, `F=1+delta*kappa`, `Q'=F*C'`,
  `Q''=F*C''+delta*kappa'*C'`, with sigma in increasing-t derivatives. Avoid overflowing direct powers; require a
  provably positive source-speed denominator and finite results. Circles use `(a+delta)*(cos eta,sin eta)` directly
  to retain correlation near collapse; general speed bounds use the correlated identity
  `b*b+(a*a-b*b)*sin(eta)^2`, with checked scaling rather than naive squared underflow/overflow.
- Trig bounds: geometry-private interval evaluation with finite quadrant reduction, an enclosing pi constant and
  explicit Taylor remainder (for example degree-20 cosine / degree-21 sine on abs(reduced angle)<=pi/2, with
  R^22/22! and R^23/23! error bounds). Use `LC_Interval` outward arithmetic and bounded enumeration; wide boxes may
  return conservative [-1,1] bounds. Form coefficients/remainders through outward interval recurrences, not
  overflowing integer factorials or unqualified rounded coefficients. Do not certify `std::sin`/`cos` samples with
  the engine's old fixed padding.
  Support zero-width point queries for the strict-event contract above, including cancellation/seam/conversion error.
- Budget API: introduce the real §3.5 check-limit type and a checked shared budget for pieces, total generated
  vertices/leaves, distance-pair work and depth. Check `requested<=limit-used` before increment/allocation, never
  refund discarded work, retain consumed counters on refusal and prevent later partial-success publication after
  exhaustion. No per-span reset. Later solver queries get remaining allowances and debit returned work on every
  status; visibility/subdivision costs require their own explicit debits. Do not expose a partial `checkCache`.
- Tests: independent dense point/first/second-derivative enclosure oracles, not only the helper's own samples;
  source/side/reversal/wrap/full/seam pins; cusp-zero derivative, near-circle collapse, rotated/thin/extreme-scale and
  large-centre conversion; cleared invalid/unbounded outputs; exact-used/one-less/zero/overflow budget boundaries.
  Tag `[lc-offsetcurve][geometry][reference-frame]`; discover/record actual cases and run Release/ASan plus all
  existing geometry, offset/topology regressions. No fitting or engine regeneration is permitted.
- Review/exit: inspect interval remainder, source-speed and coordinate/error proofs separately from passing tests.
  Commit this complete geometry-helper leaf, record actual SHA/evidence and reconcile the plan before activating
  `.b-ref-events`. Event certification, trimming/components and whole-cache adoption remain waiting children.
- Implemented: factory-validated world-aligned, centre-relative power-of-two frame; exact input axes/ratio/distance
  are interval-enclosed. Rounded hypot chooses scale only. Taylor recurrence/remainders bracket real trig values;
  wide boxes use [-1,1]. Zero-width bounds and explicit whole-seam hulls include cancellation and conversion error.
  Source/offset jets, first/second derivatives and product bounds are covered; floating jets are midpoint seeds only.
  Zero distance returns source bounds. Circles preserve the common radius factor. Unprovable extreme denominators
  refuse with cleared outputs. `WorkBudget` is noncopyable, checks before increment and irreversibly latches refusal.
- Review: coordinator read the complete scoped diff separately from the test results, checked the Taylor remainder,
  input-axis/scaling and signed derivative/product identities, seam/conversion handling, finite/refusal paths and
  budget induction. No known unresolved implementation finding. No existing engine/intersection behavior changed.
- Evidence (`build/evidence/1A-1.b-ref-frames/summary.md` and reports): all three targets build Release/ASan, exit 0.
  Seven registered new cases / 359105 assertions pass both, independent 113-bit oracle (not assumed wider long double).
  Serialized final Release geometry: 44 JUnit entries / 363283 assertions; ASan geometry/topology/ellipse: 106 /
  1088124, no failures/skips/sanitizer findings. All Release geometry testcase names are present in the combined ASan
  report. Release offset: 250 / 1017759; full visible Release: 4617 / JUnit tests=6220437, no failures/errors, every S0
  and prior-progress entry retained, seven new entries since progress, the same 74 skips. DXF: 1028 / 39932 / three
  unchanged skips; DWG: 1084 / 34952 / four unchanged skips. Every process exits 0; ASan detect_leaks=0, no LSan claim.
- Evidence handling: initial focused Release briefly overlapped list-only discovery; retain it as exploratory and
  use the later serialized final geometry/broad runs. One premature parse of a running ASan report was incomplete,
  not passing; final parsing followed exit 0. A line-count discrepancy triggered hash verification, which confirmed
  no source mutation. All final processes finished. No product test failed or baseline expectation was weakened.
- Commit audit: only geometry cpp/header/tests staged, 914 additions; existing CMake/qmake registration suffices.
  SHA-256 cpp `cfb8b472babc424d08617863a23dde03db7641ec11c52a4ce1419ed6235db8fb`,
  header `face28b177e99ad0017b2a8fd395f124062e913e7c902d7bbc14a3fefa55ab6d`,
  tests `459ef9e74bca566a57cdb047b9f2c2f92f24a04366b4ff116ca516055bb8c373`; unchanged from freeze through
  verification and matching the implementation commit. Roadmap changes remain the separate post-commit checkpoint.
- Next: strict event-core card below. An enclosure/frame success is not an event/component/checkCache success.

#### 1A-1.b-ref-events-core. Strict intersection certificates and bounded work

**Executed card — done at 834a46a88e2b017dc473f75ff86ad64aad99e6fe, 2026-10-15**
- Activated from abaa1a13c738a68ccc4b0070bb2a5f23296133b1; no prepared code counted as done before verification.
- Tracked product tree clean on activation; untracked `build-asan/` preserved. Gibbs starts implementation;
  coordinator independently reviews proof/accounting and owns all builds, tests, evidence and commits.
- Owner: Gibbs for `librecad/src/lib/engine/document/entities/lc_parametriccurveintersection.{h,cpp}` and
  `librecad/src/lib/engine/document/entities/tests/lc_parametriccurveintersection_tests.cpp`; coordinator owns
  roadmap/builds/evidence/Git. Dependency: frames at 26d107ccf, plus the committed budget/progress prerequisites.
  No geometry-adapter, materializing-engine, entity, fitting or build-list changes belong here.
- Preflight: current `LC_ParametricIntersection` has estimated parameters/kind only. `LC_ParametricCurves::bound`
  supplies curve/derivative intervals. Current Krawczyk uses a magnitude-based F(mid) error estimate and inflated
  boxes; default terminal classification, proximity-to-join treatment and deduplication are numerical policies.
  Current pair stats cover hierarchy/refinement, not certificate comparisons or every preparation box.
- API tasks: add opt-in strict certification, default off. Return segment identities, isolating parameter intervals,
  the actual proof domain (including inflation), and proof category: interval-unique transverse or qualified analytic
  endpoint/cusp/tangency; ordinary results remain explicitly uncertified. Strict `Ok` requires every returned event
  certified and exhaustive candidate exclusion; any undecided contact/order gives no partial event vector.
- Add an optional analytic-certificate hook, unavailable by default. It must certify absence or one isolated contact
  over the complete requested box, with explicit proof/isolation domains and source occurrences. Validate finite,
  ordered, in-domain identities/enclosures before using it. Shared endpoint identity or spatial proximity alone cannot
  exclude another nearby root. Frame-specific certificates are implemented in the next child, not fabricated here.
- Readiness correction from current-code proof review: a closed leaf interval with a stationary cusp endpoint cannot
  satisfy strictly positive directional-derivative bounds, however finely it is split. Add an optional analytic
  whole-interval leaf-injectivity hook, unavailable by default, with an explicit no-self-intersection contract and
  validated segment/domain identity. This lets the next child prove stationary-end branches without accepting an
  unproved small/nonmonotone leaf. A contact certificate alone is not a leaf-injectivity proof. Qualify this hook with
  independent analytic stationary-end curves and invalid/missing-domain cases; no fabricated ellipse certificate here.
- Krawczyk tasks: form F(mid) by subtracting outward zero-width point enclosures, not rounded estimates plus heuristic
  epsilon. Floating jets may choose a finite nonsingular preconditioner only; all certificate arithmetic is outward.
  Preserve the actual box for which inclusion/exclusion was proved and establish transverse uniqueness before
  labeling it. Validate strict zero-width bound support; invalid/nonfinite input bounds give `InvalidInput`, while
  valid but unprovable topology/certificates give `AmbiguousTopology`. Limits give `LimitExceeded` with counters intact.
- Strict exclusions: replace floating-angle injectivity claims with a strictly positive interval directional
  derivative over the entire relevant path. Disable small-image shortcuts, unconditional adjacency skips,
  tolerance-radius join exclusion, terminal numerical contact acceptance, endpoint promotion by proximity and
  tolerance-only deduplication. Analytic or interval proof may qualify an exclusion; otherwise subdivide or refuse.
  Default mode and all its regressions retain current behavior.
- Spatial/path accounting review: strict broad phase must retain bounded balanced spatial pruning, including
  separated-Y and anchored-cloud cases, rather than replacing the committed hierarchy with a quadratic scan.
  Across-segment projection proofs must include the complete intervening continuous branch range, including gaps
  between refined cells and partial tails. Validate contiguous ordered parameter segments and finite 2D seeds even
  for single/disjoint branches; invalidity must not depend on whether a candidate happens to reach Krawczyk.
- Event identity/order: keep distinct parameter occurrences, including across a seam. Overlapping isolating boxes
  merge only with proven root identity/uniqueness on a common domain; otherwise refine or refuse. Distinct root boxes
  must have proven order, not approximate sorted estimates. A partition-boundary root must not disappear or duplicate.
- Work contract: add strict `maxGeneratedItems`, default 65536, and `work.generatedItems` covering initial/child
  preparation boxes and every new certificate record, including later-discarded records. Check before each creation,
  never refund; zero allowance is invalid in strict mode. Existing maxLeaves still caps retained leaves. Add strict
  `work.certificateBoxPairs` and charge before comparing/merging; include these in totalBoxPairs and its shared
  cap (zero in default mode). Return all counters on every status. Future reference callers pass remaining vertices
  into the generated-item allowance and debit that counter, not generatedLeaves again because those leaves are already
  included. They debit totalBoxPairs on every status; adapter subdivisions remain additional explicit work.
- Tests: `[lc-offsetcurve][geometry][intersection-strict]`, registered in the existing full test target. Use analytic
  independent adapters with point enclosures and derivative bounds; cover transverse proofs, cancellation where the
  old evaluation heuristic is insufficient, roots at partitions/endpoints, two close distinct roots, tiny loops,
  adjacent re-crossings, missing/invalid analytic certificates, unresolved tangencies/retraces, invalid inputs and
  exact/one-less/zero/overflow preparation/pair/depth caps. Assert no partial vectors on every refusal. Keep ordinary
  crossing/tangency/depth/budget/progress tests unchanged and include default-parity checks for the opt-in API.
- Validation: complete discovery before any test run; record nonzero strict case count. Build all three targets in
  Release/ASan, then run strict tests, all geometry/offset/topology regressions, full visible Release and both CI fast
  lanes against S0 and the latest committed slice. Serialize processes per build directory, freeze/hash source, parse
  only completed XML, inspect scoped diff and commit/checkpoint. No full-reference/component/adoption success claim.
- Historical next-readiness at the strict-core checkpoint: `.b-ref-rate` first corrected the frame's parameterization. Then `.b-ref-events-critical` qualified interval F/F'
  cusp isolation, critical double-root/collapse refusal or analytic proof, seam occurrences and bounded source-end
  cutter construction; do not certify rounded atan2 directions or rounded-pi coverage. Then `.b-ref-events-isolate`
  integrates complete own/opposite/cutter enumeration and the shared budget before components can become ready.
- Critical-child precondition discovered during proof review: the current frame's canonical whole end aliases its
  opening point, but `eta=start+sigma*t` at rounded-double 2*pi has a distinct mathematical left limit. An independent
  113-bit probe (a=100,b=50) establishes the nonzero seam jump. Point hulls do not repair C1 continuity for strict
  root proofs. Before binding the frame to `LC_ParametricCurves`, use a continuous whole-ellipse parameterization
  with rate `real(2*pi)/parameterEnd` enclosed through the real-pi bracket; include the rate and its square in first/
  second derivatives and F/F' formulas. Keep the numeric domain, phase, side and whole seam conventions. Arcs also
  enclose their true directed sweep from exact endpoint doubles and real-period winding; unit rate is exact only
  when that sweep equals the stored numeric extent. Never hide wrapped/subtraction error at an open source end.
  Requalify zero-width/range/product bounds, endpoint limits and old engine geometry within accounted numeric error.
  This is mandatory correction work in `.b-ref-rate`, not completed by the standalone frame helper or
  the generic strict solver. The core's smooth-piece contract remains a mathematical adapter obligation.
- Implemented/reviewed: separate opt-in `StrictSolver`, leaving the ordinary solver body unchanged. It uses outward
  point residuals, actual inflated proof domains, qualified analytic contact and whole-leaf injectivity hooks, checked
  spatial/range indices, full-path interval projections and global shared-branch occurrence order. Generated items
  and certificate pair comparisons are charged without refunds. No partial vector is published on any refusal.
  Numeric parameter/point fields are seeds, not fixed-accuracy root positions. Physical-node accuracy must come from
  the certified enclosures and bounded refinement in the later isolate/qualification children, not those midpoints.
- Proof review: coordinator inspected the scoped methods and checked the Krawczyk criterion against
  [Rump, Theorem 13.3](https://www.tuhh.de/ti3/rump/intlab/ActaNumerica2010.pdf) (printed p89). Strict interior inclusion
  with a truthful enclosing residual/Jacobian establishes uniqueness/regularity; a heuristic error or determinant
  threshold is not its substitute. C1 adapter domains and complete analytic certificates are explicit obligations.
- Initial evidence/resolution (`build/evidence/1A-1.b-ref-events-core/`): attempt-1 Release/ASan both exit42, three failed
  assertions in two new test identities. Independent diagnostics prove the tiny-loop roots lie in certified intervals
  while their midpoints are coarse; new assertions now test root/seed containment, not unpromised 1e-7 seed accuracy.
  Anchored-cloud work is 742/1507/3040/6109 at sizes128/256/512/1024, linear rather than constant64; the test now pins
  success under8N at all four sizes, matching the existing default anchored regression contract. No old baseline
  assertion, default cap or core policy was weakened; both failed reports and diagnostic probes remain retained.
- Final evidence: all three targets build Release/ASan, attempts1/2 exit0. Completed discovery finds14 registered
  strict cases; focused attempt2 has16 JUnit entries /634 assertions both, no failures/skips. Independent five-mode
  probes compile/run in Release/ASan under180s watchdogs, exit0; no sanitizer findings, detect_leaks=0, no LSan claim.
  Release geometry/offset union290 /1380036; ASan geometry/topology/ellipse122 /1088758; full visible Release4633 /
  JUnit tests=6221071, zero failures/errors, every prior-frame and S0 entry retained, same74 skips. DXF1028 /39932 /
  three unchanged skips; DWG1084 /34952 /four unchanged skips. Every final process exits0 and finished. ASan linking
  reports a compact-unwind size warning, not a failed build. Discovery/test processes were serialized per directory;
  only completed XML was parsed. Detailed commands/results are in the ignored summary/report artifacts.
- Commit audit: only intersection cpp/header/tests, 1504 additions/3 deletions. Tested/frozen/committed SHA-256:
  cpp `3d2e0698adf0c9bf0fdd6980cbfecf11da6272352844da19da3d42d543fd1f08`,
  header `ac16a42fc294855d55c204e65843f73fe53a8f6bcbbc35cf9f7fba74bafa6191`,
  tests `b071d60b0ffaa95bd6a505c24280fabc8cec371ba10049e781dc2a674e27de4b`.
  Next is the rate card below, not an ellipse-adapter or persistent-feature completion claim.

#### 1A-1.b-ref-rate. Continuous exact-sweep parameterization

**Executed — done at bcd6c5a4fe8a56d30b731f5e2e310e359b8ecfe5, 2026-10-15**
- Handoff: tracked product tree clean, only pre-existing untracked `build-asan/` preserved. Gibbs owns the three
  scoped files; coordinator owns roadmap, proof review, all build/test directories, evidence and Git.
- Owner: Gibbs for `librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.{h,cpp}` and
  `librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_geometry_tests.cpp`; coordinator owns roadmap,
  builds/evidence/Git. Dependencies: frames26d107ccf, strict core834a46a88. No engine, intersection core, adapter,
  component, fitting, persistent entity or registration changes belong here.
- Finding: endpoint-only canonicalization of a rounded whole period is not a continuous real curve. The independent
  113-bit a100,b50 probe gives a seam jump1.9363366072701934e-14. Wrapped/open arc endpoint subtraction and winding
  also need explicit real-angle error handling. Overlapping point hulls or display tolerance do not prove continuity.
- API/preflight: retain `Sweep` and the engine's stored numeric domain `T=abs(signedSweep)`, normalized basis and
  phase. Add immutable outward traversal-rate and **unwrapped** angle accessors to `ReferenceFrame`. Do not change the
  physical-angle helpers `ellipsePoint`, `outwardNormal`, curvature/distance queries, normalization, engine options
  or `engineSignedDistance`. `cacheReversed` still has no effect on physical side/source traversal.
- Rate contract: let the ideal directed angle be real2*pi for a whole source, otherwise exact input `angle2-angle1`
  plus winding k*real2*pi (k=+1 for forward wrap, -1 for reverse wrap, 0 otherwise). With sigma the traversal sign,
  enclose positive gamma=`abs(idealSweep)/T` using the real-pi bracket/outward arithmetic. Define
  `eta(t)=start+sigma*gamma*t`. Whole gamma is real2*pi/T; an arc has exact unit rate only when its ideal sweep equalsT.
  Validate finite positive rate before factory success. Retain exact endpoint identity without an endpoint-only jump.
- Jet/bounds tasks: use this angle/rate in every zero-width/range query and every point/first/second/product formula.
  First derivatives gain gamma, second derivatives gamma², speed-squared gamma², cross products sigma*gamma³;
  future F' gains gamma, while physical curvature and outward normal do not. Include circle cancellation, zero-distance
  and extreme-denominator refusal branches. Whole canonical trig evaluation is justified by real periodicity;
  unwrapped closing angle still advances a full real turn for quadrant/event classification.
- Error policy: stage/clear failed outputs as before; no guessed pi, rounded-atan2 or unqualified libm certificate.
  Any engine-domain/ideal-angle discrepancy must be enclosed/accounted for, not silently absorbed by raising the
  geometry tolerance. This is a corrected reference parameterization, not a materializing-engine behavior change.
- Tests: `[lc-offsetcurve][geometry][reference-rate]`. Independent multiprecision oracles derive the ideal sweep
  directly from input endpoint doubles and real pi, not from the helper's returned rate/angle. Cover whole C1 endpoint
  limits and all derivative/product factors, wrapped/nonwrapped/reversed arc endpoints and tiny usable/full/near-full
  boundaries, rotations/thin/extreme scales/large centres, zero distance and circle collapse, invalid/cleared outputs.
  Retain prior geometry cases and prove old engine geometry is within explicitly bounded numeric discrepancy; no
  fixture expectation or numeric floor is weakened to hide the seam. No topology/cusp result is exposed yet.
- Validation/exit: discover and complete nonzero focused cases; all three targets Release/ASan; focused and all
  geometry/offset/topology regressions, full visible Release, both CI lanes against S0/latest core. Freeze/hash source,
  serialize tests per directory, parse completed XML and independently review rate/endpoint and derivative proofs.
  Commit this helper correction and reconcile the plan before promoting `.b-ref-events-critical`.

**Execution evidence and reconciliation.**
- Activation base6480195a7e1156b44cfc4912b206ac61d1fe2acf. Gibbs implemented the three owned paths; coordinator
  separately reviewed the complete diff, periodicity/chain-rule/clearing proofs and exact-formula discrepancy bound.
  `traversalRate()` and `boundUnwrappedAngle()` now expose the actual committed contract. Engine/normalization/budget/
  default solver/registration behavior is unchanged; no adapter, cusp, topology, materialization or entity is claimed.
- Six registered `[reference-rate]` cases pass Release and ASan:6 JUnit entries/505703 assertions each, exit0,
  no failures/errors/skips. Independent 113-bit oracles derive ideal sweeps from input doubles and real pi. Tiny
  forward/reverse wraps discriminate every gamma factor; rotations, endpoint limits, optional products, circles,
  zero distance, extreme scales, invalid ranges and cleared failures are covered.
- Both initial focused runs exit42 with18 TinyWrap assertions: a finite-precision `sin(Quad(2*pi))` residue was
  incorrectly treated as an exact-zero endpoint oracle. The correction evaluates an equivalent endpoint-relative
  angle on the entire final half, not just at the endpoint. Failed reports are retained. No production enclosure,
  cap/tolerance or prior baseline assertion was weakened. Conservative bounds need not exclude unit-rate values;
  the new tests instead prove the two exact quantities differ and the correct one is enclosed.
- All three target builds Release/ASan, attempts1/2, exit0, parallel4 each/combined<=8. Full visible Release exit0:
  4639 entries/6726774 JUnit tests/74 identical skips; all prior-core/S0 identities retained, plus6/75 respectively.
  Combined geometry/topology/ellipse ASan repeat exit0:128 entries/1594461 assertions/0 skips/findings, prior-core
  entries retained plus6. Original geometry union XML passes296/1885739/0 skips, but its terminal observation was
  lost across context/registry expiry; full Release is an explicitly passing superset. The initial ASan union also
  has complete passing XML; the repeat supplies an explicitly observed exit. No unobserved exit is claimed.
- DXF/DWG CI lanes exit0 with unchanged1028/39932/3 skips and1084/34952/4 skips; prior testcase/skip identities
  retained. Exact exclusions remain §0.8. Tests serialized per directory, only completed XML parsed; all required
  processes finished. `ASAN_OPTIONS=detect_leaks=0`; no LSan claim. Compact-unwind size warning is not a link failure.
  Commands/failures/reviews/reports are under `build/evidence/1A-1.b-ref-rate/{summary.md,review-notes.md,*junit.xml}`.
- Product commit audit: only geometry cpp/header/tests,396 additions/45 deletions. Tested/committed SHA-256:
  cpp `161149f26c4d233c8ae6a0133fea6947af080a432474d24d33e040057249e66d`,
  header `7638c9c33c51ffc588bd3199dc22de1ec70da8c87f81cff536cafd9972a15f25`,
  tests `43b53c30a9e57101225b4259aa5bce03454c7e30c53f8c4bc20e862f154ada75`.
  R01/R02/R12 foundations advance, not their full parent completion. The next card below was refined after inspecting
  this committed source and Gibbs's read-only exact-threshold/budget/hook preflight.

#### 1A-1.b-ref-events-critical. Critical Adapter and Proven Regions

**Executed — done at 943406abe5cfff8b188947757f101791bc3da8ad, 2026-10-15**
- Handoff: tracked tree clean, only existing untracked `build-asan/` preserved. Gibbs starts product edits after this
  activation; coordinator retains roadmap, build/test directories, independent review, evidence and Git ownership.
- Owner: Gibbs, same three geometry cpp/header/existing-test paths as `.b-ref-rate`; coordinator owns roadmap,
  builds/evidence/Git and independent proof review. Dependencies: frames26d107ccf, strict core834a46a88,
  ratebcd6c5a4f. Inspect actual checkpoint HEAD/status first; preserve pre-existing `build-asan/`. No changes to
  engine materialization, intersection core/default solver, topology components, fitting, entity or build lists.
- Activation facts: `ReferenceFrame` has positive `traversalRate()`, `boundUnwrappedAngle()` and checked source/offset
  jet/bound APIs over `[0,T]`. Factory success does NOT guarantee every valid bounds query succeeds: the existing
  thin-ellipse regression constructs a frame with an unrepresentable offset denominator. Pre-publication descriptor
  bounds/seed checks below are mandatory. `LC_ParametricCurves` and strict proof/domain records live in
  `lc_parametriccurveintersection.h`; reread all seven virtual signatures and strict certificate validation, not a
  guessed `lc_parametriccurves.h`. At activation, `WorkBudget` lacked the depth accessor/propagated-limit latch;
  this slice implements both.
  Installed Boost's exact rational backend is available; keep its types private to the cpp. No new dependency is needed.
- Result/API contract: add `ReferenceFrame::boundOffsetFactor(t0,t1,f,ft)` with finite outward F/F'_t and both outputs
  cleared on refusal; share its physical curvature/rate kernel with `boundOffsetJet`, not an independent formula.
  Add factory-owned immutable `CriticalAdapter final : LC_ParametricCurves`, with const frame/regime/occurrence/region
  accessors, and `analyseCritical(basis,signedDistance,WorkBudget&)`. Its result has `LC_IntersectionStatus`, typed
  reason (`None`, `CollapsedCircle`, `UnresolvedGeometry`), optional `unique_ptr<const CriticalAdapter>` and cumulative
  `CheckWork`. Typed occurrences carry parameter enclosures, logical quadrant/winding identity, `SimpleCusp` or
  `CriticalStationary`, and `MonotonePolynomial` or `ExactAxisThreshold` provenance. Region records retain a segment,
  axis-band flag and regular/single-cusp/stationary projection proof. No public mutable/unchecked construction.
  `Ok` means complete critical classification and gap-free proven descriptors, NOT exhaustive intersections/topology.

**Ordered implementation tasks.**
1. Preflight finite normalized raw inputs without repairing them. Charge the initial analysis item before building
   the fixed exact kernel. Handle zero/outward distance before squaring. For inward D=-delta>0, construct exact
   `cpp_rational` values from input doubles, `A=Mx²+My²`, r and D. Compare `D²` with `A*r⁴` at the minimum curvature
   radius and `D²*r²` with A at the maximum. This selects positive F, minimum stationary, intermediate, maximum
   stationary or negative F. For r==1, first test exact `D²==A`: collapse is nonisolated and refuses. Do not use rounded
   `minRadiusOfCurvature`, overlap of intervals, or power-of-two-only equality. Example100/double(.6)/36 is strictly
   above the minimum, not exactly critical; rotated(3,4) axes with r=.5 admit exact1.25/10 threshold tests.
2. Map true axes k*real(pi/2) into enclosed parameter bands through the committed gamma/unwrapped-angle contract.
   Separately prove each axis inside, outside or exactly at an established endpoint by comparison with the ideal
   unwrapped endpoint expressions. Neither overlap nor clipping of an enclosure proves membership; uncertain
   membership refuses. Retain strict constant-bracket inequalities when an exact-double endpoint equals a bracket
   edge. Example forward0->M_PI_2 has its true pi/2 axis just OUTSIDE, even if its enclosure overlaps the endpoint.
   Intersect coverage bands with `[0,T]` only after this classification; a band midpoint is never the true axis.
   Build contiguous regular/band descriptors with identical stored boundaries, no gaps/dropped bands, no zero-length
   published segment. Bounded fixed candidate axes and checked linear capacities precede adaptive refinement.
3. For0<r<1 and intermediate D, use the strictly monotone polynomial on z=sin²(eta):
   `P(z)=r²*(A*r⁴-D²)+3*A*r⁴*(1-r²)*z+3*A*r²*(1-r²)²*z²+A*(1-r²)³*z³`.
   For inward D, sign(P)==sign(F). Evaluate it exactly at outward z endpoints (clamped only to the proven range[0,1]);
   its monotonicity avoids interval coefficient cancellation near thresholds. If a rational must become a double
   interval, verify exact inequalities at both endpoints, allow only bounded `nextafter` corrections and refuse an
   unsuccessful finite bracket. Fixed-degree kernels have bounded bit lengths for finite IEEE64 inputs. No rounded
   transcendental sign or fixed-precision multiprecision estimate is a certificate.
   Optional scalar refinements may stop at adjacent representable endpoints only when exact residual signs verify
   the already unique monotone root remains bracketed. This is a completed scalar certificate, not acceptance of an
   unresolved geometry box; angle/membership/region/publication gates still follow.
4. Count/isolate the one simple cusp per true quadrant, then prove membership and distinct order in the actual
   directed/wrapped sweep. Exact threshold equality instead gives isolated stationary-axis occurrences, subject to
   the SAME proven inside/outside/established-endpoint membership gate before clipping or publication. Use bounded
   refinements and certified enclosures, never root midpoints as exact geometry. A cusp need not be a representable
   segment endpoint. Close to a free end, separate inside/outside or refuse; do not manufacture an exact non-axis
   endpoint cusp by rounding `asin`/curvature. Qualify exact stationary endpoint cases through the endpoint identity.
5. Prove every regular/axis-transition region and local construction-join neighborhood. Physical curvature is strictly
   monotone within a true quadrant. If its unique simple F zero is eta_c, use the existence identity
   `C'_eta dot N(eta_c)=(a*r/h_c)*sin(eta_c-eta)`: F and sine change sign together, so the offset projection is strictly
   monotone despite the stationary point. Do not substitute a rounded approximate cusp normal. Constant F sign and
   a source projection prove regular regions; exact critical regimes have isolated stationary zeros without a sign
   reversal. Axis bands need their own proof; two unresolved nearby cusps cannot inherit the one-quadrant proof.
   Separate/qualify them or refuse. Never claim a proof across an unresolved transition band.
   The same cusp-normal argument also covers an angular span<real pi with exactly one completely counted simple F
   zero strictly inside: both factors change sign only there. Constant-sign intervals of span<real pi admit a dual
   source projection with derivative cos(eta-mid)>0. These existence proofs, not rounded normals, qualify cached
   cross-axis join neighborhoods; unresolved membership or two roots still refuses.
6. Before publication, perform charged checks for each descriptor's full offset bounds and endpoint/midpoint
   zero-width bounds/seeds, matching strict preparation. Finite frame construction alone is insufficient. Valid but
   unrepresentable bounds/seeds refuse with `AmbiguousTopology/UnresolvedGeometry`, never an `Ok` adapter that
   immediately fails strict preflight. All descriptor proofs and all preflight checks must succeed before publication.
   Publish one branch with contiguous C1 segments and continuous one-sided jets: `branchCount()==1`, `next(0)==0`
   only for whole sources, otherwise-1. `evaluate()` returns finite frame midpoint seeds; `bound()` delegates to the
   outward frame and validates segment/range membership, including zero-width queries. Invalid indices/requests
   clear outputs. The adapter owns immutable cached proofs, not a dangling budget reference.
7. Override `certifyInjective()` only for requests wholly covered by a cached mathematical proof; the returned
   certificate interval must cover the request and remain inside the named segment. `certifyIntersection()` returns
   `AnalyticExclusion` only if joint injectivity over the complete intervening path proves no distinct-occurrence
   contact, or only the recognized construction-shared end, throughout the WHOLE requested rectangle. Its
   `proofDomain` remains within both named segments even if the underlying proof spans a larger region. Whole-seam
   exclusion additionally requires exact real-period lifting into a jointly injective neighborhood. Endpoint equality
   or each leaf's separate injectivity is insufficient; otherwise return `Unavailable`. No `Unique` contact, arbitrary
   tangency or fabricated two-occurrence cusp intersection is promised by this leaf.

**Budget, failure and staging protocol.**
- Add `WorkBudget::depthLimit()` and `latchLimitExceeded()`; preserve the first latched invalid/limit state and consumed
  counters. Never synthesize `limit+1`, overflow an unsigned depth or invent consumed work to propagate a refusal.
- Root: `checkDepth(0)`, then `consumeVertices(1)` before allocation; `consumePairs(1)` before fixed threshold
  examination. Charge each new queued box once before creation and each examination once before fixed angle/
  polynomial/proof calculations. A split checks child depth and consumes two vertices before queuing either child.
  Promotion to a region reuses that box's token; each distinct occurrence certificate consumes another vertex before
  insertion. All generated/dead/discarded work remains spent. No cubic-piece debit: no cubics exist in this leaf.
  Const adapter callbacks are fixed-cost cached proofs/bounds; no hidden adaptive search or resettable private budget.
  Consume one pair before each descriptor's fixed pre-publication group (one full bounds query, three zero-width
  bounds and three seeds); these checks are not a free post-processing loop. Return an already latched budget status
  before doing new work, and take the cumulative work snapshot on every path.
- Malformed input: `InvalidInput`; valid but numerically unresolved geometry or nonisolated collapse:
  `AmbiguousTopology` with reason; exhausted count/depth: `LimitExceeded`. Midpoint stagnation without a completed
  certificate is unresolved, never success. The exact verified scalar-root terminal case above is distinct from an
  unresolved region/join subdivision. Every failure returns no adapter/partial published list, but retains cumulative
  consumed work and budget
  latch. Resource refusal has priority once it occurs. No cap increase/default-tolerance change to hide failures.

**Validation and exit.**
- Register nonzero `[lc-offsetcurve][geometry][reference-critical]` cases in the existing file. Independent
  multiprecision oracles check F/F'_t, real-axis/rate mapping, true cusp containment/count/order and outward jets, not
  the returned adapter's own midpoint output. Cover whole/open/wrapped/reversed, rotated(3,4) exact thresholds,
  irrational axis length,100/.6/36 and neighboring doubles, both sides of each threshold, very close distinct cusps,
  interior simple cusps/near-free-end classification, exact stationary endpoints and circle collapse. Include
  a5/r.5/D10, forward0->M_PI_2: prove the true stationary axis is outside, never publish a clipped false occurrence;
  separately a5/r.5/D1.25 at raw angle0 proves an exact stationary endpoint. The existing thin-frame failure must
  produce no successful adapter or partial list.
- Assert contiguous coverage of all `[0,T]`, covered axis bands, truthful whole-region injectivity, invalid request
  clearing and all failure result staging. Test unresolved bands, rational conversion underflow/overflow, no false
  exact equality, seam/join exclusion versus nearby re-crossing, and `Unavailable` for unproved pair boxes. Use
  strict-query smoke cases to exercise the hook validator; full own/opposite/cutter exhaustiveness is the later
  isolate leaf after the two core prerequisites below.
- Probe exact consumed vertex/pair caps, each one-less cap with counts>1, depth exhaustion, no-progress subdivision,
  pre-spent budgets, first-latch preservation and unchanged cubic counts. Zero limits remain `InvalidInput`.
- Freeze/hash the three files; build all three targets in both configs, parallel4 each/combined<=8. Discover/focus
  `[reference-critical]` in Release/ASan; run geometry/offset union, combined ASan, full visible Release and both CI
  lanes with the exact §0.8 exclusions. Compare completed XML testcase/skip multisets to rate and S0; no new required
  skips. Record attempts/fixes, explicit exits/counts/hashes and separate §0.7 reviews under
  `build/evidence/1A-1.b-ref-events-critical/`. Commit product, then reconcile/commit only this roadmap (§1.4).
  Literal new-tag commands, after building the frozen tree and creating that evidence directory:

```bash
build/librecad_tests --list-tests "[reference-critical]"
QT_QPA_PLATFORM=offscreen build/librecad_tests "[reference-critical]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-events-critical/release-attempt-1.junit.xml
QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[reference-critical]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-events-critical/asan-attempt-1.junit.xml
```

- Readiness review: Gibbs/coordinator inspected the rate commit and actual strict validators/WorkBudget. Two
  independent findings (factory-versus-query success and clipped stationary-axis membership) are resolved in the
  ordered tasks/tests above, not left as future TODOs. Per-node accounting and whole-seam proof domains were also
  checked. This is readiness evidence, not implementation or passing adapter-test evidence.
**Execution evidence and reconciliation.**
- Activation basecec441396640ec53c8cbfb35a5c42a9f0a4a3b8f. Gibbs implemented the three owned paths; coordinator
  independently reviewed the whole diff, exact rational/angle/root bounds, projection existence proofs, periodic
  lifting, named solver domains, immutable staging, finite publication and irreversible accounting. Existing build
  registrations suffice; no default solver/core/engine/entity/filter/UI changes. R01/R02/R12 advance only as foundations.
- Pre-build review corrected a thin-ellipse inverse-bracket refusal: coarse competing upper bounds could choose
  1-z near1. Exact P(1/2) selects the small variable<=.5; whole/reversed a100/r.02/D5.3 andr.01/D10 regressions pass.
  Positive exact polynomial signs independently prove the counterexamples; no nonexistent native failure is claimed.
- Both focused attempt1 runs exit42,9 entries/264431 assertions/one failure: a5/r.5/D=nextafter(1.25,+inf) refused
  an already complete adjacent-float scalar root bracket. Exact residual signs now verify that terminal bracket
  before stopping; unresolved geometry subdivision still refuses. The card was refined before the fix. Failed reports
  retained; initial cpp hash0a338648c01bb3d7efb46aaad1507622e4819eceab71016cf3739d1a74025f1a. No test/cap/tolerance weakening.
- Nine registered cases discovered, exit0; focused attempt2 Release/ASan exit0:9 entries/268244 assertions each,
  no errors/failures/skips/findings. Geometry/offset Release exit0:305/2153983/0 skips; combined geometry/topology/
  ellipse ASan exit0:137/1862705/0. Both retain every rate entry plus9. Full visible Release exit0:4648 entries/
  6995018 JUnit tests/74 identical skips; all rate/S0 identities retained, plus9/84 respectively.
- All three targets Release/ASan, build attempts1/2, exit0, parallel4 each/combined<=8. DXF/DWG CI lanes exit0:
  unchanged1028/39932/3 skips and1084/34952/4 skips, all rate identities retained; exact §0.8 exclusions. Only completed
  XML parsed, tests serialized per directory, all required processes finished. `ASAN_OPTIONS=detect_leaks=0`, no LSan
  claim; compact-unwind size warning is not a link failure. Independent installed-Boost probe compile/run0 checks
  conversion/extreme predicates, thin selection and completed scalar brackets; transcendental values are estimates.
  Detailed commands/reviews/attempts live under `build/evidence/1A-1.b-ref-events-critical/`.
- Commit audit: only geometry cpp/header/tests,1264 additions/18 deletions. Frozen/tested/committed SHA-256:
  cpp `832c1d1242a97674ba61b968b9f06c6279a49f08492986057fc0394d8df9aeb7`,
  header `ec4adc0fa760b1091c4e8e1f5803f0e1d2f09a8379a66042222a9099677ebb74`,
  tests `da182a946eb0703fced58d95b3042ae77c7e8a707a40a3ef694a32cd628a554f`.
- Read-only next-leaf preflight exposed two core API/accounting gaps, not critical-adapter TODOs. The named children
  below now precede geometry isolation. Critical preparation is done; events/components/check/adoption/file/UI parents
  remain incomplete. This checkpoint promotes accounting only, not refinement or isolation.
  Structural audit:27 main rows,57 dependency nodes,15 requirements,14 formats(8DXF/6DWG), acyclic; all new children
  have named predecessors and no parent is prematurely done.

#### 1A-1.b-ref-events-accounting. Observed Adaptive Depth

**Executed — done at290d2658fdc2d9bb55da6205ae7c3afaaade6a4a, 2026-10-15**
- Handoff: tracked tree clean, only existing untracked `build-asan/` preserved. Gibbs starts the three core paths;
  coordinator retains roadmap, build/test directories, independent review, evidence and Git ownership.
- Owner: Gibbs for `lc_parametriccurveintersection.{h,cpp}` and existing
  `tests/lc_parametriccurveintersection_tests.cpp`; coordinator owns roadmap, independent review, builds/evidence/Git.
  Dependency: critical943406abe. No geometry/engine/entity/build-list changes. Confirm the actual checkpoint HEAD,
  clean tracked tree and current solver methods before editing; preserve `build-asan/`.
- Finding: `LC_IntersectionWorkStats` exposes pair/generated counts but not actual adaptive depth. Passing configured
  limits or BVH height to `WorkBudget::checkDepth` would invent observed work. This child changes observability only,
  not solver decisions, event identities, old counters, options/defaults, root classification or refusal behavior.
- Add zero-initialized unsigned `deepestPreparation`, `deepestPairRefinement`, `deepestOrderingRefinement` plus const
  `deepest()` returning their maximum. Root depth is0; no examination also yields0, distinguished by existing counts.
  These are ENTERED adaptive levels, not queued/unexamined children, configured maxima, index height or fixed Newton
  iterations. A future refinement child adds its own root-contraction statistic, not a reinterpretation of ordering.
- Preparation: ordinary `buildLeaves()` and strict `prepare()`, after popping a box immediately before its first
  bounds examination. Each segment starts at0. Pair solving: ordinary `solvePair()` and strict `solve()`, after the
  pair charge succeeds and a cell is popped, before bounds/proof work; each initial leaf-pair starts at0.
- Strict ordering: after a comparison charge succeeds, observe its current `attempt`. Pass `attempt+1` into
  `shrink()` and observe that depth only AFTER its contraction charge succeeds and before contraction work. Check
  `attempt<maxDepth` before forming the increment; preserve `maxDepth=0`. Nested certificate validation/merge work
  stays at the enclosing depth. Ordinary fixed contact/Newton probes are not adaptive ordering; its field remains0.
- Retain observed maxima on EVERY `Ok`, `InvalidInput`, `AmbiguousTopology`, `LimitExceeded` exit. Rejected charges
  and uncreated children never raise them; examined terminal boxes do. Updates must not reset existing work between
  segments/pairs, change old charge order, or add another allocation/comparison. Existing counters remain identical.
- Tests: nonzero `[intersection-accounting]`. Root-only transverse/preparation/empty-invalid cases; truthful loose
  bounds forcing independently observable deeper preparation; exact-depth and one-less limits; pair subdivision and
  ordering contraction; failed child allocation/next examination/contraction charge do not record hypothetical depth;
  invalid bounds/ambiguity after subdivision retain prior maxima. Preserve all existing default/strict topology and
  exact-counter assertions. Use adapter-recorded examination boxes to verify depth, not the new fields as their oracle.
- Freeze/hash three files; build all three targets Release/ASan (parallel4 each/combined<=8), discover/focus the new
  tag and existing `[intersection-strict],[intersection-budget]`, full geometry/offset and combined ASan, full visible
  Release, both exact-exclusion CI lanes. Compare complete XML to critical/S0; no new skips/lost tests. Evidence:
  `build/evidence/1A-1.b-ref-events-accounting/`; actual exits/counts/hashes and separate §0.7 reviews precede the
  product commit, then this plan-only checkpoint. Next: refine the session card against the newly committed stats.
  Literal new-tag commands after the frozen builds and evidence-directory creation:

```bash
build/librecad_tests --list-tests "[intersection-accounting]"
QT_QPA_PLATFORM=offscreen build/librecad_tests "[intersection-accounting]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-events-accounting/release-attempt-1.junit.xml
QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[intersection-accounting]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-events-accounting/asan-attempt-1.junit.xml
```

**Execution evidence and reconciliation.**
- Activation basebea4ef7bfc541870eb01c8fea519140a482867ab. Gibbs implemented the three core paths; coordinator
  independently reviewed every added observation, charge/pop/failure/overflow guard and the dyadic/call-trace test
  oracles. Production changes only record maxima and pass guarded depth to private shrink; old geometry, options,
  callbacks, counters, charge order and ordinary/strict decisions remain unchanged. Existing build lists suffice.
- Both focused attempt1 runs exit42:5 entries/153 assertions/two checks failed in one new test. It assumed ordinary
  and strict preparation stop at the same refused bound at depth3. Source and independent trace show ordinary mode
  continues to its maxLeafDepth8 and strict mode stops at3. Only the new expected-depth value was corrected; status
  and trace-to-stat checks remain, no old assertion/cap/tolerance/production behavior changed. Failed reports retained;
  initial test hash64017610a7e15d7c8585dbcac8ee1da4368ecac15eb4ea1629a158e9168275c2.
- Five registered cases discovered, exit0. Focus attempt2 Release/ASan exit0:5 entries/153 assertions each, no errors/
  failures/skips/findings. Old strict/budget union exit0 both:32 JUnit section entries/2194 assertions/0 skips; all
  previous strict16 and budget16 report entries retained. Geometry Release310/2154136/0 and combined ASan142/1862858/0,
  both exit0 and every critical baseline entry retained plus5.
- Full visible Release exit0:4653 entries/6995171 JUnit tests/74 identical skips, all critical/S0 identities retained,
  plus5/89. All three targets Release/ASan builds, attempts1/2, exit0, parallel4 each/combined<=8. DXF/DWG lanes exit0:
  unchanged1028/39932/3 skips and1084/34952/4 skips with exact §0.8 exclusions. Only complete XML parsed; serialized
  test binaries per directory; all required processes finished. `ASAN_OPTIONS=detect_leaks=0`, no LSan claim. Compact-
  unwind size warning is not a build failure. Commands/reviews/attempts: `build/evidence/1A-1.b-ref-events-accounting/`.
- Commit audit: only intersection cpp/header/tests,440 additions/2 deletions. Frozen/tested/committed SHA-256:
  cpp `c3f1ea66063532b5cb4c0fe463ec5dfe68a39e7f692d04df7a8c6895a2ae1611`,
  header `f4b099167d368fd826acef1326332528e643f2206249ac3555127ab35598ada2`,
  tests `0b347cd8908fffd678ed14a4310306075d15eaf0eff1b3a2e5e2becacbb2e03f`.
- Next-core declaration/privacy prototype compiled syntax-only0 using actual Ninja gnu++17 flags: move-only owned
  PIMPL, private issuer/index handles and rejection of event/snapshot-to-handle construction. This proves declaration
  readiness only, not runtime/session behavior. Independent Boost-only cusp-image probe compile/run0 covers48 cases
  plus exact stationary identities. The separate critical-refinement gap and geometry depth-admission diagnostic are
  now owned by the named child below, not hidden core changes. R02/R12 and all downstream parents remain incomplete.
  Post-commit structural audit:27 main rows,58 dependency nodes,15 requirements,14 formats(8DXF/6DWG), acyclic;
  only the core retained-session card is promoted now.

#### 1A-1.b-ref-events-refine. Trusted Root Contraction

**Executed — done atf4f87f48ed024589e31a9a9f1c543340ca1a6c49, 2026-10-15**
- Activation: tracked tree clean; only existing untracked `build-asan/` preserved. Accounting290d2658f and its
  plan checkpoint2e75f4c7a are ancestors of HEAD. Gibbs owns product edits; coordinator owns verification/roadmap/Git.
- Qualified charge units: two fixed generated items cover session/PIMPL control state and issuer lifetime metadata,
  admitted before allocation; one extra item per newly retained root covers its history/cached-image record in addition
  to the existing event-certificate charge. Subsequent contraction uses existing charged certificate work and staged
  publication charge. Legacy queries consume none of the additional items. Exact/one-less tests must verify this
  schedule; it is a bounded logical work model, not a claim that each token equals one heap allocation.
- Owner: Gibbs for the same three intersection paths; coordinator owns roadmap, independent proof/lifetime review,
  builds/evidence/Git. Dependency: accounting290d2658f. No geometry/engine/entity/build-list edits. Confirm clean tracked
  tree/actual checkpoint HEAD, preserve `build-asan/`, reread `StrictSolver::run/shrink/merge/order` and new depth fields.
- Exact paths: `librecad/src/lib/engine/document/entities/lc_parametriccurveintersection.h`, the adjacent `.cpp`, and
  `librecad/src/lib/engine/document/entities/tests/lc_parametriccurveintersection_tests.cpp`. Existing registrations
  cover these files; any additional product/build path requires revising the card before editing it.
- Activation facts at2e75f4c7a: strict run moves `m_events` out and holds references to curves/options/segments. Private noninflating
  shrink preserves category/proofDomain through makeEvent but loses its state on return. New observation fields cannot
  reconstruct per-root history. Gibbs/coordinator preflight and the language/privacy probe qualify the API below;
  no implementation or runtime coverage is claimed by the prototype under `build/evidence/1A-1.b-ref-events-refine/`.
- Exact public shape (types/fields below, not an implementation stub):

```cpp
LC_CertifiedQueryResult findCertifiedIntersections(
    std::unique_ptr<const LC_ParametricCurves> curves, LC_IntersectionOptions options);

class LC_CertifiedIntersectionSession {
public:
    std::size_t rootCount() const;
    LC_CertifiedRootHandle rootHandle(std::size_t index) const;
    std::optional<LC_CertifiedRootSnapshot> snapshot(const LC_CertifiedRootHandle&) const;
    LC_CertifiedContractionResult contract(
        const LC_CertifiedRootHandle&, const LC_CertifiedContractionAllowance&);
    const LC_IntersectionWorkStats& cumulativeWork() const;
};
```

- `LC_CertifiedQueryResult`: status/work/unique owned session. `LC_CertifiedRootSnapshot`: event value, cached outward
  imageX/imageY in provider coordinates, admittedContractions. `LC_CertifiedContractionAllowance`: remainingPairs,
  remainingItems, absoluteRootDepth (zero defaults mean no allowance). `LC_CertifiedContractionResult`: status,
  optional newly contracted snapshot, per-call work. Handle: private issuer identity and stable root index, default
  invalid, no event/snapshot/index-to-valid-handle public constructor. Session: private factory constructor, deleted
  copying, declared moves and out-of-line PIMPL/destructor; issuer survives moves. No raw-public-event proof entry.
  A moved-from session has zero roots and rejects handles without work. Move assignment expires handles belonging to
  the overwritten destination while preserving the transferred issuer; destruction expires that issuer. Tokens must
  not rely on a reused allocation address. Require no caller-held mutable geometry aliases for the provider's lifetime.

**Ordered implementation tasks.**
1. Add the types/declarations and distinct zero-initialized `deepestRootRefinement`, included in deepest(). Implement
   an owned PIMPL that owns provider and copied options BEFORE binding retained solver references. New factory always
   runs strict mode and returns a session only on complete success, including a valid empty-root result. Providers
   represent immutable owned geometry; callbacks capturing dangling references cannot be repaired by ownership.
2. Reuse the strict proof machinery, not a second Krawczyk implementation. Keep legacy `findIntersections()` behavior,
   decisions and old counters unchanged in BOTH policies. Add retained-session mode/state publication internally;
   charge issued metadata/certificate records and precheck bounded capacities before allocation. Failure returns no
   session/partial root list and all spent work. Snapshot/handle access is cached O(1), no uncharged bound queries.
3. Track admitted contraction history with each root from established existence through ordering. Exact merges inherit
   maximum parent history; it follows the root through removal/compaction/sort, never a stale parallel index. Initial
   pair exploration has its own observed depth, not an invented root-contraction count. Freeze immutable issuance
   proof/category/domain, segment IDs and logical root identity AFTER final verified identity merges.
4. For the NEW owned entry, enforce original absolute per-root depth during ordering before admission, not by rejecting
   overconsumption after issuance. This cumulative policy is intentionally stricter than legacy per-comparison loops;
   document/test it specifically for the new factory. Do not silently change legacy strict or ordinary decisions.
5. Validate issuer/lifetime/index before examination. Wrong-session, expired, default/out-of-range handles yield
   InvalidInput, empty new snapshot and zero work, without mutation. Edited public snapshot copies never enter trusted
   state. Post-issuance indices are stable; contracting one root cannot resort, merge away or retarget another.
6. Contract the issued isolation with noninflating Krawczyk/qualified analytic enclosures. Even an Undecided Krawczyk
   image contains the known root by the mean-value argument; another uniqueness inclusion is unnecessary. Intersect
   previous intervals, retain original provenance/domains and the SAME root. Exclusion contradicts existence and
   refuses; unsupported singular contraction honestly refuses. No arbitrary tangency proof is minted.
   Define internal session validity separately from per-call status: an invalid/default/foreign handle is call-local
   InvalidInput with zero work and no state change; unchanged/unsupported contraction is nonterminal ambiguity;
   resource exhaustion is sticky LimitExceeded; malformed provider evidence, exclusion of an issued root or empty
   required image intersection is terminal proof invalidity. On terminal invalidity preserve historical data/work
   privately for diagnostics but issue no trusted snapshot/handle (rootCount becomes zero); subsequent valid-handle
   contractions return InvalidInput with no new work. A limit latch preserves the last valid cached snapshots but
   forbids further proof work. Invalid-handle calls never clear either latch, and moves preserve both states.
7. Each call intersects pair/item/absolute-depth allowances with original session ceilings minus cumulative spent work.
   Check before increment/examination/allocation; failed admission consumes no hypothetical depth. Resource exhaustion
   latches irreversibly. Maintain fresh per-call and separate cumulative ledgers; maxima cannot be subtracted. Record
   absolute root depth actually entered, including admitted ordering/contraction attempts; it persists across calls.
8. Stage new isolation, finite images intersected with the previously cached images, and midpoint seeds. Publish only
   after every check succeeds; an empty/nonfinite image intersection contradicts the retained root. Failure
   preserves previous certified geometry/provenance, while admitted history/work remains spent. Strictly unchanged
   isolation yields ambiguity when another contraction was requested. Geometry checks its outward physical-accuracy
   allowance BEFORE requesting more work; parameter width or a midpoint residual is never world-space accuracy.

**Validation and exit.**
- Nonzero `[intersection-refine]`: compile-time rejection of session copying/event-to-handle conversion; owned provider/
  options lifetime, move construction/assignment/moved-from use and destruction; wrong/expired/default handles,
  edited snapshots, ordering history/merge inheritance,
  repeated-call exact/one-less/pre-spent ceilings and latches. Check per-call smaller maxima after a different root has
  raised cumulative maxima, no max subtraction; no state retargeting across close roots or sorting.
- Inject a provider contradiction after issuance: verify no subsequent trusted snapshots/handles or successful
  contractions, preserved diagnostic work, and no latch reset through a bad handle or move. Separately verify a
  resource latch retains prior valid cached geometry but allows no further proof work, and ambiguity is not silently
  upgraded to a new result. The fixtures intentionally violate provider evidence only to exercise fail-closed behavior.
- Independent analytic roots/interval images prove original-domain/root containment and contraction. Cover cancellation-
  sensitive residuals, singular analytic contacts, no-progress/representation limits, failed publication with spent
  history and preserved geometry. No new cancellation API is implied. Old strict/budget counter assertions stay intact.
- Freeze/hash, build all three targets Release/ASan, discover/focus new and old core tags, geometry/offset and combined
  ASan, full visible Release and exact-exclusion fast lanes; compare complete XML to accounting/S0. Evidence below
  `build/evidence/1A-1.b-ref-events-refine/`, no new skips. Separate provenance/lifetime/cap reviews, actual results and
  product commit precede the plan-only checkpoint. Only then activate the geometry critical-refinement child.
  Literal new-tag commands after frozen builds and evidence-directory creation:

```bash
build/librecad_tests --list-tests "[intersection-refine]"
QT_QPA_PLATFORM=offscreen build/librecad_tests "[intersection-refine]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-events-refine/release-attempt-1.junit.xml
QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[intersection-refine]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-events-refine/asan-attempt-1.junit.xml
```

**Execution evidence and reconciliation.**
- Gibbs authored/froze only intersection header/cpp/existing tests from2e75f4c7a; coordinator independently reviewed
  every ownership, identity, interval/provenance, merge/history, admission, overflow, latch and publication path.
  Existing registrations suffice. Strict proof/Krawczyk machinery is retained, not duplicated. Legacy policies,
  decisions, callbacks/counters and old tests remain unchanged. Fourteen source-declared cases execute natively.
- Review found retained malformed analytic categories could reset ambiguity to Ok and preserve trusted state.
  Retained mode now returns terminal InvalidInput for malformed categories/self-diagonal credentials; legacy ambiguity
  remains intact. Scoped per-call ledger reset and terminal invalidation on throwing providers prevent dangling
  stack-ledger access. Tests prove bad handles/moves cannot clear proof/limit latches; no known issue remains in this leaf.
- All three named targets build Release/ASan, attempt1 exit0, parallel4 each/combined<=8. Both discoveries exit0:14
  registered. Focus attempts1/2 exit0 both:14 entries/490 assertions/0 skips/errors/failures/findings. Discovery handles
  were explicitly reaped after attempt1 launched, so final attempt2 was repeated after confirmed discovery completion;
  both reports retained, no source or assertion/cap change. Final runs serialize binaries per build directory.
- Old strict/budget/accounting union exit0 both:37 entries/2347 assertions/0 skips (prior strict/budget32 plus existing
  accounting5). Release geometry324/2154626/0, combined ASan156/1863348/0, both exit0, accounting baseline+14.
  Full visible Release exit0:4667 entries/6995661 JUnit tests/74 identical skips; every accounting/S0 identity retained,
  +14/+103. DXF1028/39932/3 and DWG1084/34952/4 unchanged, both exit0 with exact §0.8 exclusions.
- Only completed XML parsed; all required native processes terminal. `QT_QPA_PLATFORM=offscreen`,
  `ASAN_OPTIONS=detect_leaks=0`, no LSan claim. No sanitizer ERROR/SUMMARY/runtime-error diagnostics. ASan compact-
  unwind size warning is not failure. Independent reviews/commands/reports: `build/evidence/1A-1.b-ref-events-refine/`.
- Product commit changes exactly three paths,1044 additions/34 deletions. Frozen/tested/committed SHA-256:
  cpp `5b8c75150ac6bf3ac0d34d07e1a3aec10019019358a6f9d3576aefa5ffb22555`,
  header `9fbdab05229e8232e058dc2f8a2c19225400d04e0fe19da265b36cbf7f6d3f53`,
  tests `6aa197be4221b1ed5556f6d56920f7938f0aaccf50df230a4402255513fb3c1f`.
- R02/R12/R13 core sub-contract is verified, not completion of those whole-plan requirements. Next: qualify literal
  critical-session declarations, immutable provenance/adapter ownership, fixed costs and first native red depth pin.
  Do not reactivate pair refinement or mark event/component/check/entity/file/UI parents done. Checkpoint only this plan.

#### 1A-1.b-ref-critical-refine. Trusted Critical Occurrences

**Done —72eb5f953f5c88cf6e26505d658fef5237d562b3, 2026-10-15.** Activated at334354633548ce55697ab30c615ece42707e1e7a;
the following activation/red/failure recipes are historical evidence, not work to repeat. Final receipt below closes only this leaf.
- Scope/owner: Gibbs, geometry header/cpp/existing geometry tests, coordinator verification/roadmap/Git. Dependency:
  `.b-ref-events-refine` atf4f87f48e, with critical943406abe as the mathematical baseline. This serial boundary fixes issued-handle,
  allowance and depth-history conventions before the geometry variant; a two-curve session is NOT a scalar solver.
- Exact paths: `librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.h`, the adjacent `.cpp`, and
  `librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_geometry_tests.cpp`. Tracked tree was clean; existing
  untracked `build-asan/` preserved. Gibbs read-only preflight qualified the declarations/cost model below. Coordinator
  syntax-only declaration/access probe compiled exit0 with actual Ninja gnu++17 flags,14 core cases remain committed;
  no critical implementation/runtime qualification is claimed. The friend/token scaffold proves the C++ access pattern,
  not access to an unmodified production budget. Evidence: `build/evidence/1A-1.b-ref-critical-refine/`.

**Qualified API and ownership.** All new names live in `LC_EllipseOffsetGeometry`:

```cpp
struct CriticalCanonicalBounds {
    LC_Interval variable, inverseAngle; // u=z or1-z, alpha=asin(sqrt(u)); diagnostics only
    bool complement = false;
};
struct CriticalOccurrenceSnapshot {
    CriticalOccurrence occurrence{};
    LC_Interval imageX, imageY; // provider/local frame, no centre addition
    std::optional<CriticalCanonicalBounds> canonical;
    unsigned admittedRounds = 0;
};
class CriticalOccurrenceHandle; // default invalid; private weak issuer/index; no public record-to-handle entry
enum class CriticalWorkSource { CurrentBudget, LastMatchingBudget };
struct CriticalContractionResult {
    LC_IntersectionStatus status = LC_IntersectionStatus::InvalidInput;
    std::optional<CriticalOccurrenceSnapshot> snapshot;
    CheckWork entered{}, work{};
    CriticalWorkSource workSource = CriticalWorkSource::LastMatchingBudget;
};
class CriticalRefinementSession {
public:
    // Deleted copying, noexcept moves, out-of-line destructor/PIMPL; private builder constructor.
    std::size_t occurrenceCount() const;
    CriticalOccurrenceHandle occurrenceHandle(std::size_t) const;
    std::optional<CriticalOccurrenceSnapshot> snapshot(const CriticalOccurrenceHandle&) const;
    CriticalContractionResult contract(const CriticalOccurrenceHandle&, WorkBudget&);
    CheckWork lastMatchingWork() const;
};
struct CriticalRetainedAnalysis {
    LC_IntersectionStatus status = LC_IntersectionStatus::InvalidInput;
    CriticalAnalysis::Reason reason = CriticalAnalysis::Reason::None;
    std::unique_ptr<const CriticalAdapter> adapter;
    std::unique_ptr<CriticalRefinementSession> refinement;
    CheckWork work{};
};
CriticalRetainedAnalysis analyseCriticalRetained(const RS_EllipseData&, double, WorkBudget&);
```

- Additive retained factory, not a second analysis followed by reconstruction: builder transfers exact provenance
  directly to private immutable shared state at publication; the unique adapter and separate session retain it
  independently. The session owns mutable canonical/bracket/history and cached occurrence records privately. Core's
  composite can consume unique adapters without destroying critical-refinement lifetime. Existing `analyseCritical`
  has no new metadata charges/allocations and preserves decisions/counts except observed-depth timing repair and the
  separately demonstrated one-sided join-progress defect below. Formerly accepted inputs retain their old contracts;
  the known incorrect D100 join refusal must become a proved success, not a weakened fixture or larger tolerance.
- Snapshot parameter uses numeric `t in[0,frame.parameterEnd()]`; u/alpha are separate diagnostics. Image coordinates
  are centre-relative, world-aligned ReferenceFrame/provider coordinates: divide raw cancellation-free Qc by exact
  power-of-two scale, DO NOT add centre. Add checked interval overload
  `ReferenceFrame::pointToWorld(xLocal,yLocal,xWorld,yWorld)`; outward centre addition occurs there, outputs clear on
  refusal. Test independent local containment first, then translated/extreme-scale world containment and node rounding.
- Stationary policy: cache exact algebraic images and qualified axis parameters at issuance, canonical=nullopt.
  No further stationary parameter-tightening operation is invented. After handle/budget/health checks, a stationary
  contraction request returns nonterminal AmbiguousTopology, no new snapshot, zero entered work and unchanged history.
  Existing sticky limits still prohibit work. Geometry checks cached world accuracy before asking for contraction;
  unresolved stationary order/accuracy honestly refuses, never claims progress or regenerates geometry.
- Moves preserve issuer/weak budget identity and transferred state; moved-from sessions are empty. Overwriting or
  destroying a session expires its old handles. Expired budgets prohibit further work but do not contradict already
  cached immutable geometry. Bad handles/budgets cannot clear health/resource latches; diagnostic copies confer no trust.

**Qualified cost model and ordered handoff.** Logical records, not individual allocator calls. Let N be occurrences,
C=1 for the IntermediateThresholds canonical family (even when the source arc has N0; otherwise0), B=1 only for first budget-token issuance:

| Stage | Additional vertices/items | Additional pair examinations | Admission/exit rule |
|---|---|---|---|
| Retained publication after successful existing analysis | 3+C+N+B | 1+N | Provenance/session/issuer, canonical, N snapshots, lazy token; charge before allocations/actual validation; failure publishes neither object and never refunds |
| Successful cusp compound round | 1+N | 3+N | One staged canonical record; scalar, inverse, common validation; N staged occurrence mapping/image records; charge stages only when entered, publish symmetric updates atomically |
| Unsupported stationary contraction | 0 | 0 | No numerical round is entered; return ambiguity with original cached proof and history |

- Overflow-safe prechecks, then charge only each actual examination/allocation. Inverse work never debited if scalar
  failure prevented entry. Record h+1 immediately after first examination's charge; persistent factory scalar depth is
  not global deepest. entered.deepest is directly observed, not a difference of maxima. Numeric bounds per round:
  one polynomial Newton step; one inverse Newton or one sign-bisection fallback; at most three fixed trig evaluations;
  existing endpoint conversion checks at most eight ULP corrections per endpoint. No hidden adaptive inner loop.
- FIRST Gibbs edits only the existing test file with `[reference-critical-depth]`, then freezes for coordinator
  Release/ASan discovery/run. Pin original whole100/.5/signedDistance-30, maxVertices6/nonbinding pairs:
  LimitExceeded/V6/P23/desired deepest0. Preserve original observed deepest1 failure before production/storage changes.
  Do not commit a knowingly red tree. Then authorize Gibbs to implement the complete geometry slice and tests under
  `[reference-critical-refine]`; repair timing first, preserve original V6/P23 and all existing resource decisions.
  Retained metadata is allocated only after successful legacy analysis, so it must not move the original failed-split
  boundary; add a retained-path equivalent plus successful-publication exact/one-less overhead guards.
- After freeze, build all three targets Release/ASan, discover/run both focused tags and old `[reference-critical]`,
  geometry/offset and combined ASan, full visible Release and both exact-exclusion fast lanes; compare to coref4f87f48e
  and S0. Record actual commands/exit/count/skip/digests, separate §0.7 reviews, product commit then plan checkpoint.
  Literal first-stage commands after evidence-directory creation and test-only frozen builds:

```bash
build/librecad_tests --list-tests "[reference-critical-depth]"
QT_QPA_PLATFORM=offscreen build/librecad_tests "[reference-critical-depth]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-critical-refine/depth-red-release.junit.xml
QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 build-asan/librecad_tests "[reference-critical-depth]" --order decl --reporter junit --out build/evidence/1A-1.b-ref-critical-refine/depth-red-asan.junit.xml
```

**Native reproduction receipt (before geometry edits).** Gibbs added only the test file,21 lines/one case/11 asserts.
All three named targets build0 both, discoveries0/one case each, then Release/ASan red focus both exit42:
one case/11 assertions/exactly one failure `result.work.deepest == 0` expanded `1 == 0`, errors0/skips0. Other10
checks pass, including LimitExceeded/no adapter/V6/P23/no pieces/snapshot equality. XML independently parsed with
`build/evidence/1A-1.b-ref-critical-refine/audit_red_report.py`, exit0; no ASan diagnostics. Failed reports retained,
all native processes terminal. Frozen unchanged geometry cpp832c1d1242a97674ba61b968b9f06c6279a49f08492986057fc0394d8df9aeb7,
header ec4adc0fa760b1091c4e8e1f5803f0e1d2f09a8379a66042222a9099677ebb74,
test-only SHA-2564e3d2c72d37fe4a9c5888f9ba91f0b35691609b8543e6a9401b0ad26fa3307a1.
This captured the bug before repair; the red intermediate tree was not committed. Gibbs subsequently implemented
the full three geometry paths; coordinator independently reviewed, validated and committed the frozen final tree.

**Native attempt1 and new join-progress finding.** Frozen full implementation builds0 both; discoveries0/19 cases;
Release/ASan focus both exit42:19 entries/14719 assertions/errors0/skips0, exactly two failures at D100 retained
issuance in inverse-progress/no-progress cases. Other17 cases pass, including the original depth pin. Diagnostic-only
CAPTURE additions retain all assertions; Release diagnostic exit42 confirms legacy `analyseCritical` itself refuses
D100 open0->2 with UnresolvedGeometry/V25/P55/deepest8, before retained publication. Failed reports stay under
`build/evidence/1A-1.b-ref-critical-refine/`; no sanitizer finding or assertion/cap relaxation.

Source and independent Gibbs review identify `joins()`'s both-sides-progress guard: one boundary reaches adjacent
doubles around the true-axis band while the other can still tighten. Initial left bound near pi/4 overlaps the
coarse cusp enclosure and cannot prove the whole span; moving it toward3pi/8 resolves the factor sign. For D100,
z>0.75 over the tightened join neighborhood and exact P(0.75)=2863.76953125>0. Repair only this progress rule:
retain any nonmoving side, require at least one STRICTLY moving side, debit the existing split/examination/depth
costs, then rerun `prove()` over the complete narrowed interval. Never move an endpoint onto the join or accept a
span from a midpoint sign alone. Both sides stagnant still refuse; adaptive depth/work caps still bound every round.
Add focused forward0->2 and reversed3->0 D100 guards for each frozen-side orientation, whole cached near-join
certificate domains and exact/one-less caps. This explicit correctness fix may turn the proven bad legacy refusal
into success; it must not change tolerances, silently accept unresolved spans, move the V6/P23 pin or weaken old tests.
- Finding: CriticalBuilder discards exact raw basis/distance/kernel, canonical z or1-z bracket, actual scalar depth,
  inverse-angle bracket and exact sweep mapping. Its initial x<=asin(x)<=x/sqrt(1-x²) bound does not converge just by
  narrowing x. Complete cusp-versus-crossing order and physical-node accuracy therefore need a separate trusted path.
- Retain private factory-issued immutable provenance behind an out-of-line implementation; never reconstruct raw input
  from frame midpoints. Issue an owned critical-refinement session with opaque occurrence handles and explicit
  `contract(handle,WorkBudget&)`. Reject copied `CriticalOccurrence` records as credentials. Retain frame/seed lifetime,
  quadrant/winding/membership/endpoint identity and separate logical occurrences even when sharing canonical scalar data.
- Bind the issuing WorkBudget's opaque lifetime identity; store only a weak token, never a borrowed budget reference
  or allocation address. WorkBudget remains noncopyable/nonmovable and has no reset API. Lazily create its private
  shared identity token at first issuance, after charging the new retained metadata; session issuance captures the
  immutable original ceilings and admitted canonical history. Before work, reject an expired token or a different
  argument budget with InvalidInput/zero proof work and no mutation. A fresh or destroyed/recreated budget must not
  restart any allowance, even at the same address. Return cumulative CheckWork from the matching budget on accepted
  calls; bad/expired-budget calls return zero new work and the stored last-matching snapshot, explicitly labeled as
  such, never the unrelated argument's counters. Retain session-local admitted history independently of unrelated
  global maxima. Mirror the core terminal-proof,
  sticky-resource and call-local invalid-handle rules; validate identity before touching proof state.
- Share one canonical scalar state across symmetric occurrences. Persist its ACTUAL admitted factory depth, not a
  restarted zero or unrelated global maximum. Contract u=z or1-z with exact positive derivative polynomial interval
  Newton, intersecting prior bracket. Verify outward dyadic endpoint conversion after each step so operand sizes stay
  bounded; do not accumulate exponentially growing rational denominators through iterations.
- Define one bounded compound contraction round per contract call, not a hidden adaptive loop: precheck the shared
  latch, original/per-root absolute depth and the bounded required costs; admit each actual examination/allocation
  immediately before it occurs, recording h+1 only once the first examination's charge succeeds. Then perform at most
  one scalar interval-Newton step, one inverse-angle step (or one sign bisection fallback), and fixed-size image/mapping
  evaluation. Scalar and angle phases share that single admitted level even if one phase stagnates. Persist h+1 and
  spent work on later refusal; failed admission records no entered level. Factory scalar bisections contribute their
  actual entered levels; unrelated region/global depth never substitutes for h. Charge one pair per scalar and inverse
  examination plus explicit item costs for new staged intervals, shared-state records and occurrence snapshots before
  work/allocation. Before activation, enumerate the exact constant item costs and fixed interval-operation bounds in
  the cost model and write exact/one-less tests. No inner loop may hide further adaptive levels or uncharged work.
  Prechecking a potential cost does not consume it; do not debit an inverse examination that was never entered after
  scalar failure. The old global observation maxima cannot reveal this per-canonical history or per-call delta.
  In complement mode use g(u)=-P(1-u), g'(u)=P'(1-u)>0, not a sign-reversed derivative.
- Independently contract alpha=asin(sqrt(u)) using certified sine/cosine interval Newton or sign bisection. Use the
  small-angle domain and qualified derivative/fallback bounds; never divide by a zero-containing interval. An adjacent-
  float complete u bracket may still permit angle progress. Scalar/angle stagnation with unresolved accuracy refuses;
  no rounded asin/atan2 phase becomes proof. All occurrences retain the same unique canonical root under their mapping.
  For an angle midpoint m use the outward residual sin²(m)-U, with U the COMPLETE scalar enclosure, and derivative
  2*sin(A)*cos(A) enclosing the entire prior angle interval A; intersect the Newton result with A. Never replace U by
  mid(U) or use a midpoint-only derivative. A sign-bisection decision must hold for every target in U; unresolved signs
  retain the old enclosure/refuse. After both phases, independently validate the common scalar/angle root and every
  mapped parameter/image; stage all affected symmetric snapshots atomically, never leave them at conflicting histories.
- Use cancellation-free cusp images, c=1-r², z=sin²(eta_c):
  `Q_c=c*(signCos*(1-z)^(3/2)*M-signSin*z^(3/2)/r*J*M)`.
  Exact major-axis minimum-threshold stationary nodes are `+/-M*(1-r²)`; minor-axis maximum-threshold nodes are
  `+/-J*M*(r-1/r)`. Cache local/provider images after exact-scale division; only the interval world-conversion overload
  adds centre outward. The independent 256-decimal-digit
  oracle probe passed48 rotated/quadrant/extreme-scale cases and stationary identities, not production certificates.
- The observation-timing repair itself preserves classifications/spent vertex/pair counts. The independently diagnosed
  join-progress repair above intentionally corrects a formerly refused valid input, with its own counts/cap tests.
  Prospective generation
  checks depthLimit/latch and charges items but does NOT observe depth. Region/join depth is recorded only after its
  examination charge succeeds and immediately before prove; scalar contraction depth only after requisite charges
  succeed and before residual work. Keep WorkBudget::checkDepth semantics, call it at actual entered work.
- Originally static, now native-red verified pin: wholea100/r.5/inward signedDistance-30 with maxVertices6/nonbinding pairs reaches root1+axes4+
  canonical seed1, then records depth1 before refusing two children. No depth1 contraction was examined; currently
  LimitExceeded/vertices6/pairs23/deepest1. Add native regression BEFORE repair, then require deepest0. Also pin queued
  children followed by failed examination, entered deeper work followed by invalidity/ambiguity, and canonical history
  independent of unrelated global maxima. Merely swapping charge/checkDepth order still counts unexamined children.
  The red pin is now captured before seed/token storage. Metadata publication follows successful existing analysis,
  so the original V6/P23 failed-split boundary must remain unchanged. Add retained-path equivalence and separate
  post-success publication-overhead guards; an earlier metadata refusal cannot substitute for the original split.
- Debit examinations, child/certificate creation and publication before work/allocation; return cumulative CheckWork
  on every status, atomically stage geometry while admitted history/work remains spent. Invalid handles consume no
  proof work. Preserve all critical-preparation correctness/resource cases, except the explicitly reviewed and tested
  formerly incorrect one-sided join refusal above; observation timing alone does not change spent counts/decisions.
  The timing repair itself preserves spent counts; new retained-state allocations add explicitly charged items and
  their own exact/one-less tests, not unreported overhead.
- Tests: independent parameter AND image containment, angle contraction with unchanged adjacent scalar endpoints,
  exact stationary nodes, rotated/reversed/wrapped mappings, symmetry without identity merging, close cusp/crossing
  ordering readiness, thin/subnormal refusals, invalid/wrong-session/expired handles, fresh and destroyed/recreated
  budget rejection, terminal invalidity and exact/one-less/pre-spent caps. Test compound-round admission and history
  when scalar or angle stagnates and when publication fails; no work/depth reset through a different occurrence.
  Validate all Release/ASan/broad/full/fast gates and private-state ownership/proof limits, commit/reconcile before isolate.
  Concrete inverse-progress fixture: M=(120,40), r=.5, inward D=125, open arc0->2 radians has exact canonical z=.5
  and one included cusp. It avoids the whole source's pi/4 seam ambiguity while exercising angle contraction after
  the scalar bracket has reached exact/adjacent endpoints. Pre-spent unrelated global depth must not replace its own history.
  Also require nonspecial M=(100,0), r=.5, inward D=30, arc0->2: an adjacent-double complete scalar bracket remains
  unchanged while the certified inverse interval strictly contracts over bounded rounds with independent parameter/
  image containment. The exact z=.5/pi/4 fixture alone cannot qualify general inverse contraction.
  Qualify this phase separation natively rather than deliberately slowing inverse Newton to make a test pass. Also
  use nonspecial D=100 with the same axis/ratio/arc: its complement root is near u=.5 and starts with a substantially
  loose inverse bracket, providing a separate general inverse-progress fixture if D30 scalar/angle reach precision
  together. Retain D30's general inverse/root/image containment coverage; record any fixture qualification correction.

**Executed slice receipt.** Gibbs froze only geometry cpp/header/existing tests; coordinator independently inspected
the full diff and separately recorded completeness and correctness outcomes in
`build/evidence/1A-1.b-ref-critical-refine/review-notes.md`, final commands/counts in `summary.md`. Exact positive
polynomial derivative/complement, full-U/full-angle inverse Newton, dyadic conversions, raw-axis local images, true-pi
mapping, weak lifetime identity, atomic staging, failed history, stage admission and whole-box join domains reviewed.
Two source findings fixed: true inverse domain(0,pi/2), not <=1; finite empty Newton exclusion terminal, unavailable
contraction ambiguous. Native attempt1's D100 refusal repaired by the independently qualified one-sided join rule;
failed/red reports retained, no old assertion/test/cap weakened. New allocation counts are logical metadata records,
not one token per allocator call. Critical functions spend WorkBudget directly, unlike core query/session statistics.

- Final attempt2 both three-target builds exit0,4+4 total jobs<=8; both union discoveries exit0,21 registered cases.
  Release/ASan focus exit0:21 entries/16424 assertions/0 skips/errors/failures. Original V6/P23 pin now deepest0.
- Legacy critical both exit0:11/268911/0, original9/268244 identities retained plus two join guards. Core/session union
  both exit0:51/2837/0, all previous identities retained; no core source change.
- Geometry/offset Release exit0:345/2171050/0; combined ASan exit0:177/1879772/0, both baseline+21. Full visible
  Release exit0:4688 entries/7012085 JUnit tests/74 identical skips, baseline+21/S0+124. No lost testcase/skip identity.
- Exact seven-exclusion fast lanes both exit0:DXF1028/39932/3,DWG1084/34952/4 unchanged skips/identities. Completed
  XML independently parsed exit0. QT_QPA_PLATFORM=offscreen; ASAN_OPTIONS=detect_leaks=0,no LSan claim/no sanitizer
  findings. Every native process terminal; existing CMake/qmake registrations confirmed; diff-check passes.
- Tested and committed SHA-256:cpp77862a021f8a799c691c8d3d7312027d6792f7701b42a32e24bff77133d5eb96,
  header213beff1546fcfd7b00f8dc8f5c6ca567f38590249cb676c9afe96edc84fceeb,
  tests161bb30e04e039261f3c3f6c869d51350b0c43d2e76b7dbb044ced81b076046d. Product commit only those three paths:
  cpp485+/15-,header72+/0-,tests897+/0-,total1454+/15-. No build/generated/plan path in product commit.
- R01/R02/R12/R13 critical sub-contract verified; stationary further tightening and machine-precision stagnation remain
  honest ambiguity. Cached accuracy must be checked before asking for refinement. Full root ordering/reference graph,
  cache validator, persistent entity, file/UI/phase2 requirements remain incomplete. Next: read-only isolation preflight
  against actual committed APIs, then scoped activation at its real checkpoint HEAD; no stale patch reapplication.

#### 1A-1.b-ref-providers. Owned Certified Providers

**Done —9c27e29807b59c70eb7799087eaf4aa0202b0618, 2026-10-15.** Activated at88c15260d3bea755f7818d5cac5412dc6b3ed948;
the following readiness/implementation recipe is historical, not work to repeat. Gibbs independently reviewed actual core/critical
APIs and the coordinator's support/cutter geometry arguments. Coordinator C++17 syntax-only declaration/privacy/
unique-provider-to-core ownership conversion probe passed exit0. Initial probe incorrectly placed a factory body
before its PIMPL definition and failed; moving that body after the complete private implementation fixes the scaffold,
as required in the production cpp. Both logs retained under `build/evidence/1A-1.b-ref-events-isolate/`; this is an
API/access qualification, not a native geometry result. Tracked tree clean at activation; untracked build-asan/ preserved.

- Owner/scope: Gibbs, existing `lc_ellipseoffsetgeometry.h`, adjacent `.cpp`, existing
  `tests/lc_offsetcurve_geometry_tests.cpp` only. Coordinator owns plan/evidence/build/test/review/Git. No engine/core/
  entity/file/UI/build-list changes. Additive provider preparation is independently safe; it must not expose `Ok` as a
  complete event/reference graph. Both refinement predecessors are verified/committed/reconciled.
- Literal API in `LC_EllipseOffsetGeometry`:

```cpp
enum class ReferenceRole { Own, Opposite, StartCutter, EndCutter };
enum class OppositeDisposition { Present, ExcludedCollapsedCircle };
enum class ReferencePreparationReason { None, CollapsedCircle, UnresolvedGeometry };
class ReferenceProviders final : public LC_ParametricCurves {
public:
    ~ReferenceProviders() override; // out-of-line PIMPL; private factory-only construction
    ReferenceProviders(const ReferenceProviders&) = delete;
    ReferenceProviders& operator=(const ReferenceProviders&) = delete;
    const ReferenceFrame& frame() const noexcept;
    std::optional<std::size_t> branchFor(ReferenceRole) const noexcept;
    OppositeDisposition oppositeDisposition() const noexcept;
    bool boundJet(std::size_t segment, double a, double b, LC_CurveJetBounds& out) const;
    // Override segments/branchCount/next/evaluate/bound and BOTH strict proof hooks.
};
struct ReferenceProviderPreparation {
    LC_IntersectionStatus status = LC_IntersectionStatus::InvalidInput;
    ReferencePreparationReason reason = ReferencePreparationReason::None;
    std::unique_ptr<const ReferenceProviders> provider;
    std::unique_ptr<CriticalRefinementSession> ownCritical, oppositeCritical;
    CheckWork work{}; // cumulative shared budget, already directly spent
};
ReferenceProviderPreparation prepareReferenceProviders(const RS_EllipseData&, double, WorkBudget&);
```

- FIRST validate raw request including finite nonzero signed distance and healthy budget. Obtain own/opposite
  `analyseCriticalRetained` sequentially with IDENTICAL validated basis and opposite signed distances, one budget;
  do not re-debit work reports. Own collapse refuses with AmbiguousTopology/CollapsedCircle and empty owners. Omit
  opposite ONLY when own delta>0/own preparation succeeded, opposite trusted result is AmbiguousTopology/CollapsedCircle
  with empty owners and healthy budget, and exact ratio1 plus exact rational axisSquared=D² confirms collapse. No
  rounded hypot or nearby-distance tolerance. All other failures propagate without partial owners. Exact support
  theorem below proves omitted centre point cannot contact own outer offset, so omission is not incomplete enumeration.
- Composite owns unique immutable adapters and immutable cutter state. Critical sessions share factory provenance
  independently and survive moving provider ownership into core. No borrowed dependency, public constructor or copied
  diagnostic-to-provider credential. Same raw basis and abs(D) yield common binary-scaled frame; no midpoint equality
  may establish provenance. `frame()` returns the own frame. Branches are contiguous: own0, optional opposite, then
  optional start/end cutters; role accessors authoritative. Absent opposite only under ExcludedCollapsedCircle,
  `oppositeCritical==nullptr` exactly then. Whole sources omit both cutters. No empty branch/gap/fake injectivity.
- Cutters use u∈[0,1], alpha=REALpi*u, two fixed pieces[0,.5],[.5,1]:
  `Q=E_end+D*(N_end*cos(alpha)+T_out*sin(alpha))`, physical D=abs(delta). Endpoint coefficients come from exact source
  geometry/outward bounds, not rounded jets. T_out is negative directed source unit tangent at start, positive at end;
  N_end is physical outward independent of reversal. Preserve symbolic zero/half/full identities, true-pi derivative
  factors and C1 endpoint limits. All local provider coordinates centre-relative/world-aligned; no centre addition.
  Strictly decreasing N projection cos(pi*u) proves whole-cap injectivity, not sampled derivative sign or a rounded phase.
- Private composite validates caller indices, finite domains, per-part mapping and returned certificates before
  forwarding both hooks. Remap every returned proofDomain/isolation segment reference; never leak local indices into
  the global provider. `next()` remaps self links for whole offsets, open offsets/cutters remain free in this child.
  Every invalid query clears geometry outputs/returns unavailable certificate; hooks are fixed-cost/allocation-free,
  budget-independent after preparation. Same-cap disjoint parameter pieces can exclude only reportable contacts by
  proven cap injectivity. Unsupported cross-provider contacts remain Unavailable, not guessed Unique/Excluded.
  The additive boundJet accessor exposes full outward point/first/second derivative bounds for oracle/downstream
  queries, validates the complete requested segment domain and clears all fields on refusal; it confers no credentials.
- Opposite support proof for D>0: equality `E(t)+D*N(t)=E(s)-D*N(s)` dotted with N(t) forces equality in
  `N(t).E(s)-D*N(t).N(s)<=h(t)+D`. Strict convex support forces E(s)=E(t), while equality also forces opposite normals,
  a contradiction. Apply only factory-qualified SAME exact ellipse/opposite equal distances; then whole original
  request boxes receive AnalyticExclusion, including restricted/reversed arcs. No arbitrary-provider shortcut.
  Outer offset/cap unique construction contact is mathematically qualified by the analogous support argument but its
  hook is deliberately deferred to isolate with full contact classification; no inner-cap blanket exclusion.

**Ordered charges.** Let C=0/2 cutters and S=ownSegments+presentOppositeSegments+2*C, absence contributes zero.
Overflow/max-size checks before arithmetic/allocation; no refunds. After both analyses identify success or the trusted
collapsed-opposite CANDIDATE, charge common V3/P1 before any independent exact-collapse/provenance validation or
allocation. The original trusted critical classification was already charged; never debit it twice. Then:

| Entered stage | Additional vertices/items | Additional pairs | Admission |
|---|---|---|---|
| Common provenance/composite/support metadata | 3 | 1 | Charge before allocation/qualification; disposition embedded, not extra token |
| Each cutter state/endpoint normalization | 1 | 1 | Before its stored state and fixed normalization |
| Each cutter check at0,.5,1 | 0 | 1 each | Before actual fixed jet/bound check, no hidden adaptive loop |
| Global segment-map records | S | S | Charge items before reserve; each validation pair immediately before entered record |

Success adds V=3+C+S,P=1+4*C+S to ALREADY spent own/opposite analyses; failures return empty owners plus current
cumulative work, spent earlier stages stay spent. Depth only records actual entered adaptive work, these fixed checks
do not invent refinement levels. This is a logical-record model, not one token per heap allocation.

**Implementation/exit sequence.** Gibbs implements factories/owned PIMPL, cutters, maps/hooks and nonhidden
`[reference-providers]` tests, then freezes all three paths. Independent256-decimal-digit oracle checks dense/zero-width cutter
points and derivatives, exact endpoint roles, reversal/rotation/wrap/extreme scales/large centres; verify injectivity/
same-cap/same-adapter hook domains, support eligibility, invalid index/NaN/out-of-range clearing, immutable lifetime
through core ownership transfer and separate critical refinement. Pin exact rotated circle M=(3,4),D5 whole/open/
reversed omission, adjacent distances/nonunit ratios/no budget failure misclassified as collapse, own-collapse refusal,
exact/one-less/pre-spent caps and every failed-stage cumulative ledger. Preserve all old tests/caps/engine behavior.
Build all three targets Release/ASan, discover/run new tag and old critical/core union, geometry/offset and combined
ASan, full visible Release and exact-exclusion fast lanes; compare to72eb5f953 and S0. Separate completeness/correctness
review, tested-tree hashes, product commit then plan-only checkpoint. This closes provider preparation only; all six
mandatory reference fixtures, construction/inner-cap/seam/cusp contacts, mixed-root ordering/identity and world-node
accuracy remain required in isolate, never reclassified as optional to close this smaller child.

**Executed slice receipt.** Gibbs froze only geometry cpp/header/existing tests; coordinator independently read the
complete diff, separately recorded completeness/correctness outcomes in `build/evidence/1A-1.b-ref-providers/review-notes.md`,
commands/counts/digests in `summary.md`. Re-derived normal/tangent signs, true-pi derivative/products, cap injectivity,
opposite support and exact-collapse eligibility; reviewed every remapped domain/index/output guard, weak critical
identity/lifetime and all pre-work charges. Removed unused offsets storage; no old body/assertion/cap changed.

- Both three-target builds attempt1 exit0,4+4totaljobs<=8; discoveries both0,16registered cases; focus both0:
  16entries/299294assertions/0skips/errors/failures. No native failed run or expectation/cap relaxation.
- Critical/refinement/depth union both0:30/284668/0,core/session union both0:51/2837/0, all prior identities retained.
- Geometry/offset Release0:361/2470344/0; combined ASan0:193/2179066/0, both baseline+16. Full visible Release0:
  4704entries/7311379JUnit tests/74identical skips,baseline+16/S0+140. No lost testcase/skip identity.
- Both exact-seven-exclusion fast lanes0:DXF1028/39932/3,DWG1084/34952/4 unchanged skips/identities. Completed XML
  independently parsed0. QT_QPA_PLATFORM=offscreen; ASan test runs ASAN_OPTIONS=detect_leaks=0,no LSan claim/no finding.
  Every native process terminal; CMake/qmake registrations confirmed, no full qmake rebuild claim; diff-check0.
- Tested/committed SHA-256:cpp e19be09fd17ca9b358cdc4fe1c981db312af0d2b38828304c88fa36b54d299e7,
  header b4b71a95ce843a7f4f1edf0b87345e6bab842f76aa2c25aca9e6e883a1d1944e,
  tests ca972b499f0b1ef4425e4619a6e121f413ff11b86c0ea1632256b8b9281a7ff6. Exactly three product paths:
  cpp323+/0-,header44+/0-,tests765+/0-,total1132additions; committed digests verified independently.
- R01/R02/R12/R13 provider sub-contract verified only. All six full-reference fixtures, contact/identity/order/node
  accuracy, components/cache/entity/formats/UI/phase2 remain required. Next isolation contact/order/API/cost preflight
  is read-only, conditional on its own evidence, not waiting for completed providers. No stale patch reapplication.

#### 1A-1.b-ref-contacts. Construction Contact Proofs

**Done —92ca5e44508ad42b1873ca8b324e77399028af8b, 2026-10-15.** Activated at0296a8790fb6136e7e8e6e4eca1d9a78a0241175;
the following activation/implementation recipe is historical, not work to repeat. Gibbs independently reviewed the actual source,
mathematics, literal API and costs. Coordinator declaration/ownership probe syntax-only0 uses current Ninja flags;
the diagnostic getter is a future declaration scaffold, not access to an unmodified header. Existing owned-provider
privacy/conversion pattern already compiled/natively verified. Actual-header/getter/private-builder compilation and
all runtime gates were subsequently verified below. Coordinator native neighborhood feasibility probe compile/link/run0 against
the tested library, all six required input families prepare0,16open endpoint neighborhoods have h'>0 within depth8.
The first link omitted lc_release_label.cpp.o and failed; retained real tested object fixed linkage, failed logs kept.
The early attempted run127 was not a geometry result. Probe chooses inherited critical depth as a diagnostic upper
bound; production h' chain starts0 by the contract below. No contact/exhaustive-event implementation is claimed yet.

- Scope/owner: Gibbs, same existing geometry header/cpp/tests only; coordinator roadmap/evidence/build/test/Git.
  Add `prepareReferenceContactProviders(const RS_EllipseData&,double,WorkBudget&) -> ReferenceProviderPreparation`.
  Add `std::optional<LC_Interval> ReferenceProviders::contactProofInterval(ReferenceRole offset,ReferenceRole cutter)
  const noexcept`, diagnostic only: null for legacy factory, invalid/absent roles or whole source. A returned copy
  cannot mint proof state or enter a public adoption constructor. Both offset roles and both ends are covered.
- Share the internal provider builder with a contact-preparation flag, publishing the const-owned PIMPL only after
  all requested contacts succeed. Existing prepareReferenceProviders keeps its costs/hooks/decisions; do not mutate a
  sealed provider, borrow callback dependencies or introduce a public prepared-result-to-provider credential.
  Successful contact preparation remains owned geometry/proofs, NOT a complete event/reference result.
- Outer offset contact with a cap is globally unique: support equality in
  `N(t).(E_end+D*w)<=h(t)+D`, |w|=1, forces E(t)=E_end and w=N_end. Cache the entire offset parameter domain;
  the known source parameter is0/T and capu0. Inner offset uses the corresponding source endpoint/capu1 and a
  bounded complete-neighborhood proof: `G=|Q-E_end|²-D²`, `G'=2*F*h`, `h=E'_t.(E-E_end)`, h(endpoint)=0,
  `h'=|E'_t|²+E''_t.(E-E_end)>0` over the WHOLE stored neighborhood. Constant F sign, including isolated stationary
  zeros, makes G strictly one-sided monotone there. Cap injectivity and exact construction identity then prove the
  only contact; midpoint values, point equality or next/adjacency alone never suffice.
- F proof: nonintermediate regimes supply qualified global sign (stationary zeros isolated, not a collapsed circle).
  For Intermediate first establish a critical-free gap using the COMPLETE factory-issued critical inventory. Seed a
  strictly positive-width neighborhood inside the nearest certified critical gap, not by20 blind halvings from a
  quadrant. If a coarse root enclosure clips0/T, refine its issued handle with the SAME budget before choosing the
  gap; preserve all symmetric updates and absolute canonical history. Never clip a root away, trust a copied public
  occurrence as a credential, or infer absence from a prefix/sample. Initial inner contact exam tries the pure exact
  polynomial endpoint predicate; if unavailable, allow ONE certified interior-point predicate in the same complete
  gap. Continuity transfers that certified sign, not a sampled estimate. Refuse if both unavailable. Successful base
  preparation does not universally imply an available endpoint predicate: quadrant membership can be excluded using
  the OTHER endpoint before the uncertain one is examined. Reuse/extract private exact logic/provenance, preserving
  old classifications/counts; no floating boundOffsetFactor-strict requirement. Point/interval diagnostic arithmetic
  seeds cannot certify F. Critical contraction directly spends WorkBudget; never externally re-debit entered/work.
- New h' subdivision chains start at0 after their own candidate examination charge, not global deepest or restarted
  canonical history. Each retry strictly shrinks toward its source endpoint; no positive-width representable gap or
  no progress gives ambiguity, exhausted original/shared limits gives LimitExceeded, contradiction InvalidInput.
  Observe depth only after admitted examination, not reservation/queued child. Canonical and radial histories are
  distinct; canonical contractions still keep their actual absolute admitted levels and original issuing budget.
- Hook contract: only if the ENTIRE requested offset interval lies inside its cached proof interval may a contact
  hook decide. If both exact singleton parameters are included, return Unique/AnalyticEndpoint/Endpoint with original
  requested global proofDomain and exact isolation. If either singleton is omitted, Excluded/AnalyticExclusion over
  the WHOLE original request. Outside the cached inner neighborhood return Unavailable, even if near the known root;
  additional nearby/off-cache intersections remain for exhaustive strict enumeration. Swap mappings correctly for
  either argument order. Prepared hooks stay allocation-free/fixed-cost, no budget mutations or new proof searches.

**Qualified charges.** C counts construction PAIRS across both present offset roles: normal open4, collapsed-opposite
open2, whole0. J is the number of generated/examined inner radial candidates on success; R extra complete-gap retries
after the initial contact exam. Only returned-base analysis/critical contraction work is already directly spent.

| Entered stage | Extra vertices/items | Extra pairs | Rule |
|---|---|---|---|
| Contact-preparation control record | 1 | 1 | Before extra metadata allocation/eligibility, including whole C0 |
| Each construction-pair record | 1 | 1 | Before record/exam; outer support proof or inner initial gap, at most two fixed exact predicates and one endpoint source-jet bound |
| Each additional gap retry | 0 | 1 | Before complete inventory reinspection; may include the single interior predicate once a usable gap exists; bounded by root refinement/work, no sampling loop |
| Each radial neighborhood candidate | 1 | 1 | Before generation/examination; fixed source h' bound, actual chain depth after pair admission |

Success adds V=1+C+J,P=1+C+R+J, plus directly spent critical contractions. On refusal separately retain actually
admitted common items/pairs, contact records C_v versus entered C_p, generated radial G versus examined X (X<=G),
and entered retries; do NOT assume item admission implies pair admission. No refunds, overflow-safe before storage.
The bounded critical inventory has at most4 source occurrences; fixed exact predicates/jet bounds are not adaptive
inner loops. Source-proof retries/critical contractions are explicit stages, not hidden inside callbacks.

**Implementation and gates.** First share factory builder/pure exact predicates without changing legacy behavior,
then cache outer and inner contact proofs, global hook/getter validation and nonhidden `[reference-contacts]` tests.
Pin own/opposite/end mappings, legacy-null accessors, invalid/out-of-cache boxes, full-box exclusions, stationary
endpoints, unavailable exact endpoint predicate handling, clipped root-gap refinement, reversed/wrapped/rotated cases,
exact/one-less/pre-spent item/pair/depth and every failed-admission ledger. Mandatory native near-threshold pin:
a100,r.5,arc0->2,D=nextafter(25,+inf) has Fpoint[-1.7763568394002509e-15,1.665334536937735e-15] yet a certified
critical-gap neighborhood radius2.8097899007262679e-9 has h'>0; reverse pins end orientation. Never reject just because
floating F straddles0. Preserve a10,r.5,delta-3,arc0->1: its additional start-cap crossing MUST remain outside endpoint
exclusion and be found by the strict query, not blanket-filtered. Qualify all six input families, not a complete
event-set claim. Freeze three files, CMake three-target Release/ASan builds/discovery, new tag plus old provider/
critical/core union, geometry/broadASan/fullRelease/exact-fast lanes, independent reviews/digests, product commit then
plan checkpoint. Full isolate still owns all crossings, mixed-root identity/order, six fixtures and world-node accuracy.

**Executed slice receipt.** Gibbs froze geometry cpp/header/existing tests only; coordinator independently read the
full diff and separately recorded completeness/correctness reviews in `build/evidence/1A-1.b-ref-contacts/review-notes.md`,
commands/counts/digests in `summary.md`. Pure exact-sign extraction preserves prior arithmetic/charges; source/cap
existence, full-domain uniqueness, exact singleton isolation, global swaps, complete inventory/gap/continuity, weak
stationary signs, direct critical history and actual radial admission independently reviewed. Ledger staged locally
then sealed as unique_ptr<const ContactLedger>; no callback mutation/adaptive work, no old assertion/cap changed.

- Both three-target builds attempt1 exit0,4+4jobs<=8; both discoveries0,17registered; focus both0:17/4383/0skips/errors/
  failures. Native strict query preserves the known endpoint AND the independently qualified additional transverse
  start-cap crossing (source/cap parameter and image containment). No product failed native attempt.
- Old provider/critical/core union both0:97entries/586799assertions/0skips, separately audited against disjoint16/30/51
  baseline reports, all identities retained. Geometry/offset Release0:378/2474727/0,combinedASan0:210/2183449/0.
- Full visible Release0:4721entries/7315762JUnit tests/74identical skips,baseline+17/S0+157; both exact-seven-exclusion
  fast lanes0:DXF1028/39932/3,DWG1084/34952/4unchanged skips/identities. Completed XML independently parsed0.
  QT_QPA_PLATFORM=offscreen; ASan tests detect_leaks=0,no LSan claim/no sanitizer findings. All needed processes terminal.
  Existing CMake/qmake registrations confirmed, no full qmake rebuild claim; diff-check0.
- Tested/committed SHA-256:cpp b0e47dc36629eeda2c6eb00c476c5efc33f6d2e86ea90fb490170322843a4851,
  header65f0520272a287020d1d7d7557708a349eb178990398e197a605c73ca7aa2aac,
  tests47f66b5de96a41dfe0821074982214fd5bc00f12616037913ad8efd2bc7ce958. Exactly three product paths:
  cpp220+/38-,header7+/1-,tests732+/0-,total959+/39-,committed digests verified.
- Coverage limit: unavailable-factor native test proves BASE refusal preservation, not successful post-base interior
  fallback. Bounded frozen-private-predicate search compile/link/run0,121506evaluations/54groups/129neighbor grids,
  NO_MATCH/qualifiedArcs0. Not proof of unreachability/coverage; no public backdoor or fallback deletion. Independent
  exact-predicate/continuity source qualification remains valid; carry natural-case/unreachability investigation into
  broader1A-1.c without advertising a native fallback-success test. No required-success disagreement/code bug found.
- Historical contact checkpoint at `92ca5e445`: contact sub-contract only; combined events/order/node accuracy,
  reference components, cache checker/entity/formats/UI/phase2 remained mandatory. Its separate strict-query probe
  passed all six query families (roots 0/0/2/2/6/5) but had image widths up to 28.479; it was not a combined event/
  accuracy result. Isolation is now committed at `dff307668`; `.b-ref-components` is next.

#### 1A-1.b-ref-events-isolate. Exhaustive Event Integration

**Done at product commit `dff307668`.** The additive factory now owns the strict query session/providers and returns
only after complete event ordering, exact same-role identity merging, critical/free-end/seam integration, outward
world-node accuracy within tolerance/16, and publication have succeeded. Failure returns no analysis and preserves
cumulative shared work. The former conditional API/cost/identity/order preflight below is historical entry evidence;
its contracts remain normative and the implementation receipt here records which are proven by native tests.

**Implementation receipt:** Gibbs implemented the geometry header/cpp and existing geometry tests; the coordinator
reviewed, repaired the preparation-limit reason and unresolved-identity diagnostic, expanded role-order/seam-oracle/
wrapped-arc/preparation-limit coverage, then performed the final independent review and test gates. The focused
`[reference-events]` suite is 15 cases/5616 assertions in Release and ASan. `[geometry]` is 154 cases/1464224
assertions in both configurations. Full Release is 3842 cases, 7321304 assertions, 74 unchanged skips. DXF fast is
745 cases/39929 assertions, 4 skips, and DWG fast is 1048 cases/35646 assertions, 36 skips; both pass in Release and
ASan. The sanitizer-linked binaries emit an `__eh_frame` compact-unwind size warning at link but no sanitizer
diagnostic in the executed lanes. The first full-suite terminal run emitted excessive fixture logs and terminated
with signal 10; rerunning with output redirected exited 0 and produced the totals above.
Captured final focused/geometry/full/format logs are retained in
`build/evidence/1A-1.b-ref-events-isolate/` (large verbose logs are gzip-compressed).

Independent rereview found that the tangent stress fixture reaches the strict query's depth cap before `order()`;
the test is named/asserted as strict-query bounded refusal with no partial analysis, not as identity-stage coverage.
Exact construction/free-end and stationary/free-end identity is covered, as are close distinct lens roots, role-major
and strict same-role ordering, all six event families, and the high-precision seam oracle. A successful fixture with
multiple pair records sharing one own occurrence is not separately pinned; keep that explicit coverage limitation
visible and add such a fixture if a natural geometry is found before relying on triple-incidence coverage. This does
not claim an event identity refusal was exercised by the tangent case. The separate native feasibility probe remains
entry evidence only: coarse images from the six strict queries were not used as the published-node proof.
- Scope remains geometry header/cpp/existing geometry tests. Consume the verified `prepareReferenceContactProviders` result
  with own/opposite/cutters in the same frame and one budget. Use role accessors, not hardcoded2/3 cutter indices;
  qualified collapsed opposite is absent with explicit disposition, never an empty/fake branch. Open-end cutters are certified semicircles from source endpoint/normal/outward directed tangent
  and a true-pi sweep, not rounded `atan2` phases; cutters classify visibility and are never retained components.
  Use `E_end+D*(N_end*cos(alpha)+T_out*sin(alpha))`, alpha advancing a REAL pi over its stored domain, from the same
  exact source frame. N/T are mathematical source unit vectors with outward bounds, not midpoint-coefficient geometry;
  numeric jets are seeds. Its N projection is strictly decreasing, proving whole-cap injectivity including endpoints.
- The provider child supplies checked owned composite remapping of segments/branches/`next()` and BOTH strict proof
  hooks/domains. Extend its private contact proofs only with qualified complete-box construction identities. Run one
  strict anchored query with own first and `anchoredBranches=1`: own-self/opposite/cutters required, cutter-cutter not.
- Precompute complete-box construction-end/join certificates. `Unique` requires existence AND exactly one isolated
  contact over its full proof domain; equality/`next()` alone is insufficient. Add intersections to own cusp/stationary
  occurrences, free ends and explicit whole0<->T equivalence, then prove identity/order of the COMPLETE occurrence set.
  Retain pair records at triple incidences. Shared parameters need identity or disjoint certified intervals, not seed
  sorting/proximity. Unsupported tangency/overlap or non-exact cross-partner identity may refuse unless a bounded proof
  is implemented; required construction/seam/source-end/cusp identities and lens crossings cannot be silently dropped.
  For a construction cap, `d(|Q-E_end|²-D²)/dt=2*F*E'_t.dot(E-E_end)` since E'_t.dot(N)=0. Qualified constant F sign
  and directed source-chord monotonicity prove no second full-circle contact over the WHOLE neighborhood, including
  isolated stationary F endpoints. Together with cap injectivity this can certify the known joint endpoint; changing
  F sign/possible extra contacts require full enumeration, never blanket endpoint exclusion.
- Use issued handles to contract root intervals until intersected outward curve images, converted through the frame
  to world space, prove distance to the chosen finite node<=tol/16. Allocate the other tol/16 to reference-span
  approximation so the existing reference-error<=tol/8 contract remains intact; qualify this split against required
  fixtures, never raise tolerance to force success. Include node rounding in the bound, not just half box diagonal.
  Pair roots use the core session; cusp/stationary occurrences use the critical session. Neither raw event carrier
  nor midpoint coincidence is authority. Refine/order the combined complete occurrence set before publication.
- Before inspecting EVERY query/refinement status, debit `totalBoxPairs` and `generatedItems` into the one budget;
  items already include preparation leaves, so never add `generatedLeaves` again. Propagate actual adaptive depths and
  irreversible limits. Composite/cutter descriptors, geometry-level ordering/refinement/publication need their own
  pre-allocation/examination debits. This external debit applies to core `findCertifiedIntersections`/session calls, whose statistics have not
  touched WorkBudget. In contrast, `analyseCriticalRetained` and `CriticalRefinementSession::contract` DIRECTLY spend
  the same WorkBudget: their cumulative `work` and per-call `entered` are diagnostics, NEVER a second debit. Require
  the correct work-source label for accepted critical calls; a last-matching snapshot from a rejected foreign budget
  must not become the caller's current work ledger. Keep these two accounting models explicit in the integration.
  Failure publishes empty staged events plus cumulative work, never partial topology.
- Tests: whole/open/wrapped/reversed, near-critical lens, source-end tangent joins plus additional nearby crossings,
  boundary/seam roots, close distinct roots, cusp-versus-crossing and cross-partner order/identity, proved world node
  accuracy, nonisolated/unsupported refusal and exact/one-less/pre-spent caps. Required fixture disagreements must be
  resolved, not reclassified as optional. Commit/reconcile this complete leaf before components/visibility work.

### 1A-2. Enumerator and class core

**Files.**
- **`rs.h`:** `EntityOffsetCurve, /**< Offset curve (LC_OffsetCurve) */` after `EntityParabola` (:198).
- **`rs_spline.{h,cpp}`:** public static `geometryFingerprint` (§3.2), no behaviour change.
- **New `lc_offsetcurve.{h,cpp}`:** both constructors, `clone`, `rtti`, `definition`, `isDefined`, `isConsistent`,
  `isWhole`, `toPlainSpline`, `normaliseBasis` (§3.3), `isUniformScale`, the tail and flag accessors, the stamp
  (§3.4), the transforms (§3.7) and the undefined state.
  - Implement the private validated constructors and real non-regenerating `adopt` factory in this slice.
  - Declare the refusal/edit-result types used by those APIs now; 1B fills their regeneration behavior, not their
    existence. Include the already implemented geometry result header instead of introducing a circular dependency.
  - The mutators, `create` and `runEngine` are temporary stubs returning `InvalidInput` until 1B. Mark the
    1B dependency in code/tests; no release or final completeness audit may leave these stubs in place.
  - `getRefPoints` returns the control points until 1A-3.
- **`rs_filterdxfrw.cpp` `writeEntity`:** the new enumerator beside `case RS2::EntitySpline:` (:30169); `writeSpline`
  serves it.
- **`rs_insert.cpp`:**
  - new value `InsertTransformCapability::OffsetCurveToSpline` (enum at :88-94);
  - `transformCapability` returns it for the new type (switch at :96-164);
  - `cloneForTransform` (:176-208): `case OffsetCurveToSpline: if (!isUniformScale(parts)) return static_cast<const LC_OffsetCurve&>(source).toPlainSpline(); break;`.
    Uniform factors fall through to `clone()`, as `CircleToEllipse` does.
- **0B row:** `drawingReloadingAs(RS2::EntitySpline)` until 1E-1 updates it. The factory builds the curve from an
  engine result (`LC_CurveOffset::createEntities` on an ellipse, then validated `LC_OffsetCurve::adopt`).
- Increase the recounted baseline drawing-row count by one (27 → 28 at the reviewed base) and retain a compile-time
  row for the new enumerator.
- **`lib/engine/document/container/tests/lc_copy_semantics_tests.cpp`:** a maker in `containerMakers()` (:183-219), so
  the independent-clone checks run on the type.
- **Interim Edit Properties guard (U2)** in `actions/drawing/modify/lc_action_modify_entity.cpp`: before the ShowDialog
  branch selects the entity, sets `InEditing` and clones it (:96-100), return early for `EntityOffsetCurve` with
  `commandMessage(tr("Offset curves cannot be edited in a dialog yet; explode one to edit it as a spline."))` and
  `finish(false)`. 1G-3 replaces this with the real case.
- **Build lists** for the new files.

**Tests** — new `lib/engine/document/entities/tests/lc_offsetcurve_tests.cpp`, `[lc-offsetcurve][class]`:
- **Validated adoption from an engine result** (a = 10, b = 5 and a = 100, b = 50; whole and arcs; both sides; d = 1, 3,
  b²/a ± ε, a lens):
  - rtti, `isDefined`, `isConsistent`, and chords identical to the engine spline's (count and end points);
  - the copy-from-spline constructor keeps layer, pen, flags and a foreign XDATA item.
  - invalid definition/layout/topology inputs cannot produce an editable object through public construction APIs.
- **Normalisation:** a ratio-2 original; a full-turn arc with ratio 2 becomes whole; a full turn from a cut ellipse,
  `(a, a + 2π)`, becomes whole.
- **Transforms:** move; rotate 37° (both overloads); mirror (both axes); uniform scale 2.5 and (−k, k);
  revertDirection; stretch all inside and part inside. Each keeps class, `isDefined` and `isConsistent`, the cache
  passes `checkCache` against the carried definition, and the distance scales.
  - Unequal scale and shear: `!isDefined()`, then behaves as a spline.
  - A cache edited through `getData()` and then moved: still inconsistent (no laundering).
  - Invalid/non-finite transforms and a degenerate mirror axis leave the object unchanged; finite singular scale
    loses editability. Test near-uniform classification bounds without silently treating a meaningful unequal scale as exact.
  - Finite transforms whose results overflow or lose the axis are refused unchanged, with no new stamp. Exercise
    large translations, rotation about a distant centre, extreme scaling and shear; no class engine run is allowed.
  - Mutate type, saved-open data, fit points and file closure flags as well as controls/knots/weights: the complete
    consistency stamp detects semantic changes; a release build refuses edits rather than relying on assertions.
- **Copies:** `clone()` keeps class, definition and grips; `toPlainSpline()` has bit-identical control points, pen and
  layer, and rtti `EntitySpline`.
- **INSERT** of a block {line, curve}: at (1,1), (2,2) (curve kept, distance × 2), (2,1) (plain spline), and mirrored;
  never empty.
- **The interim guard:** ends the action and frees the clone.

### 1A-3. Grips, `moveRef`, snaps

**Files:** `lc_offsetcurve.cpp`, implementing `rebuildGrips`, `getRefPoints`, `moveRef` and the four snap overrides as
§3.6 says.

**Tests** (`[lc-offsetcurve][grips]`):
- **Grip counts:** whole 4, lens 2, arcs as computed; every grip on the cache within 2·tol; none on the seam.
- **Grip order:** grips follow the cache order and reverse with it.
- **`moveRef`** changes nothing on an editable curve, and no control point moves. After unequal scaling or a direct
  cache mutation, it delegates to spline control-point editing and never launders the old definition into consistency.
- **Snaps:**
  - centre = the original's centre;
  - on a whole original, Endpoint returns grips or lens corners, never the seam;
  - on an arc with an open result, Endpoint returns the real ends;
  - out-parameters are always written;
  - an undefined curve snaps as a spline.
  - a closed result from an arc source has no endpoint seam; an inconsistent cache exposes no stale definition grips/snaps.

### 1B. Run limits, refusal enum, `adopt`/`create`, mutators, engine-run counter

**Precondition:** use the S0 build and baseline; retain the #3117 arc behavior measured at S0.

**Files.**
- **`lc_offsetoutputbudget.h`:** `LC_OffsetRunLimits` and `preview()` (§3.1). Optional `LC_CurveOffset::applyRunLimits`.
- **`lc_offsetcurve.{h,cpp}`:** `LC_OffsetCurveRefusal`, `LC_OffsetEditResult`, `runEngine` (with the relaxed atomic
  counter behind `engineRuns()`), `adoptCache`, `adopt`, `create`, `throughParameters`, the four mutators and the
  `setBasis` fast path (§3.8).

**Tests** (`[lc-offsetcurve][edit]`):
- **Round trips:** setDistance, setTowardCentre and setThrough each equal a fresh `create` with the same parameters.
- **Exactly one regeneration** for a meaningful `setThrough` changing both side and distance; no public mutator chain.
  A normal foot outside an arc or on a removed interval refuses the through-point edit if the result cannot contain it.
- **Runtime eligibility/no-ops:** inconsistent/undefined objects refuse safely in Release; exact no-ops do not run the
  engine, clear owned unknown extensions or add undo work. Successful edits clear only owned unknown extensions.
- **Refused edits** leave the object bit-identical (stamp, data, grips) and report the refusal:
  - `d ≤ tol` gives TooSmall;
  - inward past b on a whole ellipse gives NothingLeft;
  - a two-piece arc gives SeveralPieces: use the current engine regression geometry a = 100, b = 5, arc 15°→315°,
    inward d = 4.0 (two visible chains after #3117); verify it still yields two results at S0;
  - tight limits give Engine.
  - Assert the refusal kind, never an engine status or a control-point count.
- **`setBasis`:**
  - a moved original and an original rotated about its centre take the fast path: `engineRuns()` unchanged and the
    cache moved exactly;
  - a reshaped original costs one engine run;
  - an arc with the same axes and sweep but another start angle costs one engine run;
  - a ratio-2 original is normalised.
- **`cacheReversed`** survives an edit: after Revert Direction and a distance edit, the cache still runs reversed.
- **No engine runs** during `clone()`, any transform, or undo/redo of a transform: `engineRuns()` is unchanged.

### 1C-1. Wrap on Offset, kept-plain outcome, Explode, non-uniform scale

**Precondition:** use the S0 build and baseline; retain the #3117 arc behavior measured at S0.

**Files.**
- **`rs_modification.h`:**
  - `RS_OffsetData` (:116-141) gains `bool asOffsetCurve = false;`;
  - `LC_OffsetSourceOutcome` (:200-242) gains `LC_OffsetCurveRefusal keptPlain = None;` and `int keptPlainCount = 0;`.
- **`rs_modification.cpp` `offsetWithOutcome`** (:1430-1775; lambda `offsetOneSource` :1473; copy loop :1525-1564;
  collection :1560-1562). Code shape:

  ```cpp
  // before the copy loop (after side resolution, :1508-1524)
  const RS_EntityContainer* owner = e.getParent();
  const bool editable = data.asOffsetCurve && !forPreviewOnly && owner != nullptr && owner->isDocument();
  const auto* ellipse = editable && e.rtti() == RS2::EntityEllipse ? static_cast<const RS_Ellipse*>(&e) : nullptr;
  const bool towardCentre = ellipse != nullptr && ((side.side == LC_CurveOffsetSide::Left) != ellipse->isReversed());
  // ... the engine run as today (cache, options at :1533-1539, createEntities at :1540-1541), :1546-1559 unchanged ...
  // just before :1560:
  if (ellipse != nullptr) {
      LC_OffsetEditResult why;
      std::unique_ptr<LC_OffsetCurve> curve;
      if (copy.entities.size() == 1) {
          curve = LC_OffsetCurve::adopt(*copy.entities.front(),
                    {ellipse->getData(), magnitude, towardCentre, false, LC_CurveOffset::relativeOffsetTolerance(e)}, &why);
      } else {
          why.refusal = LC_OffsetCurveRefusal::SeveralPieces;
      }
      if (curve) { copy.entities.front() = std::move(curve); }
      else { result.keptPlain = why.refusal; ++result.keptPlainCount; }
  }
  ```

  - The hand-over at `:1735-1748` works unchanged (`RS_Document::addEntity` sets the parent).
  - Wrapping keeps the chords, so `measureAll`, usage and `copiesMade` are unchanged.
  - The preview cache stores results before any wrap, and previews never wrap.
  - `options` does not exist on a cache hit (`:1532`): take the tolerance from the helpers, never from a hoisted
    `options`.
- **`explode` (:2759):** first line in the loop:
  `if (e->rtti() == RS2::EntityOffsetCurve) { ctx += static_cast<const LC_OffsetCurve*>(e)->toPlainSpline().release(); ctx -= e; continue; }`.
  The caller keeps pens and layers (`lc_action_block_explode.cpp:40-44`).
- **`scale`** (:1828-1893), in the non-isotropic branch next to the circle substitution (:1843-1858):
  `else if (rtti == RS2::EntityOffsetCurve && !LC_OffsetCurve::isUniformScale(data.factor)) { entityToScale = …toPlainSpline().release(); createdEntities.append(entityToScale); }`.
  The temporaries are freed at :1884-1886.
- **`main/doc_plugin_interface.cpp` `Plugin_Entity::scale`** (:818-830; it has no substitution today):
  `RS_Entity* ne = (entity->rtti() == RS2::EntityOffsetCurve && !LC_OffsetCurve::isUniformScale(f)) ? …toPlainSpline().release() : entity->clone();`.
  Its `!ok` branch (:822-826) can never run, because `addToUndo` always returns true; say so in a comment.

**Tests** (`[lc-offsetcurve][modification]`; the source ellipse lives in an `RS_Graphic`; copy the file-local fixtures
`inwardOffset`, `BatchGuard`, `towards`, `outcomeFor` from `rs_modification_offset_tests.cpp:57-71`, `:220-239`):
- **Option off:** results identical to today, and the existing pins stay green (`lc_offsetmatrixtests.cpp`, the
  offset action tests).
- **Option on:**
  - a whole ellipse and one-piece arcs give `EntityOffsetCurve`;
  - copies at k·d, keep originals on and off;
  - the preview stays `EntitySpline`;
  - a two-piece arc (the 1B case) gives two plain splines and `keptPlain == SeveralPieces`.
- **Explode** gives one spline, then a second Explode gives lines.
- **Scale (2,1)** through `RS_Modification::scale` gives a plain spline in the document.
- **Plugin path** gives a plain spline. Fixture: `Drawing` (`lc_layerlist_tests.cpp:63-75`) plus
  `Doc_plugin_interface plugin(&m_context, nullptr)`; `librecad_tests` only.

### 1C-2. Offset again

**Files:** `rs_modification.cpp`, plus a helper `LC_OffsetCurveAgain` (in `lc_offsetcurve.{h,cpp}`, or a new
`lib/modification/lc_offsetcurveagain.{h,cpp}`).

```cpp
// constructed once per source when editable && e.rtti() == RS2::EntityOffsetCurve
LC_OffsetCurveAgain::LC_OffsetCurveAgain(const LC_OffsetCurve& c, LC_CurveOffsetSide side) {
  if (!c.isDefined() || !c.isConsistent()) return;                  // inactive: today's path
  m_def = c.definition();
  const bool anticlockwise = m_def.basis.reversed == m_def.cacheReversed;   // the side is resolved on the cache
  m_sigma2 = ((side == LC_CurveOffsetSide::Left) == anticlockwise) ? -1.0 : 1.0;   // -1 = towards the centre
  m_s1 = m_def.towardCentre ? -m_def.distance : m_def.distance;
  RS_Ellipse basis{nullptr, m_def.basis};  m_whole = !basis.isEllipticArc();
  const double a = m_def.basis.majorP.magnitude(), b = a * m_def.basis.ratio;
  const double tol1 = LC_CurveOffset::toleranceFor(basis, m_def.distance, m_def.relTolerance);
  m_firstUntrimmed = !m_def.towardCentre ||
      (m_whole ? m_def.distance < b * b / a
               : m_def.distance < minRadiusOfCurvature(m_def.basis)
                 && endsOnExactOffset(c.getData(), m_def.basis, m_s1, 2 * tol1));  // unordered pair of ends
  m_active = true;
}
// bool rebase(double magnitude, const LC_OffsetRunLimits& limits, const LC_OffsetSourceBudget& left,
//             const RS_Entity& source, LC_CurveOffsetMaterializationResult& out):
//   s = m_s1 + m_sigma2 * magnitude;   basis = RS_Ellipse{nullptr, m_def.basis} with source's pen and layer
//   (a) |s| > toleranceFor(basis, |s|, relativeOffsetTolerance(basis))
//   (b) whole: m_firstUntrimmed || (m_def.towardCentre && m_sigma2 < 0)    // erosions by discs compose
//       arc:   m_firstUntrimmed && (s > 0 || -s < minRadiusOfCurvature(m_def.basis))
//   run createEntities(basis, side = (s < 0) != basis.isReversed() ? Left : Right, |s|), options clamped by limits, budget `left`
//   (c) Ok, one entity, adopt() passes; for an arc with s < 0, the result's ends lie on O_s(eta1), O_s(eta2) within 2*tol
//   out = that run (entities replaced by the adopted LC_OffsetCurve, its usage kept); any failure returns false.
```

- **In the copy loop,** when the helper is active, try `again.rebase(...)` first; if it returns false, run today's
  engine call on the cache. The series therefore never ends where today's path succeeds.
- **Rebase only when all hold:** `data.asOffsetCurve`, not a preview, the source is top-level, and the curve is
  defined and consistent. Otherwise use today's path, so the option off gives exactly today's result.
- **Offset curves never nest.** A copy that falls back is a plain spline, reported as kept plain.
- **Previews** show the cache's offset, within tolerance of what the commit makes.
- **Bounded attempts:** at most one rebase engine attempt and one cache fallback attempt per copy; structural refusal
  performs no rebase engine call. Both use explicit work limits and the remaining output budget. Charge only final
  committed output to output usage, record attempted work separately, and do not retry indefinitely or silently claim
  that the entity's regeneration counter counts tool/helper calls.
- **Keep Originals off** deletes the source curve, as today.

**Tests** (`[lc-offsetcurve][modification]`):
- **Admitted chains** (geometries of `dep.cpp:445-451`): a whole ellipse with a = 100, ratio 0.5, rotated 30°, centre
  (1234.5, −987.25); arcs 30°+170° and 100°+250°. Each is rebased, `LC_OffsetCurve`, and within tolerance of the
  exact offset at s.
- **Sum zero:** a curve at +10, offset inward by 5 with four copies, gives four copies. s = +5, −5 and −10 are rebased;
  the copy at s = 0 must use the cache fallback, remain plain and be reported. This is required fallback coverage,
  independent of whether an old numerical failure band still exists.
- **Near-critical whole-ellipse composition** (`librecad_tests` only): a = 100, ratio 0.6, first offset outward 10,
  then offset inward 46.036 (net signed distance s = −36.036). The #3117 engine now produces a valid closed result for
  this case; expect successful rebase/adoption as one `LC_OffsetCurve`, geometrically within the requested tolerance.
  Additional failure-band characterization may inform the roadmap, but cannot replace or omit the required deterministic
  zero-sum fallback test. Qualify the near-critical success against the strengthened validator before adopting it.
- **Revert Direction,** then offset again towards the centre: the copies lie inside.
- **An undefined curve** is offset as a spline (plain results).

### 1D. Remaining type sites

Add `RS2::EntityOffsetCurve` beside the existing `RS2::EntitySpline` at each applicable site below. The spline's arm
then serves the subclass. This table is a seed list from 7c4a93fa8, not a complete current census: at S0 search all
type switches, RTTI comparisons, clone/transform tables and consumers, then record the actual count and disposition.

| Site | HEAD | Without it |
|---|---|---|
| `lib/information/rs_information.cpp`, neighbour-chord test | 450 | 60 of 60 phantom intersections between its own chords; phantom intersection snaps |
| `actions/drawing/modify/lc_action_modify_offset.cpp`, type list | 44-45 (append at the end; a test pins the spline types after Polyline, `lc_action_modify_offset_tests.cpp:256-258`) | Offset does not admit the curve |
| `actions/drawing/draw/line/lc_action_draw_line_parallel.cpp`, chord → parent | 119-120 | a parallel of one chord |
| `actions/drawing/draw/line/lc_action_draw_line_parallel_through.cpp` | 40 and 48 | the same; not accepted |
| `lib/creation/rs_creation.cpp`, `createParallel` | 166 | nothing created |
| `lib/filters/rs_filterdxfrw.cpp`, hatch-loop flattening and spline edge | 32105, 32173 | the boundary is written as chords, or the edge is dropped |
| `lib/engine/overlays/preview/rs_preview.cpp`, budget exemption | 90 | move previews of copies 2+ drawn as boxes |
| `lib/actions/rs_snapper.cpp`, `isContainerTarget` | 75 | an own-type catch at ResolveAll finds nothing |
| `lib/generators/makercamsvg/lc_makercamsvg.cpp` | 229 | not exported to MakerCAM SVG |
| `main/doc_plugin_interface.cpp`, `Plugin_Entity::getData` | 364 | plugins see `DPI::UNKNOWN` (with it they see `SPLINE`) |
| `actions/drawing/selection/lc_action_select_intersected.cpp` | 176 | accepted as cutter, intersects nothing |
| `lib/engine/document/container/rs_entitycontainer.cpp`, `updateSplines` | 754 | harmless today; for parity |
| `actions/drawing/draw/spline/lc_action_spline_modify_explode.cpp`, type list and case | 39-40, 198 | Draw > Spline > Explode refuses it (D14) |
| `actions/drawing/modify/lc_action_modify_trim.cpp`, landed spline-chord → parent mapping | current | a chord of the curve, instead of the curve, would be the limit |

**Deliberately not added:**
- The four control-point tools (`lc_action_spline_add_point.cpp:29`, `..._append_point.cpp:32`,
  `..._remove_between.cpp:30`, `..._remove_points.cpp:30`): they rebuild a plain spline and would silently convert
  the curve.
- `lc_action_modify_round.cpp:49` and `doc_plugin_interface.cpp:662`: they need nothing.
- `rs_filterjww.cpp:1639`: JWW export writes nothing (`DL_Jww::out()` returns NULL).
- `rs_filterdxf.cpp:1374`: not built.
- The UI sites (type names, providers, dialogs, Quick Info, menus, Select Window, menu activator) get their own cases
  in 1G, not the spline's. `lc_action_modify_entity.cpp:110` is 1A-2's guard, then 1G-3's case.

**Tests** (`[lc-offsetcurve][sites]`), from the parity rows of `prototypes/type_shape_operation_matrix/tsom_probe.cpp`:
- intersections of the curve with a line and with a second curve (2 each), with no neighbour-chord hits;
- the intersection snap is not at a chord joint;
- Trim with the curve as limit: one-sided, and the curve is unchanged;
- Break/Divide of a crossing line;
- Parallel and Parallel Through make one spline;
- a preview of three move copies has no boxes;
- MakerCAM export contains a path (pattern `spline_offset_representation_tests.cpp:924-975`);
- the plugin type is SPLINE;
- a hatch bounded by a closed curve: a valid loop, area within tolerance of πab + L·d + πd² (outward), and the DXF
  hatch edge is one SPLINE edge (`makeFullEllipseHatch`, `rs_hatch_tests.cpp:262`).

Test that single Trim resolves an offset curve's chord to its parent as a limit. Test that Trim Two with a chord of the
curve as the first pick is refused and leaves geometry and undo state unchanged; #3113's generic ownership guard is
already the prerequisite, not work in this feature.

### 1E-1. Shared schema, DXF XDATA and DWG EED read/write

**Files.**
- **New `lib/filters/lc_offsetcurvexdata.{h,cpp}`** (precedent `lc_parabolaspline.{h,cpp}`, `CMakeLists.txt:1127-1128`,
  `src.pro:606`, `:1068`): the optional-result `forExport`, `parseAndStrip`, raw-carrier preflight and import adoption (§3.10).
  - It has its own small readers, because the file-local readers of `rs_filterdxfrw.cpp` (`:332-422`) are not visible.
  - Reference implementation: `prototypes/redteam_integrity_work/rt_offsetcurve.cpp` (`makeSubList`,
    `appendForExport`, `parseAndStrip`, `adoptOnImport`).
- **`rs_filterdxfrw.cpp` `writeSpline`** (:30729-30822). Before dispatch to either file writer:

  ```cpp
    getEntityAttributes(&sp, s);
    auto extData = LC_OffsetCurveXData::forExport(*s, std::move(sp.extData));
    if (!extData) { m_writeFailed = true; return; }
    sp.extData = std::move(*extData);
    if (m_dwgW) {
        // existing native DWG SPLINE/EED writer
    } else {
        // existing DXF SPLINE/XDATA writer
    }
  ```
  Apply the same preparation to the earlier R12 POLYLINE branch with `includeDefinition = false` before writing it;
  returning early must not leave a stale schema-1 marker on downgraded geometry.
  The new type's complete geometry/metadata preflight runs before the existing insufficient-control early return.
  Invalid or unwritable offset-curve geometry latches `m_writeFailed`; never inherit the ordinary spline's log-and-drop
  behavior. R12 must establish its promised approximation bound for this type or fail, not silently use unbounded
  display chords. Existing unmarked spline export policy need not change. Verify the current codec transaction and
  `finalizeDxfWrite`/`finalizeDwgWrite` path with a late filter failure; no duplicate staging implementation is planned.
- **`rs_filterdxfrw.cpp` `addSpline`** (:8034-8139). Every exit copies the record's own list through
  `setEntityAttributes` (:32993-32995): conic (:8024), degree 2 (:8050), degree above 3 (:8083), main path (:8109),
  fit points (:8121), and `addHelix` → `addSpline` (:8151). So strip a copy of the record once, at the top:

  ```cpp
    LC_OffsetCurveXData::Parsed oc = LC_OffsetCurveXData::parseAndStrip(data->extData);
    std::optional<DRW_Spline> stripped;
    if (oc.changed) { stripped.emplace(*data); stripped->extData = std::move(oc.remaining); data = &*stripped; }
    ...
    // Capture from the raw DRW carrier before any XY projection, knot rounding or geometry repair.
    auto rawCache = (data->eType == DRW::SPLINE && oc.status == LC_OffsetCurveXData::Status::Schema1)
        ? LC_OffsetCurveXData::rawCacheForAdoption(*data) : std::nullopt;
    // main path: the eligible marked candidate keeps validated raw controls/knots and unit weights.
    std::unique_ptr<RS_Spline> spline = rawCache
        ? makeRawMarkedSpline(m_currentContainer, *rawCache)
        : makeSplineFromDrw(m_currentContainer, *data, data->weightlist);
    setEntityAttributes(spline.get(), data);
    if (rawCache)
        if (auto curve = LC_OffsetCurveXData::adopt(*spline, oc)) spline = std::move(curve);   // copy-from-spline
    m_currentContainer->addEntity(spline.release());
  ```

  `DRW_Spline` is copyable; a `DRW_Helix` is sliced, which is fine because no exit reads beyond `DRW_Spline`. Confirm
  real DXF and DWG readers both invoke this callback with decoded XDATA/EED; no format-specific adoption bypass.
  The pseudocode's `rawCacheForAdoption` returns a bounded `optional<RS_SplineData>` under R14; its failure records
  a diagnostic and can never be rescued by `makeSplineFromDrw` repairs. `makeRawMarkedSpline` is a narrowly scoped
  constructor/update helper, not a second general importer. Keep actual `normalVec` zero-sentinel behavior from
  §3.10 in its eligibility tests. Capture raw eligibility before any early import branch; only the main SPLINE
  control path may adopt. Other branches retain plain geometry and stripped known-owned metadata.
- **Codec trace:** inspect the ordinary SPLINE encode/parse and common EED path in `drw_entities.cpp`,
  `libdxfrw.cpp`, `libdwgr.cpp` and the DWG reader/writer. Prove that the complete schema survives each codec before
  relying on document adoption. Add focused codec fixes/tests only for losses demonstrated by those checks.
- **0B row:** `drawing(EntityOffsetCurve)`, reloading as itself in both DXF and DWG. Neither round-trip bit may be
  listed as a known gap or `notRun` for this new type.
- Keep filter changes localized to spline serialization and parsing, using existing save/finalization mechanisms.
  APPID registration is already handled by #3119;
  inspect current XDATA-tail and import-finalization ownership before editing those paths.

**Tests** — new `lib/filters/tests/lc_offsetcurve_file_tests.cpp`, built on `Bench`; separate cases tagged
`[lc-offsetcurve][files][dxf]` and `[lc-offsetcurve][files][dwg]` share the fixture/oracles:
- **Writer smoke:** independently save whole/arc curves in DXF R2018 ASCII and native DWG R2018, in model space and
  a block definition. Inspect actual output payloads and verify geometry/definition; save success alone is not a
  writer pass. 1E-3 extends writer coverage to every application version.
- **Reader smoke:** load checked-in DXF and DWG input fixtures and adopt whole/arc definitions in model space and a
  block definition without invoking the writer in the same test. Verify geometry/definition and zero regeneration;
  1E-2 records fixture provenance and 1E-3 extends reader coverage to every application version.
- **Read/write payload inspection:** capture `DRW_Spline` from each actual output file through a low-level interface;
  verify degree 1 and degree 3 separately, degree-appropriate control/knot counts and multiplicities, unit weights,
  closed/periodic semantics and the typed metadata fields. Verify
  the `LibreCad` application reference is registered/resolvable in the output.
- **Read without regeneration:** adopting a saved cache leaves `engineRuns()` unchanged. Writing also preserves the
  current cache rather than rerunning the engine and changing its geometry.
- **Raw input preflight:** marked 3D controls, incompatible normals, null/mismatched arrays, missing/synthesized/
  rounding-collapsed knots, partial weights and fit-only/repair cases cannot become editable. Valid rescaled knot
  vectors and unspecified DWG normals pass without shape-changing conversion. Inspect raw codec callbacks as well
  as the projected plain fallback; test both ASCII/binary DXF and native DWG where the field is representable.
- **Source-independent reload:** delete the source ellipse before export; either format restores the editable curve.
- **R12:** one POLYLINE, no owned schema-1 marker in the file, and reimported geometry within export tolerance.
- **APPID registration:** exactly one case-insensitive `LibreCad` record matching the emitted group, while preserving
  one record per other used XDATA app (upstream #3119 regression; DXF table inspection, DWG EED reference resolution).
- **Registration:**
  - in `librecad_tests` after `spline_offset_representation_tests.cpp`;
  - in `LIBRECAD_DXF_FAST_TEST_SOURCES` near the existing filter/XDATA coverage; choose the insertion point from the
    current CMake list;
  - not in the DWG list: the DXF lane defines `DWGSUPPORT`.

### 1E-2. DXF/DWG metadata safety, fallback, fixtures, copy and paste

Aggregate of `1E-2.safety`, `.fixtures` and `.copy` (§0.6), committed/checkpointed in that order. The following tests
belong to those leaves, not a final uncommitted catch-all. Required safety regressions accompany any production fix.

**Tests** (same file unless stated). Run each applicable metadata case through both DXF and DWG, including saving
the imported result again. Check both entity geometry and owned/foreign metadata:

- **Group merging:**
  - a pre-existing `LibreCad` group with other items, placed before or after another application's group;
  - two stored `LibreCad` groups, one spelled `LIBRECAD`, alongside a distinct padded `" LibreCad "` foreign group;
  - nested marker-like foreign items, a malformed known schema before a valid one, an ill-typed schema discriminator,
    unknown schema 2 with nested markers, and conflicting flag bit 3.

  Expect one group in the file, the foreign data kept, and the header spelling kept.
- **Foreign binary XDATA across formats:** include group 1004 in a non-`LibreCad` application group and a 1070 value
  with its full unsigned 16-bit pattern; compare 1070 as `uint16_t`, not a signed integer. DXF → DWG → DXF and
  DWG → DXF → DWG must preserve the supported payload, protecting #3116 while checking the metadata merge.
- **Lost definition, then save as DXF and DWG:** read a curve, then (a) explode it, (b) scale it (2,1) in place,
  (c) make its cache inconsistent. Save/open each result as a plain spline with the resulting geometry; remove stale
  schema-1 metadata and empty owned groups while retaining supported foreign metadata.
- **Unbalanced owned braces:** an `LC_OFFSET_CURVE` with no closing brace loads as a plain spline, and both subsequent
  saves succeed after the owned malformed sub-list is stripped. If unrelated stored data prevents safe serialization,
  export reports failure rather than silently omitting a valid curve's definition.
  Distinguish this known-schema repair from an unbalanced unknown-schema list: keep unknown raw data, remain plain,
  and report a subsequent save failure if the codec cannot represent it. Do not silently strip unknown versions.
- **Failed save transaction:** force unsafe owned metadata or a propagated codec failure while saving over an existing
  DXF/DWG file; return failure, retain the previous target, and release temporary carriers. A rejected save must not
  leave a partially written drawing that appears successful.
  Include a late `writeSpline` filter-only refusal after a valid entity was already emitted, invalid-control and R12
  approximation refusals, an initially absent target, and a successful retry. Compare previous target bytes exactly,
  directory entries/stage cleanup and live entity/metadata/history state. Reuse the existing transactional/finalizer
  mechanisms; do not merely assert a false return after the codec has already published a bad replacement.
- **Unknown trailing items** in a schema-1 sub-list: allowed codes are adopted and written back; 1004, 1003 and 1005
  are dropped. An oversized/deep/ill-typed unknown owned tail is discarded with diagnostics while the valid base
  definition survives; test 64-item and 4096-encoded-byte boundaries independently, including Unicode and unbalanced braces.
- **Owned future data:** no-edit round trips and clones preserve safe tail/flag bits; successful move/rotate/mirror/
  scale/reverse and distance/side/basis changes clear them under D18. Refusals/no-ops preserve them. Foreign groups
  remain intact and no mutated shared variant affects another clone.
- **Axis/precision/schema checks:** read/write the 1012 major-axis vector at large centres and small valid axes;
  reject the older private prototype's second-1011 layout for adoption, malformed point types, unsupported Z values
  and nonrepresentable bounds without dropping the visible spline. Verify exact field-code order in actual codec output.
- **Hostile values:**
  - factor 0.5 and 1e9; angle2 1e300; two sub-lists;
  - distance negative, zero or 1e300; ratio 0, negative or 1e6; NaN coordinates;
  - schema 2 (stays raw); the group on an unrelated cubic.

  No crash or hang; plain spline or raw XDATA, as §3.10 says.
- **A hatch** bounded by an offset curve saves and reloads (the edge as SPLINE).
- **Checked-in fixtures** (cross-compiler and reader check): a version-indexed `offset_curves_<version>.<ext>` set
  under `lib/filters/tests/testdata/`, with a README, reproducible independent generator and provenance. For every
  schema-capable application reader row in §3.10, include degree-1 and degree-3 carriers (in one or paired files) with
  whole/arc coverage, both sides, representative scales and a block-owned case; include input-only DWG R13/R14
  fixtures and an R12 geometry-only input.
  Maintain two distinct fixture roles: (1) inputs authored by a tool/source independent of LibreCAD's writer under
  test, for exact-version DXF/DWG reader coverage; (2) LibreCAD-generated output for cross-compiler/write-payload
  inspection. Do not label a self-generated file as independent reader evidence. A fast-lane test asserts every input
  cache degree and stored definition. Include independently sourced binary DXF input fixtures at R14 and R2018 with
  both cache degrees and import them as editable; separately generate binary DXF output through the existing low-level
  codec and inspect its decoded payload. Record producer/tool version, exact file version, origin, hash, cache degree
  and expected definition independently of the parser under test. If an independent producer is unavailable for a
  required reader row/degree, record the exact gap and keep that row incomplete rather than substituting a same-run
  round trip.
- **Copy and paste** (`librecad_tests`):
  - paste into the same drawing;
  - paste into a drawing with other units (distance × f);
  - library insert (template `lc_copy_identity_tests.cpp:724`);
  - Save Block As.
- **Manual (not in CI): ezdxf.** Use an available installation and record its version; the old private script is an
  optional example. Audit the generated fixtures and exercise external move/rotate/uniform scale. Those preserve the
  definition through typed XDATA when supported by the tool. External reflection/unequal scale without updating the
  private flags/definition must fail adoption and leave plain geometry; LibreCAD's own supported mirror path updates
  the definition and remains editable. Record actual results and unavailable capabilities in release evidence.

### 1E-3. Version matrix, conversion and edit/save/reopen acceptance

Aggregate of `1E-3.matrix` and `.conversion` (§0.6). Commit/reconcile the matrix before conversion; the parent remains
partial until both leaves and required native validation pass.

**Files:** extend `lc_offsetcurve_file_tests.cpp` and its test-support helpers. Use an explicit `FormatCase` table
containing `RS2::FormatType`, expected `DRW::Version`, extension and whether the format preserves the definition.
Guard only the DWG entries/cases with `DWGSUPPORT`; use the exact enum names from §3.10, including `FormatDWG` for
R2000 and `FormatDXFRW` for R2007.

**Required checks:**

- For each schema-capable reader row, load that exact version's checked-in independently authored fixture with both
  degree-1 and degree-3 carriers. The R12 row loads an independent POLYLINE geometry-only fixture. LibreCAD writer
  output may exercise codec integration or second-generation persistence, but never substitutes for the independent
  fixture in the reader receipt. Separately test the legacy DWG R13/R14 input-only fixtures at both degrees; they do
  not add writer rows to the 14-row application matrix.
- For every schema-capable row of §3.10, run writer and reader assertions as distinct cases, then run write → read →
  write → read with degree-1 and degree-3 caches, whole and arc originals, both offset sides, translated/rotated bases,
  reversed cache direction, and an entity inside a block. Verify the requested file
  version via DXF `$ACADVER` or the DWG file signature; a file saved under the wrong version is a failure.
- Keep a version-indexed receipt table showing writer payload inspection, reader adoption/definition recovery,
  second-generation reload and expected downgrade for each of the 14 application rows; add two reader-only receipt
  rows for legacy DWG R13/R14. Every cell has a nonzero
  selected-test count; a combined round-trip count or a target name does not prove that a direction ran.
- At each generation, assert `EntityOffsetCurve`, the normalized definition (including signed-side semantics and
  saved tolerance), a consistent cache passing §3.5, target-representable layer/pen/visibility, owner context and
  supported foreign XDATA/EED. Common fixtures use indexed colors, default lineweight and other attributes supported
  by all matrix versions; newer-only attributes follow the codec's documented downgrade rules. Use scale-aware
  numeric comparisons; DXF's 16-digit text rounding and DWG encoding must satisfy the
  geometric tolerance. Do not compare remapped handles or text-rounded control points bit for bit.
- Cover both cache degrees in both conversion directions without requiring a full Cartesian geometry product:
  DXF R2018 → each supported DWG version → DXF R2018, and DWG R2018 → each schema-capable DXF version → DWG R2018.
  Add a repeated alternating DXF/DWG chain to catch duplicate schema blocks and accumulated definition drift.
- Open a saved curve, change its distance/side/basis through supported mutators, save to the other format, and reopen.
  The newly edited definition and geometry must be present, with no stale original sub-list. Verify successful edits
  still work after reload and that refused edits leave the saved curve unchanged.
- Exercise R12 downgrade from both a newly created curve and a DWG-imported curve. Reopen as geometry-only POLYLINE;
  there is no marker that could incorrectly restore editability. Ordinary unmarked SPLINEs and unknown-schema inputs
  retain their documented plain/raw behavior in each supported format.
- Exercise binary DXF codec writing and reading at R14 and R2018 using a `DRW_Spline` with the shared serialized
  schema. Inspect actual encoded output and decode it; separately import independently sourced binary input fixtures
  into the document as editable curves. A same-run encoded file may test codec integration but cannot replace the
  independent reader fixture.
- Run the new `[files][dxf]`, `[files][dwg]` and conversion cases in `librecad_tests` and the DWG-enabled DXF fast lane.
  Require a nonzero DWG case count; a target named "DXF" must not hide skipped DWG coverage. Compile the new shared
  schema module once without `DWGSUPPORT`, using its compile command with the define removed for an isolated check.
  Standard CMake targets currently define the macro unconditionally.
- Audit generated DXF with ezdxf and inspect representative DWG output with an available independent reader such as
  LibreDWG, ACadSharp or ODA. Verify the ordinary spline and registered metadata, record the verifier/version and
  results, and state unavailable tools explicitly; self-reader acceptance is not evidence of external verification.

**Done when:** the complete native matrix passes with no silent entity drops, no new-type known gaps, and no semantic
fallback for valid supported files. Record separate reader and writer case counts for each application version and
for the binary DXF codec, both conversion directions, and any external-reader results in step status/release evidence.
The public ASCII DXF and native DWG write paths, both import paths, independently sourced reader fixtures, and binary
DXF codec read/write are covered; same-run self-round trips alone do not close any reader obligation.

### 1G-1. Type name and match descriptor, Quick Info, context menus, Select Window, menu activator

1. **Type name + match descriptor, in one commit.** Quick Selection asserts a descriptor exists for every row
   (`lc_dlgquickselection.cpp:221-228`).
   - A row `{tr("Offset Curve"), RS2::EntityOffsetCurve}` in `ui/components/utils/lc_entitymetauiutils.cpp:77-105`
     (template :85).
   - New `lib/selection/metaentity/entities/lc_matchdescriptor_offsetcurve.{h,cpp}` (template
     `lc_matchdescriptor_spline.cpp:29-58`; richer `lc_matchdescriptor_hyperbola.cpp`), with properties: distance,
     side, the original's centre x/y, major radius, minor radius, rotation, start/end angle, length.
   - Include and `init` in `lc_entitymatchdescriptorsregistry.cpp` (:26-50, :87).
2. **Quick Info:** two cases in `ui/dock_widgets/entity_info/lc_quickinfoentitydata.cpp` (`getEntityDescription`
   :133-274, `processEntity` :279-424), two new functions declared in the header, templates the ellipse pair
   (:712-791) and the spline pair (:1259-1288). Text "Offset curve" with distance, side, the original's centre, radii
   and angles, and length. **No control points.**
3. **Context menu** (`ui/main/init/lc_menufactory_graphicview.cpp`, `createGVMenuEntitySpecific` :563-1126):
   - a `case RS2::EntityOffsetCurve:` with `"ModifyOffset"`, `"BlocksExplode"`, `"ModifyRevertDirection"`, then
     `createGVMenuModifyGeneral(...)` (template: the spline case :814-829);
   - add `"ModifyOffset"` to the Ellipse menu (:878-907), and fix the missing comma after `"ModifyCut"` there.
4. **Select Window** (`ui/action_options/selection/lc_selectwindowoptions.cpp`): `EntityOffsetCurve` with the spline
   types in `onTypeToggled` (:198-202) and in the case list (:283-287).
5. **Menu activator,** code `"OC"`: `ui/components/creators/lc_menuactivator.cpp` (`parseEntityType` :151-222,
   `getEntityTypeStr` :279-350, `getShortcutView` :458-555), and one row in
   `ui/dialogs/creators/lc_dlg_menu_assigner.cpp:106-129`.

**Tests** (`[lc-offsetcurve][ui]`, `librecad_tests`):
- **Descriptor coverage:** every row of the name table except `EntityBlock` (never a drawing entity) has a match
  descriptor. The registry keeps its first `LC_ActionContext` forever (`lc_entitymatchdescriptorsregistry.cpp:63-70`),
  so pass a `static LC_ActionContext`.
- **Quick Info** text through `setDocumentAndView` and `setOptions`: it contains the distance and no control point.
- **Select Window** with "Spline" selects the curve.

### 1G-2. Refusal texts, busy cursor, property provider, refusal-aware property sheet

**Files.**
- **N5:** the three public statics on `LC_ActionModifyOffset` (§3.1).
  Cover every refusal value, including `Inconsistent`, and geometric validation/limit details carried by `checkReason`.
- **Busy cursor:** an RAII guard, e.g. `ui/components/utils/lc_busycursorguard.h`. It calls
  `QGuiApplication::setOverrideCursor(Qt::WaitCursor)` and `restoreOverrideCursor()`, and does nothing unless
  `qobject_cast<QGuiApplication*>(QCoreApplication::instance())`. Every engine-running UI edit uses it: sheet, dialog,
  grip drag, 2A.
- **Provider:** new `ui/dock_widgets/property_sheet/metaentity/entities/lc_propertiesprovider_offsetcurve.{h,cpp}`
  (base `LC_EntityTypePropertiesProvider(RS2::EntityOffsetCurve, …)`). Registration: include, member, `make_unique`,
  and a case in `lc_entity_property_containerprovider.{h,cpp}` (`h:29-53, 79-104`; `cpp:26-49, 52-79, 132-238`).

  | Row | Helper | Notes |
  |---|---|---|
  | Distance | `addLinearDistance` | `ATTR_POSITIVIE_VALUES_ONLY` (text provider :116-118) |
  | Side | `addEnum` | `LC_EnumDescriptor{"offsetSide", {{0, tr("Away from the centre")}, {1, tr("Towards the centre")}}}` |
  | Original centre | `addVector` | |
  | Original major / minor radius | `addLinearDistance` | |
  | Original rotation | `addWCSAngle` | |
  | Original start / end angle | `addRawAngle` | parameters, not WCS angles; show for every curve (never build a row that can add nothing: `addMultipleProperties` dereferences `props.first()`) |
  | Length | `addReadOnlyString` | in `doCreateCalculatedProperties` |

  - Links: "Explode to spline" (`RS2::ActionBlocksExplode`, precedent polyline provider :115) and "Offset"
    (`RS2::ActionModifyOffset`, precedent circle provider :73).
  - Setters capture `const bool multi = list.size() > 1;` and pass `LC_OffsetRunLimits::preview()` when it is true,
    otherwise `LC_OffsetRunLimits{}`.
  - Undefined or inconsistent curves: definition rows read-only; all setters still enforce runtime eligibility.
  - Also add an "Offset" link to the ellipse provider (`lc_propertiesprovider_ellipse.cpp:116-142` has none).
- **Refusal-aware sheet (U5).** Today the setters return nothing
  (`lc_entitypropertyvaluedelegate.h:45`, `:55-62`). Every row is an `LC_PropertyMulti`, and the editor writes the
  first sub-property and copies its *resulting* value to the others (`lc_property_multi.cpp:98-121`,
  `lc_property_single.h:77-82`). So a refusal must be signalled, not inferred from a comparison: a comparison would also
  drop layer and pen edits made through the same delegate.

  This is real generic infrastructure work in 1G-2.a: scope the property storage/single/multi/delegate files,
  `lc_propertysheetwidget.{h,cpp}`, `lib/engine/document/rs_document.{h,cpp}`, and a new
  `ui/dock_widgets/property_sheet/tests/lc_propertysheettransaction_tests.cpp` registered in `librecad_tests`.
  Its non-offset line/pen/layer fixtures test compatible default providers independently of the new offset provider.
  Run focused transaction/property tests, existing undo/document tests, ASan ownership checks and the full visible
  regression suite before that shared-history slice's commit. Use the context/outcome API without the offset provider
  first; do not introduce an unsafe intermediate pending queue and promise its lifetime fix in a later release.

  Replace the former boolean/raw-clone sketch with this transaction contract in `lc_entitypropertyvaluedelegate.h`,
  the property storage/single/multi classes and `lc_propertysheetwidget.{h,cpp}`:

  ```cpp
  enum class LC_PropertySetOutcome { Changed, Unchanged, Refused };
  // Context API contract; adapt names to the existing interfaces in 1G-2.a.
  // Defaults preserve existing providers; the sheet owns all accepted candidates.
  RS_Entity* currentEntityFor(RS_Entity* liveOriginal) const; // borrowed; matches existing mutable getter callbacks
  LC_PropertySetOutcome finishEntityModification(
      RS_Entity* liveOriginal, std::unique_ptr<RS_Entity> candidate, LC_PropertySetOutcome outcome);
  ```

  - **Operation outcome:** initialize/reset it for every setter invocation, including early equality/read-only exits;
    never infer refusal solely from equal geometry or retain a raw refused-clone pointer across operations. Existing
    void setters have a compatibility adapter with default accepted/changed behavior. New offset setters explicitly
    report `Changed`, `Unchanged` or `Refused`; an equality no-op is accepted, not refused. Expose the last outcome
    through `LC_PropertyValueStorage<T>`, `LC_PropertyAtomic` and `LC_PropertySingle<T>` without requiring every
    unrelated provider to be rewritten.
  - **Stable ownership:** delegates keep the live original identity, not a pointer to an intermediate pending clone.
    Reads and the next setter resolve `currentEntityFor(original)` so a distance edit followed by a layer edit sees
    the distance already pending. Clone that effective entity, then hand the candidate to the context with unique
    ownership. No delegate may keep a pointer to a candidate that the context can replace or delete.
  - **Pending queue:** exactly one live-original -> latest-pending-clone entry per entity. `Changed` replaces the
    pending candidate and disposes of the superseded uncommitted clone once; `Unchanged`/`Refused` dispose only of the
    new candidate and retain any earlier accepted change. Never enqueue an intermediate clone as both an addition
    and a deletion. Getter/delegate rebinding must cover every property row, not just the last edited delegate.
  - **Propagation:** `LC_PropertyMulti` stops only for `Refused` on the first sub-property. `Unchanged` still propagates
    its accepted effective value to later entities. A later refusal does not roll back earlier accepted candidates.
    An all-unchanged selection stages nothing and creates no undo cycle. Reset outcome state even when
    `LC_PropertySingle::setValue` takes its equality shortcut, so a preceding refusal cannot leak into the next edit.
  - **Delayed commit:** coalesce the 30 ms callbacks with one pending-commit generation/token. A callback from an old
    selection/document cannot commit into a new one. At flush, pass each original and final clone once to one
    modification batch; then rebuild/rebind property delegates to the committed live entities before more edits.
    An empty queue returns before `undoableModify`. Refusal messages/cache refresh do not discard earlier accepted
    pending edits, and layer/pen edits remain valid after a refusal.
  - **Cancellation/lifetime:** on selection change, detach, destruction, undo or redo, use one explicit policy:
    flush accepted pending edits before changing a live selection; discard candidates when the old document is
    detached/destroyed; cancel stale callbacks and rebuild caches/delegates on undo/redo. Delete only uncommitted
    candidates, never the document-owned original or an already handed-off clone. Cover all paths under ASan.
  - **History traversal is an explicit boundary:** synchronously flush accepted pending edits before a document
    Undo/Redo request, invalidate its queued timer generation, then traverse history; suppress selection-triggered
    flushing while preparing/traversing it. Undo therefore undoes the just-accepted edit, not an older unrelated cycle.
    A real new edit intentionally replaces an old redo branch; no-op/refused edits do not flush/prune redo history.
    No expired timer may recreate an edit after Undo or clear the redo cycle needed to redo it.
  - **Before/after hooks:** existing `RS_Undo::updateUndoState()` notifications occur before `changeUndoState()`;
    they are not a post-traversal refresh. In `rs_document.{h,cpp}`, override virtual `undo()`/`redo()` to bracket the
    existing base call with document modification-listener hooks. Add default no-op before/after methods to
    `LC_DocumentModificationListener`; do not reorder existing notifications or require unrelated listeners to change.
    The sheet registers/unregisters with its current document, flushes on the before hook and rebuilds/rebinds only
    after the after hook (also handle a no-op traversal). Guard reentrancy and detached-document lifetime. Normal
    programmatic document Undo/Redo follows this same path, not just the QAction handler.
  - **Refusing setter:** report the scoped operation outcome and call
    `m_actionContext->commandMessage(LC_ActionModifyOffset::offsetCurveRefusal(result))`. A user-selected read-only
    multi-selection fallback would revise U5 and the success criteria explicitly; it is not the implementation default.
  - **Multi-selection contract:** preserve the existing first-property propagation model. If the first setter refuses,
    stop propagation; if it succeeds and a later entity refuses, commit only accepted changes in one undo step and
    report the refusal. Selection order must not silently change. A no-op on the first entity still propagates the
    requested value to other entities that need it; no-op and refusal are different results.

**Tests** (`[lc-offsetcurve][ui]`, `librecad_tests`):
- **The provider fills headlessly** (pattern `ui/dock_widgets/layer_widget/tests/lc_layerblockwidgets_tests.cpp:285-304`).
- **A distance edit through the sheet:**
  - the sheet must be shown (`createPropertiesContainer` returns null otherwise, :582-594);
  - drive it with `doProcessLateRequest`, wait more than 30 ms, and check `countUndoCycles()`.
- **Refusals:**
  - a refused edit leaves the document and the undo stack unchanged, and the row valid;
  - with two curves selected where the first refuses, the second is unchanged;
  - a layer edit of an offset curve is still applied.
  - reverse the selection order: first succeeds, later refuses; accepted changes form one undo step, refused entities
    remain unchanged, and undo/redo refresh the row without dangling clones;
  - a no-op first entity with a changed second entity, all-no-op selection, repeated refusal then success, and a layer/
    pen edit immediately after refusal. Existing non-offset providers retain default accepted behavior.
- **Delayed-transaction ownership:** two distance edits before the 30 ms flush yield one final replacement and one
  undo cycle; distance then layer/pen preserves both edits; a subsequent refusal retains the earlier accepted change.
  Exercise selection change, document switch/detach/destruction, an old queued callback, cancellation and undo/redo.
  Assert only the live original and final candidate enter the batch; intermediate clones are deleted exactly once,
  delegates read the final state, and no stale original/pending pointer is dereferenced (ASan).
- **History boundary:** pending changed edit -> Undo -> expired timer -> Redo restores only that accepted edit;
  no-op/refused pending input preserves an existing redo branch. A pending real new edit deliberately replaces that
  branch before traversal. Test direct document calls and QAction calls; refresh observes post-traversal entities,
  not the old states from `updateUndoState()`. Existing document listeners still receive their original callbacks.

### 1G-3. Edit Properties case and widget

**Files.**
- **`LC_ActionModifyEntity`** (`lc_action_modify_entity.cpp:102-117`): replace 1A-2's interim guard with
  `case RS2::EntityOffsetCurve:`.
- **`LC_DlgEntityProperties::prepareTypeSpecificUI`** (`ui/dialogs/entity/lc_dlg_entityproperties.cpp:169-266`,
  template :221-226): one case and one include (:29-44).
- **New `ui/dialogs/entity/lc_propertieseditingwidget_offsetcurve.{h,cpp,ui}`:**
  - CMake gets `.h` and `.cpp` only (AUTOUIC; ellipse pattern `CMakeLists.txt:1540-1541`); `src.pro` gets HEADERS,
    SOURCES and FORMS.
  - Copy the **ellipse** widget's per-field `editingFinished` pattern (`lc_propertieseditingwidget_ellipse.cpp:35-42`,
    `:71-137`), not the hyperbola, parabola or spline-points widgets, whose `updateEntityData()` is never called.
  - A hidden, word-wrapped `QLabel` shows a refusal's reason (the command line sits behind the modal dialog), and the
    field is restored from the entity.
  - Values picked in the drawing come through `interactiveInputUpdate`, which emits `editingFinished`
    (`lc_entitypropertieseditorwidget.cpp:55-58`).
- **Keep the `InEditing` replace unconditional** (:168-184), as for every other type: there is no safe entity equality,
  and the dialog also edits layer and pen on the same clone (`lc_dlg_entityproperties.cpp:161-167`). A refused field
  never changes the clone.

**Tests:**
- The widget builds headlessly (pattern `rs_hatch_tests.cpp:1460-1492`) and applies a distance edit to its entity.
- A refused value shows the label and restores the field.
- The commit path needs the application window (`lc_action_modify_entity.cpp:99`, `:179`): cover it in 1I-a's GUI
  pass.

### 1H. Grip drag in the default action

**Files:** `actions/drawing/rs_actiondefault.cpp`.
- **The hit test needs nothing.** It reaches `getRefPoints()` through `RS_Document::getNearestSelectedRefInfo`
  (`rs_document.cpp:79-103`).
- **Preview:** a `case RS2::EntityOffsetCurve:` in the `MovingRef` switch (:333), beside the Circle case (:479-495):
  1. `m_actionData->v2 = mouse;` (the raw snap point; no `getSnapAngleAwarePoint`);
  2. `curve->throughParameters(v2, &d, &toward)`, closed form, no engine;
  3. `m_preview->addEntity(new RS_Polyline(nullptr, …))` with the points of `offsetPolyline(original, d, toward, 128)`.
     For a whole original inward with b²/a ≤ d < b, keep only the lens (Appendix A). `RS_Preview` exempts polylines
     from its budget (`rs_preview.cpp:87-88`);
  4. `addClone = false`, so the generic clone + `moveRef` per mouse move (:516-529) does not run.

  An undefined or inconsistent curve keeps the generic path. The 128-point preview is indicative, not a cache adopted
  on commit. Bound point count and require finite output. Do not present an untrimmed self-crossing path as the final
  result for a trimmed arc: when the cheap analytic path cannot determine its retained topology, show the existing
  cache as a bounded polyline while tracking the proposed through-point, and compute/validate the real result only on
  completion. Do not invoke the reference checker/intersection search per mouse move to obtain a prettier preview.
- **Prompt:** an offset-curve case for `MovingRef` without the angle-snap modifier text (the default falls back to
  `MOD_SHIFT_ANGLE_SNAP`, :1041-1043).
- **Commit** (`onMouseMovingRefCompleted`, :677-822; the clone is made at :688): a case in the switch at :690 that:
  1. sets `moveRefOnClone = false; m_actionData->v2 = mouse;`;
  2. runs `setThrough(v2, LC_OffsetRunLimits{})` on the clone inside the busy-cursor guard;
  3. on refusal: `delete clone; clone = nullptr;` and `commandMessage(LC_ActionModifyOffset::offsetCurveRefusal(r))`.

  Guard the `moveRef` block (:801-803) and the `undoableModify` block (:805-817) with `clone != nullptr`. The tail
  (:818-821: `goToNeutralStatus(); deleteSnapper(); redraw(); m_movingJustCompleted = true;`) always runs: an early
  return leaks the clone and leaves the action in `MovingRef`. The relative zero was moved to the grip on entering
  `MovingRef` (:300); a refusal leaves it there, as an Escape-cancelled drag does.

**Tests** (`[lc-offsetcurve][grip-drag]`, `librecad_tests`):
- **Harness:** a probe subclass after `HoverProbe` (`lib/actions/tests/lc_snappercatchtests.cpp:100-112`).
  - Attach a real `QMouseEvent`: the Neutral press dereferences `e->originalEvent->modifiers()`
    (`rs_actiondefault.cpp:875`).
  - Drive press, a move of more than 10 px, a move, then release (`m_actionData` is private; without `init()`,
    `m_completeMovingByMousePressed` is false, so the commit happens on release).
  - Count undo steps with `countUndoCycles()` (`rs_undo.h:61`).
- **Cases:**
  - one replace, the distance equals the pointer's distance within tolerance, one undo step, and undo/redo restore;
  - the preview holds one polyline and no spline clone, and `engineRuns()` is unchanged during the moves;
  - closed-arc/multi-component/near-cusp preview uncertainty uses bounded existing-cache feedback; non-finite pointer
    input adds no invalid preview geometry; repeated motion runs neither regeneration nor adaptive validation;
  - a refused drag (into the centre of a whole ellipse, past b): document and undo stack unchanged, status back to
    Neutral, no leak (ASan);
  - dragging across the curve flips the side.

### 1F. Option "Editable offset curve" and messages

This step is last in phase 1, so the default never turns on before the UI exists.

**Files** (options are saved under the group "ActionModifyOffset"):
- **`actions/drawing/modify/lc_action_modify_offset.{h,cpp}`:**
  - getter and setter beside `h:53-56` and `cpp:483-498`;
  - `save("EditableOffsetCurve", …)` in `doSaveOptions` (:57-66);
  - `loadBool("EditableOffsetCurve", true)` in `doLoadOptions` (:68-89);
  - **not** in the constructor (:49-52).
  - The data object is created at :46 and passed at :99-100 (commit) and :317-318 (preview). One object serves both,
    so only `forPreviewOnly` keeps the preview a spline.
- **Message block** in `doTriggerCompletion` (:215-295): collect `keptPlain` inside the loop at :229-255 (before
  `m_pendingOutcome.reset()` at :256), and emit right after the failure message (:257-262):
  `commandMessage(tr("%n offset(s) made as plain splines, not editable offset curves: %1", "", keptPlain).arg(reasons.join("; ")))`.
  Set it only when `asOffsetCurve` is on and the source is a top-level ellipse or an offset curve.
- **`ui/action_options/modify/lc_offset_options_widget.{ui,h,cpp}`:**
  - `cbEditableOffsetCurve` at grid column 12, with the spacer moved to column 13;
  - connect it with `QCheckBox::clicked` (as :40-43), not `toggled`; `setChecked` in `doUpdateByAction` (:83-93); a
    slot like `cbKeepOriginalsClicked` (:109-112);
  - the `LC_GuardedSignalsBlocker({...});` at :74-82 is an unnamed temporary that blocks nothing (Appendix C). Do not
    rely on it;
  - touch the `.cpp` after editing the `.ui`.
- **`ui/action_options/modify/lc_offset_options_filler.cpp`:** `addBoolean({"a_editableOffsetCurve", …})` like :51-56.

**Tests** (`[lc-offsetcurve][option]`, `lc_action_modify_offset_tests.cpp`):
- **Default on:** remove the key first, as `KeepOriginalsSetting::remove()` does (:273-301), or add
  `void remove() const` to `lc::test::SettingGuard`; then call `loadOptions()` and expect the option on.
- **Option on:** an ellipse gives `EntityOffsetCurve` and the preview stays a spline (do not use `checkMade`,
  `lc_offsetmatrixtests.cpp:338-339`); a two-piece arc gives one captured message.
- **Existing pins unchanged:** `messages.isEmpty()`, `size() == 1`, and the matrix test's ellipse rows (:632-705).

### 1I-a. Release checks (agent)

- Full suite `--order decl`, plus random order with two seeds; both fast lanes with the CI excludes.
- The complete 1E-3 DXF/DWG version and conversion matrix, with recorded case counts for each format and the binary
  DXF input check. Valid editable curves must reload as `EntityOffsetCurve` in every schema-capable row.
- ASan: `[lc-offsetcurve]`, `[entity-coverage]`, and the offset, action and property-sheet tags.
- **qmake build** of the branch, as a shadow build:
  ```bash
  BQ=<worktree>/build-qmake; mkdir -p "$BQ"
  cd "$BQ" && /opt/homebrew/bin/qmake6 -r CONFIG+=sdk_no_version_check <worktree>/librecad.pro
  make -C "$BQ/libraries" -j8 && make -C "$BQ/librecad" -j8
  ```
  `build-qmake/` is untracked; never stage it. The macOS post-link step briefly writes `.qm` files into
  `<worktree>/librecad/ts`; make sure none is left staged or modified.
- **Hidden GUI pass,** `[.gui][lc-offsetcurve]`, added to `ui/main/tests/lc_docklayout_tests.cpp` (fixture
  `testWindow()` :114, `DockFixture` :136, new-drawing case :188-211). Run it as
  `QT_QPA_PLATFORM=offscreen build/librecad_tests "[.gui][lc-offsetcurve]" --order decl`.
  Discover this exact selector first and require nonzero matched/executed cases; `[gui]` does not select `[.gui]`.
  - No `Qt6::Test` is linked to any test target: trigger QActions with `getAction(...)->trigger()`, send `QMouseEvent`s
    to `QG_GraphicView` with `QApplication::sendEvent`, and close modal dialogs from `QTimer::singleShot` via
    `QApplication::activeModalWidget()`.
  - Seed `ActionModifyOffset/DistanceFixed` and `Distance` in settings, and show the Properties dock.
  - Sequence: new drawing → add an ellipse → Modify > Offset with the option on → select the curve → read the
    property rows → edit Distance → Edit Properties → save DXF and DWG with
    `LC_DocumentsStorage::exportGraphics` → reopen each with `loadDocument` and check editability and the saved
    definition. Then Explode → save/open both formats again and check plain spline geometry. Never Save As: it opens
    a native file dialog.
- **Manual:** the independent file checks (1E-2/1E-3) and measured load-check cost on 2,000 curves, with the revised
  validator's work caps and actual results recorded. The old prototype's sub-millisecond cost is not a target or proof.
- **PR texts:** drafts under `build/evidence/release/pr-texts.md`, for PRs B, C and D; summarize their validation in
  the committed roadmap checkpoint so durable status does not depend on ignored artifacts.
  - What is solved: grips, the centre snap, distance editing, Explode, and editable DXF/DWG round trips across the
    supported version matrix. Other readers can render the ordinary spline cache when they support the file version.
  - What is not solved: a true ellipse, spline-source offsets (#2892), trimming the curve, two-piece arc results, and
    chord-based queries as for every spline.
  - The 0B known gaps, and the guard's compile messages.

### 1I-u. User actions

Push the branch; watch `dwg-dxf-verify` and `build-all`; post D0 and the DXF answer on #2961; open PRs B, C and D,
respecting §0.2.

---

## 6. Phase 2 — "Update from Source" and "Restore Original"

Explicit editing by the embedded original. "Update from Source": select offset curves, pick an ellipse in the drawing, and
the curves take it as their new original. "Restore Original": insert the embedded original as an ellipse.

### 2A. Action "Update from Source"

- **Tests belong to this slice.** Introduce and register
  `actions/drawing/modify/tests/lc_action_offsetcurve_source_tests.cpp` in `librecad_tests`, tagged
  `[lc-offsetcurve][update-source]`. Implement the Update cases listed in 2D-a now, before this slice's commit;
  2D-a reruns/extends them, not defers their creation. Run focused Release/ASan plus adjacent modification tests.
- **New files** `actions/drawing/modify/lc_action_offsetcurve_updatesource.{h,cpp}`, class
  `LC_ActionOffsetCurveUpdateSource`. Its `RS2::ActionOffsetCurveUpdateSource` is appended after
  `ActionModifyAlignRef` (`rs.h:460`). ActionType values are not persisted.
- **Template: `LC_ActionModifyAlign`** in entity mode.
  - Base: `LC_ActionPreSelectionAwareBase("ActionOffsetCurveUpdateSource", ctx, RS2::ActionOffsetCurveUpdateSource, {RS2::EntityOffsetCurve})`.
  - Override `onSelectionCompleted` to wait for the pick (`lc_action_modify_align.cpp:75-78`), with
    `isAllowTriggerOnEmptySelection()` returning false.
- **Context entities.** Override `doInitWithContextEntity` and ignore a context entity whose rtti is not
  `EntityOffsetCurve`; `init` then takes the curves from the selection (`lc_actionpreselectionawarebase.cpp:62-67`).
  This is needed because any entity's context menu offers the most recent action (`lc_menufactory_graphicview.cpp:89`,
  `:520-531`), and the base pushes the context entity unchecked (:47-54).
- **Picking the source:** `catchEntityByEvent(e, RS2::EntityEllipse)` (`rs_previewactioninterface.h:199`), both in the
  move highlight and on release, with explicit ownership filtering. Accept a live ellipse whose parent is the target
  document/container open for editing; refuse a generated child of an INSERT, dimension or other owner. A visible
  locked source may be read without modification; target curves still obey their locks. No engine run per mouse move.
- **`doTriggerModifications(ctx)`:**
  - `ctx.dontSetActiveLayerAndPen()`;
  - per curve: skip any non-`EntityOffsetCurve` entry, and skip undefined, inconsistent, hidden or locked curves;
  - clone, then `setBasis(picked.getData(), limits)`, with `preview()` limits when more than one curve is selected
    (D13);
  - an unchanged original means no replace;
  - on success `ctx.replace(curve, clone)`; on refusal delete the clone and count the reason;
  - return false when nothing changed, after deleting every clone (`undoableExecute` does not free refused batches,
    `lc_undosection.cpp:116-134`);
  - report "n updated, m refused: …" through `commandMessage`.

  On failure keep the selection (as Offset does), not Align's unconditional clear.

### 2B. Action "Restore Original"

- **Tests belong to this slice.** Extend the source-action test file from 2A with the Restore cases listed in 2D-a,
  including geometry-equality boundaries, ownership and undo/redo. Run them in Release/ASan before committing 2B;
  integration persistence/UI checks remain in 2D-a.
- **New files** `actions/drawing/modify/lc_action_offsetcurve_restoreoriginal.{h,cpp}`, `RS2::ActionOffsetCurveRestoreOriginal`.
  Template `LC_ActionDimOrdinateRebase` (pre-select, then trigger), with the same context-entity guard as 2A.
- **For each selected, defined curve:** insert `RS_Ellipse(document, definition().basis)`, unless a top-level ellipse
  of the same container already equals the original, or one was inserted earlier in the same batch.
  - Set `e->setLayer(curve->getLayer(false)); e->setPen(curve->getPen(false));` and call
    `ctx.dontSetActiveLayerAndPen()`. The template does not, and `setupAndUndoableAdd` would otherwise apply the
    current layer and pen (`lc_undosection.cpp:140-160`; a pre-existing bug in "Ordinates Re-base", Appendix C).
  - Skip curves on locked layers: `undoableExecute` adds regardless.
- **Equality** (there is no helper; `RS_EllipseData` has render caches):
  - normalise both;
  - use local geometric scale for tolerance, plus an explicit floating-point ULP allowance for large world centres;
    do not multiply `1e-9` by world-coordinate magnitude, which could merge visibly distinct nearby ellipses;
  - equal centres and semi-axes; major axes parallel or antiparallel;
  - a normalized exact circle has no distinguished major-axis direction: whole circles compare centre/radius, and
    circular arcs compare their actual endpoints and mid-sweep point. Do not invent an arbitrary near-circle threshold
    that merges distinct ellipses. Include equivalent angle-shifted/rotated circle representations in tests;
  - whole equals whole; arcs compare the unordered pair {start, end} plus the mid-sweep point.
- Search only live ellipses in the same target container; exclude deleted/undo-retained entities and other block
  definitions. Geometry-equal ellipses on another layer in that container count as already present. Uncertain numerical
  equality is not evidence to suppress restoration. Restore only curves with a defined, consistent embedded original.
- **One undo step;** report "n inserted, m skipped".

### 2C. Registration, menus, links

- **`ui/lc_actionhandlerfactory.cpp`:** includes (~:106) and two cases in `doCreateActionInstance` (pattern :765-767).
- **`ui/main/init/lc_actionfactory.cpp` `createModifyActions`** (pattern :392): two QActions,
  `"OffsetCurveUpdateSource"` / `tr("Update from Source")` and `"OffsetCurveRestoreOriginal"` / `tr("Restore Original")`.
  - Reuse existing icons (e.g. `:/icons/offset.lci` and an ellipse icon); no new art.
  - Do **not** add them to `fillActionsList(modifyActions, …)` (:1156-1184), which feeds the Modify menu, toolbar and
    dock. They live in the context menu and the property links.
- **Command-line names:** none (U4).
- **Context menu of the type** (1G-1 item 3): add both names.
- **Provider links** (1G-2): "Update from source" and "Restore original", single-entity links; the multi-selection
  link goes in `doCreateSelectedSetCommands` (pattern `lc_propertiesprovider_spline.cpp:153-163`).
- **Build lists:** `CMakeLists.txt` beside :732-733, `src.pro` beside :1542 and :1708.

### 2D-a. Tests and release checks (agent)

Rerun and extend `actions/drawing/modify/tests/lc_action_offsetcurve_source_tests.cpp`,
`[lc-offsetcurve][update-source]` (`librecad_tests`; both actions can be tested headlessly). The focused Update and
Restore cases below are implemented/registered with 2A and 2B respectively; they are not postponed until this gate.
- **Update from Source:**
  - a moved original takes the fast path (`engineRuns()` unchanged, cache moved exactly);
  - a rotated, a reshaped, a trimmed (new angles), a mirrored and a reversed original;
  - a failing re-base (inward, b < d) keeps the curve unchanged and reports it;
  - locked and hidden curves are skipped; a picked non-ellipse is refused;
  - a context entity that is a line is ignored;
  - a generated ellipse inside an INSERT is refused as source, while a visible locked document ellipse can be read;
  - inconsistent targets and no-op updates create no replacements or undo cycle;
  - one undo step, and undo/redo restore.
- **Restore Original:**
  - inserts one ellipse with the curve's layer and pen; a second call inserts none;
  - a reversed but equal ellipse counts as equal;
  - two curves on one original insert one ellipse.
  - deleted originals and matching geometry in another block do not suppress restoration; a large world centre must
    not cause two distinct local ellipses to compare equal.
  - equivalent whole circles and circular arcs with different axis directions deduplicate; distinct near-circles and
    complementary arcs do not. Deduplication is geometry-based, not a comparison of ellipse render caches.
- **Persistence after either action:** save/open as DXF R2018 and DWG R2018. Updated curves retain the new definition
  and remain editable; restored originals reload as ellipses with the expected geometry, layer and pen.
- Then the 1I-a checks again: suite, lanes, ASan on the new tags, qmake, and the GUI pass extended with both actions.

### 2D-u. User actions

Push, CI, PR E.

---

## 7. Risks, and what to watch

1. **A missed type-keyed site fails silently:** an empty INSERT, or an entity missing from a save that reports success.
   0B is the gate; Appendix B lists the sites.
2. **Rules rest on single-run prototypes:** grips, transforms, drag, the file group, the check. Every rule has a test
   in 1A–1H; do not ship a rule without its test.
3. **Offset engine topology and latency.** #3117 fixes several known trimmed-arc failures and adds corner/topology
   tests; it does not establish correctness for all ellipse arcs. Re-run the bounded geometry sweep at S0, retain
   invariants rather than internal status/control counts, and measure responsiveness before deciding busy-cursor scope.
   The revised checker must bound its accepted geometric error and reject unresolved topology/work-limit cases;
   its numerical contract does not prove an exact algebraic identity.
4. **One bad XDATA item, or an empty trailing group, fails the whole save** (DXF and DWG). `forExport` validates
   first; 1E-2 tests loss of definition followed by DXF and DWG save/open.
5. **Shared XDATA variants:** editing a stored variant changes every clone. Always build new vectors.
6. **Precision:** DXF writes 16 significant digits, and the original and the control points round independently. Never
   compare bit for bit across a DXF save.
7. **Cross-compiler determinism is unknown.** The checked-in fixtures (1E-2) run on MSVC and GCC in CI.
8. **Upstream drift during implementation:** when needed, rebase at a verified checkpoint; rerun affected baseline and focused tests;
   recensus all type sites and re-read anchors rather than assuming the current local `origin/master` remains current.
9. **Acceptance:** the UI files belong mostly to another maintainer (sand1024), and the grip set was never put to
   users. The user posts D0 before PR D (§0.2).
10. **Not run anywhere:** AutoCAD and BricsCAD (ezdxf and ODA stand in); a real window system (offscreen only).

### Discovered during implementation

- 0B-2's initial mixed-block failures were fixture ordering, not unsupported entities: `addEntity()` prepends hatch/
  image records. Controlled fixtures use `appendEntity()` and assert their premise; no baseline gap was invented.
- 1A-1.a's numeric findings and fixes are recorded in its executed card and tests, not deferred as new-type gaps.
- Reference analysis needs the pre-allocation pair cap and consumed-work contract in `.b-budget` before it can reuse
  the intersection solver as a bounded decode/adoption validator.
- v7's read-only source review found an independent P1 termination hazard in `classifyContact()`'s zero-step
  doubling loop. `.b-progress` fixed and verified it atfb70681e2; this is distinct from the committed pair-budget
  fix. Do not reopen or reapply either fix merely because its original discovery is retained here.
- Shared budget verification exposed avoidable X-only candidate work before refinement; the budget leaf now includes
  bounded two-axis aggregate pruning. Required original geometry tests stay unchanged and default limits are not
  raised. Its executed card records both failed runs and concrete work measurements.
- Existing intersection `Ok`/point estimates do not certify event enclosures/order. `.b-ref-events` must add the
  strict opt-in contract before the graph can support adoption; ordinary callers retain their existing defaults.
- Raw spline conversion drops Z and repairs/rounds geometry; R14 requires eligibility checks before that conversion.
  The DWG normal is an unspecified zero sentinel, so normal validation must not reject every legitimate DWG carrier.
- DXF publication is already transactional and consults the filter's finalization failure latch. R15 tests this
  existing mechanism and offset-specific early returns; a second staging implementation would be unnecessary scope.
- `toleranceFor()` intentionally exposes the numeric floor even when the engine's default options reject equality
  with that floor. In 1B, qualify large-centre transformed/read caches and explicit editing: a caller may request the
  next representable tolerance above that floor to meet the existing strict guard, without changing engine defaults,
  broadening saved relative-factor policy or hiding geometric error. Test one-run edits and both-format reload at
  that boundary; genuine arithmetic/geometry refusal still preserves the old object.

### Plan review 2026-10-15

The following is the historical findings inventory. Some owning leaves are now committed; others remain pending.
Current completion is determined by §0 and the executed cards, not by inclusion in this table. A documentation
correction is never evidence that its product fix has been implemented or verified.

| Finding | Required correction | Verification owner |
|---|---|---|
| Three-point acceptance misses between-sample deviation and topology | Bounded adaptive geometry and retained-component validation | 1A-1.b/.c |
| Remainder-only arc normalization promotes a zero sweep | Classify raw canonical whole/full/tiny/multiple-turn sweeps before construction | 1A-1.a |
| Length-cache hash/debug assertions omit semantic mutations | Complete cache stamp and release-mode edit guards | 1A-2, 1B |
| Public raw constructors can bypass validation | Private validated constructors and real public adoption factory in class-core slice | 1A-2 |
| Arc source can have a closed result with no real endpoint seam | Separate source kind and validated cache closure | 1A-3 |
| Endpoint-minus-centre encoding loses a small major axis | Typed 1012 displacement and explicit pre-release schema compatibility | 1E-1.a, 1E-3 |
| Tail item count alone does not bound encoded payloads | Byte/depth/type caps, safe whole-tail policy and failure propagation | 1E-1.a, 1E-2 |
| Optional fallback search leaves required behavior untested | Deterministic zero-sum fallback, bounded rebase attempts | 1C-2 |
| Automatic revert and pre-commit completion can lose progress | Preserved edits, implementation commit then nonrecursive plan checkpoint | Every slice, §1.4 |
| A candidate can match only a proper subset of a valid offset | Exactly one retained reference component plus bidirectional ordered coverage | 1A-1.b-ref/.b-check |
| Finite transform inputs can overflow, and grip tolerance can span distant local features | Stage/validate transformed results; local geometry/ULP grip identity | 1A-2, 1H |
| Refusal-only property handling cannot distinguish no-ops or own rapid-edit clones safely | Changed/Unchanged/Refused outcomes and one-original/latest-candidate transactions | 1G-2.a/.c |
| Early undo-state callbacks cannot refresh post-traversal state or define pending-edit policy | Synchronous pre-traversal flush, default before/after document hooks and stale-timer/redo tests | 1G-2.a/.c |
| Undefined/inconsistent generic dragging conflicts with an inert moveRef | Inert only while editable; base-spline fallback never restamps stale metadata | 1A-3, 1H |
| Trimming APPID whitespace changes codec identity | Exact case-insensitive identity without whitespace normalization; padded names stay foreign | 1E-1.a, 1E-2 |
| Malformed/unknown schema precedence and redundant side flags are underspecified | Explicit Invalid/diagnostics, owned-range grammar, raw unknown policy and bit-3 consistency | 1E-1.a, 1E-2 |
| Phase-2 tests arrive after their implementation commit; Trim gates the wrong PR | Focused tests accompany 2A/2B; PR C contains 1D | 2A/2B, §0.2 |
| Circle axis direction is nonunique | Geometry-based circle/arc deduplication without merging distinct near-circles | 2B |
| Mixed current statuses and aggregate steps hide unfinished work | Reconcile committed/prepared states; explicit leaf recipes and dependency checks | §0.3/§0.6, every checkpoint |
| Pair caps do not bound a zero-step contact loop | Deterministic classification progress/iteration guards and underflow regressions | 1A-1.b-progress |
| Estimated intersections do not establish certified topology | Strict event enclosures/proof category and unresolved-order refusal | 1A-1.b-ref-events |
| Whole-ellipse distance or unbudgeted visibility seeding can invalidate arc proofs | Restricted-source bounds and charge-before-allocation shared visibility accounting | 1A-1.b-ref-components/.b-check-bounds |
| XY projection and importer repairs can manufacture valid-looking caches | Raw carrier preflight and shape-preserving marked import before validated adoption | 1E-1.c, 1E-2.safety |
| A strict normal check would reject DWG's unspecified zero normal | Accept the known zero sentinel only with full raw 2D controls; reject incompatible explicit planes | 1E-1.c |
| Raw schema sweep precheck conflicts with normalization tolerance | One pre-construction sweep contract; remove the strict-2π/constructed-whole shortcut | 1E-1.a |
| Log-and-return export or a wrong GUI selector can produce false green evidence | Offset-specific failure latch, existing transaction tests and nonzero exact-selector checks | 1E-1.b, 1E-2.safety, 1I-a |
| A fresh critical-contraction budget can reset spent work | Bind an opaque issuing-budget lifetime, original ceilings and persistent canonical history; reject fresh/expired identities before proof work | 1A-1.b-ref-critical-refine |
| A proof contradiction can leave reusable trusted state live | Terminal invalidity disables trusted snapshots/handles; distinguish invalid calls, nonterminal ambiguity and sticky limits | Both refinement children |
| Scalar/angle refinement can conceal work or use a midpoint target as proof | One bounded charged compound round; residual sin²(mid)-U and whole-angle derivative; positive complement derivative | 1A-1.b-ref-critical-refine |
| An exact pi/4 special case can falsely qualify general inverse refinement | Keep exact fixture and require unchanged adjacent scalar plus contracted angle for nonspecial D30 case | 1A-1.b-ref-critical-refine |
| A borrowed/default composite drops strict proofs or leaks local segment domains | Immutable owned providers, validated both-hook remapping and cleared refusal outputs; critical sessions retain independent provenance | 1A-1.b-ref-providers |
| Rounded circle collapse or fixed cutter indices hide valid geometry | Charged exact trusted-collapse eligibility, explicit immutable absence disposition, contiguous role accessors; adjacent-distance/resource negatives | 1A-1.b-ref-providers |
| Exact visibility is confused with tolerated engine trimming or sampled clearance | Whole-interval restricted-source proof, explicit trim epsilon, complete events/coverage, all errors in Lipschitz bound; preserve 2*tol/D-2.5*tol | 1A-1.b-ref-components/.b-check-bounds |
| Endpoint equality/partial neighborhoods hide a nearby real cap crossing | Complete cached offset-box radial/support proof, original global domains, exact isolation; outside-cache Unavailable plus independent transverse recrossing regression | 1A-1.b-ref-contacts |
| Floating F straddles zero at valid near-threshold endpoints | Exact polynomial/global regime and complete issued critical gap; same-budget refinement, true history and whole h' proof | 1A-1.b-ref-contacts |
| QueryOk or a base-refusal test is overclaimed as node accuracy/fallback coverage | Explicit evidence classes:6/6strictqueries but coarse images; source-qualified fallback, native base negative and bounded NO_MATCH are distinct | 1A-1.b-ref-events-isolate, 1A-1.c |

---

## 8. Out of scope

- **A stored link and automatic follow** (the research's phases 3–4). #3098 merged `RS_Entity::lineage()` /
  `ownerLineage()`, but they live in memory only, and a copy is renumbered only for INSERTs and owned texts
  (`lc_undosection.cpp:35-37`, `:80-89`). An in-session link would need `hasTrackedLineage()` to cover ellipses and
  offset curves; a link that survives a save needs a persisted id. It would also need the research's two undo fixes
  (add-then-delete resurrected by undo; nested cycles leaving `autoUpdateBorders` off).
- **Other sources:** spline, parabola and hyperbola sources (#2892), and polyline-segment sources.
- **Other results:** two-piece results (D8), an ellipse result (D11), and fewer control points in the cache or the
  exploded spline.
- **Other operations:** trimming or extending the curve itself; Draw > Parallel for ellipses; automatically recognizing
  unmarked offsets already in drawings. Valid schema-1 file adoption is required and is not excluded by this row.

---

## 9. Appendices

### A. Closed forms (ellipse with semi-axes a ≥ b, parameter η, in the original's frame)

- `E(η) = (a cos η, b sin η)`, `E'(η) = (−a sin η, b cos η)`, `r(η) = |E'(η)| = sqrt(a² sin²η + b² cos²η)`.
- Outward unit normal: `N(η) = (b cos η, a sin η) / r(η)`.
- Offset with signed s (s > 0 outward, s < 0 towards the centre): `O(η) = ((a + s·b/r) cos η, (b + s·a/r) sin η)`.
- Radius of curvature: `ρ(η) = r(η)³ / (a·b)`. Smallest: `b²/a` at η = 0, π. Largest: `a²/b` at η = ±π/2.
- Inward, whole ellipse:
  - smooth for d < b²/a;
  - for b²/a ≤ d < b, a lens with two corners on the major axis, at parameters with
    `sin²η = (a²d²/b² − b²)/(a² − b²)` (clamp to [0, 1]) and x = ±(c²/a)·|cos η|, where c² = a² − b²;
  - for a = b (a circle), there is no lens: the offset is a circle of radius a − d until it vanishes at d = a;
  - nothing is left from d = b. The engine's criterion: half-extent `c·sqrt(1 − (d/b)²)` within
    requestedGeometry + classification.
- General noncircular ellipse offsets use the engine's bounded cubic approximation. A circular basis and zero-distance
  degeneracies are exceptions to any blanket claim that an ellipse offset can never have an exact conic representation;
  this feature rejects zero distance and validates its actual cached result.

### B. Type-keyed sites (seed census at 7c4a93fa8; mandatory recensus at S0)

**Need the new enumerator, a case or a row** (1A-2, 1D, 1G):
- **Files:** `rs_filterdxfrw.cpp` (`writeEntity` :30169; hatch edges :32105, :32173); `lc_makercamsvg.cpp:229`.
- **Engine:** `rs_insert.cpp` (`transformCapability` :96-164, `cloneForTransform` :176-208); `rs_information.cpp:450`;
  `rs_creation.cpp:166`; `rs_preview.cpp:90`; `rs_snapper.cpp:75`; `rs_entitycontainer.cpp:754`;
  `rs_modification.cpp` (explode :2759, scale :1843-1858).
- **Plugins:** `doc_plugin_interface.cpp:364` and `:818-830`.
- **Actions:** `lc_action_select_intersected.cpp:176`; `lc_action_modify_offset.cpp:44-45`; both parallel actions;
  `lc_action_spline_modify_explode.cpp:39-40, 198`; the landed single-Trim chord → parent mapping.
- **UI:**
  - `lc_entitymetauiutils.cpp:77-105` and the match descriptor registry;
  - the property container provider;
  - `lc_action_modify_entity.cpp:102-117` and `lc_dlg_entityproperties.cpp:169-266`;
  - `lc_quickinfoentitydata.cpp` (two switches);
  - `lc_menufactory_graphicview.cpp:563-1126`;
  - `lc_selectwindowoptions.cpp:198-202, 283-287`;
  - `lc_menuactivator.cpp` (three switches) and `lc_dlg_menu_assigner.cpp:106-129`.

**Candidate no-op sites** (recheck at S0; upstream edits may have introduced new dispatch points):
- `rs_filterjww.cpp:1639` (JWW export is dead) and `rs_filterdxf.cpp:1374` (not built).
- The Python bindings (`RS_OPT_PYTHON` is never defined).
- The `rs_entity.cpp` debug stream.
- Revert Direction (admits every container), `LC_ActionInfoProperties` (its default arm allows the hover glow),
  `RS_Entity::trimmable()` (no spline case, as today), `LC_UndoSection::setupAndUndoableAdd` (the same as a spline).
- Printing, PDF, PNG, SVG and the console converters (all through `draw()`).
- The DWG-only paths and libdxfrw (no type switches).
- `.ts` files and icons (feature PRs touch neither).

### C. Pre-existing defects found while planning (not in scope; candidates for separate tasks)

- **Trim:**
  - Line Join can flag a polyline segment as deleted in place (`lc_action_modify_line_join.cpp:345, :349`; read only).
- **Edit Properties:**
  - For hyperbola, parabola and spline points it applies no geometry edit (`updateEntityData()` is never called).
  - Leader, Tolerance, Solid, ConstructionLine, Wipeout and MLeader leave a stuck action (U3).
  - `LC_ActionModifyEntity` creates a new editor on every trigger and deletes only the last (`:120-121`).
- **Context menu** (`lc_menufactory_graphicview.cpp`): missing commas fuse `"DrawCircleTan3" "DrawCircleInscribe"`
  (:625-626) and drop separators (:880-881, :1013-1014).
- **Quick Info:** the hyperbola's End shows `getStartpoint()` (`lc_quickinfoentitydata.cpp:796`).
- **Match descriptor:** the spline's length is registered with `addBoolean` (`lc_matchdescriptor_spline.cpp:53`).
- **Toolbar:** the Spline toolbar is built from `curveActions` (`lc_toolbarfactory.cpp:332`).
- **Signal blockers:** `LC_GuardedSignalsBlocker({...});` as an unnamed temporary blocks nothing, in at least 9
  widgets (e.g. `lc_offset_options_widget.cpp:74-82`).
- **"Ordinates Re-base"** puts its result on the current layer and pen (`lc_actiondimordinaterebase.cpp:49-67`).
- **Ellipse provider:** uses `addWCSAngle` for its own start/end angle rows, which are parameters.
- **Hatches, MakerCAM, plugins:**
  - A hatch with a parabola edge loses that edge on save.
  - MakerCAM SVG drops parabolas and hyperbolas.
  - Plugins see Hyperbola, Parabola, DimArc, Wipeout and MLeader as UNKNOWN.
- **R12** drops a solid hatch silently (`rs_filterdxfrw.cpp:32039-32054` with `:12751-12758`).
- **DXF APPID:** XDATA app registration is fixed by #3119. The separate closed-spline writer-layout concern remains
  a candidate for verification, not an offset-curve prerequisite.
- **Offset engine:** #3117 fixed the previously listed trimmed-arc corner and false-empty cases. Broader unsupported
  topology may still exist; use the S0 characterization sweep rather than retaining the old defect claims.
- **`RS_Ellipse::getNearestPointOnEntity`:** elliptic-arc interior critical points and endpoints are handled by #3112;
  keep this helper distinct from the new type's distance-to-offset validator.
- **CI path filter:** `dwg-dxf-verify.yml` names `rs_entitycontainer.*` under the wrong directory (fixed in 0B-1).

### D. Historical key anchors (last checked at 7c4a93fa8)

These line numbers are navigation hints only and predate the 16 commits up to 41678d976. During S0, resolve each
symbol with `rg` in the fetched checkout; do not edit based on these numeric locations. The type-site census in 1D and
Appendix B supersedes this anchor table.

| What | Where |
|---|---|
| `EntityParabola`; ActionType Modify block | `rs.h:198`; `:433-460` (`ActionModifyAlignRef` :460) |
| `offsetWithOutcome`; copy loop; collection; hand-over | `rs_modification.cpp:1430-1775`; `:1525-1564`; `:1560-1562`; `:1735-1748` |
| `scale` substitution; `trim`; `explode` | `rs_modification.cpp:1843-1858`; `:1999-2108`; `:2759` |
| `RS_OffsetData`; `LC_OffsetBatchLimits`; `LC_OffsetSourceOutcome` | `rs_modification.h:116-141`; `:147-163`; `:200-242` |
| `InsertTransformCapability`; `transformCapability`; `cloneForTransform` | `rs_insert.cpp:88-94`; `:96-164`; `:176-208` |
| `geometryFingerprint`; `arcLengthTable` | `rs_spline.cpp:193-216`; `:878-889` |
| `switchMajorMinor`; `scale`; `mirror` | `rs_ellipse.cpp:581-603`; `:1204-1256`; `:1299-1324` |
| `makeOffsetOptions`; tolerance; `computeScale`; `shrinksAway` | `lc_curveoffset.cpp:4996-5001`; `:4962-4965`; `:601-622`; `:469-494` |
| `addSpline` (main path); `addHelix` | `rs_filterdxfrw.cpp:8034-8139` (`:8106-8112`); `:8148-8152` |
| `writeEntity` (spline case, default); `writeSpline` (R12, `getEntityAttributes`) | `rs_filterdxfrw.cpp:30128-30297` (`:30169`, `:30237`); `:30729-30822` (`:30743-30781`, `:30815`) |
| `setMTextRtlMarker`; empty-group removal; `isLibreCadApp`; XDATA readers | `rs_filterdxfrw.cpp:155-188`; `:167-175`; `:136-139`; `:332-422` |
| `emitRequired("LibreCad")`; `setEntityAttributes` ext data; `getEntityAttributes` tail | `rs_filterdxfrw.cpp:27449`; `:32993-32995`; `:33230-33236` |
| Hatch spline edges | `rs_filterdxfrw.cpp:32105`, `:32173` |
| `fileExport` (public); `writeEntity` (private) | `rs_filterdxfrw.h:2230`; `:2429` |
| `LC_UndoSection` relative-zero step; `undoableAdd`; `undoableExecute`; `setupAndUndoableAdd` | `lc_undosection.cpp:47-57`; `:69-91`; `:110-138`; `:140-160` |
| `librecad_tests` list; DWG / DXF fast lists; excludes | `CMakeLists.txt:2584-2788`; `:3154-3168`; `:3242-3255`; `:2541-2543` |
| `src.pro` HEADERS / SOURCES / FORMS | `:450` / `:1028` / `:2337-2350` |
| Test support | `test/lc_filtertestsupport.h`, `test/lc_sourcefonts.h:42`, `test/lc_actiontestsupport.h:46-116` |
| Default action: hit test, preview switch, Circle case, commit | `rs_actiondefault.cpp:283-319`, `:333`, `:479-495`, `:677-822` |

### E. Review log

#### v1 → v2 (2026-10-14)

- **Base and anchors:** rebased on 7c4a93fa8 (#3090, #3091, #3092, #3098, #3099). 45 anchors moved, all in 11 files;
  none changed meaning. The stale statements about #3091 and the "open" PRs were removed.
- **Executability (critic):**
  - added S0 (worktree, builds, baseline file);
  - 0A deferred to the other session;
  - 1I and 2D split into agent and user rows;
  - the plan file is never staged;
  - status values aligned with the plan-implementer;
  - steps split into session-sized pieces;
  - 1F moved after the UI;
  - dependencies corrected (1E needs 1D and 1C-1; 1G needs 1C-1 and 1D; texts before their users; ActionTypes inside
    2A and 2B);
  - concurrent-work and release-gate sections added.
- **Phase 0 (compiled):**
  - the `static_assert` catches inserted enumerators, not appended ones; MSVC needs C4062 enabled;
  - the factory uses #3098's headless helpers, never `lc::test::application()`;
  - added `reloadsAs`, the `knownGaps`/`notRun` mechanism, and shadow-`rs.h` checks;
  - added `rs.h` to the CI path filter;
  - 0A's tests must gate the crash with `REQUIRE`.
- **Class (compiled):**
  - `LC_OffsetRunLimits` instead of the modification-layer limits;
  - the engine-run counter is always compiled (`BUILD_TESTS` is not defined for the library);
  - normalisation order (full turn first, then axis swap);
  - the `setBasis` fast path restricted to proven motions;
  - mutators re-apply `cacheReversed`;
  - `moveRef` does nothing;
  - the tolerance helper moved into 1A-1 and named `toleranceFor`;
  - the engine-layout precondition; the check uses 360 samples;
  - `geometryFingerprint` as a public static;
  - no laundering of an edited cache in transforms;
  - snaps keyed on "whole", with out-parameters always written;
  - the relative factor of edits fixed; no own "nothing left" guard;
  - the a = b lens guard.
- **Modification and files:**
  - `adopt()` and the copy-from-spline constructor (no second engine run; attributes kept);
  - the side of offset again uses the cache's direction (`cacheReversed`);
  - `addSpline` strips a copy of the record so every exit drops the sub-list;
  - case-insensitive merging of `LibreCad` groups; unbalanced braces stripped;
  - trailing items filtered; adoption preconditions spelled out;
  - `toPlainSpline()` as the sliced copy;
  - the top-level rule;
  - concrete test geometries; CI registration (DXF lane only); checked-in fixtures; copy and paste tests.
- **UI and phase 2:**
  - a refusal-aware property sheet (no comparison; no propagation of a refused value);
  - the grip-drag refusal frees the clone and runs the tail;
  - the phase-2 actions guard context entities and keep the curve's layer and pen;
  - the Edit Properties replace stays unconditional;
  - the qmake commands written out;
  - Qt 6.12 re-baseline;
  - the GUI-pass mechanics;
  - "Original" vs "Source" terminology fixed.

#### v2 → v3 (2026-10-14)

- Rebased the plan's factual baseline from 7c4a93fa8 to local `origin/master` 41678d976; explicitly require a fresh
  fetch, clean implementation worktree and S0 revalidation before code changes.
- Marked Trim Two ownership safety complete from #3113; retained the new entity's missing type-keyed Trim mapping.
- Reconciled offset behavior with #3117 and #3112: replaced the old multi-piece fixture, removed the now-successful
  `a=100, b/a=0.6, net inward 36.036` failure expectation, and made current geometry measurements mandatory.
- Removed pending-work assumptions for APPID registration (#3119), binary XDATA writing (#3116) and leader load
  completion (#3109); added regression coverage for those shared paths.
- Reclassified numeric type-site counts and line anchors as historical; added a required current-tree census and
  post-load completion in coverage tests.
- Updated release gates, workspace/status tracking, risks and the stale-defect appendix; kept the persistent entity,
  serialization schema and phased implementation scope unchanged.

#### v3 → v4 (2026-10-14)

- Made DXF and DWG reading/writing an explicit user requirement and a PR C completion gate.
- Added the complete application version matrix: seven editable DXF versions, six editable DWG versions, and the
  DXF R12 geometry-only downgrade; included binary DXF codec read/write and the existing legacy-reader path.
- Expanded 1E-1 into shared XDATA/EED write preparation and import adoption, with codec payload inspection and export
  failure propagation. Removed stale owned schema-1 metadata when geometry loses its editable definition.
- Renamed the proposed tests to `lc_offsetcurve_file_tests.cpp`; added 1E-3 for every supported version, both conversion
  directions, repeated save/open and edit/save/reopen checks, plus independent-reader evidence when available.
- Extended metadata-fallback tests, GUI save/open checks and phase-2 action persistence to both formats. The GUI test
  now saves the editable entity before checking the exploded geometry.

#### v4 → v5 (2026-10-15)

- Added a completeness/verification matrix, ordered sub-plans, explicit readiness criteria and named child slices for
  geometry, file integration and generic property behavior. Only S0 is ready for execution preflight at this snapshot.
- Added mandatory implementation commits followed by committed self-reconciliation using the real implementation
  SHA. Checkpoints do not record their own hash or recursively create more checkpoints. Recovery preserves committed
  and uncommitted work; automatic slice reverts were removed.
- Replaced line-number failure equality with stable structured test identities, scoped regression gates and tested-tree
  evidence. S0 tracks the canonical plan in the implementation worktree; private prototypes/logs are not prerequisites.
- Strengthened geometry readiness: adaptive bounded whole-span checking, topology/closure coverage, proper full-turn
  classification, complete cache consistency stamps, release-mode guards, one-run/no-op mutation and deterministic fallback.
- Corrected the unreleased schema to use a 1012 major-axis vector, bounded unknown-tail bytes/depth/types, and defined
  safe future-extension behavior after edits. Expanded actual DXF/DWG payload, failed-save and precision tests.
- Added multi-selection refusal/no-op coverage and source-action ownership, large-coordinate equality and persistence
  checks. Historical timing/calendar/algebraic claims were qualified rather than treated as proof of readiness.

#### v5 → v6 (2026-10-15)

- Reconciled the execution checkout/base and recorded actual S0 build/test evidence, separate from historical
  prototypes. Added a current-readiness handoff with explicit ownership and no concurrent Git/shared-build writers.
- Split reference graph analysis from cache validation; specified complete one-component coverage, shared work caps,
  deterministic current-engine fixtures and conservative geometry/topology refusals. No fitting is allowed on load.
- Closed finite-output transform, local grip identity, undefined/inconsistent drag, and circle-deduplication gaps.
- Defined schema Invalid/diagnostics and ownership precedence, redundant flag validation, codec-consistent APPID
  identity, and malformed unknown-version preservation. Expanded both-format safety tests without dropping DWG scope.
- Replaced unsafe pending-clone/boolean property edits with scoped three-way outcomes and one-original/latest-clone
  ownership. Added rapid-edit, layer-after-distance, refusal-after-success and cancellation/undo/redo checks.
- Moved focused phase-2 tests into their implementation slices; corrected the independently mergeable PR C trim gate.
- Strengthened tested-tree verification and the post-commit reconciliation checklist. Product completion still requires
  a verified slice commit followed by its plan-only checkpoint; no recursive checkpoint hashes or false passing gates.

#### v6 → v7 (2026-10-15)

- Reviewed the current tree at `41d83689e` and preserved all four prepared product/test changes. S0, 0B-1, 0B-2
  and 1A-1.a remain done; budget and round-trip patches remain uncommitted/runtime-unverified. Their mixed-tree
  Release/ASan builds completed with exit 0; that is compilation evidence only. No new runtime test was run by
  this documentation pass.
- Added R01-R15 traceability, separate completeness/correctness review gates, executable reference/check/file leaves,
  precise dependency promotion and an immediate validation command queue. Removed stale all-pending/S0-only status
  and clarified historical versus active handoff instructions.
- Gibbs independently audited geometry/readiness/persistence without edits or builds. Integrated all six findings:
  raw-carrier eligibility, contact-loop termination, certified event intervals, restricted-source visibility accounting,
  schema sweep precheck and the hidden GUI selector. The coordinator confirmed current source/API behavior before
  changing the plan; these are planned fixes, not claims of completed product repairs.
- Kept existing DXF/DWG transaction/finalization infrastructure; added strict new-type early-return/R12 refusal and
  target/document-preservation tests. Accounted for the DWG zero-normal sentinel and raw knot/weight repair hazards.
- Tightened isolated-tested-tree proof, staged-path ownership, nonrecursive documentation commits, post-checkpoint
  ancestry/hash/status checks and reopening of newly discovered defects. Every implementation leaf still requires
  its own tested commit followed by a durable plan reconciliation; local commits do not authorize publication.
- Plan-review evidence: `git diff --check` exit 0; `node build/evidence/plan-v7/audit.mjs` exit 0: 27 main rows,
  51 dependency nodes (including aggregates), no cycle/unlisted node, 15 requirement IDs, 14 unique format enums
  (8 DXF/6 DWG), 82 unique headings and 16 balanced code fences. The first inventory probe exposed the missing
  `.b` aggregate row; it was added and the audit passed. Gibbs's second independent read-only review found no
  remaining material contract/dependency issue. No product/runtime-completion status was promoted by this audit.

#### v7 → v8 (2026-10-15)

- Reviewed the actual tree at290d2658fdc2d9bb55da6205ae7c3afaaade6a4a. This documentation checkpoint reconciles the
  already verified accounting slice; it adds no product code and activates no implementation. Its three committed
  file digests match the frozen tested source. Existing completed JUnit reports were reparsed, not rerun: accounting
  Release/ASan5 entries/153 assertions, old core32/2194, full4653/6995171/74 identical skips, combined ASan142/1862858/0,
  DXF1028/39932/3 and DWG1084/34952/4; baseline identities/skips retained, all report audits exit0. Native exits and
  the initial failed-test correction remain in the executed card. Declaration/oracle probes remain limited evidence.
- Completeness: reconciled all affected status/ledger/parent/queue entries, retained R01-R15 and all14 format rows,
  and kept entity/file/UI/phase-2 completion open. Corrected historical text that still described committed strict
  mode/rate/progress work as missing; ordinary-mode heuristics are not strict proof machinery.
- Correctness: independent Gibbs read-only review found four actionable gaps, all added to the owning cards and §7:
  critical budget identity/reset, terminal provider/proof invalidity, bounded compound rounds and full interval inverse
  residuals, and an exact-special-case-only fixture. These are planned fixes, not completed runtime repairs.
  Gibbs's second read-only pass found no remaining blocking contradiction in those revised contracts; critical public
  declarations and constant cost qualification remain explicit future entry gates, not evidence of runtime readiness.
- Sub-plans/readiness: core retained-session API is declaration-qualified with exact paths, lifetime/move rules,
  original proof/history preservation and negative tests. Critical-occurrence refinement is a separate waiting
  geometry child; literal declarations and constant cost model must be qualified before activation. Isolation waits
  for BOTH; pair-root handles cannot replace critical-root provenance or physical-node accuracy proofs.
- Slice commits: verified-tree digests, explicit staging, enabled hooks and independent review precede each product
  commit. The compact operator checklist distinguishes active processes, uncommitted validated work, implementation
  commits awaiting reconciliation and completed checkpoints. No timeout authorizes competing work or reimplementation.
- Self-update: every product SHA drives an immediate plan-only checkpoint, parent/requirement status review, dependency
  and next-card recalculation, ancestry/digest/status checks. Checkpoints do not recursively record their own SHA.
  A plan-only request leaves the next card inactive; authorized implementation resumes with Gibbs at actual HEAD.
- Structural validation: `node build/evidence/plan-v7/audit.mjs` checks the documented dependency mirror and inventory;
  27 main rows,58 nodes,15 requirements,14 unique format enums(8DXF/6DWG),91 unique headings,24 balanced fences, acyclic.
  The dependency mirror was cross-reviewed against §0.3/§0.6; this audit is not an automatic proof of every prose
  dependency or any numerical algorithm. `git diff --check` passes. No new native build/test run was needed for this
  documentation-only review, and no new runtime coverage or full-plan completion is claimed.

**v8 execution reconciliation, critical slice72eb5f953.** Completeness: all touched critical APIs/build registrations/
tests accounted for, R01-R15 and14formats retained, parents still incomplete. Correctness: separate source/math review
and native red/failed reproductions resolved inverse-domain/exclusion and one-sided join defects, no weakened caps/
assertions. Readiness: both refinement predecessors are done; isolation is conditional on its own literal API/proof/
cost preflight, not ready by assertion. Commits: three frozen reviewed/tested product paths only at actual SHA above,
committed digests independently verified; this checkpoint changes only the roadmap. Self-update: all active status/
queue/ledger/card references reconciled, failed evidence retained, next activation must use actual checkpoint HEAD.
Structural audit passes:27main rows,58nodes,15requirements,14formats(8DXF/6DWG),91headings,28balanced fences,acyclic;
this audits the mirrored graph/inventory, not numerical correctness. No full reference/entity/file/UI completion claim.

**v8 execution reconciliation, provider slice9c27e2980.** Completeness: additive owned-provider APIs, all touched
paths/build registrations/tests and collapsed/absent roles accounted for; main aggregate row corrected to include
already completed critical refinement as well as providers. R01-R15 and14format rows retained; later phases open.
Correctness: independent geometry/domain/lifetime/resource review and full native gates pass without old-body/cap
changes. Sixteen focused cases/299294 assertions pass Release/ASan; full4704/7311379/74same skips,combinedASan
193/2179066,both fast lanes pass, frozen/committed digests equal. Readiness: provider predecessor done, isolation
conditional on its own contact/order/fixture/API/cost evidence, not ready by assertion. Commit: product changes
exactly three tested paths; this reconciliation only the roadmap, no recursive checkpoint hash. Self-update: active
statuses/queue/ledger/card and risks refreshed; failed scaffold log retained; next activation uses actual checkpoint
HEAD. Independent whole/restricted visibility identity review plus standalone113-bit72675-check oracle adds a bounded
future proof option, NOT a product implementation or substitute for complete topology/trim allowances. Structural
audit passes27mainrows,59nodes,15requirements,14formats,92headings,30balancedfences,acyclic; diff-check0.

**v8 execution reconciliation, contact slice92ca5e445.** Completeness: both-role/end proof APIs, old factory
compatibility, build/test paths, failures/ownership/cost/history accounted for; R01-R15/14formats and all later phases
retained. Correctness: independent exact-predicate/inventory/continuity/radial/support/full-domain review, const ledger
seal and native extra-transverse-root regression pass. Focus17/4383both,old97/586799both,geometry378/2474727,
ASan210/2183449,full4721/7315762/74same skips,bothfast0; tested/committed digests equal. Successful interior fallback
is NOT natively covered: negative base-refusal test and bounded121506-evaluation NO_MATCH search remain distinct,
source proof qualified and broader1A-1.c carries the reachability investigation. Readiness: contacts done, isolation
conditional own literal API/cost/identity/order/world-node-accuracy gate. Six strict-query families now observedOk,
but initial world images coarse to28.479, not complete combined geometry events/accuracy. Commit: exactly three
tested product paths at actual SHA, this checkpoint only roadmap; no recursive own hash. Self-update: status/main
aggregate/queue/ledger/card/risks reconciled, no stale implementation restart; use actual checkpoint HEAD next.
An initial reconciliation patch failed context validation and changed no file; corrected targeted patches applied,
no product/native result affected. Structural audit0:27mainrows,60nodes,15requirements,14formats,93headings,
30balancedfences,acyclic; diff-check0. No reference/cache/entity/file/UI/phase2 or whole-goal completion claim.

#### v9 → v10 (2026-10-15)

- Completeness: reconciled the current event-isolation commit/evidence and the remaining shared-own-occurrence coverage
  limitation; the component leaf now names all required files, requirement IDs, selectors, fixture outcomes, budget
  cases, error exits and downstream checker/engine-compatibility ownership. DXF/DWG, entity, UI and phase-2 work remain
  open.
- Correctness: integrated Gibbs and Anscombe's readiness findings. Root parameters are enclosures, so each root fringe
  must be contracted using its issuer and bounded as a whole before it can be attached to a node. Physical components
  are weakly connected components of an incidence-preserving directed multigraph; no successor/pairing is guessed at
  crossings, and branchy components are not assumed to be cacheable paths. Exact physical-D visibility is normative;
  rho, evaluation/marginal/dangling/tiny engine heuristics and the rho-floor bypass are kept separate and cannot change
  reference topology. Crossing count/connectivity differences force plain geometry rather than a tolerance exception.
- Sub-plan/readiness: added an activation card with exact product paths, existing API facts, owner roles, private
  continuation/budget invariants, ordered tasks, expected six-fixture outcomes, a shared-own success fixture, literal
  Release/ASan/full/fast-lane commands, independent-oracle requirements, digests and recovery rules. It becomes active
  only after this plan-only checkpoint and exact-HEAD reconciliation.
- Commits: restated one named executable leaf per product commit; each verified implementation commit is followed by
  exactly one plan-only checkpoint, never bundled with the next product slice. Failed reports and unrelated work stay
  preserved; hooks, explicit path staging and tested-tree digests remain mandatory.
- Self-update: the post-commit checklist remains the authoritative reconciliation loop and now names the component
  contract's parent/readiness consequences. Product and plan commits have separate receipts; the checkpoint records
  the product SHA, not its own SHA, then the next leaf activates from actual HEAD.
- Plan-only validation: `git diff --check` and the structural audit passed; 27 main rows, 60 dependency nodes, 15
  requirements, 14 format rows (8 DXF/6 DWG), 94 headings, 32 balanced fences, acyclic. No product code, runtime
  verification or component completion is claimed by this plan update.

#### v10 → v11 (2026-10-16)

- Completeness: made requirement-to-code/caller/format/build/test/evidence traceability an explicit per-slice artifact;
  a placeholder, unreachable test, or unregistered path cannot satisfy a feature row. Reconciled the main status and
  `.b-ref-components` recipe with the actual dirty three-path worktree instead of the prior ready-card snapshot.
- Correctness: separated review sign-off from completeness and required reviewer/tree identity, proof assumptions,
  domain coverage, independent falsifying oracle, and actual serialized bytes plus reload behavior where applicable.
  Recorded Release attempt 6 as failing evidence: the aggregate stationary flag is not present in those six fixtures,
  while the dedicated exact-threshold case owns that assertion; the required shared-own-occurrence success case still
  reaches depth 20 and remains a correctness blocker. The isolated-equality test passes but does not close the graph.
- Sub-plan/readiness: changed the card from `ready` to `in progress`, fixed its actual base at `c3f091c3f...`, and
  specified ordered repair tasks, one-writer file ownership, blocker closure and non-activation of `.b-check-layout`.
  No downstream slice may start from a subset or a bounded refusal where success is required.
- Slice commits: clarified that each executable leaf has exactly one product commit after its full evidence and two
  reviews; independently shippable work must be split before implementation. A red run remains in the same slice,
  with failed evidence preserved and no progress-only product commit.
- Self-update: every implementation commit now requires a plan-version/date bump and synchronized header, status row,
  parent, requirements, recipe, queue and next card. The plan checkpoint audits the actual code diff and recomputes
  readiness from implementation plus checkpoint SHAs; stale ready labels and unresolved blockers are forbidden.
- Validation: documentation-only change; `git diff --check` and the structural inventory/dependency audit pass
  (28 main rows, 60 dependency nodes, 15 requirements, 14 format rows [8 DXF/6 DWG], 95 headings, 32 balanced fences,
  acyclic). This audits the mirrored graph/inventory, not every prose dependency or numerical claim. No product code,
  runtime verification, or `.b-ref-components` completion is claimed.

#### v11 → v12 (2026-10-16)

- Correctness review of the live component builder found that graph assembly derives components only from retained
  span edges and skips nodes with no incoming/outgoing edge. The existing whole-source isolated-equality refusal is
  narrow; it does not establish that all restricted-source, endpoint-cap or pair-tangency event points are non-retained.
- Added an explicit zero-edge event-node audit to the general correctness gate and the component graph stage. Each
  such node must be proven non-retained, represented as a singleton if the API supports it, or cause a no-partial
  ambiguity refusal; edge-only assembly may not silently omit it.
- Readiness remains in progress; this is a correctness obligation to investigate, not a claim that a concrete second
  isolated-point counterexample or implementation defect has already been measured. Existing uncommitted code and
  failed Release attempt 6 remain preserved; no product files were changed by this plan refinement.
- Validation: documentation-only change; `git diff --check` and the structural inventory/dependency audit pass
  (28 main rows, 60 dependency nodes, 15 requirements, 14 format rows [8 DXF/6 DWG], 96 headings, 32 balanced fences,
  acyclic). This does not validate every prose dependency or numerical claim. No product build/test result or
  component completion is claimed.

#### v12 → v13 (2026-10-16)

- Split the review contract into five explicit tracks: completeness and plan-contract correctness; independent
  implementation correctness; executable sub-plan/readiness; one verified implementation commit per slice; and an
  immediate, separate plan-only self-update commit after each implementation commit.
- Added a dedicated plan-correctness review to the slice template and §0.7. It checks current source/API/spec facts,
  acceptance rules, fallback behavior and dependency consistency before a leaf is called ready; implementation review
  separately checks the code against that contract. Coverage, plan correctness and implementation correctness now
  have distinct recorded outcomes.
- Tightened readiness to reject stale API/base assumptions and unresolved design decisions. Investigations must have a
  bounded question and observable closure test; each leaf names ordered tasks, exact validation identities, owners,
  evidence, recovery and its commit boundary before implementation.
- Reaffirmed that commits happen only after all required validation and review for one executable slice, with no
  passing-subset commits or product/plan bundling. After that commit, the plan must immediately reconcile actual code,
  evidence, requirements, parent status, risks and next readiness, then land as a separate nonrecursive checkpoint.
- Reconciled the active component card to the five files currently modified, including the parametric-intersection
  helper pair. Removed stale attempt-6 hashes and labels from current-state claims; its failures remain preserved as
  historical evidence, and the current source remains unvalidated until frozen and rerun.
- Refined the isolated-node contract to audit only own-role nodes; opposite/cutter-only events are not own-span graph
  vertices. Required non-vacuous evidence for the hidden-own-node path (or a proof it is unreachable), and added a
  correctness gate for exact axis-to-parameter enclosures near wrapped/reversed tiny sweeps.
- Validation: documentation-only change; `git diff --check` passes. Structural counts: 28 main status rows, 15
  requirements, 14 format rows (8 DXF/6 DWG), 24 executable recipe rows, 100 headings and 32 balanced fence lines.
  Dependency edges were not changed; this update does not claim a fresh full prose/DAG audit. No product files changed,
  and no implementation test or component completion is claimed.

#### v13 → v14 (2026-10-16)

- **Completeness and plan correctness before implementation:** added a mandatory five-step readiness decision order.
  It starts from the actual checkout, traces each requirement through callers, representations, formats, build targets,
  tests and evidence, validates API/spec/dependency contracts, then records an explicit ready/conditional/waiting
  decision. Investigations now require a bounded question, probe, outcome rule and unresolved-work disposition.
- **Correctness after implementation:** clarified that preflight reviews cannot approve future code. Final completeness
  is reconciled against the actual diff; plan correctness is rechecked when runtime evidence changes assumptions; an
  independent implementation review challenges the frozen diff, negative paths and independent oracle before commit.
  The active component slice began before this protocol and is not credited with a retroactive preflight; it must still
  close final reviews, frozen-tree validation and its blockers before commit.
- **Executable sub-plans and readiness:** extended the activation card with a recorded readiness decision and made the
  gate order explicit. A leaf is not ready while contract, dependency, acceptance, oracle or stopping-rule questions
  remain unresolved. If investigation changes the contract, affected cards and downstream dependencies return to
  review before implementation continues.
- **Commits after slices:** specified exactly one product commit per executable leaf, never an aggregate or a bundle
  of leaves. Product work cannot move to the next leaf until its plan-only reconciliation checkpoint is committed; a
  failed checkpoint stops product work while preserving the implementation commit and pending plan changes.
- **Self-update after commit:** expanded the mandatory audit to reconcile status, parent aggregates, R01-R15, format
  matrix, recipes, evidence, queue, next card and version history from actual implementation/checkpoint SHAs. Require
  acyclic dependencies, real predecessor links, no stale readiness/completion claims, `git diff --check`, and an
  explicit structural/DAG audit (or a recorded manual audit if no checker exists).
- Validation is documentation-only; no product files, component status, implementation evidence or commit state are
  changed by these plan requirements. `git diff --check` passes. Structural inventory: 28 main status rows, 15
  requirements, 14 format rows (8 DXF/6 DWG), 25 executable recipe rows, 111 headings and 32 balanced fence lines.
  No repository plan/DAG audit script was found; dependency edges are unchanged by v14 and were checked for this
  revision, but this is not a full prose/dependency audit. No product tests or component completion are claimed.

#### v14 → v15 (2026-10-16)

- **Completeness:** made ASCII DXF, binary DXF codec, native DWG and DXF↔DWG conversion distinct format lanes.
  Replaced the combined 14-row save/open wording with independent reader and writer expectations for each version;
  retained R12's geometry-only downgrade and legacy DWG input-only boundary. R07 now requires separate per-version
  receipts plus binary DXF codec coverage.
- **Correctness:** a same-implementation round trip can no longer stand in for reader evidence. Added independently
  sourced DXF, DWG and binary DXF input-fixture requirements with producer/version/hash provenance; separate writer
  payload inspection; exact file-version checks; and an explicit incomplete status when independent reader fixtures
  are unavailable. Conversion fan-out remains validated in both directions.
- **Sub-plan/readiness:** aligned 1E-1 smoke tests, 1E-2 fixture roles, 1E-3 per-version receipts, the executable
  recipes and the PR C gate to the four independent application directions. Binary DXF remains explicitly low-level
  at R14/R2018 because the application export interface currently writes ASCII only.
- **Current-state reconciliation:** corrected `.b-ref-components` from stale in-progress/pending wording to the
  attempt-8 validated, single-implementation-commit-ready state. Preserved attempt 6 as historical red and attempt 7
  as pre-fix green; downstream `.b-check-layout` remains waiting for the implementation commit and plan checkpoint.
  The five product/test-file digests were rechecked and still match attempt 8; no product file was changed here.
- **Validation:** documentation-only. `git diff --check` passes. Inventory: 28 main status rows, 60 dependency nodes,
  15 requirements, 14 application format rows (8 DXF/6 DWG), 24 executable leaf recipes (25 table rows including
  the header), 102 Markdown headings and 32 balanced fence lines. No in-repository plan/DAG checker exists; dependency edges are unchanged and the current
  mirrored inventory is acyclic. No product build or test was run for this plan update.

#### v15 → v16 (2026-10-16)

- **Implementation commit:** completed `1A-1.b-ref-components` as `9682d5cac65f30cfd86aa10556678ceb88b1448b`,
  containing only the five frozen product/test paths (1,430 insertions, 12 deletions). The committed file digests
  match the attempt-8 validation identity; no product edits followed validation.
- **Evidence and correctness:** re-read the attempt-8 JUnit suite headers: focused Release/ASan 21,557 assertions,
  reference regression 11,559, engine regression 727,115, full Release 7,342,935 with 74 unchanged skips, DXF fast
  Release/ASan 39,932 with 3 unchanged optional skips, and DWG fast Release/ASan 34,952 with 4 unchanged optional
  skips. All report errors/failures are zero. Attempt 6 remains historical red; attempt 7 remains pre-fix green.
- **Self-update:** marked the implementation and `.b-ref`/`.b-ref-events` aggregates complete at the actual SHA;
  reconciled the parent `1A-1` as partial with `.b-check` and `.c` still pending; updated the active card, evidence,
  recovery and immediate queue. `.b-check-layout` remains waiting until this plan-only checkpoint is committed and
  its activation base/API contract is re-read. No next product leaf is bundled with this commit.
- **Validation:** `git diff --check` passes; committed code hashes match the frozen card; the v15 exact version of the
  product/test tree is unchanged. Documentation-only plan checkpoint apart from the previously committed product
  slice; no new product build/test was run after the commit because its files and bytes did not change.
  Structural inventory: 28 main status rows, 60 dependency nodes, 15 requirements, 14 application format rows
  (8 DXF/6 DWG), 24 executable leaf recipes (25 table rows including the header), 103 Markdown headings and 32
  balanced fence lines. No plan/DAG checker is available in-repo; dependency edges did not change and the mirrored
  graph remains acyclic.

#### v16 → v17 (2026-10-16)

- **Completeness:** reconciled the completed `.a` implementation with its plan section. `isEngineLayout`, cache
  Bezier extraction and `checkCache` were still listed as `.a` deliverables despite not existing in the committed
  slice, and duplicated the later `.b-check` scope. `.a` now names only implemented geometry/normalization/tolerance
  work; layout, bounds and topology are assigned to their executable `.b-check` children. Updated the current
  `.b-ref` aggregate to complete and `.b-check` to pending rather than waiting on a completed predecessor.
- **Plan correctness:** checked current `RS_SplineData`, `RS_Spline::validate`, `LC_OffsetCheckLimits`/`WorkBudget`,
  and `LC_CurveOffset` output construction at the v16 checkpoint. The producer's cubic path uses ClampedOpen degree 3,
  grouped knots and complete unit weights, while its all-straight path can emit a line or degree-1 spline; the latter's
  reachability for admitted ellipse inputs was not proven. The plan now treats it as a bounded entry probe whose
  disposition must be checked against R05, not as an assumed cubic success or automatically permitted plain fallback.
  The normalized checker requires complete unit weights; only the
  separate raw-file eligibility path may materialize absent weights. Knot reparameterization, exact repeated groups
  and closure-flag responsibilities are stated consistently between layout and topology.
- **Sub-plan/readiness:** added a waiting `.b-check-layout` candidate card with exact predecessor/code-base facts,
  source/API scope, probe outcomes and stopping rules, ordered tasks, no-allocation/counter contract, positive and
  negative cases, minimum registered selector count, Release/ASan and adjacent/full/fast validation, recovery and one
  product commit boundary. It is deliberately not marked ready; activation requires the v17 plan checkpoint and a
  fresh actual-HEAD/source check. No product edits are authorized by this plan-only update.
- **Commits after slices:** tightened the five-gate table with explicit pass artifacts, tested-tree/digest identity,
  clean path-scoped index requirements and a hard stop on failed review/validation. Added explicit product-commit and
  plan-checkpoint postcondition fields to the activation-card template; the existing one-leaf/one-product-commit rule
  and separate immediate plan-only checkpoint remain unchanged.
- **Self-update after commit:** made completeness, plan correctness and implementation correctness distinct records;
  requires actual commit/diff reread, synchronized status/requirements/matrix/DAG/evidence/queue, recalculated readiness,
  plan-only checkpoint and ancestry/path/digest/worktree verification. Updated the top status, immediate queue and
  current card without rewriting historical attempt evidence.
- **Validation:** documentation-only. Product files and implementation evidence are unchanged; no product builds or
  tests were run. `git diff --check` passes. Inventory: 28 main status rows (29 table rows including the header), 15
  requirements, 14 application format rows (8 DXF/6 DWG) plus one conversion row, 24 executable recipe rows (25
  table rows including the header), 104 headings and 34 balanced fence lines. No dependency edges changed; the
  affected chain `.b-ref-components → .b-check-layout → .b-check-bounds → .b-check-topology` was checked and is
  acyclic. No in-repository plan/DAG checker exists, so the full graph audit remains a mandatory activation/checkpoint
  gate. The existing untracked `build-asan/` directory is preserved and not part of the plan diff.

#### v17 → v18 (2026-10-16)

- **Completeness:** traced the producer's intentional straight-output forms through R05/D16, cache checking, schema
  import eligibility, DXF/DWG version lanes, cross-format conversion and R12 geometry-only export. Both degree-1 and
  degree-3 normalized cache layouts now have explicit owners and assertions; valid line output is no longer an
  untracked fallback/drop case.
- **Plan correctness:** inspected the current `chainEntity` implementation, `RS_SplineData`, `RS_Spline::validate`,
  checker budget call sites, persistent adoption contract and SPLINE codec paths. Chose exact `RS_Line` -> degree-1
  ClampedOpen normalization at the 1B engine-result boundary; checker accepts degree 1 or 3, with checked respective
  control/knot formulas and extraction; line spans use exact segment bounds and cubics retain conservative bounds.
  The cache check budget is renamed to `maxCacheSpans` without renaming the separate producer-output budget. Raw
  file eligibility and the 14 reader/writer lanes now require degree-1/degree-3 coverage; R12 explicitly stays
  geometry-only. No finite-probe claim is used to erase a producer branch.
- **Sub-plan/readiness:** rewrote `.b-check-layout` as a degree-1/degree-3 layout slice with exact line normalization
  assigned to 1B, checked-arithmetic cases for both formulas, a 10-case minimum selector count, exact-linear and
  cubic oracles, expanded failure review prompts and a post-v18 activation gate. It remains `waiting` until this
  plan checkpoint is committed and actual HEAD/callers are refreshed; no code was changed.
- **Correctness:** distinguished exact linear-span bounds from adaptive cubic bounds, retained affine knot-domain
  support and exact repeated knot groups, and kept topology/closure semantics out of layout validation. Degree-1
  adoption must not be silently downgraded to plain geometry, while malformed/unsupported layouts still refuse.
- **Commits and self-update:** the five gates remain separately auditable: requirement completeness, plan-contract
  correctness, implementation correctness, executable readiness, and verified product-commit plus immediate
  plan-only self-reconciliation. Updated the active card, current queue and version history; historical validation
  receipts were left untouched. The required slice cycle still freezes source/digests, commits exactly one tested
  leaf, then separately commits the plan with the product SHA and checkpoint postconditions.
- **Validation:** documentation-only; `git diff --check` passes. Inventory: 15 requirements, 14 application format
  rows (8 DXF/6 DWG) plus one conversion row, 24 executable leaf recipes (25 table rows including the header), 105
  headings and 34 balanced fence lines. The 13 schema-capable application reader versions require independent
  degree-1/degree-3 fixtures; R12 and legacy DWG R13/R14 have separate read-only contracts; binary DXF R14/R2018
  remains a distinct codec lane. The affected chain `.b-ref-components → .b-check-layout → .b-check-bounds →
  .b-check-topology` is acyclic; 1B line normalization has no dependency edge back into the layout prerequisite. No
  in-repository plan/DAG checker exists, so the full graph audit remains a readiness/checkpoint requirement. No
  product build or test is claimed. Existing untracked `build-asan/` is preserved.

#### v18 → v19 (2026-10-16)

- **Completeness:** closed the `.b-check-layout` producer-shape probe against `createEntities`/`materializeBranches`
  and `chainEntity`: all entity forms are line, degree-1 spline or degree-3 spline; each has a named editable-path
  disposition. Confirmed the persistent class is future 1B work, line normalization is explicitly assigned there, and
  no current consumer or test registration changes the layout leaf's file scope.
- **Plan correctness:** re-read current source at `127231e41e20c4b434c0800bbde98dcb16f55ac3`: producer routing,
  `RS_Spline::setDegree`/`validate`, all `LC_OffsetCheckLimits` uses and the separate engine-output budget. Confirmed
  the degree-specific knot/control formulas, checker-only cap rename, all-or-none combined budget preflight, and
  line-normalization boundary. No source change or unresolved contract choice remains for this leaf.
- **Sub-plan/readiness:** changed `.b-check-layout` from waiting to ready at source base `127231e41`; added exact
  inspected files/lines, selector target/count, cost model, evidence/frozen-identity/recovery fields, and separate
  preflight completeness/plan-correctness passes. Gibbs owns exactly the three geometry header/implementation/test
  paths; the future 1B line adapter is a downstream caller and not an entry blocker. Readiness expires on any
  product source/API change.
- **Implementation correctness:** remains pending by design until the implementation diff exists. The card names
  independent count/overflow, layout, extraction, knot-reparameterization and refusal-atomicity challenges; the
  linear/cubic oracles remain distinct.
- **Commit and self-update:** this is a plan-only activation checkpoint. The next product slice still requires a
  frozen three-path identity, passing focused/adjacent/full/fast checks, closed final Gate 1/2 reviews and exactly one
  product commit. Its immediate follow-up must be a separate plan-only reconciliation with exact product SHA, evidence,
  parent/readiness updates and verified ancestry/path scope before further product work.
- **Validation:** `git diff --check` passes; the preflight command
  `QT_QPA_PLATFORM=offscreen build/librecad_tests '[curve-offset][direct][materialize]' --order decl` passed 5 cases /
  226 assertions. No product file changed. Inventory: 15 requirements, 14 application format rows plus one
  conversion row, 24 executable leaf recipes (25 rows including header), 106 headings and 34 balanced fence lines.
  The affected cache-check chain remains acyclic. Existing untracked `build-asan/` is preserved.

#### v20 → v21 (2026-10-16)

- **Completeness:** replaced the underspecified `.b-check-bounds` matching recipe with explicit candidate-span/reference-
  fragment coverage, visible-edge interiors, exact occurrence fringes, event nodes, candidate-span witness completeness,
  conservative pruning, unresolved-pair refusal and the topology consumer's refinement handoff. Kept topology,
  persistence, file formats and UI outside this bounds slice; they remain separately owned and incomplete.
- **Plan and implementation correctness:** specified symmetric setwise chord-Hausdorff bounds from cubic de Casteljau
  residual controls, local exact-offset interpolation error `M2*h^2/8`, outward interval threshold comparisons, exact
  restricted-source clearance and explicit fringe/node error accounting. Independent review exposed the traversal gap
  in setwise proximity; `.b-check-topology` now separately requires strict local monotonicity/injectivity, exact event
  incidence and a complete ordered non-reusing assignment. Follow-up review required consistent node-clearance terms,
  strict interval-separated endpoint projections, and an explicit degree-1/constant-span disposition. The frozen
  implementation correctness gate remains future work and cannot be approved by this plan review.
- **Executable sub-plan/readiness:** added a file-scoped activation card with exact base and source hashes, owner and
  dependency boundaries, current APIs, ordered implementation tasks, registered selector and oracle requirements,
  deterministic work accounting, refusal cases, validation sequence, recovery policy and explicit pass artifacts.
  Independent reviews by Anscombe closed plan correctness and completeness at the recorded product base. Their final
  completeness pass required explicit all-visible-witnesses-separated refusal, cross-stage same-ledger tests, and
  distinct crossing/cusp/seam incidence assertions; all are in the card. Activation checkpoint `138694560926cbb8e4cb34198fae83dcb6bb4c9a` and the separate ready receipt complete the pre-implementation gates; the ready receipt stores no recursive self-hash.
- **Commits after slices:** retained one frozen/tested product commit per leaf, with exact path/digest identity and no
  bundling of topology or caller work into the bounds commit. A failed review, test or reconciliation stops the queue;
  preserve evidence and do not start a successor slice.
- **Self-update after each product commit:** require immediate source/diff/evidence reread and a separate plan-only
  checkpoint updating slice status, requirements, aggregate state, dependencies, format/build/test matrices, receipts,
  queue, readiness and next-card base. Verify checkpoint ancestry, path scope, product hashes, DAG and worktree before
  resuming product edits. Checkpoints never recursively hash themselves.
- **Validation:** documentation-only; no product files changed and no bounds implementation is claimed. Fresh existing
  `[cache-layout]` Release and ASan baselines each pass 16 cases/3,311 assertions with zero skips/failures. Independent
  plan reviews found actionable clearance/endpoint and completeness-test gaps; all were corrected, then Anscombe's final
  read-only math and completeness passes found no remaining plan blocker. `git diff --check` passes. The documented
  inventory/DAG audit passes with 28 main rows, 60 dependency nodes, 15 requirements, 14 format rows (8 DXF/6 DWG),
  36 balanced fences, 104 headings and an acyclic dependency graph; its archived helper's old matrix-heading selector
  was adapted in the invocation, not modified. Activation checkpoint `138694560926cbb8e4cb34198fae83dcb6bb4c9a`
  directly follows `6f25975f6` and passes path/ancestry/product-digest checks; a separate ready receipt follows it.
  Existing untracked `build-asan/` is preserved.

#### v24 → v25 (2026-10-16)

- **1. Completeness and plan correctness:** reconciled the bounds leaf against the committed three-path diff rather
  than its former pending card. R02/R05/R12/R13 now trace through the bounded witness relation, every visible/hidden
  alternative, the cusp/trim/crossing/seam incidence cases, shared work ledger, refusal atomicity, test registration,
  and the explicit handoff to topology. The noncircular reference oracle now proves outward coordinate range and
  endpoint-chord residual over the entire covered domain, with independent Q(a)/Q(b) checks mapped to all four
  returned endpoint fields. The plan distinguishes geometric proximity from injectivity, order, reverse coverage and
  public cache validity; those remain owned by `.b-check-topology` and later leaves.
- **2. Correctness:** records the independent Anscombe final review on the frozen product commit, with no actionable
  finding. The correctness contract covers interval direction/rounding, full-domain partition coverage, wrapped and
  reversed angle mapping, endpoint-coordinate association, finite arithmetic, conservative pruning, clearance,
  budget/lifetime and no-partial refusal. The earlier v22-v24 findings are recorded as resolved by the implemented
  cusp/trim tests, exact-rational whole-interval oracle and four endpoint assertions; tests and sampled cross-checks
  are not described as the proof itself.
- **3. Executable sub-plan and readiness:** changed `.b-check-bounds` from pending to done, records its exact code
  parent (`a0ce67a3b8746770ce49fb1ff76616c9da3d23fe`), activation/amendment checkpoints, three owned paths, measured
  selectors, review outcomes and evidence identities. Updated `1A-1` and `.b-check` aggregate states. The next leaf is
  explicitly waiting, not implicitly ready: after this checkpoint, re-read actual HEAD and current bounds-result
  callers, close completeness and plan correctness, write its bounded task/ownership/oracle/resource/validation plan,
  then record a fresh readiness decision before Gibbs edits.
- **4. Commit after the slice:** product commit `ce1ed51924d20a708e8bec059f7f9dd46235a232` directly follows the v24
  endpoint-contract plan checkpoint and contains exactly the three declared product/test files, 1,485 insertions.
  The card records frozen SHA-256 identities and attempt-specific focused, adjacent, full and fast-lane receipts; no
  plan/evidence file was mixed into the product commit.
- **5. Self-update after committing:** this v25 edit is the immediate, separate plan-only reconciliation. Its
  required checkpoint is the direct child of the product commit and must contain only this roadmap. Before further
  product work, verify the checkpoint parent/path, `git diff --check`, unchanged product digests, full worktree status
  and preservation of the existing untracked `build-asan/`; do not recursively store the checkpoint's own hash.
- **Validation:** final focused Release/ASan each pass 25 cases/46,104 assertions; adjacent Release/ASan each pass
  145/757,599; full Release passes 4,793 cases/7,392,350 assertions with 74 skips; DXF fast Release/ASan each pass
  1,028/39,932 with 3 skips; DWG fast Release/ASan each pass 1,084/34,952 with 4 skips. All failures/errors are zero,
  and skip identities match baselines. Documentation consistency, diff scope and final digest checks are run before
  the plan-only checkpoint; no broader feature completion is inferred from this bounds slice.

#### v25 → v26 (2026-10-16)

- **1. Completeness and plan correctness:** verified the v25 checkpoint `4ca63677c27043004aae45e0fe54d42db48664a5`
  is the direct one-file child of product commit `ce1ed51924d20a708e8bec059f7f9dd46235a232`; its product hashes,
  diff-check and preserved untracked `build-asan/` match the completed card. Re-probed actual APIs and callers: bounds
  owns layout/reference/witness data but not `WorkBudget` or pair thresholds; no production `checkCache` caller exists;
  reference components are source-parameter ordered without implicit crossing successors; CMake already registers the
  geometry tests. R02/R05/R12/R13 remain incomplete until the global chain leaf and later adoption callers finish.
- **2. Correctness:** split the previous single topology sketch into local proof and global assignment. Local proof now
  must re-run geometry/clearance bounds when refining, prove candidate/reference endpoint pairs as well as setwise
  distance, and treat fringe/node witnesses as zero-measure endpoint-incidence evidence, never positive-length
  coverage. An independent review found the first draft did not forbid flipping a local cell's orientation; v26 now
  fixes normalized candidate-start/reference-lower and candidate-end/reference-upper pairing across every split and
  explicitly rejects a same-image reversal even if its subpieces pass setwise geometry. It has exact degree-1/cubic
  derivative conditions, strict outward reference endpoint separation, consistent `cacheReversed` order,
  root-refusal policy and no-partial failure. Added exact-binary folded cubic `[0,1,-1/4,1]` with derivative roots
  `(9±sqrt(5))/19` as an independent same-image reversal oracle. Removed public `TooNear`: the present certificate
  has a clearance lower bound, not an upper-bound proof of actual nearness. The global child owns component
  count/order, event transitions, two-way coverage, closure flags, bounded assignment and reason mapping. Independent
  read-only follow-up review confirms the orientation gap is closed and reports no remaining plan-correctness blocker.
- **3. Executable sub-plan/readiness:** closed Gates 1 and 3 for `.b-check-topology-local` at unchanged product base
  `4ca63677c`; v26 was committed as `fd4697059572af28c08ecbb532e687af1603ee1c`, its direct child. Recorded exact three-file
  ownership and SHA-256 baselines, current APIs, pair-helper refactor contract, move-only local result semantics,
  12-case minimum, selector, independent oracle, work ledger, Release/ASan/full/fast/qmake6 app-build commands,
  recovery and one commit boundary. `.b-check-topology-chain` is explicitly `waiting` for the local commit/checkpoint
  and fresh actual-source/API/readiness review; its cycle-start alignment is a named bounded entry probe, not an
  assumed behavior.
- **4. Commit after each slice:** local topology gets exactly one product commit (`Prove local offset-cache topology`)
  after final diff reviews, frozen hashes and all required tests/builds pass. Its three files only may be staged. The
  global chain/public validator, entity integration, serialization, UI and plan are excluded from that product
  commit. A failed stage is preserved and fixed in the same leaf; no passing subset is committed.
- **5. Self-update after committing:** require immediate separate v28 plan-only reconciliation before any next product
  edit. It must read the actual local product commit and reconcile requirements, evidence, parent/aggregate state,
  DAG, risks, queue and global-chain readiness; verify it is the direct child and owns only this roadmap, product
  hashes are unchanged, worktree state is preserved, and `git diff --check` passes. The checkpoint does not store its
  own hash recursively.
- **Validation:** plan-only source/API probes at `4ca63677c` confirmed 25 enumerated cache-bounds test cases, 110 test
  functions in the adjacent `--list-tests` union (its saved JUnit receipts are 145 invocations/757,599 assertions), qmake6 at
  `/opt/homebrew/bin/qmake6`, and test registration at `CMakeLists.txt:2612`. The three product-file SHA-256 values
  equal the v25 committed card. Reconciled all adjacent-suite receipt labels to distinguish listed test functions from
  JUnit invocations. No product code/tests/builds were changed or claimed passing in v26. The v26 plan-only checkpoint
  is committed as `fd4697059572af28c08ecbb532e687af1603ee1c`, a direct child of `4ca63677c`; it contains only this
  roadmap. Product hashes matched the v25 card at that checkpoint, and `build-asan/` was preserved.

#### v26 → v29 (2026-10-16; product commit reconciled by this plan checkpoint)

- **1. Completeness:** made each requirement trace bidirectional and evidence-bearing: request/contract → source and
  callers → representations and persistence/version/conversion paths → owning leaf → registered test/oracle → build
  target → durable receipt. New obligations discovered in implementation or review must be added to the matrix and
  assigned an owner before the slice can close; an exclusion must name both reason and successor owner. The active
  local-topology leaf is explicitly limited to local injectivity, orientation and endpoint pairing; global coverage,
  component uniqueness, cycle/closure and public validity remain downstream.
- **1. Plan-contract correctness:** separated this from completeness and made it a recorded pass/fail review of the
  actual symbols, authoritative mathematical/API assumptions, numerical and resource rules, fallback/refusal semantics,
  oracle independence, file/version matrix, DAG and scope boundaries. The proof conditions now explicitly distinguish
  strict projected-derivative positivity from nonnegative derivative plus strict endpoint separation; a non-strict
  chord or endpoint claim cannot silently become an accepted witness.
- **2. Implementation correctness:** clarified that review is on one immutable, tested diff and challenges both
  accepted and refused behavior, including whole-domain enclosure, rounding, overflow, shared-work debit, identity,
  lifetime, atomic failure and applicable serialization. It must demonstrate a valid positive and an adversarial
  negative and reject both false acceptance and unconditional refusal. Gibbs independently reviewed the current
  three-path diff and reported no actionable correctness findings; the exact digests and all passing receipts are in
  the active card.
- **3. Executable sub-plan/readiness:** added six explicit gate-result fields to the card template; readiness records
  the actual base and expires on source, toolchain, ownership or scope changes. The active card replaces stale red
  findings with a passing 20-case focused selector, genuine event-root incidence fixture, same-image fold refusal,
  whole-circle seam regression and tested dyadic-reflection helper. The global ordered-chain leaf remains waiting.
- **4. Commit after each slice:** committed exactly the three owned paths as `ef3ba756b52796c02b8490caedcaa48b8db69605`,
  direct child of activation checkpoint `fd4697059572af28c08ecbb532e687af1603ee1c`. The staged names, pre-commit
  binary-diff digest, all three file digests, hook result and committed path list were verified against the tested
  freeze. The roadmap, evidence reports and pre-existing `build-asan/` were excluded.
- **5. Self-update after committing:** this v29 is the immediate separate one-file child of the product commit. It
  reconciles actual product ancestry and digests, all five gates, status, requirements, evidence, parent aggregates,
  DAG, risks and next-leaf readiness. Immediately after committing this file, verify direct-child/path/digest/worktree
  postconditions; product work remains paused until they pass. The local-topology leaf is considered done only after
  that audit. v29 does not store its own SHA.
- **Validation:** final focused Release/ASan: 20 cases/6,823 assertions each; bounds Release/ASan: 25/46,104 each;
  adjacent Release/ASan: 145/757,599 each; full Release: 4,813 cases/7,399,173 assertions, 74 unchanged skips;
  DXF fast Release/ASan: 1,028/39,932, 3 unchanged skips; DWG fast Release/ASan: 1,084/34,952, 4 unchanged skips.
  Every JUnit report has zero failures/errors, and skip identities match baseline. qmake6 project generation and app
  `make -j4` both exit 0. Product validation was run on the unchanged three-path diff. This file is the plan-only direct
  child of the verified product commit; its post-commit parent/path/hash/status audit must pass with no other tracked
  path in the checkpoint. The active card records that transition and keeps the next card waiting until the audit and
  a fresh readiness probe pass. Pre-existing
  untracked `build-asan/` and historical red evidence remain preserved.
