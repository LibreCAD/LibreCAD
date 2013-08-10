About LibreCAD
==============

LibreCAD is a 2D CAD drawing tool based on the community edition of QCAD (www.qcad.org).
LibreCAD has been re-structured and ported to qt4 and works natively cross platform between OSX, Windows and Linux.
See http://www.librecad.org

User Manual and wiki
------------------

We are in the process of building a user manual and wiki:

http://wiki.librecad.org/index.php/Main_Page

UNIX and OSX users
------------------

Unzip or checkout a version of LibreCAD into a directory.
CD into that directory and follow these instructions:

Build makefile and compile LibreCAD

```
qmake librecad.pro
make
```

After successful compiling, the executible is generated:

Linux: unix/librecad
OS/X: LibreCAD.app/Contents/MacOS/LibreCAD

A sample building script for OS/X is included as scripts/build-osx.sh. This script file also generates a LibreCAD.dmg.

Ubuntu/Debian users
-------------------

Make sure you have the qt-4 SDK installed
Install the qt4 SDK by executing the following commands:

```
$ sudo apt-get install g++ gcc make git-core libqt4-dev qt4-qmake \
libqt4-help qt4-dev-tools libboost-all-dev libmuparser-dev libfreetype6-dev
```

Alternatively, you make sure you have deb-src lines enabled in your sources.list file, and run,

```
$ sudo apt-get build-dep librecad
```

For SVN see also: 
http://www.librecad.org/2010/10/debian-64-bit-and-ubuntu-compile-how-to/

For git see also:
http://librecad.org/cms/home/from-source/linux.html

NOTE 1: On systems like fedora (& Ubuntu??) You might need to run qmake-qt4 instead of just qmake

Windows Users
-------------

Building steps are also given at our wiki page:

http://wiki.librecad.org/index.php/LibreCAD_Installation_from_Source

A sample build batch file is included as scripts/build-windows.bat. If successful, this building script generates a Windows installer file using NSIS(http://nsis.sourceforge.net/Main_Page). 

- Download a copy of Qt SDK,  4.8.4 for example from http://qt-project.org/downloads 

- Download boost, from https://sourceforge.net/projects/boost/files/boost/
- unzip into C:\boost\, for example C:\boost\1_53_0 (in this directory you will find boost root directory, INSTALL, index, Jamroot etc.. etc).

- Download muParser 2.2.2 or later from http://sourceforge.net/projects/muparser/files/muparser/
- Create a directory named "muparser" in `C:\`
- Unzip muparser_v2_2_2.zip into `C:\muparser\`

Notes: At this point you will have the following directory structure: C:\muparser\muparser_v2_2_2\ (assuming you are using muparser-2.2.2). If you prefer to keep muParser in other locations, you should specify the directiory location with a custom.pro file in LibreCAD source folder, for example, the following setting is equivalent to the default muparser path in common.pro:

`MUPARSER_DIR = /muparser/muparser_v2_2_2`

- Start Qt Desktop using "Qt 4.8.4 for Desktop (MinGW)" shortcut.
- In Qt Desktop console, navigate to muParser build directory (C:\muparser\muparser_v2_2_2\build\), then type the following command to built muParser library:
  `mingw32-make -fmakefile.mingw`

After installation, start Qt Creator and load LibreCAD.pro,
from the build menu select "Build All".

OSX USERS
---------

install macports from http://www.macports.org/

After that install QT and a new gcc, which should version 4.4 or later.

Install a version of Qt, boost and muparser, for example
`$ sudo port install gcc46 qt4-creator-mac qt4-mac boost muparser`

Select the right compiler, as LibreCAD doesn't build with the default llvm-gcc42,
`$ sudo port select --set gcc mp-gcc46`

Build a makefile in the LibreCAD source folder,
`$ qmake librecad.pro -r -spec mkspec/macports`

If the previous step is successful, you can build LibreCAD by issuing,
`$ make -j4'

After a successful build, the generated executible of LibreCAD can be found as LibreCAD.app/Contents/MacOS/LibreCAD.

Alternatively, you may try the building script comes with LibreCAD at scripts/build-osx.sh
`$ cd scripts/`
`$ ./build-osx.sh`



