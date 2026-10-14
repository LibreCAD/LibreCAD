/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_opentypefont.h"
#include "lc_textshaper.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <QDir>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <algorithm>
#include <future>
#include <numeric>
#include <set>

namespace {
QString fontPath() {
#ifdef LC_OPENTYPE_TEST_FONT_PATH
    return QString::fromUtf8(LC_OPENTYPE_TEST_FONT_PATH);
#else
    return QFileInfo(QString::fromUtf8(__FILE__)).absoluteDir().absoluteFilePath(
        "../../../../../../support/fonts/opentype/amiri-1.003/Amiri-Regular.ttf");
#endif
}
#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
std::shared_ptr<const LC_OpenTypeFont> font() {
    static const auto loaded = LC_OpenTypeFont::load(fontPath());
    REQUIRE_FALSE(loaded.error);
    REQUIRE(loaded.font);
    return loaded.font;
}
LC_TextShaper::Result shape(const QString &text) {
    LC_TextShaper::Run run;
    run.contextText = text;
    return LC_TextShaper::shape(*font(), run);
}
std::vector<std::uint32_t> ids(const LC_TextShaper::Result &result) {
    std::vector<std::uint32_t> output;
    for (const auto &glyph : result.glyphs) output.push_back(glyph.id);
    return output;
}
#endif
} // namespace

TEST_CASE("OpenType resources fail explicitly without changing profiles", "[opentype][resource][headless]") {
    const auto profile = LC_OpenTypeFont::amiriProfile();
    REQUIRE(profile.id == "lc-amiri-1.003-ot-v1");
    REQUIRE(profile.designHeight == 647);
#if !defined(LC_WITH_OPENTYPE) || !LC_WITH_OPENTYPE
    REQUIRE_FALSE(LC_OpenTypeFont::backendAvailable());
    REQUIRE(LC_OpenTypeFont::load(fontPath()).error.code == LC_OpenTypeFont::ErrorCode::BackendUnavailable);
#else
    REQUIRE(LC_OpenTypeFont::backendAvailable());
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    REQUIRE(LC_OpenTypeFont::load(temporary.filePath("missing.ttf")).error.code == LC_OpenTypeFont::ErrorCode::MissingFile);
    auto changed = profile;
    changed.sha256 = QByteArray(64, '0');
    REQUIRE(LC_OpenTypeFont::load(fontPath(), changed).error.code == LC_OpenTypeFont::ErrorCode::ChecksumMismatch);
    changed = profile;
    changed.designHeight = 0;
    REQUIRE(LC_OpenTypeFont::load(fontPath(), changed).error.code == LC_OpenTypeFont::ErrorCode::InvalidProfile);
    changed = profile;
    changed.ascender = 1;
    REQUIRE(LC_OpenTypeFont::load(fontPath(), changed).error.code == LC_OpenTypeFont::ErrorCode::InvalidProfile);
    changed = profile;
    changed.faceIndex = 7;
    REQUIRE(LC_OpenTypeFont::load(fontPath(), changed).error.code == LC_OpenTypeFont::ErrorCode::InvalidFont);
    QFile invalid(temporary.filePath("invalid.ttf"));
    REQUIRE(invalid.open(QIODevice::WriteOnly));
    const QByteArray invalidBytes("not an OpenType font");
    REQUIRE(invalid.write(invalidBytes) == invalidBytes.size());
    invalid.close();
    changed = profile;
    changed.sha256 = QCryptographicHash::hash(invalidBytes, QCryptographicHash::Sha256).toHex();
    REQUIRE(LC_OpenTypeFont::load(invalid.fileName(), changed).error.code == LC_OpenTypeFont::ErrorCode::InvalidFont);
    QFile oversized(temporary.filePath("oversized.ttf"));
    REQUIRE(oversized.open(QIODevice::WriteOnly));
    REQUIRE(oversized.resize(64 * 1024 * 1024 + 1));
    oversized.close();
    REQUIRE(LC_OpenTypeFont::load(oversized.fileName()).error.code == LC_OpenTypeFont::ErrorCode::ResourceLimit);

    // Omit GSUB from a disposable copy. The exact-byte check is updated only
    // for this corruption test, not for the production named profile.
    QFile original(fontPath());
    REQUIRE(original.open(QIODevice::ReadOnly));
    QByteArray noSubstitution = original.readAll();
    const unsigned tables = (static_cast<unsigned char>(noSubstitution.at(4)) << 8) |
                             static_cast<unsigned char>(noSubstitution.at(5));
    bool found = false;
    for (unsigned table = 0; table < tables; ++table) {
        const int offset = 12 + static_cast<int>(16 * table);
        REQUIRE(offset + 16 <= noSubstitution.size());
        if (noSubstitution.mid(offset, 4) == "GSUB") {
            noSubstitution.replace(offset, 4, "LCXX");
            found = true;
            break;
        }
    }
    REQUIRE(found);
    QFile missingTable(temporary.filePath("no-gsub.ttf"));
    REQUIRE(missingTable.open(QIODevice::WriteOnly));
    REQUIRE(missingTable.write(noSubstitution) == noSubstitution.size());
    missingTable.close();
    changed = profile;
    changed.sha256 = QCryptographicHash::hash(noSubstitution, QCryptographicHash::Sha256).toHex();
    REQUIRE(LC_OpenTypeFont::load(missingTable.fileName(), changed).error.code == LC_OpenTypeFont::ErrorCode::MissingTables);
    const auto exact = font();
    REQUIRE(exact->checksum() == profile.sha256);
    REQUIRE(exact->glyphCount() == 6710);
    REQUIRE(exact->normalization() == Catch::Approx(9.0 / 647));
    REQUIRE(LC_OpenTypeFont::shapingEngineIdentity().startsWith("HarfBuzz-OT/"));
#endif
}

#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
TEST_CASE("Pinned Arabic shaping retains uncoded contextual glyphs and font positioning", "[opentype][shaping][headless]") {
    const auto joining = shape(QString::fromUtf8(u8"ببب"));
    REQUIRE(joining);
    REQUIRE(joining.unsupportedRanges.empty());
    REQUIRE(ids(joining) == std::vector<std::uint32_t>{1589, 3999, 3958});
    REQUIRE(joining.glyphs[0].xAdvance == 883);
    REQUIRE(joining.glyphs[1].xAdvance == 244);
    REQUIRE(joining.glyphs[2].xAdvance == 233);
    REQUIRE(joining.xAdvance == 1360);
    REQUIRE(joining.glyphs[0].source.start == 2);
    REQUIRE(joining.glyphs[1].source.start == 1);
    REQUIRE(joining.glyphs[2].source.start == 0);
    const auto ligature = shape(QString::fromUtf8(u8"لا"));
    REQUIRE(ligature);
    REQUIRE(ids(ligature) == std::vector<std::uint32_t>{2518, 2513});
    REQUIRE(ligature.xAdvance == 642);
    const auto marked = shape(QString::fromUtf8(u8"لَا"));
    REQUIRE(marked);
    REQUIRE(ids(marked) == std::vector<std::uint32_t>{2518, 94, 2513});
    REQUIRE(marked.glyphs[1].xAdvance == 0);
    REQUIRE(marked.glyphs[1].xOffset == -76);
    REQUIRE(marked.xAdvance == 642);
    for (const QString &input : {QString::fromUtf8(u8"بَّ"), QString::fromUtf8(u8"بَّ")}) {
        const auto result = shape(input);
        REQUIRE(result);
        REQUIRE(ids(result) == std::vector<std::uint32_t>{1439, 97, 56});
        REQUIRE(result.glyphs[0].xAdvance == 0);
        REQUIRE(result.glyphs[0].xOffset == 453);
        REQUIRE(result.glyphs[0].yOffset == 51);
        REQUIRE(result.glyphs[1].xOffset == 253);
        REQUIRE(result.xAdvance == 926);
    }
}

TEST_CASE("Every core Arabic triple shapes through the production source mapping", "[opentype][shaping][corpus][headless]") {
    const std::vector<ushort> alphabet{
        0x627, 0x628, 0x62a, 0x62b, 0x62c, 0x62d, 0x62e, 0x62f,
        0x630, 0x631, 0x632, 0x633, 0x634, 0x635, 0x636, 0x637,
        0x638, 0x639, 0x63a, 0x641, 0x642, 0x643, 0x644, 0x645,
        0x646, 0x647, 0x648, 0x64a};
    std::set<std::uint32_t> used;
    unsigned cases = 0;
    for (ushort a : alphabet) for (ushort b : alphabet) for (ushort c : alphabet) {
        QString input;
        input += QChar(a);
        input += QChar(b);
        input += QChar(c);
        const auto result = shape(input);
        REQUIRE(result);
        REQUIRE(result.unsupportedRanges.empty());
        REQUIRE_FALSE(result.glyphs.empty());
        int previous = 3;
        for (const auto &glyph : result.glyphs) {
            REQUIRE(glyph.id > 0);
            REQUIRE(glyph.id < font()->glyphCount());
            REQUIRE(glyph.cluster <= previous);
            REQUIRE(glyph.source.start >= 0);
            REQUIRE(glyph.source.length > 0);
            REQUIRE(glyph.source.start + glyph.source.length <= 3);
            previous = glyph.cluster;
            used.insert(glyph.id);
        }
        ++cases;
    }
    REQUIRE(cases == 21952);
    REQUIRE(used.size() == 415);
    for (auto id : used) REQUIRE(font()->outline(id));
}

TEST_CASE("Clusters are UTF16 logical source intervals, not RTL glyph adjacency", "[opentype][source][headless]") {
    LC_TextShaper::Run run;
    run.contextText = QString::fromUtf8(u8"ببب");
    run.sourceOffset = 17;
    auto result = LC_TextShaper::shape(*font(), run);
    REQUIRE(result);
    REQUIRE(result.glyphs[0].cluster == 19);
    REQUIRE(result.glyphs[1].cluster == 18);
    REQUIRE(result.glyphs[2].cluster == 17);
    for (const auto &glyph : result.glyphs) REQUIRE(glyph.source.length == 1);
    const QString original = run.contextText;
    run.itemStart = 1;
    run.itemLength = 1;
    result = LC_TextShaper::shape(*font(), run);
    REQUIRE(result);
    REQUIRE_FALSE(result.glyphs.empty());
    for (const auto &glyph : result.glyphs) {
        REQUIRE(glyph.source.start == 18);
        REQUIRE(glyph.source.length == 1);
    }
    REQUIRE(run.contextText == original);
    REQUIRE(ids(result) != ids(shape(QString::fromUtf8(u8"ب"))));

    // A supplementary unsupported scalar still occupies two original units.
    run = {};
    run.contextText = QString::fromUcs4(U"\U0001F600");
    result = LC_TextShaper::shape(*font(), run);
    REQUIRE(result);
    REQUIRE(result.unsupportedRanges.size() == 1);
    REQUIRE(result.unsupportedRanges[0].start == 0);
    REQUIRE(result.unsupportedRanges[0].length == 2);
    run.itemStart = 1;
    REQUIRE(LC_TextShaper::shape(*font(), run).error.code == LC_OpenTypeFont::ErrorCode::InvalidInput);

    run = {};
    run.contextText = QString(QChar(0xD800));
    result = LC_TextShaper::shape(*font(), run);
    REQUIRE(result);
    REQUIRE(ids(result) == ids(shape(QString(QChar(0xFFFD)))));
    REQUIRE(result.glyphs[0].source.length == 1);
    REQUIRE(run.contextText.at(0).unicode() == 0xD800);

    // Replacement of an unpaired surrogate consumes exactly its original
    // UTF-16 unit; a neighboring well-formed pair must remain indivisible.
    run = {};
    for (ushort unit : {ushort(0xD800), ushort('A'), ushort(0xDC00), ushort(0xD83D), ushort(0xDE00)})
        run.contextText += QChar(unit);
    const QString malformed = run.contextText;
    result = LC_TextShaper::shape(*font(), run);
    REQUIRE(result);
    REQUIRE(ids(result) == ids(shape(QString::fromUcs4(U"\uFFFD" U"A\uFFFD\U0001F600"))));
    REQUIRE(run.contextText == malformed);
    for (const auto &glyph : result.glyphs) {
        REQUIRE_FALSE(glyph.source.start == 4);
        REQUIRE_FALSE(glyph.source.start + glyph.source.length == 4);
        REQUIRE(glyph.source.length == (glyph.source.start == 3 ? 2 : 1));
    }
    run.itemStart = 2;
    run.itemLength = 3;
    result = LC_TextShaper::shape(*font(), run);
    REQUIRE(result);
    for (const auto &glyph : result.glyphs) {
        REQUIRE(glyph.source.start >= 2);
        REQUIRE_FALSE(glyph.source.start == 4);
        REQUIRE_FALSE(glyph.source.start + glyph.source.length == 4);
    }

    run = {};
    run.contextText = QString(10000, QChar(0x0378));
    result = LC_TextShaper::shape(*font(), run);
    REQUIRE(result);
    REQUIRE(result.unsupportedRanges.size() == 10000);
    REQUIRE(result.unsupportedRanges.front().start == 0);
    REQUIRE(result.unsupportedRanges.back().start == 9999);
}

TEST_CASE("Joining controls survive shaping while empty ink and real spacing remain distinct", "[opentype][controls][headless]") {
    const auto plain = shape(QString::fromUtf8(u8"بب"));
    const auto joined = shape(QString::fromUtf8(u8"ب\u200Dب"));
    const auto broken = shape(QString::fromUtf8(u8"ب\u200Cب"));
    REQUIRE(plain);
    REQUIRE(joined);
    REQUIRE(broken);
    // ZWJ may select a different required contextual substitution; it does not
    // promise the same metrics as plain adjacency. These are hb-shape fixtures.
    REQUIRE(plain.xAdvance == 1019);
    REQUIRE(joined.xAdvance == 1073);
    REQUIRE(broken.xAdvance == 1852);
    REQUIRE(ids(joined) == std::vector<std::uint32_t>{1589, 1, 1611});
    REQUIRE(joined.glyphs[1].xAdvance == 0);
    REQUIRE(ids(shape(QString::fromUtf8(u8"ب\u200D"))) == std::vector<std::uint32_t>{1, 1611});
    REQUIRE(ids(shape(QString::fromUtf8(u8"ب\u200C"))) == std::vector<std::uint32_t>{1, 56});
    const auto space = shape(" ");
    REQUIRE(space);
    REQUIRE(space.xAdvance > 0);
    REQUIRE(space.glyphs.size() == 1);
    const auto outline = font()->outline(space.glyphs[0].id);
    REQUIRE(outline);
    REQUIRE_FALSE(outline.outline->hasInk);
    REQUIRE(outline.outline->commands.empty());
    REQUIRE(shape(QString()).glyphs.empty());
    for (const QString &text : {QStringLiteral("\n"), QStringLiteral("\t"), QString(QChar(0x2029))})
        REQUIRE(shape(text).error.code == LC_OpenTypeFont::ErrorCode::InvalidInput);
    LC_TextShaper::Run huge;
    huge.contextText = QString(1024 * 1024 + 1, QChar('a'));
    REQUIRE(LC_TextShaper::shape(*font(), huge).error.code == LC_OpenTypeFont::ErrorCode::ResourceLimit);
}

TEST_CASE("Native outlines retain original origins, curves and closed composite contours", "[opentype][outline][headless]") {
    LC_TextShaper::Run latin;
    latin.contextText = "A";
    latin.script = "Latn";
    latin.rightToLeft = false;
    const auto shaped = LC_TextShaper::shape(*font(), latin);
    REQUIRE(shaped);
    REQUIRE(shaped.glyphs.size() == 1);
    const auto outline = font()->outline(shaped.glyphs[0].id);
    REQUIRE(outline);
    REQUIRE(outline.outline->hasInk);
    REQUIRE(outline.outline->inkBounds == QRectF(-14, -3, 632, 650));
    const auto &commands = outline.outline->commands;
    REQUIRE_FALSE(commands.empty());
    REQUIRE(commands.front().type == LC_OpenTypeFont::Command::Type::MoveTo);
    REQUIRE(commands.back().type == LC_OpenTypeFont::Command::Type::Close);
    const auto count = [&commands](LC_OpenTypeFont::Command::Type type) {
        return std::count_if(commands.begin(), commands.end(), [type](const auto &command) { return command.type == type; });
    };
    REQUIRE(count(LC_OpenTypeFont::Command::Type::MoveTo) == count(LC_OpenTypeFont::Command::Type::Close));
    REQUIRE(count(LC_OpenTypeFont::Command::Type::QuadraticTo) > 0);
    for (const auto &glyph : shape(QString::fromUtf8(u8"ببببَّلا" )).glyphs) {
        const auto contextual = font()->outline(glyph.id);
        REQUIRE(contextual);
        REQUIRE(contextual.outline->hasInk);
    }
    REQUIRE(font()->outline(font()->glyphCount()).error.code == LC_OpenTypeFont::ErrorCode::InvalidGlyph);
}

TEST_CASE("Outline cache is immutable, bounded and safe across concurrent callers", "[opentype][cache][headless]") {
    const auto exact = font();
    exact->clearOutlineCache();
    const auto first = exact->outline(3999);
    REQUIRE(first);
    const auto repeated = exact->outline(3999);
    REQUIRE(repeated);
    REQUIRE(first.outline == repeated.outline);
    REQUIRE(exact->cacheStats().hits > 0);
    const auto savedCommands = first.outline->commands.size();
    for (unsigned id = 0; id < 700; ++id) REQUIRE(exact->outline(id));
    REQUIRE(exact->cacheStats().entries <= 512);
    REQUIRE(exact->cacheStats().bytes <= 8 * 1024 * 1024);
    REQUIRE(first.outline->commands.size() == savedCommands);
    std::vector<std::future<bool>> workers;
    for (unsigned thread = 0; thread < 8; ++thread) {
        workers.push_back(std::async(std::launch::async, [exact, thread] {
            for (unsigned n = 0; n < 100; ++n) {
                if (!exact->outline(1500 + thread * 10 + n % 10)) return false;
                LC_TextShaper::Run run;
                run.contextText = QString::fromUtf8(u8"لَا ببب بَّ");
                if (!LC_TextShaper::shape(*exact, run)) return false;
            }
            return true;
        }));
    }
    for (auto &worker : workers) REQUIRE(worker.get());
    exact->clearOutlineCache();
    REQUIRE(exact->cacheStats().entries == 0);
    REQUIRE(exact->cacheStats().bytes == 0);
    REQUIRE(first.outline->commands.size() == savedCommands);
}
#endif
