/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cmath>
#include <filesystem>
#include <memory>

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

#include "lc_action_modify_trim.h"
#include "lc_action_offsetcurve_updatesource.h"
#include "lc_action_offsetcurve_restoreoriginal.h"
#include "lc_actionhandlerfactory.h"
#include "lc_actiontestsupport.h"
#include "lc_offsetcurveactiontestsupport.h"
#include "lc_filtertestsupport.h"
#include "../../offset_worker/lc_offsetworkerrequest.h"
#include "lc_offsetcurve.h"
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
    const auto format = GENERATE(RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018);
    const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
    CAPTURE(kind);
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

    constexpr double semiMajor = 10.0;
    constexpr double ratio = .2;
    constexpr double vertexRadius = semiMajor * ratio * ratio;
    const double distance = kind == SourceKind::BoundedHyperbola ? .5 * vertexRadius : .05;
    auto* target = native(graphic, distance);
    const LC_OffsetSourceGeometry sourceData = kind == SourceKind::BoundedHyperbola
        ? LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {semiMajor, 0}, ratio, -.4, .4, false}}
        : sourceGeometry(kind);
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
    const RS_Vector sourcePick = kind == SourceKind::BoundedHyperbola
        ? RS_Vector{semiMajor, 0} : sourcePickPoint(kind);
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
    else cutterDirection *= .5;
    auto* cutter = new RS_Line(&graphic, jet.point - cutterDirection, jet.point + cutterDirection);
    cutter->setLayer(graphic.findLayer("0"));
    graphic.addEntity(cutter);
    const RS_Vector targetPick = nativeCurve->getPointAt(
        parameterStart + targetFraction * (parameterEnd - parameterStart));
    REQUIRE(targetPick.valid);
    TrimActionProbe trim(&context);
    trim.init(0);
    mouse(trim, view, jet.point + cutterDirection * .75, true);
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
