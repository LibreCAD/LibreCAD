/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <QApplication>
#include <QComboBox>
#include <QEventLoop>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPointer>
#include <QTimer>
#include <QToolButton>
#include <cstring>
#include <limits>
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>
#include "lc_actioncontext.h"
#include "lc_actiongroupmanager.h"
#include "lc_actiontestsupport.h"
#include "lc_offsetcurvepropertyqueue.h"
#include "lc_offsetcurveui.h"
#include "lc_offsetcurvereadutils.h"
#include "lc_property_lineedit_with_button.h"
#include "rs_layer.h"
#include "drw_base.h"
#include "lc_properties_sheet.h"
#include "lc_property_bool.h"
#include "lc_property_container.h"
#include "lc_property_double.h"
#include "lc_property_linewidth.h"
#include "lc_propertysheetwidget.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_math.h"

namespace {
class Graphic final : public RS_Graphic {
public:
    using RS_Graphic::countUndoCycles;
};

class Context final : public LC_ActionContext {
public:
    void commandMessage(const QString& message) override { messages.append(message); }
    QStringList messages;
    RS2::ActionType interactiveAction = RS2::ActionNone;
    void setCurrentAction(RS2::ActionType type, void*) override { interactiveAction = type; }
};

QList<LC_Property*> namedRows(const LC_PropertyContainer* root, const QString& name) {
    QList<LC_Property*> rows;
    for (auto* row : root->findChildren<LC_Property*>()) {
        if (row->getName() == name || row->getName().endsWith('$' + name)) rows.append(row);
    }
    return rows;
}

void waitForCommit() {
    QEventLoop loop;
    QTimer::singleShot(80, &loop, &QEventLoop::quit);
    loop.exec();
}
void key(QWidget& widget, int code, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
    for (auto type : {QEvent::KeyPress, QEvent::KeyRelease}) {
        QKeyEvent event(type, code, modifiers);
        QApplication::sendEvent(&widget, &event);
    }
}
void type(QWidget& widget, const QString& text) {
    for (auto character : text) {
        QKeyEvent event(QEvent::KeyPress, 0, Qt::NoModifier, QString(character));
        QApplication::sendEvent(&widget, &event);
    }
}

LC_OffsetCurve* addArc(Graphic& graphic, RS_Vector center = {0, 0}, double distance = .25) {
    LC_EllipseOffsetDefinition definition;
    definition.basis = RS_EllipseData{center, {1, 0}, 1, 0, .1, false};
    definition.distance = distance;
    definition.relTolerance = .01;
    const double radius = 1 + distance;
    const RS_Line cache{nullptr, center + RS_Vector{radius, 0},
        center + RS_Vector{radius * std::cos(.1), radius * std::sin(.1)}};
    auto curve = LC_OffsetCurve::adopt(cache, definition, .01);
    REQUIRE(curve);
    curve->setParent(&graphic);
    curve->setLayer(graphic.getActiveLayer());
    auto* result = curve.release();
    graphic.addEntity(result);
    return result;
}
LC_OffsetCurve* addWhole(Graphic& graphic) {
    const RS_Ellipse source{&graphic, RS_EllipseData{{0, 0}, {5, 0}, 1}};
    auto made = LC_OffsetCurve::create(source, 1, false, {}, 1e-4);
    REQUIRE(made);
    made.entity->setParent(&graphic);
    made.entity->setLayer(graphic.getActiveLayer());
    auto* result = made.entity.release();
    graphic.addEntity(result);
    return result;
}

struct Snapshot {
    std::uint64_t fingerprint;
    LC_EllipseOffsetDefinition definition;
    unsigned long long id;
    const RS_Vector* controls;
    RS_Layer* layer;
    RS_Pen pen;
    bool defined, consistent;
    std::vector<const RS_Entity*> children;
    explicit Snapshot(const LC_OffsetCurve& curve)
        : fingerprint(RS_Spline::geometryFingerprint(curve.getData())), definition(curve.definition()),
          id(curve.getId()), controls(curve.getData().controlPoints.data()), layer(curve.getLayer()),
          pen(curve.getPen(false)), defined(curve.isDefined()), consistent(curve.isConsistent()) {
        for (const auto* child : curve) children.push_back(child);
    }
    void check(const LC_OffsetCurve& curve, bool exactStorage = true) const {
        CHECK(RS_Spline::geometryFingerprint(curve.getData()) == fingerprint);
        if (exactStorage) CHECK(curve.getData().controlPoints.data() == controls);
        CHECK(curve.getId() == id);
        CHECK(curve.getLayer() == layer);
        CHECK(curve.getPen(false) == pen);
        CHECK(curve.isDefined() == defined);
        CHECK(curve.isConsistent() == consistent);
        const auto& now = curve.definition();
        CHECK(now.distance == definition.distance);
        CHECK(now.towardCentre == definition.towardCentre);
        CHECK(now.cacheReversed == definition.cacheReversed);
        CHECK(now.relTolerance == definition.relTolerance);
        CHECK(now.basis.center.x == definition.basis.center.x);
        CHECK(now.basis.center.y == definition.basis.center.y);
        CHECK(now.basis.majorP.x == definition.basis.majorP.x);
        CHECK(now.basis.majorP.y == definition.basis.majorP.y);
        CHECK(now.basis.ratio == definition.basis.ratio);
        CHECK(now.basis.angle1 == definition.basis.angle1);
        CHECK(now.basis.angle2 == definition.basis.angle2);
        CHECK(now.basis.reversed == definition.basis.reversed);
        REQUIRE(curve.count() == children.size());
        if (exactStorage)
            for (std::size_t i = 0; i < children.size(); ++i) CHECK(curve.entityAt(static_cast<int>(i)) == children[i]);
    }
};

struct Fixture {
    QApplication* app = lc::test::application();
    Graphic graphic;
    Context context;
    LC_ActionGroupManager actions{nullptr};
    lc::test::TestGraphicView view;
    std::unique_ptr<LC_PropertySheetWidget> sheet;
    Fixture() {
        REQUIRE(app != nullptr);
        graphic.initForNewDocument();
        view.setDocument(&graphic);
        context.setDocumentAndView(&graphic, &view);
        sheet = std::make_unique<LC_PropertySheetWidget>(nullptr, &context, &actions);
        sheet->getOptions()->noSelectionWorkspace = false;
        sheet->getOptions()->noSelectionGraphicView = false;
        sheet->getOptions()->showComputed = true;
        sheet->getOptions()->showSingleEntityCommands = false;
        sheet->getOptions()->showMultiEntityCommands = false;
        sheet->resize(620, 900);
    }
    ~Fixture() {
        if (sheet) sheet->setGraphicView(nullptr);
        context.setDocumentAndView(nullptr, nullptr);
    }
    void show(const QList<RS_Entity*>& entities) {
        graphic.select(entities);
        sheet->show();
        sheet->setGraphicView(&view);
        QApplication::processEvents();
        auto* root = properties()->getPropertyContainer();
        REQUIRE(root);
        for (auto* child : root->childProperties()) child->expand();
        QApplication::processEvents();
    }
    LC_PropertiesSheet* properties() const {
        auto* result = sheet->findChild<LC_PropertiesSheet*>();
        REQUIRE(result);
        return result;
    }
    LC_Property* property(const QString& name) const {
        const auto rows = namedRows(properties()->getPropertyContainer(), name);
        QStringList available;
        for (auto* row : properties()->getPropertyContainer()->findChildren<LC_Property*>())
            available.append(row->getName());
        INFO("Requested " << name.toStdString() << "; available: " << available.join(',').toStdString());
        REQUIRE(rows.size() == 1);
        return rows.front();
    }
    LC_OffsetCurvePropertyQueue* queue() const { return sheet->offsetPropertyQueue(); }
    QLineEdit* beginNumber(const QString& name) {
        auto* props = properties();
        props->stopInplaceEdit(false, false);
        auto* row = property(name);
        REQUIRE(row->isEditableByUser());
        for (auto* parent = row->parent(); parent != nullptr; parent = parent->parent())
            if (auto* container = qobject_cast<LC_PropertyContainer*>(parent)) container->expand();
        props->setActiveProperty(row, true);
        REQUIRE(props->activeProperty() == row);
        // Event routing uses the painted value parts; a refill invalidates them.
        // Synchronous repaint avoids flushing the delayed native commit timer.
        props->viewport()->repaint();
        props->setFocus();
        const auto oldEditors = props->findChildren<QLineEdit*>();
        key(*props, Qt::Key_Return);
        for (auto* editor : props->findChildren<QLineEdit*>()) {
            if (!oldEditors.contains(editor) && editor->isVisible()) return editor;
        }
        FAIL("The real property view did not create a visible QLineEdit");
        return nullptr;
    }
    void editNumber(const QString& name, const QString& text, bool useReturn = true) {
        QPointer<QLineEdit> editor = beginNumber(name);
        editor->setFocus();
        key(*editor, Qt::Key_A, Qt::ControlModifier);
        type(*editor, text);
        if (useReturn) key(*editor, Qt::Key_Return);
        if (editor) {
            QFocusEvent focusOut(QEvent::FocusOut, Qt::TabFocusReason);
            QApplication::sendEvent(editor, &focusOut);
        }
        // No event-loop wait here: rapid-field tests must share the one delayed batch.
    }
    void toggleSide() {
        auto* props = properties();
        props->stopInplaceEdit(false, false);
        props->setActiveProperty(property("towardCentre"), true);
        props->setFocus();
        key(*props, Qt::Key_Space);
    }
};

void ranOnce(const LC_OffsetCurveEditResult& result) {
    REQUIRE(result);
    CHECK(result.changed);
    CHECK(result.engineRuns == 1);
    REQUIRE(result.engineStatus);
    CHECK(*result.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.checkReason);
    CHECK(*result.checkReason == LC_OffsetCheckResult::Reason::None);
    CHECK(result.checkWork.cacheSpans > 0);
}
std::vector<LC_OffsetCurve*> live(const Graphic& graphic) {
    std::vector<LC_OffsetCurve*> result;
    for (auto* entity : graphic) {
        if (!entity->isDeleted() && entity->rtti() == RS2::EntityOffsetCurve) {
            result.push_back(static_cast<LC_OffsetCurve*>(entity));
        }
    }
    return result;
}
}

TEST_CASE("Native sheet population has definition rows and no spline control net", "[gui][properties][offsetcurve]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    f.show({source});
    for (const auto* name : {"distance", "towardCentre", "centerX", "centerY", "radiusMajor", "radiusMinor",
                             "angle", "angle1", "angle2", "length", "definitionState", "layer", "color", "linetype", "linewidth"}) {
        CHECK(f.property(name) != nullptr);
    }
    for (const auto* name : {"degree", "closed", "controlPoints", "weights", "splineCommands"}) {
        CHECK(namedRows(f.properties()->getPropertyContainer(), name).isEmpty());
    }
    CHECK(f.queue()->lastAttempts().empty());
    CHECK(f.queue()->pendingCount() == 0);
    CHECK(f.graphic.countUndoCycles() == 0);
    CHECK_FALSE(f.property("length")->isEditableByUser());
    CHECK(static_cast<LC_PropertyDouble*>(f.property("length"))->value() == source->getLength());
    // Focusing and leaving an exact/rounded display is not a geometry edit.
    auto* editor = f.beginNumber("distance");
    QFocusEvent focusOut(QEvent::FocusOut, Qt::TabFocusReason);
    QApplication::sendEvent(editor, &focusOut);
    waitForCommit();
    CHECK(f.queue()->lastAttempts().empty());
    CHECK(f.graphic.countUndoCycles() == 0);
}

TEST_CASE("Real native distance editor stages once and commits one replacement with undo redo", "[gui][properties][offsetcurve]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    const Snapshot before(*source);
    f.show({source});
    f.editNumber("distance", "0.5");
    REQUIRE(f.queue()->lastAttempts().size() == 1);
    ranOnce(f.queue()->lastAttempts().front().result);
    CHECK(f.queue()->pendingCount() == 1);
    before.check(*source);
    const auto result = f.queue()->lastAttempts().front().result;
    waitForCommit();
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(live(f.graphic).size() == 1);
    auto* replacement = live(f.graphic).front();
    CHECK(replacement != source);
    CHECK(replacement->definition().distance == .5);
    CHECK(replacement->hasEditableDefinition());
    CHECK(f.queue()->lastAttempts().front().result.engineRuns == result.engineRuns);
    REQUIRE(f.graphic.undo());
    CHECK(live(f.graphic).front() == source);
    before.check(*source, false); // Undo rebuilds the generated display cache.
    REQUIRE(f.graphic.redo());
    CHECK(live(f.graphic).front() == replacement);
    CHECK(replacement->definition().distance == .5);
}

TEST_CASE("Native editor restores refusals parse errors and exact no ops without undo", "[gui][properties][offsetcurve]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    const Snapshot before(*source);
    f.show({source});
    SECTION("invalid negative distance is not made positive") {
        f.editNumber("distance", "-0.5", false); // actual focus-out completion
        REQUIRE(f.queue()->lastAttempts().size() == 1);
        const auto& result = f.queue()->lastAttempts().front().result;
        CHECK(result.refusal == LC_OffsetCurveRefusal::InvalidInput);
        CHECK(result.engineRuns == 0);
        CHECK_FALSE(result.engineStatus);
        CHECK_FALSE(result.checkReason);
    }
    SECTION("parse failure has no native attempt") {
        f.editNumber("distance", "not_a_number");
        CHECK(f.queue()->lastAttempts().empty());
        REQUIRE_FALSE(f.context.messages.isEmpty());
        CHECK(f.context.messages.back().contains("finite"));
    }
    SECTION("exact same value does not stage") {
        f.editNumber("distance", "0.25");
        CHECK(f.queue()->lastAttempts().empty());
    }
    CHECK(static_cast<LC_PropertyDouble*>(f.property("distance"))->value() == .25);
    CHECK(f.queue()->pendingCount() == 0);
    waitForCommit();
    CHECK(f.graphic.countUndoCycles() == 0);
    before.check(*source);
}

TEST_CASE("Native multi row retains requested payload when the first target refuses", "[gui][properties][offsetcurve]") {
    for (bool invalidFirst : {true, false}) {
        Fixture f;
        auto* invalid = addArc(f.graphic);
        invalid->getData().controlPoints.front().x += .25;
        auto* valid = addArc(f.graphic, {3, 0});
        const Snapshot before(*invalid);
        f.show(invalidFirst ? QList<RS_Entity*>{invalid, valid} : QList<RS_Entity*>{valid, invalid});
        REQUIRE(f.property("distance")->isEditableByUser());
        f.editNumber("distance", "0.5");
        REQUIRE(f.queue()->lastAttempts().size() == 2);
        for (const auto& attempt : f.queue()->lastAttempts()) {
            if (attempt.original == invalid) {
                CHECK(attempt.result.refusal == LC_OffsetCurveRefusal::Inconsistent);
                CHECK(attempt.result.engineRuns == 0);
                CHECK_FALSE(attempt.result.engineStatus);
                CHECK_FALSE(attempt.result.checkReason);
            }
            else ranOnce(attempt.result);
        }
        before.check(*invalid);
        REQUIRE_FALSE(f.context.messages.isEmpty());
        CHECK(f.context.messages.back().contains("1 changed, 0 unchanged, 1 refused"));
        waitForCommit();
        CHECK(f.graphic.countUndoCycles() == 1);
        before.check(*invalid);
        CHECK_FALSE(invalid->isDeleted());
        REQUIRE(live(f.graphic).size() == 2);
        for (auto* curve : live(f.graphic)) if (curve != invalid) CHECK(curve->definition().distance == .5);
    }
}

TEST_CASE("A mixed native value matching the first target still edits the other target", "[gui][properties][offsetcurve]") {
    Fixture f;
    auto* first = addArc(f.graphic, {0, 0}, .25);
    auto* second = addArc(f.graphic, {3, 0}, .5);
    const Snapshot before(*first);
    f.show({first, second});
    CHECK(f.property("distance")->isMultiValue());
    f.editNumber("distance", "0.25");
    REQUIRE(f.queue()->lastAttempts().size() == 2);
    const auto& noOp = f.queue()->lastAttempts()[0].result;
    CHECK(noOp);
    CHECK_FALSE(noOp.changed);
    CHECK(noOp.engineRuns == 0);
    CHECK_FALSE(noOp.engineStatus);
    CHECK_FALSE(noOp.checkReason);
    ranOnce(f.queue()->lastAttempts()[1].result);
    before.check(*first);
    waitForCommit();
    REQUIRE(live(f.graphic).size() == 2);
    for (auto* curve : live(f.graphic)) CHECK(curve->definition().distance == .25);
    CHECK_FALSE(first->isDeleted());
    CHECK(second->isDeleted());
    CHECK(f.graphic.countUndoCycles() == 1);
}

TEST_CASE("Native undefined rows are disabled but valid peers and attributes remain editable", "[gui][properties][offsetcurve]") {
    Fixture f;
    auto* invalid = addArc(f.graphic);
    invalid->scale({0, 0}, {2, 3});
    REQUIRE_FALSE(invalid->isDefined());
    const Snapshot before(*invalid);
    SECTION("undefined single selection") {
        f.show({invalid});
        CHECK_FALSE(f.property("distance")->isEditableByUser());
        CHECK_FALSE(f.property("centerX")->isEditableByUser());
        CHECK(f.property("linewidth")->isEditableByUser());
        auto* width = static_cast<LC_PropertyLineWidth*>(f.property("linewidth"));
        REQUIRE(width->setValue(RS2::Width07, PropertyChangeReasonEdit));
        REQUIRE(f.queue()->lastAttempts().size() == 1);
        const auto& result = f.queue()->lastAttempts().front().result;
        CHECK(result);
        CHECK(result.changed);
        CHECK(result.engineRuns == 0);
        CHECK_FALSE(result.engineStatus);
        CHECK_FALSE(result.checkReason);
        waitForCommit();
        REQUIRE(live(f.graphic).size() == 1);
        CHECK_FALSE(live(f.graphic).front()->isDefined());
        CHECK(live(f.graphic).front()->getPen(false).getWidth() == RS2::Width07);
    }
    SECTION("undefined first and valid later") {
        auto* valid = addArc(f.graphic, {3, 0});
        f.show({invalid, valid});
        REQUIRE(f.property("distance")->isEditableByUser());
        f.editNumber("distance", "0.5");
        REQUIRE(f.queue()->lastAttempts().size() == 2);
        CHECK(f.queue()->lastAttempts()[0].result.refusal == LC_OffsetCurveRefusal::Undefined);
        ranOnce(f.queue()->lastAttempts()[1].result);
        waitForCommit();
        before.check(*invalid);
        CHECK_FALSE(invalid->isDeleted());
    }
}

TEST_CASE("Rapid native geometry side and attributes compose without erasing an earlier accepted field", "[gui][properties][offsetcurve]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    f.show({source});
    f.editNumber("distance", "0.5");
    ranOnce(f.queue()->lastAttempts().front().result);
    f.toggleSide();
    ranOnce(f.queue()->lastAttempts().front().result);
    auto* width = static_cast<LC_PropertyLineWidth*>(f.property("linewidth"));
    REQUIRE(width->setValue(RS2::Width07, PropertyChangeReasonEdit));
    CHECK(f.queue()->lastAttempts().front().result.engineRuns == 0);
    // A later refused field must not remove the accepted candidate or its pen.
    f.editNumber("distance", "-1");
    CHECK(f.queue()->lastAttempts().front().result.refusal == LC_OffsetCurveRefusal::InvalidInput);
    CHECK(f.queue()->pendingCount() == 1);
    CHECK(source->definition().distance == .25);
    waitForCommit();
    REQUIRE(live(f.graphic).size() == 1);
    auto* replacement = live(f.graphic).front();
    CHECK(replacement->definition().distance == .5);
    CHECK(replacement->definition().towardCentre);
    CHECK(replacement->getPen(false).getWidth() == RS2::Width07);
    CHECK(f.graphic.countUndoCycles() == 1);
}

TEST_CASE("Native sheet preserves raw parameters and each peer's other coordinate in a rotated UCS", "[gui][properties][offsetcurve]") {
    Fixture f;
    auto* first = addArc(f.graphic, {0, 0});
    auto* second = addArc(f.graphic, {8, 4});
    f.view.getViewPort()->createUCS({2, 3}, .4);
    f.show({first, second});
    const double oldY1 = f.view.getViewPort()->toUCS(first->definition().basis.center).y;
    const double oldY2 = f.view.getViewPort()->toUCS(second->definition().basis.center).y;
    f.editNumber("centerX", "5");
    REQUIRE(f.queue()->pendingCount() == 2);
    for (auto* source : {first, second}) {
        const auto* candidate = f.queue()->value(f.queue()->epoch(), source);
        REQUIRE(candidate);
        const auto center = f.view.getViewPort()->toUCS(candidate->definition().basis.center);
        CHECK(std::abs(center.x - 5) < 1e-12);
        CHECK(std::abs(center.y - (source == first ? oldY1 : oldY2)) < 1e-12);
    }
    f.editNumber("angle2", "12");
    REQUIRE(f.queue()->lastAttempts().size() == 2);
    for (const auto& attempt : f.queue()->lastAttempts()) ranOnce(attempt.result);
    for (auto* source : {first, second}) {
        const auto* candidate = f.queue()->value(f.queue()->epoch(), source);
        REQUIRE(candidate);
        CHECK(std::abs(candidate->definition().basis.angle2 - RS_Math::deg2rad(12)) < 1e-12);
        CHECK(candidate->definition().basis.angle1 == 0);
        CHECK(candidate->definition().basis.reversed == source->definition().basis.reversed);
        CHECK(candidate->definition().cacheReversed == source->definition().cacheReversed);
        CHECK(candidate->definition().relTolerance == source->definition().relTolerance);
    }
    waitForCommit();
    CHECK(f.graphic.countUndoCycles() == 1);
}

TEST_CASE("Native sheet radius and rotation edits each return one checked native result", "[gui][properties][offsetcurve]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    f.show({source});
    SECTION("major radius preserves physical minor") {
        f.editNumber("radiusMajor", "2");
        ranOnce(f.queue()->lastAttempts().front().result);
        const auto* curve = f.queue()->value(f.queue()->epoch(), source);
        REQUIRE(curve);
        CHECK(curve->definition().basis.majorP.magnitude() == 2);
        CHECK(curve->definition().basis.ratio == .5);
    }
    SECTION("minor radius changes ratio once") {
        f.editNumber("radiusMinor", "0.8");
        ranOnce(f.queue()->lastAttempts().front().result);
        const auto* curve = f.queue()->value(f.queue()->epoch(), source);
        REQUIRE(curve);
        CHECK(curve->definition().basis.majorP.magnitude() == 1);
        CHECK(curve->definition().basis.ratio == .8);
    }
    SECTION("rotation follows UCS rather than raw parameter semantics") {
        f.view.getViewPort()->createUCS({2, 3}, .4);
        f.sheet->refill();
        for (auto* child : f.properties()->getPropertyContainer()->childProperties()) child->expand();
        f.editNumber("angle", "20");
        ranOnce(f.queue()->lastAttempts().front().result);
        auto* viewport = f.view.getViewPort();
        const double expected = viewport->toWorldAngle(viewport->toAbsUCSAngle(RS_Math::deg2rad(20)));
        const auto* curve = f.queue()->value(f.queue()->epoch(), source);
        REQUIRE(curve);
        CHECK(std::abs(curve->definition().basis.majorP.angle() - expected) < 1e-12);
        CHECK(curve->definition().basis.angle2 == source->definition().basis.angle2);
    }
    waitForCommit();
    CHECK(f.graphic.countUndoCycles() == 1);
}

TEST_CASE("Native sheet reports actual producer and checker exhaustion without retries or publication", "[gui][properties][offsetcurve]") {
    for (bool checker : {false, true}) {
        Fixture f;
        auto* source = addWhole(f.graphic);
        const Snapshot before(*source);
        f.show({source});
        auto limits = LC_OffsetCurveUI::sheetLimits();
        if (checker) limits.checkLimits.maxCacheSpans = 1;
        else limits.maxSamples = 1;
        f.queue()->setRunLimits(limits);
        f.editNumber("distance", "2");
        REQUIRE(f.queue()->lastAttempts().size() == 1);
        const auto& result = f.queue()->lastAttempts().front().result;
        CHECK_FALSE(result);
        CHECK(result.engineRuns == 1);
        REQUIRE(result.engineStatus);
        if (checker) {
            CHECK(result.refusal == LC_OffsetCurveRefusal::CheckFailed);
            CHECK(*result.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(result.checkReason);
            CHECK(*result.checkReason == LC_OffsetCheckResult::Reason::LimitExceeded);
            CHECK(f.context.messages.back().contains("bounded cache validation limit"));
        }
        else {
            CHECK(result.refusal == LC_OffsetCurveRefusal::Engine);
            CHECK(*result.engineStatus == LC_CurveOffsetStatus::LimitExceeded);
            CHECK_FALSE(result.checkReason);
            CHECK(f.context.messages.back().contains("bounded producer limit"));
        }
        CHECK(f.queue()->pendingCount() == 0);
        waitForCommit();
        CHECK(f.graphic.countUndoCycles() == 0);
        before.check(*source);
    }
}

TEST_CASE("Native center picker buttons issue and complete their own scalar tags",
          "[gui][properties][offsetcurve][h-sheet-review]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    f.show({source});
    const bool x = GENERATE(false, true);
    const QString name = x ? "centerX" : "centerY";
    auto* line = f.beginNumber(name);
    auto* editor = dynamic_cast<LC_PropertyLineEditWithButton*>(line->parentWidget());
    REQUIRE(editor);
    auto* button = editor->getToolButton();
    REQUIRE(button->isVisible());
    REQUIRE(button->isEnabled());
    button->click();
    auto* input = f.context.getInteractiveInputInfo();
    REQUIRE(input->state == LC_ActionContext::InteractiveInputInfo::REQUESTED);
    CHECK(input->requestorTag == f.property(name)->getName());
    CHECK(input->inputType == (x ? LC_ActionContext::InteractiveInputInfo::POINT_X :
                                  LC_ActionContext::InteractiveInputInfo::POINT_Y));
    CHECK(f.context.interactiveAction == (x ? RS2::ActionInteractivePickPoint_X :
                                             RS2::ActionInteractivePickPoint_Y));
    REQUIRE(input->requestor == f.sheet.get());
    // Complete the actual request without supplying or repairing its tag.
    input->wcsPoint = {4, 9};
    input->requestor->onLateRequestCompleted(false);
    REQUIRE(f.queue()->lastAttempts().size() == 1);
    const auto& result = f.queue()->lastAttempts().front().result;
    REQUIRE(result);
    CHECK(result.changed);
    CHECK(result.engineRuns <= 1);
    const auto* candidate = f.queue()->value(f.queue()->epoch(), source);
    REQUIRE(candidate);
    CHECK(candidate->definition().basis.center.x == (x ? 4 : 0));
    CHECK(candidate->definition().basis.center.y == (x ? 0 : 9));
    waitForCommit();
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo());
    CHECK_FALSE(source->isDeleted());
    CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("Native pending candidates and stale getters cannot cross detach or sheet destruction", "[gui][properties][offsetcurve]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    f.show({source});
    auto getter = static_cast<LC_PropertyDouble*>(f.property("distance"))->getAsDelegateValue()->getFunValueGet();
    f.editNumber("distance", "0.5");
    REQUIRE(f.queue()->pendingCount() == 1);
    SECTION("detach and reattach same document") {
        f.sheet->setGraphicView(nullptr);
        CHECK(f.queue()->pendingCount() == 0);
        CHECK(getter() == 0);
        f.sheet->setGraphicView(&f.view);
        waitForCommit();
        CHECK(f.graphic.countUndoCycles() == 0);
        CHECK(live(f.graphic).front() == source);
    }
    SECTION("sheet destruction before timeout") {
        f.sheet.reset();
        CHECK(getter() == 0);
        waitForCommit();
        CHECK(f.graphic.countUndoCycles() == 0);
        CHECK(live(f.graphic).front() == source);
    }
}

TEST_CASE("Native pending candidates are dropped when view or document dies", "[gui][properties][offsetcurve]") {
    (void)lc::test::application();
    Context context;
    LC_ActionGroupManager actions(nullptr);
    LC_PropertySheetWidget sheet(nullptr, &context, &actions);
    sheet.getOptions()->noSelectionWorkspace = false;
    sheet.getOptions()->noSelectionGraphicView = false;
    auto graphic = std::make_unique<Graphic>();
    graphic->initForNewDocument();
    auto* source = addArc(*graphic);
    graphic->select(QList<RS_Entity*>{source});
    auto view = std::make_unique<lc::test::TestGraphicView>();
    view->setDocument(graphic.get());
    context.setDocumentAndView(graphic.get(), view.get());
    sheet.show();
    sheet.setGraphicView(view.get());
    auto* props = sheet.findChild<LC_PropertiesSheet*>();
    REQUIRE(props);
    const auto rows = namedRows(props->getPropertyContainer(), "distance");
    REQUIRE(rows.size() == 1);
    auto* property = static_cast<LC_PropertyDouble*>(rows.front());
    auto getter = property->getAsDelegateValue()->getFunValueGet();
    REQUIRE(property->setValue(.5, PropertyChangeReasonEdit));
    REQUIRE(sheet.offsetPropertyQueue()->pendingCount() == 1);
    SECTION("view destroyed") {
        view.reset();
        CHECK(sheet.offsetPropertyQueue()->pendingCount() == 0);
        CHECK(getter() == 0);
        waitForCommit();
        CHECK(graphic->countUndoCycles() == 0);
    }
    SECTION("document destroyed while view remains") {
        graphic.reset();
        CHECK(sheet.offsetPropertyQueue()->pendingCount() == 0);
        CHECK(getter() == 0);
        waitForCommit();
    }
    context.setDocumentAndView(nullptr, nullptr);
    sheet.setGraphicView(nullptr);
}

// UNQUALIFIED review-follow-up cases. No results are claimed by this artifact.
TEST_CASE("Native accepted edits survive same document refill selection and type rows",
          "[gui][properties][offsetcurve][h-sheet-review]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    auto* ordinary = new RS_Line(&f.graphic, {0, 4}, {1, 4});
    ordinary->setLayer(f.graphic.findLayer("0"));
    f.graphic.addEntity(ordinary);
    f.show({source, ordinary});
    auto* combo = f.sheet->findChild<QComboBox*>("cbSelection");
    REQUIRE(combo);
    combo->setCurrentIndex(combo->findData(RS2::EntityOffsetCurve));
    auto getter = static_cast<LC_PropertyDouble*>(f.property("distance"))->getAsDelegateValue()->getFunValueGet();
    f.editNumber("distance", "0.5");
    REQUIRE(f.queue()->pendingCount() == 1);
    const auto oldEpoch = f.queue()->epoch();
    const unsigned change = GENERATE(0u, 1u, 2u, 3u);
    if (change == 0) f.sheet->refill();
    if (change == 1) {
        combo->setCurrentIndex(combo->findData(RS2::EntityContainer));
        combo->setCurrentIndex(combo->findData(RS2::EntityOffsetCurve));
    }
    if (change == 2) f.sheet->setGraphicView(&f.view);
    if (change == 3) {
        f.graphic.unselect(source);
        f.sheet->refill();
    }
    CHECK(f.queue()->epoch() != oldEpoch);
    CHECK(getter() == 0); // the retired row cannot read either original or candidate
    REQUIRE(f.queue()->pendingCount() == 1);
    const auto* pending = f.queue()->value(f.queue()->epoch(), source);
    REQUIRE(pending);
    CHECK(pending->definition().distance == .5);
    waitForCommit();
    REQUIRE(live(f.graphic).size() == 1);
    auto* made = live(f.graphic).front();
    CHECK(made->definition().distance == .5);
    CHECK(made->getFlag(RS2::FlagSelected) == (change != 3));
    CHECK(ordinary->isSelected());
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo());
    CHECK_FALSE(source->isDeleted());
    CHECK(source->definition().distance == .25);
    CHECK_FALSE(f.graphic.undo());
    REQUIRE(f.graphic.redo());
    CHECK(live(f.graphic).front()->definition().distance == .5);
}

TEST_CASE("Native staged replacement never overwrites subsequent in place source changes",
          "[gui][properties][offsetcurve][h-sheet-review]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    auto* otherLayer = new RS_Layer("H_REVIEW_CONCURRENT_LAYER");
    f.graphic.addLayer(otherLayer);
    f.show({source});
    f.editNumber("distance", "0.5");
    REQUIRE(f.queue()->pendingCount() == 1);
    const unsigned change = GENERATE(0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u, 17u);
    INFO("Concurrent source change " << change);
    if (change == 0) source->setPen(RS_Pen(RS_Color(23, 67, 101), RS2::Width07, RS2::DashLine));
    if (change == 1) source->setLayer(otherLayer);
    if (change == 2) REQUIRE(source->setDistance(.75));
    if (change == 3) source->getData().controlPoints.front().x += .5;
    if (change == 4) {
        auto* line = dynamic_cast<RS_Line*>(source->entityAt(0));
        REQUIRE(line);
        line->setEndpoint(line->getEndpoint() + RS_Vector(.25, 0));
    }
    if (change == 5) REQUIRE(source->setOwnedExtensions(0x20, {{1000, QStringLiteral("concurrent")}}));
    if (change == 6) source->setSourceHandle(991);
    if (change == 7) source->setMaterialHandle(992);
    if (change == 8) source->setUserDefVar("concurrent", "kept");
    if (change == 9) source->setDrwExtData({std::make_shared<DRW_Variant>(1000, std::string("concurrent"))});
    if (change == 10) source->setVisible(false);
    if (change == 11) source->setReactorHandles({17, 19});
    if (change == 12) source->entityAt(0)->setVisible(false);
    if (change == 13) source->entityAt(0)->setPen(RS_Pen(RS_Color(9, 23, 47), RS2::Width11, RS2::DashLine));
    if (change == 14) { auto pen = source->getPen(false); pen.setAlpha(.5f); source->setPen(pen); }
    if (change == 15) { auto pen = source->getPen(false); pen.setDashOffset(.125); source->setPen(pen); }
    if (change == 16) {
        auto pen = source->getPen(false);
        auto color = pen.getColor(); color.setColorName("CONCURRENT$COLOR"); pen.setColor(color); source->setPen(pen);
    }
    if (change == 17) {
        auto pen = source->entityAt(0)->getPen(false); pen.setAlpha(.5f); source->entityAt(0)->setPen(pen);
    }
    const auto id = source->getId();
    const auto fingerprint = RS_Spline::geometryFingerprint(source->getData());
    const auto definition = source->definition();
    const auto pen = source->getPen(false);
    const auto layer = source->getLayer(false);
    const auto flags = source->getFlags();
    const auto handle = source->sourceHandle();
    const auto material = source->materialHandle();
    const auto tailFlags = source->unknownOwnedFlags();
    const auto ext = source->getDrwExtData();
    const auto reactors = source->reactorHandles();
    auto* child = source->entityAt(0);
    const auto endpoint = child->getEndpoint();
    const auto childFlags = child->getFlags();
    const auto childPen = child->getPen(false);
    CHECK(f.queue()->value(f.queue()->epoch(), source) == nullptr);
    waitForCommit();
    REQUIRE(f.queue()->pendingCount() == 0);
    CHECK_FALSE(source->isDeleted());
    CHECK(source->getId() == id);
    CHECK(RS_Spline::geometryFingerprint(source->getData()) == fingerprint);
    CHECK(source->definition().distance == definition.distance);
    CHECK(source->getPen(false) == pen);
    CHECK(source->getPen(false).getAlpha() == pen.getAlpha());
    CHECK(source->getPen(false).dashOffset() == pen.dashOffset());
    CHECK(source->getPen(false).getColor().colorName() == pen.getColor().colorName());
    CHECK(source->getLayer(false) == layer);
    CHECK(source->getFlags() == flags);
    CHECK(source->sourceHandle() == handle);
    CHECK(source->materialHandle() == material);
    CHECK(source->unknownOwnedFlags() == tailFlags);
    CHECK(source->getDrwExtData() == ext);
    CHECK(source->reactorHandles() == reactors);
    CHECK(source->getUserDefVar("concurrent") == (change == 8 ? "kept" : ""));
    child = source->entityAt(0);
    REQUIRE(child);
    CHECK(child->getEndpoint().x == endpoint.x);
    CHECK(child->getEndpoint().y == endpoint.y);
    CHECK(child->getFlags() == childFlags);
    CHECK(child->getPen(false) == childPen);
    CHECK(child->getPen(false).getAlpha() == childPen.getAlpha());
    CHECK(f.context.messages.join(" ").contains("source changed", Qt::CaseInsensitive));
    CHECK(f.graphic.countUndoCycles() == 0);
}

TEST_CASE("Native concurrent conflict is detected before composing another staged field",
          "[gui][properties][offsetcurve][h-sheet-review]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    f.show({source});
    f.editNumber("distance", "0.5");
    REQUIRE(f.queue()->pendingCount() == 1);
    source->setPen(RS_Pen(RS_Color(73, 37, 11), RS2::Width11, RS2::SolidLine));
    unsigned invoked = 0;
    f.queue()->apply(f.queue()->epoch(), {source},
        [&invoked](LC_OffsetCurve& candidate, const LC_OffsetRunLimits& limits) {
            ++invoked;
            return candidate.setTowardCentre(true, limits);
        });
    CHECK(invoked == 0);
    CHECK(f.queue()->pendingCount() == 0);
    REQUIRE(f.queue()->lastAttempts().size() == 1);
    CHECK_FALSE(f.queue()->lastAttempts().front().editInvoked);
    CHECK(f.queue()->lastAttempts().front().contextRefusal.contains("changed"));
    waitForCommit();
    CHECK_FALSE(source->isDeleted());
    CHECK(source->definition().distance == .25);
    CHECK(f.graphic.countUndoCycles() == 0);
}

TEST_CASE("Native partial conflict cancels only that source and commits the rest in one step",
          "[gui][properties][offsetcurve][h-sheet-review]") {
    Fixture f;
    auto* conflicted = addArc(f.graphic);
    auto* accepted = addArc(f.graphic, {4, 8});
    f.show({conflicted, accepted});
    f.editNumber("distance", "0.5");
    REQUIRE(f.queue()->pendingCount() == 2);
    const RS_Pen pen(RS_Color(37, 73, 109), RS2::Width07, RS2::DashLine);
    conflicted->setPen(pen);
    waitForCommit();
    CHECK_FALSE(conflicted->isDeleted());
    CHECK(conflicted->isSelected());
    CHECK(conflicted->getPen(false) == pen);
    CHECK(conflicted->definition().distance == .25);
    CHECK(accepted->isDeleted());
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo());
    CHECK_FALSE(accepted->isDeleted());
    CHECK(accepted->definition().distance == .25);
    CHECK(conflicted->getPen(false) == pen);
    CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("Native radius aggregate leaves an already matching oblique peer completely untouched",
          "[gui][properties][offsetcurve][h-sheet-review]") {
    Fixture f;
    const bool major = GENERATE(false, true);
    auto make = [&](RS_Vector axis, double ratio) {
        const RS_Ellipse source(nullptr, RS_EllipseData{{0, 0}, axis, ratio, 0., .1, false});
        auto result = LC_OffsetCurve::create(source, .25, false, {}, .01);
        REQUIRE(result);
        auto* curve = result.entity.release();
        curve->setLayer(f.graphic.findLayer("0"));
        f.graphic.addEntity(curve);
        return curve;
    };
    auto* changed = make(major ? RS_Vector(6, 8) : RS_Vector(3, 4), major ? .3 : .4);
    auto* unchanged = make({3, 4}, .6);
    REQUIRE(unchanged->setOwnedExtensions(0x10, {{1000, QStringLiteral("must survive")}}));
    const auto axis = unchanged->definition().basis.majorP;
    const auto ratio = unchanged->definition().basis.ratio;
    Snapshot before(*unchanged);
    f.show({changed, unchanged});
    f.editNumber(major ? "radiusMajor" : "radiusMinor", major ? "5" : "3");
    REQUIRE(f.queue()->lastAttempts().size() == 2);
    unsigned ran = 0, noops = 0;
    for (const auto& attempt : f.queue()->lastAttempts()) {
        REQUIRE(attempt.editInvoked);
        REQUIRE(attempt.result);
        if (attempt.result.changed) { ranOnce(attempt.result); ++ran; }
        else {
            ++noops;
            CHECK(attempt.result.engineRuns == 0);
            CHECK_FALSE(attempt.result.engineStatus);
            CHECK_FALSE(attempt.result.checkReason);
            CHECK(attempt.result.checkWork.vertices == 0);
        }
    }
    CHECK(ran == 1);
    CHECK(noops == 1);
    CHECK(f.queue()->pendingCount() == 1);
    waitForCommit();
    before.check(*unchanged);
    CHECK_FALSE(unchanged->isDeleted());
    CHECK(std::memcmp(&unchanged->definition().basis.majorP.x, &axis.x, sizeof(double)) == 0);
    CHECK(std::memcmp(&unchanged->definition().basis.majorP.y, &axis.y, sizeof(double)) == 0);
    CHECK(std::memcmp(&unchanged->definition().basis.ratio, &ratio, sizeof(double)) == 0);
    CHECK(unchanged->unknownOwnedFlags() == 0x10);
    CHECK(std::get<QString>(unchanged->unknownOwnedTail().front().value) == "must survive");
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo());
    CHECK_FALSE(changed->isDeleted());
    before.check(*unchanged);
    CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("Native mixed center and rotation edits preserve an already matching peer exactly",
          "[gui][properties][offsetcurve][h-sheet-review]") {
    Fixture f;
    auto* first = addArc(f.graphic);
    auto* second = addArc(f.graphic, {8, 4});
    const unsigned field = GENERATE(0u, 1u, 2u);
    if (field == 2) {
        auto basis = first->definition().basis;
        basis.majorP = {1, 1};
        REQUIRE(first->setBasis(basis));
        basis = second->definition().basis;
        basis.majorP = {0, 1};
        REQUIRE(second->setBasis(basis));
    } else f.view.getViewPort()->createUCS({2, 3}, .4);
    REQUIRE(first->setOwnedExtensions(0x20, {{1000, QStringLiteral("matching-peer")}}));
    f.show({first, second});
    const Snapshot before(*first);
    const QString name = field == 0 ? "centerX" : field == 1 ? "centerY" : "angle";
    auto* row = static_cast<LC_PropertyDouble*>(f.property(name));
    REQUIRE(row->isMultiValue());
    const auto value = row->value();
    const auto display = field == 2 ? RS_Math::rad2deg(value) : value;
    if (field == 2) REQUIRE(RS_Math::deg2rad(display) == value);
    f.editNumber(name, QString::number(display, 'g', 17));
    REQUIRE(f.queue()->lastAttempts().size() == 2);
    for (const auto& attempt : f.queue()->lastAttempts()) {
        REQUIRE(attempt.result);
        if (attempt.original == first) {
            CHECK_FALSE(attempt.result.changed);
            CHECK(attempt.result.engineRuns == 0);
            CHECK_FALSE(attempt.result.engineStatus);
            CHECK_FALSE(attempt.result.checkReason);
        } else CHECK(attempt.result.changed);
    }
    before.check(*first);
    waitForCommit();
    before.check(*first);
    CHECK(first->unknownOwnedFlags() == 0x20);
    CHECK(first->unknownOwnedTail().size() == 1);
    CHECK_FALSE(first->isDeleted());
    CHECK(second->isDeleted());
    CHECK(f.graphic.countUndoCycles() == 1);
}

TEST_CASE("Native computed length is available for a safe display without an editable definition",
          "[gui][properties][offsetcurve][h-sheet-review]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    const bool undefined = GENERATE(false, true);
    if (undefined) source->scale({0, 0}, {2, 3});
    else source->getData().controlPoints.front().x += .5;
    REQUIRE_FALSE(source->hasEditableDefinition());
    auto length = LC_OffsetCurveRead::cachedLength(*source);
    REQUIRE(length);
    f.show({source});
    auto* row = dynamic_cast<LC_PropertyDouble*>(f.property("length"));
    REQUIRE(row);
    CHECK_FALSE(row->property("offsetCurveUnavailable").toBool());
    CHECK_FALSE(row->isEditableByUser());
    CHECK(row->value() == *length);
    CHECK(f.queue()->lastAttempts().empty());
    CHECK(f.graphic.countUndoCycles() == 0);
}

TEST_CASE("Native computed length refuses empty foreign nonfinite and nested display caches",
          "[gui][properties][offsetcurve][h-sheet-review]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    f.show({source});
    class Nested final : public RS_EntityContainer {
    public:
        explicit Nested(RS_EntityContainer* parent) : RS_EntityContainer(parent) {}
        double getLength() const override { ++reads; return 999.; }
        mutable unsigned reads = 0;
    };
    const unsigned invalid = GENERATE(0u, 1u, 2u, 3u, 4u, 5u);
    auto* child = source->entityAt(0);
    struct Parent {
        RS_Entity* entity;
        RS_EntityContainer* parent;
        bool restore = false;
        ~Parent() { if (restore) entity->setParent(parent); }
    } parent{child, child->getParent()};
    Nested* nested = nullptr;
    if (invalid == 0) source->clear();
    if (invalid == 1) { parent.restore = true; child->setParent(&f.graphic); }
    if (invalid == 2) {
        auto* line = dynamic_cast<RS_Line*>(child);
        REQUIRE(line);
        auto point = line->getStartpoint(); point.z = 1.;
        line->setStartpoint(point);
    }
    if (invalid == 3) {
        auto* line = dynamic_cast<RS_Line*>(child);
        REQUIRE(line);
        auto point = line->getStartpoint(); point.x = std::numeric_limits<double>::quiet_NaN();
        line->setStartpoint(point);
    }
    if (invalid == 4) { nested = new Nested(source); source->addEntity(nested); }
    if (invalid == 5) {
        auto* line = dynamic_cast<RS_Line*>(child);
        REQUIRE(line);
        const auto large = std::numeric_limits<double>::max();
        line->setStartpoint({-large, 0}); line->setEndpoint({large, 0});
    }
    CHECK_FALSE(LC_OffsetCurveRead::cachedLength(*source).has_value());
    f.sheet->refill();
    auto* row = dynamic_cast<LC_PropertyDouble*>(f.property("length"));
    REQUIRE(row);
    CHECK(row->property("offsetCurveUnavailable").toBool());
    CHECK_FALSE(row->isEditableByUser());
    CHECK(std::isfinite(row->value()));
    if (nested) CHECK(nested->reads == 0);
    CHECK(f.queue()->lastAttempts().empty());
    CHECK(f.graphic.countUndoCycles() == 0);
}

TEST_CASE("Native length count cap refuses before reading any child length",
          "[gui][properties][offsetcurve][h-sheet-review][budget]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    f.show({source});
    class Line final : public RS_Line {
    public:
        explicit Line(RS_EntityContainer* parent) : RS_Line(parent, {0, 0}, {1, 0}) {}
        double getLength() const override { ++reads; return 1.; }
        mutable unsigned reads = 0;
    };
    source->setAutoUpdateBorders(false);
    auto* observed = new Line(source);
    source->addEntity(observed);
    const auto cap = LC_OffsetCheckLimits{}.maxVertices;
    while (std::size_t(source->count()) <= cap)
        source->addEntity(new RS_Line(source, {0, 0}, {1, 0}));
    CHECK_FALSE(LC_OffsetCurveRead::cachedLength(*source).has_value());
    f.sheet->refill();
    CHECK(f.property("length")->property("offsetCurveUnavailable").toBool());
    CHECK(observed->reads == 0);
    CHECK(f.graphic.countUndoCycles() == 0);
}

TEST_CASE("Native pending source owner removal locking and context changes refuse without publication",
          "[gui][properties][offsetcurve][h-sheet-review][lifetime]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    f.show({source});
    f.editNumber("distance", "0.5");
    REQUIRE(f.queue()->pendingCount() == 1);
    const unsigned change = GENERATE(0u, 1u, 2u, 3u);
    RS_EntityContainer foreign;
    struct Restore {
        Fixture& f;
        RS_Entity* source;
        bool parent = false;
        ~Restore() {
            if (parent) source->setParent(&f.graphic);
            f.context.setDocumentAndView(&f.graphic, &f.view);
        }
    } restore{f, source};
    if (change == 0) { restore.parent = true; source->setParent(&foreign); }
    if (change == 1) REQUIRE(f.graphic.removeEntity(source));
    if (change == 2) source->getLayer(true)->lock(true);
    if (change == 3) f.context.setDocumentAndView(nullptr, nullptr); // no sheet notification
    waitForCommit();
    CHECK(f.queue()->pendingCount() == 0);
    CHECK(f.graphic.countUndoCycles() == 0);
    if (change != 1) {
        CHECK_FALSE(source->getFlag(RS2::FlagDeleted));
        CHECK(source->getFlag(RS2::FlagSelected));
        CHECK(source->definition().distance == .25);
    }
}

TEST_CASE("Native cancelled timer cannot consume a later context generation",
          "[gui][properties][offsetcurve][h-sheet-review]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    f.show({source});
    f.editNumber("distance", "0.5");
    REQUIRE(f.queue()->pendingCount() == 1);
    f.queue()->cancel();
    f.queue()->apply(f.queue()->epoch(), {source},
        [](LC_OffsetCurve& candidate, const LC_OffsetRunLimits& limits) {
            return candidate.setDistance(.75, limits);
        });
    REQUIRE(f.queue()->pendingCount() == 1);
    waitForCommit();
    REQUIRE(live(f.graphic).size() == 1);
    CHECK(live(f.graphic).front()->definition().distance == .75);
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo());
    CHECK_FALSE(source->isDeleted());
    CHECK(source->definition().distance == .25);
    CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("Native cancelled rejected and no op candidates release their snapshot ownership",
          "[gui][properties][offsetcurve][h-sheet-review][ownership]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    auto marker = std::make_shared<DRW_Variant>(1000, std::string("ownership witness"));
    source->setDrwExtData({marker});
    f.show({source});
    const auto owners = marker.use_count();
    f.queue()->apply(f.queue()->epoch(), {source},
        [](LC_OffsetCurve& candidate, const LC_OffsetRunLimits& limits) {
            return candidate.setDistance(.25, limits);
        });
    CHECK(f.queue()->pendingCount() == 0);
    CHECK(marker.use_count() == owners);
    CHECK(source->isSelected());
    const bool conflict = GENERATE(false, true);
    f.queue()->apply(f.queue()->epoch(), {source},
        [](LC_OffsetCurve& candidate, const LC_OffsetRunLimits& limits) {
            return candidate.setDistance(.5, limits);
        });
    REQUIRE(f.queue()->pendingCount() == 1);
    CHECK(marker.use_count() > owners);
    if (conflict) source->setMaterialHandle(23);
    else f.queue()->cancel();
    waitForCommit();
    CHECK(f.queue()->pendingCount() == 0);
    CHECK(marker.use_count() == owners);
    CHECK_FALSE(source->isDeleted());
    CHECK(source->isSelected());
    CHECK(source->definition().distance == .25);
    CHECK(f.graphic.countUndoCycles() == 0);
}

TEST_CASE("Native refill finishes an active editor before retiring its getters",
          "[gui][properties][offsetcurve][h-sheet-review]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    auto* ordinary = new RS_Line(&f.graphic, {0, 4}, {1, 4});
    ordinary->setLayer(f.graphic.findLayer("0"));
    f.graphic.addEntity(ordinary);
    f.show({source, ordinary});
    auto* combo = f.sheet->findChild<QComboBox*>("cbSelection");
    REQUIRE(combo);
    combo->setCurrentIndex(combo->findData(RS2::EntityOffsetCurve));
    auto* editor = f.beginNumber("distance");
    key(*editor, Qt::Key_A, Qt::ControlModifier);
    type(*editor, "0.5"); // no Return or focus-out before changing the sheet
    const bool typeRow = GENERATE(false, true);
    if (typeRow) combo->setCurrentIndex(combo->findData(RS2::EntityContainer));
    else f.sheet->refill();
    REQUIRE(f.queue()->pendingCount() == 1);
    REQUIRE(f.queue()->lastAttempts().size() == 1);
    ranOnce(f.queue()->lastAttempts().front().result);
    waitForCommit();
    REQUIRE(live(f.graphic).size() == 1);
    CHECK(live(f.graphic).front()->definition().distance == .5);
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo());
    CHECK_FALSE(source->isDeleted());
    CHECK_FALSE(f.graphic.undo());
}

TEST_CASE("Native source snapshot caps user keys before cloning or invoking an edit",
          "[gui][properties][offsetcurve][h-sheet-review][budget]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    const unsigned count = GENERATE(64u, 65u);
    for (unsigned i = 0; i < count; ++i) source->setUserDefVar(QString("k%1").arg(i), "v");
    CHECK(source->userDefVarCount() == count);
    f.show({source});
    unsigned invoked = 0;
    f.queue()->apply(f.queue()->epoch(), {source},
        [&invoked](LC_OffsetCurve& candidate, const LC_OffsetRunLimits& limits) {
            ++invoked;
            return candidate.setDistance(.25, limits);
        });
    CHECK(invoked == (count == 64 ? 1 : 0));
    REQUIRE(f.queue()->lastAttempts().size() == 1);
    const auto& attempt = f.queue()->lastAttempts().front();
    CHECK(attempt.editInvoked == (count == 64));
    if (count == 64) {
        REQUIRE(attempt.result);
        CHECK_FALSE(attempt.result.changed);
        CHECK(attempt.result.engineRuns == 0);
    } else CHECK(attempt.contextRefusal.contains("bounded"));
    CHECK(f.queue()->pendingCount() == 0);
    waitForCommit();
    CHECK_FALSE(source->isDeleted());
    CHECK(source->isSelected());
    CHECK(source->userDefVarCount() == count);
    CHECK(f.graphic.countUndoCycles() == 0);
}

TEST_CASE("Native pending snapshots detect EED pointee payload and provenance changes",
          "[gui][properties][offsetcurve][h-sheet-review][metadata]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    const unsigned state = GENERATE(0u, 1u, 2u);
    if (state == 1) source->scale({0, 0}, {2, 3});
    if (state == 2) source->getData().controlPoints.front().x += .25;
    const bool defined = source->isDefined(), consistent = source->isConsistent();
    std::vector<std::shared_ptr<DRW_Variant>> tags{
        std::make_shared<DRW_Variant>(1001, std::string("FOREIGN")),
        std::make_shared<DRW_Variant>(1000, std::string("before")),
        std::make_shared<DRW_Variant>(1070, std::int32_t{1}),
        std::make_shared<DRW_Variant>(1071, std::int64_t{2}),
        std::make_shared<DRW_Variant>(1040, 0.),
        std::make_shared<DRW_Variant>(1010, DRW_Coord{1, 2, 0}),
        std::make_shared<DRW_Variant>(1004, std::vector<std::uint8_t>{0, 255}),
        std::make_shared<DRW_Variant>(1003, std::string("fixture-layer"))};
    tags.back()->setDwgRawLayerReference(77, DRW::AC1032);
    source->setDrwExtData(tags);
    f.show({source});
    auto* width = static_cast<LC_PropertyLineWidth*>(f.property("linewidth"));
    REQUIRE(width->setValue(RS2::Width07, PropertyChangeReasonEdit));
    REQUIRE(f.queue()->pendingCount() == 1);
    REQUIRE(f.queue()->lastAttempts().size() == 1);
    CHECK(f.queue()->lastAttempts().front().result.engineRuns == 0);
    const auto originalWidth = source->getPen(false).getWidth();
    const auto id = source->getId();
    const unsigned change = GENERATE(0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u);
    if (change == 0) tags[1]->addString(1000, "after");
    if (change == 1) tags[2]->addInt(1070, 42);
    if (change == 2) tags[3]->addInt64(1071, std::int64_t{1} << 40);
    if (change == 3) tags[4]->addDouble(1040, -0.);
    if (change == 4) tags[5]->setCoordZ(-0.);
    if (change == 5) tags[6]->content.b->front() = 128;
    if (change == 6) tags[7]->setLayerRefName("fixture-layer");
    if (change == 7) tags[7]->setDwgRawLayerReference(99, DRW::AC1032);
    if (change == 8) tags[7]->setDwgRawLayerReference(77, DRW::AC1015);
    if (change == 9) tags[1]->addString(1006, "before");
    if (change == 10) tags[1]->addInt(1000, 42);
    CHECK(f.queue()->value(f.queue()->epoch(), source) == nullptr);
    waitForCommit();
    CHECK(f.queue()->pendingCount() == 0);
    CHECK_FALSE(source->isDeleted());
    CHECK(source->getId() == id);
    CHECK(source->getPen(false).getWidth() == originalWidth);
    CHECK(source->isDefined() == defined);
    CHECK(source->isConsistent() == consistent);
    CHECK(source->getDrwExtData() == tags);
    CHECK(f.context.messages.join(" ").contains("source changed", Qt::CaseInsensitive));
    CHECK(f.graphic.countUndoCycles() == 0);
}

TEST_CASE("Native EED snapshot caps are checked before cloning and invoking an edit",
          "[gui][properties][offsetcurve][h-sheet-review][metadata][budget]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    using Queue = LC_OffsetCurvePropertyQueue;
    const unsigned kind = GENERATE(0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u);
    std::vector<std::shared_ptr<DRW_Variant>> tags;
    if (kind <= 1) tags.assign(Queue::MaximumSnapshotItems + kind,
        std::make_shared<DRW_Variant>(1070, std::int32_t{1}));
    if (kind == 2 || kind == 3) tags.push_back(std::make_shared<DRW_Variant>(1000,
        std::string(Queue::MaximumSnapshotBytes + (kind == 3), 'x')));
    if (kind == 4 || kind == 5) tags.push_back(std::make_shared<DRW_Variant>(1004,
        std::vector<std::uint8_t>(Queue::MaximumSnapshotBytes + (kind == 5), 128)));
    if (kind == 6 || kind == 7) {
        tags.push_back(std::make_shared<DRW_Variant>(1000, std::string(Queue::MaximumSnapshotBytes/2, 'x')));
        tags.push_back(std::make_shared<DRW_Variant>(1004,
            std::vector<std::uint8_t>(Queue::MaximumSnapshotBytes/2 + (kind == 7), 128)));
    }
    if (kind == 8) {
        auto inactive = std::make_shared<DRW_Variant>(1000, std::string(2*Queue::MaximumSnapshotBytes, 'x'));
        inactive->addInt(1070, 1); // inactive string storage is not part of the value snapshot
        tags.push_back(std::move(inactive));
    }
    source->setDrwExtData(tags);
    f.show({source});
    unsigned invoked = 0;
    f.queue()->apply(f.queue()->epoch(), {source},
        [&invoked](LC_OffsetCurve& candidate, const LC_OffsetRunLimits&) {
            ++invoked;
            candidate.setPen(RS_Pen(RS_Color(37, 73, 109), RS2::Width07, RS2::DashLine));
            LC_OffsetCurveEditResult result;
            result.refusal = LC_OffsetCurveRefusal::None;
            result.changed = true;
            return result;
        });
    const bool admitted = kind % 2 == 0;
    CHECK(invoked == (admitted ? 1 : 0));
    REQUIRE(f.queue()->lastAttempts().size() == 1);
    CHECK(f.queue()->lastAttempts().front().editInvoked == admitted);
    if (!admitted) CHECK(f.queue()->lastAttempts().front().contextRefusal.contains("bounded"));
    CHECK(f.queue()->pendingCount() == (admitted ? 1 : 0));
    f.queue()->cancel();
    waitForCommit();
    CHECK(source->getDrwExtData() == tags);
    CHECK_FALSE(source->isDeleted());
    CHECK(f.graphic.countUndoCycles() == 0);
}

TEST_CASE("Suppressed native sheet refills retain live rows and actual picker completion",
          "[gui][properties][offsetcurve][h-sheet-review][picker]") {
    Fixture f;
    auto* source = addArc(f.graphic);
    f.show({source});
    const bool picker = GENERATE(false, true);
    const auto epoch = f.queue()->epoch();
    auto* row = f.property(picker ? "centerX" : "distance");
    if (picker) {
        auto* line = f.beginNumber("centerX");
        auto* editor = dynamic_cast<LC_PropertyLineEditWithButton*>(line->parentWidget());
        REQUIRE(editor);
        editor->getToolButton()->click();
        REQUIRE(f.context.getInteractiveInputInfo()->state == LC_ActionContext::InteractiveInputInfo::REQUESTED);
    }
    f.sheet->setShouldHandleSelectionChange(false);
    f.sheet->updateFormats();
    CHECK(f.queue()->epoch() == epoch);
    CHECK(f.property(picker ? "centerX" : "distance") == row);
    if (picker) {
        auto* request = f.context.getInteractiveInputInfo();
        REQUIRE(request->requestorTag == row->getName());
        request->wcsPoint = {4, 9};
        request->requestor->onLateRequestCompleted(false);
    } else f.editNumber("distance", "0.5");
    REQUIRE(f.queue()->lastAttempts().size() == 1);
    REQUIRE(f.queue()->lastAttempts().front().result);
    CHECK(f.queue()->lastAttempts().front().result.changed);
    REQUIRE(f.queue()->pendingCount() == 1);
    waitForCommit();
    REQUIRE(live(f.graphic).size() == 1);
    auto* replacement = live(f.graphic).front();
    if (picker) CHECK(replacement->definition().basis.center == RS_Vector(4, 0));
    else CHECK(replacement->definition().distance == .5);
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo());
    CHECK_FALSE(source->isDeleted());
    CHECK_FALSE(f.graphic.undo());
}
