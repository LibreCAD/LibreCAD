# Copyright (C) 2026 LibreCAD.org and Dongxu Li
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(lcfribidi STATIC
    src/fribidi-bidi.c
    src/fribidi-run.c
    src/fribidi-brackets.c)
set_target_properties(lcfribidi PROPERTIES
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF C_STANDARD 99)
target_include_directories(lcfribidi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(lcfribidi PRIVATE
    FRIBIDI_LIB_STATIC DONT_HAVE_FRIBIDI_CONFIG_H
    HAVE_STDLIB_H=1 HAVE_STRING_H=1 HAVE_STRINGIZE=1)
if(MSVC)
    target_compile_options(lcfribidi PRIVATE /UDEBUG)
else()
    target_compile_options(lcfribidi PRIVATE -UDEBUG)
endif()
