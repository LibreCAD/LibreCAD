/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
****************************************************************************/

// LC_LinFile turns a vendor line type file into records and back: the two-line
// "*NAME,description" / "A,d1,d2,..." format, the name a dialog may give a line
// type, and the reason a record is left out. A later dialog words the issues.

#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <QByteArray>
#include <QLocale>
#include <QString>
#include <QStringList>

#include "lc_linfile.h"
#include "lc_linetype.h"

namespace {

LC_LineType makeEntry(const QString& name, std::vector<double> pattern,
                      const QString& description = QString()) {
    LC_LineType entry(name);
    entry.description = description;
    entry.pattern = std::move(pattern);
    return entry;
}

QByteArray textFile(const QStringList& lines) {
    return lines.join(QLatin1Char('\n')).toUtf8() + '\n';
}

bool sameBits(double a, double b) {
    return std::memcmp(&a, &b, sizeof(double)) == 0;
}

// Restores the default locale however the test leaves it.
class DefaultLocaleGuard {
public:
    DefaultLocaleGuard() : m_previous(QLocale()) {}
    ~DefaultLocaleGuard() { QLocale::setDefault(m_previous); }

private:
    QLocale m_previous;
};

} // namespace

TEST_CASE("LC_LineType::checkName accepts a plain name", "[linetype][linfile][name]") {
    CHECK(LC_LineType::checkName(QStringLiteral("VENDOR_TAB")) == LC_LineType::NameCheck::Ok);
    CHECK(LC_LineType::isValidName(QStringLiteral("VENDOR_TAB")));
    CHECK(LC_LineType::checkName(QStringLiteral("a")) == LC_LineType::NameCheck::Ok);
}

TEST_CASE("LC_LineType::checkName refuses a blank name", "[linetype][linfile][name]") {
    CHECK(LC_LineType::checkName(QString()) == LC_LineType::NameCheck::Empty);
    CHECK(LC_LineType::checkName(QStringLiteral("    ")) == LC_LineType::NameCheck::Empty);
    CHECK(LC_LineType::checkName(QStringLiteral("\t\n")) == LC_LineType::NameCheck::Empty);
    CHECK_FALSE(LC_LineType::isValidName(QStringLiteral("   ")));
}

TEST_CASE("LC_LineType::checkName counts the trimmed name's length",
          "[linetype][linfile][name]") {
    const QString atLimit(255, QChar(u'A'));
    const QString overLimit(256, QChar(u'A'));
    CHECK(LC_LineType::checkName(atLimit) == LC_LineType::NameCheck::Ok);
    CHECK(LC_LineType::checkName(overLimit) == LC_LineType::NameCheck::TooLong);
    // The blanks around a name do not count.
    CHECK(LC_LineType::checkName(QStringLiteral(" ") + atLimit + QStringLiteral(" ")) ==
          LC_LineType::NameCheck::Ok);
    CHECK(LC_LineType::checkName(QStringLiteral(" ") + overLimit + QStringLiteral(" ")) ==
          LC_LineType::NameCheck::TooLong);
}

TEST_CASE("LC_LineType::checkName refuses every reserved character",
          "[linetype][linfile][name]") {
    const char reserved[] = {'<', '>', '/', '\\', '"', ':', ';', '?',
                             '*', '|', ',', '=', '`'};
    for (const char c : reserved) {
        const QString name = QStringLiteral("VENDOR") + QChar(QLatin1Char(c)) +
                             QStringLiteral("TAB");
        INFO("reserved character " << c);
        CHECK(LC_LineType::checkName(name) == LC_LineType::NameCheck::ReservedCharacter);
        CHECK_FALSE(LC_LineType::isValidName(name));
    }
}

TEST_CASE("LC_LineType::checkName refuses a control character inside a name",
          "[linetype][linfile][name]") {
    CHECK(LC_LineType::checkName(QStringLiteral("VEN\tDOR")) ==
          LC_LineType::NameCheck::ControlCharacter);
    CHECK(LC_LineType::checkName(QStringLiteral("VEN") + QChar(u'\0') +
                                 QStringLiteral("DOR")) ==
          LC_LineType::NameCheck::ControlCharacter);
    CHECK(LC_LineType::checkName(QStringLiteral("VEN") + QChar(u'\x1F') +
                                 QStringLiteral("DOR")) ==
          LC_LineType::NameCheck::ControlCharacter);
    CHECK(LC_LineType::checkName(QStringLiteral("VEN") + QChar(u'\x7F') +
                                 QStringLiteral("DOR")) ==
          LC_LineType::NameCheck::ControlCharacter);
    CHECK(LC_LineType::checkName(QStringLiteral("VEN") + QChar(u'\x85') +
                                 QStringLiteral("DOR")) ==
          LC_LineType::NameCheck::ControlCharacter);
}

TEST_CASE("LC_LinFile::read reads the records of a line type file",
          "[linetype][linfile][read]") {
    const QByteArray file = textFile({
        QStringLiteral("; a vendor line type file"),
        QStringLiteral(""),
        QStringLiteral("*VENDOR_TAB,Vendor tab"),
        QStringLiteral("A,1.5,-.5,0,-.25"),
        QStringLiteral(""),
        QStringLiteral("*VENDOR_PLAIN"),
        QStringLiteral("A,5.5,-2.5"),
        QStringLiteral("*VENDOR_DOT,Three dots"),
        QStringLiteral("A, .5 , -.25 , 0"),
    });

    const LC_LinFile::ReadResult result = LC_LinFile::read(file);
    REQUIRE(result.issues.empty());
    REQUIRE(result.entries.size() == 3);

    CHECK(result.entries[0].name == QStringLiteral("VENDOR_TAB"));
    CHECK(result.entries[0].description == QStringLiteral("Vendor tab"));
    CHECK(result.entries[0].pattern == std::vector<double>{1.5, -0.5, 0.0, -0.25});
    CHECK(result.entries[0].origin == LC_LineType::Origin::Imported);
    CHECK_FALSE(result.entries[0].hasImportedRecord);

    CHECK(result.entries[1].name == QStringLiteral("VENDOR_PLAIN"));
    CHECK(result.entries[1].description.isEmpty());
    CHECK(result.entries[1].pattern == std::vector<double>{5.5, -2.5});

    CHECK(result.entries[2].name == QStringLiteral("VENDOR_DOT"));
    CHECK(result.entries[2].description == QStringLiteral("Three dots"));
    CHECK(result.entries[2].pattern == std::vector<double>{0.5, -0.25, 0.0});
}

TEST_CASE("LC_LinFile::read keeps a description that holds commas whole",
          "[linetype][linfile][read]") {
    const LC_LinFile::ReadResult result = LC_LinFile::read(
        textFile({QStringLiteral("*VENDOR_COMMA,First, second, third"),
                  QStringLiteral("A,1,-1")}));
    REQUIRE(result.issues.empty());
    REQUIRE(result.entries.size() == 1);
    CHECK(result.entries[0].name == QStringLiteral("VENDOR_COMMA"));
    CHECK(result.entries[0].description == QStringLiteral("First, second, third"));
    CHECK(result.entries[0].pattern == std::vector<double>{1.0, -1.0});
}

TEST_CASE("LC_LinFile::read turns a control character in a description into a space",
          "[linetype][linfile][read]") {
    const LC_LinFile::ReadResult result = LC_LinFile::read(textFile(
        {QStringLiteral("*VENDOR_TAB,a") + QChar(u'\x01') + QStringLiteral("b"),
         QStringLiteral("A,1,-1")}));
    REQUIRE(result.issues.empty());
    REQUIRE(result.entries.size() == 1);
    CHECK(result.entries[0].description == QStringLiteral("a b"));
}

TEST_CASE("LC_LinFile::read reads CRLF and a UTF-8 byte order mark alike",
          "[linetype][linfile][read]") {
    const QString plain = QStringList{
        QStringLiteral("; comment"),
        QStringLiteral("*VENDOR_TAB,Vendor tab"),
        QStringLiteral("A,1.5,-.5"),
        QStringLiteral("*VENDOR_DOT,dot"),
        QStringLiteral("A,0,-3"),
    }.join(QLatin1Char('\n'));

    const LC_LinFile::ReadResult lf = LC_LinFile::read(plain.toUtf8() + '\n');
    QByteArray crlf = plain.toUtf8();
    crlf.replace('\n', "\r\n");
    crlf += "\r\n";
    const LC_LinFile::ReadResult windows = LC_LinFile::read(crlf);
    QByteArray crOnly = plain.toUtf8();
    crOnly.replace('\n', '\r');
    crOnly += '\r';
    const LC_LinFile::ReadResult classicMac = LC_LinFile::read(crOnly);
    const LC_LinFile::ReadResult bom =
        LC_LinFile::read(QByteArray::fromHex("EFBBBF") + plain.toUtf8() + '\n');

    for (const LC_LinFile::ReadResult* result : {&lf, &windows, &classicMac, &bom}) {
        REQUIRE(result->issues.empty());
        REQUIRE(result->entries.size() == 2);
        CHECK(result->entries[0].name == QStringLiteral("VENDOR_TAB"));
        CHECK(result->entries[0].description == QStringLiteral("Vendor tab"));
        CHECK(result->entries[0].pattern == std::vector<double>{1.5, -0.5});
        CHECK(result->entries[1].name == QStringLiteral("VENDOR_DOT"));
        CHECK(result->entries[1].pattern == std::vector<double>{0.0, -3.0});
    }
}

TEST_CASE("LC_LinFile::read drops a byte order mark at the start of any line",
          "[linetype][linfile][read]") {
    QByteArray file = "*VENDOR_TAB,tab\nA,1,-1\n";
    // A file concatenated from two files carries a second mark.
    file += QByteArray::fromHex("EFBBBF");
    file += "*VENDOR_DOT,dot\nA,0,-3\n";

    const LC_LinFile::ReadResult result = LC_LinFile::read(file);
    REQUIRE(result.issues.empty());
    REQUIRE(result.entries.size() == 2);
    CHECK(result.entries[0].name == QStringLiteral("VENDOR_TAB"));
    CHECK(result.entries[0].pattern == std::vector<double>{1.0, -1.0});
    CHECK(result.entries[1].name == QStringLiteral("VENDOR_DOT"));
    CHECK(result.entries[1].pattern == std::vector<double>{0.0, -3.0});
}

TEST_CASE("LC_LinFile::read scales every length", "[linetype][linfile][read]") {
    const QByteArray file = textFile({QStringLiteral("*VENDOR_SCALE,scaled"),
                                      QStringLiteral("A,2.5,-1.25,0")});
    const LC_LinFile::ReadResult result = LC_LinFile::read(file, 25.4);
    REQUIRE(result.issues.empty());
    REQUIRE(result.entries.size() == 1);
    const std::vector<double>& pattern = result.entries[0].pattern;
    REQUIRE(pattern.size() == 3);
    CHECK(pattern[0] == 2.5 * 25.4);
    CHECK(pattern[1] == -1.25 * 25.4);
    CHECK(pattern[2] == 0.0 * 25.4);
}

TEST_CASE("LC_LinFile::read reports an element the scale makes non-finite",
          "[linetype][linfile][read]") {
    const LC_LinFile::ReadResult result = LC_LinFile::read(
        textFile({QStringLiteral("*VENDOR_SCALE,scaled"),
                  QStringLiteral("A,1e308,-1e308")}),
        25.4);
    CHECK(result.entries.empty());
    REQUIRE(result.issues.size() == 1);
    CHECK(result.issues[0].kind == LC_LinFile::Issue::Kind::BadNumber);
    CHECK(result.issues[0].line == 2);
    CHECK(result.issues[0].text == QStringLiteral("1e308"));
}

TEST_CASE("LC_LinFile::read treats an unusable scale as 1",
          "[linetype][linfile][read]") {
    const QByteArray file = textFile({QStringLiteral("*VENDOR_SCALE,scaled"),
                                      QStringLiteral("A,2.5,-1.25")});
    const std::vector<double> scales{
        0.0, -1.0, std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()};
    for (const double scale : scales) {
        INFO("scale " << scale);
        const LC_LinFile::ReadResult result = LC_LinFile::read(file, scale);
        REQUIRE(result.issues.empty());
        REQUIRE(result.entries.size() == 1);
        CHECK(result.entries[0].pattern == std::vector<double>{2.5, -1.25});
    }
}

TEST_CASE("LC_LinFile::read reports each kind of issue and reads on",
          "[linetype][linfile][read][issue]") {
    QString manyLine = QStringLiteral("A");
    for (int i = 0; i < LC_LinFile::kMaxElements + 1; ++i) {
        manyLine += QStringLiteral(",1");
    }
    const QStringList lines{
        QStringLiteral("; a file mixing good and bad records"), // 1
        QStringLiteral("A,1,-1"),                                // 2 orphan
        QStringLiteral("nonsense"),                              // 3 unknown
        QStringLiteral("*VENDOR_MISSING,no pattern"),            // 4 missing
        QStringLiteral("*VENDOR_BADNUM,bad number"),             // 5
        QStringLiteral("A,1,abc"),                               // 6 bad number
        QStringLiteral("*VENDOR_FIRST,first gap"),               // 7
        QStringLiteral("A,-5,5"),                                // 8 first gap
        QStringLiteral("*VENDOR_NOGAP,cannot draw"),             // 9
        QStringLiteral("A,5,6"),                                 // 10 cannot draw
        QStringLiteral("*VENDOR_COMPLEX,complex"),               // 11
        QStringLiteral("A,1,-1,[SHAPE]"),                        // 12 complex
        QStringLiteral("*BAD:NAME,invalid"),                     // 13 invalid name
        QStringLiteral("A,1,-1"),                                // 14
        QStringLiteral("*DASHED,custom dashed"),                 // 15 built in
        QStringLiteral("A,1,-1"),                                // 16
        QStringLiteral("*VENDOR_DUP,first"),                     // 17
        QStringLiteral("A,1,-1"),                                // 18 accepted
        QStringLiteral("*vendor_dup,second"),                    // 19 duplicate
        QStringLiteral("A,2,-2"),                                // 20
        QStringLiteral("*VENDOR_MANY,many"),                     // 21
        manyLine,                                                // 22 too many
        QStringLiteral("*VENDOR_GOOD,still read"),               // 23
        QStringLiteral("A,3,-3"),                                // 24 accepted
    };

    const LC_LinFile::ReadResult result = LC_LinFile::read(textFile(lines));

    const std::vector<std::pair<LC_LinFile::Issue::Kind, int>> expected{
        {LC_LinFile::Issue::Kind::OrphanPattern, 2},
        {LC_LinFile::Issue::Kind::UnknownLine, 3},
        {LC_LinFile::Issue::Kind::MissingPattern, 4},
        {LC_LinFile::Issue::Kind::BadNumber, 6},
        {LC_LinFile::Issue::Kind::FirstElementNotDash, 8},
        {LC_LinFile::Issue::Kind::CannotDraw, 9},
        {LC_LinFile::Issue::Kind::ComplexElement, 12},
        {LC_LinFile::Issue::Kind::InvalidName, 13},
        {LC_LinFile::Issue::Kind::BuiltInName, 15},
        {LC_LinFile::Issue::Kind::DuplicateName, 19},
        {LC_LinFile::Issue::Kind::TooManyElements, 21},
    };
    const QStringList texts{
        QStringLiteral("A,1,-1"),
        QStringLiteral("nonsense"),
        QStringLiteral("VENDOR_MISSING"),
        QStringLiteral("abc"),
        QStringLiteral("-5"),
        QStringLiteral("VENDOR_NOGAP"),
        QStringLiteral("[SHAPE]"),
        QStringLiteral("BAD:NAME"),
        QStringLiteral("DASHED"),
        QStringLiteral("vendor_dup"),
        QStringLiteral("VENDOR_MANY"),
    };
    REQUIRE(result.issues.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        INFO("issue " << i);
        CHECK(result.issues[i].kind == expected[i].first);
        CHECK(result.issues[i].line == expected[i].second);
        CHECK(result.issues[i].text == texts[static_cast<int>(i)]);
    }

    // The good records around the bad ones still made it.
    REQUIRE(result.entries.size() == 2);
    CHECK(result.entries[0].name == QStringLiteral("VENDOR_DUP"));
    CHECK(result.entries[0].pattern == std::vector<double>{1.0, -1.0});
    CHECK(result.entries[1].name == QStringLiteral("VENDOR_GOOD"));
    CHECK(result.entries[1].pattern == std::vector<double>{3.0, -3.0});
}

TEST_CASE("LC_LinFile::read refuses the canonical built-in names",
          "[linetype][linfile][read][name]") {
    const QByteArray file = textFile({
        QStringLiteral("*VENDOR_OK,fine"),
        QStringLiteral("A,1,-1"),
        QStringLiteral("*dashed,lower case"),
        QStringLiteral("A,1,-1"),
        QStringLiteral("*Continuous,case"),
        QStringLiteral("A,1,-1"),
        QStringLiteral("*DOT,dot"),
        QStringLiteral("A,1,-1"),
        QStringLiteral("*BYLAYER,bylayer"),
        QStringLiteral("A,1,-1"),
        QStringLiteral("*ByBlock,byblock"),
        QStringLiteral("A,1,-1"),
    });
    const LC_LinFile::ReadResult result = LC_LinFile::read(file);
    REQUIRE(result.entries.size() == 1);
    CHECK(result.entries[0].name == QStringLiteral("VENDOR_OK"));

    const QStringList names{QStringLiteral("dashed"), QStringLiteral("Continuous"),
                            QStringLiteral("DOT"), QStringLiteral("BYLAYER"),
                            QStringLiteral("ByBlock")};
    REQUIRE(result.issues.size() == names.size());
    for (std::size_t i = 0; i < names.size(); ++i) {
        INFO("issue " << i);
        CHECK(result.issues[i].kind == LC_LinFile::Issue::Kind::BuiltInName);
        CHECK(result.issues[i].text == names[static_cast<int>(i)]);
    }
    CHECK(result.issues[0].line == 3);
    CHECK(result.issues[1].line == 5);
    CHECK(result.issues[2].line == 7);
    CHECK(result.issues[3].line == 9);
    CHECK(result.issues[4].line == 11);
}

TEST_CASE("LC_LinFile::read accepts a built-in alias as a line type of its own",
          "[linetype][linfile][read][name]") {
    const LC_LinFile::ReadResult result = LC_LinFile::read(textFile({
        QStringLiteral("*ACAD_ISO02W100,dashed iso"),
        QStringLiteral("A,.5,-.25"),
        QStringLiteral("*acad_iso07w100,dotted iso"),
        QStringLiteral("A,0,-.25"),
    }));
    REQUIRE(result.issues.empty());
    REQUIRE(result.entries.size() == 2);
    CHECK(result.entries[0].name == QStringLiteral("ACAD_ISO02W100"));
    CHECK(result.entries[0].description == QStringLiteral("dashed iso"));
    CHECK(result.entries[0].pattern == std::vector<double>{0.5, -0.25});
    CHECK(result.entries[1].name == QStringLiteral("acad_iso07w100"));
    CHECK(result.entries[1].description == QStringLiteral("dotted iso"));
    CHECK(result.entries[1].pattern == std::vector<double>{0.0, -0.25});
}

TEST_CASE("LC_LinFile::read keeps the first spelling of an alias",
          "[linetype][linfile][read][name]") {
    const LC_LinFile::ReadResult result = LC_LinFile::read(textFile({
        QStringLiteral("*ACAD_ISO02W100,first"),
        QStringLiteral("A,1,-1"),
        QStringLiteral("*acad_iso02w100,second"),
        QStringLiteral("A,2,-2"),
    }));
    REQUIRE(result.entries.size() == 1);
    CHECK(result.entries[0].name == QStringLiteral("ACAD_ISO02W100"));
    CHECK(result.entries[0].pattern == std::vector<double>{1.0, -1.0});
    REQUIRE(result.issues.size() == 1);
    CHECK(result.issues[0].kind == LC_LinFile::Issue::Kind::DuplicateName);
    CHECK(result.issues[0].line == 3);
    CHECK(result.issues[0].text == QStringLiteral("acad_iso02w100"));
}

TEST_CASE("LC_LinFile::read keeps the first record of a repeated key",
          "[linetype][linfile][read][name]") {
    const LC_LinFile::ReadResult result = LC_LinFile::read(textFile({
        QStringLiteral("*VENDOR_TAB,first"),
        QStringLiteral("A,1,-1"),
        QStringLiteral("*vendor_tab ,second"),
        QStringLiteral("A,2,-2"),
    }));
    REQUIRE(result.entries.size() == 1);
    CHECK(result.entries[0].name == QStringLiteral("VENDOR_TAB"));
    CHECK(result.entries[0].description == QStringLiteral("first"));
    CHECK(result.entries[0].pattern == std::vector<double>{1.0, -1.0});
    REQUIRE(result.issues.size() == 1);
    CHECK(result.issues[0].kind == LC_LinFile::Issue::Kind::DuplicateName);
    CHECK(result.issues[0].text == QStringLiteral("vendor_tab"));
}

TEST_CASE("LC_LinFile::read refuses a name with a colon",
          "[linetype][linfile][read][name]") {
    const LC_LinFile::ReadResult result = LC_LinFile::read(textFile({
        QStringLiteral("*BAD:NAME,invalid"),
        QStringLiteral("A,1,-1"),
    }));
    CHECK(result.entries.empty());
    REQUIRE(result.issues.size() == 1);
    CHECK(result.issues[0].kind == LC_LinFile::Issue::Kind::InvalidName);
    CHECK(result.issues[0].line == 1);
    CHECK(result.issues[0].text == QStringLiteral("BAD:NAME"));
}

TEST_CASE("LC_LinFile reads and writes numbers in the C locale",
          "[linetype][linfile][locale]") {
    DefaultLocaleGuard restoreDefault;
    // A German default writes a decimal comma; the file format does not.
    QLocale::setDefault(QLocale(QLocale::German));

    const LC_LinFile::ReadResult result = LC_LinFile::read(textFile({
        QStringLiteral("*VENDOR_LOCALE,comma is a separator"),
        QStringLiteral("A,1.5,-1.5"),
    }));
    REQUIRE(result.issues.empty());
    REQUIRE(result.entries.size() == 1);
    CHECK(result.entries[0].pattern == std::vector<double>{1.5, -1.5});

    const QByteArray out = LC_LinFile::write(
        {makeEntry(QStringLiteral("VENDOR_LOCALE"), {1.5, -1.5})});
    CHECK(out == QByteArray("*VENDOR_LOCALE\nA,1.5,-1.5\n"));
}

TEST_CASE("LC_LinFile::read decodes UTF-8 and falls back to Latin-1",
          "[linetype][linfile][encodings]") {
    const QString letter = QString::fromUtf16(u"\u00F8"); // o with a stroke
    const QString utf8File =
        QStringLiteral("*VENDOR_UTF8,M") + letter + QStringLiteral("r\nA,1,-1\n");
    const LC_LinFile::ReadResult utf8 = LC_LinFile::read(utf8File.toUtf8());
    REQUIRE(utf8.issues.empty());
    REQUIRE(utf8.entries.size() == 1);
    CHECK(utf8.entries[0].description == QStringLiteral("M") + letter +
                                            QStringLiteral("r"));

    QByteArray latin1 = QStringLiteral("*VENDOR_LATIN1,M").toUtf8();
    latin1.append(static_cast<char>(0xF8));
    latin1.append("r\nA,1,-1\n");
    const LC_LinFile::ReadResult old = LC_LinFile::read(latin1);
    REQUIRE(old.issues.empty());
    REQUIRE(old.entries.size() == 1);
    CHECK(old.entries[0].description == QStringLiteral("M") + letter +
                                         QStringLiteral("r"));
}

TEST_CASE("LC_LinFile honors the element limit", "[linetype][linfile][limits]") {
    QString atLimit = QStringLiteral("A");
    for (int i = 0; i < LC_LinFile::kMaxElements; ++i) {
        atLimit += (i % 2 == 0) ? QStringLiteral(",1") : QStringLiteral(",-1");
    }
    const LC_LinFile::ReadResult accepted = LC_LinFile::read(textFile({
        QStringLiteral("*VENDOR_MANY,many"),
        atLimit,
    }));
    REQUIRE(accepted.issues.empty());
    REQUIRE(accepted.entries.size() == 1);
    CHECK(accepted.entries[0].pattern.size() ==
          static_cast<std::size_t>(LC_LinFile::kMaxElements));

    QString overLimit = atLimit + QStringLiteral(",1");
    const LC_LinFile::ReadResult refused = LC_LinFile::read(textFile({
        QStringLiteral("*VENDOR_MANY,many"),
        overLimit,
    }));
    CHECK(refused.entries.empty());
    REQUIRE(refused.issues.size() == 1);
    CHECK(refused.issues[0].kind == LC_LinFile::Issue::Kind::TooManyElements);
    CHECK(refused.issues[0].text == QStringLiteral("VENDOR_MANY"));
}

TEST_CASE("LC_LinFile::read guards a pattern line before splitting it",
          "[linetype][linfile][limits]") {
    QByteArray file = "*VENDOR_MANY,many\nA";
    file += QByteArray(5000000, ',');
    file += "\n*VENDOR_AFTER,ok\nA,1,-1\n";

    const LC_LinFile::ReadResult result = LC_LinFile::read(file);
    REQUIRE(result.issues.size() == 1);
    CHECK(result.issues[0].kind == LC_LinFile::Issue::Kind::TooManyElements);
    CHECK(result.issues[0].line == 1);
    CHECK(result.issues[0].text == QStringLiteral("VENDOR_MANY"));
    REQUIRE(result.entries.size() == 1);
    CHECK(result.entries[0].name == QStringLiteral("VENDOR_AFTER"));
    CHECK(result.entries[0].pattern == std::vector<double>{1.0, -1.0});
}

TEST_CASE("LC_LinFile::read survives a huge line of garbage",
          "[linetype][linfile][limits]") {
    QByteArray file(2 * 1024 * 1024, 'x');
    file += "\n*VENDOR_AFTER,ok\nA,1,-1\n";
    const LC_LinFile::ReadResult result = LC_LinFile::read(file);
    REQUIRE(result.entries.size() == 1);
    CHECK(result.entries[0].name == QStringLiteral("VENDOR_AFTER"));
    CHECK(result.entries[0].pattern == std::vector<double>{1.0, -1.0});
    REQUIRE(result.issues.size() == 1);
    CHECK(result.issues[0].kind == LC_LinFile::Issue::Kind::UnknownLine);
    CHECK(result.issues[0].line == 1);
}

TEST_CASE("LC_LinFile::write writes the two lines of each entry",
          "[linetype][linfile][write]") {
    const QByteArray out = LC_LinFile::write({
        makeEntry(QStringLiteral("VENDOR_ONE"), {1.5, -0.5},
                  QStringLiteral("First, with comma")),
        makeEntry(QStringLiteral("VENDOR_TWO"), {2.0, -2.0}),
    });
    CHECK(out ==
          QByteArray("*VENDOR_ONE,First, with comma\nA,1.5,-0.5\n"
                     "*VENDOR_TWO\nA,2,-2\n"));
}

TEST_CASE("LC_LinFile::write formats doubles shortest round trip",
          "[linetype][linfile][write]") {
    const double precise = 0.1 + 0.2;
    const QByteArray out = LC_LinFile::write({makeEntry(
        QStringLiteral("VENDOR_NUMBERS"),
        {0.1, -1e-7, 123456.789, -0.0, precise, -precise})});
    CHECK(out ==
          QByteArray("*VENDOR_NUMBERS\n"
                     "A,0.1,-1e-07,123456.789,0,0.30000000000000004,-0.30000000000000004\n"));
}

TEST_CASE("LC_LinFile::write writes the shortest text that reads back",
          "[linetype][linfile][write]") {
    SECTION("a third needs sixteen digits") {
        const double third = 1.0 / 3.0;
        const QByteArray out = LC_LinFile::write(
            {makeEntry(QStringLiteral("VENDOR_THIRD"), {third, -third})});
        CHECK(out == QByteArray("*VENDOR_THIRD\n"
                                "A,0.3333333333333333,-0.3333333333333333\n"));
        const LC_LinFile::ReadResult back = LC_LinFile::read(out);
        REQUIRE(back.issues.empty());
        REQUIRE(back.entries.size() == 1);
        CHECK(sameBits(back.entries[0].pattern[0], third));
        CHECK(sameBits(back.entries[0].pattern[1], -third));
    }

    SECTION("the extremes survive") {
        const double denorm = std::numeric_limits<double>::denorm_min();
        const double maximum = std::numeric_limits<double>::max();
        const double tiny = (2.0 / 3.0) * 1e-5;
        const std::vector<double> pattern{denorm, -1.0, maximum, -1.0, tiny, -1.0};
        const QByteArray out = LC_LinFile::write(
            {makeEntry(QStringLiteral("VENDOR_EXTREMES"), pattern)});
        const LC_LinFile::ReadResult back = LC_LinFile::read(out);
        REQUIRE(back.issues.empty());
        REQUIRE(back.entries.size() == 1);
        REQUIRE(back.entries[0].pattern.size() == pattern.size());
        for (std::size_t i = 0; i < pattern.size(); ++i) {
            INFO("element " << i);
            CHECK(sameBits(back.entries[0].pattern[i], pattern[i]));
        }
    }
}

TEST_CASE("LC_LinFile::write keeps one record per key",
          "[linetype][linfile][write]") {
    std::vector<LC_LinFile::Issue> issues;
    const QByteArray out = LC_LinFile::write(
        {makeEntry(QStringLiteral("VENDOR_TAB"), {1.0, -1.0}),
         makeEntry(QStringLiteral("vendor_tab"), {2.0, -2.0})},
        &issues);
    CHECK(out == QByteArray("*VENDOR_TAB\nA,1,-1\n"));
    REQUIRE(issues.size() == 1);
    CHECK(issues[0].kind == LC_LinFile::Issue::Kind::DuplicateName);
    CHECK(issues[0].line == 2);
    CHECK(issues[0].text == QStringLiteral("vendor_tab"));

    const LC_LinFile::ReadResult back = LC_LinFile::read(out);
    REQUIRE(back.issues.empty());
    REQUIRE(back.entries.size() == 1);
    CHECK(back.entries[0].name == QStringLiteral("VENDOR_TAB"));
}

TEST_CASE("LC_LinFile::write decides the element limit on the written list",
          "[linetype][linfile][write]") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    std::vector<double> filled;
    for (int i = 0; i < 150; ++i) {
        filled.push_back(i % 2 == 0 ? 1.0 : -1.0);
        filled.push_back(nan);
    }
    std::vector<double> longPattern;
    for (int i = 0; i < LC_LinFile::kMaxElements + 44; ++i) {
        longPattern.push_back(i % 2 == 0 ? 1.0 : -1.0);
    }
    std::vector<LC_LinFile::Issue> issues;
    const QByteArray out = LC_LinFile::write(
        {makeEntry(QStringLiteral("VENDOR_FILLED"), filled),
         makeEntry(QStringLiteral("VENDOR_LONG"), longPattern)},
        &issues);
    CHECK(out.startsWith(QByteArray("*VENDOR_FILLED\n")));
    REQUIRE(issues.size() == 1);
    CHECK(issues[0].kind == LC_LinFile::Issue::Kind::TooManyElements);
    CHECK(issues[0].line == 2);
    CHECK(issues[0].text == QStringLiteral("VENDOR_LONG"));

    const LC_LinFile::ReadResult back = LC_LinFile::read(out);
    REQUIRE(back.issues.empty());
    REQUIRE(back.entries.size() == 1);
    CHECK(back.entries[0].name == QStringLiteral("VENDOR_FILLED"));
    CHECK(back.entries[0].pattern.size() == 150);
}

TEST_CASE("LC_LinFile::write leaves out and reports what it cannot hold",
          "[linetype][linfile][write]") {
    std::vector<double> many;
    for (int i = 0; i < LC_LinFile::kMaxElements + 1; ++i) {
        many.push_back(i % 2 == 0 ? 1.0 : -1.0);
    }
    std::vector<LC_LinFile::Issue> issues;
    const QByteArray out = LC_LinFile::write(
        {
            makeEntry(QStringLiteral("VENDOR_GOOD"), {1.0, -1.0}),
            makeEntry(QStringLiteral("BAD:NAME"), {1.0, -1.0}),
            makeEntry(QStringLiteral("VENDOR_NOGAP"), {5.0, 6.0}),
            makeEntry(QStringLiteral("VENDOR_MANY"), many),
            makeEntry(QStringLiteral("VENDOR_LAST"), {2.0, -2.0}),
        },
        &issues);

    CHECK(out == QByteArray("*VENDOR_GOOD\nA,1,-1\n*VENDOR_LAST\nA,2,-2\n"));
    REQUIRE(issues.size() == 3);
    const std::vector<std::pair<LC_LinFile::Issue::Kind, int>> expected{
        {LC_LinFile::Issue::Kind::InvalidName, 2},
        {LC_LinFile::Issue::Kind::CannotDraw, 3},
        {LC_LinFile::Issue::Kind::TooManyElements, 4},
    };
    const QStringList texts{QStringLiteral("BAD:NAME"), QStringLiteral("VENDOR_NOGAP"),
                            QStringLiteral("VENDOR_MANY")};
    for (std::size_t i = 0; i < expected.size(); ++i) {
        INFO("issue " << i);
        CHECK(issues[i].kind == expected[i].first);
        CHECK(issues[i].line == expected[i].second);
        CHECK(issues[i].text == texts[static_cast<int>(i)]);
    }
}

TEST_CASE("LC_LinFile::write turns a control character in a description into a space",
          "[linetype][linfile][write]") {
    const QByteArray out = LC_LinFile::write({makeEntry(
        QStringLiteral("VENDOR_CTRL"), {1.0, -1.0}, QStringLiteral("a\tb\nc"))});
    CHECK(out == QByteArray("*VENDOR_CTRL,a b c\nA,1,-1\n"));
}

TEST_CASE("LC_LinFile::write trims the description the reader trims",
          "[linetype][linfile][write]") {
    const QByteArray out = LC_LinFile::write({
        makeEntry(QStringLiteral("VENDOR_PADDED"), {1.0, -1.0},
                  QStringLiteral("  two words  ")),
        makeEntry(QStringLiteral("VENDOR_BLANK"), {1.0, -1.0}, QStringLiteral("   ")),
    });
    CHECK(out ==
          QByteArray("*VENDOR_PADDED,two words\nA,1,-1\n*VENDOR_BLANK\nA,1,-1\n"));
}

TEST_CASE("LC_LinFile::write starts a pattern that begins with a gap at its dash",
          "[linetype][linfile][write]") {
    const QByteArray out = LC_LinFile::write(
        {makeEntry(QStringLiteral("VENDOR_GAP"), {-2.0, 1.0, -3.0, 2.0})});
    CHECK(out == QByteArray("*VENDOR_GAP\nA,1,-3,2,-2\n"));

    const LC_LinFile::ReadResult back = LC_LinFile::read(out);
    REQUIRE(back.issues.empty());
    REQUIRE(back.entries.size() == 1);
    CHECK(back.entries[0].pattern == std::vector<double>{1.0, -3.0, 2.0, -2.0});
}

TEST_CASE("LC_LinFile::write drops a non-finite element by writing the canonical form",
          "[linetype][linfile][write]") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    std::vector<LC_LinFile::Issue> issues;
    const QByteArray out = LC_LinFile::write(
        {
            makeEntry(QStringLiteral("VENDOR_NAN"), {10.0, nan, -5.0}),
            makeEntry(QStringLiteral("VENDOR_INF"), {10.0, -inf, -5.0, 2.0, -3.0}),
        },
        &issues);
    CHECK(issues.empty());
    CHECK_FALSE(out.contains("nan"));
    CHECK_FALSE(out.contains("inf"));
    CHECK(out == QByteArray("*VENDOR_NAN\nA,10,-5\n"
                            "*VENDOR_INF\nA,10,-5,2,-3\n"));

    const LC_LinFile::ReadResult back = LC_LinFile::read(out);
    REQUIRE(back.issues.empty());
    REQUIRE(back.entries.size() == 2);
    CHECK(back.entries[0].pattern == std::vector<double>{10.0, -5.0});
    CHECK(back.entries[1].pattern == std::vector<double>{10.0, -5.0, 2.0, -3.0});
}

TEST_CASE("LC_LinFile round trips the entries it writes",
          "[linetype][linfile][roundtrip]") {
    const double precise = 0.1 + 0.2;
    const std::vector<LC_LineType> entries{
        makeEntry(QStringLiteral("VENDOR_TAB"), {5.0, -2.5, 0.0, -2.5},
                  QStringLiteral("tab line")),
        makeEntry(QStringLiteral("VENDOR_PLAIN"), {1.5, -1.5}),
        makeEntry(QStringLiteral("VENDOR_DOT"), {0.0, -3.0}, QStringLiteral("dots")),
        makeEntry(QStringLiteral("VENDOR_PRECISE"), {precise, -precise, 0.1, -0.1},
                  QStringLiteral("precise")),
        makeEntry(QStringLiteral("DASHED"), {12.7, -6.35},
                  QStringLiteral("built-in spelling")),
    };

    const QByteArray out = LC_LinFile::write(entries);
    REQUIRE_FALSE(out.isEmpty());
    const LC_LinFile::ReadResult back = LC_LinFile::read(out);

    // A built-in name is written but the reader refuses it.
    REQUIRE(back.issues.size() == 1);
    CHECK(back.issues[0].kind == LC_LinFile::Issue::Kind::BuiltInName);
    CHECK(back.issues[0].text == QStringLiteral("DASHED"));
    CHECK(back.issues[0].line == 9);

    REQUIRE(back.entries.size() == 4);
    for (std::size_t i = 0; i < back.entries.size(); ++i) {
        INFO("entry " << i);
        CHECK(back.entries[i].name == entries[i].name);
        CHECK(back.entries[i].description == entries[i].description);
        CHECK(back.entries[i].pattern == entries[i].pattern);
        CHECK(back.entries[i].origin == LC_LineType::Origin::Imported);
        CHECK_FALSE(back.entries[i].hasImportedRecord);
    }
}
