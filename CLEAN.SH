#!/bin/sh

# Little script to clean out the generated and makefiles
rm -rf `find . -type f -name Makefile`
rm -rf `find . -type d -name generated`
rm -rf LibreCAD.app
rm -rf unix
rm -f core *.core vgcore.* core.*
rm -f *.dmg
