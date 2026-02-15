# CircleTools (LibreCAD plugin)

CircleTools is a LibreCAD 2.2.x plugin that adds batch operations for circles:

1. Find circles by diameter (with tolerance) and move matches to a chosen layer.
2. Change diameter of selected circles (rectangle selection is supported in the plugin workflow).
3. Pick a reference circle (diameter) and then change diameter of all matching circles.

## Logging

Logging is **enabled by default**. The plugin appends debug output to:

- `CircleTools.log` in the OS temporary directory (`QStandardPaths::TempLocation`).

## Build (qmake)

LibreCAD 2.2.x uses qmake for the plugin collection:

```bash
cd plugins
qmake
make
```

Or build only this plugin:

```bash
cd plugins/circletools
qmake
make
```

On Windows, open `plugins/plugins.pro` in Qt Creator and build.
