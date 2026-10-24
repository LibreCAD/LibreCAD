# LibreCAD Privacy Policy

*Last updated: 23 October 2026*

LibreCAD is a free, open-source 2D CAD program made by volunteers. **It does
not collect, store, sell or share personal data.** It has no user accounts,
advertising, analytics or telemetry, and it never uploads your drawings.

This policy covers the LibreCAD program however you installed it: from the
Microsoft Store, from an installer or package published on GitHub, or built
from source.

## What stays on your computer

- **Drawings and exports** are read and written where you choose.
- **Recovery and backup copies.** While you work, LibreCAD regularly saves a
  recovery copy of the drawing you are editing, if it has unsaved changes:
  beside the drawing or, for a drawing that has never been saved, in the
  system's temporary folder. Saving the drawing deletes that copy; closing it
  without saving leaves the copy in place. When you save, LibreCAD also keeps
  the previous version as a backup beside the drawing. Both are hidden files
  unless you clear **Hide backup and autosave files**, and you can turn both
  off by clearing **Auto save backup each**, both under **Options >
  Application Preferences > Defaults**.
- **Settings**, such as preferences, recently opened files, workspaces, pen
  palettes and keyboard shortcuts, and thumbnails for the block library, are
  kept in your user profile: on Windows, in the registry and in LibreCAD's
  application data folder, or in a folder you choose under **Options >
  Application Preferences > Paths**.
- **Crash reports.** If LibreCAD crashes, it may write a technical report,
  such as a call stack, to a file on your computer. It is never sent anywhere.

Uninstalling the Microsoft Store version removes the settings it stored,
with two exceptions: changes it made to settings files that an earlier
LibreCAD installation had already created stay in that installation's files,
and settings files in a folder you chose outside your AppData folder stay
too. The Windows installer, the macOS disk image and the AppImage leave
settings in place for the next version, and you can delete them yourself.
Removing the snap package removes the settings in its own folder,
`~/snap/librecad`, but snapd first saves a snapshot of them and by default
keeps it for 31 days; `snap remove --purge librecad` skips the snapshot.
Settings files in a folder you chose outside it stay. Nobody else ever
receives a copy of any of this.

## When LibreCAD uses the network

- **Checking for new versions.** By default LibreCAD asks GitHub for the list
  of LibreCAD releases when it starts, so that it can tell you about a newer
  version; it also asks when you click **Check for New Version** in the About
  dialog (**Help > About**; on macOS, in the **LibreCAD** menu). The request
  is an anonymous HTTPS request to `api.github.com`. It contains nothing from
  your drawings and no account details. Like any web request it shows GitHub
  your IP address, and Qt, the library LibreCAD is built with, adds your
  system's locale, a language and region such as `de-CH`, to it. GitHub
  handles it under the
  [GitHub Privacy Statement](https://docs.github.com/site-policy/privacy-policies/github-general-privacy-statement).
  To turn the check off, clear **Options > Application Preferences > Startup >
  Check for new versions on startup**.
- **Links you open.** The Help menu opens web pages, such as the user manual,
  the wiki, the forum, the chat, the issue tracker and the donation page, in
  your web browser. Those sites have their own privacy policies.

LibreCAD makes no other network connections.

## Children

LibreCAD collects no data from anyone, including children.

## Changes

Changes to this policy are published in this file, and the Git history of the
[LibreCAD repository](https://github.com/LibreCAD/LibreCAD) shows each one.

## Contact

Questions about this policy are welcome on the
[issue tracker](https://github.com/LibreCAD/LibreCAD/issues) or the
[forum](https://forum.librecad.org/).
