/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_OPENTYPEFONT_P_H
#define LC_OPENTYPEFONT_P_H

// Private implementation shared only by the font resource and run shaper.
#include "lc_opentypefont.h"
#include <mutex>
#include <unordered_map>

#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
#include <hb.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

struct LC_OpenTypeFont::Impl {
    Profile profile;
    QByteArray bytes;
    QByteArray checksum;
    unsigned int glyphCount = 0;
    mutable std::mutex mutex;
    struct CacheEntry {
        std::shared_ptr<const Outline> outline;
        std::size_t bytes = 0;
        std::uint64_t lastUse = 0;
    };
    mutable std::unordered_map<std::uint32_t, CacheEntry> cache;
    mutable CacheStats stats;
    mutable std::uint64_t clock = 0;
    static constexpr std::size_t maximumCacheBytes = 8 * 1024 * 1024;
    static constexpr std::size_t maximumCacheEntries = 512;
#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
    FT_Library library = nullptr;
    FT_Face face = nullptr;
    hb_blob_t *blob = nullptr;
    hb_face_t *hbFace = nullptr;
    hb_font_t *hbFont = nullptr;
#endif
    ~Impl();
};

#endif
