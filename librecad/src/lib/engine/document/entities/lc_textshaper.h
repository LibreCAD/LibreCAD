/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_TEXTSHAPER_H
#define LC_TEXTSHAPER_H

#include "lc_opentypefont.h"
#include <QByteArray>
#include <QString>
#include <cstdint>
#include <vector>

/** Shapes one logical compatible run; bidi itemization/reordering is external. */
class LC_TextShaper final {
public:
    struct SourceRange {
        int start = 0;
        int length = 0;
    };
    struct Run {
        // The caller bounds this context at paragraph/level/isolate boundaries.
        // Font/size item boundaries can retain pre/post context inside this text.
        QString contextText;
        int itemStart = 0;
        int itemLength = -1; // -1 means the remaining contextText.
        int sourceOffset = 0; // Added to contextText UTF-16 indices.
        bool rightToLeft = true;
        QByteArray script = "Arab";
        QByteArray language = "ar";
        bool beginningOfText = true;
        bool endOfText = true;
    };
    struct Glyph {
        std::uint32_t id = 0;
        int cluster = 0; // UTF-16 index, including sourceOffset.
        SourceRange source;
        int xAdvance = 0;
        int yAdvance = 0;
        int xOffset = 0;
        int yOffset = 0;
        bool unsafeToBreak = false;
    };
    struct Result {
        std::vector<Glyph> glyphs; // HarfBuzz order; do not reverse RTL again.
        std::vector<SourceRange> unsupportedRanges;
        std::int64_t xAdvance = 0;
        std::int64_t yAdvance = 0;
        LC_OpenTypeFont::Error error;
        explicit operator bool() const { return !error; }
    };

    static Result shape(const LC_OpenTypeFont &font, const Run &run);
};

#endif
