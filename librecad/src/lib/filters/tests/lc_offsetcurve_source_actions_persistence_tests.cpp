/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cmath>
#include <filesystem>
#include <memory>

#include <QApplication>
#include <QCursor>
#include <QFile>
#include <QMouseEvent>
#include <QStringList>
#include <QTemporaryDir>

#include "lc_action_offsetcurve_updatesource.h"
#include "lc_action_offsetcurve_restoreoriginal.h"
#include "lc_actionhandlerfactory.h"
#include "lc_actiontestsupport.h"
#include "lc_filtertestsupport.h"
#include "lc_offsetcurve.h"
#include "rs_ellipse.h"
#include "rs_layer.h"
#include "rs_line.h"

namespace {
namespace geo = LC_EllipseOffsetGeometry;
class Context final : public LC_ActionContext {
public:
    QStringList messages;
    void commandMessage(const QString& message) override { messages.append(message); }
};
LC_OffsetCurve* native(RS_Graphic& graphic) {
    RS_Ellipse source(nullptr, {{0, 0}, {1, 0}, 1., 0., .1, false});
    auto prepared = LC_OffsetCurve::create(source, .25, false);
    CAPTURE(prepared.refusal);
    REQUIRE(prepared);
    prepared.entity->setLayer(graphic.findLayer("0"));
    auto* result = prepared.entity.release();
    graphic.addEntity(result);
    return result;
}
LC_OffsetCurve* onlyNative(RS_Graphic& graphic) {
    LC_OffsetCurve* result = nullptr;
    for (auto* entity : graphic) {
        if (entity->isDeleted() || entity->rtti() != RS2::EntityOffsetCurve) continue;
        REQUIRE(result == nullptr);
        result = dynamic_cast<LC_OffsetCurve*>(entity);
        REQUIRE(result);
    }
    REQUIRE(result);
    return result;
}
void basisEquals(const RS_EllipseData& actual, const RS_EllipseData& expected) {
    CHECK(actual.center == expected.center);
    CHECK(actual.majorP == expected.majorP);
    CHECK(actual.ratio == expected.ratio);
    CHECK(actual.angle1 == expected.angle1);
    CHECK(actual.angle2 == expected.angle2);
    CHECK(actual.reversed == expected.reversed);
}
void certified(const LC_OffsetCurve& curve) {
    REQUIRE(curve.hasEditableDefinition());
    auto check = geo::checkCache(curve.getData(), curve.definition(), curve.checkedWorldTolerance());
    CAPTURE(check.reason);
    REQUIRE(check);
}
void mouse(RS_ActionInterface& action, lc::test::TestGraphicView& view, const RS_Vector& point, bool click) {
    double x = 0, y = 0;
    view.getViewPort()->toUI(point, x, y);
    QMouseEvent event(click ? QEvent::MouseButtonRelease : QEvent::MouseMove,
        QPointF(x, y), QPointF(x, y), click ? Qt::LeftButton : Qt::NoButton,
        Qt::NoButton, Qt::NoModifier);
    if (click) action.mouseReleaseEvent(&event);
    else action.mouseMoveEvent(&event);
}
void reopened(RS_Graphic& graphic, const LC_EllipseOffsetDefinition& expected, int ellipseCount,
              const RS_Pen& pen, const QString& layer) {
    auto* curve = onlyNative(graphic);
    certified(*curve);
    basisEquals(curve->definition().basis, expected.basis);
    CHECK(curve->definition().distance == expected.distance);
    CHECK(curve->definition().towardCentre == expected.towardCentre);
    CHECK(curve->definition().cacheReversed == expected.cacheReversed);
    CHECK(curve->definition().relTolerance == expected.relTolerance);
    REQUIRE(curve->getLayer(true));
    CHECK(curve->getLayer(true)->getName() == layer);
    CHECK(curve->getPen(true) == pen);
    int ellipses = 0, other = 0;
    for (const auto* entity : graphic) {
        if (entity->isDeleted()) continue;
        if (entity->rtti() == RS2::EntityEllipse) {
            ++ellipses;
            auto* ellipse = dynamic_cast<const RS_Ellipse*>(entity);
            REQUIRE(ellipse);
            CHECK_FALSE(dynamic_cast<const LC_OffsetCurve*>(entity));
            basisEquals(ellipse->getData(), expected.basis);
            REQUIRE(ellipse->getLayer(true));
            CHECK(ellipse->getLayer(true)->getName() == layer);
            CHECK(ellipse->getPen(true) == pen);
        } else if (entity->rtti() != RS2::EntityOffsetCurve) ++other;
    }
    CHECK(ellipses == ellipseCount);
    CHECK(other == 0);
}
}

TEST_CASE("I UI registered source actions persist updated native and restored ellipse through R2018",
          "[lc-offsetcurve][i-ui][persistence][dxf][dwg]") {
    REQUIRE(lc::test::application());
    lc::test::QuietLog quiet;
    const auto format = GENERATE(RS2::FormatDXFRW2018, RS2::FormatDWG2018);
    const bool dwg = format == RS2::FormatDWG2018;
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    view.getViewPort()->setOffsetAndFactor(20, 20, 30.);
    Context context;
    context.setDocumentAndView(&graphic, &view);
    auto* curve = native(graphic);
    auto* layer = new RS_Layer("I_UI_SOURCE_ACTIONS");
    graphic.addLayer(layer);
    curve->setLayer(layer);
    const RS_Pen pen(RS_Color(17, 31, 47), RS2::Width11, RS2::SolidLine);
    curve->setPen(pen);
    const auto before = curve->definition();
    auto expected = before;
    expected.basis.center = {4, 8};
    expected.basis.ratio = .8;
    auto* source = new RS_Ellipse(&graphic, expected.basis);
    source->setLayer(graphic.findLayer("0"));
    graphic.addEntity(source);
    if (!curve->isSelected()) graphic.select(QList<RS_Entity*>{curve}, true);
    auto update = LC_ActionsHandlerFactory::createActionInstance(RS2::ActionOffsetCurveUpdateSource, &context);
    REQUIRE(update);
    update->init(0);
    update->setSnapMode(RS_SnapMode{});
    const auto fingerprint = RS_Spline::geometryFingerprint(curve->getData());
    const auto count = graphic.count();
    for (unsigned i = 0; i < 3; ++i) mouse(*update, view, {5, 8}, false);
    CHECK(RS_Spline::geometryFingerprint(curve->getData()) == fingerprint);
    CHECK(graphic.count() == count);
    {
        struct Cursor {
            Cursor() { QApplication::setOverrideCursor(Qt::CrossCursor); }
            ~Cursor() { QApplication::restoreOverrideCursor(); }
        } cursor;
        mouse(*update, view, {5, 8}, true);
        REQUIRE(QApplication::overrideCursor());
        CHECK(QApplication::overrideCursor()->shape() == Qt::CrossCursor);
    }
    auto* updateAction = dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(update.get());
    REQUIRE(updateAction);
    CHECK(updateAction->lastReport().updated == 1);
    CHECK(updateAction->lastReport().refused == 0);
    REQUIRE(curve->isDeleted());
    auto* updated = onlyNative(graphic);
    certified(*updated);
    basisEquals(updated->definition().basis, expected.basis);
    expected = updated->definition();
    // Removing the picked source must not affect the value-owned native basis.
    REQUIRE(graphic.removeEntity(source));
    if (!updated->isSelected()) graphic.select(QList<RS_Entity*>{updated}, true);
    auto restore = LC_ActionsHandlerFactory::createActionInstance(RS2::ActionOffsetCurveRestoreOriginal, &context);
    REQUIRE(restore);
    restore->init(0);
    auto* restoreAction = dynamic_cast<LC_ActionOffsetCurveRestoreOriginal*>(restore.get());
    REQUIRE(restoreAction);
    CHECK(restoreAction->lastReport().restored == 1);
    CHECK(restoreAction->lastReport().refused == 0);
    CHECK_FALSE(updated->isDeleted());

    const auto saveOpen = [&](const QString& stem, int ellipses) {
        const QString file = directory.filePath(stem + (dwg ? ".dwg" : ".dxf"));
        const auto path = std::filesystem::path(file.toStdString());
        const auto cache = RS_Spline::geometryFingerprint(updated->getData());
        const auto members = graphic.count();
        REQUIRE(lc::test::exportDrawing(graphic, path, format));
        CHECK(RS_Spline::geometryFingerprint(updated->getData()) == cache);
        CHECK(graphic.count() == members);
        basisEquals(updated->definition().basis, expected.basis);
        QFile bytes(file);
        REQUIRE(bytes.open(QIODevice::ReadOnly));
        const auto header = bytes.read(dwg ? 6 : bytes.size());
        if (dwg) CHECK(header == "AC1032");
        else {
            CHECK(header.contains("$ACADVER"));
            CHECK(header.contains("AC1032"));
        }
        RS_Graphic loaded;
        REQUIRE(lc::test::importDrawing(loaded, path, format));
        reopened(loaded, expected, ellipses, pen, layer->getName());
    };
    saveOpen("updated-and-restored", 1);
    REQUIRE(graphic.undo()); // only Restore Original
    CHECK_FALSE(updated->isDeleted());
    basisEquals(updated->definition().basis, expected.basis);
    saveOpen("updated-with-restoration-undone", 0);
    REQUIRE(graphic.redo());
    saveOpen("updated-with-restoration-redone", 1);
    REQUIRE(graphic.undo());
    REQUIRE(graphic.undo()); // Update from Source is a separate single step
    CHECK_FALSE(curve->isDeleted());
    CHECK(updated->isDeleted());
    basisEquals(curve->definition().basis, before.basis);
    CHECK_FALSE(graphic.undo());
}
