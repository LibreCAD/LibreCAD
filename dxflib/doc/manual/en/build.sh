#! /bin/sh

MANSTYLEHOME=/home/andrew/data/RibbonSoft/projects/QCad2/manstyle

echo ----------------------------------------------------------  doc
cd /home/andrew/data/RibbonSoft/projects/QCad2/dxflib/doc/manual/en/

$MANSTYLEHOME/manstyle index.xml html html >log_html

$MANSTYLEHOME/manstyle index.xml manual.ps ps >log_ps

#echo ----------------------------------------------------------  pdf
ps2pdf manual.ps
#echo ----------------------------------------------------------  ps book
#psbook "manual_reference.ps" | psnup -2 >manual_reference_book.ps
