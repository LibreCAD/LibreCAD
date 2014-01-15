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
# lang/gcc4{7,8,9}
# or
# lang/clang33 and devel/llvm33 and devel/libc++

scriptpath="$( readlink -f "${0}" )"
scriptpath="${scriptpath%/*}"

if [ -z "${use_cxx}" ]
then
	if [ "$( which g++47 )" ]
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
		echo "No supported compiler found. Install one of lang/{gcc4{7,8,9},clang33}" >&2
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
libs=

compiler_version=${use_cxx##*++}
case "${use_cxx}" in
	'g++'*)
		rpath="$( make -C /usr/ports/lang/gcc${compiler_version} -V LOCALBASE )"/lib/gcc${compiler_version}
	;;
	'clang++'*)
		if [ ! -e "$( make -C /usr/ports/lang/clang${compiler_version} -V LOCALBASE )"/lib/libc++.so ]
		then
			echo "Install devel/libc++" >&2
			exit 1
		fi
		cxxflags="-I /usr/local/include/c++/v1"
		libs="-stdlib=libc++"
	;;
esac
spec="freebsd-${use_cxx}"

cd "${scriptpath}/.."
qmake-qt4 librecad.pro ${spec:+-spec ${spec}} "${rpath:+QMAKE_RPATHDIR=\"${rpath}\"}" "${cxxflags:+QMAKE_CXXFLAGS=\"${cxxflags}\"}" "${libs:+QMAKE_LIBS=\"${libs}\"}"
make -j$( /sbin/sysctl -n hw.ncpu )
