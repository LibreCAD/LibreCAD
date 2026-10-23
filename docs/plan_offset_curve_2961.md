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

# Editable Ellipse Offset Curves: Implementation Roadmap (#2961)

Updated: 2026-10-22. Status: A-core and X-H1 committed; A-strict in progress; native feature incomplete.
This is the single active roadmap for this work. It replaces the routing rules
and duplicated status mirrors in the earlier draft, not its product requirements.
This checkpoint records tested A-core and auxiliary X-H1 commits. Main-track
successor slices remain incomplete; no native entity or complete topology checker
is claimed.

## 1. Verified Baseline and Scope

| Item | Verified state |
|---|---|
| Working tree used for this update | `/private/tmp/lc2961-v513-plan` |
| Initial audited checkpoint | `9309885cef66aef0cca0f3c8862e0c210518da3b` |
| Initial product base | `9c06e9c77b313b62cc20e9ec39c22952fee501f2` |
| Difference between initial checkpoints | Only this plan file; no product-source change |
| Roadmap consolidation commit | `87ebe5428` |
| Current feature branch | `codex/offset-2961-implementation` |
| Latest product checkpoint | `0eb2a0cfa31c29ba7740e4fb698c8417f4d43fa5` (A-core) |
| Latest auxiliary checkpoint | `29ab281635f85590a7db1f7a02db1fcf65ce3cfb` (X-H1) |
| Geometry implementation | `lc_ellipseoffsetgeometry.{h,cpp}`, `lc_parametriccurveintersection.{h,cpp}` |
| Existing geometry stages | Normalization, reference frames/jets, critical/event/component analysis, cache layout, bounds, local topology |
| A-core | Atomic work adoption, separate ordering allowance and sticky terminal status implemented; A04/A10 registered |
| Missing geometry stages | Strict-solver/geometry accounting integration, occurrence terminals, complete global topology checker |
| Native entity | No `LC_OffsetCurve` or `EntityOffsetCurve` yet |
| Product integration | No native offset adoption, private file schema, creation/editing UI, or target-trim integration yet |
| Build registration | Both geometry implementations in CMake and qmake; both geometry test TUs in `librecad_tests` |
| Validation harness | X-H1 promoted into scripts/offset-curve-validation; X-H2 onward pending; historical scratch retained unchanged |
| Latest validation | A-core, geometry and format fast lanes passed Release/ASan; full Release and qmake6 passed; see section 14 |

Recheck these facts against the selected implementation checkout before starting
A-strict. Inventory Qt6/Catch2/build tools and available independently authored
DXF/DWG fixtures/inspection tools once, recording provenance/access gaps early;
source-level progress need not wait for unrelated external-tool installation. The user's main checkout is a different, dirty branch and is not the
source baseline above. Do not overwrite its unrelated changes or apply this plan
to it without first reconciling the branch and source differences. Use the existing
isolated feature worktree or a suitable managed worktree.

### Historical Preservation

The full pre-consolidation, unfrozen V513 draft was copied without modification:

- Path: `/private/tmp/lc2961-plan-history-v513-20261022.md`
- Length: 5,596,050 bytes.
- SHA-256: `845240e8b9b21e87c8ffed3e620bdff595afed30b93f152a2fdfe3f8c659ed40`.
- Committed history remains available from checkpoint
  `9309885cef66aef0cca0f3c8862e0c210518da3b`.
- Frozen predecessor V512 remains untouched at
  `/private/tmp/lc2961-v512-git/docs/plan_offset_curve_2961.md`;
  SHA-256 `209a86a9e0dab1966057a9788bbc083b70f149092d4c3406377330614b741b34`.

Those are historical evidence, not alternative implementation instructions.
Temporary archives are local evidence, not promised permanent release artifacts.
Preserve any evidence needed for delivery in Git or the project's durable artifact
store before removing a temporary worktree.

The previous V513 mechanical P0a-d result applies only to the old draft bytes.
It does not validate this rewritten plan. Actual A-core source review and test
results are recorded in section 14. No candidate freeze, formal receipt, or
whole-feature completion is claimed.

### Scope That Must Not Be Reduced

Implement editable offsets of 2D ellipses and elliptic arcs, with an owned source
definition and an ordinary spline cache. Include native creation, repeated offset,
safe edits/transforms/ownership, selection/properties/grips, DXF and DWG reading
and writing, source-update/restore actions, and Trim/Trim Two.

Trim results intentionally become ordinary spline fragments. General editable
offsets of arbitrary source types, associative links to a live source object,
3D offsets, and a new DWG custom class are not part of this feature.

Keep both cache degrees 1 and 3. Do not silently drop line-shaped engine output,
multiple-piece plain fallbacks, R12 downgrade, legacy input, binary-DXF codec
coverage, unknown metadata preservation, or save-failure atomicity.

## 2. Review Findings and Corrections

| Priority | Finding | Implementation consequence |
|---|---|---|
| P1 | Product work is blocked by recursively generated plan/receipt gates rather than a source dependency | Replace recursive routing with the finite dependency order in section 5; retain real review/test/commit controls |
| P1 | Solver work is adopted through separate counter updates; an exception can hide work already spent | A-core adds atomic adoption; A-strict exposes work on every exit; A-reference integrates both handoffs |
| P1 | Local geometric witnesses do not establish a unique complete traversal | B publishes occurrence-specific terminals; C proves exact-once coverage, legal transitions and phase closure |
| P1 | A valid file projection/repair could incorrectly acquire editability | G-adapter checks the raw carrier before conversion, then invokes the same complete adoption checker |
| P1 | Native creation or serialization could report success while dropping geometry/definition | Explicit kept-plain outcomes, four independent format lanes, registered APPID, and failure propagation |
| P1 | Harness workers can default missing/unknown results to PASS | X-H3 rejects all nonconforming results; independent checking derives outcomes separately |
| P2 | Stale A13/A14 specifications contradict the corrected fixtures | Keep only the canonical caps and nonzero tangent fixture in section 7 |
| P2 | Resource claims omit comparisons, validation visits, capacity preparation and callback multiplicity | Separate counters and an explicit operation/capacity census; no comparator-side failing debit |
| P2 | Helper-only tests or aggregate counts can hide missing integration | Exact named cases, reachable assertions, production call-site review and independent geometry/file oracles |
| P2 | H5 invokes undefined helpers, invents source tuples and omits post-dispatch rechecks | Finite X-H5 implementation and hostile tests before harness completion |
| P2 | H6 inventory omits current predecessor cases; H7 is absent | Preserve existing scenario IDs, add missing cases, generate/check complete manifests and reports twice |
| P2 | Hundreds of historical status mirrors make the next task ambiguous | One status table, one next-slice card, append only bounded commit summaries |

This roadmap is an implementation specification, not an additional permission
system. Carry out the user's authorized work, observing actual tool/sandbox
requirements. A Markdown gate cannot prohibit necessary source reads, tests, or
ordinary implementation. A real failed test or unresolved correctness issue blocks
the affected slice; it does not require another numbered plan candidate.

## 3. Requirements and Closure Evidence

| ID | Requirement | Owners | Evidence required for closure |
|---|---|---|---|
| R01 | Finite 2D domain and correct whole/arc normalization | A-reference, D-core, D-edit | Zero/tiny/full-turn, wrapped/reversed, ratio swap, circle, extreme scale and nonfinite boundary cases |
| R02 | Bounded geometric and complete topology validation | A-core through C | Conservative bounds, hostile controls, unique full path, exact incidence, ambiguity/limit refusals |
| R03 | Runtime consistency and transactional mutation | D-core, D-edit, H-sheet through H-drag | Complete fingerprint; no laundering of externally edited caches; refused/no-op edits preserve state |
| R04 | Clone, transforms, owner/INSERT behavior | D-core, E-tools, F-types | Independent child ownership, attributes, reflection parity, demotion, ASan |
| R05 | Creation, repeated offset and explicit fallback | D-edit, E-tools, E-again, H-enable | Option-off parity, one-piece adoption, multi-piece and zero-sum fallbacks, bounded attempts |
| R06 | Complete type dispatch | F-types, F-names, H-sheet through H-drag | Current source census with every site disposition; no silent save/drop/cast gap |
| R07 | DXF and DWG application read AND write | G-adapter, G-fixtures, G-matrix | Separate independent read and inspected-write evidence at every applicable row; conversions and edited reopen |
| R08 | Safe schema/metadata compatibility | G-schema, G-fixtures | Typed bounded tail, APPID linkage, duplicate/malformed/unknown policies and stale-owned-data removal |
| R09 | Selection, properties, dialog, grip editing | D-grips, H-sheet, H-dialog, H-drag | Success/refusal/cancel/no-op, mixed selection, one undo step, no motion-time engine/checker |
| R10 | Update from Source and Restore Original | I-update, I-restore, I-ui | Live owner checks, deduplication, no stored live dependency, undo and both-format persistence |
| R11 | Build/regression portability | Every source slice, Z-release | CMake and qmake6; focused/adjacent/full/fast lanes; available GCC/MSVC CI evidence |
| R12 | Bounded irreversible work and responsive UI | A-core through C, E-again, H-drag, T-kernel | Checked work/allocation census, exact/one-less tests, runtime envelope, no hidden unbounded scan |
| R13 | Durable progress after each implementation slice | Every source slice | Reviewed/tested product commit, separate self-updating plan checkpoint, next concrete card |
| R14 | No repaired/projected raw carrier becomes editable | G-adapter, G-fixtures | Original Z, normal, counts, nulls, knots, weights, mode and fit-data checked before conversion |
| R15 | Failed save changes neither target nor document | G-adapter, G-fixtures | Existing target byte equality; absent target stays absent; staging cleanup; document/metadata unchanged |

R07 is conjunctive: DXF read, DXF write, DWG read, and DWG write are four separate
application obligations. Codec self-round-trips and geometry-only tests do not
close these rows. External interoperability unavailable on this machine is
reported as unverified, never as PASS.

## 4. Execution and Commit Protocol

Use these five controls once per finite slice:

1. **Completeness:** map its R IDs to concrete implementation sites, positive and
   refusal tests, and independent expected results. Do not transfer unfinished
   acceptance to a later slice without naming the actual dependency.
2. **Plan correctness/readiness:** verify current signatures, ownership, arithmetic,
   exception behavior and an executable test recipe. Resolve mathematical/API
   contradictions before using them, with a local plan correction rather than a
   new lifecycle/version chain.
3. **Implementation correctness:** review the complete diff and run the exact
   focused tests plus affected regressions. Record actual source/build/test
   identities; an implementation may not certify itself by reading its own output.
4. **Commit the slice:** stage only reviewed source/tests/build-registration files,
   inspect the staged diff, and commit the validated tree. Do not include unrelated
   work. Keep hooks and compare committed blobs with the validated ones.
5. **Self-update:** immediately record the actual product SHA, paths, commands,
   outcomes, residual risks and next slice in this file, then make a separate
   plan-only commit. Review that checkpoint before the next product edit. Never
   make the document depend on its own future commit SHA or a future review ID.

A source/build change after validation requires rerunning affected validation.
Existing failures require exact pre-change evidence; do not alter an expected
result or add a skip simply to obtain green output. Distinguish self-review from
independent review, and never invent an independent PASS. Use an available
authorized reviewer for shared geometry, persistence and ownership changes; any
unobtained independent review remains explicit in the final audit.

A failed experiment preserves prior results and gets a new attempt directory.
Do not restart the entire plan, reuse a stale receipt, relabel an old test run, or
create another active plan. Product commits can proceed in order without completing
the optional execution of unrelated external tools; full release closure still
requires their declared evidence or an explicit unresolved status.

### Slice Card Template

Each next card records:

- ID, status, prerequisite commits and requirement IDs.
- Exact files to edit, existing API facts, new API behavior and non-goals.
- Ordered implementation operations and ownership/publication point.
- Finite numeric/work/output limits and exception/refusal policy.
- Exact test names/selectors, independent oracle, expected assertions and commands.
- Commit scope, validation evidence paths, unresolved items and next slice.

Statuses are `pending`, `in progress`, `validated`, or `committed`.
Use `blocked` only with a specific external or correctness dependency and a
named next action. A document edit alone cannot promote a product status.

## 5. Finite Implementation Order

Execute the main track in the listed order. A dependency is the prerequisite
implementation, not a recursive approval packet. Each row is a separate source/
test commit followed by the section 4 plan checkpoint.

| Slice | Dependency | Deliverable | Status |
|---|---|---|---|
| A-core | Existing geometry base | Atomic work adoption and sticky terminal status (`0eb2a0cfa`) | committed |
| A-strict | A-core | Strict-solver bounded ordering/validation/capacity and exit snapshots | in progress; not committed |
| A-reference | A-strict | Geometry scans/callback ownership and exception-safe work transfers | pending |
| A-cache | A-reference | Candidate/proof split accounting and inherited combined depth | pending |
| B | A-cache | Immutable occurrence-specific producer terminals | pending |
| C | B | Full unique-path topology checker and public result | pending |
| D-core | C | Native enum/entity, real validated adoption, stamp, clone and transforms | pending |
| D-edit | D-core | Real create/mutators, one-run staging, run limits and refusal results | pending |
| D-grips | D-edit | Cached definition grips, safe snaps and through-parameter query | pending |
| E-tools | D-grips | Tool result adoption, explicit plain fallback, explode/scale/INSERT/plugin | pending |
| E-again | E-tools | Bounded repeated offset/rebase with plain fallback | pending |
| F-types | E-again | Complete engine/document/filter type-site integration census | pending |
| F-names | F-types | Names, quick info, context/selection/menu descriptors | pending |
| G-schema | F-names | Shared typed XDATA/EED definition parser/merger | pending |
| G-adapter | G-schema | Actual application DXF/DWG read/write and failure propagation | pending |
| G-fixtures | G-adapter | Hostile metadata/raw carrier, ownership and failed-save regressions | pending |
| G-matrix | G-fixtures | Independent fixtures/output inspection for complete version/conversion matrix | pending |
| H-sheet | G-matrix | Refusal-aware property provider and busy cursor | pending |
| H-dialog | H-sheet | Full Edit Properties widget and commit path | pending |
| H-drag | H-dialog | Bounded mouse preview, one-run grip commit and undo | pending |
| H-enable | H-drag | Persisted default-on option and kept-plain command messages | pending |
| I-update | H-enable | Update from Source action and tests | pending |
| I-restore | I-update | Restore Original action and equality/dedup tests | pending |
| I-ui | I-restore | Source-action registration/links and persistence/GUI coverage | pending |
| T-kernel | I-ui | Pure bounded stored-cache trim preparation/clipping | pending |
| T-actions | T-kernel | Trim/Trim Two atomic owner-level replacement | pending |
| T-files | T-actions | Trim persistence/demotion across format lanes | pending |
| Z-release | T-files and auxiliary track closure | Complete regressions, build/GUI/interoperability audit | pending |

The auxiliary harness track is X-H1 -> X-H2 -> X-H3 -> X-H4 -> X-H5 ->
X-H6 -> X-H7 (section 13). Retain these obligations as a separate finite
deliverable. They must not block starting/testing product slices; complete them
before Z-release and before claiming the whole roadmap done. Ordinary tools can
validate product slices while this track is unfinished. No old harness receipt
grants product correctness.


### Implementation File Map

All paths below are relative to the selected feature worktree. New paths are
proposed, not claims that a class/test already exists. Confirm signatures before
editing; changes beyond the named concern require a scope note in the slice card.

| Owner | Existing sites / planned additions |
|---|---|
| A-core/A-reference/A-cache/B/C | librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.{h,cpp}; tests/lc_offsetcurve_geometry_tests.cpp in that entity directory |
| A-strict | Same directory: lc_parametriccurveintersection.{h,cpp}; tests/lc_parametriccurveintersection_tests.cpp |
| D-core/D-edit/D-grips | librecad/src/lib/engine/rs.h; entity directory rs_spline.{h,cpp}, rs_insert.cpp, new lc_offsetcurve.{h,cpp}, existing lc_offsetoutputbudget.h, new tests/lc_offsetcurve_tests.cpp; container/tests/lc_copy_semantics_tests.cpp; interim action guard |
| E-tools/E-again | librecad/src/lib/modification/rs_modification.{h,cpp} and tests/rs_modification_offset_tests.cpp; optional small lc_offsetcurveagain helper; librecad/src/main/doc_plugin_interface.cpp |
| F-types/F-names | Current rg-derived enum/dispatch census; lib/selection/metaentity/lc_entitymatchdescriptorsregistry; ui/action_options/selection/lc_selectwindowoptions; entity name/quick-info/context/menu-activator registries |
| G-schema | librecad/src/lib/filters/new lc_offsetcurvexdata.{h,cpp}; tests/lc_offsetcurvexdata_tests.cpp |
| G-adapter/G-fixtures/G-matrix | librecad/src/lib/filters/rs_filterdxfrw.{h,cpp}; existing filter tests/testdata and new lc_offsetcurve_filter_tests.cpp; libraries/libdxfrw only for demonstrated codec loss plus focused codec tests |
| H-sheet | ui/dock_widgets/property_sheet/metaentity/entities/new lc_propertiesprovider_offsetcurve.{h,cpp}; lc_entity_property_containerprovider; shared busy-cursor utility and refusal text |
| H-dialog | actions/drawing/modify/lc_action_modify_entity.cpp; ui/dialogs/entity/lc_dlg_entityproperties.cpp; new lc_propertieseditingwidget_offsetcurve.{h,cpp,ui} |
| H-drag | librecad/src/actions/drawing/rs_actiondefault.cpp; action/snapper tests |
| H-enable | actions/drawing/modify/lc_action_modify_offset.{h,cpp}; ui/action_options/modify/lc_offset_options_widget.{h,cpp,ui}, lc_offset_options_filler.cpp; option/action tests |
| I-update/I-restore/I-ui | actions/drawing/modify/new lc_action_offsetcurve_updatesource/restoreoriginal.{h,cpp}; tests/lc_action_offsetcurve_source_tests.cpp; action factories/context/property links |
| T-kernel | librecad/src/lib/modification/new lc_offsetcachetrim.{h,cpp}; tests/lc_offsetcachetrim_tests.cpp |
| T-actions/T-files | actions/drawing/modify/lc_action_modify_trim.{h,cpp}, tests/lc_action_modify_trim_tests.cpp; modification/batch ownership and offset filter tests |
| All new sources/tests/UI | CMakeLists.txt; librecad/src/src.pro (HEADERS/SOURCES/FORMS as appropriate); exact Catch2 target registration |

### Immediate Next Card: A-strict

**Files (only these three):**

- `librecad/src/lib/engine/document/entities/lc_parametriccurveintersection.h`
- `librecad/src/lib/engine/document/entities/lc_parametriccurveintersection.cpp`
- `librecad/src/lib/engine/document/entities/tests/lc_parametriccurveintersection_tests.cpp`

**Base:** committed A-core `0eb2a0cfa31c29ba7740e4fb698c8417f4d43fa5`.
Geometry-side counters/adoptWork/latches now exist. StrictSolver still has
uncounted ordering, post-callback certificate admission, incomplete record/
capacity charging and no factory workOnExit. Its segment vector is already
borrowed; preserve that lifetime contract and make validation/accessor work explicit.

**Steps:**

1. Capture current strict/refinement and planned-selector discovery in Release/
   ASan. Append solver ordering limit/stats and validation visits/next-call stats;
   preserve existing aggregate field meanings and ordinary-mode decisions.
2. Bound validation and mandatory seed capacity before scans/geometric callbacks.
   Count accepted leaves separately from pending roots. Preserve canonical A13
   full-query caps 4/3 ordinary and 6/5 retained, plus separate preflight refusals.
3. Reserve bounded seed/split/output/scratch capacity before debit/storage.
   Admit two children atomically; account for remaining mandatory seeds. Record
   actual entered depths, not a refused or merely queued child's depth.
4. Admit certificate callbacks before invocation, including unavailable/throwing
   results. Atomically preflight the identity callback's pair/item dimensions.
   Remove duplicate parent/helper charges; keep the three-term pair total.
5. Replace only strict ordering with deterministic counted operations whose
   comparison admission occurs outside the comparator. Ordering cap zero is a
   valid zero-comparison strict allowance: refuse LimitExceeded before a needed
   predicate; do not change ordinary mode or geometry CheckLimits' nonzero rule.
6. Add optional workOnExit to the certified factory and friend declaration.
   Initialize before guards; report admitted setup/query work on all returns and
   throws without replacing the provider exception. Retained invalidation/retry
   preserves cumulative work and does no work after invalidation.
7. Implement A08/A12/A13 in the budget-accounting selector and A14 in
   intersection-refine. Retain existing regression names and independently
   expected geometry. Run focused/adjacent/full Release, geometry and fast
   Release/ASan, plus incremental qmake6. Review the complete three-path diff.
8. Commit the validated slice, then update this roadmap in a separate commit.
   Next is A-reference; it owns geometry-side query handoffs and fixed scans.

**Exit:** four new exact identities pass with nonzero assertions, retained-throw
and tight-cap oracles hold, existing strict/geometry identities remain covered,
and legacy non-strict behavior is unchanged. No global checker, entity, UI,
geometry-side handoff or file-format edit belongs in this slice. Source changes
outside the three files require a concrete dependency note, not silent scope growth.

## 6. Geometry and Work Contracts

### 6.1 Inputs and Normalization

Use the implemented `LC_EllipseOffsetDefinition`:
normalized `RS_EllipseData basis`, positive distance magnitude,
`towardCentre`, `cacheReversed`, and relative tolerance.

Validate raw finite XY vectors/angles, exactly zero Z, finite nonzero axis and
positive finite ratio before constructing an ellipse. Preserve canonical whole
`(0,0)`. Distinguish a genuine one-turn difference from a zero/tiny sweep before
modulo reduction; do not turn a tiny arc or multiple turns into a whole ellipse.
Full-turn allowance is `1e-12 * 2*pi`; zero/tiny refusal uses
`RS_TOLERANCE_ANGLE`. Test both sides independently.

Canonicalize a genuine full turn before `switchMajorMinor()` for ratio > 1.
Use ellipse setters/current normalization, not direct angle/isArc field writes.
Wrapped and reversed arcs retain their directed sweep. Validate finite normalized
results. Do not conflate raw file validation with ordinary visible-geometry repair.

The geometry checker never runs the materializer. The physical signed outward
distance is `towardCentre ? -distance : distance`. Engine left/right mapping is
`towardCentre = (side == Left) != basis.reversed`. Candidate reversal only
changes traversal; it never changes physical side or source orientation.

### 6.2 One Irreversible Budget

Keep current starting limits: 4096 candidate spans, 65536 logical vertices,
`1 << 20` geometric/event pairs, maximum combined depth 20. Qualify corpus
acceptance and a finite runtime envelope before release. Add a separate
`maxOrderingComparisons` limit with finite initial default `1 << 20` in
both the shared geometry ledger and strict-query options. Qualify its acceptance/
runtime envelope from the call graph and corpus in A-strict/A-reference; any
adjustment needs explicit evidence and regression tests, not another plan gate.
Old withdrawn `1 << 26`/`1 << 27` guesses are not defaults. Do not silently clamp an
unsupported caller value or broaden pair meaning.

| Category | Charged operation |
|---|---|
| cacheSpans | Initial normalized candidate spans once; each candidate child span before construction |
| vertices | Each logical proof/terminal/witness/state/arc/frontier/signature record before first storage |
| distancePairs | Each declared geometric/event/topology examination, seed/transition and non-sort comparison |
| orderingComparisons | Each strict/fixed ordering or four-value reduction comparison before evaluation |
| deepest | Maximum entered `cacheDepth + referenceDepth`, checked without overflow |
| validationSegmentVisits / validationNextCalls | Separate strict-solver validation statistics; not added to either pair or comparison cap |

Every dynamic loop/allocation/data-dependent scan needs a source-bounded owner.
Record operation, logical unit, admission location, maximum, capacity preparation,
exception path, public work delta and test. Fixed exclusions need a source proof
and runtime envelope, not a claim that they are "small".

Preflight `size + delta`, `max_size()`, remaining budget and paired reserves
before a not-yet-admitted operation. Reserving capacity is not generating logical
records: do not charge unused capacity and charge the same record again later.
A reserve failure changes no logical items/counters, though earlier capacity
reservations may remain. After debit, paired appends must be demonstrably
nonthrowing or use a staged publication that cannot expose half a pair.

Atomic adoption receives complete additive deltas plus absolute depth maximum.
Any dimension/arithmetic/depth failure preserves all prior counters/depth and
latches LimitExceeded. First terminal status wins; no operation refunds spent work,
resets a stage ledger or publishes a successful prefix.

Candidate split: atomically admit `cacheSpans += 2, vertices += 2` after
capacity preparation. The parent is popped, not recreated; no third vertex.
Proof-only split: `vertices += 2, cacheSpans += 0`. Apply the distinction at
both cache split sites and all three `CriticalBuilder::split` callers.
A proved region reuses its admitted proof-box token in paired region/segment
storage; do not double-charge a representation transition.

Reference subdivision increments inherited reference depth; candidate subdivision
and virtual candidate cuts increment candidate depth. Carry both through all
witnesses and check their sum on child entry. A child refused before entry does not
raise observed maximum depth.

### 6.3 A-strict: Solver API and Work

Own `lc_parametriccurveintersection.{h,cpp}` and its test TU.

- Preserve `totalBoxPairs = candidateBoxPairs + refinementBoxPairs +
  certificateBoxPairs`. Ordering and validation counters are separate.
- Borrow immutable segment/metadata views once with correct provider lifetime.
  Bound input validation before accessor/geometry calls; no unbounded copy or scan.
- Admit each solve cell, candidate pair, root/event/output record, callback and
  ordered-occurrence scratch before work/storage. Checked reserve precedes debit.
- Each solver-dispatched certificate/identity callback receives one admission
  before invocation, including unavailable/throwing callbacks. Paired pair/item
  limits are preflighted together. A refused callback never runs.
- `certificateBoxPairs` also includes explicitly named contraction/order-attempt
  units; it is not simply callback count. Do not double-count analytic/merge work.
- Replace in-scope strict `nth_element`/sort operations with deterministic,
  explicitly counted ordering (heapsort/BVH construction as appropriate).
  Charge outside any standard-library comparator; canonical ties are deterministic.
  Preserve the three legacy non-strict solver ordering paths and existing behavior.
- Checked scratch formulas include `2*n-1`, `n`, `2*n`. Admit one logical
  order-scratch token per event. Empty/singleton ordering does no comparisons.
- Derive/reserve the queue bound `1 + min(D, P, floor(I/2))` from remaining depth,
  pair and item allowances with overflow checks. Preparation omits the pair term
  and accounts separately for still-reserved mandatory seeds. Reject an invalid
  bound rather than reserving the caller's unconstrained maximum.
- Add an optional `workOnExit` output to `findCertifiedIntersections`, including
  its friend declaration. Initialize before validation; fill on all returns and
  exceptions. Preserve existing source call compatibility.
- Retained `cumulativeWork()` is monotone. A throwing retained query invalidates
  its credential/root, preserves admitted work, rethrows the original exception,
  and retry performs no work. No allocation/callback fault hook only for tests.

### 6.4 A-reference: Scans and Both Handoffs

Own the geometry header/implementation/test TU, consuming the A-strict API.

- Stable in-place insertion sorting suffices for the fixed inventories:
  at most 21 axis entries and 20 critical occurrences. Stationary and intermediate
  regimes are mutually exclusive; stationary occurrences are at most 11.
- Admit each strict finite-key comparison outside a comparator. Equal keys retain
  generation order. Axis sort maximum: 210 comparisons/250 moves; occurrence sort:
  190 comparisons/228 moves. Zero/singleton counts are zero.
- Four-value rational reductions need six comparisons after their unique enclosing
  pair admission. Inventory every fixed scan, dispatch, span advance and witness
  examination; charge before inspecting data.
- Provider internal calls belong to the bounded envelope of their outer callback.
  In particular identity certification invokes its intersection helper twice; these
  are not two new solver-dispatched callbacks. Outside-core geometry certificates
  have a separately named owning admission.
- Adopt one-shot and retained-query deltas atomically, exactly once, including
  exceptional exits. Compute deltas safely from monotone snapshots; a caught
  exception cannot lose work or be replaced by an accounting exception.
- Reserve the initial classify root, retained-record storage, staged snapshots,
  paired region/segment slots and join output before debit/append.
- Preserve immutable provider/event ownership after caller lifetime ends. No partial
  analysis/session escapes a failed admission.

### 6.5 A-cache and B

A-cache integrates the split/depth/coverage rules into
`CacheBoundsBuilder::compare`, `CacheLocalTopologyBuilder::subdivide`,
proof-only critical splitting, and bounds dispatch/coverage advancement.
Test the production helper AND inspect/test the actual call sites.

B publishes immutable terminals keyed by exact role, source endpoint or event,
occurrence, incident directed edge and edge side. A rounded coordinate or shared
node does not establish terminal identity. Pair-trim is terminal only when no
compatible paired retained continuation exists. Through events, hidden cusps,
wrong occurrence orientations and arbitrary interior points are not terminals.
Keep producer-provenance mapping separate from public cache validity.

### 6.6 C: Full Geometry/Topology Transaction

Planned public `LC_OffsetCheckResult` has reasons:
`None, InvalidInput, Layout, TooSmall, ReferenceUnresolved, OffCurve,
Ambiguous, Topology, LimitExceeded`, original candidate span attribution
(or npos before attribution), and final shared `CheckWork`.

`checkCache(cache, definition, tol, limits)` performs all of:

1. Validate finite positive tolerance/distance, normalized source, limits and layout.
   Degree 1 with n spans: n+1 controls/n+3 knots; degree 3: 3n+1 controls/3n+5 knots.
   Require ClampedOpen data, exact unit weights, no fit/saved-open repair payload.
   End multiplicities are 2 or 4, internal multiplicities 1 or 3, finite strictly
   increasing knot groups. Accept positive affine/narrow knot domains, not merged
   groups. Check exact counts/arithmetic before allocation.
2. Distance <= tolerance returns TooSmall. Analyze restricted-source visibility,
   events and components with the same shared budget and immutable reference data.
3. Bound every candidate/reference subcurve with outward intervals in the
   centre-relative, uniformly scaled ReferenceFrame. Retain every relevant visible
   edge/event alternative; do not greedily pick the first witness.
4. Reuse the committed pair-proof/local-topology machinery. Certify actual refined
   pairs, not a coarse parent's inherited Hausdorff result. Candidate line order
   must be strictly increasing. Cubic projected derivative Bernstein coefficients
   are all nonnegative with at least one positive; stationary endpoints are allowed.
   Reference projected derivative is nonnegative with strictly separated endpoint
   projections. Normalize cacheReversed once, never flip a cell independently.
5. Build bounded directed states/transitions over touching normalized candidate
   domains and exact reference incidences. Legal transitions are same-edge progress,
   exact source continuation without hidden interval, authorized pair-event
   incoming/outgoing incidences, and explicit retained seam. Never join by
   coordinate equality or query a reference jet across the wrap.
6. Select one complete candidate-compatible producer path; other reference branches/
   components can remain unselected. Tile all candidate domains and every selected
   edge's visible interior exactly once. A fringe/node certifies an endpoint/
   incidence, never positive-length coverage. Preserve both roles at a join.
7. For closed candidates, consider all certified start cuts; unroll at most one lap
   per seed and return to its own stable paired frontier. Coalesce equivalent
   subdivisions only while retaining proofs. Normalize the complete directed
   edge/occurrence/event signature modulo rotation after each independent one-lap
   proof; different interior cuts on the same edge alone are not another cycle.
8. Memoize equivalent partial states and count semantic paths with saturation at
   two. A second non-equivalent cycle/order is Topology; unresolved cut equivalence
   is Ambiguous. Never enumerate exponentially many duplicate assignments.
9. Verify endpoint policy and closure flags: periodic requires closed; either true
   flag requires exact closed endpoint controls; both false may still describe a
   geometrically closed clamped cache. Do not infer periodic derivative continuity.
10. Publish success only after complete bounds, local order, legal unique path,
    bidirectional exact-once coverage, terminal/cut return and flag checks.

Geometric proof uses setwise Hausdorff bounds, not equal parameters or samples.
For chord deviations eC/eQ and symmetric endpoint-to-opposite-segment chord bound
H, use `E = eC + eQ + H`. A reference second-derivative bound M2 over width h
gives `eQ <= M2*h*h/8`. The cubic chord-difference Bernstein controls are
`0, P1-(2P0+P3)/3, P2-(P0+2P3)/3, 0`.
Include conversions/rounding in outward bounds.

Require `E.hi <= (2*tol/frame.scale()).lo`; independently require source
clearance lower bound `Dlocal-E` to be at least the upper bound of
`(abs(distance)-2.5*tol)/frame.scale()`. The exact-D premise applies only to
the certified visible restricted-source fragment. A whole-ellipse foot/sample
cannot prove an arc's visibility/clearance.

For a fringe, retain its exact occurrence/node and full isolating enclosure.
Candidate-to-node error determines its clearance loss; fringe-to-node separation
separately enters geometry/reverse coverage. Do not double-subtract that separation
or give a hidden root side exact-D clearance. Hidden/cutter-only geometry cannot
witness the cache.

Public reasons describe evidence, not guesses: lack of a proved witness may be
OffCurve only with separation proof; unresolved intervals/cuts are Ambiguous;
unresolved visibility/classification is ReferenceUnresolved; invalid/missing/
nonunique incidence/coverage is Topology. Exhaustion takes precedence over
stage-level unresolved status and returns LimitExceeded plus final work.
Do not add TooNear without an independent upper-bound proof.

Required falsifiers include a cubic line net `[0,4/3,-1/3,1]` whose image is
close but traversal reverses; a passing prefix with missing/repeated tail;
coordinate-equal wrong occurrence; branch switch; false seam join; ambiguous
cycle; and invalid closure flags. A successful subset is never an editable cache.


## 7. Exact Geometry Acceptance Tests

Every listed identity is a separate Catch2 TEST_CASE with a reachable, nonzero
assertion contract. SECTIONs/aliases/skips cannot stand in for identities. Preserve
the existing strict regression names as an exact set; the historical count 61 is
not a current test receipt. A04/A10 are implemented and validated; the remaining
A/B/C identities are planned, not already registered.

### 7.1 A: Four Leaves, Fourteen Identities

A01-A13 use `[lc-offsetcurve][geometry][cache-budget-accounting]`.
A14 uses `[lc-offsetcurve][geometry][intersection-refine]` in the intersection
test TU. Record its addition separately from existing refinement tests.

| ID | Exact test name | Owner |
|---|---|---|
| A01 | `cache split atomically charges two children` | A-cache |
| A02 | `one-less split budget refuses without partial debit` | A-cache |
| A03 | `reference depth survives subdivision and enforces combined depth` | A-cache |
| A04 | `shared work budget preserves sticky refusal` | A-core |
| A05 | `axis inventory ordering comparisons are separately budgeted` | A-reference |
| A06 | `critical occurrence ordering comparisons are separately budgeted` | A-reference |
| A07 | `critical fixed scans charge before iteration` | A-reference |
| A08 | `provider analytic scans are covered by query debit` | A-strict |
| A09 | `bounds dispatch and coverage advancement charge before work` | A-cache |
| A10 | `work budget terminal latches preserve counters` | A-core |
| A11 | `geometry root and retained-record capacity checks precede debit` | A-reference |
| A12 | `strict-core seed and split admissions precede queue mutation` | A-strict |
| A13 | `strict-core validation, pair work, ordering comparisons and output scratch are bounded` | A-strict |
| A14 | `retained certificate callback throw preserves admitted work` | A-strict |

A01/A02 start at one charged span and vertex: two children produce final counts
3/3. One-less in either dimension refuses with both previous counts still 1.
Review production calls as well as the helper: the parent pop adds nothing and
proof-only splits add no candidate span.

A04 tests complete adoption from nonzero pre-spent state, each additive dimension
individually short, over-depth and overflow. A10 directly tests both public terminal
latches in both orders, preservation/idempotence and admission after refusal.

A08 asserts one pre-callback admission for available/unavailable direct calls,
atomic pair/item preflight, no callback on refusal, no refund on throw, and bounded
internal multiplicity without duplicate debit. Contraction/order parents remain
separate units. A11/A12 test observable small-cap boundaries; review the actual
max_size/checked arithmetic ordering rather than inventing allocator hooks.

**Canonical A13 fixture (earlier smaller full-query caps are retired):**

| Subcase | Expected result and work |
|---|---|
| Two valid line segments, ordinary cap 4 | Complete query succeeds |
| Same, ordinary cap 3 | Second-leaf item admission refuses; no partial public result |
| Same, retained cap 6 | Complete retained query succeeds |
| Same, retained cap 5 | Second-leaf item admission refuses; no session |
| Ordinary cap 1 / retained cap 3 | Separate mandatory-root preflight refusal, not full-query success |
| Retained cap 0 | InvalidInput before setup/accessors/callbacks, zero generated work |
| Retained cap 1 | LimitExceeded before setup/accessors/callbacks, zero generated work |
| Sufficient cap, second interval malformed | InvalidInput, distinct from root-cap refusal |

Use an independent straight-line provider. Assert each validation visit,
next/accessor/callback count and cumulative snapshot, not merely status. Include
oversized provider input in synchronous/retained entry points, exact/one-less
pair and ordering caps, canonical ties, output scratch and no retained owner on
failure. Populate workOnExit even for early/exception exits.

**Canonical A14 fixture:** distinct branches
`A(t)=(t,0)`, `B(u)=(u,(u-1/2)^2)`, domains [0,1].
At t=u=1/2 both derivatives are (1,0), nonzero and parallel. The singular Jacobian
makes Krawczyk inconclusive and reaches the source-supported analytic fallback;
do not force a zero derivative. Create a valid retained session before enabling
exactly one throwing certificate callback. Require original exception propagation,
certificate/total pair counters each +2 (contraction plus callback admission),
deepest root refinement +1, credential/root invalidation, and no work on retry.
The existing throw-bounds test stays separate.

### 7.2 B: Eight Terminal Identities

Selector: `[lc-offsetcurve][geometry][reference-terminals]`.

| ID | Exact test name |
|---|---|
| B01 | `source start maps to exact terminal` |
| B02 | `source end maps to exact terminal` |
| B03 | `pair trim terminates without paired continuation` |
| B04 | `paired retained continuation blocks trim terminal` |
| B05 | `through event is not a terminal` |
| B06 | `wrong paired occurrence orientation is not a terminal` |
| B07 | `hidden simple cusp is not a terminal` |
| B08 | `arbitrary interior point is not a terminal` |

Assert exact terminal role/event/occurrence/node/edge side and producer mapping,
not coordinate proximity. B04 uses the G2 incomplete E0-only lens. B07 verifies
the hidden cusp has no terminal; it is not the public C50 reason test.

### 7.3 C: Fifty-Four Public Checker Identities

Selector: `[lc-offsetcurve][geometry][cache-topology-chain]`.
Partition: C01-C28 path/cut, C29-C37 nine public reasons, C38-C46 attribution/
resources/producers, C47-C54 public endpoint outcomes. B tests do not count as C.

| ID | Exact test name |
|---|---|
| C01 | `cache topology accepts degree one complete path` |
| C02 | `cache topology accepts generated cubic complete path` |
| C03 | `cache topology accepts monotone cubic with stationary endpoint` |
| C04 | `cache topology accepts forward cache orientation` |
| C05 | `cache topology accepts reversed cache orientation` |
| C06 | `cache topology rejects unauthorized disconnected component join` |
| C07 | `cache topology rejects missing reference tail` |
| C08 | `cache topology rejects repeated reference tail` |
| C09 | `cache topology rejects candidate loop and retrace` |
| C10 | `cache topology rejects crossing branch switch` |
| C11 | `cache topology accepts exact event fringe join` |
| C12 | `cache topology rejects wrong event occurrence identity` |
| C13 | `cache topology follows source continuation without hidden interval` |
| C14 | `cache topology rejects continuation across hidden interval` |
| C15 | `cache topology accepts retained seam whole source cycle` |
| C16 | `cache topology rejects hidden seam inward lens shortcut` |
| C17 | `cache topology accepts closed trimmed cycle` |
| C18 | `cache topology rejects open endpoint mismatch` |
| C19 | `cache topology closure metadata false false permits open and closed geometry` |
| C20 | `cache topology closed metadata requires exact closure` |
| C21 | `cache topology periodic metadata requires closed metadata` |
| C22 | `cache topology periodic metadata does not infer derivative continuity` |
| C23 | `cache topology aligns phase at existing knot` |
| C24 | `cache topology aligns forward dyadic interior phase` |
| C25 | `cache topology aligns reversed dyadic interior phase` |
| C26 | `cache topology canonicalizes nearby same edge cut frontiers` |
| C27 | `cache topology rejects distinct directed event cycles` |
| C28 | `cache topology refuses unresolved cut correspondence` |
| C29 | `cache topology directly asserts None reason` |
| C30 | `cache topology directly asserts InvalidInput reason` |
| C31 | `cache topology directly asserts Layout reason` |
| C32 | `cache topology directly asserts TooSmall reason` |
| C33 | `cache topology directly asserts ReferenceUnresolved reason` |
| C34 | `cache topology directly asserts OffCurve reason` |
| C35 | `cache topology directly asserts Ambiguous reason` |
| C36 | `cache topology directly asserts Topology reason` |
| C37 | `cache topology directly asserts LimitExceeded reason` |
| C38 | `cache topology attributes original spans deterministically` |
| C39 | `cache topology admits exact cumulative resource caps` |
| C40 | `cache topology one-less cache span cap refuses atomically` |
| C41 | `cache topology one-less vertex cap refuses atomically` |
| C42 | `cache topology one-less distance pair cap refuses atomically` |
| C43 | `cache topology one-less combined depth cap refuses atomically` |
| C44 | `cache topology preserves shared pre-spent work and sticky refusal` |
| C45 | `cache topology admits deterministic 64 span cycle` |
| C46 | `cache topology selects separate paths from two component producer` |
| C47 | `cache topology accepts source start terminal` |
| C48 | `cache topology accepts source end terminal` |
| C49 | `cache topology accepts pair trim without retained continuation` |
| C50 | `cache topology rejects hidden cusp endpoint` |
| C51 | `cache topology rejects through event endpoint` |
| C52 | `cache topology rejects wrong paired terminal orientation` |
| C53 | `cache topology rejects arbitrary interior endpoint` |
| C54 | `cache topology rejects G2 upper lens E0 only path` |

Each row needs requirement IDs, fixture, independently derived expected reason/
path, stable event/cut signature, exact span/work attribution, reachable assertion
and evidence location. Exact discovery/JUnit identity equality and nonzero console
assertion totals are required. C39 covers every dimension including ordering;
ordering one-less refusal is an additional labeled subcase, not a reason to omit
C40-C44 or silently renumber the table.

C11 and C23-C26 run both cacheReversed values. C24/C25 cross source sweep sign
with both cache orders: all four combinations. For every closed success, assert
both cut sides, directed clearance/coverage, stable frontier identity and exactly
one-lap return. Equivalent cut subdivisions must not create a second path; different
directed event cycles must not be merged.

C09/C12 refuse wrong-return or wrong-occurrence paths even when coordinates agree.
C16/C54 reject a proper subset of the G2 closed lens. C28 distinguishes unresolved
correspondence (Ambiguous) from a fabricated transition at a certified non-event
cut (Topology). Preserve separate labels and reasons.

### 7.4 Independent Fixture Ledger

`E(a,b,start,end)` uses semi-major/minor radii and radians; (0,0) is whole.
Use stable source role/root enclosure/event kind keys. Runtime indices locate
records but are never the independent oracle.

| Fixture | Source and signed outward D | Independent expected traversal |
|---|---|---|
| G0 | E(10,5,0,0), D=+3 | One visible whole cycle with explicit retained seam |
| G1 | E(10,5,0,0), D=-3 | Three retained directed edges, two transverse joins and retained seam |
| G2 | E(100,50,0,0), D=-49 | Closed E0 -> E1 lens; source seam is hidden |
| G3 | E(100,50,10*pi/180,350*pi/180), D=-49.5 | Closed trimmed two-edge lens from an arc; no source seam |
| G4 | Major radius 100, ratio .05, angles pi/12 to 7*pi/4, reference D=-4 | Separate open E0 and closed E1/E2 paths; materializer Left(4) reports engine distance +4 |

For G4 the event-analysis tolerance 1e-5 is not the materializer fit tolerance.
Independently derive root enclosures/visibility and the E0 source-start/cutter
terminal, E1/E2 joins and hidden cusp disposition. Record reference outward versus
engine left-normal signs. C46 admits each separately materialized branch rather
than requiring every component in one cache.

Phase fixture: E(1,1,0,0), D=+1, tol=.02, quarter-turn cubic controls
`(2,0), (2,9/8), (9/8,2), (0,2)`. Split at u=1/4 and rotate the pieces;
physical angle atan2(403,945) is not the reference parameter. Convert with the
outward start/rate lift. Verify independently:
`v=u(1-u)`, `r^2/4-1=v^2(57-50v)/256 <= 89/8192`,
radial error < .011 and positive polar derivative. Test existing-knot, interior
dyadic, event and non-event cuts, both sweep signs and both cache orders.

C50 uses G4's exact binary64 ratio
`3602879701896397/72057594037927936`, D=-4, and the degree-1 segment joining
`(-0x1.8700a72e08b38p+6,-0x1.8c704c47d1742p+1)` and
`(-0x1.8700a72e08b38p+6,0x1.8c704c47d1742p+1)`.
With the real tolerance helper, the independent outward oracle separates this
cusp-only candidate from all visible spans by >28.26 world units.
Expected public reason is OffCurve before terminal/path matching, not Topology.
B07 and the fully witnessed C36 Topology discriminator remain separate.

Derive fixtures analytically or with independent high-precision calculations and
retain provenance. Dense sampling detects regressions but cannot establish
conservative acceptance, exact incidence or whole-path coverage.

## 8. Native Entity, Creation and Tools

### 8.1 D-core: Real Validated Entity

Add `RS2::EntityOffsetCurve`, `lc_offsetcurve.{h,cpp}`,
entity tests and build registrations. `LC_OffsetCurve : RS_Spline` owns the
normalized definition, consistency stamp, validated closure, cached grips, safe
unknown owned flags/tail and existing common attributes.

Required behavior: rtti, clone, definition, isDefined/isConsistent/
hasEditableDefinition, source isWhole, result cacheFormsClosedChain, toPlainSpline,
validated adopt, normalization and transform classification. Add creation/mutation
APIs in D-edit with real implementations, not public success stubs.

- Raw constructors are private with a validated-cache tag. Public adopt invokes
  the complete C checker; no caller-controlled bypass.
- Adopt an exact line as degree 1, knots [0,0,1,1], unit weights, or a supported
  degree-1/degree-3 spline without refitting. Preserve owner, layer, pen, flags,
  visibility and supported metadata.
- Saved cache direction is already represented; adoption never reverses it again.
  Reversal of fresh engine output is a distinct D-edit operation.
- Fingerprint representation/mode, controls, knots, weights, fit/saved-open payload
  and closure flags. Expose/reuse the current anonymous spline geometry hash as
  public static geometryFingerprint, preserving arcLengthTable lookup and adding
  offset coverage for omitted fields. Hash equality detects
  mutation; it is not geometric proof.
- Clone/deep-copy reparents children cheaply. Build the embedded original from
  ellipse data, not a live source clone carrying handles/parent/XDATA.
- Stale/inconsistent/undefined instances remain safely spline-shaped. All edits,
  definition UI and snaps enforce runtime eligibility in release builds.
  Move/reversal cannot launder an externally edited cache into editability.
- No validation/regeneration in update, clone, paint, hover, undo/redo, INSERT
  expansion or ordinary transforms.

Stage finite move/rotate/mirror/uniform-scale changes to cache, definition and
grips. Carry the definition only if consistent before the operation; normalize
and restamp after successful publication. Scale distance by abs(k); reflected
side/cache parity follows the ellipse API. Refuse nonfinite input, zero rotation
vector, degenerate mirror axis, overflow or loss of a representable axis unchanged.

Representable singular/nonuniform scale or nonzero shear loses the definition.
Document tools substitute a plain spline; direct API/hatch clones can retain the
enum but be undefined. Classify uniformity from justified numerical bounds, not a
broad tolerance admitting meaningful shear. RevertDirection toggles cacheReversed
and grip order. Clear unknown owned extensions only on meaningful success.

Add INSERT capability handling, native copy-semantics fixture, writeEntity's
spline-shaped dispatch and an interim safe Edit Properties refusal until H-dialog.
Recount entity coverage rows rather than pinning a historical row count.

Tests: line/cubic adoption, both sides/whole/arcs/lenses, attributes, tampered
fields, no laundering, all transforms including extreme finite coordinates,
no-op/refusal tails, clone independence, uniform/nonuniform/mirrored INSERT,
interim dialog and undefined spline behavior.

### 8.2 D-edit: One Staged Regeneration

Keep limits in the engine layer (for example `lc_offsetoutputbudget.h`),
not a modification-layer include cycle. `LC_OffsetRunLimits` carries the source
output budget and optional maxSamples/maxIntersectionPairs; zero retains engine
defaults, nonzero values clamp them. Preview follows existing preview batch limits.

Refusals: None, InvalidInput, Undefined, Inconsistent, TooSmall, NothingLeft,
SeveralPieces, NotCacheRepresentable, CheckFailed, Engine. Preserve engineStatus
and checkReason, not just a boolean/log line.

- runEngine builds a data-only temporary ellipse, resolves side once, applies
  limits, calls the materializer once and counts that call in engineRuns.
- Map zero results to NothingLeft; multiple to SeveralPieces; an unsupported
  one-result representation to NotCacheRepresentable.
- Normalize output losslessly, run the complete checker, then publish. create uses
  its already-validated private path without validating twice; public adopt never
  gains an untrusted "already checked" flag.
- setDistance/setTowardCentre/setThrough/setBasis require a defined, consistent
  instance, validate input, then prepare the whole next state. No-op runs no engine.
  Refusal preserves cache, definition, grips, stamp, attributes and extensions.
- setThrough proposes distance and side together. Require its point on the final
  retained cache within tolerance. An arc foot outside the sweep or on a removed
  loop cannot qualify.
- setBasis has a rigid-motion fast path only if that motion reproduces all
  normalized source fields, including directed angles/reversal. Test antiparallel
  axis with pi-shifted angles. Changed start/sweep/shape/reflection needs proof
  or normal regeneration.
- Prepare throwing allocations and child ownership before live mutation. A strong
  exception guarantee needs a real no-throw publication/swap strategy.

Test every refusal/no-op, exact engine counts, attributes, zero/one/multiple
outputs, extreme arithmetic, critical/lens cases, direct line normalization even
if current producer examples are all splines, and Release/ASan parity.

### 8.3 D-grips: Cached Grips and Safe Snaps

Compute grips only at adoption/meaningful edits: visible quarter-axis vertices;
arc midpoint or cache-nearest fallback; deduplicate locally; cache arc-length
midpoint if otherwise empty. Retain directed cache order.

Editable getRefPoints uses cached grips; editable moveRef is inert.
Undefined/inconsistent instances delegate to spline control behavior without
restamping. Center snap uses the embedded source. Whole/closed endpoint snaps
use visible grips/corners, never an arbitrary seam; open arcs use cache endpoints.
Whole distance-along snap is invalid. Initialize distance and entity out-pointers.

Through queries are bounded analytic queries, not engine/checker runs. Match grips
by stable index or local deltas plus explicit ULP allowance, not tolerance times
world-center magnitude; refuse ambiguous matches. Test small curves near 1e12,
closed results from arcs and no engine/checker work on repeated paint/hover/query.

### 8.4 E-tools: Adoption, Fallback and Ownership

Add `RS_OffsetData::asOffsetCurve` (data default false until H-enable) and an
explicit keptPlain reason/count. Only committed top-level document ellipses/
eligible curves are wrapped, never previews or generated children.

Adopt already-produced single output without another materializer call.
Several pieces, topology or representation refusal remain visible ordinary
geometry with a reported reason. Preserve option-off output/usage behavior.

Explode once gives one plain spline, then ordinary chords on another explode.
Nonuniform document/plugin/INSERT scale substitutes a plain spline before
transforming. Preserve parent/layer/pen/foreign data; free refused temporaries.
Cover copy/paste/blocks and ordinary ellipse/arc regressions. A spline-derived
container is not an RS_AtomicEntity.

### 8.5 E-again: Bounded Rebase Then Cache Fallback

Eligible repeated offsets never nest definitions. Resolve side on the displayed
cache; combine original signed outward distance and requested increment with
basis.reversed/cacheReversed accounted exactly once.

Rebase on the source only for proved compatible/untrimmed composition. A whole
ellipse's outward-first composition or further inward erosion can qualify under
the retained-set rule; arcs require untrimmed first output plus compatible new
side/curvature/endpoints. Use exact sweep minimum curvature and endpoints.

At most one bounded rebase attempt and one cache fallback per copy. Structural
refusal makes no rebase call. Failed rebase, zero-sum, several pieces or rejected
adoption uses the existing cache offset and reports plain geometry.
Charge attempted work separately and committed output once, using remaining
output/source allowance. Preview stays ordinary bounded geometry.

Mandatory cases: whole E(100,50) and arcs after rotation/translation; both cache
orders; outward10 then four inward5 copies (zero-sum copy must be plain fallback);
near-critical E(100,60), outward10 then inward46.036 successful rebase when the
checker qualifies it; undefined fallback; Keep Originals both ways; one undo batch.

### 8.6 F-types/F-names: No Silent Dispatch Gap

Enumerate current EntitySpline/EntitySplinePoints/ellipse enum switches with rg.
Classify each as direct support, safe delegation, demotion or explicit refusal.
Reconcile actual enum/coverage registry rather than an old fixed count.

Cover filters (save/selection/hatch), INSERT, copy/paste/blocks, explode/scale/
plugin, intersections/distance/area/length, snaps/hits, default action, preview,
trim limits, type-match descriptors, select-window, property/dialog, quick info,
naming/context/menu activation. Chords select their owning curve. Trim-as-limit
leaves it editable; target trim refuses safely until T-actions.

F-names adds name, distance/source/side/length without control-point editing;
descriptor coverage, context Offset/Explode/Revert, Spline selection filter and
menu activator OC. Use established icons/layouts. Test full entity coverage and
child-owner selection; no silent drop-on-save.

## 9. Shared DXF/DWG Persistence

### 9.1 G-schema: One Typed Definition

Ordinary SPLINE is the carrier. DXF uses XDATA; DWG equivalent typed EED under
a registered APPID. Other CAD readers display the cache; no new custom DWG class.

Implement shared `LC_OffsetCurveXData` in the filters layer without DWG-only APIs.
parseAndStrip returns bounded diagnostics, definition/tail/flags and remaining
data. forExport returns optional merged XData so unsafe merge fails the save.

~~~text
1001 LibreCad
1000 LC_OFFSET_CURVE
1002 {
1070 1
1000 ELLIPSE
1011 centerX centerY centerZ
1012 majorAxisDisplacementX majorAxisDisplacementY majorAxisDisplacementZ
1040 ratio
1040 angle1
1040 angle2
1070 flags
1041 distance
1040 relativeTolerance
[safe unknown owned tail]
1002 }
~~~

Exact code/type/order is mandatory. 1012 is displacement, not a second world
endpoint; the private 1011-axis-endpoint prototype must not be misread. Require
finite XY/exact zero Z, ratio normalizing to (0,1], finite positive distance,
factor .1 to 10 times default. Classify raw angles before setters snap them.

Flags: bit0 source reversed; bit1 toward center; bit2 cache reversed; bit3 equals
`towardCentre != basis.reversed`; bits4-14 reserved/preserved. Reject conflicting
bit3 or invalid int/range. Meaningful successful edit clears unknown owned tail/
bits; no-op/refusal/clone/no-edit save preserves them.

Parser/merger rules:

- APPID matches case-insensitively without trimming: " LibreCad " is foreign.
  Split at every1001; keep preamble/foreign order.
- Recognize only a top-level marker followed by an opening brace. Nested markers
  and unrelated following tokens are not ours.
- A typed schema !=1 stays raw/noneditable; never inspect its nested apparent
  schema1 markers. Malformed unknown braces stay diagnosed raw.
- Strip known/malformed owned candidates only within their group; missing closing
  brace strips to its group end, not across another APPID. Multiple known
  candidates, including malformed, invalidate adoption; do not select a convenient
  valid one.
- Status precedence: Invalid, unique Schema1, Other, NoSubList. Unknown schema can
  coexist raw with one valid schema1 definition.
- Safe owned tail:1000, balanced1002, finite1010-1013, finite1040-1042, int16-1070,
  int32-1071. Unsupported owned codes1003/1004/1005 are dropped with a reason;
  foreign payload is untouched. Retain a whole balanced safe tail only at <=64
  items, <=4096 target-encoded bytes, depth<=8. Never truncate through braces.
- Bound scans/copies/reserves before allocation. Preflight the complete balanced
  DWG EED chunk against uint16 payload bytes; no split through an open brace.
- Export strips stale known schema1 even for plain/undefined/inconsistent splines.
  Append exactly one current definition when eligible; merge compatible LibreCad
  groups into the first with its spelling. Never mutate shared DRW_Variant objects.
- Invalid current definition or unsafe merge fails serialization, not silent
  definition omission.

Test parser/writer independently with hand-authored expected types/items, byte
boundaries, duplicates/groups/case/whitespace, unknown coexistence, nested markers,
malformed braces and tail/range limits.

### 9.2 G-adapter: Raw Eligibility Before Conversion

Trace actual `RS_FilterDXFRW::addSpline/writeSpline/writeEntity`,
DRW_Spline encode/parse and common EED handling in both codecs. Invoke the shared
merger before DXF/DWG branching, including R12's early POLYLINE path.

Only original DRW::SPLINE with a unique schema1 candidate can be adopted; HELIX
never becomes editable. Before makeSplineFromDrw projects/repairs it, validate
raw declared/vector counts, nonnull controls, finite XY/exact Z=0, normal,
degree/mode/layout/knots, complete-or-absent unit weights, and no fit/saved-open
repair dependency.

The codec's zero normal sentinel is acceptable only with complete raw 2D checks;
explicit incompatible plane is not. DWG control data normally has no DXF normal.
Do not infer planarity from flags or reject every DWG for its unspecified normal.

Construct eligible controls/knots without converter rounding/synthesis/
reclassification (materialize implied unit weights), then real adopt. Invalid
schema/eligibility/check retains ordinary visible geometry with bounded diagnostic,
stripping only known owned data. Unmarked import retains existing behavior.

Preserve layer/pen/visibility/block owner/foreign XDATA/EED. Reuse writeAppId's
existing LibreCad registration, not a parallel registry. Inspect actual APPID
uniqueness/case and EED handle resolution.

Reuse DwgDxfOutputTransaction, codec finalization/filter staging; no duplicate
transaction. Every encoding/normalization/R12/count/metadata failure latches
m_writeFailed before finalize. Logging and returning is not success. Failed save
keeps target bytes/absence, document/metadata/undo state and cleans staging.
Do not promise durability beyond the existing publication contract.

### 9.3 G-fixtures: Safety and Ownership

Add separately named cases for raw Z/normal/null/count/knot/partial-weight/fit-mode
defects; every malformed/unknown/duplicate/flag/tail schema case through real file
import; foreign-data preservation; no-edit and edited unknown tails; clone/paste/
block/uniform and nonuniform INSERT; no source ellipse on reopen; and hatch
ordinary-spline export.

Failure cases cover unsafe metadata, invalid controls, codec overflow and R12
approximation against existing/absent targets, with exact unchanged document/
target assertions. Include binary input and DWG EED, not helper-only tests.
Do not exclude hostile fixtures from CI or allocate huge inputs for overflow tests.
Use existing filter test support and fixture directories.

### 9.4 G-matrix: Four Independent Application Lanes

Every editable row runs cache degrees1 AND3. Reader fixtures are independently
authored, not produced by this feature's writer. Writer checks inspect actual
output with independent expected payload. Same-format/edit/reopen adds coverage
but cannot substitute for either direction.

| Format | Version | RS2::FormatType | Read | Write |
|---|---|---|---|---|
| DXF | R12 AC1009 | FormatDXFRW12 | Ordinary POLYLINE | Geometry-only POLYLINE, no owned schema |
| DXF | R14 AC1014 | FormatDXFRW14 | Editable SPLINE/XDATA | SPLINE/XDATA |
| DXF | R2000 AC1015 | FormatDXFRW2000 | Editable SPLINE/XDATA | SPLINE/XDATA |
| DXF | R2004 AC1018 | FormatDXFRW2004 | Editable SPLINE/XDATA | SPLINE/XDATA |
| DXF | R2007 AC1021 | FormatDXFRW | Editable SPLINE/XDATA | SPLINE/XDATA |
| DXF | R2010 AC1024 | FormatDXFRW2010 | Editable SPLINE/XDATA | SPLINE/XDATA |
| DXF | R2013 AC1027 | FormatDXFRW2013 | Editable SPLINE/XDATA | SPLINE/XDATA |
| DXF | R2018 AC1032 | FormatDXFRW2018 | Editable SPLINE/XDATA | SPLINE/XDATA |
| DWG | R2000 AC1015 | FormatDWG | Editable SPLINE/EED | SPLINE/EED |
| DWG | R2004 AC1018 | FormatDWG2004 | Editable SPLINE/EED | SPLINE/EED |
| DWG | R2007 AC1021 | FormatDWG2007 | Editable SPLINE/EED | SPLINE/EED |
| DWG | R2010 AC1024 | FormatDWG2010 | Editable SPLINE/EED | SPLINE/EED |
| DWG | R2013 AC1027 | FormatDWG2013 | Editable SPLINE/EED | SPLINE/EED |
| DWG | R2018 AC1032 | FormatDWG2018 | Editable SPLINE/EED | SPLINE/EED |

Additional required lanes:

- Legacy DWG R13/R14 read-only reaching shared adoption; no new writer.
- Binary DXF R14/R2018 codec read AND write; input reaches application adoption.
  Application export stays ASCII, no invented binary application option.
- Both DXF->DWG and DWG->DXF, alternating chains, edit/second save/reopen.
- R12 both degrees independently within approximation tolerance; no re-adoption.
- Explicit enums, never numeric ranges. Guard DWG-only references and compile the
  shared schema without DWGSUPPORT.
- Fixture generator/version/arguments or hand construction, redistribution rights,
  hashes and independently expected geometry/metadata. Commit suitable fixtures;
  CI cannot depend on a local tool install.
- Available external ODA/ACadSharp/libreDWG inspection of actual DXF/DWG output,
  with line-only controls and logs. Known rejection must be resolved or reported;
  same-library reopen cannot establish interoperability. Missing tools/fixtures
  leave their specific evidence unverified.


## 10. Editing UI and Source Actions

### 10.1 H-sheet

Add the offset-specific property provider and registration in the existing
container provider. Show distance, side, source center/radii/rotation, raw start/
end parameters and calculated length. No editable control-net rows.
Undefined/inconsistent definition rows are read-only; runtime setter guards remain.

Refusal-aware setters restore displayed values and report the precise engine/
checker reason, including Inconsistent and LimitExceeded. Multi-selection stages
per-entity edits using preview limits, counts refusals, preserves refused targets
and applies accepted changes in one established undo batch; no dangling clone or
empty property group.

Use an RAII busy-cursor guard only with a real QGuiApplication; no cursor imbalance
on exception/refusal. Test widget/provider construction and edits in librecad_tests,
not a QCoreApplication-only fast lane. Reuse translated command/refusal text.

### 10.2 H-dialog

Replace the temporary safe Edit Properties refusal with a native type case and
an offset editing widget following the ellipse widget's per-field editingFinished
pattern. Register .h/.cpp in CMake and .h/.cpp/.ui in qmake as established.

A refused field edit leaves the clone unchanged, restores its value and shows a
word-wrapped reason inside the dialog. Interactive picked values use the same
setter path. Preserve existing unconditional edited-clone replacement on dialog
accept because layer/pen may also have changed; do not invent geometry-only
equality to suppress legitimate dialog changes.

Test success/refusal/cancel, same-value and attribute-only edits, undefined state,
no unexpected regeneration and real application commit/undo.

### 10.3 H-drag

In the default action, select a stable owning grip, track the raw snapped pointer
and use a bounded analytic/existing-cache polyline preview (at most128 points).
No per-motion clone, materializer, adaptive checker or intersection search.

If a cheap arc/lens preview cannot establish retained topology, display bounded
existing-cache feedback plus the proposed point; do not present a self-crossing
untrimmed curve as the expected final result. Reject nonfinite preview points.

On completion, stage one clone, call setThrough once under the busy guard and
replace once on success. On refusal delete the clone and report; cleanup always
returns the action to Neutral, releases snapper/preview and preserves document/
undo state. Undefined/inconsistent curves keep safe ordinary-spline behavior.

Tests use a real mouse event through press/move/release, not private-state
shortcuts: no engine runs during repeated moves, one replace/undo cycle on success,
side flip, declined/ambiguous grip, invalid point, no leaked clone and exact
undo/redo restoration. Check the application commit path, not preview alone.

### 10.4 H-enable

Only now expose and default on the Editable offset curve option, persisted under
ActionModifyOffset/EditableOffsetCurve. Defaults load with options, not a
constructor side effect. Wire checkbox clicked and proper signal-blocker lifetime
using existing UI conventions. Add the compact options-filler entry.

Messages report plain fallback reasons/counts only when requested on a top-level
ellipse/eligible curve. Keep source outcome diagnostics until completion messages
are assembled. Test absent-setting default, persisted off/on, previews unchanged,
multi-piece notice and existing offset matrix/message pins.

### 10.5 I-update

Implement LC_ActionOffsetCurveUpdateSource with existing preselection-aware
action patterns. Targets must be live document-owned, visible, unlocked, defined
and consistent offset curves. Ignore wrong-type context-menu entities.

Pick a live ellipse in the same editable container, not an INSERT/dimension child.
A visible locked source can be read without modification; target locks still
apply. Highlight/pick performs no engine work. No persistent source handle/link.

Clone, setBasis and replace successful changed targets in one batch. No-op makes
no replacement; refusals keep selection and report counts/reasons. Free all
rejected/no-op clones. Tests: rigid fast path, shape/sweep/mirror/reversal,
invalid/context/owner/lock cases, mixed successes/refusals and one-step undo/redo.

### 10.6 I-restore

Implement LC_ActionOffsetCurveRestoreOriginal. Insert an ellipse from embedded
normalized data with the curve's pen/layer, disabling active-layer/pen substitution.
Skip hidden/locked/undefined/inconsistent targets as established.

Deduplicate against live same-container ellipses and earlier inserts in this batch,
not deleted undo entities or another block. Equality uses local geometry plus a
small explicit ULP allowance, not world-center-relative tolerance. Parallel/
antiparallel axes can be equivalent; exact circles have no unique axis direction.
Whole circles compare center/radius; circular arcs compare endpoints and mid-sweep.
Do not merge near-circles, complementary arcs or numerically uncertain geometry.
An equal ellipse on another layer in the same container still counts as present.

Tests: first/second restore, shared originals, reversed/equivalent circle forms,
distinct near-circles and large-center neighbors, deleted/foreign-block originals,
attributes, ownership and one-step undo.

### 10.7 I-ui

Register both action enums/factory cases, translated QActions, type context entries
and single-/multi-selection property links. Reuse offset/ellipse icons. Do not add
unrequested command names or global Modify toolbar/menu clutter.

Update/restore tests belong to their own slices, not this final integration step.
Here rerun them and test UI links plus DXF/DWG R2018 save/open: updated curves keep
their new editable definition, restored originals reload as ordinary ellipses.
Extend the interactive smoke script with mixed selection, busy state and refusal.

## 11. Trim Stored Offset Caches

### 11.1 User Contract

Offset as Trim limit stays unchanged/editable. Offset as Trim/Trim Two target
becomes ordinary spline fragment(s), preserving exactly the stored displayed cache
within arithmetic error and dropping the owned offset definition. No engine
rerun, refit, analytic-source extension or schema2 is introduced.

Accept visible unlocked live top-level replacement targets. A generated chord
resolves to its curve owner; it cannot become a separate replacement target.
Refuse INSERT/dimension/container children, hidden/locked/foreign-owned targets
and stale/inconsistent curves. Validate each Trim Two operand before mutation.

Open cache: include source endpoints in ordered cut boundaries, remove the unique
clicked interval, retain its positive-length complements (at most two).
Closed cache: require at least two distinct usable intersections, cyclically
remove the clicked interval, emit one open remaining spline.

No intersections is refusal, not unchanged success. Refuse ambiguous projection/
picked boundary, tangency/overlap, unresolved root identity/order, empty output,
nonfinite/unsupported layout, exceeded limits or incomplete prepared output.
No endpoint extension. Coordinate-near distinct roots must not be merged.

### 11.2 T-kernel: Pure Preparation

Planned files: `librecad/src/lib/modification/lc_offsetcachetrim.{h,cpp}`
and `librecad/src/lib/modification/tests/lc_offsetcachetrim_tests.cpp`.
Confirm current shared clipping/intersection APIs and include ownership before
adding the helper; reuse proven spline/Bezier primitives, not a new fit engine.

Inputs are validated cached spline data, a supported cutter, picked chord/
parameter information and explicit work/output limits. Result is an owned prepared
fragment set, refusal, root/fragment counts, endpoint/interval provenance and
consumed work. There are no document/selection/undo mutations.

Required cutter scope: ordinary line, arc, circle, ellipse, and normalized
degree-1/degree-3 spline caches (including another offset cache). Use current
intersection primitives plus conservative refinement. Unsupported curve
representations return an explicit refusal, never a guessed root.

Implementation order:

1. Define finite span-pair, root/refinement/depth, output-span/control/knot and
   comparison limits from the current APIs. Check arithmetic and capacity before
   work/storage, support cancellation and preserve no-partial-output semantics.
2. Resolve the click/chord to one unique (span, parameter) occurrence. Collect
   cutter roots with certified/enclosed parameter provenance; refine/deduplicate
   without merging separate close roots or relying on coordinate equality.
3. Select the unique clicked interval under open/closed rules. Build the retained
   intervals in original traversal order; refusal if selection is ambiguous.
4. Extract by exact knot insertion/de Casteljau subdivision, without refitting.
   Keep direction/weights if present; emit valid finite open/clamped spline data.
5. Return complete staged fragments or refusal only. Review every exception/
   allocation boundary; no success with a partially prepared second fragment.

Endpoint oracles are distinct: cut-facing endpoint matches its selected cutter
root; preserved open-source endpoint matches evaluation at the original domain
endpoint and need not lie on the cutter. If both labels apply, assert both.
For closed output both endpoints are cut-facing. Check exact-once retained interval
coverage independently from the helper's own selected intervals/labels.

Tests (`[lc-offsetcurve][trim-kernel]`): open end/middle/multiple-root selections,
closed two/many roots, both directions, existing knots and interior cubic cuts,
each cutter type, two-component source examples, geometry over every retained
parameter range, and all endpoint labels. Refusals include no/root-only,
near-tangent, overlap, nearby-distinct roots, ambiguous click, bad layout/
arithmetic, exhausted budgets/cancellation, and no partial result. Tests for
document ownership belong to T-actions, not this pure kernel.

### 11.3 T-actions: Atomic Trim/Trim Two

Edit the actual current
`librecad/src/actions/drawing/modify/lc_action_modify_trim.{h,cpp}`,
its action tests, and modification/batch interfaces only where necessary.
Use explicit EntityOffsetCurve dispatch before generic RS_AtomicEntity logic;
never cast the spline-derived container as atomic.

Preview and commit share the pure preparation routine. Trim Two prepares BOTH
replacement sets (offset/ordinary or offset/offset) before applying one
LC_DocumentModificationBatch, using current ownership checks. If either side
fails, preserve both entities, selection, metadata and undo/redo state.

Each plain fragment inherits layer/pen/visibility and supported foreign data.
Strip known owned offset marker/tail/lineage; no stale editability claim. Show an
action-level notice that trim converts the offset to ordinary splines.
Do not introduce explanatory instructional panels into the drawing UI.

Tests (`[lc-offsetcurve][trim-action]`): offset target, unchanged offset limit,
both Trim Two pick orders, mixed atomic/offset, two offsets, two-fragment open and
closed cases, generated-child owner selection, every lock/visibility/owner/stale
refusal, preview/commit equality, notice and exact one-step undo/redo of the native
source and resulting fragments.

### 11.4 T-files: Demotion Cannot Reappear

Save/reopen prepared fragments through every applicable section 9.4 row, binary
codec lanes and both conversions. Assert ordinary spline/polyline entity type,
fragment count/order, geometry/attributes/foreign data and absence of owned schema.
No silent re-adoption after a second save. Undo before save restores the native
definition correctly. No schema change is needed.

Run focused/adjacent Release/ASan, full Release and format fast lanes; build both
systems and exercise actual Trim/Trim Two GUI paths before slice closure.

## 12. Validation Recipes and Evidence

### 12.1 Build Identity

Capture checkout HEAD, dirty diff/path hashes, compiler, CMake/Ninja/qmake6/make
versions, Qt prefix, configuration flags, target paths and binary hashes.
Use fresh out-of-tree Release, ASan and qmake6 build directories for the selected
worktree, not a different branch's cached binary. After the first configured
baseline, incremental reuse is allowed for that same worktree/toolchain/config;
reconfigure for source-list changes and rebuild all affected targets. Start fresh
if identity/configuration differs or stale artifacts cannot be excluded.
Set WT, REL, ASAN, QM and
QT_PREFIX to real discovered paths; check availability first.

Required CMake targets are explicit: librecad_tests, librecad_dxf_fast_tests and
librecad_dwg_fast_tests. Building the default all target alone is not test proof.
Find runtime binary locations from the actual build/CTest configuration; do not
assume an app-bundle executable lives at the build root.

Example (substitute captured paths):

~~~bash
cmake -S "$WT" -B "$REL" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON \
  -DQt6_DIR="$QT_PREFIX/lib/cmake/Qt6"
cmake --build "$REL" --target \
  librecad_tests librecad_dxf_fast_tests librecad_dwg_fast_tests --parallel 4

cmake -S "$WT" -B "$ASAN" -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON \
  -DQt6_DIR="$QT_PREFIX/lib/cmake/Qt6" \
  -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address"
cmake --build "$ASAN" --target \
  librecad_tests librecad_dxf_fast_tests librecad_dwg_fast_tests --parallel 4
~~~

qmake6 application lane, from a fresh QM directory:

~~~bash
/opt/homebrew/bin/qmake6 -r CONFIG+=sdk_no_version_check "$WT/librecad.pro"
make -j4
~~~

Discover the actual executable/prefix elsewhere rather than assuming Homebrew on
every platform. Do not reintroduce qmake5 fixes. qmake6 proves application source/
UI registration/build; it does not prove Catch2 test registration.

Keep combined build jobs <=8. Serialize each build directory/binary's execution
and use unique report paths. If ASan leak detection is unsupported by the host,
record baseline/tool evidence before using ASAN_OPTIONS=detect_leaks=0; that does
not disable other sanitizer findings.

### 12.2 Test Execution

For each slice, capture fresh pre-change discovery and affected baseline reports.
After implementation run each planned identity once per configuration. Use
QT_QPA_PLATFORM=offscreen for headless GUI tests, actual target binary paths and
unique JUnit plus console reports:

~~~bash
QT_QPA_PLATFORM=offscreen "$TEST_BIN" "$SELECTOR" --list-tests
QT_QPA_PLATFORM=offscreen "$TEST_BIN" "$SELECTOR" \
  --order decl --reporter junit --out "$JUNIT_REPORT"
~~~

The console assertion summary is needed in addition to JUnit. The baseline uses
Catch2 v3; configure console and JUnit reporters in one execution using its
supported per-reporter output syntax, verified against the built binary's help.
Do not run a second reporter pass then claim those are distinct test identities.

Parse XML with an XML parser. Compare exact names, errors/failures/skips and
assertions, not grep counts. A missing selector or zero assertions is failure.
Pre-edit zero discovery is expected only for planned new tests; after registration
it cannot pass. No stale cross-tree baseline or unexplained new skip is accepted.

Geometry gates:

- A-core: A04/A10 plus affected existing WorkBudget/geometry tests.
- A-strict: A08/A12/A13/A14 plus all existing strict/refinement regressions.
- A-reference: A05/A06/A07/A11 plus reference/events/components regressions.
- A-cache: A01/A02/A03/A09 plus layout/bounds/local-topology regressions.
- A aggregate: all14 exact new identities and the freshly captured existing
  strict regression identity set, in Release/ASan.
- B: all8 terminal identities plus reference/local/bounds regressions.
- C: all54 exact chain identities plus A/B and existing geometry regressions.
- Strict/reference regression selector union includes reference-critical,
  reference-critical-depth, reference-critical-refine, reference-events and
  reference-components under [lc-offsetcurve][geometry]; confirm registration
  rather than trusting historical counts.

For every source slice, build both fast targets. Run affected adjacent tests and
full librecad_tests Release; run focused and format fast Release/ASan when shared
geometry/filter behavior changes. The final gate reruns full Release and all
feature/adjacent/fast lanes in both configurations. Declare any unsupported ASan
full-suite or platform lane explicitly rather than implying it ran.

Use librecad/src/test/lc_filtertestsupport.h for actual application format tests,
fonts, quiet logging and consistent fixtures. Fast lanes use QCoreApplication;
keep real property/action/window tests in librecad_tests. Do not reuse numeric
format-range classification from spline_offset_representation_tests.cpp:
FormatDWG2007 must be explicitly recognized.

### 12.3 Independent Correctness Checks

- Analytic/high-precision reference construction verifies geometry, roots,
  visibility, direction, endpoint roles and phase return.
- Metamorphic scale/translation/rotation/reversal checks supplement, not replace,
  independently expected outcomes.
- Inspect actual written raw SPLINE/XDATA/EED/APPID data with a reader/decoder
  independent of the new schema helper; test application adoption separately.
- Use deterministic stress/cap tests (including64 spans), exact/one-less/overflow
  and repeated seeds. Record counters and observed runtime/memory envelope;
  do not promise a global time/RSS bound from one benchmark.
- Review ownership, iterator invalidation, signed/unsigned arithmetic, overflow,
  callback exceptions, mixed-state swaps and transaction finalization.
- Run existing static analysis/sanitizers supported by the project on changed
  targets and report tool/coverage limits. Missing external tools are not PASS.

Evidence per slice is finite: baseline.json, test identity map, command/build logs,
JUnit/console outputs, fixture hashes, review findings/disposition and commit/
blob verification. Use one source identity and attempt ID; do not create another
gate merely to hash a preceding gate.

## 13. Auxiliary Harness: Finite Repair Track

This track is unfinished engineering work, not an authority to block product
implementation. Keep its existing scratch inputs untouched until copied/reviewed:
`/private/tmp/lc2961-v512-readiness-20261022-01-harness`.
Its local synthetic runs (9 H1,17 H2,10 H3/H4) are historical progress, not current
formal receipts or product validation.

Put reproducible harness source/tests under a clearly scoped version-controlled
tools directory `scripts/offset-curve-validation/`; reconcile existing script
conventions before extending it.
Do not make release builds depend on Node or this harness.
Each X-H row gets ordinary focused tests, review, one implementation commit and
one plan update, not another candidate/freeze lifecycle.

| Slice | Current state | Required repair and exit |
|---|---|---|
| X-H1 | Committed, 29ab28163 | Schema 2 / offset-2961; explicit validation/synthetic purpose; bounded parsing and readback; inert descriptors; 24 passing tests and independent review |
| X-H2 | Partial preflight capture/tests | Capture actual source/runtime/input tuple and exact inventory; no synthetic placeholder source rows; stable read and race/refusal tests |
| X-H3 | Runner lacks production worker wiring | Configure real registry/CLI, explicit worker contract and exact output inventory; missing/undefined/unknown status is FAIL, never default PASS |
| X-H4 | Runner exposes row count, not complete joined rows | Return all joined events with pinned identity/order/output hashes; test persistence, replay, mutation and duplicate/missing events |
| X-H5 | Draft evidence validator incomplete, no tests | Implement missing helpers, actual tuple validation, absent-file publication and before/after-dispatch rechecks |
| X-H6 | Planned inventory stale/incomplete | Preserve original71 scenario IDs; add stale-current-predecessor, strict worker-result and newly found regression cases; derive count from explicit unique inventory |
| X-H7 | Generator/checker/reports absent | Generate exact source/runtime/case manifests; independent validation and four canonical suites run twice with reproducible semantic results |

### Concrete H2 Capture Corrections

Retain the same bigint parent identity from before child spawn through child
completion and receipt publication, including child-failure paths. Comparing a
new parent sample with itself is not evidence of stability. Use no-follow I/O,
exclusive file creation, exact membership/mode/link-count checks and readback.
Flush files and containing directories where supported; record platform limits.
Capture actual stdout/stderr/exit, sanitized child environment and unchanged
parent environment. Durable receipts require real pinned source/input/runtime
bytes; synthetic controls stay explicitly synthetic.

### Concrete H3/H4 Corrections

The current CLI collapses many bad arguments into ARGUMENTS_INVALID. Implement
section 13.1's distinct bounded argument classifications in X-H3 and test them
through the real CLI; do not claim the existing parser already supplies them.

run-formal-gates currently accepts missing/unrecognized worker result as PASS.
Require an explicit validated result object, recognized terminal status, expected
event order and exact output inventory. Bind actual output file hashes; reject
unexpected/missing/duplicate files/events and a worker claiming PASS after failure.

The independent checker derives its expected event sequence from the pinned
registry and actual outputs. Runner-returned ledger rows are data to verify,
never an independent oracle. Test process error, thrown callback, partial output,
malformed JSON and status omission as separate cases.

### Concrete H5 Corrections

The draft references undefined readNoFollow, assertRealDirectory,
objectStatIdentity, inodeIdentity, validateReceiptRootParents and buildGenesis.
Implement them with tests or replace them with existing proven helpers before
claiming executable coverage.

Do not validate a fabricated manifest with zero-byte/dummy source records.
Validate actual pinned inputs and exact registry output cardinality.

An expected absent manifest is not an unconditional lstat error: distinguish
ENOENT from other failures and create with exclusive semantics. Refuse existing
files, symlinks/hardlinks, wrong parents, unexpected mode/link count and mutation.

Validate actual output bytes/stat/mode/link count/path and manifest identity both
before and AFTER dispatch. A correct pre-dispatch read is not a post-dispatch
race guarantee. Recheck relevant parent identity; capture and test failure paths,
including permission errors, disappearing/replaced files and output substitution.
Do not claim filesystem race elimination beyond the implemented open/fstat/
publication model.

### Independence, Inventory and Completion

A checker may share inert schema descriptions/constants, but it implements its own
parsing, expected-result derivation and validation; it must not call the production
validator it claims to independently check.

Preserve the old71 scenario identities and add coverage rather than deleting one
to keep a magic count. Explicitly include stale V512/current predecessor cases
and the new false-PASS/undefined-helper/inventory/post-dispatch defects. Old
V509/V510/V511 cases remain historical-input discriminators, not active routing.

X-H7 runs the named suites `receipt-schema`, `capture-readback`,
`formal-runner-boundary`, and `genesis-ledger-p0f-joins`. It records their
definitions, exact case inventories, source/runtime/
input identities, independent results and two complete runs. Compare semantic
outcomes/deterministic payloads while excluding declared run IDs/timestamps from
byte equality. No self-referential future commit hash, fake reviewer PASS,
fabricated gate artifacts or automatic authority escalation.

**Next auxiliary card: X-H2.** Own preflight capture/source-tuple helpers and
focused tests under the tools directory. Reconcile the historical capture code
with schema 2; derive the candidate/blob, source manifest, exact case inventory,
runtime/executable and runner identities from actual stable bytes. Bind the
sanitized child environment without mutating the parent. Recheck inputs and the
original bigint directory identities before spawn, after every child outcome and
before/after exclusive durable publication. Preserve failed child diagnostics;
never publish a synthetic source tuple as validation evidence. Add real-child
positive controls plus mutation, replaced-parent, output-cap and failure tests.
Keep X-H3 worker registry/CLI and X-H5 formal evidence publication out of this
slice. Review/test/commit it, then immediately update this roadmap.


### 13.1 Preserved H6 Refusal Inventory

Ranges expand to separately executed identities in the order shown. The original
71 scenarios comprise11 CLI,18 identity,33 receipt-tree,5 race and4 join cases.
This is a coverage floor, not a target to preserve by dropping newly found bugs.
Historical version/hash values are synthetic negative metadata, not routes or
instructions to reopen old evidence.

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
| H6-TREE-05a..05f | Report bytes/hash/size; stdout/stderr sidecar; basename/ID | RECEIPT_BINDING / FILE_BINDING_MISMATCH |
| H6-TREE-06a..06b | Wrong mode; nonregular file | FILESYSTEM / FILE_METADATA_MISMATCH |
| H6-TREE-07a..07b | Missing/extra tree entry | FILESYSTEM / RECEIPT_TREE_MEMBERSHIP |
| H6-TREE-08a..08c | Symlink; hardlink; alias | FILESYSTEM / PATH_NOT_REGULAR_SINGLE_LINK |
| H6-TREE-09a..09c | Overlap; ancestor/descendant; protected-path overlap | PATH_POLICY / PATH_POLICY_VIOLATION |
| H6-RACE-01 | Parent changes before exclusive creation | ROOT_PREFLIGHT / PARENT_IDENTITY_CHANGED |
| H6-RACE-02a..02b | Report/sidecar changes between validations | REVALIDATION / RECEIPT_CHANGED |
| H6-RACE-03 | Readback differs | REVALIDATION / READBACK_MISMATCH |
| H6-RACE-04 | Output root appears after final check | ROOT_CAS / ROOT_ALREADY_EXISTS |
| H6-JOIN-01 | Same tree in both ordinals | RECEIPT_JOIN / RECEIPT_TREE_DUPLICATE |
| H6-JOIN-02 | Duplicate ID through exported transaction | RECEIPT_JOIN / RECEIPT_ID_DUPLICATE |
| H6-JOIN-03 | Pair order mismatch through exported transaction | RECEIPT_JOIN / RECEIPT_ORDER_MISMATCH |
| H6-JOIN-04 | Matching digest strings but wrong referenced bytes | RECEIPT_JOIN / REFERENCED_BYTES_MISMATCH |

**Correction to inherited expectations:** CLI09/10 and JOIN02/03 must not demand
contradictory early/late classifications of the same condition. Assign receipt
content ID/order validation once to the shared transaction; exercise both CLI and
direct transaction ingress as distinct tests with the common join outcome above.
Likewise JOIN04 is a direct persisted-join test on a previously valid prefix;
ordinary initial bad sidecar input belongs to TREE05. Require each case to reach
its intended production boundary, not merely any refusal. Pin validation order
in X-H3 before X-H6 registration.

New required identities, in addition to the71:

- H6-ID-02g/03d: synthetic stale V512 revision/candidate.
- H6-ID-02h/03e: synthetic stale pre-consolidation V513 revision/candidate.
- H6-WORKER-01..04: missing result, missing status, unknown status, explicit FAIL.
- H6-WORKER-05..07: missing/extra/duplicate output inventory.
- H6-EVIDENCE-01..04: missing helper/export, dummy source tuple, absent manifest,
  output changed after dispatch. The absent-manifest valid case succeeds;
  nonexistent helper/dummy tuple/post-dispatch substitution must fail.
- Add positive controls, real-CLI argument forwarding and post-root genesis/
  ledger/manifest mutation cases to explicit inventories rather than calling
  component test totals H6 completion.

Pre-root refusals require zero worker launches, no owned root/output, and identical
full sorted process.env and input-tree bytes/metadata. The external-root race is
the explicit exception: preserve the independently created root/sentinel's
identity, mode, link count, bytes/hash and exact entries; never clean it up.
Post-root tests preserve their valid prefix and prove no later output/dispatch,
not that genesis never existed. Use the real CLI/shared production transaction
with narrow filesystem/worker dependency injection, not copied parsers, global
monkey patches or mocked success validators.

Run synthetic qualification twice, then immutable-input replay twice with isolated
derived mutation trees if exercising real receipts. Original input trees remain
byte-identical before/after every run. Preserve failed artifacts; a retry uses a
new attempt namespace, not a new plan version.

## 14. Completion Audit and Current Blockers

### Definition of Complete

All main-track slices and auxiliary deliverables are committed with
their immediate plan checkpoints. R01-R15 each have actual source/test/evidence
links, not just a checked status. No temporary mutation/UI stub, silent skip or
unimplemented decoder is counted as native feature support.

Required final evidence:

- All 14 A, 8 B and 54 C identities and lower-level regressions in Release/ASan.
- Native adoption/edit/transform/owner/snaps/tools/UI/source/trim tests and actual
  application interaction, including refusal/cancel/no-op and undo/redo.
- All 14 application format rows with separate reader/writer expectations,
  both degrees, conversions, binary lanes, legacy input and R12 downgrade.
- Raw-carrier/tail/APPID/save-atomicity/trim-demotion tests and fixture provenance.
- CMake application/tests, qmake6 application, available platform CI/static/
  sanitizer runs and explicit unavailable-tool/residual-risk accounting.
- Reviewed clean commit sequence and source/test blob correspondence. Publication,
  push or merge only when separately requested; commits are not a merge claim.

A green subset is progress, not completion. If an external interoperability row
cannot run, keep that precise row open and report the dependency. Do not restart
the whole roadmap or disguise the gap with a synthetic fixture.

### Current Blocker Dispositions

| Item | State | Next action |
|---|---|---|
| Recursive plan/activation routing | Removed as a planning deadlock | Execute A-core's finite card |
| Atomic budget API | Committed and validated (`0eb2a0cfa`) | Integrate solver/geometry work in A-strict/A-reference |
| Strict work/ordering/exception proof | A-strict implementation and review in progress; no commit or closure claim | Finish A-strict validation, then A-reference integration |
| Complete semantic path checker | Missing implementation | A-cache -> B -> C |
| Native feature/UI/persistence/trim | Not implemented | Follow D through T; do not label scaffolding complete |
| Independent DWG/version fixtures | Required, not yet qualified | Inventory/tool/license/provenance during G-fixtures; close all rows in G-matrix |
| Harness production readiness | X-H1 committed; actual capture, workers and independent evidence incomplete | X-H2 through X-H7, independent from starting product work |
| Independent review and test evidence | A-core independent static review and actual test reports complete | Obtain fresh review/validation for each successor |
| Dirty unrelated main checkout | Not the selected source baseline | Preserve it; reconcile only if changing implementation worktree |

### Commit Ledger

Record completed slices and actual product commits, not historical plan retries.
The plan checkpoint column never depends on its own future SHA.

| Slice | Product commit | Plan checkpoint | Evidence and result |
|---|---|---|---|
| Existing foundations | 9c06e9c77b313b62cc20e9ec39c22952fee501f2 | 9309885cef66aef0cca0f3c8862e0c210518da3b | Historical source baseline; no new test PASS asserted |
| A-core | 0eb2a0cfa31c29ba7740e4fb698c8417f4d43fa5 | e31d5a024b473b26148c66bd2726c290bf7976ef | Independent static review: no findings; Release/ASan and qmake6 passed as detailed below |
| X-H1 | 29ab281635f85590a7db1f7a02db1fcf65ce3cfb | This immediate plan-only checkpoint | 24/24 Node tests; syntax/whitespace checks; independent review fixes applied |

### X-H1 Validation (2026-10-22)

Committed scope: four files in scripts/offset-curve-validation. Node v26.12.0:
24 tests passed, zero failures/skips; three JavaScript syntax checks and staged
whitespace validation passed. Optional POSIX tooling, not a build dependency.
Independent reviewer 01a16673-d27c-7a73-a34b-4c8519852336 found final readback
identity gaps, protected-root aliases (including dangling symlinks), and a missing
PASS inventory digest equality check. All were fixed and regression-tested; the
bounded final recheck found no remaining issue. No product runtime/build change.

Receipt files and owned directories retain full identities through the final
receipt/set boundary. Ancestors use device/inode/type/mode so unrelated sibling
activity does not cause a false refusal. Enumeration stops at the fourth entry;
JSON nesting is capped at 64. Explicit synthetic fixtures cannot enter the default
validation-purpose path. No actual source/runtime capture, formal receipts,
hostile-filesystem race elimination or X-H2-X-H7 completion is claimed.

Evidence: /private/tmp/lc2961-impl-evidence.9GtLO2/X-H1-node-tests.tap.
The historical scratch tree remains unchanged. The next main card remains
A-strict, with A-reference following its tested commit; the auxiliary next card
is X-H2 above.

### A-core Validation (2026-10-22)

Committed scope: exactly the geometry header, implementation and test TU from
the original A-core card. No solver integration, entity or file/UI code changed.
The validated staged bytes equal the committed blobs.

- Worker: Gibbs implementation role (agent `01a1663b-5c1d-7333-98a2-ad49b643a3f4`).
- Independent static reviewer: `01a16640-a9d5-7171-ae1f-16b92d7d23a2`;
  no actionable defects or A04/A10 coverage gaps. Runtime claims come from the
  actual parent-run reports, not the reviewer's static assessment.
- Qt 6.12.0; AppleClang 21.0.0.21000334; Catch2 3.16.1.
- Release and ASan A04/A10: exactly two registered names, 3,942 assertions passed.
- Release and ASan geometry: 233 cases, 1,546,769 assertions passed.
- Full Release: 3,855 cases (3,829 passed, 26 pre-existing skips),
  7,402,261 assertions passed; no new skip or missing pre-existing JUnit name.
- DXF fast Release/ASan: 744 cases (741 passed, same three baseline skips),
  39,929 assertions passed; exact baseline name-set equality.
- DWG fast Release/ASan: 1,006 cases (1,002 passed, same four baseline skips),
  34,948 assertions passed; exact baseline name-set equality.
- Fresh CMake Release/ASan explicitly built all three named targets; qmake6
  recursive generation and full `make -j2` completed successfully. The initial
  build jobs were bounded to eight across the three builds.
- No AddressSanitizer diagnostic in the completed scoped runs. Full application
  ASan was not run for A-core; full Release and all geometry/format lanes were.
- Whitespace validation passed. Unrelated metadata-initializer warnings and the
  host FreeType deployment-target warning remain outside this slice.

Before-change comparison used the existing product-base build
`/private/tmp/lc2961-v209-A-core-a03-baseline-release`. Its source tree differs
from product base 9c06e9c77 only in this roadmap; Ninja confirmed the application
test target up to date. Baseline full Release passed 3,853 cases with the same
26 skips and 7,397,511 assertions. Fresh baseline fast binaries were also built/
run and their actual JUnit names/skips compared to both new configurations.

Evidence: `/private/tmp/lc2961-impl-evidence.9GtLO2` (JUnit, console, build logs and summaries).
Builds: `/private/tmp/lc2961-impl-release.M4hycU`, `/private/tmp/lc2961-impl-asan.Npox8c`,
`/private/tmp/lc2961-impl-qmake6.BUYu0S`. These are local artifacts, not permanent
CI/report storage or evidence of unavailable external-reader tests.

Validated source SHA-256:

| File in entity directory | SHA-256 |
|---|---|
| lc_ellipseoffsetgeometry.h | ea4b5998a5cf1207591176671c7dcc8312b22ca16f73f16e3c598e683d3f23c5 |
| lc_ellipseoffsetgeometry.cpp | f9019113ec5abaeb0ba692976088ae592e5dca6c16f00a99a81e3c6c496a3f6e |
| tests/lc_offsetcurve_geometry_tests.cpp | e8ebfbe6709eaebcd34c723bf39e216fa68f3b4972d057e0ca3375481cac4e73 |

After every slice update the status table, affected requirements, risks and next
card together. Use actual commit IDs in the next plan-only checkpoint; never
write the future checkpoint's own hash. Keep one active roadmap.
