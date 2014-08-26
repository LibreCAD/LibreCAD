#!/bin/sh

# Copyright abandoned 2013 Tamas TEVESZ <ice@extreme.hu>
# 
# this script builds LibreCAD on FreeBSD
# The following ports must be installed:
# 
# x11-toolkits/qt4-gui devel/qt4-linguist devel/qt4-help-tools
# graphics/qt4-svg databases/qt4-sql(?) textproc/qt4-clucene(?)
# devel/binutils devel/boost-libs math/muparser
# 
# lang/gcc{4{7,8,9},5}
# or
# lang/clang3{3,4} and devel/llvm3{3,4} and devel/libc++

scriptpath="$( readlink -f "${0}" )"
scriptpath="${scriptpath%/*}"

if [ -z "${use_cxx}" ]
then
	if [ "$( which g++5 )" ]
	then
		use_cxx="g++5"
	elif [ "$( which g++49 )" ]
	then
		use_cxx="g++49"
	elif [ "$( which g++48 )" ]
	then
		use_cxx="g++48"
	elif [ "$( which g++47 )" ]
	then
		use_cxx="g++47"
	elif [ "$( which clang++34 )" ]
	then
		use_cxx="clang++34"
	elif [ "$( which clang++33 )" ]
	then
		use_cxx="clang++33"
	else
		echo "No supported compiler found. Install one of lang/{gcc{4{7,8,9},5},clang3{3,4}}" >&2
		exit 1
	fi
elif [ -z "$( which ${use_cxx} )" ]
then
	echo "Selected compiler ${use_cxx} not found" >&2
	exit 1
fi

rpath=
cxxflags=
libs=
compiler=
extra=
local_base="$( make -f /usr/share/mk/bsd.port.mk -V LOCALBASE )"

compiler_version=${use_cxx##*++}
case "${use_cxx}" in
	'g++'*)
		rpath="${local_base}/lib/gcc${compiler_version}"
		compiler="g++"
	;;
	'clang++'*)
		if [ ! -e "${local_base}/lib/libc++.so" ]
		then
			echo "Install devel/libc++" >&2
			exit 1
		fi
		cxxflags="-I${local_base}/include/c++/v1"
		libs="-stdlib=libc++"
		compiler="clang"
		extra="build_muparser=true"
	;;
esac

cd "${scriptpath}/.."

qmake-qt4 librecad.pro \
	${compiler:+QMAKE_COMPILER=${compiler}} \
	QMAKE_CXX=${use_cxx} \
	QMAKE_LINK=${use_cxx} \
	${rpath:+QMAKE_RPATHDIR=\"${rpath}\"} \
	${cxxflags:+QMAKE_CXXFLAGS=\"${cxxflags}\"} \
	${libs:+QMAKE_LIBS=\"${libs}\"} ${extra:+${extra}}

make -j$( /sbin/sysctl -n hw.ncpu )
