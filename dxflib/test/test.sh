#!/bin/sh

for file in `find . -follow -name "*.dxf"`
do
	./test $file
	echo
	echo
done

