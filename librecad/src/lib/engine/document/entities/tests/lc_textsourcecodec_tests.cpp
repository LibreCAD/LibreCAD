/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textsourcecodec.h"

#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <random>

namespace {
using Codec = LC_TextSourceCodec;
using Document = LC_TextDocument;
using Syntax = Document::Syntax;
using Code = Codec::ErrorCode;
Codec::Options options(bool toDxf, Syntax syntax = Syntax::MText) {
    Codec::Options result;
    result.syntax = syntax;
    result.sourceCaretCodes = !toDxf;
    result.targetCaretCodes = toDxf;
    return result;
}
Document parsed(const QString &source, bool caret, Syntax syntax = Syntax::MText, bool escapes = true) {
    Document::Options result;
    result.syntax = syntax;
    result.decodeCaretCodes = caret;
    result.decodeDxfEscapes = escapes;
    return Document::parse(source, result);
}
QString converted(const QString &input, const QString &expected, const Codec::Options &settings) {
    INFO("input UTF8 hex " << input.toUtf8().toHex().toStdString());
    INFO("expected UTF8 hex " << expected.toUtf8().toHex().toStdString());
    const auto before = input;
    const auto result = Codec::transcode(input, settings);
    INFO(result.error.message.toStdString());
    REQUIRE(result);
    REQUIRE(result.source);
    CHECK(*result.source == expected);
    CHECK(result.changed == (expected != input));
    CHECK(input == before);
    const auto bytes = Codec::transcode(input.toUtf8(), settings);
    INFO(bytes.error.message.toStdString());
    REQUIRE(bytes);
    CHECK(*bytes.source == expected.toUtf8());
    CHECK(bytes.changed == result.changed);
    return *result.source;
}
void failed(const QString &input, const Codec::Options &settings, Code code) {
    const auto before = input;
    const auto result = Codec::transcode(input, settings);
    CHECK_FALSE(result);
    CHECK_FALSE(result.source);
    CHECK_FALSE(result.changed);
    CHECK(result.error.code == code);
    CHECK_FALSE(result.error.message.isEmpty());
    CHECK(input == before);
}
QString mtextLiteral(ushort value) {
    // Independent expected spelling table, not the production transcoder.
    switch (value) {
    case '\\': return QStringLiteral("\\\\");
    case '{': return QStringLiteral("\\{");
    case '}': return QStringLiteral("\\}");
    case ';': return QStringLiteral("\\;");
    case '^': return QStringLiteral("\\^");
    case '%': return QStringLiteral("\\U+0025");
    default: return QString(QChar(value));
    }
}
} // namespace

TEST_CASE("Caret transcoding preserves exact same-convention values and byte storage", "[text-source-codec][headless][exact]") {
    for (const auto caret : {false, true}) {
        for (const auto syntax : {Syntax::Plain, Syntax::MText}) {
            auto settings = options(caret, syntax);
            settings.sourceCaretCodes = caret;
            QString malformed = QStringLiteral("{\\Unknown^I;\\Sب^ ج;\\}");
            malformed += QChar(0xd800); malformed += QChar(0); malformed += QChar(0xdc00);
            for (const auto &input : {QString{}, QStringLiteral(""), malformed}) {
                const auto result = Codec::transcode(input, settings);
                REQUIRE(result);
                CHECK(*result.source == input);
                CHECK(result.source->isNull() == input.isNull());
                CHECK_FALSE(result.changed);
            }
            for (const auto &input : {QByteArray{}, QByteArray(""), QByteArray("\xFF\0^I{\\Z;", 9)}) {
                const auto result = Codec::transcode(input, settings);
                REQUIRE(result);
                CHECK(*result.source == input);
                CHECK(result.source->isNull() == input.isNull());
                CHECK_FALSE(result.changed);
            }
        }
    }
    QString text = QStringLiteral("retained^I");
    auto *textPointer = text.data();
    Codec::Options same;
    auto textResult = Codec::transcode(QString::fromRawData(text.constData(), text.size()), same);
    REQUIRE(textResult);
    textPointer[0] = 'X';
    CHECK(*textResult.source == QStringLiteral("retained^I"));
    QByteArray bytes("retained\0^I", 11);
    auto *bytePointer = bytes.data();
    auto byteResult = Codec::transcode(QByteArray::fromRawData(bytes.constData(), bytes.size()), same);
    REQUIRE(byteResult);
    bytePointer[0] = 'X';
    CHECK(*byteResult.source == QByteArray("retained\0^I", 11));
}

TEST_CASE("Plain Arabic carets convert to exact controls and canonical DXF spellings", "[text-source-codec][headless][controls]") {
    const auto source = QStringLiteral("بَ^Iت^Jج^M^Jد^ هـ");
    const auto target = QStringLiteral("بَ\tت\nج\r\nد^هـ");
    const auto actual = converted(source, target, options(false, Syntax::Plain));
    const auto document = parsed(actual, false, Syntax::Plain);
    REQUIRE(document.paragraphs.size() == 3);
    CHECK(document.paragraphs[0].text == QStringLiteral("بَ\tت"));
    CHECK(document.paragraphs[1].text == QStringLiteral("ج"));
    CHECK(document.paragraphs[2].text == QStringLiteral("د^هـ"));
    converted(QStringLiteral("ب\tت\r\nج^ هـ"), QStringLiteral("ب^Iت^M^Jج^  هـ"), options(true, Syntax::Plain));
    for (const auto syntax : {Syntax::Plain, Syntax::MText}) {
        QString controls, encoded;
        for (ushort unit = 0; unit != 32; ++unit) {
            controls += QChar(unit); controls += QChar(0x0628);
            encoded += QChar('^'); encoded += QChar(unit + 64); encoded += QChar(0x0628);
        }
        converted(controls, encoded, options(true, syntax));
        converted(encoded, controls, options(false, syntax));
    }
}

TEST_CASE("Every active ASCII caret pair retains its independently specified modulo value", "[text-source-codec][headless][ascii]") {
    for (ushort next = 0; next != 128; ++next) {
        if (next == '\r' || next == '\n') continue; // These are literal paragraph boundaries, not pairs.
        INFO("ASCII follower " << next);
        const auto value = ushort(next == ' ' ? '^' : next < 64 ? next + 62 : next - 64);
        const QString input = QString(QChar('^')) + QChar(next) + QStringLiteral("ب");
        auto plain = options(false, Syntax::Plain);
        plain.decodeDxfEscapes = false;
        converted(input, QString(QChar(value)) + QStringLiteral("ب"), plain);
        const auto output = converted(input, mtextLiteral(value) + QStringLiteral("ب"), options(false));
        const auto document = parsed(output, false);
        if (value == '\r' || value == '\n') {
            REQUIRE(document.paragraphs.size() == 2);
            CHECK(document.paragraphs[0].text.isEmpty());
            CHECK(document.paragraphs[1].text == QStringLiteral("ب"));
        } else {
            REQUIRE(document.paragraphs.size() == 1);
            CHECK(document.paragraphs[0].text == QString(QChar(value)) + QStringLiteral("ب"));
        }
    }
    for (const auto separator : {QChar('\r'), QChar('\n'), QChar(0x2028), QChar(0x2029)}) {
        const QString input = QString(QChar('^')) + separator + QStringLiteral("ب");
        converted(input, input, options(false));
    }
    for (const auto &input : {QStringLiteral("^"), QStringLiteral("^ب"), QString::fromUcs4(U"^\U0001f600")})
        converted(input, input, options(false));
}

TEST_CASE("Formatted Arabic source keeps font metrics paint scopes escapes and paragraph semantics", "[text-source-codec][headless][format]") {
    const auto source = QStringLiteral("{\\fAmiri|b0|i0;\\H2x;\\W.5;\\Q12;\\C1;\\Lبَ^Iت\\l\\Pج^ هـ}\\~\\\\\\{\\}\\;");
    const auto expected = QStringLiteral("{\\fAmiri|b0|i0;\\H2x;\\W.5;\\Q12;\\C1;\\Lبَ\tت\\l\\Pج\\^هـ}\\~\\\\\\{\\}\\;");
    const auto output = converted(source, expected, options(false));
    const auto document = parsed(output, false);
    REQUIRE(document.paragraphs.size() == 2);
    const auto &first = document.paragraphs[0];
    CHECK(first.text == QStringLiteral("بَ\tت"));
    REQUIRE(first.styleAt(0));
    CHECK(first.styleAt(0)->font == QStringLiteral("Amiri"));
    CHECK(first.styleAt(0)->fontSpecification == QStringLiteral("Amiri|b0|i0"));
    CHECK(first.styleAt(0)->heightFactor == 2);
    CHECK(first.styleAt(0)->widthFactor == .5);
    CHECK(first.styleAt(0)->obliqueDegrees == 12);
    CHECK(first.styleAt(0)->color == 1);
    CHECK(first.styleAt(0)->underline);
    CHECK(document.paragraphs[1].text == QStringLiteral("ج^هـ\u00a0\\{};"));
    REQUIRE(document.paragraphs[1].styleAt(0));
    CHECK_FALSE(document.paragraphs[1].styleAt(0)->underline);
    CHECK(document.paragraphs[1].styleAt(0)->heightFactor == 2);
    CHECK(document.paragraphs[1].styleAt(document.paragraphs[1].text.size() - 1)->heightFactor == 1);
    converted(QStringLiteral("{\\f{Amiri}\\H2xب\tت}\\Pج"), QStringLiteral("{\\f{Amiri}\\H2xب^Iت}\\Pج"), options(true));
    converted(QStringLiteral("\\^I\\{\\}\\\\^I\\U+0628%%C"), QStringLiteral("\\^I\\{\\}\\\\\t\\U+0628%%C"), options(false));
}

TEST_CASE("Stack separators and padding remain raw while scoped and nested child text converts", "[text-source-codec][headless][stack]") {
    const auto input = QStringLiteral("\\S{\\fAmiri;ب^Iت}^ {\\C2;ج^aد};");
    const auto expected = QStringLiteral("\\S{\\fAmiri;ب\tت}^ {\\C2;ج!د};");
    const auto output = converted(input, expected, options(false));
    const auto document = parsed(output, false);
    REQUIRE(document.paragraphs[0].objects.size() == 1);
    const auto &object = document.paragraphs[0].objects[0];
    CHECK(object.separator == '^');
    CHECK(object.upper->paragraphs[0].text == QStringLiteral("ب\tت"));
    CHECK(object.upper->paragraphs[0].styleAt(0)->font == QStringLiteral("Amiri"));
    CHECK(object.lower->paragraphs[0].text == QStringLiteral("ج!د"));
    CHECK(object.lower->paragraphs[0].styleAt(0)->color == 2);
    converted(QStringLiteral("\\S{\\S{ب^Iت}/ج;}#{\\W2;د^?هـ};"),
              QStringLiteral("\\S{\\S{ب\tت}/ج;}#{\\W2;د\\}هـ};"), options(false));
    converted(QStringLiteral("\\S{ب^Iت}^ ج;"), QStringLiteral("\\S{ب^ Iت}^ ج;"), options(true));
    for (const auto &source : {QStringLiteral("\\Sب^ ج;"), QStringLiteral("\\Sب/ج;"), QStringLiteral("\\Sب#ج;")}) {
        converted(source, source, options(false));
        converted(source, source, options(true));
    }
}

TEST_CASE("Decoded literals cannot activate syntax or fuse with a numeric format argument", "[text-source-codec][headless][lexing]") {
    for (const auto &command : {QStringLiteral("\\H2"), QStringLiteral("\\W2"), QStringLiteral("\\Q2"), QStringLiteral("\\C2"), QStringLiteral("\\c2")}) {
        const auto output = converted(command + QStringLiteral("^xب"), command + QStringLiteral("\\U+0038ب"), options(false));
        const auto document = parsed(output, false);
        CHECK(document.paragraphs[0].text == QStringLiteral("8ب"));
        REQUIRE(document.paragraphs[0].styleAt(0));
        if (command.startsWith(QStringLiteral("\\H"))) CHECK(document.paragraphs[0].styleAt(0)->heightFactor == 2);
        if (command.startsWith(QStringLiteral("\\W"))) CHECK(document.paragraphs[0].styleAt(0)->widthFactor == 2);
        auto noEscapes = options(false); noEscapes.decodeDxfEscapes = false;
        converted(command + QStringLiteral("^xب"), command + QStringLiteral("{8}ب"), noEscapes);
    }
    const auto exponent = converted(QStringLiteral("\\H2e^x"), QStringLiteral("\\H2;e8"), options(false));
    const auto exponentDocument = parsed(exponent, false);
    CHECK(exponentDocument.paragraphs[0].text == QStringLiteral("e8"));
    CHECK(exponentDocument.paragraphs[0].styleAt(0)->heightFactor == 2);
    converted(QStringLiteral("\\H2e^a"), QStringLiteral("\\H2e!"), options(false));
    converted(QStringLiteral("\\H2E^ " ), QStringLiteral("\\H2;E\\^"), options(false));
    converted(QStringLiteral("\\W2^e^eC"), QStringLiteral("\\W2\\U+0025\\U+0025C"), options(false));
    converted(QStringLiteral("^e^eC"), QStringLiteral("\\U+0025\\U+0025C"), options(false, Syntax::Plain));
    const QString slash = QString(QChar('^')) + QChar(30) + QStringLiteral("U+0628");
    converted(slash, QStringLiteral("\\U+005CU+0628"), options(false, Syntax::Plain));
    converted(slash, QStringLiteral("\\\\U+0628"), options(false));
}

TEST_CASE("Opaque insensitive properties are retained and sensitive ambiguity fails atomically", "[text-source-codec][headless][unknown]") {
    converted(QStringLiteral("{\\Zopaque;ب}^Iت"), QStringLiteral("{\\Zopaque;ب}\tت"), options(false));
    converted(QStringLiteral("\\H1..2;ب^Iت"), QStringLiteral("\\H1..2;ب\tت"), options(false));
    converted(QStringLiteral("\\fFont^I;ب"), QStringLiteral("\\fFont^I;ب"), options(false));
    for (const auto &source : {QStringLiteral("\\Z^I;ب"), QStringLiteral("\\Zbad^I"), QStringLiteral("\\Sbad\\^;"), QStringLiteral("\\f{bad^I")})
        failed(source, options(false), Code::AmbiguousSyntax);
    failed(QStringLiteral("\\Z\t;ب"), options(true), Code::AmbiguousSyntax);
    failed(QStringLiteral("\\fFont\t;ب"), options(true), Code::AmbiguousSyntax);
    failed(QStringLiteral("\\fFont^J;ب"), options(true), Code::SemanticMismatch);
}

TEST_CASE("Signed incomplete exponents stay malformed literal source rather than activating a numeric format", "[text-source-codec][headless][signed-exponent]") {
    for (const auto &command : {QStringLiteral("\\H2"), QStringLiteral("\\W2"), QStringLiteral("\\Q2")}) {
        for (const auto &tail : {QStringLiteral("e+"), QStringLiteral("e-"), QStringLiteral("E+"), QStringLiteral("E-")}) {
            const QString prefix = command + tail;
            const QString input = prefix + QStringLiteral("^x");
            const auto original = parsed(input, true);
            REQUIRE(original.paragraphs.size() == 1);
            CHECK(original.paragraphs[0].text == prefix + QStringLiteral("8"));
            REQUIRE(original.paragraphs[0].styleAt(0));
            CHECK(original.paragraphs[0].styleAt(0)->heightFactor == 1);
            CHECK(original.paragraphs[0].styleAt(0)->widthFactor == 1);
            CHECK(original.paragraphs[0].styleAt(0)->obliqueDegrees == 0);
            REQUIRE(original.tokens.size() == 2);
            CHECK(original.tokens[0].kind == Document::TokenKind::Malformed);
            CHECK(original.tokens[0].source == Document::Range{0, int(prefix.size())});
            CHECK(original.tokens[1].kind == Document::TokenKind::Escape);
            converted(input, prefix + QStringLiteral("\\U+0038"), options(false));
            auto noEscapes = options(false); noEscapes.decodeDxfEscapes = false;
            converted(input, prefix + QStringLiteral("{8}"), noEscapes);
        }
    }
}

TEST_CASE("Integer color commands cannot fuse a literal exponent tail and need no delimiter rewrite", "[text-source-codec][headless][integer-tail]") {
    for (const auto &command : {QStringLiteral("\\C2"), QStringLiteral("\\c2")}) {
        for (const auto &tail : {QStringLiteral("e"), QStringLiteral("E")}) {
            for (const bool escapes : {false, true}) {
                auto settings = options(false); settings.decodeDxfEscapes = escapes;
                const auto output = converted(command + tail + QStringLiteral("^x"), command + tail + QStringLiteral("8"), settings);
                const auto document = parsed(output, false, Syntax::MText, escapes);
                REQUIRE(document.paragraphs.size() == 1);
                CHECK(document.paragraphs[0].text == tail + QStringLiteral("8"));
                REQUIRE(document.paragraphs[0].styleAt(0));
                CHECK(document.paragraphs[0].styleAt(0)->color == 2);
                CHECK(document.paragraphs[0].styleAt(0)->trueColor == command.startsWith(QStringLiteral("\\c")));
            }
        }
    }
}

TEST_CASE("Relative height and width suffixes finish numeric grammar without a delimiter rewrite", "[text-source-codec][headless][relative-tail]") {
    for (const auto &command : {QStringLiteral("\\H2"), QStringLiteral("\\W2")}) {
        const bool height = command.startsWith(QStringLiteral("\\H"));
        const auto initial = height ? QStringLiteral("\\H3;") : QStringLiteral("\\W3;");
        for (const auto &suffix : {QStringLiteral("x"), QStringLiteral("X")}) {
            for (const auto &tail : {QStringLiteral("e"), QStringLiteral("E")}) {
                for (const bool escapes : {false, true}) {
                    auto settings = options(false); settings.decodeDxfEscapes = escapes;
                    const QString prefix = initial + command + suffix + tail;
                    const auto output = converted(prefix + QStringLiteral("^xب"), prefix + QStringLiteral("8ب"), settings);
                    const auto document = parsed(output, false, Syntax::MText, escapes);
                    REQUIRE(document.paragraphs.size() == 1);
                    CHECK(document.paragraphs[0].text == tail + QStringLiteral("8ب"));
                    REQUIRE(document.paragraphs[0].styleAt(0));
                    CHECK(document.paragraphs[0].styleAt(0)->heightFactor == (height ? 6 : 1));
                    CHECK(document.paragraphs[0].styleAt(0)->widthFactor == (height ? 1 : 6));
                    CHECK(document.paragraphs[0].styleAt(0)->obliqueDegrees == 0);
                }
            }
        }
    }
}

TEST_CASE("Every active ASCII caret follower is safely quotable after known numeric MTEXT commands", "[text-source-codec][headless][numeric-ascii]") {
    const QString numericExtensions = QStringLiteral("0123456789.+-eExX");
    for (const auto &command : {QStringLiteral("\\H2"), QStringLiteral("\\W2"), QStringLiteral("\\Q2"), QStringLiteral("\\C2"), QStringLiteral("\\c2")}) {
        for (const bool escapes : {false, true}) {
            auto settings = options(false); settings.decodeDxfEscapes = escapes;
            for (ushort follower = 0; follower != 128; ++follower) {
                if (follower == '\r' || follower == '\n') continue;
                INFO("command " << command.toStdString() << ", ASCII follower " << follower << ", escapes " << escapes);
                const ushort value = follower == ' ' ? '^' : follower < 64 ? follower + 62 : follower - 64;
                QString spelling;
                switch (value) {
                case '\\': spelling = QStringLiteral("\\\\"); break;
                case '{': spelling = QStringLiteral("\\{"); break;
                case '}': spelling = QStringLiteral("\\}"); break;
                case ';': spelling = QStringLiteral("\\;"); break;
                case '^': spelling = QStringLiteral("\\^"); break;
                default:
                    if (numericExtensions.contains(QChar(value))) {
                        if (escapes) spelling = QStringLiteral("\\U+") + QString::number(value, 16).rightJustified(4, QChar('0')).toUpper();
                        else spelling = QStringLiteral("{") + QChar(value) + QStringLiteral("}");
                    } else if (value == '%' && escapes) spelling = QStringLiteral("\\U+0025");
                    else spelling = QString(QChar(value));
                }
                const QString input = command + QChar('^') + QChar(follower) + QStringLiteral("ب");
                const auto output = converted(input, command + spelling + QStringLiteral("ب"), settings);
                const auto document = parsed(output, false, Syntax::MText, escapes);
                const bool boundary = value == '\r' || value == '\n';
                REQUIRE(document.paragraphs.size() == (boundary ? 2 : 1));
                CHECK(document.paragraphs.back().text == (boundary ? QStringLiteral("ب") : QString(QChar(value)) + QStringLiteral("ب")));
                REQUIRE(document.paragraphs.back().styleAt(0));
                const auto &style = *document.paragraphs.back().styleAt(0);
                CHECK(style.heightFactor == (command.startsWith(QStringLiteral("\\H")) ? 2 : 1));
                CHECK(style.widthFactor == (command.startsWith(QStringLiteral("\\W")) ? 2 : 1));
                CHECK(style.obliqueDegrees == (command.startsWith(QStringLiteral("\\Q")) ? 2 : 0));
                CHECK(style.color == (command.startsWith(QStringLiteral("\\C")) || command.startsWith(QStringLiteral("\\c")) ? 2 : -1));
                CHECK(style.trueColor == command.startsWith(QStringLiteral("\\c")));
            }
        }
    }
}

TEST_CASE("UTF8 conversion preserves BOM NUL and invalid unaffected bytes without normalization", "[text-source-codec][headless][bytes]") {
    const QByteArray bom("\xEF\xBB\xBF");
    const QByteArray input = bom + QStringLiteral("ب^Iت").toUtf8() + QByteArray("\0", 1);
    const QByteArray expected = bom + QStringLiteral("ب\tت").toUtf8() + QByteArray("\0", 1);
    const auto output = Codec::transcode(input, options(false));
    INFO(output.error.message.toStdString());
    REQUIRE(output);
    CHECK(*output.source == expected);
    CHECK(output.source->endsWith(QByteArray("\0", 1)));
    for (const auto &raw : {QByteArray("A\xFF\0B", 4), QByteArray("{\\Z\xFF;}", 7)}) {
        const auto result = Codec::transcode(raw, options(false));
        REQUIRE(result);
        CHECK(*result.source == raw);
        CHECK_FALSE(result.changed);
    }
    const QByteArray invalid("A\xFF^I", 4);
    const auto rejected = Codec::transcode(invalid, options(false));
    CHECK_FALSE(rejected);
    CHECK_FALSE(rejected.source);
    CHECK(rejected.error.code == Code::InvalidUtf8);
    const auto invalidControl = Codec::transcode(QByteArray("A\xFF\t", 3), options(true));
    CHECK_FALSE(invalidControl);
    CHECK_FALSE(invalidControl.source);
    CHECK(invalidControl.error.code == Code::InvalidUtf8);
    QString malformed = QStringLiteral("ب"); malformed += QChar(0xd800); malformed += QStringLiteral("^Iت");
    QString malformedExpected = QStringLiteral("ب"); malformedExpected += QChar(0xd800); malformedExpected += QStringLiteral("\tت");
    const auto valueResult = Codec::transcode(malformed, options(false));
    REQUIRE(valueResult);
    CHECK(*valueResult.source == malformedExpected);
}

TEST_CASE("Codec budgets validate complete outputs and recursive work before successful publication", "[text-source-codec][headless][limits]") {
    auto settings = options(true);
    settings.maxSourceUnits = 1;
    failed(QStringLiteral("ب\t"), settings, Code::ResourceLimit);
    settings = options(true); settings.maxOutputUnits = 2;
    failed(QStringLiteral("\t\t"), settings, Code::ResourceLimit);
    settings.maxOutputUnits = 4;
    converted(QStringLiteral("\t\t"), QStringLiteral("^I^I"), settings);
    settings = options(false); settings.maxTraversalNodes = 0;
    failed(QStringLiteral("^I"), settings, Code::ResourceLimit);
    settings = options(false); settings.maxStackDepth = 0;
    failed(QStringLiteral("\\S{ب^I}/ج;"), settings, Code::ResourceLimit);
    settings = options(false); settings.maxScopeDepth = 0;
    failed(QStringLiteral("{ب^I}"), settings, Code::ResourceLimit);
    settings = options(false); settings.maxDiagnostics = 0;
    converted(QStringLiteral("\\Zopaque;^I"), QStringLiteral("\\Zopaque;\t"), settings);
    failed(QStringLiteral("\\!^I"), settings, Code::ResourceLimit);
    settings = options(true); settings.maxOutputBytes = 1;
    const auto bytes = Codec::transcode(QByteArray("\t"), settings);
    CHECK_FALSE(bytes); CHECK_FALSE(bytes.source); CHECK(bytes.error.code == Code::ResourceLimit);
    settings = options(false); settings.maxSourceBytes = 1;
    const auto sourceBytes = Codec::transcode(QByteArray("^I"), settings);
    CHECK_FALSE(sourceBytes); CHECK_FALSE(sourceBytes.source); CHECK(sourceBytes.error.code == Code::ResourceLimit);
    for (int field = 0; field != 11; ++field) {
        settings = options(false);
        switch (field) {
        case 0: settings.syntax = static_cast<Syntax>(99); break;
        case 1: settings.maxSourceUnits = -1; break;
        case 2: settings.maxOutputUnits = -1; break;
        case 3: settings.maxSourceBytes = -1; break;
        case 4: settings.maxOutputBytes = -1; break;
        case 5: settings.maxScopeDepth = -1; break;
        case 6: settings.maxStackDepth = -1; break;
        case 7: settings.maxDiagnostics = -1; break;
        case 8: settings.maxTraversalNodes = -1; break;
        case 9: settings.maxStackDepth = 65; break;
        default: settings.maxStackDepth = std::numeric_limits<int>::max(); break;
        }
        failed(QStringLiteral("^I"), settings, Code::InvalidOptions);
        const auto badBytes = Codec::transcode(QByteArray("^I"), settings);
        CHECK_FALSE(badBytes); CHECK_FALSE(badBytes.source); CHECK(badBytes.error.code == Code::InvalidOptions);
    }
}

TEST_CASE("Seeded text corpus independently predicts canonical caret output", "[text-source-codec][headless][property]") {
    std::mt19937 random(0x06280644);
    const ushort alphabet[] = {0, 1, 9, 10, 13, 30, 31, 'A', '0', '^', ' ', 0x0628, 0x064e, 0x200d, 0x200c, 0x00a0, 0xfe8f};
    for (int sample = 0; sample != 200; ++sample) {
        QString raw, expected;
        for (int index = 0; index != 100; ++index) {
            const ushort unit = alphabet[random() % (sizeof(alphabet) / sizeof(alphabet[0]))];
            raw += QChar(unit);
            if (unit == '^') expected += QStringLiteral("^ ");
            else if (unit < 32) { expected += QChar('^'); expected += QChar(unit + 64); }
            else expected += QChar(unit);
        }
        converted(raw, expected, options(true, Syntax::Plain));
    }
}

TEST_CASE("Consecutive CR then CRLF has two boundaries in raw and canonical caret spelling", "[text-source-codec][headless][paragraph-regression]") {
    const auto raw = QStringLiteral("A\r\r\nB");
    const auto caret = QStringLiteral("A^M^M^JB");
    const auto rawDocument = parsed(raw, false, Syntax::Plain);
    const auto caretDocument = parsed(caret, true, Syntax::Plain);
    REQUIRE(rawDocument.paragraphs.size() == 3);
    REQUIRE(caretDocument.paragraphs.size() == 3);
    CHECK(rawDocument.paragraphs[0].text == QStringLiteral("A"));
    CHECK(rawDocument.paragraphs[1].text.isEmpty());
    CHECK(rawDocument.paragraphs[2].text == QStringLiteral("B"));
    converted(raw, caret, options(true, Syntax::Plain));
    converted(caret, raw, options(false, Syntax::Plain));
}
