# R01-R15 Closure Evidence Index

2026-10-24. Production/test links are relative to this directory. Section 14
links use heading anchors, not shifting line numbers. Rows navigate the actual
implementation, tests and qualified product commits; they are not fifteen new
gates. Final host results are below. Preserve the cited slices' explicit limits.

| Requirement | Current Production | Current Tests | Recorded Section 14 / Product |
| --- | --- | --- | --- |
| R01 Domain / normalization | [lc_ellipseoffsetgeometry.cpp](../librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.cpp) | [geometry](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_geometry_tests.cpp), [native edits](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_edit_tests.cpp) | [A-reference](plan_offset_curve_2961.md#a-reference-validation-2026-10-22) `5c12fbeef`; [D-edit](plan_offset_curve_2961.md#d-edit-validation-2026-10-23) `4300c1031`. |
| R02 Geometry / complete topology | [lc_offsetresultcheck.cpp](../librecad/src/lib/engine/document/entities/lc_offsetresultcheck.cpp), [lc_offsetpathsearch.cpp](../librecad/src/lib/engine/document/entities/lc_offsetpathsearch.cpp) | [geometry](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_geometry_tests.cpp), [path search](../librecad/src/lib/engine/document/entities/tests/lc_offsetpathsearch_tests.cpp) | [C topology](plan_offset_curve_2961.md#c-topology-validation-2026-10-23) `d4a0b081a`; C27 tests routing, not a public ellipse certificate. |
| R03 Consistency / atomic edits | [lc_offsetcurve.cpp](../librecad/src/lib/engine/document/entities/lc_offsetcurve.cpp), [edit snapshot](../librecad/src/ui/components/utils/lc_offsetcurveeditsnapshot.cpp) | [native edits](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_edit_tests.cpp), [dialog](../librecad/src/ui/dialogs/entity/tests/lc_offsetcurve_propertiesdialog_tests.cpp) | [D-edit](plan_offset_curve_2961.md#d-edit-validation-2026-10-23) `4300c1031`; [H-dialog](plan_offset_curve_2961.md#h-dialog-validation-2026-10-23) `22f9e5609`. |
| R04 Clone / transforms / owners | [lc_offsetcurvetransform.cpp](../librecad/src/lib/engine/document/entities/lc_offsetcurvetransform.cpp), [rs_insert.cpp](../librecad/src/lib/engine/document/entities/rs_insert.cpp) | [transforms](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurvetransform_tests.cpp), [copy semantics](../librecad/src/lib/engine/document/container/tests/lc_copy_semantics_tests.cpp) | [D-core](plan_offset_curve_2961.md#d-core-validation-2026-10-23) `92758859c`; [E-tools](plan_offset_curve_2961.md#e-tools-validation-2026-10-23) `a37982bd6`. |
| R05 Create / repeat / kept plain | [rs_creation.cpp](../librecad/src/lib/creation/rs_creation.cpp), [lc_offsetcurveagain.cpp](../librecad/src/lib/engine/document/entities/lc_offsetcurveagain.cpp), [Offset action](../librecad/src/actions/drawing/modify/lc_action_modify_offset.cpp) | [dispatch](../librecad/src/lib/creation/tests/lc_offsetcurve_action_dispatch_tests.cpp), [repeat](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurveagain_tests.cpp), [option/messages](../librecad/src/actions/drawing/modify/tests/lc_offsetcurve_enable_tests.cpp) | [E-tools](plan_offset_curve_2961.md#e-tools-validation-2026-10-23) `a37982bd6`; [E-again](plan_offset_curve_2961.md#e-again-validation-2026-10-23) `bf7b512cf`; [H-enable](plan_offset_curve_2961.md#h-enable-validation-2026-10-24) `3d7195631`. |
| R06 Type dispatch / names | [rs.h](../librecad/src/lib/engine/rs.h), [descriptor registry](../librecad/src/lib/selection/metaentity/lc_entitymatchdescriptorsregistry.cpp) | [types/actions](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_types_tests.cpp), [names](../librecad/src/ui/components/utils/tests/lc_offsetcurve_names_tests.cpp) | [F-types](plan_offset_curve_2961.md#f-types-validation-2026-10-23) `49c9b328b`; [F-names](plan_offset_curve_2961.md#f-names-validation-2026-10-23) `f0fb28fab`; later Trim transitions: [T-actions](plan_offset_curve_2961.md#t-actions-validation-2026-10-24) `49bb7755f`. |
| R07 DXF/DWG read AND write | [rs_filterdxfrw.cpp](../librecad/src/lib/filters/rs_filterdxfrw.cpp) | [format matrix](../librecad/src/lib/filters/tests/lc_offsetcurve_matrix_tests.cpp), [trim files](../librecad/src/lib/filters/tests/lc_offsetcachetrim_file_tests.cpp) | [G-matrix](plan_offset_curve_2961.md#g-matrix-validation-2026-10-23) `79ae1930b`; [T-files](plan_offset_curve_2961.md#t-files-validation-2026-10-24) `905158174`. Preserve primary/auxiliary reader limits; final GUI is separately build-scoped. |
| R08 Schema / safe metadata | [lc_offsetcurvexdata.cpp](../librecad/src/lib/filters/lc_offsetcurvexdata.cpp), [native storage](../librecad/src/lib/engine/document/entities/lc_offsetcurve.cpp) | [schema](../tests/lc_offsetcurvexdata_tests.cpp), [native entity](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_tests.cpp) | [G-schema](plan_offset_curve_2961.md#g-schema-validation-2026-10-23) `305042903`; wiring/packetization: [G-adapter](plan_offset_curve_2961.md#g-adapter-validation-2026-10-23) `32597762b`. |
| R09 Selection / native UI / grips | [property provider](../librecad/src/ui/dock_widgets/property_sheet/metaentity/entities/lc_propertiesprovider_offsetcurve.cpp), [dialog editor](../librecad/src/ui/dialogs/entity/lc_propertieseditingwidget_offsetcurve.cpp), [Default action](../librecad/src/actions/drawing/rs_actiondefault.cpp) | [sheet](../librecad/src/ui/dock_widgets/property_sheet/tests/lc_offsetcurve_properties_tests.cpp), [dialog](../librecad/src/ui/dialogs/entity/tests/lc_offsetcurve_propertiesdialog_tests.cpp), [mouse/grips](../librecad/src/actions/drawing/tests/lc_offsetcurve_drag_tests.cpp) | [H-sheet](plan_offset_curve_2961.md#h-sheet-validation-2026-10-23) `678590af4`; [H-dialog](plan_offset_curve_2961.md#h-dialog-validation-2026-10-23) `22f9e5609`; [H-drag](plan_offset_curve_2961.md#h-drag-validation-2026-10-23) `f214b3408`. Automated cases do not establish every physical UI variant. |
| R10 Update / Restore / entry points | [Update](../librecad/src/actions/drawing/modify/lc_action_offsetcurve_updatesource.cpp), [Restore](../librecad/src/actions/drawing/modify/lc_action_offsetcurve_restoreoriginal.cpp), [source UI](../librecad/src/ui/components/utils/lc_offsetcurveui.cpp) | [Update](../librecad/src/actions/drawing/modify/tests/lc_action_offsetcurve_source_tests.cpp), [Restore](../librecad/src/actions/drawing/modify/tests/lc_action_offsetcurve_restoreoriginal_tests.cpp), [source links](../librecad/src/ui/components/utils/tests/lc_offsetcurve_source_ui_tests.cpp) | [I-update](plan_offset_curve_2961.md#i-update-validation-2026-10-24) `379729bb3`; [I-restore](plan_offset_curve_2961.md#i-restore-validation-2026-10-24) `0d3fed653`; [I-ui](plan_offset_curve_2961.md#i-ui-validation-2026-10-24) `4a231d243`. |
| R11 Build / regression / portability | [CMakeLists.txt](../CMakeLists.txt), [src.pro](../librecad/src/src.pro), [file dialog service](../librecad/src/ui/dialogs/file/export/layers/lc_filedialogservice.cpp) | [registered Trim actions](../librecad/src/actions/drawing/modify/tests/lc_offsetcachetrim_action_tests.cpp), [Save As filters](../librecad/src/ui/main/persistence/tests/lc_dwg_save_filter_tests.cpp) | [T-actions](plan_offset_curve_2961.md#t-actions-validation-2026-10-24) `49bb7755f`; [T-files](plan_offset_curve_2961.md#t-files-validation-2026-10-24) `905158174`; [Save As](plan_offset_curve_2961.md#z-save-filters-validation-2026-10-24) `091de1a6a`. Host CMake/qmake evidence is not all-platform CI. |
| R12 Bounded work / UI responsiveness | [geometry budgets](../librecad/src/lib/engine/document/entities/lc_ellipseoffsetgeometry.cpp), [stored-cache trim](../librecad/src/lib/modification/lc_offsetcachetrim.cpp) | [topology budgets](../librecad/src/lib/engine/document/entities/tests/lc_offsetcurve_topology_budget_tests.cpp), [trim kernel](../librecad/src/lib/modification/tests/lc_offsetcachetrim_tests.cpp), [isolated failures](../librecad/src/lib/modification/tests/lc_offsetcachetrim_failure_tests.cpp) | [A-core](plan_offset_curve_2961.md#a-core-validation-2026-10-22) `0eb2a0cfa`; [T-kernel](plan_offset_curve_2961.md#t-kernel-validation-2026-10-24) `04c712069`; later numeric delta: [T-files](plan_offset_curve_2961.md#t-files-validation-2026-10-24) `905158174`. |
| R13 Durable product / checkpoint | [Trim action example](../librecad/src/actions/drawing/modify/lc_action_modify_trim.cpp) | [actual action TU](../librecad/src/actions/drawing/modify/tests/lc_offsetcachetrim_action_tests.cpp) | [Commit Ledger](plan_offset_curve_2961.md#commit-ledger), [T-actions](plan_offset_curve_2961.md#t-actions-validation-2026-10-24) `49bb7755f`, [T-files](plan_offset_curve_2961.md#t-files-validation-2026-10-24) `905158174` / plan `a1ea520d6`; [Save As](plan_offset_curve_2961.md#z-save-filters-validation-2026-10-24) `091de1a6a` / plan `861b58957`. |
| R14 Original raw carrier / no repair | [lc_offsetcurvecarrier.cpp](../librecad/src/lib/filters/lc_offsetcurvecarrier.cpp), [admission boundary](../librecad/src/lib/filters/rs_filterdxfrw.cpp) | [carrier](../tests/lc_offsetcurvecarrier_tests.cpp), [actual adapter](../librecad/src/lib/filters/tests/lc_offsetcurve_adapter_tests.cpp) | [G-schema](plan_offset_curve_2961.md#g-schema-validation-2026-10-23) `305042903`; [G-adapter](plan_offset_curve_2961.md#g-adapter-validation-2026-10-23) `32597762b`. Raw helper success alone is not C certification. |
| R15 Failed-save target / document atomics | [export boundary](../librecad/src/lib/filters/rs_filterdxfrw.cpp) | [hostile files](../librecad/src/lib/filters/tests/lc_offsetcurve_hostilefile_tests.cpp), [persistence/refusals](../librecad/src/lib/filters/tests/lc_offsetcurve_persistence_tests.cpp) | [G-fixtures](plan_offset_curve_2961.md#g-fixtures-validation-2026-10-23) `7e46b2de8`; legacy refusal scope: [T-files](plan_offset_curve_2961.md#t-files-validation-2026-10-24) `905158174`. Stream injection is not physical disk exhaustion. |

## Final Host Qualification

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

Exact-name audit found A14+B8+C54 bases once/unskipped in both final main XMLs.
Each also contains 43 unskipped mandatory SECTION detail rows, not 43 extra bases.
The report audit preserves predecessor membership, multiplicity and skip states;
no test retirement or rename allowance was used. Membership is not continuous
geometry or binary/linkage proof; ordinary execution, freeze and blob records
provide their separate evidence. C27 remains a routing-kernel test.

[Actual GUI observations](offset_curve_interactive_smoke.md#recorded-checks-2026-10-24)
retain exact historical/final application and output identities. Final actual
CMake/qmake DWG2018 and DXF Save/Open preserve native/restored-ellipse/plain-spline
roles. Second DWG generation and real Trim/Undo-before-save native checks are
specifically CMake observations. Keyboard navigation was restored OFF; effective
Editable ON/original Offset options were preserved. Approved normal-profile
recents/paths/filters/window writes are not exact profile rollback or isolation.

## Retained Evidence

Raw artifacts remain under `/private/tmp/lc2961-impl-evidence.9GtLO2`; do not infer
their contents from filenames alone. These committed hashes identify the actual
receipts used here. No temporary evidence tree was deleted or rewritten for a
retry. Full commands, exits, compact logs and native XML remain separate from this
committed summary; publication of raw logs is not claimed.

| Artifact (relative to evidence root) | SHA256 |
| --- | --- |
| `Z-save-filters-test-invocations-01/run-manifest.json` | `1c9d8b3f9f643e51a2d1cf384c19da181dc0a539b44d82cfe5bf369f2e24cfba` |
| `Z-save-filters-report-audit-01.json` | `95b7d37257a997e9d7d0d85a48d712ce2cebf92578af2a606b818b73ed85cbbf` |
| `Z-save-filters-committed-blob-audit-01.json` | `05df0b99fefcc7364908b542271c6450c3e4582736f11ed972b7558a8fbe1d19` |
| `Z-final-mandatory-geometry-01.json` | `b4156ec0e338a933a12c732e566bb4e4b9f2264289fd6e3f07365829f78c781b` |
| `Z-retained-G-matrix-01.json` | `5d21c991da46a3b5a27ef4b9cd984682906d5cc76b45657641b45576e94589aa` |
| `Z-physical-observations-01.md` | `2a093c70c1b12298703a08065a9204f27d16354cd34be9f96d2b1789d99d4ea2` |

## Limits

- Host: macOS 27.0.1 arm64, Qt 6.12, AppleClang 21, Catch2 3.16.1. GCC/MSVC and
  Linux/Windows CI were unavailable; full main-suite ASan was not run. ASan used
  `detect_leaks=0` because host leak detection is unsupported. Existing compiler/
  linker warnings remain, not warning-clean or leak-free qualification.
- Static analysis attempted 29 real compile-database TUs, with 20 without fatal
  preprocessing/parser failures and nine Boost-blocked. The final Trim follow-up
  retained diagnostics/exit 2. Newer T-files endpoint/CMC and Save As bytes were not
  analyzer-qualified. Independent source reviews and runtime checks are separate.
- Retained G qualifies 128 mandatory outputs plus 48 auxiliary observations, not
  whole-CAD conformance. The 24 ODA-converted auxiliary duplicate-Continuous
  strict-reader diagnostics remain. No fresh external CAD execution was inferred
  from byte rechecks. Genuine R13/R14 lanes remain read-only, with qualified
  atomic refusal for unsupported unrelated foreign-object DWG migration.
- X-H7 completed two actual qualifications for the archived observed candidate;
  later bounded API bytes do not inherit its exact runtime measurement. No future
  timing guarantee or automatic H7 rerun is implied.
- A rounded exact-top trim query can conservatively reach its unchanged depth
  limit and refuse without mutation; nearby/left picks succeed. There was no
  tolerance/cap increase or snapping workaround.
- Physical context-menu, compact-widget, held-drag Escape/away-back, locked-peer
  and complete manual-catalogue variants remain unobserved where the smoke record
  says so. Automated actual-Qt coverage is not relabeled physical observation.
