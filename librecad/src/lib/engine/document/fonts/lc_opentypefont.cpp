/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_opentypefont.h"
#include "lc_opentypefont_p.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <algorithm>
#include <cmath>
#include <limits>
#include <new>

#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
#include <hb-ot.h>
#include FT_OUTLINE_H
#include FT_BBOX_H
#if !HB_VERSION_ATLEAST(2, 6, 0)
#error "LibreCAD OpenType shaping requires HarfBuzz 2.6 or newer"
#endif
#if FREETYPE_MAJOR < 2 || (FREETYPE_MAJOR == 2 && FREETYPE_MINOR < 10)
#error "LibreCAD OpenType outlines require FreeType 2.10 or newer"
#endif
#endif

namespace {
using Font = LC_OpenTypeFont;
Font::Error failure(Font::ErrorCode code, const QString &message) {
    return {code, message};
}
#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
constexpr qint64 maximumFontBytes = 64 * 1024 * 1024;
constexpr std::size_t maximumOutlineCommands = 100000;
struct Decomposition {
    Font::Outline *outline;
    bool open = false;
    bool limit = false;
    void append(Font::Command command) {
        if (outline->commands.size() >= maximumOutlineCommands) {
            limit = true;
            return;
        }
        outline->commands.push_back(command);
    }
};
QPointF point(const FT_Vector *value) {
    return {static_cast<double>(value->x), static_cast<double>(value->y)};
}
int moveTo(const FT_Vector *destination, void *user) {
    auto &state = *static_cast<Decomposition *>(user);
    if (state.open)
        state.append({Font::Command::Type::Close, {}, {}, {}});
    state.append({Font::Command::Type::MoveTo, point(destination), {}, {}});
    state.open = true;
    return state.limit ? 1 : 0;
}
int lineTo(const FT_Vector *destination, void *user) {
    auto &state = *static_cast<Decomposition *>(user);
    state.append({Font::Command::Type::LineTo, point(destination), {}, {}});
    return state.limit ? 1 : 0;
}
int conicTo(const FT_Vector *control, const FT_Vector *destination, void *user) {
    auto &state = *static_cast<Decomposition *>(user);
    state.append({Font::Command::Type::QuadraticTo, point(control), point(destination), {}});
    return state.limit ? 1 : 0;
}
int cubicTo(const FT_Vector *first, const FT_Vector *second,
            const FT_Vector *destination, void *user) {
    auto &state = *static_cast<Decomposition *>(user);
    state.append({Font::Command::Type::CubicTo, point(first), point(second), point(destination)});
    return state.limit ? 1 : 0;
}
#endif
} // namespace

LC_OpenTypeFont::Impl::~Impl() {
#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
    if (hbFont) hb_font_destroy(hbFont);
    if (hbFace) hb_face_destroy(hbFace);
    if (blob) hb_blob_destroy(blob);
    if (face) FT_Done_Face(face);
    if (library) FT_Done_FreeType(library);
#endif
}

LC_OpenTypeFont::LC_OpenTypeFont(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}
LC_OpenTypeFont::~LC_OpenTypeFont() = default;

LC_OpenTypeFont::Profile LC_OpenTypeFont::amiriProfile() {
    Profile result;
    result.id = QStringLiteral("lc-amiri-1.003-ot-v1");
    result.sha256 = "cd2550c0f4c05eb341bf97958211aaa39382bca96577ba3a67d4a3b4912c43c0";
    return result;
}

bool LC_OpenTypeFont::backendAvailable() {
#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
    return true;
#else
    return false;
#endif
}

QString LC_OpenTypeFont::shapingEngineIdentity() {
#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
    return QStringLiteral("HarfBuzz-OT/") + QString::fromLatin1(hb_version_string());
#else
    return QStringLiteral("OpenType-unavailable");
#endif
}

LC_OpenTypeFont::LoadResult LC_OpenTypeFont::load(const QString &path, const Profile &profile) {
#if !defined(LC_WITH_OPENTYPE) || !LC_WITH_OPENTYPE
    Q_UNUSED(path)
    Q_UNUSED(profile)
    return {{}, failure(ErrorCode::BackendUnavailable, QStringLiteral("OpenType support is disabled in this build"))};
#else
    if (profile.id.isEmpty() || profile.sha256.size() != 64 ||
        QByteArray::fromHex(profile.sha256).toHex() != profile.sha256.toLower() ||
        !profile.unitsPerEm || !std::isfinite(profile.designHeight) || profile.designHeight <= 0 ||
        profile.lineHeight <= 0 || profile.ascender <= profile.descender || profile.faceIndex > 65535) {
        return {{}, failure(ErrorCode::InvalidProfile, QStringLiteral("Invalid OpenType font profile"))};
    }
    QFile source(path);
    if (!source.exists())
        return {{}, failure(ErrorCode::MissingFile, QStringLiteral("The exact OpenType font resource is missing"))};
    if (!source.open(QIODevice::ReadOnly))
        return {{}, failure(ErrorCode::UnreadableFile, QStringLiteral("The OpenType font resource cannot be read"))};
    if (source.size() <= 0 || source.size() > maximumFontBytes)
        return {{}, failure(ErrorCode::ResourceLimit, QStringLiteral("OpenType font resource size is outside limits"))};
    auto impl = std::make_unique<Impl>();
    impl->profile = profile;
    // Read a bounded amount even if the file grows after the size check.
    impl->bytes = source.read(maximumFontBytes + 1);
    if (source.error() != QFileDevice::NoError)
        return {{}, failure(ErrorCode::UnreadableFile, QStringLiteral("Reading the OpenType font resource failed"))};
    if (impl->bytes.size() > maximumFontBytes)
        return {{}, failure(ErrorCode::ResourceLimit, QStringLiteral("OpenType font resource exceeds the size limit"))};
    impl->checksum = QCryptographicHash::hash(impl->bytes, QCryptographicHash::Sha256).toHex();
    if (impl->checksum != profile.sha256.toLower())
        return {{}, failure(ErrorCode::ChecksumMismatch, QStringLiteral("OpenType font resource checksum does not match its profile"))};
    if (FT_Init_FreeType(&impl->library) ||
        FT_New_Memory_Face(impl->library,
                           reinterpret_cast<const FT_Byte *>(impl->bytes.constData()),
                           static_cast<FT_Long>(impl->bytes.size()),
                           static_cast<FT_Long>(profile.faceIndex), &impl->face)) {
        return {{}, failure(ErrorCode::InvalidFont, QStringLiteral("FreeType rejected the OpenType font resource"))};
    }
    if (!FT_IS_SFNT(impl->face) || !FT_IS_SCALABLE(impl->face) ||
        FT_HAS_MULTIPLE_MASTERS(impl->face) || FT_HAS_COLOR(impl->face)) {
        return {{}, failure(ErrorCode::UnsupportedFont, QStringLiteral("Only static, scalable monochrome OpenType fonts are supported"))};
    }
    if (impl->face->units_per_EM != profile.unitsPerEm ||
        impl->face->ascender != profile.ascender || impl->face->descender != profile.descender ||
        impl->face->height != profile.lineHeight) {
        return {{}, failure(ErrorCode::InvalidProfile, QStringLiteral("OpenType font metrics do not match the saved profile"))};
    }
    impl->blob = hb_blob_create(impl->bytes.constData(), static_cast<unsigned>(impl->bytes.size()),
                               HB_MEMORY_MODE_READONLY, nullptr, nullptr);
    impl->hbFace = hb_face_create(impl->blob, profile.faceIndex);
    impl->glyphCount = hb_face_get_glyph_count(impl->hbFace);
    if (!impl->glyphCount || impl->glyphCount > 100000 ||
        impl->glyphCount != static_cast<unsigned>(impl->face->num_glyphs) ||
        hb_face_get_upem(impl->hbFace) != profile.unitsPerEm) {
        return {{}, failure(ErrorCode::InvalidFont, QStringLiteral("HarfBuzz and FreeType disagree about the font face"))};
    }
    if (profile.requireArabicTables) {
        for (hb_tag_t tag : {HB_TAG('G', 'S', 'U', 'B'), HB_TAG('G', 'P', 'O', 'S'), HB_TAG('G', 'D', 'E', 'F')}) {
            hb_blob_t *table = hb_face_reference_table(impl->hbFace, tag);
            const unsigned length = hb_blob_get_length(table);
            hb_blob_destroy(table);
            if (!length)
                return {{}, failure(ErrorCode::MissingTables, QStringLiteral("The certified Arabic face lacks a required OpenType table"))};
        }
    }
    hb_face_make_immutable(impl->hbFace);
    impl->hbFont = hb_font_create(impl->hbFace);
    hb_ot_font_set_funcs(impl->hbFont);
    hb_font_set_scale(impl->hbFont, static_cast<int>(profile.unitsPerEm), static_cast<int>(profile.unitsPerEm));
    hb_font_make_immutable(impl->hbFont);
    return {std::shared_ptr<const LC_OpenTypeFont>(new LC_OpenTypeFont(std::move(impl))), {}};
#endif
}

const LC_OpenTypeFont::Profile &LC_OpenTypeFont::profile() const { return m_impl->profile; }
const QByteArray &LC_OpenTypeFont::checksum() const { return m_impl->checksum; }
unsigned int LC_OpenTypeFont::glyphCount() const { return m_impl->glyphCount; }
double LC_OpenTypeFont::normalization() const { return 9.0 / m_impl->profile.designHeight; }

LC_OpenTypeFont::OutlineResult LC_OpenTypeFont::outline(std::uint32_t glyphId) const {
#if !defined(LC_WITH_OPENTYPE) || !LC_WITH_OPENTYPE
    Q_UNUSED(glyphId)
    return {{}, failure(ErrorCode::BackendUnavailable, QStringLiteral("OpenType support is disabled in this build"))};
#else
    if (glyphId >= m_impl->glyphCount)
        return {{}, failure(ErrorCode::InvalidGlyph, QStringLiteral("Glyph ID is outside the exact font face"))};
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    auto cached = m_impl->cache.find(glyphId);
    if (cached != m_impl->cache.end()) {
        ++m_impl->stats.hits;
        cached->second.lastUse = ++m_impl->clock;
        return {cached->second.outline, {}};
    }
    ++m_impl->stats.misses;
    // NO_RECURSE must not be used: composite glyphs need their complete outlines.
    if (FT_Load_Glyph(m_impl->face, glyphId, FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP))
        return {{}, failure(ErrorCode::OutlineFailure, QStringLiteral("FreeType could not load the requested glyph outline"))};
    auto result = std::make_shared<Outline>();
    const FT_GlyphSlot slot = m_impl->face->glyph;
    if (slot->format != FT_GLYPH_FORMAT_OUTLINE)
        return {{}, failure(ErrorCode::OutlineFailure, QStringLiteral("The requested glyph is not an outline"))};
    if (slot->outline.n_contours > 0 && slot->outline.n_points > 0) {
        if (slot->outline.n_points > 30000 || slot->outline.n_contours > 10000 || FT_Outline_Check(&slot->outline))
            return {{}, failure(ErrorCode::ResourceLimit, QStringLiteral("The glyph outline is invalid or exceeds complexity limits"))};
        FT_BBox bounds{};
        if (FT_Outline_Get_BBox(&slot->outline, &bounds))
            return {{}, failure(ErrorCode::OutlineFailure, QStringLiteral("The glyph ink bounds could not be computed"))};
        result->inkBounds = QRectF(static_cast<double>(bounds.xMin), static_cast<double>(bounds.yMin),
                                  static_cast<double>(bounds.xMax - bounds.xMin),
                                  static_cast<double>(bounds.yMax - bounds.yMin));
        Decomposition state{result.get()};
        const FT_Outline_Funcs functions{moveTo, lineTo, conicTo, cubicTo, 0, 0};
        const int error = FT_Outline_Decompose(&slot->outline, &functions, &state);
        if (state.open) state.append({Command::Type::Close, {}, {}, {}});
        if (state.limit)
            return {{}, failure(ErrorCode::ResourceLimit, QStringLiteral("The glyph contour command limit was exceeded"))};
        if (error)
            return {{}, failure(ErrorCode::OutlineFailure, QStringLiteral("The glyph contour decomposition failed"))};
        result->hasInk = true;
    }
    const std::size_t bytes = sizeof(Outline) + result->commands.capacity() * sizeof(Command);
    while (!m_impl->cache.empty() &&
           (m_impl->cache.size() >= Impl::maximumCacheEntries || m_impl->stats.bytes + bytes > Impl::maximumCacheBytes)) {
        auto oldest = std::min_element(m_impl->cache.begin(), m_impl->cache.end(),
            [](const auto &left, const auto &right) { return left.second.lastUse < right.second.lastUse; });
        m_impl->stats.bytes -= oldest->second.bytes;
        m_impl->cache.erase(oldest);
    }
    if (bytes <= Impl::maximumCacheBytes) {
        m_impl->cache.emplace(glyphId, Impl::CacheEntry{result, bytes, ++m_impl->clock});
        m_impl->stats.bytes += bytes;
        m_impl->stats.entries = m_impl->cache.size();
    }
    return {result, {}};
#endif
}

LC_OpenTypeFont::CacheStats LC_OpenTypeFont::cacheStats() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->stats;
}
void LC_OpenTypeFont::clearOutlineCache() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->cache.clear();
    m_impl->stats.entries = 0;
    m_impl->stats.bytes = 0;
}
