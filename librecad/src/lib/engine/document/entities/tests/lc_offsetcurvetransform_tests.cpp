/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

#include "drw_base.h"
#include "lc_offsetcurve.h"
#include "lc_offsetcurvetransform.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_text.h"

namespace tx = LC_OffsetCurveTransform;
using Step = tx::Step;
namespace {
void number(double actual, double expected) {
    CHECK((actual == expected || (std::isnan(actual) && std::isnan(expected))));
}
void point(const RS_Vector &actual, const RS_Vector &expected) {
    number(actual.x, expected.x); number(actual.y, expected.y); number(actual.z, expected.z);
    CHECK(actual.valid == expected.valid);
}
void points(const std::vector<RS_Vector> &actual, const std::vector<RS_Vector> &expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) { CAPTURE(i); point(actual[i], expected[i]); }
}
void numbers(const std::vector<double> &actual, const std::vector<double> &expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) { CAPTURE(i); number(actual[i], expected[i]); }
}
void cache(const RS_SplineData &actual, const RS_SplineData &expected) {
    points(actual.controlPoints, expected.controlPoints); points(actual.fitPoints, expected.fitPoints);
    numbers(actual.knotslist, expected.knotslist); numbers(actual.weights, expected.weights);
    numbers(actual.savedOpenKnots, expected.savedOpenKnots);
    CHECK(actual.degree == expected.degree); CHECK(actual.type == expected.type);
    CHECK(actual.savedOpenType == expected.savedOpenType);
    CHECK(actual.m_closedFlag == expected.m_closedFlag); CHECK(actual.m_periodicFlag == expected.m_periodicFlag);
}
void definition(const LC_EllipseOffsetDefinition &actual, const LC_EllipseOffsetDefinition &expected) {
    point(actual.basis.center, expected.basis.center); point(actual.basis.majorP, expected.basis.majorP);
    number(actual.basis.ratio, expected.basis.ratio);
    number(actual.basis.angle1, expected.basis.angle1); number(actual.basis.angle2, expected.basis.angle2);
    CHECK(actual.basis.reversed == expected.basis.reversed); CHECK(actual.basis.isArc == expected.basis.isArc);
    number(actual.basis.angleDegrees, expected.basis.angleDegrees);
    number(actual.basis.startAngleDegrees, expected.basis.startAngleDegrees);
    number(actual.basis.otherAngleDegrees, expected.basis.otherAngleDegrees);
    number(actual.basis.angularLength, expected.basis.angularLength);
    number(actual.distance, expected.distance); number(actual.relTolerance, expected.relTolerance);
    CHECK(actual.towardCentre == expected.towardCentre); CHECK(actual.cacheReversed == expected.cacheReversed);
}
void insertData(const RS_InsertData &actual, const RS_InsertData &expected) {
    CHECK(actual.name == expected.name); point(actual.insertionPoint, expected.insertionPoint);
    point(actual.scaleFactor, expected.scaleFactor); point(actual.extrusion, expected.extrusion);
    number(actual.angle, expected.angle); CHECK(actual.cols == expected.cols); CHECK(actual.rows == expected.rows);
    point(actual.spacing, expected.spacing); CHECK(actual.blockSource == expected.blockSource);
    CHECK(actual.updateMode == expected.updateMode);
}
void attributes(RS_Entity &entity) {
    RS_Pen pen{RS_Color{31, 77, 149}, RS2::Width00, RS2::SolidLine};
    pen.setAlpha(.375f); pen.setDashOffset(.75); entity.setPen(pen);
    entity.setVisible(false); entity.setSelectionFlag(true);
    entity.setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("STAGE_TEST")),
        std::make_shared<DRW_Variant>(1000, std::string("source attribute")),
        std::make_shared<DRW_Variant>(1071, std::int32_t{19})});
}

struct Attributes {
    unsigned long long id;
    RS_EntityContainer *parent;
    RS_Layer *layer;
    RS_Pen pen;
    unsigned flags;
    bool visible;
    std::vector<std::shared_ptr<DRW_Variant>> tagPointers;
    std::vector<DRW_Variant> tagValues;

    explicit Attributes(const RS_Entity &entity) : id(entity.getId()), parent(entity.getParent()), layer(entity.getLayer(false)),
        pen(entity.getPen(false)), flags(entity.getFlags()), visible(entity.isVisible()), tagPointers(entity.getDrwExtData()) {
        for (const auto &tag : tagPointers) { REQUIRE(tag); tagValues.emplace_back(*tag); }
    }
    void check(const RS_Entity &entity) const {
        CHECK(entity.getId() == id);
        CHECK(entity.getParent() == parent); CHECK(entity.getLayer(false) == layer);
        const auto actualPen = entity.getPen(false);
        CHECK_FALSE(actualPen != pen); CHECK(actualPen.getFlags() == pen.getFlags());
        CHECK(actualPen.getLineTypeId() == pen.getLineTypeId());
        CHECK(actualPen.getLineTypeName() == pen.getLineTypeName());
        CHECK(actualPen.getColor().getFlags() == pen.getColor().getFlags());
        number(actualPen.getAlpha(), pen.getAlpha()); number(actualPen.dashOffset(), pen.dashOffset());
        number(actualPen.getScreenWidth(), pen.getScreenWidth()); CHECK(entity.getFlags() == flags);
        CHECK(entity.isVisible() == visible);
        const auto &tags = entity.getDrwExtData();
        REQUIRE(tags.size() == tagValues.size());
        for (std::size_t i = 0; i < tags.size(); ++i) {
            REQUIRE(tags[i]); CHECK(tags[i] == tagPointers[i]);
            CHECK(tags[i]->code() == tagValues[i].code()); REQUIRE(tags[i]->type() == tagValues[i].type());
            if (tags[i]->type() == DRW_Variant::STRING) {
                REQUIRE(tags[i]->content.s); CHECK(*tags[i]->content.s == *tagValues[i].content.s);
            } else { REQUIRE(tags[i]->type() == DRW_Variant::INTEGER); CHECK(tags[i]->i_val() == tagValues[i].i_val()); }
        }
    }
};

struct NativeValues {
    LC_EllipseOffsetDefinition geometry;
    std::vector<RS_Vector> grips;
    bool defined, consistent;
    double tolerance;
    unsigned flags;
    LC_OffsetCurve::OwnedTail tail;
    explicit NativeValues(const LC_OffsetCurve &native) : geometry(native.definition()), grips(native.cachedDefinitionGrips()),
        defined(native.isDefined()), consistent(native.isConsistent()), tolerance(native.checkedWorldTolerance()),
        flags(native.unknownOwnedFlags()), tail(native.unknownOwnedTail()) {}
    void check(const LC_OffsetCurve &native) const {
        definition(native.definition(), geometry); points(native.cachedDefinitionGrips(), grips);
        CHECK(native.isDefined() == defined); CHECK(native.isConsistent() == consistent);
        number(native.checkedWorldTolerance(), tolerance); CHECK(native.unknownOwnedFlags() == flags);
        const auto &actual = native.unknownOwnedTail(); REQUIRE(actual.size() == tail.size());
        for (std::size_t i = 0; i < actual.size(); ++i) {
            CHECK(actual[i].code == tail[i].code); REQUIRE(actual[i].value.index() == tail[i].value.index());
            if (const auto *s = std::get_if<QString>(&tail[i].value)) CHECK(std::get<QString>(actual[i].value) == *s);
            else if (const auto *p = std::get_if<RS_Vector>(&tail[i].value)) point(std::get<RS_Vector>(actual[i].value), *p);
            else if (const auto *d = std::get_if<double>(&tail[i].value)) number(std::get<double>(actual[i].value), *d);
            else if (const auto *n = std::get_if<std::int16_t>(&tail[i].value)) CHECK(std::get<std::int16_t>(actual[i].value) == *n);
            else CHECK(std::get<std::int32_t>(actual[i].value) == std::get<std::int32_t>(tail[i].value));
        }
    }
};

// Value snapshots also retain exact ordered child identities. No fingerprint is
// used as evidence that source geometry or transport attributes survived staging.
struct Snapshot {
    const RS_Entity *identity;
    Attributes attrs;
    std::optional<RS_SplineData> spline;
    std::optional<RS_LineData> line;
    std::optional<RS_CircleData> circle;
    std::optional<RS_ArcData> arc;
    std::optional<RS_EllipseData> ellipse;
    std::optional<RS_PointData> dot;
    std::optional<RS_InsertData> insert;
    std::optional<LC_InsertSourceEditStatus> insertStatus;
    std::optional<NativeValues> native;
    std::vector<Snapshot> children;
    explicit Snapshot(const RS_Entity &entity) : identity(&entity), attrs(entity) {
        if (const auto *s = dynamic_cast<const RS_Spline *>(&entity)) spline = s->getData();
        if (const auto *l = dynamic_cast<const RS_Line *>(&entity)) line = l->getData();
        if (const auto *c = dynamic_cast<const RS_Circle *>(&entity)) circle = c->getData();
        if (const auto *a = dynamic_cast<const RS_Arc *>(&entity)) arc = a->getData();
        if (const auto *e = dynamic_cast<const RS_Ellipse *>(&entity)) ellipse = e->getData();
        if (const auto *p = dynamic_cast<const RS_Point *>(&entity)) dot = p->getData();
        if (const auto *i = dynamic_cast<const RS_Insert *>(&entity)) {
            insert = i->getData(); insertStatus = i->lastSourceEditStatus();
        }
        if (const auto *n = dynamic_cast<const LC_OffsetCurve *>(&entity)) native.emplace(*n);
        if (const auto *c = dynamic_cast<const RS_EntityContainer *>(&entity))
            for (const auto *child : *c) { REQUIRE(child); children.emplace_back(*child); }
    }
    void check(const RS_Entity &entity) const {
        CHECK(&entity == identity); attrs.check(entity);
        if (spline) { const auto *s = dynamic_cast<const RS_Spline *>(&entity); REQUIRE(s); cache(s->getData(), *spline); }
        if (line) {
            const auto *l = dynamic_cast<const RS_Line *>(&entity); REQUIRE(l);
            point(l->getData().startpoint, line->startpoint); point(l->getData().endpoint, line->endpoint);
        }
        if (circle) {
            const auto *c = dynamic_cast<const RS_Circle *>(&entity); REQUIRE(c);
            point(c->getCenter(), circle->center); number(c->getRadius(), circle->radius);
        }
        if (arc) {
            const auto *a = dynamic_cast<const RS_Arc *>(&entity); REQUIRE(a);
            const auto &actual = a->getData();
            point(actual.center, arc->center); number(actual.radius, arc->radius);
            number(actual.angle1, arc->angle1); number(actual.angle2, arc->angle2);
            CHECK(actual.reversed == arc->reversed);
            number(actual.startAngleDegrees, arc->startAngleDegrees);
            number(actual.otherAngleDegrees, arc->otherAngleDegrees); number(actual.angularLength, arc->angularLength);
        }
        if (ellipse) {
            const auto *e = dynamic_cast<const RS_Ellipse *>(&entity); REQUIRE(e);
            LC_EllipseOffsetDefinition actual, expected;
            actual.basis = e->getData(); expected.basis = *ellipse; definition(actual, expected);
        }
        if (dot) {
            const auto *p = dynamic_cast<const RS_Point *>(&entity); REQUIRE(p); point(p->getPos(), dot->pos);
        }
        if (insert) {
            const auto *i = dynamic_cast<const RS_Insert *>(&entity); REQUIRE(i);
            insertData(i->getData(), *insert); CHECK(i->lastSourceEditStatus() == *insertStatus);
        }
        if (native) { const auto *n = dynamic_cast<const LC_OffsetCurve *>(&entity); REQUIRE(n); native->check(*n); }
        if (const auto *c = dynamic_cast<const RS_EntityContainer *>(&entity)) {
            REQUIRE(static_cast<std::size_t>(c->size()) == children.size());
            std::size_t i = 0;
            for (const auto *child : *c) { REQUIRE(child); children[i++].check(*child); }
        }
    }
};

std::unique_ptr<LC_OffsetCurve> transformRing() {
    RS_SplineData data{3, false};
    std::array<RS_Vector, 4> net{{{2, 0}, {2, 9./8}, {9./8, 2}, {0, 2}}};
    for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
        for (unsigned i = quadrant ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i]);
        for (auto &p : net) p = {-p.y, p.x};
    }
    for (unsigned i = 0; i <= 4; ++i)
        for (unsigned n = 0; n < (i == 0 || i == 4 ? 4u : 3u); ++n) data.knotslist.push_back(i);
    data.weights.assign(data.controlPoints.size(), 1); data.m_closedFlag = true;
    RS_Spline carrier{nullptr, data};
    LC_EllipseOffsetDefinition geometry;
    geometry.basis = {{0, 0}, {1, 0}, 1}; geometry.distance = 1; geometry.relTolerance = .01;
    auto native = LC_OffsetCurve::adopt(carrier, geometry, .02);
    REQUIRE(native);
    REQUIRE(native->setOwnedExtensions(0x4010, {{1000, QStringLiteral("keep on identity")}, {1070, std::int16_t{7}}}));
    attributes(*native);
    return native;
}

RS_SplineData segmentData() {
    RS_SplineData data{1, false};
    data.controlPoints = {{1, 2}, {3, 4}}; data.weights = {1, 1}; data.knotslist = {0, 0, 1, 1};
    return data;
}
const RS_Spline &asSpline(const tx::Result &result) {
    REQUIRE(result);
    const auto *spline = dynamic_cast<const RS_Spline *>(result.entity.get()); REQUIRE(spline);
    return *spline;
}

class CountedLine : public RS_Line {
public:
    explicit CountedLine(int &copies) : RS_Line(nullptr, {1, 2}, {3, 4}), m_copies(&copies) {}
    RS_Entity *clone() const override { ++*m_copies; return new CountedLine(*this); }
private:
    int *m_copies;
};

RS_InsertData insertedData() {
    return {QStringLiteral("fixture"), {3, 4}, {1, 1, 1}, 0, 1, 1, {0, 0}, nullptr, RS2::NoUpdate};
}
class TrackedInsert : public RS_Insert {
public:
    explicit TrackedInsert(std::vector<char> &calls) : RS_Insert(nullptr, insertedData()), m_calls(&calls) {
        addEntity(new RS_Line(this, {1, 2}, {3, 4}));
    }
    RS_Entity *clone() const override { return new TrackedInsert(*this); }
    void scale(const RS_Vector &center, const RS_Vector &factor) override {
        m_calls->push_back('S'); RS_Insert::scale(center, factor);
    }
    void move(const RS_Vector &offset) override { m_calls->push_back('M'); RS_Insert::move(offset); }
private:
    std::vector<char> *m_calls;
};
}

TEST_CASE("Offset typed staging retains exact native translation without changing source", "[lc-offsetcurve][transform-tool]") {
    auto source = transformRing(); const Snapshot before(*source);
    auto made = tx::prepare(*source, {Step::move({4, 8})});
    REQUIRE(made); CHECK_FALSE(made.keptPlain);
    const auto *native = dynamic_cast<const LC_OffsetCurve *>(made.entity.get()); REQUIRE(native);
    CHECK(native->hasEditableDefinition()); point(native->definition().basis.center, {4, 8});
    auto expected = source->toPlainSpline(); expected->move({4, 8});
    cache(native->getData(), expected->getData()); before.check(*source);
}

TEST_CASE("Offset typed staging uses actual transformed plain data when native refuses", "[lc-offsetcurve][transform-tool]") {
    for (unsigned operation = 0; operation < 5; ++operation) {
        CAPTURE(operation);
        auto source = transformRing(); const Snapshot before(*source);
        auto expected = source->toPlainSpline();
        const auto step = operation == 0 ? Step::move({.1, .2}) : operation == 1 ? Step::rotate({0, 0}, .3) :
            operation == 2 ? Step::scale({0, 0}, {2, 3}) : operation == 3 ? Step::shear(.25) : Step::reverse();
        switch (operation) {
        case 0: expected->move({.1, .2}); break;
        case 1: expected->rotate({0, 0}, .3); break;
        case 2: expected->scale({0, 0}, {2, 3}); break;
        case 3: expected->shear(.25); break;
        case 4: expected->revertDirection(); break;
        }
        auto made = tx::prepare(*source, {step}, operation >= 2);
        REQUIRE(made); CHECK(made.keptPlain); CHECK(made.entity->rtti() == RS2::EntitySpline);
        cache(asSpline(made).getData(), expected->getData()); before.check(*source);
    }
}

TEST_CASE("Offset typed staging preserves legitimate identities and bounded empty plans", "[lc-offsetcurve][transform-tool]") {
    auto source = transformRing(); const Snapshot before(*source);
    for (const auto &steps : {tx::Steps{}, tx::Steps{Step::move({0, 0}), Step::rotate({7, 9}, 0),
        Step::scale({1, 2}, {1, 1}), Step::shear(0)}}) {
        auto made = tx::prepare(*source, steps);
        REQUIRE(made); CHECK_FALSE(made.keptPlain);
        const auto *native = dynamic_cast<const LC_OffsetCurve *>(made.entity.get()); REQUIRE(native);
        cache(native->getData(), source->getData()); NativeValues(*source).check(*native);
        before.check(*source);
    }
}

TEST_CASE("Offset typed staging validates every step before any clone", "[lc-offsetcurve][transform-tool]") {
    const double nan = std::numeric_limits<double>::quiet_NaN(), inf = std::numeric_limits<double>::infinity();
    int copies = 0; CountedLine source(copies); attributes(source); const Snapshot before(source);
    const std::vector<Step> invalid{Step::move({nan, 0}), Step::move(RS_Vector{false}), Step::move({0, 0, 1e-300}),
        Step::rotate({inf, 0}, 0), Step::rotate({0, 0}, nan), Step::scale({0, 0, 1}, {1, 1}),
        Step::scale({0, 0}, {inf, 1}), Step::scale({0, 0}, RS_Vector{false}),
        Step::mirror({1, 2}, {1, 2}), Step::mirror({0, 0}, {1, 2, 1e-300}),
        Step::mirror({.1, .3}, {std::nextafter(.1, inf), std::nextafter(.3, inf)}), Step::shear(inf),
        Step{static_cast<Step::Kind>(99)}};
    for (const auto &bad : invalid) {
        auto made = tx::prepare(source, {Step::move({4, 8}), bad});
        CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::InvalidSteps);
        CHECK(made.failedStep == 1); CHECK(copies == 0); before.check(source);
    }
    tx::Steps steps;
    for (std::size_t i = 0; i < tx::MaximumSteps; ++i) REQUIRE(steps.append(Step::move({0, 0})));
    REQUIRE(tx::prepare(source, steps)); CHECK(copies == 1);
    CHECK_FALSE(steps.append(Step::reverse()));
    auto made = tx::prepare(source, steps);
    CHECK_FALSE(made); CHECK(made.refusal == tx::Refusal::InvalidSteps); CHECK(copies == 1);
    const tx::Steps oversized{Step::reverse(), Step::reverse(), Step::reverse(), Step::reverse(), Step::reverse(),
        Step::reverse(), Step::reverse(), Step::reverse(), Step::reverse()};
    CHECK_FALSE(tx::prepare(source, oversized)); CHECK(copies == 1); before.check(source);
}

TEST_CASE("Offset typed staging rejects the formerly silent double mirror refusal", "[lc-offsetcurve][transform-tool]") {
    auto source = transformRing(); const Snapshot before(*source);
    for (const bool forcePlain : {false, true}) {
        auto made = tx::prepare(*source, {Step::mirror({0, 0}, {0, 0})}, forcePlain);
        CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::InvalidSteps);
        CHECK(made.failedStep == 0); before.check(*source);
    }
    auto made = tx::prepare(*source, {Step::mirror({0, 0}, {1e-300, 0})});
    auto expected = source->toPlainSpline(); expected->mirror({0, 0}, {1, 0});
    cache(asSpline(made).getData(), expected->getData()); before.check(*source);
}

TEST_CASE("Offset typed staging retains native reverse when actual cache agrees", "[lc-offsetcurve][transform-tool]") {
    auto source = transformRing(); const Snapshot before(*source);
    auto made = tx::prepare(*source, {Step::reverse()}); REQUIRE(made); CHECK_FALSE(made.keptPlain);
    const auto *native = dynamic_cast<const LC_OffsetCurve *>(made.entity.get()); REQUIRE(native);
    CHECK(native->definition().cacheReversed != source->definition().cacheReversed);
    auto expected = source->toPlainSpline(); expected->revertDirection();
    cache(native->getData(), expected->getData()); before.check(*source);
}

TEST_CASE("Offset typed staging carries all steps through a permanent plain fallback", "[lc-offsetcurve][transform-tool]") {
    auto source = transformRing(); const Snapshot before(*source);
    auto made = tx::prepare(*source, {Step::rotate({0, 0}, .3), Step::move({4, 8}), Step::reverse()});
    REQUIRE(made); CHECK(made.keptPlain);
    auto expected = source->toPlainSpline(); expected->rotate({0, 0}, .3); expected->move({4, 8}); expected->revertDirection();
    cache(asSpline(made).getData(), expected->getData()); before.check(*source);
}

TEST_CASE("Offset typed staging does not treat rounded-away nonidentity equality as native success", "[lc-offsetcurve][transform-tool]") {
    auto source = transformRing();
    auto shifted = tx::prepare(*source, {Step::move({4, 8})}); REQUIRE(shifted);
    const auto *native = dynamic_cast<const LC_OffsetCurve *>(shifted.entity.get()); REQUIRE(native);
    const Snapshot before(*native);
    const double tiny = std::numeric_limits<double>::denorm_min();
    auto made = tx::prepare(*native, {Step::move({tiny, tiny})});
    REQUIRE(made); CHECK(made.keptPlain); CHECK(made.entity->rtti() == RS2::EntitySpline);
    cache(asSpline(made).getData(), native->getData()); before.check(*native);
}

TEST_CASE("Offset typed staging allows finite singular scales only as plain geometry", "[lc-offsetcurve][transform-tool]") {
    auto source = transformRing(); const Snapshot before(*source);
    for (const RS_Vector factor : {RS_Vector{0, 1}, RS_Vector{0, 0}, RS_Vector{1e-300, 1}}) {
        auto made = tx::prepare(*source, {Step::scale({0, 0}, factor)});
        REQUIRE(made); CHECK(made.keptPlain); CHECK(made.entity->rtti() == RS2::EntitySpline);
        auto expected = source->toPlainSpline(); expected->scale({0, 0}, factor);
        cache(asSpline(made).getData(), expected->getData()); before.check(*source);
    }
}

TEST_CASE("Offset typed staging refuses overflow after an earlier successful step", "[lc-offsetcurve][transform-tool]") {
    auto source = transformRing(); const Snapshot before(*source);
    auto made = tx::prepare(*source, {Step::move({4, 8}), Step::scale({0, 0}, {1e308, 1e308}), Step::reverse()});
    CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::InvalidResult);
    CHECK(made.failedStep == 1); before.check(*source);
    RS_Spline plain(nullptr, segmentData()); const Snapshot plainBefore(plain);
    made = tx::prepare(plain, {Step::scale({0, 0}, {1e308, 1e308})});
    CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::InvalidResult);
    plainBefore.check(plain);
}

TEST_CASE("Offset typed staging demotes valid stale data without changing source", "[lc-offsetcurve][transform-tool]") {
    auto source = transformRing(); source->getData().weights[1] = 2;
    CHECK_FALSE(source->isConsistent()); const Snapshot before(*source);
    auto made = tx::prepare(*source, {Step::move({4, 8})});
    REQUIRE(made); CHECK(made.keptPlain); CHECK(made.entity->rtti() == RS2::EntitySpline);
    auto expected = source->toPlainSpline(); expected->move({4, 8});
    cache(asSpline(made).getData(), expected->getData()); before.check(*source);
}

TEST_CASE("Offset typed staging rejects finite reversed active domains before copying", "[lc-offsetcurve][transform-tool]") {
    auto source = transformRing(); auto bad = segmentData();
    bad.type = RS_SplineData::SplineType::Standard;
    bad.knotslist = {-1, .5 * RS_TOLERANCE, 0, 1}; source->getData() = bad;
    const Snapshot before(*source);
    auto made = tx::prepare(*source, {Step::move({4, 8})});
    CHECK_FALSE(made); CHECK(made.refusal == tx::Refusal::InvalidCache); CHECK_FALSE(made.entity);
    before.check(*source);
    bad.knotslist = {-1, 0, 0, 1}; source->getData() = bad;
    const Snapshot zeroDomain(*source);
    made = tx::prepare(*source, {Step::move({4, 8})});
    CHECK_FALSE(made); CHECK(made.refusal == tx::Refusal::InvalidCache); zeroDomain.check(*source);
}

TEST_CASE("Offset typed staging rejects malformed representation fields and resource caps", "[lc-offsetcurve][transform-tool]") {
    for (unsigned malformed = 0; malformed < 13; ++malformed) {
        CAPTURE(malformed);
        auto source = transformRing(); auto &data = source->getData();
        switch (malformed) {
        case 0: data.degree = std::numeric_limits<std::size_t>::max(); break;
        case 1: data.type = static_cast<RS_SplineData::SplineType>(99); break;
        case 2: data.savedOpenType = static_cast<RS_SplineData::SplineType>(99); break;
        case 3: data.weights.pop_back(); break;
        case 4: data.weights[1] = 0; break;
        case 5: data.weights[1] = std::numeric_limits<double>::quiet_NaN(); break;
        case 6: data.controlPoints[0].z = 1e-300; break;
        case 7: data.knotslist[4] = 1; data.knotslist[5] = 1 - .5 * RS_TOLERANCE; break;
        case 8: data.savedOpenKnots = {0, 0, 1, 1}; break;
        case 9: data.controlPoints.resize(LC_OffsetCheckLimits{}.maxVertices + 1, {0, 0}); break;
        case 10: data.fitPoints.resize(LC_OffsetCheckLimits{}.maxVertices + 1, {0, 0}); break;
        case 11: data.knotslist.resize(4 * LC_OffsetCheckLimits{}.maxVertices + 1, 0); break;
        case 12: data.savedOpenKnots.resize(4 * LC_OffsetCheckLimits{}.maxVertices + 1, 0); break;
        }
        const Snapshot before(*source);
        auto made = tx::prepare(*source, {Step::move({4, 8})});
        CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::InvalidCache);
        before.check(*source);
    }
}

TEST_CASE("Offset typed staging refuses direct and nested borrowed plain children", "[lc-offsetcurve][transform-tool]") {
    RS_Line line(nullptr, {1, 2}, {3, 4}); attributes(line);
    RS_EntityContainer borrowed(nullptr, false); borrowed.addEntity(&line); attributes(borrowed);
    const Snapshot directBefore(borrowed);
    auto made = tx::prepare(borrowed, {Step::move({4, 8})});
    CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::UnsafeContainer);
    directBefore.check(borrowed);
    RS_EntityContainer owned;
    auto *nested = new RS_EntityContainer(&owned, false); nested->addEntity(&line); owned.addEntity(nested);
    attributes(owned); const Snapshot nestedBefore(owned);
    made = tx::prepare(owned, {Step::move({4, 8})});
    CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::UnsafeContainer);
    nestedBefore.check(owned);
}

TEST_CASE("Offset typed staging refuses borrowed native groups and demotes owned transparent children", "[lc-offsetcurve][transform-tool]") {
    auto source = transformRing();
    RS_EntityContainer borrowed(nullptr, false); borrowed.addEntity(source.get());
    const Snapshot borrowedBefore(borrowed);
    auto made = tx::prepare(borrowed, {Step::move({4, 8})});
    CHECK_FALSE(made); CHECK(made.refusal == tx::Refusal::UnsafeContainer); borrowedBefore.check(borrowed);
    RS_EntityContainer owned;
    owned.addEntity(source->clone()); const Snapshot ownedBefore(owned);
    made = tx::prepare(owned, {Step::move({4, 8})});
    REQUIRE(made); CHECK(made.keptPlain);
    const auto *group = dynamic_cast<const RS_EntityContainer *>(made.entity.get()); REQUIRE(group);
    REQUIRE(group->count() == 1);
    CHECK(group->entityAt(0)->rtti() == RS2::EntitySpline);
    auto expected = source->toPlainSpline(); expected->move({4, 8});
    const auto *actual = dynamic_cast<const RS_Spline *>(group->entityAt(0)); REQUIRE(actual);
    cache(actual->getData(), expected->getData()); CHECK(actual->getParent() == group);
    ownedBefore.check(owned);
}

TEST_CASE("Offset typed staging detaches ordinary owned children even for small scales", "[lc-offsetcurve][transform-tool]") {
    RS_EntityContainer owned;
    auto *line = new RS_Line(&owned, {1, 2}, {3, 4}); owned.addEntity(line); attributes(owned);
    const Snapshot before(owned);
    auto made = tx::prepare(owned, {Step::scale({0, 0}, {0, 1}), Step::move({4, 8})}); REQUIRE(made);
    const auto *staged = dynamic_cast<const RS_EntityContainer *>(made.entity.get()); REQUIRE(staged);
    REQUIRE(staged->size() == 1);
    const auto *copy = dynamic_cast<const RS_Line *>(*staged->begin()); REQUIRE(copy); CHECK(copy != line);
    point(copy->getStartpoint(), {4, 10}); point(copy->getEndpoint(), {4, 12}); before.check(owned);
}

TEST_CASE("Offset typed staging stops INSERT at its first rejected source edit", "[lc-offsetcurve][transform-tool]") {
    std::vector<char> calls; TrackedInsert source(calls); attributes(source); const Snapshot before(source);
    auto made = tx::prepare(source, {Step::scale({0, 0}, {0, 1}), Step::move({4, 8})});
    CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::SourceEditRejected);
    CHECK(made.failedStep == 0); REQUIRE(calls.size() == 1); CHECK(calls[0] == 'S'); before.check(source);
    calls.clear();
    made = tx::prepare(source, {Step::move({4, 8}), Step::scale({0, 0}, {0, 1}), Step::move({8, 16})});
    CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::SourceEditRejected);
    CHECK(made.failedStep == 1); REQUIRE(calls.size() == 2);
    CHECK(calls[0] == 'M'); CHECK(calls[1] == 'S'); before.check(source);
    calls.clear();
    auto control = tx::prepare(source, {Step::move({4, 8})}); REQUIRE(control);
    REQUIRE(calls.size() == 1); CHECK(calls[0] == 'M');
    const auto *insert = dynamic_cast<const RS_Insert *>(control.entity.get()); REQUIRE(insert);
    CHECK(insert->lastSourceEditSucceeded()); point(insert->getData().insertionPoint, {7, 12}); before.check(source);
}

TEST_CASE("Offset typed staging checks nested INSERT failures and legitimate prior-error identities", "[lc-offsetcurve][transform-tool]") {
    std::vector<char> calls;
    RS_EntityContainer owned; auto *insert = new TrackedInsert(calls); owned.addEntity(insert);
    const Snapshot before(owned);
    auto made = tx::prepare(owned, {Step::scale({0, 0}, {0, 1}), Step::move({4, 8})});
    CHECK_FALSE(made); CHECK(made.refusal == tx::Refusal::SourceEditRejected); CHECK(made.failedStep == 0);
    REQUIRE(calls.size() == 1); CHECK(calls[0] == 'S'); before.check(owned);
    insert->scale({0, 0}, {0, 1}); REQUIRE_FALSE(insert->lastSourceEditSucceeded());
    const Snapshot priorError(*insert); calls.clear();
    made = tx::prepare(*insert, {Step::move({0, 0}), Step::rotate({0, 0}, 0)});
    REQUIRE(made); CHECK(calls.empty()); priorError.check(*insert);
    made = tx::prepare(*insert, {Step::move({4, 8})}); REQUIRE(made);
    const auto *edited = dynamic_cast<const RS_Insert *>(made.entity.get()); REQUIRE(edited);
    CHECK(edited->lastSourceEditSucceeded()); priorError.check(*insert);
}

TEST_CASE("Offset typed staging refuses INSERT cache-only shear and reverse", "[lc-offsetcurve][transform-tool]") {
    std::vector<char> calls; TrackedInsert source(calls); const Snapshot before(source);
    for (const auto &step : {Step::shear(.25), Step::reverse()}) {
        auto made = tx::prepare(source, {Step::move({4, 8}), step});
        CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::SourceEditRejected);
        CHECK(made.failedStep == 1); CHECK(calls.empty()); before.check(source);
    }
}

TEST_CASE("Offset typed staging refuses native group circle overflow without publishing a prefix", "[lc-offsetcurve][transform-tool]") {
    for (const bool prefix : {false, true}) {
        CAPTURE(prefix);
        RS_EntityContainer owned;
        auto native = transformRing(); native->reparent(&owned); owned.addEntity(native.release());
        auto *circle = new RS_Circle(&owned, RS_CircleData{{0, 0}, 1e308}); owned.addEntity(circle);
        REQUIRE(circle->hasValidBorders()); const Snapshot before(owned);
        tx::Steps steps;
        if (prefix) REQUIRE(steps.append(Step::move({4, 8})));
        REQUIRE(steps.append(Step::scale({0, 0}, {2, 2})));
        REQUIRE(steps.append(Step::move({8, 16})));
        const auto made = tx::prepare(owned, steps);
        CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::InvalidResult);
        CHECK(made.failedStep == (prefix ? 1 : 0)); before.check(owned);
    }
}

TEST_CASE("Offset typed staging validates arc ellipse and point siblings after each step", "[lc-offsetcurve][transform-tool]") {
    for (unsigned kind = 0; kind < 3; ++kind) {
        CAPTURE(kind);
        RS_EntityContainer owned;
        auto native = transformRing(); native->reparent(&owned); owned.addEntity(native.release());
        RS_Entity *sibling = nullptr;
        if (kind == 0) sibling = new RS_Arc(&owned, {{1e308, 0}, 1, 0, .5, false});
        if (kind == 1) sibling = new RS_Ellipse(&owned, {{1e308, 0}, {1, 0}, .5});
        if (kind == 2) sibling = new RS_Point(&owned, RS_Vector{1e308, 0});
        REQUIRE(sibling); owned.addEntity(sibling); REQUIRE(sibling->hasValidBorders());
        const Snapshot before(owned);
        const auto made = tx::prepare(owned, {Step::move({4, 8}), Step::scale({0, 0}, {2, 2}), Step::move({8, 16})});
        CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::InvalidResult);
        CHECK(made.failedStep == 1); before.check(owned);
    }
}

TEST_CASE("Offset typed staging checks primitive fields and actual bounds before cloning", "[lc-offsetcurve][transform-tool]") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    for (unsigned kind = 0; kind < 5; ++kind) {
        CAPTURE(kind);
        RS_EntityContainer owned;
        auto native = transformRing(); native->reparent(&owned); owned.addEntity(native.release());
        RS_Entity *sibling = nullptr;
        if (kind == 0) {
            auto *arc = new RS_Arc(&owned, {{0, 0}, 1, 0, .5, false});
            arc->getData().angle1 = nan; REQUIRE(arc->hasValidBorders()); sibling = arc;
        }
        if (kind == 1) {
            auto *ellipse = new RS_Ellipse(&owned, {{0, 0}, {2, 0}, .5});
            ellipse->setRatio(nan); sibling = ellipse;
        }
        if (kind == 2) sibling = new RS_Point(&owned, RS_Vector{1, 2, 1e-300});
        if (kind == 3) sibling = new RS_Circle(&owned, RS_CircleData{{1e308, 0}, 1e308});
        if (kind == 4) {
            auto *circle = new RS_Circle(&owned, RS_CircleData{{0, 0}, 1});
            circle->setRadius(nan); sibling = circle;
        }
        REQUIRE(sibling); owned.addEntity(sibling);
        const Snapshot before(owned);
        const auto made = tx::prepare(owned, {Step::move({4, 8})});
        CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::InvalidResult);
        CHECK(made.failedStep == tx::MaximumSteps); before.check(owned);
    }
}

TEST_CASE("Offset typed staging retains finite ordinary primitive siblings", "[lc-offsetcurve][transform-tool]") {
    RS_EntityContainer owned;
    auto native = transformRing(); native->reparent(&owned); owned.addEntity(native.release());
    owned.addEntity(new RS_Circle(&owned, RS_CircleData{{3, 4}, 2}));
    owned.addEntity(new RS_Arc(&owned, {{3, 4}, 2, 0, .5, false}));
    owned.addEntity(new RS_Ellipse(&owned, {{3, 4}, {2, 0}, .5}));
    owned.addEntity(new RS_Point(&owned, RS_Vector{3, 4}));
    const Snapshot before(owned);
    const auto made = tx::prepare(owned, {Step::move({4, 8}), Step::scale({0, 0}, {2, 2})});
    REQUIRE(made); CHECK(made.keptPlain);
    const auto *group = dynamic_cast<const RS_EntityContainer *>(made.entity.get()); REQUIRE(group);
    REQUIRE(group->size() == 5);
    const auto *circle = dynamic_cast<const RS_Circle *>(group->entityAt(1)); REQUIRE(circle);
    point(circle->getCenter(), {14, 24}); number(circle->getRadius(), 4);
    const auto *arc = dynamic_cast<const RS_Arc *>(group->entityAt(2)); REQUIRE(arc);
    point(arc->getCenter(), {14, 24}); number(arc->getRadius(), 4);
    const auto *ellipse = dynamic_cast<const RS_Ellipse *>(group->entityAt(3)); REQUIRE(ellipse);
    point(ellipse->getCenter(), {14, 24}); point(ellipse->getMajorP(), {4, 0}); number(ellipse->getRatio(), .5);
    const auto *dot = dynamic_cast<const RS_Point *>(group->entityAt(4)); REQUIRE(dot); point(dot->getPos(), {14, 24});
    for (const auto *child : *group) CHECK(child->hasValidBorders());
    before.check(owned);
}

TEST_CASE("Offset typed staging leaves ordinary INSERT text and image carrier policy unchanged", "[lc-offsetcurve][transform-tool]") {
    RS_Insert source(nullptr, insertedData());
    RS_TextData textData;
    textData.insertionPoint = {1, 2}; textData.secondPoint = {2, 2}; textData.height = 1;
    textData.widthRel = 1; textData.updateMode = RS2::NoUpdate;
    source.addEntity(new RS_Text(&source, textData));
    source.appendEntity(new RS_Image(&source, {0, {1, 2}, {1, 0}, {0, 1}, {2, 3}, QString{}, 50, 50, 0}));
    const Snapshot before(source);
    const auto identity = tx::prepare(source, {}); REQUIRE(identity);
    const auto *unchanged = dynamic_cast<const RS_Insert *>(identity.entity.get()); REQUIRE(unchanged);
    REQUIRE(unchanged->size() == 2); CHECK(unchanged->entityAt(0)->rtti() == RS2::EntityText);
    CHECK(unchanged->entityAt(1)->rtti() == RS2::EntityImage);
    auto expected = std::unique_ptr<RS_Entity>{source.clone()}; expected->move({4, 8});
    const auto moved = tx::prepare(source, {Step::move({4, 8})}); REQUIRE(moved);
    const auto *actual = dynamic_cast<const RS_Insert *>(moved.entity.get()); REQUIRE(actual);
    const auto *ordinary = dynamic_cast<const RS_Insert *>(expected.get()); REQUIRE(ordinary);
    insertData(actual->getData(), ordinary->getData()); CHECK(actual->size() == ordinary->size()); before.check(source);
}

TEST_CASE("Offset typed staging refuses unknown native sibling carriers independent of order", "[lc-offsetcurve][transform-tool]") {
    for (const bool nativeFirst : {false, true}) {
        RS_EntityContainer owned;
        auto native = transformRing(); native->reparent(&owned);
        if (nativeFirst) owned.addEntity(native.release());
        owned.appendEntity(new RS_Image(&owned, {0, {1, 2}, {1, 0}, {0, 1}, {2, 3}, QString{}, 50, 50, 0}));
        if (!nativeFirst) owned.addEntity(native.release());
        const Snapshot before(owned);
        const auto made = tx::prepare(owned, {Step::move({4, 8})});
        CHECK_FALSE(made); CHECK_FALSE(made.entity); CHECK(made.refusal == tx::Refusal::UnsafeContainer);
        CHECK(made.failedStep == tx::MaximumSteps); before.check(owned);
    }
}
