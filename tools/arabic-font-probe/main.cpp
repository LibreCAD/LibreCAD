/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/

#include <hb.h>
#include <hb-ot.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_BBOX_H

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct FreeType {
    FT_Library library = nullptr;
    FT_Face face = nullptr;
    explicit FreeType(const char *path) {
        if (FT_Init_FreeType(&library)) throw std::runtime_error("FreeType initialization failed");
        if (FT_New_Face(library, path, 0, &face)) {
            FT_Done_FreeType(library);
            library = nullptr;
            throw std::runtime_error("Cannot load font face 0");
        }
    }
    ~FreeType() {
        if (face) FT_Done_Face(face);
        if (library) FT_Done_FreeType(library);
    }
    FreeType(const FreeType &) = delete;
    FreeType &operator=(const FreeType &) = delete;
};

struct Result {
    std::vector<hb_glyph_info_t> info;
    std::vector<hb_glyph_position_t> positions;
};

Result shape(hb_font_t *font, const std::vector<std::uint32_t> &text,
             const char *featureText = nullptr, const char *shaper = "ot") {
    std::unique_ptr<hb_buffer_t, decltype(&hb_buffer_destroy)> buffer(hb_buffer_create(), hb_buffer_destroy);
    hb_buffer_set_direction(buffer.get(), HB_DIRECTION_RTL);
    hb_buffer_set_script(buffer.get(), HB_SCRIPT_ARABIC);
    hb_buffer_set_language(buffer.get(), hb_language_from_string("ar", -1));
    // Reproduce the original audit; production source mapping uses
    // MONOTONE_CHARACTERS and has separate fixtures.
    hb_buffer_set_cluster_level(buffer.get(), HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES);
    hb_buffer_add_codepoints(buffer.get(), text.data(), static_cast<int>(text.size()),
                             0, static_cast<int>(text.size()));
    hb_feature_t feature{};
    if (featureText && !hb_feature_from_string(featureText, -1, &feature))
        throw std::runtime_error("Invalid audit feature");
    const char *shapers[] = {shaper, nullptr};
    if (!hb_buffer_allocation_successful(buffer.get()) ||
        !hb_shape_full(font, buffer.get(), featureText ? &feature : nullptr,
                       featureText ? 1 : 0, shapers))
        throw std::runtime_error(std::string("Shaping failed or shaper unavailable: ") + shaper);
    unsigned count = 0;
    const auto *info = hb_buffer_get_glyph_infos(buffer.get(), &count);
    const auto *positions = hb_buffer_get_glyph_positions(buffer.get(), nullptr);
    if (count == 0 || !info || !positions) throw std::runtime_error("Empty shaping output");
    return {{info, info + count}, {positions, positions + count}};
}

bool equal(const Result &a, const Result &b) {
    if (a.info.size() != b.info.size()) return false;
    for (std::size_t i = 0; i < a.info.size(); ++i) {
        const auto &ap = a.positions[i];
        const auto &bp = b.positions[i];
        if (a.info[i].codepoint != b.info[i].codepoint || a.info[i].cluster != b.info[i].cluster ||
            ap.x_advance != bp.x_advance || ap.y_advance != bp.y_advance ||
            ap.x_offset != bp.x_offset || ap.y_offset != bp.y_offset) return false;
    }
    return true;
}

// Independent engines can distribute advance among glyphs differently.
// Compare absolute origins and total advance, not per-glyph bookkeeping.
std::int64_t placementError(const Result &a, const Result &b) {
    if (a.info.size() != b.info.size()) return INT64_MAX;
    std::int64_t ax = 0, ay = 0, bx = 0, by = 0, error = 0;
    for (std::size_t i = 0; i < a.info.size(); ++i) {
        if (a.info[i].codepoint != b.info[i].codepoint) return INT64_MAX;
        const auto &ap = a.positions[i];
        const auto &bp = b.positions[i];
        error = std::max(error, std::abs(ax + ap.x_offset - bx - bp.x_offset));
        error = std::max(error, std::abs(ay + ap.y_offset - by - bp.y_offset));
        ax += ap.x_advance; ay += ap.y_advance;
        bx += bp.x_advance; by += bp.y_advance;
    }
    return std::max({error, std::abs(ax - bx), std::abs(ay - by)});
}
} // namespace

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3 || (argc == 3 && std::string(argv[2]) != "--coretext")) {
        std::fprintf(stderr, "Usage: arabic-font-probe Amiri-Regular.ttf [--coretext]\n");
        return 2;
    }
    try {
        const bool compareCoreText = argc == 3;
        FreeType ft(argv[1]);
        FT_Int major, minor, patch;
        FT_Library_Version(ft.library, &major, &minor, &patch);
        std::printf("HarfBuzz=%s FreeType=%d.%d.%d clusterPolicy=MONOTONE_GRAPHEMES\n",
                    hb_version_string(), major, minor, patch);
        if (FT_Load_Char(ft.face, 0x41, FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP) ||
            ft.face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
            throw std::runtime_error("Cannot load calibration outline");
        FT_BBox bounds{};
        if (FT_Outline_Get_BBox(&ft.face->glyph->outline, &bounds))
            throw std::runtime_error("Cannot measure calibration outline");
        std::printf("upem=%u glyphs=%ld ascender=%d descender=%d height=%d\n",
                    ft.face->units_per_EM, ft.face->num_glyphs, ft.face->ascender,
                    ft.face->descender, ft.face->height);
        std::printf("ABox=(%ld,%ld,%ld,%ld) Aadvance=%ld scale9=%.15f\n",
                    bounds.xMin, bounds.yMin, bounds.xMax, bounds.yMax,
                    ft.face->glyph->metrics.horiAdvance, 9.0 / bounds.yMax);
        const bool metricsMatch = ft.face->units_per_EM == 1000 && ft.face->num_glyphs == 6710 &&
            ft.face->ascender == 1124 && ft.face->descender == -634 && ft.face->height == 1758 &&
            bounds.xMin == -14 && bounds.yMin == -3 && bounds.xMax == 618 && bounds.yMax == 647 &&
            ft.face->glyph->metrics.horiAdvance == 612;

        std::unique_ptr<hb_blob_t, decltype(&hb_blob_destroy)> blob(hb_blob_create_from_file(argv[1]), hb_blob_destroy);
        if (!hb_blob_get_length(blob.get())) throw std::runtime_error("Cannot read HarfBuzz font bytes");
        std::unique_ptr<hb_face_t, decltype(&hb_face_destroy)> face(hb_face_create(blob.get(), 0), hb_face_destroy);
        std::unique_ptr<hb_font_t, decltype(&hb_font_destroy)> font(hb_font_create(face.get()), hb_font_destroy);
        hb_ot_font_set_funcs(font.get());
        hb_font_set_scale(font.get(), ft.face->units_per_EM, ft.face->units_per_EM);
        if (hb_face_get_upem(face.get()) != ft.face->units_per_EM ||
            hb_face_get_glyph_count(face.get()) != static_cast<unsigned>(ft.face->num_glyphs))
            throw std::runtime_error("FreeType/HarfBuzz face identity mismatch");
        std::set<unsigned> encoded;
        FT_UInt gid = 0;
        FT_ULong cp = FT_Get_First_Char(ft.face, &gid);
        while (gid) {
            encoded.insert(gid);
            cp = FT_Get_Next_Char(ft.face, cp, &gid);
        }

        const std::vector<std::vector<std::uint32_t>> samples = {
            {0x628,0x628,0x628}, {0x644,0x627}, {0x644,0x64e,0x627},
            {0x633,0x644,0x627,0x645}, {0x645,0x62d,0x645,0x62f},
            {0x628,0x64e,0x651}, {0x628,0x651,0x64e}, {0x628,0x650,0x651},
            {0x644,0x644,0x647}, {0x639,0x631}, {0x642,0x62d},
            {0x628,0x200d,0x628}, {0x628,0x200c,0x628}, {0x66e,0x66e}, {0x750,0x750}};
        std::int64_t maxError = 0;
        for (const auto &sample : samples) {
            const auto result = shape(font.get(), sample);
            std::printf("text:");
            for (auto scalar : sample) std::printf(" U+%04X", scalar);
            std::printf("\n");
            for (std::size_t i = 0; i < result.info.size(); ++i) {
                const auto &p = result.positions[i];
                std::printf("  gid=%u cluster=%u advance=(%d,%d) offset=(%d,%d) encoded=%d\n",
                            result.info[i].codepoint, result.info[i].cluster,
                            p.x_advance, p.y_advance, p.x_offset, p.y_offset,
                            encoded.count(result.info[i].codepoint) ? 1 : 0);
            }
            if (compareCoreText) maxError = std::max(maxError, placementError(result, shape(font.get(), sample, nullptr, "coretext")));
        }
        const std::vector<std::uint32_t> alphabet = {
            0x627,0x628,0x62a,0x62b,0x62c,0x62d,0x62e,0x62f,0x630,0x631,0x632,0x633,0x634,0x635,
            0x636,0x637,0x638,0x639,0x63a,0x641,0x642,0x643,0x644,0x645,0x646,0x647,0x648,0x64a};
        std::size_t total = 0, uncodedCount = 0, offsets = 0, curs = 0, rlig = 0, independent = 0;
        std::set<unsigned> used, uncoded;
        for (auto a : alphabet) for (auto b : alphabet) for (auto c : alphabet) {
            const std::vector<std::uint32_t> text{a,b,c};
            const auto result = shape(font.get(), text);
            bool hasUncoded = false, hasOffset = false;
            for (std::size_t i = 0; i < result.info.size(); ++i) {
                const auto id = result.info[i].codepoint;
                if (!id || id >= static_cast<unsigned>(ft.face->num_glyphs))
                    throw std::runtime_error("Missing or invalid glyph in core corpus");
                used.insert(id);
                if (!encoded.count(id)) { hasUncoded = true; uncoded.insert(id); }
                hasOffset |= result.positions[i].x_offset || result.positions[i].y_offset;
            }
            ++total; uncodedCount += hasUncoded; offsets += hasOffset;
            curs += !equal(result, shape(font.get(), text, "curs=0"));
            rlig += !equal(result, shape(font.get(), text, "rlig=0"));
            if (compareCoreText) {
                maxError = std::max(maxError, placementError(result, shape(font.get(), text, nullptr, "coretext")));
                ++independent;
            }
        }
        std::printf("cmapGlyphs=%zu coreTriples=%zu usesUncodedGlyph=%zu usesOffset=%zu cursChanges=%zu rligChanges=%zu distinctUsed=%zu distinctUncoded=%zu\n",
                    encoded.size(), total, uncodedCount, offsets, curs, rlig, used.size(), uncoded.size());
        if (compareCoreText)
            std::printf("CoreText triples=%zu samples=%zu maxPlacementError=%lld\n", independent, samples.size(), static_cast<long long>(maxError));
        else std::printf("CoreText=NOT_RUN (request --coretext on a supported build)\n");
        const bool corpusMatch = encoded.size() == 1622 && total == 21952 && uncodedCount == 20944 &&
            offsets == 671 && curs == 418 && rlig == 8393 && used.size() == 415 && uncoded.size() == 387;
        if (!metricsMatch || !corpusMatch || (compareCoreText && maxError > 1)) {
            std::fprintf(stderr, "FAIL: pinned audit differs; investigate font/engine identity, do not update expectations automatically\n");
            return 1;
        }
        std::printf("Pinned audit: PASS\n");
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
