/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <sstream>
#include <functional>
#include <filesystem>
#include <boost/multiprecision/cpp_int.hpp>
#include <limits>
#include <utility>
#include <QPointer>
#include <QScopeGuard>
#include <QMouseEvent>
#include "lc_action_modify_trim.h"
#include "../../../../lib/offset_worker/lc_offsetworkerclient.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QThread>
#include <QProcess>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStandardPaths>
#ifdef Q_OS_UNIX
#include <cerrno>
#include <csignal>
#include <unistd.h>
#endif
#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_offsetcurve.h"
#include "lc_offsetcachetrim.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "lc_offsetcurvexdata.h"
#include "lc_action_offsetcurve_updatesource.h"
#include "lc_curveoffset.h"
#include "lc_hyperbola.h"
#include "lc_parabola.h"
#include "lc_splinepoints.h"
#include "lc_offsetcurveactiontestsupport.h"
#include "lc_filtertestsupport.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_polyline.h"
#include "lc_undosection.h"
#include "rs_selection.h"

namespace {
std::unique_ptr<LC_OffsetWorkerCompletion> joinedTrimWorker(
    const LC_OffsetWorkerRequest::Request& request);

using OracleRational = boost::multiprecision::cpp_rational;
using OraclePolynomial = std::vector<OracleRational>;

void trimPolynomial(OraclePolynomial& p) {
    while (p.size() > 1 && p.back() == 0) p.pop_back();
}

OraclePolynomial addPolynomial(OraclePolynomial a, const OraclePolynomial& b) {
    a.resize(std::max(a.size(), b.size()), OracleRational{0});
    for (std::size_t i = 0; i < b.size(); ++i) a[i] += b[i];
    trimPolynomial(a);
    return a;
}

OraclePolynomial multiplyPolynomial(const OraclePolynomial& a, const OraclePolynomial& b) {
    OraclePolynomial out(a.size() + b.size() - 1, OracleRational{0});
    for (std::size_t i = 0; i < a.size(); ++i)
        for (std::size_t j = 0; j < b.size(); ++j) out[i + j] += a[i] * b[j];
    trimPolynomial(out);
    return out;
}

OraclePolynomial derivativePolynomial(const OraclePolynomial& p) {
    if (p.size() < 2) return {OracleRational{0}};
    OraclePolynomial out(p.size() - 1);
    for (std::size_t i = 1; i < p.size(); ++i) out[i - 1] = OracleRational{i} * p[i];
    trimPolynomial(out);
    return out;
}

OracleRational evaluatePolynomial(const OraclePolynomial& p, const OracleRational& x) {
    OracleRational value{0};
    for (auto it = p.rbegin(); it != p.rend(); ++it) value = value * x + *it;
    return value;
}

OraclePolynomial remainderPolynomial(OraclePolynomial a, const OraclePolynomial& b) {
    trimPolynomial(a);
    if (b.size() == 1 && b.front() == 0) return {OracleRational{0}};
    while (!(a.size() == 1 && a.front() == 0) && a.size() >= b.size()) {
        const std::size_t shift = a.size() - b.size();
        const OracleRational factor = a.back() / b.back();
        for (std::size_t i = 0; i < b.size(); ++i) a[shift + i] -= factor * b[i];
        trimPolynomial(a);
    }
    return a;
}

std::vector<OraclePolynomial> sturmSequence(const OraclePolynomial& input) {
    OraclePolynomial p = input;
    trimPolynomial(p);
    if (p.size() < 2) return {p};
    std::vector<OraclePolynomial> sequence{p, derivativePolynomial(p)};
    while (!(sequence.back().size() == 1 && sequence.back().front() == 0)) {
        OraclePolynomial remainder = remainderPolynomial(
            sequence[sequence.size() - 2], sequence.back());
        if (remainder.size() == 1 && remainder.front() == 0) break;
        for (auto& coefficient : remainder) coefficient = -coefficient;
        sequence.push_back(std::move(remainder));
    }
    return sequence;
}

int sturmVariations(const std::vector<OraclePolynomial>& sequence,
                    const OracleRational& x) {
    int variations = 0;
    int previous = 0;
    for (const auto& polynomial : sequence) {
        const auto value = evaluatePolynomial(polynomial, x);
        const int sign = value > 0 ? 1 : value < 0 ? -1 : 0;
        if (!sign) continue;
        if (previous && previous != sign) ++variations;
        previous = sign;
    }
    return variations;
}

int sturmRootCount(const std::vector<OraclePolynomial>& sequence,
                   const OracleRational& lo, const OracleRational& hi) {
    return sturmVariations(sequence, lo) - sturmVariations(sequence, hi);
}

OraclePolynomial divideLinearFactor(const OraclePolynomial& p,
                                    const OracleRational& root) {
    OraclePolynomial quotient(p.size() - 1, OracleRational{0});
    quotient.back() = p.back();
    for (std::size_t i = quotient.size() - 1; i > 0; --i)
        quotient[i - 1] = p[i] + root * quotient[i];
    trimPolynomial(quotient);
    return quotient;
}

struct OracleRootInterval { OracleRational lo, hi; };

void isolateRoots(const OraclePolynomial& polynomial,
                  const std::vector<OraclePolynomial>& sequence,
                  const OracleRational& lo, const OracleRational& hi,
                  const int count, const unsigned depth,
                  std::vector<OracleRootInterval>& roots) {
    if (!count) return;
    if (count == 1 && depth >= 56) {
        roots.push_back({lo, hi});
        return;
    }
    const OracleRational middle = (lo + hi) / 2;
    if (evaluatePolynomial(polynomial, middle) == 0) {
        roots.push_back({middle, middle});
        const OraclePolynomial quotient = divideLinearFactor(polynomial, middle);
        const auto quotientSturm = sturmSequence(quotient);
        isolateRoots(quotient, quotientSturm, OracleRational{0}, OracleRational{1},
            sturmRootCount(quotientSturm, OracleRational{0}, OracleRational{1}), 0, roots);
        return;
    }
    const int leftCount = sturmRootCount(sequence, lo, middle);
    const int rightCount = sturmRootCount(sequence, middle, hi);
    if (leftCount + rightCount != count || depth >= 128) return;
    isolateRoots(polynomial, sequence, lo, middle, leftCount, depth + 1, roots);
    isolateRoots(polynomial, sequence, middle, hi, rightCount, depth + 1, roots);
}

OraclePolynomial cubicBezierCoordinate(const std::array<OracleRational, 4>& p) {
    return {p[0], 3 * (p[1] - p[0]),
        3 * (p[0] - 2 * p[1] + p[2]),
        -p[0] + 3 * p[1] - 3 * p[2] + p[3]};
}

struct CircleBezierOracle {
    bool valid = false;
    std::size_t distinctRoots = 0;
    std::size_t repeatedRoots = 0;
    double minimumSeparation = 0;
    double selectedArcSweep = 0;
    double selectedArcMargin = 0;
    double maximumAngularError = 0;
};

CircleBezierOracle exactCircleBezierOracle(const RS_SplineData& cache,
    const RS_Vector& center, const double radius, const RS_Vector& pick) {
    CircleBezierOracle result;
    if (cache.degree != 3 || cache.type != RS_SplineData::SplineType::ClampedOpen ||
        cache.controlPoints.size() < 4 || (cache.controlPoints.size() - 1) % 3 ||
        cache.weights.size() != cache.controlPoints.size() ||
        cache.knotslist.size() != cache.controlPoints.size() + 4 || !(radius > 0) ||
        !std::isfinite(radius)) return result;
    if (!std::all_of(cache.weights.begin(), cache.weights.end(),
                     [](double weight) { return weight == 1.0; })) return result;
    const std::size_t spanCount = (cache.controlPoints.size() - 1) / 3;
    std::vector<double> expectedKnots(4, 0.0);
    for (std::size_t span = 1; span < spanCount; ++span)
        expectedKnots.insert(expectedKnots.end(), 3, double(span));
    expectedKnots.insert(expectedKnots.end(), 4, double(spanCount));
    if (cache.knotslist != expectedKnots) return result;

    struct RootPoint { RS_Vector point; double angle; double positionError; };
    std::vector<RootPoint> rootPoints;
    const OracleRational exactRadius{radius};
    const OracleRational radiusSquared = exactRadius * exactRadius;
    const OracleRational cx{center.x}, cy{center.y};
    for (std::size_t offset = 0; offset + 3 < cache.controlPoints.size(); offset += 3) {
        std::array<OracleRational, 4> xControls, yControls;
        OracleRational minX, maxX, minY, maxY;
        for (std::size_t i = 0; i < 4; ++i) {
            const auto& point = cache.controlPoints[offset + i];
            if (!std::isfinite(point.x) || !std::isfinite(point.y)) return result;
            xControls[i] = OracleRational{point.x} - cx;
            yControls[i] = OracleRational{point.y} - cy;
            if (i == 0) minX = maxX = xControls[i], minY = maxY = yControls[i];
            else {
                minX = std::min(minX, xControls[i]); maxX = std::max(maxX, xControls[i]);
                minY = std::min(minY, yControls[i]); maxY = std::max(maxY, yControls[i]);
            }
        }
        // Bezier convex hull exclusion avoids constructing exact polynomials
        // for spans that cannot meet the circle's bounding square.
        if (minX > exactRadius || maxX < -exactRadius ||
            minY > exactRadius || maxY < -exactRadius) continue;

        const auto x = cubicBezierCoordinate(xControls);
        const auto y = cubicBezierCoordinate(yControls);
        OraclePolynomial implicit = addPolynomial(multiplyPolynomial(x, x),
                                                   multiplyPolynomial(y, y));
        implicit[0] -= radiusSquared;
        trimPolynomial(implicit);
        if (evaluatePolynomial(implicit, OracleRational{0}) == 0 ||
            evaluatePolynomial(implicit, OracleRational{1}) == 0) return result;
        const auto sturm = sturmSequence(implicit);
        const int count = sturmRootCount(sturm, OracleRational{0}, OracleRational{1});
        if (count < 0) return result;
        result.distinctRoots += std::size_t(count);

        const auto& gcd = sturm.back();
        if (gcd.size() > 1) {
            const auto gcdSturm = sturmSequence(gcd);
            const int repeated = sturmRootCount(gcdSturm, OracleRational{0}, OracleRational{1});
            if (repeated < 0) return result;
            result.repeatedRoots += std::size_t(repeated);
        }

        std::vector<OracleRootInterval> intervals;
        isolateRoots(implicit, sturm, OracleRational{0}, OracleRational{1}, count, 0, intervals);
        if (intervals.size() != std::size_t(count)) return result;
        const auto derivativeBound = [](const OraclePolynomial& coordinate) {
            OracleRational bound{0};
            for (std::size_t i = 1; i < coordinate.size(); ++i) {
                OracleRational term = OracleRational{i} * coordinate[i];
                if (term < 0) term = -term;
                bound += term;
            }
            double converted = bound.convert_to<double>();
            if (converted > 0)
                converted = std::nextafter(converted, std::numeric_limits<double>::infinity());
            return converted;
        };
        const double speedBound = std::nextafter(std::hypot(derivativeBound(x),
            derivativeBound(y)), std::numeric_limits<double>::infinity());
        for (const auto& interval : intervals) {
            const OracleRational parameter = (interval.lo + interval.hi) / 2;
            const double localX = evaluatePolynomial(x, parameter).convert_to<double>();
            const double localY = evaluatePolynomial(y, parameter).convert_to<double>();
            const auto ulp = [](double value) {
                return std::max(value - std::nextafter(value, -INFINITY),
                    std::nextafter(value, INFINITY) - value);
            };
            const double halfWidth = ((interval.hi - interval.lo) / 2).convert_to<double>();
            const double parameterRadius = std::nextafter(halfWidth,
                std::numeric_limits<double>::infinity());
            const double motionError = std::nextafter(speedBound * parameterRadius,
                std::numeric_limits<double>::infinity());
            const double coordinateError = std::nextafter(std::hypot(ulp(localX), ulp(localY)),
                std::numeric_limits<double>::infinity());
            const double positionError = std::nextafter(motionError + coordinateError,
                std::numeric_limits<double>::infinity());
            if (!std::isfinite(positionError) || !(positionError < radius / 2)) return result;
            const double angularError = std::nextafter(std::asin(positionError /
                (radius - positionError)) + 64 * std::numeric_limits<double>::epsilon(),
                std::numeric_limits<double>::infinity());
            result.maximumAngularError = std::max(result.maximumAngularError, angularError);
            rootPoints.push_back({{localX, localY}, std::atan2(localY, localX),
                                  positionError});
        }
    }
    if (!result.distinctRoots || rootPoints.size() != result.distinctRoots) return result;

    result.minimumSeparation = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < rootPoints.size(); ++i) {
        for (std::size_t j = i + 1; j < rootPoints.size(); ++j) {
            const auto delta = rootPoints[i].point - rootPoints[j].point;
            const double centerDistance = std::nextafter(std::hypot(delta.x, delta.y), 0.0);
            const double lower = std::nextafter(centerDistance - rootPoints[i].positionError -
                rootPoints[j].positionError, -std::numeric_limits<double>::infinity());
            result.minimumSeparation = std::min(result.minimumSeparation, lower);
        }
    }

    if (rootPoints.size() >= 2) {
        std::vector<double> angles;
        angles.reserve(rootPoints.size());
        for (const auto& root : rootPoints) {
            double angle = root.angle;
            if (angle < 0) angle += 2 * M_PI;
            angles.push_back(angle);
        }
        std::sort(angles.begin(), angles.end());
        double pickAngle = std::atan2(pick.y - center.y, pick.x - center.x);
        if (pickAngle < 0) pickAngle += 2 * M_PI;
        for (std::size_t i = 0; i < angles.size(); ++i) {
            const double start = angles[i];
            const double end = i + 1 < angles.size() ? angles[i + 1] : angles[0] + 2 * M_PI;
            const double selected = pickAngle < start ? pickAngle + 2 * M_PI : pickAngle;
            if (selected > start && selected < end) {
                result.selectedArcSweep = end - start;
                result.selectedArcMargin = std::min(selected - start, end - selected);
                break;
            }
        }
    }
    result.valid = true;
    return result;
}
}

class LC_OffsetTrimWorkerTestAccess {
public:
    static void deadline(LC_OffsetTrimWorker& worker, qint64 value) { worker.m_commitLimit = value; }
    static bool clientBusy(const LC_OffsetTrimWorker& worker) { return worker.m_client.busy(); }
    static bool processRunning(const LC_OffsetTrimWorker& worker) {
        const auto& process = worker.m_client.m_process;
        return process && process->state() == QProcess::Running && process->processId() > 0;
    }
    static void previewDeadline(LC_OffsetTrimWorker& worker, qint64 value) { worker.m_previewLimit = value; }
    static void onStarted(LC_OffsetTrimWorker& worker, std::function<void(QProcess&)> hook) {
        worker.m_client.m_testStarted = std::move(hook);
    }
};

class LC_ActionModifyTrimTestAccess {
public:
    static LC_OffsetTrimWorker& worker(LC_ActionModifyTrim& action) { return *action.nativeTrimWorkerForTest(); }
};

namespace {
class Messages final : public LC_ActionContext {
public:
    QStringList messages;
    void commandMessage(const QString& s) override { messages << s; }
};
template<bool Both> class CacheTrim final : public LC_ActionModifyTrim {
public:
    explicit CacheTrim(LC_ActionContext* c) : LC_ActionModifyTrim(c, Both) {}
    LC_StoredCacheTrimOptions options;
    mutable std::function<void()> onOptionsRead;
    using LC_ActionModifyTrim::ChooseLimitEntity;
    using LC_ActionModifyTrim::ChooseTrimEntity;
    using RS_PreviewActionInterface::m_preview;
    void contextTarget(RS_Entity* e, RS_Vector p) { doInitWithContextEntity(e, p); }
    void guardedHoverAsync(RS_Vector p) { auto e = lc::test::eventAt(p.x, p.y); onMouseMoveEvent(getStatus(), &e); }
    void guardedHover(RS_Vector p) { guardedHoverAsync(p); waitForTrim(); }
    void cancel() { auto e = lc::test::eventAt(0, 0); onMouseRightButtonRelease(getStatus(), &e); }
    RS_Entity* retainedLimit() const { return m_limitEntity; }
    void stalePublicEvents() {
        QMouseEvent move(QEvent::MouseMove, QPointF{1, 1}, QPointF{1, 1}, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        mouseMoveEvent(&move);
        QMouseEvent press(QEvent::MouseButtonPress, QPointF{1, 1}, QPointF{1, 1}, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        mousePressEvent(&press);
        QMouseEvent release(QEvent::MouseButtonRelease, QPointF{1, 1}, QPointF{1, 1}, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        mouseReleaseEvent(&release);
        QMouseEvent cancel(QEvent::MouseButtonRelease, QPointF{1, 1}, QPointF{1, 1}, Qt::RightButton, Qt::NoButton, Qt::NoModifier);
        mouseReleaseEvent(&cancel);
    }
    bool pending() const { return hasPendingNativeTrim(); }
    void togglePreviewRefs() { m_showRefEntitiesOnPreview = !m_showRefEntitiesOnPreview; }
    void waitForTrim() {
        QElapsedTimer elapsed; elapsed.start();
        while (pending()) {
            REQUIRE(elapsed.elapsed() < 35000);
            QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1);
        }
    }
    void clickAsync(RS_Vector p) { auto e = lc::test::eventAt(p.x, p.y); onMouseLeftButtonRelease(getStatus(), &e); }
    void click(RS_Vector p) { clickAsync(p); waitForTrim(); }
    RS_Entity* caughtAt(RS_Vector p) const {
        const auto e = lc::test::eventAt(p.x, p.y);
        return catchEntityByEvent(&e, RS2::ResolveNone);
    }
    void hoverAsync(RS_Vector p) {
        deletePreviewAndHighlights(); auto e = lc::test::eventAt(p.x, p.y);
        onMouseMoveEvent(getStatus(), &e); drawPreviewAndHighlights();
    }
    void hover(RS_Vector p) { hoverAsync(p); waitForTrim(); }
protected:
    LC_StoredCacheTrimOptions storedCacheTrimOptions() const override {
        const auto value = options;
        if (onOptionsRead) { auto callback = std::move(onOptionsRead); callback(); }
        return value;
    }
};
class UpdateSourceProbe final : public LC_ActionOffsetCurveUpdateSource {
public:
    using LC_ActionOffsetCurveUpdateSource::LC_ActionOffsetCurveUpdateSource;
    void use(RS_Entity* source) { useSource(source); }
};
template<bool Both> struct Drawing {
    const bool ready = lc::test::application() != nullptr;
    RS_Graphic graphic;
    lc::test::TestGraphicView view;
    Messages context;
    std::unique_ptr<CacheTrim<Both>> action;
    Drawing() {
        graphic.initForNewDocument(); view.setDocument(&graphic); context.setDocumentAndView(&graphic, &view);
        action = std::make_unique<CacheTrim<Both>>(&context);
    }
};
LC_OffsetCurve* addNative(RS_Graphic& graphic, double dx = 0) {
    RS_SplineData data{3, false};
    std::array<RS_Vector, 4> net{{{2, 0}, {2, 9./8}, {9./8, 2}, {0, 2}}};
    for (unsigned q = 0; q < 4; ++q) {
        for (unsigned i = q ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i] + RS_Vector{dx, 0});
        for (auto& p : net) p = {-p.y, p.x};
    }
    for (unsigned q = 0; q <= 4; ++q) for (unsigned i = 0; i < (q == 0 || q == 4 ? 4u : 3u); ++i) data.knotslist.push_back(q);
    data.weights.assign(data.controlPoints.size(), 1); data.m_closedFlag = true;
    RS_Spline cache{&graphic, data}; LC_EllipseOffsetDefinition def;
    def.basis = {{dx, 0}, {1, 0}, 1}; def.distance = 1; def.relTolerance = .01;
    LC_OffsetCheckResult check; auto native = LC_OffsetCurve::adopt(cache, def, .02, &check);
    REQUIRE(check); REQUIRE(native);
    REQUIRE(native->setOwnedExtensions(0x10, {{1000, QStringLiteral("owned trim tail")}}));
    native->setUserDefVar("trim-foreign", "keep"); native->setOwnerLineage(67);
    native->setSourceHandle(101); native->setXDictHandle(102); native->setReactorHandles({103});
    native->setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("TRIM_FOREIGN")),
        std::make_shared<DRW_Variant>(1000, std::string("keep exactly"))});
    auto* out = native.release(); graphic.addEntity(out); return out;
}
std::array<LC_OffsetCurve*, 2> addTrimmedProducerNatives(RS_Graphic& graphic, bool reverse) {
    const double pi = std::acos(-1.0);
    const RS_EllipseData basis{{0, 0}, {100, 0}, .05, pi / 12, 7 * pi / 4, false};
    const RS_Ellipse source{nullptr, basis};
    auto options = LC_CurveOffset::makeDirectOptions(source, 4, 1e-4);
    options.mode = LC_CurveOffsetMode::Trimmed;
    auto made = LC_CurveOffset::createEntities(source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 4), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(made.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(made.entities.size() == 2);
    std::array<LC_OffsetCurve*, 2> result{};
    for (const auto& entity : made.entities) {
        const auto* spline = dynamic_cast<const RS_Spline*>(entity.get());
        REQUIRE(spline != nullptr);
        auto data = spline->getData();
        REQUIRE_FALSE(data.controlPoints.empty());
        const bool closed = data.controlPoints.front().x == data.controlPoints.back().x &&
                            data.controlPoints.front().y == data.controlPoints.back().y;
        const std::size_t component = closed ? 1 : 0;
        REQUIRE(result[component] == nullptr);
        if (reverse) std::reverse(data.controlPoints.begin(), data.controlPoints.end());
        LC_EllipseOffsetDefinition definition;
        definition.basis = basis; definition.distance = 4; definition.towardCentre = true;
        definition.relTolerance = .01; definition.cacheReversed = reverse;
        RS_Spline candidate{nullptr, data};
        LC_OffsetCheckResult check;
        auto native = LC_OffsetCurve::adopt(candidate, definition, .001, &check);
        CAPTURE(component, reverse, check.reason);
        REQUIRE(check); REQUIRE(native); REQUIRE(native->hasEditableDefinition());
        native->setLayer(graphic.getActiveLayer());
        result[component] = native.release();
        graphic.addEntity(result[component]);
    }
    REQUIRE(result[0] != nullptr); REQUIRE(result[1] != nullptr);
    return result;
}
RS_Vector pickBetweenVerticalContacts(const RS_Spline& spline, double x) {
    double t0 = 0, t1 = 0;
    REQUIRE(spline.getParameterDomain(t0, t1));
    std::vector<double> breaks{t0};
    for (const double knot : spline.getData().knotslist) {
        if (knot > t0 && knot < t1 && knot > breaks.back()) breaks.push_back(knot);
    }
    breaks.push_back(t1);
    std::vector<double> roots;
    for (std::size_t i = 1; i < breaks.size(); ++i) {
        double lo = breaks[i - 1], hi = breaks[i];
        const double xLo = spline.getPointAt(lo).x, xHi = spline.getPointAt(hi).x;
        if ((xLo < x && xHi < x) || (xLo > x && xHi > x) || xLo == x || xHi == x) continue;
        const bool increasing = xLo < xHi;
        for (unsigned step = 0; step < 64; ++step) {
            const double mid = lo + (hi - lo) / 2;
            const double xMid = spline.getPointAt(mid).x;
            if ((xMid < x) == increasing) lo = mid; else hi = mid;
        }
        const double root = lo + (hi - lo) / 2;
        if (roots.empty() || root - roots.back() > 1e-10) roots.push_back(root);
    }
    REQUIRE_FALSE(roots.empty());
    const double pickParameter = roots.size() > 1 ? (roots[0] + roots[1]) / 2 : (t0 + roots[0]) / 2;
    const RS_Vector pick = spline.getPointAt(pickParameter);
    REQUIRE(pick.valid);
    return pick;
}
RS_Line* line(RS_Graphic& g, RS_Vector a = {-3, .3}, RS_Vector b = {3, .3}) {
    auto* e = new RS_Line(&g, a, b); g.addEntity(e); return e;
}
std::vector<RS_Entity*> live(RS_Graphic& g) {
    std::vector<RS_Entity*> out; for (auto* e : g) if (e && !e->isDeleted()) out.push_back(e); return out;
}
bool canUndo(RS_Graphic& g) { bool a = false, b = false; g.collectUndoState(a, b); return a; }
void plain(const RS_Entity& e, const LC_OffsetCurve& source) {
    REQUIRE(e.rtti() == RS2::EntitySpline); CHECK(e.getParent() == source.getParent());
    CHECK(e.getPen(false) == source.getPen(false)); CHECK(e.getLayer(false) == source.getLayer(false));
    CHECK(e.getUserDefVar("trim-foreign") == "keep"); CHECK(e.sourceHandle() == 0);
    CHECK(e.xDictHandle() == 0); CHECK(e.reactorHandles().empty()); CHECK(e.ownerLineage() == 0);
    CHECK(e.lineage() != source.lineage());
    const auto parsed = LC_OffsetCurveXData::parseAndStrip(e.getDrwExtData());
    REQUIRE(parsed.remaining); CHECK(parsed.knownCandidates == 0); REQUIRE(e.getDrwExtData().size() == 2);
    REQUIRE(e.getDrwExtData()[1]->type() == DRW_Variant::STRING);
    CHECK(*e.getDrwExtData()[1]->content.s == "keep exactly");
}
}

TEST_CASE("T actions trim a native owner rather than its generated chord with one undo", "[lc-offsetcurve][trim-action]") {
    Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = line(f.graphic);
    auto* layer = new RS_Layer("TrimOwner"); f.graphic.addLayer(layer); source->setLayer(layer);
    source->setPen(RS_Pen{RS_Color{20, 80, 120}, RS2::Width07, RS2::DashLine});
    const auto stamp = RS_Spline::geometryFingerprint(source->getData());
    f.action->click({-2.8, .3}); REQUIRE(f.action->getStatus() == CacheTrim<false>::ChooseTrimEntity);
    f.action->hover({2, 0}); CHECK_FALSE(source->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    REQUIRE(f.action->m_preview->count() > 0);
    std::vector<std::uint64_t> preview;
    for (const auto* e : *f.action->m_preview) if (const auto* spline = dynamic_cast<const RS_Spline*>(e)) preview.push_back(RS_Spline::geometryFingerprint(spline->getData()));
    f.action->click({2, 0}); REQUIRE(source->isDeleted()); CHECK_FALSE(limit->isDeleted());
    const auto entities = live(f.graphic); REQUIRE(entities.size() == 2);
    auto* fragment = entities[0] == limit ? entities[1] : entities[0]; plain(*fragment, *source);
    REQUIRE(preview.size() == 1); CHECK(preview.front() == RS_Spline::geometryFingerprint(static_cast<RS_Spline*>(fragment)->getData()));
    REQUIRE(f.context.messages.size() == 1); CHECK(f.context.messages.front().contains("ordinary splines"));
    REQUIRE(f.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK(fragment->isDeleted());
    CHECK(RS_Spline::geometryFingerprint(source->getData()) == stamp); CHECK(source->hasEditableDefinition());
    CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo()); CHECK(source->isDeleted()); CHECK_FALSE(fragment->isDeleted());
}

TEST_CASE("T actions use a native as an unchanged certified limit", "[lc-offsetcurve][trim-action]") {
    Drawing<false> f; auto* limit = addNative(f.graphic); auto* target = line(f.graphic);
    const auto stamp = RS_Spline::geometryFingerprint(limit->getData());
    f.action->click({2, 0}); f.action->click({0, .3});
    REQUIRE(target->isDeleted()); CHECK_FALSE(limit->isDeleted()); CHECK(limit->hasEditableDefinition());
    CHECK(RS_Spline::geometryFingerprint(limit->getData()) == stamp);
    const auto entities = live(f.graphic); REQUIRE(entities.size() == 3);
    for (const auto* e : entities) if (e != limit) CHECK(e->rtti() == RS2::EntityLine);
    CHECK(f.context.messages.isEmpty());
}

TEST_CASE("T actions prepare mixed Trim Two in both mouse pick orders", "[lc-offsetcurve][trim-action]") {
    for (bool firstNative : {false, true}) {
        Drawing<true> f; auto* source = addNative(f.graphic); auto* ordinary = line(f.graphic);
        f.action->click(firstNative ? RS_Vector{2, 0} : RS_Vector{0, .3});
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        f.action->click(firstNative ? RS_Vector{0, .3} : RS_Vector{2, 0});
        REQUIRE(source->isDeleted()); REQUIRE(ordinary->isDeleted());
        const auto entities = live(f.graphic); REQUIRE(entities.size() == 3);
        CHECK(std::count_if(entities.begin(), entities.end(), [](const auto* e) { return e->rtti() == RS2::EntitySpline; }) == 1);
        REQUIRE(f.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK_FALSE(ordinary->isDeleted());
        for (const auto* e : entities) CHECK(e->isDeleted());
        CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo());
    }
}

TEST_CASE("T actions stage two native sources before either is replaced", "[lc-offsetcurve][trim-action]") {
    Drawing<true> f; auto* a = addNative(f.graphic); auto* b = addNative(f.graphic, 1);
    f.action->click({-2, 0}); f.action->click({3, 0});
    REQUIRE(a->isDeleted()); REQUIRE(b->isDeleted()); const auto entities = live(f.graphic); REQUIRE(entities.size() == 2);
    for (const auto* e : entities) CHECK(e->rtti() == RS2::EntitySpline);
    REQUIRE(f.graphic.undo()); CHECK_FALSE(a->isDeleted()); CHECK_FALSE(b->isDeleted()); CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("Stored cache Trim output admission has one bounded caller-lowerable limit",
          "[lc-offsetcurve][trim-action][trim-output-limit]") {
    LC_StoredCacheTrimOptions options;
    CHECK(options.admittedOutputEntities() == 16);
    options.maxOutputEntities = 16;
    CHECK(options.admittedOutputEntities() == 16);
    options.maxOutputEntities = 17;
    CHECK(options.admittedOutputEntities() == 16);
    options.maxOutputEntities = 4;
    CHECK(options.admittedOutputEntities() == 4);
    options.maxOutputEntities = 0;
    CHECK(options.admittedOutputEntities() == 0);
}

TEST_CASE("T actions preserve both sources and selection on cancellation or exhausted second-side budget", "[lc-offsetcurve][trim-action]") {
    for (bool cancel : {false, true}) {
        Drawing<true> f; auto* a = addNative(f.graphic); auto* b = line(f.graphic);
        RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {a, b}, true);
        const auto before = f.graphic.count();
        if (cancel) f.action->options.cancellation.requested = std::make_shared<std::atomic_bool>(true);
        else f.action->options.maxOutputEntities = 1;
        f.action->click({2, 0}); f.action->click({0, .3});
        CHECK_FALSE(a->isDeleted()); CHECK_FALSE(b->isDeleted()); CHECK(a->isSelected()); CHECK(b->isSelected());
        CHECK(f.graphic.count() == before); CHECK_FALSE(canUndo(f.graphic));
        REQUIRE(f.context.messages.size() == 1); CHECK(f.context.messages.front().contains("nothing changed"));
    }
}

TEST_CASE("T actions refuse locked hidden stale and foreign-owned native replacements", "[lc-offsetcurve][trim-action]") {
    for (int defect = 0; defect < 4; ++defect) {
        Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = line(f.graphic);
        LC_DocumentModificationBatch batch;
        if (defect == 0) { auto* layer = new RS_Layer("Locked"); layer->lock(true); f.graphic.addLayer(layer); source->setLayer(layer); }
        if (defect == 1) source->setVisible(false);
        if (defect == 2) source->getData().controlPoints[1].x += .01;
        RS_EntityContainer foreign{nullptr, false}; if (defect == 3) source->setParent(&foreign);
        const auto result = RS_Modification::trimStoredCache(f.graphic, *source, {2, 0}, *limit, {0, .3}, false, batch);
        CHECK_FALSE(result.result); CHECK(batch.entitiesToAdd.empty()); CHECK(batch.entitiesToDelete.empty());
        CHECK_FALSE(source->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
        if (defect == 3) source->setParent(&f.graphic);
    }
}

TEST_CASE("T actions refuse a root pick and a display cap before publication", "[lc-offsetcurve][trim-action]") {
    for (bool cap : {false, true}) {
        Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = line(f.graphic, {0, -3}, {0, 3});
        if (cap) f.action->options.maxDisplayEntities = 0;
        f.action->contextTarget(source, cap ? RS_Vector{2, 0} : RS_Vector{0, 2});
        f.action->click({0, -2.8});
        CHECK_FALSE(source->isDeleted()); CHECK_FALSE(limit->isDeleted());
        CHECK_FALSE(canUndo(f.graphic)); REQUIRE(f.context.messages.size() == 1);
        CHECK(f.context.messages.front().contains("nothing changed"));
    }
}

TEST_CASE("T actions resolve an open native child and retain two ordinary fragments", "[lc-offsetcurve][trim-action]") {
    Drawing<false> f;
    const RS_Vector a{2, 0}, b{2 * std::cos(.01), 2 * std::sin(.01)};
    RS_SplineData data{1, false}; data.controlPoints = {a, b}; data.weights = {1, 1}; data.knotslist = {0, 0, 1, 1};
    RS_Spline cache{&f.graphic, data}; LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1, 0, .01, false}; definition.distance = 1; definition.relTolerance = .01;
    LC_OffsetCheckResult check; auto native = LC_OffsetCurve::adopt(cache, definition, .02, &check);
    REQUIRE(check); REQUIRE(native); auto* source = native.release(); f.graphic.addEntity(source);
    const double low = b.y / 4, high = 3 * b.y / 4;
    RS_SplineData u{1, false}; u.controlPoints = {{1, low}, {3, low}, {3, high}, {1, high}};
    u.weights.assign(4, 1); u.knotslist = {0, 0, 1, 2, 3, 3};
    auto* limit = new RS_Spline(&f.graphic, u); f.graphic.addEntity(limit);
    REQUIRE(source->size() > 0); f.action->contextTarget(source->entityAt(0), (a + b) * .5);
    f.action->click({3, b.y * .5}); REQUIRE(source->isDeleted()); CHECK_FALSE(limit->isDeleted());
    const auto entities = live(f.graphic); REQUIRE(entities.size() == 3);
    std::vector<RS_Spline*> fragments;
    for (auto* e : entities) if (e != limit) { REQUIRE(e->rtti() == RS2::EntitySpline); fragments.push_back(static_cast<RS_Spline*>(e)); }
    REQUIRE(fragments.size() == 2); CHECK(fragments[0]->getData().controlPoints.front() == a);
    CHECK(fragments[1]->getData().controlPoints.back() == b);
    CHECK(std::abs(fragments[0]->getData().controlPoints.back().y - low) < 1e-11);
    CHECK(std::abs(fragments[1]->getData().controlPoints.front().y - high) < 1e-11);
    REQUIRE(f.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK_FALSE(f.graphic.undo());
}

namespace {
template<bool Both> void removedLimit(bool hover) {
    Drawing<Both> f; auto* source = addNative(f.graphic); auto* target = line(f.graphic);
    f.action->click({2, 0}); REQUIRE(f.action->getStatus() == CacheTrim<Both>::ChooseTrimEntity);
    REQUIRE(f.graphic.removeEntity(source)); // Actually frees the captured native address.
    const auto count = f.graphic.count();
    std::ostringstream before; before << static_cast<const RS_Undo&>(f.graphic);
    if (hover) f.action->hover({0, .3}); else f.action->click({0, .3});
    CHECK_FALSE(target->isDeleted()); CHECK(f.graphic.count() == count); CHECK_FALSE(canUndo(f.graphic));
    CHECK(f.action->getStatus() == CacheTrim<Both>::ChooseLimitEntity); CHECK(f.action->m_preview->isEmpty());
    std::ostringstream after; after << static_cast<const RS_Undo&>(f.graphic); CHECK(after.str() == before.str());
    REQUIRE(f.context.messages.size() == 1); CHECK(f.context.messages.front().contains("stored source"));
}
}

TEST_CASE("T actions reject a freed retained native limit before hover or commit getters",
          "[lc-offsetcurve][trim-action][retained-owner][review-fix]") {
    for (bool hover : {false, true}) { removedLimit<false>(hover); removedLimit<true>(hover); }
}

TEST_CASE("T actions reject a freed context native target before picking a limit",
          "[lc-offsetcurve][trim-action][retained-owner][review-fix]") {
    for (bool hover : {false, true}) {
        Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = line(f.graphic);
        f.action->contextTarget(source, {2, 0}); REQUIRE(f.graphic.removeEntity(source));
        const auto count = f.graphic.count();
        if (hover) f.action->hover({-2.8, .3}); else f.action->click({-2.8, .3});
        CHECK_FALSE(limit->isDeleted()); CHECK(f.graphic.count() == count); CHECK_FALSE(canUndo(f.graphic));
        CHECK(f.action->m_preview->isEmpty()); REQUIRE(f.context.messages.size() == 1);
        CHECK(f.context.messages.front().contains("stored source"));
    }
}

namespace {
std::string trimGeometry(const RS_Entity& entity) {
    std::ostringstream out; out << std::hexfloat << int(entity.rtti()) << ':';
    const auto vector = [&](const RS_Vector& p) { out << p.x << ',' << p.y << ',' << p.z << ';'; };
    if (const auto* spline = dynamic_cast<const RS_Spline*>(&entity)) {
        const auto& d = spline->getData(); out << d.degree << ':' << int(d.type) << ':' << d.m_closedFlag << d.m_periodicFlag;
        for (const auto& p : d.controlPoints) vector(p);
        for (double x : d.knotslist) out << x << ';';
        for (double x : d.weights) out << x << ';';
    } else if (const auto* arc = dynamic_cast<const RS_Arc*>(&entity)) {
        const auto& d = arc->getData(); vector(d.center); out << d.radius << ':' << d.angle1 << ':' << d.angle2 << ':' << d.reversed;
    } else if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&entity)) {
        const auto& d = ellipse->getData(); vector(d.center); vector(d.majorP); out << d.ratio << ':' << d.angle1 << ':' << d.angle2 << ':' << d.reversed;
    } else if (const auto* line = dynamic_cast<const RS_Line*>(&entity)) {
        const auto& d = line->getData(); vector(d.startpoint); vector(d.endpoint);
    } else if (const auto* polyline = dynamic_cast<const RS_Polyline*>(&entity)) {
        out << polyline->isClosed() << ':';
        for (const auto* child : *polyline) out << trimGeometry(*child);
    }
    return out.str();
}
bool sameTrimGeometry(const RS_Entity& expected, const RS_Entity& actual) {
    const double scale = std::max({1.0, expected.getLength(), actual.getLength()});
    const double tolerance = 2e-8 * scale;
    const auto ulp = [](double value) {
        double spacing = 0.0;
        for (const double direction : {-INFINITY, INFINITY}) {
            const double adjacent = std::nextafter(value, direction);
            if (std::isfinite(adjacent))
                spacing = std::max(spacing, std::abs(adjacent - value));
        }
        return spacing;
    };
    // DXF readers may round world endpoints by a few ULPs at large origins.
    const auto pointTolerance = [tolerance, &ulp](const RS_Vector& a, const RS_Vector& b) {
        const double coordinateUlp = 4.0 * std::hypot(
            std::max(ulp(a.x), ulp(b.x)), std::max(ulp(a.y), ulp(b.y)));
        return std::max(tolerance, coordinateUlp);
    };
    const auto closePoint = [&pointTolerance](const RS_Vector& a, const RS_Vector& b) {
        return a.distanceTo(b) <= pointTolerance(a, b);
    };
    const double lengthTolerance = std::max(tolerance,
        pointTolerance(expected.getStartpoint(), actual.getStartpoint()) +
        pointTolerance(expected.getEndpoint(), actual.getEndpoint()));
    if (expected.rtti() != actual.rtti() ||
        std::abs(expected.getLength() - actual.getLength()) > lengthTolerance) return false;
    if (const auto* line = dynamic_cast<const RS_Line*>(&expected)) {
        const auto* other = dynamic_cast<const RS_Line*>(&actual);
        if (!other) return false;
        const auto& a = line->getData();
        const auto& b = other->getData();
        return (closePoint(a.startpoint, b.startpoint) && closePoint(a.endpoint, b.endpoint)) ||
               (closePoint(a.startpoint, b.endpoint) && closePoint(a.endpoint, b.startpoint));
    }
    if (const auto* polyline = dynamic_cast<const RS_Polyline*>(&expected)) {
        const auto* other = dynamic_cast<const RS_Polyline*>(&actual);
        if (!other || polyline->isClosed() != other->isClosed() || polyline->size() != other->size()) return false;
        for (qsizetype i = 0; i < polyline->size(); ++i) {
            if (!sameTrimGeometry(*polyline->entityAt(i), *other->entityAt(i))) return false;
        }
        return true;
    }
    if (const auto* spline = dynamic_cast<const RS_Spline*>(&expected)) {
        const auto* other = dynamic_cast<const RS_Spline*>(&actual);
        if (!other) return false;
        double a0 = 0.0, a1 = 0.0, b0 = 0.0, b1 = 0.0;
        if (!spline->getParameterDomain(a0, a1) || !other->getParameterDomain(b0, b1) ||
            !(a1 > a0) || !(b1 > b0)) return false;
        for (unsigned i = 0; i <= 128; ++i) {
            const double fraction = static_cast<double>(i) / 128.0;
            LC_CurveJet a, b;
            const auto side = i == 128 ? LC_CurveEvaluationSide::Left : LC_CurveEvaluationSide::Right;
            if (!spline->tryEvaluateJet(a0 + (a1 - a0) * fraction, side, a) ||
                !other->tryEvaluateJet(b0 + (b1 - b0) * fraction, side, b) ||
                !closePoint(a.point, b.point)) return false;
        }
        return true;
    }
    return closePoint(expected.getStartpoint(), actual.getStartpoint()) &&
           closePoint(expected.getEndpoint(), actual.getEndpoint());
}
template <typename ExpectedRange, typename ActualRange>
bool sameTrimGeometrySet(const ExpectedRange& expected, const ActualRange& actual) {
    if (expected.size() != actual.size()) return false;
    std::vector<bool> matched(actual.size(), false);
    for (const auto* entity : expected) {
        std::size_t candidate = 0;
        while (candidate < actual.size() &&
               (matched[candidate] || !sameTrimGeometry(*entity, *actual[candidate]))) {
            ++candidate;
        }
        if (candidate == actual.size()) return false;
        matched[candidate] = true;
    }
    return true;
}
std::vector<std::string> trimPreview(const RS_EntityContainer& container) {
    std::vector<std::string> out;
    for (const auto* e : container) if (e && (e->rtti() == RS2::EntitySpline || e->rtti() == RS2::EntityArc || e->rtti() == RS2::EntityEllipse))
        out.push_back(trimGeometry(*e));
    return out;
}
}

// Mandatory section 11.3 positive gates. Authored only: no build or execution
// is implied by completing the typed staging path; refusal/skip is not success.
TEST_CASE("T actions must replace ordinary conics with a native in both Trim Two pick orders",
          "[lc-offsetcurve][trim-action][mandatory-conic]") {
    for (unsigned kind = 0; kind < 4; ++kind) for (bool nativeFirst : {false, true}) {
        Drawing<true> f; auto* source = addNative(f.graphic);
        std::unique_ptr<RS_Entity> ordinary;
        if (kind == 0) ordinary = std::make_unique<RS_Circle>(nullptr, RS_CircleData{{1, 0}, 2});
        if (kind == 1) ordinary = std::make_unique<RS_Arc>(nullptr, RS_ArcData{{1, 0}, 2, .5, 5.8, false});
        if (kind >= 2) ordinary = std::make_unique<RS_Ellipse>(nullptr,
            RS_EllipseData{{1, 0}, {2, 0}, .75, kind == 2 ? 0. : .5, kind == 2 ? 0. : 5.8, false});
        auto* conic = ordinary.release(); conic->setLayer(f.graphic.getActiveLayer()); f.graphic.addEntity(conic);
        conic->setUserDefVar("ordinary-trim", "retained");
        conic->setOwnerLineage(267); conic->setSourceHandle(301); conic->setXDictHandle(302); conic->setReactorHandles({303});
        conic->setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("CONIC_FOREIGN")),
            std::make_shared<DRW_Variant>(1000, std::string("retain exactly"))});
        const auto conicTags = conic->getDrwExtData();
        const RS_Vector nativePick{2, 0}, conicPick{-1, 0};
        f.action->click(nativeFirst ? nativePick : conicPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        f.action->hover(nativeFirst ? conicPick : nativePick);
        REQUIRE_FALSE(f.action->m_preview->isEmpty());
        const auto preview = trimPreview(*f.action->m_preview);
        CHECK_FALSE(source->isDeleted()); CHECK_FALSE(conic->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
        f.action->click(nativeFirst ? conicPick : nativePick);
        REQUIRE(source->isDeleted()); REQUIRE(conic->isDeleted());
        const auto outputs = live(f.graphic); REQUIRE(outputs.size() >= 2); REQUIRE(outputs.size() <= 4);
        std::vector<std::string> committed; for (const auto* e : outputs) committed.push_back(trimGeometry(*e));
        CHECK(committed == preview);
        std::size_t natives = 0, conics = 0;
        for (const auto* output : outputs) {
            REQUIRE(output->getParent() == &f.graphic); CHECK(output->isVisible());
            CHECK(dynamic_cast<const LC_OffsetCurve*>(output) == nullptr);
            if (output->rtti() == RS2::EntitySpline) { ++natives; plain(*output, *source); }
            else {
                ++conics; CHECK(output->rtti() == (kind < 2 ? RS2::EntityArc : RS2::EntityEllipse));
                CHECK(output->getUserDefVar("ordinary-trim") == "retained");
                CHECK(output->getPen(false) == conic->getPen(false)); CHECK(output->getLayer(false) == conic->getLayer(false));
                REQUIRE(output->getLength() > 0); CHECK(output->getLength() < conic->getLength());
                CHECK(output->ownerLineage() == 0); CHECK(output->sourceHandle() == 0);
                CHECK(output->xDictHandle() == 0); CHECK(output->reactorHandles().empty()); CHECK(output->lineage() != conic->lineage());
                CHECK(output->getDrwExtData() == conicTags);
                const auto parsed = LC_OffsetCurveXData::parseAndStrip(output->getDrwExtData());
                REQUIRE(parsed.remaining); CHECK(parsed.knownCandidates == 0);
            }
        }
        CHECK(natives == 1); CHECK(conics >= 1); REQUIRE(f.graphic.undo());
        CHECK(conic->getDrwExtData() == conicTags); CHECK(conic->ownerLineage() == 267);
        CHECK(conic->sourceHandle() == 301); CHECK(conic->xDictHandle() == 302); CHECK(conic->reactorHandles() == std::vector<quint32>{303});
        CHECK_FALSE(source->isDeleted()); CHECK_FALSE(conic->isDeleted()); CHECK(source->hasEditableDefinition());
        for (const auto* output : outputs) CHECK(output->isDeleted());
        CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo());
        CHECK(source->isDeleted()); CHECK(conic->isDeleted());
        for (const auto* output : outputs) CHECK_FALSE(output->isDeleted());
    }
}

TEST_CASE("T actions must retain a native limit while replacing an ordinary circle target",
          "[lc-offsetcurve][trim-action][mandatory-conic]") {
    Drawing<false> f; auto* limit = addNative(f.graphic);
    auto* circle = new RS_Circle(&f.graphic, RS_CircleData{{1, 0}, 2}); f.graphic.addEntity(circle);
    const auto fingerprint = RS_Spline::geometryFingerprint(limit->getData());
    const auto definition = limit->definition();
    f.action->click({2, 0}); f.action->click({-1, 0});
    REQUIRE(circle->isDeleted()); CHECK_FALSE(limit->isDeleted()); CHECK(limit->hasEditableDefinition());
    CHECK(RS_Spline::geometryFingerprint(limit->getData()) == fingerprint);
    CHECK(limit->definition().distance == definition.distance); CHECK(limit->unknownOwnedFlags() == 0x10);
    const auto outputs = live(f.graphic); REQUIRE(outputs.size() >= 2);
    for (const auto* output : outputs) if (output != limit) REQUIRE(output->rtti() == RS2::EntityArc);
    CHECK(f.context.messages.isEmpty()); REQUIRE(f.graphic.undo()); CHECK_FALSE(circle->isDeleted());
    CHECK_FALSE(limit->isDeleted()); CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo()); CHECK(circle->isDeleted());
}

TEST_CASE("Native Trim action publishes each admitted open and closed producer component",
          "[gui][lc-offsetcurve][trim-action][producer-components][trim-worker-preview]") {
    for (bool reverse : {false, true}) for (std::size_t component : {0u, 1u}) {
        CAPTURE(reverse, component);
        Drawing<false> f;
        auto natives = addTrimmedProducerNatives(f.graphic, reverse);
        auto* native = natives[component];
        auto* peer = natives[1u - component];
        const auto original = trimGeometry(*native);

        double minX = native->getData().controlPoints.front().x;
        double maxX = minX;
        double minY = native->getData().controlPoints.front().y;
        double maxY = minY;
        for (const auto& point : native->getData().controlPoints) {
            minX = std::min(minX, point.x); maxX = std::max(maxX, point.x);
            minY = std::min(minY, point.y); maxY = std::max(maxY, point.y);
        }
        REQUIRE(minX < maxX); REQUIRE(minY < maxY);
        const double cutX = minX + (maxX - minX) * (95.0 / 256.0);
        const double margin = std::max(1.0, maxY - minY);
        auto* cutter = line(f.graphic, {cutX, minY - margin}, {cutX, maxY + margin});
        const RS_Vector cutPick{cutX, (minY + maxY) / 2};
        const RS_Vector trimPick = pickBetweenVerticalContacts(*native, cutX);

        f.action->contextTarget(native, trimPick);
        f.action->hoverAsync(cutPick);
        REQUIRE(f.action->pending());
        f.action->waitForTrim();
        const auto preview = trimPreview(*f.action->m_preview);
        REQUIRE_FALSE(preview.empty());
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(peer->isDeleted());
        CHECK(trimGeometry(*native) == original); CHECK(peer->hasEditableDefinition());

        f.action->click(cutPick);
        REQUIRE(native->isDeleted()); CHECK_FALSE(peer->isDeleted());
        std::vector<std::string> committed;
        for (const auto* entity : live(f.graphic)) {
            if (entity == peer || entity == cutter) continue;
            CHECK(dynamic_cast<const LC_OffsetCurve*>(entity) == nullptr);
            committed.push_back(trimGeometry(*entity));
        }
        CHECK(committed == preview);
        REQUIRE(f.graphic.undo()); CHECK_FALSE(native->isDeleted()); CHECK_FALSE(peer->isDeleted());
        CHECK(trimGeometry(*native) == original); CHECK_FALSE(f.graphic.undo());
        REQUIRE(f.graphic.redo()); CHECK(native->isDeleted()); CHECK_FALSE(peer->isDeleted());
    }
}

TEST_CASE("T actions trim normalized ordinary spline targets with a native in both pick orders",
          "[lc-offsetcurve][trim-action][ordinary-spline]") {
    for (bool nativeFirst : {false, true}) {
        Drawing<true> f; auto* source = addNative(f.graphic);
        RS_SplineData data{1, false}; data.controlPoints = {{-3, .3}, {3, .3}};
        data.knotslist = {0, 0, 1, 1}; data.weights = {1, 1};
        auto* ordinary = new RS_Spline(&f.graphic, data); REQUIRE(ordinary->validate());
        ordinary->setUserDefVar("ordinary-trim", "spline"); f.graphic.addEntity(ordinary);
        f.action->click(nativeFirst ? RS_Vector{2, 0} : RS_Vector{0, .3});
        f.action->hover(nativeFirst ? RS_Vector{0, .3} : RS_Vector{2, 0});
        const auto preview = trimPreview(*f.action->m_preview); REQUIRE(preview.size() == 3);
        f.action->click(nativeFirst ? RS_Vector{0, .3} : RS_Vector{2, 0});
        REQUIRE(source->isDeleted()); REQUIRE(ordinary->isDeleted()); const auto outputs = live(f.graphic); REQUIRE(outputs.size() == 3);
        std::vector<std::string> committed; unsigned ordinaryPieces = 0;
        for (const auto* e : outputs) {
            REQUIRE(e->rtti() == RS2::EntitySpline); CHECK(dynamic_cast<const LC_OffsetCurve*>(e) == nullptr);
            committed.push_back(trimGeometry(*e));
            if (e->getUserDefVar("ordinary-trim") == "spline") ++ordinaryPieces;
        }
        CHECK(ordinaryPieces == 2); CHECK(committed == preview);
        REQUIRE(f.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK_FALSE(ordinary->isDeleted());
        CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo());
        CHECK(source->isDeleted()); CHECK(ordinary->isDeleted());
    }
}

TEST_CASE("T conic staging refusals preserve both sources selection redo metadata and an existing batch prefix",
          "[lc-offsetcurve][trim-action][conic-atomic-refusal]") {
    for (bool nativeFirst : {false, true}) for (unsigned defect = 0; defect < 6; ++defect) {
        Drawing<true> f; auto* native = addNative(f.graphic);
        auto* circle = new RS_Circle(&f.graphic, RS_CircleData{{1, 0}, 2}); f.graphic.addEntity(circle);
        circle->setUserDefVar("ordinary-trim", "keep"); circle->setSourceHandle(201); circle->setXDictHandle(202); circle->setReactorHandles({203});
        circle->setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("CONIC_FOREIGN")),
            std::make_shared<DRW_Variant>(1000, std::string("untouched"))});
        RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {native, circle}, true);
        auto* marker = line(f.graphic, {-5, -5}, {-4, -5});
        REQUIRE(f.graphic.undoableModify(f.view.getViewPort(), [&](LC_DocumentModificationBatch& b) { b.remove(marker); return true; }));
        REQUIRE(f.graphic.undo());
        const auto nativeShape = trimGeometry(*native), circleShape = trimGeometry(*circle);
        const auto nativeTags = native->getDrwExtData(), circleTags = circle->getDrwExtData();
        std::ostringstream before; before << static_cast<const RS_Undo&>(f.graphic);
        LC_StoredCacheTrimOptions options;
        if (defect == 0) options.maxOutputEntities = 1;
        if (defect == 1) options.maxDisplayEntities = 0;
        if (defect == 2) options.kernel.maxOutputSpans = 0;
        if (defect == 3) options.cancellation.requested = std::make_shared<std::atomic_bool>(true);
        if (defect == 4) options.kernel.maxRoots = 1;
        auto* locked = new RS_Layer("ConicReplacementLocked"); f.graphic.addLayer(locked);
        if (defect == 5) { circle->setLayer(locked); locked->lock(true); }
        const auto* circleLayer = circle->getLayer(false); const auto circlePen = circle->getPen(false);
        const auto* nativeLayer = native->getLayer(false); const auto nativePen = native->getPen(false);
        auto prefix = std::make_unique<RS_Line>(nullptr, RS_Vector{-8, -8}, RS_Vector{-7, -7});
        LC_DocumentModificationBatch batch; batch.add(prefix.get()); batch.remove(marker);
        const auto addBefore = batch.entitiesToAdd, deleteBefore = batch.entitiesToDelete;
        const auto result = RS_Modification::trimStoredCache(f.graphic,
            nativeFirst ? static_cast<RS_Entity&>(*native) : static_cast<RS_Entity&>(*circle),
            nativeFirst ? RS_Vector{2, 0} : RS_Vector{-1, 0},
            nativeFirst ? static_cast<RS_Entity&>(*circle) : static_cast<RS_Entity&>(*native),
            nativeFirst ? RS_Vector{-1, 0} : RS_Vector{2, 0}, true, batch, options);
        CHECK_FALSE(result.result); CHECK(batch.entitiesToAdd == addBefore); CHECK(batch.entitiesToDelete == deleteBefore);
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(circle->isDeleted()); CHECK(native->isSelected()); CHECK(circle->getFlag(RS2::FlagSelected));
        CHECK(trimGeometry(*native) == nativeShape); CHECK(trimGeometry(*circle) == circleShape);
        CHECK(native->getDrwExtData() == nativeTags); CHECK(circle->getDrwExtData() == circleTags);
        CHECK(circle->getLayer(false) == circleLayer); CHECK(circle->getPen(false) == circlePen);
        CHECK(native->getLayer(false) == nativeLayer); CHECK(native->getPen(false) == nativePen);
        CHECK(native->unknownOwnedFlags() == 0x10); CHECK(circle->getUserDefVar("ordinary-trim") == "keep");
        CHECK(circle->sourceHandle() == 201); CHECK(circle->xDictHandle() == 202); CHECK(circle->reactorHandles() == std::vector<quint32>{203});
        std::ostringstream after; after << static_cast<const RS_Undo&>(f.graphic); CHECK(after.str() == before.str());
        if (defect == 5) locked->lock(false);
        REQUIRE(f.graphic.redo()); CHECK(marker->isDeleted());
    }
}

TEST_CASE("T conic Trim Two action refuses before publishing either side in both pick orders",
          "[lc-offsetcurve][trim-action][conic-atomic-refusal]") {
    for (bool nativeFirst : {false, true}) {
        Drawing<true> f; auto* native = addNative(f.graphic);
        auto* circle = new RS_Circle(&f.graphic, RS_CircleData{{1, 0}, 2}); f.graphic.addEntity(circle);
        RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {native, circle}, true);
        f.action->options.maxOutputEntities = 1;
        f.action->click(nativeFirst ? RS_Vector{2, 0} : RS_Vector{-1, 0});
        f.action->hover(nativeFirst ? RS_Vector{-1, 0} : RS_Vector{2, 0}); CHECK(f.action->m_preview->isEmpty());
        f.action->click(nativeFirst ? RS_Vector{-1, 0} : RS_Vector{2, 0});
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(circle->isDeleted()); CHECK(native->isSelected()); CHECK(circle->isSelected());
        CHECK_FALSE(canUndo(f.graphic)); REQUIRE(f.context.messages.size() == 1); CHECK(f.context.messages.front().contains("nothing changed"));
    }
}

TEST_CASE("T conic Trim Two real mouse events share preview commit and exact one step history",
          "[gui][lc-offsetcurve][trim-action][conic-events]") {
    for (bool nativeFirst : {false, true}) {
        Drawing<true> f; auto* native = addNative(f.graphic);
        auto* ellipse = new RS_Ellipse(&f.graphic, {{1, 0}, {2, 0}, .75}); f.graphic.addEntity(ellipse);
        f.view.getViewPort()->setSize(640, 480); f.view.getViewPort()->setOffsetAndFactor(192, 240, 64);
        f.action->init(0);
        const auto nativeBefore = trimGeometry(*native), ellipseBefore = trimGeometry(*ellipse);
        const auto send = [&](QEvent::Type type, RS_Vector point) {
            const QPointF pos(f.view.getViewPort()->toGuiX(point.x), f.view.getViewPort()->toGuiY(point.y));
            QMouseEvent event(type, pos, pos, type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
            if (type == QEvent::MouseButtonPress) f.action->mousePressEvent(&event);
            else if (type == QEvent::MouseButtonRelease) { f.action->mouseReleaseEvent(&event); f.action->waitForTrim(); }
            else { f.action->mouseMoveEvent(&event); f.action->waitForTrim(); }
        };
        const RS_Vector first = nativeFirst ? RS_Vector{2, 0} : RS_Vector{-1, 0};
        const RS_Vector second = nativeFirst ? RS_Vector{-1, 0} : RS_Vector{2, 0};
        send(QEvent::MouseButtonPress, first); send(QEvent::MouseButtonRelease, first);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        send(QEvent::MouseMove, second); const auto preview = trimPreview(*f.action->m_preview); REQUIRE_FALSE(preview.empty());
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ellipse->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
        send(QEvent::MouseButtonPress, second); send(QEvent::MouseButtonRelease, second);
        REQUIRE(native->isDeleted()); REQUIRE(ellipse->isDeleted()); const auto outputs = live(f.graphic);
        std::vector<std::string> committed; for (const auto* output : outputs) committed.push_back(trimGeometry(*output));
        CHECK(committed == preview); REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ellipse->isDeleted()); CHECK(native->hasEditableDefinition());
        CHECK(trimGeometry(*native) == nativeBefore); CHECK(trimGeometry(*ellipse) == ellipseBefore);
        CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo());
        for (const auto* output : outputs) CHECK_FALSE(output->isDeleted());
    }
}

TEST_CASE("T native pairing explicitly refuses disabled through points spline trimming without a no op clone",
          "[lc-offsetcurve][trim-action][ordinary-spline]") {
    Drawing<true> f; auto* native = addNative(f.graphic);
    LC_SplinePointsData data; data.useControlPoints = true; data.controlPoints = {{-3, .3}, {0, 1}, {3, .3}};
    auto* points = new LC_SplinePoints(&f.graphic, data); f.graphic.addEntity(points);
    const auto* controls = points->getData().controlPoints.data();
    LC_DocumentModificationBatch batch;
    const auto result = RS_Modification::trimStoredCache(f.graphic, *native, {2, 0}, *points, {0, 1}, true, batch);
    CHECK_FALSE(result.result); REQUIRE(result.cacheRefusal); CHECK(*result.cacheRefusal == LC_OffsetCacheTrim::Refusal::UnsupportedLayout);
    CHECK(batch.entitiesToAdd.empty()); CHECK(batch.entitiesToDelete.empty());
    CHECK_FALSE(native->isDeleted()); CHECK_FALSE(points->isDeleted()); CHECK(native->hasEditableDefinition());
    CHECK(points->getData().controlPoints.data() == controls); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("T native Trim action previews commits and undoes a worker-created typed hyperbola offset",
          "[gui][lc-offsetcurve][trim-action][typed-source][hyperbola]") {
    Drawing<false> f;
    auto* previous = addNative(f.graphic);
    LC_HyperbolaData hyperbolaData{{0, 0}, {3, 0}, 1.0, -1.5, 1.5, false};
    auto* hyperbola = new LC_Hyperbola(&f.graphic, hyperbolaData);
    f.graphic.addEntity(hyperbola);
    f.graphic.select(QList<RS_Entity*>{previous}, true);

    UpdateSourceProbe update(&f.context);
    update.init(0);
    update.setSourceKind(LC_OffsetSourceGeometry::Kind::BoundedHyperbola);
    update.use(hyperbola);
    lc::test::awaitUpdate(update);
    REQUIRE(update.lastReport().updated == 1);
    REQUIRE(previous->isDeleted());
    LC_OffsetCurve* native = nullptr;
    for (auto* entity : live(f.graphic)) {
        if (entity->rtti() != RS2::EntityOffsetCurve) continue;
        REQUIRE(native == nullptr);
        native = dynamic_cast<LC_OffsetCurve*>(entity);
    }
    REQUIRE(native);
    REQUIRE(native->hasGeneralizedSourceDefinition());
    REQUIRE(native->sourceDefinition());
    CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::BoundedHyperbola);
    const auto original = trimGeometry(*native);

    double first = 0, last = 0;
    REQUIRE(native->getParameterDomain(first, last));
    const auto breaks = native->getBreakParameters();
    REQUIRE(breaks.size() >= 2);
    const double parameter = breaks.front() + (breaks[1] - breaks.front()) * .43;
    const RS_Vector limitPick = native->getPointAt(breaks.front() + (breaks[1] - breaks.front()) * .9);
    const RS_Vector point = native->getPointAt(parameter);
    const double step = std::max(1e-7, (last - first) * 1e-5);
    const RS_Vector before = native->getPointAt(parameter - step);
    const RS_Vector after = native->getPointAt(parameter + step);
    RS_Vector normal{-(after.y - before.y), after.x - before.x};
    REQUIRE(normal.magnitude() > RS_TOLERANCE);
    normal /= normal.magnitude();
    auto* cutter = line(f.graphic, point - normal * 20.0, point + normal * 20.0);

    const RS_Vector trimPick = point + normal * 10.0;
    f.action->contextTarget(native, limitPick);
    const auto previewUndoEpoch = f.graphic.undoStateEpoch();
    f.action->hoverAsync(trimPick);
    REQUIRE(f.action->pending());
    f.action->waitForTrim();
    const auto preview = trimPreview(*f.action->m_preview);
    REQUIRE_FALSE(preview.empty());
    CHECK_FALSE(native->isDeleted());
    CHECK(trimGeometry(*native) == original);
    CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);
    f.action->click(trimPick);
    REQUIRE(native->isDeleted());
    const auto outputs = live(f.graphic);
    std::vector<RS_Entity*> fragments;
    std::vector<std::string> committed;
    for (auto* output : outputs) {
        if (output == cutter || output == hyperbola) continue;
        REQUIRE(output->rtti() != RS2::EntityOffsetCurve);
        fragments.push_back(output);
        committed.push_back(trimGeometry(*output));
    }
    REQUIRE_FALSE(fragments.empty());
    CHECK(committed == preview);

    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
    const auto expectedFileGeometry = live(f.graphic);
    for (std::size_t i = 0; i < formats.size(); ++i) {
        const bool dwg = formats[i] == RS2::FormatDWG || formats[i] == RS2::FormatDWG2018;
        const auto file = std::filesystem::path(directory.filePath(
            QStringLiteral("typed-hyperbola-trim-%1").arg(i) + (dwg ? ".dwg" : ".dxf")).toStdString());
        REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[i]));
        RS_Graphic reopened;
        REQUIRE(lc::test::importDrawing(reopened, file, formats[i]));
        std::vector<const RS_Entity*> reopenedGeometry;
        for (auto* entity : live(reopened)) {
            CHECK(entity->rtti() != RS2::EntityOffsetCurve);
            reopenedGeometry.push_back(entity);
        }
        CAPTURE(formats[i]);
        CHECK(sameTrimGeometrySet(expectedFileGeometry, reopenedGeometry));
    }

    REQUIRE(f.graphic.undo());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    CHECK_FALSE(hyperbola->isDeleted());
    REQUIRE(native->hasGeneralizedSourceDefinition());
    CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::BoundedHyperbola);
    CHECK(trimGeometry(*native) == original);
    for (const auto* output : fragments) CHECK(output->isDeleted());
    REQUIRE(f.graphic.redo());
    CHECK(native->isDeleted());
    for (const auto* output : fragments) CHECK_FALSE(output->isDeleted());
}

TEST_CASE("T Trim Two action trims a worker-created typed hyperbola against circular and elliptic cutters",
          "[gui][lc-offsetcurve][trim-action][typed-source][hyperbola][trim-two]") {
    constexpr double rotation = .37;
    for (const unsigned profile : {0u, 1u, 2u, 3u, 4u, 5u})
    for (const bool leftBranch : {false, true})
    for (const unsigned cutterKind : {0u, 1u})
    for (const bool hyperbolaFirst : {false, true}) {
        CAPTURE(profile, leftBranch, cutterKind, hyperbolaFirst);
        Drawing<true> f;
        auto* previous = addNative(f.graphic);
        const LC_HyperbolaData sourceData = profile == 0
            ? LC_HyperbolaData{{0, 0}, {3, 0}, 1.0, -1.5, 1.5, leftBranch}
            : profile == 1
                ? LC_HyperbolaData{{5, -4}, {3 * std::cos(rotation), 3 * std::sin(rotation)},
                                   .75, -.55, 1.15, leftBranch}
                : profile == 2
                    ? LC_HyperbolaData{{-2, 6}, {3.5 * std::cos(-.58), 3.5 * std::sin(-.58)},
                                       .6, -1.1, .9, leftBranch}
                    : profile == 3
                        ? LC_HyperbolaData{{9, -7}, {8 * std::cos(.23), 8 * std::sin(.23)},
                                           .4, -1.1, .9, leftBranch}
                        : profile == 4
                            ? LC_HyperbolaData{{0, 0}, {3, 0}, 1.0, 1.5, -1.5, leftBranch}
                            : LC_HyperbolaData{{12, -8}, {3 * std::cos(.31), 3 * std::sin(.31)},
                                               .7, 1.4, -.7, leftBranch};
        auto* hyperbola = new LC_Hyperbola(&f.graphic, sourceData);
        f.graphic.addEntity(hyperbola);
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::BoundedHyperbola);
        update.use(hyperbola);
        lc::test::awaitUpdate(update);
        REQUIRE(update.lastReport().updated == 1);
        REQUIRE(previous->isDeleted());
        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        REQUIRE(native->hasGeneralizedSourceDefinition());
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::BoundedHyperbola);

        double first = 0, last = 0;
        REQUIRE(native->getParameterDomain(first, last));
        const auto breaks = native->getBreakParameters();
        REQUIRE(breaks.size() >= 2);
        const double parameter = breaks.front() + (breaks[1] - breaks.front()) * .43;
        const RS_Vector intersection = native->getPointAt(parameter);
        const double step = std::max(1e-7, (last - first) * 1e-5);
        const RS_Vector before = native->getPointAt(parameter - step);
        const RS_Vector after = native->getPointAt(parameter + step);
        RS_Vector tangent{after.x - before.x, after.y - before.y};
        REQUIRE(tangent.magnitude() > RS_TOLERANCE);
        tangent /= tangent.magnitude();
        RS_Vector normal{-tangent.y, tangent.x};
        constexpr double circleRadius = 1.0;
        const RS_Vector circleCenter = intersection + tangent * circleRadius;
        RS_Entity* cutter = nullptr;
        RS_Vector cutterPick;
        if (cutterKind == 0) {
            cutter = new RS_Circle(&f.graphic, RS_CircleData{circleCenter, circleRadius});
            cutterPick = circleCenter + normal * circleRadius;
        } else {
            cutter = new RS_Ellipse(&f.graphic,
                RS_EllipseData{circleCenter, tangent * (1.1 * circleRadius), .95, 0, 0, false});
            cutterPick = circleCenter + normal * (1.045 * circleRadius);
        }
        f.graphic.addEntity(cutter);
        const RS_Vector nativePick = native->getPointAt(
            breaks.front() + (breaks[1] - breaks.front()) * .9);
        const auto nativeBefore = trimGeometry(*native);
        const auto cutterBefore = trimGeometry(*cutter);
        const auto directTrim = LC_OffsetCacheTrim::prepare(native->getData(), *cutter,
            {nativePick});
        REQUIRE(directTrim.refusal == LC_OffsetCacheTrim::Refusal::None);
        REQUIRE(directTrim.rootCount >= 2);
        const RS_Vector firstPick = hyperbolaFirst ? nativePick : cutterPick;
        const RS_Vector secondPick = hyperbolaFirst ? cutterPick : nativePick;
        f.action->click(firstPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        const auto previewUndoEpoch = f.graphic.undoStateEpoch();
        f.action->hover(secondPick);
        const auto preview = trimPreview(*f.action->m_preview);
        REQUIRE_FALSE(preview.empty());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(trimGeometry(*native) == nativeBefore);
        CHECK(trimGeometry(*cutter) == cutterBefore);
        CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);

        f.action->click(secondPick);
        REQUIRE(native->isDeleted());
        REQUIRE(cutter->isDeleted());
        std::vector<std::string> committed;
        std::vector<RS_Entity*> fragments;
        for (auto* entity : live(f.graphic)) {
            if (entity == hyperbola) continue;
            fragments.push_back(entity);
            committed.push_back(trimGeometry(*entity));
        }
        CHECK(committed == preview);
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntitySpline;
        }));
        CHECK(std::any_of(fragments.begin(), fragments.end(), [&](const RS_Entity* entity) {
            return entity->rtti() == (cutterKind == 0 ? RS2::EntityArc : RS2::EntityEllipse);
        }));

        QTemporaryDir directory;
        REQUIRE(directory.isValid());
        const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
            RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
        QString libreDwgReader = qEnvironmentVariable("DWGREAD");
        if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
        const auto close = [](double a, double b) {
            return std::abs(a - b) <= 2e-10 * std::max({1.0, std::abs(a), std::abs(b)});
        };
        const auto closeVector = [&](const RS_Vector& a, const RS_Vector& b) {
            return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
        };
        const auto sameGeometry = [&](const RS_Entity& expected, const RS_Entity& actual) {
            if (expected.rtti() != actual.rtti()) return false;
            if (const auto* a = dynamic_cast<const RS_Spline*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Spline*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                if (x.degree != y.degree || x.m_closedFlag != y.m_closedFlag ||
                    x.m_periodicFlag != y.m_periodicFlag || x.controlPoints.size() != y.controlPoints.size() ||
                    x.knotslist.size() != y.knotslist.size() || x.weights.size() != y.weights.size()) return false;
                for (std::size_t i = 0; i < x.controlPoints.size(); ++i)
                    if (!closeVector(x.controlPoints[i], y.controlPoints[i])) return false;
                for (std::size_t i = 0; i < x.knotslist.size(); ++i)
                    if (!close(x.knotslist[i], y.knotslist[i])) return false;
                for (std::size_t i = 0; i < x.weights.size(); ++i)
                    if (!close(x.weights[i], y.weights[i])) return false;
                return true;
            }
            if (const auto* a = dynamic_cast<const RS_Arc*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Arc*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                return closeVector(x.center, y.center) && close(x.radius, y.radius) &&
                    close(x.angle1, y.angle1) && close(x.angle2, y.angle2) && x.reversed == y.reversed;
            }
            if (const auto* a = dynamic_cast<const RS_Ellipse*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Ellipse*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                if (!x.isArc || !y.isArc) return false;
                const bool sameEnds = closeVector(a->getStartpoint(), b->getStartpoint()) &&
                    closeVector(a->getEndpoint(), b->getEndpoint());
                const bool reversedEnds = closeVector(a->getStartpoint(), b->getEndpoint()) &&
                    closeVector(a->getEndpoint(), b->getStartpoint());
                return closeVector(x.center, y.center) && close(x.majorP.magnitude(), y.majorP.magnitude()) &&
                    close(x.ratio, y.ratio) && close(a->getLength(), b->getLength()) &&
                    closeVector(a->getMiddlePoint(), b->getMiddlePoint()) && (sameEnds || reversedEnds);
            }
            if (const auto* a = dynamic_cast<const LC_Hyperbola*>(&expected)) {
                const auto* b = dynamic_cast<const LC_Hyperbola*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                const double xAxisLength = x.majorP.magnitude();
                const double yAxisLength = y.majorP.magnitude();
                if (!closeVector(x.center, y.center) || !close(xAxisLength, yAxisLength) ||
                    !close(x.ratio, y.ratio) || xAxisLength == 0.0 || yAxisLength == 0.0)
                    return false;
                const double axisAlignment = std::abs(
                    (x.majorP.x * y.majorP.x + x.majorP.y * y.majorP.y) /
                    (xAxisLength * yAxisLength));
                if (!close(axisAlignment, 1.0)) return false;
                const RS_Vector aStart = a->getStartpoint();
                const RS_Vector aEnd = a->getEndpoint();
                const RS_Vector bStart = b->getStartpoint();
                const RS_Vector bEnd = b->getEndpoint();
                const bool matchingEnds =
                    (closeVector(aStart, bStart) && closeVector(aEnd, bEnd)) ||
                    (closeVector(aStart, bEnd) && closeVector(aEnd, bStart));
                return matchingEnds && closeVector(a->getMiddlePoint(), b->getMiddlePoint());
            }
            return false;
        };
        for (std::size_t i = 0; i < formats.size(); ++i) {
            const bool dwg = formats[i] == RS2::FormatDWG || formats[i] == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("typed-hyperbola-trim-two-%1-%2-%3")
                    .arg(profile).arg(leftBranch ? QStringLiteral("left") : QStringLiteral("right")).arg(i) +
                (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[i]));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
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
                unsigned externalSplines = 0;
                unsigned externalArcs = 0;
                unsigned externalEllipses = 0;
                for (const QJsonValue& value : document.object().value("OBJECTS").toArray()) {
                    const QString entity = value.toObject().value("entity").toString();
                    externalSplines += entity == QLatin1String("SPLINE");
                    externalArcs += entity == QLatin1String("ARC");
                    externalEllipses += entity == QLatin1String("ELLIPSE");
                }
                CHECK(externalSplines > 0);
                const bool hasCutterFragments = cutterKind == 0 ? externalArcs > 0 : externalEllipses > 0;
                CHECK(hasCutterFragments);
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, formats[i]));
            auto reopenedEntities = live(reopened);
            unsigned reopenedHyperbolas = 0;
            for (auto* entity : reopenedEntities) {
                CHECK(entity->rtti() != RS2::EntityOffsetCurve);
                if (entity->rtti() != RS2::EntityHyperbola) continue;
                ++reopenedHyperbolas;
                const auto* reopenedHyperbola = dynamic_cast<const LC_Hyperbola*>(entity);
                REQUIRE(reopenedHyperbola);
                CAPTURE(hyperbola->getData(), reopenedHyperbola->getData());
                CAPTURE(hyperbola->getStartpoint(), hyperbola->getEndpoint(), hyperbola->getMiddlePoint(),
                        reopenedHyperbola->getStartpoint(), reopenedHyperbola->getEndpoint(),
                        reopenedHyperbola->getMiddlePoint());
                CHECK(sameGeometry(*hyperbola, *reopenedHyperbola));
            }
            CAPTURE(formats[i]);
            CHECK(reopenedHyperbolas == 1);
            const auto expectedEntities = live(f.graphic);
            REQUIRE(reopenedEntities.size() == expectedEntities.size());
            std::vector<bool> consumed(reopenedEntities.size(), false);
            for (const auto* expected : expectedEntities) {
                bool found = false;
                for (std::size_t j = 0; j < reopenedEntities.size(); ++j) {
                    if (consumed[j] || !sameGeometry(*expected, *reopenedEntities[j])) continue;
                    consumed[j] = true;
                    found = true;
                    break;
                }
                CHECK(found);
            }
        }

        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK_FALSE(hyperbola->isDeleted());
        REQUIRE(native->hasGeneralizedSourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::BoundedHyperbola);
        CHECK(trimGeometry(*native) == nativeBefore);
        CHECK(trimGeometry(*cutter) == cutterBefore);
        for (const auto* fragment : fragments) CHECK(fragment->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK(cutter->isDeleted());
        for (const auto* fragment : fragments) CHECK_FALSE(fragment->isDeleted());
    }
}

TEST_CASE("T Trim Two commits certified exact- and post-cusp hyperbola caches atomically",
          "[gui][lc-offsetcurve][trim-action][typed-source][hyperbola][cusp][trim-two]") {
    Drawing<true> f;
    auto* previous = addNative(f.graphic);
    const double ratio = GENERATE(0.4, std::sqrt(1.0 / 3.5));
    const double translation = GENERATE(0.0, 1e8, 1e9, 2e9);
    const RS_Vector origin = translation == 0.0
                                 ? RS_Vector{-2, 6}
                                 : RS_Vector{translation, -translation};
    if (translation > 0.0) {
        const double zoom = translation > 1.5e9 ? 1.0 : (translation > 5e8 ? 2.0 : 4.0);
        f.view.getViewPort()->setOffsetAndFactor(qRound(200.0 - origin.x * zoom),
                                                  qRound(200.0 - origin.y * zoom), zoom);
    }
    auto* hyperbola = new LC_Hyperbola(&f.graphic,
        LC_HyperbolaData{origin, {3.5 * std::cos(-.58), 3.5 * std::sin(-.58)},
                         ratio, -1.1, .9, false});
    f.graphic.addEntity(hyperbola);
    CAPTURE(ratio, 3.5 * ratio * ratio, translation, origin);
    f.graphic.select(QList<RS_Entity*>{previous}, true);
    UpdateSourceProbe update(&f.context);
    update.init(0);
    update.setSourceKind(LC_OffsetSourceGeometry::Kind::BoundedHyperbola);
    update.use(hyperbola);
    lc::test::awaitUpdate(update);
    REQUIRE(update.lastReport().updated == 1);
    REQUIRE(previous->isDeleted());

    LC_OffsetCurve* native = nullptr;
    for (auto* entity : live(f.graphic))
        if (entity->rtti() == RS2::EntityOffsetCurve)
            native = dynamic_cast<LC_OffsetCurve*>(entity);
    REQUIRE(native);
    const std::string nativeBefore = trimGeometry(*native);
    const auto breaks = native->getBreakParameters();
    REQUIRE(breaks.size() >= 2);
    const double parameter = breaks.front() + (breaks[1] - breaks.front()) * .43;
    const RS_Vector intersection = native->getPointAt(parameter);
    const double step = std::max(1e-7, (breaks[1] - breaks.front()) * 1e-5);
    RS_Vector tangent = native->getPointAt(parameter + step) - native->getPointAt(parameter - step);
    REQUIRE(tangent.magnitude() > RS_TOLERANCE);
    tangent /= tangent.magnitude();
    const RS_Vector normal{-tangent.y, tangent.x};
    const RS_Vector circleCenter = intersection + tangent;
    auto* circle = new RS_Circle(&f.graphic, RS_CircleData{circleCenter, 1.0});
    f.graphic.addEntity(circle);
    const std::string circleBefore = trimGeometry(*circle);

    const RS_Vector circlePick = circleCenter + normal;
    const RS_Vector nativePick = native->getPointAt(
        breaks.front() + (breaks[1] - breaks.front()) * .9);
    const auto direct = LC_OffsetCacheTrim::prepare(native->getData(), *circle,
        LC_OffsetCacheTrim::Pick{nativePick});
    CAPTURE(direct.rootCount, static_cast<int>(direct.refusal), direct.fragments.size(),
            direct.work.intersections.totalBoxPairs,
            direct.work.intersections.generatedLeaves);
    REQUIRE(direct.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(direct.rootCount == 2);
    REQUIRE(direct.fragments.size() == 2);
    const auto undoNativeBefore = trimGeometry(*native);
    const auto undoCircleBefore = trimGeometry(*circle);
    f.action->click(circlePick);
    REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
    const auto previewUndoEpoch = f.graphic.undoStateEpoch();
    f.action->hover(nativePick);
    REQUIRE(f.action->m_preview);
    const auto preview = trimPreview(*f.action->m_preview);
    REQUIRE(preview.size() == 3);
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(circle->isDeleted());
    CHECK(trimGeometry(*native) == nativeBefore);
    CHECK(trimGeometry(*circle) == circleBefore);
    CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);

    f.action->click(nativePick);
    CHECK(native->isDeleted());
    CHECK(circle->isDeleted());
    std::vector<RS_Entity*> fragments;
    std::vector<std::string> committed;
    for (auto* entity : live(f.graphic)) {
        if (entity == hyperbola) continue;
        fragments.push_back(entity);
        committed.push_back(trimGeometry(*entity));
    }
    std::sort(committed.begin(), committed.end());
    auto expected = preview;
    std::sort(expected.begin(), expected.end());
    CHECK(committed == expected);

    REQUIRE(f.graphic.undo());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(circle->isDeleted());
    CHECK(trimGeometry(*native) == undoNativeBefore);
    CHECK(trimGeometry(*circle) == undoCircleBefore);
    for (const auto* fragment : fragments) CHECK(fragment->isDeleted());
    REQUIRE(f.graphic.redo());
    CHECK(native->isDeleted());
    CHECK(circle->isDeleted());
    for (const auto* fragment : fragments) CHECK_FALSE(fragment->isDeleted());
}

TEST_CASE("T Trim Two action trims a worker-created typed parabola against circular and elliptic cutters",
          "[gui][lc-offsetcurve][trim-action][typed-source][parabola][trim-two]") {
    for (const bool highOrigin : {false, true})
    for (const bool nearCusp : {false, true})
    for (const bool asymmetric : {false, true})
    for (const double scale : {0.1, 1.0, 10.0, 1e6})
    for (const bool transformed : {false, true})
    for (const bool oppositeSide : {false, true})
    for (const unsigned cutterKind : {0u, 1u})
    for (const bool parabolaFirst : {false, true}) {
        if (nearCusp && (asymmetric || scale != 1.0 || transformed)) continue;
        if (highOrigin && (nearCusp || scale != 1e6)) continue;
        if (!highOrigin && scale == 1e6) continue;
        CAPTURE(highOrigin, nearCusp, asymmetric, scale, transformed, oppositeSide, cutterKind, parabolaFirst);
        Drawing<true> f;
        const double seedRadius = highOrigin ? scale : 1.0;
        RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {seedRadius, 0}, 1, 0, .1, false});
        const double offsetDistance = scale * (nearCusp ? .45 : .05);
        auto initial = LC_OffsetCurve::create(seed, offsetDistance, oppositeSide);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);
        const double angle = transformed ? .37 : 0.0;
        const double cosine = std::cos(angle), sine = std::sin(angle);
        const RS_Vector origin = highOrigin ? RS_Vector{1e13, -2e13} : RS_Vector{};
        const RS_Vector localTranslation = transformed ? RS_Vector{2 * scale, -scale} : RS_Vector{};
        const RS_Vector translation = origin + localTranslation;
        if (highOrigin) {
            constexpr double zoom = 5e-5;
            f.view.getViewPort()->setSize(640, 480);
            f.view.getViewPort()->setOffsetAndFactor(
                qRound(320.0 - translation.x * zoom),
                qRound(240.0 - translation.y * zoom), zoom);
        }
        const auto mapPoint = [&](RS_Vector point) {
            point *= scale;
            return RS_Vector{cosine * point.x - sine * point.y,
                             sine * point.x + cosine * point.y} + translation;
        };
        const std::array<RS_Vector, 3> controlPoints = asymmetric
            ? std::array<RS_Vector, 3>{{{-1, 0}, {0, 1}, {2, 0}}}
            : std::array<RS_Vector, 3>{{{-1, 1}, {0, -1}, {1, 1}}};
        const LC_ParabolaData sourceData{{mapPoint(controlPoints[0]), mapPoint(controlPoints[1]),
                                          mapPoint(controlPoints[2])}};
        auto* parabola = new LC_Parabola(&f.graphic, sourceData);
        f.graphic.addEntity(parabola);
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Parabola);
        update.use(parabola);
        lc::test::awaitUpdate(update);
        REQUIRE(update.lastReport().updated == 1);
        REQUIRE(previous->isDeleted());
        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        REQUIRE(native->hasGeneralizedSourceDefinition());
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Parabola);
        CHECK(native->sourceDefinition()->signedDistance == (oppositeSide ? -offsetDistance : offsetDistance));

        double first = 0, last = 0;
        REQUIRE(native->getParameterDomain(first, last));
        const auto breaks = native->getBreakParameters();
        REQUIRE(breaks.size() >= 2);
        const double parameter = breaks.front() + (breaks[1] - breaks.front()) * .43;
        const RS_Vector intersection = native->getPointAt(parameter);
        const double step = std::max(1e-7, (last - first) * 1e-5);
        const RS_Vector before = native->getPointAt(parameter - step);
        const RS_Vector after = native->getPointAt(parameter + step);
        RS_Vector tangent{after.x - before.x, after.y - before.y};
        REQUIRE(tangent.magnitude() > RS_TOLERANCE);
        tangent /= tangent.magnitude();
        const RS_Vector normal{-tangent.y, tangent.x};
        const double circleRadius = .1 * scale;
        const RS_Vector circleCenter = intersection + tangent * circleRadius;
        RS_Entity* cutter = nullptr;
        RS_Vector cutterPick;
        if (cutterKind == 0) {
            cutter = new RS_Circle(&f.graphic, RS_CircleData{circleCenter, circleRadius});
            cutterPick = circleCenter + normal * circleRadius;
        } else {
            cutter = new RS_Ellipse(&f.graphic,
                RS_EllipseData{circleCenter, tangent * (1.4 * circleRadius), .5, 0, 0, false});
            cutterPick = circleCenter + normal * (.7 * circleRadius);
        }
        f.graphic.addEntity(cutter);
        const RS_Vector nativePick = native->getPointAt(
            breaks.front() + (breaks[1] - breaks.front()) * .9);
        const auto nativeBefore = trimGeometry(*native);
        const auto cutterBefore = trimGeometry(*cutter);
        const RS_Vector initialVertex = parabola->getData().m_controlPoints[1];
        f.action->click(parabolaFirst ? nativePick : cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        const auto previewUndoEpoch = f.graphic.undoStateEpoch();
        f.action->hover(parabolaFirst ? cutterPick : nativePick);
        const auto preview = trimPreview(*f.action->m_preview);
        REQUIRE_FALSE(preview.empty());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(trimGeometry(*native) == nativeBefore);
        CHECK(trimGeometry(*cutter) == cutterBefore);
        CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);

        f.action->click(parabolaFirst ? cutterPick : nativePick);
        REQUIRE(native->isDeleted());
        REQUIRE(cutter->isDeleted());
        std::vector<RS_Entity*> fragments;
        std::vector<std::string> committed;
        for (auto* entity : live(f.graphic)) {
            if (entity == parabola) continue;
            REQUIRE(entity->rtti() != RS2::EntityOffsetCurve);
            fragments.push_back(entity);
            committed.push_back(trimGeometry(*entity));
        }
        CHECK(committed == preview);
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntitySpline;
        }));
        CHECK(std::any_of(fragments.begin(), fragments.end(), [&](const RS_Entity* entity) {
            return entity->rtti() == (cutterKind == 0 ? RS2::EntityArc : RS2::EntityEllipse);
        }));

        QTemporaryDir directory;
        REQUIRE(directory.isValid());
        const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
            RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
        QString libreDwgReader = qEnvironmentVariable("DWGREAD");
        if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
        const auto close = [](double a, double b) {
            return std::abs(a - b) <= 2e-10 * std::max({1.0, std::abs(a), std::abs(b)});
        };
        const auto closeVector = [&](const RS_Vector& a, const RS_Vector& b) {
            return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
        };
        const auto sameGeometry = [&](const RS_Entity& expected, const RS_Entity& actual) {
            if (expected.rtti() != actual.rtti()) return false;
            if (const auto* a = dynamic_cast<const RS_Spline*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Spline*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                if (x.degree != y.degree || x.m_closedFlag != y.m_closedFlag ||
                    x.m_periodicFlag != y.m_periodicFlag || x.controlPoints.size() != y.controlPoints.size() ||
                    x.knotslist.size() != y.knotslist.size() || x.weights.size() != y.weights.size()) return false;
                for (std::size_t i = 0; i < x.controlPoints.size(); ++i)
                    if (!closeVector(x.controlPoints[i], y.controlPoints[i])) return false;
                for (std::size_t i = 0; i < x.knotslist.size(); ++i)
                    if (!close(x.knotslist[i], y.knotslist[i])) return false;
                for (std::size_t i = 0; i < x.weights.size(); ++i)
                    if (!close(x.weights[i], y.weights[i])) return false;
                return true;
            }
            if (const auto* a = dynamic_cast<const RS_Arc*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Arc*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                return closeVector(x.center, y.center) && close(x.radius, y.radius) &&
                    close(x.angle1, y.angle1) && close(x.angle2, y.angle2) && x.reversed == y.reversed;
            }
            if (const auto* a = dynamic_cast<const RS_Ellipse*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Ellipse*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                if (x.isArc || y.isArc) {
                    if (!x.isArc || !y.isArc) return false;
                    const bool sameEnds = closeVector(a->getStartpoint(), b->getStartpoint()) &&
                        closeVector(a->getEndpoint(), b->getEndpoint());
                    const bool reversedEnds = closeVector(a->getStartpoint(), b->getEndpoint()) &&
                        closeVector(a->getEndpoint(), b->getStartpoint());
                    return closeVector(x.center, y.center) && close(x.majorP.magnitude(), y.majorP.magnitude()) &&
                        close(x.ratio, y.ratio) && close(a->getLength(), b->getLength()) &&
                        closeVector(a->getMiddlePoint(), b->getMiddlePoint()) && (sameEnds || reversedEnds);
                }
                return closeVector(x.center, y.center) && closeVector(x.majorP, y.majorP) &&
                    close(x.ratio, y.ratio) && close(x.angle1, y.angle1) && close(x.angle2, y.angle2) &&
                    x.reversed == y.reversed;
            }
            if (const auto* a = dynamic_cast<const LC_Parabola*>(&expected)) {
                const auto* b = dynamic_cast<const LC_Parabola*>(&actual);
                if (!b) return false;
                for (std::size_t i = 0; i < a->getData().m_controlPoints.size(); ++i)
                    if (!closeVector(a->getData().m_controlPoints[i], b->getData().m_controlPoints[i])) return false;
                return true;
            }
            return false;
        };
        for (std::size_t i = 0; i < formats.size(); ++i) {
            const bool dwg = formats[i] == RS2::FormatDWG || formats[i] == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("typed-parabola-trim-two-%1").arg(i) + (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[i]));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                unsigned splines = 0, arcs = 0, ellipses = 0;
                for (const QJsonValue& value : document.object().value("OBJECTS").toArray()) {
                    const QString entity = value.toObject().value("entity").toString();
                    splines += entity == QLatin1String("SPLINE");
                    arcs += entity == QLatin1String("ARC");
                    ellipses += entity == QLatin1String("ELLIPSE");
                }
                CHECK(splines > 0);
                const bool hasCutterFragments = cutterKind == 0 ? arcs > 0 : ellipses > 0;
                CHECK(hasCutterFragments);
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, formats[i]));
            const auto expectedEntities = live(f.graphic);
            const auto reopenedEntities = live(reopened);
            REQUIRE(reopenedEntities.size() == expectedEntities.size());
            CHECK(std::count_if(reopenedEntities.begin(), reopenedEntities.end(), [](const RS_Entity* entity) {
                return entity->rtti() == RS2::EntityOffsetCurve;
            }) == 0);
            std::vector<bool> consumed(reopenedEntities.size(), false);
            for (const auto* expected : expectedEntities) {
                bool found = false;
                for (std::size_t j = 0; j < reopenedEntities.size(); ++j) {
                    if (consumed[j] || !sameGeometry(*expected, *reopenedEntities[j])) continue;
                    consumed[j] = true;
                    found = true;
                    break;
                }
                CHECK(found);
            }
            unsigned reopenedParabolas = 0;
            for (const auto* entity : reopenedEntities) {
                if (entity->rtti() != RS2::EntityParabola) continue;
                ++reopenedParabolas;
                const auto* loadedParabola = dynamic_cast<const LC_Parabola*>(entity);
                REQUIRE(loadedParabola);
                for (std::size_t j = 0; j < sourceData.m_controlPoints.size(); ++j)
                    CHECK(closeVector(loadedParabola->getData().m_controlPoints[j], sourceData.m_controlPoints[j]));
            }
            CAPTURE(formats[i]);
            CHECK(reopenedParabolas == 1);
        }

        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK_FALSE(parabola->isDeleted());
        REQUIRE(native->hasGeneralizedSourceDefinition());
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Parabola);
        CHECK(trimGeometry(*native) == nativeBefore);
        CHECK(trimGeometry(*cutter) == cutterBefore);
        CHECK(parabola->getData().m_controlPoints[1] == initialVertex);
        for (const auto* fragment : fragments) CHECK(fragment->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK(cutter->isDeleted());
        for (const auto* fragment : fragments) CHECK_FALSE(fragment->isDeleted());
    }
}

TEST_CASE("T Update from Source preserves exact and multi-branch parabola cusp offsets",
          "[gui][lc-offsetcurve][source-update][typed-source][parabola][cusp]") {
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
    for (const double distance : {0.5, 0.51}) {
        CAPTURE(distance);
        Drawing<false> f;
        RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false});
        auto initial = LC_OffsetCurve::create(seed, distance, false);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);
        const std::string previousGeometry = trimGeometry(*previous);

        const LC_ParabolaData sourceData{{RS_Vector{-1, 1}, RS_Vector{0, -1}, RS_Vector{1, 1}}};
        auto* parabola = new LC_Parabola(&f.graphic, sourceData);
        f.graphic.addEntity(parabola);
        f.graphic.select(QList<RS_Entity*>{previous}, true);
        const auto undoEpoch = f.graphic.undoStateEpoch();

        const LC_CurveOffsetGeometryResult direct = LC_CurveOffset::buildDirectBranches(*parabola,
            LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, distance),
            LC_CurveOffset::makeDirectOptions(*parabola, distance, 1e-4),
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(direct.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(direct.branches.size() == (distance == .5 ? 1u : 3u));

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Parabola);
        update.use(parabola);
        lc::test::awaitUpdate(update);
        const auto& report = update.lastReport();
        REQUIRE(report.updated == 1);
        CHECK(report.refused == 0);
        CHECK(previous->isDeleted());
        LC_OffsetCurve* replacement = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(replacement == nullptr);
            replacement = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(replacement);
        CHECK(replacement != previous);
        REQUIRE(replacement->hasGeneralizedSourceDefinition());
        REQUIRE(replacement->sourceDefinition());
        CHECK(replacement->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Parabola);
        CHECK(replacement->sourceDefinition()->signedDistance == distance);
        const auto& knots = replacement->getData().knotslist;
        std::size_t disconnectedBranchSeparators = 0;
        for (std::size_t i = 0; i < knots.size();) {
            std::size_t next = i + 1;
            while (next < knots.size() && knots[next] == knots[i]) ++next;
            if (i > 0 && next < knots.size() && next - i == replacement->getData().degree + 1)
                ++disconnectedBranchSeparators;
            i = next;
        }
        CHECK(disconnectedBranchSeparators == direct.branches.size() - 1);
        CHECK(live(f.graphic).size() == 2);
        CHECK(f.graphic.undoStateEpoch() > undoEpoch);

        const auto cacheFingerprint = RS_Spline::geometryFingerprint(replacement->getData());
        for (std::size_t i = 0; i < formats.size(); ++i) {
            const bool dwg = formats[i] == RS2::FormatDWG || formats[i] == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("parabola-cusp-cache-%1-%2").arg(distance).arg(i) +
                (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[i]));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, formats[i]));
            LC_OffsetCurve* loaded = nullptr;
            for (auto* entity : live(reopened)) {
                if (entity->rtti() != RS2::EntityOffsetCurve) continue;
                REQUIRE(loaded == nullptr);
                loaded = dynamic_cast<LC_OffsetCurve*>(entity);
            }
            CAPTURE(distance, formats[i]);
            REQUIRE(loaded);
            REQUIRE(loaded->hasGeneralizedSourceDefinition());
            REQUIRE(loaded->sourceDefinition());
            CHECK(loaded->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Parabola);
            CHECK(loaded->sourceDefinition()->signedDistance == distance);
            CHECK(RS_Spline::geometryFingerprint(loaded->getData()) == cacheFingerprint);
        }

        REQUIRE(f.graphic.undo());
        CHECK_FALSE(previous->isDeleted());
        CHECK(trimGeometry(*previous) == previousGeometry);
        CHECK(replacement->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(previous->isDeleted());
        CHECK_FALSE(replacement->isDeleted());
    }
}

TEST_CASE("T Source update preserves parabola cusp topology under similarity transforms",
          "[gui][lc-offsetcurve][source-update][typed-source][parabola][cusp][transform]") {
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
    std::size_t fileIndex = 0;
    for (const double scale : {0.1, 1.0, 10.0})
    for (const bool transformed : {false, true})
    for (const double ratio : {1.0, 1.02}) {
        CAPTURE(scale, transformed, ratio);
        const double distance = .5 * scale * ratio;
        const double angle = transformed ? .37 : 0.0;
        const double cosine = std::cos(angle), sine = std::sin(angle);
        const RS_Vector translation = transformed ? RS_Vector{2 * scale, -scale} : RS_Vector{};
        const auto mapPoint = [&](RS_Vector point) {
            point *= scale;
            return RS_Vector{cosine * point.x - sine * point.y,
                             sine * point.x + cosine * point.y} + translation;
        };
        const LC_ParabolaData sourceData{{mapPoint({-1, 1}), mapPoint({0, -1}), mapPoint({1, 1})}};
        Drawing<false> f;
        RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false});
        auto initial = LC_OffsetCurve::create(seed, distance, false);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);
        auto* parabola = new LC_Parabola(&f.graphic, sourceData);
        f.graphic.addEntity(parabola);
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        const auto direct = LC_CurveOffset::buildDirectBranches(*parabola,
            LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, distance),
            LC_CurveOffset::makeDirectOptions(*parabola, distance, 1e-4),
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(direct.status == LC_CurveOffsetStatus::Ok);
        const std::size_t expectedBranches = ratio == 1.0 ? 1u : 3u;
        REQUIRE(direct.branches.size() == expectedBranches);

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Parabola);
        update.use(parabola);
        lc::test::awaitUpdate(update);
        REQUIRE(update.lastReport().updated == 1);
        CHECK(update.lastReport().refused == 0);
        CHECK(previous->isDeleted());
        LC_OffsetCurve* replacement = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(replacement == nullptr);
            replacement = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(replacement);
        REQUIRE(replacement->sourceDefinition());
        CHECK(replacement->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Parabola);
        CHECK(replacement->sourceDefinition()->signedDistance == distance);
        std::size_t separators = 0;
        const auto& knots = replacement->getData().knotslist;
        for (std::size_t i = 0; i < knots.size();) {
            std::size_t next = i + 1;
            while (next < knots.size() && knots[next] == knots[i]) ++next;
            if (i > 0 && next < knots.size() && next - i == replacement->getData().degree + 1)
                ++separators;
            i = next;
        }
        CHECK(separators == expectedBranches - 1);
        const auto replacementGeometry = trimGeometry(*replacement);
        const auto cacheFingerprint = RS_Spline::geometryFingerprint(replacement->getData());
        for (const auto format : formats) {
            const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("parabola-cusp-similarity-%1").arg(fileIndex++) +
                (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, format));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                CHECK_FALSE(document.object().value("OBJECTS").toArray().isEmpty());
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, format));
            LC_OffsetCurve* loaded = nullptr;
            for (auto* entity : live(reopened)) {
                if (entity->rtti() != RS2::EntityOffsetCurve) continue;
                REQUIRE(loaded == nullptr);
                loaded = dynamic_cast<LC_OffsetCurve*>(entity);
            }
            CAPTURE(scale, transformed, ratio, format);
            REQUIRE(loaded);
            REQUIRE(loaded->sourceDefinition());
            CHECK(loaded->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Parabola);
            CHECK(loaded->sourceDefinition()->signedDistance == distance);
            CHECK(RS_Spline::geometryFingerprint(loaded->getData()) == cacheFingerprint);
        }
        REQUIRE(f.graphic.undo());
        CHECK_FALSE(previous->isDeleted());
        CHECK(replacement->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(previous->isDeleted());
        CHECK_FALSE(replacement->isDeleted());
        CHECK(trimGeometry(*replacement) == replacementGeometry);
    }
}

TEST_CASE("T Source update preserves opposite-side cusp topology for an asymmetric parabola",
          "[gui][lc-offsetcurve][source-update][typed-source][parabola][cusp][asymmetric]") {
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
    std::size_t fileIndex = 0;
    for (const double scale : {0.1, 1.0, 10.0})
    for (const bool transformed : {false, true})
    for (const double ratio : {1.0, 1.02}) {
        CAPTURE(scale, transformed, ratio);
        const double distance = -2.25 * scale * ratio;
        const double angle = transformed ? .37 : 0.0;
        const double cosine = std::cos(angle), sine = std::sin(angle);
        const RS_Vector translation = transformed ? RS_Vector{2 * scale, -scale} : RS_Vector{};
        const auto mapPoint = [&](RS_Vector point) {
            point *= scale;
            return RS_Vector{cosine * point.x - sine * point.y,
                             sine * point.x + cosine * point.y} + translation;
        };
        const LC_ParabolaData sourceData{{mapPoint({-1, 0}), mapPoint({0, 1}), mapPoint({2, 0})}};
        Drawing<false> f;
        RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {10 * scale, 0}, 1, 0, .1, false});
        auto initial = LC_OffsetCurve::create(seed, std::abs(distance), true);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);
        auto* parabola = new LC_Parabola(&f.graphic, sourceData);
        f.graphic.addEntity(parabola);
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        const auto direct = LC_CurveOffset::buildDirectBranches(*parabola,
            LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, std::abs(distance)),
            LC_CurveOffset::makeDirectOptions(*parabola, std::abs(distance), 1e-4),
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(direct.status == LC_CurveOffsetStatus::Ok);
        const std::size_t expectedBranches = 2u;
        REQUIRE(direct.branches.size() == expectedBranches);

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Parabola);
        update.use(parabola);
        lc::test::awaitUpdate(update);
        REQUIRE(update.lastReport().updated == 1);
        CHECK(update.lastReport().refused == 0);
        CHECK(previous->isDeleted());
        LC_OffsetCurve* replacement = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(replacement == nullptr);
            replacement = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(replacement);
        REQUIRE(replacement->sourceDefinition());
        CHECK(replacement->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Parabola);
        CHECK(replacement->sourceDefinition()->signedDistance == distance);
        std::size_t separators = 0;
        const auto& knots = replacement->getData().knotslist;
        for (std::size_t i = 0; i < knots.size();) {
            std::size_t next = i + 1;
            while (next < knots.size() && knots[next] == knots[i]) ++next;
            if (i > 0 && next < knots.size() && next - i == replacement->getData().degree + 1)
                ++separators;
            i = next;
        }
        CHECK(separators == expectedBranches - 1);
        const auto replacementGeometry = trimGeometry(*replacement);
        const auto cacheFingerprint = RS_Spline::geometryFingerprint(replacement->getData());
        for (const auto format : formats) {
            const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("asymmetric-parabola-cusp-%1").arg(fileIndex++) +
                (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, format));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                CHECK_FALSE(document.object().value("OBJECTS").toArray().isEmpty());
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, format));
            LC_OffsetCurve* loaded = nullptr;
            for (auto* entity : live(reopened)) {
                if (entity->rtti() != RS2::EntityOffsetCurve) continue;
                REQUIRE(loaded == nullptr);
                loaded = dynamic_cast<LC_OffsetCurve*>(entity);
            }
            CAPTURE(scale, transformed, ratio, format);
            REQUIRE(loaded);
            REQUIRE(loaded->sourceDefinition());
            CHECK(loaded->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Parabola);
            CHECK(loaded->sourceDefinition()->signedDistance == distance);
            CHECK(RS_Spline::geometryFingerprint(loaded->getData()) == cacheFingerprint);
        }
        REQUIRE(f.graphic.undo());
        CHECK_FALSE(previous->isDeleted());
        CHECK(replacement->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(previous->isDeleted());
        CHECK_FALSE(replacement->isDeleted());
        CHECK(trimGeometry(*replacement) == replacementGeometry);
    }
}

TEST_CASE("T Trim Two trims reconstructed parabolas across transformed frames "
          "with undo and redo",
          "[gui][lc-offsetcurve][trim-action][typed-source][parabola][from-"
          "four-points][trim-two]") {
  struct Frame {
    double scale;
    double angle;
    RS_Vector translation;
    bool reversed;
    bool denseEnd;
    unsigned profile;
    bool expectLimitRefusal;
    bool allowAmbiguousCandidate = false;
    bool oppositeEndClustered = false;
    bool interiorAsymmetric = false;
    bool interiorBalanced = false;
    bool towardCentre = false;
  };
  const std::array<Frame, 86> frames{{
      {1.0, 0.0, {}, false, false, 0, false},
      {1e-6, 0.0, {}, false, false, 0, false},
      {1e6, 0.0, {}, false, false, 0, false},
      {1e-2, 0.6435011088, {1e6, -2e6}, true, false, 0, false},
      {1.0, 0.0, {}, false, true, 0, false},
      {1e-3, 0.0, {}, false, true, 0, false},
      {1e-2, 0.0, {}, false, true, 0, false},
      {1e3, 0.0, {}, false, true, 0, false},
      {1e2, 0.0, {}, false, true, 0, false},
      {1e-5, 0.0, {}, false, true, 0, false},
      {1e-4, 0.0, {}, false, true, 0, false},
      {1e6, 0.0, {}, false, true, 0, false},
      {1e-6, 0.0, {}, false, true, 0, true},
      {1e-6, 0.0, {}, false, true, 1, true},
      {1e-3, 0.0, {}, false, true, 1, false},
      {1e-2, 0.0, {}, false, true, 1, false},
      {1.0, 0.0, {}, false, true, 1, false},
      {1e2, 0.0, {}, false, true, 1, false},
      {1e3, 0.0, {}, false, true, 1, false},
      {1e-5, 0.0, {}, false, true, 1, true},
      {2e-5, 0.0, {}, false, true, 1, false},
      {5e-5, 0.0, {}, false, true, 1, false},
      {1e-4, 0.0, {}, false, true, 1, false},
      {1e6, 0.0, {}, false, true, 1, false},
      {1e-6, 0.0, {}, false, true, 2, true},
      {1e-3, 0.0, {}, false, true, 2, false},
      {1e-2, 0.0, {}, false, true, 2, false},
      {1.0, 0.0, {}, false, true, 2, false},
      {1e2, 0.0, {}, false, true, 2, false},
      {1e3, 0.0, {}, false, true, 2, false},
      {1e-5, 0.0, {}, false, true, 2, false},
      {1e-4, 0.0, {}, false, true, 2, false},
      {1e6, 0.0, {}, false, true, 2, false},
      {1e-2, 0.6435011088, {1e5, -2e5}, true, true, 0, false, true},
      {1e-2, 0.6435011088, {1e5, -2e5}, true, true, 1, false, true},
      {1e-2, 0.6435011088, {1e5, -2e5}, true, true, 2, false, true},
      {1e-1, 0.6435011087932844, {1e6, -2e6}, true, true, 2, false, true, true},
      {1.0, 0.6435011087932844, {1e6, -2e6}, true, true, 2, false, true, true},
      {1e-2, 0.6435011087932844, {1e6, -2e6}, true, true, 2, false, true, true},
      {1e-3, 0.6435011087932844, {1e6, -2e6}, true, true, 2, false, true, true},
      {1e-1, 0.6435011087932844, {1e6, -2e6}, true, true, 0, false, true, true},
      {1e-1, 0.6435011087932844, {1e6, -2e6}, true, true, 1, false, true, true},
      {1e-3, 0.6435011087932844, {1e6, -2e6}, true, true, 0, false, true, true},
      {1e-2, 0.6435011087932844, {1e6, -2e6}, true, true, 0, false, true, true},
      {1.0, 0.6435011087932844, {1e6, -2e6}, true, true, 0, false, true, true},
      {1e-3, 0.6435011087932844, {1e6, -2e6}, true, true, 1, false, true, true},
      {1e-2, 0.6435011087932844, {1e6, -2e6}, true, true, 1, false, true, true},
      {1.0, 0.6435011087932844, {1e6, -2e6}, true, true, 1, false, true, true},
      {1e-1, 0.6435011087932844, {1e6, -2e6}, true, true, 0, false, true},
      {1e-1, 0.6435011087932844, {1e6, -2e6}, true, true, 1, false, true},
      {1e-1, 0.6435011087932844, {1e6, -2e6}, true, true, 2, false, true},
      {1e-3, 0.6435011087932844, {1e6, -2e6}, true, true, 0, false, true},
      {1e-2, 0.6435011087932844, {1e6, -2e6}, true, true, 0, false, true},
      {1.0, 0.6435011087932844, {1e6, -2e6}, true, true, 0, false, true},
      {1e-3, 0.6435011087932844, {1e6, -2e6}, true, true, 1, false, true},
      {1e-2, 0.6435011087932844, {1e6, -2e6}, true, true, 1, false, true},
      {1.0, 0.6435011087932844, {1e6, -2e6}, true, true, 1, false, true},
      {1e-3, 0.6435011087932844, {1e6, -2e6}, true, true, 2, false, true},
      {1e-2, 0.6435011087932844, {1e6, -2e6}, true, true, 2, false, true},
      {1.0, 0.6435011087932844, {1e6, -2e6}, true, true, 2, false, true},
      {1e-3, 0.6435011087932844, {1e6, -2e6}, true, true, 0, false, false, false, true},
      {1e-2, 0.6435011087932844, {1e6, -2e6}, true, true, 0, false, false, false, true},
      {1e-1, 0.6435011087932844, {1e6, -2e6}, true, true, 0, false, false, false, true},
      {1.0, 0.6435011087932844, {1e6, -2e6}, true, true, 0, false, false, false, true},
      {1e-3, 0.6435011087932844, {1e6, -2e6}, true, true, 1, false, false, false, true},
      {1e-2, 0.6435011087932844, {1e6, -2e6}, true, true, 1, false, false, false, true},
      {1e-1, 0.6435011087932844, {1e6, -2e6}, true, true, 1, false, false, false, true},
      {1.0, 0.6435011087932844, {1e6, -2e6}, true, true, 1, false, false, false, true},
      {1e-3, 0.6435011087932844, {1e6, -2e6}, true, true, 2, false, false, false, true},
      {1e-2, 0.6435011087932844, {1e6, -2e6}, true, true, 2, false, false, false, true},
      {1e-1, 0.6435011087932844, {1e6, -2e6}, true, true, 2, false, false, false, true},
      {1.0, 0.6435011087932844, {1e6, -2e6}, true, true, 2, false, false, false, true},
      {1e-3, 0.6435011087932844, {1e6, -2e6}, true, false, 0, false, true, false, false, true},
      {1e-2, 0.6435011087932844, {1e6, -2e6}, true, false, 0, false, true, false, false, true},
      {1e-1, 0.6435011087932844, {1e6, -2e6}, true, false, 0, false, true, false, false, true},
      {1.0, 0.6435011087932844, {1e6, -2e6}, true, false, 0, false, true, false, false, true},
      {1e-3, 0.6435011087932844, {1e6, -2e6}, true, false, 1, false, true, false, false, true},
      {1e-2, 0.6435011087932844, {1e6, -2e6}, true, false, 1, false, true, false, false, true},
      {1e-1, 0.6435011087932844, {1e6, -2e6}, true, false, 1, false, true, false, false, true},
      {1.0, 0.6435011087932844, {1e6, -2e6}, true, false, 1, false, true, false, false, true},
      {1e-3, 0.6435011087932844, {1e6, -2e6}, true, false, 2, false, true, false, false, true},
      {1e-2, 0.6435011087932844, {1e6, -2e6}, true, false, 2, false, true, false, false, true},
      {1e-1, 0.6435011087932844, {1e6, -2e6}, true, false, 2, false, true, false, false, true},
      {1.0, 0.6435011087932844, {1e6, -2e6}, true, false, 2, false, true, false, false, true},
      {1.0, 0.6435011087932844, {1e9, -2e9}, true, false, 2, false, true, false, false, true},
      {1.0, 0.6435011087932844, {1e9, -2e9}, true, false, 2, false, true, false, false, true, true},
  }};
  for (const Frame &frame : frames) {
    CAPTURE(frame.scale, frame.angle, frame.translation.x,
            frame.translation.y, frame.reversed, frame.denseEnd,
            frame.profile, frame.expectLimitRefusal,
            frame.allowAmbiguousCandidate, frame.oppositeEndClustered,
            frame.interiorAsymmetric, frame.interiorBalanced,
            frame.towardCentre);
    const double scale = frame.scale;
    const double angle = frame.angle;
    const RS_Vector translation = frame.translation;
    const auto mapPoint = [&](RS_Vector point) {
      point *= scale;
      return RS_Vector{std::cos(angle) * point.x - std::sin(angle) * point.y,
                       std::sin(angle) * point.x + std::cos(angle) * point.y} +
             translation;
    };
    Drawing<true> f;
    const double zoom = 15.0 / scale;
    const double viewportX = 320.0 - translation.x * zoom;
    const double viewportY = 240.0 - translation.y * zoom;
    if (viewportX >= std::numeric_limits<int>::min() &&
        viewportX <= std::numeric_limits<int>::max() &&
        viewportY >= std::numeric_limits<int>::min() &&
        viewportY <= std::numeric_limits<int>::max()) {
      f.view.getViewPort()->setSize(640, 480);
      f.view.getViewPort()->setOffsetAndFactor(qRound(viewportX),
                                                qRound(viewportY), zoom);
    }
    RS_Ellipse seed(nullptr,
                    RS_EllipseData{{0, 0}, {scale, 0}, 1, 0, .1, false});
    const double offsetDistance = 0.5 * scale;
    auto initial = LC_OffsetCurve::create(seed, offsetDistance,
                                           frame.towardCentre);
    REQUIRE(initial);
    auto *previous = initial.entity.release();
    f.graphic.addEntity(previous);

    std::array<RS_Vector, 3> controls{{{-10, 5}, {0, -5}, {10, 5}}};
    if (frame.denseEnd) {
      const std::array<std::array<RS_Vector, 3>, 3> profiles{{
          {{{-2.0, 1.0}, {0.0, -1.0}, {2.0, 1.0}}},
          {{{-1.6, 0.8}, {0.3, -0.7}, {2.2, 1.5}}},
          {{{0.0, 0.0}, {1.0, 2.0}, {2.2, 0.4}}},
      }};
      controls = profiles[frame.profile];
    }
    const auto pointAt = [&](const double t) {
      const double oneMinusT = 1.0 - t;
      return mapPoint(controls[0] * (oneMinusT * oneMinusT) +
                      controls[1] * (2.0 * oneMinusT * t) +
                      controls[2] * (t * t));
    };
    const std::array<double, 4> sampleParameters = frame.interiorAsymmetric
        ? std::array<double, 4>{{0.05, 0.2, 0.65, 0.95}}
        : frame.interiorBalanced
        ? std::array<double, 4>{{0.1, 0.3, 0.7, 0.9}}
        : frame.denseEnd
        ? (frame.oppositeEndClustered
               ? std::array<double, 4>{{0.0, 0.98, 0.999, 1.0}}
               : std::array<double, 4>{{0.0, 0.95, 0.995, 1.0}})
        : std::array<double, 4>{{0.0, 0.3, 0.7, 1.0}};
    std::vector<RS_Vector> samples;
    for (const double parameter : sampleParameters)
      samples.push_back(pointAt(parameter));
    if (frame.reversed)
      std::reverse(samples.begin(), samples.end());
    std::array<RS_Vector, 3> expectedControls;
    std::transform(controls.begin(), controls.end(), expectedControls.begin(),
                   mapPoint);
    const auto candidates = LC_ParabolaData::From4Points(samples);
    REQUIRE_FALSE(candidates.empty());
    const std::array<double, 3> independentParameters{{0.15, 0.5, 0.85}};
    auto candidate = std::find_if(
        candidates.begin(), candidates.end(), [&](const auto &value) {
          if (!value.isValid())
            return false;
          if (frame.denseEnd) {
            const double tolerance = std::max(
                1e-8 * scale,
                16.0 * std::numeric_limits<double>::epsilon() *
                    std::max(std::abs(translation.x),
                             std::abs(translation.y)));
            return std::all_of(
                independentParameters.begin(), independentParameters.end(),
                [&](const double t) {
                  const RS_Vector expected = pointAt(t);
                  return value.fromX(value.findX(expected)).distanceTo(expected) <=
                         tolerance;
                });
          }
          const double tolerance = std::max(
              1e-8 * scale,
              16.0 * std::numeric_limits<double>::epsilon() *
                  std::max(std::abs(translation.x), std::abs(translation.y)));
          const bool forward = std::equal(
              value.m_controlPoints.begin(), value.m_controlPoints.end(),
              expectedControls.begin(),
              [tolerance](const RS_Vector &actual, const RS_Vector &expected) {
                return actual.distanceTo(expected) <= tolerance;
              });
          const bool reversed = std::equal(
              value.m_controlPoints.begin(), value.m_controlPoints.end(),
              expectedControls.rbegin(),
              [tolerance](const RS_Vector &actual, const RS_Vector &expected) {
                return actual.distanceTo(expected) <= tolerance;
              });
          return forward || reversed;
        });
    if (frame.oppositeEndClustered)
      CHECK(candidate == candidates.end());
    if (candidate == candidates.end() && frame.allowAmbiguousCandidate) {
      const double tolerance = std::max(
          1e-8 * scale,
          16.0 * std::numeric_limits<double>::epsilon() *
              std::max(std::abs(translation.x), std::abs(translation.y)));
      candidate = std::find_if(
          candidates.begin(), candidates.end(), [&](const auto &value) {
            if (!value.isValid())
              return false;
            return std::all_of(samples.begin(), samples.end(),
                [&](const RS_Vector &sample) {
                  return value.fromX(value.findX(sample)).distanceTo(sample) <=
                         tolerance;
                });
          });
    }
    REQUIRE(candidate != candidates.end());

    auto *parabola = new LC_Parabola(&f.graphic, *candidate);
    f.graphic.addEntity(parabola);
    f.graphic.select(QList<RS_Entity *>{previous}, true);
    UpdateSourceProbe update(&f.context);
    update.init(0);
    update.setSourceKind(LC_OffsetSourceGeometry::Kind::Parabola);
    update.use(parabola);
    lc::test::awaitUpdate(update);
    REQUIRE(update.lastReport().updated == 1);
    REQUIRE(previous->isDeleted());

    LC_OffsetCurve *native = nullptr;
    for (RS_Entity *entity : live(f.graphic)) {
      if (entity->rtti() != RS2::EntityOffsetCurve)
        continue;
      REQUIRE(native == nullptr);
      native = dynamic_cast<LC_OffsetCurve *>(entity);
    }
    REQUIRE(native);
    REQUIRE(native->sourceDefinition());
    REQUIRE(native->sourceDefinition()->source.kind() ==
            LC_OffsetSourceGeometry::Kind::Parabola);
    const auto *retainedSource =
        std::get_if<LC_ParabolaData>(&native->sourceDefinition()->source.value);
    REQUIRE(retainedSource);
    for (std::size_t i = 0; i < retainedSource->m_controlPoints.size(); ++i)
      CHECK(retainedSource->m_controlPoints[i].distanceTo(
                candidate->m_controlPoints[i]) == 0.0);

    const auto breaks = native->getBreakParameters();
    REQUIRE(breaks.size() >= 2);
    const double parameter = (breaks.front() + breaks[1]) * .5;
    const RS_Vector intersection = native->getPointAt(parameter);
    const double step = std::max(1e-7, (breaks[1] - breaks.front()) * 1e-5);
    RS_Vector tangent = native->getPointAt(parameter + step) -
                        native->getPointAt(parameter - step);
    REQUIRE(tangent.magnitude() >
            std::numeric_limits<double>::epsilon() * scale);
    tangent /= tangent.magnitude();
    const RS_Vector normal{-tangent.y, tangent.x};
    const double radius = .1 * scale;
    const RS_Vector circleCenter = intersection + tangent * radius;
    auto *cutter =
        new RS_Circle(&f.graphic, RS_CircleData{circleCenter, radius});
    f.graphic.addEntity(cutter);
    const RS_Vector nativePick =
        native->getPointAt(parameter + (breaks[1] - breaks.front()) * .1);
    const RS_Vector cutterPick = circleCenter + normal * radius;
    const auto nativeBefore = trimGeometry(*native);
    const auto cutterBefore = trimGeometry(*cutter);

    if (frame.expectLimitRefusal) {
      LC_DocumentModificationBatch refusedBatch;
      const auto refused = RS_Modification::trimStoredCache(
          f.graphic, *native, nativePick, *cutter, cutterPick, true,
          refusedBatch);
      CHECK_FALSE(refused.result);
      REQUIRE(refused.cacheRefusal);
      CHECK(*refused.cacheRefusal == LC_OffsetCacheTrim::Refusal::LimitExceeded);
      CHECK(refusedBatch.entitiesToAdd.empty());

      f.action->click(nativePick);
      REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
      f.action->hover(cutterPick);
      f.action->waitForTrim();
      CHECK(trimPreview(*f.action->m_preview).empty());
      const auto entityCount = f.graphic.count();
      const auto undoEpoch = f.graphic.undoStateEpoch();
      f.action->click(cutterPick);
      CHECK_FALSE(f.action->pending());
      CHECK(f.graphic.count() == entityCount);
      CHECK(f.graphic.undoStateEpoch() == undoEpoch);
      CHECK_FALSE(native->isDeleted());
      CHECK_FALSE(cutter->isDeleted());
      CHECK(trimGeometry(*native) == nativeBefore);
      CHECK(trimGeometry(*cutter) == cutterBefore);
      continue;
    }

    f.action->click(nativePick);
    REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
    f.action->hover(cutterPick);
    f.action->waitForTrim();
    const auto preview = trimPreview(*f.action->m_preview);
    REQUIRE_FALSE(preview.empty());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    CHECK(trimGeometry(*native) == nativeBefore);
    CHECK(trimGeometry(*cutter) == cutterBefore);

    f.action->click(cutterPick);
    REQUIRE(native->isDeleted());
    REQUIRE(cutter->isDeleted());
    std::vector<RS_Entity *> fragments;
    std::vector<std::string> committed;
    for (RS_Entity *entity : live(f.graphic)) {
      if (entity == parabola)
        continue;
      fragments.push_back(entity);
      if (entity->rtti() == RS2::EntitySpline ||
          entity->rtti() == RS2::EntityArc || entity->rtti() == RS2::EntityLine)
        committed.push_back(trimGeometry(*entity));
    }
    auto expected = preview;
    std::sort(committed.begin(), committed.end());
    std::sort(expected.begin(), expected.end());
    CHECK(committed == expected);
    CHECK(std::any_of(fragments.begin(), fragments.end(),
                      [](const RS_Entity *entity) {
                        return entity->rtti() == RS2::EntitySpline;
                      }));

    REQUIRE(f.graphic.undo());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    CHECK(trimGeometry(*native) == nativeBefore);
    CHECK(trimGeometry(*cutter) == cutterBefore);
    for (const RS_Entity *fragment : fragments)
      CHECK(fragment->isDeleted());
    REQUIRE(f.graphic.redo());
    CHECK(native->isDeleted());
    CHECK(cutter->isDeleted());
    for (const RS_Entity *fragment : fragments)
      CHECK_FALSE(fragment->isDeleted());

    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const std::array<RS2::FormatType, 5> formats{
        {RS2::FormatDXFRW14, RS2::FormatDXFRW2000, RS2::FormatDWG,
         RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty())
      libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
    std::vector<const RS_Entity *> expectedGeometry;
    for (const RS_Entity *fragment : fragments)
      if (fragment->rtti() == RS2::EntitySpline ||
          fragment->rtti() == RS2::EntityArc)
        expectedGeometry.push_back(fragment);
    REQUIRE_FALSE(expectedGeometry.empty());
    const double geometryTolerance =
        std::max(1e-8 * scale, 64.0 * std::numeric_limits<double>::epsilon() *
                                   std::max(std::abs(translation.x),
                                            std::abs(translation.y)));
    const auto close = [geometryTolerance](const double a, const double b) {
      return std::abs(a - b) <= geometryTolerance;
    };
    const auto closeVector = [&](const RS_Vector &a, const RS_Vector &b) {
      return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
    };
    const auto sameGeometry = [&](const RS_Entity &expectedEntity,
                                  const RS_Entity &actualEntity) {
      if (expectedEntity.rtti() != actualEntity.rtti())
        return false;
      if (const auto *expectedSpline =
              dynamic_cast<const RS_Spline *>(&expectedEntity)) {
        const auto *actualSpline =
            dynamic_cast<const RS_Spline *>(&actualEntity);
        if (!actualSpline)
          return false;
        const auto &expectedData = expectedSpline->getData();
        const auto &actualData = actualSpline->getData();
        if (expectedData.degree != actualData.degree ||
            expectedData.m_closedFlag != actualData.m_closedFlag ||
            expectedData.m_periodicFlag != actualData.m_periodicFlag ||
            expectedData.controlPoints.size() !=
                actualData.controlPoints.size() ||
            expectedData.knotslist.size() != actualData.knotslist.size() ||
            expectedData.weights.size() != actualData.weights.size())
          return false;
        for (std::size_t i = 0; i < expectedData.controlPoints.size(); ++i)
          if (!closeVector(expectedData.controlPoints[i],
                           actualData.controlPoints[i]))
            return false;
        for (std::size_t i = 0; i < expectedData.knotslist.size(); ++i)
          if (!close(expectedData.knotslist[i], actualData.knotslist[i]))
            return false;
        for (std::size_t i = 0; i < expectedData.weights.size(); ++i)
          if (!close(expectedData.weights[i], actualData.weights[i]))
            return false;
        return true;
      }
      if (const auto *expectedArc =
              dynamic_cast<const RS_Arc *>(&expectedEntity)) {
        const auto *actualArc = dynamic_cast<const RS_Arc *>(&actualEntity);
        if (!actualArc)
          return false;
        const auto &expectedData = expectedArc->getData();
        const auto &actualData = actualArc->getData();
        return closeVector(expectedData.center, actualData.center) &&
               close(expectedData.radius, actualData.radius) &&
               close(expectedData.angle1, actualData.angle1) &&
               close(expectedData.angle2, actualData.angle2) &&
               expectedData.reversed == actualData.reversed;
      }
      return false;
    };
    for (std::size_t lane = 0; lane < formats.size(); ++lane) {
      const bool dwg = formats[lane] == RS2::FormatDWG ||
                       formats[lane] == RS2::FormatDWG2018;
#ifndef DWGSUPPORT
      if (dwg)
        continue;
#endif
      const QString suffix =
          dwg ? QStringLiteral("dwg") : QStringLiteral("dxf");
      const QString fileName = directory.filePath(
          QStringLiteral("reconstructed-%1-%2.%3")
              .arg(QStringLiteral("frame-%1-%2-%3")
                       .arg(&frame - frames.data())
                       .arg(frame.denseEnd ? QStringLiteral("dense")
                                           : QStringLiteral("regular"))
                       .arg(frame.profile))
              .arg(lane)
              .arg(suffix));
      const std::filesystem::path path(fileName.toStdString());
      REQUIRE(lc::test::exportDrawing(f.graphic, path, formats[lane]));
#ifdef DWGSUPPORT
      if (dwg && !libreDwgReader.isEmpty()) {
        QProcess reader;
        reader.start(libreDwgReader,
                     {"-O", "minJSON", lc::test::toQString(path)});
        REQUIRE(reader.waitForStarted());
        REQUIRE(reader.waitForFinished(30000));
        INFO(reader.readAllStandardError().toStdString());
        REQUIRE(reader.exitStatus() == QProcess::NormalExit);
        REQUIRE(reader.exitCode() == 0);
        const QJsonDocument document =
            QJsonDocument::fromJson(reader.readAllStandardOutput());
        REQUIRE(document.isObject());
        CHECK_FALSE(document.object().value("OBJECTS").toArray().isEmpty());
      }
#endif
      RS_Graphic reopened;
      REQUIRE(lc::test::importDrawing(reopened, path, formats[lane]));
      const auto reopenedEntities = live(reopened);
      CHECK(std::none_of(reopenedEntities.begin(), reopenedEntities.end(),
                         [](const RS_Entity *entity) {
                           return entity->rtti() == RS2::EntityOffsetCurve;
                         }));
      std::vector<const RS_Entity *> reopenedGeometry;
      for (const RS_Entity *entity : reopenedEntities)
        if (entity->rtti() == RS2::EntitySpline ||
            entity->rtti() == RS2::EntityArc)
          reopenedGeometry.push_back(entity);
      std::vector<bool> matched(reopenedGeometry.size(), false);
      for (const RS_Entity *expectedEntity : expectedGeometry) {
        bool found = false;
        for (std::size_t i = 0; i < reopenedGeometry.size(); ++i) {
          if (matched[i] ||
              !sameGeometry(*expectedEntity, *reopenedGeometry[i]))
            continue;
          matched[i] = true;
          found = true;
          break;
        }
        CHECK(found);
      }
    }
  }
}

TEST_CASE("T Trim Two trims an endpoint-tangent parabola",
          "[gui][lc-offsetcurve][trim-action][typed-source][parabola][tangent]["
          "trim-two]") {
  REQUIRE(lc::test::ensureSettings());
  Drawing<true> f;
  const bool hasRequestedProfile =
      qEnvironmentVariableIsSet("LC_OFFSET_TRIM_PROFILE");
  const int profile = hasRequestedProfile
      ? qEnvironmentVariableIntValue("LC_OFFSET_TRIM_PROFILE")
      : GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
               19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
               35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
               51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
               67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82,
               83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
               99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
               112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
               125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137,
               138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150,
               151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162,
               163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174,
               175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186,
               187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198,
               199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210,
               211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222);
  const bool obliqueCurvatureProfile = profile >= 51 && profile <= 68;
  const bool shallowBendCurvatureProfile = profile >= 69 && profile <= 86;
  const bool nearVerticalCurvatureProfile = profile >= 87 && profile <= 104;
  const bool oppositeBendCurvatureProfile = profile >= 105 && profile <= 122;
  const bool backtrackingCurvatureProfile = profile >= 123 && profile <= 140;
  const bool backtrackingCuspProfile = profile >= 141 && profile <= 152;
  const bool strongBacktrackingCurvatureProfile =
      profile >= 153 && profile <= 170;
  const bool strongBacktrackingCuspProfile = profile >= 171 && profile <= 182;
  const bool backtrackingEndCurvatureProfile = profile >= 183 && profile <= 200;
  const bool backtrackingEndCuspProfile = profile >= 201 && profile <= 212;
  const bool curvatureCuspProfile =
      backtrackingCuspProfile || strongBacktrackingCuspProfile ||
      backtrackingEndCuspProfile;
  const bool curvatureActionProfile =
      obliqueCurvatureProfile || shallowBendCurvatureProfile ||
      nearVerticalCurvatureProfile || oppositeBendCurvatureProfile ||
      backtrackingCurvatureProfile || backtrackingCuspProfile ||
      strongBacktrackingCurvatureProfile || strongBacktrackingCuspProfile ||
      backtrackingEndCurvatureProfile || backtrackingEndCuspProfile;
  const int curvatureProfileIndex =
      profile <= 59 ? profile - 51
      : profile <= 68 ? profile - 60
      : profile <= 77 ? profile - 69
      : profile <= 86 ? profile - 78
      : profile <= 95 ? profile - 87
      : profile <= 104 ? profile - 96
      : profile <= 113 ? profile - 105
      : profile <= 122 ? profile - 114
      : profile <= 131 ? profile - 123
      : profile <= 140 ? profile - 132
      : profile <= 146 ? profile - 141
      : profile <= 152 ? profile - 147
      : profile <= 170 ? profile - 153
      : profile <= 182 ? profile - 171
      : profile <= 200 ? profile - 183
                       : profile - 201;
  const int curvatureScaleIndex = backtrackingCuspProfile
                                      ? curvatureProfileIndex / 2
                                  : strongBacktrackingCuspProfile
                                      ? curvatureProfileIndex / 4
                                  : backtrackingEndCuspProfile
                                      ? curvatureProfileIndex / 4
                                  : strongBacktrackingCurvatureProfile
                                      ? curvatureProfileIndex / 6
                                  : backtrackingEndCurvatureProfile
                                      ? curvatureProfileIndex / 6
                                      : curvatureProfileIndex / 3;
  const int curvatureRatioIndex = backtrackingCuspProfile
                                      ? curvatureProfileIndex % 2
                                  : strongBacktrackingCuspProfile
                                      ? (curvatureProfileIndex % 4) / 2
                                  : backtrackingEndCuspProfile
                                      ? (curvatureProfileIndex % 4) / 2
                                  : strongBacktrackingCurvatureProfile
                                      ? (curvatureProfileIndex % 6) / 2
                                  : backtrackingEndCurvatureProfile
                                      ? (curvatureProfileIndex % 6) / 2
                                      : curvatureProfileIndex % 3;
  const bool reversedCurvatureProfile =
      (profile >= 60 && profile <= 68) || (profile >= 78 && profile <= 86) ||
      (profile >= 96 && profile <= 104) ||
      (profile >= 114 && profile <= 122) ||
      (profile >= 132 && profile <= 140) ||
      (profile >= 147 && profile <= 152) ||
      ((strongBacktrackingCurvatureProfile ||
        strongBacktrackingCuspProfile) &&
       (curvatureProfileIndex % 2) == 1) ||
      (backtrackingEndCurvatureProfile &&
       (curvatureProfileIndex % 2) == 1) ||
      (backtrackingEndCuspProfile &&
       (curvatureProfileIndex % 2) == 1);
  const bool highOrigin = profile == 2;
  const bool highOriginLargeScale =
      profile >= 213 && profile <= 222;
  const bool highOriginBacktracking = profile == 214;
  const bool scaleExtreme = profile == 1;
  const bool reversedTangentTraversal =
      profile == 4 || profile == 6 || profile == 8 || profile == 10 ||
      profile == 12 || profile == 14 || profile == 16 || profile == 18 ||
      profile == 20 || profile == 22 || (profile >= 39 && profile <= 50) ||
      reversedCurvatureProfile;
  const bool steepAsymmetricTangents =
      (profile >= 23 && profile <= 50) || highOriginLargeScale ||
      curvatureActionProfile;
  const bool asymmetricTangents = profile == 3 || reversedTangentTraversal ||
                                  steepAsymmetricTangents ||
                                  (profile >= 5 && profile <= 22);
  const bool oppositeBend = (profile >= 5 && profile <= 16) ||
                            oppositeBendCurvatureProfile;
  const bool multiRadiusTangentProfile =
      oppositeBend || backtrackingCurvatureProfile || backtrackingCuspProfile ||
      strongBacktrackingCurvatureProfile || strongBacktrackingCuspProfile ||
      backtrackingEndCurvatureProfile || backtrackingEndCuspProfile;
  const bool lineCutter = (profile >= 11 && profile <= 22) || profile == 26 ||
                          (profile >= 29 && profile <= 34) ||
                          (profile >= 45 && profile <= 50) ||
                          (curvatureActionProfile && curvatureRatioIndex == 1);
  const bool asymmetricSmallScale =
      profile == 7 || profile == 8 || profile == 13 || profile == 14 ||
      profile == 19 || profile == 20 || profile == 24 || profile == 31 ||
      profile == 33 || profile == 35 || profile == 37 || profile == 40 ||
      profile == 43 || profile == 46 || profile == 49 ||
      (curvatureActionProfile && curvatureScaleIndex == 0);
  const bool asymmetricLargeScale =
      profile == 9 || profile == 10 || profile == 15 || profile == 16 ||
      profile == 21 || profile == 22 || profile == 25 || profile == 32 ||
      profile == 34 || profile == 36 || profile == 38 || profile == 41 ||
      profile == 44 || profile == 47 || profile == 50 ||
      highOriginLargeScale ||
      (curvatureActionProfile && curvatureScaleIndex == 2);
  const int scaleProfile = scaleExtreme ? GENERATE(0, 1) : 0;
  const double profileScale = scaleExtreme ? (scaleProfile == 0 ? 1e-3 : 1e3)
                              : asymmetricSmallScale ? 1e-3
                              : asymmetricLargeScale ? 1e3
                                                     : 1.0;
  RS_Ellipse seed(
      nullptr,
      RS_EllipseData{{0, 0}, {1000.0 * profileScale, 0}, 1, 0, .1, false});
  const RS_Vector sourceTranslation =
      highOrigin ? RS_Vector{1e9, -2e9}
      : highOriginLargeScale ? RS_Vector{1e12, -2e12}
      : scaleExtreme
          ? (scaleProfile == 0 ? RS_Vector{1e3, -2e3} : RS_Vector{1e9, -2e9})
      : asymmetricSmallScale ? RS_Vector{1e3, -2e3}
      : asymmetricLargeScale ? RS_Vector{1e9, -2e9}
      : asymmetricTangents   ? RS_Vector{1e6, -2e6}
      : profile == 0 ? GENERATE(RS_Vector{0.0, 0.0}, RS_Vector{1e6, -2e6},
                                RS_Vector{1e8, -2e8})
                     : RS_Vector{0.0, 0.0};
  const bool towardCentre =
      highOrigin || sourceTranslation.x != 0.0 ? GENERATE(false, true) : false;
  auto initial = LC_OffsetCurve::create(seed, 0.5 * profileScale, towardCentre);
  REQUIRE(initial);
  auto *previous = initial.entity.release();
  f.graphic.addEntity(previous);

  const double tangentSlope =
      highOrigin ? GENERATE(1e-4, 1e-5, 1e-6, 1e-7, 1e-8, 5e-9, 2e-9, 1.5e-9,
                            1.25e-9, 1.125e-9, 1.10625e-9, 1.1046875e-9,
                            1.1044921875e-9, 1.1044677734375e-9)
      : scaleExtreme               ? (scaleProfile == 0 ? 1e-6 : 1e-8)
      : sourceTranslation.x == 0.0 ? GENERATE(0.2, 1e-8, 1e-10, 1e-12, 1e-14)
                                   : 1e-8;
  const bool includeHighOriginCircle = highOrigin && tangentSlope == 1e-8;
  const bool tangentCircleBoundary =
      steepAsymmetricTangents || (scaleExtreme && scaleProfile == 1) ||
      profile == 5 || profile == 6 || includeHighOriginCircle;
  const double sourceRotation = highOrigin ? 0.371234567890123
                                : sourceTranslation.x == 0.0
                                    ? GENERATE(0., 0.371234567890123)
                                    : 0.371234567890123;
  CAPTURE(profile, scaleProfile, asymmetricTangents, reversedTangentTraversal,
          oppositeBend, asymmetricSmallScale, asymmetricLargeScale,
          sourceTranslation, profileScale, towardCentre, tangentSlope,
          sourceRotation);
  const double endpointSpacing =
      scaleExtreme ? (scaleProfile == 0 ? 1e-3 : 1e5) : 10.0 * profileScale;
  const std::array<std::array<RS_Vector, 2>, 6> additionalHighOriginLayouts{{
      {{{0.2, 0.9797958971132712}, {0.9, -0.4358898943540673}}},
      {{{0.6, 0.8}, {0.9, -0.4358898943540673}}},
      {{{1.0, 0.2}, {0.7, -0.35}}},
      {{{1.0, -0.4}, {0.8, 0.25}}},
      {{{-0.2, 0.9797958971132712}, {0.9, -0.4358898943540673}}},
      {{{0.9, 0.4358898943540673}, {-0.2, -0.9797958971132712}}},
  }};
  std::array<RS_Vector, 2> endpointTangents =
      steepAsymmetricTangents
          ? highOriginBacktracking
                ? std::array<RS_Vector, 2>{{{-0.6, 0.8},
                                            {0.9, -0.4358898943540673}}}
          : profile == 215
              ? std::array<RS_Vector, 2>{{{0.8, -0.6}, {0.6, 0.8}}}
          : profile == 216
              ? std::array<RS_Vector, 2>{{{0.2, 0.9797958971132712},
                                          {0.2, -0.9797958971132712}}}
          : profile >= 217 && profile <= 222
              ? additionalHighOriginLayouts[static_cast<std::size_t>(profile - 217)]
          : profile <= 26 || highOriginLargeScale
                ? std::array<RS_Vector, 2>{{{0.8, 0.6}, {0.6, -0.8}}}
            : (profile >= 39 && profile <= 41) ||
                    (profile >= 45 && profile <= 47)
                ? std::array<RS_Vector, 2>{{{-0.6, -0.8}, {-0.8, 0.6}}}
            : profile >= 39
                ? std::array<RS_Vector, 2>{{{-0.9, 0.4358898943540673},
                                            {-0.2, -0.9797958971132712}}}
            : (profile == 27 || profile == 29 || profile == 31 ||
               profile == 32 || profile == 35 || profile == 36)
                ? std::array<RS_Vector, 2>{{{0.8, -0.6}, {0.6, 0.8}}}
                : std::array<RS_Vector, 2>{{{0.2, 0.9797958971132712},
                                            {0.9, -0.4358898943540673}}}
      : profile == 4 || profile == 18 || profile == 20 || profile == 22
          ? std::array<RS_Vector, 2>{{{-0.7, 0.35}, {-1.0, -0.2}}}
      : reversedTangentTraversal
          ? std::array<RS_Vector, 2>{{{-0.8, -0.25}, {-1.0, 0.4}}}
      : oppositeBend ? std::array<RS_Vector, 2>{{{1.0, -0.4}, {0.8, 0.25}}}
      : asymmetricTangents
          ? std::array<RS_Vector, 2>{{{1.0, 0.2}, {0.7, -0.35}}}
          : std::array<RS_Vector, 2>{
                {{1.0, tangentSlope}, {1.0, -tangentSlope}}};
  if (obliqueCurvatureProfile) {
    endpointTangents = reversedCurvatureProfile
        ? std::array<RS_Vector, 2>{{{-0.9, 0.4358898943540673}, {-0.6, -0.8}}}
        : std::array<RS_Vector, 2>{{{0.6, 0.8}, {0.9, -0.4358898943540673}}};
  } else if (shallowBendCurvatureProfile) {
    endpointTangents = reversedCurvatureProfile
        ? std::array<RS_Vector, 2>{{{-0.7, 0.35}, {-1.0, -0.2}}}
        : std::array<RS_Vector, 2>{{{1.0, 0.2}, {0.7, -0.35}}};
  } else if (nearVerticalCurvatureProfile) {
    endpointTangents = reversedCurvatureProfile
        ? std::array<RS_Vector, 2>{{{-0.2, 0.9797958971132712},
                                    {-0.2, -0.9797958971132712}}}
        : std::array<RS_Vector, 2>{{{0.2, 0.9797958971132712},
                                    {0.2, -0.9797958971132712}}};
  } else if (oppositeBendCurvatureProfile) {
    endpointTangents = reversedCurvatureProfile
        ? std::array<RS_Vector, 2>{{{-0.8, -0.25}, {-1.0, 0.4}}}
        : std::array<RS_Vector, 2>{{{1.0, -0.4}, {0.8, 0.25}}};
  } else if (backtrackingCurvatureProfile) {
    endpointTangents = reversedCurvatureProfile
        ? std::array<RS_Vector, 2>{{{-0.9, 0.4358898943540673},
                                    {0.2, -0.9797958971132712}}}
        : std::array<RS_Vector, 2>{{{-0.2, 0.9797958971132712},
                                    {0.9, -0.4358898943540673}}};
  } else if (backtrackingCuspProfile) {
    endpointTangents = reversedCurvatureProfile
        ? std::array<RS_Vector, 2>{{{-0.9, 0.4358898943540673},
                                    {0.2, -0.9797958971132712}}}
        : std::array<RS_Vector, 2>{{{-0.2, 0.9797958971132712},
                                    {0.9, -0.4358898943540673}}};
  } else if (strongBacktrackingCurvatureProfile ||
             strongBacktrackingCuspProfile) {
    endpointTangents = reversedCurvatureProfile
        ? std::array<RS_Vector, 2>{{{-0.9, 0.4358898943540673},
                                    {0.6, -0.8}}}
        : std::array<RS_Vector, 2>{{{-0.6, 0.8},
                                    {0.9, -0.4358898943540673}}};
  } else if (backtrackingEndCurvatureProfile ||
             backtrackingEndCuspProfile) {
    endpointTangents = reversedCurvatureProfile
        ? std::array<RS_Vector, 2>{{{0.2, 0.9797958971132712},
                                    {-0.9, -0.4358898943540673}}}
        : std::array<RS_Vector, 2>{{{0.9, 0.4358898943540673},
                                    {-0.2, -0.9797958971132712}}};
  }
  LC_ParabolaData sourceData = LC_ParabolaData::FromEndPointsTangents(
      reversedTangentTraversal
          ? std::array<RS_Vector, 2>{{{endpointSpacing, 0.0}, {0.0, 0.0}}}
          : std::array<RS_Vector, 2>{{{0.0, 0.0}, {endpointSpacing, 0.0}}},
      endpointTangents);
  REQUIRE(sourceData.isValid());
  if (curvatureActionProfile) {
    constexpr std::array<double, 3> curvatureRatios{{0.1, 0.5, 0.9}};
    const double vertexRadius =
        2.0 * sourceData.m_focus.distanceTo(sourceData.m_vertex);
    REQUIRE(std::isfinite(vertexRadius));
    REQUIRE(vertexRadius > 0.0);
    auto* seedOffset = dynamic_cast<LC_OffsetCurve*>(previous);
    REQUIRE(seedOffset);
    const double curvatureRatio = curvatureCuspProfile
        ? (curvatureRatioIndex == 0 ? 1.0 : 1.02)
        : curvatureRatios[curvatureRatioIndex];
    const double offsetDistance = curvatureRatio * vertexRadius;
    const auto distanceEdit = seedOffset->setDistance(offsetDistance);
    REQUIRE(distanceEdit);
    CHECK(seedOffset->definition().distance == offsetDistance);
  }
  const double cosine = std::cos(sourceRotation);
  const double sine = std::sin(sourceRotation);
  for (auto &point : sourceData.m_controlPoints) {
    point = {cosine * point.x - sine * point.y + sourceTranslation.x,
             sine * point.x + cosine * point.y + sourceTranslation.y};
  }
  auto *parabola = new LC_Parabola(&f.graphic, sourceData);
  f.graphic.addEntity(parabola);
  f.graphic.select(QList<RS_Entity *>{previous}, true);

  UpdateSourceProbe update(&f.context);
  update.init(0);
  update.setSourceKind(LC_OffsetSourceGeometry::Kind::Parabola);
  update.use(parabola);
  lc::test::awaitUpdate(update);
  REQUIRE(update.lastReport().updated == 1);
  REQUIRE(previous->isDeleted());

  LC_OffsetCurve *native = nullptr;
  for (RS_Entity *entity : live(f.graphic)) {
    if (entity->rtti() != RS2::EntityOffsetCurve)
      continue;
    REQUIRE(native == nullptr);
    native = dynamic_cast<LC_OffsetCurve *>(entity);
  }
  REQUIRE(native);
  REQUIRE(native->sourceDefinition());
  CHECK(native->sourceDefinition()->source.kind() ==
        LC_OffsetSourceGeometry::Kind::Parabola);
  const auto *retainedSource =
      std::get_if<LC_ParabolaData>(&native->sourceDefinition()->source.value);
  REQUIRE(retainedSource);
  for (std::size_t i = 0; i < sourceData.m_controlPoints.size(); ++i)
    CHECK(retainedSource->m_controlPoints[i].distanceTo(
              sourceData.m_controlPoints[i]) == 0.0);

  const auto breaks = native->getBreakParameters();
  REQUIRE(breaks.size() >= 2);
  const double parameter = (breaks.front() + breaks[1]) * .5;
  const RS_Vector intersection = native->getPointAt(parameter);
  const double step = std::max(1e-7, (breaks[1] - breaks.front()) * 1e-5);
  RS_Vector tangent = native->getPointAt(parameter + step) -
                      native->getPointAt(parameter - step);
  REQUIRE(tangent.magnitude() > RS_TOLERANCE);
  tangent /= tangent.magnitude();
  const RS_Vector normal{-tangent.y, tangent.x};
  const double radius = (multiRadiusTangentProfile || steepAsymmetricTangents) && !lineCutter
                            ? GENERATE(.05, .1, .25) * profileScale
                            : .1 * profileScale;
  const bool cutterFirst = lineCutter ? GENERATE(false, true) : false;
  CAPTURE(cutterFirst);
  const RS_Vector circleCenter = intersection + tangent * radius;
  RS_Entity *cutter = nullptr;
  RS_Vector cutterPick;
  if (scaleExtreme || lineCutter) {
    cutter = new RS_Line(&f.graphic,
                         RS_LineData{intersection - normal * radius * 2.0,
                                     intersection + normal * radius * 2.0});
    cutterPick = intersection + normal * radius;
  } else {
    cutter = new RS_Circle(&f.graphic, RS_CircleData{circleCenter, radius});
    cutterPick = circleCenter + normal * radius;
  }
  f.graphic.addEntity(cutter);
  const RS_Vector nativePick =
      native->getPointAt(breaks.front() + (breaks[1] - breaks.front()) *
                                              (scaleExtreme ? .25 : .9));
  const auto nativeBefore = trimGeometry(*native);
  const auto cutterBefore = trimGeometry(*cutter);

  LC_OffsetCacheTrim::Work trimWork;
  const auto directTrim = LC_OffsetCacheTrim::prepare(
      native->getData(), *cutter, {nativePick}, {}, {}, &trimWork);
  CAPTURE(profile, curvatureScaleIndex, curvatureRatioIndex, intersection,
          nativePick, cutterPick, radius, directTrim.refusal,
          trimWork.hasIntersectionFailureDomain,
          trimWork.intersections.totalBoxPairs,
          trimWork.intersections.generatedLeaves);
  if (curvatureCuspProfile &&
      directTrim.refusal == LC_OffsetCacheTrim::Refusal::LimitExceeded) {
    CHECK(directTrim.fragments.empty());
    CHECK(directTrim.conicFragments.empty());
    CHECK(directTrim.fragmentCount == 0);
    CHECK_FALSE(directTrim.removed);
    CHECK(trimGeometry(*native) == nativeBefore);
    CHECK(trimGeometry(*cutter) == cutterBefore);
    f.action->click(nativePick);
    REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
    f.action->hover(cutterPick);
    REQUIRE(f.action->m_preview);
    CHECK(trimPreview(*f.action->m_preview).empty());
    f.action->click(cutterPick);
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    CHECK(trimGeometry(*native) == nativeBefore);
    CHECK(trimGeometry(*cutter) == cutterBefore);
    return;
  }
  REQUIRE(directTrim.refusal == LC_OffsetCacheTrim::Refusal::None);
  CHECK(trimWork.intersections.deepestPairRefinement <=
        LC_OffsetCacheTrim::Limits{}.maxDepth);

  if (scaleExtreme) {
    const RS_Vector circlePick = circleCenter + normal * radius;
    const auto oracle = exactCircleBezierOracle(native->getData(), circleCenter,
                                                radius, circlePick);
    CAPTURE(scaleProfile, sourceTranslation, profileScale, towardCentre,
            oracle.distinctRoots, oracle.repeatedRoots,
            oracle.minimumSeparation, oracle.selectedArcSweep,
            oracle.selectedArcMargin);
    REQUIRE(oracle.valid);
    CHECK(oracle.distinctRoots == 2);
    CHECK(oracle.repeatedRoots == 0);
    CHECK(oracle.minimumSeparation > radius * 1e-6);
    CHECK(oracle.selectedArcSweep > 0);
    CHECK(oracle.selectedArcSweep < 2 * M_PI);
    CHECK(oracle.selectedArcMargin > 2 * oracle.maximumAngularError + 1e-12);
  }

  if (tangentCircleBoundary) {
    const RS_Vector circlePick = circleCenter + normal * radius;
    const RS_Vector tangentCircleCenter = intersection + normal * radius;
    RS_Circle tangentCircle{nullptr, {tangentCircleCenter, radius}};
    const auto tangentOracle =
        exactCircleBezierOracle(native->getData(), tangentCircleCenter, radius,
                                tangentCircleCenter + normal * radius);
    CAPTURE(tangentOracle.valid, tangentOracle.distinctRoots,
            tangentOracle.repeatedRoots, tangentOracle.minimumSeparation);
    const auto tangentTrim = LC_OffsetCacheTrim::prepareConic(
        native->getData(), tangentCircle,
        {tangentCircleCenter + normal * radius});
    CAPTURE(tangentTrim.refusal, tangentTrim.rootCount,
            tangentTrim.work.failureStage,
            tangentTrim.work.hasIntersectionFailureDomain,
            tangentTrim.work.intersections.totalBoxPairs,
            tangentTrim.work.intersections.generatedLeaves,
            tangentTrim.work.intersections.generatedItems,
            tangentTrim.work.intersections.orderingComparisons,
            tangentTrim.work.intersectionFailureDepth);
    const bool independentlySeparatedSecant =
        tangentOracle.valid && tangentOracle.distinctRoots == 2 &&
        tangentOracle.repeatedRoots == 0 &&
        tangentOracle.minimumSeparation > radius * 1e-6;
    if (independentlySeparatedSecant) {
      CHECK(tangentTrim.refusal == LC_OffsetCacheTrim::Refusal::None);
      CHECK(tangentTrim.rootCount == 2);
      REQUIRE(tangentTrim.conicFragments.size() == 1);
      CHECK(tangentTrim.fragments.empty());
    } else {
      CHECK(tangentTrim.refusal != LC_OffsetCacheTrim::Refusal::None);
      CHECK(tangentTrim.fragments.empty());
      CHECK(tangentTrim.conicFragments.empty());
      CHECK(tangentTrim.fragmentCount == 0);
      CHECK_FALSE(tangentTrim.removed);
    }
    CHECK(trimGeometry(*native) == nativeBefore);

    if (includeHighOriginCircle && !towardCentre) {
      for (const double inwardFraction : {1e-2, 1e-3, 1e-4, 1e-5, 1e-6}) {
        const RS_Vector nearCenter =
            tangentCircleCenter - normal * (radius * inwardFraction);
        RS_Circle nearCircle{nullptr, {nearCenter, radius}};
        const RS_Vector nearPick = nearCenter + normal * radius;
        const auto oracle = exactCircleBezierOracle(
            native->getData(), nearCenter, radius, nearPick);
        CAPTURE(inwardFraction, oracle.valid, oracle.distinctRoots,
                oracle.repeatedRoots, oracle.minimumSeparation);
        REQUIRE(oracle.valid);
        CHECK(oracle.distinctRoots == 2);
        CHECK(oracle.repeatedRoots == 0);
        LC_OffsetCacheTrim::Work nearWork;
        const auto nearTrim = LC_OffsetCacheTrim::prepareConic(
            native->getData(), nearCircle, {nearPick}, {}, {}, {}, &nearWork);
        CAPTURE(nearTrim.refusal, nearTrim.rootCount, nearWork.failureStage,
                nearWork.intersections.deepestPairRefinement,
                nearWork.intersections.deepestRootRefinement,
                nearWork.intersections.totalBoxPairs,
                nearWork.intersections.generatedLeaves);
        CHECK(nearTrim.refusal == LC_OffsetCacheTrim::Refusal::None);
        CHECK(nearTrim.rootCount == 2);
        REQUIRE(nearTrim.conicFragments.size() == 1);
        CHECK(trimGeometry(*native) == nativeBefore);
      }

      for (const double inwardFraction : {5e-7, 1e-7, 1e-8}) {
        const RS_Vector nearCenter =
            tangentCircleCenter - normal * (radius * inwardFraction);
        RS_Circle nearCircle{nullptr, {nearCenter, radius}};
        const RS_Vector nearPick = nearCenter + normal * radius;
        const auto oracle = exactCircleBezierOracle(
            native->getData(), nearCenter, radius, nearPick);
        CAPTURE(inwardFraction, oracle.valid, oracle.distinctRoots,
                oracle.repeatedRoots, oracle.minimumSeparation);
        const bool independentlySeparatedSecant =
            oracle.valid && oracle.distinctRoots == 2 &&
            oracle.repeatedRoots == 0 &&
            oracle.minimumSeparation > radius * 1e-6;
        LC_OffsetCacheTrim::Work nearWork;
        const auto nearTrim = LC_OffsetCacheTrim::prepareConic(
            native->getData(), nearCircle, {nearPick}, {}, {}, {}, &nearWork);
        CAPTURE(nearTrim.refusal, nearTrim.rootCount, nearWork.failureStage,
                nearWork.intersections.deepestPairRefinement,
                nearWork.intersections.deepestRootRefinement,
                nearWork.intersections.totalBoxPairs,
                nearWork.intersections.generatedLeaves);
        if (independentlySeparatedSecant) {
          CHECK(nearTrim.refusal == LC_OffsetCacheTrim::Refusal::None);
          CHECK(nearTrim.rootCount == 2);
          REQUIRE(nearTrim.conicFragments.size() == 1);
        } else {
          CHECK(nearTrim.refusal != LC_OffsetCacheTrim::Refusal::None);
          CHECK(nearTrim.rootCount == 0);
          CHECK(nearTrim.fragments.empty());
          CHECK(nearTrim.conicFragments.empty());
          CHECK(nearTrim.fragmentCount == 0);
          CHECK_FALSE(nearTrim.removed);
        }
        CHECK(trimGeometry(*native) == nativeBefore);
      }
    }

    auto *circle =
        new RS_Circle(&f.graphic, RS_CircleData{circleCenter, radius});
    f.graphic.addEntity(circle);
    const auto cacheOnly =
        LC_OffsetCacheTrim::prepare(native->getData(), *circle, {nativePick});
    const auto conicOnly = LC_OffsetCacheTrim::prepareConic(
        native->getData(), *circle, {circlePick});
    const auto circleOracle = exactCircleBezierOracle(
        native->getData(), circleCenter, radius, circlePick);
    CAPTURE(cacheOnly.refusal, cacheOnly.rootCount, cacheOnly.fragments.size(),
            conicOnly.refusal, conicOnly.rootCount,
            conicOnly.conicFragments.size(), conicOnly.work.failureStage,
            conicOnly.work.hasIntersectionFailureDomain,
            conicOnly.work.intersections.totalBoxPairs,
            conicOnly.work.intersections.generatedLeaves,
            circleOracle.valid, circleOracle.distinctRoots,
            circleOracle.repeatedRoots, circleOracle.minimumSeparation);
    LC_OffsetCacheTrim::Work circleWork;
    const auto circleTrim = LC_OffsetCacheTrim::prepareConic(
        native->getData(), *circle, {circlePick},
        LC_OffsetCacheTrim::Pick{nativePick}, {}, {}, &circleWork);
    CAPTURE(circleTrim.refusal, circleTrim.rootCount,
            circleTrim.conicFragments.size(), circleWork.failureStage,
            circleWork.hasIntersectionFailureDomain,
            circleWork.intersections.totalBoxPairs,
            circleWork.intersections.generatedLeaves,
            circleWork.intersections.deepestPairRefinement);
    CHECK(circleTrim.refusal == LC_OffsetCacheTrim::Refusal::None);
    CHECK(circleTrim.rootCount == 2);
    REQUIRE(circleTrim.conicFragments.size() == 1);
    CHECK_FALSE(circleWork.hasIntersectionFailureDomain);
    CHECK(cacheOnly.refusal == LC_OffsetCacheTrim::Refusal::None);
    CHECK(cacheOnly.rootCount == 2);
    CHECK_FALSE(cacheOnly.fragments.empty());
    const bool independentlySeparatedCircleSecant =
        circleOracle.valid && circleOracle.distinctRoots == 2 &&
        circleOracle.repeatedRoots == 0 &&
        circleOracle.minimumSeparation > radius * 1e-6;
    if (curvatureCuspProfile && !independentlySeparatedCircleSecant) {
      CHECK(conicOnly.refusal == LC_OffsetCacheTrim::Refusal::LimitExceeded);
      CHECK(conicOnly.fragments.empty());
      CHECK(conicOnly.conicFragments.empty());
      CHECK(conicOnly.fragmentCount == 0);
      CHECK_FALSE(conicOnly.removed);
    } else {
      CHECK(conicOnly.refusal == LC_OffsetCacheTrim::Refusal::None);
      CHECK(conicOnly.rootCount == 2);
      CHECK(conicOnly.work.failureStage == LC_OffsetCacheTrim::Stage::None);
      REQUIRE(conicOnly.conicFragments.size() == 1);
    }
    CHECK(trimGeometry(*native) == nativeBefore);

    LC_StoredCacheTrimOptions workerOptions;
    const auto limits =
        RS_Modification::storedCacheTrimRemainingLimits(workerOptions.kernel);
    REQUIRE(limits);
    const LC_OffsetWorkerRequest::Token token{
        1, 3, 17, qint64(f.graphic.undoStateEpoch())};
    const auto request = RS_Modification::storedCacheTrimRequest(
        *native, nativePick, *circle, circlePick, true, token, *limits);
    REQUIRE(request);
    const auto completion = joinedTrimWorker(*request);
    const auto &workerResult =
        std::get<LC_OffsetCacheTrim::Result>(completion->reply().result);
    CAPTURE(workerResult.refusal, workerResult.rootCount,
            workerResult.work.failureStage,
            workerResult.work.hasIntersectionFailureDomain,
            workerResult.work.intersections.totalBoxPairs,
            workerResult.work.intersections.generatedLeaves);
    CHECK(workerResult.refusal == LC_OffsetCacheTrim::Refusal::None);
    CHECK(workerResult.rootCount == 2);
    CHECK_FALSE(workerResult.work.hasIntersectionFailureDomain);

    LC_StoredCacheTrimCompletions received{*request, *completion};
    received.publicationUndoEpoch = f.graphic.undoStateEpoch();
    LC_DocumentModificationBatch preparedBatch;
    const auto preparedTrim = RS_Modification::trimStoredCachePrepared(
        f.graphic, *native, nativePick, *circle, circlePick, true,
        preparedBatch, received, workerOptions);
    REQUIRE(preparedTrim.result);
    CHECK_FALSE(preparedTrim.cacheRefusal);
    CHECK(preparedTrim.cacheWork.failureStage ==
          workerResult.work.failureStage);
    CHECK_FALSE(preparedTrim.cacheWork.hasIntersectionFailureDomain);
    REQUIRE(preparedBatch.entitiesToAdd.size() ==
            circleTrim.fragments.size() + circleTrim.conicFragments.size());
    CHECK(preparedBatch.entitiesToDelete.size() == 2);
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(circle->isDeleted());

    LC_DocumentModificationBatch circleBatch;
    const auto trimTwo = RS_Modification::trimStoredCache(
        f.graphic, *native, nativePick, *circle, circlePick, true, circleBatch);
    REQUIRE(trimTwo.result);
    CHECK_FALSE(trimTwo.cacheRefusal);
    CHECK_FALSE(trimTwo.cacheWork.hasIntersectionFailureDomain);
    REQUIRE(circleBatch.entitiesToAdd.size() ==
            preparedBatch.entitiesToAdd.size());
    CHECK(circleBatch.entitiesToDelete == preparedBatch.entitiesToDelete);
    for (qsizetype i = 0; i < circleBatch.entitiesToAdd.size(); ++i) {
      CHECK(circleBatch.entitiesToAdd[i]->rtti() ==
            preparedBatch.entitiesToAdd[i]->rtti());
      CHECK(trimGeometry(*circleBatch.entitiesToAdd[i]) ==
            trimGeometry(*preparedBatch.entitiesToAdd[i]));
    }

    f.action->click(nativePick);
    REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
    CHECK(f.action->caughtAt(circlePick) == circle);
    f.action->hover(circlePick);
    const auto circlePreview = trimPreview(*f.action->m_preview);
    REQUIRE(circlePreview.size() == preparedBatch.entitiesToAdd.size());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(circle->isDeleted());
    f.action->click(circlePick);
    CHECK(native->isDeleted());
    CHECK(circle->isDeleted());
    auto committedCircle = trimPreview(f.graphic);
    std::sort(committedCircle.begin(), committedCircle.end());
    auto expectedCircle = circlePreview;
    std::sort(expectedCircle.begin(), expectedCircle.end());
    CHECK(committedCircle == expectedCircle);
    std::vector<RS_Entity *> circleFragments;
    for (auto *entity : live(f.graphic))
      if (entity != native && entity != circle && entity != parabola &&
          entity != cutter)
        circleFragments.push_back(entity);
    REQUIRE(circleFragments.size() == circlePreview.size());
    REQUIRE(f.graphic.undo());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(circle->isDeleted());
    for (const auto *fragment : circleFragments)
      CHECK(fragment->isDeleted());
    REQUIRE(f.graphic.redo());
    CHECK(native->isDeleted());
    CHECK(circle->isDeleted());
    for (const auto *fragment : circleFragments)
      CHECK_FALSE(fragment->isDeleted());

    QTemporaryDir circleDirectory;
    REQUIRE(circleDirectory.isValid());
    const std::array<RS2::FormatType, 5> circleFormats{
        {RS2::FormatDXFRW14, RS2::FormatDXFRW2000, RS2::FormatDWG,
         RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
        QString circleLibreDwgReader = qEnvironmentVariable("DWGREAD");
        if (circleLibreDwgReader.isEmpty())
            circleLibreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
        for (std::size_t lane = 0; lane < circleFormats.size(); ++lane) {
            const auto format = circleFormats[lane];
            const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
#ifndef DWGSUPPORT
            if (dwg) continue;
#endif
            const auto file = std::filesystem::path(circleDirectory.filePath(
                QStringLiteral("high-origin-circle-trim-%1").arg(lane) +
                (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, format));
#ifdef DWGSUPPORT
            if (dwg && !circleLibreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(circleLibreDwgReader,
                    {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document =
                    QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                const QByteArray json = document.toJson(QJsonDocument::Compact);
                for (const RS_Entity* expected : circleFragments) {
                    const char* dwgType = expected->rtti() == RS2::EntitySpline ? "SPLINE" :
                        expected->rtti() == RS2::EntityLine ? "LINE" :
                        expected->rtti() == RS2::EntityArc ? "ARC" :
                        expected->rtti() == RS2::EntityEllipse ? "ELLIPSE" : nullptr;
                    REQUIRE(dwgType);
                    const QByteArray marker = QByteArrayLiteral("\"") + QByteArray{dwgType} +
                                              QByteArrayLiteral("\"");
                    CHECK(json.contains(marker));
                }
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, format));
            std::vector<const RS_Entity*> reopenedGeometry;
            for (const RS_Entity* entity : live(reopened)) {
                CHECK(entity->rtti() != RS2::EntityOffsetCurve);
                if (std::any_of(circleFragments.begin(), circleFragments.end(),
                    [entity](const RS_Entity* expected) { return expected->rtti() == entity->rtti(); }))
                    reopenedGeometry.push_back(entity);
            }
            CAPTURE(format, lane);
            CHECK(sameTrimGeometrySet(circleFragments, reopenedGeometry));
        }

        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(circle->isDeleted());
        for (const auto* fragment : circleFragments) CHECK(fragment->isDeleted());
        f.action = std::make_unique<CacheTrim<true>>(&f.context);

        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(circle->isDeleted());
        CHECK(circle->getCenter() == circleCenter);
        CHECK(circle->getRadius() == radius);
        REQUIRE(f.graphic.removeEntity(circle));
    }

    const RS_Vector firstPick = cutterFirst ? cutterPick : nativePick;
    const RS_Vector secondPick = cutterFirst ? nativePick : cutterPick;
    f.action->click(firstPick);
    REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
    CHECK(f.action->caughtAt(secondPick) == (cutterFirst ? native : cutter));
    f.action->hover(secondPick);
    const auto preview = trimPreview(*f.action->m_preview);
    REQUIRE_FALSE(preview.empty());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    CHECK(trimGeometry(*native) == nativeBefore);
    CHECK(trimGeometry(*cutter) == cutterBefore);

    f.action->click(secondPick);
    CHECK(native->isDeleted());
    CHECK(cutter->isDeleted());
    std::vector<RS_Entity*> fragments;
    std::vector<std::string> committed;
    std::vector<std::string> committedLines;
    for (RS_Entity* entity : live(f.graphic)) {
        if (entity == parabola)
            continue;
        fragments.push_back(entity);
        if (entity->rtti() == RS2::EntityLine)
            committedLines.push_back(trimGeometry(*entity));
        else if (entity->rtti() == RS2::EntitySpline || entity->rtti() == RS2::EntityArc)
            committed.push_back(trimGeometry(*entity));
    }
    auto expected = preview;
    std::sort(committed.begin(), committed.end());
    std::sort(expected.begin(), expected.end());
    CHECK(committed == expected);
    CHECK(committedLines.size() == ((scaleExtreme || lineCutter) ? 1u : 0u));
    CHECK_FALSE(fragments.empty());

    REQUIRE(f.graphic.undo());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    CHECK(trimGeometry(*native) == nativeBefore);
    CHECK(trimGeometry(*cutter) == cutterBefore);
    for (const RS_Entity* fragment : fragments)
        CHECK(fragment->isDeleted());
    REQUIRE(f.graphic.redo());
    CHECK(native->isDeleted());
    CHECK(cutter->isDeleted());
    for (const RS_Entity* fragment : fragments)
        CHECK_FALSE(fragment->isDeleted());

    if (highOrigin || scaleExtreme || asymmetricTangents) {
        QTemporaryDir directory;
        REQUIRE(directory.isValid());
        const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14,
            RS2::FormatDXFRW2000, RS2::FormatDWG, RS2::FormatDXFRW2018,
            RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
        QString libreDwgReader = qEnvironmentVariable("DWGREAD");
        if (libreDwgReader.isEmpty())
            libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
        std::vector<const RS_Entity*> expectedGeometry(fragments.begin(),
                                                       fragments.end());
        REQUIRE_FALSE(expectedGeometry.empty());

        const double geometryTolerance = highOriginLargeScale
            ? 128.0 * std::numeric_limits<double>::epsilon() * 2e12
            : scaleExtreme ? (scaleProfile == 0 ? 1e-7 : 1e-4)
            : asymmetricSmallScale ? 1e-7 : 1e-4;
        const auto closePoint = [=](const RS_Vector& a, const RS_Vector& b) {
            return a.valid && b.valid &&
                   a.distanceTo(b) <= geometryTolerance;
        };
        const auto sameEndpoints = [&](const RS_Entity& expected,
                                       const RS_Entity& actual) {
            const RS_Vector expectedStart = expected.getStartpoint();
            const RS_Vector expectedEnd = expected.getEndpoint();
            const RS_Vector actualStart = actual.getStartpoint();
            const RS_Vector actualEnd = actual.getEndpoint();
            return (closePoint(expectedStart, actualStart) &&
                    closePoint(expectedEnd, actualEnd)) ||
                   (closePoint(expectedStart, actualEnd) &&
                    closePoint(expectedEnd, actualStart));
        };
        const auto matchesGeometry = [&](const RS_Entity& expected,
                                         const RS_Entity& actual) {
            if (expected.rtti() != actual.rtti())
                return false;
            if (const auto* expectedLine = dynamic_cast<const RS_Line*>(&expected)) {
                const auto* actualLine = dynamic_cast<const RS_Line*>(&actual);
                return actualLine && sameEndpoints(*expectedLine, *actualLine);
            }
            if (const auto* expectedSpline = dynamic_cast<const RS_Spline*>(&expected)) {
                const auto* actualSpline = dynamic_cast<const RS_Spline*>(&actual);
                if (!actualSpline)
                    return false;
                double expectedStart = 0.0, expectedEnd = 0.0;
                double actualStart = 0.0, actualEnd = 0.0;
                if (!expectedSpline->getParameterDomain(expectedStart, expectedEnd) ||
                    !actualSpline->getParameterDomain(actualStart, actualEnd) ||
                    !(expectedEnd > expectedStart) || !(actualEnd > actualStart))
                    return false;
                bool matchesForward = true;
                bool matchesReverse = true;
                for (unsigned i = 0; i <= 32; ++i) {
                    const double fraction = static_cast<double>(i) / 32.0;
                    const RS_Vector expectedPoint = expectedSpline->getPointAt(
                        expectedStart + (expectedEnd - expectedStart) * fraction);
                    const RS_Vector actualForward = actualSpline->getPointAt(
                        actualStart + (actualEnd - actualStart) * fraction);
                    const RS_Vector actualReverse = actualSpline->getPointAt(
                        actualEnd - (actualEnd - actualStart) * fraction);
                    if (!expectedPoint.valid || !actualForward.valid ||
                        !actualReverse.valid)
                        return false;
                    matchesForward &= closePoint(expectedPoint, actualForward);
                    matchesReverse &= closePoint(expectedPoint, actualReverse);
                }
                return matchesForward || matchesReverse;
            }
            if (const auto* expectedArc = dynamic_cast<const RS_Arc*>(&expected)) {
                const auto* actualArc = dynamic_cast<const RS_Arc*>(&actual);
                return actualArc &&
                    closePoint(expectedArc->getCenter(), actualArc->getCenter()) &&
                    std::abs(expectedArc->getRadius() - actualArc->getRadius()) <=
                        geometryTolerance &&
                    std::abs(expectedArc->getLength() - actualArc->getLength()) <=
                        geometryTolerance &&
                    sameEndpoints(*expectedArc, *actualArc);
            }
            if (const auto* expectedEllipse = dynamic_cast<const RS_Ellipse*>(&expected)) {
                const auto* actualEllipse = dynamic_cast<const RS_Ellipse*>(&actual);
                return actualEllipse &&
                    closePoint(expectedEllipse->getCenter(), actualEllipse->getCenter()) &&
                    closePoint(expectedEllipse->getMajorP(), actualEllipse->getMajorP()) &&
                    std::abs(expectedEllipse->getRatio() - actualEllipse->getRatio()) <=
                        geometryTolerance &&
                    std::abs(expectedEllipse->getLength() - actualEllipse->getLength()) <=
                        geometryTolerance &&
                    sameEndpoints(*expectedEllipse, *actualEllipse);
            }
            return false;
        };

        for (std::size_t lane = 0; lane < formats.size(); ++lane) {
            const bool dwg = formats[lane] == RS2::FormatDWG ||
                             formats[lane] == RS2::FormatDWG2018;
#ifndef DWGSUPPORT
            if (dwg)
                continue;
#endif
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("endpoint-parabola-trim-%1")
                    .arg(lane) + (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[lane]));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader,
                    {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document =
                    QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                const QByteArray json = document.toJson(QJsonDocument::Compact);
                for (const RS_Entity* expected : expectedGeometry) {
                    const char* dwgType = expected->rtti() == RS2::EntitySpline ? "SPLINE" :
                        expected->rtti() == RS2::EntityLine ? "LINE" :
                        expected->rtti() == RS2::EntityArc ? "ARC" :
                        expected->rtti() == RS2::EntityEllipse ? "ELLIPSE" : nullptr;
                    REQUIRE(dwgType);
                    const QByteArray marker = QByteArrayLiteral("\"") + dwgType +
                                              QByteArrayLiteral("\"");
                    CHECK(json.contains(marker));
                }
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, formats[lane]));
            std::vector<const RS_Entity*> reopenedGeometry;
            for (const RS_Entity* entity : live(reopened)) {
                CHECK(entity->rtti() != RS2::EntityOffsetCurve);
                if (entity->rtti() == RS2::EntitySpline ||
                    entity->rtti() == RS2::EntityLine ||
                    entity->rtti() == RS2::EntityArc ||
                    entity->rtti() == RS2::EntityEllipse)
                    reopenedGeometry.push_back(entity);
            }
            CAPTURE(formats[lane]);
            std::vector<bool> matched(reopenedGeometry.size(), false);
            for (const RS_Entity* expected : expectedGeometry) {
                bool found = false;
                for (std::size_t i = 0; i < reopenedGeometry.size(); ++i) {
                    if (matched[i] ||
                        !matchesGeometry(*expected, *reopenedGeometry[i]))
                        continue;
                    matched[i] = true;
                    found = true;
                    break;
                }
                CHECK(found);
            }
        }
    }
}

TEST_CASE("T Trim Two trims exact-stall and multi-branch cusp caches",
          "[gui][lc-offsetcurve][trim-action][typed-source][parabola][cusp][trim-two]") {
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
    std::size_t fileIndex = 0;
    const auto close = [](double a, double b) {
        return std::abs(a - b) <= 2e-9 * std::max({1.0, std::abs(a), std::abs(b)});
    };
    const auto closeVector = [&](const RS_Vector& a, const RS_Vector& b) {
        return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
    };
    const auto sameGeometry = [&](const RS_Entity& expected, const RS_Entity& actual) {
        if (expected.rtti() != actual.rtti()) return false;
        if (const auto* a = dynamic_cast<const RS_Line*>(&expected)) {
            const auto* b = dynamic_cast<const RS_Line*>(&actual);
            return b && closeVector(a->getData().startpoint, b->getData().startpoint) &&
                closeVector(a->getData().endpoint, b->getData().endpoint);
        }
        if (const auto* a = dynamic_cast<const RS_Spline*>(&expected)) {
            const auto* b = dynamic_cast<const RS_Spline*>(&actual);
            if (!b) return false;
            const auto& x = a->getData();
            const auto& y = b->getData();
            if (x.degree != y.degree || x.m_closedFlag != y.m_closedFlag ||
                x.m_periodicFlag != y.m_periodicFlag || x.controlPoints.size() != y.controlPoints.size() ||
                x.knotslist.size() != y.knotslist.size() || x.weights.size() != y.weights.size()) return false;
            for (std::size_t i = 0; i < x.controlPoints.size(); ++i)
                if (!closeVector(x.controlPoints[i], y.controlPoints[i])) return false;
            for (std::size_t i = 0; i < x.knotslist.size(); ++i)
                if (!close(x.knotslist[i], y.knotslist[i])) return false;
            for (std::size_t i = 0; i < x.weights.size(); ++i)
                if (!close(x.weights[i], y.weights[i])) return false;
            return true;
        }
        return false;
    };
    for (const double distance : {0.5, 0.51})
    for (const bool circularCutter : {false, true})
    for (const bool parabolaFirst : {false, true}) {
        CAPTURE(distance, circularCutter, parabolaFirst);
        Drawing<true> f;
        RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false});
        auto initial = LC_OffsetCurve::create(seed, distance, false);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);
        const LC_ParabolaData sourceData{{RS_Vector{-1, 1}, RS_Vector{0, -1}, RS_Vector{1, 1}}};
        auto* parabola = new LC_Parabola(&f.graphic, sourceData);
        f.graphic.addEntity(parabola);
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Parabola);
        update.use(parabola);
        lc::test::awaitUpdate(update);
        REQUIRE(update.lastReport().updated == 1);
        REQUIRE(previous->isDeleted());
        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Parabola);
        CHECK(native->sourceDefinition()->signedDistance == distance);

        const auto direct = LC_CurveOffset::buildDirectBranches(*parabola,
            LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, distance),
            LC_CurveOffset::makeDirectOptions(*parabola, distance, 1e-4),
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(direct.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(direct.branches.size() == (distance == .5 ? 1u : 3u));

        const auto breaks = native->getBreakParameters();
        REQUIRE(breaks.size() >= 2);
        constexpr double cutterHalfLength = .01;
        const double spanWidth = breaks[1] - breaks[0];
        REQUIRE(spanWidth > 0);
        const double parameter = breaks[0] + spanWidth * .5;
        const double step = std::max(1e-8, spanWidth * 1e-4);
        RS_Vector tangent = native->getPointAt(parameter + step) - native->getPointAt(parameter - step);
        REQUIRE(tangent.magnitude() > RS_TOLERANCE);
        tangent /= tangent.magnitude();
        const RS_Vector normal{-tangent.y, tangent.x};
        const RS_Vector intersection = native->getPointAt(parameter);
        const RS_Vector cutterStart = intersection - normal * cutterHalfLength;
        const RS_Vector cutterEnd = intersection + normal * cutterHalfLength;
        const RS_Vector nativePick = native->getPointAt(parameter + spanWidth * .1);
        RS_Entity* cutter = circularCutter
            ? static_cast<RS_Entity*>(new RS_Circle(&f.graphic, RS_CircleData{intersection, cutterHalfLength}))
            : static_cast<RS_Entity*>(new RS_Line(&f.graphic, RS_LineData{cutterStart, cutterEnd}));
        f.graphic.addEntity(cutter);
        const auto cutterPick = circularCutter
            ? intersection + RS_Vector{cutterHalfLength, 0}
            : intersection + (cutterEnd - cutterStart) * .25;
        const auto kernelTrim = LC_OffsetCacheTrim::prepare(native->getData(), *cutter,
            LC_OffsetCacheTrim::Pick{nativePick});
        REQUIRE(kernelTrim.refusal == LC_OffsetCacheTrim::Refusal::None);
        if (circularCutter) CHECK(kernelTrim.rootCount == 2);
        const auto nativeBefore = trimGeometry(*native);
        const auto cutterBefore = trimGeometry(*cutter);
        f.action->click(parabolaFirst ? nativePick : cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        f.action->hover(parabolaFirst ? cutterPick : nativePick);
        f.action->waitForTrim();
        const auto preview = trimPreview(*f.action->m_preview);
        REQUIRE_FALSE(preview.empty());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(trimGeometry(*native) == nativeBefore);
        CHECK(trimGeometry(*cutter) == cutterBefore);

        f.action->click(parabolaFirst ? cutterPick : nativePick);
        REQUIRE(native->isDeleted());
        REQUIRE(cutter->isDeleted());
        std::vector<RS_Entity*> fragments;
        std::vector<std::string> committed;
        for (auto* entity : live(f.graphic)) {
            if (entity == parabola) continue;
            REQUIRE(entity->rtti() != RS2::EntityOffsetCurve);
            fragments.push_back(entity);
            if (entity->rtti() == RS2::EntitySpline || entity->rtti() == RS2::EntityArc ||
                entity->rtti() == RS2::EntityEllipse)
                committed.push_back(trimGeometry(*entity));
        }
        auto expectedPreview = preview;
        std::sort(committed.begin(), committed.end());
        std::sort(expectedPreview.begin(), expectedPreview.end());
        CHECK(committed == expectedPreview);
        CHECK(fragments.size() >= direct.branches.size());
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntitySpline;
        }));

        std::vector<const RS_Entity*> expected;
        for (const auto* entity : fragments)
            if (entity->rtti() == RS2::EntityLine || entity->rtti() == RS2::EntitySpline)
                expected.push_back(entity);
        REQUIRE_FALSE(expected.empty());
        for (const auto format : formats) {
            const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("symmetric-cusp-trim-%1-%2").arg(circularCutter).arg(fileIndex++) +
                (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, format));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                CHECK_FALSE(document.object().value("OBJECTS").toArray().isEmpty());
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, format));
            const auto reopenedEntities = live(reopened);
            CHECK(std::count_if(reopenedEntities.begin(), reopenedEntities.end(), [](const RS_Entity* entity) {
                return entity->rtti() == RS2::EntityOffsetCurve;
            }) == 0);
            std::vector<const RS_Entity*> actual;
            for (const auto* entity : reopenedEntities)
                if (entity->rtti() == RS2::EntityLine || entity->rtti() == RS2::EntitySpline)
                    actual.push_back(entity);
            CAPTURE(distance, parabolaFirst, format);
            REQUIRE(actual.size() == expected.size());
            std::vector<bool> consumed(actual.size(), false);
            for (const auto* sourceEntity : expected) {
                bool found = false;
                for (std::size_t i = 0; i < actual.size(); ++i) {
                    if (consumed[i] || !sameGeometry(*sourceEntity, *actual[i])) continue;
                    consumed[i] = true;
                    found = true;
                    break;
                }
                CHECK(found);
            }
        }

        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(trimGeometry(*native) == nativeBefore);
        CHECK(trimGeometry(*cutter) == cutterBefore);
        for (const auto* fragment : fragments) CHECK(fragment->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK(cutter->isDeleted());
        for (const auto* fragment : fragments) CHECK_FALSE(fragment->isDeleted());
    }
}

TEST_CASE("T Trim Two preserves transformed beyond-cusp parabola branches",
          "[gui][lc-offsetcurve][trim-action][typed-source][parabola][cusp][transform]") {
    constexpr double angle = .37;
    const double cosine = std::cos(angle), sine = std::sin(angle);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
    const auto close = [](double a, double b) {
        return std::abs(a - b) <= 2e-9 * std::max({1.0, std::abs(a), std::abs(b)});
    };
    const auto closeVector = [&](const RS_Vector& a, const RS_Vector& b) {
        return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
    };
    const auto sameGeometry = [&](const RS_Entity& expected, const RS_Entity& actual) {
        if (expected.rtti() != actual.rtti()) return false;
        if (const auto* a = dynamic_cast<const RS_Line*>(&expected)) {
            const auto* b = dynamic_cast<const RS_Line*>(&actual);
            if (!b) return false;
            const auto& x = a->getData();
            const auto& y = b->getData();
            return (closeVector(x.startpoint, y.startpoint) && closeVector(x.endpoint, y.endpoint)) ||
                (closeVector(x.startpoint, y.endpoint) && closeVector(x.endpoint, y.startpoint));
        }
        if (const auto* a = dynamic_cast<const RS_Spline*>(&expected)) {
            const auto* b = dynamic_cast<const RS_Spline*>(&actual);
            if (!b) return false;
            const auto& x = a->getData();
            const auto& y = b->getData();
            if (x.degree != y.degree || x.m_closedFlag != y.m_closedFlag ||
                x.m_periodicFlag != y.m_periodicFlag || x.controlPoints.size() != y.controlPoints.size() ||
                x.knotslist.size() != y.knotslist.size() || x.weights.size() != y.weights.size()) return false;
            for (std::size_t i = 0; i < x.controlPoints.size(); ++i)
                if (!closeVector(x.controlPoints[i], y.controlPoints[i])) return false;
            for (std::size_t i = 0; i < x.knotslist.size(); ++i)
                if (!close(x.knotslist[i], y.knotslist[i])) return false;
            for (std::size_t i = 0; i < x.weights.size(); ++i)
                if (!close(x.weights[i], y.weights[i])) return false;
            return true;
        }
        return false;
    };
    std::size_t fileIndex = 0;
    for (const unsigned profileKind : {0u, 1u})
    for (const bool reflected : {false, true})
    for (const bool affineFrame : {false, true})
    for (const double scale : {.1, 1.0, 10.0})
    for (const unsigned cutterKind : {0u, 1u, 2u})
    for (const bool parabolaFirst : {false, true}) {
        CAPTURE(profileKind, reflected, affineFrame, scale, cutterKind, parabolaFirst);
        const RS_Vector translation{2 * scale, -scale};
        const auto mapPoint = [&](RS_Vector point) {
            if (reflected) point.x = -point.x;
            if (affineFrame) point = {2 * point.x + .35 * point.y, .6 * point.y};
            point *= scale;
            return RS_Vector{cosine * point.x - sine * point.y,
                             sine * point.x + cosine * point.y} + translation;
        };
        const LC_ParabolaData sourceData = profileKind == 0
            ? LC_ParabolaData{{mapPoint({-1, 1}), mapPoint({0, -1}), mapPoint({1, 1})}}
            : LC_ParabolaData{{mapPoint({-1, 0}), mapPoint({0, 1}), mapPoint({2, 0})}};
        const RS_Vector controlTangent = sourceData.m_controlPoints[2] - sourceData.m_controlPoints[0];
        const RS_Vector acceleration = (sourceData.m_controlPoints[0] - sourceData.m_controlPoints[1] * 2.0 +
                                        sourceData.m_controlPoints[2]) * 2.0;
        const double tangentSpeed = controlTangent.magnitude();
        const double curvatureCross = controlTangent.x * acceleration.y - controlTangent.y * acceleration.x;
        REQUIRE(tangentSpeed > RS_TOLERANCE);
        REQUIRE(std::abs(curvatureCross) > RS_TOLERANCE);
        const double cuspRadius = tangentSpeed * tangentSpeed * tangentSpeed / std::abs(curvatureCross);
        const double distance = (profileKind == 0 ? 1.02 : -1.02) * cuspRadius * (reflected ? -1 : 1);
        Drawing<true> f;
        RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {1000, 0}, 1, 0, 1, false});
        auto initial = LC_OffsetCurve::create(seed, std::abs(distance), distance < 0);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);
        auto* parabola = new LC_Parabola(&f.graphic, sourceData);
        f.graphic.addEntity(parabola);
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        const auto direct = LC_CurveOffset::buildDirectBranches(*parabola,
            LC_CurveOffset::makeSideRequest(distance < 0 ? LC_CurveOffsetSide::Right : LC_CurveOffsetSide::Left,
                                            std::abs(distance)),
            LC_CurveOffset::makeDirectOptions(*parabola, std::abs(distance)),
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(direct.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(direct.branches.size() >= 2);
        if (!affineFrame)
            REQUIRE(direct.branches.size() == (profileKind == 0 ? 3u : 2u));

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Parabola);
        update.use(parabola);
        lc::test::awaitUpdate(update);
        REQUIRE(update.lastReport().updated == 1);
        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->signedDistance == distance);

        const auto breaks = native->getBreakParameters();
        REQUIRE(breaks.size() >= 2);
        const double spanWidth = breaks[1] - breaks[0];
        REQUIRE(spanWidth > 0);
        const double parameter = breaks[0] + spanWidth * .5;
        const double step = std::max(1e-8, spanWidth * 1e-4);
        RS_Vector tangent = native->getPointAt(parameter + step) - native->getPointAt(parameter - step);
        REQUIRE(tangent.magnitude() > RS_TOLERANCE);
        tangent /= tangent.magnitude();
        const RS_Vector normal{-tangent.y, tangent.x};
        const RS_Vector intersection = native->getPointAt(parameter);
        const double cutterRadius = .01 * scale;
        const RS_Vector cutterStart = intersection - normal * cutterRadius;
        const RS_Vector cutterEnd = intersection + normal * cutterRadius;
        const auto nativePick = native->getPointAt(parameter + spanWidth * .1);
        const double tangentAngle = std::atan2(tangent.y, tangent.x);
        const double arcStart = RS_Math::correctAngle(tangentAngle - M_PI_4);
        const double arcEnd = RS_Math::correctAngle(arcStart + 1.5 * M_PI);
        RS_Entity* cutter = cutterKind == 1
            ? static_cast<RS_Entity*>(new RS_Circle(&f.graphic, RS_CircleData{intersection, cutterRadius}))
            : cutterKind == 2
                ? static_cast<RS_Entity*>(new RS_Arc(&f.graphic,
                    RS_ArcData{intersection, cutterRadius, arcStart, arcEnd, false}))
                : static_cast<RS_Entity*>(new RS_Line(&f.graphic, RS_LineData{cutterStart, cutterEnd}));
        f.graphic.addEntity(cutter);
        const double cutterPickAngle = cutterKind == 2 ? tangentAngle + M_PI_2 : 0.0;
        const auto cutterPick = cutterKind == 0
            ? intersection + (cutterEnd - cutterStart) * .25
            : intersection + RS_Vector{cutterRadius * std::cos(cutterPickAngle),
                                       cutterRadius * std::sin(cutterPickAngle)};
        const auto prepared = LC_OffsetCacheTrim::prepare(native->getData(), *cutter,
            LC_OffsetCacheTrim::Pick{nativePick});
        REQUIRE(prepared.refusal == LC_OffsetCacheTrim::Refusal::None);
        if (cutterKind != 0) CHECK(prepared.rootCount == 2);

        const auto nativeBefore = trimGeometry(*native);
        const auto cutterBefore = trimGeometry(*cutter);
        f.action->click(parabolaFirst ? nativePick : cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        f.action->hover(parabolaFirst ? cutterPick : nativePick);
        f.action->waitForTrim();
        REQUIRE_FALSE(trimPreview(*f.action->m_preview).empty());
        f.action->click(parabolaFirst ? cutterPick : nativePick);
        REQUIRE(native->isDeleted());
        REQUIRE(cutter->isDeleted());
        std::vector<RS_Entity*> fragments;
        for (auto* entity : live(f.graphic)) if (entity != parabola) fragments.push_back(entity);
        CHECK(fragments.size() >= direct.branches.size());

        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(trimGeometry(*native) == nativeBefore);
        CHECK(trimGeometry(*cutter) == cutterBefore);
        for (const auto* fragment : fragments) CHECK(fragment->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK(cutter->isDeleted());
        for (const auto* fragment : fragments) CHECK_FALSE(fragment->isDeleted());

        std::vector<const RS_Entity*> expected;
        for (const auto* fragment : fragments)
            if (fragment->rtti() == RS2::EntityLine || fragment->rtti() == RS2::EntitySpline)
                expected.push_back(fragment);
        REQUIRE_FALSE(expected.empty());
        for (const auto format : formats) {
            const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("transformed-cusp-trim-%1-%2-%3-%4-%5-%6-%7")
                    .arg(profileKind).arg(reflected).arg(affineFrame).arg(scale)
                    .arg(cutterKind).arg(parabolaFirst).arg(fileIndex++) +
                (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, format));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                CHECK_FALSE(document.object().value("OBJECTS").toArray().isEmpty());
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, format));
            const auto reopenedEntities = live(reopened);
            CHECK(std::none_of(reopenedEntities.begin(), reopenedEntities.end(), [](const RS_Entity* entity) {
                return entity->rtti() == RS2::EntityOffsetCurve;
            }));
            std::vector<const RS_Entity*> actual;
            for (const auto* entity : reopenedEntities)
                if (entity->rtti() == RS2::EntityLine || entity->rtti() == RS2::EntitySpline)
                    actual.push_back(entity);
            CAPTURE(profileKind, reflected, affineFrame, scale, cutterKind, parabolaFirst, format);
            REQUIRE(actual.size() == expected.size());
            std::vector<bool> consumed(actual.size(), false);
            for (const auto* sourceEntity : expected) {
                bool found = false;
                for (std::size_t i = 0; i < actual.size(); ++i) {
                    if (consumed[i] || !sameGeometry(*sourceEntity, *actual[i])) continue;
                    consumed[i] = true;
                    found = true;
                    break;
                }
                CHECK(found);
            }
        }
    }
}

TEST_CASE("T Trim Two trims exact and beyond-cusp asymmetric branches",
          "[gui][lc-offsetcurve][trim-action][typed-source][parabola][cusp][asymmetric][trim-two]") {
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
    std::size_t fileIndex = 0;
    const auto close = [](double a, double b) {
        return std::abs(a - b) <= 2e-9 * std::max({1.0, std::abs(a), std::abs(b)});
    };
    const auto closeVector = [&](const RS_Vector& a, const RS_Vector& b) {
        return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
    };
    const auto sameGeometry = [&](const RS_Entity& expected, const RS_Entity& actual) {
        if (expected.rtti() != actual.rtti()) return false;
        if (const auto* a = dynamic_cast<const RS_Line*>(&expected)) {
            const auto* b = dynamic_cast<const RS_Line*>(&actual);
            return b && closeVector(a->getData().startpoint, b->getData().startpoint) &&
                closeVector(a->getData().endpoint, b->getData().endpoint);
        }
        if (const auto* a = dynamic_cast<const RS_Spline*>(&expected)) {
            const auto* b = dynamic_cast<const RS_Spline*>(&actual);
            if (!b) return false;
            const auto& x = a->getData();
            const auto& y = b->getData();
            if (x.degree != y.degree || x.m_closedFlag != y.m_closedFlag ||
                x.m_periodicFlag != y.m_periodicFlag || x.controlPoints.size() != y.controlPoints.size() ||
                x.knotslist.size() != y.knotslist.size() || x.weights.size() != y.weights.size()) return false;
            for (std::size_t i = 0; i < x.controlPoints.size(); ++i)
                if (!closeVector(x.controlPoints[i], y.controlPoints[i])) return false;
            for (std::size_t i = 0; i < x.knotslist.size(); ++i)
                if (!close(x.knotslist[i], y.knotslist[i])) return false;
            for (std::size_t i = 0; i < x.weights.size(); ++i)
                if (!close(x.weights[i], y.weights[i])) return false;
            return true;
        }
        return false;
    };
    for (const double ratio : {1.0, 1.02})
    for (const bool circularCutter : {false, true})
    for (const bool parabolaFirst : {false, true}) {
        CAPTURE(ratio, circularCutter, parabolaFirst);
        const double distance = -2.25 * ratio;
        Drawing<true> f;
        RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {10, 0}, 1, 0, .1, false});
        auto initial = LC_OffsetCurve::create(seed, std::abs(distance), true);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);
        const LC_ParabolaData sourceData{{RS_Vector{-1, 0}, RS_Vector{0, 1}, RS_Vector{2, 0}}};
        auto* parabola = new LC_Parabola(&f.graphic, sourceData);
        f.graphic.addEntity(parabola);
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Parabola);
        update.use(parabola);
        lc::test::awaitUpdate(update);
        REQUIRE(update.lastReport().updated == 1);
        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->signedDistance == distance);
        const auto direct = LC_CurveOffset::buildDirectBranches(*parabola,
            LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, std::abs(distance)),
            LC_CurveOffset::makeDirectOptions(*parabola, std::abs(distance), 1e-4),
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(direct.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(direct.branches.size() == 2);

        const auto breaks = native->getBreakParameters();
        REQUIRE(breaks.size() >= 2);
        const double spanWidth = breaks[1] - breaks[0];
        REQUIRE(spanWidth > 0);
        const double parameter = breaks[0] + spanWidth * .5;
        const double step = std::max(1e-8, spanWidth * 1e-4);
        RS_Vector tangent = native->getPointAt(parameter + step) - native->getPointAt(parameter - step);
        REQUIRE(tangent.magnitude() > RS_TOLERANCE);
        tangent /= tangent.magnitude();
        const RS_Vector normal{-tangent.y, tangent.x};
        const auto intersection = native->getPointAt(parameter);
        constexpr double cutterHalfLength = .01;
        const RS_Vector cutterStart = intersection - normal * cutterHalfLength;
        const RS_Vector cutterEnd = intersection + normal * cutterHalfLength;
        const auto nativePick = native->getPointAt(parameter + spanWidth * .1);
        RS_Entity* cutter = circularCutter
            ? static_cast<RS_Entity*>(new RS_Circle(&f.graphic, RS_CircleData{intersection, cutterHalfLength}))
            : static_cast<RS_Entity*>(new RS_Line(&f.graphic, RS_LineData{cutterStart, cutterEnd}));
        f.graphic.addEntity(cutter);
        const auto cutterPick = circularCutter
            ? intersection + RS_Vector{cutterHalfLength, 0}
            : intersection + (cutterEnd - cutterStart) * .25;
        const auto kernelTrim = LC_OffsetCacheTrim::prepare(native->getData(), *cutter,
            LC_OffsetCacheTrim::Pick{nativePick});
        REQUIRE(kernelTrim.refusal == LC_OffsetCacheTrim::Refusal::None);
        CHECK_FALSE(kernelTrim.fragments.empty());
        if (circularCutter) CHECK(kernelTrim.rootCount == 2);
        const auto& cacheData = native->getData();
        std::size_t spanCount = 0;
        std::size_t cuspSpan = cacheData.controlPoints.size();
        for (std::size_t begin = 0; begin < cacheData.knotslist.size();) {
            std::size_t end = begin + 1;
            while (end < cacheData.knotslist.size() && cacheData.knotslist[end] == cacheData.knotslist[begin]) ++end;
            if (end < cacheData.knotslist.size()) {
                if (end - begin == cacheData.degree + 1) cuspSpan = spanCount;
                ++spanCount;
            }
            begin = end;
        }
        REQUIRE(cuspSpan < spanCount);
        std::vector<bool> untouchedBranchSpans(spanCount - cuspSpan, false);
        for (const auto& fragment : kernelTrim.fragments)
            for (const auto& section : fragment.sourceSections) {
                if (section.rawSpan < cuspSpan) continue;
                REQUIRE(section.from == 0);
                REQUIRE(section.to == 1);
                untouchedBranchSpans[section.rawSpan - cuspSpan] = true;
            }
        CHECK(std::all_of(untouchedBranchSpans.begin(), untouchedBranchSpans.end(),
                          [](const bool kept) { return kept; }));

        const auto nativeBefore = trimGeometry(*native);
        const auto cutterBefore = trimGeometry(*cutter);
        f.action->click(parabolaFirst ? nativePick : cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        f.action->hover(parabolaFirst ? cutterPick : nativePick);
        f.action->waitForTrim();
        const auto preview = trimPreview(*f.action->m_preview);
        REQUIRE_FALSE(preview.empty());
        f.action->click(parabolaFirst ? cutterPick : nativePick);
        REQUIRE(native->isDeleted());
        REQUIRE(cutter->isDeleted());
        std::vector<RS_Entity*> fragments;
        std::vector<std::string> committed;
        for (auto* entity : live(f.graphic)) {
            if (entity == parabola) continue;
            fragments.push_back(entity);
            if (entity->rtti() == RS2::EntitySpline || entity->rtti() == RS2::EntityArc ||
                entity->rtti() == RS2::EntityEllipse)
                committed.push_back(trimGeometry(*entity));
        }
        auto expectedPreview = preview;
        std::sort(committed.begin(), committed.end());
        std::sort(expectedPreview.begin(), expectedPreview.end());
        CHECK(committed == expectedPreview);
        CHECK(fragments.size() >= direct.branches.size());

        std::vector<const RS_Entity*> expected;
        for (const auto* entity : fragments)
            if (entity->rtti() == RS2::EntityLine || entity->rtti() == RS2::EntitySpline)
                expected.push_back(entity);
        REQUIRE_FALSE(expected.empty());
        for (const auto format : formats) {
            const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("asymmetric-cusp-trim-%1-%2").arg(circularCutter).arg(fileIndex++) +
                (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, format));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                CHECK_FALSE(document.object().value("OBJECTS").toArray().isEmpty());
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, format));
            std::vector<const RS_Entity*> actual;
            const auto reopenedEntities = live(reopened);
            CHECK(std::count_if(reopenedEntities.begin(), reopenedEntities.end(), [](const RS_Entity* entity) {
                return entity->rtti() == RS2::EntityOffsetCurve;
            }) == 0);
            for (const auto* entity : reopenedEntities)
                if (entity->rtti() == RS2::EntityLine || entity->rtti() == RS2::EntitySpline)
                    actual.push_back(entity);
            CAPTURE(format, parabolaFirst);
            REQUIRE(actual.size() == expected.size());
            std::vector<bool> consumed(actual.size(), false);
            for (const auto* sourceEntity : expected) {
                bool found = false;
                for (std::size_t i = 0; i < actual.size(); ++i) {
                    if (consumed[i] || !sameGeometry(*sourceEntity, *actual[i])) continue;
                    consumed[i] = true;
                    found = true;
                    break;
                }
                CHECK(found);
            }
        }

        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(trimGeometry(*native) == nativeBefore);
        CHECK(trimGeometry(*cutter) == cutterBefore);
        for (const auto* fragment : fragments) CHECK(fragment->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK(cutter->isDeleted());
        for (const auto* fragment : fragments) CHECK_FALSE(fragment->isDeleted());
    }
}

TEST_CASE("T Trim Two action trims a worker-created typed line offset against a circle in both pick orders",
          "[gui][lc-offsetcurve][trim-action][typed-source][line][trim-two]") {
    const std::array<double, 4> orientations{{
        0.0, 0.37, 1.5707963267948966, 1.5707953267948966}};
    for (const double scale : {1.0, 1e5, 1e6})
    for (const double angle : orientations)
    for (const bool lineFirst : {false, true}) {
        CAPTURE(scale, angle, lineFirst);
        Drawing<true> f;
        const RS_Vector center = scale == 1.0 ? RS_Vector{0, 0}
            : scale == 1e5 ? RS_Vector{1e12, -2e12} : RS_Vector{1e13, -2e13};
        if (center.x != 0.0) {
            const double zoom = scale == 1e6 ? 1e-5 : .001;
            f.view.getViewPort()->setSize(640, 480);
            f.view.getViewPort()->setOffsetAndFactor(
                qRound(320.0 - center.x * zoom),
                qRound(240.0 - center.y * zoom), zoom);
        }
        RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {scale, 0}, 1, 0, .1, false});
        auto initial = LC_OffsetCurve::create(seed, .25 * scale, false);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);
        const RS_Vector direction{std::cos(angle), std::sin(angle)};
        const RS_Vector normal{-direction.y, direction.x};
        auto* source = new RS_Line(&f.graphic, RS_LineData{
            center - direction * scale, center + direction * scale});
        f.graphic.addEntity(source);
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Line);
        update.use(source);
        lc::test::awaitUpdate(update);
        REQUIRE(update.lastReport().updated == 1);
        REQUIRE(previous->isDeleted());
        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Line);

        double first = 0, last = 0;
        REQUIRE(native->getParameterDomain(first, last));
        const auto breaks = native->getBreakParameters();
        REQUIRE(breaks.size() >= 2);
        const double parameter = breaks.front() + (breaks[1] - breaks.front()) * .43;
        const RS_Vector intersection = native->getPointAt(parameter);
        auto* cutter = new RS_Line(&f.graphic,
            RS_LineData{intersection - normal * (2.0 * scale),
                        intersection + normal * (2.0 * scale)});
        f.graphic.addEntity(cutter);
        const RS_Vector nativePick = native->getPointAt(
            breaks.front() + (breaks[1] - breaks.front()) * .9);
        const RS_Vector cutterPick = intersection + normal * (1.5 * scale);

        f.action->click(lineFirst ? nativePick : cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        const auto previewUndoEpoch = f.graphic.undoStateEpoch();
        f.action->hover(lineFirst ? cutterPick : nativePick);
        CHECK_FALSE(f.action->m_preview->isEmpty());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);

        f.action->click(lineFirst ? cutterPick : nativePick);
        REQUIRE(native->isDeleted());
        REQUIRE(cutter->isDeleted());
        std::vector<RS_Entity*> fragments;
        for (auto* entity : live(f.graphic)) {
            if (entity == source) continue;
            REQUIRE(entity->rtti() != RS2::EntityOffsetCurve);
            fragments.push_back(entity);
        }
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntitySpline;
        }));
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntityLine;
        }));

        QTemporaryDir directory;
        REQUIRE(directory.isValid());
        const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
            RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
        QString libreDwgReader = qEnvironmentVariable("DWGREAD");
        if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
        const auto close = [](double a, double b) {
            return std::abs(a - b) <= 2e-10 * std::max({1.0, std::abs(a), std::abs(b)});
        };
        const auto closeVector = [&](const RS_Vector& a, const RS_Vector& b) {
            return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
        };
        const auto sameGeometry = [&](const RS_Entity& expected, const RS_Entity& actual) {
            if (expected.rtti() != actual.rtti()) return false;
            if (const auto* a = dynamic_cast<const RS_Spline*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Spline*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                if (x.degree != y.degree || x.m_closedFlag != y.m_closedFlag ||
                    x.m_periodicFlag != y.m_periodicFlag || x.controlPoints.size() != y.controlPoints.size() ||
                    x.knotslist.size() != y.knotslist.size() || x.weights.size() != y.weights.size()) return false;
                for (std::size_t i = 0; i < x.controlPoints.size(); ++i)
                    if (!closeVector(x.controlPoints[i], y.controlPoints[i])) return false;
                for (std::size_t i = 0; i < x.knotslist.size(); ++i)
                    if (!close(x.knotslist[i], y.knotslist[i])) return false;
                for (std::size_t i = 0; i < x.weights.size(); ++i)
                    if (!close(x.weights[i], y.weights[i])) return false;
                return true;
            }
            if (const auto* a = dynamic_cast<const RS_Line*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Line*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                return (closeVector(x.startpoint, y.startpoint) && closeVector(x.endpoint, y.endpoint)) ||
                    (closeVector(x.startpoint, y.endpoint) && closeVector(x.endpoint, y.startpoint));
            }
            return false;
        };
        for (std::size_t i = 0; i < formats.size(); ++i) {
            const bool dwg = formats[i] == RS2::FormatDWG || formats[i] == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("typed-line-trim-two-%1").arg(i) + (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[i]));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                unsigned splines = 0, lines = 0;
                for (const QJsonValue& value : document.object().value("OBJECTS").toArray()) {
                    const QString entity = value.toObject().value("entity").toString();
                    splines += entity == QLatin1String("SPLINE");
                    lines += entity == QLatin1String("LINE");
                }
                CHECK(splines > 0);
                CHECK(lines > 0);
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, formats[i]));
            const auto expectedEntities = live(f.graphic);
            const auto reopenedEntities = live(reopened);
            REQUIRE(reopenedEntities.size() == expectedEntities.size());
            CHECK(std::none_of(reopenedEntities.begin(), reopenedEntities.end(), [](const RS_Entity* entity) {
                return entity->rtti() == RS2::EntityOffsetCurve;
            }));
            std::vector<bool> consumed(reopenedEntities.size(), false);
            for (const auto* expected : expectedEntities) {
                bool found = false;
                for (std::size_t j = 0; j < reopenedEntities.size(); ++j) {
                    if (consumed[j] || !sameGeometry(*expected, *reopenedEntities[j])) continue;
                    consumed[j] = true;
                    found = true;
                    break;
                }
                CHECK(found);
            }
        }
        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK_FALSE(source->isDeleted());
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Line);
        for (const auto* fragment : fragments) CHECK(fragment->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK(cutter->isDeleted());
        for (const auto* fragment : fragments) CHECK_FALSE(fragment->isDeleted());
    }
}

TEST_CASE("T Trim Two action trims a worker-created typed arc offset against a line in both pick orders",
          "[gui][lc-offsetcurve][trim-action][typed-source][arc][trim-two]") {
    const std::array<std::pair<double, double>, 3> veryHighOriginDomains{{
        {1.2, 3.55}, {2.4, 4.75}, {0.15, 5.95}}};
    for (const unsigned profile : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u,
                                   9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u, 17u, 18u})
    for (const bool arcFirst : {false, true}) {
        CAPTURE(profile, arcFirst);
        Drawing<true> f;
        const bool highOrigin = profile >= 9;
        const bool veryHighOrigin = profile >= 11;
        const RS_Vector center = profile < 9 ? RS_Vector{}
            : veryHighOrigin ? RS_Vector{1e13, -2e13} : RS_Vector{1e12, -2e12};
        if (highOrigin) {
            const double zoom = veryHighOrigin ? 1e-5 : .001;
            f.view.getViewPort()->setSize(640, 480);
            f.view.getViewPort()->setOffsetAndFactor(
                qRound(320.0 - center.x * zoom),
                qRound(240.0 - center.y * zoom), zoom);
        }
        const double offsetDistance = veryHighOrigin ? 25000.0 : highOrigin ? 2500.0 : .25;
        RS_Ellipse seed(nullptr, RS_EllipseData{
            {0, 0}, {veryHighOrigin ? 200000.0 : highOrigin ? 20000.0 : 1.0, 0}, 1, 0, .1, false});
        const bool towardCentre = profile == 7 || profile == 8 || profile == 9 || profile == 12;
        auto initial = LC_OffsetCurve::create(seed, offsetDistance, towardCentre);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);
        const unsigned highOriginVariant = profile >= 13 ? profile - 13 : 0;
        const auto [veryHighStart, veryHighEnd] =
            veryHighOriginDomains[highOriginVariant / 2];
        const RS_ArcData sourceData = profile >= 13
            ? RS_ArcData{center, 1e6, veryHighStart, veryHighEnd,
                         highOriginVariant % 2 == 1}
            : profile >= 9
            ? RS_ArcData{center, veryHighOrigin ? 1e6 : 1e5, .35, 2.7,
                         profile == 10 || profile == 12}
            : profile == 0
            ? RS_ArcData{{0, 0}, 2, .1, 1.4, false}
            : profile == 1
                ? RS_ArcData{{3, -4}, 3.2, 2.0 * M_PI - .55, .55, false}
                : profile == 2
                    ? RS_ArcData{{-5, 3}, 2.8, .4, 2.0 * M_PI - .4, true}
                    : profile == 3
                        ? RS_ArcData{{3, -4}, 4.0, 1e-4, 2.0 * M_PI - 1e-4, false}
                        : profile == 4
                            ? RS_ArcData{{-3, 4}, 4.3, 2.0 * M_PI - 1e-4, 1e-4, true}
                            : profile == 5
                                ? RS_ArcData{{3, -4}, 4.0, 1e-8, 2.0 * M_PI - 1e-8, false}
                                : profile == 6
                                    ? RS_ArcData{{-3, 4}, 4.3, 2.0 * M_PI - 1e-8, 1e-8, true}
                                    : profile == 7
                                        ? RS_ArcData{{3, -4}, 4.0, 1e-8, 2.0 * M_PI - 1e-8, false}
                                        : RS_ArcData{{-3, 4}, 4.3, 2.0 * M_PI - 1e-8, 1e-8, true};
        REQUIRE(sourceData.isValid());
        auto* source = new RS_Arc(&f.graphic, sourceData);
        f.graphic.addEntity(source);
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Arc);
        update.use(source);
        lc::test::awaitUpdate(update);
        REQUIRE(update.lastReport().updated == 1);
        REQUIRE(previous->isDeleted());
        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Arc);
        CHECK(native->definition().distance == offsetDistance);
        CHECK(native->definition().towardCentre == towardCentre);

        double first = 0, last = 0;
        REQUIRE(native->getParameterDomain(first, last));
        const auto breaks = native->getBreakParameters();
        REQUIRE(breaks.size() >= 2);
        const double parameter = breaks.front() + (breaks[1] - breaks.front()) * .43;
        const RS_Vector intersection = native->getPointAt(parameter);
        const RS_Vector before = native->getPointAt(parameter - 1e-5);
        const RS_Vector after = native->getPointAt(parameter + 1e-5);
        RS_Vector tangent{after.x - before.x, after.y - before.y};
        REQUIRE(tangent.magnitude() > RS_TOLERANCE);
        tangent /= tangent.magnitude();
        const RS_Vector normal{-tangent.y, tangent.x};
        const double cutterHalfLength = veryHighOrigin ? 100000.0 : highOrigin ? 10000.0 : 3.0;
        auto* cutter = new RS_Line(&f.graphic,
            RS_LineData{intersection - normal * cutterHalfLength,
                        intersection + normal * cutterHalfLength});
        f.graphic.addEntity(cutter);
        const RS_Vector nativePick = native->getPointAt(
            breaks.front() + (breaks[1] - breaks.front()) * .9);
        const RS_Vector cutterPick = intersection + normal * cutterHalfLength * .65;

        f.action->click(arcFirst ? nativePick : cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        const auto previewUndoEpoch = f.graphic.undoStateEpoch();
        f.action->hover(arcFirst ? cutterPick : nativePick);
        CHECK_FALSE(f.action->m_preview->isEmpty());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);

        f.action->click(arcFirst ? cutterPick : nativePick);
        REQUIRE(native->isDeleted());
        REQUIRE(cutter->isDeleted());
        std::vector<RS_Entity*> fragments;
        for (auto* entity : live(f.graphic)) {
            if (entity == source) continue;
            REQUIRE(entity->rtti() != RS2::EntityOffsetCurve);
            fragments.push_back(entity);
        }
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntitySpline;
        }));
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntityLine;
        }));
        QTemporaryDir directory;
        REQUIRE(directory.isValid());
        const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
            RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
        QString libreDwgReader = qEnvironmentVariable("DWGREAD");
        if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
        const auto close = [](double a, double b) {
            return std::abs(a - b) <= 2e-10 * std::max({1.0, std::abs(a), std::abs(b)});
        };
        const auto closeVector = [&](const RS_Vector& a, const RS_Vector& b) {
            return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
        };
        const auto sameGeometry = [&](const RS_Entity& expected, const RS_Entity& actual) {
            if (expected.rtti() != actual.rtti()) return false;
            if (const auto* a = dynamic_cast<const RS_Spline*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Spline*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                if (x.degree != y.degree || x.m_closedFlag != y.m_closedFlag ||
                    x.m_periodicFlag != y.m_periodicFlag || x.controlPoints.size() != y.controlPoints.size() ||
                    x.knotslist.size() != y.knotslist.size() || x.weights.size() != y.weights.size()) return false;
                for (std::size_t i = 0; i < x.controlPoints.size(); ++i)
                    if (!closeVector(x.controlPoints[i], y.controlPoints[i])) return false;
                for (std::size_t i = 0; i < x.knotslist.size(); ++i)
                    if (!close(x.knotslist[i], y.knotslist[i])) return false;
                for (std::size_t i = 0; i < x.weights.size(); ++i)
                    if (!close(x.weights[i], y.weights[i])) return false;
                return true;
            }
            if (const auto* a = dynamic_cast<const RS_Line*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Line*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                return (closeVector(x.startpoint, y.startpoint) && closeVector(x.endpoint, y.endpoint)) ||
                    (closeVector(x.startpoint, y.endpoint) && closeVector(x.endpoint, y.startpoint));
            }
            if (const auto* a = dynamic_cast<const RS_Arc*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Arc*>(&actual);
                if (!b) return false;
                const RS_Vector aStart = a->getStartpoint();
                const RS_Vector aEnd = a->getEndpoint();
                const RS_Vector bStart = b->getStartpoint();
                const RS_Vector bEnd = b->getEndpoint();
                const bool sameEndpoints = closeVector(aStart, bStart) && closeVector(aEnd, bEnd);
                const bool reversedEndpoints = closeVector(aStart, bEnd) && closeVector(aEnd, bStart);
                return closeVector(a->getCenter(), b->getCenter()) && close(a->getRadius(), b->getRadius()) &&
                    closeVector(a->getMiddlePoint(), b->getMiddlePoint()) &&
                    close(a->getLength(), b->getLength()) && (sameEndpoints || reversedEndpoints);
            }
            return false;
        };
        for (std::size_t i = 0; i < formats.size(); ++i) {
            const bool dwg = formats[i] == RS2::FormatDWG || formats[i] == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("typed-arc-trim-two-%1").arg(i) + (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[i]));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                unsigned splines = 0, lines = 0, arcs = 0;
                for (const QJsonValue& value : document.object().value("OBJECTS").toArray()) {
                    const QString entity = value.toObject().value("entity").toString();
                    splines += entity == QLatin1String("SPLINE");
                    lines += entity == QLatin1String("LINE");
                    arcs += entity == QLatin1String("ARC");
                }
                CHECK(splines > 0);
                CHECK(lines > 0);
                CHECK(arcs > 0);
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, formats[i]));
            const auto expectedEntities = live(f.graphic);
            const auto reopenedEntities = live(reopened);
            REQUIRE(reopenedEntities.size() == expectedEntities.size());
            CHECK(std::none_of(reopenedEntities.begin(), reopenedEntities.end(), [](const RS_Entity* entity) {
                return entity->rtti() == RS2::EntityOffsetCurve;
            }));
            std::vector<bool> consumed(reopenedEntities.size(), false);
            for (const auto* expected : expectedEntities) {
                bool found = false;
                for (std::size_t j = 0; j < reopenedEntities.size(); ++j) {
                    if (consumed[j] || !sameGeometry(*expected, *reopenedEntities[j])) continue;
                    consumed[j] = true;
                    found = true;
                    break;
                }
                CHECK(found);
            }
            unsigned reopenedArcs = 0;
            for (const auto* entity : reopenedEntities) {
                if (entity->rtti() != RS2::EntityArc) continue;
                ++reopenedArcs;
                const auto* loadedArc = dynamic_cast<const RS_Arc*>(entity);
                REQUIRE(loadedArc);
                CHECK(sameGeometry(*source, *loadedArc));
            }
            CAPTURE(formats[i]);
            CHECK(reopenedArcs == 1);
        }
        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK_FALSE(source->isDeleted());
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Arc);
        for (const auto* fragment : fragments) CHECK(fragment->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK(cutter->isDeleted());
        for (const auto* fragment : fragments) CHECK_FALSE(fragment->isDeleted());
    }
}

TEST_CASE("T Trim Two action trims a worker-created typed circle offset against a line in both pick orders",
          "[gui][lc-offsetcurve][trim-action][typed-source][circle][trim-two]") {
    for (const double scale : {0.1, 1.0, 10.0, 1e5, 1e6, 1e7})
    for (const bool circleFirst : {false, true}) {
        CAPTURE(scale, circleFirst);
        Drawing<true> f;
        const RS_Vector center = scale == 1e5 ? RS_Vector{1e12, -2e12}
            : scale >= 1e6 ? RS_Vector{1e13, -2e13} : RS_Vector{0, 0};
        if (center.x != 0.0) {
            const double zoom = scale >= 1e6 ? 1e-5 * (1e6 / scale) : .001;
            f.view.getViewPort()->setSize(640, 480);
            f.view.getViewPort()->setOffsetAndFactor(
                qRound(320.0 - center.x * zoom),
                qRound(240.0 - center.y * zoom), zoom);
        }
        RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {scale, 0}, 1, 0, .1, false});
        auto initial = LC_OffsetCurve::create(seed, .25 * scale, false);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);
        auto* source = new RS_Circle(&f.graphic, RS_CircleData{center, 2 * scale});
        f.graphic.addEntity(source);
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Circle);
        update.use(source);
        lc::test::awaitUpdate(update);
        REQUIRE(update.lastReport().updated == 1);
        REQUIRE(previous->isDeleted());
        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Circle);

        double first = 0, last = 0;
        REQUIRE(native->getParameterDomain(first, last));
        const auto breaks = native->getBreakParameters();
        REQUIRE(breaks.size() >= 2);
        const double parameter = first + (last - first) * .487;
        const RS_Vector intersection = native->getPointAt(parameter);
        const double step = std::max(1e-7, (last - first) * 1e-5);
        const RS_Vector before = native->getPointAt(parameter - step);
        const RS_Vector after = native->getPointAt(parameter + step);
        RS_Vector tangent{after.x - before.x, after.y - before.y};
        REQUIRE(tangent.magnitude() > RS_TOLERANCE);
        tangent /= tangent.magnitude();
        const RS_Vector normal{-tangent.y, tangent.x};
        RS_Vector cutterDirection = normal;
        REQUIRE(cutterDirection.magnitude() > RS_TOLERANCE);
        cutterDirection /= cutterDirection.magnitude();
        cutterDirection += tangent;
        REQUIRE(cutterDirection.magnitude() > RS_TOLERANCE);
        cutterDirection = cutterDirection / cutterDirection.magnitude() * (12.0 * scale);
        auto* cutter = new RS_Line(&f.graphic,
            RS_LineData{intersection - cutterDirection, intersection + cutterDirection});
        f.graphic.addEntity(cutter);
        const RS_Vector nativePick = native->getPointAt(first + (last - first) * .1);
        const RS_Vector cutterPick = intersection + cutterDirection * .75;
        f.action->click(circleFirst ? nativePick : cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        const auto previewUndoEpoch = f.graphic.undoStateEpoch();
        f.action->hover(circleFirst ? cutterPick : nativePick);
        CHECK_FALSE(f.action->m_preview->isEmpty());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);

        f.action->click(circleFirst ? cutterPick : nativePick);
        REQUIRE(native->isDeleted());
        REQUIRE(cutter->isDeleted());
        std::vector<RS_Entity*> fragments;
        for (auto* entity : live(f.graphic)) {
            if (entity == source) continue;
            REQUIRE(entity->rtti() != RS2::EntityOffsetCurve);
            fragments.push_back(entity);
        }
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntitySpline;
        }));
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntityLine;
        }));
        const RS_CircleData sourceBefore = source->getData();
        QTemporaryDir directory;
        REQUIRE(directory.isValid());
        const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
            RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
        QString libreDwgReader = qEnvironmentVariable("DWGREAD");
        if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
        const auto close = [](double a, double b) {
            return std::abs(a - b) <= 2e-10 * std::max({1.0, std::abs(a), std::abs(b)});
        };
        const auto closeVector = [&](const RS_Vector& a, const RS_Vector& b) {
            return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
        };
        const auto sameGeometry = [&](const RS_Entity& expected, const RS_Entity& actual) {
            if (expected.rtti() != actual.rtti()) return false;
            if (const auto* a = dynamic_cast<const RS_Spline*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Spline*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                if (x.degree != y.degree || x.m_closedFlag != y.m_closedFlag ||
                    x.m_periodicFlag != y.m_periodicFlag || x.controlPoints.size() != y.controlPoints.size() ||
                    x.knotslist.size() != y.knotslist.size() || x.weights.size() != y.weights.size()) return false;
                for (std::size_t i = 0; i < x.controlPoints.size(); ++i)
                    if (!closeVector(x.controlPoints[i], y.controlPoints[i])) return false;
                for (std::size_t i = 0; i < x.knotslist.size(); ++i)
                    if (!close(x.knotslist[i], y.knotslist[i])) return false;
                for (std::size_t i = 0; i < x.weights.size(); ++i)
                    if (!close(x.weights[i], y.weights[i])) return false;
                return true;
            }
            if (const auto* a = dynamic_cast<const RS_Line*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Line*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                return (closeVector(x.startpoint, y.startpoint) && closeVector(x.endpoint, y.endpoint)) ||
                    (closeVector(x.startpoint, y.endpoint) && closeVector(x.endpoint, y.startpoint));
            }
            if (const auto* a = dynamic_cast<const RS_Circle*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Circle*>(&actual);
                return b && closeVector(a->getData().center, b->getData().center) &&
                    close(a->getData().radius, b->getData().radius);
            }
            return false;
        };
        for (std::size_t i = 0; i < formats.size(); ++i) {
            const bool dwg = formats[i] == RS2::FormatDWG || formats[i] == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("typed-circle-trim-two-%1").arg(i) + (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[i]));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                unsigned splines = 0, lines = 0, circles = 0;
                for (const QJsonValue& value : document.object().value("OBJECTS").toArray()) {
                    const QString entity = value.toObject().value("entity").toString();
                    splines += entity == QLatin1String("SPLINE");
                    lines += entity == QLatin1String("LINE");
                    circles += entity == QLatin1String("CIRCLE");
                }
                CHECK(splines > 0);
                CHECK(lines > 0);
                CHECK(circles > 0);
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, formats[i]));
            const auto expectedEntities = live(f.graphic);
            const auto reopenedEntities = live(reopened);
            REQUIRE(reopenedEntities.size() == expectedEntities.size());
            CHECK(std::none_of(reopenedEntities.begin(), reopenedEntities.end(), [](const RS_Entity* entity) {
                return entity->rtti() == RS2::EntityOffsetCurve;
            }));
            std::vector<bool> consumed(reopenedEntities.size(), false);
            for (const auto* expected : expectedEntities) {
                bool found = false;
                for (std::size_t j = 0; j < reopenedEntities.size(); ++j) {
                    if (consumed[j] || !sameGeometry(*expected, *reopenedEntities[j])) continue;
                    consumed[j] = true;
                    found = true;
                    break;
                }
                CHECK(found);
            }
            unsigned reopenedCircles = 0;
            for (const auto* entity : reopenedEntities) {
                if (entity->rtti() != RS2::EntityCircle) continue;
                ++reopenedCircles;
                const auto* loadedCircle = dynamic_cast<const RS_Circle*>(entity);
                REQUIRE(loadedCircle);
                CHECK(closeVector(loadedCircle->getData().center, sourceBefore.center));
                CHECK(close(loadedCircle->getData().radius, sourceBefore.radius));
            }
            CAPTURE(formats[i]);
            CHECK(reopenedCircles == 1);
        }
        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK_FALSE(source->isDeleted());
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Circle);
        for (const auto* fragment : fragments) CHECK(fragment->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK(cutter->isDeleted());
        for (const auto* fragment : fragments) CHECK_FALSE(fragment->isDeleted());
    }
}

TEST_CASE("T Trim kernel certifies exact internal knots on a worker-created closed circle cache",
          "[gui][lc-offsetcurve][trim-action][typed-source][circle][trim-two][exact-knot]") {
    Drawing<true> f;
    RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false});
    auto initial = LC_OffsetCurve::create(seed, .25, false);
    REQUIRE(initial);
    auto* previous = initial.entity.release();
    f.graphic.addEntity(previous);
    auto* source = new RS_Circle(&f.graphic, RS_CircleData{{0, 0}, 2});
    f.graphic.addEntity(source);
    f.graphic.select(QList<RS_Entity*>{previous}, true);

    UpdateSourceProbe update(&f.context);
    update.init(0);
    update.setSourceKind(LC_OffsetSourceGeometry::Kind::Circle);
    update.use(source);
    lc::test::awaitUpdate(update);
    REQUIRE(update.lastReport().updated == 1);
    LC_OffsetCurve* native = nullptr;
    for (auto* entity : live(f.graphic))
        if (entity->rtti() == RS2::EntityOffsetCurve) native = dynamic_cast<LC_OffsetCurve*>(entity);
    REQUIRE(native);
    const auto breaks = native->getBreakParameters();
    REQUIRE(breaks.size() >= 5);
    const auto& cache = native->getData();
    REQUIRE(cache.degree == 3);
    const RS_Vector firstKnot = cache.controlPoints[cache.degree];
    const RS_Vector lineEnd{-2 * firstKnot.x, 2 * firstKnot.y};
    auto* cutter = new RS_Line(&f.graphic, firstKnot, lineEnd);
    f.graphic.addEntity(cutter);
    const auto pick = native->getPointAt((breaks[2] + breaks[3]) * .5);
    const auto result = LC_OffsetCacheTrim::prepare(native->getData(), *cutter, {pick});
    INFO("refusal=" << int(result.refusal) << ", roots=" << result.rootCount
         << ", pairs=" << result.work.intersections.totalBoxPairs
         << ", leaves=" << result.work.intersections.generatedLeaves
         << ", items=" << result.work.intersections.generatedItems
         << ", depth=" << result.work.intersections.deepestPairRefinement
         << ", kernelItems=" << result.work.kernelItems
         << ", comparisons=" << result.work.kernelComparisons);
    CHECK(result.refusal == LC_OffsetCacheTrim::Refusal::None);
    CHECK(result.rootCount == 2);
    CHECK(result.fragments.size() == 1);
    REQUIRE(result.roots.size() == 2);
    bool exactKnot = false;
    bool interiorCrossing = false;
    for (const auto& root : result.roots) {
        const double sourceParameter = double(root.target.rawSpan) + root.target.localParameter.lo();
        exactKnot |= root.proof == LC_IntersectionProof::AnalyticEndpoint &&
                     sourceParameter == std::round(sourceParameter);
        interiorCrossing |= (root.proof == LC_IntersectionProof::IntervalUniqueTransverse ||
                             root.proof == LC_IntersectionProof::AnalyticMonotoneTransverse) &&
                            std::abs(sourceParameter - std::round(sourceParameter)) > .1;
    }
    CHECK(exactKnot);
    CHECK(interiorCrossing);

    RS_SplineData lineData{1, false};
    lineData.controlPoints = {firstKnot, lineEnd};
    lineData.knotslist = {0, 0, 1, 1};
    lineData.weights = {1, 1};
    const RS_Vector cutterPick = (firstKnot + lineEnd) * .5;
    const auto lineResult = LC_OffsetCacheTrim::prepare(lineData, *native, {cutterPick});
    INFO("line refusal=" << int(lineResult.refusal) << ", roots=" << lineResult.rootCount);
    REQUIRE(lineResult.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(lineResult.rootCount == 2);
    REQUIRE(result.fragments.size() == 1);
    REQUIRE(lineResult.fragments.size() == 1);

    const auto nativePick = native->getPointAt((breaks[2] + breaks[3]) * .5);
    f.action->click(nativePick);
    REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
    const auto previewUndoEpoch = f.graphic.undoStateEpoch();
    f.action->hover(cutterPick);
    CHECK_FALSE(f.action->m_preview->isEmpty());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);
    f.action->click(cutterPick);
    REQUIRE(native->isDeleted());
    REQUIRE(cutter->isDeleted());
    const auto committed = live(f.graphic);
    CHECK(std::any_of(committed.begin(), committed.end(), [](const RS_Entity* entity) {
        return entity->rtti() == RS2::EntitySpline;
    }));
    REQUIRE(f.graphic.undo());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    REQUIRE(f.graphic.redo());
    CHECK(native->isDeleted());
    CHECK(cutter->isDeleted());

    const auto expected = live(f.graphic);
    REQUIRE(expected.size() == 3);
    CHECK(std::count_if(expected.begin(), expected.end(), [](const RS_Entity* entity) {
        return entity->rtti() == RS2::EntitySpline;
    }) == 1);
    CHECK(std::count_if(expected.begin(), expected.end(), [](const RS_Entity* entity) {
        return entity->rtti() == RS2::EntityLine;
    }) == 1);
    CHECK(std::count_if(expected.begin(), expected.end(), [](const RS_Entity* entity) {
        return entity->rtti() == RS2::EntityCircle;
    }) == 1);
    const auto close = [](double a, double b) {
        return std::abs(a - b) <= 2e-10 * std::max({1.0, std::abs(a), std::abs(b)});
    };
    const auto closeVector = [&](const RS_Vector& a, const RS_Vector& b) {
        return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
    };
    const auto sameGeometry = [&](const RS_Entity& a, const RS_Entity& b) {
        if (a.rtti() != b.rtti()) return false;
        if (const auto* x = dynamic_cast<const RS_Spline*>(&a)) {
            const auto* y = dynamic_cast<const RS_Spline*>(&b);
            if (!y) return false;
            const auto& xd = x->getData();
            const auto& yd = y->getData();
            if (xd.degree != yd.degree || xd.m_closedFlag != yd.m_closedFlag ||
                xd.m_periodicFlag != yd.m_periodicFlag || xd.controlPoints.size() != yd.controlPoints.size() ||
                xd.knotslist.size() != yd.knotslist.size() || xd.weights.size() != yd.weights.size()) return false;
            for (std::size_t i = 0; i < xd.controlPoints.size(); ++i)
                if (!closeVector(xd.controlPoints[i], yd.controlPoints[i])) return false;
            for (std::size_t i = 0; i < xd.knotslist.size(); ++i)
                if (!close(xd.knotslist[i], yd.knotslist[i])) return false;
            for (std::size_t i = 0; i < xd.weights.size(); ++i)
                if (!close(xd.weights[i], yd.weights[i])) return false;
            return true;
        }
        if (const auto* x = dynamic_cast<const RS_Line*>(&a)) {
            const auto* y = dynamic_cast<const RS_Line*>(&b);
            if (!y) return false;
            const auto& xd = x->getData();
            const auto& yd = y->getData();
            return (closeVector(xd.startpoint, yd.startpoint) && closeVector(xd.endpoint, yd.endpoint)) ||
                   (closeVector(xd.startpoint, yd.endpoint) && closeVector(xd.endpoint, yd.startpoint));
        }
        const auto* x = dynamic_cast<const RS_Circle*>(&a);
        const auto* y = dynamic_cast<const RS_Circle*>(&b);
        return x && y && closeVector(x->getData().center, y->getData().center) &&
               close(x->getData().radius, y->getData().radius);
    };
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
    QString libreDwgReader = qEnvironmentVariable("DWGREAD");
    if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
    for (std::size_t i = 0; i < formats.size(); ++i) {
        const bool dwg = formats[i] == RS2::FormatDWG || formats[i] == RS2::FormatDWG2018;
        const auto file = std::filesystem::path(directory.filePath(
            QStringLiteral("typed-circle-exact-knot-trim-two-%1").arg(i) +
            (dwg ? ".dwg" : ".dxf")).toStdString());
        REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[i]));
#ifdef DWGSUPPORT
        if (dwg && !libreDwgReader.isEmpty()) {
            QProcess reader;
            reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
            REQUIRE(reader.waitForStarted());
            REQUIRE(reader.waitForFinished(30000));
            INFO(reader.readAllStandardError().toStdString());
            REQUIRE(reader.exitStatus() == QProcess::NormalExit);
            REQUIRE(reader.exitCode() == 0);
            const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
            REQUIRE(document.isObject());
            unsigned splines = 0, lines = 0, circles = 0;
            for (const QJsonValue& value : document.object().value("OBJECTS").toArray()) {
                const QString entity = value.toObject().value("entity").toString();
                splines += entity == QLatin1String("SPLINE");
                lines += entity == QLatin1String("LINE");
                circles += entity == QLatin1String("CIRCLE");
            }
            CHECK(splines == 1);
            CHECK(lines == 1);
            CHECK(circles == 1);
        }
#endif
        RS_Graphic reopened;
        REQUIRE(lc::test::importDrawing(reopened, file, formats[i]));
        const auto actual = live(reopened);
        REQUIRE(actual.size() == expected.size());
        CHECK(std::none_of(actual.begin(), actual.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntityOffsetCurve;
        }));
        std::array<bool, 3> consumed{};
        for (const auto* expectedEntity : expected) {
            bool found = false;
            for (std::size_t j = 0; j < actual.size(); ++j) {
                if (consumed[j] || !sameGeometry(*expectedEntity, *actual[j])) continue;
                consumed[j] = true;
                found = true;
                break;
            }
            CAPTURE(formats[i], expectedEntity->rtti());
            CHECK(found);
        }
    }
}

TEST_CASE("T Trim Two action trims a worker-created typed ellipse offset against a line in both pick orders",
          "[gui][lc-offsetcurve][trim-action][typed-source][ellipse][trim-two]") {
    constexpr double rotation = .41;
    constexpr double fullRotation = .67;
    constexpr double inwardRotation = .93;
    constexpr double reversedArcRotation = -.38;
    constexpr double nearCuspRotation = 1.17;
    for (const unsigned profile : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u,
                                   12u, 13u, 14u, 15u, 16u, 17u, 18u, 19u, 20u,
                                   21u, 22u, 23u, 24u, 25u, 26u, 27u, 28u, 29u, 30u, 31u,
                                   32u, 33u, 34u, 35u, 36u, 37u})
    for (const bool ellipseFirst : {false, true}) {
        CAPTURE(profile, ellipseFirst);
        Drawing<true> f;
        RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false});
        const double offsetDistance = profile >= 35 ? .03 :
                                      profile == 5 ? .0135 : profile == 6 ? .003375 :
                                      profile == 7 ? .00084375 : profile == 8 ? .0075 :
                                      profile == 9 ? .0225 : profile == 10 ? .03 :
                                      profile == 11 ? .0315 : profile == 12 ? .03 :
                                      profile == 13 ? .054 : profile == 14 ? .09 :
                                      profile == 15 ? .12 : profile == 16 ? .126 :
                                      profile == 17 ? .0675 : profile == 18 ? .1215 :
                                      profile == 19 ? .2025 : profile == 20 ? .27 :
                                      profile == 21 ? .1875 : profile == 22 ? .3375 :
                                      profile == 23 ? .5625 : profile == 25 ? .5 :
                                      profile == 26 ? .25 :
                                      profile == 27 || profile == 28 || profile == 29 ||
                                      profile == 30 || profile == 31 || profile == 32 ||
                                      profile == 33 ? .5 : .054;
        const bool towardCentre = profile == 3 ||
                                  (profile >= 5 && profile != 25 && profile != 28 &&
                                   profile != 29 && profile != 32);
        auto initial = LC_OffsetCurve::create(seed, offsetDistance, towardCentre);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);
        const RS_EllipseData sourceData = profile == 0
            ? RS_EllipseData{{0, 0}, {3, 0}, .7, 0, 0, false}
            : profile == 1
                ? RS_EllipseData{{5, -4}, {3 * std::cos(rotation), 3 * std::sin(rotation)},
                                 .5, -.8, 1.1, false, true}
                : profile == 2
                    ? RS_EllipseData{{8, -6}, {2.5 * std::cos(fullRotation),
                                               2.5 * std::sin(fullRotation)},
                                     .9, 0, 0, false}
                    : profile == 3
                        ? RS_EllipseData{{-6, 8}, {2.5 * std::cos(inwardRotation),
                                                   2.5 * std::sin(inwardRotation)},
                                         .9, 0, 0, false}
                    : profile == 4
                        ? RS_EllipseData{{-4, 6}, {3 * std::cos(reversedArcRotation),
                                                   3 * std::sin(reversedArcRotation)},
                                         .6, .25, 1.6, true, true}
                    : profile == 29 || profile == 30
                        ? RS_EllipseData{{-6, 8}, {3 * std::cos(nearCuspRotation),
                                                   3 * std::sin(nearCuspRotation)},
                                         .6, -.4, 2.4, true, false}
                    : profile == 32 || profile == 33
                        ? RS_EllipseData{{-6, 8}, {3 * std::cos(nearCuspRotation),
                                                   3 * std::sin(nearCuspRotation)},
                                         .6, -.4, 2.4, true, true}
                    : profile == 35
                        ? RS_EllipseData{{4, -3}, {3 * std::cos(.37), 3 * std::sin(.37)},
                                         .2, .8, 1.5, true, false}
                    : profile == 36
                        ? RS_EllipseData{{4, -3}, {3 * std::cos(.37), 3 * std::sin(.37)},
                                         .2, -.45, .75, true, false}
                    : profile == 37
                        ? RS_EllipseData{{4, -3}, {3 * std::cos(.37), 3 * std::sin(.37)},
                                         .2, -.45, .75, true, false}
                    : profile == 34
                        ? RS_EllipseData{{-6, 8}, {3 * std::cos(nearCuspRotation),
                                                   3 * std::sin(nearCuspRotation)},
                                         .2, -.4, 2.4, true, false}
                    : profile == 31
                        ? RS_EllipseData{{-6, 8}, {3 * std::cos(nearCuspRotation),
                                                   3 * std::sin(nearCuspRotation)},
                                         .6, .25, 1.6, true, false}
                    : profile == 25 || profile == 26 || profile == 27 || profile == 28
                        ? RS_EllipseData{{-6, 8}, {3 * std::cos(nearCuspRotation),
                                                   3 * std::sin(nearCuspRotation)},
                                         .6, .25, 1.6, true, profile != 28}
                        : RS_EllipseData{{-6, 8}, {3 * std::cos(nearCuspRotation),
                                                   3 * std::sin(nearCuspRotation)},
                                         profile == 6 ? .05 : profile == 7 ? .025 : profile == 24 ? .2 :
                                             profile >= 21 ? .5 : profile >= 17 ? .3 :
                                             profile >= 12 ? .2 : .1,
                                         0, 0, false, profile == 24};
        auto* source = new RS_Ellipse(&f.graphic, sourceData);
        f.graphic.addEntity(source);
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Ellipse);
        update.use(source);
        lc::test::awaitUpdate(update);
        if (profile == 20) {
            REQUIRE(update.lastReport().updated == 0);
            REQUIRE(update.lastReport().refused == 1);
            REQUIRE(update.lastReport().outcomes.size() == 1);
            CHECK(update.lastReport().outcomes.front().refusal.contains(
                QStringLiteral("bounded cache validation limit reached")));
            CHECK_FALSE(previous->isDeleted());
            CHECK_FALSE(source->isDeleted());
            CHECK(previous->definition().distance == offsetDistance);
            continue;
        }
        REQUIRE(update.lastReport().updated == 1);
        REQUIRE(previous->isDeleted());
        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        CHECK(native->hasEditableDefinition());
        CHECK_FALSE(native->sourceDefinition());
        CHECK(native->definition().distance == offsetDistance);
        CHECK(native->definition().towardCentre == towardCentre);

        double first = 0, last = 0;
        REQUIRE(native->getParameterDomain(first, last));
        REQUIRE(native->getBreakParameters().size() >= 2);
        const double parameter = first + (last - first) *
                                 (profile == 36 ? .9 :
                                  (profile == 27 || profile == 30 || profile == 31 ||
                                   profile == 32 || profile == 33 || profile == 34) ? .25 :
                                  profile == 29 ? .75 : .487);
        const RS_Vector intersection = native->getPointAt(parameter);
        const double step = std::max(1e-7, (last - first) * 1e-5);
        const RS_Vector before = native->getPointAt(parameter - step);
        const RS_Vector after = native->getPointAt(parameter + step);
        RS_Vector tangent{after.x - before.x, after.y - before.y};
        REQUIRE(tangent.magnitude() > RS_TOLERANCE);
        tangent /= tangent.magnitude();
        RS_Vector cutterDirection{-tangent.y, tangent.x};
        REQUIRE(cutterDirection.magnitude() > RS_TOLERANCE);
        cutterDirection /= cutterDirection.magnitude();
        if (profile != 3 && profile < 5) cutterDirection += tangent;
        REQUIRE(cutterDirection.magnitude() > RS_TOLERANCE);
        cutterDirection = cutterDirection / cutterDirection.magnitude() * (profile >= 35 ? 2.0 : 18.0);
        auto* cutter = new RS_Line(&f.graphic,
            RS_LineData{intersection - cutterDirection, intersection + cutterDirection});
        f.graphic.addEntity(cutter);
        const std::uint64_t nativeGeometryFingerprint = RS_Spline::geometryFingerprint(native->getData());
        const RS_LineData cutterGeometry = cutter->getData();

        double nativePickFraction = profile >= 5 ? .25 : .1;
        bool hasCertifiedArcPick = false;
        const RS_Vector cutterPick = intersection + cutterDirection * .75;
        if (profile >= 5 && (!ellipseFirst || profile >= 27)) {
            for (const double fraction : {.05, .1, .15, .2, .25, .3, .35, .4,
                                          .45, .5, .55, .6, .65, .7, .75, .8,
                                          .85, .9, .95}) {
                if (profile == 25 && fraction != .25) continue;
                CAPTURE(fraction);
                const auto probe = LC_OffsetCacheTrim::prepare(native->getData(), *cutter,
                    {native->getPointAt(first + (last - first) * fraction)});
                if (profile >= 26) {
                    if (probe.refusal == LC_OffsetCacheTrim::Refusal::None) {
                        if (!hasCertifiedArcPick) nativePickFraction = fraction;
                        hasCertifiedArcPick = true;
                    } else {
                        CHECK(probe.fragments.empty());
                    }
                    continue;
                }
                if (profile == 23 && fraction == .5) {
                    CHECK(probe.refusal == LC_OffsetCacheTrim::Refusal::LimitExceeded);
                    CHECK(probe.fragments.empty());
                    continue;
                }
                REQUIRE(probe.refusal == LC_OffsetCacheTrim::Refusal::None);
                if (profile >= 10) CHECK_FALSE(probe.fragments.empty());
                else CHECK(probe.fragments.size() == 1);
            }
        }
        if (profile >= 26 && !ellipseFirst) {
            if (profile == 37) CHECK_FALSE(hasCertifiedArcPick);
            else CHECK(hasCertifiedArcPick);
        }
        const RS_Vector nativePick = native->getPointAt(first + (last - first) * nativePickFraction);
        f.action->click(ellipseFirst ? nativePick : cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        const auto previewUndoEpoch = f.graphic.undoStateEpoch();
        f.action->hover(ellipseFirst ? cutterPick : nativePick);
        f.action->waitForTrim();
        if (profile == 37) CHECK(f.action->m_preview->isEmpty());
        else if (profile != 5) CHECK_FALSE(f.action->m_preview->isEmpty());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);

        f.action->click(ellipseFirst ? cutterPick : nativePick);
        INFO(f.context.messages.join('\n').toStdString());
        if (profile == 37) {
            CHECK_FALSE(native->isDeleted());
            CHECK_FALSE(cutter->isDeleted());
            CHECK(RS_Spline::geometryFingerprint(native->getData()) == nativeGeometryFingerprint);
            CHECK(cutter->getData().startpoint == cutterGeometry.startpoint);
            CHECK(cutter->getData().endpoint == cutterGeometry.endpoint);
            CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);
            CHECK(f.context.messages.join('\n').contains(
                QStringLiteral("uncertain, tangent or overlapping intersections")));
            continue;
        }
        REQUIRE(native->isDeleted());
        REQUIRE(cutter->isDeleted());
        std::vector<RS_Entity*> fragments;
        for (auto* entity : live(f.graphic)) {
            if (entity == source) continue;
            REQUIRE(entity->rtti() != RS2::EntityOffsetCurve);
            fragments.push_back(entity);
        }
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntitySpline;
        }));
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntityLine;
        }));
        CHECK(native->hasEditableDefinition());
        CHECK_FALSE(native->sourceDefinition());
        QTemporaryDir directory;
        REQUIRE(directory.isValid());
        const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
            RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
        QString libreDwgReader = qEnvironmentVariable("DWGREAD");
        if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
        const auto close = [](double a, double b) {
            return std::abs(a - b) <= 2e-10 * std::max({1.0, std::abs(a), std::abs(b)});
        };
        const auto closeVector = [&](const RS_Vector& a, const RS_Vector& b) {
            return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
        };
        const auto sameGeometry = [&](const RS_Entity& expected, const RS_Entity& actual) {
            if (expected.rtti() != actual.rtti()) return false;
            if (const auto* a = dynamic_cast<const RS_Spline*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Spline*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                if (x.degree != y.degree || x.m_closedFlag != y.m_closedFlag ||
                    x.m_periodicFlag != y.m_periodicFlag || x.controlPoints.size() != y.controlPoints.size() ||
                    x.knotslist.size() != y.knotslist.size() || x.weights.size() != y.weights.size()) return false;
                for (std::size_t i = 0; i < x.controlPoints.size(); ++i)
                    if (!closeVector(x.controlPoints[i], y.controlPoints[i])) return false;
                for (std::size_t i = 0; i < x.knotslist.size(); ++i)
                    if (!close(x.knotslist[i], y.knotslist[i])) return false;
                for (std::size_t i = 0; i < x.weights.size(); ++i)
                    if (!close(x.weights[i], y.weights[i])) return false;
                return true;
            }
            if (const auto* a = dynamic_cast<const RS_Line*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Line*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                return (closeVector(x.startpoint, y.startpoint) && closeVector(x.endpoint, y.endpoint)) ||
                    (closeVector(x.startpoint, y.endpoint) && closeVector(x.endpoint, y.startpoint));
            }
            if (const auto* a = dynamic_cast<const RS_Ellipse*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Ellipse*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                if (x.isArc || y.isArc) {
                    if (!x.isArc || !y.isArc) return false;
                    const RS_Vector aStart = a->getStartpoint();
                    const RS_Vector aEnd = a->getEndpoint();
                    const RS_Vector bStart = b->getStartpoint();
                    const RS_Vector bEnd = b->getEndpoint();
                    const bool sameEndpoints = closeVector(aStart, bStart) && closeVector(aEnd, bEnd);
                    const bool reversedEndpoints = closeVector(aStart, bEnd) && closeVector(aEnd, bStart);
                    return closeVector(x.center, y.center) && close(x.majorP.magnitude(), y.majorP.magnitude()) &&
                        close(x.ratio, y.ratio) && close(a->getLength(), b->getLength()) &&
                        closeVector(a->getMiddlePoint(), b->getMiddlePoint()) && (sameEndpoints || reversedEndpoints);
                }
                return closeVector(x.center, y.center) && closeVector(x.majorP, y.majorP) &&
                    close(x.ratio, y.ratio) && close(x.angle1, y.angle1) && close(x.angle2, y.angle2) &&
                    x.reversed == y.reversed && x.isArc == y.isArc;
            }
            return false;
        };
        for (std::size_t i = 0; i < formats.size(); ++i) {
            const bool dwg = formats[i] == RS2::FormatDWG || formats[i] == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("typed-ellipse-trim-two-%1-%2").arg(profile).arg(i) +
                (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[i]));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                unsigned splines = 0, lines = 0, ellipses = 0;
                for (const QJsonValue& value : document.object().value("OBJECTS").toArray()) {
                    const QString entity = value.toObject().value("entity").toString();
                    splines += entity == QLatin1String("SPLINE");
                    lines += entity == QLatin1String("LINE");
                    ellipses += entity == QLatin1String("ELLIPSE");
                }
                CHECK(splines > 0);
                CHECK(lines > 0);
                CHECK(ellipses > 0);
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, formats[i]));
            const auto expectedEntities = live(f.graphic);
            const auto reopenedEntities = live(reopened);
            REQUIRE(reopenedEntities.size() == expectedEntities.size());
            CHECK(std::none_of(reopenedEntities.begin(), reopenedEntities.end(), [](const RS_Entity* entity) {
                return entity->rtti() == RS2::EntityOffsetCurve;
            }));
            std::vector<bool> consumed(reopenedEntities.size(), false);
            for (const auto* expected : expectedEntities) {
                bool found = false;
                for (std::size_t j = 0; j < reopenedEntities.size(); ++j) {
                    if (consumed[j] || !sameGeometry(*expected, *reopenedEntities[j])) continue;
                    consumed[j] = true;
                    found = true;
                    break;
                }
                CHECK(found);
            }
            unsigned reopenedEllipses = 0;
            for (const auto* entity : reopenedEntities) {
                if (entity->rtti() != RS2::EntityEllipse) continue;
                ++reopenedEllipses;
                const auto* loadedEllipse = dynamic_cast<const RS_Ellipse*>(entity);
                REQUIRE(loadedEllipse);
                CHECK(sameGeometry(*source, *loadedEllipse));
            }
            CAPTURE(formats[i]);
            CHECK(reopenedEllipses == 1);
        }
        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK_FALSE(source->isDeleted());
        CHECK(native->hasEditableDefinition());
        CHECK_FALSE(native->sourceDefinition());
        for (const auto* fragment : fragments) CHECK(fragment->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK(cutter->isDeleted());
        for (const auto* fragment : fragments) CHECK_FALSE(fragment->isDeleted());
    }
}

TEST_CASE("T Trim Two trims a worker-created high-origin ellipse offset",
          "[gui][lc-offsetcurve][trim-action][typed-source][ellipse][high-origin][trim-two]") {
    REQUIRE(lc::test::ensureSettings());
    const std::array<std::pair<double, double>, 4> profiles{{
        {0.25, 0.67}, {0.45, 0.17}, {0.45, 1.2}, {0.75, 0.67}}};
    for (const double frame : {1e12, 1e13})
    for (const auto& [axisRatio, rotation] : profiles)
    for (const bool towardCentre : {false, true})
    for (const bool ellipseFirst : {false, true}) {
        CAPTURE(frame, axisRatio, rotation, towardCentre, ellipseFirst);
        Drawing<true> f;
        const bool largestFrame = frame == 1e13;
        const double sourceRadius = largestFrame ? 1e6 : 1e5;
        const double offsetDistance = sourceRadius * .025;
        const RS_Vector center{frame, -2 * frame};
        const double zoom = largestFrame ? 5e-5 : .001;
        f.view.getViewPort()->setSize(640, 480);
        f.view.getViewPort()->setOffsetAndFactor(
            qRound(320.0 - center.x * zoom),
            qRound(240.0 - center.y * zoom), zoom);

        RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {sourceRadius * .2, 0}, .25, 0, 0, false});
        auto initial = LC_OffsetCurve::create(seed, offsetDistance, towardCentre);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);

        auto* source = new RS_Ellipse(&f.graphic,
            RS_EllipseData{center, {sourceRadius * std::cos(rotation), sourceRadius * std::sin(rotation)},
                           axisRatio, 0, 0, false});
        f.graphic.addEntity(source);
        f.graphic.select(QList<RS_Entity*>{previous}, true);
        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Ellipse);
        update.use(source);
        lc::test::awaitUpdate(update);
        INFO(f.context.messages.join('\n').toStdString());
        REQUIRE(update.lastReport().updated == 1);
        REQUIRE(previous->isDeleted());

        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        CHECK(native->definition().distance == offsetDistance);
        CHECK(native->definition().towardCentre == towardCentre);

        double first = 0, last = 0;
        REQUIRE(native->getParameterDomain(first, last));
        const RS_Vector major{std::cos(rotation), std::sin(rotation)};
        const RS_Vector target = center + major * (towardCentre ? .9 : 1.2) * sourceRadius;
        double nativePickParameter = first;
        double nearestDistance = std::numeric_limits<double>::infinity();
        for (unsigned i = 0; i <= 8192; ++i) {
            const double parameter = first + (last - first) * i / 8192.0;
            const double distance = native->getPointAt(parameter).distanceTo(target);
            if (distance >= nearestDistance) continue;
            nearestDistance = distance;
            nativePickParameter = parameter;
        }
        const RS_Vector nativePick = native->getPointAt(nativePickParameter);
        const RS_Vector minor{-major.y, major.x};
        const double cutterHalfLength = sourceRadius * axisRatio * 1.5;
        const RS_Vector cutterPick = center + minor * sourceRadius * axisRatio * .8;
        auto* cutter = new RS_Line(&f.graphic,
            RS_LineData{center - minor * cutterHalfLength,
                        center + minor * cutterHalfLength});
        f.graphic.addEntity(cutter);
        const auto trimProbe = LC_OffsetCacheTrim::prepare(
            native->getData(), *cutter, {nativePick});
        INFO("high-origin ellipse trim probe refusal=" << static_cast<int>(trimProbe.refusal)
             << " roots=" << trimProbe.rootCount
             << " fragments=" << trimProbe.fragments.size());
        REQUIRE(trimProbe.refusal == LC_OffsetCacheTrim::Refusal::None);

        const auto geometryBefore = RS_Spline::geometryFingerprint(native->getData());
        f.action->click(ellipseFirst ? nativePick : cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        RS_Entity* expectedLimit = ellipseFirst ? static_cast<RS_Entity*>(native) : cutter;
        CHECK(f.action->retainedLimit() == expectedLimit);
        const auto undoEpoch = f.graphic.undoStateEpoch();
        f.action->hover(ellipseFirst ? cutterPick : nativePick);
        f.action->waitForTrim();
        REQUIRE_FALSE(f.action->m_preview->isEmpty());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(f.graphic.undoStateEpoch() == undoEpoch);
        f.action->click(ellipseFirst ? cutterPick : nativePick);
        INFO(f.context.messages.join('\n').toStdString());
        REQUIRE(native->isDeleted());
        REQUIRE(cutter->isDeleted());
        CHECK(native->hasEditableDefinition());
        CHECK_FALSE(native->sourceDefinition());
        std::vector<RS_Entity*> fragments;
        for (auto* entity : live(f.graphic)) {
            if (entity == source) continue;
            CHECK(entity->rtti() != RS2::EntityOffsetCurve);
            fragments.push_back(entity);
        }
        REQUIRE(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntitySpline;
        }));

        QTemporaryDir directory;
        REQUIRE(directory.isValid());
        const std::array<RS2::FormatType, 5> formats{{
            RS2::FormatDXFRW14, RS2::FormatDXFRW2000, RS2::FormatDWG,
            RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
        const double geometryTolerance = largestFrame ? .02 : 1e-3;
        const auto sameRoundTripGeometry = [geometryTolerance](const RS_Entity& expected,
                                                               const RS_Entity& actual) {
            if (expected.rtti() != actual.rtti()) return false;
            if (const auto* a = dynamic_cast<const RS_Spline*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Spline*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                if (x.degree != y.degree || x.m_closedFlag != y.m_closedFlag ||
                    x.m_periodicFlag != y.m_periodicFlag ||
                    x.controlPoints.size() != y.controlPoints.size() ||
                    x.knotslist.size() != y.knotslist.size() ||
                    x.weights.size() != y.weights.size())
                    return false;
                for (std::size_t i = 0; i < x.controlPoints.size(); ++i)
                    if (x.controlPoints[i].distanceTo(y.controlPoints[i]) > geometryTolerance)
                        return false;
                for (std::size_t i = 0; i < x.knotslist.size(); ++i)
                    if (std::abs(x.knotslist[i] - y.knotslist[i]) > 1e-12)
                        return false;
                for (std::size_t i = 0; i < x.weights.size(); ++i)
                    if (std::abs(x.weights[i] - y.weights[i]) > 1e-12)
                        return false;
                return true;
            }
            if (const auto* a = dynamic_cast<const RS_Ellipse*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Ellipse*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                return x.center.distanceTo(y.center) <= geometryTolerance &&
                       x.majorP.distanceTo(y.majorP) <= geometryTolerance &&
                       std::abs(x.ratio - y.ratio) <= 1e-12 &&
                       std::abs(x.angle1 - y.angle1) <= 1e-12 &&
                       std::abs(x.angle2 - y.angle2) <= 1e-12 &&
                       x.isArc == y.isArc && x.reversed == y.reversed;
            }
            if (const auto* a = dynamic_cast<const RS_Line*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Line*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                const bool same = x.startpoint.distanceTo(y.startpoint) <= geometryTolerance &&
                                  x.endpoint.distanceTo(y.endpoint) <= geometryTolerance;
                const bool reversed = x.startpoint.distanceTo(y.endpoint) <= geometryTolerance &&
                                      x.endpoint.distanceTo(y.startpoint) <= geometryTolerance;
                return same || reversed;
            }
            return trimGeometry(expected) == trimGeometry(actual);
        };
        for (std::size_t i = 0; i < formats.size(); ++i) {
            const bool dwg = formats[i] == RS2::FormatDWG ||
                             formats[i] == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("high-origin-ellipse-trim-%1-%2")
                    .arg(towardCentre ? 1 : 0)
                    .arg(ellipseFirst ? 1 : 0).arg(i) +
                (dwg ? ".dwg" : ".dxf")
            ).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[i]));
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, formats[i]));
            const auto reopenedEntities = live(reopened);
            CHECK(std::none_of(reopenedEntities.begin(), reopenedEntities.end(),
                               [](const RS_Entity* entity) {
                                   return entity->rtti() == RS2::EntityOffsetCurve;
                               }));
            const auto expectedEntities = live(f.graphic);
            REQUIRE(reopenedEntities.size() == expectedEntities.size());
            std::vector<bool> matched(reopenedEntities.size(), false);
            for (const auto* expected : expectedEntities) {
                bool found = false;
                for (std::size_t j = 0; j < reopenedEntities.size(); ++j) {
                    if (matched[j] || !sameRoundTripGeometry(*expected,
                                                              *reopenedEntities[j]))
                        continue;
                    matched[j] = true;
                    found = true;
                    break;
                }
                CAPTURE(formats[i], expected->rtti());
                CHECK(found);
            }
        }

        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(RS_Spline::geometryFingerprint(native->getData()) == geometryBefore);
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK(cutter->isDeleted());
        for (const auto* fragment : fragments) CHECK_FALSE(fragment->isDeleted());
    }
}

TEST_CASE("T Trim Two action trims worker-created typed spline and spline-points sources in both pick orders",
          "[gui][lc-offsetcurve][trim-action][typed-source][spline][fit-points][trim-two]") {
    for (unsigned profile = 0; profile <= 35; ++profile)
    for (const bool nativeFirst : {false, true}) {
        const bool fitPoints = profile >= 5;
        const bool splinePointsSource = profile == 2 || fitPoints;
        CAPTURE(profile, fitPoints, splinePointsSource, nativeFirst);
        Drawing<true> f;
        RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false});
        const double sourceOffset = profile == 25 ? 0.1 : profile == 26 ? 0.5 : 0.15;
        auto initial = LC_OffsetCurve::create(seed, sourceOffset, false);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);

        RS_Entity* source = nullptr;
        const auto kind = splinePointsSource ? LC_OffsetSourceGeometry::Kind::SplinePoints :
                                               LC_OffsetSourceGeometry::Kind::Spline;
        if (splinePointsSource) {
            LC_SplinePointsData data{false, false};
            if (fitPoints) {
                data.closed = profile >= 8;
                data.splinePoints = profile == 20 || profile == 21
                    ? std::vector<RS_Vector>{{0, 0}, {10, 0}, {10.001, 0}, {13, 8}, {4, 13}, {-3, 8}}
                    : profile >= 22 && profile <= 24
                    ? std::vector<RS_Vector>{}
                    : profile == 15 || profile == 16 || profile == 17 || profile == 18 || profile == 19
                    ? std::vector<RS_Vector>{}
                    : profile == 14
                    ? std::vector<RS_Vector>{}
                    : profile == 13
                    ? std::vector<RS_Vector>{{-4, 0}, {-2.5, 2.4}, {-0.5, 3.1},
                                             {2.2, 2.7}, {3.8, 0.6}, {1.8, -2.7},
                                             {-2.3, -2.1}}
                    : profile == 12
                    ? std::vector<RS_Vector>{{-4, 0}, {-1, 3.2}, {2.8, 2.1},
                                             {3.6, -1.8}, {-1.7, -3}}
                    : profile == 11
                    ? std::vector<RS_Vector>{{-4, 0}, {0, 3}, {4, 0}, {0, -3}}
                    : profile == 10
                    ? std::vector<RS_Vector>{{-5, 0}, {-4, 3}, {-1, 5}, {3, 4},
                                             {5, 0}, {3, -4}, {-1, -5}, {-4, -3}}
                    : profile >= 8
                        ? std::vector<RS_Vector>{{-4, 0}, {-2, 3}, {2, 3},
                                                 {4, 0}, {2, -3}, {-2, -3}}
                    : profile == 6
                    ? std::vector<RS_Vector>{{-4, 0}, {-1, 2}, {2, 3}, {4, 0}}
                    : profile == 7
                        ? std::vector<RS_Vector>{{-4, 0}, {-2, 1.5}, {0, 2.2},
                                                 {2, 1.4}, {4, 0}}
                        : std::vector<RS_Vector>{{-4, 0}, {0, 3}, {4, 0}};
                if (profile == 25 || profile == 26)
                    data.splinePoints = {{-4.0, -2.0}, {4.0, -2.0}, {0.0, 5.0}};
                if (profile == 27) {
                    data.splinePoints.clear();
                    constexpr double twoPi = 6.2831853071795864769;
                    for (unsigned i = 0; i < 11; ++i) {
                        const double angle = twoPi * static_cast<double>(i) / 11.0;
                        const double radius = 4.0 + 0.3 * std::cos(3.0 * angle) +
                                              0.1 * std::sin(2.0 * angle);
                        data.splinePoints.emplace_back(radius * std::cos(angle),
                                                       0.85 * radius * std::sin(angle));
                    }
                }
                if (profile == 28) {
                    data.splinePoints.clear();
                    constexpr double twoPi = 6.2831853071795864769;
                    for (unsigned i = 0; i < 13; ++i) {
                        const double angle = twoPi * static_cast<double>(i) / 13.0;
                        const double radius = 4.2 + 0.25 * std::cos(4.0 * angle) -
                                              0.12 * std::sin(3.0 * angle);
                        data.splinePoints.emplace_back(radius * std::cos(angle),
                                                       0.9 * radius * std::sin(angle));
                    }
                }
                if (profile == 29) {
                    data.splinePoints.clear();
                    data.fitTolerance = 3e-4;
                    constexpr double twoPi = 6.2831853071795864769;
                    for (unsigned i = 0; i < 14; ++i) {
                        const double angle = twoPi * static_cast<double>(i) / 14.0;
                        const double radius = 4.1 + 0.22 * std::cos(5.0 * angle) +
                                              0.09 * std::sin(3.0 * angle);
                        data.splinePoints.emplace_back(radius * std::cos(angle),
                                                       0.88 * radius * std::sin(angle));
                    }
                }
                if (profile == 30) {
                    data.splinePoints.clear();
                    data.fitTolerance = 1e-4;
                    constexpr double twoPi = 6.2831853071795864769;
                    for (unsigned i = 0; i < 15; ++i) {
                        const double angle = twoPi * static_cast<double>(i) / 15.0;
                        const double radius = 4.3 + 0.18 * std::cos(5.0 * angle) +
                                              0.14 * std::sin(3.0 * angle);
                        data.splinePoints.emplace_back(radius * std::cos(angle),
                                                       0.9 * radius * std::sin(angle));
                    }
                }
                if (profile == 31) {
                    data.splinePoints.clear();
                    data.fitTolerance = 1e-4;
                    constexpr double twoPi = 6.2831853071795864769;
                    for (unsigned i = 0; i < 32; ++i) {
                        const double angle = twoPi * static_cast<double>(i) / 32.0;
                        const double radius = 5.0 + 0.15 * std::cos(5.0 * angle) +
                                              0.06 * std::sin(7.0 * angle);
                        data.splinePoints.emplace_back(radius * std::cos(angle),
                                                       0.92 * radius * std::sin(angle));
                    }
                }
                if (profile == 32) {
                    data.splinePoints.clear();
                    data.fitTolerance = 1e-4;
                    constexpr double twoPi = 6.2831853071795864769;
                    for (unsigned i = 0; i < 64; ++i) {
                        const double angle = twoPi * static_cast<double>(i) / 64.0;
                        const double radius = 5.5 + 0.12 * std::cos(7.0 * angle) +
                                              0.05 * std::sin(13.0 * angle);
                        data.splinePoints.emplace_back(radius * std::cos(angle),
                                                       0.94 * radius * std::sin(angle));
                    }
                }
                if (profile == 33) {
                    data.splinePoints.clear();
                    data.fitTolerance = 1e-4;
                    constexpr double twoPi = 6.2831853071795864769;
                    for (unsigned i = 0; i < 128; ++i) {
                        const double angle = twoPi * static_cast<double>(i) / 128.0;
                        const double radius = 6.0 + 0.1 * std::cos(5.0 * angle) +
                                              0.04 * std::sin(9.0 * angle);
                        data.splinePoints.emplace_back(radius * std::cos(angle),
                                                       0.96 * radius * std::sin(angle));
                    }
                }
                if (profile == 34) {
                    data.splinePoints.clear();
                    data.fitTolerance = 1e-4;
                    constexpr double twoPi = 6.2831853071795864769;
                    for (unsigned i = 0; i < 256; ++i) {
                        const double angle = twoPi * static_cast<double>(i) / 256.0;
                        const double radius = 6.5 + 0.08 * std::cos(7.0 * angle) +
                                              0.03 * std::sin(13.0 * angle);
                        data.splinePoints.emplace_back(radius * std::cos(angle),
                                                       0.97 * radius * std::sin(angle));
                    }
                }
                if (profile == 35) {
                    data.closed = false;
                    data.splinePoints.clear();
                    data.fitTolerance = 1e-4;
                    for (unsigned i = 0; i < 256; ++i) {
                        const double t = static_cast<double>(i) / 255.0;
                        const double x = -8.0 + 16.0 * t;
                        const double y = 1.8 * std::sin(0.42 * x) +
                                         0.24 * std::sin(1.1 * x) + 0.04 * x;
                        data.splinePoints.emplace_back(x, y);
                    }
                }
                if (profile == 14) {
                    constexpr double twoPi = 6.2831853071795864769;
                    for (unsigned i = 0; i < 9; ++i) {
                        const double angle = twoPi * static_cast<double>(i) / 9.0;
                        const double radius = 3.2 + 0.4 * std::cos(2.0 * angle);
                        data.splinePoints.emplace_back(radius * std::cos(angle),
                                                       0.8 * radius * std::sin(angle));
                    }
                }
                if (profile == 15) {
                    constexpr double twoPi = 6.2831853071795864769;
                    for (unsigned i = 0; i < 10; ++i) {
                        const double angle = twoPi * static_cast<double>(i) / 10.0;
                        const double radius = 3.0 + 0.35 * std::cos(3.0 * angle);
                        data.splinePoints.emplace_back(radius * std::cos(angle),
                                                       0.75 * radius * std::sin(angle));
                    }
                }
                if (profile == 16 || profile == 17 || profile == 18 || profile == 19) {
                    constexpr double twoPi = 6.2831853071795864769;
                    for (unsigned i = 0; i < 12; ++i) {
                        const double angle = twoPi * static_cast<double>(i) / 12.0;
                        const double radius = 3.4 + 0.05 * std::cos(3.0 * angle) +
                                              0.02 * std::sin(5.0 * angle);
                        data.splinePoints.emplace_back(radius * std::cos(angle),
                                                       0.9 * radius * std::sin(angle));
                    }
                    if (profile == 18) data.splinePoints.push_back(data.splinePoints.front());
                    if (profile == 19)
                        data.splinePoints.insert(data.splinePoints.begin() + 6, data.splinePoints[5]);
                }
                if (profile == 9 || profile == 15 || profile == 17) {
                    const double angle = profile == 9 ? 0.47 : profile == 15 ? 0.29 : 0.63;
                    const double scale = profile == 9 ? 2.0 : profile == 15 ? 1.5 : 2.25;
                    const double originX = profile == 9 ? 18.0 : profile == 15 ? -12.0 : 35.0;
                    const double originY = profile == 9 ? -11.0 : profile == 15 ? 8.0 : -21.0;
                    const double cosine = std::cos(angle);
                    const double sine = std::sin(angle);
                    for (RS_Vector& point : data.splinePoints) {
                        const double x = point.x;
                        const double y = point.y;
                        point = {originX + scale * (x * cosine - y * sine),
                                 originY + scale * (x * sine + y * cosine)};
                    }
                }
                if (profile == 21) {
                    const double angle = 0.63;
                    constexpr double scale = 2.25;
                    const double cosine = std::cos(angle);
                    const double sine = std::sin(angle);
                    for (RS_Vector& point : data.splinePoints) {
                        const double x = point.x;
                        const double y = point.y;
                        point = {35.0 + scale * (x * cosine - y * sine),
                                 -21.0 + scale * (x * sine + y * cosine)};
                    }
                }
                if (profile >= 22 && profile <= 24) {
                    constexpr double starPi = 3.14159265358979323846;
                    for (const std::size_t vertex : {0U, 2U, 4U, 1U, 3U}) {
                        const double angle = -starPi / 2.0 +
                            2.0 * starPi * static_cast<double>(vertex) / 5.0;
                        data.splinePoints.emplace_back(8.0 * std::cos(angle),
                                                       8.0 * std::sin(angle));
                    }
                    if (profile == 23) {
                        const RS_Vector repeatedStarPoint = data.splinePoints[1];
                        data.splinePoints.insert(data.splinePoints.begin() + 2,
                                                 repeatedStarPoint);
                    }
                    if (profile == 24) {
                        for (RS_Vector& point : data.splinePoints) {
                            const double reflectedY = -point.y;
                            point = {1.0e6 + 1.25 * (point.x * std::cos(0.73) -
                                                      reflectedY * std::sin(0.73)),
                                     -2.0e6 + 1.25 * (point.x * std::sin(0.73) +
                                                      reflectedY * std::cos(0.73))};
                        }
                        std::reverse(data.splinePoints.begin(), data.splinePoints.end());
                    }
                    data.fitTolerance = 1e-8;
                }
                if (profile == 26) {
                    for (RS_Vector& point : data.splinePoints) {
                        const double reflectedY = -point.y;
                        point = {35.0 + 1.5 * (point.x * std::cos(0.63) -
                                                reflectedY * std::sin(0.63)),
                                 -21.0 + 1.5 * (point.x * std::sin(0.63) +
                                                reflectedY * std::cos(0.63))};
                    }
                    std::reverse(data.splinePoints.begin(), data.splinePoints.end());
                }
                if (profile == 10) data.fitTolerance = 1e-4;
                if (profile == 15 || profile == 17 || profile == 18 || profile == 19)
                    data.fitTolerance = 2e-4;
                if (profile == 7) data.fitTolerance = 1e-3;
            } else {
                data.useControlPoints = true;
                data.controlPoints = {{-4, 0}, {0, 3}, {4, 0}};
            }
            source = new LC_SplinePoints(&f.graphic, data);
            if (profile == 6) {
                auto* fitted = dynamic_cast<LC_SplinePoints*>(source);
                REQUIRE(fitted);
                LC_CurveJet startJet;
                LC_CurveJet endJet;
                REQUIRE(fitted->tryEvaluateJet(0.0, LC_CurveEvaluationSide::Right, startJet));
                REQUIRE(fitted->tryEvaluateJet(static_cast<double>(fitted->getSegmentCount()),
                    LC_CurveEvaluationSide::Left, endJet));
                auto& fittedData = fitted->getData();
                fittedData.fitTangentsConstrained = true;
                fittedData.fitStartTangent = startJet.first;
                fittedData.fitEndTangent = endJet.first;
                REQUIRE(fitted->fitTangentsMatchCurve());
            }
        } else if (profile == 4) {
            RS_Spline closed(nullptr, RS_SplineData{3, false});
            const std::array<RS_Vector, 8> controlPoints{{
                {-4, 0}, {-3, 3}, {0, 4}, {3, 3},
                {4, 0}, {3, -3}, {0, -4}, {-3, -3}}};
            const std::array<double, 8> weights{{1, .8, 1.1, .9, 1.2, .85, 1.05, .95}};
            for (std::size_t i = 0; i < controlPoints.size(); ++i)
                closed.addControlPoint(controlPoints[i], weights[i]);
            closed.setClosed(true);
            RS_SplineData data = closed.getData();
            source = new RS_Spline(&f.graphic, data);
        } else {
            RS_SplineData data{profile == 1 ? 3 : 2, false};
            if (profile == 1) {
                data.controlPoints = {{-5, 0}, {-3, 3}, {-1, 2}, {2, 4}, {5, 0}};
                data.knotslist = {0, 0, 0, 0, .45, 1, 1, 1, 1};
                data.weights = {1, .8, 1.2, .9, 1};
            } else if (profile == 3) {
                data = RS_SplineData{3, false};
                data.controlPoints = {{-5, 0}, {-4, 2}, {-2, 3}, {1, 3}, {3, 2}, {5, 0}};
                data.knotslist = {0, 0, 0, 0, .45, .45, 1, 1, 1, 1};
                data.weights = {1, .8, 1.2, .9, 1.1, 1};
            } else {
                data.controlPoints = {{-4, 0}, {0, 3}, {4, 0}};
                data.knotslist = {0, 0, 0, 1, 1, 1};
                data.weights = {1, .8, 1};
            }
            source = new RS_Spline(&f.graphic, data);
        }
        f.graphic.addEntity(source);
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(kind);
        update.use(source);
        lc::test::awaitUpdate(update);
        REQUIRE(update.lastReport().updated == 1);
        REQUIRE(previous->isDeleted());
        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        REQUIRE(native->hasGeneralizedSourceDefinition());
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == kind);

        double first = 0, last = 0;
        REQUIRE(native->getParameterDomain(first, last));
        const auto breaks = native->getBreakParameters();
        REQUIRE(breaks.size() >= 2);
        const double step = std::max(1e-7, (last - first) * 1e-5);
        double parameter = breaks.front() + (breaks[1] - breaks.front()) * .43;
        auto tangentAt = [&](const double t) {
            const RS_Vector before = native->getPointAt(t - step);
            const RS_Vector after = native->getPointAt(t + step);
            RS_Vector tangent{after.x - before.x, after.y - before.y};
            if (tangent.magnitude() > RS_TOLERANCE) tangent /= tangent.magnitude();
            return tangent;
        };
        RS_Vector tangent = tangentAt(parameter);
        REQUIRE(tangent.magnitude() > RS_TOLERANCE);
        RS_Vector intersection = native->getPointAt(parameter);
        RS_Vector cutterDirection{-tangent.y, tangent.x};
        REQUIRE(cutterDirection.magnitude() > RS_TOLERANCE);
        cutterDirection /= cutterDirection.magnitude();
        cutterDirection += tangent;
        REQUIRE(cutterDirection.magnitude() > RS_TOLERANCE);
        cutterDirection = cutterDirection / cutterDirection.magnitude();
        cutterDirection *= 16.0;
        RS_Vector nativePick = native->getPointAt(breaks.front() + (breaks[1] - breaks.front()) * .9);
        if (profile == 16 || profile == 18 || profile == 19 || profile == 20 || profile == 21 || profile >= 22) {
            bool foundCertifiedCut = false;
            int lastRefusal = -1;
            RS_LineData lastCutterData;
            RS_Vector lastPick;
            for (std::size_t interval = 0; interval + 1 < breaks.size() && !foundCertifiedCut; ++interval) {
                for (const double contactFraction : {.1, .2, .3, .4, .5, .6, .7, .8, .9}) {
                    parameter = breaks[interval] +
                        (breaks[interval + 1] - breaks[interval]) * contactFraction;
                    tangent = tangentAt(parameter);
                    if (tangent.magnitude() <= RS_TOLERANCE) continue;
                    intersection = native->getPointAt(parameter);
                    for (const double tangentSkew : {-1.0, 0.0, 1.0}) {
                        RS_Vector direction{-tangent.y, tangent.x};
                        direction += tangent * tangentSkew;
                        if (direction.magnitude() <= RS_TOLERANCE) continue;
                        direction = direction / direction.magnitude() * 16.0;
                        cutterDirection = direction;
                        RS_Line probeCutter(nullptr,
                            RS_LineData{intersection - direction, intersection + direction});
                        const RS_Vector pick = native->getPointAt(
                            breaks[interval] + (breaks[interval + 1] - breaks[interval]) * .95);
                        lastCutterData = probeCutter.getData();
                        lastPick = pick;
                        const auto probe = LC_OffsetCacheTrim::prepare(native->getData(), probeCutter, {pick});
                        lastRefusal = static_cast<int>(probe.refusal);
                        if (probe.refusal != LC_OffsetCacheTrim::Refusal::None) continue;
                        nativePick = pick;
                        foundCertifiedCut = true;
                        break;
                    }
                    if (foundCertifiedCut) break;
                }
            }
            CAPTURE(lastRefusal);
            if (profile >= 22 && !foundCertifiedCut) {
                CHECK(lastRefusal == static_cast<int>(LC_OffsetCacheTrim::Refusal::AmbiguousRoots));
                nativePick = lastPick;
                auto* cutter = new RS_Line(&f.graphic, lastCutterData);
                f.graphic.addEntity(cutter);
                const RS_Vector cutterPick = intersection + cutterDirection * .75;
                const auto undoEpoch = f.graphic.undoStateEpoch();
                f.action->click(nativeFirst ? nativePick : cutterPick);
                REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
                f.action->hover(nativeFirst ? cutterPick : nativePick);
                CHECK(f.action->m_preview->isEmpty());
                CHECK_FALSE(native->isDeleted());
                CHECK_FALSE(cutter->isDeleted());
                f.action->click(nativeFirst ? cutterPick : nativePick);
                CHECK_FALSE(native->isDeleted());
                CHECK_FALSE(cutter->isDeleted());
                CHECK(f.graphic.undoStateEpoch() == undoEpoch);
                CHECK(native->hasGeneralizedSourceDefinition());
                CHECK(native->sourceDefinition()->source.kind() == kind);
                continue;
            }
            REQUIRE(foundCertifiedCut);
        }
        auto* cutter = new RS_Line(&f.graphic,
            RS_LineData{intersection - cutterDirection, intersection + cutterDirection});
        f.graphic.addEntity(cutter);

        const RS_Vector cutterPick = intersection + cutterDirection * .75;
        f.action->click(nativeFirst ? nativePick : cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        const auto previewUndoEpoch = f.graphic.undoStateEpoch();
        f.action->hover(nativeFirst ? cutterPick : nativePick);
        CHECK_FALSE(f.action->m_preview->isEmpty());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);

        f.action->click(nativeFirst ? cutterPick : nativePick);
        INFO(f.context.messages.join('\n').toStdString());
        REQUIRE(native->isDeleted());
        REQUIRE(cutter->isDeleted());
        std::vector<RS_Entity*> fragments;
        for (auto* entity : live(f.graphic)) {
            if (entity == source) continue;
            REQUIRE(entity->rtti() != RS2::EntityOffsetCurve);
            fragments.push_back(entity);
        }
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntitySpline;
        }));
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntityLine;
        }));
        QTemporaryDir directory;
        REQUIRE(directory.isValid());
        const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
            RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
        QString libreDwgReader = qEnvironmentVariable("DWGREAD");
        if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
        const auto close = [](double a, double b) {
            return std::abs(a - b) <= 2e-10 * std::max({1.0, std::abs(a), std::abs(b)});
        };
        const auto closeVector = [&](const RS_Vector& a, const RS_Vector& b) {
            return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
        };
        const auto sameSpline = [&](const RS_SplineData& a, const RS_SplineData& b) {
            if (a.degree != b.degree || a.m_closedFlag != b.m_closedFlag ||
                a.m_periodicFlag != b.m_periodicFlag || a.controlPoints.size() != b.controlPoints.size() ||
                a.knotslist.size() != b.knotslist.size() || a.weights.size() != b.weights.size()) return false;
            for (std::size_t i = 0; i < a.controlPoints.size(); ++i)
                if (!closeVector(a.controlPoints[i], b.controlPoints[i])) return false;
            for (std::size_t i = 0; i < a.knotslist.size(); ++i)
                if (!close(a.knotslist[i], b.knotslist[i])) return false;
            for (std::size_t i = 0; i < a.weights.size(); ++i)
                if (!close(a.weights[i], b.weights[i])) return false;
            return true;
        };
        const auto sameGeometry = [&](const RS_Entity& expected, const RS_Entity& actual) {
            if (const auto* a = dynamic_cast<const LC_SplinePoints*>(&expected)) {
                if (const auto* b = dynamic_cast<const LC_SplinePoints*>(&actual)) {
                    const auto& x = a->getData();
                    const auto& y = b->getData();
                    if (x.closed != y.closed) return false;
                    if (x.closed && !x.cut && !x.useControlPoints && y.cut) {
                        const double sourceEnd = static_cast<double>(a->getSegmentCount());
                        const double actualEnd = static_cast<double>(b->getSegmentCount());
                        if (!(sourceEnd > 0.0) || !(actualEnd > 0.0)) return false;
                        for (unsigned sample = 0; sample <= 128; ++sample) {
                            const double fraction = static_cast<double>(sample) / 128.0;
                            LC_CurveJet sourceJet;
                            LC_CurveJet actualJet;
                            const auto side = sample == 128
                                ? LC_CurveEvaluationSide::Left
                                : LC_CurveEvaluationSide::Right;
                            if (!a->tryEvaluateJet(sourceEnd * fraction, side, sourceJet) ||
                                !b->tryEvaluateJet(actualEnd * fraction, side, actualJet))
                                return false;
                            const double scale = std::max({1.0, std::abs(sourceJet.point.x),
                                std::abs(sourceJet.point.y), std::abs(actualJet.point.x),
                                std::abs(actualJet.point.y)});
                            if (sourceJet.point.distanceTo(actualJet.point) > 2e-8 * scale)
                                return false;
                        }
                        return true;
                    }
                    if (x.cut != y.cut) return false;
                    if (x.fitTangentsConstrained &&
                        (!y.fitTangentsConstrained ||
                         !closeVector(x.fitStartTangent, y.fitStartTangent) ||
                         !closeVector(x.fitEndTangent, y.fitEndTangent))) return false;
                    if (x.useControlPoints) {
                        if (x.controlPoints.size() != y.controlPoints.size()) return false;
                        for (std::size_t i = 0; i < x.controlPoints.size(); ++i)
                            if (!closeVector(x.controlPoints[i], y.controlPoints[i])) return false;
                    } else {
                        if (x.splinePoints.size() != y.splinePoints.size()) return false;
                        for (std::size_t i = 0; i < x.splinePoints.size(); ++i)
                            if (!closeVector(x.splinePoints[i], y.splinePoints[i])) return false;
                        if (!close(x.fitTolerance, y.fitTolerance)) return false;
                    }
                    return true;
                }
                const auto* b = dynamic_cast<const RS_Spline*>(&actual);
                if (!b) return false;
                const auto& sourceData = a->getData();
                const auto& loadedData = b->getData();
                if (sourceData.fitTangentsConstrained &&
                    (!loadedData.fitTangentsConstrained ||
                     !closeVector(sourceData.fitStartTangent, loadedData.fitStartTangent) ||
                     !closeVector(sourceData.fitEndTangent, loadedData.fitEndTangent)))
                    return false;
                if (!sourceData.useControlPoints &&
                    !close(sourceData.fitTolerance, loadedData.fitTolerance)) return false;
                if (profile < 6) {
                    RS_SplineData data{2, false};
                    data.controlPoints = a->getControlPoints();
                    const auto n = data.controlPoints.size();
                    if (n < 3) return false;
                    data.knotslist.insert(data.knotslist.end(), 3, 0.0);
                    for (std::size_t i = 3; i < n; ++i)
                        data.knotslist.push_back(double(i - 2) / double(n - 2));
                    data.knotslist.insert(data.knotslist.end(), 3, 1.0);
                    return sameSpline(data, loadedData);
                }
                double loadedStart = 0.0, loadedEnd = 0.0;
                if (!b->getParameterDomain(loadedStart, loadedEnd) ||
                    !(loadedEnd > loadedStart)) return false;
                const double sourceEnd = static_cast<double>(a->getSegmentCount());
                if (!(sourceEnd > 0.0)) return false;
                for (unsigned sample = 0; sample <= 128; ++sample) {
                    const double fraction = static_cast<double>(sample) / 128.0;
                    const double sourceParameter = sourceEnd * fraction;
                    const double loadedParameter = loadedStart +
                        (loadedEnd - loadedStart) * fraction;
                    LC_CurveJet sourceJet;
                    LC_CurveJet loadedJet;
                    if (!a->tryEvaluateJet(sourceParameter,
                            sample == 128 ? LC_CurveEvaluationSide::Left
                                          : LC_CurveEvaluationSide::Right,
                            sourceJet) ||
                        !b->tryEvaluateJet(loadedParameter,
                            sample == 128 ? LC_CurveEvaluationSide::Left
                                          : LC_CurveEvaluationSide::Right,
                            loadedJet)) return false;
                    const auto& sourcePoint = sourceJet.point;
                    const auto& loadedPoint = loadedJet.point;
                    const double scale = std::max({1.0, std::abs(sourcePoint.x),
                        std::abs(sourcePoint.y), std::abs(loadedPoint.x),
                        std::abs(loadedPoint.y)});
                    if (sourcePoint.distanceTo(loadedPoint) > 2e-8 * scale) return false;
                }
                return true;
            }
            if (expected.rtti() != actual.rtti()) return false;
            if (const auto* a = dynamic_cast<const RS_Spline*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Spline*>(&actual);
                return b && sameSpline(a->getData(), b->getData());
            }
            if (const auto* a = dynamic_cast<const RS_Line*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Line*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                return (closeVector(x.startpoint, y.startpoint) && closeVector(x.endpoint, y.endpoint)) ||
                    (closeVector(x.startpoint, y.endpoint) && closeVector(x.endpoint, y.startpoint));
            }
            return false;
        };
        for (std::size_t i = 0; i < formats.size(); ++i) {
            const bool dwg = formats[i] == RS2::FormatDWG || formats[i] == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("typed-spline-trim-two-%1-%2").arg(profile).arg(i) +
                    (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[i]));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                unsigned splines = 0, lines = 0;
                for (const QJsonValue& value : document.object().value("OBJECTS").toArray()) {
                    const QString entity = value.toObject().value("entity").toString();
                    splines += entity == QLatin1String("SPLINE");
                    lines += entity == QLatin1String("LINE");
                }
                CHECK(splines > 0);
                CHECK(lines > 0);
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, formats[i]));
            const auto expectedEntities = live(f.graphic);
            const auto reopenedEntities = live(reopened);
            REQUIRE(reopenedEntities.size() == expectedEntities.size());
            CHECK(std::none_of(reopenedEntities.begin(), reopenedEntities.end(), [](const RS_Entity* entity) {
                return entity->rtti() == RS2::EntityOffsetCurve;
            }));
            std::vector<bool> consumed(reopenedEntities.size(), false);
            for (const auto* expected : expectedEntities) {
                CAPTURE(profile, fitPoints, nativeFirst, formats[i], expected->rtti());
                bool found = false;
                for (std::size_t j = 0; j < reopenedEntities.size(); ++j) {
                    if (consumed[j] || !sameGeometry(*expected, *reopenedEntities[j])) continue;
                    consumed[j] = true;
                    found = true;
                    break;
                }
                CHECK(found);
            }
            CAPTURE(profile, fitPoints, nativeFirst, formats[i]);
        }
        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK_FALSE(source->isDeleted());
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == kind);
        for (const auto* fragment : fragments) CHECK(fragment->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK(cutter->isDeleted());
        for (const auto* fragment : fragments) CHECK_FALSE(fragment->isDeleted());
    }
}

TEST_CASE("T Trim Two action trims worker-created mixed-bulge and high-segment polyline sources in both pick orders",
          "[gui][lc-offsetcurve][trim-action][typed-source][polyline][trim-two]") {
    for (const unsigned segmentCount : {0u, 256u})
    for (const unsigned profileKind : {0u, 1u, 2u, 3u})
    for (const double sourceBulge : {1.0, .5})
        for (const bool addThirdSpan : {false, true})
        for (const bool trimLineSpan : {false, true}) for (const bool polylineFirst : {false, true}) {
        if ((profileKind && !segmentCount) ||
            (segmentCount && (sourceBulge != 1.0 || addThirdSpan || trimLineSpan))) continue;
        CAPTURE(segmentCount, profileKind, sourceBulge, addThirdSpan, trimLineSpan, polylineFirst);
        Drawing<true> f;
        RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false});
        auto initial = LC_OffsetCurve::create(seed, .15, false);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);
        auto* source = new RS_Polyline(&f.graphic);
        if (segmentCount) {
            for (unsigned i = 0; i <= segmentCount; ++i) {
                const double x = 0.1 * static_cast<double>(i);
                double y = 0.02 * x;
                if (profileKind == 1 && x > 6.4) y += 0.1 * (x - 6.4);
                if (profileKind == 2) y = 0.15 * std::sin(0.3 * x) + 0.02 * x;
                if (profileKind == 3) y = 0.001 * x * x;
                source->addVertex({x, y});
            }
        }
        else {
            source->addVertex({0, 0}, sourceBulge);
            source->addVertex({2, 0});
            source->addVertex({2, 2});
            if (addThirdSpan) source->addVertex({0, 2});
        }
        source->endPolyline();
        f.graphic.addEntity(source);
        const bool belowFiveOutputCap = sourceBulge == .5 && addThirdSpan && !trimLineSpan && !polylineFirst;
        if (belowFiveOutputCap) f.action->options.maxOutputEntities = 4;
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Polyline);
        update.use(source);
        lc::test::awaitUpdate(update);
        REQUIRE(update.lastReport().updated == 1);
        REQUIRE(previous->isDeleted());
        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        REQUIRE(native->hasGeneralizedSourceDefinition());
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Polyline);

        double first = 0, last = 0;
        REQUIRE(native->getParameterDomain(first, last));
        const auto breaks = native->getBreakParameters();
        REQUIRE(breaks.size() >= (trimLineSpan ? 3u : 2u));
        if (addThirdSpan) REQUIRE(breaks.size() >= 4);
        const std::size_t span = segmentCount ? (breaks.size() - 2) / 2 :
                                 trimLineSpan ? breaks.size() - 2 : 0;
        const double parameter = breaks[span] + (breaks[span + 1] - breaks[span]) * .43;
        const RS_Vector intersection = native->getPointAt(parameter);
        const double step = std::max(1e-7, (last - first) * 1e-5);
        const RS_Vector before = native->getPointAt(parameter - step);
        const RS_Vector after = native->getPointAt(parameter + step);
        RS_Vector tangent{after.x - before.x, after.y - before.y};
        REQUIRE(tangent.magnitude() > RS_TOLERANCE);
        tangent /= tangent.magnitude();
        RS_Vector cutterDirection{-tangent.y, tangent.x};
        REQUIRE(cutterDirection.magnitude() > RS_TOLERANCE);
        cutterDirection /= cutterDirection.magnitude();
        if (segmentCount == 256) cutterDirection = {0.0, 1.0};
        else {
            cutterDirection += tangent;
            REQUIRE(cutterDirection.magnitude() > RS_TOLERANCE);
            cutterDirection = cutterDirection / cutterDirection.magnitude();
        }
        cutterDirection *= 16.0;
        auto* cutter = new RS_Line(&f.graphic,
            RS_LineData{intersection - cutterDirection, intersection + cutterDirection});
        f.graphic.addEntity(cutter);
        const RS_Vector nativePick = native->getPointAt(breaks[span] + (breaks[span + 1] - breaks[span]) * .9);
        const RS_Vector cutterPick = intersection + cutterDirection * .75;
        if (profileKind == 3) {
            const auto curveByLine = LC_OffsetCacheTrim::prepare(native->getData(), *cutter, {nativePick});
            RS_SplineData lineCache{1, false};
            lineCache.controlPoints = {cutter->getStartpoint(), cutter->getEndpoint()};
            lineCache.knotslist = {0, 0, 1, 1}; lineCache.weights = {1, 1};
            const auto lineByCurve = LC_OffsetCacheTrim::prepare(lineCache, *native, {cutterPick});
            CAPTURE(static_cast<int>(curveByLine.refusal), curveByLine.rootCount,
                    static_cast<int>(lineByCurve.refusal), lineByCurve.rootCount,
                    curveByLine.work.intersections.totalBoxPairs,
                    lineByCurve.work.intersections.totalBoxPairs,
                    lineByCurve.work.intersections.generatedLeaves);
            CHECK(curveByLine.refusal == LC_OffsetCacheTrim::Refusal::None);
            CHECK(curveByLine.rootCount == 1);
            CHECK(lineByCurve.refusal == LC_OffsetCacheTrim::Refusal::None);
            CHECK(lineByCurve.rootCount == 1);
        }
        f.action->click(polylineFirst ? nativePick : cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        const auto previewUndoEpoch = f.graphic.undoStateEpoch();
        f.action->hover(polylineFirst ? cutterPick : nativePick);
        if (belowFiveOutputCap) {
            CHECK(f.action->m_preview->isEmpty());
            CHECK_FALSE(native->isDeleted()); CHECK_FALSE(cutter->isDeleted());
            CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);
            const auto count = f.graphic.count();
            f.action->click(polylineFirst ? cutterPick : nativePick);
            CHECK_FALSE(native->isDeleted()); CHECK_FALSE(cutter->isDeleted());
            CHECK(f.graphic.count() == count);
            CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);
            REQUIRE_FALSE(f.context.messages.isEmpty());
            CHECK(f.context.messages.back().contains("nothing changed"));
            continue;
        }
        CHECK_FALSE(f.action->m_preview->isEmpty());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(f.graphic.undoStateEpoch() == previewUndoEpoch);

        f.action->click(polylineFirst ? cutterPick : nativePick);
        REQUIRE(native->isDeleted());
        REQUIRE(cutter->isDeleted());
        std::vector<RS_Entity*> fragments;
        for (auto* entity : live(f.graphic)) {
            if (entity == source) continue;
            REQUIRE(entity->rtti() != RS2::EntityOffsetCurve);
            fragments.push_back(entity);
        }
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntitySpline;
        }));
        CHECK(std::any_of(fragments.begin(), fragments.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntityLine;
        }));
        QTemporaryDir directory;
        REQUIRE(directory.isValid());
        const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
            RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
#ifdef DWGSUPPORT
        QString libreDwgReader = qEnvironmentVariable("DWGREAD");
        if (libreDwgReader.isEmpty()) libreDwgReader = QStandardPaths::findExecutable("dwgread");
#endif
        const auto close = [](double a, double b) {
            return std::abs(a - b) <= 2e-10 * std::max({1.0, std::abs(a), std::abs(b)});
        };
        const auto closeVector = [&](const RS_Vector& a, const RS_Vector& b) {
            return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
        };
        const auto sameSegment = [&](const RS_Entity& expected, const RS_Entity& actual) {
            if (expected.rtti() != actual.rtti()) return false;
            if (const auto* a = dynamic_cast<const RS_Spline*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Spline*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                if (x.degree != y.degree || x.m_closedFlag != y.m_closedFlag ||
                    x.m_periodicFlag != y.m_periodicFlag || x.controlPoints.size() != y.controlPoints.size() ||
                    x.knotslist.size() != y.knotslist.size() || x.weights.size() != y.weights.size()) return false;
                for (std::size_t i = 0; i < x.controlPoints.size(); ++i)
                    if (!closeVector(x.controlPoints[i], y.controlPoints[i])) return false;
                for (std::size_t i = 0; i < x.knotslist.size(); ++i)
                    if (!close(x.knotslist[i], y.knotslist[i])) return false;
                for (std::size_t i = 0; i < x.weights.size(); ++i)
                    if (!close(x.weights[i], y.weights[i])) return false;
                return true;
            }
            if (const auto* a = dynamic_cast<const RS_Line*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Line*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                return (closeVector(x.startpoint, y.startpoint) && closeVector(x.endpoint, y.endpoint)) ||
                    (closeVector(x.startpoint, y.endpoint) && closeVector(x.endpoint, y.startpoint));
            }
            if (const auto* a = dynamic_cast<const RS_Arc*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Arc*>(&actual);
                if (!b) return false;
                const auto& x = a->getData();
                const auto& y = b->getData();
                return closeVector(x.center, y.center) && close(x.radius, y.radius) &&
                    close(x.angle1, y.angle1) && close(x.angle2, y.angle2) && x.reversed == y.reversed;
            }
            return false;
        };
        const auto sameGeometry = [&](const RS_Entity& expected, const RS_Entity& actual) {
            if (const auto* a = dynamic_cast<const RS_Polyline*>(&expected)) {
                const auto* b = dynamic_cast<const RS_Polyline*>(&actual);
                if (!b || a->isClosed() != b->isClosed() || a->size() != b->size()) return false;
                for (qsizetype i = 0; i < a->size(); ++i)
                    if (!sameSegment(*a->entityAt(i), *b->entityAt(i))) return false;
                return true;
            }
            return sameSegment(expected, actual);
        };
        for (std::size_t i = 0; i < formats.size(); ++i) {
            const bool dwg = formats[i] == RS2::FormatDWG || formats[i] == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("typed-polyline-trim-two-%1").arg(i) + (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[i]));
#ifdef DWGSUPPORT
            if (dwg && !libreDwgReader.isEmpty()) {
                QProcess reader;
                reader.start(libreDwgReader, {"-O", "minJSON", lc::test::toQString(file)});
                REQUIRE(reader.waitForStarted());
                REQUIRE(reader.waitForFinished(30000));
                INFO(reader.readAllStandardError().toStdString());
                REQUIRE(reader.exitStatus() == QProcess::NormalExit);
                REQUIRE(reader.exitCode() == 0);
                const QJsonDocument document = QJsonDocument::fromJson(reader.readAllStandardOutput());
                REQUIRE(document.isObject());
                unsigned splines = 0, lines = 0;
                for (const QJsonValue& value : document.object().value("OBJECTS").toArray()) {
                    const QString entity = value.toObject().value("entity").toString();
                    splines += entity == QLatin1String("SPLINE");
                    lines += entity == QLatin1String("LINE");
                }
                CHECK(splines > 0);
                CHECK(lines > 0);
            }
#endif
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, formats[i]));
            const auto expectedEntities = live(f.graphic);
            const auto reopenedEntities = live(reopened);
            REQUIRE(reopenedEntities.size() == expectedEntities.size());
            CHECK(std::none_of(reopenedEntities.begin(), reopenedEntities.end(), [](const RS_Entity* entity) {
                return entity->rtti() == RS2::EntityOffsetCurve;
            }));
            std::vector<bool> consumed(reopenedEntities.size(), false);
            for (const auto* expected : expectedEntities) {
                bool found = false;
                for (std::size_t j = 0; j < reopenedEntities.size(); ++j) {
                    if (consumed[j] || !sameGeometry(*expected, *reopenedEntities[j])) continue;
                    consumed[j] = true;
                    found = true;
                    break;
                }
                CAPTURE(sourceBulge, trimLineSpan, polylineFirst, formats[i], expected->rtti());
                CHECK(found);
            }
        }
        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK_FALSE(source->isDeleted());
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Polyline);
        for (const auto* fragment : fragments) CHECK(fragment->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK(cutter->isDeleted());
        for (const auto* fragment : fragments) CHECK_FALSE(fragment->isDeleted());
    }
}

TEST_CASE("Sinusoidal Trim Two resolves multiple roots across operand roles",
          "[lc-offsetcurve][trim-action][polyline][multi-root][sinusoidal]") {
    Drawing<true> f;
    RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false});
    auto initial = LC_OffsetCurve::create(seed, .15, false);
    REQUIRE(initial);
    auto* previous = initial.entity.release(); f.graphic.addEntity(previous);
    auto* source = new RS_Polyline(&f.graphic);
    constexpr unsigned sampleSegments = 32;
    for (unsigned i = 0; i <= sampleSegments; ++i) {
        const double x = .8 * double(i);
        source->addVertex({x, .5 * std::sin(.3 * x)});
    }
    source->endPolyline(); f.graphic.addEntity(source);
    f.graphic.select(QList<RS_Entity*>{previous}, true);
    UpdateSourceProbe update(&f.context);
    update.init(0); update.setSourceKind(LC_OffsetSourceGeometry::Kind::Polyline); update.use(source);
    lc::test::awaitUpdate(update);
    REQUIRE(update.lastReport().updated == 1);
    LC_OffsetCurve* native = nullptr;
    for (auto* entity : live(f.graphic)) {
        if (entity->rtti() != RS2::EntityOffsetCurve) continue;
        native = dynamic_cast<LC_OffsetCurve*>(entity);
        REQUIRE(native);
    }
    REQUIRE(native);
    const auto spanCount = [](const RS_SplineData& data) {
        std::size_t count = 0;
        for (std::size_t i = 0; i < data.knotslist.size();) {
            const double knot = data.knotslist[i++];
            while (i < data.knotslist.size() && data.knotslist[i] == knot) ++i;
            ++count;
        }
        return count - 1;
    };
    const auto before = RS_Spline::geometryFingerprint(native->getData());
    const auto breaks = native->getBreakParameters(); REQUIRE(breaks.size() > sampleSegments);
    constexpr double lineY = -.3;
    auto* line = new RS_Line(&f.graphic, RS_LineData{{-16, lineY}, {42, lineY}});
    f.graphic.addEntity(line);
    const RS_Vector linePick{40, lineY};
    RS_SplineData lineCache{1, false};
    lineCache.controlPoints = {{-16, lineY}, {42, lineY}};
    lineCache.knotslist = {0, 0, 1, 1}; lineCache.weights = {1, 1};
    const auto byLine = LC_OffsetCacheTrim::prepare(lineCache, *native, {linePick});
    const auto afterSecond = RS_Spline::geometryFingerprint(native->getData());
    REQUIRE(byLine.roots.size() == 2);
    const auto& root = byLine.roots.front().isolation;
    const double nativeParameter = .5 * (root.parameterB.lo() + root.parameterB.hi()) + .001;
    const auto nativePick = native->getPointAt(nativeParameter);
    const auto byCurve = LC_OffsetCacheTrim::prepare(native->getData(), *line, {nativePick});
    const auto afterFirst = RS_Spline::geometryFingerprint(native->getData());
    CAPTURE(spanCount(native->getData()), native->getData().controlPoints.size(),
            native->getData().knotslist.size(), byCurve.work.inputSpans, byLine.work.inputSpans,
            byCurve.rootCount, byLine.rootCount, static_cast<int>(byCurve.refusal),
            static_cast<int>(byLine.refusal));
    CHECK(before == afterFirst);
    CHECK(before == afterSecond);
    CHECK(byCurve.refusal == LC_OffsetCacheTrim::Refusal::None);
    CHECK(byLine.refusal == LC_OffsetCacheTrim::Refusal::None);
    CHECK(byLine.rootCount == 2);
    CHECK(byCurve.rootCount > 0);
    const auto overlaps = [](const LC_Interval& a, const LC_Interval& b) {
        return a.isValid() && b.isValid() && a.lo() <= b.hi() && b.lo() <= a.hi();
    };
    CHECK(std::any_of(byCurve.roots.begin(), byCurve.roots.end(), [&](const auto& selectedRoot) {
        return std::any_of(byLine.roots.begin(), byLine.roots.end(), [&](const auto& fullRoot) {
            return overlaps(selectedRoot.worldX, fullRoot.worldX) &&
                   overlaps(selectedRoot.worldY, fullRoot.worldY);
        });
    }));
    LC_DocumentModificationBatch directBatch;
    const auto directEpoch = f.graphic.undoStateEpoch();
    const auto direct = RS_Modification::trimStoredCache(f.graphic, *native, nativePick,
        *line, linePick, true, directBatch);
    REQUIRE(direct.result);
    CHECK_FALSE(direct.cacheRefusal.has_value());
    CHECK(directBatch.entitiesToAdd.size() <= 16);
    CHECK(directBatch.entitiesToDelete.size() == 2);
    CHECK(f.graphic.undoStateEpoch() == directEpoch);
    CHECK_FALSE(native->isDeleted()); CHECK_FALSE(line->isDeleted());
    CHECK(RS_Spline::geometryFingerprint(native->getData()) == before);
    qDeleteAll(directBatch.entitiesToAdd);
    for (const bool nativeFirst : {false, true}) {
        CAPTURE(nativeFirst);
        if (f.action->isFinished()) f.action = std::make_unique<CacheTrim<true>>(&f.context);
        const auto firstPick = nativeFirst ? nativePick : linePick;
        const auto secondPick = nativeFirst ? linePick : nativePick;
        f.action->click(firstPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        const auto epoch = f.graphic.undoStateEpoch();
        f.action->hover(secondPick);
        CHECK_FALSE(f.action->m_preview->isEmpty());
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(line->isDeleted());
        CHECK(f.graphic.undoStateEpoch() == epoch);
        f.action->click(secondPick);
        REQUIRE(native->isDeleted()); REQUIRE(line->isDeleted());
        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(line->isDeleted());
    }
}

TEST_CASE("Closed high-segment polyline Trim Two distinguishes connected and "
          "overlapping offsets",
          "[lc-offsetcurve][trim-action][polyline][closed][high-segment]") {
    const unsigned segmentCount = GENERATE(256u, 512u, 1024u, 2048u);
    const bool towardCentre = GENERATE(true, false);
    CAPTURE(segmentCount, towardCentre);
    Drawing<true> f;
    RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false});
    auto initial = LC_OffsetCurve::create(seed, .15, towardCentre);
    REQUIRE(initial);
    auto *previous = initial.entity.release();
    f.graphic.addEntity(previous);

    constexpr double radius = 10.0;
    auto *source = new RS_Polyline(&f.graphic);
    for (unsigned i = 0; i < segmentCount; ++i) {
      const double angle =
          2.0 * std::acos(-1.0) * static_cast<double>(i) / segmentCount;
      source->addVertex({radius * std::cos(angle), radius * std::sin(angle)});
    }
    source->setClosed(true);
    source->endPolyline();
    f.graphic.addEntity(source);
    f.graphic.select(QList<RS_Entity *>{previous}, true);

    UpdateSourceProbe update(&f.context);
    update.init(0);
    update.setSourceKind(LC_OffsetSourceGeometry::Kind::Polyline);
    update.use(source);
    lc::test::awaitUpdate(update);
    for (const auto& outcome : update.lastReport().outcomes) INFO(outcome.refusal.toStdString());
    REQUIRE(update.lastReport().updated == 1);

    LC_OffsetCurve *native = nullptr;
    for (auto *entity : live(f.graphic)) {
      if (entity->rtti() != RS2::EntityOffsetCurve)
        continue;
      REQUIRE(native == nullptr);
      native = dynamic_cast<LC_OffsetCurve *>(entity);
    }
    REQUIRE(native);
    REQUIRE(native->sourceDefinition());
    CHECK(native->sourceDefinition()->source.kind() ==
          LC_OffsetSourceGeometry::Kind::Polyline);
    const auto before = RS_Spline::geometryFingerprint(native->getData());
    const auto &nativeData = native->getData();
    REQUIRE_FALSE(nativeData.controlPoints.empty());
    const double cacheEndpointGap = std::hypot(
        nativeData.controlPoints.front().x - nativeData.controlPoints.back().x,
        nativeData.controlPoints.front().y - nativeData.controlPoints.back().y);
    CHECK_FALSE(nativeData.m_closedFlag);
    CHECK(cacheEndpointGap == 0.0);
    if (segmentCount >= 512) {
      CHECK(nativeData.controlPoints.size() > segmentCount);
      if (towardCentre) {
        const double pi = std::acos(-1.0);
        const double expectedRadius = radius + .15 / std::cos(pi / static_cast<double>(segmentCount));
        double maximumRadialError = 0.0;
        for (const RS_Vector& point : nativeData.controlPoints) {
          maximumRadialError = std::max(maximumRadialError,
              std::abs(point.magnitude() - expectedRadius));
        }
        CAPTURE(expectedRadius, maximumRadialError);
        CHECK(maximumRadialError < 3e-4);
      }
      else {
        double minimumSupportClearance = std::numeric_limits<double>::infinity();
        for (const RS_Vector& point : nativeData.controlPoints) {
          for (unsigned edge = 0; edge < segmentCount; ++edge) {
            const RS_Vector start = source->getVertex(static_cast<int>(edge));
            const RS_Vector end = source->getVertex(static_cast<int>((edge + 1) % segmentCount));
            const RS_Vector direction = end - start;
            const double length = direction.magnitude();
            const double signedSide =
                (direction.x * (point.y - start.y) - direction.y * (point.x - start.x)) / length;
            minimumSupportClearance = std::min(minimumSupportClearance, signedSide - .15);
          }
        }
        CAPTURE(minimumSupportClearance);
        CHECK(minimumSupportClearance >= -3e-4);
      }
    }

    const double lineY = segmentCount == 2048 ? 1.4142135623730951 : 1.337;
    auto *line =
        new RS_Line(&f.graphic, RS_LineData{{-15, lineY}, {15, lineY}});
    f.graphic.addEntity(line);
    const RS_Vector linePick{14, lineY};
    RS_SplineData lineCache{1, false};
    lineCache.controlPoints = {{-15, lineY}, {15, lineY}};
    lineCache.knotslist = {0, 0, 1, 1};
    lineCache.weights = {1, 1};

    const auto byLine =
        LC_OffsetCacheTrim::prepare(lineCache, *native, {linePick});
    REQUIRE(byLine.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(byLine.rootCount == 2);
    const auto &rightRoot = byLine.roots.back().isolation;
    const double nativeParameter =
        .5 * (rightRoot.parameterB.lo() + rightRoot.parameterB.hi()) + .001;
    const RS_Vector nativePick = native->getPointAt(nativeParameter);
    const auto byCurve =
        LC_OffsetCacheTrim::prepare(native->getData(), *line, {nativePick});
    CAPTURE(byCurve.rootCount, static_cast<int>(byCurve.refusal),
            byCurve.work.intersections.totalBoxPairs,
            byCurve.work.intersections.generatedLeaves, source->isClosed(),
            source->size(), native->getData().m_closedFlag,
            native->getData().controlPoints.size(),
            native->getData().knotslist.size(), cacheEndpointGap);
    CHECK(RS_Spline::geometryFingerprint(native->getData()) == before);
    REQUIRE(byCurve.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(byCurve.rootCount == 2);
    const auto overlaps = [](const LC_Interval &a, const LC_Interval &b) {
      return a.isValid() && b.isValid() && a.lo() <= b.hi() && b.lo() <= a.hi();
    };
    CHECK(std::any_of(
        byCurve.roots.begin(), byCurve.roots.end(),
        [&](const auto &selectedRoot) {
          return std::any_of(
              byLine.roots.begin(), byLine.roots.end(),
              [&](const auto &fullRoot) {
                return overlaps(selectedRoot.worldX, fullRoot.worldX) &&
                       overlaps(selectedRoot.worldY, fullRoot.worldY);
              });
        }));

    const auto epoch = f.graphic.undoStateEpoch();
    const auto entityCount = live(f.graphic).size();
    if (towardCentre) {
      f.action->click(linePick);
      REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
      f.action->hover(nativePick);
    }
    else {
      f.action->contextTarget(native, nativePick);
      f.action->hover(linePick);
    }
    CHECK_FALSE(f.action->m_preview->isEmpty());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(line->isDeleted());
    CHECK(f.graphic.undoStateEpoch() == epoch);
    CHECK(RS_Spline::geometryFingerprint(native->getData()) == before);
    f.action->click(towardCentre ? nativePick : linePick);
    REQUIRE(native->isDeleted());
    REQUIRE(line->isDeleted());
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
        RS2::FormatDWG, RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
    const auto expected = live(f.graphic);
    for (std::size_t i = 0; i < formats.size(); ++i) {
      const bool dwg = formats[i] == RS2::FormatDWG || formats[i] == RS2::FormatDWG2018;
      const auto file = std::filesystem::path(directory.filePath(
          QStringLiteral("closed-polyline-trim-two-%1-%2-%3")
              .arg(segmentCount).arg(towardCentre).arg(i) +
              (dwg ? ".dwg" : ".dxf")).toStdString());
      REQUIRE(lc::test::exportDrawing(f.graphic, file, formats[i]));
      RS_Graphic reopened;
      REQUIRE(lc::test::importDrawing(reopened, file, formats[i]));
      std::vector<const RS_Entity*> reopenedEntities;
      for (const auto* entity : live(reopened)) {
        CHECK(entity->rtti() != RS2::EntityOffsetCurve);
        reopenedEntities.push_back(entity);
      }
      CAPTURE(towardCentre, formats[i]);
      CHECK(sameTrimGeometrySet(expected, reopenedEntities));
    }
    REQUIRE(f.graphic.undo());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(line->isDeleted());
    CHECK(live(f.graphic).size() == entityCount);
    CHECK(RS_Spline::geometryFingerprint(native->getData()) == before);
}

TEST_CASE("Dense sinusoidal cache maps component roots and enforces the Trim Two output cap",
          "[lc-offsetcurve][trim-action][polyline][multi-root][dense][output-limit]") {
    Drawing<true> f;
    RS_Ellipse seed(nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false});
    auto initial = LC_OffsetCurve::create(seed, .15, false);
    REQUIRE(initial);
    auto* previous = initial.entity.release(); f.graphic.addEntity(previous);
    auto* source = new RS_Polyline(&f.graphic);
    constexpr unsigned sampleSegments = 256;
    for (unsigned i = 0; i <= sampleSegments; ++i) {
        const double x = .1 * double(i);
        source->addVertex({x, .5 * std::sin(.3 * x)});
    }
    source->endPolyline(); f.graphic.addEntity(source);
    f.graphic.select(QList<RS_Entity*>{previous}, true);
    UpdateSourceProbe update(&f.context);
    update.init(0); update.setSourceKind(LC_OffsetSourceGeometry::Kind::Polyline); update.use(source);
    lc::test::awaitUpdate(update);
    REQUIRE(update.lastReport().updated == 1);
    LC_OffsetCurve* native = nullptr;
    for (auto* entity : live(f.graphic)) {
        if (entity->rtti() != RS2::EntityOffsetCurve) continue;
        native = dynamic_cast<LC_OffsetCurve*>(entity);
        REQUIRE(native);
    }
    REQUIRE(native);
    const auto before = RS_Spline::geometryFingerprint(native->getData());
    const auto breaks = native->getBreakParameters();
    REQUIRE(breaks.size() > sampleSegments);
    const auto middleSpan = (breaks.size() - 2) / 2;
    const double parameter = breaks[middleSpan] + .43 *
        (breaks[middleSpan + 1] - breaks[middleSpan]);
    const double lineY = native->getPointAt(parameter).y;
    auto* line = new RS_Line(&f.graphic, RS_LineData{{-16, lineY}, {42, lineY}});
    f.graphic.addEntity(line);
    const RS_Vector linePick{40, lineY};
    RS_SplineData lineCache{1, false};
    lineCache.controlPoints = {{-16, lineY}, {42, lineY}};
    lineCache.knotslist = {0, 0, 1, 1}; lineCache.weights = {1, 1};
    const auto byLine = LC_OffsetCacheTrim::prepare(lineCache, *native, {linePick});
    REQUIRE(byLine.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(byLine.rootCount == 2);
    const RS_Vector nativePick = native->getPointAt(
        breaks[middleSpan + 20] + .5 * (breaks[middleSpan + 21] - breaks[middleSpan + 20]));
    const auto byCurve = LC_OffsetCacheTrim::prepare(native->getData(), *line, {nativePick});
    CHECK(byCurve.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(byCurve.rootCount == 1);
    CHECK(byCurve.fragments.size() > LC_StoredCacheTrimOptions::maxOutputEntitiesLimit);
    const auto& selectedRoot = byCurve.roots.front();
    CHECK(std::any_of(byLine.roots.begin(), byLine.roots.end(), [&](const auto& fullRoot) {
        return selectedRoot.worldX.lo() <= fullRoot.worldX.hi() &&
               fullRoot.worldX.lo() <= selectedRoot.worldX.hi() &&
               selectedRoot.worldY.lo() <= fullRoot.worldY.hi() &&
               fullRoot.worldY.lo() <= selectedRoot.worldY.hi();
    }));

    LC_DocumentModificationBatch batch;
    const auto epoch = f.graphic.undoStateEpoch();
    const auto refused = RS_Modification::trimStoredCache(f.graphic, *native, nativePick,
        *line, linePick, true, batch);
    CHECK_FALSE(refused.result);
    REQUIRE(refused.cacheRefusal == LC_OffsetCacheTrim::Refusal::LimitExceeded);
    CHECK(batch.entitiesToAdd.isEmpty());
    CHECK(batch.entitiesToDelete.isEmpty());
    CHECK(f.graphic.undoStateEpoch() == epoch);
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(line->isDeleted());
    CHECK(RS_Spline::geometryFingerprint(native->getData()) == before);
}

TEST_CASE("T Trim Two admits exactly sixteen outputs and refuses seventeen atomically",
          "[gui][lc-offsetcurve][trim-action][trim-output-limit][typed-source]") {
    for (const std::size_t farComponents : {1u, 2u}) for (const bool nativeFirst : {false, true}) {
        CAPTURE(farComponents, nativeFirst);
        Drawing<true> f;
        auto* native = addNative(f.graphic);

        RS_SplineData cutterData{1, false};
        std::vector<double> xValues;
        for (std::size_t i = 0; i < 14; ++i) xValues.push_back(-1.8 + 3.6 * double(i) / 13.0);
        for (std::size_t i = 0; i < farComponents; ++i) xValues.push_back(4.0 + double(i));
        cutterData.knotslist = {0, 0};
        for (std::size_t i = 0; i < xValues.size(); ++i) {
            const double x = xValues[i];
            cutterData.controlPoints.push_back({x, -3});
            cutterData.controlPoints.push_back({x, 3});
            cutterData.weights.push_back(1);
            cutterData.weights.push_back(1);
            const double nextKnot = double(i + 1);
            cutterData.knotslist.insert(cutterData.knotslist.end(), {nextKnot, nextKnot});
        }
        RS_Spline* cutter = new RS_Spline(&f.graphic, cutterData);
        f.graphic.addEntity(cutter);
        const RS_Vector nativePick{2, 0};
        const RS_Vector cutterPick{-1.5, -2};
        f.action->click(nativeFirst ? nativePick : cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        const auto previewEpoch = f.graphic.undoStateEpoch();
        f.action->hover(nativeFirst ? cutterPick : nativePick);
        const std::size_t expectedOutputs = farComponents == 1 ? 16 : 17;
        INFO("preview output count=" << f.action->m_preview->count());
        if (expectedOutputs == 16) {
            REQUIRE(f.action->m_preview->count() == 16);
            CHECK_FALSE(native->isDeleted()); CHECK_FALSE(cutter->isDeleted());
            CHECK(f.graphic.undoStateEpoch() == previewEpoch);
            f.action->click(nativeFirst ? cutterPick : nativePick);
            CHECK(native->isDeleted()); CHECK(cutter->isDeleted());
            CHECK(live(f.graphic).size() == 16);
            REQUIRE(f.graphic.undo()); CHECK_FALSE(native->isDeleted()); CHECK_FALSE(cutter->isDeleted());
            REQUIRE(f.graphic.redo()); CHECK(native->isDeleted()); CHECK(cutter->isDeleted());
        } else {
            CHECK(f.action->m_preview->isEmpty());
            CHECK_FALSE(native->isDeleted()); CHECK_FALSE(cutter->isDeleted());
            CHECK(f.graphic.undoStateEpoch() == previewEpoch);
            const auto count = f.graphic.count();
            f.action->click(nativeFirst ? cutterPick : nativePick);
            CHECK_FALSE(native->isDeleted()); CHECK_FALSE(cutter->isDeleted());
            CHECK(f.graphic.count() == count);
            CHECK(f.graphic.undoStateEpoch() == previewEpoch);
            REQUIRE_FALSE(f.context.messages.isEmpty());
            CHECK(f.context.messages.back().contains("nothing changed"));
        }
    }
}

TEST_CASE("T plain cache target availability never enables one sided ordinary only Trim Two publication",
          "[lc-offsetcurve][trim-action][ordinary-spline]") {
    Drawing<true> f; auto* unrelatedNative = addNative(f.graphic, 10);
    RS_SplineData data{1, false}; data.controlPoints = {{-3, .3}, {3, .3}}; data.knotslist = {0, 0, 1, 1}; data.weights = {1, 1};
    auto* spline = new RS_Spline(&f.graphic, data); f.graphic.addEntity(spline);
    auto* ordinaryLine = line(f.graphic, {0, -2}, {0, 2});
    f.action->click({-2, .3}); f.action->hover({0, 1}); f.action->click({0, 1});
    CHECK_FALSE(spline->isDeleted()); CHECK_FALSE(ordinaryLine->isDeleted()); CHECK_FALSE(unrelatedNative->isDeleted());
    CHECK_FALSE(canUndo(f.graphic)); CHECK(f.graphic.count() == 3);
}

TEST_CASE("T trim forgets a destroyed document even while Context and view retain its address",
          "[lc-offsetcurve][trim-action][correctness][lifetime]") {
    Drawing<false> f;
    auto document = std::make_unique<RS_Graphic>(); document->initForNewDocument();
    const auto restore = qScopeGuard([&] {
        f.view.setDocument(&f.graphic); f.context.setDocumentAndView(&f.graphic, &f.view);
    });
    f.action.reset(); f.view.setDocument(document.get()); f.context.setDocumentAndView(document.get(), &f.view);
    f.action = std::make_unique<CacheTrim<false>>(&f.context);
    auto* source = addNative(*document); line(*document);
    f.action->click({2, 0}); REQUIRE(f.action->retainedLimit() == source);
    f.action->guardedHover({0, .3});
    auto* const identity = document.get(); document.reset();
    CHECK(f.view.getDocument() == identity); CHECK(f.context.getDocument() == identity);
    f.view.redrawMethod = RS2::RedrawNone;
    f.action->stalePublicEvents(); f.action->trigger(); f.action->cancel();
    CHECK(f.action->isFinished()); CHECK(f.action->retainedLimit() == nullptr);
    CHECK(f.action->m_preview->isEmpty()); CHECK(f.action->m_preview->getParent() == nullptr);
    CHECK(f.view.redrawMethod == RS2::RedrawNone);
    f.action.reset(); // The preview/base action destructor must not visit the dead document.
}

TEST_CASE("T trim forgets a destroyed captured view before hover trigger cancel and destruction",
          "[lc-offsetcurve][trim-action][correctness][lifetime]") {
    Drawing<false> f; f.action.reset();
    auto view = std::make_unique<lc::test::TestGraphicView>(); view->setDocument(&f.graphic);
    QPointer<RS_GraphicView> probe = view.get(); f.context.setDocumentAndView(&f.graphic, view.get());
    const auto restore = qScopeGuard([&] { f.context.setDocumentAndView(&f.graphic, &f.view); });
    f.action = std::make_unique<CacheTrim<false>>(&f.context);
    auto* source = addNative(f.graphic); auto* target = line(f.graphic);
    f.action->click({2, 0}); REQUIRE(f.action->retainedLimit() == source);
    f.action->guardedHover({0, .3}); view.reset(); REQUIRE(probe.isNull());
    f.action->stalePublicEvents(); f.action->trigger(); f.action->cancel();
    CHECK(f.action->isFinished()); CHECK(f.action->m_preview->isEmpty());
    CHECK_FALSE(source->isDeleted()); CHECK_FALSE(target->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    f.action.reset();
}

TEST_CASE("T trim rejects a closing or switched captured view without drawing through it",
          "[lc-offsetcurve][trim-action][correctness][lifetime]") {
    for (bool closing : {false, true}) {
        Drawing<false> f; auto* source = addNative(f.graphic); auto* target = line(f.graphic);
        f.action->click({2, 0});
        REQUIRE(f.action->retainedLimit() == source);
        // Draw real Trim preparation before invalidating the captured context.
        f.action->hover({0, .3});
        REQUIRE_FALSE(f.action->m_preview->isEmpty());
        auto* oldPreviewStore = f.view.getViewPort()->getOverlayEntitiesContainer(RS2::ActionPreviewEntity);
        auto* oldHighlightStore = f.view.getViewPort()->getOverlayEntitiesContainer(RS2::OverlayEffects);
        REQUIRE_FALSE(oldPreviewStore->isEmpty()); REQUIRE_FALSE(oldHighlightStore->isEmpty());
        REQUIRE(oldPreviewStore->findEntityIndex(f.action->m_preview.get()) >= 0);
        lc::test::TestGraphicView other; other.setDocument(&f.graphic);
        Messages otherContext; otherContext.setDocumentAndView(&f.graphic, &other);
        auto otherAction = std::make_unique<CacheTrim<false>>(&otherContext);
        otherAction->click({2, 0}); otherAction->hover({0, .3});
        REQUIRE_FALSE(otherAction->m_preview->isEmpty());
        auto* newPreviewStore = other.getViewPort()->getOverlayEntitiesContainer(RS2::ActionPreviewEntity);
        auto* newHighlightStore = other.getViewPort()->getOverlayEntitiesContainer(RS2::OverlayEffects);
        REQUIRE_FALSE(newPreviewStore->isEmpty()); REQUIRE_FALSE(newHighlightStore->isEmpty());
        REQUIRE(newPreviewStore->findEntityIndex(otherAction->m_preview.get()) >= 0);
        const std::vector<RS_Entity*> newPreviewEntries(newPreviewStore->begin(), newPreviewStore->end());
        const std::vector<RS_Entity*> newHighlightEntries(newHighlightStore->begin(), newHighlightStore->end());
        const auto newPreviewCount = otherAction->m_preview->count();
        const auto members = live(f.graphic);
        const auto sourceStamp = RS_Spline::geometryFingerprint(source->getData());
        const auto targetStart = target->getStartpoint(), targetEnd = target->getEndpoint();
        std::ostringstream undoBefore; undoBefore << static_cast<const RS_Undo&>(f.graphic);
        const auto restore = qScopeGuard([&] { f.context.setDocumentAndView(&f.graphic, &f.view); });
        if (closing) f.view.beginClose(); else f.context.setDocumentAndView(&f.graphic, &other);
        f.view.redrawMethod = other.redrawMethod = RS2::RedrawNone;
        f.action->stalePublicEvents(); f.action->trigger(); f.action->cancel();
        CHECK(f.action->isFinished()); CHECK(f.action->m_preview->isEmpty());
        CHECK(f.action->m_preview->getParent() == nullptr);
        CHECK(oldPreviewStore->isEmpty()); CHECK(oldHighlightStore->isEmpty());
        CHECK(std::vector<RS_Entity*>(newPreviewStore->begin(), newPreviewStore->end()) == newPreviewEntries);
        CHECK(std::vector<RS_Entity*>(newHighlightStore->begin(), newHighlightStore->end()) == newHighlightEntries);
        CHECK(otherAction->m_preview->count() == newPreviewCount);
        CHECK_FALSE(otherAction->isFinished());
        // Destruction of the abandoned action must not clear the second view either.
        f.action.reset();
        CHECK(oldPreviewStore->isEmpty()); CHECK(oldHighlightStore->isEmpty());
        CHECK(std::vector<RS_Entity*>(newPreviewStore->begin(), newPreviewStore->end()) == newPreviewEntries);
        CHECK(std::vector<RS_Entity*>(newHighlightStore->begin(), newHighlightStore->end()) == newHighlightEntries);
        CHECK(otherAction->m_preview->count() == newPreviewCount);
        CHECK(f.view.redrawMethod == RS2::RedrawNone); CHECK(other.redrawMethod == RS2::RedrawNone);
        CHECK_FALSE(source->isDeleted()); CHECK_FALSE(target->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
        CHECK(live(f.graphic) == members); CHECK(f.graphic.count() == members.size());
        CHECK(RS_Spline::geometryFingerprint(source->getData()) == sourceStamp); CHECK(source->hasEditableDefinition());
        CHECK(target->getStartpoint() == targetStart); CHECK(target->getEndpoint() == targetEnd);
        std::ostringstream undoAfter; undoAfter << static_cast<const RS_Undo&>(f.graphic);
        CHECK(undoAfter.str() == undoBefore.str());
    }
}

TEST_CASE("T trim discards freed saved native owner and child Context picks before base initialization",
          "[lc-offsetcurve][trim-action][correctness][context-init]") {
    for (bool child : {false, true}) {
        Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = line(f.graphic);
        REQUIRE(source->size() > 0);
        f.context.saveContextMenuActionContext(child ? source->entityAt(0) : source, {2, 0}, true);
        REQUIRE(f.graphic.removeEntity(source));
        f.action->init(0);
        CHECK(f.context.getContextMenuActionContextEntity() == nullptr);
        CHECK_FALSE(f.context.getContextMenuActionClickPosition().valid);
        CHECK(f.action->m_preview->isEmpty()); CHECK_FALSE(limit->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    }
}

TEST_CASE("T trim maps a live saved native child through its direct owning container",
          "[lc-offsetcurve][trim-action][correctness][context-init]") {
    Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = line(f.graphic);
    REQUIRE(source->size() > 0);
    f.context.saveContextMenuActionContext(source->entityAt(0), {2, 0}, false);
    f.action->init(0); f.action->click({-2.8, .3});
    CHECK(source->isDeleted()); CHECK_FALSE(limit->isDeleted());
    REQUIRE(f.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("T trim cancels retained picks after an actual undo epoch change",
          "[lc-offsetcurve][trim-action][correctness][epoch]") {
    Drawing<false> f; auto* source = addNative(f.graphic); auto* target = line(f.graphic);
    auto* history = line(f.graphic, {10, 10}, {11, 10});
    { LC_UndoSection section(&f.graphic, f.view.getViewPort()); section.addUndoable(history); }
    f.action->click({2, 0}); const auto epoch = f.graphic.undoStateEpoch();
    REQUIRE(f.graphic.undo()); REQUIRE(f.graphic.undoStateEpoch() != epoch);
    std::ostringstream before; before << static_cast<const RS_Undo&>(f.graphic);
    f.action->click({0, .3}); // One actual attempted target pick is refused before any new limit pick.
    CHECK(f.action->retainedLimit() == nullptr); CHECK(f.action->m_preview->isEmpty());
    CHECK_FALSE(source->isDeleted()); CHECK_FALSE(target->isDeleted());
    std::ostringstream after; after << static_cast<const RS_Undo&>(f.graphic); CHECK(after.str() == before.str());
}

namespace {
RS_Polyline* polylineLimit(RS_Graphic& graphic, bool arc) {
    auto polyline = std::make_unique<RS_Polyline>(&graphic);
    if (arc) { polyline->addVertex({2, 0}, 1); polyline->addVertex({-2, 0}); }
    else { polyline->addVertex({-3, .3}); polyline->addVertex({3, .3}); }
    polyline->calculateBorders(); auto* out = polyline.release(); graphic.addEntity(out); return out;
}
}

TEST_CASE("T ordinary one sided Trim preserves owned polyline line and arc limits",
          "[lc-offsetcurve][trim-action][correctness][nested-limit]") {
    for (bool arc : {false, true}) {
        Drawing<false> f; auto* limit = polylineLimit(f.graphic, arc);
        auto* target = line(f.graphic, {0, -3}, {0, 3});
        auto* edge = limit->entityAt(0); const auto start = edge->getStartpoint(), end = edge->getEndpoint();
        f.action->click(arc ? RS_Vector{2, 0} : RS_Vector{-2.8, .3});
        REQUIRE(f.action->retainedLimit()); CHECK(f.action->retainedLimit() != edge);
        CHECK(f.action->retainedLimit()->getParent() == nullptr);
        f.action->guardedHover({0, -1}); f.action->click({0, -1});
        REQUIRE(target->isDeleted()); CHECK_FALSE(limit->isDeleted());
        CHECK(edge->getStartpoint() == start); CHECK(edge->getEndpoint() == end);
        REQUIRE(f.graphic.undo()); CHECK_FALSE(target->isDeleted()); CHECK_FALSE(f.graphic.undo());
    }
}

TEST_CASE("T native one sided Trim uses an owned polyline edge copy after its real owner is freed",
          "[lc-offsetcurve][trim-action][correctness][nested-limit]") {
    for (bool removeOwner : {false, true}) {
        Drawing<false> f; auto* source = addNative(f.graphic); auto* limit = polylineLimit(f.graphic, false);
        auto* edge = limit->entityAt(0); f.action->click({-2.8, .3});
        REQUIRE(f.action->retainedLimit()); REQUIRE(f.action->retainedLimit() != edge);
        CHECK(f.action->retainedLimit()->getParent() == nullptr);
        if (removeOwner) REQUIRE(f.graphic.removeEntity(limit));
        f.action->guardedHover({2, 0}); f.action->click({2, 0});
        REQUIRE(source->isDeleted()); if (!removeOwner) CHECK_FALSE(limit->isDeleted());
        REQUIRE(f.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK_FALSE(f.graphic.undo());
    }
}

TEST_CASE("T nested native limits are not converted into detached admissible native clones",
          "[lc-offsetcurve][trim-action][correctness][nested-limit]") {
    Drawing<false> f; auto* source = addNative(f.graphic); auto* target = line(f.graphic);
    RS_EntityContainer borrowed(nullptr, false); source->setParent(&borrowed);
    f.action->click({2, 0}); f.action->click({0, .3});
    CHECK_FALSE(source->isDeleted()); CHECK_FALSE(target->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    source->setParent(&f.graphic);
}

namespace {
class LateLimitLine final : public RS_Line {
public:
    explicit LateLimitLine(RS_EntityContainer* parent) : RS_Line(parent, {-3, .3}, {3, .3}) {}
    mutable unsigned startReads = 0;
    mutable std::function<void()> finalRead;
    RS_Vector getStartpoint() const override {
        const auto point = RS_Line::getStartpoint();
        // Initial source stamp, raw line admission, then final source stamp.
        if (++startReads == 3 && finalRead) { auto callback = std::move(finalRead); callback(); }
        return point;
    }
};
}

TEST_CASE("T one sided native limit closure is revalidated after an actual late target getter",
          "[lc-offsetcurve][trim-action][correctness][late-limit]") {
    for (unsigned defect = 0; defect < 3; ++defect) {
        CAPTURE(defect);
        Drawing<false> f; auto* limit = addNative(f.graphic);
        auto target = std::make_unique<LateLimitLine>(&f.graphic); auto* actual = target.get();
        f.graphic.addEntity(target.release());
        auto prefix = std::make_unique<RS_Line>(nullptr, RS_Vector{10, 0}, RS_Vector{11, 0});
        auto* old = line(f.graphic, {10, 1}, {11, 1});
        LC_DocumentModificationBatch batch; batch.entitiesToAdd.append(prefix.get()); batch.entitiesToDelete.append(old);
        const auto additions = batch.entitiesToAdd, deletions = batch.entitiesToDelete;
        bool called = false, removed = false;
        actual->startReads = 0;
        actual->finalRead = [&] {
            called = true;
            if (defect == 0) removed = f.graphic.removeEntity(limit);
            else if (defect == 1) limit->getData().controlPoints[1].x += .01;
            else {
                limit->delUserDefVar("trim-foreign");
                limit->setUserDefVar("trim-foreign", "late changed value");
                REQUIRE(limit->getUserDefVar("trim-foreign") == "late changed value");
            }
        };
        const auto result = RS_Modification::trimStoredCache(f.graphic, *actual, {0, .3}, *limit, {2, 0}, false, batch);
        REQUIRE(called); CHECK(actual->startReads == 3); if (defect == 0) CHECK(removed);
        CHECK_FALSE(result.result); REQUIRE(result.cacheRefusal);
        CHECK(result.trimmed1 == nullptr); CHECK(result.trimmed2 == nullptr);
        CHECK(*result.cacheRefusal == LC_OffsetCacheTrim::Refusal::UnsupportedCutter);
        CHECK(batch.entitiesToAdd == additions); CHECK(batch.entitiesToDelete == deletions);
        CHECK_FALSE(actual->isDeleted()); CHECK_FALSE(old->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    }
}

TEST_CASE("T unchanged locked native limits remain read only admissible with full snapshots",
          "[lc-offsetcurve][trim-action][correctness][late-limit]") {
    Drawing<false> f; auto* limit = addNative(f.graphic); auto* target = line(f.graphic);
    auto* layer = new RS_Layer("ReadOnlyNativeLimit"); f.graphic.addLayer(layer); limit->setLayer(layer); layer->lock(true);
    const auto stamp = RS_Spline::geometryFingerprint(limit->getData());
    LC_DocumentModificationBatch batch;
    const auto cleanup = qScopeGuard([&] { for (auto* output : std::as_const(batch.entitiesToAdd)) delete output; });
    const auto result = RS_Modification::trimStoredCache(f.graphic, *target, {0, .3}, *limit, {2, 0}, false, batch);
    REQUIRE(result.result); REQUIRE(batch.entitiesToAdd.size() > 0);
    CHECK(batch.entitiesToDelete == QList<RS_Entity*>{target});
    CHECK_FALSE(limit->isDeleted()); CHECK(limit->hasEditableDefinition());
    CHECK(RS_Spline::geometryFingerprint(limit->getData()) == stamp);
    CHECK_FALSE(target->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
}

namespace {
class AllNativeClosureLine final : public RS_Line {
public:
    explicit AllNativeClosureLine(RS_EntityContainer* parent) : RS_Line(parent, {-3, .3}, {3, .3}) {}
    mutable std::size_t startReads = 0;
    std::size_t callbackAt = 0;
    mutable std::function<void()> onFinalRead;
    RS_Vector getStartpoint() const override {
        const auto point = RS_Line::getStartpoint();
        if (++startReads == callbackAt && onFinalRead) {
            auto callback = std::move(onFinalRead); callback();
        }
        return point;
    }
};
LC_TrimResult trimNativeLinePair(Drawing<true>& f, LC_OffsetCurve& native, AllNativeClosureLine& line,
                                bool nativeFirst, LC_DocumentModificationBatch& batch) {
    line.startReads = 0;
    if (nativeFirst) return RS_Modification::trimStoredCache(f.graphic, native, {2, 0}, line, {0, .3}, true, batch);
    return RS_Modification::trimStoredCache(f.graphic, line, {0, .3}, native, {2, 0}, true, batch);
}
std::size_t finalNativeLineStartRead(bool nativeFirst) {
    // An actual unchanged preparation identifies the final endpoint read in
    // each pick order. No injected success or guessed read-count can skip it.
    Drawing<true> f; auto* native = addNative(f.graphic);
    auto* line = new AllNativeClosureLine(&f.graphic); f.graphic.addEntity(line);
    LC_DocumentModificationBatch batch;
    const auto cleanup = qScopeGuard([&] { for (auto* e : std::as_const(batch.entitiesToAdd)) delete e; });
    const auto result = trimNativeLinePair(f, *native, *line, nativeFirst, batch);
    REQUIRE(result.result); REQUIRE(line->startReads >= 3);
    REQUIRE(result.trimmed1); REQUIRE(result.trimmed2);
    CHECK_FALSE(native->isDeleted()); CHECK_FALSE(line->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    // In this nonconic pair the last root-line start read is the final source
    // stamp, after both output sets have already been fully constructed.
    return line->startReads;
}
}

TEST_CASE("T Trim Two all native input closure accepts an unchanged final getter in both pick orders",
          "[lc-offsetcurve][trim-action][correctness][all-native-closure]") {
    for (bool nativeFirst : {false, true}) {
        const auto finalRead = finalNativeLineStartRead(nativeFirst);
        Drawing<true> f; auto* native = addNative(f.graphic);
        auto* line = new AllNativeClosureLine(&f.graphic); f.graphic.addEntity(line);
        auto snapshot = LC_OffsetCurveEditSnapshot::capture(*native); REQUIRE(snapshot);
        auto prefix = std::make_unique<RS_Line>(nullptr, RS_Vector{10, 0}, RS_Vector{11, 0});
        auto* old = ::line(f.graphic, {10, 1}, {11, 1});
        LC_DocumentModificationBatch batch; batch.entitiesToAdd.append(prefix.get()); batch.entitiesToDelete.append(old);
        const auto cleanup = qScopeGuard([&] {
            for (qsizetype i = 1; i < batch.entitiesToAdd.size(); ++i) delete batch.entitiesToAdd.at(i);
        });
        bool called = false; line->callbackAt = finalRead; line->onFinalRead = [&] { called = true; };
        const auto result = trimNativeLinePair(f, *native, *line, nativeFirst, batch);
        REQUIRE(called); REQUIRE(line->startReads == finalRead); REQUIRE(result.result);
        REQUIRE(result.trimmed1); REQUIRE(result.trimmed2); CHECK(result.nativeDemotions == 1);
        REQUIRE(batch.entitiesToAdd.size() > 1); CHECK(std::as_const(batch.entitiesToAdd).front() == prefix.get());
        REQUIRE(batch.entitiesToDelete.size() == 3); CHECK(std::as_const(batch.entitiesToDelete).front() == old);
        CHECK(batch.entitiesToDelete.at(1) == (nativeFirst ? static_cast<RS_Entity*>(native) : line));
        CHECK(batch.entitiesToDelete.at(2) == (nativeFirst ? static_cast<RS_Entity*>(line) : native));
        CHECK(batch.entitiesToAdd.contains(result.trimmed1)); CHECK(batch.entitiesToAdd.contains(result.trimmed2));
        for (qsizetype i = 1; i < batch.entitiesToAdd.size(); ++i)
            CHECK(batch.entitiesToAdd.at(i)->rtti() != RS2::EntityOffsetCurve);
        REQUIRE(snapshot->matches(*native)); CHECK_FALSE(native->isDeleted()); CHECK_FALSE(line->isDeleted());
        CHECK(line->RS_Line::getStartpoint() == (RS_Vector{-3, .3}));
        CHECK(line->RS_Line::getEndpoint() == (RS_Vector{3, .3})); CHECK_FALSE(canUndo(f.graphic));
    }
}

TEST_CASE("T Trim Two revalidates every native after a later ordinary endpoint getter in both pick orders",
          "[lc-offsetcurve][trim-action][correctness][all-native-closure]") {
    for (bool nativeFirst : {false, true}) {
        const auto finalRead = finalNativeLineStartRead(nativeFirst);
        for (unsigned defect = 0; defect < 4; ++defect) {
            CAPTURE(nativeFirst, defect, finalRead);
            Drawing<true> f; auto* native = addNative(f.graphic);
            auto* layer = new RS_Layer("AllNativeClosure"); f.graphic.addLayer(layer); native->setLayer(layer);
            auto* line = new AllNativeClosureLine(&f.graphic); f.graphic.addEntity(line);
            line->setUserDefVar("ordinary-target", "preserved");
            const auto lineId = line->getId(), lineage = line->lineage();
            const auto pen = line->getPen(false); auto* lineLayer = line->getLayer(false);
            RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {native, line}, true);
            auto* history = ::line(f.graphic, {12, 12}, {13, 12});
            { LC_UndoSection section(&f.graphic, f.view.getViewPort()); section.addUndoable(history); }
            REQUIRE(f.graphic.undo()); // Keep a genuine redo state across refusal.
            auto prefix = std::make_unique<RS_Line>(nullptr, RS_Vector{10, 0}, RS_Vector{11, 0});
            auto* old = ::line(f.graphic, {10, 1}, {11, 1});
            LC_DocumentModificationBatch batch; batch.entitiesToAdd.append(prefix.get()); batch.entitiesToDelete.append(old);
            const auto additions = batch.entitiesToAdd, deletions = batch.entitiesToDelete;
            std::vector<RS_Entity*> expectedMembers(f.graphic.begin(), f.graphic.end());
            std::ostringstream before; before << static_cast<const RS_Undo&>(f.graphic);
            const auto epoch = f.graphic.undoStateEpoch();
            bool called = false, removed = false;
            std::unique_ptr<LC_OffsetCurveEditSnapshot> expectedNative;
            line->callbackAt = finalRead;
            line->onFinalRead = [&] {
                called = true;
                if (defect == 0) {
                    removed = f.graphic.removeEntity(native);
                    expectedMembers.erase(std::remove(expectedMembers.begin(), expectedMembers.end(), native), expectedMembers.end());
                } else {
                    if (defect == 1) native->getData().controlPoints[1].x += .01;
                    else if (defect == 2) {
                        native->delUserDefVar("trim-foreign");
                        native->setUserDefVar("trim-foreign", "late Trim Two mutation");
                        REQUIRE(native->getUserDefVar("trim-foreign") == "late Trim Two mutation");
                    }
                    else layer->lock(true);
                    expectedNative = LC_OffsetCurveEditSnapshot::capture(*native);
                }
            };
            const auto result = trimNativeLinePair(f, *native, *line, nativeFirst, batch);
            REQUIRE(called); CHECK(line->startReads == finalRead);
            CHECK_FALSE(result.result); REQUIRE(result.cacheRefusal);
            CHECK(*result.cacheRefusal == LC_OffsetCacheTrim::Refusal::UnsupportedLayout);
            CHECK(result.trimmed1 == nullptr); CHECK(result.trimmed2 == nullptr); CHECK(result.nativeDemotions == 0);
            CHECK(batch.entitiesToAdd == additions); CHECK(batch.entitiesToDelete == deletions);
            CHECK(std::vector<RS_Entity*>(f.graphic.begin(), f.graphic.end()) == expectedMembers);
            if (defect == 0) { CHECK(removed); CHECK(f.graphic.findEntityIndex(native) < 0); }
            else {
                REQUIRE(f.graphic.findEntityIndex(native) >= 0); REQUIRE(expectedNative);
                REQUIRE(expectedNative->matches(*native)); CHECK_FALSE(native->isDeleted()); CHECK(native->isSelected());
            }
            CHECK(line->getId() == lineId); CHECK(line->lineage() == lineage);
            CHECK(line->RS_Line::getStartpoint() == (RS_Vector{-3, .3}));
            CHECK(line->RS_Line::getEndpoint() == (RS_Vector{3, .3}));
            CHECK(line->getPen(false) == pen); CHECK(line->getLayer(false) == lineLayer);
            CHECK(line->getUserDefVar("ordinary-target") == "preserved");
            CHECK_FALSE(line->isDeleted()); CHECK(line->isSelected()); CHECK_FALSE(old->isDeleted());
            CHECK(f.graphic.undoStateEpoch() == epoch);
            std::ostringstream after; after << static_cast<const RS_Undo&>(f.graphic); CHECK(after.str() == before.str());
        }
    }
}

namespace {
namespace trimWorkerRequest = LC_OffsetWorkerRequest;
struct WorkerTrimBatch {
    LC_DocumentModificationBatch value;
    ~WorkerTrimBatch() { qDeleteAll(value.entitiesToAdd); }
};

std::unique_ptr<LC_OffsetWorkerCompletion> joinedTrimWorker(const trimWorkerRequest::Request& request) {
    REQUIRE(lc::test::application() != nullptr);
    REQUIRE(QFileInfo(LC_OffsetWorkerClient::helperPath()).isExecutable());
    LC_OffsetWorkerClient client;
    std::optional<LC_OffsetWorkerClient::Outcome> outcome;
    REQUIRE(client.submit(request, LC_OffsetWorkerClient::Mode::Commit,
        [&](LC_OffsetWorkerClient::Outcome received) { outcome.emplace(std::move(received)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) {
        REQUIRE(elapsed.elapsed() < 35000);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        if (!outcome) QThread::msleep(1);
    }
    REQUIRE(outcome->status == LC_OffsetWorkerClient::Status::Completed);
    REQUIRE(outcome->reaped); REQUIRE(outcome->exactGeometryWorkKnown); REQUIRE(outcome->completed);
    return std::move(outcome->completed);
}

RS_SplineData workerTrimCache(const RS_Entity& source) {
    if (const auto* spline = dynamic_cast<const RS_Spline*>(&source)) return spline->getData();
    REQUIRE(source.rtti() == RS2::EntityLine);
    RS_SplineData data{1, false};
    data.controlPoints = {source.getStartpoint(), source.getEndpoint()};
    data.knotslist = {0, 0, 1, 1}; data.weights = {1, 1};
    return data;
}

trimWorkerRequest::Cutter workerTrimCutter(const RS_Entity& source) {
    if (const auto* circle = dynamic_cast<const RS_Circle*>(&source)) return circle->getData();
    if (const auto* arc = dynamic_cast<const RS_Arc*>(&source)) return arc->getData();
    if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&source)) return ellipse->getData();
    if (const auto* spline = dynamic_cast<const RS_Spline*>(&source)) return spline->getData();
    return trimWorkerRequest::Segment{source.getStartpoint(), source.getEndpoint()};
}

trimWorkerRequest::Request cacheTrimJob(RS_Document& document, const RS_Entity& target,
    const RS_Entity& cutter, RS_Vector pick, const LC_OffsetCacheTrim::Limits& limits, qint64 id = 1) {
    trimWorkerRequest::Request request;
    request.token = {id, 3, 17, qint64(document.undoStateEpoch())};
    request.operation = trimWorkerRequest::TrimCache{workerTrimCache(target), workerTrimCutter(cutter), {pick}, limits};
    return request;
}

void sameTrimWork(const LC_OffsetCacheTrim::Work& a, const LC_OffsetCacheTrim::Work& b) {
#define CHECK_TRIM_WORK(f) CHECK(a.f == b.f)
    CHECK_TRIM_WORK(inputSpans); CHECK_TRIM_WORK(inputRecords); CHECK_TRIM_WORK(rootRecords);
    CHECK_TRIM_WORK(kernelItems); CHECK_TRIM_WORK(kernelComparisons);
    CHECK_TRIM_WORK(outputSpans); CHECK_TRIM_WORK(outputControls); CHECK_TRIM_WORK(outputKnots);
    CHECK_TRIM_WORK(failureStage);
#undef CHECK_TRIM_WORK
    const auto& x = a.intersections; const auto& y = b.intersections;
#define CHECK_TRIM_CORE(f) CHECK(x.f == y.f)
    CHECK_TRIM_CORE(candidateBoxPairs); CHECK_TRIM_CORE(refinementBoxPairs); CHECK_TRIM_CORE(certificateBoxPairs);
    CHECK_TRIM_CORE(totalBoxPairs); CHECK_TRIM_CORE(generatedLeaves); CHECK_TRIM_CORE(generatedItems);
    CHECK_TRIM_CORE(orderingComparisons); CHECK_TRIM_CORE(validationSegmentVisits); CHECK_TRIM_CORE(validationNextCalls);
    CHECK_TRIM_CORE(deepestPreparation); CHECK_TRIM_CORE(deepestPairRefinement);
    CHECK_TRIM_CORE(deepestOrderingRefinement); CHECK_TRIM_CORE(deepestRootRefinement);
#undef CHECK_TRIM_CORE
}

std::string workerTrimGeometry(const RS_Entity& entity) {
    if (entity.rtti() != RS2::EntityLine) return trimGeometry(entity);
    std::ostringstream out; out << std::hexfloat;
    const auto a = entity.getStartpoint(), b = entity.getEndpoint();
    out << a.x << ':' << a.y << ':' << a.z << ':' << b.x << ':' << b.y << ':' << b.z;
    return out.str();
}

void sameTrimOutputs(const LC_DocumentModificationBatch& a, const LC_DocumentModificationBatch& b) {
    REQUIRE(a.entitiesToAdd.size() == b.entitiesToAdd.size());
    CHECK(a.entitiesToDelete == b.entitiesToDelete);
    CHECK(a.setActiveLayer == b.setActiveLayer); CHECK(a.setActivePen == b.setActivePen);
    for (qsizetype i = 0; i < a.entitiesToAdd.size(); ++i) {
        const auto* x = a.entitiesToAdd[i]; const auto* y = b.entitiesToAdd[i];
        CHECK(x->rtti() == y->rtti()); CHECK(workerTrimGeometry(*x) == workerTrimGeometry(*y));
        CHECK(x->getPen(false) == y->getPen(false)); CHECK(x->getLayer(false) == y->getLayer(false));
        CHECK(x->getParent() == y->getParent()); CHECK(x->ownerLineage() == 0);
        CHECK(x->lineage() != y->lineage()); CHECK(x->sourceHandle() == 0);
    }
}
}

TEST_CASE("Prepared trim consumption matches synchronous output attributes and every work counter",
          "[lc-offsetcurve][trim-action][trim-worker-consumption]") {
    Drawing<false> f; auto* native = addNative(f.graphic); auto* cutter = line(f.graphic);
    LC_StoredCacheTrimOptions options;
    const auto limits = RS_Modification::storedCacheTrimRemainingLimits(options.kernel); REQUIRE(limits);
    const auto request = cacheTrimJob(f.graphic, *native, *cutter, {2, 0}, *limits);
    const auto completion = joinedTrimWorker(request);
    WorkerTrimBatch expected, actual;
    const auto synchronous = RS_Modification::trimStoredCache(f.graphic, *native, {2, 0}, *cutter, {0, .3}, false,
        expected.value, options);
    REQUIRE(synchronous.result);
    LC_StoredCacheTrimCompletions received{request, *completion};
    received.publicationUndoEpoch = f.graphic.undoStateEpoch();
    const auto result = RS_Modification::trimStoredCachePrepared(f.graphic, *native, {2, 0}, *cutter, {0, .3}, false,
        actual.value, received, options);
    REQUIRE(result.result); sameTrimWork(result.cacheWork, synchronous.cacheWork);
    CHECK(result.nativeDemotions == synchronous.nativeDemotions);
    CHECK(result.intersection1 == synchronous.intersection1); CHECK(result.intersection2 == synchronous.intersection2);
    sameTrimOutputs(actual.value, expected.value);
    for (const auto* output : actual.value.entitiesToAdd) plain(*output, *native);
    CHECK_FALSE(native->isDeleted()); CHECK_FALSE(cutter->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("Stored trim request builder matches original inputs and binds the second job to joined work",
          "[lc-offsetcurve][trim-action][trim-worker-request]") {
    Drawing<true> f; auto* native = addNative(f.graphic); auto* ordinary = line(f.graphic);
    LC_StoredCacheTrimOptions options;
    const trimWorkerRequest::Token token{1, 3, 17, qint64(f.graphic.undoStateEpoch())};
    for (bool nativeFirst : {false, true}) {
        RS_Entity& a = nativeFirst ? static_cast<RS_Entity&>(*native) : *ordinary;
        RS_Entity& b = nativeFirst ? static_cast<RS_Entity&>(*ordinary) : *native;
        const RS_Vector ap = nativeFirst ? RS_Vector{2, 0} : RS_Vector{0, .3};
        const RS_Vector bp = nativeFirst ? RS_Vector{0, .3} : RS_Vector{2, 0};
        auto first = RS_Modification::storedCacheTrimRequest(a, ap, b, bp, true, token, options.kernel);
        REQUIRE(first); REQUIRE(std::holds_alternative<trimWorkerRequest::TrimCache>(first->operation));
        auto completion = joinedTrimWorker(*first);
        auto nextToken = token; ++nextToken.requestId;
        auto second = RS_Modification::storedCacheTrimRequest(a, ap, b, bp, true, nextToken,
            options.kernel, first.get(), completion.get());
        REQUIRE(second);
        auto remaining = RS_Modification::storedCacheTrimRemainingLimits(options.kernel, completion.get()); REQUIRE(remaining);
        auto expected = cacheTrimJob(f.graphic, b, a, bp, *remaining, 2);
        CHECK(trimWorkerRequest::encode(*second) == trimWorkerRequest::encode(expected));
        auto changed = *first; std::get<trimWorkerRequest::TrimCache>(changed.operation).pick.worldPoint.x += 1;
        CHECK_FALSE(RS_Modification::storedCacheTrimRequest(a, ap, b, bp, true, nextToken,
            options.kernel, &changed, completion.get()));
        CHECK_FALSE(RS_Modification::storedCacheTrimRequest(a, ap, b, bp, false, nextToken,
            options.kernel, first.get(), completion.get()));
        CHECK_FALSE(RS_Modification::storedCacheTrimRequest(a, ap, b, bp, true, token,
            options.kernel, first.get(), completion.get()));
        CHECK_FALSE(RS_Modification::storedCacheTrimRequest(a, ap, b, bp, true, nextToken,
            options.kernel, first.get(), nullptr));
    }
}

TEST_CASE("Stored trim preparation owns complete joined sequences without exposing a prefix",
          "[lc-offsetcurve][trim-action][trim-worker-preparation]") {
    Drawing<true> f; auto* native = addNative(f.graphic); auto* ordinary = line(f.graphic);
    LC_StoredCacheTrimOptions options;
    const trimWorkerRequest::Token token{1, 3, 17, qint64(f.graphic.undoStateEpoch())};
    LC_StoredCacheTrimPreparation preparation(*native, {2, 0}, *ordinary, {0, .3}, true, options.kernel);
    CHECK_FALSE(preparation.ready()); CHECK_FALSE(preparation.completions(f.graphic.undoStateEpoch()));
    auto firstRequest = preparation.request(token); REQUIRE(firstRequest);
    CHECK_FALSE(preparation.request(token));
    auto wrongRequest = *firstRequest; ++wrongRequest.token.requestId;
    auto wrong = joinedTrimWorker(wrongRequest);
    CHECK_FALSE(preparation.accept(std::move(wrong))); CHECK_FALSE(preparation.ready());
    auto first = joinedTrimWorker(*firstRequest); REQUIRE(preparation.accept(std::move(first)));
    CHECK_FALSE(preparation.ready()); CHECK_FALSE(preparation.completions(f.graphic.undoStateEpoch()));
    auto nextToken = token; ++nextToken.requestId;
    auto secondRequest = preparation.request(nextToken); REQUIRE(secondRequest);
    auto second = joinedTrimWorker(*secondRequest); REQUIRE(preparation.accept(std::move(second)));
    REQUIRE(preparation.ready()); CHECK_FALSE(preparation.request(nextToken));
    auto received = preparation.completions(f.graphic.undoStateEpoch()); REQUIRE(received);
    WorkerTrimBatch expected, actual;
    const auto sync = RS_Modification::trimStoredCache(f.graphic, *native, {2, 0}, *ordinary, {0, .3}, true,
        expected.value, options);
    const auto result = RS_Modification::trimStoredCachePrepared(f.graphic, *native, {2, 0}, *ordinary, {0, .3}, true,
        actual.value, *received, options);
    REQUIRE(sync.result); REQUIRE(result.result); sameTrimWork(result.cacheWork, sync.cacheWork);
    sameTrimOutputs(actual.value, expected.value);
    CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ordinary->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("Stored trim preparation completes a conic pair with one matched job",
          "[lc-offsetcurve][trim-action][trim-worker-preparation]") {
    Drawing<true> f; auto* native = addNative(f.graphic);
    auto* circle = new RS_Circle(&f.graphic, RS_CircleData{{1, 0}, 2}); f.graphic.addEntity(circle);
    LC_StoredCacheTrimOptions options;
    LC_StoredCacheTrimPreparation preparation(*native, {2, 0}, *circle, {-1, 0}, true, options.kernel);
    auto request = preparation.request({1, 3, 17, qint64(f.graphic.undoStateEpoch())}); REQUIRE(request);
    REQUIRE(std::holds_alternative<trimWorkerRequest::TrimConic>(request->operation));
    auto completion = joinedTrimWorker(*request); REQUIRE(preparation.accept(std::move(completion)));
    REQUIRE(preparation.ready()); auto received = preparation.completions(f.graphic.undoStateEpoch()); REQUIRE(received);
    CHECK(received->secondRequest == nullptr); CHECK(received->second == nullptr);
    WorkerTrimBatch actual;
    const auto result = RS_Modification::trimStoredCachePrepared(f.graphic, *native, {2, 0}, *circle, {-1, 0}, true,
        actual.value, *received, options);
    CHECK(result.result); CHECK(result.nativeDemotions == 1); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("Stored trim preparation completes one-sided cache trim with one matched job",
          "[lc-offsetcurve][trim-action][trim-worker-preparation]") {
    Drawing<false> f; auto* native = addNative(f.graphic); auto* cutter = line(f.graphic);
    LC_StoredCacheTrimOptions options;
    LC_StoredCacheTrimPreparation preparation(*native, {2, 0}, *cutter, {0, .3}, false, options.kernel);
    auto request = preparation.request({1, 3, 17, qint64(f.graphic.undoStateEpoch())}); REQUIRE(request);
    REQUIRE(std::holds_alternative<trimWorkerRequest::TrimCache>(request->operation));
    auto completion = joinedTrimWorker(*request); REQUIRE(preparation.accept(std::move(completion)));
    REQUIRE(preparation.ready()); auto received = preparation.completions(f.graphic.undoStateEpoch()); REQUIRE(received);
    CHECK(received->secondRequest == nullptr); CHECK(received->second == nullptr);
    WorkerTrimBatch actual;
    const auto result = RS_Modification::trimStoredCachePrepared(f.graphic, *native, {2, 0}, *cutter, {0, .3}, false,
        actual.value, *received, options);
    CHECK(result.result); CHECK(result.nativeDemotions == 1); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("Stored trim preparation retains a first-job refusal without starting a second job",
          "[lc-offsetcurve][trim-action][trim-worker-preparation]") {
    Drawing<true> f; auto* native = addNative(f.graphic);
    auto* distant = new RS_Line(&f.graphic, RS_Vector{-3, 100}, RS_Vector{3, 100}); f.graphic.addEntity(distant);
    LC_StoredCacheTrimOptions options;
    LC_StoredCacheTrimPreparation preparation(*native, {2, 0}, *distant, {0, 100}, true, options.kernel);
    auto request = preparation.request({1, 3, 17, qint64(f.graphic.undoStateEpoch())}); REQUIRE(request);
    auto completion = joinedTrimWorker(*request); REQUIRE(preparation.accept(std::move(completion)));
    REQUIRE(preparation.ready()); CHECK_FALSE(preparation.request({2, 3, 17, qint64(f.graphic.undoStateEpoch())}));
    auto received = preparation.completions(f.graphic.undoStateEpoch()); REQUIRE(received);
    WorkerTrimBatch actual;
    const auto result = RS_Modification::trimStoredCachePrepared(f.graphic, *native, {2, 0}, *distant, {0, 100}, true,
        actual.value, *received, options);
    CHECK_FALSE(result.result); CHECK(result.cacheRefusal == LC_OffsetCacheTrim::Refusal::NoUsableRoots);
    CHECK(actual.value.entitiesToAdd.empty()); CHECK(actual.value.entitiesToDelete.empty()); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("Async trim worker owns detached inputs and returns only a complete joined sequence",
          "[lc-offsetcurve][trim-action][trim-worker-async]") {
    Drawing<true> f; auto* native = addNative(f.graphic); auto* ordinary = line(f.graphic);
    const auto copy = [](RS_Entity* source) { std::unique_ptr<RS_Entity> value(source->clone()); value->setParent(nullptr); return value; };
    LC_OffsetTrimWorker worker; std::optional<LC_OffsetTrimWorker::Outcome> outcome;
    int ticks = 0; QTimer heartbeat; QObject::connect(&heartbeat, &QTimer::timeout, [&] { ++ticks; }); heartbeat.start(0);
    LC_StoredCacheTrimOptions options;
    REQUIRE(worker.start(copy(native), {2, 0}, copy(ordinary), {0, .3}, true, options.kernel, 17,
        qint64(f.graphic.undoStateEpoch()), LC_OffsetWorkerClient::Mode::Commit,
        [&](LC_OffsetTrimWorker::Outcome value) { outcome.emplace(std::move(value)); }));
    CHECK(f.graphic.count() == 2); CHECK_FALSE(canUndo(f.graphic));
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) { REQUIRE(elapsed.elapsed() < 35000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    heartbeat.stop(); CHECK(ticks > 0); REQUIRE(outcome->status == LC_OffsetTrimWorker::Status::Completed);
    REQUIRE(outcome->clientStatus); CHECK(*outcome->clientStatus == LC_OffsetWorkerClient::Status::Completed);
    REQUIRE(outcome->prepared); CHECK(outcome->prepared->target().getParent() == nullptr);
    CHECK(outcome->prepared->target().getLayer(false) == nullptr);
    auto received = outcome->prepared->completions(f.graphic.undoStateEpoch()); REQUIRE(received);
    REQUIRE(received->second); WorkerTrimBatch actual, expected;
    const auto result = RS_Modification::trimStoredCachePrepared(f.graphic, *native, {2, 0}, *ordinary, {0, .3}, true,
        actual.value, *received, options);
    const auto sync = RS_Modification::trimStoredCache(f.graphic, *native, {2, 0}, *ordinary, {0, .3}, true,
        expected.value, options);
    REQUIRE(result.result); REQUIRE(sync.result); sameTrimOutputs(actual.value, expected.value);
    sameTrimWork(result.cacheWork, sync.cacheWork); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("Native Trim action commits asynchronously with a responsive event loop",
          "[gui][lc-offsetcurve][trim-action][trim-worker-commit]") {
    Drawing<true> f; auto* native = addNative(f.graphic); auto* ordinary = line(f.graphic);
    f.action->click({2, 0});
    int ticks = 0; QTimer heartbeat; QObject::connect(&heartbeat, &QTimer::timeout, [&] { ++ticks; }); heartbeat.start(0);
    f.action->clickAsync({0, .3}); REQUIRE(f.action->pending());
    CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ordinary->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    f.action->waitForTrim(); heartbeat.stop(); CHECK(ticks > 0);
    REQUIRE(native->isDeleted()); REQUIRE(ordinary->isDeleted());
    REQUIRE(f.graphic.undo()); CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ordinary->isDeleted());
    CHECK_FALSE(f.graphic.undo()); REQUIRE(f.graphic.redo()); CHECK(native->isDeleted()); CHECK(ordinary->isDeleted());
}

TEST_CASE("Native Trim preview supersedes an earlier hover and matches the final commit",
          "[gui][lc-offsetcurve][trim-action][trim-worker-preview]") {
    Drawing<false> f; auto* native = addNative(f.graphic); auto* limit = line(f.graphic);
    f.action->click({-2.8, .3});
    int ticks = 0; QTimer heartbeat; QObject::connect(&heartbeat, &QTimer::timeout, [&] { ++ticks; }); heartbeat.start(0);
    f.action->hoverAsync({2, 0}); REQUIRE(f.action->pending());
    f.action->hoverAsync({-2, 0}); REQUIRE(f.action->pending());
    CHECK_FALSE(native->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    f.action->waitForTrim(); heartbeat.stop(); CHECK(ticks > 0);
    const auto preview = trimPreview(*f.action->m_preview); REQUIRE_FALSE(preview.empty());
    WorkerTrimBatch expected;
    const auto synchronous = RS_Modification::trimStoredCache(f.graphic, *native, {-2, 0}, *limit, {-2.8, .3}, false, expected.value);
    REQUIRE(synchronous.result);
    std::vector<std::string> expectedGeometry;
    for (auto* entity : std::as_const(expected.value.entitiesToAdd)) expectedGeometry.push_back(trimGeometry(*entity));
    CHECK(preview == expectedGeometry);
    f.action->click({-2, 0}); REQUIRE(native->isDeleted());
    std::vector<std::string> committed;
    for (auto* entity : live(f.graphic)) if (entity != limit) committed.push_back(trimGeometry(*entity));
    CHECK(committed == preview); REQUIRE(f.graphic.undo()); CHECK_FALSE(native->isDeleted());
}

TEST_CASE("Native context-target preview seals a nested limit and cancels on replacement",
          "[gui][lc-offsetcurve][trim-action][trim-worker-preview][nested][asan]") {
    for (unsigned removal = 0; removal < 6; ++removal) {
        CAPTURE(removal);
        Drawing<false> f; auto* native = addNative(f.graphic); auto* limit = polylineLimit(f.graphic, false);
        limit->setLayer(f.graphic.getActiveLayer());
        f.action->contextTarget(native, {2, 0});
        f.action->hoverAsync({-2.8, .3}); REQUIRE(f.action->pending());
        if (removal) {
            if (removal == 1) REQUIRE(limit->removeEntity(limit->entityAt(0)));
            else if (removal == 2) REQUIRE(f.graphic.removeEntity(limit));
            else if (removal == 3) limit->entityAt(0)->move({0, .1});
            else if (removal == 4) limit->setVisible(false);
            else f.graphic.getActiveLayer()->freeze(true);
            f.action->waitForTrim(); CHECK(f.action->m_preview->isEmpty());
            CHECK_FALSE(native->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
        } else {
            f.action->waitForTrim(); const auto preview = trimPreview(*f.action->m_preview); REQUIRE_FALSE(preview.empty());
            f.action->click({-2.8, .3}); REQUIRE(native->isDeleted());
            std::vector<std::string> committed;
            for (auto* entity : live(f.graphic)) if (entity != limit) committed.push_back(trimGeometry(*entity));
            CHECK(committed == preview); REQUIRE(f.graphic.undo()); CHECK_FALSE(native->isDeleted());
        }
    }
}

TEST_CASE("Native Trim preview cancels on Escape and suppresses late delivery on destruction",
          "[gui][lc-offsetcurve][trim-action][trim-worker-preview][lifetime][asan]") {
    SECTION("Escape and retry") {
        Drawing<false> f; auto* native = addNative(f.graphic); line(f.graphic);
        f.action->click({-2.8, .3}); f.action->hoverAsync({2, 0}); REQUIRE(f.action->pending());
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier); f.action->keyPressEvent(&escape);
        CHECK_FALSE(f.action->pending()); CHECK(f.action->m_preview->isEmpty()); CHECK_FALSE(canUndo(f.graphic));
        f.action->hover({2, 0}); CHECK_FALSE(trimPreview(*f.action->m_preview).empty()); CHECK_FALSE(native->isDeleted());
    }
    SECTION("action") {
        Drawing<false> f; auto* native = addNative(f.graphic); auto* limit = line(f.graphic);
        f.action->click({-2.8, .3}); f.action->hoverAsync({2, 0}); REQUIRE(f.action->pending());
        f.action.reset();
        QElapsedTimer elapsed; elapsed.start();
        while (elapsed.elapsed() < 150) { QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(limit->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    }
    SECTION("document") {
        Drawing<false> f; f.action.reset();
        auto document = std::make_unique<RS_Graphic>(); document->initForNewDocument();
        f.view.setDocument(document.get()); f.context.setDocumentAndView(document.get(), &f.view);
        f.action = std::make_unique<CacheTrim<false>>(&f.context);
        addNative(*document); line(*document);
        f.action->click({-2.8, .3}); f.action->hoverAsync({2, 0}); REQUIRE(f.action->pending());
        document.reset(); CHECK(f.action->isFinished()); CHECK_FALSE(f.action->pending());
        f.action.reset(); f.view.setDocument(&f.graphic); f.context.setDocumentAndView(&f.graphic, &f.view);
        QElapsedTimer elapsed; elapsed.start();
        while (elapsed.elapsed() < 150) { QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    }
}

TEST_CASE("Native Trim public mouse move tolerates action destruction during preview admission",
          "[gui][lc-offsetcurve][trim-action][trim-worker-preview][lifetime][asan]") {
    Drawing<false> f; auto* native = addNative(f.graphic); auto* limit = line(f.graphic);
    f.view.getViewPort()->setSize(640, 480); f.view.getViewPort()->setOffsetAndFactor(192, 240, 64);
    f.action->init(0); f.action->click({-2.8, .3});
    f.action->onOptionsRead = [&] { f.action.reset(); };
    const QPointF pos(f.view.getViewPort()->toGuiX(2), f.view.getViewPort()->toGuiY(0));
    QMouseEvent event(QEvent::MouseMove, pos, pos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    auto* action = f.action.get(); action->mouseMoveEvent(&event);
    CHECK_FALSE(f.action); CHECK_FALSE(native->isDeleted()); CHECK_FALSE(limit->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("Native Trim preview rechecks sources after option callbacks",
          "[gui][lc-offsetcurve][trim-action][trim-worker-preview][stale][asan]") {
    SECTION("hovered target removed") {
        Drawing<false> f; auto* native = addNative(f.graphic); auto* limit = line(f.graphic);
        f.action->click({-2.8, .3});
        f.action->onOptionsRead = [&] { REQUIRE(f.graphic.removeEntity(native)); };
        f.action->hoverAsync({2, 0});
        CHECK_FALSE(f.action->pending()); CHECK(trimPreview(*f.action->m_preview).empty());
        CHECK_FALSE(limit->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    }
    SECTION("nested limit removed") {
        Drawing<false> f; auto* native = addNative(f.graphic); auto* limit = polylineLimit(f.graphic, false);
        f.action->contextTarget(native, {2, 0});
        f.action->onOptionsRead = [&] { REQUIRE(limit->removeEntity(limit->entityAt(0))); };
        f.action->hoverAsync({-2.8, .3});
        CHECK_FALSE(f.action->pending()); CHECK(trimPreview(*f.action->m_preview).empty());
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    }
    SECTION("source changed without undo") {
        Drawing<false> f; auto* native = addNative(f.graphic); auto* limit = line(f.graphic);
        f.action->click({-2.8, .3});
        f.action->onOptionsRead = [&] { limit->move({0, .1}); };
        f.action->hoverAsync({2, 0});
        CHECK_FALSE(f.action->pending()); CHECK(trimPreview(*f.action->m_preview).empty());
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    }
}

TEST_CASE("Native Trim commit rejects source changes from option callbacks before publication",
          "[gui][lc-offsetcurve][trim-action][trim-worker-commit][stale]") {
    Drawing<false> f; auto* native = addNative(f.graphic); auto* target = line(f.graphic);
    f.action->click({2, 0});
    f.action->onOptionsRead = [&] { target->move({0, .1}); };
    f.action->clickAsync({0, .3});
    CHECK_FALSE(f.action->pending()); CHECK_FALSE(native->isDeleted()); CHECK_FALSE(target->isDeleted());
    CHECK_FALSE(canUndo(f.graphic)); REQUIRE(f.context.messages.size() == 1);
    CHECK(f.context.messages.front().contains("nothing changed"));
}

TEST_CASE("Native Trim preview clears a refused worker result and recovers",
          "[gui][lc-offsetcurve][trim-action][trim-worker-preview][refusal]") {
    Drawing<false> f; auto* native = addNative(f.graphic); auto* limit = line(f.graphic);
    f.action->click({-2.8, .3}); f.action->options.kernel.maxBoxPairs = 0;
    f.action->hoverAsync({2, 0}); REQUIRE(f.action->pending()); f.action->waitForTrim();
    CHECK(f.action->m_preview->isEmpty());
    CHECK_FALSE(native->isDeleted()); CHECK_FALSE(limit->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    CHECK(f.context.messages.isEmpty());
    f.action->options = {}; f.action->hover({2, 0});
    CHECK_FALSE(trimPreview(*f.action->m_preview).empty()); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("Native Trim preview discards undrawn private content on cancellation",
          "[gui][lc-offsetcurve][trim-action][trim-worker-preview][stale]") {
    Drawing<false> f; addNative(f.graphic); auto* limit = line(f.graphic);
    f.action->click({-2.8, .3});
    f.action->m_preview->addEntity(new RS_Line(nullptr, {10, 10}, {11, 10}));
    f.action->guardedHoverAsync({2, 0}); REQUIRE(f.action->pending());
    limit->move({0, .1}); f.action->waitForTrim();
    CHECK(f.action->m_preview->isEmpty()); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("Native Trim preview cancels after action and document changes",
          "[gui][lc-offsetcurve][trim-action][trim-worker-preview][stale]") {
    for (unsigned change = 0; change < 5; ++change) {
        CAPTURE(change);
        Drawing<false> f; auto* native = addNative(f.graphic); auto* limit = line(f.graphic);
        f.action->click({-2.8, .3}); f.action->hoverAsync({2, 0}); REQUIRE(f.action->pending());
        if (change == 0) {
            const auto before = limit->getStartpoint(); limit->move({0, .1});
            REQUIRE(limit->getStartpoint() != before);
        }
        else if (change == 1) RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {native}, true);
        else if (change == 2) f.view.getViewPort()->setOffsetAndFactor(100, 100, 3);
        else if (change == 3) f.action->options.maxOutputEntities = 1;
        else f.action->togglePreviewRefs();
        f.action->waitForTrim(); CHECK(f.action->m_preview->isEmpty());
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(limit->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
        CHECK(f.context.messages.isEmpty());
    }
}

TEST_CASE("Native Trim action refuses changed inputs while a worker is pending",
          "[gui][lc-offsetcurve][trim-action][trim-worker-commit][stale]") {
    for (unsigned change = 0; change < 6; ++change) {
        CAPTURE(change);
        Drawing<true> f; auto* native = addNative(f.graphic); auto* ordinary = line(f.graphic);
        native->setLayer(f.graphic.getActiveLayer()); ordinary->setLayer(f.graphic.getActiveLayer());
        f.action->click({2, 0}); f.action->clickAsync({0, .3}); REQUIRE(f.action->pending());
        const auto epoch = f.graphic.undoStateEpoch();
        if (change == 0) ordinary->move({0, .1});
        else if (change == 1) RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {ordinary}, true);
        else if (change == 2) f.view.getViewPort()->setOffsetAndFactor(100, 100, 3);
        else if (change == 3) f.action->options.maxOutputEntities = 1;
        else if (change == 4) f.graphic.getActiveLayer()->lock(true);
        else *native->getDrwExtData()[1]->content.s = "changed";
        f.action->waitForTrim();
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ordinary->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
        CHECK(f.graphic.undoStateEpoch() == epoch);
        REQUIRE(f.context.messages.size() == 1); CHECK(f.context.messages.front().contains("nothing changed"));
    }
}

TEST_CASE("Native Trim action cancels a pending worker and can retry",
          "[gui][lc-offsetcurve][trim-action][trim-worker-commit][cancel]") {
    Drawing<false> f; auto* native = addNative(f.graphic); auto* ordinary = line(f.graphic);
    f.action->click({2, 0}); f.action->clickAsync({0, .3}); REQUIRE(f.action->pending());
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier); f.action->keyPressEvent(&escape);
    CHECK_FALSE(f.action->pending()); CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ordinary->isDeleted());
    CHECK_FALSE(canUndo(f.graphic));
    f.action->click({0, .3}); REQUIRE(ordinary->isDeleted()); CHECK_FALSE(native->isDeleted());
    REQUIRE(f.graphic.undo()); CHECK_FALSE(ordinary->isDeleted()); CHECK_FALSE(f.graphic.undo());
}

namespace {
void trimActionWorkerFailure(bool preview, bool timeout) {
    CAPTURE(preview, timeout);
    Drawing<false> f; auto* native = addNative(f.graphic); auto* ordinary = line(f.graphic);
    f.action->click(preview ? RS_Vector{-2.8, .3} : RS_Vector{2, 0});
    auto& worker = LC_ActionModifyTrimTestAccess::worker(*f.action);
    if (timeout) {
        LC_OffsetTrimWorkerTestAccess::deadline(worker, 500);
        LC_OffsetTrimWorkerTestAccess::previewDeadline(worker, 500);
    }
    qint64 childId = 0; bool faultInjected = false;
    LC_OffsetTrimWorkerTestAccess::onStarted(worker, [&](QProcess& process) {
        childId = process.processId();
        if (timeout) {
#ifdef Q_OS_UNIX
            faultInjected = ::kill(static_cast<pid_t>(childId), SIGSTOP) == 0;
#endif
        } else { process.kill(); faultInjected = true; }
    });
    if (preview) f.action->hoverAsync({2, 0}); else f.action->clickAsync({0, .3});
    REQUIRE(f.action->pending());
    int ticks = 0; QTimer heartbeat; QObject::connect(&heartbeat, &QTimer::timeout, [&] { ++ticks; }); heartbeat.start(0);
    f.action->waitForTrim();
    heartbeat.stop(); REQUIRE(childId > 0); REQUIRE(faultInjected);
    if (timeout) CHECK(ticks > 0);
    CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ordinary->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    if (preview) { CHECK(f.action->m_preview->isEmpty()); CHECK(f.context.messages.isEmpty()); }
    else {
        REQUIRE(f.context.messages.size() == 1);
        CHECK(f.context.messages.front().contains(timeout ? "timed out" : "worker failed"));
    }
    QElapsedTimer elapsed; elapsed.start();
    while (worker.busy()) {
        REQUIRE(elapsed.elapsed() < 5000);
        QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1);
    }
#ifdef Q_OS_UNIX
    errno = 0;
    CHECK(::kill(static_cast<pid_t>(childId), 0) == -1);
    CHECK(errno == ESRCH);
#endif
    LC_OffsetTrimWorkerTestAccess::deadline(worker, LC_OffsetWorkerClient::Deadlines::commit);
    LC_OffsetTrimWorkerTestAccess::previewDeadline(worker, LC_OffsetWorkerClient::Deadlines::preview);
    if (preview) {
        f.action->hover({2, 0}); CHECK_FALSE(trimPreview(*f.action->m_preview).empty());
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    } else {
        f.action->click({0, .3}); REQUIRE(ordinary->isDeleted()); CHECK_FALSE(native->isDeleted());
        REQUIRE(f.graphic.undo()); CHECK_FALSE(ordinary->isDeleted()); CHECK_FALSE(f.graphic.undo());
    }
}
}

TEST_CASE("Native Trim action reaps a killed real helper and retries",
          "[gui][lc-offsetcurve][trim-action][trim-worker-action-fault]") {
    trimActionWorkerFailure(false, false); trimActionWorkerFailure(true, false);
}

#ifdef Q_OS_UNIX
TEST_CASE("Native Trim action reaps a stopped helper after its deadline and retries",
          "[gui][lc-offsetcurve][trim-action][trim-worker-action-fault][timeout]") {
    trimActionWorkerFailure(false, true); trimActionWorkerFailure(true, true);
}
#endif

TEST_CASE("Native Trim action suppresses late delivery after action or document destruction",
          "[gui][lc-offsetcurve][trim-action][trim-worker-commit][lifetime][asan]") {
    SECTION("action") {
        Drawing<true> f; auto* native = addNative(f.graphic); auto* ordinary = line(f.graphic);
        f.action->click({2, 0}); f.action->clickAsync({0, .3}); REQUIRE(f.action->pending());
        f.action.reset();
        QElapsedTimer elapsed; elapsed.start();
        while (elapsed.elapsed() < 150) { QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ordinary->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    }
    SECTION("document") {
        Drawing<true> f; f.action.reset();
        auto document = std::make_unique<RS_Graphic>(); document->initForNewDocument();
        f.view.setDocument(document.get()); f.context.setDocumentAndView(document.get(), &f.view);
        f.action = std::make_unique<CacheTrim<true>>(&f.context);
        addNative(*document); line(*document);
        f.action->click({2, 0}); f.action->clickAsync({0, .3}); REQUIRE(f.action->pending());
        document.reset(); CHECK(f.action->isFinished()); CHECK_FALSE(f.action->pending());
        f.action.reset(); f.view.setDocument(&f.graphic); f.context.setDocumentAndView(&f.graphic, &f.view);
        QElapsedTimer elapsed; elapsed.start();
        while (elapsed.elapsed() < 150) { QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    }
}

TEST_CASE("Async trim worker consumes sealed detached limit snapshots",
          "[lc-offsetcurve][trim-action][trim-worker-async][trim-sources]") {
    Drawing<false> f; auto* native = addNative(f.graphic);
    auto limit = std::make_unique<RS_Line>(nullptr, RS_Vector{-3, .3}, RS_Vector{3, .3});
    auto inputs = LC_OffsetCreationSources::captureDetachedLimit(f.graphic, *native, *limit); REQUIRE(inputs);
    LC_OffsetTrimWorker worker; LC_StoredCacheTrimOptions options;
    std::optional<LC_OffsetTrimWorker::Outcome> outcome;
    REQUIRE(worker.start(inputs->cloneWorkerSource(0), {2, 0}, inputs->cloneWorkerSource(1), {0, .3},
        false, options.kernel, 17, qint64(f.graphic.undoStateEpoch()), LC_OffsetWorkerClient::Mode::Commit,
        [&](LC_OffsetTrimWorker::Outcome value) { outcome.emplace(std::move(value)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) { REQUIRE(elapsed.elapsed() < 35000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    REQUIRE(outcome->status == LC_OffsetTrimWorker::Status::Completed); REQUIRE(outcome->prepared);
    REQUIRE(inputs->matchesDetached(f.graphic, limit.get()));
    auto receipt = outcome->prepared->completions(f.graphic.undoStateEpoch()); REQUIRE(receipt);
    WorkerTrimBatch actual, expected;
    const auto result = RS_Modification::trimStoredCachePrepared(f.graphic, *native, {2, 0}, *limit, {0, .3},
        false, actual.value, *receipt, options);
    const auto synchronous = RS_Modification::trimStoredCache(f.graphic, *native, {2, 0}, *limit, {0, .3},
        false, expected.value, options);
    REQUIRE(result.result); REQUIRE(synchronous.result); sameTrimOutputs(actual.value, expected.value);
    sameTrimWork(result.cacheWork, synchronous.cacheWork); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("Async trim worker cancels without delivery and recovers after a shared deadline",
          "[lc-offsetcurve][trim-action][trim-worker-async][timeout]") {
    Drawing<false> f; auto* native = addNative(f.graphic); auto* cutter = line(f.graphic);
    const auto copy = [](RS_Entity* source) { std::unique_ptr<RS_Entity> value(source->clone()); value->setParent(nullptr); return value; };
    LC_OffsetTrimWorker worker; LC_StoredCacheTrimOptions options;
    std::optional<LC_OffsetTrimWorker::Outcome> outcome; int deliveries = 0;
    const auto start = [&] { return worker.start(copy(native), {2, 0}, copy(cutter), {0, .3}, false, options.kernel,
        17, qint64(f.graphic.undoStateEpoch()), LC_OffsetWorkerClient::Mode::Commit,
        [&](LC_OffsetTrimWorker::Outcome value) { ++deliveries; outcome.emplace(std::move(value)); }); };
    const auto wait = [&](const std::function<bool()>& ready) {
        QElapsedTimer elapsed; elapsed.start();
        while (!ready()) { REQUIRE(elapsed.elapsed() < 35000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    };
    REQUIRE(start()); wait([&] { return LC_OffsetTrimWorkerTestAccess::processRunning(worker) || bool(outcome); });
    REQUIRE(LC_OffsetTrimWorkerTestAccess::processRunning(worker));
    worker.cancel(); wait([&] { return !worker.busy(); }); CHECK(deliveries == 0);
    LC_OffsetTrimWorkerTestAccess::deadline(worker, 500); REQUIRE(start());
    wait([&] { return LC_OffsetTrimWorkerTestAccess::processRunning(worker) || bool(outcome); });
    REQUIRE(LC_OffsetTrimWorkerTestAccess::processRunning(worker)); QThread::msleep(600);
    wait([&] { return bool(outcome); }); REQUIRE(outcome->status == LC_OffsetTrimWorker::Status::TimedOut);
    CHECK_FALSE(outcome->prepared); CHECK(deliveries == 1); outcome.reset();
    LC_OffsetTrimWorkerTestAccess::deadline(worker, LC_OffsetWorkerClient::Deadlines::commit);
    REQUIRE(start()); wait([&] { return bool(outcome); }); REQUIRE(outcome->status == LC_OffsetTrimWorker::Status::Completed);
    REQUIRE(outcome->prepared); CHECK(deliveries == 2); CHECK(f.graphic.count() == 2); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("Async trim worker survives source-document death and suppresses delivery after owner death",
          "[lc-offsetcurve][trim-action][trim-worker-async][lifetime][asan]") {
    REQUIRE(lc::test::application());
    auto graphic = std::make_unique<RS_Graphic>(); graphic->initForNewDocument();
    auto* native = addNative(*graphic); auto* cutter = line(*graphic);
    const auto copy = [](RS_Entity* source) { std::unique_ptr<RS_Entity> value(source->clone()); value->setParent(nullptr); return value; };
    auto worker = std::make_unique<LC_OffsetTrimWorker>(); LC_StoredCacheTrimOptions options;
    std::optional<LC_OffsetTrimWorker::Outcome> outcome; int deliveries = 0;
    REQUIRE(worker->start(copy(native), {2, 0}, copy(cutter), {0, .3}, false, options.kernel,
        17, qint64(graphic->undoStateEpoch()), LC_OffsetWorkerClient::Mode::Commit,
        [&](LC_OffsetTrimWorker::Outcome value) { ++deliveries; outcome.emplace(std::move(value)); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!LC_OffsetTrimWorkerTestAccess::clientBusy(*worker)) {
        REQUIRE(elapsed.elapsed() < 35000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1);
    }
    SECTION("document dies") {
        graphic.reset();
        while (!outcome) { REQUIRE(elapsed.elapsed() < 35000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
        REQUIRE(outcome->status == LC_OffsetTrimWorker::Status::Completed); REQUIRE(outcome->prepared);
        CHECK(outcome->prepared->target().getParent() == nullptr);
        CHECK(outcome->prepared->target().getLayer(false) == nullptr); CHECK(deliveries == 1);
    }
    SECTION("worker dies") {
        worker.reset();
        QElapsedTimer settle; settle.start();
        while (settle.elapsed() < 100) { QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
        CHECK(deliveries == 0); CHECK_FALSE(outcome); CHECK(graphic->count() == 2); CHECK_FALSE(canUndo(*graphic));
    }
}

TEST_CASE("Async trim worker can be destroyed by its completion callback",
          "[lc-offsetcurve][trim-action][trim-worker-async][lifetime][asan]") {
    Drawing<false> f; auto* native = addNative(f.graphic); auto* cutter = line(f.graphic);
    const auto copy = [](RS_Entity* source) { std::unique_ptr<RS_Entity> value(source->clone()); value->setParent(nullptr); return value; };
    auto worker = std::make_unique<LC_OffsetTrimWorker>(); LC_StoredCacheTrimOptions options;
    std::optional<LC_OffsetTrimWorker::Outcome> outcome;
    REQUIRE(worker->start(copy(native), {2, 0}, copy(cutter), {0, .3}, false, options.kernel,
        17, qint64(f.graphic.undoStateEpoch()), LC_OffsetWorkerClient::Mode::Commit,
        [&](LC_OffsetTrimWorker::Outcome value) { outcome.emplace(std::move(value)); worker.reset(); }));
    QElapsedTimer elapsed; elapsed.start();
    while (!outcome) { REQUIRE(elapsed.elapsed() < 35000); QCoreApplication::processEvents(QEventLoop::AllEvents); QThread::msleep(1); }
    CHECK_FALSE(worker); REQUIRE(outcome->status == LC_OffsetTrimWorker::Status::Completed);
    REQUIRE(outcome->prepared); CHECK(outcome->prepared->ready()); CHECK_FALSE(canUndo(f.graphic));
}

TEST_CASE("Prepared nonconic Trim Two uses two original operands and the exact shared remaining limits",
          "[lc-offsetcurve][trim-action][trim-worker-consumption]") {
    for (bool nativeFirst : {false, true}) {
        Drawing<true> f; auto* native = addNative(f.graphic); auto* ordinary = line(f.graphic);
        RS_Entity& a = nativeFirst ? static_cast<RS_Entity&>(*native) : *ordinary;
        RS_Entity& b = nativeFirst ? static_cast<RS_Entity&>(*ordinary) : *native;
        const RS_Vector ap = nativeFirst ? RS_Vector{2, 0} : RS_Vector{0, .3};
        const RS_Vector bp = nativeFirst ? RS_Vector{0, .3} : RS_Vector{2, 0};
        LC_StoredCacheTrimOptions options;
        const auto initial = RS_Modification::storedCacheTrimRemainingLimits(options.kernel); REQUIRE(initial);
        const auto firstRequest = cacheTrimJob(f.graphic, a, b, ap, *initial);
        const auto first = joinedTrimWorker(firstRequest);
        const auto remaining = RS_Modification::storedCacheTrimRemainingLimits(options.kernel, first.get()); REQUIRE(remaining);
        CHECK(remaining->maxBoxPairs < initial->maxBoxPairs);
        CHECK(remaining->maxInputRecords < initial->maxInputRecords);
        const auto secondRequest = cacheTrimJob(f.graphic, b, a, bp, *remaining, 2);
        const auto second = joinedTrimWorker(secondRequest);
        WorkerTrimBatch expected, actual;
        const auto sync = RS_Modification::trimStoredCache(f.graphic, a, ap, b, bp, true, expected.value, options);
        REQUIRE(sync.result);
        LC_StoredCacheTrimCompletions received{firstRequest, *first, &secondRequest, second.get(),
            f.graphic.undoStateEpoch()};
        const auto result = RS_Modification::trimStoredCachePrepared(f.graphic, a, ap, b, bp, true, actual.value, received, options);
        REQUIRE(result.result); sameTrimWork(result.cacheWork, sync.cacheWork);
        sameTrimOutputs(actual.value, expected.value); CHECK(result.nativeDemotions == 1);

        // A missing second result or a second query with fresh full caps cannot
        // be replaced by a parent-side numeric retry or a first-side publication.
        for (bool missing : {false, true}) {
            auto wrongRequest = cacheTrimJob(f.graphic, b, a, bp, *initial, 2);
            std::unique_ptr<LC_OffsetWorkerCompletion> wrong;
            if (!missing) wrong = joinedTrimWorker(wrongRequest);
            LC_StoredCacheTrimCompletions rejected{firstRequest, *first,
                missing ? nullptr : &wrongRequest, wrong.get(), f.graphic.undoStateEpoch()};
            WorkerTrimBatch refused;
            const auto no = RS_Modification::trimStoredCachePrepared(f.graphic, a, ap, b, bp, true, refused.value, rejected, options);
            CHECK_FALSE(no.result); CHECK(no.cacheRefusal == LC_OffsetCacheTrim::Refusal::InvalidInput);
            CHECK(no.trimmed1 == nullptr); CHECK(no.trimmed2 == nullptr);
            CHECK(refused.value.entitiesToAdd.empty()); CHECK(refused.value.entitiesToDelete.empty());
            sameTrimWork(no.cacheWork, std::get<LC_OffsetCacheTrim::Result>(first->reply().result).work);
        }
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ordinary->isDeleted());
    }
}

TEST_CASE("Prepared conic Trim Two consumes one paired op3 result in either operand order",
          "[lc-offsetcurve][trim-action][trim-worker-consumption]") {
    for (bool nativeFirst : {false, true}) {
        Drawing<true> f; auto* native = addNative(f.graphic);
        auto* circle = new RS_Circle(&f.graphic, RS_CircleData{{1, 0}, 2}); f.graphic.addEntity(circle);
        LC_StoredCacheTrimOptions options;
        const auto limits = RS_Modification::storedCacheTrimRemainingLimits(options.kernel); REQUIRE(limits);
        trimWorkerRequest::Request request;
        request.token = {1, 3, 17, qint64(f.graphic.undoStateEpoch())};
        request.operation = trimWorkerRequest::TrimConic{native->getData(), circle->getData(), {{-1, 0}},
            LC_OffsetCacheTrim::Pick{{2, 0}}, *limits};
        const auto completion = joinedTrimWorker(request);
        CHECK_FALSE(RS_Modification::storedCacheTrimRemainingLimits(options.kernel, completion.get()));
        WorkerTrimBatch expected, actual;
        RS_Entity& a = nativeFirst ? static_cast<RS_Entity&>(*native) : *circle;
        RS_Entity& b = nativeFirst ? static_cast<RS_Entity&>(*circle) : *native;
        const RS_Vector ap = nativeFirst ? RS_Vector{2, 0} : RS_Vector{-1, 0};
        const RS_Vector bp = nativeFirst ? RS_Vector{-1, 0} : RS_Vector{2, 0};
        const auto sync = RS_Modification::trimStoredCache(f.graphic, a, ap, b, bp, true, expected.value, options);
        REQUIRE(sync.result);
        LC_StoredCacheTrimCompletions received{request, *completion};
        received.publicationUndoEpoch = f.graphic.undoStateEpoch();
        const auto result = RS_Modification::trimStoredCachePrepared(f.graphic, a, ap, b, bp, true, actual.value, received, options);
        REQUIRE(result.result); sameTrimWork(result.cacheWork, sync.cacheWork); sameTrimOutputs(actual.value, expected.value);
        REQUIRE(result.trimmed1); REQUIRE(result.trimmed2); CHECK(result.nativeDemotions == 1);
    }
}

TEST_CASE("Prepared trim consumes all conic operand modes without changing the paired query shape",
          "[lc-offsetcurve][trim-action][trim-worker-consumption]") {
    for (unsigned kind = 0; kind < 4; ++kind) for (unsigned mode = 0; mode < 4; ++mode) {
        CAPTURE(kind, mode);
        Drawing<true> f; auto* native = addNative(f.graphic);
        std::unique_ptr<RS_Entity> value;
        if (kind == 0) value = std::make_unique<RS_Circle>(nullptr, RS_CircleData{{1, 0}, 2});
        if (kind == 1) value = std::make_unique<RS_Arc>(nullptr, RS_ArcData{{1, 0}, 2, .5, 5.8, false});
        if (kind >= 2) value = std::make_unique<RS_Ellipse>(nullptr,
            RS_EllipseData{{1, 0}, {2, 0}, .75, kind == 2 ? 0. : .5, kind == 2 ? 0. : 5.8, false});
        auto* conic = value.release(); f.graphic.addEntity(conic);
        conic->setUserDefVar("ordinary-trim", "retained");
        const bool both = mode >= 2, nativeFirst = mode == 0 || mode == 2;
        RS_Entity& a = nativeFirst ? static_cast<RS_Entity&>(*native) : *conic;
        RS_Entity& b = nativeFirst ? *conic : static_cast<RS_Entity&>(*native);
        const RS_Vector ap = nativeFirst ? RS_Vector{2, 0} : RS_Vector{-1, 0};
        const RS_Vector bp = nativeFirst ? RS_Vector{-1, 0} : RS_Vector{2, 0};
        LC_StoredCacheTrimOptions options;
        const auto limits = RS_Modification::storedCacheTrimRemainingLimits(options.kernel); REQUIRE(limits);
        auto request = cacheTrimJob(f.graphic, *native, *conic, {2, 0}, *limits);
        if (mode != 0) request.operation = trimWorkerRequest::TrimConic{native->getData(),
            workerTrimCutter(*conic), {{-1, 0}}, both ? std::optional<LC_OffsetCacheTrim::Pick>{{{2, 0}}} : std::nullopt,
            *limits};
        const auto built = RS_Modification::storedCacheTrimRequest(a, ap, b, bp, both, request.token, options.kernel);
        REQUIRE(built); CHECK(trimWorkerRequest::encode(*built) == trimWorkerRequest::encode(request));
        const auto completion = joinedTrimWorker(request);
        WorkerTrimBatch expected, actual;
        const auto sync = RS_Modification::trimStoredCache(f.graphic, a, ap, b, bp, both, expected.value, options);
        REQUIRE(sync.result);
        LC_StoredCacheTrimCompletions received{request, *completion, nullptr, nullptr, f.graphic.undoStateEpoch()};
        const auto result = RS_Modification::trimStoredCachePrepared(f.graphic, a, ap, b, bp, both,
            actual.value, received, options);
        REQUIRE(result.result); sameTrimWork(result.cacheWork, sync.cacheWork);
        sameTrimOutputs(actual.value, expected.value);
        CHECK(result.nativeDemotions == sync.nativeDemotions);
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(conic->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
        for (const auto* output : actual.value.entitiesToAdd) {
            CHECK_FALSE(dynamic_cast<const LC_OffsetCurve*>(output));
            CHECK(output->getUserDefVar("ordinary-trim") ==
                (output->rtti() == RS2::EntitySpline ? QString{} : QStringLiteral("retained")));
        }
    }
}

TEST_CASE("Prepared trim consumes original ordinary spline and double native operands in either order",
          "[lc-offsetcurve][trim-action][trim-worker-consumption]") {
    for (bool secondNative : {false, true}) for (bool nativeFirst : {false, true}) for (bool both : {false, true}) {
        CAPTURE(secondNative, nativeFirst, both);
        Drawing<true> f; auto* native = addNative(f.graphic);
        RS_Entity* other;
        if (secondNative) other = addNative(f.graphic, 1);
        else {
            RS_SplineData data{1, false}; data.controlPoints = {{-3, .3}, {3, .3}};
            data.knotslist = {0, 0, 1, 1}; data.weights = {1, 1};
            other = new RS_Spline(&f.graphic, data); f.graphic.addEntity(other);
        }
        RS_Entity& a = nativeFirst ? static_cast<RS_Entity&>(*native) : *other;
        RS_Entity& b = nativeFirst ? *other : static_cast<RS_Entity&>(*native);
        const RS_Vector np = secondNative ? RS_Vector{-2, 0} : RS_Vector{2, 0};
        const RS_Vector op = secondNative ? RS_Vector{3, 0} : RS_Vector{0, .3};
        const RS_Vector ap = nativeFirst ? np : op, bp = nativeFirst ? op : np;
        LC_StoredCacheTrimOptions options;
        const auto limits = RS_Modification::storedCacheTrimRemainingLimits(options.kernel); REQUIRE(limits);
        const auto firstRequest = cacheTrimJob(f.graphic, a, b, ap, *limits);
        const auto first = joinedTrimWorker(firstRequest);
        std::optional<trimWorkerRequest::Request> secondRequest;
        std::unique_ptr<LC_OffsetWorkerCompletion> second;
        if (both) {
            const auto remaining = RS_Modification::storedCacheTrimRemainingLimits(options.kernel, first.get());
            REQUIRE(remaining); secondRequest = cacheTrimJob(f.graphic, b, a, bp, *remaining, 2);
            second = joinedTrimWorker(*secondRequest);
        }
        WorkerTrimBatch expected, actual;
        const auto sync = RS_Modification::trimStoredCache(f.graphic, a, ap, b, bp, both, expected.value, options);
        REQUIRE(sync.result);
        LC_StoredCacheTrimCompletions received{firstRequest, *first,
            secondRequest ? &*secondRequest : nullptr, second.get(), f.graphic.undoStateEpoch()};
        const auto result = RS_Modification::trimStoredCachePrepared(f.graphic, a, ap, b, bp, both,
            actual.value, received, options);
        REQUIRE(result.result); sameTrimWork(result.cacheWork, sync.cacheWork); sameTrimOutputs(actual.value, expected.value);
        CHECK(result.nativeDemotions == sync.nativeDemotions);
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(other->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    }
}

TEST_CASE("Prepared trim refuses mismatched retained requests live inputs picks limits and publication epoch",
          "[lc-offsetcurve][trim-action][trim-worker-consumption]") {
    Drawing<false> f; auto* native = addNative(f.graphic); auto* cutter = line(f.graphic);
    LC_StoredCacheTrimOptions options;
    const auto limits = RS_Modification::storedCacheTrimRemainingLimits(options.kernel); REQUIRE(limits);
    const auto request = cacheTrimJob(f.graphic, *native, *cutter, {2, 0}, *limits);
    const auto completion = joinedTrimWorker(request);
    for (unsigned defect = 0; defect < 7; ++defect) {
        CAPTURE(defect);
        auto retained = request; auto currentOptions = options; RS_Vector pick{2, 0};
        if (defect == 0) ++retained.token.requestId;
        if (defect == 1) std::get<trimWorkerRequest::TrimCache>(retained.operation).cache.weights[0] = 2;
        if (defect == 2) pick.x = 1.9;
        if (defect == 3) --currentOptions.kernel.maxBoxPairs;
        if (defect == 4) { cutter->setStartpoint({-4, .3}); cutter->setEndpoint({4, .3}); }
        if (defect == 6) std::get<trimWorkerRequest::TrimCache>(retained.operation).cache.controlPoints[0].z = -0.0;
        LC_StoredCacheTrimCompletions received{retained, *completion};
        received.publicationUndoEpoch = f.graphic.undoStateEpoch() + (defect == 5 ? 1 : 0);
        auto prefix = std::make_unique<RS_Line>(nullptr, RS_Vector{8, 0}, RS_Vector{9, 0});
        LC_DocumentModificationBatch batch; batch.add(prefix.get()); batch.remove(cutter);
        const auto adds = batch.entitiesToAdd, removes = batch.entitiesToDelete;
        const auto no = RS_Modification::trimStoredCachePrepared(f.graphic, *native, pick, *cutter, {0, .3}, false,
            batch, received, currentOptions);
        CHECK_FALSE(no.result); CHECK(no.cacheRefusal == LC_OffsetCacheTrim::Refusal::InvalidInput);
        CHECK(no.trimmed1 == nullptr); CHECK(no.trimmed2 == nullptr); CHECK(no.nativeDemotions == 0);
        CHECK(batch.entitiesToAdd == adds); CHECK(batch.entitiesToDelete == removes);
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(cutter->isDeleted());
        cutter->setStartpoint({-3, .3}); cutter->setEndpoint({3, .3});
    }
}

TEST_CASE("Prepared trim retains entered refusal work and never submits a missing second query",
          "[lc-offsetcurve][trim-action][trim-worker-consumption]") {
    Drawing<true> f; auto* native = addNative(f.graphic); auto* cutter = line(f.graphic);
    LC_StoredCacheTrimOptions options; options.kernel.maxBoxPairs = 0;
    const auto limits = RS_Modification::storedCacheTrimRemainingLimits(options.kernel); REQUIRE(limits);
    const auto request = cacheTrimJob(f.graphic, *native, *cutter, {2, 0}, *limits);
    const auto completion = joinedTrimWorker(request);
    const auto& prepared = std::get<LC_OffsetCacheTrim::Result>(completion->reply().result);
    REQUIRE(prepared.refusal == LC_OffsetCacheTrim::Refusal::LimitExceeded);
    CHECK_FALSE(RS_Modification::storedCacheTrimRemainingLimits(options.kernel, completion.get()));
    LC_StoredCacheTrimCompletions received{request, *completion};
    received.publicationUndoEpoch = f.graphic.undoStateEpoch();
    WorkerTrimBatch pending;
    const auto no = RS_Modification::trimStoredCachePrepared(f.graphic, *native, {2, 0}, *cutter, {0, .3}, true,
        pending.value, received, options);
    CHECK_FALSE(no.result); CHECK(no.cacheRefusal == prepared.refusal);
    sameTrimWork(no.cacheWork, prepared.work);
    CHECK(pending.value.entitiesToAdd.empty()); CHECK(pending.value.entitiesToDelete.empty());
}

TEST_CASE("Prepared trim preserves final native seals and late cancellation after a real ordinary getter",
          "[lc-offsetcurve][trim-action][trim-worker-consumption]") {
    for (unsigned mode = 0; mode < 3; ++mode) {
        CAPTURE(mode);
        Drawing<true> f; auto* native = addNative(f.graphic);
        auto* ordinary = new AllNativeClosureLine(&f.graphic); f.graphic.addEntity(ordinary);
        LC_StoredCacheTrimOptions options;
        auto cancelled = std::make_shared<std::atomic_bool>(false); options.cancellation.requested = cancelled;
        const auto limits = RS_Modification::storedCacheTrimRemainingLimits(options.kernel); REQUIRE(limits);
        const auto firstRequest = cacheTrimJob(f.graphic, *native, *ordinary, {2, 0}, *limits);
        const auto first = joinedTrimWorker(firstRequest);
        const auto remaining = RS_Modification::storedCacheTrimRemainingLimits(options.kernel, first.get()); REQUIRE(remaining);
        const auto secondRequest = cacheTrimJob(f.graphic, *ordinary, *native, {0, .3}, *remaining, 2);
        const auto second = joinedTrimWorker(secondRequest);
        LC_StoredCacheTrimCompletions received{firstRequest, *first, &secondRequest, second.get(),
            f.graphic.undoStateEpoch()};
        std::size_t lastRead;
        {
            WorkerTrimBatch probe; ordinary->startReads = 0;
            const auto positive = RS_Modification::trimStoredCachePrepared(f.graphic, *native, {2, 0}, *ordinary, {0, .3},
                true, probe.value, received, options);
            REQUIRE(positive.result); lastRead = ordinary->startReads; REQUIRE(lastRead > 0);
        }
        unsigned fired = 0; ordinary->startReads = 0; ordinary->callbackAt = lastRead;
        ordinary->onFinalRead = [&] {
            ++fired;
            if (mode == 1) {
                native->delUserDefVar("trim-foreign");
                native->setUserDefVar("trim-foreign", "late prepared mutation");
            }
            if (mode == 2) cancelled->store(true, std::memory_order_relaxed);
        };
        WorkerTrimBatch pending;
        const auto result = RS_Modification::trimStoredCachePrepared(f.graphic, *native, {2, 0}, *ordinary, {0, .3},
            true, pending.value, received, options);
        REQUIRE(fired == 1); CHECK(ordinary->startReads == lastRead);
        if (mode == 1) REQUIRE(native->getUserDefVar("trim-foreign") == "late prepared mutation");
        CHECK(result.result == (mode == 0));
        if (mode != 0) {
            CHECK(result.trimmed1 == nullptr); CHECK(result.trimmed2 == nullptr);
            CHECK(pending.value.entitiesToAdd.empty()); CHECK(pending.value.entitiesToDelete.empty());
            CHECK(result.cacheRefusal == (mode == 2 ? LC_OffsetCacheTrim::Refusal::Cancelled :
                LC_OffsetCacheTrim::Refusal::UnsupportedLayout));
        }
        CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ordinary->isDeleted()); CHECK_FALSE(canUndo(f.graphic));
    }
}

namespace {
template<bool Both> void ownTrimUndoCycleRetry() {
    CAPTURE(Both);
    Drawing<Both> f; auto* native = addNative(f.graphic); auto* ordinary = line(f.graphic);
    auto* history = line(f.graphic, {10, 10}, {11, 10});
    { LC_UndoSection cycle(&f.graphic, f.view.getViewPort()); cycle.addUndoable(history); }
    REQUIRE(f.graphic.undo()); // A failed empty own cycle must preserve this real redo tail.
    RS_Selection::selectEntitiesList(&f.graphic, f.view.getViewPort(), {native, ordinary}, true);
    auto nativeBefore = LC_OffsetCurveEditSnapshot::capture(*native); REQUIRE(nativeBefore);
    const auto nativeGeometry = trimGeometry(*native);
    const auto nativeDataStamp = RS_Spline::geometryFingerprint(native->getData());
    const auto nativeId = native->getId(), nativeLineage = native->lineage();
    const auto nativePen = native->getPen(false); auto* nativeLayer = native->getLayer(false);
    const auto nativeDefinition = native->definition(); const auto nativeTolerance = native->checkedWorldTolerance();
    std::vector<unsigned long long> childIdsBefore;
    std::vector<std::pair<RS_Vector, RS_Vector>> childEndsBefore;
    for (const auto* child : *native) {
        childIdsBefore.push_back(child->getId()); childEndsBefore.emplace_back(child->getStartpoint(), child->getEndpoint());
    }
    f.view.getViewPort()->setSize(640, 480); f.view.getViewPort()->setOffsetAndFactor(192, 240, 64);
    f.action->init(0);
    const auto send = [&](QEvent::Type type, RS_Vector point) {
        const QPointF pos(f.view.getViewPort()->toGuiX(point.x), f.view.getViewPort()->toGuiY(point.y));
        QMouseEvent event(type, pos, pos, type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
            type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
        if (type == QEvent::MouseButtonPress) f.action->mousePressEvent(&event);
        else if (type == QEvent::MouseButtonRelease) { f.action->mouseReleaseEvent(&event); f.action->waitForTrim(); }
        else { f.action->mouseMoveEvent(&event); f.action->waitForTrim(); }
    };
    const auto click = [&](RS_Vector point) { send(QEvent::MouseButtonPress, point); send(QEvent::MouseButtonRelease, point); };
    click({2, 0}); REQUIRE(f.action->retainedLimit() == native);
    REQUIRE(f.action->getStatus() == CacheTrim<Both>::ChooseTrimEntity);
    f.action->options.maxOutputEntities = 0;
    const auto beforeEpoch = f.graphic.undoStateEpoch();
    std::ostringstream before; before << static_cast<const RS_Undo&>(f.graphic);
    click({0, .3}); // Refused worker output must not open an undo cycle.
    REQUIRE(f.graphic.undoStateEpoch() == beforeEpoch);
    REQUIRE_FALSE(native->isDeleted()); REQUIRE_FALSE(ordinary->isDeleted());
    CHECK(native->isSelected()); CHECK(ordinary->isSelected()); REQUIRE(nativeBefore->matches(*native));
    REQUIRE(f.action->retainedLimit() == native);
    REQUIRE(f.action->getStatus() == CacheTrim<Both>::ChooseTrimEntity);
    REQUIRE(f.context.messages.size() == 1); CHECK(f.context.messages.front().contains("work or output limit"));
    std::ostringstream refused; refused << static_cast<const RS_Undo&>(f.graphic); CHECK(refused.str() == before.str());
    bool undoAvailable = false, redoAvailable = false; f.graphic.collectUndoState(undoAvailable, redoAvailable);
    CHECK_FALSE(undoAvailable); CHECK(redoAvailable);
    f.action->options = {}; // Retry the same retained limit, without init or selecting it again.
    send(QEvent::MouseMove, {0, .3}); REQUIRE_FALSE(f.action->m_preview->isEmpty());
    click({0, .3});
    REQUIRE(ordinary->isDeleted()); CHECK(native->isDeleted() == Both);
    std::vector<RS_Entity*> outputs;
    for (auto* entity : live(f.graphic)) if (entity != native) outputs.push_back(entity);
    REQUIRE_FALSE(outputs.empty());
    for (const auto* entity : outputs) CHECK(entity->rtti() != RS2::EntityOffsetCurve);
    if constexpr (!Both) REQUIRE(nativeBefore->matches(*native));
    f.graphic.collectUndoState(undoAvailable, redoAvailable); CHECK(undoAvailable); CHECK_FALSE(redoAvailable);
    REQUIRE(f.graphic.undo()); CHECK_FALSE(native->isDeleted()); CHECK_FALSE(ordinary->isDeleted());
    for (const auto* entity : outputs) CHECK(entity->isDeleted());
    std::vector<unsigned long long> childIdsAfter;
    for (const auto* child : *native) childIdsAfter.push_back(child->getId());
    INFO("Undo native root ID " << nativeId << " -> " << native->getId());
    REQUIRE_FALSE(childIdsBefore.empty()); REQUIRE_FALSE(childIdsAfter.empty());
    INFO("Undo first display child ID " << childIdsBefore.front() << " -> " << childIdsAfter.front());
    // Undoing deletion calls update(): the root/semantic cache are restored,
    // but a replaced native's temporary display children get fresh identities.
    CHECK((childIdsAfter != childIdsBefore) == Both);
    if constexpr (!Both) REQUIRE(nativeBefore->matches(*native));
    CHECK(native->getId() == nativeId); CHECK(native->lineage() == nativeLineage);
    CHECK(native->getParent() == &f.graphic); CHECK(native->getLayer(false) == nativeLayer); CHECK(native->getPen(false) == nativePen);
    CHECK(trimGeometry(*native) == nativeGeometry);
    CHECK(RS_Spline::geometryFingerprint(native->getData()) == nativeDataStamp); CHECK(native->hasEditableDefinition());
    CHECK(native->checkedWorldTolerance() == nativeTolerance);
    const auto& definitionAfter = native->definition();
    CHECK(definitionAfter.distance == nativeDefinition.distance); CHECK(definitionAfter.towardCentre == nativeDefinition.towardCentre);
    CHECK(definitionAfter.relTolerance == nativeDefinition.relTolerance); CHECK(definitionAfter.cacheReversed == nativeDefinition.cacheReversed);
    const auto& a = definitionAfter.basis; const auto& b = nativeDefinition.basis;
    CHECK(a.center == b.center); CHECK(a.majorP == b.majorP); CHECK(a.ratio == b.ratio);
    CHECK(a.angle1 == b.angle1); CHECK(a.angle2 == b.angle2); CHECK(a.reversed == b.reversed); CHECK(a.isArc == b.isArc);
    CHECK(a.angleDegrees == b.angleDegrees); CHECK(a.startAngleDegrees == b.startAngleDegrees);
    CHECK(a.otherAngleDegrees == b.otherAngleDegrees); CHECK(a.angularLength == b.angularLength);
    CHECK(native->sourceHandle() == 101); CHECK(native->xDictHandle() == 102);
    CHECK(native->reactorHandles() == std::vector<quint32>{103}); CHECK(native->ownerLineage() == 67);
    CHECK(native->getUserDefVar("trim-foreign") == "keep"); CHECK(native->unknownOwnedFlags() == 0x10);
    REQUIRE(native->unknownOwnedTail().size() == 1); CHECK(native->unknownOwnedTail().front().code == 1000);
    CHECK(std::get<QString>(native->unknownOwnedTail().front().value) == QStringLiteral("owned trim tail"));
    REQUIRE(native->count() == childEndsBefore.size()); std::size_t childIndex = 0;
    for (const auto* child : *native) {
        CHECK(child->getParent() == native); CHECK(child->rtti() == RS2::EntityLine);
        CHECK(child->getStartpoint() == childEndsBefore[childIndex].first);
        CHECK(child->getEndpoint() == childEndsBefore[childIndex].second); ++childIndex;
    }
    f.graphic.collectUndoState(undoAvailable, redoAvailable); CHECK_FALSE(undoAvailable); CHECK(redoAvailable);
    REQUIRE(f.graphic.redo()); CHECK(ordinary->isDeleted()); CHECK(native->isDeleted() == Both);
    for (const auto* entity : outputs) CHECK_FALSE(entity->isDeleted());
}
}

TEST_CASE("T actual public mouse Trim admits only its own undo cycle and retries an empty refusal in both modes",
          "[gui][lc-offsetcurve][trim-action][correctness][own-undo-epoch]") {
    ownTrimUndoCycleRetry<false>(); ownTrimUndoCycleRetry<true>();
}

namespace {
class TrimCompletionObserver final : public LC_DocumentModificationListener {
public:
    std::function<void()> completed;
    unsigned calls = 0;
    void graphicModified(const RS_Graphic*, bool) override {}
    void undoStateChanged(const RS_Document*, bool undoAvailable, bool) override {
        if (undoAvailable && calls == 0) {
            ++calls; // Arm once before invoking an observer that itself changes undo state.
            completed();
        }
    }
};
}

TEST_CASE("T Trim completion does not adopt an external undo from the actual end cycle notification",
          "[lc-offsetcurve][trim-action][correctness][completion-boundary]") {
    Drawing<false> f; auto* native = addNative(f.graphic); auto* target = line(f.graphic);
    const auto sourceGeometry = trimGeometry(*native);
    f.action->click({2, 0}); REQUIRE(f.action->retainedLimit() == native);
    const auto admitted = f.graphic.undoStateEpoch();
    TrimCompletionObserver observer; bool undone = false;
    observer.completed = [&] { undone = f.graphic.undo(); };
    f.graphic.setModificationListener(&observer, true);
    const auto detach = qScopeGuard([&] { f.graphic.setModificationListener(&observer, false); });
    f.action->click({0, .3});
    REQUIRE(observer.calls == 1); REQUIRE(undone);
    REQUIRE(f.graphic.undoStateEpoch() == admitted + 2);
    CHECK_FALSE(target->isDeleted()); CHECK_FALSE(native->isDeleted());
    CHECK(trimGeometry(*native) == sourceGeometry); CHECK(native->hasEditableDefinition());
    CHECK(f.action->retainedLimit() == nullptr);
    CHECK(f.action->getStatus() == CacheTrim<false>::ChooseLimitEntity);
    CHECK(f.action->m_preview->isEmpty());
    REQUIRE(f.context.messages.size() == 1); CHECK(f.context.messages.front().contains("stored source"));
    bool undoAvailable = false, redoAvailable = false; f.graphic.collectUndoState(undoAvailable, redoAvailable);
    CHECK_FALSE(undoAvailable); CHECK(redoAvailable);
    REQUIRE(f.graphic.redo()); CHECK(target->isDeleted()); CHECK_FALSE(native->isDeleted());
    CHECK(observer.calls == 1);
}

TEST_CASE("T Trim completion abandons detached closing destroyed and switched views at the end cycle boundary",
          "[lc-offsetcurve][trim-action][correctness][completion-boundary]") {
    for (unsigned teardown = 0; teardown < 4; ++teardown) {
        CAPTURE(teardown);
        Drawing<true> f; f.action.reset();
        auto view = std::make_unique<lc::test::TestGraphicView>(); view->setDocument(&f.graphic);
        QPointer<RS_GraphicView> probe = view.get();
        lc::test::TestGraphicView other; other.setDocument(&f.graphic);
        f.context.setDocumentAndView(&f.graphic, view.get());
        const auto restore = qScopeGuard([&] { f.context.setDocumentAndView(&f.graphic, &f.view); });
        f.action = std::make_unique<CacheTrim<true>>(&f.context);
        auto* native = addNative(f.graphic); auto* target = line(f.graphic);
        f.action->click({2, 0});
        TrimCompletionObserver observer;
        observer.completed = [&] {
            if (teardown == 0) f.context.setDocumentAndView(nullptr, nullptr);
            else if (teardown == 1) view->beginClose();
            else if (teardown == 2) view.reset();
            else f.context.setDocumentAndView(&f.graphic, &other);
            if (view) view->redrawMethod = RS2::RedrawNone;
            f.view.redrawMethod = other.redrawMethod = RS2::RedrawNone;
        };
        f.graphic.setModificationListener(&observer, true);
        const auto detach = qScopeGuard([&] { f.graphic.setModificationListener(&observer, false); });
        f.action->click({0, .3});
        REQUIRE(observer.calls == 1); CHECK(f.action->isFinished());
        CHECK(f.action->retainedLimit() == nullptr); CHECK(f.action->m_preview->isEmpty());
        CHECK(f.action->m_preview->getParent() == nullptr);
        CHECK(f.context.messages.isEmpty()); // No success notice through the obsolete context.
        CHECK(native->isDeleted()); CHECK(target->isDeleted()); // The already committed batch remains intact.
        if (teardown == 2) CHECK(probe.isNull());
        else CHECK(view->redrawMethod == RS2::RedrawNone);
        CHECK(f.view.redrawMethod == RS2::RedrawNone); CHECK(other.redrawMethod == RS2::RedrawNone);
        f.action.reset(); // Base teardown must also avoid the invalidated view.
        if (view) CHECK(view->redrawMethod == RS2::RedrawNone);
        CHECK(other.redrawMethod == RS2::RedrawNone);
    }
}

TEST_CASE("R03 Trim Two seals both natives after final peer mutation or late cancellation",
          "[lc-offsetcurve][trim-action][r03-final-closure]") {
    class FourthReadLine final : public RS_Line {
    public:
        FourthReadLine(LC_OffsetCurve& root, const RS_LineData& data) : RS_Line(&root, data) {}
        RS_Vector getStartpoint() const override {
            const auto point = RS_Line::getStartpoint();
            if (armed && ++reads == 4 && onFourth) {
                auto callback = std::move(onFourth);
                callback();
            }
            return point;
        }
        bool armed = false;
        mutable unsigned reads = 0;
        mutable std::function<void()> onFourth;
    };
    for (bool reverse : {false, true}) for (unsigned mode : {0u, 1u, 2u}) {
        CAPTURE(reverse, mode); // New metadata, cancellation, unchanged positive control.
        Drawing<true> f;
        auto* left = addNative(f.graphic);
        auto* right = addNative(f.graphic, 1);
        auto* first = reverse ? right : left;
        auto* later = reverse ? left : right;
        const RS_Vector firstPick = reverse ? RS_Vector{3, 0} : RS_Vector{-2, 0};
        const RS_Vector laterPick = reverse ? RS_Vector{-2, 0} : RS_Vector{3, 0};
        const auto* old = dynamic_cast<const RS_Line*>(later->entityAt(0));
        REQUIRE(old);
        const auto data = old->getData();
        auto* observed = new FourthReadLine(*later, data);
        observed->setPen(old->getPen(false)); observed->setLayer(old->getLayer(false));
        REQUIRE(later->removeEntity(const_cast<RS_Line*>(old)));
        later->insertEntity(0, observed);
        REQUIRE(first->hasEditableDefinition()); REQUIRE(later->hasEditableDefinition());
        const auto firstGeometry = RS_Spline::geometryFingerprint(first->getData());
        const auto laterGeometry = RS_Spline::geometryFingerprint(later->getData());
        REQUIRE(first->getUserDefVar("R03_TRIM_FINAL_METADATA").isEmpty());
        auto prefix = std::make_unique<RS_Line>(nullptr, RS_Vector{10, 0}, RS_Vector{11, 0});
        auto* oldPrefix = line(f.graphic, {10, 1}, {11, 1});
        LC_DocumentModificationBatch batch;
        batch.entitiesToAdd.append(prefix.get()); batch.entitiesToDelete.append(oldPrefix);
        const auto addPrefix = batch.entitiesToAdd, deletePrefix = batch.entitiesToDelete;
        const auto cleanup = qScopeGuard([&] {
            while (batch.entitiesToAdd.size() > 1) delete batch.entitiesToAdd.takeLast();
        });
        LC_StoredCacheTrimOptions options;
        auto cancelled = std::make_shared<std::atomic_bool>(false);
        options.cancellation.requested = cancelled;
        unsigned fired = 0, readAtMutation = 0;
        observed->onFourth = [&] {
            ++fired; readAtMutation = observed->reads;
            if (mode == 0) first->setUserDefVar("R03_TRIM_FINAL_METADATA", "external final change");
            if (mode == 1) cancelled->store(true, std::memory_order_relaxed);
        };
        observed->armed = true;
        f.graphic.setModified(false);
        const auto result = RS_Modification::trimStoredCache(f.graphic, *first, firstPick,
            *later, laterPick, true, batch, options);
        REQUIRE(fired == 1); REQUIRE(readAtMutation == 4);
        CHECK(observed->getData().startpoint == data.startpoint);
        CHECK(observed->getData().endpoint == data.endpoint);
        CHECK(RS_Spline::geometryFingerprint(first->getData()) == firstGeometry);
        CHECK(RS_Spline::geometryFingerprint(later->getData()) == laterGeometry);
        CHECK_FALSE(first->isDeleted()); CHECK_FALSE(later->isDeleted());
        CHECK_FALSE(oldPrefix->isDeleted()); CHECK_FALSE(f.graphic.isModified());
        CHECK_FALSE(canUndo(f.graphic));
        if (mode < 2) {
            CHECK_FALSE(result.result); REQUIRE(result.cacheRefusal);
            CHECK(result.trimmed1 == nullptr); CHECK(result.trimmed2 == nullptr);
            CHECK(batch.entitiesToAdd == addPrefix); CHECK(batch.entitiesToDelete == deletePrefix);
            if (mode == 0) {
                CHECK(*result.cacheRefusal == LC_OffsetCacheTrim::Refusal::UnsupportedLayout);
                CHECK(first->getUserDefVar("R03_TRIM_FINAL_METADATA") == "external final change");
            } else CHECK(*result.cacheRefusal == LC_OffsetCacheTrim::Refusal::Cancelled);
        } else {
            REQUIRE(result.result); CHECK_FALSE(result.cacheRefusal);
            REQUIRE(result.trimmed1); REQUIRE(result.trimmed2);
            CHECK(result.nativeDemotions == 2);
            REQUIRE(batch.entitiesToAdd.size() == 3); REQUIRE(batch.entitiesToDelete.size() == 3);
            CHECK(batch.entitiesToAdd.front() == prefix.get()); CHECK(batch.entitiesToDelete.front() == oldPrefix);
            plain(*result.trimmed1, *first); plain(*result.trimmed2, *later);
        }
    }
}

TEST_CASE("T Trim Two trims worker-created high-origin typed source families",
          "[gui][lc-offsetcurve][trim-action][typed-source][spline][fit-points][polyline][high-origin][trim-two]") {
    REQUIRE(lc::test::ensureSettings());
    for (const double frame : {1e12, 1e13})
    for (const unsigned sourceFamily : {0u, 1u, 2u})
    for (const bool splineFirst : {false, true}) {
        CAPTURE(frame, sourceFamily, splineFirst);
        const bool largestFrame = frame == 1e13;
        const RS_Vector center{frame, -2 * frame};
        const double scale = largestFrame ? 1e6 : 1e5;
        const double distance = scale * .025;
        Drawing<true> f;
        const double zoom = largestFrame ? 5e-5 : .001;
        f.view.getViewPort()->setSize(640, 480);
        f.view.getViewPort()->setOffsetAndFactor(
            qRound(320.0 - center.x * zoom),
            qRound(240.0 - center.y * zoom), zoom);

        RS_Ellipse seed(nullptr,
            RS_EllipseData{{0, 0}, {2 * scale, 0}, 1, 0, .1, false});
        auto initial = LC_OffsetCurve::create(seed, distance, false);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);

        RS_Entity* source = nullptr;
        const auto sourceKind = sourceFamily == 0
            ? LC_OffsetSourceGeometry::Kind::Spline
            : sourceFamily == 1 ? LC_OffsetSourceGeometry::Kind::SplinePoints
                                : LC_OffsetSourceGeometry::Kind::Polyline;
        if (sourceFamily == 1) {
            LC_SplinePointsData sourceData{false, false};
            sourceData.splinePoints = {
                center, center + RS_Vector{.8 * scale, 1.2 * scale},
                center + RS_Vector{2.2 * scale, .6 * scale},
                center + RS_Vector{3 * scale, 0}};
            sourceData.fitTolerance = 1e-3;
            source = new LC_SplinePoints(&f.graphic, sourceData);
        } else if (sourceFamily == 0) {
            RS_SplineData sourceData{3, false};
            sourceData.controlPoints = {
                center, center + RS_Vector{.8 * scale, 1.2 * scale},
                center + RS_Vector{2.2 * scale, .6 * scale},
                center + RS_Vector{3 * scale, 0}};
            sourceData.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
            sourceData.weights = {1, 1, 1, 1};
            source = new RS_Spline(&f.graphic, sourceData);
        } else {
            auto* polyline = new RS_Polyline(&f.graphic);
            polyline->addVertex(center);
            polyline->addVertex(center + RS_Vector{scale, 0});
            polyline->addVertex(center + RS_Vector{2 * scale, 0});
            polyline->endPolyline();
            source = polyline;
        }
        f.graphic.addEntity(source);
        f.graphic.select(QList<RS_Entity*>{previous}, true);
        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(sourceKind);
        update.use(source);
        lc::test::awaitUpdate(update);
        INFO(f.context.messages.join('\n').toStdString());
        REQUIRE(update.lastReport().updated == 1);
        REQUIRE(previous->isDeleted());

        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == sourceKind);
        CHECK(native->sourceDefinition()->signedDistance == distance);

        double first = 0, last = 0;
        REQUIRE(native->getParameterDomain(first, last));
        const double parameter = first + (last - first) * .31;
        const double step = (last - first) * 1e-3;
        const RS_Vector intersection = native->getPointAt(parameter);
        RS_Vector tangent = native->getPointAt(parameter + step) -
                            native->getPointAt(parameter - step);
        REQUIRE(tangent.magnitude() > RS_TOLERANCE);
        tangent /= tangent.magnitude();
        RS_Vector cutterDirection{-tangent.y, tangent.x};
        cutterDirection = cutterDirection / cutterDirection.magnitude() * scale;
        auto* cutter = new RS_Line(&f.graphic,
            RS_LineData{intersection - cutterDirection,
                        intersection + cutterDirection});
        f.graphic.addEntity(cutter);
        const RS_Vector nativePick = native->getPointAt(first + (last - first) * .1);
        const RS_Vector cutterPick = intersection + cutterDirection * .65;
        const auto prepared = LC_OffsetCacheTrim::prepare(
            native->getData(), *cutter, {nativePick});
        CAPTURE(prepared.refusal, prepared.rootCount, prepared.fragments.size());
        REQUIRE(prepared.refusal == LC_OffsetCacheTrim::Refusal::None);
        REQUIRE(prepared.rootCount >= 1);

        const auto geometryBefore = RS_Spline::geometryFingerprint(native->getData());
        f.action->click(splineFirst ? nativePick : cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
        const auto undoEpoch = f.graphic.undoStateEpoch();
        f.action->hover(splineFirst ? cutterPick : nativePick);
        f.action->waitForTrim();
        REQUIRE_FALSE(f.action->m_preview->isEmpty());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(f.graphic.undoStateEpoch() == undoEpoch);
        f.action->click(splineFirst ? cutterPick : nativePick);
        REQUIRE(native->isDeleted());
        REQUIRE(cutter->isDeleted());

        QTemporaryDir directory;
        REQUIRE(directory.isValid());
        const std::array<RS2::FormatType, 5> formats{{
            RS2::FormatDXFRW14, RS2::FormatDXFRW2000, RS2::FormatDWG,
            RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
        for (std::size_t lane = 0; lane < formats.size(); ++lane) {
            const auto format = formats[lane];
            const bool dwg = format == RS2::FormatDWG ||
                             format == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("high-origin-typed-trim-%1-%2-%3")
                    .arg(sourceFamily).arg(splineFirst ? 1 : 0).arg(lane) +
                (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, format));
            const QByteArray odaOutput = qgetenv("LC_OFFSET_ODA_OUTPUT_DIR");
            if (dwg && !odaOutput.isEmpty()) {
                const std::filesystem::path odaDirectory(odaOutput.constData());
                std::filesystem::create_directories(odaDirectory);
                std::filesystem::copy_file(
                    file, odaDirectory / file.filename(),
                    std::filesystem::copy_options::overwrite_existing);
            }
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, format));
            const auto expected = live(f.graphic);
            const auto actual = live(reopened);
            REQUIRE(actual.size() == expected.size());
            INFO("expected rtti list: " << expected.front()->rtti() << ","
                 << expected.back()->rtti() << "; reopened rtti list: "
                 << actual.front()->rtti() << "," << actual.back()->rtti());
            CHECK(std::none_of(actual.begin(), actual.end(), [](const RS_Entity* entity) {
                return entity->rtti() == RS2::EntityOffsetCurve;
            }));
            std::vector<bool> matched(actual.size(), false);
            for (const auto* expectedEntity : expected) {
                bool found = false;
                for (std::size_t i = 0; i < actual.size(); ++i) {
                    if (matched[i] || !sameTrimGeometry(*expectedEntity, *actual[i]))
                        continue;
                    matched[i] = true;
                    found = true;
                    break;
                }
                CAPTURE(format, expectedEntity->rtti());
                CHECK(found);
            }
        }
        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == sourceKind);
        if (sourceFamily == 1) {
            const auto& restored = std::get<LC_SplinePointsData>(
                native->sourceDefinition()->source.value);
            CHECK(restored.splinePoints.size() == 4);
            CHECK(restored.fitTolerance == 1e-3);
        } else if (sourceFamily == 0) {
            CHECK(RS_Spline::geometryFingerprint(native->getData()) == geometryBefore);
        } else {
            const auto& restored = std::get<LC_OffsetPolylineData>(
                native->sourceDefinition()->source.value);
            CHECK(restored.segments.size() == 2);
        }
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK(cutter->isDeleted());
    }
}

TEST_CASE("T Update from Source preserves ordinary branches for a high-origin polyline corner",
          "[gui][lc-offsetcurve][source-update][typed-source][polyline][high-origin][branches]") {
    REQUIRE(lc::test::ensureSettings());
    const RS_Vector center{1e12, -2e12};
    constexpr double scale = 1e5;
    constexpr double distance = 2500.0;
    for (const bool towardCentre : {false, true})
    for (const bool nativeFirst : {false, true}) {
    CAPTURE(towardCentre, nativeFirst);
    Drawing<true> f;
    RS_Ellipse seed(nullptr,
        RS_EllipseData{{0, 0}, {2 * scale, 0}, 1, 0, .1, false});
    auto initial = LC_OffsetCurve::create(seed, distance, towardCentre);
    REQUIRE(initial);
    auto* previous = initial.entity.release();
    f.graphic.addEntity(previous);

    auto* source = new RS_Polyline(&f.graphic);
    source->addVertex(center);
    source->addVertex(center + RS_Vector{scale, 0});
    source->addVertex(center + RS_Vector{scale, scale});
    source->endPolyline();
    f.graphic.addEntity(source);
    f.graphic.select(QList<RS_Entity*>{previous}, true);

    UpdateSourceProbe update(&f.context);
    update.init(0);
    update.setSourceKind(LC_OffsetSourceGeometry::Kind::Polyline);
    update.use(source);
    lc::test::awaitUpdate(update);
    INFO(f.context.messages.join('\n').toStdString());
    REQUIRE(update.lastReport().updated == 1);
    REQUIRE(previous->isDeleted());
    CHECK_FALSE(source->isDeleted());

    LC_OffsetCurve* native = nullptr;
    for (auto* entity : live(f.graphic)) {
        if (entity->rtti() != RS2::EntityOffsetCurve) continue;
        REQUIRE(native == nullptr);
        native = dynamic_cast<LC_OffsetCurve*>(entity);
    }
    REQUIRE(native);
    REQUIRE(native->hasGeneralizedSourceDefinition());
    REQUIRE(native->sourceDefinition());
    CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Polyline);
    const auto breaks = native->getBreakParameters();
    REQUIRE(breaks.size() >= 3);
    const double start = breaks[0];
    const double end = breaks[1];
    REQUIRE(end > start);
    const double parameter = start + (end - start) * .5;
    const double step = (end - start) * 1e-3;
    const RS_Vector intersection = native->getPointAt(parameter);
    RS_Vector tangent = native->getPointAt(parameter + step) -
                        native->getPointAt(parameter - step);
    REQUIRE(tangent.magnitude() > RS_TOLERANCE);
    tangent /= tangent.magnitude();
    RS_Vector cutterDirection{-tangent.y, tangent.x};
    cutterDirection = cutterDirection / cutterDirection.magnitude() * (distance * 4);
    auto* cutter = new RS_Line(&f.graphic,
        RS_LineData{intersection - cutterDirection, intersection + cutterDirection});
    f.graphic.addEntity(cutter);
    const RS_Vector nativePick = native->getPointAt(start + (end - start) * .1);
    const RS_Vector cutterPick = intersection + cutterDirection * .5;
    const auto prepared = LC_OffsetCacheTrim::prepare(native->getData(), *cutter, {nativePick});
    CAPTURE(prepared.refusal, prepared.rootCount, prepared.fragments.size());
    REQUIRE(prepared.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(prepared.rootCount >= 1);

    f.action->click(nativeFirst ? nativePick : cutterPick);
    REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
    const auto undoEpoch = f.graphic.undoStateEpoch();
    f.action->hover(nativeFirst ? cutterPick : nativePick);
    f.action->waitForTrim();
    REQUIRE_FALSE(f.action->m_preview->isEmpty());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    CHECK(f.graphic.undoStateEpoch() == undoEpoch);
    f.action->click(nativeFirst ? cutterPick : nativePick);
    REQUIRE(native->isDeleted());
    REQUIRE(cutter->isDeleted());
    CHECK_FALSE(source->isDeleted());

    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const std::array<RS2::FormatType, 5> formats{{
        RS2::FormatDXFRW14, RS2::FormatDXFRW2000, RS2::FormatDWG,
        RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
    for (std::size_t lane = 0; lane < formats.size(); ++lane) {
        const auto format = formats[lane];
        const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
        const auto file = std::filesystem::path(directory.filePath(
            QStringLiteral("high-origin-polyline-corner-trim-%1-%2-%3")
                .arg(towardCentre ? 1 : 0).arg(nativeFirst ? 1 : 0).arg(lane) +
            (dwg ? ".dwg" : ".dxf")).toStdString());
        REQUIRE(lc::test::exportDrawing(f.graphic, file, format));
        const QByteArray odaOutput = qgetenv("LC_OFFSET_ODA_OUTPUT_DIR");
        if (dwg && !odaOutput.isEmpty()) {
            const std::filesystem::path odaDirectory(odaOutput.constData());
            std::filesystem::create_directories(odaDirectory);
            std::filesystem::copy_file(
                file, odaDirectory / file.filename(),
                std::filesystem::copy_options::overwrite_existing);
        }
        RS_Graphic reopened;
        REQUIRE(lc::test::importDrawing(reopened, file, format));
        const auto expected = live(f.graphic);
        const auto actual = live(reopened);
        REQUIRE(actual.size() == expected.size());
        CHECK(std::none_of(actual.begin(), actual.end(), [](const RS_Entity* entity) {
            return entity->rtti() == RS2::EntityOffsetCurve;
        }));
        std::vector<bool> matched(actual.size(), false);
        for (const auto* expectedEntity : expected) {
            bool found = false;
            for (std::size_t i = 0; i < actual.size(); ++i) {
                if (matched[i] || !sameTrimGeometry(*expectedEntity, *actual[i])) continue;
                matched[i] = true;
                found = true;
                break;
            }
            CAPTURE(format, expectedEntity->rtti());
            CHECK(found);
        }
    }
    REQUIRE(f.graphic.undo());
    CHECK_FALSE(native->isDeleted());
    CHECK_FALSE(cutter->isDeleted());
    REQUIRE(f.graphic.redo());
    CHECK(native->isDeleted());
    CHECK(cutter->isDeleted());
    }
}

TEST_CASE("T Trim trims a high-origin closed polyline cache on both offset sides",
          "[gui][lc-offsetcurve][trim-action][typed-source][polyline][closed][high-origin]") {
    REQUIRE(lc::test::ensureSettings());
    const RS_Vector center{1e12, -2e12};
    constexpr double radius = 1e5;
    constexpr double distance = 2500.0;
    for (const bool towardCentre : {false, true}) {
        CAPTURE(towardCentre);
        Drawing<false> f;
        constexpr double zoom = .003;
        f.view.getViewPort()->setSize(640, 480);
        f.view.getViewPort()->setOffsetAndFactor(
            qRound(320.0 - center.x * zoom),
            qRound(240.0 - center.y * zoom), zoom);
        RS_Ellipse seed(nullptr,
            RS_EllipseData{{0, 0}, {2 * radius, 0}, 1, 0, .1, false});
        auto initial = LC_OffsetCurve::create(seed, distance, towardCentre);
        REQUIRE(initial);
        auto* previous = initial.entity.release();
        f.graphic.addEntity(previous);

        auto* source = new RS_Polyline(&f.graphic);
        source->addVertex(center + RS_Vector{-radius, -radius});
        source->addVertex(center + RS_Vector{radius, -radius});
        source->addVertex(center + RS_Vector{radius, radius});
        source->addVertex(center + RS_Vector{-radius, radius});
        source->setClosed(true);
        source->endPolyline();
        f.graphic.addEntity(source);
        f.graphic.select(QList<RS_Entity*>{previous}, true);

        UpdateSourceProbe update(&f.context);
        update.init(0);
        update.setSourceKind(LC_OffsetSourceGeometry::Kind::Polyline);
        update.use(source);
        lc::test::awaitUpdate(update);
        INFO(f.context.messages.join('\n').toStdString());
        REQUIRE(update.lastReport().updated == 1);
        REQUIRE(previous->isDeleted());

        LC_OffsetCurve* native = nullptr;
        for (auto* entity : live(f.graphic)) {
            if (entity->rtti() != RS2::EntityOffsetCurve) continue;
            REQUIRE(native == nullptr);
            native = dynamic_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(native);
        REQUIRE(native->hasGeneralizedSourceDefinition());
        REQUIRE(native->sourceDefinition());
        CHECK(native->sourceDefinition()->source.kind() == LC_OffsetSourceGeometry::Kind::Polyline);
        const auto breaks = native->getBreakParameters();
        REQUIRE(breaks.size() >= 2);
        const double first = breaks.front();
        const double last = breaks.back();
        REQUIRE(last > first);
        auto* cutter = new RS_Line(&f.graphic,
            RS_LineData{center + RS_Vector{0, -2 * radius},
                        center + RS_Vector{0, 2 * radius}});
        f.graphic.addEntity(cutter);
        const RS_Vector nativePick = native->getPointAt(first + (last - first) * .1);
        const RS_Vector cutterPick = center + RS_Vector{0, radius};
        const auto prepared = LC_OffsetCacheTrim::prepare(native->getData(), *cutter, {nativePick});
        CAPTURE(prepared.refusal, prepared.rootCount, prepared.fragments.size());
        REQUIRE(prepared.refusal == LC_OffsetCacheTrim::Refusal::None);
        REQUIRE(prepared.rootCount >= 2);
        CHECK_FALSE(source->isDeleted());
        CHECK_FALSE(prepared.fragments.empty());

        RS_SplineData lineCache;
        lineCache.degree = 1;
        lineCache.controlPoints = {cutter->getStartpoint(), cutter->getEndpoint()};
        lineCache.knotslist = {0, 0, 1, 1};
        lineCache.weights = {1, 1};
        const auto reversePrepared = LC_OffsetCacheTrim::prepare(lineCache, *native, {cutterPick});
        CAPTURE(reversePrepared.refusal, reversePrepared.work.failureStage,
                reversePrepared.work.rootRecords, reversePrepared.work.intersectionFailureDepth);
        REQUIRE(reversePrepared.refusal == LC_OffsetCacheTrim::Refusal::None);
        CHECK(reversePrepared.rootCount >= 2);
        CHECK_FALSE(reversePrepared.fragments.empty());

        f.action->click(cutterPick);
        REQUIRE(f.action->getStatus() == CacheTrim<false>::ChooseTrimEntity);
        const auto undoEpoch = f.graphic.undoStateEpoch();
        f.action->hover(nativePick);
        f.action->waitForTrim();
        REQUIRE_FALSE(f.action->m_preview->isEmpty());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK(f.graphic.undoStateEpoch() == undoEpoch);
        f.action->click(nativePick);
        REQUIRE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        CHECK_FALSE(source->isDeleted());
        REQUIRE(f.graphic.undo());
        CHECK_FALSE(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());
        REQUIRE(f.graphic.redo());
        CHECK(native->isDeleted());
        CHECK_FALSE(cutter->isDeleted());

        QTemporaryDir directory;
        REQUIRE(directory.isValid());
        const std::array<RS2::FormatType, 5> formats{{
            RS2::FormatDXFRW14, RS2::FormatDXFRW2000, RS2::FormatDWG,
            RS2::FormatDXFRW2018, RS2::FormatDWG2018}};
        const auto expectedGeometry = live(f.graphic);
        for (std::size_t lane = 0; lane < formats.size(); ++lane) {
            const auto format = formats[lane];
            const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
            const auto file = std::filesystem::path(directory.filePath(
                QStringLiteral("closed-high-origin-trim-%1-%2")
                    .arg(towardCentre ? 1 : 0).arg(lane) +
                (dwg ? ".dwg" : ".dxf")).toStdString());
            REQUIRE(lc::test::exportDrawing(f.graphic, file, format));
            const QByteArray odaOutput = qgetenv("LC_OFFSET_ODA_OUTPUT_DIR");
            if (dwg && !odaOutput.isEmpty()) {
                const std::filesystem::path odaDirectory(odaOutput.constData());
                std::filesystem::create_directories(odaDirectory);
                std::filesystem::copy_file(
                    file, odaDirectory / file.filename(),
                    std::filesystem::copy_options::overwrite_existing);
            }
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, file, format));
            const auto reopenedGeometry = live(reopened);
            CHECK(std::none_of(reopenedGeometry.begin(), reopenedGeometry.end(), [](const RS_Entity* entity) {
                return entity->rtti() == RS2::EntityOffsetCurve;
            }));
            CAPTURE(towardCentre, format);
            CHECK(sameTrimGeometrySet(expectedGeometry, reopenedGeometry));
        }
    }
}

TEST_CASE("T Trim Two trims a high-origin closed polyline cache across its cutter seam",
          "[gui][lc-offsetcurve][trim-action][typed-source][polyline][closed][high-origin][trim-two]") {
    REQUIRE(lc::test::ensureSettings());
    const double baseRadius = 1e5;
    constexpr double distance = 2500.0;
    const auto profiles = GENERATE(Catch::Generators::table<double, double, bool, int, double, double>({
        {0.0, 1.0, false, 0, 1.0, 1.0}, {.37, 1.0, false, 0, 1.0, 1.0},
        {0.0, .7, false, 0, 1.0, 1.0}, {.37, .7, false, 0, 1.0, 1.0},
        {0.0, 1.0, true, 0, 1.0, 1.0}, {0.0, 1.0, false, 1, 1.0, 1.0},
        {0.0, 1.0, false, 2, 1.0, 1.0}, {0.0, 1.0, false, 3, 1.0, 1.0},
        {0.0, 1.0, false, 4, 1.0, 1.0}, {.37, 1.0, false, 4, 1.0, 10.0},
        {0.0, 1.0, false, 5, 1.0, 1.0},
        {.37, .7, false, 1, 1.7, 1.0}, {.37, .7, false, 3, .6, 1.0},
        {.37, .7, false, 5, 1.7, 1.0},
        {.37, .7, false, 1, 1.0, .001},
        {.37, .7, false, 1, 1.0, 10.0},
        {.9, .7, false, 2, .6, 10.0},
        {.37, .7, false, 3, 1.0, 10.0},
        {.37, 1.0, false, 7, 1.0, 10.0},
        {.37, 1.0, false, 0, 1.0, 10.0},
        {0.0, 1.0, false, 5, 1.7, 10.0},
        {0.0, 1.0, false, 6, 1.0, 1.0},
        {.37, 1.0, false, 6, 1.0, 10.0},
        {0.0, 1.0, false, 7, 1.0, 1.0},
        {.37, 1.0, false, 7, 1.0, 1.0},
        {.9, .7, false, 7, 1.0, 1.0},
        {.37, 1.0, false, 7, 1.0, 10.0},
        {.37, 1.0, false, 7, .6, 1.0},
        {.37, 1.0, false, 7, 1.7, 1.0},
        {.37, 1.0, false, 8, 1.0, 1.0},
        {.37, 1.0, false, 8, 1.0, 10.0},
        {.9, .7, false, 8, 1.0, 1.0},
        {.9, .7, false, 8, .6, 1.0},
        {.9, .7, false, 8, 1.7, 1.0},
        {.9, .7, false, 8, 1.7, 10.0},
        {.37, 1.0, false, 8, .6, 1.0},
        {.37, 1.0, false, 8, 1.7, 1.0}}));
    const double rotation = std::get<0>(profiles);
    const double verticalRatio = std::get<1>(profiles);
    const bool bulged = std::get<2>(profiles);
    const int topology = std::get<3>(profiles);
    const double scale = std::get<4>(profiles);
    const double originScale = std::get<5>(profiles);
    const double radius = baseRadius * scale;
    const RS_Vector center{1e12 * originScale, -2e12 * originScale};
    const double cosine = std::cos(rotation), sine = std::sin(rotation);
    const auto world = [&](double x, double y) {
        return center + RS_Vector{cosine * x - sine * y, sine * x + cosine * y};
    };
    CAPTURE(rotation, verticalRatio, bulged, topology, scale, originScale);
    for (const bool towardCentre : {false, true}) {
        for (const bool nativeFirst : {false, true}) {
            CAPTURE(towardCentre, nativeFirst);
            Drawing<true> f;
            constexpr double zoom = .003;
            f.view.getViewPort()->setSize(640, 480);
            f.view.getViewPort()->setOffsetAndFactor(
                qRound(320.0 - center.x * zoom),
                qRound(240.0 - center.y * zoom), zoom);
            RS_Ellipse seed(nullptr,
                RS_EllipseData{{0, 0}, {2 * radius, 0}, 1, 0, .1, false});
            auto initial = LC_OffsetCurve::create(seed, distance, towardCentre);
            REQUIRE(initial);
            auto* previous = initial.entity.release();
            f.graphic.addEntity(previous);

            auto* source = new RS_Polyline(&f.graphic);
            const double bulge = bulged ? std::tan(3.14159265358979323846 / 8.0) : 0.0;
            if (topology == 1 || topology == 4) {
                source->addVertex(world(-radius, -radius * verticalRatio));
                source->addVertex(world(radius, -radius * verticalRatio));
                source->addVertex(world(radius, 0));
                source->addVertex(world(0, 0));
                source->addVertex(world(0, radius * verticalRatio));
                source->addVertex(world(-radius, radius * verticalRatio));
            } else if (topology == 2) {
                source->addVertex(world(radius, -radius * verticalRatio));
                source->addVertex(world(-radius, -radius * verticalRatio));
                source->addVertex(world(-radius, 0));
                source->addVertex(world(0, 0));
                source->addVertex(world(0, radius * verticalRatio));
                source->addVertex(world(radius, radius * verticalRatio));
            } else if (topology == 3) {
                source->addVertex(world(-radius, -radius * verticalRatio));
                source->addVertex(world(radius, -radius * verticalRatio));
                source->addVertex(world(radius, .25 * radius * verticalRatio));
                source->addVertex(world(.3 * radius, .25 * radius * verticalRatio));
                source->addVertex(world(.3 * radius, radius * verticalRatio));
                source->addVertex(world(-radius, radius * verticalRatio));
            } else if (topology == 5) {
                source->addVertex(world(-radius, -radius * verticalRatio));
                source->addVertex(world(radius, -radius * verticalRatio));
                source->addVertex(world(radius, radius * verticalRatio));
                source->addVertex(world(radius / 3, radius * verticalRatio));
                source->addVertex(world(radius / 3, -radius / 3 * verticalRatio));
                source->addVertex(world(-radius / 3, -radius / 3 * verticalRatio));
                source->addVertex(world(-radius / 3, radius * verticalRatio));
                source->addVertex(world(-radius, radius * verticalRatio));
            } else if (topology == 6) {
                source->addVertex(world(-radius, -radius * verticalRatio));
                source->addVertex(world(radius, -radius * verticalRatio));
                source->addVertex(world(radius, radius * verticalRatio));
                source->addVertex(world(.6 * radius, radius * verticalRatio));
                source->addVertex(world(.6 * radius, -.25 * radius * verticalRatio));
                source->addVertex(world(.3 * radius, -.25 * radius * verticalRatio));
                source->addVertex(world(.3 * radius, radius * verticalRatio));
                source->addVertex(world(-.3 * radius, radius * verticalRatio));
                source->addVertex(world(-.3 * radius, -.25 * radius * verticalRatio));
                source->addVertex(world(-.6 * radius, -.25 * radius * verticalRatio));
                source->addVertex(world(-.6 * radius, radius * verticalRatio));
                source->addVertex(world(-radius, radius * verticalRatio));
            } else if (topology == 7 || topology == 8) {
                const double mirror = topology == 8 ? -1.0 : 1.0;
                source->addVertex(world(mirror * -radius, -radius * verticalRatio));
                source->addVertex(world(mirror * radius, -radius * verticalRatio));
                source->addVertex(world(mirror * radius, radius * verticalRatio));
                source->addVertex(world(mirror * .7 * radius, radius * verticalRatio));
                source->addVertex(world(mirror * .7 * radius, -.5 * radius * verticalRatio));
                source->addVertex(world(mirror * .2 * radius, -.5 * radius * verticalRatio));
                source->addVertex(world(mirror * .2 * radius, radius * verticalRatio));
                source->addVertex(world(mirror * -.2 * radius, radius * verticalRatio));
                source->addVertex(world(mirror * -.2 * radius, -.5 * radius * verticalRatio));
                source->addVertex(world(mirror * -.7 * radius, -.5 * radius * verticalRatio));
                source->addVertex(world(mirror * -.7 * radius, radius * verticalRatio));
                source->addVertex(world(mirror * -radius, radius * verticalRatio));
            } else {
                source->addVertex(world(-radius, -radius * verticalRatio), bulge);
                source->addVertex(world(radius, -radius * verticalRatio), bulge);
                source->addVertex(world(radius, radius * verticalRatio), bulge);
                source->addVertex(world(-radius, radius * verticalRatio), bulge);
            }
            source->setClosed(true);
            source->endPolyline();
            f.graphic.addEntity(source);
            f.graphic.select(QList<RS_Entity*>{previous}, true);

            UpdateSourceProbe update(&f.context);
            update.init(0);
            update.setSourceKind(LC_OffsetSourceGeometry::Kind::Polyline);
            update.use(source);
            lc::test::awaitUpdate(update);
            REQUIRE(update.lastReport().updated == 1);
            REQUIRE(previous->isDeleted());

            LC_OffsetCurve* native = nullptr;
            for (auto* entity : live(f.graphic)) {
                if (entity->rtti() != RS2::EntityOffsetCurve) continue;
                REQUIRE(native == nullptr);
                native = dynamic_cast<LC_OffsetCurve*>(entity);
            }
            REQUIRE(native);
            const auto breaks = native->getBreakParameters();
            REQUIRE(breaks.size() >= 2);
            const RS_Vector nativePick = native->getPointAt(
                breaks.front() + (breaks.back() - breaks.front()) * .1);
            const bool horizontalCutter = topology >= 4;
            const double cutterX = topology == 1 ? -.5 * radius : topology == 2 ? .5 * radius :
                                   topology == 3 ? .65 * radius : 0;
            const double cutterY = topology == 4 ? .5 * radius * verticalRatio :
                                   topology == 7 || topology == 8
                                       ? -.25 * radius * verticalRatio : 0;
            auto* cutter = new RS_Line(&f.graphic, horizontalCutter
                ? RS_LineData{world(-2 * radius, cutterY), world(2 * radius, cutterY)}
                : RS_LineData{world(cutterX, -2 * radius), world(cutterX, 2 * radius)});
            f.graphic.addEntity(cutter);
            const RS_Vector cutterPick = horizontalCutter
                ? world(topology == 5 ? -.65 * radius : topology == 6 ? -.8 * radius :
                        topology == 7 ? -.85 * radius : topology == 8 ? .85 * radius :
                        -.5 * radius, cutterY)
                : world(cutterX, radius);
            RS_SplineData cutterCache;
            cutterCache.degree = 1;
            cutterCache.controlPoints = {cutter->getStartpoint(), cutter->getEndpoint()};
            cutterCache.knotslist = {0, 0, 1, 1};
            cutterCache.weights = {1, 1};
            const auto nativeFirstPrepared = LC_OffsetCacheTrim::prepare(
                native->getData(), *cutter, {nativePick});
            const auto lineFirstPrepared = LC_OffsetCacheTrim::prepare(
                cutterCache, *native, {cutterPick});
            CAPTURE(nativeFirstPrepared.refusal, nativeFirstPrepared.work.failureStage,
                    nativeFirstPrepared.rootCount, lineFirstPrepared.refusal,
                    lineFirstPrepared.work.failureStage, lineFirstPrepared.rootCount);
            REQUIRE(nativeFirstPrepared.refusal == LC_OffsetCacheTrim::Refusal::None);
            CHECK(nativeFirstPrepared.rootCount >= 2);
            if (topology == 5) { CHECK(nativeFirstPrepared.rootCount == 4); }
            if (topology == 6 || topology == 7 || topology == 8) {
                const bool nativeTargetHasSixRoots = towardCentre == (topology == 8);
                CHECK(nativeFirstPrepared.rootCount == (nativeTargetHasSixRoots ? 6 : 2));
            }
            REQUIRE(lineFirstPrepared.refusal == LC_OffsetCacheTrim::Refusal::None);
            CHECK(lineFirstPrepared.rootCount >= 2);
            if (topology == 5) { CHECK(lineFirstPrepared.rootCount == 4); }
            if (topology == 6 || topology == 7 || topology == 8) {
                CHECK(lineFirstPrepared.rootCount == 6);
            }

            const auto firstPick = nativeFirst ? nativePick : cutterPick;
            const auto secondPick = nativeFirst ? cutterPick : nativePick;
            f.action->click(firstPick);
            REQUIRE(f.action->getStatus() == CacheTrim<true>::ChooseTrimEntity);
            const auto undoEpoch = f.graphic.undoStateEpoch();
            f.action->hover(secondPick);
            const auto preview = trimPreview(*f.action->m_preview);
            REQUIRE_FALSE(preview.empty());
            CHECK_FALSE(native->isDeleted());
            CHECK_FALSE(cutter->isDeleted());
            CHECK(f.graphic.undoStateEpoch() == undoEpoch);

            f.action->click(secondPick);
            REQUIRE(native->isDeleted());
            REQUIRE(cutter->isDeleted());
            CHECK_FALSE(source->isDeleted());
            const auto output = live(f.graphic);
            CHECK(std::none_of(output.begin(), output.end(), [](const RS_Entity* entity) {
                return entity->rtti() == RS2::EntityOffsetCurve;
            }));
            std::vector<std::string> committedPreviewEntities;
            for (const auto* entity : output) {
                if (entity != source && (entity->rtti() == RS2::EntitySpline ||
                    entity->rtti() == RS2::EntityArc || entity->rtti() == RS2::EntityEllipse)) {
                    committedPreviewEntities.push_back(trimGeometry(*entity));
                }
            }
            CHECK(committedPreviewEntities == preview);
            REQUIRE(f.graphic.undo());
            CHECK_FALSE(native->isDeleted());
            CHECK_FALSE(cutter->isDeleted());
            CHECK_FALSE(source->isDeleted());
            REQUIRE(f.graphic.redo());
            CHECK(native->isDeleted());
            CHECK(cutter->isDeleted());
            CHECK_FALSE(source->isDeleted());

            QTemporaryDir directory;
            REQUIRE(directory.isValid());
            const std::array<RS2::FormatType, 5> formats{{RS2::FormatDXFRW14,
                RS2::FormatDXFRW2000, RS2::FormatDWG, RS2::FormatDXFRW2018,
                RS2::FormatDWG2018}};
            const auto expectedGeometry = live(f.graphic);
            for (std::size_t lane = 0; lane < formats.size(); ++lane) {
                const auto format = formats[lane];
                const bool dwg = format == RS2::FormatDWG || format == RS2::FormatDWG2018;
                const std::filesystem::path file(directory.filePath(
                    QStringLiteral("closed-high-origin-trim-two-%1-%2-%3-%4-%5-%6-%7-%8-%9")
                        .arg(towardCentre ? 1 : 0).arg(nativeFirst ? 1 : 0)
                        .arg(qRound(rotation * 100)).arg(qRound(verticalRatio * 100))
                        .arg(bulged ? 1 : 0).arg(topology).arg(qRound(scale * 100))
                        .arg(qRound(originScale * 1000)).arg(lane) +
                    (dwg ? ".dwg" : ".dxf")).toStdString());
                REQUIRE(lc::test::exportDrawing(f.graphic, file, format));
                const QByteArray odaOutput = qgetenv("LC_OFFSET_ODA_OUTPUT_DIR");
                if (dwg && !odaOutput.isEmpty()) {
                    const std::filesystem::path odaDirectory(odaOutput.constData());
                    std::filesystem::create_directories(odaDirectory);
                    std::filesystem::copy_file(file, odaDirectory / file.filename(),
                        std::filesystem::copy_options::overwrite_existing);
                }
                RS_Graphic reopened;
                REQUIRE(lc::test::importDrawing(reopened, file, format));
                const auto reopenedGeometry = live(reopened);
                CHECK(sameTrimGeometrySet(expectedGeometry, reopenedGeometry));
            }
        }
    }
}
