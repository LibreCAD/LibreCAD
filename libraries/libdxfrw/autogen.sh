#!/bin/sh

aclocal \
&& libtoolize -c \
&& automake --copy --gnu --add-missing -f\
&& autoconf
