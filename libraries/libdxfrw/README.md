libdxfrw ![Build status](https://api.travis-ci.org/LibreCAD/libdxfrw.svg?branch=master)
==========

libdxfrw is a free C++ library to read and write DXF files in both formats, ascii and binary form.
It also has rudimentary capabilities to read DWG files.
It is licensed under the terms of the GNU General Public License version 2 (or at you option
any later version).


libdxfrw was created by [LibreCAD](https://github.com/LibreCAD/LibreCAD) contributors in the process of making LibreCAD.
As the original code at [SourceForge](https://sourceforge.net/projects/libdxfrw) was no longer supported by the orignal authors, this repo has become its successor.

If you are looking for historical information about the project, it's still there:
http://sourceforge.net/projects/libdxfrw


Please note:
----------
When you clone or download this project to build [LibreCAD_3](https://github.com/LibreCAD/LibreCAD_3) use the branch **LibreCAD_3**. The master or other branches may have incompatible interface definitions which are not yet implemented in LibreCAD_3!

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

See how we use it in LibreCAD V3 : https://github.com/LibreCAD/LibreCAD_3/tree/master/persistence/libdxfrw
