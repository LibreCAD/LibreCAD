libdxfrw
==========

libdxfrw is a free C++ library to read and write DXF files in both formats, ascii and binary form.
It also has DWG read/write support maintained in LibreCAD's in-tree fork; see
`DWG_REFERENCE_COVERAGE_STATUS.md` and `DWG_ROADMAP.md` for the current DWG
coverage notes.
It is licensed under the terms of the GNU General Public License version 2 (or at your option
any later version).


libdxfrw was created by [LibreCAD](https://github.com/LibreCAD/LibreCAD) contributors in the process of making LibreCAD.
As the original code at [SourceForge](https://sourceforge.net/projects/libdxfrw) was no longer supported by the original authors, this repo has become its successor.

If you are looking for historical information about the project, it's still there:
http://sourceforge.net/projects/libdxfrw


Please note:
----------
This copy is vendored by LibreCAD. The top-level LibreCAD CMake build is the
primary build definition used by the in-tree tests; standalone CMake and qmake
metadata may lag until the source-list reconciliation work lands.

Building and installing the library
==========

Debug version
----------

```
mkdir build
cd build
cmake ..
make 
sudo make install
```

Non-debug version
----------

```
mkdir release
cd release
cmake -DCMAKE_BUILD_TYPE=Release ..
make 
sudo make install
```

Ubuntu/Mint Folks
----------

```
mkdir release
cd release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr .. && make all
make 
sudo make install
```


Example usage of the library
==========

See LibreCAD's DXF/DWG filter integration in `librecad/src/lib/filters/`.
