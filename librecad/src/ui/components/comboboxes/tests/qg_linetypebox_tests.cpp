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
**********************************************************************/

// The linetype combo offers a drawing's table beside the built-ins: a table
// entry is a row that writes its own spelling into a pen, a built-in row writes
// its enum unless the drawing redefines it, and an unknown name gets one
// temporary row. Without a list the combo is exactly the built-in combo.

#include <cstdlib>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <QAbstractItemView>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QIcon>
#include <QImage>
#include <QList>
#include <QPalette>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "lc_actiontestsupport.h"
#include "lc_iconengineshared.h"
#include "lc_linetype.h"
#include "lc_linetypelist.h"
#include "lc_linetypenames.h"
#include "qg_linetypebox.h"
#include "rs_color.h"
#include "rs_graphic.h"
#include "rs_pen.h"

namespace {

// An imported record as the filter will make it from an LTYPE entry.
void addLineType(RS_Graphic& graphic, const QString& name, const std::vector<double>& pattern,
                 const QString& description = QString()) {
    auto* entry = new LC_LineType(name);
    entry->pattern = pattern;
    entry->description = description;
    entry->origin = LC_LineType::Origin::Imported;
    graphic.addLineType(entry);
}

struct LitRun {
    int start;
    int length;
};

qreal pixelRatio(const QImage& image) {
    return image.devicePixelRatio() > 0.0 ? image.devicePixelRatio() : 1.0;
}

// The lit runs along the middle row of a picture, in logical pixels; the lci
// resources have a transparent background, so alpha is the only thing to look at.
std::vector<LitRun> litRuns(const QImage& image) {
    const qreal ratio = pixelRatio(image);
    std::vector<LitRun> runs;
    const int y = image.height() / 2;
    int x = 0;
    while (x < image.width()) {
        while (x < image.width() && image.pixelColor(x, y).alpha() <= 32) {
            ++x;
        }
        const int start = x;
        while (x < image.width() && image.pixelColor(x, y).alpha() > 32) {
            ++x;
        }
        if (x > start) {
            const int logicalStart = qRound(start / ratio);
            const int logicalEnd = qRound(x / ratio);
            runs.push_back({logicalStart, logicalEnd - logicalStart});
        }
    }
    return runs;
}

// A picture drawn in whole device pixels keeps the whole-pixel layout the
// pixel-exact cases describe; another ratio leaves them out.
void skipUnlessWholePixelRatio(const QImage& image) {
    const qreal ratio = pixelRatio(image);
    if (std::abs(ratio - qRound(ratio)) > 0.001) {
        SKIP("the picture is not drawn in whole device pixels");
    }
}

int logicalWidth(const QImage& image) {
    return qRound(image.width() / pixelRatio(image));
}

QImage rowImage(const QG_LineTypeBox& box, int row, QIcon::Mode mode = QIcon::Normal) {
    return box.itemIcon(row).pixmap(box.iconSize(), mode).toImage().convertToFormat(QImage::Format_ARGB32);
}

QColor firstLitColor(const QImage& image) {
    const int y = image.height() / 2;
    for (int x = 0; x < image.width(); ++x) {
        const QColor color = image.pixelColor(x, y);
        if (color.alpha() > 32) {
            return color;
        }
    }
    return QColor();
}

RS_Pen namedPen(const QString& name) {
    RS_Pen pen(RS_Color(Qt::red), RS2::Width00, RS2::SolidLine);
    pen.setLineTypeName(name);
    return pen;
}

QString mainColorKey(const int mode, const int state) {
    return LC_SVGIconEngineAPI::getColorAppKeyName(
        QString::fromLatin1(LC_SVGIconEngineAPI::KEY_COLOR_MAIN), mode, state);
}

/// Sets the main-color properties the icon engine reads and puts the previous
/// values back afterwards, so no case depends on what ran before it.
class IconColorGuard {
public:
    IconColorGuard()
        : m_anyKey(mainColorKey(LC_SVGIconEngineAPI::AnyMode,
                                 LC_SVGIconEngineAPI::AnyState)),
          m_keys{m_anyKey,
                 mainColorKey(QIcon::Normal, QIcon::Off),
                 mainColorKey(QIcon::Active, QIcon::Off),
                 mainColorKey(QIcon::Selected, QIcon::Off),
                 mainColorKey(QIcon::Disabled, QIcon::Off)} {
        for (const QString& key : m_keys) {
            m_values.append(qApp->property(key.toUtf8().constData()));
        }
    }
    ~IconColorGuard() {
        for (int i = 0; i < m_keys.size(); ++i) {
            setProperty(m_keys.at(i), m_values.at(i));
        }
    }
    void clear() {
        for (const QString& key : m_keys) {
            setProperty(key, QVariant());
        }
    }
    void setAny(const QColor& color) {
        clear();
        setProperty(m_anyKey, QVariant(color.name()));
    }
    void setMode(const QIcon::Mode mode, const QColor& color) {
        setProperty(mainColorKey(mode, QIcon::Off), QVariant(color.name()));
    }
private:
    static void setProperty(const QString& key, const QVariant& value) {
        qApp->setProperty(key.toUtf8().constData(), value);
    }
    QString m_anyKey;
    QStringList m_keys;
    QList<QVariant> m_values;
};

} // namespace

TEST_CASE("A drawing line type is a row of the combo", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});
    addLineType(graphic, "VENDOR_DOT", {0.0, -5.0, 0.0, -5.0, 1.0, -5.0});

    QG_LineTypeBox builtIn;
    builtIn.init(true, false);
    const int builtInRows = builtIn.count();

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);

    // The special rows, then the two table rows sorted by key, then one
    // separator and the built-in rows.
    REQUIRE(box.count() == builtInRows + 1 + 2);
    const int tabRow = box.findText(QStringLiteral("VENDOR_TAB"));
    const int dotRow = box.findText(QStringLiteral("VENDOR_DOT"));
    REQUIRE(tabRow >= 0);
    REQUIRE(dotRow >= 0);
    CHECK(box.itemText(tabRow + 1).isEmpty());
    CHECK(tabRow < builtInRows);
    // Sorted by LC_LineType::key().
    CHECK(dotRow < tabRow);

    // The seeds are the built-in rows already: no table row repeats one.
    CHECK(box.findText(QStringLiteral("CONTINUOUS")) < 0);
    CHECK(box.findText(QStringLiteral("DASHED")) < 0);
    CHECK(box.findText(QStringLiteral("ByLayer")) < 0);

    RS_Pen pen;
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_TAB")));
    CHECK(box.currentIndex() == tabRow);
    box.applyLineTypeTo(pen);
    CHECK(pen.hasLineTypeName());
    CHECK(pen.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
    CHECK(pen.getLineType() == RS2::SolidLine);
}

TEST_CASE("A built-in row writes the enum unless the drawing redefines it", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);

    // Only the seed: the row still means the enum, as every existing host needs.
    box.setLineType(RS2::DashLine);
    RS_Pen plain;
    box.applyLineTypeTo(plain);
    CHECK_FALSE(plain.hasLineTypeName());
    CHECK(plain.getLineType() == RS2::DashLine);

    // A record with other dashes makes the drawing paint the built-in from its
    // table, so the row must carry the canonical name.
    addLineType(graphic, "DASHED", {64.0, -32.0});
    box.rebuild(&graphic.getLineTypeList());
    box.setLineType(RS2::DashLine);
    RS_Pen redefined;
    box.applyLineTypeTo(redefined);
    CHECK(redefined.hasLineTypeName());
    CHECK(redefined.getLineTypeName() == QStringLiteral("DASHED"));
    CHECK(redefined.getLineType() == RS2::DashLine);
}

TEST_CASE("An alias spelling the drawing holds has its own row", "[gui][linetype][combo]") {
    (void)lc::test::application();
    REQUIRE(LC_LineTypeNames::nameToLineType(QStringLiteral("ACAD_ISO09W100")) == RS2::PhantomLine);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "ACAD_ISO09W100", {24.0, -3.0, 6.0, -3.0, 6.0, -3.0});

    QG_LineTypeBox builtIn;
    builtIn.init(true, false);

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const int row = box.findText(QStringLiteral("ACAD_ISO09W100"));
    REQUIRE(row >= 0);
    CHECK(row < builtIn.count());

    RS_Pen pen;
    box.setLineTypeOf(namedPen(QStringLiteral("ACAD_ISO09W100")));
    CHECK(box.currentIndex() == row);
    box.applyLineTypeTo(pen);
    CHECK(pen.getLineTypeName() == QStringLiteral("ACAD_ISO09W100"));
    CHECK(pen.getLineType() == RS2::PhantomLine);
}

TEST_CASE("A name the drawing lacks gets one temporary row", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    // The separator that introduces the table row is already there.
    const int baselineWithTable = box.count();

    box.setLineTypeOf(namedPen(QStringLiteral("NOT_IN_LIST")));
    CHECK(box.currentIndex() >= 0);
    RS_Pen pen;
    box.applyLineTypeTo(pen);
    CHECK(pen.getLineTypeName() == QStringLiteral("NOT_IN_LIST"));
    CHECK(box.count() == baselineWithTable + 1);

    // Another unknown name reuses the one temporary row.
    box.setLineTypeOf(namedPen(QStringLiteral("ALSO_MISSING")));
    CHECK(box.count() == baselineWithTable + 1);
    box.rebuild(&graphic.getLineTypeList());
    CHECK(box.count() == baselineWithTable + 1);
    box.rebuild(&graphic.getLineTypeList());
    CHECK(box.count() == baselineWithTable + 1);

    // A known name drops it.
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_TAB")));
    CHECK(box.count() == baselineWithTable);
    box.setLineTypeOf(RS_Pen(RS_Color(Qt::black), RS2::Width00, RS2::DashLine));
    CHECK(box.count() == baselineWithTable);
}

TEST_CASE("An untouched selection gives the pen back its own line type", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    // Another spelling of the same folded key.
    const RS_Pen source = namedPen(QStringLiteral("vendor_tab"));
    REQUIRE(source.hasLineTypeName());

    box.setLineTypeOf(source);
    RS_Pen back;
    box.applyLineTypeTo(back);
    CHECK(back.getLineTypeId() == source.getLineTypeId());
    CHECK(back.getLineTypeName() == QStringLiteral("vendor_tab"));

    // Leaving the row and returning keeps the remembered source pen.
    const int sourceRow = box.currentIndex();
    box.setLineType(RS2::DashLine);
    box.setCurrentIndex(sourceRow);
    RS_Pen again;
    box.applyLineTypeTo(again);
    CHECK(again.getLineTypeId() == source.getLineTypeId());

    // On another row the selection writes that row.
    box.setLineType(RS2::DashLine);
    RS_Pen other;
    box.applyLineTypeTo(other);
    CHECK(other.getLineType() == RS2::DashLine);
    CHECK_FALSE(other.hasLineTypeName());
}

TEST_CASE("A rebuild is silent and keeps the selection", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic first;
    first.initForNewDocument();
    addLineType(first, "VENDOR_TAB", {10.0, -5.0});
    RS_Graphic second;
    second.initForNewDocument();
    addLineType(second, "VENDOR_TAB", {12.0, -6.0});
    RS_Graphic third;
    third.initForNewDocument();

    QG_LineTypeBox box;
    box.init(&first.getLineTypeList(), true, false);
    int emissions = 0;
    QObject::connect(&box, &QG_LineTypeBox::lineTypeChanged,
                     [&emissions](RS2::LineType) { ++emissions; });
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_TAB")));
    emissions = 0;

    box.rebuild(&second.getLineTypeList());
    CHECK(emissions == 0);
    RS_Pen secondPen;
    box.applyLineTypeTo(secondPen);
    CHECK(secondPen.getLineTypeName() == QStringLiteral("VENDOR_TAB"));

    box.rebuild(&third.getLineTypeList());
    CHECK(emissions == 0);
    RS_Pen thirdPen;
    box.applyLineTypeTo(thirdPen);
    CHECK(thirdPen.getLineTypeName() == QStringLiteral("VENDOR_TAB"));

    box.rebuild(nullptr);
    CHECK(emissions == 0);
    CHECK(box.findText(QStringLiteral("VENDOR_TAB")) < 0);
    QG_LineTypeBox builtIn;
    builtIn.init(true, false);
    CHECK(box.count() == builtIn.count());
}

TEST_CASE("The combo without a list is the built-in combo", "[gui][linetype][combo]") {
    (void)lc::test::application();
    QG_LineTypeBox box;
    box.init(true, true, true);
    REQUIRE(box.count() == 37);

    // Every built-in enum round-trips; the Unchanged row reports through
    // isUnchanged() instead of getLineType(), exactly as before.
    int builtInRows = 0;
    for (int i = 0; i < box.count(); ++i) {
        const int data = box.itemData(i).toInt();
        if (data == static_cast<int>(RS2::LineTypeUnchanged)) {
            continue;
        }
        ++builtInRows;
        box.setLineType(static_cast<RS2::LineType>(data));
        CHECK(box.getLineType() == static_cast<RS2::LineType>(data));
    }
    CHECK(builtInRows == box.count() - 1);

    const int dashSmall = box.findData(static_cast<int>(RS2::DashLine2));
    REQUIRE(dashSmall >= 0);
    CHECK(box.itemText(dashSmall) == QG_LineTypeBox::tr("Dash (small)"));

    box.setLineType(RS2::DashLine);
    RS_Pen pen;
    box.applyLineTypeTo(pen);
    CHECK_FALSE(pen.hasLineTypeName());
    CHECK(pen.getLineType() == RS2::DashLine);

    QG_LineTypeBox other;
    other.init(nullptr, true, true, true);
    REQUIRE(other.count() == box.count());
    for (int i = 0; i < box.count(); ++i) {
        CHECK(other.itemText(i) == box.itemText(i));
        CHECK(other.itemData(i).toInt() == box.itemData(i).toInt());
    }
}

TEST_CASE("A rebuild forgets a pen whose row the user left", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_TAB")));
    // The user moves to another row; the remembered pen no longer applies.
    box.setLineType(RS2::DashLine);
    box.rebuild(&graphic.getLineTypeList());

    CHECK(box.getLineType() == RS2::DashLine);
    RS_Pen pen;
    box.applyLineTypeTo(pen);
    CHECK(pen.getLineType() == RS2::DashLine);
    CHECK_FALSE(pen.hasLineTypeName());
}

TEST_CASE("Without a list the combo still keeps the pen it was set from", "[gui][linetype][combo]") {
    (void)lc::test::application();
    QG_LineTypeBox box;
    box.init(true, false);
    const int rows = box.count();
    const RS_Pen source = namedPen(QStringLiteral("VENDOR_TAB"));

    box.setLineTypeOf(source);
    // No list: the row count never grows and the name stays in the pen.
    CHECK(box.count() == rows);
    RS_Pen back;
    box.applyLineTypeTo(back);
    CHECK(back.getLineTypeId() == source.getLineTypeId());
    CHECK(back.getLineTypeName() == QStringLiteral("VENDOR_TAB"));

    const int sourceRow = box.currentIndex();
    box.setLineType(RS2::DashLine);
    RS_Pen other;
    box.applyLineTypeTo(other);
    CHECK(other.getLineType() == RS2::DashLine);
    CHECK_FALSE(other.hasLineTypeName());

    box.setCurrentIndex(sourceRow);
    RS_Pen again;
    box.applyLineTypeTo(again);
    CHECK(again.getLineTypeId() == source.getLineTypeId());
    CHECK(again.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
}

TEST_CASE("A silent rebuild reports the row it restored", "[gui][linetype][combo]") {
    (void)lc::test::application();
    REQUIRE(LC_LineTypeNames::nameToLineType(QStringLiteral("ACAD_ISO09W100")) == RS2::PhantomLine);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "ACAD_ISO09W100", {24.0, -3.0, 6.0, -3.0, 6.0, -3.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    box.setLineTypeOf(namedPen(QStringLiteral("ACAD_ISO09W100")));
    box.rebuild(nullptr);

    CHECK(box.getLineType() == RS2::PhantomLine);
    CHECK(box.currentIndex() == box.findData(static_cast<int>(RS2::PhantomLine)));
}

TEST_CASE("A separator leaves with the temporary row", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const int base = box.count();
    box.setLineTypeOf(namedPen(QStringLiteral("NOT_IN_LIST")));
    // One separator and one temporary row, as no table row exists yet.
    CHECK(box.count() == base + 2);
    box.setLineTypeOf(RS_Pen(RS_Color(Qt::black), RS2::Width00, RS2::DashLine));
    CHECK(box.count() == base);
}

TEST_CASE("A canonical spelling without a row gets no temporary row", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), false, false);
    const int base = box.count();
    box.setLineTypeOf(namedPen(QStringLiteral("BYLAYER")));
    CHECK(box.count() == base);
    CHECK(box.findText(QG_LineTypeBox::tr("%1 (not in this drawing)").arg(QStringLiteral("BYLAYER"))) < 0);
}

TEST_CASE("Blanks around a table spelling do not make another name", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    // A record keeps the blanks of its name; a pen trims them.
    addLineType(graphic, "VENDOR_PAD ", {10.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const int base = box.count();
    const int padRow = box.findText(QStringLiteral("VENDOR_PAD "));
    REQUIRE(padRow >= 0);

    // The table's row, not a temporary one.
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_PAD")));
    CHECK(box.count() == base);
    CHECK(box.currentIndex() == padRow);

    // A rebuild finds the row again.
    box.rebuild(&graphic.getLineTypeList());
    CHECK(box.count() == base);
    CHECK(box.currentIndex() == padRow);

    // The row a user picks writes the name a pen holds.
    QG_LineTypeBox other;
    other.init(&graphic.getLineTypeList(), true, false);
    other.setCurrentIndex(padRow);
    RS_Pen pen;
    other.applyLineTypeTo(pen);
    CHECK(pen.getLineTypeName() == QStringLiteral("VENDOR_PAD"));
}

TEST_CASE("A host that reads the pen inside the signal gets the pen it set",
          "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    SECTION("with a list") {
        QG_LineTypeBox box;
        box.init(&graphic.getLineTypeList(), true, false);
        const RS_Pen source = namedPen(QStringLiteral("vendor_tab"));
        RS_Pen seen;
        bool read = false;
        QObject::connect(&box, &QG_LineTypeBox::lineTypeChanged, [&](RS2::LineType) {
            box.applyLineTypeTo(seen);
            read = true;
        });
        box.setLineTypeOf(source);
        REQUIRE(read);
        CHECK(seen.getLineTypeId() == source.getLineTypeId());
        CHECK(seen.getLineTypeName() == QStringLiteral("vendor_tab"));
    }

    SECTION("without a list") {
        QG_LineTypeBox box;
        box.init(true, false);
        const RS_Pen source = namedPen(QStringLiteral("VENDOR_TAB"));
        RS_Pen seen;
        bool read = false;
        QObject::connect(&box, &QG_LineTypeBox::lineTypeChanged, [&](RS2::LineType) {
            box.applyLineTypeTo(seen);
            read = true;
        });
        box.setLineTypeOf(source);
        REQUIRE(read);
        CHECK(seen.hasLineTypeName());
        CHECK(seen.getLineTypeId() == source.getLineTypeId());
        CHECK(seen.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
    }
}

TEST_CASE("A drawing line type row previews its dashes", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});
    addLineType(graphic, "VENDOR_SOLID", {});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const int tabRow = box.findText(QStringLiteral("VENDOR_TAB"));
    REQUIRE(tabRow >= 0);
    const QImage tab = rowImage(box, tabRow);
    REQUIRE(!tab.isNull());
    skipUnlessWholePixelRatio(tab);
    // The stock pictures' 32x12 area, centred in the icon.
    const int areaX = (logicalWidth(tab) - 32) / 2;
    const std::vector<LitRun> runs = litRuns(tab);
    REQUIRE(runs.size() >= 2);
    CHECK(runs.front().start == areaX);
    CHECK(runs.at(1).start > runs.front().start + runs.front().length);

    const int solidRow = box.findText(QStringLiteral("VENDOR_SOLID"));
    REQUIRE(solidRow >= 0);
    const QImage solid = rowImage(box, solidRow);
    skipUnlessWholePixelRatio(solid);
    const std::vector<LitRun> solidRuns = litRuns(solid);
    REQUIRE(solidRuns.size() == 1);
    CHECK(solidRuns.front().start == areaX);
    CHECK(solidRuns.front().length == 32);
}

TEST_CASE("Different dashes give different pictures", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});
    addLineType(graphic, "VENDOR_DOT", {0.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const int tabRow = box.findText(QStringLiteral("VENDOR_TAB"));
    const int dotRow = box.findText(QStringLiteral("VENDOR_DOT"));
    REQUIRE(tabRow >= 0);
    REQUIRE(dotRow >= 0);
    const QImage tab = rowImage(box, tabRow);
    const QImage dot = rowImage(box, dotRow);
    CHECK(tab != dot);
    const std::vector<LitRun> dotRuns = litRuns(dot);
    REQUIRE(dotRuns.size() >= 3);
    for (const LitRun& run : dotRuns) {
        CHECK(run.length <= 4);
    }
}

TEST_CASE("A redefined built-in row shows the drawing's dashes", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic seed;
    seed.initForNewDocument();
    RS_Graphic redefined;
    redefined.initForNewDocument();
    addLineType(redefined, "DASHED", {64.0, -32.0});

    QG_LineTypeBox plain;
    plain.init(&seed.getLineTypeList(), true, false);
    QG_LineTypeBox shown;
    shown.init(&redefined.getLineTypeList(), true, false);
    // The built-in row by its text: findData() would answer with the drawing's
    // row of the same enum first.
    const int plainDash = plain.findText(QG_LineTypeBox::tr("Dash"));
    const int shownDash = shown.findText(QG_LineTypeBox::tr("Dash"));
    REQUIRE(plainDash >= 0);
    REQUIRE(shownDash >= 0);
    CHECK(rowImage(plain, plainDash) != rowImage(shown, shownDash));

    const int plainContinuous = plain.findText(QG_LineTypeBox::tr("Continuous"));
    const int shownContinuous = shown.findText(QG_LineTypeBox::tr("Continuous"));
    REQUIRE(plainContinuous >= 0);
    REQUIRE(shownContinuous >= 0);
    CHECK(rowImage(plain, plainContinuous) == rowImage(shown, shownContinuous));
    const int plainDot = plain.findText(QG_LineTypeBox::tr("Dot"));
    const int shownDot = shown.findText(QG_LineTypeBox::tr("Dot"));
    REQUIRE(plainDot >= 0);
    REQUIRE(shownDot >= 0);
    CHECK(rowImage(plain, plainDot) == rowImage(shown, shownDot));
}

TEST_CASE("A name without a definition shows that it has none", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});
    addLineType(graphic, "VENDOR_SOLID", {});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_GONE")));
    const int temporary = box.currentIndex();
    REQUIRE(temporary >= 0);
    const QImage missing = rowImage(box, temporary);
    REQUIRE(!missing.isNull());
    const int tabRow = box.findText(QStringLiteral("VENDOR_TAB"));
    const int solidRow = box.findText(QStringLiteral("VENDOR_SOLID"));
    REQUIRE(tabRow >= 0);
    REQUIRE(solidRow >= 0);
    CHECK(missing != rowImage(box, tabRow));
    CHECK(missing != rowImage(box, solidRow));
}

TEST_CASE("The combo says how the drawing defines a pen's line type",
          "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic first;
    first.initForNewDocument();
    addLineType(first, "VENDOR_TAB", {10.0, -5.0}, QStringLiteral("Tabulated"));
    addLineType(first, "VENDOR_SOLID", {});
    // A record keeps the blanks around its name; the pen's name is trimmed.
    addLineType(first, "VENDOR_PAD ", {8.0, -2.0});

    QG_LineTypeBox box;
    box.init(&first.getLineTypeList(), true, false);
    const int rows = box.count();
    const int selected = box.currentIndex();

    const std::optional<LC_LineType> tableEntry = box.definitionOf(namedPen(QStringLiteral("VENDOR_TAB")));
    REQUIRE(tableEntry.has_value());
    CHECK(tableEntry->name == QStringLiteral("VENDOR_TAB"));
    CHECK(tableEntry->description == QStringLiteral("Tabulated"));
    CHECK(tableEntry->pattern == std::vector<double>({10.0, -5.0}));

    // The folded key finds the record through another spelling too.
    const std::optional<LC_LineType> otherSpelling =
        box.definitionOf(namedPen(QStringLiteral("vendor_tab")));
    REQUIRE(otherSpelling.has_value());
    CHECK(otherSpelling->name == QStringLiteral("VENDOR_TAB"));
    CHECK(otherSpelling->pattern == std::vector<double>({10.0, -5.0}));

    const std::optional<LC_LineType> padded =
        box.definitionOf(namedPen(QStringLiteral("VENDOR_PAD")));
    REQUIRE(padded.has_value());
    CHECK(padded->name == QStringLiteral("VENDOR_PAD "));
    CHECK(padded->pattern == std::vector<double>({8.0, -2.0}));

    // A plain built-in has no record to give.
    CHECK_FALSE(box.definitionOf(namedPen(QStringLiteral("DASHED"))).has_value());
    // A record that only names a line type defines nothing.
    CHECK_FALSE(box.definitionOf(namedPen(QStringLiteral("VENDOR_SOLID"))).has_value());
    // A pen without a name has none either.
    CHECK_FALSE(box.definitionOf(RS_Pen(RS_Color(Qt::black), RS2::Width00, RS2::DashLine))
                    .has_value());
    // A name the drawing lacks adds no row and moves no selection.
    CHECK_FALSE(box.definitionOf(namedPen(QStringLiteral("VENDOR_GONE"))).has_value());
    CHECK(box.count() == rows);
    CHECK(box.currentIndex() == selected);

    RS_Graphic redefined;
    redefined.initForNewDocument();
    addLineType(redefined, "DASHED", {64.0, -32.0}, QStringLiteral("Redefined"));
    QG_LineTypeBox dashBox;
    dashBox.init(&redefined.getLineTypeList(), true, false);
    const std::optional<LC_LineType> dashEntry =
        dashBox.definitionOf(namedPen(QStringLiteral("DASHED")));
    REQUIRE(dashEntry.has_value());
    CHECK(dashEntry->name == QStringLiteral("DASHED"));
    CHECK(dashEntry->description == QStringLiteral("Redefined"));
    CHECK(dashEntry->pattern == std::vector<double>({64.0, -32.0}));

    RS_Graphic second;
    second.initForNewDocument();
    addLineType(second, "DASHED", {40.0, -20.0});
    dashBox.rebuild(&second.getLineTypeList());
    const std::optional<LC_LineType> rebuiltEntry =
        dashBox.definitionOf(namedPen(QStringLiteral("DASHED")));
    REQUIRE(rebuiltEntry.has_value());
    CHECK(rebuiltEntry->pattern == std::vector<double>({40.0, -20.0}));

    QG_LineTypeBox noList;
    noList.init(true, false);
    CHECK_FALSE(noList.definitionOf(namedPen(QStringLiteral("VENDOR_TAB"))).has_value());
}

TEST_CASE("The By Layer row shows what the layer draws", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const int tabRow = box.findText(QStringLiteral("VENDOR_TAB"));
    REQUIRE(tabRow >= 0);
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_TAB")));
    REQUIRE(box.currentIndex() == tabRow);
    box.setLayerLineTypeOf(namedPen(QStringLiteral("VENDOR_TAB")));
    CHECK(box.currentIndex() == tabRow);
    CHECK(rowImage(box, 0) == rowImage(box, tabRow));

    // A plain built-in takes the picture of its own row; by its text, because
    // findData() would answer with a drawing row of the same enum first.
    const int continuousRow = box.findText(QG_LineTypeBox::tr("Continuous"));
    REQUIRE(continuousRow > 0);
    box.setLayerLineTypeOf(RS_Pen(RS_Color(Qt::black), RS2::Width00, RS2::SolidLine));
    CHECK(rowImage(box, 0) == rowImage(box, continuousRow));
    CHECK(rowImage(box, 0) != rowImage(box, tabRow));

    // A name the drawing does not define shows the "no definition" picture a
    // pen of that name gets, and the selection stays where it was.
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_GONE")));
    const int temporary = box.currentIndex();
    REQUIRE(temporary >= 0);
    const QImage missing = rowImage(box, temporary);
    box.setLayerLineTypeOf(namedPen(QStringLiteral("VENDOR_GONE")));
    CHECK(box.currentIndex() == temporary);
    CHECK(rowImage(box, 0) == missing);
}

TEST_CASE("Dashes and dots never run together", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_DASHDOT", {12.0, -4.0, 0.0, -4.0});
    addLineType(graphic, "VENDOR_PAIR", {64.0, -32.0});
    addLineType(graphic, "VENDOR_DOT", {0.0, -5.0});
    addLineType(graphic, "VENDOR_MANY", {9.0, -3.0, 1.0, -3.0, 0.0, -3.0, 2.0, -3.0, 5.0, -3.0, 0.0, -3.0});
    addLineType(graphic, "VENDOR_CUT", {12.7, -6.35, 0.0, -6.35});
    addLineType(graphic, "VENDOR_TINY", {2.0e-320, -1.0e-320, 0.0, -1.0e-320});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const auto runsOf = [&box](const QString& name) {
        const int row = box.findText(name);
        REQUIRE(row >= 0);
        const QImage image = rowImage(box, row);
        skipUnlessWholePixelRatio(image);
        return litRuns(image);
    };
    const auto checkStretches = [](const std::vector<LitRun>& runs) {
        for (std::size_t i = 1; i < runs.size(); ++i) {
            const int stretch = runs[i].start - (runs[i - 1].start + runs[i - 1].length);
            CHECK(stretch >= 3);
        }
    };

    const std::vector<LitRun> dashDot = runsOf(QStringLiteral("VENDOR_DASHDOT"));
    REQUIRE(dashDot.size() >= 2);
    for (const LitRun& run : dashDot) {
        CHECK(run.length >= 3);
    }
    checkStretches(dashDot);
    // Long and short runs alternate, and the gaps around a dot are the same.
    for (std::size_t i = 0; i < dashDot.size(); ++i) {
        if (i + 1 < dashDot.size()) {
            CHECK((dashDot[i].length > 4) != (dashDot[i + 1].length > 4));
        }
        if (i > 0 && i + 1 < dashDot.size() && dashDot[i].length == 3) {
            const int before = dashDot[i].start - (dashDot[i - 1].start + dashDot[i - 1].length);
            const int after = dashDot[i + 1].start - (dashDot[i].start + dashDot[i].length);
            CHECK(before == after);
        }
    }

    const std::vector<LitRun> pair = runsOf(QStringLiteral("VENDOR_PAIR"));
    REQUIRE(pair.size() == 3);
    CHECK(pair[0].length == pair[1].length);
    CHECK(pair[1].length == pair[2].length);
    for (std::size_t i = 1; i < pair.size(); ++i) {
        CHECK(pair[i].start - (pair[i - 1].start + pair[i - 1].length) ==
              pair[1].start - (pair[0].start + pair[0].length));
    }

    const std::vector<LitRun> dots = runsOf(QStringLiteral("VENDOR_DOT"));
    REQUIRE(dots.size() >= 3);
    for (const LitRun& run : dots) {
        CHECK(run.length == 3);
    }

    // The right edge may cut a picture, but it never leaves a sliver of a run.
    const std::vector<LitRun> cut = runsOf(QStringLiteral("VENDOR_CUT"));
    REQUIRE(!cut.empty());
    for (const LitRun& run : cut) {
        CHECK(run.length >= 3);
    }

    // Lengths at the edge of what a double holds lay out like any others.
    const std::vector<LitRun> tiny = runsOf(QStringLiteral("VENDOR_TINY"));
    REQUIRE(tiny.size() >= 2);
    CHECK(tiny[0].length > 3);
    CHECK(tiny[1].length == 3);
    checkStretches(tiny);

    const std::vector<LitRun> many = runsOf(QStringLiteral("VENDOR_MANY"));
    REQUIRE(!many.empty());
    checkStretches(many);
    const int width = logicalWidth(rowImage(box, box.findText(QStringLiteral("VENDOR_MANY"))));
    for (const LitRun& run : many) {
        CHECK(run.start >= 0);
        CHECK(run.start + run.length <= width);
    }
}

TEST_CASE("A preview takes the colors of the stock pictures",
          "[gui][linetype][combo]") {
    (void)lc::test::application();
    IconColorGuard guard;
    guard.clear();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    const QColor chosen(QStringLiteral("#ff8000"));
    REQUIRE(chosen.isValid());
    guard.setAny(chosen);

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const int tabRow = box.findText(QStringLiteral("VENDOR_TAB"));
    REQUIRE(tabRow >= 0);
    for (const QIcon::Mode mode : {QIcon::Normal, QIcon::Active, QIcon::Selected}) {
        const QImage image = rowImage(box, tabRow, mode);
        const QColor actual = firstLitColor(image);
        CHECK(actual.alpha() > 32);
        CHECK(actual.rgb() == chosen.rgb());
    }

    // A mode with a property of its own takes that one.
    const QColor selectedColor(QStringLiteral("#0080ff"));
    REQUIRE(selectedColor.isValid());
    guard.setMode(QIcon::Selected, selectedColor);
    QG_LineTypeBox marked;
    marked.init(&graphic.getLineTypeList(), true, false);
    const int markedRow = marked.findText(QStringLiteral("VENDOR_TAB"));
    REQUIRE(markedRow >= 0);
    CHECK(firstLitColor(rowImage(marked, markedRow, QIcon::Selected)).rgb() ==
          selectedColor.rgb());
    CHECK(firstLitColor(rowImage(marked, markedRow, QIcon::Normal)).rgb() == chosen.rgb());

    // Nothing set: the palette's text color stands in for the template's.
    guard.clear();
    QG_LineTypeBox plain;
    plain.init(&graphic.getLineTypeList(), true, false);
    const int plainRow = plain.findText(QStringLiteral("VENDOR_TAB"));
    REQUIRE(plainRow >= 0);
    CHECK(firstLitColor(rowImage(plain, plainRow)).rgb() ==
          plain.palette().color(QPalette::Text).rgb());
}

TEST_CASE("The combo without a list keeps its pictures", "[gui][linetype][combo]") {
    (void)lc::test::application();
    QG_LineTypeBox box;
    box.init(true, false);
    QComboBox reference;
    CHECK(box.iconSize() == reference.iconSize());

    const int dashRow = box.findData(static_cast<int>(RS2::DashLine));
    REQUIRE(dashRow >= 0);
    CHECK(rowImage(box, dashRow) ==
          QIcon(QStringLiteral(":linetypes/linetype03.lci"))
              .pixmap(box.iconSize()).toImage().convertToFormat(QImage::Format_ARGB32));
    const int continuousRow = box.findData(static_cast<int>(RS2::SolidLine));
    REQUIRE(continuousRow >= 0);
    CHECK(rowImage(box, continuousRow) ==
          QIcon(QStringLiteral(":linetypes/linetype01.lci"))
              .pixmap(box.iconSize()).toImage().convertToFormat(QImage::Format_ARGB32));

    box.setLineType(RS2::DashLine);
    CHECK_FALSE(box.definitionOf(namedPen(QStringLiteral("VENDOR_TAB"))).has_value());

    // A list hands out the 40x14 size, and giving the list up hands the size
    // back to the style, as a combo that never had one follows it.
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});
    box.rebuild(&graphic.getLineTypeList());
    CHECK(box.iconSize() == QSize(40, 14));
    box.rebuild(nullptr);
    CHECK(box.iconSize() == reference.iconSize());
}

TEST_CASE("The drawing's rows come before the built-in rows", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});
    addLineType(graphic, "VENDOR_DOT", {0.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    // The fixed special rows keep their indices.
    CHECK(box.itemText(0) == QG_LineTypeBox::tr("By Layer"));
    CHECK(box.itemText(1) == QG_LineTypeBox::tr("By Block"));

    const int dotRow = box.findText(QStringLiteral("VENDOR_DOT"));
    const int tabRow = box.findText(QStringLiteral("VENDOR_TAB"));
    REQUIRE(dotRow >= 0);
    REQUIRE(tabRow >= 0);
    // The drawing's rows follow the special rows, sorted by key, then one
    // separator and the built-in rows.
    CHECK(dotRow == 2);
    CHECK(tabRow == 3);
    CHECK(box.itemText(tabRow + 1).isEmpty());
    CHECK(box.itemText(tabRow + 2) == QG_LineTypeBox::tr("Continuous"));

    QG_LineTypeBox noList;
    noList.init(true, false);
    CHECK(box.count() == noList.count() + 2 + 1);

    // A drawing that adds none and has no temporary row has no separator.
    RS_Graphic seed;
    seed.initForNewDocument();
    QG_LineTypeBox seeded;
    seeded.init(&seed.getLineTypeList(), true, false);
    CHECK(seeded.count() == noList.count());
    for (int i = 0; i < seeded.count(); ++i) {
        CHECK(!seeded.itemText(i).isEmpty());
    }
}

TEST_CASE("An enum never selects a drawing row", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const int drawingRow = box.findText(QStringLiteral("VENDOR_TAB"));
    REQUIRE(drawingRow >= 0);
    REQUIRE(box.itemData(drawingRow).toInt() == static_cast<int>(RS2::SolidLine));

    box.setLineType(RS2::SolidLine);
    CHECK(box.currentIndex() != drawingRow);
    CHECK(box.currentText() == QG_LineTypeBox::tr("Continuous"));

    // An unnamed pen follows the same rule.
    box.setLineTypeOf(RS_Pen(RS_Color(Qt::black), RS2::Width00, RS2::SolidLine));
    CHECK(box.currentIndex() != drawingRow);
    CHECK(box.currentText() == QG_LineTypeBox::tr("Continuous"));

    // A rebuild keeps an enum-only selection on the built-in row.
    box.setLineType(RS2::SolidLine);
    box.rebuild(&graphic.getLineTypeList());
    CHECK(box.currentIndex() != drawingRow);
    CHECK(box.currentText() == QG_LineTypeBox::tr("Continuous"));
}

TEST_CASE("A drawing row says what it is", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});
    addLineType(graphic, "VENDOR_HIDDEN", {64.0, -32.0}, QStringLiteral("Hidden line"));
    addLineType(graphic, "VENDOR_SOLID", {}, QStringLiteral("A solid vendor line"));
    addLineType(graphic, "VENDOR_EMPTY", {});
    addLineType(graphic, "VENDOR_LONG", {1250000.0, -250000.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const auto tooltip = [&box](const QString& name) {
        const int row = box.findText(name);
        REQUIRE(row >= 0);
        return box.itemData(row, Qt::ToolTipRole).toString();
    };
    CHECK(tooltip(QStringLiteral("VENDOR_HIDDEN")) == QStringLiteral("Hidden line — 64, -32"));
    CHECK(tooltip(QStringLiteral("VENDOR_TAB")) == QStringLiteral("10, -5"));
    CHECK(tooltip(QStringLiteral("VENDOR_LONG")) == QStringLiteral("1250000, -250000"));
    // A record without dashes says why the list is empty.
    CHECK(tooltip(QStringLiteral("VENDOR_SOLID")) ==
          QStringLiteral("A solid vendor line — no dashes"));
    CHECK(tooltip(QStringLiteral("VENDOR_EMPTY")) == QStringLiteral("no dashes"));

    RS_Graphic redefined;
    redefined.initForNewDocument();
    addLineType(redefined, "DASHED", {64.0, -32.0});
    QG_LineTypeBox dashBox;
    dashBox.init(&redefined.getLineTypeList(), true, false);
    const int dashRow = dashBox.findText(QG_LineTypeBox::tr("Dash"));
    REQUIRE(dashRow >= 0);
    CHECK(dashBox.itemData(dashRow, Qt::ToolTipRole).toString() ==
          QG_LineTypeBox::tr("As this drawing defines it: %1").arg(QStringLiteral("64, -32")));

    dashBox.setLineTypeOf(namedPen(QStringLiteral("VENDOR_GONE")));
    const int temporary = dashBox.currentIndex();
    REQUIRE(temporary >= 0);
    CHECK(dashBox.itemData(temporary, Qt::ToolTipRole).toString() ==
          QG_LineTypeBox::tr("This drawing has no definition for %1").arg(QStringLiteral("VENDOR_GONE")));
}

TEST_CASE("A preview weighs what a stock picture weighs", "[gui][linetype][combo]") {
    (void)lc::test::application();
    IconColorGuard guard;
    guard.clear();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const int row = box.findText(QStringLiteral("VENDOR_TAB"));
    REQUIRE(row >= 0);
    const QImage image = rowImage(box, row);
    REQUIRE(!image.isNull());
    skipUnlessWholePixelRatio(image);
    const qreal ratio = pixelRatio(image);

    // The stock pictures' 32x12 area, centred in the icon.
    const int areaX = (logicalWidth(image) - 32) / 2;
    const int column = qRound((areaX + 3) * ratio);
    int lit = 0;
    QColor color;
    for (int y = 0; y < image.height(); ++y) {
        const QColor pixel = image.pixelColor(column, y);
        if (pixel.alpha() > 32) {
            ++lit;
            color = pixel;
        }
    }
    CHECK(lit == qRound(3 * ratio));
    const QColor text = box.palette().color(QPalette::Text);
    CHECK(color.alpha() == 255);
    CHECK(color.red() == text.red());
    CHECK(color.green() == text.green());
    CHECK(color.blue() == text.blue());

    int firstLit = image.width();
    int lastLit = -1;
    for (int x = 0; x < image.width(); ++x) {
        for (int y = 0; y < image.height(); ++y) {
            if (image.pixelColor(x, y).alpha() > 32) {
                firstLit = std::min(firstLit, x);
                lastLit = std::max(lastLit, x);
                break;
            }
        }
    }
    CHECK(qRound(firstLit / ratio) == areaX);
    CHECK(qRound(lastLit / ratio) <= areaX + 31);
}

TEST_CASE("A built-in the drawing redefines is one of the drawing's rows too",
          "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic seed;
    seed.initForNewDocument();
    RS_Graphic redefined;
    redefined.initForNewDocument();
    addLineType(redefined, "DASHED", {64.0, -32.0}, QStringLiteral("Hidden line"));

    QG_LineTypeBox seedBox;
    seedBox.init(&seed.getLineTypeList(), true, false);
    CHECK(seedBox.findText(QStringLiteral("DASHED")) < 0);

    QG_LineTypeBox box;
    box.init(&redefined.getLineTypeList(), true, false);
    const int ownRow = box.findText(QStringLiteral("DASHED"));
    REQUIRE(ownRow >= 0);
    // The drawing's group follows the special rows and precedes the separator.
    CHECK(ownRow == 2);
    CHECK(box.itemText(ownRow + 1).isEmpty());
    CHECK(box.itemData(ownRow).toInt() == static_cast<int>(RS2::DashLine));
    CHECK(box.itemData(ownRow, Qt::ToolTipRole).toString() == QStringLiteral("Hidden line — 64, -32"));

    // The row writes the record's name.
    box.setCurrentIndex(ownRow);
    RS_Pen written;
    box.applyLineTypeTo(written);
    CHECK(written.getLineTypeName() == QStringLiteral("DASHED"));

    // A pen that names the built-in selects the drawing-group row.
    box.setLineTypeOf(namedPen(QStringLiteral("DASHED")));
    CHECK(box.currentIndex() == ownRow);

    // An enum-only pen still finds the stock row.
    box.setLineTypeOf(RS_Pen(RS_Color(Qt::black), RS2::Width00, RS2::DashLine));
    CHECK(box.currentIndex() != ownRow);
    CHECK(box.itemText(box.currentIndex()) == QG_LineTypeBox::tr("Dash"));
}

TEST_CASE("Setting a pen moves the selection once", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    box.setLineType(RS2::DashLine);

    QList<int> seen;
    QObject::connect(&box, &QComboBox::currentIndexChanged,
                     [&seen](const int index) { seen.append(index); });
    int changed = 0;
    QObject::connect(&box, &QG_LineTypeBox::lineTypeChanged,
                     [&changed](RS2::LineType) { ++changed; });

    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_GONE")));
    // The temporary row and its separator come and go quietly; only the final
    // selection may be reported.
    CHECK(seen.size() <= 1);
    for (const int index : seen) {
        CHECK(index == box.currentIndex());
    }
    CHECK(changed == 1);
}

TEST_CASE("The pictures follow a color change", "[gui][linetype][combo]") {
    (void)lc::test::application();
    IconColorGuard guard;
    guard.clear();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});
    const QColor before(QStringLiteral("#ff8000"));
    const QColor after(QStringLiteral("#0080ff"));
    REQUIRE(before.isValid());
    REQUIRE(after.isValid());
    guard.setAny(before);

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    const int tabRow = box.findText(QStringLiteral("VENDOR_TAB"));
    REQUIRE(tabRow >= 0);
    box.setLayerLineTypeOf(namedPen(QStringLiteral("VENDOR_TAB")));
    REQUIRE(firstLitColor(rowImage(box, tabRow)).rgb() == before.rgb());

    guard.setAny(after);
    QEvent styleEvent(QEvent::StyleChange);
    QCoreApplication::sendEvent(&box, &styleEvent);
    CHECK(firstLitColor(rowImage(box, tabRow)).rgb() == after.rgb());
    // The By Layer row holds a copy, so it needs the new color too.
    CHECK(firstLitColor(rowImage(box, 0)).rgb() == after.rgb());

    // A combo without that row has no copy to repaint: its first row stays
    // what it was.
    QG_LineTypeBox plain;
    plain.init(&graphic.getLineTypeList(), false, false);
    const QString firstText = plain.itemText(0);
    plain.setLayerLineTypeOf(namedPen(QStringLiteral("VENDOR_TAB")));
    QEvent again(QEvent::StyleChange);
    QCoreApplication::sendEvent(&plain, &again);
    CHECK(plain.itemText(0) == firstText);
}

TEST_CASE("A rebuild keeps the kind of row", "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic redefined;
    redefined.initForNewDocument();
    addLineType(redefined, "DASHED", {64.0, -32.0});

    QG_LineTypeBox box;
    box.init(&redefined.getLineTypeList(), true, false);
    // The built-in row, chosen by its enum as the other cases do.
    box.setLineType(RS2::DashLine);
    const QString builtInText = box.itemText(box.currentIndex());
    CHECK(builtInText == QG_LineTypeBox::tr("Dash"));
    box.rebuild(&redefined.getLineTypeList());
    // The selection stays on the stock row, not the drawing's row of the same
    // name.
    CHECK(box.itemText(box.currentIndex()) == builtInText);
    RS_Pen written;
    box.applyLineTypeTo(written);
    CHECK(written.getLineTypeName() == QStringLiteral("DASHED"));
    CHECK(written.getLineType() == RS2::DashLine);

    // A drawing row is restored by its name.
    const int ownRow = box.findText(QStringLiteral("DASHED"));
    REQUIRE(ownRow >= 0);
    box.setCurrentIndex(ownRow);
    box.rebuild(&redefined.getLineTypeList());
    CHECK(box.currentText() == QStringLiteral("DASHED"));

    // A list that does not redefine the built-in leaves its stock row.
    RS_Graphic seed;
    seed.initForNewDocument();
    box.rebuild(&seed.getLineTypeList());
    CHECK(box.currentText() == QG_LineTypeBox::tr("Dash"));
}

TEST_CASE("List mode keeps the special rows where they are",
          "[gui][linetype][combo]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0});

    QG_LineTypeBox noList;
    noList.init(true, true, true);
    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, true, true);

    // The four special rows keep their order and indices; the drawing's rows
    // follow them.
    REQUIRE(noList.count() >= 4);
    for (int i = 0; i < 4; ++i) {
        CHECK(box.itemText(i) == noList.itemText(i));
    }
    const int tabRow = box.findText(QStringLiteral("VENDOR_TAB"));
    REQUIRE(tabRow >= 0);
    CHECK(tabRow == 4);

    box.setLineType(RS2::LineByLayer);
    CHECK(box.currentIndex() == 1);
    box.setLineType(RS2::LineByBlock);
    CHECK(box.currentIndex() == 2);
    box.setLineType(RS2::NoPen);
    CHECK(box.currentIndex() == 3);
    box.setLineType(RS2::LineTypeUnchanged);
    CHECK(box.currentIndex() == 0);
    CHECK(box.isUnchanged());

    // The By Layer row is index 1 here, not row 0.
    box.setLayerLineTypeOf(namedPen(QStringLiteral("VENDOR_TAB")));
    CHECK(rowImage(box, 1) == rowImage(box, tabRow));
    CHECK(rowImage(box, 0) != rowImage(box, 1));
}

TEST_CASE("Local preview pictures", "[.][fixture][linetype-preview-shots]") {
    const QString outDir = qEnvironmentVariable("LC_PREVIEW_OUT");
    if (outDir.isEmpty() || !QFileInfo(outDir).isDir()) {
        SKIP("LC_PREVIEW_OUT does not name an existing directory");
    }
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addLineType(graphic, "VENDOR_TAB", {10.0, -5.0}, QStringLiteral("Tabulated"));
    addLineType(graphic, "VENDOR_DOT", {0.0, -5.0});
    addLineType(graphic, "VENDOR_DASHDOT", {12.0, -4.0, 0.0, -4.0}, QStringLiteral("Dash dot sample"));
    addLineType(graphic, "VENDOR_LONG", {30.0, -6.0, 6.0, -6.0, 6.0, -6.0});
    addLineType(graphic, "DASHED", {64.0, -32.0});

    QG_LineTypeBox box;
    box.init(&graphic.getLineTypeList(), true, false);
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_DASHDOT")));
    // The temporary row stays behind the drawing rows; the selection stays on a
    // drawing row for the closed picture.
    box.setLineTypeOf(namedPen(QStringLiteral("VENDOR_GONE")));
    const int dashDotRow = box.findText(QStringLiteral("VENDOR_DASHDOT"));
    REQUIRE(dashDotRow >= 0);
    box.setCurrentIndex(dashDotRow);

    box.show();
    QCoreApplication::processEvents();
    const auto save = [&outDir](const QString& name, const QPixmap& pixmap) {
        const QString path = QDir(outDir).filePath(name);
        CHECK(pixmap.save(path));
        CHECK(QFileInfo(path).size() > 0);
    };
    save(QStringLiteral("combo_closed.png"), box.grab());

    box.showPopup();
    QCoreApplication::processEvents();
    QAbstractItemView* view = box.view();
    if (view != nullptr) {
        save(QStringLiteral("combo_list_selected.png"), view->window()->grab());
        view->scrollToTop();
        QCoreApplication::processEvents();
        save(QStringLiteral("combo_list_top.png"), view->window()->grab());
        view->scrollToBottom();
        QCoreApplication::processEvents();
        save(QStringLiteral("combo_list_bottom.png"), view->window()->grab());
    }
    else {
        save(QStringLiteral("combo_list_selected.png"), box.grab());
    }
    box.hidePopup();
    QCoreApplication::processEvents();
}
