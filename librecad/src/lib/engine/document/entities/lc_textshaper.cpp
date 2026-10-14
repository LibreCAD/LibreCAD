/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textshaper.h"
#include "lc_opentypefont_p.h"

#include <algorithm>
#include <limits>
#include <memory>

namespace {
#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
constexpr int maximumInputUnits = 1024 * 1024;
constexpr unsigned int maximumOutputGlyphs = 2 * 1024 * 1024;
bool splitsSurrogate(const QString &text, int position) {
    return position > 0 && position < text.size() &&
           text.at(position - 1).isHighSurrogate() && text.at(position).isLowSurrogate();
}
#endif
LC_TextShaper::Result invalid(LC_OpenTypeFont::ErrorCode code, const QString &message) {
    LC_TextShaper::Result result;
    result.error = {code, message};
    return result;
}
} // namespace

LC_TextShaper::Result LC_TextShaper::shape(const LC_OpenTypeFont &font, const Run &run) {
#if !defined(LC_WITH_OPENTYPE) || !LC_WITH_OPENTYPE
    Q_UNUSED(font)
    Q_UNUSED(run)
    return invalid(LC_OpenTypeFont::ErrorCode::BackendUnavailable,
                   QStringLiteral("OpenType support is disabled in this build"));
#else
    const auto size = run.contextText.size();
    if (size > maximumInputUnits)
        return invalid(LC_OpenTypeFont::ErrorCode::ResourceLimit,
                       QStringLiteral("The shaping context exceeds the input limit"));
    if (run.itemStart < 0 || run.itemStart > size || run.itemLength < -1 ||
        (run.itemLength >= 0 && run.itemLength > size - run.itemStart) || run.sourceOffset < 0 ||
        size > std::numeric_limits<int>::max() - run.sourceOffset)
        return invalid(LC_OpenTypeFont::ErrorCode::InvalidInput,
                       QStringLiteral("Invalid UTF-16 shaping interval"));
    const int length = run.itemLength < 0 ? static_cast<int>(size) - run.itemStart : run.itemLength;
    const int end = run.itemStart + length;
    if (splitsSurrogate(run.contextText, run.itemStart) || splitsSurrogate(run.contextText, end))
        return invalid(LC_OpenTypeFont::ErrorCode::InvalidInput,
                       QStringLiteral("A shaping item boundary splits a Unicode scalar"));
    if (run.script.size() != 4 || run.language.isEmpty() || run.language.size() > 63 ||
        run.language.contains('\0'))
        return invalid(LC_OpenTypeFont::ErrorCode::InvalidInput,
                       QStringLiteral("Shaping requires an explicit valid script and language"));
    const hb_script_t script = hb_script_from_string(run.script.constData(), run.script.size());
    if (script == HB_SCRIPT_INVALID || script == HB_SCRIPT_UNKNOWN)
        return invalid(LC_OpenTypeFont::ErrorCode::InvalidInput,
                       QStringLiteral("The shaping script is unknown"));
    for (int index = run.itemStart; index < end; ++index) {
        const ushort code = run.contextText.at(index).unicode();
        if (code == '\n' || code == '\r' || code == '\t' || code == 0x2028 || code == 0x2029)
            return invalid(LC_OpenTypeFont::ErrorCode::InvalidInput,
                           QStringLiteral("Paragraph and tab boundaries must be itemized before shaping"));
    }
    Result result;
    if (!length) return result;
    using Buffer = std::unique_ptr<hb_buffer_t, decltype(&hb_buffer_destroy)>;
    Buffer buffer(hb_buffer_create(), hb_buffer_destroy);
    if (!hb_buffer_allocation_successful(buffer.get()))
        return invalid(LC_OpenTypeFont::ErrorCode::ResourceLimit,
                       QStringLiteral("The shaping buffer could not be allocated"));
    hb_buffer_set_direction(buffer.get(), run.rightToLeft ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
    hb_buffer_set_script(buffer.get(), script);
    hb_buffer_set_language(buffer.get(), hb_language_from_string(run.language.constData(), run.language.size()));
    hb_buffer_set_cluster_level(buffer.get(), HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
    // BOT/EOT apply at actual applicable context edges, not every font/size item.
    unsigned int flags = HB_BUFFER_FLAG_DEFAULT;
    if (run.beginningOfText && run.itemStart == 0) flags |= HB_BUFFER_FLAG_BOT;
    if (run.endOfText && end == size) flags |= HB_BUFFER_FLAG_EOT;
    hb_buffer_set_flags(buffer.get(), static_cast<hb_buffer_flags_t>(flags));
    hb_buffer_set_replacement_codepoint(buffer.get(), 0xFFFD);
    hb_buffer_add_utf16(buffer.get(), reinterpret_cast<const std::uint16_t *>(run.contextText.utf16()),
                        static_cast<int>(size), static_cast<unsigned>(run.itemStart), length);
    // Request OpenType explicitly: a silent fallback shaper is not this profile.
    const char *const shapers[] = {"ot", nullptr};
    if (!hb_buffer_allocation_successful(buffer.get()) ||
        !hb_shape_full(font.m_impl->hbFont, buffer.get(), nullptr, 0, shapers))
        return invalid(LC_OpenTypeFont::ErrorCode::ShapingFailure,
                       QStringLiteral("The OpenType shaper failed"));
    unsigned int count = 0;
    const hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buffer.get(), &count);
    const hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buffer.get(), nullptr);
    if (!hb_buffer_allocation_successful(buffer.get()) || count > maximumOutputGlyphs ||
        (count && (!info || !positions)))
        return invalid(LC_OpenTypeFont::ErrorCode::ResourceLimit,
                       QStringLiteral("The shaping output exceeds its resource limits"));
    std::vector<int> boundaries;
    boundaries.reserve(static_cast<std::size_t>(count) + 2);
    boundaries.push_back(run.itemStart);
    boundaries.push_back(end);
    for (unsigned index = 0; index < count; ++index) {
        if (info[index].cluster < static_cast<unsigned>(run.itemStart) ||
            info[index].cluster >= static_cast<unsigned>(end) ||
            info[index].codepoint >= font.glyphCount())
            return invalid(LC_OpenTypeFont::ErrorCode::ShapingFailure,
                           QStringLiteral("The shaper returned an invalid font glyph or source cluster"));
        boundaries.push_back(static_cast<int>(info[index].cluster));
    }
    std::sort(boundaries.begin(), boundaries.end());
    boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());
    result.glyphs.reserve(count);
    for (unsigned index = 0; index < count; ++index) {
        const int cluster = static_cast<int>(info[index].cluster);
        const auto next = std::upper_bound(boundaries.begin(), boundaries.end(), cluster);
        Glyph glyph;
        glyph.id = info[index].codepoint;
        glyph.cluster = run.sourceOffset + cluster;
        glyph.source = {glyph.cluster, *next - cluster};
        glyph.xAdvance = positions[index].x_advance;
        glyph.yAdvance = positions[index].y_advance;
        glyph.xOffset = positions[index].x_offset;
        glyph.yOffset = positions[index].y_offset;
        glyph.unsafeToBreak = (hb_glyph_info_get_glyph_flags(&info[index]) & HB_GLYPH_FLAG_UNSAFE_TO_BREAK) != 0;
        result.xAdvance += glyph.xAdvance;
        result.yAdvance += glyph.yAdvance;
        if (!glyph.id) result.unsupportedRanges.push_back(glyph.source);
        result.glyphs.push_back(glyph);
    }
    // Collect then sort/deduplicate rather than searching the growing vector
    // for each .notdef: long unsupported input must not become quadratic.
    auto &unsupported = result.unsupportedRanges;
    std::sort(unsupported.begin(), unsupported.end(), [](const SourceRange &a, const SourceRange &b) {
        return a.start < b.start || (a.start == b.start && a.length < b.length);
    });
    unsupported.erase(std::unique(unsupported.begin(), unsupported.end(), [](const SourceRange &a, const SourceRange &b) {
        return a.start == b.start && a.length == b.length;
    }), unsupported.end());
    return result;
#endif
}
