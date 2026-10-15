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

Plan **v6**, 2026-10-15. This file is the implementation source of truth. Every implementation slice ends with a
verified commit followed by a committed reconciliation of this plan (§1.4).
Design baseline reviewed: `41678d976` (#3121). Fetched execution base: **b05fb541df651dd9cb55a222dee4563e6127c436**
("Keep default shortcuts independent of translations (#3140)").
Status: **IMPLEMENTATION — S0 baseline validated; documentation commit/checkpoint pending. Product slices remain pending.**
Authoritative execution plan: `/Users/dli/dev/LibreCAD/.claude/worktrees/lc-offset-curve-2961/docs/plan_offset_curve_2961.md`.
Execution branch: `codex/offset-curve-2961`. The research checkout retains the v5 seed only; it is not a second live plan.

**Required file support.** Phase 1 includes DXF and DWG reading and writing. A valid editable offset curve must retain
its spline geometry, embedded ellipse definition and editability after save/open in either format and conversion in
either direction. Support every writable version exposed by the current application (§3.10); DXF R12 has an explicit
geometry-only downgrade. This is a release requirement, covered by 1E-1 through 1E-3.

**How this plan was checked.** v1 was anchored at 7ecebc8fc; v2 had six design/API reviews at 7c4a93fa8. v3 reconciled
the codebase against local `origin/master` at 41678d976. v4 adds the requested DXF/DWG read/write contract and checks
its version matrix against the actual format enum, export mapping and codec version guards. Before implementation,
fetch the intended upstream branch and repeat S0 against that exact commit: verify symbols/type-site census, re-run
the guard compile, validate the new class declaration against current APIs, and review modification, file-format,
UI and phase-2 call sites. v5 reviews completeness, correctness, slice readiness and the commit/reconciliation loop.
v6 checks the executable sub-plans against b05fb541d, tightens numeric/topology and metadata contracts, and makes the
commit/reconciliation evidence auditable. Prototype compilation and measurements remain historical evidence.

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
| S0 | Worktree, tracked plan, build baseline and evidence (documentation commit) | — | — | 0 product | validated; Release/ASan builds and baseline checks passed; commit/checkpoint pending |
| 0A | Trim Two must not replace an owned child | — | — | — | done on baseline by #3113 (`609ca81ae`); verify current tests at S0, then add the new-type mapping/test in 1D |
| 0B-1 | Guard skeleton: `rowOf` table, pragmas, `static_assert`, kinds-only walk; registration; CI path filter; shadow-`rs.h` checks | B | S0 | 250 | pending |
| 0B-2 | Entity factory header; clone and INSERT checks; pin tests | B | 0B-1 | 500 | pending |
| 0B-3 | DXF 2018 and DWG 2018 round trips, `reloadsAs`, `knownGaps`/`notRun` | B | 0B-2 | 200 | pending |
| 1A-1 | Closed forms, engine-layout test, validity check, tolerance helpers | C | S0 | 900 | pending |
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
  1E-1 through 1E-3 checks; the new RTTI writer case alone does not complete file support.
- **Before PR D**, resolve grip decision D0 and post the two-sentence answer to sand1024's DXF question from #2523 on
  #2961. Do not treat the provisional grip default as user-approved.

### 0.3 Sub-plans and readiness

Implementation proceeds in this default order:
`S0 → 0B-1 → 0B-2 → 0B-3 → 1A-1 → 1A-2 → 1A-3 → 1B → 1C-1 → 1C-2 → 1D → 1E-1 → 1E-2 → 1E-3 →
1G-1 → 1G-2 → 1G-3 → 1H → 1F → 1I-a → 2A → 2B → 2C → 2D-a`.
0A is an upstream prerequisite verified during S0. User publication rows do not block local implementation or phase 2.
Use an already documented default, including D0, while a publication discussion remains pending.

**A slice is ready only when:** its required predecessors are done; its current symbols/files and baseline are known;
its behavioral contract and refusal/fallback policy are explicit; its validation commands, expected test families and
nonzero case-count checks are recorded; and no unresolved design choice changes its success criteria. Existing exact
counts come from S0; count new cases at registration and record the result before commit instead of inventing totals
before writing tests. Readiness is not inferred from a
LOC estimate or an old prototype. Recheck it after every commit, rebase or overlapping change.

Before editing a slice, put its concrete card below the relevant step heading, using this template. Keep the card in
this file; build logs and machine-generated artifacts live under ignored `build/evidence/<slice-id>/`.

```yaml
slice: <step-or-child-id>
state: ready
base_sha: <actual-checkout-sha>
owner: <one-agent-and-nonoverlapping-file-scope>
dependencies: <done-ids-and-implementation-shas>
requirements: <rows-from-section-0.4-and-step-specific-contracts>
contract: <observable-success-and-refusal-behavior>
files: <explicit-product-test-build-documentation-paths>
tasks: <ordered-small-changes-with-one-reviewable-purpose>
validation: <commands-case-counts-oracles-and-adjacent-regressions>
evidence: <tested-tree-command-exit-count-skips-and-report-identities>
recovery: <how-to-preserve-and-resume-uncommitted-work>
review: <resolved-findings-or-explicit-remaining-question>
implementation_sha: pending
next: <candidate-next-slice-and-entry-check>
```

The detailed step sections remain the technical specification. The following sub-plans define the commit boundaries
and the evidence needed to activate them. All product work is currently pending.

| Step | Initial readiness / entry check | Commit-sized sub-plan and exit evidence |
|---|---|---|
| S0 | Ready for preflight | Select clean implementation worktree; copy and track this plan there; record fetched base, toolchain, baseline reports, format matrix and type census. Commit the prepared roadmap; record its SHA in a plan checkpoint. |
| 0A | Existing upstream code | Verify #3113 ancestry and current trim tests. Record evidence in S0; only the new-type mapping belongs to 1D. |
| 0B-1 | Waiting for S0 | Add exhaustive enum rows, compile-time guard, source registration and CI paths together. Prove both inserted and appended shadow enumerators fail compilation. |
| 0B-2 | Waiting for 0B-1 | Build valid entity fixtures; add clone and INSERT checks. Separate fixture defects from actual gaps; avoid GUI dependencies in the fast lane. |
| 0B-3 | Waiting for 0B-2 | Run real DXF/DWG save/open and load completion; record stable known-gap identities and valid row totals. This completes the guard before adding RTTI. |
| 1A-1.a | Conditional: current ellipse/jet APIs confirmed | Implement finite-domain validation, full-turn normalization, signed geometry and tolerance helpers. Test degeneracy, circle limit, extreme scale and reversed/wrapped arcs. |
| 1A-1.b-ref | Waiting for .a | Implement the bounded exact-reference/event graph in §3.5.1, without fitting or materializing splines; test visibility, components and budgets against current trim behavior. |
| 1A-1.b-check | Waiting for .b-ref | Implement the adaptive cache-to-reference check, complete-component coverage and topology policy in §3.5. Use shared work counters; unresolved geometry refuses adoption. |
| 1A-1.c | Waiting for .b | Add adversarial and current-engine corpus checks, failure budgets and performance measurements. Record acceptance/refusal results; endpoints and midpoint checks alone cannot close this parent step. |
| 1A-2 | Waiting for 0B-3 and all 1A-1 children | Add type, core class, complete consistency stamp, transforms, clone/INSERT/write dispatch and interim UI guard. Compile CMake and update qmake lists; run copy/ownership and enum coverage. |
| 1A-3 | Waiting for 1A-2 | Implement definition-backed grips/snaps with runtime consistency checks and separate cache closure. Test closed results from arc sources and stale cache fallback. |
| 1B | Waiting for 1A-3 | Implement input/refusal checks, one-run mutation, staged cache adoption and resource limits. Verify release-mode refusals preserve state; remove regeneration stubs. |
| 1C-1 | Waiting for 1B | Wrap eligible tool output; preserve plain-result diagnostics, Explode and unequal-scale substitutions. Test preview/commit parity, layer/pen, ownership and undo. |
| 1C-2 | Waiting for 1C-1 | Implement non-nesting offset-again and bounded rebase/fallback. Required zero-sum fallback and near-critical success tests replace the optional old failure-band search. |
| 1D | Waiting for 1A-2; run after 1C-2 by default | Resolve every current type site, including Trim's chord mapping. Commit the disposition census and parity tests; verify owned chords are never replaced or double-deleted. |
| 1E-1.a | Waiting for 1B, 1C-1 and 1D | Implement bounded typed schema parsing/merging, major-axis vector representation and stale-schema removal. Test parser behavior before changing filter callbacks. |
| 1E-1.b | Waiting for .a | Prepare metadata in the shared writer and R12 downgrade; inspect actual DXF XDATA and DWG EED output. Verify error propagation and APPID resolution. |
| 1E-1.c | Waiting for .b | Integrate common import adoption and codec regressions; verify both formats restore editable curves without regeneration. Update both 0B round-trip expectations. |
| 1E-2 | Waiting for all 1E-1 children | Add malformed/oversized metadata, unknown-tail edit policy, transformed/demoted geometry, binary fixture and foreign-data tests in both formats. Commit reproducible fixtures and provenance. |
| 1E-3 | Waiting for 1E-2 and 0B-3 | Execute all 14 version rows, both conversion directions and edit/save/open. Record nonzero DWG coverage, binary codec results and available independent-reader evidence. |
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
`1A-1.b` is the aggregate of `.b-ref` and `.b-check`, not an extra implementation commit. Any other step
may be split before editing by adding named children, dependencies and validation to its card. Keep coherent changes
such as a new type name and its required registry entry in one buildable commit.

### 0.4 Completeness and correctness coverage

| Requirement | Owning steps | Required oracle |
|---|---|---|
| Finite 2D domain and unambiguous whole/arc normalization | 1A-1, 1B | Tiny/zero/full-turn, circle, extreme ratio/scale and non-finite cases; no accidental whole ellipse |
| Bounded geometry and topology validation | 1A-1, 1E-1 | Adaptive bounds, hostile control edits, closure/end/direction checks, explicit limit/ambiguity refusal |
| Runtime consistency and safe edit state | 1A-2, 1B, 1G, 1H | Mutated mode/flags/knots/controls cannot retain active definition; refused edits do not change geometry or undo |
| Clone, transforms, ownership and INSERT | 1A-2, 1C, 1D | Independent children, preserved attributes, reflection/scale parity, unsupported transform demotion, ASan |
| Creation, repeated offset and fallback | 1B, 1C, 1F | Option-off parity, one-piece adoption, required multi-piece/zero-sum fallback, bounded attempts and diagnostics |
| Complete type integration | 0B, 1D, 1G | Current census with disposition for every site; no silent save/drop/INSERT gap for the new type |
| DXF and DWG read/write | 1E-1 through 1E-3 | Version matrix, actual codec payloads, both conversion directions and edited reopen |
| Metadata compatibility and resource limits | 1E-1, 1E-2 | Registered app, bounded typed tails/bytes/depth, invalid/unknown schema policy, stale metadata removal |
| UI edit and selection behavior | 1G, 1H, 1F | Dialog/sheet/drag success/refusal/cancel/no-op, mixed selection and one-step undo |
| Source actions | 2A through 2D | Live document source, no stored dependency, normalized restore/deduplication and both-format persistence |
| Build and regression portability | 0B, 1I, 2D | CMake and qmake6, focused/adjacent tests, GCC/MSVC fixture coverage where CI is available |
| Responsiveness and bounded work | 1A-1, 1B, 1C-2, 1H | Deterministic work caps plus measured import/edit costs; no timing assertions or mouse-move regeneration |
| Durable implementation progress | Every slice | Implementation commit, evidence, reviewed plan checkpoint and a re-evaluated next ready card |

Before phase 1 or the whole plan is marked complete, audit this matrix against the committed tree. A product stub,
unimplemented required callback, skipped format, missing qmake entry, unresolved correctness finding or missing
post-commit reconciliation leaves the corresponding row incomplete. Separate user publication tasks and explicitly
out-of-scope features from implementation completion.

### 0.5 Current readiness and implementation handoff

At the recorded execution base, only S0 is active. Release/ASan configurations, all three target builds, Release full
and fast-lane baselines, and focused ASan trim/ellipse checks passed, with existing skips recorded in its card.
No product step is done. 0B-1 is the next candidate, not yet `ready`: S0's documentation commit/checkpoint must finish
first. Gibbs starts that first product slice; the coordinator owns the plan,
shared build directories and commits. Assign explicit file ownership before any parallel work; never run concurrent
Git/index operations or concurrent builds that mutate the same build tree.

The five requested review tracks are closed at the **plan** level by: §0.4 (complete coverage), §§3.3-3.10 (correctness
contracts), §0.3 plus the leaf cards (sub-plans/readiness), §1.4 steps 2-3 (verified slice commits), and §1.4 steps 4-6
(post-commit self-update and continuation). Product closure still requires the owning slice's evidence.

Before promoting a leaf card, check these concrete prerequisites:

| Card | Fact that must be established before editing | Evidence required before exit |
|---|---|---|
| 0B-1 | Current enum is contiguous; each of 50 values has a disposition; current CMake/CI locations confirmed | 50/27/23 kinds census, both negative shadow compiles, full and fast-lane tag counts |
| 1A-1.a | Raw sweep rules and finite/local coordinate domain are fixed | Boundary tests on both sides of each threshold and independent closed-form comparisons |
| 1A-1.b-ref | Exact-reference graph interface and shared work accounting are specified (§3.5.1) | Open, lens, closed-arc and multi-component fixtures; no engine regeneration; every cap exit observed |
| 1A-1.b-check | Reference analysis is qualified and does not accept ambiguous topology | Whole-span distance bounds, bidirectional coverage and malicious subset/retrace tests |
| 1A-2 | Guard and all geometry children are done; no raw constructor bypass exists | Current-header compile, complete stamp mutation cases, transform overflow and ownership checks |
| 1E-1.a | Typed schema grammar, malformed/unknown ownership and flag consistency are settled | Codec-independent exact-code/type tests, duplicate/malformed cases and bounded tails |
| 1E-1.b/.c | Writer placement and common DXF/DWG adoption callbacks confirmed | Real codec payloads, save failure propagation, no regeneration and attribute preservation |
| 1G-2.a | Existing provider/delegate and document Undo/Redo contracts are understood before generic edits | Old-provider defaults, rapid pending edits, before/after history hooks, refusal/no-op tests and ASan; full regression suite for shared history changes |
| 2A/2B | Live-document ownership, normalized equality and undo rules are fixed | Generated-child refusal, circle/arc equality, local-scale tolerance and one-step undo |

These are entry/exit gates, not permission requests. Resolve an algorithm/API mismatch in the owning card and revise
its downstream contracts before coding; do not silently narrow required scope. The default order remains §0.3, with
`.b-ref` then `.b-check` replacing its aggregate `.b`. Ready leaves may proceed independently only when predecessors,
file ownership, validation ownership and commit ordering permit it.

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
  from the same build directory at once.

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
  - Do not use `[offset-curve]` (easily confused with the existing `[curve-offset]`, 178 cases), nor the existing
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

For every step or named child slice:

1. **Prepare.** Reconcile actual HEAD and relevant changes with the roadmap; complete its ready card. Record explicit
   file scope, contract, commands and expected test cases before implementation. Keep unrelated user edits untouched.
2. **Verify and review.** Implement, run focused and required adjacent checks, inspect the full scoped diff, and resolve
   correctness findings. Record actual exit codes, case counts, skips, artifact paths and the tested tree. Mark
   `validated` only while that evidence still matches the files being committed.
3. **Commit the implementation.** Inspect the index; stage owned product/test/build files by explicit path, and commit
   one coherent slice with the repository's sentence-title style. For a documentation-only slice, commit its scoped
   documentation instead. Check `git show --stat HEAD` and capture `git rev-parse HEAD`. Hooks must pass; a failed commit
   leaves the slice uncommitted and must be repaired without `--no-verify` or silently widening scope.
   Verify that the committed product tree matches the tested one. If hooks changed code or unrelated unstaged source
   affected the build, validate the committed slice in isolation before marking it done; preserve the other edits.
   A pre-existing staged change is not implicitly authorized for this commit. Use a clean index in the dedicated
   worktree or preserve its ownership and stop the commit operation until it can be isolated; never unstage another
   contributor's files as cleanup. Record the tested source-tree identity (base SHA plus scoped diff/digest for an
   uncommitted test run), not just the SHA from which editing began.
4. **Self-update after the commit.** With the real implementation SHA now known, reread the changed code and reconcile
   the plan: mark the child done, update parent progress, record test/evidence identities and the date, correct invalid
   assumptions/anchors/estimates, add any discovered follow-up slice, and recalculate downstream readiness. Fill the
   next ready card or the precise unresolved prerequisite. Do not mark a parent done while required children remain.
5. **Commit the plan checkpoint.** Commit only the reconciled roadmap, for example
   `Record offset curve slice 1B validation and next steps`. This checkpoint records the implementation SHA, not its
   own SHA. It does not trigger another checkpoint, which avoids recursive self-referential commits. Check its diff
   and the worktree status; the slice becomes `done` only when this reconciliation is durable.
6. **Continue.** Start the next ready slice automatically. A successful commit is a checkpoint, not the end of the run.

**Mandatory post-commit reconciliation checklist (steps 4-5).** Verify the implementation SHA exists and contains the
intended paths; link it from the executed card/ledger; record commands, exit codes, case counts, baseline differences
and review outcome; update every affected parent and coverage row; correct the current API/file anchors; record any
newly discovered fix as a named leaf with dependencies; replace the next card's stale base SHA with actual HEAD; and
refresh the top-level status. Review the plan-only staged diff before committing it. A plan edit made before the
implementation commit cannot stand in for this post-commit check. A checkpoint changes no product files and does not
require a redundant full test run. Its own SHA is discoverable from Git history and is not written into itself.

Keep validation evidence for the entire slice; do not cherry-pick the final green run while hiding failed or skipped
required cases. Resolve the failure, record the cause/fix and rerun affected checks. If a verifier is unavailable,
record it as unavailable, never as passing. Required native tests/builds are gates; optional external tools and user
publication actions are separately labeled and cannot be used to claim those native gates passed.

If interrupted after step 3, record `committed — reconciliation pending` and recover by inspecting HEAD and the scoped
diff; finish steps 4–5 before more product work. Never reimplement or revert a successful slice because its checkpoint
is missing. If a checkpoint commit fails, preserve both the implementation commit and pending plan changes and repair
the checkpoint. A subsequent correctness discovery reopens the parent and creates a new fix slice; it does not justify
amending published history or concealing the finding.

Use this compact ledger under each executed card; expand it only with meaningful evidence:

| Slice | Implementation SHA | Tested tree / evidence | Review outcome | Parent / next readiness |
|---|---|---|---|---|
| No implementation slices committed yet | — | S0 builds/baseline validated; see S0 reports/counts | — | S0 checkpoint pending; 0B-1 waiting |

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
separate. The current request updates this process in the plan; execution starts when implementation is requested.
The existing execution has reached S0 only; updating this roadmap does not itself complete S0 or authorize a false
product-completion entry. After an interruption, read the ledger and Git history before choosing the next leaf.

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
| D16 | Sources in this release | A top-level `RS_Ellipse`, whole or arc, with a one-piece result passing bounded geometry/topology validation. Other successful tool output stays plain and is reported. |
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
                                   NotOneCubic, CheckFailed, Engine };
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
    std::size_t maxCubicPieces = 4096;
    std::size_t maxVertices = 65536;
    std::size_t maxDistancePairs = std::size_t{1} << 20;
    unsigned maxDepth = 20;
};
struct LC_OffsetCheckResult {
    enum class Reason { None, InvalidInput, Layout, TooSmall, OffCurve, TooNear, Topology, LimitExceeded };
    Reason reason = Reason::None;
    std::size_t span = 0;
    std::size_t work = 0;
};
LC_OffsetCheckResult checkCache(const RS_SplineData& cache, const LC_EllipseOffsetDefinition& definition,
                                double tol, const LC_OffsetCheckLimits& limits = {});
```

`tol = LC_CurveOffset::toleranceFor(RS_Ellipse{nullptr, basis}, distance, factor)`: the engine's own requested
geometry tolerance (1A-1). Validate finite positive inputs before evaluating it. Default check limits are concrete
starting bounds aligned with existing engine limits; qualify their acceptance/cost in 1A-1.c before freezing them.

The old endpoints/midpoints validator is insufficient: a cubic may agree at those points and stray between them.
Use a bounded adaptive check with conservative error accounting; do not claim an exact algebraic identity.

1. **Validate domain and layout before work/allocation.** Require finite 2D controls/knots/weights and normalized
   source fields, valid positive limits, degree 3, ClampedOpen representation, `n ≥ 1` spans, `3n+1` controls and
   `3n+5` knots. Weights are empty or exactly one per control, all 1. Knots have four equal initial values, `n-1`
   internal groups of three, and four equal final values, increasing between groups. Reject invalid count arithmetic,
   unexpected fit representation and non-finite/degenerate geometry. Span k uses `[3k .. 3k+3]`.
2. **Small distance:** `distance ≤ tol` gives `TooSmall`. The caller must distinguish invalid inputs, geometric
   refusal and a work limit; none may become successful adoption with an unchecked cache.
3. **Construct bounded reference geometry.** Use the same source sweep convention as `EllipseSource::make`, including
   reversed/wrapped arcs. Reuse existing `LC_Interval`, parametric intersection and spline jet/stroke primitives where
   applicable. Form adaptive reference segments for the ellipse and signed exact offset with explicit approximation
   error at most `tol/8`, split at relevant cusp/corner/trim events. This is pure bounded geometry analysis; it does not
   call `createEntities`, refit or regenerate the saved cache. In 1A-1.b record the current reference adapter/helper
   used for visible intervals; extract a small read-only helper if needed instead of duplicating the offset engine's
   trimming rules. Unresolved visibility or event classification refuses adoption.
4. **Check the whole cubic, not three samples.** Subdivide with de Casteljau or the existing conservative jet bounds.
   Bound each leaf's curve-to-chord error. For an appropriately matched reference segment, the maximum distance of
   the two cache-chord endpoints to that convex segment, plus both approximation errors, bounds the whole cache leaf's
   distance to the offset. Require this bound ≤ `2·tol`; refine uncertain leaves and refuse if it cannot be established.
   Compute the minimum cache/source segment distance with conservative error subtraction and require
   `≥ d − 2.5·tol`. Candidate nearest-point samples or golden-section searches may accelerate matching, but their
   sampling alone cannot prove acceptance. Spatial bounds and pair caps keep the search bounded.
5. **Check topology and coverage.** The definition must have exactly one retained nondegenerate reference component;
   a cache matching only one component of a multi-component result is not adoptable. Require one connected cache chain
   with the stated direction (allowing `cacheReversed`), matching **all** retained reference intervals and allowed trim
   nodes of that component. Check coverage in both directions, not just cache-to-reference proximity. Whole-source results must close;
   untrimmed arc results must retain the offset endpoints. Trimmed arcs may close, but must form a complete retained
   component. Reject retracing, missing tails, extra loops and unexplained joins. Use source-parameter intervals to
   distinguish legitimate lens-corner jumps from broken ordering. Ambiguous topology returns `Topology`, preserving
   plain geometry on import or keeping the old editable object on a refused edit.
6. **Bound every path.** Check vertex/piece/pair/depth caps before expansion; non-finite intermediate bounds or exhausted
   work produce a refusal (`InvalidInput` or `LimitExceeded`), never an acceptance. Work counters support deterministic
   tests. Adaptive validation runs at adoption/edit/load, not painting or mouse motion. Use local coordinates and
   scale-aware error handling; extreme coordinates must not silently overflow a validity test.

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

Implementation order for that leaf:

1. Read `EllipseSource::make`/`OffsetCurves`, `nodeBranches` and `trimBranches` at the execution base. Record sweep,
   side, numeric-floor and visibility-tolerance conventions in the card. `LC_Interval` lives in `lib/math/`; use its
   outward bounds, `lc_curvejet.h` and `lc_parametriccurveintersection.h`, not a new ad-hoc sampling engine.
2. Construct exact signed-offset jets and bounded parameter intervals, splitting at curvature-zero/cusp events and
   isolating own-offset/opposite-offset intersections and open-end half-circle **cutters**. The cutters classify
   visibility; they are not output geometry appended to the offset. Qualify corner, tangent and overlapping events;
   unresolved events return a refusal. Do not reuse fitted cubic control points as an allegedly independent exact
   reference. A helper extraction must preserve existing engine behavior and its tests.
3. Classify retained intervals with conservative distance-to-source/endpoint bounds and the engine's documented trim
   allowances. Build explicit adjacency and ordered components; account for closed-lens source-parameter jumps.
   Reconstruct the complete expected component set, not merely the portion nearest the proposed cache. If current
   engine fitting/classification and exact analysis disagree beyond the agreed tolerance, keep plain geometry and
   resolve/record that reproducible disagreement before making the fixture a required success.
4. Share one checked work-accounting object across event isolation, subdivision and all distance/intersection queries.
   `maxVertices` is the total generated reference/cache leaves, not a fresh allowance per span; pair/depth/piece caps
   include failed/refined queries. Check before allocation or increment and use overflow-safe arithmetic. Exhaustion
   returns `LimitExceeded` with no partially successful reference result.
5. Qualify the following committed deterministic fixtures before declaring the adapter done: a whole outward offset,
   a whole inward untrimmed offset, a whole inward lens, an open retained arc, a closed result from an arc, and an arc
   with multiple retained components. From current `lc_curveoffsetellipsetests.cpp`, use a=100,b=5,15deg..315deg,d=4
   for multi-component coverage; a=100,b=50,10deg..350deg,d=49.5 for the closed arc; a=100,b=60,d=36.036 for the
   near-critical whole lens. Requalify rather than pinning spline counts, then add rotated/reversed/large-centre forms.

`1A-1.b-check` subsequently owns conservative whole-span error bounds, source clearance, bidirectional ordered
coverage and candidate topology. Add explicit tests for a short sub-arc taken from a valid cache, one disconnected
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
  | not one degree-3 ClampedOpen `RS_Spline` | `NotOneCubic` |
  | §3.5 fails | `CheckFailed`, with its `checkReason` preserved |

  Input guards do not duplicate the engine's geometric "nothing left" decision. Keep that decision in the engine.
- **`adoptCache(candidate, next)`:** `getData() = candidate;` (closed flags as the engine made them: false);
  `m_def = next;` then `if (next.cacheReversed) RS_Spline::revertDirection(); else update();` (both rebuild the
  chords); then update validated closure, rebuild grips and set `m_stamp = cacheFingerprint(getData())`. The arc-length table is keyed by the
  fingerprint and refreshes itself (`rs_spline.cpp:878-889`).
- **`adopt(engineResult, def)`:** normalise `def.basis`; require one degree-3 ClampedOpen `RS_Spline` in the engine
  layout; run §3.5 with `tol` from `def.relTolerance`; construct through the private validated-tag copy-from-spline
  constructor. 1C wraps engine results that already ran under the batch's budget this way, and 1E
  adopts loaded splines this way. **Neither ever calls `create()`.**
  The definition passed to the validator includes `cacheReversed`. A loaded cache is already in its saved direction;
  the copy-from-spline adoption constructor must not reverse it again. `adoptCache` applies reversal only to fresh
  engine output produced in the source's original direction.
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
is needed.

The mandatory application-level read/write matrix at the reviewed baseline is:

| Format | File version | `RS2::FormatType` | Save/open expectation |
|---|---|---|---|
| DXF R12 | AC1009 | `FormatDXFRW12` | POLYLINE geometry within export tolerance; no editable offset definition |
| DXF R14 | AC1014 | `FormatDXFRW14` | SPLINE + XDATA; restores editable curve |
| DXF R2000 | AC1015 | `FormatDXFRW2000` | SPLINE + XDATA; restores editable curve |
| DXF R2004 | AC1018 | `FormatDXFRW2004` | SPLINE + XDATA; restores editable curve |
| DXF R2007 | AC1021 | `FormatDXFRW` | SPLINE + XDATA; restores editable curve |
| DXF R2010 | AC1024 | `FormatDXFRW2010` | SPLINE + XDATA; restores editable curve |
| DXF R2013 | AC1027 | `FormatDXFRW2013` | SPLINE + XDATA; restores editable curve |
| DXF R2018 | AC1032 | `FormatDXFRW2018` | SPLINE + XDATA; restores editable curve |
| DWG R2000 | AC1015 | `FormatDWG` | SPLINE + EED; restores editable curve |
| DWG R2004 | AC1018 | `FormatDWG2004` | SPLINE + EED; restores editable curve |
| DWG R2007 | AC1021 | `FormatDWG2007` | SPLINE + EED; restores editable curve |
| DWG R2010 | AC1024 | `FormatDWG2010` | SPLINE + EED; restores editable curve |
| DWG R2013 | AC1027 | `FormatDWG2013` | SPLINE + EED; restores editable curve |
| DWG R2018 | AC1032 | `FormatDWG2018` | SPLINE + EED; restores editable curve |

At S0, reconcile this explicit table with `RS2::FormatType` and `RS_FilterDXFRW::fileExport`; never infer versions from
enum ranges. Guard DWG-only format references with `DWGSUPPORT`; the shared schema module uses ordinary DRW variants
and has no dependency on a DWG-only API. Standard CMake targets currently define `DWGSUPPORT`. The application
currently writes ASCII DXF. Binary DXF input uses the same adoption path and needs a reader regression fixture;
verify schema read/write through the existing low-level binary DXF codec as well. Its existing writer generates the
fixture; the current application export interface remains the entry point for ASCII DXF.
Legacy DWG R13/R14 remain input-only through the existing readers; the six DWG rows above define the required write
targets. Accepted legacy SPLINE/EED input still reaches the common adoption callback.

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
   - angles finite, and whole (decided through `RS_Ellipse`, §3.3) or |angle2 − angle1| ≤ 2π;
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
be degree 3, ClampedOpen (its closed and periodic flags may be set; ODA sets them), in the engine layout, with all
weights exactly 1.0 (`makeSplineFromDrw` fills 1.0 when the file has none), and it must pass §3.5. Use
the public validated `LC_OffsetCurve::adopt(*spline, def, why)` factory; its private copy-from-spline constructor keeps
what `setEntityAttributes` set. `addHelix` strips but never adopts.

The same `addSpline` path must be reached by actual DXF and DWG imports. Preserve layer, pen, visibility, block owner
and supported foreign XDATA/EED when replacing the plain spline with the editable entity. Check a low-level callback
capture of each saved file as well as the document entity, so a round trip cannot pass solely because both ends of the
new adapter share the same metadata mistake.

**Other outputs.**
- R12 writes and reloads as a POLYLINE without any owned schema-1 sub-list; preserve supported foreign XDATA through
  the existing codec. Verify geometry against the exported spline within the R12 approximation tolerance.
- Hatch boundary edges carry no XDATA: an offset-curve edge is written as a SPLINE edge and reloads as
  `LC_SplinePoints`, as every spline edge does.
- #3119 registers XDATA application names. Assert `LibreCad` appears exactly once under case-insensitive APPID lookup,
  matching the spelling used by the emitted group; use the existing registration mechanism.

---

## 4. Phase 0 — prerequisites

### S0. Worktree, tracked plan, builds and baseline (documentation slice)

**Execution card, 2026-10-15**
- State: validated; documentation commit/checkpoint pending; base `b05fb541df651dd9cb55a222dee4563e6127c436` fetched from origin.
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
- Next: commit S0 and its plan checkpoint; then promote the concrete 0B-1 card and hand its product scope to Gibbs.
- Implementation SHA: pending; no product completion is claimed.

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
generated child without deleting it or damaging undo state. See the 1D tests and the PR D gate in §0.2.

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

**Prepared execution card (waiting only for S0 checkpoint)**
- Owner: Gibbs; coordinator owns this plan, shared builds and commits.
- Base: b05fb541d plus S0 documentation commits; refresh actual HEAD at handoff.
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
- Review: current enum/CMake/CI locations confirmed by Gibbs; baseline reports above. Implementation SHA pending.

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

- **DXF and DWG round trip.** For each drawing row: a fresh graphic with the one entity; `lc::test::exportDrawing` and
  `importDrawing` with `FormatDXFRW2018` and `FormatDWG2018`, inside `QuietLog`; run the application's post-load
  completion hook before checking load-dependent entities; expect one live top-level
  entity whose `rtti() == reloadsAs`.
  - `reloadsAs` is the type itself unless the row says otherwise. Record what Hyperbola, Parabola and SplinePoints
    actually reload as: `handleQuadraticConicSpline` (`rs_filterdxfrw.cpp:8001-8030`) can turn a degree-2 spline back
    into a conic.
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

### 1A-1. Closed forms, engine-layout test, validity check, tolerance helpers

**Files.**
- New `lib/engine/document/entities/lc_ellipseoffsetgeometry.{h,cpp}`, containing:
  - `struct LC_EllipseOffsetDefinition` (§3.2);
  - free geometry operations in namespace `LC_EllipseOffsetGeometry`; this module owns `normaliseBasis` from .a,
    so its tests do not depend on the not-yet-created `LC_OffsetCurve` class. The later class static forwards to it;
  - point, derivative and outward normal at η;
  - `exactOffsetPoint(basis, η, s)` (signed s; towards the centre is negative);
  - the sweep interval of §3.5;
  - distances from a point to the original and to O(η) (§3.5 step 4);
  - `minRadiusOfCurvature(basis)` over the sweep;
  - the lens corners (Appendix A; guard a = b and clamp sin²η to [0, 1]);
  - `isEngineLayout`; Bezier evaluation of span k; distance from a point to the cache's Bezier geometry;
  - the inside test of the whole ellipse;
  - `offsetPolyline(basis, d, toward, samples)` (for 1H);
  - `checkCache` (§3.5).
  - the check limits/result and bounded reference/visibility adapter needed by §3.5, reusing current interval and
    jet primitives; document any small read-only engine helper extraction before implementation.
- `lc_curveoffset.{h,cpp}`: two appended public helpers, no behaviour change:
  - `double relativeOffsetTolerance(const RS_Entity& source)`: 1e-4 for ellipses (`kEllipseRelativeOffsetTolerance`),
    1e-6 otherwise;
  - `double toleranceFor(const RS_Entity& source, double distanceMagnitude, double relativeFactor)`: the formula of
    `lc_curveoffset.cpp:4962-4965` with the file-local `computeScale` (`:601-622`).

  Do not name it `requestedTolerance`: a parameter of that name in `makeDirectOptions` / `makeOffsetOptions` would hide
  an unqualified call.
- Build lists for the two new files (§1.3).

**Precondition:** use the S0 build and baseline; retain the #3117 arc behavior measured at S0.

Execute leaf children 1A-1.a, .b-ref, .b-check and .c in order (§0.3); each has its own compile/test and commit checkpoint.
`.b` is their reference/check aggregate for parent progress, not another commit.

**Tests** — new `lib/engine/document/entities/tests/lc_offsetcurve_geometry_tests.cpp`, `[lc-offsetcurve][geometry]`,
registered in `librecad_tests` next to `lc_curveoffsetellipsetests.cpp`:
- **Closed forms against brute-force dense sampling** on random ellipses, including ratio 1: distances, b²/a, lens
  corners, minimum radius of curvature.
- **`toleranceFor`:** `makeOffsetOptions(e, d).tolerance.requestedGeometry == toleranceFor(e, d, relativeOffsetTolerance(e))`.
- **Layout:** `isEngineLayout` accepts the engine's output and a rescaled knot vector; it rejects other knot vectors,
  non-unit weights and degree 2.
- **Domain and normalization:** canonical whole, genuine ±2π sweeps, zero/tiny noncanonical sweeps, multiple turns,
  wrapped/reversed arcs, circles, extreme valid scales/ratios, non-finite values and precision loss. Include a case
  whose small arc angles would be snapped away if raw validation happened after ellipse construction.
- **Current engine output:** exercise ratios 0.05–0.95, distances 1.01·tol … 3a, whole/arcs and both sides. Record the
  supported accepted geometries and explicit topology/limit refusals; one output entity alone does not prove eligibility.
  Include closed results from arc sources and boundaries near cusp/trim events.
- **`checkCache` rejects:** a result moved by 5·tol; one control point moved by 0.5% and by 2% of a; `d ≤ tol`; a
  wrong-side result.
- **Adversarial acceptance:** controls edited so old endpoints/midpoints agree but quarter-span geometry deviates;
  missing arc tails, reversed/retraced pieces, false closure, extra loops, non-finite knots/weights and malformed
  vector counts. Use independent dense evaluations to check claimed error bounds, not the validator's own sample list.
- **Work limits:** deterministic exits for piece/vertex/pair/depth caps and invalid zero limits; no overflowing work
  counter, unbounded recursion or successful unchecked adoption. Measure current import/edit costs without timing asserts.

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
  `src.pro:606`, `:1068`): the optional-result `forExport`, `parseAndStrip` and import adoption (§3.10).
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
- **`rs_filterdxfrw.cpp` `addSpline`** (:8034-8139). Every exit copies the record's own list through
  `setEntityAttributes` (:32993-32995): conic (:8024), degree 2 (:8050), degree above 3 (:8083), main path (:8109),
  fit points (:8121), and `addHelix` → `addSpline` (:8151). So strip a copy of the record once, at the top:

  ```cpp
    LC_OffsetCurveXData::Parsed oc = LC_OffsetCurveXData::parseAndStrip(data->extData);
    std::optional<DRW_Spline> stripped;
    if (oc.changed) { stripped.emplace(*data); stripped->extData = std::move(oc.remaining); data = &*stripped; }
    ...
    // main path (:8106-8112):
    std::unique_ptr<RS_Spline> spline = makeSplineFromDrw(m_currentContainer, *data, data->weightlist);
    setEntityAttributes(spline.get(), data);
    if (data->eType == DRW::SPLINE && oc.status == LC_OffsetCurveXData::Status::Schema1)
        if (auto curve = LC_OffsetCurveXData::adopt(*spline, oc)) spline = std::move(curve);   // copy-from-spline
    m_currentContainer->addEntity(spline.release());
  ```

  `DRW_Spline` is copyable; a `DRW_Helix` is sliced, which is fine because no exit reads beyond `DRW_Spline`. Confirm
  real DXF and DWG readers both invoke this callback with decoded XDATA/EED; no format-specific adoption bypass.
- **Codec trace:** inspect the ordinary SPLINE encode/parse and common EED path in `drw_entities.cpp`,
  `libdxfrw.cpp`, `libdwgr.cpp` and the DWG reader/writer. Prove that the complete schema survives each codec before
  relying on document adoption. Add focused codec fixes/tests only for losses demonstrated by those checks.
- **0B row:** `drawing(EntityOffsetCurve)`, reloading as itself in both DXF and DWG. Neither round-trip bit may be
  listed as a known gap or `notRun` for this new type.
- Keep filter changes localized to spline serialization and parsing. APPID registration is already handled by #3119;
  inspect current XDATA-tail and import-finalization ownership before editing those paths.

**Tests** — new `lib/filters/tests/lc_offsetcurve_file_tests.cpp`, built on `Bench`; separate cases tagged
`[lc-offsetcurve][files][dxf]` and `[lc-offsetcurve][files][dwg]` share the fixture/oracles:
- **DXF R2018 and DWG R2018 smoke:** create, write, read and adopt whole/arc curves on both sides, in model space and
  a block definition. Check geometry and definition, not only save success or entity count. 1E-3 extends these checks
  to the complete version and conversion matrix.
- **Read/write payload inspection:** capture `DRW_Spline` from each actual output file through a low-level interface;
  verify degree, control points, knots, unit weights, closed/periodic semantics and the typed metadata fields. Verify
  the `LibreCad` application reference is registered/resolvable in the output.
- **Read without regeneration:** adopting a saved cache leaves `engineRuns()` unchanged. Writing also preserves the
  current cache rather than rerunning the engine and changing its geometry.
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
- **Checked-in fixtures** (cross-compiler check): `lib/filters/tests/testdata/dxf/offset_curves_r2018.dxf` and
  `testdata/dwg/offset_curves_r2018.dwg`, with a README and the generator (precedent
  `embedded_mtext_external_r2018.*`). They hold whole and arc curves, both sides, a = 10 and 100, and one inside a
  block. A fast-lane test asserts that every curve is adopted with its stored definition. This catches a curve written
  on macOS that MSVC would demote. Include a binary DXF input fixture with the same schema and cache, generated through
  the existing binary codec; import must adopt it as an editable curve. Record each fixture's version, origin and
  expected definition independently of the parser under test.
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

**Files:** extend `lc_offsetcurve_file_tests.cpp` and its test-support helpers. Use an explicit `FormatCase` table
containing `RS2::FormatType`, expected `DRW::Version`, extension and whether the format preserves the definition.
Guard only the DWG entries/cases with `DWGSUPPORT`; use the exact enum names from §3.10, including `FormatDWG` for
R2000 and `FormatDXFRW` for R2007.

**Required checks:**

- For every schema-capable row of §3.10, run write → read → write → read with whole and arc originals, both offset
  sides, translated/rotated bases, reversed cache direction, and an entity inside a block. Verify the requested file
  version via DXF `$ACADVER` or the DWG file signature; a file saved under the wrong version is a failure.
- At each generation, assert `EntityOffsetCurve`, the normalized definition (including signed-side semantics and
  saved tolerance), a consistent cache passing §3.5, target-representable layer/pen/visibility, owner context and
  supported foreign XDATA/EED. Common fixtures use indexed colors, default lineweight and other attributes supported
  by all matrix versions; newer-only attributes follow the codec's documented downgrade rules. Use scale-aware
  numeric comparisons; DXF's 16-digit text rounding and DWG encoding must satisfy the
  geometric tolerance. Do not compare remapped handles or text-rounded control points bit for bit.
- Cover both conversion directions without requiring a full Cartesian version product: DXF R2018 → each supported
  DWG version → DXF R2018, and DWG R2018 → each schema-capable DXF version → DWG R2018. Add a repeated alternating
  DXF/DWG chain to catch duplicate schema blocks and accumulated definition drift.
- Open a saved curve, change its distance/side/basis through supported mutators, save to the other format, and reopen.
  The newly edited definition and geometry must be present, with no stale original sub-list. Verify successful edits
  still work after reload and that refused edits leave the saved curve unchanged.
- Exercise R12 downgrade from both a newly created curve and a DWG-imported curve. Reopen as geometry-only POLYLINE;
  there is no marker that could incorrectly restore editability. Ordinary unmarked SPLINEs and unknown-schema inputs
  retain their documented plain/raw behavior in each supported format.
- Exercise binary DXF codec writing and reading at R14 and R2018 using a `DRW_Spline` with the shared serialized
  schema. Inspect the decoded payload and import each binary file into the document as an editable curve. The fixed
  binary input fixture also verifies reading independently of a same-run export.
- Run the new `[files][dxf]`, `[files][dwg]` and conversion cases in `librecad_tests` and the DWG-enabled DXF fast lane.
  Require a nonzero DWG case count; a target named "DXF" must not hide skipped DWG coverage. Compile the new shared
  schema module once without `DWGSUPPORT`, using its compile command with the define removed for an isolated check.
  Standard CMake targets currently define the macro unconditionally.
- Audit generated DXF with ezdxf and inspect representative DWG output with an available independent reader such as
  LibreDWG, ACadSharp or ODA. Verify the ordinary spline and registered metadata, record the verifier/version and
  results, and state unavailable tools explicitly; self-reader acceptance is not evidence of external verification.

**Done when:** the complete native matrix passes with no silent entity drops, no new-type known gaps, and no semantic
fallback for valid supported files. Record version/case counts and any external-reader results in the step status and
release evidence. The public ASCII DXF and native DWG write paths, both import paths, and existing binary DXF codec
read/write are covered.

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
  `QT_QPA_PLATFORM=offscreen build/librecad_tests "[lc-offsetcurve][gui]" --order decl`.
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

(none yet)

### Plan review 2026-10-15

The following findings are addressed in this revision of the plan. Their product implementation and verification are
still pending in the owning slices; a documentation change is not evidence that the code has been fixed.

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
