/*
 * This file is part of LibreCAD, distributed under the GNU General Public
 * License version 2 or (at your option) any later version.
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QLocale>
#include <cmath>

#include "lc_actiontestsupport.h"
#include "lc_dimarc.h"
#include "lc_dimordinate.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_graphic.h"
#include "rs_insert.h"

TEST_CASE("Dimension values are geometric and independent of labels", "[dimension][clipboard]") {
    RS_DimensionData data;
    data.definitionPoint = RS_Vector(0.0, 0.0);
    data.text = " ";

    RS_DimAligned aligned(nullptr, data, {RS_Vector(0, 0), RS_Vector(3, 4)});
    REQUIRE(aligned.getMeasuredValue());
    CHECK(*aligned.getMeasuredValue() == Catch::Approx(5.0));
    CHECK(aligned.getMeasuredValueText() == "5");

    RS_DimLinear linear(nullptr, data, {RS_Vector(0, 0), RS_Vector(3, 4), 0.0, 0.0});
    REQUIRE(linear.getMeasuredValue());
    CHECK(*linear.getMeasuredValue() == Catch::Approx(3.0));
    linear.setAngle(M_PI_2);
    CHECK(*linear.getMeasuredValue() == Catch::Approx(4.0));

    RS_DimRadial radial(nullptr, data, {RS_Vector(3, 4), 0});
    RS_DimDiametric diametric(nullptr, data, {RS_Vector(6, 8), 0});
    REQUIRE(radial.getMeasuredValue());
    REQUIRE(diametric.getMeasuredValue());
    CHECK(*radial.getMeasuredValue() == Catch::Approx(5));
    CHECK(*diametric.getMeasuredValue() == Catch::Approx(10));

    LC_DimOrdinate ordinate(nullptr, data, {RS_Vector(-3, 4), RS_Vector(0, 0), true});
    REQUIRE(ordinate.getMeasuredValue());
    CHECK(*ordinate.getMeasuredValue() == Catch::Approx(-3));
    ordinate.setForXDirection(false);
    CHECK(*ordinate.getMeasuredValue() == Catch::Approx(4));

    RS_DimensionData rotatedData(data);
    rotatedData.horizontalAxisDirection = M_PI_2;
    LC_DimOrdinate rotated(nullptr, rotatedData, {RS_Vector(-3, 4), RS_Vector(0, 0), true});
    REQUIRE(rotated.getMeasuredValue());
    CHECK(*rotated.getMeasuredValue() == Catch::Approx(4));
    rotated.setForXDirection(false);
    CHECK(*rotated.getMeasuredValue() == Catch::Approx(3));

    RS_DimensionData angularData(data);
    angularData.definitionPoint = RS_Vector(0, 1);
    RS_DimAngular angular(nullptr, angularData,
                          {RS_Vector(1, 0), RS_Vector(0, 0), RS_Vector(0, 0), RS_Vector(1, -1)});
    REQUIRE(angular.getMeasuredValue());
    CHECK(*angular.getMeasuredValue() == Catch::Approx(3 * M_PI_2));
    CHECK(angular.getMeasuredValueText().toDouble() == Catch::Approx(270));
    std::unique_ptr<RS_Entity> angularClone(angular.clone());
    auto* clonedAngular = static_cast<RS_DimAngular*>(angularClone.get());
    REQUIRE(clonedAngular->getMeasuredValue());
    CHECK(*clonedAngular->getMeasuredValue() == Catch::Approx(3 * M_PI_2));
    angular.rotate(RS_Vector(0, 0), M_PI_2);
    REQUIRE(angular.getMeasuredValue());
    CHECK(*angular.getMeasuredValue() == Catch::Approx(3 * M_PI_2));

    const QLocale oldLocale;
    QLocale::setDefault(QLocale(QLocale::German));
    RS_DimAligned decimal(nullptr, data, {RS_Vector(0, 0), RS_Vector(1.25, 0)});
    CHECK(decimal.getMeasuredValueText() == "1.25");
    QLocale::setDefault(oldLocale);

    RS_DimAligned invalid(nullptr, data, {RS_Vector(false), RS_Vector(0, 0)});
    CHECK_FALSE(invalid.getMeasuredValue());
    CHECK(invalid.getMeasuredValueText().isEmpty());
    RS_DimAligned zero(nullptr, data, {RS_Vector(0, 0), RS_Vector(0, 0)});
    CHECK(zero.getMeasuredValueText() == "0");
    RS_DimAligned tiny(nullptr, data, {RS_Vector(0, 0), RS_Vector(1e-12, 0)});
    CHECK(tiny.getMeasuredValueText().toDouble() == Catch::Approx(1e-12).margin(1e-24));
    RS_DimAligned large(nullptr, data, {RS_Vector(0, 0), RS_Vector(1e100, 0)});
    CHECK(large.getMeasuredValueText().toDouble() == Catch::Approx(1e100));
}

TEST_CASE("Dimension copy uses the generated label without rebuilding it", "[dimension][clipboard]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_DimensionData data;
    data.definitionPoint = RS_Vector(0, 3);
    data.middleOfText = RS_Vector(5, 3);
    data.style = "Standard";
    data.text = "fixed label";

    RS_DimAligned aligned(&graphic, data, {RS_Vector(0, 0), RS_Vector(10, 0)});
    aligned.update();
    const auto count = aligned.count();
    const auto oldMeasurement = aligned.getMeasurement();
    CHECK(aligned.getFormattedValueText() == "fixed label");
    CHECK(aligned.count() == count);
    CHECK(aligned.getMeasurement() == oldMeasurement);
    const auto* cachedStyle = aligned.getEffectiveCachedDimStyle();
    REQUIRE(cachedStyle != nullptr);
    CHECK(aligned.getFormattedValueText() == "fixed label");
    CHECK(aligned.getEffectiveCachedDimStyle() == cachedStyle);
    aligned.clearCachedDimStyle();
    RS_InsertData insertData;
    insertData.updateMode = RS2::NoUpdate;
    auto* arrow = new RS_Insert(&aligned, insertData);
    RS_MTextData arrowText(RS_Vector(0, 0), 1, 30, RS_MTextData::VAMiddle, RS_MTextData::HACenter,
                           RS_MTextData::LeftToRight, RS_MTextData::Exact, 1, "arrow text", "Standard", 0, RS2::NoUpdate);
    arrow->addEntity(new RS_MText(arrow, arrowText));
    aligned.addEntity(arrow);
    CHECK(aligned.getFormattedValueText() == "fixed label");
    aligned.addEntity(new RS_MText(&aligned, arrowText));
    CHECK(aligned.getFormattedValueText().isEmpty());
    aligned.setLabel(QStringLiteral("fixed\\S+0.1^-0.2;"));
    aligned.update();
    CHECK(aligned.getFormattedValueText() == "fixed+0.1^-0.2");
    aligned.setLabel(" ");
    aligned.update();
    CHECK(aligned.getFormattedValueText().isEmpty());
    CHECK(aligned.getMeasuredValueText() == "10");
    auto* style = graphic.getResolvedDimStyle("Standard", RS2::EntityDimAligned);
    REQUIRE(style != nullptr);
    style->scaling()->setLinearFactor(2);
    style->linearFormat()->setFormat(RS2::Decimal);
    style->linearFormat()->setDecimalFormatSeparatorChar('.');
    aligned.setLabel(QString());
    aligned.update();
    CHECK(aligned.getMeasuredValueText() == "10");
    bool numeric = false;
    const double displayed = aligned.getFormattedValueText().toDouble(&numeric);
    REQUIRE(numeric);
    CHECK(displayed == Catch::Approx(20));
    aligned.setLabel("{<>*<>}");
    aligned.update();
    CHECK(aligned.getFormattedValueText() == "400");
    aligned.setLabel(QStringLiteral("尺寸 Ø"));
    aligned.update();
    CHECK(aligned.getFormattedValueText() == QStringLiteral("尺寸 Ø"));
}

TEST_CASE("Formatted copy follows architectural units", "[dimension][clipboard]") {
    (void)lc::test::application();
    const bool oldUnitless = LC_GET_ONE_BOOL("Appearance", "UnitlessGrid", true);
    struct RestoreUnitless {
        bool value;
        ~RestoreUnitless() { LC_SET_ONE("Appearance", "UnitlessGrid", value); }
    } restore{oldUnitless};
    LC_SET_ONE("Appearance", "UnitlessGrid", true);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.setUnit(RS2::Inch);
    auto* style = graphic.getResolvedDimStyle("Standard", RS2::EntityDimAligned);
    REQUIRE(style != nullptr);
    style->linearFormat()->setFormat(RS2::Architectural);
    style->linearFormat()->setDecimalPlaces(2);
    RS_DimensionData data;
    data.definitionPoint = RS_Vector(0, 3);
    data.middleOfText = RS_Vector(9, 3);
    data.style = "Standard";
    RS_DimAligned dimension(&graphic, data, {RS_Vector(0, 0), RS_Vector(18.5, 0)});
    dimension.update();
    CHECK(dimension.getMeasuredValueText() == "18.5");
    CHECK(dimension.getFormattedValueText() == QStringLiteral("1'-6 1/2\""));
}

TEST_CASE("Arc dimensions keep their measured length when scaled", "[dimension][clipboard]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_DimensionData data;
    data.definitionPoint = RS_Vector(10, 0);
    data.middleOfText = RS_Vector(0, 10);
    data.style = "Standard";
    LC_DimArcData arcData(15, 10 * M_PI_2, RS_Vector(0, 0), RS_Vector(0, 1), RS_Vector(1, 0));
    arcData.arcSymbol = 1;
    arcData.isPartial = true;
    LC_DimArc arc(&graphic, data, arcData);
    REQUIRE(arc.getMeasuredValue());
    CHECK(*arc.getMeasuredValue() == Catch::Approx(10 * M_PI_2));
    CHECK(arc.getFormattedValueText().startsWith(QStringLiteral("∩ ")));
    arcData.arcSymbol = 0;
    LC_DimArc inlineSymbol(&graphic, data, arcData);
    CHECK(inlineSymbol.getFormattedValueText().startsWith(QStringLiteral("∩ ")));
    arcData.arcSymbol = 2;
    LC_DimArc noSymbol(&graphic, data, arcData);
    CHECK_FALSE(noSymbol.getFormattedValueText().contains(QStringLiteral("∩")));
    arc.scale(RS_Vector(0, 0), RS_Vector(2, 2));
    REQUIRE(arc.getMeasuredValue());
    CHECK(*arc.getMeasuredValue() == Catch::Approx(20 * M_PI_2));
}

TEST_CASE("Each measured dimension exposes its generated label", "[dimension][clipboard]") {
    (void)lc::test::application();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_DimensionData data;
    data.definitionPoint = RS_Vector(0, 3);
    data.middleOfText = RS_Vector(5, 3);
    data.style = "Standard";
    data.text = "fixed label";

    for (const auto type : {RS2::EntityDimRadial, RS2::EntityDimDiametric}) {
        auto* style = graphic.getResolvedDimStyle("Standard", type);
        REQUIRE(style != nullptr);
        style->linearFormat()->setFormat(RS2::Decimal);
        style->linearFormat()->setDecimalPlaces(2);
        style->linearFormat()->setDecimalFormatSeparatorChar(',');
    }
    RS_DimensionData automatic(data);
    automatic.text.clear();
    RS_DimLinear linear(&graphic, data, {RS_Vector(0, 0), RS_Vector(3, 4), 0, 0});
    RS_DimRadial radial(&graphic, automatic, {RS_Vector(3, 4), 0});
    RS_DimDiametric diametric(&graphic, automatic, {RS_Vector(6, 8), 0});
    LC_DimOrdinate ordinate(&graphic, data, {RS_Vector(-3, 4), RS_Vector(5, 4), true});
    linear.update();
    radial.update();
    diametric.update();
    ordinate.update();
    CHECK(linear.getFormattedValueText() == "fixed label");
    CHECK(radial.getFormattedValueText() == "3,16");
    CHECK(diametric.getFormattedValueText() == "7,81");
    CHECK(ordinate.getFormattedValueText() == "fixed label");

    RS_DimensionData angularData(data);
    angularData.definitionPoint = RS_Vector(0, 1);
    angularData.middleOfText = RS_Vector(1, -1);
    angularData.text.clear();
    RS_DimAngular angular(&graphic, angularData,
                          {RS_Vector(1, 0), RS_Vector(0, 0), RS_Vector(0, 0), RS_Vector(1, -1)});
    angular.update();
    CHECK(angular.getFormattedValueText() == QStringLiteral("270°"));
}
