/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "../lc_offsetworkerclient.h"
#include "../lc_offsetworkersourcecache.h"
#include "../lc_offsetworkertolerance.h"
#include "../lc_offsetworkeredit.h"
#include "lc_offsetcurvexdata.h"
#include "lc_filtertestsupport.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_spline.h"
#include "lc_parabola.h"
#include "rs_pen.h"

#include <catch2/catch_test_macros.hpp>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QEventLoop>
#include <QFileInfo>
#include <QThread>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#include "lc_offsetbatchpreparation.h"

// The declared friend changes deadlines only, never process paths or completion values.
class LC_OffsetWorkerClientTestAccess {
public:
    static void lowerDeadlines(LC_OffsetWorkerClient& client) {
        client.m_startupLimit = 1;
        client.m_previewLimit = 1;
        client.m_commitLimit = 1;
    }
    static void restoreDeadlines(LC_OffsetWorkerClient& client) {
        client.m_startupLimit = LC_OffsetWorkerClient::Deadlines::startup;
        client.m_previewLimit = LC_OffsetWorkerClient::Deadlines::preview;
        client.m_commitLimit = LC_OffsetWorkerClient::Deadlines::commit;
    }
};

namespace {
using Client = LC_OffsetWorkerClient;
using Completion = LC_OffsetWorkerCompletion;
namespace Req = LC_OffsetWorkerRequest;
namespace Reply = LC_OffsetWorkerResult;
namespace Geo = LC_EllipseOffsetGeometry;

static_assert(!std::is_default_constructible_v<Completion>);
static_assert(!std::is_copy_constructible_v<Completion>);
static_assert(!std::is_copy_assignable_v<Completion>);
static_assert(std::is_nothrow_move_constructible_v<Completion>);
static_assert(std::is_nothrow_move_assignable_v<Completion>);
static_assert(!std::is_constructible_v<Completion, Req::Request&&, Reply::Reply&&>);

constexpr int SuiteTimeoutMs = 24000;
constexpr int WaitTimeoutMs = 3500;
constexpr const char* Tags = "[lc-offsetcurve][worker-client]";

class Runtime {
public:
    Runtime() {
        // The target's existing settings-isolation listener owns QSettings setup.
        if (!QCoreApplication::instance()) application = std::make_unique<QCoreApplication>(argc, argv);
        elapsed.start();
    }

    void check() const {
        INFO("client suite watchdog elapsed=" << elapsed.elapsed() << "ms");
        REQUIRE(elapsed.elapsed() < SuiteTimeoutMs);
    }

    void helperAvailable() const {
        check();
        const QFileInfo helper(Client::helperPath());
        REQUIRE(helper.isAbsolute());
        REQUIRE(helper.isFile());
        REQUIRE(helper.isExecutable());
        CHECK(helper.absolutePath() == QDir(QCoreApplication::applicationDirPath()).absolutePath());
#ifdef Q_OS_WIN
        CHECK(helper.fileName() == QStringLiteral("LibreCADOffsetWorker.exe"));
#else
        CHECK(helper.fileName() == QStringLiteral("LibreCADOffsetWorker"));
#endif
    }

    template<class Predicate> void until(Predicate ready) const {
        QElapsedTimer waiting;
        waiting.start();
        while (!ready()) {
            check();
            INFO("client callback watchdog elapsed=" << waiting.elapsed() << "ms");
            REQUIRE(waiting.elapsed() < WaitTimeoutMs);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            if (!ready()) QThread::msleep(1);
        }
        check();
    }

    void settle() const {
        QElapsedTimer waiting;
        waiting.start();
        while (waiting.elapsed() < 15) {
            check();
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QThread::msleep(1);
        }
    }

private:
    int argc = 1;
    char name[27] = "offset-worker-client-tests";
    char* argv[2] = {name, nullptr};
    std::unique_ptr<QCoreApplication> application;
    QElapsedTimer elapsed;
};

Runtime& runtime() {
    static Runtime instance;
    return instance;
}

class OwnedClient {
public:
    explicit OwnedClient(Runtime& rt) {
        rt.helperAvailable();
        client = std::make_unique<Client>();
    }
    ~OwnedClient() {
        // Client destruction disconnects callbacks and performs bounded kill/reap,
        // quarantining its notifier if the OS cannot reap within its fixed deadline.
        client.reset();
    }
    Client& get() const { return *client; }
private:
    std::unique_ptr<Client> client;
};

struct Receipts {
    std::array<std::optional<Client::Outcome>, 4> outcomes;
    std::array<unsigned, 4> calls{};

    Client::Callback into(std::size_t slot) {
        return [this, slot](Client::Outcome outcome) {
            // Assertions stay outside Qt's event dispatch and signal stacks.
            ++calls[slot];
            outcomes[slot].emplace(std::move(outcome));
        };
    }
    const Client::Outcome& at(std::size_t slot) const {
        REQUIRE(calls[slot] == 1);
        REQUIRE(outcomes[slot]);
        return *outcomes[slot];
    }
};

Req::Token token(qint64 id) { return {id, 11, 13, 17}; }

LC_EllipseOffsetDefinition definition() {
    return {{{0, 0}, {1, 0}, 1, 0, .001, false}, 1, false, false, 1e-4};
}

Req::Request generate(qint64 id) { return {token(id), Req::Generate{definition(), {}, {}}}; }

Req::Request validate(qint64 id) {
    RS_SplineData cache{1, false};
    cache.controlPoints = {{2, 0}, {2 * std::cos(.001), 2 * std::sin(.001)}};
    cache.knotslist = {0, 0, 1, 1};
    cache.weights = {1, 1};
    return {token(id), Req::ValidateCache{std::move(cache), definition(), .01, {}}};
}

Req::Request heavyValidate(qint64 id) {
    constexpr std::size_t spans = 4096;
    constexpr double step = .001 / double(spans);
    const auto point = [](double angle) { return RS_Vector{101 * std::cos(angle), 101 * std::sin(angle)}; };
    const auto tangent = [](double angle) { return RS_Vector{-101 * std::sin(angle), 101 * std::cos(angle)}; };
    RS_SplineData cache{3, false};
    cache.controlPoints.reserve(3 * spans + 1);
    cache.knotslist.reserve(3 * spans + 5);
    cache.controlPoints.push_back(point(0));
    cache.knotslist.assign(4, 0.0);
    for (std::size_t i = 0; i < spans; ++i) {
        const double from = double(i) * step, to = double(i + 1) * step;
        cache.controlPoints.push_back(point(from) + tangent(from) * (step / 3));
        cache.controlPoints.push_back(point(to) - tangent(to) * (step / 3));
        cache.controlPoints.push_back(point(to));
        cache.knotslist.insert(cache.knotslist.end(), i + 1 == spans ? 4 : 3, double(i + 1));
    }
    cache.weights.assign(cache.controlPoints.size(), 1.0);
    auto def = definition();
    def.basis.majorP = {100, 0};
    return {token(id), Req::ValidateCache{std::move(cache), def, .01, {}}};
}

void boundedOutcome(const Client::Outcome& outcome) {
    CHECK(outcome.reaped);
    CHECK(outcome.elapsedMilliseconds >= 0);
    CHECK(outcome.diagnostics.size() <= 65536);
    CHECK(outcome.transportWork.payloadBytes <= LC_OffsetWorkerWire::Limits{}.frameBytes);
}

const Completion& completed(const Client::Outcome& outcome, qint64 id, std::size_t operation) {
    INFO("client status=" << int(outcome.status) << " diagnostics=" << outcome.diagnostics.toStdString());
    boundedOutcome(outcome);
    REQUIRE(outcome.status == Client::Status::Completed);
    REQUIRE(outcome.completed);
    CHECK(outcome.exactGeometryWorkKnown);
    CHECK(outcome.transportWork.payloadBytes > 0);
    CHECK(outcome.transportWork.items > 0);
    const auto& completion = *outcome.completed;
    const auto& request = completion.request();
    const auto& reply = completion.reply();
    CHECK(request.token.requestId == id);
    CHECK(request.token.generation == 11);
    CHECK(request.token.sourceRevision == 13);
    CHECK(request.token.undoEpoch == 17);
    CHECK(reply.token.requestId == request.token.requestId);
    CHECK(reply.token.generation == request.token.generation);
    CHECK(reply.token.sourceRevision == request.token.sourceRevision);
    CHECK(reply.token.undoEpoch == request.token.undoEpoch);
    CHECK(reply.operation == operation);
    CHECK(request.operation.index() == operation);
    CHECK(reply.failure == Reply::Failure::None);
    return completion;
}

void stopped(const Client::Outcome& outcome, Client::Status status, bool knownWork = false) {
    boundedOutcome(outcome);
    CHECK(outcome.status == status);
    CHECK_FALSE(outcome.completed);
    CHECK(outcome.exactGeometryWorkKnown == knownWork);
}

bool exact(double a, double b) { return std::memcmp(&a, &b, sizeof(double)) == 0; }
bool exact(const RS_Vector& a, const RS_Vector& b) {
    return exact(a.x, b.x) && exact(a.y, b.y) && exact(a.z, b.z) && a.valid == b.valid;
}
template<class T> void exactValues(const std::vector<T>& a, const std::vector<T>& b) {
    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) CHECK(exact(a[i], b[i]));
}
void exactCache(const RS_SplineData& a, const RS_SplineData& b) {
    CHECK(a.degree == b.degree);
    CHECK(a.type == b.type);
    CHECK(a.savedOpenType == b.savedOpenType);
    CHECK(a.m_closedFlag == b.m_closedFlag);
    CHECK(a.m_periodicFlag == b.m_periodicFlag);
    exactValues(a.controlPoints, b.controlPoints);
    exactValues(a.knotslist, b.knotslist);
    exactValues(a.weights, b.weights);
    exactValues(a.fitPoints, b.fitPoints);
    exactValues(a.savedOpenKnots, b.savedOpenKnots);
}

void nativeOutput(const Completion& completion, const RS_Entity& attributes) {
    REQUIRE(std::holds_alternative<Reply::Native>(completion.reply().result));
    const auto& value = std::get<Reply::Native>(completion.reply().result);
    REQUIRE(value.refusal == LC_OffsetCurveRefusal::None);
    REQUIRE(value.curve);
    auto curve = completion.native(attributes);
    REQUIRE(curve);
    CHECK(curve->hasEditableDefinition());
    CHECK(curve->getParent() == attributes.getParent());
    CHECK(curve->getLayer() == attributes.getLayer());
    CHECK(curve->getPen(false) == attributes.getPen(false));
    CHECK(curve->getFlags() == attributes.getFlags());
    CHECK(curve->getUserDefVar("client-local") == attributes.getUserDefVar("client-local"));
    exactCache(curve->getData(), value.curve->cache);
    exactValues(curve->cachedDefinitionGrips(), value.curve->grips);
    CHECK(exact(curve->getMiddlePoint(), value.curve->midpoint));
    // This is an independent C run on the actual materialized cache, not codec proof.
    const auto checked = Geo::checkCache(curve->getData(), curve->definition(), curve->checkedWorldTolerance());
    REQUIRE(checked);
    REQUIRE(checked.analysis);
    CHECK(curve->cacheFormsClosedChain() == checked.analysis->closed());
    CHECK(exact(curve->checkedWorldTolerance(), value.curve->worldTolerance));
    REQUIRE_FALSE(curve->cachedDefinitionGrips().empty());
    CHECK(Geo::isFinite2D(curve->getMiddlePoint()));
    REQUIRE(curve->setOwnedExtensions(0x10, {{1000, QStringLiteral("old geometry tail")}}));
    const auto identity = curve->getId();
    const auto applied = completion.apply(*curve);
    REQUIRE(applied); CHECK(applied.changed);
    CHECK(applied.engineRuns == value.engineRuns);
    CHECK(applied.engineStatus == value.engineStatus); CHECK(applied.checkReason == value.checkReason);
    CHECK(applied.checkWork.distancePairs == value.checkWork.distancePairs);
    CHECK(applied.throughWork.distancePairs == value.throughWork.distancePairs);
    CHECK(curve->getId() == identity); CHECK(curve->unknownOwnedFlags() == 0);
    CHECK(curve->unknownOwnedTail().empty()); exactCache(curve->getData(), value.curve->cache);
    // A late invalidation cannot be cured by applying a result intended for an
    // earlier input; the caller's final seal is an additional transaction guard.
    curve->getData().controlPoints.front().x += 1;
    const auto stale = RS_Spline::geometryFingerprint(curve->getData());
    const auto rejected = completion.apply(*curve);
    CHECK(rejected.refusal == LC_OffsetCurveRefusal::Inconsistent); CHECK_FALSE(rejected.changed);
    CHECK(rejected.engineRuns == value.engineRuns);
    CHECK(rejected.checkWork.distancePairs == value.checkWork.distancePairs);
    CHECK(RS_Spline::geometryFingerprint(curve->getData()) == stale);
}

struct LowerDeadlines {
    Client& client;
    explicit LowerDeadlines(Client& value) : client(value) { LC_OffsetWorkerClientTestAccess::lowerDeadlines(client); }
    ~LowerDeadlines() { LC_OffsetWorkerClientTestAccess::restoreDeadlines(client); }
};

class CancelBeforeDelivery final : public QObject {
public:
    explicit CancelBeforeDelivery(Client& value) : client(value) { client.installEventFilter(this); }
    bool intercepted = false;
protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (!intercepted && watched == &client && event->type() == QEvent::MetaCall && !client.busy()) {
            intercepted = true;
            client.cancel();
        }
        return false;
    }
private:
    Client& client;
};

class DestroyBeforeDelivery final : public QObject {
public:
    DestroyBeforeDelivery(Client& value, std::unique_ptr<QObject>& valueOwner) : client(value), owner(valueOwner) {
        client.installEventFilter(this);
    }
    bool intercepted = false;
protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (!intercepted && watched == &client && event->type() == QEvent::MetaCall && !client.busy()) {
            intercepted = true;
            owner.reset();
            return true; // The watched receiver was deleted; do not dispatch its functor.
        }
        return false;
    }
private:
    Client& client;
    std::unique_ptr<QObject>& owner;
};
}

TEST_CASE("Worker client deadlines use a monotonic inclusive boundary", Tags) {
    for (const auto limit : {Client::Deadlines::startup, Client::Deadlines::preview, Client::Deadlines::commit,
                            Client::Deadlines::terminate, Client::Deadlines::reap}) {
        CAPTURE(limit);
        REQUIRE(limit > 0);
        CHECK_FALSE(Client::Deadlines::expired(0, limit));
        CHECK_FALSE(Client::Deadlines::expired(limit - 1, limit));
        CHECK(Client::Deadlines::expired(limit, limit));
        CHECK(Client::Deadlines::expired(limit + 1, limit));
        CHECK(Client::Deadlines::expired(std::numeric_limits<qint64>::max(), limit));
    }
    CHECK(Client::Deadlines::expired(0, 0));
}

TEST_CASE("Worker client real completions materialize exact caches with caller-owned attributes", Tags) {
    auto& rt = runtime();
    Receipts receipts, rejected;
    RS_EntityContainer parent;
    RS_Layer layer(QStringLiteral("client-local-layer"));
    RS_Line attributes(&parent, {17, 19}, {23, 29});
    attributes.setLayer(&layer);
    attributes.setPen(RS_Pen{RS_Color{17, 31, 47}, RS2::Width11, RS2::DashLine});
    attributes.setFlag(RS2::FlagTransparent);
    attributes.setVisible(false);
    attributes.setUserDefVar("client-local", "not transported to child");
    OwnedClient owned(rt);
    auto& client = owned.get();
    for (std::size_t slot = 0; slot < 2; ++slot) {
        const auto request = slot == 0 ? generate(10) : validate(11);
        REQUIRE(client.submit(request, Client::Mode::Commit, receipts.into(slot)));
        CHECK(client.busy());
        CHECK(receipts.calls[slot] == 0);
        // Attribute choice remains in the parent and is read only at materialization.
        attributes.setPen(RS_Pen{RS_Color{53, 71, int(89 + slot)}, RS2::Width11, RS2::DashLine});
        rt.until([&] { return receipts.calls[slot] != 0; });
        CHECK_FALSE(client.busy());
        const auto& completion = completed(receipts.at(slot), request.token.requestId, slot);
        nativeOutput(completion, attributes);
        if (slot == 1) exactCache(std::get<Req::ValidateCache>(request.operation).cache,
                                  std::get<Reply::Native>(completion.reply().result).curve->cache);
    }
    for (const qint64 id : {qint64{-1}, qint64{0}, qint64{10}, qint64{11}})
        CHECK_FALSE(client.submit(generate(id), Client::Mode::Preview, rejected.into(0)));
    CHECK_FALSE(client.submit(generate(12), Client::Mode::Preview, Client::Callback{}));
    CHECK_FALSE(client.submit(generate(12), static_cast<Client::Mode>(2), rejected.into(0)));
    REQUIRE(client.submit(generate(12), Client::Mode::Preview, receipts.into(2)));
    rt.until([&] { return receipts.calls[2] != 0; });
    completed(receipts.at(2), 12, 0);
    rt.settle();
    CHECK(rejected.calls[0] == 0);
    CHECK(receipts.calls[0] == 1);
    CHECK(receipts.calls[1] == 1);
    CHECK(receipts.calls[2] == 1);
}

TEST_CASE("Worker client reports exact known work for a real geometric refusal", Tags) {
    auto& rt = runtime();
    Receipts receipts;
    OwnedClient owned(rt);
    auto request = validate(1);
    std::get<Req::ValidateCache>(request.operation).cache.degree = 2;
    REQUIRE(owned.get().submit(request, Client::Mode::Preview, receipts.into(0)));
    rt.until([&] { return receipts.calls[0] != 0; });
    const auto& completion = completed(receipts.at(0), 1, 1);
    REQUIRE(std::holds_alternative<Reply::Native>(completion.reply().result));
    const auto& value = std::get<Reply::Native>(completion.reply().result);
    CHECK(value.refusal == LC_OffsetCurveRefusal::CheckFailed);
    REQUIRE(value.checkReason);
    CHECK(*value.checkReason == LC_OffsetCheckResult::Reason::Layout);
    CHECK(value.engineRuns == 0);
    CHECK_FALSE(value.engineStatus);
    CHECK_FALSE(value.curve);
    const RS_Line attributes(nullptr, {0, 0}, {1, 0});
    CHECK_FALSE(completion.native(attributes));
}

TEST_CASE("Worker ordinary completions materialize exact plain output with fresh identities", Tags) {
    auto& rt = runtime(); OwnedClient owned{rt};
    RS_EntityContainer owner; RS_Layer layer{QStringLiteral("ordinary local")};
    RS_Line attributes{&owner, RS_LineData{{0, 0}, {1, 1}}};
    attributes.setLayer(&layer); attributes.setSourceHandle(91); attributes.setXDictHandle(92);
    attributes.setReactorHandles({93}); attributes.setOwnerLineage(94);
    attributes.setUserDefVar(QStringLiteral("ordinary-local"), QStringLiteral("kept"));
    const LC_OffsetCurveXData::XData foreign{
        std::make_shared<DRW_Variant>(1001, std::string("ORDINARY_FOREIGN")),
        std::make_shared<DRW_Variant>(1000, std::string("keep exactly"))};
    const auto stale = LC_OffsetCurveXData::forExport(foreign, LC_OffsetCurveXData::Definition{definition(), 0, {}});
    REQUIRE(stale); attributes.setDrwExtData(*stale);
    for (int mode = 0; mode < 5; ++mode) {
        auto input = generate(mode + 1); auto& op = std::get<Req::Generate>(input.operation);
        if (mode == 0) { op.definition.basis.center = {1e7, 0}; op.limits.outputBudget.maxDeepEntities = 31; }
        if (mode == 1 || mode == 4) op.limits.checkLimits.maxVertices = 1;
        if (mode == 2 || mode == 3) {
            const auto pi = std::acos(-1.0);
            op.definition.basis = mode == 2 ? RS_EllipseData{{0, 0}, {5, 0}, 1} :
                RS_EllipseData{{0, 0}, {100, 0}, .05, pi / 12, 7 * pi / 4, false};
            op.definition.distance = mode == 2 ? 5 : 4;
            op.definition.towardCentre = true; op.definition.relTolerance = mode == 2 ? 1e-4 : 1e-5;
        }
        if (mode == 4) op.throughPoint = RS_Vector{50, 50};
        Receipts receipts; REQUIRE(owned.get().submit(input, Client::Mode::Commit, receipts.into(0)));
        rt.until([&] { return receipts.outcomes[0].has_value(); });
        const auto& receipt = completed(receipts.at(0), mode + 1, 0);
        const auto& result = std::get<Reply::Native>(receipt.reply().result);
        CHECK(result.engineRuns == 1); CHECK_FALSE(result.changed); CHECK_FALSE(receipt.native(attributes));
        REQUIRE(result.ordinaryStatus == Reply::OrdinaryStatus::Complete);
        auto output = receipt.ordinary(attributes);
        if (mode == 4) { CHECK_FALSE(output); continue; }
        REQUIRE(output); REQUIRE(output->size() == result.ordinary.size());
        CHECK(output->empty() == (mode == 2));
        for (std::size_t i = 0; i < output->size(); ++i) {
            const auto& entity = *(*output)[i];
            CHECK(entity.rtti() != RS2::EntityOffsetCurve); CHECK(entity.getParent() == &owner);
            CHECK(entity.getLayer(false) == &layer); CHECK(entity.getPen(false) == attributes.getPen(false));
            CHECK(entity.getUserDefVar("ordinary-local") == QStringLiteral("kept"));
            const auto tags = LC_OffsetCurveXData::parseAndStrip(entity.getDrwExtData());
            CHECK(tags.knownCandidates == 0); REQUIRE(tags.remaining);
            REQUIRE(tags.remaining->size() == foreign.size() + 1);
            CHECK((*tags.remaining)[0] == foreign[0]); CHECK((*tags.remaining)[1] == foreign[1]);
            CHECK((*tags.remaining)[2]->code() == 1001);
            CHECK(*(*tags.remaining)[2]->content.s == LC_OffsetCurveXData::AppId);
            CHECK(entity.sourceHandle() == 0); CHECK(entity.xDictHandle() == 0);
            CHECK(entity.reactorHandles().empty()); CHECK(entity.ownerLineage() == 0);
            CHECK(entity.lineage() != attributes.lineage()); CHECK(entity.getId() != attributes.getId());
            const auto& raw = result.ordinary[i];
            if (const auto* line = std::get_if<Req::Segment>(&raw)) {
                REQUIRE(entity.rtti() == RS2::EntityLine);
                CHECK(exact(entity.getStartpoint(), line->start)); CHECK(exact(entity.getEndpoint(), line->end));
            } else {
                REQUIRE(entity.rtti() == RS2::EntitySpline);
                exactCache(static_cast<const RS_Spline&>(entity).getData(), std::get<RS_SplineData>(raw));
            }
        }
        CHECK(owner.count() == 0); CHECK(attributes.sourceHandle() == 91);
        CHECK(LC_OffsetCurveXData::parseAndStrip(attributes.getDrwExtData()).knownCandidates == 1);
    }
}

TEST_CASE("Worker client joins typed source generation for every modeled source family", Tags) {
    auto& rt = runtime();
    OwnedClient owned{rt};
    RS_EntityContainer owner;
    RS_Layer layer{QStringLiteral("typed source")};
    RS_Line attributes{&owner, RS_LineData{{0, 0}, {1, 0}}};
    attributes.setLayer(&layer);

    RS_SplineData spline{2, false};
    spline.controlPoints = {{0, 0}, {1, 2}, {3, 0}};
    spline.knotslist = {0, 0, 0, 1, 1, 1};
    spline.weights = {1, .7, 1};
    LC_SplinePointsData points{false, false};
    points.useControlPoints = true;
    points.controlPoints = {{0, 0}, {1, 1}, {2, 0}};
    LC_SplinePointsData fitPoints{false, false};
    fitPoints.splinePoints = {{0, 0}, {1, 2}, {2, 0}};
    RS_SplineData degreeOne{1, false};
    degreeOne.controlPoints = {{0, 0}, {1, 1}, {2, 0}};
    degreeOne.knotslist = {0, 0, .5, 1, 1};
    RS_SplineData cubic{3, false};
    cubic.controlPoints = {{0, 0}, {1, 2}, {2, 2}, {3, 0}};
    cubic.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    RS_Spline periodic{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30},
                                   RS_Vector{20, 50}, RS_Vector{-15, 25}})
        periodic.addControlPoint(point);
    periodic.setClosed(true);
    auto wrapped = periodic.getData();
    wrapped.weights.clear();
    RS_Spline periodicLinear{nullptr, RS_SplineData{1, false}};
    for (const RS_Vector& point :
         {RS_Vector{0, 0}, RS_Vector{30, -15}, RS_Vector{60, 0}, RS_Vector{55, 35}, RS_Vector{20, 50}, RS_Vector{-15, 25}})
        periodicLinear.addControlPoint(point);
    periodicLinear.setKnotVector({0, 0, .5, 1.5, 2.5, 4, 5, 5});
    periodicLinear.setClosed(true);
    auto wrappedLinear = periodicLinear.getData();
    wrappedLinear.weights.clear();
    const double pi = std::acos(-1.0);
    LC_OffsetPolylineData closedArcPolyline{{2, 0},
                                            {2, 0},
                                            true,
                                            {RS_ArcData{{0, 0}, 2, 0, pi / 2, false}, RS_ArcData{{0, 0}, 2, pi / 2, pi, false},
                                             RS_ArcData{{0, 0}, 2, pi, 3 * pi / 2, false},
                                             RS_ArcData{{0, 0}, 2, 3 * pi / 2, 2 * pi, false}}};
    const auto clampedRationalSpline = [](const std::size_t degree) {
        RS_SplineData data;
        data.degree = degree;
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.savedOpenType = data.type;
        for (std::size_t i = 0; i <= degree; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree);
            data.controlPoints.emplace_back(4.0 * t, 2.0 * t * (1.0 - t));
            data.weights.push_back(0.75 + 0.5 * t);
        }
        data.knotslist.insert(data.knotslist.end(), degree + 1, 0.0);
        data.knotslist.insert(data.knotslist.end(), degree + 1, 1.0);
        return data;
    };
    RS_Spline tangentConstrainedSpline{nullptr, RS_SplineData{3, false}};
    REQUIRE(tangentConstrainedSpline.setFitPointsWithTangents(
        {{0, 0}, {2, 3}, {6, 4}, {9, 0}}, {2, 1}, {1, -3}));
    RS_Polyline mixedBulgePolyline;
    mixedBulgePolyline.addVertex({0, 0}, .2);
    mixedBulgePolyline.addVertex({8, 0}, -.2);
    mixedBulgePolyline.addVertex({8, 6});
    mixedBulgePolyline.addVertex({0, 6});
    mixedBulgePolyline.setClosed(true);
    mixedBulgePolyline.endPolyline();
    const auto mixedBulgeSource = LC_OffsetSourceGeometry::capture(mixedBulgePolyline);
    REQUIRE(mixedBulgeSource);
    REQUIRE(mixedBulgeSource->kind() == LC_OffsetSourceGeometry::Kind::Polyline);
    const std::vector<LC_OffsetSourceGeometry> sources{
        LC_OffsetSourceGeometry{RS_EllipseData{{0, 0}, {5, 0}, .5, 0, 1.5, false}},
        LC_OffsetSourceGeometry{spline},
        LC_OffsetSourceGeometry{points},
        LC_OffsetSourceGeometry{fitPoints},
        LC_OffsetSourceGeometry{degreeOne},
        LC_OffsetSourceGeometry{cubic},
        LC_OffsetSourceGeometry{LC_ParabolaData{{RS_Vector{0, 1}, RS_Vector{1, 0}, RS_Vector{2, 1}}}},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {2, 0}, .5, -.5, .5}},
        LC_OffsetSourceGeometry{RS_LineData{{0, 0}, {2, 0}}},
        LC_OffsetSourceGeometry{RS_ArcData{{0, 0}, 2, .25, 1.5, false}},
        LC_OffsetSourceGeometry{RS_CircleData{{0, 0}, 2}},
        LC_OffsetSourceGeometry{
            LC_OffsetPolylineData{{0, 0}, {2, 2}, false, {RS_ArcData{{1, 0}, 1, M_PI, 0, false}, RS_LineData{{2, 0}, {2, 2}}}}},
        LC_OffsetSourceGeometry{LC_OffsetPolylineData{
            {0, 0},
            {0, 0},
            true,
            {RS_LineData{{0, 0}, {2, 0}}, RS_LineData{{2, 0}, {2, 2}}, RS_LineData{{2, 2}, {0, 2}}, RS_LineData{{0, 2}, {0, 0}}}}},
        LC_OffsetSourceGeometry{wrapped},
        LC_OffsetSourceGeometry{closedArcPolyline},
        LC_OffsetSourceGeometry{wrappedLinear},
        LC_OffsetSourceGeometry{clampedRationalSpline(4)},
        LC_OffsetSourceGeometry{clampedRationalSpline(5)},
        LC_OffsetSourceGeometry{clampedRationalSpline(6)},
        LC_OffsetSourceGeometry{LC_HyperbolaData{{0, 0}, {2, 0}, .5, -.5, .5, true}},
        LC_OffsetSourceGeometry{tangentConstrainedSpline.getData()},
        *mixedBulgeSource};

    std::vector<std::pair<LC_OffsetSourceDefinition, RS_SplineData>> cacheCandidates;
    std::size_t multiBranchCacheCount = 0;
    for (std::size_t i = 0; i < sources.size(); ++i) {
        CAPTURE(i);
        const qint64 requestId = static_cast<qint64>(i + 1);
        const double sourceDistance = i + 1 == sources.size() ? -.25 : .25;
        const Req::Request input{token(requestId), Req::GenerateSource{LC_OffsetSourceDefinition{sources[i], sourceDistance, 1e-4, false}, {}}};
        Receipts receipts;
        REQUIRE(owned.get().submit(input, Client::Mode::Commit, receipts.into(0)));
        rt.until([&] { return receipts.calls[0] != 0; });
        const auto& completion = completed(receipts.at(0), requestId, 7);
        const auto& result = std::get<Reply::Native>(completion.reply().result);
        CAPTURE(i, result.ordinary.size());
        REQUIRE(result.engineStatus);
        CHECK(*result.engineStatus == LC_CurveOffsetStatus::Ok);
        REQUIRE(result.ordinaryStatus == Reply::OrdinaryStatus::Complete);
        if (i == 13) {
            REQUIRE(result.ordinary.size() == 1);
            const auto* cache = std::get_if<RS_SplineData>(&result.ordinary.front());
            REQUIRE(cache);
            auto directSource = sources[i].makeEntity();
            REQUIRE(directSource);
            const LC_OffsetSourceDefinition directDefinition{sources[i], .25, 1e-4, false};
            const auto directOptionsValue = LC_OffsetWorkerTolerance::directOptions(directDefinition, *directSource);
            REQUIRE(directOptionsValue);
            const auto directOptions = *directOptionsValue;
            const auto directGeometry =
                LC_CurveOffset::buildDirectBranches(*directSource, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, .25),
                                                    directOptions, LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(directGeometry.status == LC_CurveOffsetStatus::Ok);
            const auto directMaterialized =
                LC_CurveOffset::materializeBranches(*directSource, directGeometry, directOptions, LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(directMaterialized.status == LC_CurveOffsetStatus::Ok);
            REQUIRE(directMaterialized.entities.size() == 1);
            const auto* directSpline = dynamic_cast<const RS_Spline*>(directMaterialized.entities.front().get());
            REQUIRE(directSpline);
            const auto& directCache = directSpline->getData();
            INFO("direct controls=" << directCache.controlPoints.size() << " knots=" << directCache.knotslist.size()
                                    << " weights=" << directCache.weights.size() << " degree=" << directCache.degree
                                    << " type=" << static_cast<int>(directCache.type));
            CHECK(directSpline->validate());
            CHECK(directCache.knotslist.size() == directCache.controlPoints.size() + static_cast<std::size_t>(directCache.degree) + 1);
            CHECK(directCache.weights.size() == directCache.controlPoints.size());
            exactCache(directCache, *cache);
        }
        CHECK_FALSE(completion.native(attributes));
        auto entities = completion.ordinary(attributes);
        REQUIRE(entities);
        REQUIRE_FALSE(entities->empty());
        for (const auto& entity : *entities) {
            CHECK(entity->getParent() == &owner);
            CHECK(entity->getLayer(false) == &layer);
            CHECK((entity->rtti() == RS2::EntityLine || entity->rtti() == RS2::EntitySpline));
        }
        auto editable = completion.nativeSource(attributes);
        RS_SplineData canonicalCache;
        const bool cacheRepresentable = LC_OffsetWorkerSourceCache::combine(result.ordinary, canonicalCache);
        if (i + 3 >= sources.size()) REQUIRE(cacheRepresentable);
        CHECK(static_cast<bool>(editable) == cacheRepresentable);
        if (cacheRepresentable) {
            if (result.ordinary.size() > 1)
                ++multiBranchCacheCount;
            REQUIRE(editable);
            REQUIRE(editable->hasGeneralizedSourceDefinition());
            CHECK_FALSE(editable->hasEditableDefinition());
            exactCache(editable->getData(), canonicalCache);
            REQUIRE(editable->sourceDefinition());
            cacheCandidates.emplace_back(*editable->sourceDefinition(), editable->getData());
            auto toleranceSource = editable->sourceDefinition()->source.makeEntity();
            REQUIRE(toleranceSource);
            const auto expectedOptions = LC_OffsetWorkerTolerance::directOptions(*editable->sourceDefinition(), *toleranceSource);
            REQUIRE(expectedOptions);
            CHECK(editable->checkedWorldTolerance() == expectedOptions->tolerance.requestedGeometry);
            CHECK(Req::Values::encodeSourceDefinition(*editable->sourceDefinition()) ==
                  Req::Values::encodeSourceDefinition(std::get<Req::GenerateSource>(input.operation).definition));
            std::unique_ptr<RS_Entity> cloned{editable->clone()};
            const auto* clonedOffset = dynamic_cast<const LC_OffsetCurve*>(cloned.get());
            REQUIRE(clonedOffset);
            CHECK(clonedOffset->hasGeneralizedSourceDefinition());
            CHECK(Req::Values::encodeSourceDefinition(*clonedOffset->sourceDefinition()) ==
                  Req::Values::encodeSourceDefinition(*editable->sourceDefinition()));
            const auto cacheStamp = RS_Spline::geometryFingerprint(editable->getData());
            const auto sourceBeforeMove = Req::Values::encodeSourceGeometry(editable->sourceDefinition()->source);
            CHECK(editable->setDistance(.5).refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
            editable->move({2, 3});
            REQUIRE(editable->hasGeneralizedSourceDefinition());
            CHECK(editable->sourceDefinition()->signedDistance == sourceDistance);
            const bool movedExactly = RS_Spline::geometryFingerprint(editable->getData()) != cacheStamp;
            auto movedSource = std::get<Req::GenerateSource>(input.operation).definition.source.makeEntity();
            REQUIRE(movedSource);
            movedSource->move({2, 3});
            const auto expectedSource = LC_OffsetSourceGeometry::capture(*movedSource);
            REQUIRE(expectedSource);
            if (movedExactly) {
                CHECK(Req::Values::encodeSourceGeometry(editable->sourceDefinition()->source) ==
                      Req::Values::encodeSourceGeometry(*expectedSource));
            }
            else {
                CHECK(RS_Spline::geometryFingerprint(editable->getData()) == cacheStamp);
                CHECK(Req::Values::encodeSourceGeometry(editable->sourceDefinition()->source) == sourceBeforeMove);
            }
            const auto previousDirection = editable->sourceDefinition()->cacheReversed;
            if (result.ordinary.size() > 1) {
                Geo::WorkBudget layoutBudget;
                const auto layout = Geo::inspectCacheLayout(editable->getData(), layoutBudget);
                CHECK(layout.status == Geo::CacheLayoutStatus::Ok);
                INFO("composite layout=" << static_cast<int>(layout.status) << " controls=" << editable->getData().controlPoints.size()
                                         << " knots=" << editable->getData().knotslist.size()
                                         << " midpoint valid=" << Geo::isFinite2D(editable->getMiddlePoint()));
                auto reversedData = editable->getData();
                const double knotSum = reversedData.knotslist.front() + reversedData.knotslist.back();
                for (double& knot : reversedData.knotslist)
                    knot = knotSum - knot;
                std::reverse(reversedData.knotslist.begin(), reversedData.knotslist.end());
                std::reverse(reversedData.controlPoints.begin(), reversedData.controlPoints.end());
                std::reverse(reversedData.weights.begin(), reversedData.weights.end());
                RS_Spline reversedCheck{nullptr, reversedData};
                CHECK(reversedCheck.validate());
            }
            editable->revertDirection();
            REQUIRE(editable->hasGeneralizedSourceDefinition());
            CHECK(editable->sourceDefinition()->cacheReversed == !previousDirection);
        }
        else
            CHECK_FALSE(editable);
    }
    CHECK(multiBranchCacheCount > 0);

    // A dyadic linear spline and offset have exactly representable geometry,
    // so this case proves typed movement succeeds when the cache certificate
    // can be transported exactly rather than only testing refusal behavior.
    RS_SplineData linear;
    linear.degree = 1;
    linear.controlPoints = {{0, 0}, {2, 0}};
    linear.knotslist = {0, 0, 1, 1};
    linear.weights = {1, 1};
    const LC_OffsetSourceDefinition linearDefinition{LC_OffsetSourceGeometry{linear}, .25, 0, false};
    REQUIRE_FALSE(cacheCandidates.empty());
    qint64 nextRequestId = static_cast<qint64>(sources.size() + 1);
    std::size_t replacementsApplied = 0;
    std::vector<LC_OffsetSourceGeometry::Kind> replacementRefusals;
    for (const auto& [sourceDefinition, cache] : cacheCandidates) {
        const Req::Request validationInput{token(nextRequestId), Req::ValidateSourceCache{cache, sourceDefinition, {}}};
        Receipts validationReceipts;
        REQUIRE(owned.get().submit(validationInput, Client::Mode::Commit, validationReceipts.into(0)));
        rt.until([&] { return validationReceipts.calls[0] != 0; });
        auto& validationCompletion = completed(validationReceipts.at(0), nextRequestId, 8);
        const auto& validation = std::get<Reply::Native>(validationCompletion.reply().result);
        CHECK(validation.refusal == LC_OffsetCurveRefusal::None);
        CHECK(validation.changed);
        auto validatedNative = validationCompletion.nativeValidatedSource(attributes);
        REQUIRE(validatedNative);
        REQUIRE(validatedNative->sourceDefinition());
        auto toleranceSource = validatedNative->sourceDefinition()->source.makeEntity();
        REQUIRE(toleranceSource);
        const auto expectedOptions = LC_OffsetWorkerTolerance::directOptions(*validatedNative->sourceDefinition(), *toleranceSource);
        REQUIRE(expectedOptions);
        CHECK(validatedNative->checkedWorldTolerance() == expectedOptions->tolerance.requestedGeometry);
        CHECK(Req::Values::encodeSourceDefinition(*validatedNative->sourceDefinition()) ==
              Req::Values::encodeSourceDefinition(sourceDefinition));
        exactCache(validatedNative->getData(), cache);

        auto replacementDefinition = *validatedNative->sourceDefinition();
        auto replacementSource = replacementDefinition.source.makeEntity();
        REQUIRE(replacementSource);
        replacementSource->move({2, 3});
        const auto replacementGeometry = LC_OffsetSourceGeometry::capture(*replacementSource);
        REQUIRE(replacementGeometry);
        replacementDefinition.source = *replacementGeometry;
        const auto previousFingerprint = RS_Spline::geometryFingerprint(validatedNative->getData());
        const auto previousDefinition = Req::Values::encodeSourceDefinition(*validatedNative->sourceDefinition());
        auto edit = LC_OffsetWorkerEdit::prepareSourceDefinition(*validatedNative, replacementDefinition, {});
        REQUIRE(edit.generateSource());
        const qint64 replacementId = nextRequestId + 1;
        Receipts editReceipts;
        REQUIRE(owned.get().submit({token(replacementId), *edit.generateSource()}, Client::Mode::Commit,
            editReceipts.into(0)));
        rt.until([&] { return editReceipts.calls[0] != 0; });
        const auto& editCompletion = completed(editReceipts.at(0), replacementId, 7);
        const auto editOutcome = editCompletion.apply(*validatedNative);
        if (editOutcome) {
            ++replacementsApplied;
            CHECK(editOutcome.changed);
            REQUIRE(validatedNative->sourceDefinition());
            CHECK(Req::Values::encodeSourceDefinition(*validatedNative->sourceDefinition()) ==
                Req::Values::encodeSourceDefinition(replacementDefinition));
            CHECK(RS_Spline::geometryFingerprint(validatedNative->getData()) != previousFingerprint);
        } else {
            CAPTURE(sourceDefinition.source.kind(), editOutcome.refusal);
            CHECK(editOutcome.refusal == LC_OffsetCurveRefusal::NotCacheRepresentable);
            replacementRefusals.push_back(sourceDefinition.source.kind());
            CHECK(Req::Values::encodeSourceDefinition(*validatedNative->sourceDefinition()) == previousDefinition);
            CHECK(RS_Spline::geometryFingerprint(validatedNative->getData()) == previousFingerprint);
        }
        nextRequestId += 2;
    }
    CHECK(replacementsApplied + replacementRefusals.size() == cacheCandidates.size());
    CHECK(replacementsApplied == cacheCandidates.size());
    CHECK(replacementRefusals.empty());

    REQUIRE(lc::test::ensureSettings());
    lc::test::QuietLog quiet;
    lc::test::OutputDirectory persistenceDirectory("offset-worker-produced-legacy-roundtrip");
    RS_Graphic persistenceGraphic;
    persistenceGraphic.initForNewDocument();
    auto* persistenceLayer = persistenceGraphic.findLayer("0");
    REQUIRE(persistenceLayer);
    struct PersistedGeometry {
        std::size_t sourceFixture;
        RS2::EntityType type;
        RS_Vector start;
        RS_Vector end;
        RS_Vector minimum;
        RS_Vector maximum;
        RS_VectorSolutions references;
        double length = 0.0;
        std::optional<RS_SplineData> splineData;
    };
    std::vector<PersistedGeometry> persistedGeometry;
    const auto persistenceGeometry = [](const RS_Entity& entity, const std::size_t sourceFixture) {
        PersistedGeometry result{sourceFixture, entity.rtti(), entity.getStartpoint(), entity.getEndpoint(),
                                 entity.getMin(), entity.getMax(), entity.getRefPoints(), entity.getLength(), std::nullopt};
        if (const auto* spline = dynamic_cast<const RS_Spline*>(&entity)) result.splineData = spline->getData();
        return result;
    };

    for (std::size_t i = 0; i < sources.size(); ++i) {
        CAPTURE(i, sources[i].kind());
        auto entity = sources[i].makeEntity();
        REQUIRE(entity);
        RS_OffsetData offset;
        offset.coord = {3, 2};
        offset.sideFallback = {10, 10};
        offset.distance = .25;
        offset.number = 1;
        offset.multipleCopies = true;
        offset.keepOriginals = true;
        LC_OffsetBatchPreparation prepared{offset, {entity.get()}, false, {}};
        LC_DocumentModificationBatch batch;
        REQUIRE_FALSE(prepared.advance(batch));
        const qint64 requestId = nextRequestId++;
        const auto workerRequest = prepared.request(token(requestId));
        REQUIRE(workerRequest);
        REQUIRE(std::holds_alternative<Req::Produce>(workerRequest->operation));
        const auto& produce = std::get<Req::Produce>(workerRequest->operation);
        if (sources[i].kind() == LC_OffsetSourceGeometry::Kind::Ellipse) {
            CHECK(produce.source.index() == 0);
        } else if (sources[i].kind() == LC_OffsetSourceGeometry::Kind::Spline) {
            CHECK(produce.source.index() == 1);
        } else {
            REQUIRE(produce.source.index() == 2);
            const auto* typed = std::get_if<LC_OffsetSourceGeometry>(&produce.source);
            REQUIRE(typed);
            CHECK(typed->kind() == sources[i].kind());
        }
        CHECK(Req::admitted(*workerRequest));
        const auto sourceBefore = Req::Values::encodeSourceGeometry(sources[i]);
        REQUIRE(sourceBefore);
        Receipts producedReceipts;
        REQUIRE(owned.get().submit(*workerRequest, Client::Mode::Commit,
            producedReceipts.into(0)));
        rt.until([&] { return producedReceipts.calls[0] != 0; });
        const auto& produced = completed(producedReceipts.at(0), requestId, 5);
        const auto& producedReply = produced.reply();
        const auto* producedValue = std::get_if<Reply::Produced>(&producedReply.result);
        REQUIRE(producedValue);
        CAPTURE(i, producedValue->outputStatus, producedValue->source.status,
            producedValue->source.copiesMade, producedValue->source.engineStatus,
            producedValue->source.detail, producedValue->outputs.size());
        INFO("worker diagnostics: " << producedReceipts.at(0).diagnostics.toStdString());
        if (sources[i].kind() == LC_OffsetSourceGeometry::Kind::Line) {
            REQUIRE(producedValue->outputs.size() == 1);
            CHECK(std::holds_alternative<Req::Segment>(producedValue->outputs.front()));
        } else if (sources[i].kind() == LC_OffsetSourceGeometry::Kind::Arc ||
                   sources[i].kind() == LC_OffsetSourceGeometry::Kind::Circle ||
                   sources[i].kind() == LC_OffsetSourceGeometry::Kind::Polyline) {
            REQUIRE(producedValue->outputs.size() == 1);
            const auto* output = std::get_if<LC_OffsetSourceGeometry>(&producedValue->outputs.front());
            REQUIRE(output);
            CHECK(output->kind() == sources[i].kind());
        }
        REQUIRE(prepared.accept(produced));
        REQUIRE(prepared.advance(batch));
        REQUIRE(prepared.outcome().sources.size() == 1);
        CHECK(prepared.outcome().sources.front().source == entity.get());
        CHECK(prepared.outcome().sources.front().complete());
        CHECK(Req::Values::encodeSourceGeometry(sources[i]) == sourceBefore);
        REQUIRE_FALSE(batch.entitiesToAdd.empty());
        RS2::EntityType expectedOutputType = RS2::EntitySpline;
        switch (sources[i].kind()) {
        case LC_OffsetSourceGeometry::Kind::Line: expectedOutputType = RS2::EntityLine; break;
        case LC_OffsetSourceGeometry::Kind::Arc: expectedOutputType = RS2::EntityArc; break;
        case LC_OffsetSourceGeometry::Kind::Circle: expectedOutputType = RS2::EntityCircle; break;
        case LC_OffsetSourceGeometry::Kind::Polyline: expectedOutputType = RS2::EntityPolyline; break;
        default: break;
        }
        for (const RS_Entity* generated : batch.entitiesToAdd) {
            REQUIRE(generated);
            CAPTURE(i, sources[i].kind(), expectedOutputType, generated->rtti());
            CHECK(generated->rtti() == expectedOutputType);
            const auto geometry = persistenceGeometry(*generated, i);
            std::unique_ptr<RS_Entity> copy{generated->clone()};
            REQUIRE(copy);
            copy->setLayer(persistenceLayer);
            persistedGeometry.push_back(geometry);
            persistenceGraphic.addEntity(copy.release());
        }
        prepared.release();
        qDeleteAll(batch.entitiesToAdd);
    }

    REQUIRE_FALSE(persistedGeometry.empty());
    struct PersistenceLane {
        RS2::FormatType format;
        const char* extension;
    };
    const std::array<PersistenceLane, 5> persistenceLanes{{
        {RS2::FormatDXFRW14, "r14.dxf"},
        {RS2::FormatDXFRW2000, "r2000.dxf"},
        {RS2::FormatDWG, "r2000.dwg"},
        {RS2::FormatDXFRW2018, "r2018.dxf"},
        {RS2::FormatDWG2018, "r2018.dwg"},
    }};
    for (const auto& lane : persistenceLanes) {
        CAPTURE(lane.extension);
        const auto path = persistenceDirectory.file(lane.extension);
        REQUIRE(lc::test::exportDrawing(persistenceGraphic, path, lane.format));
        RS_Graphic reopened;
        REQUIRE(lc::test::importDrawing(reopened, path, lane.format));
        std::vector<PersistedGeometry> reopenedGeometry;
        for (const RS_Entity* entity : reopened) {
            if (entity->isDeleted()) continue;
            CAPTURE(entity->rtti());
            reopenedGeometry.push_back(persistenceGeometry(*entity, 0));
        }
        REQUIRE(reopenedGeometry.size() == persistedGeometry.size());
        for (std::size_t i = 0; i < persistedGeometry.size(); ++i) {
            CAPTURE(i, persistedGeometry[i].sourceFixture, persistedGeometry[i].type, reopenedGeometry[i].type);
            CHECK(reopenedGeometry[i].type == persistedGeometry[i].type);
            const auto checkVector = [](const RS_Vector& actual, const RS_Vector& expected, const char* field) {
                INFO("geometry field=" << field);
                const double scale = std::max({1.0, std::abs(expected.x), std::abs(expected.y)});
                CHECK(std::abs(actual.x - expected.x) <= 1e-8 * scale);
                CHECK(std::abs(actual.y - expected.y) <= 1e-8 * scale);
            };
            checkVector(reopenedGeometry[i].start, persistedGeometry[i].start, "start");
            checkVector(reopenedGeometry[i].end, persistedGeometry[i].end, "end");
            if (persistedGeometry[i].type != RS2::EntitySpline) {
                checkVector(reopenedGeometry[i].minimum, persistedGeometry[i].minimum, "minimum");
                checkVector(reopenedGeometry[i].maximum, persistedGeometry[i].maximum, "maximum");
            }
            REQUIRE(reopenedGeometry[i].references.size() == persistedGeometry[i].references.size());
            for (std::size_t point = 0; point < persistedGeometry[i].references.size(); ++point)
                checkVector(reopenedGeometry[i].references[point], persistedGeometry[i].references[point], "reference");
            const double lengthScale = std::max(1.0, std::abs(persistedGeometry[i].length));
            CHECK(std::abs(reopenedGeometry[i].length - persistedGeometry[i].length) <= 1e-8 * lengthScale);
            REQUIRE(reopenedGeometry[i].splineData.has_value() == persistedGeometry[i].splineData.has_value());
            if (persistedGeometry[i].splineData) {
                const auto& expected = *persistedGeometry[i].splineData;
                const auto& actual = *reopenedGeometry[i].splineData;
                CAPTURE(expected.degree, expected.type, expected.controlPoints.size(), expected.knotslist.size(),
                        expected.weights.size(), actual.degree, actual.type, actual.controlPoints.size(),
                        actual.knotslist.size(), actual.weights.size());
                CHECK(actual.degree == expected.degree);
                CHECK(actual.type == expected.type);
                CHECK(actual.fitTolerance == expected.fitTolerance);
                CHECK(actual.fitTangentsConstrained == expected.fitTangentsConstrained);
                checkVector(actual.fitStartTangent, expected.fitStartTangent, "fit start tangent");
                checkVector(actual.fitEndTangent, expected.fitEndTangent, "fit end tangent");
                REQUIRE(actual.controlPoints.size() == expected.controlPoints.size());
                REQUIRE(actual.knotslist.size() == expected.knotslist.size());
                REQUIRE(actual.weights.size() == expected.weights.size());
                REQUIRE(actual.fitPoints.size() == expected.fitPoints.size());
                for (std::size_t point = 0; point < expected.controlPoints.size(); ++point)
                    checkVector(actual.controlPoints[point], expected.controlPoints[point], "spline control");
                for (std::size_t point = 0; point < expected.fitPoints.size(); ++point)
                    checkVector(actual.fitPoints[point], expected.fitPoints[point], "spline fit point");
                for (std::size_t knot = 0; knot < expected.knotslist.size(); ++knot)
                    CHECK(std::abs(actual.knotslist[knot] - expected.knotslist[knot]) <=
                          1e-8 * std::max(1.0, std::abs(expected.knotslist[knot])));
                for (std::size_t weight = 0; weight < expected.weights.size(); ++weight)
                    CHECK(std::abs(actual.weights[weight] - expected.weights[weight]) <=
                          1e-8 * std::max(1.0, std::abs(expected.weights[weight])));
            }
        }
    }

    const qint64 linearRequestId = nextRequestId;
    Receipts linearReceipts;
    REQUIRE(owned.get().submit({token(linearRequestId), Req::GenerateSource{linearDefinition, {}}},
        Client::Mode::Commit, linearReceipts.into(0)));
    rt.until([&] { return linearReceipts.calls[0] != 0; });
    auto& linearCompletion = completed(linearReceipts.at(0), linearRequestId, 7);
    auto linearEditable = linearCompletion.nativeSource(attributes);
    REQUIRE(linearEditable);
    REQUIRE(linearEditable->hasGeneralizedSourceDefinition());
    auto linearToleranceSource = linearDefinition.source.makeEntity();
    REQUIRE(linearToleranceSource);
    const auto linearOptions = LC_OffsetWorkerTolerance::directOptions(
        linearDefinition, *linearToleranceSource);
    REQUIRE(linearOptions);
    CHECK(linearEditable->checkedWorldTolerance() ==
          linearOptions->tolerance.requestedGeometry);
    const auto linearCacheStamp = RS_Spline::geometryFingerprint(linearEditable->getData());
    linearEditable->move({2, 3});
    REQUIRE(linearEditable->hasGeneralizedSourceDefinition());
    CHECK(RS_Spline::geometryFingerprint(linearEditable->getData()) != linearCacheStamp);
    auto movedLinearSource = linearDefinition.source.makeEntity();
    REQUIRE(movedLinearSource);
    movedLinearSource->move({2, 3});
    const auto expectedLinearSource = LC_OffsetSourceGeometry::capture(*movedLinearSource);
    REQUIRE(expectedLinearSource);
    CHECK(Req::Values::encodeSourceGeometry(linearEditable->sourceDefinition()->source) ==
        Req::Values::encodeSourceGeometry(*expectedLinearSource));
    CHECK(owned.get().healthyAndIdle());
}

TEST_CASE("Worker joined op8 validates a typed native source cache before materialization", Tags) {
    auto& rt = runtime();
    OwnedClient owned{rt};
    RS_EntityContainer owner;
    RS_Line attributes{&owner, RS_LineData{{0, 0}, {1, 0}}};
    const LC_OffsetSourceDefinition definition{
        LC_OffsetSourceGeometry{RS_LineData{{0, 0}, {4, 0}}}, .5, 1e-4, false};

    Receipts generatedReceipts;
    REQUIRE(owned.get().submit({token(1), Req::GenerateSource{definition, {}}},
        Client::Mode::Commit, generatedReceipts.into(0)));
    rt.until([&] { return generatedReceipts.calls[0] != 0; });
    const auto& generated = completed(generatedReceipts.at(0), 1, 7);
    const auto& generatedResult = std::get<Reply::Native>(generated.reply().result);
    REQUIRE(generatedResult.ordinary.size() == 1);
    RS_SplineData cache;
    if (const auto* spline = std::get_if<RS_SplineData>(&generatedResult.ordinary.front())) cache = *spline;
    else {
        const auto& line = std::get<Req::Segment>(generatedResult.ordinary.front());
        cache.degree = 1; cache.controlPoints = {line.start, line.end};
        cache.knotslist = {0, 0, 1, 1}; cache.weights = {1, 1};
    }

    Receipts validatedReceipts;
    const Req::ValidateSourceCache validation{cache, definition, {}};
    REQUIRE(owned.get().submit({token(2), validation}, Client::Mode::Commit, validatedReceipts.into(0)));
    rt.until([&] { return validatedReceipts.calls[0] != 0; });
    const auto& validated = completed(validatedReceipts.at(0), 2, 8);
    auto native = validated.nativeValidatedSource(attributes);
    REQUIRE(native);
    CHECK(native->hasGeneralizedSourceDefinition());
    REQUIRE(native->sourceDefinition());
    CHECK(Req::Values::encodeSourceDefinition(*native->sourceDefinition()) ==
        Req::Values::encodeSourceDefinition(definition));

    auto altered = validation;
    altered.cache.controlPoints.back().y += 1;
    Receipts rejectedReceipts;
    REQUIRE(owned.get().submit({token(3), altered}, Client::Mode::Commit, rejectedReceipts.into(0)));
    rt.until([&] { return rejectedReceipts.calls[0] != 0; });
    const auto& rejected = completed(rejectedReceipts.at(0), 3, 8);
    CHECK_FALSE(rejected.nativeValidatedSource(attributes));
    CHECK(std::get<Reply::Native>(rejected.reply().result).refusal ==
        LC_OffsetCurveRefusal::NotCacheRepresentable);
    CHECK(owned.get().healthyAndIdle());
}

TEST_CASE("Worker typed-source edit replaces cache and definition atomically",
          "[lc-offsetcurve][worker-client][typed-source-edit]") {
    auto& rt = runtime();
    OwnedClient owned{rt};
    RS_EntityContainer owner;
    RS_Line attributes{&owner, RS_LineData{{0, 0}, {1, 0}}};
    const LC_OffsetSourceDefinition originalDefinition{
        LC_OffsetSourceGeometry{RS_LineData{{0, 0}, {4, 0}}}, .5, 1e-4, false};
    Receipts initialReceipts;
    REQUIRE(owned.get().submit({token(1), Req::GenerateSource{originalDefinition, {}}},
        Client::Mode::Commit, initialReceipts.into(0)));
    rt.until([&] { return initialReceipts.calls[0] != 0; });
    const auto& initialCompletion = completed(initialReceipts.at(0), 1, 7);
    auto editable = initialCompletion.nativeSource(attributes);
    REQUIRE(editable);

    auto replacementDefinition = originalDefinition;
    replacementDefinition.signedDistance = .75;
    auto preparation = LC_OffsetWorkerEdit::prepareSourceDefinition(*editable, replacementDefinition, {});
    REQUIRE(preparation.generateSource());
    CHECK_FALSE(preparation.result());
    Receipts editReceipts;
    REQUIRE(owned.get().submit({token(2), *preparation.generateSource()}, Client::Mode::Commit,
        editReceipts.into(0)));
    rt.until([&] { return editReceipts.calls[0] != 0; });
    const auto& editCompletion = completed(editReceipts.at(0), 2, 7);
    const auto oldCache = RS_Spline::geometryFingerprint(editable->getData());
    const auto outcome = editCompletion.apply(*editable);
    REQUIRE(outcome);
    CHECK(outcome.changed);
    REQUIRE(editable->sourceDefinition());
    CHECK(Req::Values::encodeSourceDefinition(*editable->sourceDefinition()) ==
        Req::Values::encodeSourceDefinition(replacementDefinition));
    CHECK(RS_Spline::geometryFingerprint(editable->getData()) != oldCache);
    CHECK(editable->hasGeneralizedSourceDefinition());
    CHECK(owned.get().healthyAndIdle());
}

TEST_CASE("Worker Produce completions materialize ordered native copies without publication authority", Tags) {
    auto& rt = runtime(); OwnedClient owned{rt}; Receipts receipts;
    RS_EntityContainer owner; RS_Layer layer{QStringLiteral("produce local")};
    RS_Ellipse attributes{&owner, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    attributes.setLayer(&layer); attributes.setSourceHandle(91); attributes.setXDictHandle(92);
    attributes.setReactorHandles({93}); attributes.setOwnerLineage(94);
    attributes.setUserDefVar(QStringLiteral("produce-local"), QStringLiteral("retained"));
    const LC_OffsetCurveXData::XData foreign{
        std::make_shared<DRW_Variant>(1001, std::string("PRODUCE_FOREIGN")),
        std::make_shared<DRW_Variant>(1000, std::string("keep exactly"))};
    auto stale = LC_OffsetCurveXData::forExport(foreign,
        LC_OffsetCurveXData::Definition{definition(), 0x10, {{1000, QStringLiteral("stale tail")}}});
    REQUIRE(stale); attributes.setDrwExtData(*stale);
    Req::Produce op; op.source = attributes.getData(); op.minimum = attributes.getMin(); op.maximum = attributes.getMax();
    op.context.topLevel = true; op.coordinate = {3, 2}; op.distance = .25;
    op.copies = 2; op.asOffsetCurve = true;
    REQUIRE(owned.get().submit({token(1), op}, Client::Mode::Commit, receipts.into(0)));
    rt.until([&] { return receipts.calls[0] != 0; });
    const auto& completion = completed(receipts.at(0), 1, 5);
    CHECK_FALSE(completion.native(attributes)); CHECK_FALSE(completion.ordinary(attributes));
    auto materialized = completion.produced(attributes); REQUIRE(materialized);
    const auto& receipt = std::get<Reply::Produced>(completion.reply().result);
    const auto& outcome = materialized->outcome;
    REQUIRE(outcome.succeeded()); CHECK(outcome.complete()); CHECK_FALSE(outcome.source);
    CHECK_FALSE(outcome.sourceRemoved); CHECK(outcome.createdEntities.empty());
    CHECK(outcome.copyOutputCounts == std::vector<std::size_t>{1, 1});
    CHECK(outcome.cacheEngineRuns == 2); CHECK(outcome.adoptionAttempts == 2);
    CHECK(outcome.adoptionCheckWork.vertices == receipt.source.adoptionCheckWork.vertices);
    CHECK(outcome.adoptionCheckWork.vertices > 0);
    REQUIRE(materialized->entities.size() == 2);
    std::size_t actualDeep = 0;
    for (std::size_t i = 0; i < materialized->entities.size(); ++i) {
        const auto* curve = dynamic_cast<const LC_OffsetCurve*>(materialized->entities[i].get()); REQUIRE(curve);
        CHECK(curve->getParent() == nullptr); CHECK(curve->getLayer(false) == &layer);
        CHECK(curve->getPen(false) == attributes.getPen(false));
        CHECK(curve->getUserDefVar("produce-local") == "retained");
        CHECK(curve->sourceHandle() == 0); CHECK(curve->xDictHandle() == 0);
        CHECK(curve->reactorHandles().empty()); CHECK(curve->ownerLineage() == 0);
        CHECK(curve->lineage() != attributes.lineage()); CHECK(curve->getId() != attributes.getId());
        CHECK(curve->unknownOwnedFlags() == 0); CHECK(curve->unknownOwnedTail().empty());
        auto stripped = LC_OffsetCurveXData::parseAndStrip(curve->getDrwExtData());
        REQUIRE(stripped.remaining); CHECK(stripped.knownCandidates == 0);
        REQUIRE(stripped.remaining->size() >= 2);
        CHECK((*stripped.remaining)[0] == foreign[0]); CHECK((*stripped.remaining)[1] == foreign[1]);
        CHECK(curve->definition().distance == double(i + 1) * op.distance);
        const auto& value = std::get<Reply::Curve>(receipt.outputs[i]);
        exactCache(curve->getData(), value.cache);
        const auto checked = Geo::checkCache(curve->getData(), curve->definition(), curve->checkedWorldTolerance());
        REQUIRE(checked); // Independent test proof, not parent materialization.
        const auto cost = measureOffsetOutput({curve}, makeDefaultOffsetSourceBudget().maxDeepEntities);
        REQUIRE(cost.status == LC_OffsetTreeStatus::Ok); actualDeep += cost.deepEntities;
    }
    CHECK(outcome.usage.deepEntities == actualDeep);
    CHECK(owner.count() == 0);
    CHECK(attributes.sourceHandle() == 91);
    CHECK(LC_OffsetCurveXData::parseAndStrip(attributes.getDrwExtData())
              .knownCandidates == 1);
    auto again = completion.produced(attributes);
    REQUIRE(again);
    CHECK(again->entities[0]->getId() != materialized->entities[0]->getId());
    CHECK(outcome.cacheEngineRuns == receipt.source.cacheEngineRuns);
}

TEST_CASE("Worker Produce plain and preview copies retain only their original "
          "drawing attributes",
          Tags) {
  auto &rt = runtime();
  OwnedClient owned{rt};
  Receipts receipts;
  RS_Layer layer{QStringLiteral("plain produce")};
  RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
  source.setLayer(&layer);
  source.setUserDefVar("not ordinary", "not copied");
  source.setSourceHandle(67);
  for (int mode = 0; mode < 2; ++mode) {
    Req::Produce op;
    op.source = source.getData();
    op.minimum = source.getMin();
    op.maximum = source.getMax();
    op.context.topLevel = true;
    op.coordinate = {3, 2};
    op.distance = .25;
    op.asOffsetCurve = mode == 1;
    op.preview = mode == 1;
    REQUIRE(owned.get().submit({token(mode + 1), op}, Client::Mode::Commit,
                               receipts.into(std::size_t(mode))));
    rt.until([&] { return receipts.calls[std::size_t(mode)] != 0; });
    const auto &completion =
        completed(receipts.at(std::size_t(mode)), mode + 1, 5);
    auto made = completion.produced(source);
    REQUIRE(made);
    REQUIRE(made->outcome.succeeded());
    CHECK(made->outcome.adoptionAttempts == 0);
    REQUIRE(made->entities.size() == 1);
    const auto &entity = *made->entities[0];
    CHECK_FALSE(dynamic_cast<const LC_OffsetCurve *>(&entity));
    CHECK(entity.getParent() == nullptr);
    CHECK(entity.getLayer(false) == &layer);
    CHECK(entity.getPen(false) == source.getPen(false));
    CHECK(entity.getDrwExtData().empty());
    CHECK(entity.getUserDefVar("not ordinary").isEmpty());
    CHECK(entity.sourceHandle() == 0);
    const auto &value =
        std::get<Reply::Produced>(completion.reply().result).outputs[0];
    if (const auto *spline = std::get_if<RS_SplineData>(&value))
      exactCache(static_cast<const RS_Spline &>(entity).getData(), *spline);
    else {
      const auto &line = std::get<Req::Segment>(value);
      CHECK(exact(entity.getStartpoint(), line.start));
      CHECK(exact(entity.getEndpoint(), line.end));
    }
  }
}

TEST_CASE("Worker Produce typed refusals materialize no prefix and cannot create native entities", Tags) {
    auto& rt = runtime(); OwnedClient owned{rt}; Receipts receipts;
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    Req::Produce op; op.source = source.getData(); op.context.topLevel = true;
    op.coordinate = {3, 2}; op.distance = 0; op.copies = 2; op.asOffsetCurve = true;
    REQUIRE(owned.get().submit({token(1), op}, Client::Mode::Commit, receipts.into(0)));
    rt.until([&] { return receipts.calls[0] != 0; });
    const auto& completion = completed(receipts.at(0), 1, 5);
    auto refused = completion.produced(source); REQUIRE(refused);
    CHECK_FALSE(refused->outcome.succeeded()); CHECK(refused->entities.empty());
    CHECK(refused->outcome.copyOutputCounts.empty()); CHECK(refused->outcome.cacheEngineRuns == 0);
    CHECK(refused->outcome.detail == LC_OffsetFailureDetail::InvalidDistance);
    CHECK_FALSE(completion.native(source)); CHECK_FALSE(completion.ordinary(source));
}

TEST_CASE("Moved worker completions cannot materialize vacated successful receipts", Tags) {
    auto& rt = runtime(); OwnedClient owned{rt}; Receipts receipts;
    RS_Ellipse attributes{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    Req::Produce op; op.source = attributes.getData(); op.minimum = attributes.getMin(); op.maximum = attributes.getMax();
    op.context.topLevel = true; op.coordinate = {3, 2}; op.distance = .25;
    op.copies = 2; op.asOffsetCurve = true;
    REQUIRE(owned.get().submit({token(1), op}, Client::Mode::Commit, receipts.into(0)));
    rt.until([&] { return receipts.calls[0] != 0; });
    const auto& original = completed(receipts.at(0), 1, 5);
    CHECK(original.isJoined());
    REQUIRE(original.produced(attributes));
    Completion moved{std::move(*receipts.outcomes[0]->completed)};
    CHECK_FALSE(original.isJoined()); CHECK(moved.isJoined());
    const auto stale = original.produced(attributes);
    CHECK_FALSE(stale.has_value());
    if (stale) { CHECK_FALSE(stale->outcome.succeeded()); CHECK_FALSE(stale->outcome.complete()); }
    auto valid = moved.produced(attributes); REQUIRE(valid); REQUIRE(valid->outcome.complete());
    REQUIRE(valid->entities.size() == 2);

    auto ordinary = generate(2); std::get<Req::Generate>(ordinary.operation).limits.checkLimits.maxVertices = 1;
    REQUIRE(owned.get().submit(ordinary, Client::Mode::Commit, receipts.into(1)));
    rt.until([&] { return receipts.calls[1] != 0; });
    const auto& source = completed(receipts.at(1), 2, 0);
    const auto before = source.ordinary(attributes); REQUIRE(before); REQUIRE_FALSE(before->empty());
    Completion transferred{std::move(*receipts.outcomes[1]->completed)};
    CHECK_FALSE(source.isJoined()); CHECK(transferred.isJoined());
    CHECK_FALSE(source.ordinary(attributes));
    CHECK_FALSE(source.native(attributes)); CHECK_FALSE(source.produced(attributes));
    CHECK_FALSE(transferred.produced(attributes));
    const auto retained = transferred.ordinary(attributes); REQUIRE(retained); CHECK(retained->size() == before->size());
    moved = std::move(transferred);
    CHECK(moved.isJoined()); CHECK_FALSE(transferred.isJoined());
    CHECK_FALSE(transferred.ordinary(attributes)); CHECK_FALSE(transferred.produced(attributes));
    REQUIRE(moved.ordinary(attributes));
}

TEST_CASE("Worker membership receipts acknowledge only their unchanged raw native cache", Tags) {
    auto& rt = runtime(); Receipts receipts; OwnedClient owned(rt);
    REQUIRE(owned.get().submit(validate(1), Client::Mode::Commit, receipts.into(0)));
    rt.until([&] { return receipts.calls[0] != 0; });
    const auto& validation = completed(receipts.at(0), 1, 1);
    RS_Line attributes{nullptr, {0, 0}, {1, 0}};
    auto curve = validation.native(attributes); REQUIRE(curve);
    const auto raw = curve->getData();
    Req::Request request{token(2), Req::CheckThrough{raw, curve->definition(), curve->checkedWorldTolerance(),
        raw.controlPoints.front(), {}}};
    REQUIRE(owned.get().submit(request, Client::Mode::Commit, receipts.into(1)));
    rt.until([&] { return receipts.calls[1] != 0; });
    const auto& membership = completed(receipts.at(1), 2, 4);
    const auto& result = std::get<Reply::Native>(membership.reply().result);
    REQUIRE(result); CHECK_FALSE(result.changed); CHECK_FALSE(result.curve);
    CHECK(result.engineRuns == 0); CHECK_FALSE(result.engineStatus); CHECK_FALSE(result.checkReason);
    CHECK(result.checkWork.vertices == 0); CHECK(result.throughWork.vertices > 0);
    CHECK_FALSE(membership.native(attributes));
    curve->setUserDefVar("membership", "retained");
    const auto applied = membership.apply(*curve);
    REQUIRE(applied); CHECK_FALSE(applied.changed); exactCache(curve->getData(), raw);
    CHECK(curve->getUserDefVar("membership") == "retained");
    CHECK(curve->hasEditableDefinition());
    curve->revertDirection(); REQUIRE(curve->hasEditableDefinition());
    const auto stale = membership.apply(*curve);
    CHECK_FALSE(stale); CHECK(stale.refusal == LC_OffsetCurveRefusal::Inconsistent);
    CHECK_FALSE(stale.changed); CHECK(stale.throughWork.vertices == applied.throughWork.vertices);
}

TEST_CASE("Worker client keeps one active job and replaces its single queued job", Tags) {
    auto& rt = runtime();
    Receipts receipts;
    OwnedClient owned(rt);
    auto& client = owned.get();
    REQUIRE(client.submit(heavyValidate(1), Client::Mode::Preview, receipts.into(0)));
    REQUIRE(client.submit(generate(2), Client::Mode::Preview, receipts.into(1)));
    REQUIRE(client.submit(generate(3), Client::Mode::Preview, receipts.into(2)));
    CHECK(client.busy());
    CHECK(receipts.calls[0] == 0);
    CHECK(receipts.calls[1] == 0);
    CHECK(receipts.calls[2] == 0);
    rt.until([&] { return receipts.calls[0] && receipts.calls[1] && receipts.calls[2]; });
    CHECK_FALSE(client.busy());
    stopped(receipts.at(0), Client::Status::Superseded);
    stopped(receipts.at(1), Client::Status::Superseded);
    const auto& completion = completed(receipts.at(2), 3, 0);
    REQUIRE(std::holds_alternative<Reply::Native>(completion.reply().result));
    CHECK(std::get<Reply::Native>(completion.reply().result).refusal == LC_OffsetCurveRefusal::None);
    rt.settle();
    CHECK(receipts.calls[0] == 1);
    CHECK(receipts.calls[1] == 1);
    CHECK(receipts.calls[2] == 1);
}

TEST_CASE("Worker client cancels a real child and recovers without resetting request IDs", Tags) {
    auto& rt = runtime();
    Receipts receipts;
    OwnedClient owned(rt);
    auto& client = owned.get();
    REQUIRE(client.submit(heavyValidate(1), Client::Mode::Preview, receipts.into(0)));
    client.cancel();
    CHECK(receipts.calls[0] == 0);
    rt.until([&] { return receipts.calls[0] != 0; });
    stopped(receipts.at(0), Client::Status::Cancelled);
    CHECK_FALSE(client.busy());
    CHECK_FALSE(client.submit(generate(1), Client::Mode::Preview, receipts.into(3)));
    client.cancel();
    REQUIRE(client.submit(generate(2), Client::Mode::Preview, receipts.into(1)));
    rt.until([&] { return receipts.calls[1] != 0; });
    completed(receipts.at(1), 2, 0);
    rt.settle();
    CHECK(receipts.calls[0] == 1);
    CHECK(receipts.calls[3] == 0);
}

TEST_CASE("Worker client cancellation suppresses an already completed deferred delivery", Tags) {
    auto& rt = runtime();
    Receipts receipts;
    OwnedClient owned(rt);
    auto& client = owned.get();
    CancelBeforeDelivery filter(client);
    REQUIRE(client.submit(generate(1), Client::Mode::Preview, receipts.into(0)));
    CHECK(receipts.calls[0] == 0);
    rt.until([&] { return receipts.calls[0] != 0; });
    REQUIRE(filter.intercepted);
    // Suppressed delivery discards its work record too: keep the reservation,
    // rather than advertise a precise total that is no longer returned.
    stopped(receipts.at(0), Client::Status::Cancelled);
    CHECK(receipts.at(0).transportWork.payloadBytes > 0);
    client.removeEventFilter(&filter);
    REQUIRE(client.submit(generate(2), Client::Mode::Preview, receipts.into(1)));
    rt.until([&] { return receipts.calls[1] != 0; });
    completed(receipts.at(1), 2, 0);
}

TEST_CASE("Worker client lowered deadlines time out real children and then recover", Tags) {
    auto& rt = runtime();
    Receipts receipts;
    OwnedClient owned(rt);
    auto& client = owned.get();
    std::size_t slot = 0;
    for (const auto mode : {Client::Mode::Preview, Client::Mode::Commit}) {
        CAPTURE(mode);
        const auto request = heavyValidate(qint64(slot + 1));
        {
            LowerDeadlines lower(client);
            REQUIRE(client.submit(request, mode, receipts.into(slot)));
            rt.until([&] { return receipts.calls[slot] != 0; });
            stopped(receipts.at(slot), Client::Status::TimedOut);
            CHECK(receipts.at(slot).elapsedMilliseconds >= 1);
        }
        // A 1 ms deadline may expire before geometry starts. This does not qualify
        // termination of a hung geometry kernel or justify raising any work cap.
        CHECK_FALSE(client.busy());
        ++slot;
        REQUIRE(client.submit(generate(qint64(slot + 1)), mode, receipts.into(slot)));
        rt.until([&] { return receipts.calls[slot] != 0; });
        completed(receipts.at(slot), qint64(slot + 1), 0);
        ++slot;
    }
}

TEST_CASE("Worker client owner destruction drops active queued and deferred callbacks", Tags) {
    auto& rt = runtime();
    rt.helperAvailable();
    Receipts receipts;
    {
        auto owner = std::make_unique<QObject>();
        auto* client = new Client(owner.get());
        REQUIRE(client->submit(heavyValidate(1), Client::Mode::Preview, receipts.into(0)));
        REQUIRE(client->submit(generate(2), Client::Mode::Preview, receipts.into(1)));
        owner.reset();
    }
    rt.settle();
    CHECK(receipts.calls[0] == 0);
    CHECK(receipts.calls[1] == 0);
    {
        auto owner = std::make_unique<QObject>();
        auto* client = new Client(owner.get());
        DestroyBeforeDelivery filter(*client, owner);
        REQUIRE(client->submit(generate(1), Client::Mode::Preview, receipts.into(2)));
        rt.until([&] { return filter.intercepted; });
        CHECK_FALSE(owner);
        rt.settle();
        CHECK(receipts.calls[2] == 0);
    }
}

TEST_CASE("Joined cached Produce completions update only current caches and revoke moved-from authority",
          "[lc-offsetcurve][worker-client][preview-portable]") {
    auto& rt = runtime();
    OwnedClient owned{rt};
    Receipts receipts;
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    source.setSourceHandle(91);
    source.setUserDefVar("cached-local", "retained");
    LC_OffsetPreviewCache parent;
    Req::CachedProduce op;
    op.produce.source = source.getData();
    op.produce.minimum = source.getMin();
    op.produce.maximum = source.getMax();
    op.produce.context.topLevel = true;
    op.produce.coordinate = {3, 2};
    op.produce.distance = .25;
    op.produce.copies = 2;
    op.produce.preview = true;
    op.produce.asOffsetCurve = true;
    auto snapshot = parent.snapshot(&source);
    REQUIRE(snapshot);
    op.cache = *snapshot;
    REQUIRE(owned.get().submit({token(1), op}, Client::Mode::Preview, receipts.into(0)));
    rt.until([&] { return receipts.calls[0] != 0; });
    const auto& first = completed(receipts.at(0), 1, 6);
    REQUIRE(first.updatePreviewCache(parent, &source));
    CHECK_FALSE(first.updatePreviewCache(parent, &source)); // Its original revision is stale now.
    snapshot = parent.snapshot(&source);
    REQUIRE(snapshot);
    REQUIRE(snapshot->entries.size() == 2);
    op.cache = *snapshot;
    op.produce.preview = false;
    op.produce.limits.maxSamples = 1;
    op.produce.limits.maxIntersectionPairs = 1;
    REQUIRE(owned.get().submit({token(2), op}, Client::Mode::Commit, receipts.into(1)));
    rt.until([&] { return receipts.calls[1] != 0; });
    completed(receipts.at(1), 2, 6);
    auto& original = *receipts.outcomes[1]->completed;
    Completion moved{std::move(original)};
    CHECK_FALSE(original.updatePreviewCache(parent, &source));
    CHECK_FALSE(original.produced(source));
    auto prepared = moved.produced(source);
    REQUIRE(prepared);
    REQUIRE(prepared->outcome.complete());
    CHECK(prepared->outcome.cacheEngineRuns == 0);
    CHECK(prepared->outcome.sampleUnits == 0);
    CHECK(prepared->outcome.reusedCacheCopies == 2);
    CHECK(prepared->outcome.reusedCopies.size() == 2);
    CHECK(prepared->outcome.adoptionAttempts == 2);
    CHECK(prepared->outcome.adoptionCheckWork.vertices > 0);
    for (const auto& entity : prepared->entities) {
        const auto* curve = dynamic_cast<const LC_OffsetCurve*>(entity.get());
        REQUIRE(curve);
        CHECK(curve->getParent() == nullptr);
        CHECK(curve->sourceHandle() == 0);
        CHECK(curve->getUserDefVar("cached-local") == "retained");
        REQUIRE(Geo::checkCache(curve->getData(), curve->definition(), curve->checkedWorldTolerance()));
    }
    RS_Line other;
    LC_CurveOffsetMaterializationResult refused;
    refused.status = LC_CurveOffsetStatus::SingularOffset;
    parent.keep(&other, LC_CurveOffsetSide::Left, 1, makeDefaultOffsetSourceBudget(), refused);
    CHECK_FALSE(moved.updatePreviewCache(parent, &source));
    CHECK(parent.snapshot(&source)->otherEntries == 1);
    CHECK(source.sourceHandle() == 91);
}

TEST_CASE("Cached Produce carries action-wide eviction across a fresh helper process",
          "[lc-offsetcurve][worker-client][preview-portable]") {
    auto& rt = runtime();
    OwnedClient owned{rt};
    Receipts receipts;
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    LC_OffsetPreviewCache parent;
    const auto budget = makeDefaultOffsetSourceBudget();
    LC_CurveOffsetMaterializationResult refused;
    refused.status = LC_CurveOffsetStatus::SingularOffset;
    parent.keep(&source, LC_CurveOffsetSide::Left, 10, budget, refused);
    std::vector<std::unique_ptr<RS_Line>> otherSources;
    otherSources.reserve(255);
    for (int i = 1; i <= 255; ++i) {
        otherSources.push_back(std::make_unique<RS_Line>(nullptr, RS_Vector{0, double(i)}, RS_Vector{1, double(i)}));
        parent.keep(otherSources.back().get(), LC_CurveOffsetSide::Left, 1, budget, refused);
    }

    auto snapshot = parent.snapshot(&source);
    REQUIRE(snapshot);
    REQUIRE(snapshot->entries.size() == 1);
    CHECK(snapshot->otherEntries == 255);
    Req::Produce produce;
    produce.source = source.getData();
    produce.minimum = source.getMin();
    produce.maximum = source.getMax();
    produce.context.topLevel = true;
    produce.coordinate = {3, 2};
    produce.distance = .25;
    produce.copies = 1;
    produce.preview = true;
    Req::CachedProduce cached{std::move(produce), *snapshot};
    REQUIRE(owned.get().submit({token(1), std::move(cached)}, Client::Mode::Preview, receipts.into(0)));
    rt.until([&] { return receipts.calls[0] != 0; });
    const auto& completion = completed(receipts.at(0), 1, 6);
    REQUIRE(completion.updatePreviewCache(parent, &source));

    const auto after = parent.snapshot(&source);
    REQUIRE(after);
    CHECK(after->cleared);
    CHECK(after->otherEntries == 0);
    REQUIRE(after->entries.size() == 1);
    LC_CurveOffsetMaterializationResult found;
    CHECK_FALSE(parent.find(otherSources.front().get(), LC_CurveOffsetSide::Left, 1, budget, found));
    CHECK(parent.find(&source, after->entries.front().side, after->entries.front().magnitude, budget, found));
    CHECK(found.status == LC_CurveOffsetStatus::Ok);
}

TEST_CASE("Ordered mixed offset batches retain numeric prefixes and bind exact helper requests",
          "[lc-offsetcurve][worker-client][ordered-preparation]") {
    auto& rt = runtime();
    OwnedClient client{rt};
    Receipts receipts;
    class CountedLine final : public RS_Line {
    public:
        using RS_Line::RS_Line;
        mutable unsigned calls = 0;
        std::vector<RS_Entity*> createOffset(const RS_Vector& point, const double& distance) const override {
            ++calls; return RS_Line::createOffset(point, distance);
        }
    };
    CountedLine line{nullptr, {0, 0}, {2, 0}}, last{nullptr, {0, 4}, {2, 4}};
    RS_Ellipse source{nullptr, RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false}};
    RS_OffsetData data; data.coord = {3, 2}; data.distance = .25; data.number = 2;
    data.multipleCopies = true; data.keepOriginals = false; data.asOffsetCurve = true;
    LC_OffsetBatchLimits limits;
    LC_OffsetPreviewCache cache;
    LC_OffsetBatchPreparation prepared{data, {&line, &source, &last}, false, limits};
    LC_DocumentModificationBatch batch;
    REQUIRE_FALSE(prepared.advance(batch, &cache)); CHECK(batch.entitiesToAdd.empty());
    CHECK(line.calls == 2); CHECK(last.calls == 0); CHECK(prepared.pendingSource() == &source);
    auto request = prepared.request(token(1), &cache); REQUIRE(request);
    REQUIRE(std::holds_alternative<Req::CachedProduce>(request->operation));
    const auto& op = std::get<Req::CachedProduce>(request->operation).produce;
    CHECK(op.limits.maxDeepEntitiesPerRequest == limits.maxDeepEntitiesPerRequest - 2);
    REQUIRE(client.get().submit(*request, Client::Mode::Commit, receipts.into(0)));
    rt.until([&] { return receipts.calls[0] != 0; });
    const auto& completion = completed(receipts.at(0), 1, 6);
    auto otherToken = prepared.request(token(2), &cache); REQUIRE(otherToken);
    CHECK_FALSE(prepared.accept(completion));
    request = prepared.request(token(1), &cache); REQUIRE(request);
    REQUIRE(prepared.accept(completion));
    REQUIRE(prepared.advance(batch, &cache)); CHECK_FALSE(prepared.pendingSource());
    REQUIRE(prepared.outcome().sources.size() == 3);
    CHECK(prepared.outcome().sources[0].source == &line);
    CHECK(prepared.outcome().sources[1].source == &source);
    CHECK(prepared.outcome().sources[2].source == &last);
    CHECK(batch.entitiesToAdd.size() == 6); CHECK(batch.entitiesToDelete.size() == 3);
    CHECK(line.calls == 2); CHECK(last.calls == 2);
    const auto outputs = batch.entitiesToAdd;
    REQUIRE(prepared.advance(batch, &cache)); CHECK(batch.entitiesToAdd == outputs);
    CHECK(line.calls == 2); CHECK(last.calls == 2);
    CHECK_FALSE(source.isDeleted()); CHECK_FALSE(line.isDeleted()); CHECK_FALSE(last.isDeleted());
}

TEST_CASE("Typed parabola sources use bounded worker preparation",
          "[lc-offsetcurve][worker-client][ordered-preparation][parabola]") {
    auto& rt = runtime();
    OwnedClient client{rt};
    Receipts receipts;
    LC_Parabola source{nullptr, LC_ParabolaData{{RS_Vector{0, 0}, RS_Vector{5, 10},
                                                 RS_Vector{10, 0}}}};
    RS_OffsetData data;
    data.coord = {5, 8}; data.distance = .5; data.number = 1;
    data.multipleCopies = true; data.keepOriginals = true;
    LC_OffsetBatchPreparation prepared{data, {&source}, false, {}};
    LC_DocumentModificationBatch batch;

    REQUIRE_FALSE(prepared.advance(batch));
    CHECK(prepared.pendingSource() == &source);
    auto request = prepared.request(token(1));
    REQUIRE(request);
    REQUIRE(std::holds_alternative<Req::Produce>(request->operation));
    const auto& operation = std::get<Req::Produce>(request->operation);
    REQUIRE(std::holds_alternative<LC_OffsetSourceGeometry>(operation.source));
    CHECK(std::get<LC_OffsetSourceGeometry>(operation.source).kind() ==
          LC_OffsetSourceGeometry::Kind::Parabola);
    REQUIRE(client.get().submit(*request, Client::Mode::Commit, receipts.into(0)));
    rt.until([&] { return receipts.calls[0] != 0; });
    const auto& completion = completed(receipts.at(0), 1, 5);
    REQUIRE(prepared.accept(completion));
    REQUIRE(prepared.advance(batch));

    REQUIRE(prepared.outcome().sources.size() == 1);
    CHECK(prepared.outcome().sources.front().source == &source);
    CHECK(prepared.outcome().sources.front().complete());
    REQUIRE_FALSE(batch.entitiesToAdd.empty());
    for (const RS_Entity* entity : batch.entitiesToAdd)
        CHECK_FALSE(dynamic_cast<const LC_OffsetCurve*>(entity));
    CHECK_FALSE(source.isDeleted());
    prepared.release();
    qDeleteAll(batch.entitiesToAdd);
    batch.entitiesToAdd.clear();
}
