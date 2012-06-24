About LibreCAD
==============

LibreCAD is a 2D CAD drawing tool based on the community edition of QCAD (www.qcad.org).
LibreCAD has been re-structured and ported to qt4 and works natively cross platform between OSX, Windows and Linux.
See http://www.librecad.org

UNIX and OSX users
------------------

Unzip or checkout a version of LibreCAD into a directory.
CD into that directory and follow these instructions:

Build makefile and compile LibreCAD

```
qmake librecad.pro
make
```

Ubuntu/Debian users
-------------------

Make sure you have the qt-4 SDK installed
Install the qt4 SDK by executing the following commands:

```
$ sudo apt-get install g++ gcc make git-core libqt4-dev qt4-qmake \
libqt4-help qt4-dev-tools libboost-all-dev libmuparser-dev
```

Alternatively, you make sure you have deb-src lines enabled in your sources.list file, and run,

```
$ sudo apt-get build-dep librecad
```

For SVN see also: 
http://www.librecad.org/2010/10/debian-64-bit-and-ubuntu-compile-how-to/

For git see also:
http://librecad.org/cms/home/from-source/linux.html

NOTE 1: On systems (Ubuntu??) You might need to run qmake-qt4 instead of just qmake

Windows Users
-------------

- Download a copy of Qt SDK < 4.8.0, 4.7.4 for example from http://qt.nokia.com/downloads/ 

- Download boost 1.48 from https://sourceforge.net/projects/boost/files/boost/1.48.0/
- unzip into C:\boost\1_48_0 (in this directory you will find boost root directory, INSTALL, index, Jamroot etc.. etc).

- Download muParser 2.2.2 from http://sourceforge.net/projects/muparser/files/muparser/Version\ 2.2.2/muparser_v2_2_2.zip
- Create a directory named "muparser" in `C:\`
- Unzip muparser_v2_2_2.zip into `C:\muparser\`

Notes: At this point you will have the following directory structure: C:\muparser\muparser_v2_2_2\. If you prefer to keep muParser in other locations, you should specify the directiory location with a custom.pro file in LibreCAD source folder, for example, the following setting is equivalent to the default muparser path in common.pro:
`MUPARSER_DIR = /muparser/muparser_v2_2_2`

- Start Qt Desktop using "Qt 4.8.0 for Desktop (MinGW)" shortcut.
- In Qt Desktop console, navigate to muParser build directory (C:\muparser\muparser_v2_2_2\build\), then type the following command to built muParser library:
  `mingw32-make -fmakefile.mingw`

After installation, start Qt Creator and load LibreCAD.pro,
from the build menu select "Build All".

OSX USERS
---------

install macports from http://www.macports.org/

After that install QT and a new gcc, at mimimum you properly need gcc 44

Install a version of Qt < 4.8.0, 4.7.4 for example
`sudo port install gcc46 qt4-creator-mac qt4-mac boost`

When installed run to build a makefile
`qmake librecad.pro -r -spec mkspec/macports`
