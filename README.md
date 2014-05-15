About LibreCAD
==============

LibreCAD is a 2D CAD drawing tool based on the community edition of QCAD (www.qcad.org).
LibreCAD has been re-structured and ported to Qt version 4 and works natively cross platform between OS X, Windows and Linux.
See http://www.librecad.org

User Manual and wiki
------------------

We are in the process of building a user manual and wiki:

http://wiki.librecad.org/index.php/Main_Page

OS X Users
----------

If you use macports, see below. If you use brew, or neither one, use this section.

Install Homebrew from http://brew.sh/.

```
gcc --version # you'll need gcc 4.4 or newer. If yours is older:
brew tap homebrew/versions
brew options gcc48
brew install [flags] gcc48
mkdir ~/bin
cd ~/bin
ln -s /usr/local/bin/gcc-4.8 gcc
ln -s /usr/local/bin/g++-4.8 g++
ln -s /usr/local/bin/gcc-ar-4.8 gcc-ar
ln -s /usr/local/bin/gcc-nm-4.8 gcc-nm
ln -s /usr/local/bin/gcc-ranlib-4.8 gcc-ranlib
source ~/.bashrc
gcc --version # make sure it's 4.8. if it's not, ~/bin might not be on your path

brew install boost qt

# Unzip or checkout a version of LibreCAD into a directory.
cd LibreCAD
./scripts/build-osx.sh
```

This creates an executable "LibreCAD.app/Contents/MacOS/LibreCAD" and package "LibreCAD.dmg".

OS X with MacPorts Users
-----------------------

install MacPorts from http://www.macports.org/

You can install LibreCad using MacPorts by:
`$ sudo port install librecad`

You can build LibreCAD manually by following steps:

Install QT and a new gcc, which should version 4.4 or later.

Install a version of Qt and boost, for example
`$ sudo port install gcc46 qt4-creator-mac qt4-mac boost`

Select the right compiler, as LibreCAD doesn't build with the default llvm-gcc42,
`$ sudo port select --set gcc mp-gcc46`

Unzip or checkout a version of LibreCAD into a directory.

```
cd LibreCAD
./scripts/build-osx.sh
```

This creates an executable "LibreCAD.app/Contents/MacOS/LibreCAD" and package "LibreCAD.dmg".

Users of Ubuntu/Debian and derivatives
--------------------------------------

Make sure you have the Qt version 4 development packages installed by
running the following commands:

```
$ sudo apt-get install g++ gcc make git-core libqt4-dev qt4-qmake libqt4-help \
qt4-dev-tools libboost-all-dev libmuparser-dev libfreetype6-dev pkg-config
```

Alternatively, you make sure you have deb-src lines enabled in your sources.list file, and run,

```
$ sudo apt-get build-dep librecad
```

For SVN see also: 
http://www.librecad.org/2010/10/debian-64-bit-and-ubuntu-compile-how-to/

For git see also:
http://librecad.org/cms/home/from-source/linux.html

Note that you will most likely need to run __qmake-qt4__ instead of just __qmake__.

Users of Red Hat and similar distibutions
-----------------------------------------

Install Qt, Boost and muParser development packages for your respective distribution;
EPEL(https://fedoraproject.org/wiki/EPEL) and similar repositories may come handy if
your base OS does not include the necessary packages.

As an example, for CentOS 6.4, after adding the EPEL repository,

```
yum groupinstall 'Desktop Platform Development' 'Development tools'
yum install qt-devel boost-devel muParser-devel
```

will install the necessary build dependencies.

Note that you will most likely need to run __qmake-qt4__ instead of just __qmake__.

FreeBSD users
-------------

See scripts/build-freebsd.sh for the list of ports that need to be installed.

Use the script itself to build LibreCAD.

Windows Users
-------------

Building steps are also given at our wiki page:

http://wiki.librecad.org/index.php/LibreCAD_Installation_from_Source

A sample build batch file is included as scripts/build-windows.bat. If successful, this building script generates a Windows installer file using NSIS(http://nsis.sourceforge.net/Main_Page). 

- Download a copy of Qt SDK,  4.8.4 for example from http://qt-project.org/downloads 

- Download boost, from https://sourceforge.net/projects/boost/files/boost/
- unzip into C:\boost\, for example C:\boost\1_53_0 (in this directory you will find boost root directory, INSTALL, index, Jamroot etc.. etc).

Start Qt Creator and load LibreCAD.pro, from the build menu select "Build All".

Generic Unix Users
------------------

Unzip or checkout a version of LibreCAD into a directory.
```
cd LibreCAD
qmake librecad.pro
make
```
The executible is generated at "unix/librecad"
