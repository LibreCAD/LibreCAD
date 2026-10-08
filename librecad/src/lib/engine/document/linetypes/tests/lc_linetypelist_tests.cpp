/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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

// LC_LineTypeList seeds the 35 built-in linetypes from
// LC_LineTypeNames::builtinMetrics(), the table that also drives the head of
// RS_FilterDXFRW::writeLTypes(). These tests pin the seed, the table (the
// group 40 literal of each old writeLType() call, not a re-sum), find() by
// folded key, the twin rule of add(), and that the list owns its entries.

#include <cstring>
#include <iterator>
#include <set>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <QString>

#include "lc_linetype.h"
#include "lc_linetypelist.h"
#include "lc_linetypenames.h"
#include "rs.h"

namespace {

// The 35 writeLType() calls at the head of writeLTypes() as of 86d2c9325
// (rs_filterdxfrw.cpp:19100-19169), in call order: name, group 73 and the
// group 40 literal. dxf_roundtrip_tests.cpp pins the same rows through the
// writer; this file pins the table they now come from.
struct HeadRow {
    RS2::LineType type;
    const char* name;
    int size;
    double length;
};

const HeadRow kHeadRows[] = {
    {RS2::SolidLine, "CONTINUOUS", 0, 0.0},
    {RS2::LineByLayer, "ByLayer", 0, 0.0},
    {RS2::LineByBlock, "ByBlock", 0, 0.0},
    {RS2::DotLine, "DOT", 2, 6.35},
    {RS2::DotLineTiny, "DOTTINY", 2, 0.9525},
    {RS2::DotLine2, "DOT2", 2, 3.175},
    {RS2::DotLineX2, "DOTX2", 2, 12.7},
    {RS2::DashLine, "DASHED", 2, 19.05},
    {RS2::DashLineTiny, "DASHEDTINY", 2, 2.8575},
    {RS2::DashLine2, "DASHED2", 2, 9.525},
    {RS2::DashLineX2, "DASHEDX2", 2, 38.1},
    {RS2::HiddenLine, "HIDDEN", 2, 9.525},
    {RS2::HiddenLineTiny, "HIDDENTINY", 2, 1.42875},
    {RS2::HiddenLine2, "HIDDEN2", 2, 4.7625},
    {RS2::HiddenLineX2, "HIDDENX2", 2, 19.05},
    {RS2::DashDotLine, "DASHDOT", 4, 25.4},
    {RS2::DashDotLineTiny, "DASHDOTTINY", 4, 3.81},
    {RS2::DashDotLine2, "DASHDOT2", 4, 12.7},
    {RS2::DashDotLineX2, "DASHDOTX2", 4, 50.8},
    {RS2::DivideLine, "DIVIDE", 6, 31.75},
    {RS2::DivideLineTiny, "DIVIDETINY", 6, 4.7625},
    {RS2::DivideLine2, "DIVIDE2", 6, 15.875},
    {RS2::DivideLineX2, "DIVIDEX2", 6, 63.5},
    {RS2::BorderLine, "BORDER", 6, 44.45},
    {RS2::BorderLineTiny, "BORDERTINY", 6, 6.6675},
    {RS2::BorderLine2, "BORDER2", 6, 22.225},
    {RS2::BorderLineX2, "BORDERX2", 6, 88.9},
    {RS2::CenterLine, "CENTER", 4, 50.8},
    {RS2::CenterLineTiny, "CENTERTINY", 4, 7.62},
    {RS2::CenterLine2, "CENTER2", 4, 28.575},
    {RS2::CenterLineX2, "CENTERX2", 4, 101.6},
    {RS2::PhantomLine, "PHANTOM", 6, 63.5},
    {RS2::PhantomLineTiny, "PHANTOMTINY", 6, 9.525},
    {RS2::PhantomLine2, "PHANTOM2", 6, 31.75},
    {RS2::PhantomLineX2, "PHANTOMX2", 6, 127.0},
};

bool sameBits(double a, double b) {
    return std::memcmp(&a, &b, sizeof(double)) == 0;
}

// Counts live instances so a test can tell whether the list freed an entry.
// Like every entry these tests add, it stands for an archived LTYPE record.
class CountedLineType final : public LC_LineType {
public:
    CountedLineType(const QString& name, int& liveCount)
        : LC_LineType(name), m_liveCount{liveCount} {
        hasImportedRecord = true;
        ++m_liveCount;
    }
    ~CountedLineType() override { --m_liveCount; }

private:
    int& m_liveCount;
};

// An entry as the import will make it from an archived LTYPE record.
LC_LineType* makeRecord(const QString& name, std::vector<double> pattern,
                        const QString& description = QString()) {
    auto* entry = new LC_LineType(name);
    entry->description = description;
    entry->pattern = std::move(pattern);
    entry->hasImportedRecord = true;
    return entry;
}

} // namespace

TEST_CASE("LC_LineTypeList seeds the 35 built-ins in writeLTypes() order",
          "[linetype][list][seed]") {
    LC_LineTypeList list;
    REQUIRE(list.count() == 35);
    CHECK(list.at(0)->name == "CONTINUOUS");
    CHECK(list.at(1)->name == "ByLayer");
    CHECK(list.at(2)->name == "ByBlock");
    CHECK(list.at(34)->name == "PHANTOMX2");

    const auto& rows = LC_LineTypeNames::builtinMetrics();
    REQUIRE(rows.size() == list.count());
    for (unsigned i = 0; i < list.count(); ++i) {
        const LC_LineType* entry = list.at(i);
        REQUIRE(entry != nullptr);
        INFO("entry " << i << " " << entry->name.toStdString());
        CHECK(entry->name == LC_LineTypeNames::lineTypeToName(rows[i].type));
        CHECK(entry->description == QString::fromUtf8(rows[i].description));
        CHECK(entry->pattern == rows[i].pattern);
        CHECK(entry->origin == LC_LineType::Origin::BuiltIn);
        CHECK_FALSE(entry->hasImportedRecord);
        CHECK(entry->key() == LC_LineTypeNames::foldName(entry->name));
    }
    CHECK_FALSE(list.isModified());
}

TEST_CASE("LC_LineTypeNames::builtinMetrics lists the 35 rows of writeLTypes()' head",
          "[linetype][metrics]") {
    const auto& rows = LC_LineTypeNames::builtinMetrics();
    REQUIRE(rows.size() == std::size(kHeadRows));

    std::set<QString> names;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const HeadRow& head = kHeadRows[i];
        const QString name = LC_LineTypeNames::lineTypeToName(rows[i].type);
        INFO("row " << i << " " << head.name);
        CHECK(rows[i].type == head.type);
        CHECK(name == head.name);
        CHECK_FALSE(name.isEmpty());
        CHECK(names.insert(name).second);
        CHECK(rows[i].description != nullptr);
        CHECK(rows[i].pattern.size() == static_cast<std::size_t>(head.size));
        // The double the old call passed, which a DWG stores as is.
        CHECK(sameBits(rows[i].length, head.length));
    }

    // The three rows without dashes are written whole from an imported record
    // (writeLType()'s `ltype = *source`); they were "Solid line", "" and "".
    CHECK(QString::fromUtf8(rows[0].description) == "Solid line");
    CHECK(QString::fromUtf8(rows[1].description).isEmpty());
    CHECK(QString::fromUtf8(rows[2].description).isEmpty());
    for (std::size_t i = 0; i < 3; ++i) {
        INFO("row " << i);
        CHECK(rows[i].pattern.empty());
        CHECK(rows[i].length == 0.0);
    }
    // Every other row has dashes: the walk's family lookup needs them.
    for (std::size_t i = 3; i < rows.size(); ++i) {
        INFO("row " << i);
        CHECK_FALSE(rows[i].pattern.empty());
        CHECK(rows[i].length > 0.0);
    }
}

TEST_CASE("LC_LineTypeList::find resolves a name by its folded key",
          "[linetype][list][fold]") {
    LC_LineTypeList list;
    LC_LineType* dashed = list.find(QStringLiteral("DASHED"));
    REQUIRE(dashed != nullptr);
    CHECK(dashed == list.at(7));

    // ASCII case and blanks around the name do not make another name: the
    // pen trims, LC_LineTypeNames::foldName folds a-z.
    CHECK(list.find(QStringLiteral("dashed")) == dashed);
    CHECK(list.find(QStringLiteral(" Dashed ")) == dashed);
    CHECK(list.find(QStringLiteral("DASHED2")) != dashed);
    CHECK(list.find(QStringLiteral("DASHED2")) != nullptr);
    CHECK(list.find(QStringLiteral("BYLAYER")) == list.at(1));
    CHECK(list.find(QStringLiteral("bylayer")) == list.at(1));
    CHECK(list.find(QStringLiteral("NO_SUCH")) == nullptr);
    CHECK(list.find(QString()) == nullptr);

    // NFC and NFD spellings of one name fold alike.
    const QString nfc = QString::fromUtf16(u"\u00D6lfarbe");
    const QString nfd = QString::fromUtf16(u"O\u0308lfarbe");
    REQUIRE(nfc != nfd);
    LC_LineType* custom = list.add(makeRecord(nfc, {5.0, -2.5}));
    REQUIRE(custom != nullptr);
    CHECK(list.count() == 36);
    CHECK(list.find(nfd) == custom);
    CHECK(list.find(QString::fromUtf16(u"\u00D6LFARBE")) == custom);
    CHECK(custom->key() == LC_LineTypeNames::foldName(nfd));
    // foldName folds a-z only (rs_pen folds the same way), so a lower-case
    // O-umlaut is another key.
    CHECK(list.find(QString::fromUtf16(u"\u00F6lfarbe")) == nullptr);
}

TEST_CASE("LC_LineTypeList::add keeps one entry per key: unpadded, then byte order, then the later one",
          "[linetype][list][twin]") {
    // The archive's own order (std::map<std::string, DRW_LType>, overwritten
    // on a repeated name), applied to each pair so arrival order is moot.
    struct Twin {
        QString name;
        double dash;
    };
    const auto check = [](const Twin& first, const Twin& second,
                          const Twin& winner) {
        LC_LineTypeList list;
        LC_LineType* survivor = list.add(
            makeRecord(first.name, {first.dash, -first.dash},
                       QString::number(first.dash)));
        REQUIRE(survivor != nullptr);
        CHECK(list.add(makeRecord(second.name, {second.dash, -second.dash},
                                  QString::number(second.dash))) == survivor);
        CHECK(list.count() == 36);
        // Spelling, dashes and description move as one unit.
        CHECK(survivor->name == winner.name);
        CHECK(survivor->pattern ==
              std::vector<double>{winner.dash, -winner.dash});
        CHECK(survivor->description == QString::number(winner.dash));
        CHECK(survivor->origin == LC_LineType::Origin::Imported);
        CHECK(survivor->hasImportedRecord);
        CHECK(list.find(QStringLiteral("vendor")) == survivor);
    };
    const Twin upper{QStringLiteral("VENDOR"), 2.0};
    const Twin mixed{QStringLiteral("Vendor"), 1.0};
    const Twin padded{QStringLiteral(" VENDOR"), 3.0};
    const Twin padded2{QStringLiteral("  vendor"), 4.0};
    const Twin upperAgain{QStringLiteral("VENDOR"), 5.0};

    SECTION("case-only twins: the byte-order-first spelling, either way") {
        check(mixed, upper, upper);
        check(upper, mixed, upper);
    }
    SECTION("a padded spelling loses to an unpadded one even when its bytes sort first") {
        check(padded, upper, upper);
        check(upper, padded, upper);
    }
    SECTION("two padded spellings: byte order") {
        check(padded, padded2, padded2);
        check(padded2, padded, padded2);
    }
    SECTION("the same spelling twice: the later record, as the archive overwrites") {
        check(upper, upperAgain, upperAgain);
    }
}

TEST_CASE("LC_LineTypeList::add on a built-in: dashes replace the seed, a name-only record keeps it",
          "[linetype][list][twin]") {
    LC_LineTypeList list;
    LC_LineType* seed = list.find(QStringLiteral("DASHED"));
    REQUIRE(seed != nullptr);
    const std::vector<double> literal = seed->pattern;
    const QString description = seed->description;
    REQUIRE(literal == std::vector<double>{12.7, -6.35});

    SECTION("a record with dashes replaces the seed's dashes and description") {
        CHECK(list.add(makeRecord(QStringLiteral("DASHED"), {64.0, -32.0},
                                  QStringLiteral("File dashed"))) == seed);
        CHECK(list.count() == 35);
        CHECK(seed->name == "DASHED");
        CHECK(seed->pattern == std::vector<double>{64.0, -32.0});
        CHECK(seed->description == "File dashed");
        CHECK(seed->origin == LC_LineType::Origin::BuiltIn);
        CHECK(seed->hasImportedRecord);
    }
    SECTION("a record without dashes names the built-in but does not erase it") {
        // What writeLType() does with such a record: keeps the literal dashes.
        CHECK(list.add(makeRecord(QStringLiteral("DASHED"), {})) == seed);
        CHECK(list.count() == 35);
        CHECK(seed->pattern == literal);
        CHECK(seed->description == description);
        CHECK(seed->origin == LC_LineType::Origin::BuiltIn);
        CHECK(seed->hasImportedRecord);
    }
    SECTION("a record without dashes under another spelling takes the spelling, not the dashes") {
        // writeLType() writes such a record under its own name with the
        // literal dashes; the entry shows the same.
        CHECK(list.add(makeRecord(QStringLiteral("Dashed"), {})) == seed);
        CHECK(seed->name == "Dashed");
        CHECK(seed->pattern == literal);
        CHECK(seed->description == description);
        CHECK(list.find(QStringLiteral("DASHED")) == seed);
    }
    SECTION("two eligible records: the byte-order-first spelling wins either way") {
        // "DASHED" sorts before "dashed"; both pass tableNameEquals, so the
        // table and writeLType()'s substitution pick the same record.
        for (const bool upperFirst : {true, false}) {
            INFO("upper first " << upperFirst);
            LC_LineTypeList fresh;
            LC_LineType* entry = fresh.find(QStringLiteral("DASHED"));
            REQUIRE(entry != nullptr);
            LC_LineType* upper = makeRecord(QStringLiteral("DASHED"), {2.0, -2.0});
            LC_LineType* lower = makeRecord(QStringLiteral("dashed"), {1.0, -1.0});
            CHECK(fresh.add(upperFirst ? upper : lower) == entry);
            CHECK(fresh.add(upperFirst ? lower : upper) == entry);
            CHECK(fresh.count() == 35);
            CHECK(entry->name == "DASHED");
            CHECK(entry->pattern == std::vector<double>{2.0, -2.0});
            CHECK(entry->origin == LC_LineType::Origin::BuiltIn);
            CHECK(entry->hasImportedRecord);
        }
    }
    SECTION("a name-only record for a built-in without dashes is taken whole") {
        // writeLType() has no literal to keep for CONTINUOUS: ltype = *source.
        LC_LineType* continuous = list.find(QStringLiteral("CONTINUOUS"));
        REQUIRE(continuous != nullptr);
        CHECK(list.add(makeRecord(QStringLiteral("CONTINUOUS"), {})) == continuous);
        CHECK(continuous->description.isEmpty());
        CHECK(continuous->pattern.empty());
        CHECK(continuous->hasImportedRecord);
    }
    SECTION("a padded record is the writer's own LTYPE: the built-in stays as it is") {
        // tableNameEquals() wants the built-in's length, so writeLType() keeps
        // the literal and writeLTypes()' tail emits " DASHED" as a record.
        CHECK(list.add(makeRecord(QStringLiteral(" DASHED"), {9.0, -9.0})) == seed);
        CHECK(list.count() == 35);
        CHECK(seed->name == "DASHED");
        CHECK(seed->pattern == literal);
        CHECK(seed->description == description);
        CHECK_FALSE(seed->hasImportedRecord);
        CHECK_FALSE(list.isModified());
        // Nor does it keep the first eligible record from taking the seed.
        CHECK(list.add(makeRecord(QStringLiteral("Dashed"), {1.0, -1.0})) == seed);
        CHECK(seed->name == "Dashed");
        CHECK(seed->pattern == std::vector<double>{1.0, -1.0});
        CHECK(seed->hasImportedRecord);
    }
    SECTION("a non-ASCII spelling that folds to a built-in's key is no record for it either") {
        // NFC folds U+212A KELVIN SIGN to K; the writer's byte compare does not.
        LC_LineType* byBlock = list.find(QStringLiteral("ByBlock"));
        REQUIRE(byBlock != nullptr);
        LC_LineType* kelvin = makeRecord(QString::fromUtf16(u"ByBloc\u212A"), {1.0, -1.0});
        CHECK(list.add(kelvin) == byBlock);
        CHECK(list.count() == 35);
        CHECK(byBlock->name == "ByBlock");
        CHECK_FALSE(byBlock->hasImportedRecord);
    }
}

TEST_CASE("LC_LineTypeList owns the entries it keeps and frees the ones it merges",
          "[linetype][list][ownership]") {
    int live = 0;
    {
        LC_LineTypeList list;
        auto* kept = new CountedLineType(QStringLiteral("VENDOR"), live);
        kept->pattern = {2.0, -2.0};
        REQUIRE(list.add(kept) == kept);
        REQUIRE(list.count() == 36);
        REQUIRE(live == 1);

        // A twin that loses is freed on the spot, the entry untouched.
        auto* loser = new CountedLineType(QStringLiteral("Vendor"), live);
        loser->pattern = {1.0, -1.0};
        REQUIRE(live == 2);
        CHECK(list.add(loser) == kept);
        CHECK(live == 1);
        CHECK(kept->pattern == std::vector<double>{2.0, -2.0});

        // A twin that wins is merged into the entry and freed: the entry
        // keeps its address, so pointers into the list stay valid.
        auto* winner = new CountedLineType(QStringLiteral("VENDOR"), live);
        winner->pattern = {4.0, -4.0};
        CHECK(list.add(winner) == kept);
        CHECK(live == 1);
        CHECK(kept->pattern == std::vector<double>{4.0, -4.0});

        CHECK(list.add(kept) == kept);
        CHECK(live == 1);
        CHECK(list.add(nullptr) == nullptr);
        CHECK(list.count() == 36);
    }
    CHECK(live == 0);
}

TEST_CASE("LC_LineTypeList::clear frees the custom entries and reseeds the built-ins",
          "[linetype][list][ownership]") {
    int live = 0;
    LC_LineTypeList list;
    auto* custom = new CountedLineType(QStringLiteral("VENDOR"), live);
    custom->pattern = {2.0, -2.0};
    REQUIRE(list.add(custom) == custom);
    LC_LineType* dashed = list.find(QStringLiteral("DASHED"));
    REQUIRE(dashed != nullptr);
    REQUIRE(list.add(makeRecord(QStringLiteral("DASHED"), {64.0, -32.0})) == dashed);
    REQUIRE(dashed->hasImportedRecord);
    REQUIRE(live == 1);

    list.clear();
    CHECK(live == 0);
    CHECK(list.count() == 35);
    CHECK(list.find(QStringLiteral("VENDOR")) == nullptr);
    dashed = list.find(QStringLiteral("DASHED"));
    REQUIRE(dashed != nullptr);
    CHECK(dashed == list.at(7));
    CHECK(dashed->pattern == std::vector<double>{12.7, -6.35});
    CHECK(dashed->origin == LC_LineType::Origin::BuiltIn);
    CHECK_FALSE(dashed->hasImportedRecord);

    // initForNewDocument() may run more than once on a drawing.
    list.clear();
    CHECK(list.count() == 35);
    CHECK(list.at(0)->name == "CONTINUOUS");
}

TEST_CASE("LC_LineTypeList starts clean and add() dirties it as RS_LayerList::add does",
          "[linetype][list]") {
    LC_LineTypeList list;
    CHECK_FALSE(list.isModified());
    list.setModified(true);
    CHECK(list.isModified());
    list.setModified(false);
    CHECK_FALSE(list.isModified());

    // A new entry dirties the list like a new layer does; the drawing
    // clears the flag when a load ends.
    list.add(makeRecord(QStringLiteral("VENDOR"), {2.0, -2.0}));
    CHECK(list.isModified());
    list.setModified(false);
    list.add(makeRecord(QStringLiteral("Vendor"), {1.0, -1.0}));
    CHECK_FALSE(list.isModified());
    // The same record again, byte for byte.
    list.add(makeRecord(QStringLiteral("VENDOR"), {2.0, -2.0}));
    CHECK_FALSE(list.isModified());
    list.add(makeRecord(QStringLiteral("DASHED"), {64.0, -32.0}));
    CHECK(list.isModified());
}
