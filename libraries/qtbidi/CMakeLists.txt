# Copyright (C) 2026 LibreCAD.org and Dongxu Li
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(lcqtbidi STATIC qt_bidi.cpp qt_bidi.h)
set_target_properties(lcqtbidi PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_compile_features(lcqtbidi PUBLIC cxx_std_17)
target_include_directories(lcqtbidi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lcqtbidi PUBLIC Qt6::Core)
