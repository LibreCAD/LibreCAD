#!/bin/sh

# Copyright abandoned 2013 Tamas TEVESZ <ice@extreme.hu>
# 
# this script builds LibreCAD on FreeBSD
# The following ports must be installed:
# 
# x11-toolkits/qt4-gui devel/qt4-linguist devel/qt4-help-tools
# graphics/qt4-svg databases/qt4-sql(?) textproc/qt4-clucene(?)
# devel/boost-libs math/muparser
# 
# lang/gcc46 or lang/gcc47 or lang/gcc48 or lang/gcc49 or
# lang/clang33 and devel/llvm33 and devel/libc++

scriptpath="$( readlink -f "${0}" )"
scriptpath="${scriptpath%/*}"

if [ -z "${use_cxx}" ]
then
	if [ "$( which g++46 )" ]
	then
		use_cxx="g++46"
	elif [ "$( which g++47 )" ]
	then
		use_cxx="g++47"
	elif [ "$( which g++48 )" ]
	then
		use_cxx="g++48"
	elif [ "$( which g++49 )" ]
	then
		use_cxx="g++49"
	elif [ "$( which clang++33 )" ]
	then
		use_cxx="clang++33"
	else
		echo "No supported compiler found. Install one of lang/{gcc4{6,7,8},clang33}" >&2
		exit 1
	fi
elif [ -z "$( which ${use_cxx} )" ]
then
	echo "Selected compiler ${use_cxx} not found" >&2
	exit 1
fi

rpath=
spec=
cxxflags=

case "${use_cxx}" in
	'g++'*)
		use_gcc=${use_cxx#'g++'}
		spec="freebsd-g++${use_gcc}"
		if [ "${use_gcc}" = "49" ]
		then
			spec="mkspec/${spec}"
		fi
		rpath="$( make -C /usr/ports/lang/gcc${use_gcc} -V LOCALBASE )"/lib/gcc${use_gcc}
	;;
	'clang++33')
		if [ ! -e "$( make -C /usr/ports/lang/clang33 -V LOCALBASE )"/lib/libc++.so ]
		then
			echo "Install devel/libc++" >&2
			exit 1
		fi
		spec='mkspec/freebsd-clang33'
		cxxflags='-stdlib=libc++'
	;;
esac

cd "${scriptpath}/.."
qmake-qt4 librecad.pro -spec ${spec} ${rpath:+QMAKE_RPATHDIR="${rpath}"} ${cxxflags:+QMAKE_CXXFLAGS="${cxxflags}"}
make -j$( sysctl -n hw.ncpu )

