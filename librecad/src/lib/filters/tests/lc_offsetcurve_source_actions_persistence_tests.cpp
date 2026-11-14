/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <optional>

#include <QApplication>
#include <QCoreApplication>
#include <QCursor>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QMouseEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>

#include <boost/multiprecision/cpp_int.hpp>

#include "lc_action_modify_trim.h"
#include "lc_action_offsetcurve_updatesource.h"
#include "lc_action_offsetcurve_restoreoriginal.h"
#include "lc_actionhandlerfactory.h"
#include "lc_actiontestsupport.h"
#include "lc_offsetcurveactiontestsupport.h"
#include "lc_filtertestsupport.h"
#include "../../offset_worker/lc_offsetworkerrequest.h"
#include "../../offset_worker/lc_offsetworkerresult.h"
#include "../../offset_worker/lc_offsetworkerclient.h"
#include "lc_offsetcurve.h"
#include "lc_offsetcachetrim.h"
#include "lc_ellipseoffsetgeometry.h"
#include "lc_splinepoints.h"
#include "lc_parabola.h"
#include "lc_hyperbola.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_polyline.h"

namespace {
namespace geo = LC_EllipseOffsetGeometry;
using SourceKind = LC_OffsetSourceGeometry::Kind;

LC_OffsetSourceGeometry sourceGeometry(SourceKind kind) {
    switch (kind) {
    case SourceKind::Line: return LC_OffsetSourceGeometry{RS_LineData{{-4, 0}, {4, 0}}};
    case SourceKind::Arc: return LC_OffsetSourceGeometry{RS_ArcData{{0, 0}, 4, .2, 1.4, false}};
    case SourceKind::Circle: return LC_OffsetSourceGeometry{RS_CircleData{{0, 0}, 4}};
    case SourceKind::Ellipse: return LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {5, 0}, .7, 0, 0, false}};
    case SourceKind::Spline: {
        RS_SplineData data{2, false};
        data.controlPoints = {{-4, 0}, {0, 3}, {4, 0}};
        data.knotslist = {0, 0, 0, 1, 1, 1};
        data.weights = {1, .8, 1};
        return LC_OffsetSourceGeometry{data};
    }
    case SourceKind::SplinePoints: {
        LC_SplinePointsData data{false, false};
        data.useControlPoints = true;
        data.controlPoints = {{-4, 0}, {0, 3}, {4, 0}};
        return LC_OffsetSourceGeometry{data};
    }
    case SourceKind::Parabola:
        return LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{-1, 1}, RS_Vector{0, -1}, RS_Vector{1, 1}}}};
    case SourceKind::BoundedHyperbola:
        return LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {3, 0}, .5, -.4, .4}};
    case SourceKind::Polyline:
        return LC_OffsetSourceGeometry{LC_OffsetPolylineData{{0, 0}, {2, 2}, false,
            {RS_ArcData{{1, 0}, 1, std::acos(-1.0), 0, false}, RS_LineData{{2, 0}, {2, 2}}}}};
    case SourceKind::Invalid: break;
    }
    return LC_OffsetSourceGeometry{RS_LineData{}};
}

RS_Vector sourcePickPoint(SourceKind kind) {
    switch (kind) {
    case SourceKind::Line:
        return {0, 0};
    case SourceKind::Polyline: return {1, 1};
    case SourceKind::Arc: return {4 * std::cos(.2), 4 * std::sin(.2)};
    case SourceKind::Circle:
    case SourceKind::Ellipse: return {4, 0};
    case SourceKind::Spline:
    case SourceKind::SplinePoints: return {0, 1.5};
    case SourceKind::Parabola: return {0, -1};
    case SourceKind::BoundedHyperbola: return {3, 0};
    case SourceKind::Invalid: break;
    }
    return {};
}

class Context final : public LC_ActionContext {
public:
    QStringList messages;
    void commandMessage(const QString& message) override { messages.append(message); }
};
class TrimActionProbe final : public LC_ActionModifyTrim {
public:
    using LC_ActionModifyTrim::LC_ActionModifyTrim;
    bool pending() const noexcept { return hasPendingNativeTrim(); }
};
LC_OffsetCurve* native(RS_Graphic& graphic, double distance = .25, bool towardCentre = false) {
    const double radius = towardCentre ? distance * 2.0 : 1.0;
    RS_Ellipse source(nullptr, {{0, 0}, {radius, 0}, 1., 0., .1, false});
    auto prepared = LC_OffsetCurve::create(source, distance, towardCentre);
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
    REQUIRE(curve.isConsistent());
    if (curve.sourceDefinition()) {
        CHECK(curve.sourceDefinition()->source.isWellFormed());
    } else {
        REQUIRE(curve.hasEditableDefinition());
        auto check = geo::checkCache(curve.getData(), curve.definition(), curve.checkedWorldTolerance());
        CAPTURE(check.reason);
        REQUIRE(check);
    }
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

#ifdef DWGSUPPORT
void checkLibreDwgSpline(const QString& readerPath, const std::filesystem::path& file,
                         const RS_SplineData& expected) {
    QProcess reader;
    reader.start(readerPath, {"-O", "minJSON", lc::test::toQString(file)});
    REQUIRE(reader.waitForStarted());
    REQUIRE(reader.waitForFinished(30000));
    INFO(reader.readAllStandardError().toStdString());
    REQUIRE(reader.exitStatus() == QProcess::NormalExit);
    REQUIRE(reader.exitCode() == 0);
    const QByteArray output = reader.readAllStandardOutput();
    REQUIRE_FALSE(output.isEmpty());
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(output, &parseError);
    REQUIRE(parseError.error == QJsonParseError::NoError);
    REQUIRE(document.isObject());

    QJsonObject externalSpline;
    for (const QJsonValue& value : document.object().value("OBJECTS").toArray()) {
        const QJsonObject object = value.toObject();
        if (object.value("entity").toString() != "SPLINE" ||
            object.value("degree").toInt() != expected.degree)
            continue;
        const QJsonArray knots = object.value("knots").toArray();
        const QJsonArray controls = object.value("ctrl_pts").toArray();
        if (knots.size() != static_cast<qsizetype>(expected.knotslist.size()) ||
            controls.size() != static_cast<qsizetype>(expected.controlPoints.size()))
            continue;

        bool sameGeometry = true;
        for (qsizetype i = 0; i < knots.size() && sameGeometry; ++i) {
            sameGeometry = std::abs(knots[i].toDouble() -
                expected.knotslist[static_cast<std::size_t>(i)]) <= 1e-11;
        }
        for (qsizetype i = 0; i < controls.size() && sameGeometry; ++i) {
            const auto index = static_cast<std::size_t>(i);
            const auto& point = expected.controlPoints[index];
            const double weight = expected.weights.empty() ? 1.0 : expected.weights[index];
            const QJsonObject actual = controls[i].toObject();
            sameGeometry = std::abs(actual.value("x").toDouble() - point.x) <= 1e-10 &&
                           std::abs(actual.value("y").toDouble() - point.y) <= 1e-10 &&
                           std::abs(actual.value("z").toDouble() - point.z) <= 1e-10 &&
                           std::abs(actual.value("w").toDouble(1.0) - weight) <= 1e-10;
        }
        if (sameGeometry) {
            externalSpline = object;
            break;
        }
    }
    REQUIRE_FALSE(externalSpline.isEmpty());
    CHECK(externalSpline.value("degree").toInt() == expected.degree);
    const QJsonArray knots = externalSpline.value("knots").toArray();
    REQUIRE(knots.size() == static_cast<qsizetype>(expected.knotslist.size()));
    for (qsizetype i = 0; i < knots.size(); ++i) {
        CHECK(knots[i].toDouble() == Catch::Approx(
            expected.knotslist[static_cast<std::size_t>(i)]).margin(1e-11));
    }
    const QJsonArray controls = externalSpline.value("ctrl_pts").toArray();
    REQUIRE(controls.size() == static_cast<qsizetype>(expected.controlPoints.size()));
    for (qsizetype i = 0; i < controls.size(); ++i) {
        const auto index = static_cast<std::size_t>(i);
        const auto& point = expected.controlPoints[index];
        const double weight = expected.weights.empty() ? 1.0 : expected.weights[index];
        const QJsonObject actual = controls[i].toObject();
        CHECK(actual.value("x").toDouble() == Catch::Approx(point.x).margin(1e-10));
        CHECK(actual.value("y").toDouble() == Catch::Approx(point.y).margin(1e-10));
        CHECK(actual.value("z").toDouble() == Catch::Approx(point.z).margin(1e-10));
        CHECK(actual.value("w").toDouble(1.0) == Catch::Approx(weight).margin(1e-10));
    }
}
#endif
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
    lc::test::awaitUpdate(*updateAction);
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
        else { CHECK(header.contains("$ACADVER")); CHECK(header.contains("AC1032")); }
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

TEST_CASE("I Update from Source preserves every typed source through five DXF and DWG formats",
          "[lc-offsetcurve][i-ui][persistence][dxf][dwg][typed][matrix]") {
    REQUIRE(lc::test::application());
    lc::test::QuietLog quiet;
    const auto kind = GENERATE(SourceKind::Line, SourceKind::Arc, SourceKind::Circle, SourceKind::Ellipse,
        SourceKind::Spline, SourceKind::SplinePoints, SourceKind::Parabola,
        SourceKind::BoundedHyperbola, SourceKind::Polyline);
    const auto format = GENERATE(RS2::FormatDXFRW14, RS2::FormatDXFRW2000, RS2::FormatDWG,
        RS2::FormatDXFRW2018, RS2::FormatDWG2018);
    const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
#ifdef DWGSUPPORT
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
    const QByteArray expectedVersion = (format == RS2::FormatDXFRW14) ? "AC1014" :
        (format == RS2::FormatDXFRW2000 || format == RS2::FormatDWG) ? "AC1015" : "AC1032";
    CAPTURE(static_cast<int>(kind), static_cast<int>(format));

    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    view.getViewPort()->setOffsetAndFactor(20, 20, 30.);
    Context context;
    context.setDocumentAndView(&graphic, &view);

    auto* target = native(graphic);
    const auto originalDefinition = target->definition();
    const double expectedSignedDistance = originalDefinition.towardCentre ?
        -originalDefinition.distance : originalDefinition.distance;
    const RS_Pen pen(RS_Color(255, 0, 0), RS2::WidthDefault, RS2::SolidLine);
    target->setPen(pen);
    auto geometry = sourceGeometry(kind);
    REQUIRE(geometry.isWellFormed());
    auto sourceOwner = geometry.makeEntity();
    REQUIRE(sourceOwner);
    auto* source = sourceOwner.get();
    source->setLayer(graphic.findLayer("0"));
    graphic.addEntity(source);
    sourceOwner.release();
    const auto expectedSource = LC_OffsetSourceGeometry::capture(*source);
    REQUIRE(expectedSource);

    graphic.select(QList<RS_Entity*>{target}, true);
    auto update = LC_ActionsHandlerFactory::createActionInstance(RS2::ActionOffsetCurveUpdateSource, &context);
    REQUIRE(update);
    update->init(0);
    update->setSnapMode(RS_SnapMode{});
    auto* updateAction = dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(update.get());
    REQUIRE(updateAction);
    updateAction->setSourceKind(kind);
    const auto pick = sourcePickPoint(kind);
    for (unsigned i = 0; i < 2; ++i) mouse(*update, view, pick, false);
    const auto fingerprint = RS_Spline::geometryFingerprint(target->getData());
    mouse(*update, view, pick, true);
    lc::test::awaitUpdate(*updateAction);

    CHECK(updateAction->lastReport().updated == 1);
    CHECK(updateAction->lastReport().refused == 0);
    REQUIRE(target->isDeleted());
    auto* updated = onlyNative(graphic);
    certified(*updated);
    CHECK(updated->getPen(true) == pen);
    const auto definition = updated->sourceDefinition();
    if (kind == SourceKind::Ellipse) {
        CHECK_FALSE(definition);
        CHECK(updated->definition().distance == originalDefinition.distance);
        basisEquals(updated->definition().basis, std::get<RS_EllipseData>(expectedSource->value));
    } else {
        REQUIRE(definition);
        CHECK(definition->source.kind() == kind);
        CHECK(definition->signedDistance == expectedSignedDistance);
        CHECK(definition->relativeTolerance == originalDefinition.relTolerance);
        CHECK(definition->cacheReversed == originalDefinition.cacheReversed);
        CHECK(LC_OffsetWorkerRequest::Values::encodeSourceGeometry(definition->source) ==
              LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*expectedSource));
    }

    const auto file = std::filesystem::path(directory.filePath(
        QStringLiteral("updated-%1").arg(static_cast<int>(kind)) + (dwg ? ".dwg" : ".dxf")).toStdString());
    REQUIRE(lc::test::exportDrawing(graphic, file, format));
    QFile bytes(QString::fromStdString(file.string()));
    REQUIRE(bytes.open(QIODevice::ReadOnly));
    const auto header = bytes.read(dwg ? 6 : bytes.size());
    if (dwg) CHECK(header == expectedVersion);
    else { CHECK(header.contains("$ACADVER")); CHECK(header.contains(expectedVersion)); }
#ifdef DWGSUPPORT
    if (dwg && !libreDwgReader.isEmpty())
        checkLibreDwgSpline(libreDwgReader, file, updated->getData());
#endif
    CHECK(RS_Spline::geometryFingerprint(updated->getData()) != fingerprint);
    RS_Graphic loaded;
    REQUIRE(lc::test::importDrawing(loaded, file, format));
    auto* reopenedCurve = onlyNative(loaded);
    certified(*reopenedCurve);
    CHECK(reopenedCurve->getPen(true) == pen);
    const auto reopenedDefinition = reopenedCurve->sourceDefinition();
    if (kind == SourceKind::Ellipse) {
        CHECK_FALSE(reopenedDefinition);
        CHECK(reopenedCurve->definition().distance == originalDefinition.distance);
        basisEquals(reopenedCurve->definition().basis, std::get<RS_EllipseData>(expectedSource->value));
    } else {
        REQUIRE(reopenedDefinition);
        CHECK(reopenedDefinition->source.kind() == kind);
        CHECK(reopenedDefinition->signedDistance == expectedSignedDistance);
        CHECK(reopenedDefinition->relativeTolerance == originalDefinition.relTolerance);
        CHECK(reopenedDefinition->cacheReversed == originalDefinition.cacheReversed);
        CHECK(LC_OffsetWorkerRequest::Values::encodeSourceGeometry(reopenedDefinition->source) ==
              LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*expectedSource));
    }
}

TEST_CASE("I transformed near-boundary fit-point source persists as an editable offset",
          "[lc-offsetcurve][i-ui][persistence][dxf][dwg][typed][fit-points][conditioning]") {
    REQUIRE(lc::test::application());
    lc::test::QuietLog quiet;
    const auto format = GENERATE(RS2::FormatDXFRW14, RS2::FormatDXFRW2000, RS2::FormatDWG,
        RS2::FormatDXFRW2018, RS2::FormatDWG2018);
    const std::size_t transformIndex = GENERATE(std::size_t{0}, std::size_t{1}, std::size_t{2});
    const std::size_t profileIndex = GENERATE(std::size_t{0}, std::size_t{1}, std::size_t{2}, std::size_t{3},
        std::size_t{4}, std::size_t{5}, std::size_t{6}, std::size_t{7}, std::size_t{8}, std::size_t{9});
    const bool closed = (profileIndex >= 4 && profileIndex <= 7) || profileIndex == 9;
    const bool towardCentre = GENERATE(false, true);
    const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
#ifdef DWGSUPPORT
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    const std::array<double, 3> scales{{1e-4, 1.0, 1e4}};
    const std::array<RS_Vector, 3> origins{{{0, 0}, {1e3, -2e3}, {-1e7, 2e7}}};
    const double scale = scales[transformIndex];
    const RS_Vector origin = origins[transformIndex] * scale;
    const bool reflected = transformIndex == 2;
    const double zoom = 30.0 / scale;
    view.getViewPort()->setOffsetAndFactor(qRound(200.0 - origin.x * zoom),
                                          qRound(200.0 - origin.y * zoom), zoom);
    Context context;
    context.setDocumentAndView(&graphic, &view);

    const double gap = profileIndex < 4 ? 2e-4
        : profileIndex < 8 ? 2e-3
        : closed ? 5.1e-4 : 5.1e-5;
    const double distanceScale = profileIndex < 4 ? .002
        : profileIndex < 8 ? .02
        : closed ? .2 : .02;
    const double distance = distanceScale * scale;
    auto* target = native(graphic, distance, towardCentre);
    target->setLayer(graphic.findLayer("0"));
    const RS_Pen pen(RS_Color(255, 0, 0), RS2::WidthDefault, RS2::SolidLine);
    target->setPen(pen);
    const auto expectedDistance = target->definition().towardCentre
        ? -target->definition().distance : target->definition().distance;

    LC_SplinePointsData data{closed, false};
    data.fitTolerance = 1e-8 * scale;
    if (profileIndex == 0)
        data.splinePoints = {{0, 0}, {gap, 0}, {1, 1}, {2, 0}, {3, 1}};
    else if (profileIndex == 1 || profileIndex == 3)
        data.splinePoints = {{0, 0}, {1, 1}, {1 + gap, 1 + gap}, {2, 0}, {3, 1}};
    else if (profileIndex == 2)
        data.splinePoints = {{0, 0}, {1, 1}, {2, 0}, {3, 1}, {3 + gap, 1}};
    else if (profileIndex == 4)
        data.splinePoints = {{0, 0}, {gap, 0}, {10, 0}, {13, 8}, {4, 13}, {-3, 8}};
    else if (profileIndex == 5)
        data.splinePoints = {{0, 0}, {10, 0}, {10 + gap, 0}, {13, 8}, {4, 13}, {-3, 8}};
    else if (profileIndex == 6)
        data.splinePoints = {{0, 0}, {10, 0}, {13, 8}, {13 + gap, 8}, {4, 13}, {-3, 8}};
    else if (profileIndex == 7)
        data.splinePoints = {{0, 0}, {10, 0}, {13, 8}, {4, 13}, {-3, 8}, {0, gap}};
    else if (closed)
        data.splinePoints = {{0, 0}, {10, 0}, {10 + gap, 0}, {13, 8}, {4, 13}, {-3, 8}};
    else
        data.splinePoints = {{0, 0}, {1, 1}, {1 + gap, 1 + gap}, {2, 0}, {3, 1}};
    if (profileIndex == 3) std::reverse(data.splinePoints.begin(), data.splinePoints.end());
    for (RS_Vector& point : data.splinePoints) {
        point.x = origin.x + scale * point.x;
        point.y = origin.y + scale * (reflected ? -point.y : point.y);
    }
    const LC_OffsetSourceGeometry expectedSource{data};
    REQUIRE(expectedSource.isWellFormed());
    auto sourceOwner = expectedSource.makeEntity();
    REQUIRE(sourceOwner);
    auto* source = sourceOwner.get();
    source->setLayer(graphic.findLayer("0"));
    graphic.addEntity(source);
    sourceOwner.release();
    const auto capturedSource = LC_OffsetSourceGeometry::capture(*source);
    REQUIRE(capturedSource);
    const auto expectedEncoding = LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*capturedSource);
    REQUIRE(expectedEncoding);

    graphic.select(QList<RS_Entity*>{target}, true);
    auto update = LC_ActionsHandlerFactory::createActionInstance(RS2::ActionOffsetCurveUpdateSource, &context);
    REQUIRE(update);
    update->init(0);
    update->setSnapMode(RS_SnapMode{});
    auto* updateAction = dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(update.get());
    REQUIRE(updateAction);
    updateAction->setSourceKind(SourceKind::SplinePoints);
    const RS_Vector pick = data.splinePoints[2];
    CAPTURE(format, profileIndex, closed, towardCentre, scale, origin.x, origin.y, reflected,
            gap, distance, pick.x, pick.y);
    for (unsigned i = 0; i < 2; ++i) mouse(*update, view, pick, false);
    mouse(*update, view, pick, true);
    lc::test::awaitUpdate(*updateAction);
    CHECK(updateAction->lastReport().updated == 1);
    CHECK(updateAction->lastReport().refused == 0);
    REQUIRE(target->isDeleted());

    auto* updated = onlyNative(graphic);
    certified(*updated);
    REQUIRE(updated->hasGeneralizedSourceDefinition());
    REQUIRE(updated->sourceDefinition());
    CHECK(updated->sourceDefinition()->source.kind() == SourceKind::SplinePoints);
    CHECK(updated->sourceDefinition()->signedDistance == expectedDistance);
    CHECK(LC_OffsetWorkerRequest::Values::encodeSourceGeometry(updated->sourceDefinition()->source) ==
          expectedEncoding);
    CHECK(updated->getPen(true) == pen);

    const QString file = directory.filePath(QStringLiteral("fit-point-%1").arg(static_cast<int>(format)) +
                                             (dwg ? ".dwg" : ".dxf"));
    const auto path = std::filesystem::path(file.toStdString());
    REQUIRE(lc::test::exportDrawing(graphic, path, format));
#ifdef DWGSUPPORT
    if (dwg && !libreDwgReader.isEmpty())
        checkLibreDwgSpline(libreDwgReader, path, updated->getData());
#endif
    RS_Graphic reopenedGraphic;
    REQUIRE(lc::test::importDrawing(reopenedGraphic, path, format));
    auto* reopenedCurve = onlyNative(reopenedGraphic);
    certified(*reopenedCurve);
    REQUIRE(reopenedCurve->hasGeneralizedSourceDefinition());
    REQUIRE(reopenedCurve->sourceDefinition());
    CHECK(reopenedCurve->sourceDefinition()->source.kind() == SourceKind::SplinePoints);
    CHECK(reopenedCurve->sourceDefinition()->signedDistance == expectedDistance);
    CHECK(LC_OffsetWorkerRequest::Values::encodeSourceGeometry(reopenedCurve->sourceDefinition()->source) ==
          expectedEncoding);
    CHECK(reopenedCurve->getPen(true) == pen);
    CHECK(RS_Spline::geometryFingerprint(reopenedCurve->getData()) ==
          RS_Spline::geometryFingerprint(updated->getData()));
}

TEST_CASE("I thin hyperbola cusp-boundary source persists as an editable offset",
          "[lc-offsetcurve][i-ui][persistence][dxf][dwg][typed][hyperbola][cusp]") {
    REQUIRE(lc::test::application());
    lc::test::QuietLog quiet;
    const double ratio = GENERATE(.05, .2);
    const bool oppositeBranch = GENERATE(false, true);
    const bool reverseBounds = GENERATE(false, true);
    const bool towardCentre = GENERATE(false, true);
    const double fraction = GENERATE(.5, 1.0, 1.5);
    const std::size_t transformIndex = GENERATE(std::size_t{0}, std::size_t{1}, std::size_t{2});
    const auto format = GENERATE(RS2::FormatDXFRW14, RS2::FormatDXFRW2000, RS2::FormatDWG,
        RS2::FormatDXFRW2018, RS2::FormatDWG2018);
    const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
#ifdef DWGSUPPORT
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    const std::array<double, 3> scales{{1e-3, 1.0, 1e4}};
    const std::array<RS_Vector, 3> origins{{{0, 0}, {1e3, -2e3}, {-1e7, 2e7}}};
    const double scale = scales[transformIndex];
    const RS_Vector origin = origins[transformIndex] * scale;
    const double zoom = 30.0 / scale;
    view.getViewPort()->setOffsetAndFactor(qRound(200.0 - origin.x * zoom),
                                          qRound(200.0 - origin.y * zoom), zoom);
    Context context;
    context.setDocumentAndView(&graphic, &view);

    constexpr double semiMajor = 10.0;
    constexpr double axisAngle = .37;
    const double vertexRadius = semiMajor * scale * ratio * ratio;
    const double distance = fraction * vertexRadius;
    const LC_HyperbolaData data{origin,
        {semiMajor * scale * std::cos(axisAngle), semiMajor * scale * std::sin(axisAngle)},
        ratio, reverseBounds ? .4 : -.4, reverseBounds ? -.4 : .4, oppositeBranch};
    LC_Hyperbola hyperbola(nullptr, data);
    REQUIRE(hyperbola.isValid());
    const auto expectedSource = LC_OffsetSourceGeometry::capture(hyperbola);
    REQUIRE(expectedSource);
    const auto expectedEncoding = LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*expectedSource);
    REQUIRE(expectedEncoding);

    auto* target = native(graphic, distance, towardCentre);
    target->setLayer(graphic.findLayer("0"));
    const RS_Pen pen(RS_Color(255, 0, 0), RS2::WidthDefault, RS2::SolidLine);
    target->setPen(pen);
    const double expectedDistance = towardCentre ? -distance : distance;

    auto sourceOwner = expectedSource->makeEntity();
    REQUIRE(sourceOwner);
    auto* source = sourceOwner.get();
    source->setLayer(graphic.findLayer("0"));
    graphic.addEntity(source);
    sourceOwner.release();
    const auto picked = dynamic_cast<const LC_Hyperbola*>(source);
    REQUIRE(picked);
    const RS_Vector pick = picked->getPrimaryVertex();
    REQUIRE(pick.valid);

    graphic.select(QList<RS_Entity*>{target}, true);
    auto update = LC_ActionsHandlerFactory::createActionInstance(RS2::ActionOffsetCurveUpdateSource, &context);
    REQUIRE(update);
    update->init(0);
    update->setSnapMode(RS_SnapMode{});
    auto* updateAction = dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(update.get());
    REQUIRE(updateAction);
    updateAction->setSourceKind(SourceKind::BoundedHyperbola);
    CAPTURE(ratio, oppositeBranch, reverseBounds, towardCentre, fraction, format, transformIndex,
            scale, origin.x, origin.y, vertexRadius, distance, pick.x, pick.y);
    for (unsigned i = 0; i < 2; ++i) mouse(*update, view, pick, false);
    mouse(*update, view, pick, true);
    lc::test::awaitUpdate(*updateAction);
    CHECK(updateAction->lastReport().updated == 1);
    CHECK(updateAction->lastReport().refused == 0);
    REQUIRE(target->isDeleted());

    auto* updated = onlyNative(graphic);
    certified(*updated);
    REQUIRE(updated->hasGeneralizedSourceDefinition());
    REQUIRE(updated->sourceDefinition());
    CHECK(updated->sourceDefinition()->source.kind() == SourceKind::BoundedHyperbola);
    CHECK(updated->sourceDefinition()->signedDistance == expectedDistance);
    CHECK(LC_OffsetWorkerRequest::Values::encodeSourceGeometry(updated->sourceDefinition()->source) ==
          expectedEncoding);
    CHECK(updated->getPen(true) == pen);

    const QString file = directory.filePath(QStringLiteral("hyperbola-%1").arg(static_cast<int>(format)) +
                                             (dwg ? ".dwg" : ".dxf"));
    const auto path = std::filesystem::path(file.toStdString());
    REQUIRE(lc::test::exportDrawing(graphic, path, format));
#ifdef DWGSUPPORT
    if (dwg && !libreDwgReader.isEmpty())
        checkLibreDwgSpline(libreDwgReader, path, updated->getData());
#endif
    RS_Graphic reopenedGraphic;
    REQUIRE(lc::test::importDrawing(reopenedGraphic, path, format));
    auto* reopenedCurve = onlyNative(reopenedGraphic);
    certified(*reopenedCurve);
    REQUIRE(reopenedCurve->hasGeneralizedSourceDefinition());
    REQUIRE(reopenedCurve->sourceDefinition());
    CHECK(reopenedCurve->sourceDefinition()->source.kind() == SourceKind::BoundedHyperbola);
    CHECK(reopenedCurve->sourceDefinition()->signedDistance == expectedDistance);
    CHECK(LC_OffsetWorkerRequest::Values::encodeSourceGeometry(reopenedCurve->sourceDefinition()->source) ==
          expectedEncoding);
    CHECK(reopenedCurve->getPen(true) == pen);
    CHECK(RS_Spline::geometryFingerprint(reopenedCurve->getData()) ==
          RS_Spline::geometryFingerprint(updated->getData()));
}

TEST_CASE("I trimmed every typed source family to plain fragments",
          "[lc-offsetcurve][trim-action][typed-source][all-source-kinds]") {
    const auto kind = GENERATE(SourceKind::Line, SourceKind::Arc, SourceKind::Circle,
        SourceKind::Ellipse, SourceKind::Spline, SourceKind::SplinePoints,
        SourceKind::Parabola, SourceKind::BoundedHyperbola, SourceKind::Polyline);
    const unsigned fitPointVariant = kind == SourceKind::SplinePoints
        ? GENERATE(0u, 1u, 2u, 3u) : 0u;
    const auto format = GENERATE(RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018);
    const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
    CAPTURE(kind);
    CAPTURE(fitPointVariant);
    CAPTURE(static_cast<int>(format));
    REQUIRE(lc::test::application());
    lc::test::QuietLog quiet;
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    constexpr double fitTransformScale = 0.5;
    constexpr double fitTransformAngle = 0.41;
    const RS_Vector fitTransformOrigin{-18.0, 27.0};
    const double fitZoom = fitPointVariant == 3 ? 15.0 / fitTransformScale : 15.0;
    const RS_Vector viewOrigin = fitPointVariant == 3 ? fitTransformOrigin : RS_Vector{};
    view.getViewPort()->setOffsetAndFactor(qRound(200.0 - viewOrigin.x * fitZoom),
        qRound(200.0 - viewOrigin.y * fitZoom),
        kind == SourceKind::SplinePoints && fitPointVariant != 0 ? fitZoom : 30.);
    Context context;
    context.setDocumentAndView(&graphic, &view);

    constexpr double semiMajor = 10.0;
    constexpr double ratio = .2;
    constexpr double vertexRadius = semiMajor * ratio * ratio;
    const double distance = kind == SourceKind::BoundedHyperbola ? .5 * vertexRadius : .05;
    auto* target = native(graphic, distance);
    LC_OffsetSourceGeometry sourceData = kind == SourceKind::BoundedHyperbola
        ? LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {semiMajor, 0}, ratio, -.4, .4, false}}
        : sourceGeometry(kind);
    if (kind == SourceKind::SplinePoints && fitPointVariant != 0) {
        LC_SplinePointsData fitPoints{true, false};
        const double gap = fitPointVariant == 1 ? 0.0 : 5.1e-4;
        fitPoints.splinePoints = fitPointVariant == 1
            ? std::vector<RS_Vector>{{0, 0}, {10, 0}, {13, 8}, {4, 13}, {-3, 8}}
            : std::vector<RS_Vector>{{0, 0}, {10, 0}, {10.0 + gap, 0},
                                     {13, 8}, {4, 13}, {-3, 8}};
        if (fitPointVariant == 3) {
            const double cosine = std::cos(fitTransformAngle);
            const double sine = std::sin(fitTransformAngle);
            for (RS_Vector& point : fitPoints.splinePoints) {
                const double x = point.x;
                const double y = -point.y;
                point = {fitTransformOrigin.x + fitTransformScale * (x * cosine - y * sine),
                         fitTransformOrigin.y + fitTransformScale * (x * sine + y * cosine)};
            }
            std::reverse(fitPoints.splinePoints.begin(), fitPoints.splinePoints.end());
            fitPoints.fitTolerance = 1e-4;
        }
        sourceData = LC_OffsetSourceGeometry{std::move(fitPoints)};
    }
    auto sourceOwner = sourceData.makeEntity();
    REQUIRE(sourceOwner);
    auto* source = sourceOwner.get();
    source->setLayer(graphic.findLayer("0"));
    graphic.addEntity(source);
    sourceOwner.release();
    graphic.select(QList<RS_Entity*>{target}, true);

    auto update = LC_ActionsHandlerFactory::createActionInstance(RS2::ActionOffsetCurveUpdateSource, &context);
    REQUIRE(update);
    update->init(0);
    update->setSnapMode(RS_SnapMode{});
    auto* updateAction = dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(update.get());
    REQUIRE(updateAction);
    updateAction->setSourceKind(kind);
    RS_Vector sourcePick = kind == SourceKind::BoundedHyperbola
        ? RS_Vector{semiMajor, 0} : sourcePickPoint(kind);
    if (kind == SourceKind::SplinePoints && fitPointVariant != 0) {
        double sourceDistance = 0.0;
        RS_Vector pickTarget{10.0, 0.0};
        if (fitPointVariant == 3) {
            const double cosine = std::cos(fitTransformAngle);
            const double sine = std::sin(fitTransformAngle);
            pickTarget = {fitTransformOrigin.x + fitTransformScale * (10.0 * cosine),
                          fitTransformOrigin.y + fitTransformScale * (10.0 * sine)};
        }
        sourcePick = source->getNearestPointOnEntity(pickTarget, true, &sourceDistance);
        REQUIRE(sourcePick.valid);
        CHECK(sourceDistance >= 0.0);
    }
    for (unsigned i = 0; i < 2; ++i) mouse(*update, view, sourcePick, false);
    mouse(*update, view, sourcePick, true);
    lc::test::awaitUpdate(*updateAction);
    REQUIRE(updateAction->lastReport().updated == 1);
    auto* nativeCurve = onlyNative(graphic);
    if (kind == SourceKind::Ellipse) {
        REQUIRE(nativeCurve->hasEditableDefinition());
        CHECK_FALSE(nativeCurve->hasGeneralizedSourceDefinition());
    }
    else {
        REQUIRE(nativeCurve->hasGeneralizedSourceDefinition());
        REQUIRE(nativeCurve->sourceDefinition());
        CHECK(nativeCurve->sourceDefinition()->source.kind() == kind);
    }
    REQUIRE(graphic.removeEntity(source));

    double parameterStart = 0.0;
    double parameterEnd = 0.0;
    REQUIRE(nativeCurve->getParameterDomain(parameterStart, parameterEnd));
    LC_CurveJet jet;
    const double cutFraction = kind == SourceKind::BoundedHyperbola ? .75 :
        kind == SourceKind::Polyline ? .68 : .5;
    const double targetFraction = kind == SourceKind::Parabola ? .9 :
        kind == SourceKind::Polyline ? .15 : .1;
    const double cutParameter = parameterStart + cutFraction * (parameterEnd - parameterStart);
    REQUIRE(nativeCurve->tryEvaluateJet(cutParameter, LC_CurveEvaluationSide::Interior, jet));
    RS_Vector cutterDirection{-jet.first.y, jet.first.x};
    REQUIRE(cutterDirection.magnitude() > 0.0);
    cutterDirection = cutterDirection / cutterDirection.magnitude();
    if (kind == SourceKind::Circle || kind == SourceKind::Ellipse) {
        RS_Vector tangent = jet.first / jet.first.magnitude();
        cutterDirection += tangent;
        REQUIRE(cutterDirection.magnitude() > 0.0);
        cutterDirection = cutterDirection / cutterDirection.magnitude() * 12.0;
    }
    else if (kind != SourceKind::SplinePoints || fitPointVariant == 0) cutterDirection *= .5;
    RS_Vector cutterStart = jet.point - cutterDirection;
    RS_Vector cutterEnd = jet.point + cutterDirection;
    RS_Vector cutterPick = jet.point + cutterDirection * .75;
    if (kind == SourceKind::SplinePoints && fitPointVariant != 0) {
        cutterStart = {5.0, -5.0};
        cutterEnd = {5.0, 18.0};
        cutterPick = {5.0, 6.5};
        if (fitPointVariant == 3) {
            const double cosine = std::cos(fitTransformAngle);
            const double sine = std::sin(fitTransformAngle);
            const auto transform = [&](const RS_Vector& point) {
                const double y = -point.y;
                return RS_Vector{fitTransformOrigin.x + fitTransformScale * (point.x * cosine - y * sine),
                                 fitTransformOrigin.y + fitTransformScale * (point.x * sine + y * cosine)};
            };
            cutterStart = transform(cutterStart);
            cutterEnd = transform(cutterEnd);
            cutterPick = transform(cutterPick);
        }
    }
    auto* cutter = new RS_Line(&graphic, cutterStart, cutterEnd);
    cutter->setLayer(graphic.findLayer("0"));
    graphic.addEntity(cutter);
    const RS_Vector targetPick = nativeCurve->getPointAt(
        parameterStart + targetFraction * (parameterEnd - parameterStart));
    REQUIRE(targetPick.valid);
    if (kind == SourceKind::SplinePoints && fitPointVariant != 0) {
        const auto preparedTrim = LC_OffsetCacheTrim::prepare(nativeCurve->getData(), *cutter, {targetPick});
        CAPTURE(fitPointVariant, static_cast<int>(preparedTrim.refusal), preparedTrim.rootCount,
                nativeCurve->getData().m_closedFlag);
        REQUIRE(preparedTrim.refusal == LC_OffsetCacheTrim::Refusal::None);
    }
    TrimActionProbe trim(&context);
    trim.init(0);
    mouse(trim, view, cutterPick, true);
    mouse(trim, view, targetPick, true);
    QElapsedTimer trimWait;
    trimWait.start();
    while (trim.pending() && trimWait.elapsed() < 35000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QThread::msleep(1);
    }
    CAPTURE(kind);
    CAPTURE(nativeCurve->size());
    for (const auto& message : context.messages) UNSCOPED_INFO(message.toStdString());
    REQUIRE(nativeCurve->isDeleted());
    CHECK(nativeCurve->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    std::vector<const RS_Spline*> fragments;
    for (const auto* entity : graphic) {
        if (!entity || entity->isDeleted() || entity == cutter) continue;
        const auto* spline = dynamic_cast<const RS_Spline*>(entity);
        REQUIRE(spline);
        CHECK(dynamic_cast<const LC_OffsetCurve*>(spline) == nullptr);
        fragments.push_back(spline);
        CHECK(spline->getDrwExtData().empty());
    }
    REQUIRE(fragments.size() == 1);
    const RS_SplineData trimmed = fragments.front()->getData();
    CHECK_FALSE(trimmed.m_closedFlag);
    REQUIRE(graphic.undo());
    CHECK_FALSE(nativeCurve->isDeleted());
    if (kind == SourceKind::Ellipse) CHECK(nativeCurve->hasEditableDefinition());
    else CHECK(nativeCurve->hasGeneralizedSourceDefinition());
    REQUIRE(graphic.redo());
    CHECK(nativeCurve->isDeleted());
    REQUIRE(fragments.size() == 1);
    const auto output = std::filesystem::path(directory.filePath(
        QStringLiteral("trimmed-source-%1.drawing").arg(static_cast<int>(format))).toStdString());
    REQUIRE(lc::test::exportDrawing(graphic, output, format));
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, output, format));
    std::vector<const RS_Spline*> reopenedFragments;
    for (const auto* entity : reopened)
        if (const auto* spline = dynamic_cast<const RS_Spline*>(entity)) reopenedFragments.push_back(spline);
    REQUIRE(reopenedFragments.size() == 1);
    CHECK(RS_Spline::geometryFingerprint(reopenedFragments.front()->getData()) ==
          RS_Spline::geometryFingerprint(trimmed));
    if (dwg && !libreDwgReader.isEmpty()) checkLibreDwgSpline(libreDwgReader, output, trimmed);
}

TEST_CASE("I persist closed mixed-bulge polyline updates and trim one cache component",
          "[lc-offsetcurve][source-action][polyline][closed][persistence][trim-action][multi-component]") {
    const auto format = GENERATE(RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018);
    const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
    CAPTURE(static_cast<int>(format));
    REQUIRE(lc::test::application());
    lc::test::QuietLog quiet;
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    view.getViewPort()->setOffsetAndFactor(200, 200, 30.);
    Context context;
    context.setDocumentAndView(&graphic, &view);

    auto* target = native(graphic);
    const auto originalCache = RS_Spline::geometryFingerprint(target->getData());
    const auto originalDefinition = target->definition();

    auto sourceOwner = std::make_unique<RS_Polyline>(&graphic);
    sourceOwner->addVertex({0, 0}, .2);
    sourceOwner->addVertex({8, 0}, -.2);
    sourceOwner->addVertex({8, 6});
    sourceOwner->addVertex({0, 6});
    sourceOwner->setClosed(true);
    sourceOwner->endPolyline();
    sourceOwner->setLayer(graphic.findLayer("0"));
    auto* source = sourceOwner.get();
    graphic.addEntity(source);
    sourceOwner.release();
    const auto expectedSource = LC_OffsetSourceGeometry::capture(*source);
    REQUIRE(expectedSource);
    const auto expectedEncoding = LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*expectedSource);
    REQUIRE(expectedEncoding);
    graphic.select(QList<RS_Entity*>{target}, true);

    auto update = LC_ActionsHandlerFactory::createActionInstance(
        RS2::ActionOffsetCurveUpdateSource, &context);
    REQUIRE(update);
    update->init(0);
    update->setSnapMode(RS_SnapMode{});
    auto* updateAction = dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(update.get());
    REQUIRE(updateAction);
    updateAction->setSourceKind(SourceKind::Polyline);
    double sourceDistance = 0.0;
    const RS_Vector sourcePick = source->getNearestPointOnEntity({4, .8}, true, &sourceDistance);
    REQUIRE(sourcePick.valid);
    CHECK(sourceDistance >= 0.0);
    for (unsigned i = 0; i < 2; ++i) mouse(*update, view, sourcePick, false);
    mouse(*update, view, sourcePick, true);
    lc::test::awaitUpdate(*updateAction);

    REQUIRE(updateAction->lastReport().updated == 1);
    CHECK(updateAction->lastReport().refused == 0);
    REQUIRE(target->isDeleted());
    auto* replacement = onlyNative(graphic);
    REQUIRE(replacement->hasGeneralizedSourceDefinition());
    REQUIRE(replacement->sourceDefinition());
    CHECK(replacement->sourceDefinition()->source.kind() == SourceKind::Polyline);
    const auto& polyline = std::get<LC_OffsetPolylineData>(replacement->sourceDefinition()->source.value);
    CHECK(polyline.closed);
    REQUIRE(polyline.segments.size() == 4);
    CHECK(LC_OffsetWorkerRequest::Values::encodeSourceGeometry(replacement->sourceDefinition()->source) ==
          expectedEncoding);

    REQUIRE(graphic.undo());
    CHECK_FALSE(target->isDeleted());
    CHECK(onlyNative(graphic) == target);
    CHECK(RS_Spline::geometryFingerprint(target->getData()) == originalCache);
    basisEquals(target->definition().basis, originalDefinition.basis);
    REQUIRE(graphic.redo());
    CHECK(target->isDeleted());
    CHECK(onlyNative(graphic) == replacement);

    const auto output = std::filesystem::path(directory.filePath(
        QStringLiteral("closed-polyline-%1.drawing").arg(static_cast<int>(format))).toStdString());
    REQUIRE(lc::test::exportDrawing(graphic, output, format));
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, output, format));
    auto* reopenedCurve = onlyNative(reopened);
    REQUIRE(reopenedCurve->hasGeneralizedSourceDefinition());
    REQUIRE(reopenedCurve->sourceDefinition());
    CHECK(LC_OffsetWorkerRequest::Values::encodeSourceGeometry(reopenedCurve->sourceDefinition()->source) ==
          expectedEncoding);
    CHECK(RS_Spline::geometryFingerprint(reopenedCurve->getData()) ==
          RS_Spline::geometryFingerprint(replacement->getData()));
    if (dwg && !libreDwgReader.isEmpty()) checkLibreDwgSpline(libreDwgReader, output, reopenedCurve->getData());

    REQUIRE(graphic.removeEntity(source));
    double parameterStart = 0.0;
    double parameterEnd = 0.0;
    REQUIRE(replacement->getParameterDomain(parameterStart, parameterEnd));
    LC_CurveJet jet;
    REQUIRE(replacement->tryEvaluateJet(parameterStart + .2 * (parameterEnd - parameterStart),
        LC_CurveEvaluationSide::Interior, jet));
    RS_Vector cutterDirection{-jet.first.y, jet.first.x};
    REQUIRE(cutterDirection.magnitude() > 0.0);
    cutterDirection = cutterDirection / cutterDirection.magnitude() * 12.0;
    auto* cutter = new RS_Line(&graphic, jet.point - cutterDirection, jet.point + cutterDirection);
    cutter->setLayer(graphic.findLayer("0"));
    graphic.addEntity(cutter);
    const RS_Vector trimPick = replacement->getPointAt(
        parameterStart + .7 * (parameterEnd - parameterStart));
    REQUIRE(trimPick.valid);
    const auto prepared = LC_OffsetCacheTrim::prepare(replacement->getData(), *cutter, {trimPick});
    CAPTURE(static_cast<int>(prepared.refusal), prepared.rootCount);
    REQUIRE(prepared.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(prepared.fragments.size() > 1);
    std::vector<std::uint64_t> expectedTrimmed;
    for (const auto& fragment : prepared.fragments)
        expectedTrimmed.push_back(RS_Spline::geometryFingerprint(fragment.data));
    std::sort(expectedTrimmed.begin(), expectedTrimmed.end());
    TrimActionProbe trim(&context);
    trim.init(0);
    const auto preTrimCache = RS_Spline::geometryFingerprint(replacement->getData());
    const auto preTrimSource = LC_OffsetWorkerRequest::Values::encodeSourceDefinition(
        *replacement->sourceDefinition());
    REQUIRE(preTrimSource);
    const auto preTrimUndoEpoch = graphic.undoStateEpoch();
    mouse(trim, view, jet.point + cutterDirection * .75, true);
    mouse(trim, view, trimPick, true);
    QElapsedTimer trimWait;
    trimWait.start();
    while (trim.pending() && trimWait.elapsed() < 35000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QThread::msleep(1);
    }
    for (const auto& message : context.messages) UNSCOPED_INFO(message.toStdString());
    CHECK(replacement->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    std::vector<std::uint64_t> actualTrimmed;
    for (const auto* entity : graphic) {
        if (entity->isDeleted() || entity->rtti() != RS2::EntitySpline) continue;
        const auto* spline = dynamic_cast<const RS_Spline*>(entity);
        REQUIRE(spline);
        actualTrimmed.push_back(RS_Spline::geometryFingerprint(spline->getData()));
    }
    std::sort(actualTrimmed.begin(), actualTrimmed.end());
    CHECK(actualTrimmed == expectedTrimmed);
    CHECK(graphic.undoStateEpoch() > preTrimUndoEpoch);
    REQUIRE_FALSE(context.messages.isEmpty());
    CHECK(context.messages.back().contains("Trim converted 1 offset curve(s) to ordinary splines"));
    REQUIRE(graphic.undo());
    CHECK_FALSE(replacement->isDeleted());
    CHECK(onlyNative(graphic) == replacement);
    CHECK(RS_Spline::geometryFingerprint(replacement->getData()) == preTrimCache);
    REQUIRE(replacement->sourceDefinition());
    CHECK(LC_OffsetWorkerRequest::Values::encodeSourceDefinition(*replacement->sourceDefinition()) == preTrimSource);
    REQUIRE(graphic.redo());
    CHECK(replacement->isDeleted());
    actualTrimmed.clear();
    for (const auto* entity : graphic) {
        if (entity->isDeleted() || entity->rtti() != RS2::EntitySpline) continue;
        const auto* spline = dynamic_cast<const RS_Spline*>(entity);
        REQUIRE(spline);
        actualTrimmed.push_back(RS_Spline::geometryFingerprint(spline->getData()));
    }
    std::sort(actualTrimmed.begin(), actualTrimmed.end());
    CHECK(actualTrimmed == expectedTrimmed);

    const auto trimmedOutput = std::filesystem::path(directory.filePath(
        QStringLiteral("closed-polyline-trim-%1.drawing").arg(static_cast<int>(format))).toStdString());
    REQUIRE(lc::test::exportDrawing(graphic, trimmedOutput, format));
    RS_Graphic trimmedReopened;
    REQUIRE(lc::test::importDrawing(trimmedReopened, trimmedOutput, format));
    std::vector<std::uint64_t> reopenedTrimmed;
    for (const auto* entity : trimmedReopened)
        if (const auto* spline = dynamic_cast<const RS_Spline*>(entity);
            spline && entity->rtti() == RS2::EntitySpline)
            reopenedTrimmed.push_back(RS_Spline::geometryFingerprint(spline->getData()));
    std::sort(reopenedTrimmed.begin(), reopenedTrimmed.end());
    CHECK(reopenedTrimmed == expectedTrimmed);
}
TEST_CASE("I replace and persist a reversed rounded polyline source",
          "[lc-offsetcurve][source-action][polyline][reversed][persistence]") {
    const bool reversedTraversal = GENERATE(false, true);
    const auto format = GENERATE(RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018);
    CAPTURE(reversedTraversal, static_cast<int>(format));
    REQUIRE(lc::test::application());
    lc::test::QuietLog quiet;
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    view.getViewPort()->setOffsetAndFactor(200, 200, 30.0);
    Context context;
    context.setDocumentAndView(&graphic, &view);

    const double halfPi = M_PI / 2.0;
    LC_OffsetPolylineData data{
        {1, 0}, {1, 0}, true,
        {RS_LineData{{1, 0}, {12, 0}},
         RS_ArcData{{12, 2}, 2, 3 * halfPi, 2 * M_PI, false},
         RS_LineData{{14, 2}, {14, 8.25}},
         RS_ArcData{{13.25, 8.25}, 0.75, 0, halfPi, false},
         RS_LineData{{13.25, 9}, {1.5, 9}},
         RS_ArcData{{1.5, 7.5}, 1.5, halfPi, M_PI, false},
         RS_LineData{{0, 7.5}, {0, 1}},
         RS_ArcData{{1, 1}, 1, M_PI, 3 * halfPi, false}}};
    if (reversedTraversal) {
        std::vector<LC_OffsetPolylineSegment> reversed;
        reversed.reserve(data.segments.size());
        for (auto it = data.segments.rbegin(); it != data.segments.rend(); ++it) {
            if (const auto* line = std::get_if<RS_LineData>(&*it)) {
                reversed.emplace_back(RS_LineData{line->endpoint, line->startpoint});
            } else {
                auto arc = std::get<RS_ArcData>(*it);
                std::swap(arc.angle1, arc.angle2);
                arc.reversed = !arc.reversed;
                reversed.emplace_back(arc);
            }
        }
        data.segments = std::move(reversed);
        std::swap(data.startpoint, data.endpoint);
    }
    LC_OffsetSourceGeometry expectedSource{data};
    REQUIRE(expectedSource.isWellFormed());
    auto sourceOwner = expectedSource.makeEntity();
    REQUIRE(sourceOwner);
    sourceOwner->setLayer(graphic.findLayer("0"));
    auto* source = sourceOwner.get();
    graphic.addEntity(source);
    sourceOwner.release();
    const auto capturedSource = LC_OffsetSourceGeometry::capture(*source);
    REQUIRE(capturedSource);
    expectedSource = *capturedSource;
    data = std::get<LC_OffsetPolylineData>(expectedSource.value);
    const auto expectedEncoding = LC_OffsetWorkerRequest::Values::encodeSourceGeometry(expectedSource);
    REQUIRE(expectedEncoding);

    const double signedDistance = reversedTraversal ? -0.25 : 0.25;
    auto* target = native(graphic, .25, reversedTraversal);
    graphic.select(QList<RS_Entity*>{target}, true);
    auto update = LC_ActionsHandlerFactory::createActionInstance(
        RS2::ActionOffsetCurveUpdateSource, &context);
    REQUIRE(update);
    update->init(0);
    update->setSnapMode(RS_SnapMode{});
    auto* action = dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(update.get());
    REQUIRE(action);
    action->setSourceKind(SourceKind::Polyline);
    double sourceDistance = 0.0;
    const RS_Vector sourcePick = source->getNearestPointOnEntity({6, 0}, true, &sourceDistance);
    REQUIRE(sourcePick.valid);
    for (unsigned i = 0; i < 2; ++i) mouse(*update, view, sourcePick, false);
    mouse(*update, view, sourcePick, true);
    lc::test::awaitUpdate(*action);
    REQUIRE(action->lastReport().updated == 1);
    auto* replacement = onlyNative(graphic);
    REQUIRE(replacement->sourceDefinition());
    CHECK(replacement->sourceDefinition()->source.kind() == SourceKind::Polyline);
    CHECK(replacement->sourceDefinition()->signedDistance == signedDistance);
    CHECK(LC_OffsetWorkerRequest::Values::encodeSourceGeometry(
        replacement->sourceDefinition()->source) == expectedEncoding);
    REQUIRE(replacement->isConsistent());
    REQUIRE(graphic.removeEntity(source));

    double cacheStart = 0.0, cacheEnd = 0.0;
    REQUIRE(replacement->getParameterDomain(cacheStart, cacheEnd));
    for (const auto& segment : data.segments) {
        const auto* arc = std::get_if<RS_ArcData>(&segment);
        if (!arc) continue;
        const double orientation = arc->reversed ? -1.0 : 1.0;
        const double expectedRadius = arc->radius - orientation * signedDistance;
        REQUIRE(expectedRadius > 0.0);
        for (const double fraction : {0.25, 0.5, 0.75}) {
            const double angle = arc->angle1 + orientation * arc->getAngleLength() * fraction;
            const RS_Vector expected = arc->center + RS_Vector::polar(expectedRadius, angle);
            double nearest = std::numeric_limits<double>::infinity();
            for (int sample = 0; sample <= 8192; ++sample) {
                const RS_Vector point = replacement->getPointAt(
                    cacheStart + (cacheEnd - cacheStart) * sample / 8192.0);
                if (!point.valid) break;
                nearest = std::min(nearest, point.distanceTo(expected));
            }
            CAPTURE(reversedTraversal, signedDistance, arc->radius, fraction, nearest);
            CHECK(nearest < 0.02);
        }
    }

    const auto cacheFingerprint = RS_Spline::geometryFingerprint(replacement->getData());
    REQUIRE(graphic.undo());
    CHECK_FALSE(target->isDeleted());
    CHECK(onlyNative(graphic) == target);
    REQUIRE(graphic.redo());
    CHECK(target->isDeleted());
    CHECK(onlyNative(graphic) == replacement);

    const auto output = std::filesystem::path(directory.filePath(
        QStringLiteral("reversed-rounded-polyline-%1-%2.drawing")
            .arg(reversedTraversal ? QStringLiteral("reverse") : QStringLiteral("forward"))
            .arg(static_cast<int>(format))).toStdString());
    REQUIRE(lc::test::exportDrawing(graphic, output, format));
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, output, format));
    auto* reopenedCurve = onlyNative(reopened);
    REQUIRE(reopenedCurve->sourceDefinition());
    CHECK(reopenedCurve->sourceDefinition()->source.kind() == SourceKind::Polyline);
    CHECK(reopenedCurve->sourceDefinition()->signedDistance == signedDistance);
    CHECK(LC_OffsetWorkerRequest::Values::encodeSourceGeometry(
        reopenedCurve->sourceDefinition()->source) == expectedEncoding);
    CHECK(RS_Spline::geometryFingerprint(reopenedCurve->getData()) == cacheFingerprint);
}

TEST_CASE("translated rounded polyline action matches an independent worker request",
          "[lc-offsetcurve][source-action][polyline][large-origin]") {
    const RS_Vector origin = GENERATE(RS_Vector{1e6, -1e6}, RS_Vector{1e9, -1e9});
    const double signedDistance = GENERATE(-.25, .25);
    const bool reversedTraversal = GENERATE(false, true);
    const auto format = GENERATE(RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018);
    CAPTURE(origin.x, origin.y, signedDistance, reversedTraversal, static_cast<int>(format));
    REQUIRE(lc::test::application());
    lc::test::QuietLog quiet;
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    constexpr double angle = .37;
    const double zoom = origin.x > 1e8 ? 2.0 : 10.0;
    view.getViewPort()->setOffsetAndFactor(qRound(200. - origin.x * zoom),
                                          qRound(200. - origin.y * zoom), zoom);
    Context context;
    context.setDocumentAndView(&graphic, &view);
    const auto transform = [&](const RS_Vector point) {
        return origin + RS_Vector{std::cos(angle) * point.x - std::sin(angle) * point.y,
                                  std::sin(angle) * point.x + std::cos(angle) * point.y};
    };

    LC_OffsetPolylineData data{
        {1, 0}, {1, 0}, true,
        {RS_LineData{{1, 0}, {12, 0}},
         RS_ArcData{{12, 2}, 2, 3 * M_PI / 2, 2 * M_PI, false},
         RS_LineData{{14, 2}, {14, 8.25}},
         RS_ArcData{{13.25, 8.25}, .75, 0, M_PI / 2, false},
         RS_LineData{{13.25, 9}, {1.5, 9}},
         RS_ArcData{{1.5, 7.5}, 1.5, M_PI / 2, M_PI, false},
         RS_LineData{{0, 7.5}, {0, 1}},
         RS_ArcData{{1, 1}, 1, M_PI, 3 * M_PI / 2, false}}};
    if (reversedTraversal) {
        std::reverse(data.segments.begin(), data.segments.end());
        for (auto& segment : data.segments) {
            if (auto* line = std::get_if<RS_LineData>(&segment)) {
                std::swap(line->startpoint, line->endpoint);
            } else {
                auto& arc = std::get<RS_ArcData>(segment);
                std::swap(arc.angle1, arc.angle2);
                arc.reversed = !arc.reversed;
            }
        }
        std::swap(data.startpoint, data.endpoint);
    }
    data.startpoint = transform(data.startpoint);
    data.endpoint = transform(data.endpoint);
    for (auto& segment : data.segments) {
        if (auto* line = std::get_if<RS_LineData>(&segment)) {
            line->startpoint = transform(line->startpoint);
            line->endpoint = transform(line->endpoint);
        } else {
            auto& arc = std::get<RS_ArcData>(segment);
            arc.center = transform(arc.center);
            arc.angle1 += angle;
            arc.angle2 += angle;
        }
    }
    const LC_OffsetSourceGeometry expectedSource{std::move(data)};
    REQUIRE(expectedSource.isWellFormed());
    auto sourceOwner = expectedSource.makeEntity();
    REQUIRE(sourceOwner);
    auto* source = dynamic_cast<RS_Polyline*>(sourceOwner.get());
    REQUIRE(source);
    source->setLayer(graphic.findLayer("0"));
    graphic.addEntity(source);
    sourceOwner.release();
    const RS_Vector sourcePick = source->getNearestPointOnEntity(transform({7, 0}), true);
    REQUIRE(sourcePick.valid);

    auto* target = native(graphic, std::abs(signedDistance), signedDistance < 0.0);
    graphic.select(QList<RS_Entity*>{target}, true);
    auto update = LC_ActionsHandlerFactory::createActionInstance(
        RS2::ActionOffsetCurveUpdateSource, &context);
    REQUIRE(update);
    update->init(0);
    update->setSnapMode(RS_SnapMode{});
    auto* action = dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(update.get());
    REQUIRE(action);
    action->setSourceKind(SourceKind::Polyline);
    mouse(*update, view, sourcePick, false);
    mouse(*update, view, sourcePick, true);
    lc::test::awaitUpdate(*action);
    REQUIRE(action->lastReport().updated == 1);
    auto* published = onlyNative(graphic);
    REQUIRE(published->sourceDefinition());
    CHECK(published->sourceDefinition()->signedDistance == signedDistance);
    const auto captured = LC_OffsetSourceGeometry::capture(*source);
    REQUIRE(captured);
    const auto sourceEncoding = LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*captured);
    REQUIRE(sourceEncoding);
    CHECK(sourceEncoding == LC_OffsetWorkerRequest::Values::encodeSourceGeometry(
        published->sourceDefinition()->source));

    const LC_OffsetWorkerRequest::Request workerRequest{{1, 0, 0, 0},
        LC_OffsetWorkerRequest::GenerateSource{*published->sourceDefinition(), {}}};
    LC_OffsetWorkerClient independentWorker;
    std::optional<LC_OffsetWorkerClient::Outcome> workerOutcome;
    QEventLoop eventLoop;
    QTimer deadline;
    deadline.setSingleShot(true);
    QObject::connect(&deadline, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    REQUIRE(independentWorker.submit(workerRequest, LC_OffsetWorkerClient::Mode::Commit,
        [&](LC_OffsetWorkerClient::Outcome outcome) {
            workerOutcome.emplace(std::move(outcome));
            eventLoop.quit();
        }));
    deadline.start(35000);
    eventLoop.exec();
    REQUIRE(workerOutcome);
    REQUIRE(workerOutcome->status == LC_OffsetWorkerClient::Status::Completed);
    REQUIRE(workerOutcome->completed);
    auto independentlyGenerated = workerOutcome->completed->nativeSource(*published);
    REQUIRE(independentlyGenerated);
    CHECK(RS_Spline::geometryFingerprint(independentlyGenerated->getData()) ==
          RS_Spline::geometryFingerprint(published->getData()));

    const auto& polyline = std::get<LC_OffsetPolylineData>(captured->value);
    double first = 0.0, last = 0.0;
    REQUIRE(published->getParameterDomain(first, last));
    const auto checkNearest = [&](const RS_Vector& expected, const double fraction) {
        double nearest = std::numeric_limits<double>::infinity();
        for (int sample = 0; sample <= 8192; ++sample) {
            const RS_Vector candidate = published->getPointAt(first +
                (last - first) * sample / 8192.0);
            if (candidate.valid) nearest = std::min(nearest, candidate.distanceTo(expected));
        }
        CAPTURE(fraction, expected, nearest);
        CHECK(nearest < .02);
    };
    for (const auto& segment : polyline.segments) {
        if (const auto* line = std::get_if<RS_LineData>(&segment)) {
            const RS_Vector tangent = line->endpoint - line->startpoint;
            REQUIRE(tangent.magnitude() > 0.0);
            const RS_Vector normal{-tangent.y / tangent.magnitude(),
                                   tangent.x / tangent.magnitude()};
            for (const double fraction : {.25, .5, .75}) {
                checkNearest(line->startpoint + tangent * fraction +
                             normal * signedDistance, fraction);
            }
            continue;
        }
        const auto& arc = std::get<RS_ArcData>(segment);
        const double direction = arc.reversed ? -1.0 : 1.0;
        const double radius = arc.radius - direction * signedDistance;
        REQUIRE(radius > 0.0);
        for (const double fraction : {.25, .5, .75}) {
            const double angleAt = arc.angle1 + direction * arc.getAngleLength() * fraction;
            const RS_Vector expected = arc.center + RS_Vector::polar(radius, angleAt);
            checkNearest(expected, fraction);
        }
    }

    REQUIRE(graphic.removeEntity(source));
    const RS_Vector cutterStart = transform({10, -30});
    const RS_Vector cutterEnd = transform({10, 30});
    auto* cutter = new RS_Line(&graphic, cutterStart, cutterEnd);
    cutter->setLayer(graphic.findLayer("0"));
    graphic.addEntity(cutter);
    const RS_Vector targetPick = published->getPointAt(first + .1 * (last - first));
    REQUIRE(targetPick.valid);
    const auto trimKernel = LC_OffsetCacheTrim::prepare(
        published->getData(), *cutter, {targetPick});
    REQUIRE(trimKernel.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(trimKernel.rootCount == 2);
    REQUIRE(trimKernel.fragments.size() == 1);
    std::vector<std::uint64_t> expectedTrimmed;
    for (const auto& fragment : trimKernel.fragments)
        expectedTrimmed.push_back(RS_Spline::geometryFingerprint(fragment.data));
    std::sort(expectedTrimmed.begin(), expectedTrimmed.end());

    graphic.select(QList<RS_Entity*>{published}, true);
    TrimActionProbe trim(&context);
    trim.init(0);
    mouse(trim, view, cutterStart, true);
    mouse(trim, view, targetPick, true);
    QElapsedTimer trimWait;
    trimWait.start();
    while (trim.pending() && trimWait.elapsed() < 35000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QThread::msleep(1);
    }
    CHECK_FALSE(trim.pending());
    CHECK(published->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    std::vector<std::uint64_t> actualTrimmed;
    for (const auto* entity : graphic) {
        if (entity->isDeleted() || entity->rtti() != RS2::EntitySpline) continue;
        const auto* spline = dynamic_cast<const RS_Spline*>(entity);
        REQUIRE(spline);
        actualTrimmed.push_back(RS_Spline::geometryFingerprint(spline->getData()));
    }
    std::sort(actualTrimmed.begin(), actualTrimmed.end());
    CHECK(actualTrimmed == expectedTrimmed);

    REQUIRE(graphic.undo());
    CHECK_FALSE(published->isDeleted());
    REQUIRE(graphic.redo());
    CHECK(published->isDeleted());
    actualTrimmed.clear();
    for (const auto* entity : graphic) {
        if (entity->isDeleted() || entity->rtti() != RS2::EntitySpline) continue;
        const auto* spline = dynamic_cast<const RS_Spline*>(entity);
        REQUIRE(spline);
        actualTrimmed.push_back(RS_Spline::geometryFingerprint(spline->getData()));
    }
    std::sort(actualTrimmed.begin(), actualTrimmed.end());
    CHECK(actualTrimmed == expectedTrimmed);

    const auto output = std::filesystem::path(directory.filePath(
        QStringLiteral("translated-rounded-trim-%1-%2-%3.drawing")
            .arg(signedDistance < 0.0 ? QStringLiteral("inward") : QStringLiteral("outward"))
            .arg(reversedTraversal ? QStringLiteral("reversed") : QStringLiteral("forward"))
            .arg(static_cast<int>(format)))
            .toStdString());
    REQUIRE(lc::test::exportDrawing(graphic, output, format));
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, output, format));
    std::vector<std::uint64_t> reopenedTrimmed;
    for (const auto* entity : reopened) {
        if (entity->isDeleted() || entity->rtti() != RS2::EntitySpline) continue;
        const auto* spline = dynamic_cast<const RS_Spline*>(entity);
        REQUIRE(spline);
        reopenedTrimmed.push_back(RS_Spline::geometryFingerprint(spline->getData()));
    }
    std::sort(reopenedTrimmed.begin(), reopenedTrimmed.end());
    CHECK(reopenedTrimmed == expectedTrimmed);
}

TEST_CASE("bounded hyperbola source trims through document actions and file lanes",
          "[lc-offsetcurve][source-action][hyperbola][trim-action][persistence]") {
    const auto format = GENERATE(RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018);
    const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
    CAPTURE(static_cast<int>(format));
    REQUIRE(lc::test::application());
    lc::test::QuietLog quiet;
    QTemporaryDir directory;
    REQUIRE(directory.isValid());

    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    view.getViewPort()->setOffsetAndFactor(200, 200, 20.0);
    Context context;
    context.setDocumentAndView(&graphic, &view);

    auto* target = native(graphic);
    const LC_OffsetSourceGeometry expectedSource{LC_HyperbolaData{
        {0, 0}, {8.0 * std::cos(0.37), 8.0 * std::sin(0.37)},
        0.5, -0.9, 1.1, false}};
    REQUIRE(expectedSource.isWellFormed());
    auto sourceOwner = expectedSource.makeEntity();
    REQUIRE(sourceOwner);
    auto* source = dynamic_cast<LC_Hyperbola*>(sourceOwner.get());
    REQUIRE(source);
    source->setLayer(graphic.findLayer("0"));
    const RS_Vector sourcePick = source->getPrimaryVertex();
    REQUIRE(sourcePick.valid);
    graphic.addEntity(source);
    sourceOwner.release();
    const auto expectedEncoding = LC_OffsetWorkerRequest::Values::encodeSourceGeometry(expectedSource);
    REQUIRE(expectedEncoding);

    graphic.select(QList<RS_Entity*>{target}, true);
    auto update = LC_ActionsHandlerFactory::createActionInstance(
        RS2::ActionOffsetCurveUpdateSource, &context);
    REQUIRE(update);
    update->init(0);
    update->setSnapMode(RS_SnapMode{});
    auto* updateAction = dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(update.get());
    REQUIRE(updateAction);
    updateAction->setSourceKind(SourceKind::BoundedHyperbola);
    for (unsigned i = 0; i < 2; ++i) mouse(*update, view, sourcePick, false);
    mouse(*update, view, sourcePick, true);
    lc::test::awaitUpdate(*updateAction);
    REQUIRE(updateAction->lastReport().updated == 1);
    CHECK(updateAction->lastReport().refused == 0);
    CHECK(target->isDeleted());

    auto* replacement = onlyNative(graphic);
    REQUIRE(replacement->sourceDefinition());
    CHECK(replacement->sourceDefinition()->source.kind() == SourceKind::BoundedHyperbola);
    CHECK(replacement->sourceDefinition()->signedDistance == 0.25);
    CHECK(LC_OffsetWorkerRequest::Values::encodeSourceGeometry(
              replacement->sourceDefinition()->source) == expectedEncoding);
    const auto cacheBeforeTrim = RS_Spline::geometryFingerprint(replacement->getData());

    const RS_Vector majorAxis{std::cos(0.37), std::sin(0.37)};
    const RS_Vector minorAxis{-majorAxis.y, majorAxis.x};
    const RS_Vector cutterCenter = majorAxis * 9.5;
    const RS_Vector cutterHalfLength = minorAxis * 20.0;
    auto* cutter = new RS_Line(&graphic, cutterCenter - cutterHalfLength,
                               cutterCenter + cutterHalfLength);
    cutter->setLayer(graphic.findLayer("0"));
    graphic.addEntity(cutter);

    double cacheStart = 0.0;
    double cacheEnd = 0.0;
    REQUIRE(replacement->getParameterDomain(cacheStart, cacheEnd));
    const RS_Vector targetPick = replacement->getPointAt(cacheStart + 0.5 * (cacheEnd - cacheStart));
    REQUIRE(targetPick.valid);
    const auto prepared = LC_OffsetCacheTrim::prepare(
        replacement->getData(), *cutter, {targetPick});
    CAPTURE(static_cast<int>(prepared.refusal), prepared.rootCount,
            prepared.fragments.size());
    REQUIRE(prepared.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(prepared.rootCount == 2);
    REQUIRE(prepared.fragments.size() == 2);
    std::vector<std::uint64_t> expectedFragments;
    for (const auto& fragment : prepared.fragments)
        expectedFragments.push_back(RS_Spline::geometryFingerprint(fragment.data));
    std::sort(expectedFragments.begin(), expectedFragments.end());

    graphic.select(QList<RS_Entity*>{replacement}, true);
    TrimActionProbe trimAction(&context);
    trimAction.init(0);
    mouse(trimAction, view, cutterCenter, true);
    mouse(trimAction, view, targetPick, true);
    QElapsedTimer trimWait;
    trimWait.start();
    while (trimAction.pending() && trimWait.elapsed() < 35000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QThread::msleep(1);
    }
    CHECK_FALSE(trimAction.pending());
    CHECK(replacement->isDeleted());
    CHECK_FALSE(cutter->isDeleted());

    const auto collectFragments = [](const RS_Graphic& drawing) {
        std::vector<std::uint64_t> fingerprints;
        for (const auto* entity : drawing) {
            if (entity->isDeleted() || entity->rtti() != RS2::EntitySpline) continue;
            const auto* spline = dynamic_cast<const RS_Spline*>(entity);
            if (spline) fingerprints.push_back(RS_Spline::geometryFingerprint(spline->getData()));
        }
        std::sort(fingerprints.begin(), fingerprints.end());
        return fingerprints;
    };
    CHECK(collectFragments(graphic) == expectedFragments);
    REQUIRE(graphic.undo());
    CHECK_FALSE(replacement->isDeleted());
    CHECK(RS_Spline::geometryFingerprint(replacement->getData()) == cacheBeforeTrim);
    REQUIRE(graphic.redo());
    CHECK(replacement->isDeleted());
    CHECK(collectFragments(graphic) == expectedFragments);

    const auto output = std::filesystem::path(directory.filePath(
        QStringLiteral("hyperbola-trim-%1.drawing").arg(static_cast<int>(format))).toStdString());
    REQUIRE(lc::test::exportDrawing(graphic, output, format));
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, output, format));
    CHECK(collectFragments(reopened) == expectedFragments);
    if (dwg) {
        QString libreDwgReader = qEnvironmentVariable("DWGREAD");
        if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
        if (!libreDwgReader.isEmpty()) {
            for (const auto* entity : reopened) {
                if (entity->rtti() != RS2::EntitySpline) continue;
                const auto* spline = dynamic_cast<const RS_Spline*>(entity);
                REQUIRE(spline);
                checkLibreDwgSpline(libreDwgReader, output, spline->getData());
                break;
            }
        }
    }
}

TEST_CASE("partial eccentric ellipse source trims through document actions and file lanes",
          "[lc-offsetcurve][source-action][ellipse][partial][trim-action][persistence]") {
    const bool reversed = GENERATE(false, true);
    const auto format = GENERATE(RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018);
    const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
    CAPTURE(reversed, static_cast<int>(format));
    REQUIRE(lc::test::application());
    lc::test::QuietLog quiet;
    QTemporaryDir directory;
    REQUIRE(directory.isValid());

    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    view.getViewPort()->setOffsetAndFactor(200, 200, 20.0);
    Context context;
    context.setDocumentAndView(&graphic, &view);

    auto* target = native(graphic);
    constexpr double axisAngle = 0.37;
    constexpr double majorRadius = 8.0;
    constexpr double ratio = 0.25;
    const LC_OffsetSourceGeometry expectedSource{RS_EllipseData{
        {0, 0}, {majorRadius * std::cos(axisAngle), majorRadius * std::sin(axisAngle)},
        ratio, reversed ? 1.1 : -1.1, reversed ? -1.1 : 1.1, reversed}};
    REQUIRE(expectedSource.isWellFormed());
    auto sourceOwner = expectedSource.makeEntity();
    REQUIRE(sourceOwner);
    auto* source = dynamic_cast<RS_Ellipse*>(sourceOwner.get());
    REQUIRE(source);
    source->setLayer(graphic.findLayer("0"));
    const RS_Vector sourcePick = source->getMajorPoint();
    REQUIRE(sourcePick.valid);
    graphic.addEntity(source);
    sourceOwner.release();

    graphic.select(QList<RS_Entity*>{target}, true);
    auto update = LC_ActionsHandlerFactory::createActionInstance(
        RS2::ActionOffsetCurveUpdateSource, &context);
    REQUIRE(update);
    update->init(0);
    update->setSnapMode(RS_SnapMode{});
    auto* updateAction = dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(update.get());
    REQUIRE(updateAction);
    updateAction->setSourceKind(SourceKind::Ellipse);
    for (unsigned i = 0; i < 2; ++i) mouse(*update, view, sourcePick, false);
    mouse(*update, view, sourcePick, true);
    lc::test::awaitUpdate(*updateAction);
    REQUIRE(updateAction->lastReport().updated == 1);
    CHECK(updateAction->lastReport().refused == 0);
    CHECK(target->isDeleted());

    auto* replacement = onlyNative(graphic);
    REQUIRE(replacement->hasEditableDefinition());
    CHECK(replacement->definition().distance == 0.25);
    const auto& actualBasis = replacement->definition().basis;
    const auto& expectedBasis = source->getData();
    CHECK(actualBasis.center == expectedBasis.center);
    CHECK(actualBasis.majorP == expectedBasis.majorP);
    CHECK(actualBasis.ratio == expectedBasis.ratio);
    CHECK(std::abs(std::remainder(actualBasis.angle1 - expectedBasis.angle1,
                                  2.0 * M_PI)) < 1e-12);
    CHECK(std::abs(std::remainder(actualBasis.angle2 - expectedBasis.angle2,
                                  2.0 * M_PI)) < 1e-12);
    CHECK(actualBasis.reversed == expectedBasis.reversed);
    const auto cacheBeforeTrim = RS_Spline::geometryFingerprint(replacement->getData());

    const RS_Vector majorAxis{std::cos(axisAngle), std::sin(axisAngle)};
    const RS_Vector minorAxis{-majorAxis.y, majorAxis.x};
    const RS_Vector cutterCenter = majorAxis * 7.0;
    const RS_Vector cutterHalfLength = minorAxis * 20.0;
    auto* cutter = new RS_Line(&graphic, cutterCenter - cutterHalfLength,
                               cutterCenter + cutterHalfLength);
    cutter->setLayer(graphic.findLayer("0"));
    graphic.addEntity(cutter);

    double cacheStart = 0.0;
    double cacheEnd = 0.0;
    REQUIRE(replacement->getParameterDomain(cacheStart, cacheEnd));
    const RS_Vector targetPick = replacement->getPointAt(cacheStart + 0.5 * (cacheEnd - cacheStart));
    REQUIRE(targetPick.valid);
    const auto prepared = LC_OffsetCacheTrim::prepare(
        replacement->getData(), *cutter, {targetPick});
    CAPTURE(static_cast<int>(prepared.refusal), prepared.rootCount,
            prepared.fragments.size());
    REQUIRE(prepared.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(prepared.rootCount == 2);
    REQUIRE(prepared.fragments.size() == 2);
    std::vector<std::uint64_t> expectedFragments;
    for (const auto& fragment : prepared.fragments)
        expectedFragments.push_back(RS_Spline::geometryFingerprint(fragment.data));
    std::sort(expectedFragments.begin(), expectedFragments.end());

    graphic.select(QList<RS_Entity*>{replacement}, true);
    TrimActionProbe trimAction(&context);
    trimAction.init(0);
    mouse(trimAction, view, cutterCenter, true);
    mouse(trimAction, view, targetPick, true);
    QElapsedTimer trimWait;
    trimWait.start();
    while (trimAction.pending() && trimWait.elapsed() < 35000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QThread::msleep(1);
    }
    CHECK_FALSE(trimAction.pending());
    CHECK(replacement->isDeleted());
    CHECK_FALSE(cutter->isDeleted());

    const auto collectFragments = [](const RS_Graphic& drawing) {
        std::vector<std::uint64_t> fingerprints;
        for (const auto* entity : drawing) {
            if (entity->isDeleted() || entity->rtti() != RS2::EntitySpline) continue;
            const auto* spline = dynamic_cast<const RS_Spline*>(entity);
            if (spline) fingerprints.push_back(RS_Spline::geometryFingerprint(spline->getData()));
        }
        std::sort(fingerprints.begin(), fingerprints.end());
        return fingerprints;
    };
    CHECK(collectFragments(graphic) == expectedFragments);
    REQUIRE(graphic.undo());
    CHECK_FALSE(replacement->isDeleted());
    CHECK(RS_Spline::geometryFingerprint(replacement->getData()) == cacheBeforeTrim);
    REQUIRE(graphic.redo());
    CHECK(replacement->isDeleted());
    CHECK(collectFragments(graphic) == expectedFragments);

    const auto output = std::filesystem::path(directory.filePath(
        QStringLiteral("partial-ellipse-trim-%1.drawing").arg(static_cast<int>(format))).toStdString());
    REQUIRE(lc::test::exportDrawing(graphic, output, format));
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, output, format));
    CHECK(collectFragments(reopened) == expectedFragments);
    if (dwg) {
        QString libreDwgReader = qEnvironmentVariable("DWGREAD");
        if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
        if (!libreDwgReader.isEmpty()) {
            for (const auto* entity : reopened) {
                if (entity->rtti() != RS2::EntitySpline) continue;
                const auto* spline = dynamic_cast<const RS_Spline*>(entity);
                REQUIRE(spline);
                checkLibreDwgSpline(libreDwgReader, output, spline->getData());
                break;
            }
        }
    }
}

TEST_CASE("rational high-degree splines trim through document actions and file lanes",
          "[lc-offsetcurve][source-action][spline][high-degree][interior-knot][trim-action][persistence]") {
    const std::size_t degree = GENERATE(std::size_t{4}, std::size_t{6});
    const std::size_t multiplicityCase = GENERATE(std::size_t{1}, std::size_t{2}, std::size_t{3});
    const std::size_t interiorMultiplicity = multiplicityCase == 3 ? degree - 1 : multiplicityCase;
    const int transformed = GENERATE(0, 1);
    const double frameAngle = transformed ? 0.37 : 0.0;
    const RS_Vector frameOrigin = transformed ? RS_Vector{2.0, -3.0} : RS_Vector{0.0, 0.0};
    const auto mapPoint = [frameAngle, frameOrigin](const RS_Vector& point) {
        const double cosine = std::cos(frameAngle);
        const double sine = std::sin(frameAngle);
        return RS_Vector{cosine * point.x - sine * point.y + frameOrigin.x,
                         sine * point.x + cosine * point.y + frameOrigin.y};
    };
    const auto format = GENERATE(RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018);
    const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
    CAPTURE(degree, interiorMultiplicity, transformed, static_cast<int>(format));
    REQUIRE(lc::test::application());
    lc::test::QuietLog quiet;
    QTemporaryDir directory;
    REQUIRE(directory.isValid());

    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    view.getViewPort()->setOffsetAndFactor(200, 200, 20.0);
    Context context;
    context.setDocumentAndView(&graphic, &view);

    RS_SplineData splineData;
    splineData.degree = degree;
    splineData.type = RS_SplineData::SplineType::ClampedOpen;
    splineData.savedOpenType = splineData.type;
    const std::size_t controlCount = splineData.degree + interiorMultiplicity + 1;
    for (std::size_t i = 0; i < controlCount; ++i) {
        const double t = double(i) / double(controlCount - 1);
        splineData.controlPoints.push_back(mapPoint(
            RS_Vector{4.0 * t, 1.2 * t * (1.0 - t) + 0.1 * t}));
        splineData.weights.push_back(0.75 + 0.5 * t);
    }
    splineData.knotslist.insert(splineData.knotslist.end(), splineData.degree + 1, 0.0);
    splineData.knotslist.insert(splineData.knotslist.end(), interiorMultiplicity, 0.5);
    splineData.knotslist.insert(splineData.knotslist.end(), splineData.degree + 1, 1.0);
    const LC_OffsetSourceGeometry expectedSource{splineData};
    REQUIRE(expectedSource.isWellFormed());
    auto sourceOwner = expectedSource.makeEntity();
    REQUIRE(sourceOwner);
    auto* source = dynamic_cast<RS_Spline*>(sourceOwner.get());
    REQUIRE(source);
    REQUIRE(source->validate());
    source->setLayer(graphic.findLayer("0"));
    double sourceStart = 0.0;
    double sourceEnd = 0.0;
    REQUIRE(source->getParameterDomain(sourceStart, sourceEnd));
    const RS_Vector sourcePick = source->getPointAt(sourceStart + 0.5 * (sourceEnd - sourceStart));
    REQUIRE(sourcePick.valid);
    graphic.addEntity(source);
    sourceOwner.release();
    const auto capturedSource = LC_OffsetSourceGeometry::capture(*source);
    REQUIRE(capturedSource);
    const auto expectedEncoding = LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*capturedSource);
    REQUIRE(expectedEncoding);

    auto* target = native(graphic, 0.02);
    graphic.select(QList<RS_Entity*>{target}, true);
    auto update = LC_ActionsHandlerFactory::createActionInstance(
        RS2::ActionOffsetCurveUpdateSource, &context);
    REQUIRE(update);
    update->init(0);
    update->setSnapMode(RS_SnapMode{});
    auto* updateAction = dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(update.get());
    REQUIRE(updateAction);
    updateAction->setSourceKind(SourceKind::Spline);
    for (unsigned i = 0; i < 2; ++i) mouse(*update, view, sourcePick, false);
    mouse(*update, view, sourcePick, true);
    lc::test::awaitUpdate(*updateAction);
    REQUIRE(updateAction->lastReport().updated == 1);
    CHECK(updateAction->lastReport().refused == 0);
    CHECK(target->isDeleted());

    auto* replacement = onlyNative(graphic);
    REQUIRE(replacement->sourceDefinition());
    CHECK(replacement->sourceDefinition()->source.kind() == SourceKind::Spline);
    CHECK(replacement->sourceDefinition()->signedDistance == 0.02);
    CHECK(LC_OffsetWorkerRequest::Values::encodeSourceGeometry(
              replacement->sourceDefinition()->source) == expectedEncoding);
    CHECK(std::get<RS_SplineData>(replacement->sourceDefinition()->source.value).degree == degree);
    CHECK(std::get<RS_SplineData>(replacement->sourceDefinition()->source.value).knotslist ==
          splineData.knotslist);
    const auto cacheBeforeTrim = RS_Spline::geometryFingerprint(replacement->getData());

    auto* cutter = new RS_Line(&graphic, mapPoint({-10.0, 0.15}), mapPoint({10.0, 0.15}));
    cutter->setLayer(graphic.findLayer("0"));
    graphic.addEntity(cutter);
    double cacheStart = 0.0;
    double cacheEnd = 0.0;
    REQUIRE(replacement->getParameterDomain(cacheStart, cacheEnd));
    const RS_Vector targetPick = replacement->getPointAt(cacheStart + 0.5 * (cacheEnd - cacheStart));
    REQUIRE(targetPick.valid);
    const auto prepared = LC_OffsetCacheTrim::prepare(
        replacement->getData(), *cutter, {targetPick});
    CAPTURE(static_cast<int>(prepared.refusal), prepared.rootCount,
            prepared.fragments.size());
    REQUIRE(prepared.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(prepared.rootCount == 2);
    REQUIRE(prepared.fragments.size() == 2);
    std::vector<std::uint64_t> expectedFragments;
    for (const auto& fragment : prepared.fragments)
        expectedFragments.push_back(RS_Spline::geometryFingerprint(fragment.data));
    std::sort(expectedFragments.begin(), expectedFragments.end());

    graphic.select(QList<RS_Entity*>{replacement}, true);
    view.getViewPort()->zoomIn(10.0, targetPick);
    TrimActionProbe trimAction(&context);
    trimAction.init(0);
    mouse(trimAction, view, mapPoint({-4.0, 0.15}), true);
    mouse(trimAction, view, targetPick, true);
    QElapsedTimer trimWait;
    trimWait.start();
    while (trimAction.pending() && trimWait.elapsed() < 35000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QThread::msleep(1);
    }
    for (const auto& message : context.messages)
        UNSCOPED_INFO(message.toStdString());
    CHECK_FALSE(trimAction.pending());
    CHECK(replacement->isDeleted());
    CHECK_FALSE(cutter->isDeleted());

    const auto sourceFingerprint = RS_Spline::geometryFingerprint(splineData);
    const auto collectFragments = [sourceFingerprint](const RS_Graphic& drawing) {
        std::vector<std::uint64_t> fingerprints;
        for (const auto* entity : drawing) {
            if (entity->isDeleted() || entity->rtti() != RS2::EntitySpline) continue;
            const auto* spline = dynamic_cast<const RS_Spline*>(entity);
            if (!spline) continue;
            const auto fingerprint = RS_Spline::geometryFingerprint(spline->getData());
            if (fingerprint != sourceFingerprint) fingerprints.push_back(fingerprint);
        }
        std::sort(fingerprints.begin(), fingerprints.end());
        return fingerprints;
    };
    CHECK(collectFragments(graphic) == expectedFragments);
    REQUIRE(graphic.undo());
    CHECK_FALSE(replacement->isDeleted());
    CHECK(RS_Spline::geometryFingerprint(replacement->getData()) == cacheBeforeTrim);
    REQUIRE(graphic.redo());
    CHECK(replacement->isDeleted());
    CHECK(collectFragments(graphic) == expectedFragments);

    const auto output = std::filesystem::path(directory.filePath(
        QStringLiteral("degree-%1-spline-trim-%2-%3.drawing")
            .arg(static_cast<qulonglong>(degree)).arg(transformed)
            .arg(static_cast<int>(format))).toStdString());
    REQUIRE(lc::test::exportDrawing(graphic, output, format));
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, output, format));
    CHECK(collectFragments(reopened) == expectedFragments);
    if (dwg) {
        QString libreDwgReader = qEnvironmentVariable("DWGREAD");
        if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
        if (!libreDwgReader.isEmpty()) {
            for (const auto* entity : reopened) {
                if (entity->rtti() != RS2::EntitySpline) continue;
                const auto* spline = dynamic_cast<const RS_Spline*>(entity);
                REQUIRE(spline);
                checkLibreDwgSpline(libreDwgReader, output, spline->getData());
                break;
            }
        }
    }
}

TEST_CASE("I trim one closed convex polyline cache component and retain the rest",
          "[lc-offsetcurve][polyline][closed][trim-action][multi-component]") {
    const int profile = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
    const double quarterArcBulge = std::tan(M_PI / 8.0);
    const auto format = GENERATE(RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018);
    const std::array<const char*, 13> profileNames{{
        "square", "rounded", "unequal-radii", "all-arc-circle-ccw", "all-arc-circle-cw",
        "all-arc-circle-cw-centerline-audit", "all-arc-circle-cw-rotated-translated",
        "all-arc-circle-ccw-rotated-translated", "all-arc-circle-cw-large-scale-translated",
        "unequal-radii-rounded-rotated-translated",
        "unequal-radii-rounded-rotated-inward",
        "unequal-radii-rounded-rotated-translated-inward",
        "unequal-radii-rounded-rotated-large-origin-inward"}};
    CAPTURE(profileNames[static_cast<std::size_t>(profile)], static_cast<int>(format));
    REQUIRE(lc::test::application());
    lc::test::QuietLog quiet;
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    view.getViewPort()->setOffsetAndFactor(200, 200, 30.);
    const RS_Vector transformedCenter = profile == 8 || profile == 12 ? RS_Vector{1e9, -1e9}
        : profile == 10 ? RS_Vector{0, 0} : RS_Vector{1e6, -1e6};
    const double transformedRadius = profile == 8 ? 2000. : 2.;
    constexpr double transformedAngle = .37;
    const RS_Vector transformedXAxis{std::cos(transformedAngle), std::sin(transformedAngle)};
    const RS_Vector transformedYAxis{-transformedXAxis.y, transformedXAxis.x};
    constexpr double transformedCutterAngle = .61;
    const RS_Vector transformedCutterAxis{std::cos(transformedCutterAngle), std::sin(transformedCutterAngle)};
    const auto transformLocal = [&](const RS_Vector& point) {
        return transformedCenter + transformedXAxis * point.x + transformedYAxis * point.y;
    };
    if (profile == 6 || profile == 7)
        view.getViewPort()->setOffsetAndFactor(qRound(200. - transformedCenter.x * 30.),
                                              qRound(200. - transformedCenter.y * 30.), 30.);
    else if (profile == 8)
        view.getViewPort()->setOffsetAndFactor(qRound(200. - transformedCenter.x * .001),
                                              qRound(200. - transformedCenter.y * .001), .001);
    else if (profile == 12)
        view.getViewPort()->setOffsetAndFactor(qRound(200. - transformedCenter.x * 2.),
                                              qRound(200. - transformedCenter.y * 2.), 2.);
    else if (profile == 9 || profile == 10 || profile == 11 || profile == 12)
        view.getViewPort()->setOffsetAndFactor(qRound(200. - transformedCenter.x * 10.),
                                              qRound(200. - transformedCenter.y * 10.), 10.);
    Context context;
    context.setDocumentAndView(&graphic, &view);

    auto* target = native(graphic, .25, profile == 10 || profile == 11 || profile == 12);
    auto sourceOwner = std::make_unique<RS_Polyline>(&graphic);
    RS_Vector sourcePick{5, 0};
    double cutterX = 5.0;
    if (profile == 1) {
        sourceOwner->addVertex({1, 0});
        sourceOwner->addVertex({9, 0}, quarterArcBulge);
        sourceOwner->addVertex({10, 1});
        sourceOwner->addVertex({10, 9}, quarterArcBulge);
        sourceOwner->addVertex({9, 10});
        sourceOwner->addVertex({1, 10}, quarterArcBulge);
        sourceOwner->addVertex({0, 9});
        sourceOwner->addVertex({0, 1}, quarterArcBulge);
    } else if (profile == 2) {
        sourcePick = {7, 0};
        cutterX = 7.0;
        sourceOwner->addVertex({1, 0});
        sourceOwner->addVertex({12, 0}, quarterArcBulge);
        sourceOwner->addVertex({14, 2});
        sourceOwner->addVertex({14, 8.25}, quarterArcBulge);
        sourceOwner->addVertex({13.25, 9});
        sourceOwner->addVertex({1.5, 9}, quarterArcBulge);
        sourceOwner->addVertex({0, 7.5});
        sourceOwner->addVertex({0, 1}, quarterArcBulge);
    } else if (profile == 3) {
        sourcePick = {2, 0};
        cutterX = 0.0;
        sourceOwner->addVertex({2, 0}, quarterArcBulge);
        sourceOwner->addVertex({0, 2}, quarterArcBulge);
        sourceOwner->addVertex({-2, 0}, quarterArcBulge);
        sourceOwner->addVertex({0, -2}, quarterArcBulge);
    } else if (profile == 4 || profile == 5) {
        sourcePick = {2, 0};
        cutterX = profile == 4 ? 0.25 : 0.0;
        sourceOwner->addVertex({2, 0}, -quarterArcBulge);
        sourceOwner->addVertex({0, -2}, -quarterArcBulge);
        sourceOwner->addVertex({-2, 0}, -quarterArcBulge);
        sourceOwner->addVertex({0, 2}, -quarterArcBulge);
    } else if (profile == 6 || profile == 8) {
        sourcePick = transformedCenter + transformedXAxis * transformedRadius;
        sourceOwner->addVertex(sourcePick, -quarterArcBulge);
        sourceOwner->addVertex(transformedCenter - transformedYAxis * transformedRadius, -quarterArcBulge);
        sourceOwner->addVertex(transformedCenter - transformedXAxis * transformedRadius, -quarterArcBulge);
        sourceOwner->addVertex(transformedCenter + transformedYAxis * transformedRadius, -quarterArcBulge);
    } else if (profile == 7) {
        sourcePick = transformedCenter + transformedXAxis * transformedRadius;
        sourceOwner->addVertex(sourcePick, quarterArcBulge);
        sourceOwner->addVertex(transformedCenter + transformedYAxis * transformedRadius, quarterArcBulge);
        sourceOwner->addVertex(transformedCenter - transformedXAxis * transformedRadius, quarterArcBulge);
        sourceOwner->addVertex(transformedCenter - transformedYAxis * transformedRadius, quarterArcBulge);
    } else if (profile == 9 || profile == 10 || profile == 11 || profile == 12) {
        sourcePick = transformLocal({7, 0});
        for (const RS_Vector point : {RS_Vector{1, 0}, RS_Vector{12, 0}, RS_Vector{14, 2},
                RS_Vector{14, 8.25}, RS_Vector{13.25, 9}, RS_Vector{1.5, 9},
                RS_Vector{0, 7.5}, RS_Vector{0, 1}})
            sourceOwner->addVertex(transformLocal(point), quarterArcBulge);
    } else {
        sourceOwner->addVertex({0, 0});
        sourceOwner->addVertex({10, 0});
        sourceOwner->addVertex({10, 10});
        sourceOwner->addVertex({0, 10});
    }
    sourceOwner->setClosed(true);
    sourceOwner->endPolyline();
    sourceOwner->setLayer(graphic.findLayer("0"));
    auto* source = sourceOwner.get();
    graphic.addEntity(source);
    sourceOwner.release();
    graphic.select(QList<RS_Entity*>{target}, true);

    auto update = LC_ActionsHandlerFactory::createActionInstance(
        RS2::ActionOffsetCurveUpdateSource, &context);
    REQUIRE(update);
    update->init(0);
    update->setSnapMode(RS_SnapMode{});
    auto* updateAction = dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(update.get());
    REQUIRE(updateAction);
    updateAction->setSourceKind(SourceKind::Polyline);
    for (unsigned i = 0; i < 2; ++i) mouse(*update, view, sourcePick, false);
    mouse(*update, view, sourcePick, true);
    lc::test::awaitUpdate(*updateAction);
    REQUIRE(updateAction->lastReport().updated == 1);
    auto* replacement = onlyNative(graphic);
    REQUIRE(replacement->sourceDefinition());
    const auto capturedSource = LC_OffsetSourceGeometry::capture(*source);
    REQUIRE(capturedSource);
    CHECK(LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*capturedSource) ==
          LC_OffsetWorkerRequest::Values::encodeSourceGeometry(
              replacement->sourceDefinition()->source));
    if (profile == 9 || profile == 10 || profile == 11 || profile == 12) {
        const double signedDistance = replacement->sourceDefinition()->signedDistance;
        double cacheStart = 0., cacheEnd = 0.;
        REQUIRE(replacement->getParameterDomain(cacheStart, cacheEnd));
        const std::size_t sampleCount = std::max<std::size_t>(16384,
            std::min<std::size_t>(65536, std::size_t(replacement->count()) * 128));
        const auto distanceToCache = [&](const RS_Vector& point) {
            const auto distanceSquared = [&](const double parameter) {
                const RS_Vector evaluated = replacement->getPointAt(parameter);
                return evaluated.valid ? (evaluated - point).squared() : INFINITY;
            };
            double bestParameter = cacheStart;
            double bestDistanceSquared = INFINITY;
            for (std::size_t sample = 0; sample <= sampleCount; ++sample) {
                const double parameter = cacheStart + (cacheEnd - cacheStart) *
                    (double(sample) / double(sampleCount));
                const double candidate = distanceSquared(parameter);
                if (candidate < bestDistanceSquared) {
                    bestDistanceSquared = candidate;
                    bestParameter = parameter;
                }
            }
            const double step = (cacheEnd - cacheStart) / double(sampleCount);
            double lower = std::max(cacheStart, bestParameter - step);
            double upper = std::min(cacheEnd, bestParameter + step);
            constexpr double inversePhi = 0.6180339887498948482;
            double left = upper - inversePhi * (upper - lower);
            double right = lower + inversePhi * (upper - lower);
            double leftDistance = distanceSquared(left), rightDistance = distanceSquared(right);
            for (int iteration = 0; iteration < 64; ++iteration) {
                if (leftDistance < rightDistance) {
                    upper = right; right = left; rightDistance = leftDistance;
                    left = upper - inversePhi * (upper - lower);
                    leftDistance = distanceSquared(left);
                } else {
                    lower = left; left = right; leftDistance = rightDistance;
                    right = lower + inversePhi * (upper - lower);
                    rightDistance = distanceSquared(right);
                }
            }
            return std::sqrt(std::min({bestDistanceSquared, leftDistance, rightDistance}));
        };
        std::size_t arcsChecked = 0;
        std::size_t arcEndpointsChecked = 0;
        for (const auto* segment : *source) {
            const auto* arc = dynamic_cast<const RS_Arc*>(segment);
            REQUIRE(arc);
            const auto& data = arc->getData();
            const double orientation = data.reversed ? -1. : 1.;
            const double startAngle = data.reversed ? data.angle2 : data.angle1;
            const double middleAngle = startAngle + orientation * data.getAngleLength() * .5;
            const RS_Vector radial = RS_Vector::polar(1., middleAngle);
            const double expectedRadius = data.radius - orientation * signedDistance;
            REQUIRE(expectedRadius > 0.);
            const RS_Vector expected = data.center + radial * expectedRadius;
            const double error = distanceToCache(expected);
            const double oracleTolerance = std::min(replacement->checkedWorldTolerance(),
                .1 * std::abs(signedDistance));
            CAPTURE(arcsChecked, signedDistance, data.radius, expectedRadius, expected, error,
                replacement->checkedWorldTolerance(), oracleTolerance);
            CHECK(error < oracleTolerance);
            ++arcsChecked;

            for (const double fraction : {.25, .75}) {
                const double angle = startAngle + orientation * data.getAngleLength() * fraction;
                const RS_Vector expectedQuarter = data.center +
                    RS_Vector::polar(expectedRadius, angle);
                const double quarterError = distanceToCache(expectedQuarter);
                CAPTURE(fraction, signedDistance, data.radius, expectedRadius, expectedQuarter,
                    quarterError, replacement->checkedWorldTolerance(), oracleTolerance);
                CHECK(quarterError < oracleTolerance);
            }

            const double endAngle = data.reversed ? data.angle1 : data.angle2;
            for (const double angle : {startAngle, endAngle}) {
                const RS_Vector expectedEndpoint = data.center +
                    RS_Vector::polar(expectedRadius, angle);
                const double endpointError = distanceToCache(expectedEndpoint);
                CAPTURE(arcEndpointsChecked, signedDistance, data.radius, expectedRadius,
                    expectedEndpoint, endpointError, replacement->checkedWorldTolerance(),
                    oracleTolerance);
                CHECK(endpointError < oracleTolerance);
                ++arcEndpointsChecked;
            }
        }
        CHECK(arcsChecked == 8);
        CHECK(arcEndpointsChecked == 16);
    }
    REQUIRE(graphic.removeEntity(source));

    double parameterStart = 0.0;
    double parameterEnd = 0.0;
    REQUIRE(replacement->getParameterDomain(parameterStart, parameterEnd));
    RS_Vector targetPick = replacement->getPointAt(
        parameterStart + ((profile == 10 || profile == 11 || profile == 12) ? .25 : .7) *
            (parameterEnd - parameterStart));
    REQUIRE(targetPick.valid);
    RS_Vector cutterStart{cutterX, -20}, cutterEnd{cutterX, 20};
    if (profile == 6 || profile == 7 || profile == 8) {
        cutterStart = transformedCenter - transformedCutterAxis * (10. * transformedRadius);
        cutterEnd = transformedCenter + transformedCutterAxis * (10. * transformedRadius);
    } else if (profile == 9) {
        cutterStart = transformLocal({7, -20});
        cutterEnd = transformLocal({7, 20});
    }
    double inwardCutterX = 10.;
    if (profile == 10 || profile == 11 || profile == 12) {
        cutterStart = transformLocal({inwardCutterX, -30.});
        cutterEnd = transformLocal({inwardCutterX, 30.});
    }
    auto* cutter = new RS_Line(&graphic, cutterStart, cutterEnd);
    cutter->setLayer(graphic.findLayer("0"));
    graphic.addEntity(cutter);
    if (profile == 5) {
        using Rational = boost::multiprecision::cpp_rational;
        LC_OffsetCheckLimits limits;
        geo::WorkBudget budget{limits};
        const auto layout = geo::inspectCacheLayout(replacement->getData(), budget);
        REQUIRE(layout.status == geo::CacheLayoutStatus::Ok);
        std::size_t certifiedCrossings = 0;
        for (const auto& span : layout.spans) {
            std::array<Rational, 4> signedDistance;
            std::array<Rational, 4> ordinate;
            std::vector<RS_Vector> controls;
            for (std::size_t control = 0; control <= span.degree; ++control) {
                signedDistance[control] = Rational(span.controls[control].x);
                ordinate[control] = Rational(span.controls[control].y);
                controls.emplace_back(span.controls[control].x, span.controls[control].y);
            }
            bool increasing = true, decreasing = true;
            for (std::size_t control = 0; control < span.degree; ++control) {
                const Rational derivative = Rational(span.degree) *
                    (signedDistance[control + 1] - signedDistance[control]);
                increasing = increasing && derivative > 0;
                decreasing = decreasing && derivative < 0;
            }
            const bool crosses = (signedDistance[0] < 0 && signedDistance[span.degree] > 0) ||
                                 (signedDistance[0] > 0 && signedDistance[span.degree] < 0);
            const auto first = signedDistance.begin();
            const auto last = first + static_cast<std::ptrdiff_t>(span.degree + 1);
            const bool sameSide = std::all_of(first, last, [](const Rational& value) { return value > 0; }) ||
                                  std::all_of(first, last, [](const Rational& value) { return value < 0; });
            if (crosses && (increasing || decreasing)) {
                ++certifiedCrossings;
                for (std::size_t control = 0; control <= span.degree; ++control)
                    CHECK((ordinate[control] >= -20 && ordinate[control] <= 20));
                CHECK(LC_OffsetCacheTrim::certifyMonotoneLineRoot(
                    controls, {cutterX, -20}, {cutterX, 20}));
            } else {
                CHECK(sameSide);
                CHECK(LC_OffsetCacheTrim::strictlySeparatedBezierFromLine(
                    controls, {cutterX, -20}, {cutterX, 20}));
            }
        }
        CHECK(certifiedCrossings == 2);
    }
    if (profile == 10 || profile == 11 || profile == 12) {
        std::size_t sampledCrossings = 0;
        std::size_t firstCrossingSample = 4097;
        double minimumAbsDistance = INFINITY;
        double previousDistance = 0.;
        for (std::size_t sample = 0; sample <= 4096; ++sample) {
            const double parameter = parameterStart + (parameterEnd - parameterStart) *
                (double(sample) / 4096.);
            const RS_Vector point = replacement->getPointAt(parameter) - transformedCenter;
            const double distance = point.dotP(transformedXAxis) - inwardCutterX;
            minimumAbsDistance = std::min(minimumAbsDistance, std::abs(distance));
            if (sample && std::signbit(previousDistance) != std::signbit(distance)) {
                ++sampledCrossings;
                firstCrossingSample = std::min(firstCrossingSample, sample);
            }
            previousDistance = distance;
        }
        if (firstCrossingSample < 4097) {
            const std::size_t pickSample = std::min<std::size_t>(4096, firstCrossingSample + 64);
            const double pickParameter = parameterStart + (parameterEnd - parameterStart) *
                (double(pickSample) / 4096.);
            targetPick = replacement->getPointAt(pickParameter);
        }
        CHECK(sampledCrossings == 2);
        CHECK(minimumAbsDistance < .01);
    }
    const auto kernel = LC_OffsetCacheTrim::prepare(replacement->getData(), *cutter, {targetPick});
    CAPTURE(static_cast<int>(kernel.refusal), kernel.rootCount);
    CAPTURE(kernel.work.intersections.totalBoxPairs, kernel.work.intersections.deepestPairRefinement,
            kernel.work.intersections.deepestRootRefinement, kernel.work.intersections.generatedItems);
    REQUIRE(kernel.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(kernel.fragments.size() == ((profile == 10 || profile == 11 || profile == 12) ? 8 : 1));
    if (profile == 10 || profile == 11 || profile == 12) {
        CHECK(kernel.rootCount == 1);
        REQUIRE(kernel.roots.size() == 1);
        CHECK((kernel.roots.front().proof == LC_IntersectionProof::IntervalUniqueTransverse ||
               kernel.roots.front().proof == LC_IntersectionProof::AnalyticMonotoneTransverse));
    }
    if (profile == 5) {
        CHECK(kernel.rootCount == 2);
        CHECK(std::count_if(kernel.roots.begin(), kernel.roots.end(), [](const auto& root) {
            return root.proof == LC_IntersectionProof::AnalyticMonotoneTransverse;
        }) == 2);
        LC_OffsetWorkerRequest::Request request{{1, 0, 0, 0}, LC_OffsetWorkerRequest::TrimCache{
            replacement->getData(), LC_OffsetWorkerRequest::Segment{{cutterX, -20}, {cutterX, 20}}, {targetPick}, {}}};
        LC_OffsetWorkerResult::Reply reply{request.token, 2, LC_OffsetWorkerResult::Failure::None, kernel};
        const auto encoded = LC_OffsetWorkerResult::encode(reply, request);
        REQUIRE(encoded);
        CHECK(LC_OffsetWorkerResult::decode(*encoded, request));
    }
    if (profile == 6 || profile == 7 || profile == 8 || profile == 9) {
        CHECK(kernel.rootCount == 2);
        CHECK(std::count_if(kernel.roots.begin(), kernel.roots.end(), [](const auto& root) {
            return root.proof == LC_IntersectionProof::AnalyticMonotoneTransverse;
        }) == 2);
    }
    const auto expectedOutputCount = kernel.fragments.size();
    std::vector<std::uint64_t> expectedTrimmed;
    for (const auto& fragment : kernel.fragments)
        expectedTrimmed.push_back(RS_Spline::geometryFingerprint(fragment.data));
    std::sort(expectedTrimmed.begin(), expectedTrimmed.end());
    const auto undoEpochBefore = graphic.undoStateEpoch();

    TrimActionProbe trim(&context);
    trim.init(0);
    mouse(trim, view, cutterStart, true);
    mouse(trim, view, targetPick, true);
    QElapsedTimer trimWait;
    trimWait.start();
    while (trim.pending() && trimWait.elapsed() < 35000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QThread::msleep(1);
    }
    for (const auto& message : context.messages) UNSCOPED_INFO(message.toStdString());
    CHECK(replacement->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    std::size_t nativeCount = 0, splineCount = 0;
    for (const auto* entity : graphic) {
        if (entity->isDeleted()) continue;
        nativeCount += entity->rtti() == RS2::EntityOffsetCurve;
        splineCount += entity->rtti() == RS2::EntitySpline;
    }
    CHECK(nativeCount == 0);
    CHECK(splineCount == expectedOutputCount);
    CHECK(graphic.undoStateEpoch() > undoEpochBefore);
    for (const auto& message : context.messages) CHECK_FALSE(message.contains("nothing changed"));

    REQUIRE(graphic.undo());
    CHECK_FALSE(replacement->isDeleted());
    std::size_t restoredSplines = 0;
    for (const auto* entity : graphic)
        if (!entity->isDeleted() && entity->rtti() == RS2::EntitySpline) ++restoredSplines;
    CHECK(restoredSplines == 0);
    REQUIRE(graphic.redo());
    CHECK(replacement->isDeleted());
    std::size_t redoneSplines = 0;
    for (const auto* entity : graphic)
        if (!entity->isDeleted() && entity->rtti() == RS2::EntitySpline) ++redoneSplines;
    CHECK(redoneSplines == expectedOutputCount);

    const auto output = std::filesystem::path(directory.filePath(
        QStringLiteral("closed-polyline-trim-%1-%2.drawing")
            .arg(QString::fromLatin1(profileNames[static_cast<std::size_t>(profile)]))
            .arg(static_cast<int>(format))).toStdString());
    REQUIRE(lc::test::exportDrawing(graphic, output, format));
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, output, format));
    std::vector<std::uint64_t> reopenedTrimmed;
    for (const auto* entity : reopened) {
        if (entity->isDeleted() || entity->rtti() != RS2::EntitySpline) continue;
        const auto* spline = dynamic_cast<const RS_Spline*>(entity);
        REQUIRE(spline);
        reopenedTrimmed.push_back(RS_Spline::geometryFingerprint(spline->getData()));
    }
    std::sort(reopenedTrimmed.begin(), reopenedTrimmed.end());
    CHECK(reopenedTrimmed == expectedTrimmed);
}

TEST_CASE("Native Trim tangency refusal preserves the curve and undo state",
          "[lc-offsetcurve][trim-action][async][tangency][atomic]") {
    REQUIRE(lc::test::application());
    lc::test::QuietLog quiet;
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    view.getViewPort()->setOffsetAndFactor(200, 200, 30.);
    Context context;
    context.setDocumentAndView(&graphic, &view);

    auto* target = native(graphic);
    const auto beforeCache = RS_Spline::geometryFingerprint(target->getData());
    const auto beforeDefinition = target->definition();
    double start = 0, end = 0;
    REQUIRE(target->getParameterDomain(start, end));
    LC_CurveJet jet;
    REQUIRE(target->tryEvaluateJet(start + .5 * (end - start),
        LC_CurveEvaluationSide::Interior, jet));
    REQUIRE(jet.first.magnitude() > 0);
    const RS_Vector halfTangent = jet.first / jet.first.magnitude() * 10.0;
    const RS_Vector targetPick = target->getPointAt(start + .25 * (end - start));
    REQUIRE(targetPick.valid);
    auto* cutter = new RS_Line(&graphic, jet.point - halfTangent, jet.point + halfTangent);
    cutter->setLayer(graphic.findLayer("0"));
    graphic.addEntity(cutter);

    const auto direct = LC_OffsetCacheTrim::prepare(target->getData(), *cutter, {targetPick});
    CAPTURE(static_cast<int>(direct.refusal), direct.rootCount);
    REQUIRE(direct.refusal != LC_OffsetCacheTrim::Refusal::None);
    CHECK(direct.fragments.empty());
    CHECK(direct.conicFragments.empty());
    CHECK_FALSE(direct.removed);

    const auto undoEpoch = graphic.undoStateEpoch();
    TrimActionProbe trim(&context);
    trim.init(0);
    mouse(trim, view, jet.point + halfTangent * .75, true);
    mouse(trim, view, targetPick, true);
    REQUIRE(trim.pending());
    QElapsedTimer wait;
    wait.start();
    while (trim.pending() && wait.elapsed() < 35000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QThread::msleep(1);
    }
    CHECK_FALSE(trim.pending());
    CHECK_FALSE(target->isDeleted());
    CHECK(onlyNative(graphic) == target);
    CHECK(RS_Spline::geometryFingerprint(target->getData()) == beforeCache);
    basisEquals(target->definition().basis, beforeDefinition.basis);
    CHECK(target->definition().distance == beforeDefinition.distance);
    CHECK(target->definition().towardCentre == beforeDefinition.towardCentre);
    CHECK(target->definition().relTolerance == beforeDefinition.relTolerance);
    CHECK(target->definition().cacheReversed == beforeDefinition.cacheReversed);
    CHECK(graphic.undoStateEpoch() == undoEpoch);
    CHECK_FALSE(cutter->isDeleted());
    CHECK(graphic.size() == 2);
    REQUIRE_FALSE(context.messages.isEmpty());
    CHECK(context.messages.back().contains("nothing changed"));
}
