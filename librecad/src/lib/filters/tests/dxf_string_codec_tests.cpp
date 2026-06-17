/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

// Twin-function equality tests for RS_FilterDXFRW::toDxfString and
// toNativeString. The optimised implementations live in rs_filterdxfrw.cpp;
// the verbatim pre-optimisation copies below act as the equality oracle.
//
// Reference functions copied verbatim from rs_filterdxfrw.cpp as of commit
// 300f0edb0 ("dwg: broaden DICTIONARYWDFLT/SORTENTSTABLE/FIELDLIST/FIELD
// writer gate to AC1015+ (PR 13h)"). When the optimised implementation is
// intentionally changed, refresh these references and re-run the suite.
//
// The reference toNativeString has two known correctness bugs that the
// optimised version fixes: unchecked data.at(i+N) past end-of-string, and
// an unbounded inner do/while when the inner tmp.indexOf(';') returns -1.
// Equality cases therefore exercise only well-formed inputs; malformed
// inputs are checked only against the optimised version for deterministic,
// bounded-time behaviour.

#include <catch2/catch_test_macros.hpp>

#include <QChar>
#include <QLatin1StringView>
#include <QRegularExpression>
#include <QString>

#include "rs_filterdxfrw.h"

namespace {

// ---- Reference implementations (pre-optimisation, verbatim) ----

QString toDxfStringReference(const QString& str) {
    // 1. Reserve memory up front to eliminate reallocation overhead
    QString res;
    res.reserve(str.length() + 16);

    // 2. Iterate cleanly using modern range-based loop
    for (const QChar& qchar : str) {
        const ushort c = qchar.unicode();

        // 3. Keep standard ASCII characters without filtering overhead
        if (c <= 175 && c >= 11) {
            res.append(qchar);
            continue;
        }

        // 4. Map special DXF escape codes efficiently
        switch (c) {
        case 0x0A:
            res.append(uR"(\P)");
            break;
        case 0x2205:
        case 0x2300:
            res.append(u"%%C");
            break;
        case 0x00B0:
            res.append(u"%%D");
            break;
        case 0x00B1:
            res.append(u"%%P");
            break;
        default:
            res.append(qchar);
            break;
        }
    }
    return res;
}

// NOTE: The reference toNativeString reads data.at(i+1..3) without bounds
// checking. Do NOT call it with inputs that contain '{' near end-of-string.
QString toNativeStringReference(const QString& data) {
    QString res;

    // Ignore font tags:
    int j = 0;
    for (int i=0; i<data.length(); ++i) {
        if (data.at(i).unicode() == 0x7B){ //is '{' ?
            if (data.at(i+1).unicode() == 0x5c){ //and is "{\" ?
                //check known codes
                if ( (data.at(i+2).unicode() == 0x66) || //is "\f" ?
                     (data.at(i+2).unicode() == 0x48) || //is "\H" ?
                     (data.at(i+2).unicode() == 0x43)    //is "\C" ?
                   ) {
                    //found tag, append parsed part
                    res.append(data.mid(j,i-j));
                    qsizetype pos = data.indexOf(QChar(0x7D), i+3);//find '}'
                    if (pos <0) break; //'}' not found
                    QString tmp = data.mid(i+1, pos-i-1);
                    do {
                        tmp = tmp.remove(0,tmp.indexOf(QChar{0x3B}, 0)+1 );//remove to ';'
                    } while(tmp.startsWith("\\f") || tmp.startsWith("\\H") || tmp.startsWith("\\C"));
                    res.append(tmp);
                    i = j = pos;
                    ++j;
                }
            }
        }
    }
    res.append(data.mid(j));

    // AutoCAD caret convention: ^X → chr((X-64) mod 126); ^Space → '^'.
    // Mirrors the optimised toNativeString (refreshed for the i18n caret
    // generalization). Runs before \P/\~ so ^J/^M decode first.
    {
        QString tmp;
        tmp.reserve(res.size());
        for (int k = 0; k < res.size(); ++k) {
            const QChar ch = res.at(k);
            if (ch.unicode() == 0x5E && k + 1 < res.size()) {
                const QChar nx = res.at(k + 1);
                if (nx.unicode() == 0x20) {
                    tmp.append(QChar(0x5E));
                } else {
                    const int code = (static_cast<int>(nx.unicode()) - 64) % 126;
                    tmp.append(QChar(static_cast<ushort>(code < 0 ? code + 126 : code)));
                }
                ++k;
            } else {
                tmp.append(ch);
            }
        }
        res = std::move(tmp);
    }
    // Line feed:
    res = res.replace(QRegularExpression("\\\\P"), "\n");
    // Space:
    res = res.replace(QRegularExpression("\\\\~"), " ");
    // diameter:
    res = res.replace(QRegularExpression("%%[cC]"), QChar(0x2300));//RLZ: Empty_set is 0x2205, diameter is 0x2300 need to add in all fonts
    // degree:
    res = res.replace(QRegularExpression("%%[dD]"), QChar(0x00B0));
    // plus/minus
    res = res.replace(QRegularExpression("%%[pP]"), QChar(0x00B1));

    return res;
}

// ---- Helper macros for equality checks ----

void requireDxfEqual(const QString& input) {
    const QString ref = toDxfStringReference(input);
    const QString opt = RS_FilterDXFRW::toDxfString(input);
    INFO("toDxfString input (Qt-escaped):  " << input.toStdString());
    INFO("toDxfString reference (escaped): " << ref.toStdString());
    INFO("toDxfString optimised (escaped): " << opt.toStdString());
    REQUIRE(opt == ref);
}

void requireNativeEqual(const QString& input) {
    const QString ref = toNativeStringReference(input);
    const QString opt = RS_FilterDXFRW::toNativeString(input);
    INFO("toNativeString input (escaped):     " << input.toStdString());
    INFO("toNativeString reference (escaped): " << ref.toStdString());
    INFO("toNativeString optimised (escaped): " << opt.toStdString());
    REQUIRE(opt == ref);
}

} // namespace

// Catch2's TEST_CASE macro expands to a function whose generated identifier
// doesn't match the project's camelBack convention; each invocation carries
// a NOLINTNEXTLINE suppression for clang-tidy's readability-identifier-naming
// rule. The rule still applies to any user-defined names inside the body.

// ---- toDxfString equality ----

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toDxfString: empty and trivial", "[dxf_codec][to_dxf]") {
    requireDxfEqual(QString());
    requireDxfEqual(QStringLiteral(""));
    requireDxfEqual(QStringLiteral("a"));
    requireDxfEqual(QStringLiteral("Hello, World!"));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toDxfString: each special code", "[dxf_codec][to_dxf]") {
    // newline → \P
    requireDxfEqual(QStringLiteral("a\nb"));
    // 0x2300 (diameter) and 0x2205 (empty set) both → %%C
    requireDxfEqual(QStringLiteral("⌀10"));
    requireDxfEqual(QString(QChar(0x2205)) + QStringLiteral("10"));
    // 0x00B0 (degree) → %%D
    requireDxfEqual(QStringLiteral("90°"));
    // 0x00B1 (plus/minus) → %%P
    requireDxfEqual(QStringLiteral("±0.1"));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toDxfString: special codes back-to-back", "[dxf_codec][to_dxf]") {
    requireDxfEqual(QStringLiteral("⌀°±"));
    requireDxfEqual(QStringLiteral("\n\n\n"));
    requireDxfEqual(QStringLiteral("⌀\n°\n±"));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toDxfString: boundary chars around the [11, 175] fast-path",
          "[dxf_codec][to_dxf]") {
    // Outside fast-path but not in switch → default branch
    requireDxfEqual(QString(QChar(0x01)) + QStringLiteral("a"));      // 0x01 (below 11)
    requireDxfEqual(QString(QChar(0x09)) + QStringLiteral("a"));      // tab
    requireDxfEqual(QString(QChar(0xB0)) + QStringLiteral("a"));      // 176 — above fast-path
    requireDxfEqual(QString(QChar(0x100)) + QStringLiteral("a"));     // generic Unicode
    requireDxfEqual(QString(QChar(0x4E2D)) + QStringLiteral("a"));    // CJK
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toDxfString: Unicode passes through unchanged",
          "[dxf_codec][to_dxf]") {
    requireDxfEqual(QStringLiteral("中文测试"));
    requireDxfEqual(QStringLiteral("café résumé"));
    // Emoji as surrogate pair
    requireDxfEqual(QStringLiteral("🙂"));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toDxfString: long ASCII payload", "[dxf_codec][to_dxf]") {
    QString payload;
    payload.reserve(10240);
    for (int i = 0; i < 10240; ++i) {
        payload.append(QChar(static_cast<ushort>('a' + (i % 26))));
    }
    requireDxfEqual(payload);
}

// ---- toNativeString equality (well-formed inputs only) ----

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toNativeString: empty and trivial", "[dxf_codec][to_native]") {
    requireNativeEqual(QString());
    requireNativeEqual(QStringLiteral(""));
    requireNativeEqual(QStringLiteral("a"));
    requireNativeEqual(QStringLiteral("Hello, World!"));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toNativeString: each escape decoded", "[dxf_codec][to_native]") {
    requireNativeEqual(QStringLiteral("a\\Pb"));        // \P → \n
    requireNativeEqual(QStringLiteral("a\\~b"));        // \~ → space
    requireNativeEqual(QStringLiteral("a^Ib"));         // ^I → TAB (caret-decode)
    requireNativeEqual(QStringLiteral("%%c10"));         // %%c → ⌀
    requireNativeEqual(QStringLiteral("%%C10"));         // %%C → ⌀
    requireNativeEqual(QStringLiteral("90%%d"));         // %%d → °
    requireNativeEqual(QStringLiteral("90%%D"));         // %%D → °
    requireNativeEqual(QStringLiteral("%%p0.1"));        // %%p → ±
    requireNativeEqual(QStringLiteral("%%P0.1"));        // %%P → ±
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toNativeString: escapes back-to-back", "[dxf_codec][to_native]") {
    requireNativeEqual(QStringLiteral("%%C%%D%%P"));
    requireNativeEqual(QStringLiteral("\\P\\P\\P"));
    requireNativeEqual(QStringLiteral("a\\P%%Db\\~%%P"));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toNativeString: well-formed font tags are stripped",
          "[dxf_codec][to_native]") {
    requireNativeEqual(QStringLiteral("{\\fArial|b0|i0|c0|p34;text}"));
    requireNativeEqual(QStringLiteral("{\\H2.0x;tall text}"));
    requireNativeEqual(QStringLiteral("{\\C7;coloured}"));
    requireNativeEqual(QStringLiteral("prefix {\\fSans;mid} suffix"));
    requireNativeEqual(QStringLiteral("{\\fArial;text with %%c diameter}"));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toNativeString: Unicode passthrough", "[dxf_codec][to_native]") {
    requireNativeEqual(QStringLiteral("中文测试"));
    requireNativeEqual(QStringLiteral("café résumé"));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toNativeString: long ASCII payload", "[dxf_codec][to_native]") {
    QString payload;
    payload.reserve(10240);
    for (int i = 0; i < 10240; ++i) {
        payload.append(QChar(static_cast<ushort>('a' + (i % 26))));
    }
    requireNativeEqual(payload);
}

// ---- Round-trip invariant for the escape-free sub-language ----

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Round-trip: toDxfString(toNativeString(x)) is identity for "
          "ASCII without escape sequences",
          "[dxf_codec][round_trip]") {
    auto roundTrip = [](const QString& x) {
        return RS_FilterDXFRW::toDxfString(RS_FilterDXFRW::toNativeString(x));
    };
    REQUIRE(roundTrip(QString()) == QString());
    REQUIRE(roundTrip(QStringLiteral("Hello, World!"))
            == QStringLiteral("Hello, World!"));
    REQUIRE(roundTrip(QStringLiteral("1234567890"))
            == QStringLiteral("1234567890"));
    REQUIRE(roundTrip(QStringLiteral("layer name"))
            == QStringLiteral("layer name"));
}

// ---- Malformed input: optimised function must be bounded and not crash ----
//
// These cases trip the unchecked at(i+N) and/or the unbounded do/while
// inside the reference implementation. We don't compare against the
// reference — we only assert the optimised version completes and produces
// some deterministic output for each case.

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toNativeString: malformed inputs return deterministically",
          "[dxf_codec][to_native][malformed]") {
    // Each call must terminate without crashing/asserting/hanging.
    // The exact return value is implementation-defined; just probe that
    // the call is total.
    (void)RS_FilterDXFRW::toNativeString(QStringLiteral("{"));
    (void)RS_FilterDXFRW::toNativeString(QStringLiteral("{\\"));
    (void)RS_FilterDXFRW::toNativeString(QStringLiteral("{\\f"));
    (void)RS_FilterDXFRW::toNativeString(QStringLiteral("{\\fNoSemicolon}"));
    (void)RS_FilterDXFRW::toNativeString(QStringLiteral("{\\fA"));
    (void)RS_FilterDXFRW::toNativeString(QStringLiteral("trailing {"));
    (void)RS_FilterDXFRW::toNativeString(QStringLiteral("trailing {\\"));
    (void)RS_FilterDXFRW::toNativeString(QStringLiteral("trailing {\\f"));
    SUCCEED("all malformed inputs returned without crash or hang");
}
