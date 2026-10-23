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

Updated: 2026-10-23. Status: all A slices, B, C and X-H1 through X-H6 committed; native feature incomplete.
This is the single active roadmap for this work. It replaces the routing rules
and duplicated status mirrors in the earlier draft, not its product requirements.
This checkpoint records tested A-core/A-strict/A-reference/A-cache/B/C and auxiliary X-H1 through X-H6 commits. Later
main-track slices remain incomplete; no native entity is claimed.

## 1. Verified Baseline and Scope

| Item | Verified state |
|---|---|
| Working tree used for this update | `/private/tmp/lc2961-v513-plan` |
| Initial audited checkpoint | `9309885cef66aef0cca0f3c8862e0c210518da3b` |
| Initial product base | `9c06e9c77b313b62cc20e9ec39c22952fee501f2` |
| Difference between initial checkpoints | Only this plan file; no product-source change |
| Roadmap consolidation commit | `87ebe5428` |
| Current feature branch | `codex/offset-2961-implementation` |
| Latest product checkpoint | `d4a0b081a` (C) |
| Latest auxiliary checkpoint | `a95a5c5ff` (H7 checker/preflight; guards ceacd01a3, workers e83aaf3f0) |
| Geometry implementation | `lc_ellipseoffsetgeometry.{h,cpp}`, `lc_parametriccurveintersection.{h,cpp}` |
| Existing geometry stages | Normalization, reference frames/jets, critical/event/component analysis, cache layout, bounds, local topology |
| A-core | Atomic work adoption, separate ordering allowance and sticky terminal status implemented; A04/A10 registered |
| A-strict | Bounded validation, capacity, ordering, callbacks and exceptional-exit snapshots; per-call retained ordering allowance |
| A-reference | Bounded geometry ordering/scans/capacity and atomic ordinary/exceptional solver-work adoption |
| A-cache | Atomic Candidate/Proof subdivision, inherited combined depth, prepared capacity, bounded dispatch/coverage |
| B terminals | Immutable exact occurrence/edge-side classification, separate source incidence, bounded storage/scans |
| C topology | Complete bounded unique-path checker, owned contact query and exact root-to-port matching; unresolved correspondence refuses Ambiguous |
| Native entity | No `LC_OffsetCurve` or `EntityOffsetCurve` yet |
| Product integration | No native offset adoption, private file schema, creation/editing UI, or target-trim integration yet |
| Build registration | Geometry, contact and routing sources in CMake/qmake; focused geometry/contact/routing/budget tests in `librecad_tests` |
| Validation harness | X-H1 through X-H6 and CLI orchestration committed; H7 pending |
| Latest validation | A-core, geometry and format fast lanes passed Release/ASan; full Release and qmake6 passed; see section 14 |

Recheck these facts against the selected implementation checkout before starting
D-core. Inventory Qt6/Catch2/build tools and available independently authored
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
| A-strict | A-core | Strict-solver bounded ordering/validation/capacity and exit snapshots (`dc016f6e2`) | committed |
| A-reference | A-strict | Geometry scans/callback ownership and exception-safe work transfers (`5c12fbeef`) | committed |
| A-cache | A-reference | Candidate/proof split accounting and inherited combined depth (`dbf895638`) | committed |
| B | A-cache | Immutable occurrence-specific producer terminals (`9bf0546b6`) | committed |
| C | B | Full unique-path topology checker and public result | committed d4a0b081a |
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

### Current Next Cards

The main track resumes at D-core (section 8.1). C is committed and its card below
is a completed implementation contract, not a request to redo it. Preserve actual
C adoption, immutable ownership and runtime consistency checks. In particular,
hashing rounded transformed coordinates does not transport a geometric proof:
D-core needs justified rounding/error and topology preservation, or an explicit
unchanged refusal when a requested transform cannot be certified. Do not weaken
the ordinary-transform no-regeneration contract or silently restamp stale data.

The active auxiliary sequence is H7 qualification driver -> complete affected regression
rerun -> two complete qualification runs. Suite inventory/worker execution is
committed separately; it is not whole H7 or native feature completion.

### C Implementation Card (Completed)

**Files:**

- `librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.h`
- `librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.cpp`
- `librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_geometry_tests.cpp`
- New `lc_offsetcachecontacts.{h,cpp}` and tests/lc_offsetcachecontacts_tests.cpp
  in the same entity directory; CMakeLists.txt and librecad/src/src.pro for exact
  source/test registration. This bounded helper reuses strict intersection proofs
  to detect nonlocal candidate contacts independently of matching proposals.
- `lc_parametriccurveintersection.{h,cpp}` and its test TU: opt-in bounded strict
  proof-domain padding. The contact query's transverse dyadic-boundary crossing
  hit depth 20 with the historical 0.001 padding (debugged box recorded below).
  Keep the old default and counters for all old callers; contact queries opt into
  1/16 padding. This changes the domain of a fresh Krawczyk proof, not geometric
  tolerance, acceptance predicates or shared work limits.
- New `lc_offsetpathsearch.{h,cpp}` and its routing test TU: extract canonical
  bounded path enumeration for direct nonunique-route testing. It cannot mint a
  geometry certificate or bypass public checkCache validation.
- New `lc_offsetcontactmatching.{h,cpp}` and its test TU: exact rational candidate
  positions anchored inside genuine owned contact proofs and specific route ports.
  Unknown/non-exact port alignment remains Ambiguous, not tolerance-based identity.
  Owner validity is separate from an empty root count, including moves, provider
  invalidation and foreign-handle refusal. Prepare owner storage before work debit.

**Base:** committed B `9bf0546b6`. Requirements R02/R12/R14; all A/B exact
identities exist. ReferenceComponentAnalysis now owns exact terminal lookups.
CacheLocalTopologyAnalysis retains certified oriented alternatives but proves
neither full reference coverage nor a unique complete producer path. No caller
may treat its Ok result as an editable-cache certificate.

**Steps:**

1. Add the public result/reason contract of section 6.6 with deterministic original
   span attribution, final CheckWork, and an owned immutable successful path proof.
   Add shared-budget and limits overloads; neither owns a caller budget. Validate
   finite normalized definitions, layout, TooSmall and exact closure flags first.
2. Preserve existing standalone bounds/local APIs and tests. Qualify actual general
   arc, whole-cycle and G2/G4 engine outputs before selecting a global algorithm:
   the existing exhaustive Cartesian proximity relation can exhaust at a tolerance
   boundary even when same-edge witnesses already cover a candidate. A complete
   checker needs bounded on-demand/refined certificates, not unconditionally
   trusting the coarse relation or converting every unresolved pair to success.
3. Build source/paired/seam transitions from exact edge-side incidence. A fringe
   certifies only a node/endpoint, never positive-length reference coverage. Every
   chosen transition records both incident occurrences and its provenance.
4. Tile normalized candidate domains and every selected reference edge exactly
   once. Refine actual paired subcurves with the shared Candidate/Proof admissions;
   retain all distinct semantic alternatives. Never infer identity/order from
   rounded coordinates or reuse a coarse pair proof after subdivision.
5. For closed paths, qualify start cuts including knot and interior dyadic phases;
   unroll one lap, return to the same paired frontier, and canonicalize the complete
   directed event/edge signature modulo rotation. Equivalent same-edge subdivisions
   are one path, different directed cycles are not. Memoize equivalent partial
   states and saturate semantic-path counts at two, with bounded scans/storage.
   Include a bounded nonlocal candidate-contact check. Local projection positivity
   alone cannot exclude a loop across matched cells, even adjacent ones. Never
   exempt an entire adjacent span pair merely because they share a construction
   endpoint: prove separation or the exact authorized contact and absence of others.
   Check missing required reference contacts as well as unexpected candidate ones.
6. Classify actual evidence: proven separation OffCurve; unresolved geometry/cut
   correspondence Ambiguous; incomplete/invalid/nonunique traversal Topology;
   unresolved reference ReferenceUnresolved; exhausted budget LimitExceeded.
   Existing stationary-interior reference probes remain unresolved; never promote
   them to success merely to test a terminal. See B validation below.
7. Register all 54 exact C names in section 7.3 with independent fixture geometry,
   role/side/signature, endpoint/cut-return, original span and work expectations.
   Phase controls cross both source directions and cacheReversed. Check the actual
   generated cubic and G4 separate producer outputs, not only hand-made tiny arcs.
8. Test exact/one-short/pre-spent caps in every dimension, sticky retries, missing
   and repeated tails, retrace, wrong occurrence, hidden cusp/seam shortcuts and
   all nine reasons. No success or partial path may escape an unfinished proof.
9. Independent full-diff review and the same geometry Release/ASan, full Release,
   format fast lanes and qmake6 checks used by B. Compare exact names/skips.
10. Commit C only after complete path/coverage/cut obligations pass; immediately
    update this roadmap separately, then start D-core. A partial path checker
    cannot authorize native adoption or be labelled C complete.

**Exit:** all 53 public identities plus C27 routing identity, and prior A/B/geometry regressions pass, with
complete unique selected paths, bidirectional coverage and precise reasons/work.
No native entity, UI or file-format edit belongs in this slice.
Changes outside this concern require a concrete dependency note.

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
- Retained contraction intersects each call's remaining ordering allowance with
  the original cumulative ceiling. The appended field defaults to SIZE_MAX to
  preserve old three-field aggregates; geometry callers supply the actual shared
  remainder. A refused comparison does not refund its already staged event.

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

Additionally, certify nonlocal contact incidence. Positive projected progress in
each paired cell does not imply a globally embedded cache: two monotone adjacent
pieces may make a small loop across their interiors. Strict candidate intersections
or equivalent outward pair proofs must exclude unexpected contacts and establish
required nonlocal reference contacts with their exact directed occurrences. A
shared endpoint is not a blanket adjacent-pair exclusion. Preserve original span
attribution and atomically adopt strict-query work, including exceptional exits.

Concrete regression: C1(t)=(3t-2t^2,2t^2-t) and
C2(s)=(1-s+2s^2,1-3s+2s^2), each on [0,1], join at (1,1) and meet again at
C1(1/2)=C2(1/2)=(1,0). Their respective chord-axis derivative projections are
constantly 2; proximity can be made arbitrarily small by scaling. Thus local
order plus tiled parameter domains cannot alone admit this candidate. Test the
actual cubic representations and retain the unexpected contact as a falsifier.

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
not a current test receipt. All fourteen A identities are implemented and
validated. All eight B identities are also registered and validated. C identities
are planned, not already registered.

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

### 7.3 C: Fifty-Four Checker Obligations

Public selector: `[lc-offsetcurve][geometry][cache-topology-chain]` (53 identities).
C27 uses `[cache-topology-routing]` and explicitly tests the extracted routing
kernel, not a synthetic public ellipse certificate. Keep its identity and test
obligation; do not count it as public geometry coverage.
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

C27 readiness correction (2026-10-23): independent review could not justify a
public nonunique-cycle fixture. G1 has exactly one visible directed cycle: its
crossings have sin(alpha)^2=11/75 and images (+/-4*sqrt(3),0); only speed s>=6
is visible. The apparent alternate routes include hidden intervals (at (7,0),
an admitted source foot with cos(phi)=14/15 has distance squared 26/3<9).
G4 has one closed cycle and one open path, not two closed alternatives. Do not
mislabel a repeated lap or hidden-interval rejection as nonunique public coverage.
Test two-path saturation with independently specified distinct routing cycles,
then exercise that SAME kernel from the public builder. A later genuine public
multi-cycle fixture is additional coverage, not fabricated evidence for this one.

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
| X-H1 | Committed, 29ab28163; follow-up 8993e2848 | Schema 2 / offset-2961; explicit validation/synthetic purpose; bounded parsing and readback; inert descriptors; 25 passing tests and independent review |
| X-H2 | Committed, 25661f1a3; hardened c939c3de7 | Actual input/runtime manifest, bounded stdin/child capture and durable receipt readback; 15 input + 32 process tests plus 25 H1 tests; independent review |
| X-H3 | Committed, 7da2bb144 | Real pinned registry/CLI, explicit worker contract and exact output inventory; missing/undefined/unknown/FAIL are refusals, never default PASS; 30 runner tests and independent review |
| X-H4 | Committed 3e06e3894; explicit API | Complete genesis/chained case rows, pinned identities/order/actual outputs, exclusive publication/replay/mutation checks; not automatic CLI persistence |
| X-H5 | Committed 6d4fd25b3; explicit API | Actual source/runtime/receipt/diagnostic/ledger/output evidence, exclusive absent publication and bound automated checker; 44 focused tests and independent review |
| X-H6 | Committed 507e44ad7; core 9170b0cc9 | All 93 obligations executable, all original71 IDs retained; owned H4/H5 protocols, precise root scopes and mutation/prefix checks; 94 inventory tests pass |
| X-H7 | Checker/preflight a95a5c5ff, guards ceacd01a3, workers e83aaf3f0 committed; qualification pending | Driver, complete affected regressions and two complete qualification runs remain |

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

**Completed X-H3 card.** New formal-registry.js, formal-runner.js,
run-formal-gates.js, test-formal-runner.js and fixtures/formal-child.js under
scripts/offset-curve-validation. Existing H1/H2 modules remain frozen unless a
specific integration defect is demonstrated. The historical runner is reference
material only; its V512 tuple and default-PASS behavior are not current APIs.

1. Define a bounded versioned registry with explicit purpose, pinned real worker
   module/source, unique ordered case IDs and exact expected output declarations.
   Bind actual bytes through the H2 input manifest; never invent source rows.
2. Require explicit recognized worker result status and exact case/output
   inventory. Missing/undefined/malformed/unknown/FAIL, child error/nonzero/signal,
   thrown callback and partial/missing/extra/duplicate output are distinct refusals.
   Read/hash actual output files, not caller-supplied digest strings.
3. Implement the shared transaction with original parent/input identities, root
   absence/exclusive creation, bounded real child lifecycle/diagnostics, and
   before/after dispatch rechecks. Preserve failure prefixes and externally created
   roots; no cleanup of a race winner. Do not mutate parent process.env.
4. Wire the real CLI to that transaction and actual workers. Distinguish bounded
   argument classifications in section 13.1; validate expected receipt pair IDs and
   order in the shared transaction once. Pre-root failures launch no worker and
   produce no owned root; test exact boundary/reason rather than any error.
5. Add positive, negative, actual-child and actual-CLI controls. H4 still owns
   complete persisted event joins; H5 owns durable evidence manifests; H6/H7 own
   their full inventories/independent qualification. Do not claim those from H3.
6. Independent review, focused plus H1/H2 regression tests, source/test commit,
   immediate plan-only checkpoint. Node remains optional developer tooling.


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

**Correction to inherited expectations:** CLI09/10 and JOIN02/03 must not demand
contradictory early/late classifications of the same condition. Assign receipt
content ID/order validation once to the shared transaction; exercise both CLI and
direct transaction ingress as distinct tests with the common join outcome above.
Likewise JOIN04 is a direct persisted-join test on a previously valid prefix;
ordinary initial bad sidecar input belongs to TREE05. Require each case to reach
its intended production boundary, not merely any refusal. Pin validation order
in X-H3 before X-H6 registration.

H4/H5 integration correction (2026-10-23): TREE05a-c exercise the persisted H5
manifest's actual report binding against a genuine original context, not a
nonexistent caller-supplied H3 expected tuple. JOIN04 exercises H4's independently
captured actual outputs. These boundaries throw typed errors and return no
status; record that protocol explicitly rather than fabricating REFUSED/FAIL.
The original71 identities remain unchanged. Explicit evidence/ledger root scopes
are separate from H3's output root. H7 pins an immutable copy of this expectation
table as an actual H2 input via LC_OFFSET_ROADMAP_SNAPSHOT; local tests default to
the live plan. Discovery-only skips never count as executed qualification cases.

JOIN01 readiness correction (2026-10-23): the exported transaction shares the
request guard with CLI08, so the exact repeated path has the same early diagnosis.
Do not demand a contradictory late join result. Stable case-fold aliases fail
earlier exact directory/filename binding, and exact three-entry membership fixes
a tree to one receipt ID; ID uniqueness also precedes physical tree uniqueness.
Keep JOIN01 as distinct API-ingress coverage, not a claim that this fixture reaches
the otherwise defensive physical-tree guard.

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
| Geometry work/exception/split/depth/terminal proof | All A slices and B committed and validated | C complete checker |
| Complete semantic path checker | C committed d4a0b081a; explicit Ambiguous refusals retained | D-core native validated adoption |
| Native feature/UI/persistence/trim | Not implemented | Follow D through T; do not label scaffolding complete |
| Independent DWG/version fixtures | Required, not yet qualified | Inventory/tool/license/provenance during G-fixtures; close all rows in G-matrix |
| Harness production readiness | X-H1/X-H2 committed; workers and independent evidence incomplete | X-H3 through X-H7, independent from starting product work |
| Independent review and test evidence | A-core independent static review and actual test reports complete | Obtain fresh review/validation for each successor |
| Dirty unrelated main checkout | Not the selected source baseline | Preserve it; reconcile only if changing implementation worktree |

### Commit Ledger

Record completed slices and actual product commits, not historical plan retries.
The plan checkpoint column never depends on its own future SHA.

| Slice | Product commit | Plan checkpoint | Evidence and result |
|---|---|---|---|
| Existing foundations | 9c06e9c77b313b62cc20e9ec39c22952fee501f2 | 9309885cef66aef0cca0f3c8862e0c210518da3b | Historical source baseline; no new test PASS asserted |
| A-core | 0eb2a0cfa31c29ba7740e4fb698c8417f4d43fa5 | e31d5a024b473b26148c66bd2726c290bf7976ef | Independent static review: no findings; Release/ASan and qmake6 passed as detailed below |
| X-H1 | 29ab281635f85590a7db1f7a02db1fcf65ce3cfb | e4b6e900f | 24/24 Node tests; syntax/whitespace checks; independent review fixes applied |
| A-strict | dc016f6e2b8294cd1f1524f22a568f9d4196459c | eff902b97 | Full Release, geometry/acceptance Release+ASan, format fast lanes and qmake6 passed; independent review |
| A-reference | 5c12fbeef5fe3d41444d44bf2cc087a77a4fcf75 | 02a8e18f3 | 4 acceptance cases, full Release, ASan geometry, format lanes and qmake6 passed; independent review |
| A-cache | dbf895638f4bbd6ee210d09e41066a93cb888024 | af68194ef | 4 acceptance cases, full Release, ASan geometry, format lanes, qmake6; independent review including stage-isolated depth proof |
| X-H1 physical parents | 8993e2848 | e8119501b | 25 receipt tests; independent review; 67-test H1/H2 integration passed |
| B | 9bf0546b6 | b12a9a02c | All 8 terminal identities; full Release, geometry Release/ASan, format lanes, qmake6; independent review |
| X-H2 | 25661f1a3 | Immediate plan-only checkpoint | 15 input + 27 process + 25 H1 tests; five syntax checks; independent input/process reviews |

### X-H5 Evidence Validation (2026-10-23)

Implementation `6d4fd25b3` adds inert evidence descriptors, actual context capture
from original H4 ownership and fresh actual joins, exclusive manifest/diagnostics/
transport publication, pinned automated checker process/callback boundaries and
original input/output/receipt/ledger/manifest readback before and after dispatch,
including failure and late substitution. Callback injection and the shipped
checker fixture are synthetic-only. Partial artifacts are retained; actual child
diagnostics and named failures never default to PASS.

Parent seven-suite report X-H5-parent-integrated-01.tap in
`/private/tmp/lc2961-impl-evidence.9GtLO2/`: 254/254 passed, zero failures/skips.
Focused author report `/private/tmp/lc2961-h5-evidence-02.tap`: 44/44 passed.
Independent reviewer 01a168a8-2b07-77a0-ab4e-4bd44a98895f reviewed the four frozen
files, separately reran 44/44, verified hashes before/after and found no P1/P2.
Four syntax checks and staged whitespace passed. No independent full-regression
rerun, H7 qualification, human approval or product validation is claimed.

Validated SHA-256 under scripts/offset-curve-validation:
p0f-evidence.js f9c3df5e1f90c40047f34b91c2a1aba1775fa1b88e6b5782c6e3f9fab526a9a7;
evidence-descriptors.js 43e9b9a47134ab1c0e22f2d0559fb4f12b07ea6b549e14cf185eaa645cbc74d4;
test-p0f-evidence.js bf94e3a0cfbcce9ca03abb30dba108b8ccb483f5fa0d1bd0fe48f3aaa269518a;
fixtures/evidence-checker.js 9dadd7a365b911deded999329e5e5fa04b94f514149632991a08f168da4b38bf.

Transport limitations remain explicit: caller snapshots cannot independently
establish H2 stdin origin/delivery/consumption; absent snapshots are labeled derived
expectations. Direct-child deadlines and open/fstat observations are not process
or filesystem containment. These limits are retained in actual evidence/reviews.

Next auxiliary slice: bind the 11 pending H6 obligations to real H4/H5 owned APIs,
then add end-to-end CLI orchestration and H7's independent checker, real preflight
and two complete qualification runs. Retain all scenario IDs. Where the old draft
expected caller-supplied hash/byte tuples, exercise mutations of the actual
persisted bindings instead: H4/H5 intentionally require their genuine owner and
derive expected bytes independently. Pin exact observed boundaries before marking
those rows executable; do not pretend a fake issuer reaches a later binding guard.

C's 18 new contact-matching cases passed 687 assertions; combined C/refinement
119 cases passed 18618 assertions. Independent static review cleared the matcher,
actual route-port integration, owner-validity guard and owner allocation ordering.
Final qualification is recorded below; native feature remains incomplete.

### X-H7 Checker And Preflight Validation (2026-10-23)

Implementation `a95a5c5ff` couples genuine preflight with its independent audit/
checker, because each's real focused tests require the other. Preflight runs
actual suite attempts, syntax checks, source/runtime/input audits, four explicit
byte-replacement directions and a real stdin-consumption trace child. Replacement
patches are not represented as Git textual patches or semantic C++ proofs.
Declared artifact outputs retain real raw events/diagnostics and exact ordered
bindings. Input/config/roadmap reads are capped before allocation. A fixed pair
of attempts runs concurrently per suite, awaits both siblings even on refusal,
persists their actual outputs in ordinal order and rejects any failed prefix.
H2's300-second deadline and all stream/data caps remain unchanged.

The independent checker imports built-ins and inert descriptors only. It derives
actual input manifests, receipt/transport/process bindings, genesis/chained events,
outputs and canonical suite reports independently; it does not call production
validators. Parent review found and corrected two completeness gaps: require all93
baseline obligations (not86), and require the unique final successful global Node
summary with no failed/cancelled/skipped/TODO counts or unknown top-level entry.
Seven consistent-map ID omission negatives and eight fully rebound raw-event
negatives now pass. Original file identities/bytes are rechecked at the end.

Author independent-checker run:35/35 passed in100.057 seconds, no skips/failures.
Parent combined X-H7-checker-preflight-parent-01.tap under
`/private/tmp/lc2961-impl-evidence.9GtLO2`:39/39 passed (35 checker,4 preflight),
100.122 seconds. Parent own preflight/driver control report
X-H7-parent-reviewed-06.tap passed11/11; the driver remains a separate uncommitted
slice. Independent reviewer separately passed those11 and probed bounded growth
reads, ordered early-FAIL/late-PASS sibling completion and absence of a successful
H2 receipt on failure. No remaining focused preflight/checker P1/P2 finding.

These are synthetic real-child controls, not the complete canonical309-case
qualification or a proof of its runtime envelope. Raw records do not independently
attest historical process origin or stdin delivery. Persisted observations and
automated checker results are not human approval, containment or native feature
completion. Next commit is the reviewed two-run driver, followed by a frozen-source
affected regression run and two complete actual validation runs.

### X-H7 Referenced Artifact Validation (2026-10-23)

Implementation `ceacd01a3` retains original external preflight artifact bytes,
full file identity/mode and exact directory membership. Strict bounded canonical
envelopes are derived from H1-bound stderr, not caller reference claims. The
combined pair admits at most64 distinct references, 8 MiB each, 128 MiB total;
single-link mode0400 files and disjoint protected/output scopes are mandatory.
Original baselines survive checker dispatch, exceptions/failure, review readback
and final metadata checks. Mutation reports PREFLIGHT_ARTIFACT_CHANGED, never a
newly blessed baseline. The manifest schema is unchanged: references are already
bound through receipt stderr.

Independent review found one P2: substring matching could mistake ordinary stderr
for an evidence envelope. Fixed by explicit pinned argv opt-in or the actual
genuine runner requirement; a valid canonical envelope kind also self opts in.
Unflagged opaque text merely mentioning the kind stays ordinary. The synthetic
control fixture opts in before capture and rejects oversized stdin before parsing.
All original44 H5 test names/behavior are preserved, with44 new guard controls.

Author final `/private/tmp/lc2961-h7-optin-full-02.tap`: 88/88 passed, zero skips,
168.147 seconds. Independent first80 rerun passed in171.102 seconds; the corrected
eight opt-in/stdin regressions passed separately in
`/private/tmp/lc2961-h7-optin-independent-targeted-01.tap`, hashes unchanged.
Review then found no remaining focused P1/P2 issue. Parent pre-fix integration
X-H7-artifact-parent-integration-01.tap passed187 cases (80 H5,94 H6,13 pipeline);
it is not represented as the final88 integration. Full affected rerun remains.

Timing is a real readiness gate: serial repetition would exceed H2's unchanged
300-second deadline. The reviewed genuine preflight runs each suite's two attempts
concurrently with a fixed limit of two, awaits both on failure and publishes actual
outputs in deterministic order. Its focused controls are not canonical timing
qualification. Next commit pairs the independent checker with genuine preflight
because their focused tests/real audit require one another; then commit the driver
and run complete frozen-source qualification. No whole H7/native completion claim.

### X-H7 Suite Worker Validation (2026-10-23)

Implementation `e83aaf3f0`: exact four-suite definitions, source/runtime/input
pins, explicit H3 registry/inventory mapping, bounded real Node execution, actual
raw events/diagnostics and deterministic semantic reports. A missing/failed/
skipped/TODO/unknown/prefix result cannot become PASS. Final global Node summary
and exact execution identities are required. Source/runtime/environment changes
refuse; generated publication is exclusive and retains failed prefixes. Bounded
own-data snapshots at both generator entrypoints prevent caller mutation across
await from redirecting publication or promoting purpose. Synthetic one-module
control groups remain supported; validation cannot substitute the actual7 files.

Author focused run passed 22 cases. Parent independent static review identified
the mutable-options defect; the correction and synthetic-control compatibility
regression pass 23/23, zero skips/failures, in X-H7-worker-parent-03.tap under
`/private/tmp/lc2961-impl-evidence.9GtLO2`. Source syntax and whitespace checks
passed. Real-child controls test execution, skipped/failed/missing/reordered
identities, mutation, deadlines/caps, strict H3 outputs and retained discovery
failure diagnostics. These controls do not qualify the full canonical suites.

Discovery readiness correction: local Node 26.12.0's nonmatching name/skip filters
omit individual definitions; the ^$ probe unexpectedly executed the cases and
was not treated as zero-execution evidence. Raw probes 02/03/04 confirm the
filter-only listing gap. The explicit isolated-child preload therefore wraps
registration to mark definitions skipped. This narrow discovery instrumentation
does not replace H1-H6 validators or execute test bodies. Actual qualification
uses unmodified node:test and rejects preload/filter argv; independent checking
must require that exact execution contract. Discovery skips never count as PASS
or executed assertions. No general Node-version compatibility is inferred.

Remaining H7 work is unchanged: guard original externally referenced preflight
artifacts through H5 dispatch/failure/final readback; independently derive all93
mandatory IDs and raw-event completion (including final summary); qualify actual
preflight/syntax/audit/byte-patch/trace evidence; retain original completed-run
owners through final publication; rerun all affected tests and execute the four
canonical suites twice in each of two full isolated qualification runs. Persisted
observations are not human approval or native feature completion.

### Worker Pipeline Validation (2026-10-23)

Implementation `1f9f94753` runs real H3 -> H4 -> H5 APIs and exposes a working CLI
with explicit ledger/checker/manifest paths. It requires its own actual H2 source
pin, snapshots ingress data without getters, rejects overlapping output scopes,
exposes no injection hooks, and retains actual failed-stage diagnostics/prefixes.
Post-root REFUSED is overall FAIL; the nested H5 result remains unchanged.
Final success revalidates the original H5 owner after H4/H2 checks. A real-stage
mutation regression proves a manifest changed at H5 return cannot escape as PASS.

Parent report X-pipeline-reviewed-02.tap in
`/private/tmp/lc2961-impl-evidence.9GtLO2`: 13/13 passed, no skips/failures.
Independent review found and verified both corrections, separately reran all13
and repeated its original two probes; no remaining focused P1/P2 issue. The earlier
276-case integrated run includes the pre-correction 11 pipeline tests; it is not
misrepresented as final13 integration. H7 must rerun all affected components.

This slice automates persisted H4/H5 evidence, not human review or product/native
completion. CLI transport remains explicitly derived rather than claimed observed;
the API can retain actual caller capture snapshots. No filesystem/process-tree
containment or cleanup of failed/external artifacts is claimed. Next auxiliary
slice is H7: genuine execution/inventory/preflight, independent checker, safety
rechecks for referenced preflight artifacts and two complete qualification runs.

### X-H6 Integration Validation (2026-10-23)

Implementation `507e44ad7` binds the remaining 11 obligations to actual owned
H4/H5 APIs. All 93 scenarios are executable; the inventory meta-test makes 94
passing cases, zero pending declarations or skips. Original71 IDs remain exact.
TREE05a-c and JOIN04 reach the genuine persisted-binding guards documented in
section 13.1. Typed exceptions have no invented terminal status. Evidence
publication/control cases retain H3/ledger prefixes, full input/environment
snapshots and actual checker dispatch counts. A hostile real checker that claims
PASS after substituting an output is refused by the actual H5 revalidation.

The shared fixture extraction preserves all 44 original H5 test names/behavior.
It also supports capture-only setup, explicit extra sources and actual worker/
checker module selection before capture; these are test controls, not validation
authority. H6 definitions register once through the parent registry in sorted ID
order. H7 can pin an immutable roadmap expectation snapshot as a real H2 input;
ordinary tests continue to check the live plan.

Parent evidence: X-H6-full-initial-01.tap (94/94) and
X-H6-pipeline-integrated-01.tap (276/276 across H1-H6 plus the then-current 11
pipeline controls) in `/private/tmp/lc2961-impl-evidence.9GtLO2`. The pipeline is
still uncommitted and under correction; its test count is not H7 qualification.
Author final H5 regression `/private/tmp/lc2961-h6-final-owned-h5-regression.tap`
passed 44/44. Independent reviewer reran H6/H5 138/138, checked the original71
against HEAD and separately probed four throwing boundaries; no H6 P1/P2 issue.

Next auxiliary slice is end-to-end CLI orchestration. Independent review found
two pipeline issues to fix before commit: final H5 evidence revalidation, and
overall FAIL normalization when H5 refuses after H3/H4 output creation. Then
finish H7's genuine suites/preflight, independent checker and two complete runs.
No native entity, human approval or whole-roadmap completion is claimed.

### C Topology Validation (2026-10-23)

Implementation `d4a0b081a` completes the immutable public checker transaction,
bounded canonical path search, whole-candidate/reference coverage, fixed-terminal
preflight, one-root symbolic phase cut, strict nonlocal contact query and exact
owned-root/incidence-port matching. No gap is waived around a numerical phase
root: paired domains retain the same symbolic root, and its full enclosing image
contributes to geometric bounds. Numerical seeds propose correspondence only;
fresh outward bounds certify it. Non-exact or unproved contact alignment refuses
Ambiguous rather than inventing a topological match. Immutable proof owners and
allocation-before-debit ordering survive exceptional exits.

All 54 named obligations are present exactly once: 53 public-checker cases and
C27's separately specified two-cycle production routing-kernel case. The latter
is not mislabeled a realizable two-cycle ellipse fixture. The final focused run
has 104 cases / 18056 assertions, all passed; native successful-assertion XML
confirms each identity executed assertions. Including retained-refinement tests,
119 cases / 18618 assertions passed. Independent static reviews cleared the
global checker/kernel, contact query, symbolic-cut correction and matcher.

Final evidence in `/private/tmp/lc2961-impl-evidence.9GtLO2`:

- `C-acceptance-release-07`, `C-acceptance-assertions-07.xml` and
  `C-identities-07.json`: exact identities, zero missing/extra/empty cases.
- `C-full-release-03`: 3979 total, 3953 passed, the same 26 baseline skips,
  7426232 passing assertions; baseline comparison has no lost identities.
- `C-geometry-asan-03`: 357 cases / 1570740 assertions, all passed.
- `C-dxf-release-02` and `C-dxf-asan-02`: 744 total, 741 passed, same 3 skips,
  39929 passing assertions in each lane.
- `C-dwg-release-02` and `C-dwg-asan-02`: 1006 total, 1002 passed, same 4 skips,
  34948 passing assertions in each lane.
- `C-build-release-24.log`, `C-build-asan-08.log`, `C-qmake6-03.log`: successful
  builds, including the qmake6 application/plugins/tools. ASan used
  `detect_leaks=0`; no full-application ASan or leak qualification is claimed.

Next main slice is D-core. Remaining H6 integration, runner orchestration and H7
are independent auxiliary obligations, not proof of native feature completion.

### X-H4 Joined Ledger Validation (2026-10-23)

Implementation `3e06e3894`: inert ledger descriptors, independent actual context
capture after H3, complete genesis and one chained event per executed case, strict
H3 source/runtime/argv/stdin/stream/result/output joins, exclusive mode-0400 ledger
publication in a new nonoverlapping directory and original-identity replay and
revalidation. Failed H3 cannot publish a complete PASS ledger. Publication errors
retain partial trees; unsupported directory fsync is explicit. No filesystem race
elimination, automatic CLI integration, H5 or H7 completion is claimed.

Parent six-suite report in `/private/tmp/lc2961-impl-evidence.9GtLO2/`:
X-H4-parent-integrated-01.tap, 210/210 passed with zero skips/failures. Focused H4
has 25 cases; author combined H1-H4 report X-H4-descriptors-integrated-06.tap has
127/127. Independent reviewer 01a168a8-2b07-77a0-ab4e-4bd44a98895f reviewed all
three frozen source/test files, separately reran H4 25/25, verified hashes before
and after, and found no P1/P2 issues. Three syntax checks and staged whitespace
passed. README documents the actual API and its limits.

Validated SHA-256 for the three files under scripts/offset-curve-validation:
ledger-descriptors.js 160a389332da609a773a77a3e8a0a3cd25a77d5a55db2ba9359071c099acf86d;
formal-ledger.js 0b8452473c4275039d86b2b9f829f0cd3632d0d396edb33bdb66b12deeb8c0b8;
test-formal-ledger.js 04a12d9b217dd6c3a6da3aba8aa4a05eeabbdc10210abc56a3e9c2f2219df341.

Next auxiliary slice: H5 actual evidence manifest, absent publication and review
dispatch with original-byte/identity checks before and after all child outcomes.
Then wire the 11 pending H6 obligations through the real H4/H5 APIs and qualify H7.
C's current source passed all 86 focused cases, full Release (3935 passed/same26
skips), ASan geometry (339 cases), both format fast lanes and qmake6. Independent
C global review remains in progress; none of that uncommitted source is marked
complete by this tooling checkpoint.

### X-H6 Inventory Core Validation (2026-10-23)

Implementation `9170b0cc9` adds explicit refusal-inventory.js, one reachable test
per executable obligation and real transaction/CLI fixture generation. All 71
original IDs remain in the 93-obligation inventory: 82 executable, 11 explicit
pending H4/H5 declarations (not skipped tests or successful evidence). Typed stdout
shape, sidecar binding and original-snapshot readback diagnoses repair five
independently reproduced boundary mismatches. Original metadata is checked before
byte readback/reparsing; both existing H1 mutation/identity diagnostics still pass.
JOIN01 and C27 readiness corrections above are evidence-based plan corrections,
not fabricated missing-feature coverage. The C source is still uncommitted.

Parent report `/private/tmp/lc2961-impl-evidence.9GtLO2/X-H6-parent-initial-03.tap`:
185/185 passed, no skips/failures (102 H1/H2/H3 plus inventory meta-test and 82
scenarios). Independent author/reviewer 01a16889-ca4e-7f01-a931-d5b9dedf50d4
reproduced all original mismatches, reviewed the fixes, and separately reran
185/185 with zero failures/skips before approving this isolated core commit.
Five changed/new JavaScript files passed syntax checks; staged whitespace passed.
Failed initial qualification reports are retained, not overwritten.

Next: finish/review/commit X-H4 joined ledger; implement X-H5 evidence review;
wire the 11 remaining inventory obligations through those actual APIs, then X-H7.
No H4, H5, full H6, H7, native entity or release completion is claimed here.
C has 47 passing path identities plus six budget identities in separate focused
runs; routing C27 and final shared source/regression review remain pending.

### X-H3 Validation (2026-10-23)

Implementation `7da2bb144`: versioned exact registry/config/result contracts,
trusted purpose bootstrap before dependency authorization, real Node child/CLI
dispatch, complete H2 inventory joins, exact declared flat output readback, and
original input/receipt/root guards through success and failure boundaries. Actual
argv/environment/runtime/source/stdin/streams/exit and bounded failure prefixes
are returned. Synthetic fixture refuses validation purpose. No ledger persistence,
independent evidence manifest or H4-H7 completion is claimed.

Parent integrated report X-H3-parent-integrated-02.tap in
`/private/tmp/lc2961-impl-evidence.9GtLO2`: 102 tests, 102 passed, zero skips/failures
(25 H1, 15 input, 32 process, 30 H3). All five new JavaScript files passed syntax
checks; whole diff whitespace passed. Independent reviewer
01a16786-0b92-7183-b563-f1bedab4ba28 identified the pre-authorization adapter access,
then cleared its trusted-bootstrap fix and reran the integrated suite before the
final H2 purpose regression. H3 executable files were unchanged by that regression.
Runner README records direct-child and observation-point limits explicitly.

Next auxiliary slice: X-H4 owns bounded canonical persisted joined events derived
from actual H3 worker rows/output snapshots. Pin expected registry/case order and
all source/runtime/receipt/output identities; do not treat runner-returned rows as
an independent oracle. Revalidate exclusive publication/readback and mutations;
preserve H3's failure prefixes without fabricating complete PASS evidence.
C remains in progress (30 scoped cases passed, only 17 of the 54 public C names).

### X-H2 Validation (2026-10-23)

Follow-up `c939c3de7` rejects inherited, hidden and accessor injection before hook
evaluation, and snapshots the plain configuration's own data properties before
authorizing any dependency. A synthetic purpose cannot change into validation
between authorization, input capture and publication. Regressions exercise zero
getter/adapter calls on refusal and mutation of the original configuration during
spawn without relabeling the synthetic receipt. Original capture schema unchanged.
Parent integrated H1/H2/H3 run: 102 tests passed, zero skips/failures, in
`/private/tmp/lc2961-impl-evidence.9GtLO2/X-H3-parent-integrated-02.tap`.
H2 focused independent rerun: 32/32 passed in X-H2-purpose-pin-independent-02.tap
in the same evidence directory. Reviewer 01a16786-0b92-7183-b563-f1bedab4ba28
cleared the final tree with no remaining P1/P2 findings; both changed JavaScript
files passed syntax checks and the whole diff passed whitespace checks.
H3 remains uncommitted at this checkpoint; C is still incomplete. The C scope
refinement below records the contact-query dependency and corrects the prior B
status label; it is not a claim that those source changes have been committed.

Six files committed: input-capture.js, preflight-capture.js, their two tests,
fixtures/capture-child.js and README.md. Actual stable bytes derive the candidate
SHA-256/length/Git-blob, explicit sorted source/input/fixture/config closure, exact
case inventory, runner and actual Node executable/runtime tuple. Immutable
snapshots retain bytes/metadata; in-process provenance prevents supplied fake
captures from revalidation. Empty/missing source/runtime tuples cannot pass.

The real child receives bounded canonical context on stdin, not per-run argv/env
changes. Two real-child receipts with distinct physical parents successfully join
H1 with identical actual argv/environment. Context delivery completion is recorded,
not misrepresented as child consumption. Parent environment stays unchanged.
The same original bigint run-root-parent identity and captured inputs/runtime are
checked before/after every child outcome and receipt publication/readback.
No malformed child result gets a fabricated receipt; immutable bounded diagnostics
retain actual exit/signal/error/stdout/stderr and stdin transport information.

All 67 integrated tests passed: 15 input, 27 process, 25 H1, no skips/failures.
Five new JavaScript files passed syntax checks; whitespace passed. Independent
reviewer 01a16687-2244-7373-9bb9-fdad31c25cb9 separately reviewed input and process
scopes and reran their tests (15/15 and 27/27); no final P1/P2 findings. The parent
found/fixed the context/environment pair incompatibility before this final tree;
its real-child regression verifies the stdin design. H1's distinct-parent defect
was committed separately first. Worker: 01a16673-d27c-7a73-a34b-4c8519852336.

Limits: 128 files, 8 MiB ordinary file, 128 MiB executable, 256 MiB total, 1 MiB
inventory/4096 cases, 32 KiB context, 64 MiB per child stream and a five-minute
direct-child kill deadline (caller may lower stream/time caps). Publication uses
exclusive/no-follow files, fsync, explicit modes and H1 readback. Unsupported
directory fsync is recorded. Descendants can delay pipe closure; transient
create/delete races between observations are not excluded. Failed publication
may leave artifacts. Explicit dependency closure does not discover transitive
imports/dynamic libraries. The synthetic fixture refuses validation purpose.
H3-H7, persisted stdin/input evidence and whole-harness completion remain pending.

Evidence: /private/tmp/lc2961-impl-evidence.9GtLO2/X-H2-integration-parent-01.tap;
source syntax checks and independent test reruns. No application/build changes.
Validated committed SHA-256:

| File under scripts/offset-curve-validation | SHA-256 |
|---|---|
| input-capture.js | 55ff4be97c7b4603729e9e1b4f924ae05147bf0620719610937f515c260e7a3f |
| preflight-capture.js | 05e0317ab327fc8ea4552f7d39680ed910c962adfc24b953f143def63138e54c |
| test-input-capture.js | 7225b1d315b4104ba505b2cf846783789138eefebef950b3f4f8ead998b17721 |
| test-preflight-capture.js | 2dcedf18a449bb60e91a2b0ad04bbb35529b5c99194ffc155d930adf0be8a103 |
| fixtures/capture-child.js | 4d5d06c4857d5098d5ae5075d39beed4477844336ab933314b1af490726c1523 |
| README.md | f097bc5a2ddb39def7a1e8ca2365be5830367366a9ee474056de8f468b11c35e |

C remains next on the main track. X-H3 is being implemented in disjoint new
tooling paths; neither track's unfinished work is labelled complete.

### B Validation (2026-10-23)

Scope: only lc_ellipseoffsetgeometry.h/.cpp and their geometry test TU. Separate
occurrence incidence and a two-slot-per-edge optional table are owned by immutable
ReferenceComponentAnalysis. Exact source/free-end and pair provenance is retained;
compatible source, paired-Own or explicit seam continuation prevents PairTrim.
Other reference roles cannot supply Own continuation. Invalid lookup IDs/sides
return no terminal. Discovery, visibility and reference geometry are unchanged.

Storage Ns+2E is checked/reserved before one atomic vertex debit. Each edge/side
and free-end/pair scan is charged before inspection. The simple open fixture has
Ns=4/E=1: six terminal storage slots and ten terminal-stage pair debits. Tests cover
every scan prefix, exact/one-short storage, pre-spent totals, sticky retries and
owner lifetime beyond the caller budget. G4's cutter crossing and G2's paired
lens endpoints use independent high-precision parameter/visibility oracles.

- Acceptance Release/ASan: exactly 8 identities, 891 passing assertions, no skips.
- Geometry Release/ASan: 253 cases, 1,552,678 assertions passed.
- Full Release: 3,875 cases (3,849 passed, same 26 pre-existing skips),
  7,408,170 assertions. No missing baseline names or new skips; exactly eight
  new top-level identities plus B01's separate JUnit section row.
- Fast DXF Release/ASan: 744 cases, same 3 skips, 39,929 assertions; fast DWG:
  1,006 cases, same 4 skips, 34,948 assertions. Exact baseline names/skips match.
- CMake built all three test targets in both configurations; full incremental
  qmake6 make -j2 passed. No ASan diagnostic in scoped runs; leak checking and
  full-app ASan are not claimed. No qmake5 changes.
- Independent reviewer 01a1663b-5c1d-7333-98a2-ad49b643a3f4 reviewed the complete
  production/test diff and reported no actionable findings. Whitespace passed.

B05 runtime-covers a retained seam through event (circle and ellipse, both sweep
directions), not same-Own-occurrence stationary-interior continuation. That branch
is source-reviewed only. Whole E(100,50), D=-25 and an interior-pi arc probe both
remain pre-terminal LimitExceeded in existing reference discovery (status 3;
the previous wording incorrectly called this AmbiguousTopology). A negative
angle probe correctly refused InvalidInput. Failed reports B-acceptance-release-
01/02/03 are retained; neither skipped cases nor changed geometry hides these
limits. Stationary source-endpoint support remains covered by prior tests. C must
preserve precise refusal and may fix a proved reference dependency explicitly.

Evidence: /private/tmp/lc2961-impl-evidence.9GtLO2/B-acceptance-{release,asan}-final,
B-geometry-{release,asan}-01, B-full-release-01, B-{dxf,dwg}-{release,asan},
B-release-build-05, B-asan-build-02, B-qmake6-build-01 (logs/XML/summaries as
applicable). Tested bytes equal committed blobs; SHA-256:

| Entity-directory file | SHA-256 |
|---|---|
| lc_ellipseoffsetgeometry.h | b0f96b19a8109797cc90457c1a76a91ca5697c6bf117df47af055cca3f0f44b5 |
| lc_ellipseoffsetgeometry.cpp | 726008f85d176265515f9c304422b1e5869eb1423ef292ba7d66b6dd5f6a0947 |
| tests/lc_offsetcurve_geometry_tests.cpp | 95d5e91b7fb735766259e0594b17c090c68b9fb870e97765e89a804680d34048 |

B is complete within its terminal contract; no public cache validity, native
entity/UI/file support or whole-roadmap completion is claimed. C is next.

### X-H1 Physical-Parent Correction (2026-10-23)

The receipt pair now distinguishes physical run-root parents by device/inode,
not path/device/inode. Different declared paths cannot hide a repeated physical
parent. The focused regression refreshes the nested stdout byte/hash binding
after mutation and reaches this exact refusal, not an earlier digest error.
Only preflight-receipt.js and its test changed in product commit 8993e2848.

Parent and independent reviewer 01a16687-2244-7373-9bb9-fdad31c25cb9 each passed
all 25 receipt tests, zero failures/skips. Parent H1/H2 integration passed all
67 tests; this is scoped integration evidence, not H2 final review or H3-H7
completion. Evidence: /private/tmp/lc2961-impl-evidence.9GtLO2/
X-H1-physical-parent-tests-02.tap and X-H2-integration-parent-01.tap. The first
test attempt hit the deliberately stale stdout digest and is retained separately.

Validated SHA-256: preflight-receipt.js
7c2a58d8c2ed02fddc9d01fda5a7669684c682164f512cb569608c7f2f406194;
test-preflight-receipt.js
4558d85404c560bb3f4f74ed71287f4078cdc310a9c31a180d9bd6d7f83793bf.
B remains the next main slice; X-H2 awaits final process-capture review. No
application or geometry correctness is inferred from these receipt tests.

### A-cache Validation (2026-10-22)

Scope: geometry header, implementation and test TU. Candidate subdivision now
atomically admits two child spans/two vertices; proof-only subdivision admits
zero spans/two vertices. All five production callers use the helper. Storage is
prepared first; no popped parent is recharged. Two-argument checkDepth safely
checks the sum; normalized reference history was already present and is preserved,
now also published on both local witness paths. Existing combined-loop guards
were preserved/hardened, not presented as an invented missing reset fix.

Independent reviewer Gibbs (01a1663b-5c1d-7333-98a2-ad49b643a3f4) found no
actionable issue in the full three-file diff. Its initial review noted that A03's
reduced-depth failure was not stage-isolated. A test-only extension resolves it:
the actual rounded controls have an exactly rational positive projected
quadratic derivative (A>0, 4AC-B^2>0), requiring candidate refinement near the
non-dyadic parameter 1/3. Both cache orders pass at depth 11 versus bounds depth
8; ceiling 10 retains successful bounds, refuses local publication, and observes
exactly 10. The bounded recheck found no new issue. Allocation exception paths
remain source-reviewed, not fault-injected.

- Exact A01/A02/A03/A09 discovery: four names in Release/ASan; final acceptance
  each passed 986 assertions. The stage-isolated section separately passed 35
  assertions; its successful-value trace is retained.
- Final full Release: 3,867 cases (3,841 passed, same 26 skips),
  7,407,279 assertions. No missing prior name or added skip against A-reference.
- Final ASan geometry: 245 cases / 1,551,787 assertions, zero failures/skips;
  prior name set retained. No sanitizer diagnostics, detect_leaks=0 as before.
- DXF fast Release/ASan: 744 cases / same 3 skips / 39,929 assertions.
  DWG fast Release/ASan: 1,006 cases / same 4 skips / 34,948 assertions.
  Name/skip comparisons passed. All CMake targets and full incremental qmake6
  passed. Final test-only strengthening did not alter either fast binary or app
  source; affected application tests were rebuilt/rerun in both configurations.

A01's cubic control hull has .03 perpendicular deviation but actual excursion
.03/(2*sqrt(3)) < .01, so it qualifies candidate refinement without requiring
unproved broad-arc acceptance. The local folded-net fixture's scalar derivative
minimum is 1/104 >0. A03's high-curvature/coarse-tolerance fixtures test internal
layers only; they are not public TooSmall/cache-acceptance claims. Initial
unqualified broad-arc/Hermite probes are retained as failures, not relabeled.

A09 independently accounts for 17 pairs after the reference stage: prepare 5;
one span plus 3*(dispatch+examination)=7; coverage span/advance/3 witnesses=5.
Logical vertices are metadata1 + occurrence-index4 + fragments3 + roots/output6
=14. All 18 pair boundaries, with/without pre-spent work, match source-derived
snapshots and sticky retries. The existing bounds helper's old no-refinement span
equality was corrected to the invariant initialSpans+2*candidateSplits; direct
production and exact/short cap tests enforce the actual split admissions.

The current whole geometry corpus passed default 4096 candidate spans, 65536
vertices, 1<<20 pairs/ordering and depth20; no default was silently broadened.
DFS scratch is one root plus a sibling per affordable remaining split. Public
global coverage/unique-path validity remains C, not an A-cache claim.

Evidence: /private/tmp/lc2961-impl-evidence.9GtLO2/A-cache-*; final full/ASan
reports use *-final, acceptance Release uses *-04-depth, and the isolated trace
is A-cache-stage-isolated-depth-trace.log. Validated committed source SHA-256:

| File in entity directory | SHA-256 |
|---|---|
| lc_ellipseoffsetgeometry.h | 44e1ee29bfdd17ad531c9fcac457b2a75f953b56f4590599b49f9255be074525 |
| lc_ellipseoffsetgeometry.cpp | c0bb0bac67938999d34526628e834233418ac1aec7f46f7ba7442bfa8695900a |
| tests/lc_offsetcurve_geometry_tests.cpp | 6de41728d7f87a546f57088371b22d147d81298e2b4b3b6250ac4148d62e7292 |

### A-reference Validation (2026-10-22)

Scope: geometry implementation and its test TU; header/API unchanged. Independent
reviewer Gibbs (01a1663b-5c1d-7333-98a2-ad49b643a3f4) reviewed the full final diff,
including later capacity guards and A11 boundary test, and found no actionable
issue. Exceptional geometry transfers/allocation failures were source-reviewed,
not fault-injected; strict factory/retained callback exception tests remain in
the passing regression suite. No new fault hooks or fabricated exception PASS.

- A05/A06/A07/A11 exact discovery: four names in both builds; Release and ASan
  acceptance each passed 2,873 assertions in four cases.
- Final full Release: 3,863 cases (3,837 passed, same 26 skips), 7,406,269
  assertions. No missing prior JUnit name or added skip against A-strict.
- ASan geometry: 241 cases / 1,550,777 assertions, zero failures/skips; no lost
  prior name or added skip. No sanitizer diagnostic; detect_leaks=0 as before.
- DXF fast Release/ASan: 744 cases / same 3 skips / 39,929 assertions.
  DWG fast Release/ASan: 1,006 cases / same 4 skips / 34,948 assertions.
  Name/skip equality against the previous slice passed. All targets rebuilt.
- CMake Release/ASan and incremental full qmake6 passed; final source hashes
  remained unchanged through review, validation and commit.

Source-derived census: whole sources start at M_PI_4. Four real axes each need
one six-comparison rational map, then stable insertion needs 3 forward / 6 reverse
predicates (27/30 total). Four cusp maps/order add another 27/30 (54/60 total).
Retained issuance adds four maps plus two four-value image reductions per
occurrence: 4*(6+6+6)=72. A compound contraction similarly adds 72. Every reduction
comparison is admitted separately, including an exhausted last predicate.
Axis inventory scans have 21 admitted visits and three adjacency checks; critical
proof scans have at most 20 quadrant/20 occurrence visits each. Three fixed
preflight seeds per admitted segment are separately charged. Provider-internal
bounded helper multiplicity remains under its outer callback admission, not
duplicated. Sort bounds stay 210/190 comparisons and 250/228 moves.

Capacity preparation precedes retained-record/staged-snapshot and paired-output
debits. Classify/frame roots use one atomic vertex/pair admission; no extra root
was invented. Factory workOnExit and retained monotone snapshots transfer all
items/pairs/ordering/depth exactly once, including throws, without retaining a
caller-budget pointer. A11 independently reproduces the provider/core boundary
and checks exact/short public refusal snapshots. Tight split fixtures changed
23 to 26 pairs solely for the three new axis adjacency checks; geometric
expectations and original split/depth boundary were retained.

Evidence: /private/tmp/lc2961-impl-evidence.9GtLO2/A-reference-* (failed initial
accounting probes retained separately). Full Release JUnit time 82.093 s;
ASan geometry 121.898 s; acceptance Release 0.039 s. These are whole test-suite
times, not per-query UI responsiveness or release-completion evidence. The
default 1<<20 ordering/pair and 65536 vertex caps passed the current corpus;
candidate-span/depth integration and its envelope remain A-cache work.

| File in entity directory | Validated SHA-256 |
|---|---|
| lc_ellipseoffsetgeometry.cpp | bb6881d5df576f55d0c7688f6be5c9535e6d4dac2fe3fd9da1a7fceecebac2b9 |
| tests/lc_offsetcurve_geometry_tests.cpp | e87d650ecebbea90c30dbae18dcb4742cdb6271bbe6bd4de78a497d821c112ff |

### A-strict Validation (2026-10-22)

Scope: exactly the solver header, implementation and test TU. Worker Gibbs
implemented the slice. Parent reproduced the tall, bit-reversed separated-cloud
pruning regression in the initial global-order proposal; adaptive longest-axis
partitioning was restored with counted per-node heapsort. Its regression uses
128 separated lines: 127 candidate pair examinations, versus refusal at 512 in
the rejected proposal. Three legacy non-strict ordering paths remain unchanged.

Independent reviewer 01a16687-2244-7373-9bb9-fdad31c25cb9 found no actionable
issue in the frozen worker diff. Parent integration review added a per-call
retained ordering allowance; bounded independent recheck resolved the sole test
oracle correction (a staged event remains charged on comparison refusal).
Failed experimental reports remain preserved, not relabeled as passing.

- Exact A08/A12/A13/A14 acceptance: 4 cases / 942 assertions, Release and ASan.
- Final ASan geometry: 237 cases / 1,547,904 assertions, zero failures/skips.
  Full Release includes the same geometry identities; no prior JUnit name lost.
- Final full Release: 3,859 cases (3,833 passed, same 26 skips),
  7,403,396 assertions passed. Name/skip comparison against A-core passed.
- Worker pre-integration focused strict/refinement suite: 50 identities and
  3,887 assertions in both configurations; all its identities remain in the
  final full/geometry reports. The final allowance adds one A13 section, not a
  fifth required acceptance identity.
- DXF fast Release/ASan: 744 cases, 3 existing skips, 39,929 assertions.
  DWG fast Release/ASan: 1,006 cases, 4 existing skips, 34,948 assertions.
  Exact baseline names/skips retained. Final allowance changes do not enter
  either fast binary's source graph; both targets were explicitly rebuilt.
- CMake all three targets and incremental full qmake6 build passed. Final
  application header/implementation bytes were rebuilt in both configurations.
- ASan used detect_leaks=0; no ASan diagnostics in completed final runs. Leak
  checking and full-app ASan are not claimed. The existing ASan compact-unwind
  size warning remains outside this slice.

Canonical A13 straight-query trace: four centroid reductions + one axis choice +
one heap comparison + two BVH index reductions + two same-branch range reductions
= 10 ordering comparisons; first eight precede the single candidate pair debit.
Caps remain 4/3 synchronous strict and 6/5 retained. The new per-call allowance
tests both current remaining work and original session ceilings at zero/one.
No partial event or owner is published on refusal; staged logical work remains.

Evidence directory: /private/tmp/lc2961-impl-evidence.9GtLO2. Final reports use
A-strict-parent-*-final-02; fast reports use A-strict-parent-{dxf,dwg}-*. The
worker's source-derived census and rejected pruning probe are retained there.
Source SHA-256 (committed blobs equal tested working bytes):

| File in entity directory | SHA-256 |
|---|---|
| lc_parametriccurveintersection.h | 7278eb9d18b10121ef066a0d7534e0a69f4e143f0822c5704dcdc645dadc8b14 |
| lc_parametriccurveintersection.cpp | 5c85d70852193f1dc2c17cd1f61e24bf2e2ff3b831d6b0682bcce82c54f70405 |
| tests/lc_parametriccurveintersection_tests.cpp | 46f18c21b78811942b4ec5e0b89488fb7d94cb403a99b241fe5dc0cdbb9196e6 |

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
B; the auxiliary next card
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
