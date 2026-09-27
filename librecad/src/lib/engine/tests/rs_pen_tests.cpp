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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

// Tests for RS_Pen's comparisons and for the linetype name a pen carries. The
// renderers that call isSameAs() are exercised in
// librecad/src/lib/gui/tests/lc_graphicviewrenderer_tests.cpp.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>

#include "lc_linetypenames.h"
#include "rs_color.h"
#include "rs_pen.h"

namespace Catch {
// Without this a failing name comparison prints { {?}, {?}, ... } for a QString.
template <>
struct StringMaker<QString> {
    static std::string convert(const QString &value) {
        return '"' + value.toStdString() + '"';
    }
};
} // namespace Catch

namespace {

RS_Pen testPen(const RS2::LineType lineType, const double screenWidth = 1.0) {
    RS_Pen pen{RS_Color(Qt::black), RS2::Width00, lineType};
    pen.setScreenWidth(screenWidth);
    return pen;
}

RS_Pen namedPen(const QString &name) {
    RS_Pen pen;
    pen.setLineTypeName(name);
    return pen;
}

// "Olfarbe" with a precomposed U+00D6 / U+00F6, and with a combining diaeresis.
QString upperOe() { return QString::fromUtf8("\xC3\x96" "lfarbe"); }
QString lowerOe() { return QString::fromUtf8("\xC3\xB6" "lfarbe"); }
QString decomposedUpperOe() { return QString(QChar(u'O')) + QChar(0x0308) + "lfarbe"; }
QString decomposedLowerOe() { return QString(QChar(u'o')) + QChar(0x0308) + "lfarbe"; }
// U+212A KELVIN SIGN, which NFC maps to an ASCII 'K'.
QString kelvinSign() { return QString(QChar(0x212A)) + "elvin"; }
QString strasse() { return QString::fromUtf8("Stra\xC3\x9F" "e"); }

// An RS_Pen without the two identity members, in the same declaration order.
struct PenWithoutIdentity : RS_Flags {
    RS2::LineType lineType = RS2::SolidLine;
    RS2::LineWidth width = RS2::Width00;
    double screenWidth = 0.;
    RS_Color color;
    float alpha = 1.;
    double dashOffset = 0.;
};

} // namespace

TEST_CASE("the painter dash offset tells two pens apart only when a pattern is drawn",
          "[pen][linetype]") {
    // The painter's running dash offset, which RS_Painter::updateDashOffset() moves
    // by the length of every entity drawn.
    const double runningOffset = -37.5;

    const RS_Pen solid = testPen(RS2::SolidLine);
    CHECK(solid.isSameAs(solid, runningOffset, false));
    CHECK_FALSE(solid.isSameAs(solid, runningOffset, true));

    const RS_Pen dashed = testPen(RS2::DashLine);
    CHECK_FALSE(dashed.isSameAs(dashed, runningOffset, true));
    CHECK(dashed.isSameAs(dashed, 0.0, true));
    RS_Pen phased = dashed;
    phased.setDashOffset(runningOffset);
    CHECK(phased.isSameAs(dashed, runningOffset, true));
    CHECK_FALSE(phased.isSameAs(dashed, runningOffset + 0.1, true));

    // Only the offset term is optional.
    RS_Pen coloured = solid;
    coloured.setColor(RS_Color(Qt::red));
    CHECK_FALSE(solid.isSameAs(coloured, runningOffset, false));

    RS_Pen wider = solid;
    wider.setWidth(RS2::Width01);
    CHECK_FALSE(solid.isSameAs(wider, runningOffset, false));

    RS_Pen translucent = solid;
    translucent.setAlpha(0.5f);
    CHECK_FALSE(solid.isSameAs(translucent, runningOffset, false));

    RS_Pen invalid = solid;
    invalid.setFlag(RS2::FlagInvalid);
    CHECK_FALSE(invalid.isSameAs(solid, runningOffset, false));
}

TEST_CASE("a pen set by enum carries no identity", "[pen][linetype]") {
    // The same invariant as the round-trip case in
    // librecad/src/lib/filters/tests/dxf_roundtrip_tests.cpp, one level up: an
    // enum reports the canonical name and carries no identity of its own.
    for (const RS2::LineType type : {
             RS2::SolidLine,
             RS2::DotLine, RS2::DotLineTiny, RS2::DotLine2, RS2::DotLineX2,
             RS2::DashLine, RS2::DashLineTiny, RS2::DashLine2, RS2::DashLineX2,
             RS2::HiddenLine, RS2::HiddenLineTiny, RS2::HiddenLine2,
             RS2::HiddenLineX2,
             RS2::PhantomLine, RS2::PhantomLineTiny, RS2::PhantomLine2,
             RS2::PhantomLineX2,
             RS2::DashDotLine, RS2::DashDotLineTiny, RS2::DashDotLine2,
             RS2::DashDotLineX2,
             RS2::DivideLine, RS2::DivideLineTiny, RS2::DivideLine2,
             RS2::DivideLineX2,
             RS2::CenterLine, RS2::CenterLineTiny, RS2::CenterLine2,
             RS2::CenterLineX2,
             RS2::BorderLine, RS2::BorderLineTiny, RS2::BorderLine2,
             RS2::BorderLineX2,
             RS2::LineByLayer, RS2::LineByBlock}) {
        INFO("linetype " << static_cast<int>(type));
        RS_Pen pen;
        pen.setLineType(type);
        CHECK(pen.getLineTypeName() == LC_LineTypeNames::lineTypeToName(type));
        // Id 0 means "the canonical name of m_lineType". That is what keeps
        // every setLineType( call site in librecad/src honest without visiting
        // them, and what lets setLineType() stay inline and lookup-free.
        CHECK(pen.getLineTypeId() == 0);
        CHECK_FALSE(pen.hasLineTypeName());
    }

    // The three values that are not drawable names report CONTINUOUS, exactly
    // as lineTypeToName() does today, so #1734 is unchanged.
    for (const RS2::LineType type :
         {RS2::NoPen, RS2::LineTypeUnchanged, RS2::LineSelected}) {
        INFO("non-drawable linetype " << static_cast<int>(type));
        RS_Pen pen;
        pen.setLineType(type);
        CHECK(pen.getLineTypeName() == QStringLiteral("CONTINUOUS"));
        CHECK(pen.getLineType() == type);
        CHECK(pen.getLineTypeId() == 0);
        CHECK_FALSE(pen.hasLineTypeName());
    }
}

TEST_CASE("two spellings of one name are two identities and one pen",
          "[pen][linetype]") {
    const RS_Pen mixedPen = namedPen(QStringLiteral("Vendor_mixedCase"));
    const RS_Pen upperPen = namedPen(QStringLiteral("VENDOR_MIXEDCASE"));

    CHECK(mixedPen.getLineTypeId() != 0);
    CHECK(upperPen.getLineTypeId() != 0);
    CHECK(mixedPen.getLineTypeId() != upperPen.getLineTypeId());
    CHECK(mixedPen.getLineTypeName() == QStringLiteral("Vendor_mixedCase"));
    CHECK(upperPen.getLineTypeName() == QStringLiteral("VENDOR_MIXEDCASE"));
    CHECK(mixedPen == upperPen);
    CHECK(mixedPen.isSameAs(upperPen, 0.0, false));
    // Equality answers "does this paint the same", not "is it spelled the same",
    // so two equal pens may report different names.
    CHECK(mixedPen.getLineTypeName() != upperPen.getLineTypeName());

    // A non-ASCII spelling interns like any other and still compares equal to
    // its ASCII-case variant. Which fold rule applies is the fold case's business.
    const RS_Pen oePen = namedPen(upperOe());
    const RS_Pen oeUpperPen = namedPen(QString::fromUtf8("\xC3\x96" "LFARBE"));
    CHECK(oePen.getLineTypeId() != 0);
    CHECK(oeUpperPen.getLineTypeId() != 0);
    CHECK(oePen.getLineTypeId() != oeUpperPen.getLineTypeId());
    CHECK(oePen == oeUpperPen);
}

TEST_CASE("a spelling that means its enum IS its enum, and one that does not is not",
          "[pen][linetype]") {
    // This case is what decides between storing one id and storing two: a pen
    // whose name means its own enum must stay interchangeable with that enum,
    // and a pen whose name means more than its enum must not.
    const RS_Pen hidden = namedPen(QStringLiteral("hidden"));
    RS_Pen hiddenByEnum;
    hiddenByEnum.setLineType(RS2::HiddenLine);
    CHECK(hidden.getLineType() == RS2::HiddenLine);
    CHECK(hidden.getLineTypeId() != 0);
    CHECK(hidden.hasLineTypeName());
    CHECK(hidden.getLineTypeName() == QStringLiteral("hidden"));
    CHECK(hidden == hiddenByEnum);
    CHECK(hidden.isSameAs(hiddenByEnum, 0.0, false));

    // An ISO alias is a PhantomLine that must still export as ACAD_ISO09W100.
    const RS_Pen alias = namedPen(QStringLiteral("ACAD_ISO09W100"));
    RS_Pen phantom;
    phantom.setLineType(RS2::PhantomLine);
    CHECK(phantom.getLineTypeId() == 0);
    CHECK(alias.getLineType() == RS2::PhantomLine);
    CHECK(alias.getLineTypeId() != 0);
    CHECK(alias.hasLineTypeName());
    CHECK(alias != phantom);
    CHECK_FALSE(alias.isSameAs(phantom, 0.0, false));

    // A vendor name nameToLineType() does not know falls through to SolidLine
    // and must not thereby become a plain solid pen.
    const RS_Pen vendor = namedPen(QStringLiteral("VENDOR_TAB"));
    RS_Pen solid;
    solid.setLineType(RS2::SolidLine);
    CHECK(vendor.getLineType() == RS2::SolidLine);
    CHECK(vendor.hasLineTypeName());
    CHECK(vendor != solid);
    CHECK_FALSE(vendor.isSameAs(solid, 0.0, false));

    const RS_Pen byLayer = namedPen(QStringLiteral("bylayer"));
    CHECK(byLayer.isLineTypeByLayer());
    CHECK(byLayer.hasLineTypeName());
    CHECK(byLayer == RS_Pen());

    for (const QString &blank : {QString(), QStringLiteral("   ")}) {
        INFO("blank name of length " << blank.size());
        const RS_Pen empty = namedPen(blank);
        CHECK(empty.getLineTypeId() == 0);
        CHECK_FALSE(empty.hasLineTypeName());
        CHECK(empty.getLineType() == RS2::LineByLayer);
        CHECK(empty.getLineTypeName() == QStringLiteral("ByLayer"));
    }

    RS_Pen invalid(RS2::FlagInvalid);
    invalid.delFlag(RS2::FlagInvalid);
    CHECK(invalid.getLineTypeId() == 0);
    CHECK_FALSE(invalid.hasLineTypeName());
    CHECK(invalid == RS_Pen(RS_Color(Qt::black), RS2::Width00, RS2::SolidLine));

    CHECK(hidden.getLineTypeFoldId() == 0);
    CHECK(alias.getLineTypeFoldId() != 0);
    CHECK(vendor.getLineTypeFoldId() != 0);
}

TEST_CASE("identity survives every pen-to-pen copy", "[pen][linetype]") {
    const RS_Pen src = namedPen(QStringLiteral("VENDOR_TAB"));

    // Copy-construction has no destination to pre-seed; it pins that both new
    // members ride the implicit memberwise copy.
    const RS_Pen constructed = src;
    CHECK(constructed.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
    CHECK(constructed.getLineTypeId() == src.getLineTypeId());
    CHECK(constructed.getLineType() == src.getLineType());
    CHECK(constructed == src);

    // The three assignment-shaped copies start from a DIFFERENT name, so a
    // half-copy shows up as "kept the wrong name" rather than as "left at 0".
    RS_Pen assigned = namedPen(QStringLiteral("STALE"));
    assigned = src;
    CHECK(assigned.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
    CHECK(assigned.getLineTypeId() == src.getLineTypeId());
    CHECK(assigned.getLineType() == src.getLineType());
    CHECK(assigned == src);

    RS_Pen updated = namedPen(QStringLiteral("STALE"));
    updated.updateBy(src);
    CHECK(updated.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
    CHECK(updated.getLineTypeId() == src.getLineTypeId());
    CHECK(updated.getLineType() == src.getLineType());
    CHECK(updated == src);

    RS_Pen fromPen = namedPen(QStringLiteral("STALE"));
    fromPen.setLineTypeFromPen(src);
    CHECK(fromPen.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
    CHECK(fromPen.getLineTypeId() == src.getLineTypeId());
    CHECK(fromPen.getLineType() == src.getLineType());
    CHECK(fromPen == src);

    // RS_Entity::getPenResolved()'s ByLayer branch in miniature: an entity pen
    // that is ByLayer takes the layer pen's line type, name included.
    RS_Pen layerPen = namedPen(QStringLiteral("VENDOR_TAB"));
    RS_Pen entPen;
    entPen.setLineType(RS2::LineByLayer);
    entPen.setLineTypeFromPen(layerPen);
    CHECK(entPen.getLineType() == RS2::SolidLine);
    CHECK(entPen.getLineTypeName() == QStringLiteral("VENDOR_TAB"));
    CHECK(entPen.hasLineTypeName());
}

TEST_CASE("setLineType(enum) clears the name", "[pen][linetype]") {
    // A user's explicit retype, as in the property sheet or the pen widget,
    // must survive the next save, and the renderer's forced selection pen must
    // not keep the name.
    RS_Pen pen = namedPen(QStringLiteral("VENDOR_TAB"));
    REQUIRE(pen.hasLineTypeName());
    pen.setLineType(RS2::DashLine);

    CHECK_FALSE(pen.hasLineTypeName());
    CHECK(pen.getLineTypeId() == 0);
    CHECK(pen.getLineType() == RS2::DashLine);
    CHECK(pen.getLineTypeName() == QStringLiteral("DASHED"));

    // The fold id is cleared too, which only equality can see.
    RS_Pen dashed;
    dashed.setLineType(RS2::DashLine);
    CHECK(pen == dashed);
}

TEST_CASE("the linetype identity costs the pen no space", "[pen][linetype]") {
    // The two ids fill padding the pen already had. A 32-bit ABI may have no
    // such hole, which the design accepts, so the comparison is 64-bit only.
    if (sizeof(void *) == 8) {
        CHECK(sizeof(RS_Pen) == sizeof(PenWithoutIdentity));
    }
}

TEST_CASE("two spellings are one name only when they differ in ASCII case or Unicode form",
          "[pen][linetype]") {
    CHECK(LC_LineTypeNames::foldName(QStringLiteral("hidden"))
          == QStringLiteral("HIDDEN"));
    CHECK(LC_LineTypeNames::foldName(QStringLiteral("Vendor_mixedCase"))
          == QStringLiteral("VENDOR_MIXEDCASE"));
    // The pen this pair makes is in "two spellings of one name are two
    // identities and one pen"; here only the fold itself is read.
    CHECK(LC_LineTypeNames::foldName(upperOe())
          == LC_LineTypeNames::foldName(QString::fromUtf8("\xC3\x96" "LFARBE")));

    // ASCII-only case: QString::toUpper() would fold these together.
    CHECK(LC_LineTypeNames::foldName(upperOe()) != LC_LineTypeNames::foldName(lowerOe()));
    CHECK(namedPen(upperOe()) != namedPen(lowerOe()));

    // NFC runs: a pure byte fold would keep these apart.
    CHECK(LC_LineTypeNames::foldName(decomposedUpperOe())
          == LC_LineTypeNames::foldName(upperOe()));
    CHECK(namedPen(decomposedUpperOe()) == namedPen(upperOe()));

    // No full case mapping: "Strasse" must not swallow "Straße".
    CHECK(LC_LineTypeNames::foldName(strasse())
          != LC_LineTypeNames::foldName(QStringLiteral("STRASSE")));
    CHECK(namedPen(strasse()) != namedPen(QStringLiteral("STRASSE")));

    // NFC BEFORE the case step: case-first would upper-case the bare "o" and
    // compose U+00D6 instead of U+00F6, and these two would part.
    CHECK(LC_LineTypeNames::foldName(decomposedLowerOe())
          == LC_LineTypeNames::foldName(lowerOe()));
    CHECK(namedPen(decomposedLowerOe()) == namedPen(lowerOe()));

    // A singleton NFC mapping into ASCII is folded at all: U+212A -> 'K'.
    CHECK(LC_LineTypeNames::foldName(kelvinSign())
          == LC_LineTypeNames::foldName(QStringLiteral("kelvin")));
    CHECK(namedPen(kelvinSign()) == namedPen(QStringLiteral("kelvin")));

    for (const QString &name : {QStringLiteral("hidden"),
                                QStringLiteral("Vendor_mixedCase"),
                                upperOe(), lowerOe(),
                                decomposedUpperOe(), decomposedLowerOe(),
                                strasse(), kelvinSign()}) {
        const QString folded = LC_LineTypeNames::foldName(name);
        CHECK(LC_LineTypeNames::foldName(folded) == folded);
    }
}

TEST_CASE("the same spelling always means the same thing", "[pen][linetype]") {
    // There is no public unit to call, so the store is read back through pens.
    const RS_Pen first = namedPen(QStringLiteral("VENDOR_TAB"));
    const RS_Pen second = namedPen(QStringLiteral("VENDOR_TAB"));
    CHECK(first.getLineTypeId() == second.getLineTypeId());
    CHECK(first == second);

    RS_Pen repeated;
    repeated.setLineTypeName(QStringLiteral("VENDOR_TAB"));
    const std::uint16_t once = repeated.getLineTypeId();
    repeated.setLineTypeName(QStringLiteral("VENDOR_TAB"));
    CHECK(repeated.getLineTypeId() == once);

    // A name that folds to its enum's canonical name gets fold 0, which only
    // equality with the enum-only pen can see. lineTypeToName() returns the
    // mixed-case "ByLayer", so an implementation folding one side fails here.
    for (const QString &name : {QStringLiteral("bylayer"), QStringLiteral("BYLAYER"),
                                QStringLiteral("ByLayer")}) {
        INFO("by-layer spelling " << name.toStdString());
        RS_Pen byEnum;
        byEnum.setLineType(RS2::LineByLayer);
        CHECK(namedPen(name) == byEnum);
    }
    for (const QString &name : {QStringLiteral("byblock"), QStringLiteral("BYBLOCK"),
                                QStringLiteral("ByBlock")}) {
        INFO("by-block spelling " << name.toStdString());
        RS_Pen byEnum;
        byEnum.setLineType(RS2::LineByBlock);
        CHECK(namedPen(name) == byEnum);
    }
}

TEST_CASE("the spelling never changes an answer", "[pen][linetype]") {
    const RS_Pen upperPen = namedPen(QStringLiteral("VENDOR_MIXEDCASE"));

    // Same enum, different meanings: both fall through to SolidLine.
    const RS_Pen vendor = namedPen(QStringLiteral("VENDOR_TAB"));
    const RS_Pen other = namedPen(QStringLiteral("VENDOR_RIP"));
    REQUIRE(vendor.getLineType() == other.getLineType());
    CHECK(vendor != other);
    CHECK_FALSE(vendor.isSameAs(other, 0.0, false));

    // Neither comparison reads the spelling id: moving a pen to a third
    // spelling of the same name changes its id and no answer.
    RS_Pen moving = namedPen(QStringLiteral("Vendor_mixedCase"));
    const std::uint16_t before = moving.getLineTypeId();
    moving.setLineTypeName(QStringLiteral("vendor_mixedcase"));
    CHECK(moving.getLineTypeId() != before);
    CHECK(moving == upperPen);
    CHECK(moving.isSameAs(upperPen, 0.0, false));
}
