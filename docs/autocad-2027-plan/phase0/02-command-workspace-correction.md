# Bottom command bar and quieter cursor

Date: October 11, 2026. The preceding engineering baseline was committed and pushed to `origin/master` as [`45c23fead4997456b2dd875eb402d3280ffe8cd3`](https://github.com/juanfelipecadavidherrera-create/librecad/commit/45c23fead4997456b2dd875eb402d3280ffe8cd3) before these changes. This correction is a subsequent local implementation; it does not implement the Home ribbon or CAD Familiar/Classic presets.

## Behavior

- The Command Line dock starts below the drawing instead of being a tab in the right palette stack. Its initial height targets 110 logical pixels and remains resizable.
- The Info Cursor overlay (coordinates, prompts and entity data following the pointer) defaults to disabled in the renderer, application actions and preferences dialog. Fixed command/coordinate controls remain available. The Info Cursor action can explicitly re-enable the feature.
- At the first startup with this correction, the restored legacy command dock is moved to the bottom, unfloated and made visible; a disabled bottom dock area is enabled. The cursor overlay is switched off even if an older version saved it as enabled.
- The migration marker `Startup/CommandWorkspaceDefaultsApplied` is saved after the corrected workspace is persisted. Later deliberate docking and cursor choices survive relaunch. Named saved workspaces remain available and may restore their original arrangements when explicitly activated.
- Dependent Info Cursor actions now reflect the enabled state immediately on settings load.

## Verification

Native arm64 build passed. The isolated offscreen dock suite passed **18 cases / 156 assertions**, including a four-window sequence: restore an old right-tabbed command bar with bottom area disabled and overlay enabled; migrate; restart; explicitly re-enable/rearrange and restart again. The native Cocoa capture passed **353 assertions**, reported overlay disabled and a visible Command Line in `Qt::BottomDockWidgetArea`, and its screenshot was inspected.

The migration/restart sequence also passed on native Cocoa (**25 assertions**). All test settings were private; these checks did not reset the user's actual preferences.

The updated app package passed dependency, arm64 architecture and ad hoc signature verification for all **67 Mach-O files**.

Local evidence is in `build/phase0-evidence/command-workspace-*`: build/install logs, dock test log, Cocoa capture log, runtime JSON and workspace screenshots. These are ignored generated artifacts. The updated development app is `build/macos-arm64-command-workspace/LibreCAD.app`; the earlier Phase 0 DMG remains the preserved baseline and does not contain this correction. The development build retains the baseline's macOS 26.5 minimum and ad hoc signing.

The remaining Phase 1 work is the top ribbon, quick access, curated Properties/Layers visibility and workspace presets/reset. DWG writing, real sheet rendering and PDF layout publishing remain in their later gates.
