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

#include <algorithm>
#include <utility>
#include <vector>

#include <QChar>
#include <QLatin1StringView>
#include <QRegularExpression>
#include <QString>

#include "rs_filterdxfrw.h"
#include "rs_mtext.h"

namespace {

// ---- Reference implementations (pre-optimisation; toDxfString since
//      caret-encodes control characters and carets) ----

QString toDxfStringReference(const QString& str) {
    // 1. Reserve memory up front to eliminate reallocation overhead
    QString res;
    res.reserve(str.length() + 16);

    // 2. Iterate cleanly using modern range-based loop
    for (const QChar& qchar : str) {
        const ushort c = qchar.unicode();

        // 3. Keep standard ASCII characters without filtering overhead
        if (c <= 175 && c >= 0x20 && c != 0x5E) {
            res.append(qchar);
            continue;
        }

        // 4. Map special DXF escape codes efficiently
        switch (c) {
        case 0x0A:
            res.append(uR"(\P)");
            break;
        case 0x5E: // a literal caret
            res.append(u"^ ");
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
            if (c < 0x20) { // other control characters: caret codes
                res.append(QChar(0x5E));
                res.append(QChar(static_cast<ushort>(c + 0x40)));
            } else {
                res.append(qchar);
            }
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
TEST_CASE("toDxfString: control characters and carets become caret codes",
          "[dxf_codec][to_dxf]") {
    CHECK(RS_FilterDXFRW::toDxfString(QStringLiteral("x\ty")) == QStringLiteral("x^Iy"));
    CHECK(RS_FilterDXFRW::toDxfString(QStringLiteral("c\rd")) == QStringLiteral("c^Md"));
    CHECK(RS_FilterDXFRW::toDxfString(QString(QChar(0)) + QStringLiteral("z")) == QStringLiteral("^@z"));
    CHECK(RS_FilterDXFRW::toDxfString(QStringLiteral("p^q")) == QStringLiteral("p^ q"));
    // toNativeString reads them back
    for (const QString& text : {QStringLiteral("x\ty"), QStringLiteral("c\rd"), QStringLiteral("p^q"),
                                QStringLiteral("^^\t^")}) {
        CHECK(RS_FilterDXFRW::toNativeString(RS_FilterDXFRW::toDxfString(text)) == text);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toDxfString: caretCodes=false leaves control characters and carets as-is",
          "[dxf_codec][to_dxf]") {
    // A DWG string holds the characters themselves; caret codes are an ASCII
    // DXF-only spelling for what it cannot hold directly (see the caretCodes
    // parameter doc). Every RS_FilterDXFRW call site that may write either
    // format passes caretCodes = (m_dwgW == nullptr).
    CHECK(RS_FilterDXFRW::toDxfString(QStringLiteral("x\ty"), false) == QStringLiteral("x\ty"));
    CHECK(RS_FilterDXFRW::toDxfString(QStringLiteral("c\rd"), false) == QStringLiteral("c\rd"));
    CHECK(RS_FilterDXFRW::toDxfString(QString(QChar(0)) + QStringLiteral("z"), false) ==
          QString(QChar(0)) + QStringLiteral("z"));
    CHECK(RS_FilterDXFRW::toDxfString(QStringLiteral("m^2"), false) == QStringLiteral("m^2"));
    // The LF-to-\P and %%D/%%C/%%P substitutions are MTEXT/text markup, not
    // an ASCII-container escape, so they still apply for either format.
    CHECK(RS_FilterDXFRW::toDxfString(QStringLiteral("a\nb"), false) == QStringLiteral(R"(a\Pb)"));
    CHECK(RS_FilterDXFRW::toDxfString(QString(QChar(0x00B0)), false) == QStringLiteral("%%D"));
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

// ---- Single-line text (TEXT, ATTRIB, ATTDEF): no MTEXT markup ----

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Single-line strings: MTEXT markup is ordinary text",
          "[dxf_codec][single_line]") {
    for (const QString& text : {QStringLiteral(R"(D:\PROJECTS\PLAN.dwg)"),
                                QStringLiteral(R"(C:\Program Files\x)"),
                                QStringLiteral(R"(\\SERVER\~TMP\A.dwg)"),
                                QStringLiteral(R"({\fArial|b0;Title})"),
                                QStringLiteral(R"({\H2.5x;tall}{\C1;red})")}) {
        CAPTURE(text.toStdString());
        CHECK(RS_FilterDXFRW::toNativeSingleLineString(text) == text);
        CHECK(RS_FilterDXFRW::toDxfSingleLineString(text) == text);
        CHECK(RS_FilterDXFRW::toDxfSingleLineString(text, false) == text);
    }
    // The same values are markup in MTEXT content.
    CHECK(RS_FilterDXFRW::toNativeString(QStringLiteral(R"(D:\PROJECTS\PLAN.dwg)")) ==
          QStringLiteral("D:\nROJECTS\nLAN.dwg"));
    CHECK(RS_FilterDXFRW::toNativeString(QStringLiteral(R"(\~TMP)")) == QStringLiteral(" TMP"));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Single-line strings: symbols and caret codes are converted",
          "[dxf_codec][single_line]") {
    CHECK(RS_FilterDXFRW::toNativeSingleLineString(QStringLiteral("45%%d %%C10 %%p0.1")) ==
          QStringLiteral("45\u00B0 \u230010 \u00B10.1"));
    CHECK(RS_FilterDXFRW::toNativeSingleLineString(QStringLiteral("A^IB^JC^MD^ E")) ==
          QStringLiteral("A\tB\nC\rD^E"));

    CHECK(RS_FilterDXFRW::toDxfSingleLineString(QStringLiteral("45\u00B0 \u230010 \u220510 \u00B10.1")) ==
          QStringLiteral("45%%D %%C10 %%C10 %%P0.1"));
    CHECK(RS_FilterDXFRW::toDxfSingleLineString(QStringLiteral("x\ty^z")) == QStringLiteral("x^Iy^ z"));
    CHECK(RS_FilterDXFRW::toDxfSingleLineString(QStringLiteral("x\ty^z"), false) == QStringLiteral("x\ty^z"));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Single-line strings: a line feed is a control character, not \\P",
          "[dxf_codec][single_line]") {
    // An ASCII DXF value cannot hold a line feed, and "\P" would read back as
    // a backslash and a P.
    CHECK(RS_FilterDXFRW::toDxfSingleLineString(QStringLiteral("a\nb")) == QStringLiteral("a^Jb"));
    CHECK(RS_FilterDXFRW::toDxfSingleLineString(QStringLiteral("a\nb"), false) == QStringLiteral("a\nb"));
    CHECK(RS_FilterDXFRW::toDxfString(QStringLiteral("a\nb")) == QStringLiteral(R"(a\Pb)"));

    for (const QString& text : {QStringLiteral("a\nb"), QStringLiteral("x\ty"), QStringLiteral("c\rd"),
                                QStringLiteral("p^q"), QStringLiteral("^^\t^"),
                                QStringLiteral(R"(D:\PROJECTS\PLAN.dwg)"),
                                QStringLiteral("45\u00B0 \u230010 \u00B10.1")}) {
        CAPTURE(text.toStdString());
        CHECK(RS_FilterDXFRW::toNativeSingleLineString(RS_FilterDXFRW::toDxfSingleLineString(text)) == text);
    }
}
// ---- RS_MText::toPlainLine: MTEXT content as the value of a single-line
//      ATTRIB ----

namespace {
// MTEXT content as RS_MText holds it, and the same on one line without codes.
const std::vector<std::pair<QString, QString>> &plainLineCases() {
    static const std::vector<std::pair<QString, QString>> cases = {
        {QString(), QString()},
        {QStringLiteral("ABC\nDEF"), QStringLiteral("ABC DEF")},
        {QStringLiteral("one\n\ntwo\n"), QStringLiteral("one two")},
        {QStringLiteral("a\r\nb"), QStringLiteral("a b")},
        {QStringLiteral("\n\tlead"), QStringLiteral(" lead")},
        {QStringLiteral("    ABC\n\tDEF \u03a9"), QStringLiteral("    ABC  DEF \u03a9")},
        {QStringLiteral(R"(\P\P)"), QString()},
        {QStringLiteral(R"({\fArial;})"), QString()},
        // the text of finding 11
        {QStringLiteral(R"({\fArial;big} \H2x;tall\Pnext 50%%d \S1^2;)"),
         QStringLiteral("big tall next 50\u00b0 1^2")},
        // a literal backslash, typed and as toNativeString() leaves "\\P"
        {QStringLiteral(R"(a\\Path)"), QStringLiteral(R"(a\Path)")},
        {QStringLiteral("a\\\nath"), QStringLiteral(R"(a\Path)")},
        // fonts: the "\f{name}" of update(), and the commands of a file
        {QStringLiteral(R"(x\f{symbol}y\f{}z \f{open)"), QStringLiteral("xyz open")},
        {QStringLiteral(R"({\L\fArial|b0|i0|c0|p34;under\l}{\C1;red}{\c255;rgb})"),
         QStringLiteral("underredrgb")},
        {QStringLiteral(R"(\Ftxt.shx|c1;x\Fgdt;j)"), QStringLiteral("xj")},
        {QStringLiteral(R"(\W0.8x;\T1.5;\Q15;\A1;\H3;x)"), QStringLiteral("x")},
        {QStringLiteral("\\pi-12,l12,t12;1.\tFirst\n\\pi0,l0;2.\tSecond"),
         QStringLiteral("1. First 2. Second")},
        // stacks: as typed, a backslash makes the next character ordinary
        {QStringLiteral(R"(\S+0.1^-0.1; \S1/2; \S1#4; \Sa\;b/c;)"),
         QStringLiteral("+0.1^-0.1 1/2 1#4 a;b/c")},
        {QStringLiteral("\\Sx\ty^z;"), QStringLiteral("x y^z")},
        {QStringLiteral("4'-0{\\H1x;\\S1#4;}\""), QStringLiteral("4'-01#4\"")},
        // a command with arguments that is not complete stays text
        {QStringLiteral(R"(C:\plans\a.dxf \Hello \Cat5 \A9; \Sopen)"),
         QStringLiteral(R"(C:\plans\a.dxf \Hello \Cat5 \A9; \Sopen)")},
        {QStringLiteral(R"(see \\Server\Share; then)"), QStringLiteral(R"(see \Server\Share; then)")},
        {QStringLiteral(R"(total: \Sum; x)"), QStringLiteral(R"(total: \Sum; x)")},
        {QStringLiteral(R"({\C1invidunt ut labore} \H2.5tall \W1wide)"),
         QStringLiteral(R"(\C1invidunt ut labore \H2.5tall \W1wide)")},
        // a command without arguments always counts
        {QStringLiteral(R"(C:\Library\old\Kits\X1)"), QStringLiteral("C:ibraryldits1")},
        {QStringLiteral("\\~nb\u00a0sp \\N col \\Xdim {\\O ov\\o \\K st\\k}"),
         QStringLiteral(" nb\u00a0sp   col dim  ov  st")},
        {QStringLiteral("ctl\x01x\ry"), QStringLiteral("ctl x y")},
        {QStringLiteral("%%C10 %%p1 100%% %%u"), QStringLiteral("\u230010 \u00b11 100%% %%u")},
        {QStringLiteral(R"(\U+00B0 \M+1815C tail\)"), QStringLiteral(R"(\U+00B0 \M+1815C tail)")},
        {QStringLiteral(R"(\{lit\} {grp} ^caret ^I)"), QStringLiteral("{lit} grp ^caret ^I")},
    };
    return cases;
}

bool hasControlCharacter(const QString& text) {
    return std::any_of(text.cbegin(), text.cend(), [](const QChar c) {
        return c.unicode() < 0x20;
    });
}
} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toPlainLine: MTEXT content becomes one line without codes",
          "[dxf_codec][mtext_plain]") {
    for (const auto& [text, plain] : plainLineCases()) {
        CAPTURE(text.toStdString());
        CHECK(RS_MText::toPlainLine(text) == plain);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("toPlainLine: the line is safe as a DXF value in either encoding",
          "[dxf_codec][mtext_plain]") {
    std::vector<QString> texts;
    for (const auto& [text, plain] : plainLineCases()) {
        texts.push_back(text);
    }
    // text that looks like the start of a command, over and over
    for (const char* piece : {"\\S", "\\S1", "\\Sa/", "\\f", "\\F", "\\p", "\\pi", "\\f{", "{", "\\H1", "\\"}) {
        texts.push_back(QString::fromLatin1(piece).repeated(5000));
    }
    for (char16_t control = 0; control < 0x20; ++control) {
        const QChar c(control);
        texts.push_back(QStringLiteral("a") + c + QStringLiteral("b\\") + c + QStringLiteral("c\\Sx/") + c
                        + QStringLiteral("y;\\f") + c + QStringLiteral(";"));
    }
    for (const QString& text : texts) {
        CAPTURE(text.left(40).toStdString());
        const QString plain = RS_MText::toPlainLine(text);
        CHECK_FALSE(hasControlCharacter(plain));
        // The line is the value of a single-line ATTRIB and is written as
        // one. What isSafeDxfRecordText() of libdxfrw rejects:
        const QString inDxf = RS_FilterDXFRW::toDxfSingleLineString(plain, true);
        CHECK_FALSE(hasControlCharacter(inDxf));
        CHECK_FALSE(hasControlCharacter(RS_FilterDXFRW::toDxfSingleLineString(plain, false)));
        // the encoder adds no code of an MTEXT, and the line has no line
        // feed: either encoder writes the same
        CHECK(inDxf == RS_FilterDXFRW::toDxfString(plain, true));
        // A DXF is read back with the line it was written from: a "\P" or a
        // "{\f...;}" that the line holds is not a code of a single-line value.
        CHECK(RS_FilterDXFRW::toNativeSingleLineString(inDxf) == plain);
    }
    const QString stack = RS_MText::toPlainLine(QStringLiteral(R"(\S1^2;)"));
    CHECK(RS_FilterDXFRW::toDxfSingleLineString(stack, true) == QStringLiteral("1^ 2"));
    CHECK(RS_FilterDXFRW::toDxfSingleLineString(stack, false) == QStringLiteral("1^2"));
}
