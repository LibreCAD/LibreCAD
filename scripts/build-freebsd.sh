#!/bin/sh

## copyright(C), 2013 Tamas TAVESZ(ice at down.royalcomp.hu), 2013
# 
# this script builds LibreCAD on FreeBSD
# The following ports must be installed:
# 
# x11-toolkits/qt4-gui devel/qt4-linguist devel/qt4-help-tools
# graphics/qt4-svg databases/qt4-sql(?) textproc/qt4-clucene(?)
# devel/boost-libs math/muparser
# 
# and one of
# lang/gcc46 lang/gcc47 lang/gcc48

SCRIPTPATH="$( dirname "$0" )"

if [ -z "${cver}" ]
then
	if [ "$( which g++46 )" ]
	then
		cver=46
	elif [ "$( which 47 )" ]
	then
		cver=47
	elif [ "$( which 48 )" ]
	then
		cver=48
	else
		# XXX: investigate clang*
		echo "No supported cver found. Install one of lang/gcc4{6,7,8}" >&1
		exit 1
	fi
fi

# Hackety hack to find the correct libstdc++
# XXX: Wonder how other apps do this...
rpath="$( g++${cver} -print-search-dirs | sed -n '/^libraries: /{s!^libraries: !!;s![:=]\(/.[^:=]*\).*!\1!;s!/gcc/.*!!;p;q;}' )"

cd "${SCRIPTPATH}"/..
qmake-qt4 librecad.pro -r -spec freebsd-g++${cver} QMAKE_RPATHDIR="${rpath}"
make -j$( sysctl -n hw.ncpu )

