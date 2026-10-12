# Phase 0: native Mac engineering baseline

Date: October 11, 2026. Starting revision: `628a1ba845931a37b8d745a8ad08d4731615812c`, `master`, version macro `2.2.2.6-alpha`. This engineering baseline is the source snapshot prepared for GitHub; generated applications, DMGs and local evidence remain ignored build output.

**The engineering baseline is complete.** A native arm64 application and development DMG were built and verified; existing tests ran; fixtures, a runtime/source inventory and the next workspace specification were added. Three delivery prerequisites were fixed. The ribbon/CAD Familiar preset has not been implemented. The complete Phase 0 product gate still needs real AutoCAD reference tasks, representative user drawings and interactive acceptance; the baseline below does not certify CAD compatibility.

## Changes delivered

| Change | Purpose |
| --- | --- |
| `macos-arm64-baseline` configure/build/test presets and `scripts/build-macos-arm64-baseline.sh` | Build the locked libraries using Apple Clang and the selected Apple SDK, outside Conda's older linker environment; keep build/install output separate |
| Preset CMake minimum corrected to 3.28 | Match the project's actual minimum |
| Mac icon-engine install destination moved inside the app's PlugIns folder | Keep LibreCAD's original `.lci` icons with the copied app |
| Packaging accepts `macdeployqt6` as well as `macdeployqt` | Support the repository's locked Qt package |
| Mac crash-handler destination moved to the per-user temporary directory | A crash previously created logs inside the sealed app bundle and invalidated its signature; the destination is prepared before signal handling |
| Packaging regression extended for a controlled crash and dylib-based Qt | Prove the crash log exists outside the app and signatures survive; retain tamper, architecture and external-dependency rejection checks |
| Opt-in isolated workspace capture | Open the generated drawing, verify five loaded entities, capture native Cocoa images and actual action/dock/shortcut data without exporting or changing the user's saved workspace |
| Fixture generator, hash manifest, source inventory, file/PDF probe runner and [workspace specification](01-workspace-specification.md) | Establish measurable input/output and UI contracts for subsequent phases |

The user’s installed `/Applications/LibreCAD.app` was not replaced. No new CAD command behavior, layout engine, proprietary artwork or paid SDK was added.

## Build and artifact evidence

| Item | Observed value |
| --- | --- |
| Host | arm64, macOS 27.0.1, 8 logical CPUs, 8 GiB memory |
| Compiler / SDK | Apple Clang 21.0.0; selected Xcode SDK macOS 26.5 |
| Libraries / tools | Checked-in `pixi.lock`; Qt 6.8.2; CMake 3.31.5; Pixi 0.81.0 native arm64 executable, SHA-256 checked against official release metadata |
| App | [Final development app](../../../build/macos-arm64-package-final/LibreCAD.app) |
| DMG | [LibreCAD Phase 0 arm64](../../../build/macos-arm64-package-final/LibreCAD-phase0-arm64.dmg), about 131 MiB |
| DMG SHA-256 | `b67e065d9866578564aecfac7ea462078ae7c0e8883b3017e865d882e7380d5e` |
| Native code | Package verification inspected **67 Mach-O files**; all included arm64 |
| Signing | Ad hoc; bundle and nested-code signatures passed; mounted DMG contents matched the finalized source app |
| Actual deployment floor | Main executable reports **macOS 26.5 minimum**, SDK 26.5. This is a host baseline, not a macOS 14-compatible release. Select and rebuild for the intended public minimum later, then test on that OS |
| Gatekeeper | Rejected, as expected for this ad-hoc development artifact; Developer ID signing/notarization remains a release step |

The dependency lock was preserved. Build dependencies live in `.pixi/`; Pixi and inspection tooling live under ignored `build/`. No global package-manager installation or shell startup-file change was needed. The baseline uses the host SDK; its record is repeatable build instructions and environment evidence, not a bit-for-bit reproducibility claim. Official Pixi documents standalone release installation: [installation documentation](https://pixi.prefix.dev/latest/installation/).

## Verification results

| Check | Result |
| --- | --- |
| Registered CTest integration/format lanes | **5/5 passed**, zero failures |
| Explicit core fast lane | **1,599 passed**, 4 skipped, **40,133 assertions passed** |
| Dock/workspace lane | **17 test cases, 131 assertions passed** |
| Native Cocoa capture | **1 test case, 351 assertions passed**, private settings active; five entities loaded, four selected (locked-layer line excluded) |
| Fixture generation | Two project-authored R2018 DXFs, independent ezdxf audit: zero errors/fixes; regeneration reproduced their hashes |
| Existing fixture manifest / hashes | Passed |
| Final package integrity / dependencies | Passed; all 67 Mach-O files verified |
| Package regression | Passed: crash report outside the bundle and signature preserved; path containing spaces; post-sign mutation rejected and stale DMG removed; missing architecture rejected; external dependency rejected |
| DMG mounted verification | Passed, source manifest matched |
| File/PDF process probes | **6/8 met their success/failure expectation; two DWG export probes failed**. The runner correctly exits nonzero for those failures |
| Syntax / document consistency | Preset JSON, Python and shell syntax, fixture hashes, Markdown fences and diff whitespace checked |

The four skipped core cases were not promoted to passes. Full application/corpus/external-reader gates were not run. The CTest target called `libdxfrw_dwg_layout_validation_tests` validates DWG binary section/page structures; its name does **not** mean paper-space layouts or sheet publishing passed.

## Actual CAD observations and follow-up backlog

| Observation | Evidence / implication | Next phase |
| --- | --- | --- |
| Existing DWG reading works for the retained `large_radial.dwg` AC1032 fixture | `dwg2dxf` reopened as one DIMENSION, independent DXF audit zero errors/fixes; `dwg2pdf` produced one page. This is a bounded fixture result, not general DWG support certification | Expand corpus and source-specific preservation diagnostics in format work |
| Retained DWG PDF fidelity is unverified | The rendered page was dominated by a large black shape. No independent reference plot is available; successful file creation does not establish a usable or accurate dimension plot | Phase 4 dimension/rendering comparison with a reference reader |
| Both generated DXFs opened/rendered and produced model PDFs | Each output had one A3 media box, approximately 420.158 × 297.039 mm. Model rendering was visually inspected; no searchable text was extracted | Preserve model printing while adding true sheets |
| Two-layout fixture still produced **one model page** | Target is A3 then A4, separate titles and viewport scales 1:1 and 1:2. Current output does not meet it | Phase 3 ownership/viewports, Phase 4 publishing |
| Both R2018 DXF → AC1032 DWG exports returned exit 1 | Console reported failed export and produced no DWG; round-trip comparisons could not run. Root cause was not isolated in this baseline | Dedicated writer/admission investigation; do not advertise successful save from these drawings |
| Invalid DWG input was rejected | Exit 1 and no output DXF | Retain file-safety behavior |
| `--monochrome` retained yellow text and a red line in the sample PDF | Visual inspection found source colors despite the flag; text remained outlined/non-searchable | Phase 4 plot-style/color/text fidelity checks |
| Imported drawing showed **Model, Model, Layout1** tabs | Existing tab bar synthesizes Model and also appends imported Model metadata without filtering | Fix model-tab identity before claiming functional layout switching |
| Mixed selection still showed Properties filter **No Selection** | Native capture recorded four selected entities; drawing/default fields remained visible | Phase 1/2 selection inspector default/filter behavior |
| First-run palette congestion persists | Runtime inventory found 327 actions and 25 docks; Properties initially hidden in the existing right-side stack | Phase 1 curated default and palette persistence |

Static inventory found **328 action entries (324 unique IDs)**, **47 default shortcut entries**, **11 document docks**, and **153 command entries**. These counts differ intentionally from runtime counts: static entries include conditional definitions, while runtime includes generated controls/CAD docks. See [source inventory](source-inventory.json) and [native runtime inventory](../../../build/phase0-evidence/ui-cocoa/runtime-workspace.json).

Native screenshots: [current workspace](../../../build/phase0-evidence/ui-cocoa/workspace-baseline.png), [Properties after mixed selection](../../../build/phase0-evidence/ui-cocoa/properties-baseline.png). They come from an isolated Cocoa test window with project fonts and the installed icon engine, not a manual drafting-session recording. Actual AutoCAD 2027 screenshots, trackpad/Retina interaction, keyboard workflow, property edits/undo/save/reopen, multiple-monitor behavior and large-drawing performance still need execution.

## Repeat the baseline

With native Pixi installed, initialize the Catch2 submodule, then install the checked-in lock and run the wrapper from a native Apple Silicon shell:

```sh
pixi install --locked
scripts/build-macos-arm64-baseline.sh
```

Qt's CPU feature query is blocked by this execution sandbox; native Qt builds/tests required the approved unsandboxed tool execution. Direct Apple toolchain selection also avoids the old Conda linker's failure with this SDK. Configure currently emits regional-translation localization warnings, and Boost emits deprecated literal-operator warnings; neither was silently treated as a compilation error.

Inspection tools are optional and separate from application dependencies:

```sh
python3 -m venv build/phase0-tools
build/phase0-tools/bin/python -m pip install -r scripts/cad-baseline-requirements.txt
XDG_CACHE_HOME="$PWD/build/phase0-cache" build/phase0-tools/bin/python scripts/make-cad-baseline-fixtures.py
python3 scripts/inventory-cad-workspace.py --output docs/autocad-2027-plan/phase0/source-inventory.json
```

Package a fresh **copy** of the installed app and then verify it:

```sh
mkdir -p build/macos-arm64-package-final
ditto build/macos-arm64-install/bin/LibreCAD.app build/macos-arm64-package-final/LibreCAD.app
scripts/package-osx-app.sh --app "$PWD/build/macos-arm64-package-final/LibreCAD.app" \
  --qt-bin "$PWD/.pixi/envs/default/bin" --mode adhoc --architectures arm64
scripts/test-osx-package.sh --app "$PWD/build/macos-arm64-package-final/LibreCAD.app" --architectures arm64
```

Use an empty output directory for a fresh probe run. Packaged Qt ships Cocoa rather than the offscreen test backend:

```sh
XDG_CACHE_HOME="$PWD/build/phase0-cache" build/phase0-tools/bin/python scripts/run-cad-baseline-smoke.py \
  --platform cocoa --executable build/macos-arm64-package-final/LibreCAD.app/Contents/MacOS/LibreCAD \
  --output build/phase0-evidence/new-smoke-run
```

To capture the isolated native workspace, use the built dock test and installed icon engine:

```sh
QT_PLUGIN_PATH="$PWD/build/macos-arm64-install/bin/LibreCAD.app/Contents/PlugIns" \
LIBRECAD_BASELINE_OUTPUT="$PWD/build/phase0-evidence/new-ui-capture" QT_QPA_PLATFORM=cocoa \
  build/macos-arm64-baseline/librecad_dock_layout_tests '[.baseline]' --reporter compact
```

Detailed local evidence is under [`build/phase0-evidence`](../../../build/phase0-evidence): `baseline-final.log`, `ui-capture-cocoa.log`, `smoke-final/results.json`, `package-final-verification.txt`, `package-final.manifest`, `package-final-regression.log`, and `dmg-verification.txt`. Build artifacts/logs are ignored, so these local evidence links will not exist in a fresh clone unless the baseline is repeated or evidence is archived separately.

## Next authorized checkpoint

The subsequent CAD Familiar ribbon/preset implementation is recorded separately in [Phase 1 results](../phase1/00-results.md). This document retains the historical Phase 0 evidence.

Subsequent user-requested correction: [bottom command bar and disabled cursor overlay](02-command-workspace-correction.md). The artifact and test results above describe the preserved Phase 0 baseline; the correction has its own updated development app and evidence.

Stop here for user review. The next implementation stage is **Phase 1: top Home ribbon and quick access, CAD Familiar/Classic arrangements, visible Properties/Layers, bottom Command Line and reliable reset/persistence**, using the existing actions. Its detailed contract is in [the workspace specification](01-workspace-specification.md). DWG export, real layout scenes and PDF sheet/color fidelity remain explicit later work, with the observed failures retained as acceptance cases.
