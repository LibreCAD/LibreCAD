# LibreCAD [![Build Status](https://travis-ci.org/LibreCAD/LibreCAD.svg?branch=master)](https://travis-ci.org/LibreCAD/LibreCAD) 

[→ **Download** ←](https://github.com/LibreCAD/LibreCAD/wiki/Download)

[LibreCAD](https://www.librecad.org) is a 2D CAD drawing tool
based on the community edition of [QCAD](https://www.qcad.org).
LibreCAD uses the cross-platform framework [Qt](https://www.qt.io/download-open-source/),
which means it works with most operating systems.
The user interface is translated in over 30 languages.  https://translate.librecad.org

LibreCAD is free software; you can redistribute it and/or modify  
it under the terms of the [GNU General Public License version 2](https://www.gnu.org/licenses/gpl-2.0.html) (GPLv2)  
as published by the Free Software Foundation.  
Please read the [LICENSE](LICENSE) file for additional information.

The master branch represents the latest pre-release code.
The 2.2.2 branch requires Qt 6.4.0 or newer.
The 2.2.1 branch requires Qt 5.15.0 or newer.
The 2.2 branch requires Qt 5.2.1 or newer.
The 2.1 branch will be the last to support Qt4.  
The 2.0 branch will be the last to support the QCAD toolbar.
![Build Status](https://travis-ci.org/LibreCAD/LibreCAD.svg?branch=2.0)](https://travis-ci.org/LibreCAD/LibreCAD) 

## DXF Converter

LibreCAD can be used as DXF to a PDF, PNG, or SVG converter. For example, to convert a `foo.dxf` to `foo.pdf`, `foo.png`, or `foo.svg`:

```bash
$ librecad dxf2pdf foo.dxf
$ librecad dxf2png foo.dxf
$ librecad dxf2svg foo.dxf
```

## Releases and Milestones

- [Releases and Prereleases](https://github.com/LibreCAD/LibreCAD/releases)
- [Milestones](https://github.com/LibreCAD/LibreCAD/milestones)

  For macOS arm64 builds, the app is __NOT__ signed. To workaround the "damaged" error ([#2162](https://github.com/LibreCAD/LibreCAD/issues/2162)):
  ```bash
  xattr -rc LibreCAD.app
  sudo codesign --force --deep --sign - LibreCAD.app
  ```


## Built with libdxfrw
[`libdxfrw`](https://github.com/LibreCAD/libdxfrw) is an associated project that allows LibreCAD to read DXF and DWG files.


## Requests and Bug Reports

- [GitHub Issues (preferred)](https://github.com/LibreCAD/LibreCAD/issues)
- [SourceForge tickets (disabled)](https://sourceforge.net/p/librecad/_list/tickets?source=navbar)


## Users Documentation

- [Users Manual](https://librecad.readthedocs.io/)
- [Wiki Main Page](https://dokuwiki.librecad.org/)


## Questions or Comments

- [LibreCAD's Forum](https://forum.librecad.org/)
- IRC: [#librecad](https://web.libera.chat/#librecad) at libera.chat


## Building

### Requirements

- [Qt](https://www.qt.io/download-open-source/) 6.4.0+ (MinGW version on Windows)
- [Boost](https://www.boost.org/) 1.55.0+

More information: [Build from source](https://github.com/LibreCAD/LibreCAD/wiki/Build-from-source)

### Building Unit Tests

To build unit tests (e.g., for `rs_math.cpp`), enable the `BUILD_TESTS` flag:

```bash
cmake -DBUILD_TESTS=ON ..
make
./build/librecad_tests
```


## Contributing

[Git and GitHub](https://github.com/LibreCAD/LibreCAD/wiki/Git-and-GitHub)

[Becoming a developer](https://github.com/LibreCAD/LibreCAD/wiki/Becoming-a-developer)

There is a [resources repository](https://github.com/LibreCAD/Resources) for people that want to indirectly  
contribute to the project by supplying icons, stylesheets, documentation, templates...

Associated downloads: <https://sourceforge.net/projects/librecad/files/Resources/>
