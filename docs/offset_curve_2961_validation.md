# R01-R15 Closure Evidence Index

2026-10-24 historical record. Production/test links are relative to this
directory. Roadmap links target the consolidated
[historical candidate evidence](plan_offset_curve_2961.md#54-historical-candidate-evidence).
Slice labels retain the original Section 14 provenance; they no longer name
separate roadmap sections. Rows navigate the recorded implementation, tests and
qualified product commits; they are not fifteen new gates. Historical Save As01,
R03-final02 and the C-search record below have distinct source scopes. Preserve
the cited slices' explicit limits. This index is not current acceptance and adds
no fresh PASS claims for the current source.

| Requirement | Recorded Production | Recorded Tests | Historical Evidence / Product |
| --- | --- | --- | --- |
| R01 Domain / normalization | [lc_ellipseoffsetgeometry.cpp](../librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.cpp) | [geometry](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_geometry_tests.cpp), [native edits](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_edit_tests.cpp) | [A-reference](plan_offset_curve_2961.md#54-historical-candidate-evidence) `5c12fbeef`; [D-edit](plan_offset_curve_2961.md#54-historical-candidate-evidence) `4300c1031`. |
| R02 Geometry / complete topology | [lc_ellipseoffsetgeometry.cpp / checkCache](../librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.cpp), [lc_offsetpathsearch.cpp](../librecad/src/lib/engine/document/entities/lc_offsetpathsearch.cpp) | [geometry](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_geometry_tests.cpp), [path search](../librecad/src/lib/engine/document/entities/tests/lc_offsetpathsearch_tests.cpp) | [C topology](plan_offset_curve_2961.md#54-historical-candidate-evidence) `d4a0b081a`; [C-search](plan_offset_curve_2961.md#54-historical-candidate-evidence) `b6814fb3a63dd66bb3aeae9e5b5e66668f8b8591`. The preserved C54 set is 53 public-checker identities plus C27 routing, not 54 public certificates. |
| R03 Consistency / atomic edits | [lc_offsetcurve.cpp](../librecad/src/lib/engine/document/entities/lc_offsetcurve.cpp), [edit snapshot](../librecad/src/ui/components/utils/lc_offsetcurveeditsnapshot.cpp), [final input seal](../librecad/src/ui/components/utils/lc_offsetcurvefinalinput.h) | [native edits](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_edit_tests.cpp), [dialog](../librecad/src/ui/dialogs/entity/tests/lc_offsetcurve_propertiesdialog_tests.cpp), [Trim publication](../librecad/src/actions/drawing/modify/tests/lc_offsetcachetrim_action_tests.cpp) | [D-edit](plan_offset_curve_2961.md#54-historical-candidate-evidence) `4300c1031`; [H-dialog](plan_offset_curve_2961.md#54-historical-candidate-evidence) `22f9e5609`; [R03 final closure](plan_offset_curve_2961.md#54-historical-candidate-evidence) `f617fcf70`. |
| R04 Clone / transforms / owners | [lc_offsetcurvetransform.cpp](../librecad/src/lib/engine/document/entities/lc_offsetcurvetransform.cpp), [rs_insert.cpp](../librecad/src/lib/engine/document/entities/rs_insert.cpp) | [transforms](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurvetransform_tests.cpp), [copy semantics](../librecad/src/lib/engine/document/container/tests/lc_copy_semantics_tests.cpp) | [D-core](plan_offset_curve_2961.md#54-historical-candidate-evidence) `92758859c`; [E-tools](plan_offset_curve_2961.md#54-historical-candidate-evidence) `a37982bd6`. |
| R05 Create / repeat / kept plain | [rs_creation.cpp](../librecad/src/lib/creation/rs_creation.cpp), [lc_offsetcurveagain.cpp](../librecad/src/lib/engine/document/entities/lc_offsetcurveagain.cpp), [Offset action](../librecad/src/actions/drawing/modify/lc_action_modify_offset.cpp) | [dispatch](../librecad/src/lib/creation/tests/lc_offsetcurve_action_dispatch_tests.cpp), [repeat](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurveagain_tests.cpp), [option/messages](../librecad/src/actions/drawing/modify/tests/lc_offsetcurve_enable_tests.cpp) | [E-tools](plan_offset_curve_2961.md#54-historical-candidate-evidence) `a37982bd6`; [E-again](plan_offset_curve_2961.md#54-historical-candidate-evidence) `bf7b512cf`; [H-enable](plan_offset_curve_2961.md#54-historical-candidate-evidence) `3d7195631`. |
| R06 Type dispatch / names | [rs.h](../librecad/src/lib/engine/rs.h), [descriptor registry](../librecad/src/lib/selection/metaentity/lc_entitymatchdescriptorsregistry.cpp) | [types/actions](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_types_tests.cpp), [names](../librecad/src/ui/components/utils/tests/lc_offsetcurve_names_tests.cpp) | [F-types](plan_offset_curve_2961.md#54-historical-candidate-evidence) `49c9b328b`; [F-names](plan_offset_curve_2961.md#54-historical-candidate-evidence) `f0fb28fab`; later Trim transitions: [T-actions](plan_offset_curve_2961.md#54-historical-candidate-evidence) `49bb7755f`. |
| R07 DXF/DWG read AND write | [rs_filterdxfrw.cpp](../librecad/src/lib/filters/rs_filterdxfrw.cpp) | [format matrix](../librecad/src/lib/filters/tests/lc_offsetcurve_matrix_tests.cpp), [trim files](../librecad/src/lib/filters/tests/lc_offsetcachetrim_file_tests.cpp) | [G-matrix](plan_offset_curve_2961.md#54-historical-candidate-evidence) `79ae1930b`; [T-files](plan_offset_curve_2961.md#54-historical-candidate-evidence) `905158174`. Preserve primary/auxiliary reader limits; final GUI is separately build-scoped. |
| R08 Schema / safe metadata | [lc_offsetcurvexdata.cpp](../librecad/src/lib/filters/lc_offsetcurvexdata.cpp), [native storage](../librecad/src/lib/engine/document/entities/lc_offsetcurve.cpp) | [schema](../tests/lc_offsetcurvexdata_tests.cpp), [native entity](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_tests.cpp) | [G-schema](plan_offset_curve_2961.md#54-historical-candidate-evidence) `305042903`; wiring/packetization: [G-adapter](plan_offset_curve_2961.md#54-historical-candidate-evidence) `32597762b`. |
| R09 Selection / native UI / grips | [property provider](../librecad/src/ui/dock_widgets/property_sheet/metaentity/entities/lc_propertiesprovider_offsetcurve.cpp), [dialog editor](../librecad/src/ui/dialogs/entity/lc_propertieseditingwidget_offsetcurve.cpp), [Default action](../librecad/src/actions/drawing/rs_actiondefault.cpp) | [sheet](../librecad/src/ui/dock_widgets/property_sheet/tests/lc_offsetcurve_properties_tests.cpp), [dialog](../librecad/src/ui/dialogs/entity/tests/lc_offsetcurve_propertiesdialog_tests.cpp), [mouse/grips](../librecad/src/actions/drawing/tests/lc_offsetcurve_drag_tests.cpp) | [H-sheet](plan_offset_curve_2961.md#54-historical-candidate-evidence) `678590af4`; [H-dialog](plan_offset_curve_2961.md#54-historical-candidate-evidence) `22f9e5609`; [H-drag](plan_offset_curve_2961.md#54-historical-candidate-evidence) `f214b3408`; [R03 final closure](plan_offset_curve_2961.md#54-historical-candidate-evidence) `f617fcf70`. Automated cases are not physical UI observations. |
| R10 Update / Restore / entry points | [Update](../librecad/src/actions/drawing/modify/lc_action_offsetcurve_updatesource.cpp), [Restore](../librecad/src/actions/drawing/modify/lc_action_offsetcurve_restoreoriginal.cpp), [source UI](../librecad/src/ui/components/utils/lc_offsetcurveui.cpp) | [Update](../librecad/src/actions/drawing/modify/tests/lc_action_offsetcurve_source_tests.cpp), [Restore](../librecad/src/actions/drawing/modify/tests/lc_action_offsetcurve_restoreoriginal_tests.cpp), [source links](../librecad/src/ui/components/utils/tests/lc_offsetcurve_source_ui_tests.cpp) | [I-update](plan_offset_curve_2961.md#54-historical-candidate-evidence) `379729bb3`; [I-restore](plan_offset_curve_2961.md#54-historical-candidate-evidence) `0d3fed653`; [I-ui](plan_offset_curve_2961.md#54-historical-candidate-evidence) `4a231d243`; [R03 final closure](plan_offset_curve_2961.md#54-historical-candidate-evidence) `f617fcf70`. |
| R11 Build / regression / portability | [CMakeLists.txt](../CMakeLists.txt), [src.pro](../librecad/src/src.pro), [file dialog service](../librecad/src/ui/dialogs/file/export/layers/lc_filedialogservice.cpp) | [registered Trim actions](../librecad/src/actions/drawing/modify/tests/lc_offsetcachetrim_action_tests.cpp), [Save As filters](../librecad/src/ui/main/persistence/tests/lc_dwg_save_filter_tests.cpp) | [T-actions](plan_offset_curve_2961.md#54-historical-candidate-evidence) `49bb7755f`; [T-files](plan_offset_curve_2961.md#54-historical-candidate-evidence) `905158174`; [Save As](plan_offset_curve_2961.md#54-historical-candidate-evidence) `091de1a6a`; [later native runs](#current-native-qualification). Host CMake/qmake evidence is not all-platform CI. |
| R12 Bounded work / UI responsiveness | [geometry budgets](../librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.cpp), [stored-cache trim](../librecad/src/lib/modification/lc_offsetcachetrim.cpp) | [topology budgets](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_topology_budget_tests.cpp), [path search](../librecad/src/lib/engine/document/entities/tests/lc_offsetpathsearch_tests.cpp), [trim kernel](../librecad/src/lib/modification/tests/lc_offsetcachetrim_tests.cpp), [isolated failures](../librecad/src/lib/modification/tests/lc_offsetcachetrim_failure_tests.cpp) | [A-core](plan_offset_curve_2961.md#54-historical-candidate-evidence) `0eb2a0cfa`; [T-kernel](plan_offset_curve_2961.md#54-historical-candidate-evidence) `04c712069`; [T-files](plan_offset_curve_2961.md#54-historical-candidate-evidence) `905158174`; [C-search](plan_offset_curve_2961.md#54-historical-candidate-evidence) `b6814fb3a63dd66bb3aeae9e5b5e66668f8b8591`. |
| R13 Durable product / checkpoint | [Trim action example](../librecad/src/actions/drawing/modify/lc_action_modify_trim.cpp) | [actual action TU](../librecad/src/actions/drawing/modify/tests/lc_offsetcachetrim_action_tests.cpp) | [Commit Ledger](plan_offset_curve_2961.md#54-historical-candidate-evidence); Save As `091de1a6a` / plan `861b58957`; [R03](plan_offset_curve_2961.md#54-historical-candidate-evidence) `f617fcf70` / plan `27ee1ecc`; [C-search](plan_offset_curve_2961.md#54-historical-candidate-evidence) `b6814fb3a63dd66bb3aeae9e5b5e66668f8b8591` / plan `4fa45611a291e06f9f0122f84120e23d1c2e87ac`. |
| R14 Original raw carrier / no repair | [lc_offsetcurvecarrier.cpp](../librecad/src/lib/filters/lc_offsetcurvecarrier.cpp), [admission boundary](../librecad/src/lib/filters/rs_filterdxfrw.cpp) | [carrier](../tests/lc_offsetcurvecarrier_tests.cpp), [actual adapter](../librecad/src/lib/filters/tests/lc_offsetcurve_adapter_tests.cpp) | [G-schema](plan_offset_curve_2961.md#54-historical-candidate-evidence) `305042903`; [G-adapter](plan_offset_curve_2961.md#54-historical-candidate-evidence) `32597762b`. Raw helper success alone is not C certification. |
| R15 Failed-save target / document atomics | [export boundary](../librecad/src/lib/filters/rs_filterdxfrw.cpp) | [hostile files](../librecad/src/lib/filters/tests/lc_offsetcurve_hostilefile_tests.cpp), [persistence/refusals](../librecad/src/lib/filters/tests/lc_offsetcurve_persistence_tests.cpp) | [G-fixtures](plan_offset_curve_2961.md#54-historical-candidate-evidence) `7e46b2de8`; legacy refusal scope: [T-files](plan_offset_curve_2961.md#54-historical-candidate-evidence) `905158174`. Stream injection is not physical disk exhaustion. |

## Final Host Qualification

The checkpoints below retain their own source/binary and report identities;
earlier successful runs are not relabeled as later-source executions.

### Historical Save As Checkpoint

Product `091de1a6a3e2ed684080608b1102ffe06b483ab3`, immediately checkpointed as
`861b58957f6284d1d315ecbcb00527ef0b295427`, includes the reviewed Save As correction.
The final twelve-lane 01 graph exited 0 in every lane, with no missing execution
or source/binary identity error. CMake Release/ASan applications and all selected
test/failure targets built; qmake6 application built. No source was rebuilt or
edited during qualification. Three source and 13 executable hashes were identical
before/after. The actual product-SHA audit matched all three committed blobs.

| Run | Cases / Assertions | Scope |
| --- | --- | --- |
| Focus Release/ASan | 3 / 158 each | Actual modal Save As; no skips |
| Full Release | 4761 / 10228234 | 4734 passed; 27 unchanged environment/fixture skips |
| Affected ASan | 1177 / 4314404 | 1176 passed; one unchanged legacy-export skip; no sanitizer diagnostic |
| DXF fast Release/ASan | 765 / 82553 each | Three unchanged skips |
| DWG fast Release/ASan | 1021 / 362942 each | Four unchanged skips |
| Adjacent Trim Release/ASan | 48 / 63167 each | No skips; identities match T-files03 |
| Storage Release/ASan | 26 / 773 each | 25 passed; one existing JWW-writer skip |
| Isolated failure modules | Four processes, all 0 | Batch 2/0/1/0; trim 1616 allocation failures, 20 late cancellations, four conic failures/four cancellations; not Catch assertions |

At Save As01, the exact-name audit found A14+B8+C54 bases once/unskipped in both
main XMLs. C54 comprises 53 public-checker identities plus C27 routing.
Each also contains 43 unskipped mandatory SECTION detail rows, not 43 extra bases.
The report audit preserves predecessor membership, multiplicity and skip states;
no test retirement or rename allowance was used. Membership is not continuous
geometry or binary/linkage proof; ordinary execution, freeze and blob records
provide their separate evidence. C27 remains a routing-kernel test.

[Actual GUI observations](offset_curve_interactive_smoke.md#recorded-checks-2026-10-24)
retain exact historical/Save As01 application and output identities. The actual
CMake/qmake DWG2018 and DXF Save/Open preserve native/restored-ellipse/plain-spline
roles. Second DWG generation and real Trim/Undo-before-save native checks are
specifically CMake observations. Keyboard navigation was restored OFF; effective
Editable ON/original Offset options were preserved. Approved normal-profile
recents/paths/filters/window writes are not exact profile rollback or isolation.

## Current Native Qualification

"Current" in this preserved heading refers to the recorded candidate checkpoints,
not the present worktree or current roadmap acceptance.

R03 product `f617fcf70b46a0236a8725140aeb10d475c7d9ec`, checkpoint `27ee1ecc`,
has the [recorded final02 qualification](plan_offset_curve_2961.md#54-historical-candidate-evidence):
202 cases / 67,779 assertions in each configuration across six complete TUs;
full Release: 4,771 cases / 10,231,099 assertions (4,744 passed, 27 unchanged skips);
affected ASan: 1,187 cases / 4,317,269 assertions (1,186 passed, one unchanged
legacy-export skip). Nineteen source and 13 executable identities were stable;
all 19 committed source blobs matched.
Four actual Trim/storage Release/ASan command/environment/exit records close
the old missing-receipts gap, including source identity references. The wrapper
cancellation at allocation 238 and three positive controls are retained native
tool observations, not extra Catch assertions.

C-search product `b6814fb3a63dd66bb3aeae9e5b5e66668f8b8591`,
checkpoint `4fa45611a291e06f9f0122f84120e23d1c2e87ac`:
[recorded implementation and validation](plan_offset_curve_2961.md#54-historical-candidate-evidence).
Build/source correspondence: CMake Release/ASan applications and all five
test/failure targets, plus qmake6, built successfully. Six source and 13 executable
identities stayed identical across the final runs; all six committed blobs match
qualified bytes. All twelve lanes exited 0 with no timeout, sanitizer diagnostic,
execution gap or identity error. The v4 audit preserves predecessor identities,
multiplicities and skip states without retirements or renames.

| C-search Execution | Recorded Result |
| --- | --- |
| Focused Release / ASan | 285 cases / 1,568,398 assertions each; no skips |
| Full Release | 4,786 cases: 4,759 passed, 27 unchanged skips; 10,236,612 assertions; exit 0; 165.539231 seconds |
| Affected ASan | 1,202 cases: 1,201 passed, one unchanged legacy-export skip; 4,322,782 assertions; 844.059022 seconds under the unchanged 1,800-second limit |
| DXF/DWG Release/ASan | DXF: 765 cases / 82,553 assertions each, three unchanged skips; DWG: 1,021 cases / 362,942 assertions each, four unchanged skips |
| Existing adjacent Trim/storage scope | Four fresh native invocations: Trim 48 cases / 63,167 assertions each, no skips; storage 26 cases / 773 assertions each, one unchanged JWW skip. Exact native identities and skip states match R03-final02 |
| Isolated failure modules | Four native processes exited 0. Batch 2/0/1/0; trim 1,616 allocation faults, 20 late cancellations, four conic faults/four cancellations; wrapper allocation 238 cancellation and three positive controls. These are not Catch assertions |

Preserved A14/B8/C54 membership and additional C-search cases:
all 76 required bases occur exactly once and unskipped in both current main XMLs.
The 43 mandatory SECTION detail rows receive no extra base credit. All 15 additions
pass within the complete 285-case focus. The retained current receipt is
`C-search-mandatory-geometry-01.json`. C27 remains routing-kernel evidence, not a
public ellipse certificate. These native-source results do not transfer the
older GUI observations to R03/C-search application bytes or create an H7 timing
claim for the new C++ candidate. The retained receipts below identify each scope.

## Retained Evidence

The original temporary artifact root was `/private/tmp/lc2961-impl-evidence.9GtLO2`.
Those historical artifacts may now be gone; this index does not assert current
availability or current acceptance. Do not infer contents from filenames alone.
The hashes below identify the receipts used for the recorded candidates, not a
fresh verification of their contents. At the recorded qualification, no temporary
evidence tree was deleted or rewritten for a retry. Full commands, exits, compact
logs and native XML were separate from this committed summary; publication of raw
logs is not claimed.

| Artifact (relative to evidence root) | SHA256 |
| --- | --- |
| `Z-save-filters-test-invocations-01/run-manifest.json` | `1c9d8b3f9f643e51a2d1cf384c19da181dc0a539b44d82cfe5bf369f2e24cfba` |
| `Z-save-filters-report-audit-01.json` | `95b7d37257a997e9d7d0d85a48d712ce2cebf92578af2a606b818b73ed85cbbf` |
| `Z-save-filters-committed-blob-audit-01.json` | `05df0b99fefcc7364908b542271c6450c3e4582736f11ed972b7558a8fbe1d19` |
| `Z-final-mandatory-geometry-01.json` | `b4156ec0e338a933a12c732e566bb4e4b9f2264289fd6e3f07365829f78c781b` |
| `Z-retained-G-matrix-01.json` | `5d21c991da46a3b5a27ef4b9cd984682906d5cc76b45657641b45576e94589aa` |
| `Z-physical-observations-01.md` | `2a093c70c1b12298703a08065a9204f27d16354cd34be9f96d2b1789d99d4ea2` |
| `R03-final-test-invocations-02/run-manifest.json` | `d17b829f6f2cb9f4851927d5d4e733ff98b77c7f8d22af0991bb6a2882d47923` |
| `R03-final-report-audit-02.json` | `43dc66e7358790cf48fb70765bca2811e7db10d2ec14938d4fca73367d9f967d` |
| `R03-final-committed-blob-audit-02.json` | `10b1a39cf1b2ad37125d276a103dbe71015ba0e32eb2fd99cab66884a805029b` |
| `R03-final-adjacent-native-tool-records-02.json` | `08de95bc70e2a20bbb753083eab480ad24cb37be1587199a908246a1548309ef` |
| `R03-wrapper-cancel-native-tool-records-01.json` | `4558c55500596820638df051ea9d2457275527bcd5daf797d16542780de7c647` |
| `C-search-test-invocations-01/run-manifest.json` | `3d223234886bd428367c5ab212787f31c0e25874b938d4102861504594f3e836` |
| `C-search-report-audit-01.json` | `07dd982a7834e533bc186544457ef156c6d5ae513a930aa9feed2829f022cf7c` |
| `C-search-committed-blob-audit-01.json` | `ffaccdb8547397ec16c5b24fbf2038213708cb1fee06fa1bff54c2ec8403c947` |

## Limits

- Host: macOS 27.0.1 arm64, Qt 6.12, AppleClang 21, Catch2 3.16.1. GCC/MSVC and
  Linux/Windows CI were unavailable; full main-suite ASan was not run. ASan used
  `detect_leaks=0` because host leak detection is unsupported. Existing compiler/
  linker warnings remain, not warning-clean or leak-free qualification.
- Static analysis attempted 29 real compile-database TUs, with 20 without fatal
  preprocessing/parser failures and nine Boost-blocked. The final Trim follow-up
  retained diagnostics/exit 2. Newer T-files endpoint/CMC and Save As bytes were not
  analyzer-qualified; neither are the newer R03/C-search changes. Independent
  source reviews and runtime checks are separate from those static attempts.
- Retained G qualifies 128 mandatory outputs plus 48 auxiliary observations, not
  whole-CAD conformance. The 24 ODA-converted auxiliary duplicate-Continuous
  strict-reader diagnostics remain. No fresh external CAD execution was inferred
  from byte rechecks. Genuine R13/R14 lanes remain read-only, with qualified
  atomic refusal for unsupported unrelated foreign-object DWG migration.
- X-H7 completed two actual qualifications for the archived observed candidate;
  its 39 source pins name JavaScript files, not this index or the harness README.
  Later C++ changes do not inherit that candidate's exact runtime measurement
  or byte manifest. No future timing guarantee or automatic H7 rerun solely for
  C++ changes is implied; current native gates own C++ semantics.
- R03 final closure does not claim generic post-handoff undo/listener exception
  atomicity or safety under arbitrary concurrent document mutation. Earlier GUI
  observations remain tied to their recorded executable bytes/profile scope.
- A rounded exact-top trim query can conservatively reach its unchanged depth
  limit and refuse without mutation; nearby/left picks succeed. There was no
  tolerance/cap increase or snapping workaround.
- Physical context-menu, compact-widget, held-drag Escape/away-back, locked-peer
  and complete manual-catalogue variants remain unobserved where the smoke record
  says so. Automated actual-Qt coverage is not relabeled physical observation.
