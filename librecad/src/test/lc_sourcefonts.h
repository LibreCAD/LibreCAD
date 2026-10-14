/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#ifndef LC_SOURCEFONTS_H
#define LC_SOURCEFONTS_H

#include <QString>

#include "rs_fontlist.h"
#include "rs_settings.h"
#include "rs_system.h"

namespace lc::test {

/**
 * The fonts of the source tree for the length of a scope, so that texts have
 * glyphs. Needs a QCoreApplication and the settings store, which
 * ensureSettings() of lc_filtertestsupport.h provides, and a target that
 * defines LIBRECAD_SOURCE_DIR.
 */
class SourceFonts {
public:
    SourceFonts() {
        static const bool systemReady = [] {
            RS_SYSTEM->init("LibreCAD", "tests", "librecad", "librecad_tests");
            return true;
        }();
        (void)systemReady;
        LC_GROUP_GUARD("Paths");
        m_previous = LC_GET_STR("Fonts", "");
        LC_SET("Fonts", QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/fonts"));
        RS_FONTLIST->clearFonts();
        RS_FONTLIST->init();
    }
    ~SourceFonts() {
        LC_GROUP_GUARD("Paths");
        LC_SET("Fonts", m_previous);
        RS_FONTLIST->clearFonts();
        RS_FONTLIST->init();
    }
    SourceFonts(const SourceFonts&) = delete;
    SourceFonts& operator=(const SourceFonts&) = delete;

private:
    QString m_previous;
};

} // namespace lc::test

#endif
