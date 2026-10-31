/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cmath>
#include <memory>

#include <QAction>
#include <QApplication>
#include <QEventLoop>
#include <QMenu>
#include <QStringList>
#include <QTimer>

#include "lc_action_offsetcurve_updatesource.h"
#include "lc_action_offsetcurve_restoreoriginal.h"
#include "lc_actionfactory.h"
#include "lc_actiongroupmanager.h"
#include "lc_actionhandlerfactory.h"
#include "lc_actiontestsupport.h"
#include "lc_defaultactioncontext.h"
#include "lc_entity_property_containerprovider.h"
#include "lc_menufactory.h"
#include "lc_offsetcurve.h"
#include "lc_propertiesprovider_offsetcurve.h"
#include "lc_property_action.h"
#include "lc_property_container.h"
#include "lc_propertysheetwidget.h"
#include "qg_actionhandler.h"
#include "qg_graphicview.h"
#include "rs_commands.h"
#include "rs_ellipse.h"
#include "rs_layer.h"
#include "rs_line.h"

namespace {
class Context final : public LC_ActionContext {
public:
    QStringList messages;
    QList<RS2::ActionType> activations;
    RS_Entity* activationEntity = nullptr;
    std::shared_ptr<RS_ActionInterface> active;
    void commandMessage(const QString& message) override { messages.append(message); }
    void setCurrentAction(RS2::ActionType type, void* data) override {
        activations.append(type);
        activationEntity = getContextMenuActionContextEntity();
        active = LC_ActionsHandlerFactory::createActionInstance(type, this, data);
        REQUIRE(active);
        active->init(0);
    }
};
class Factory final : public LC_ActionFactory {
public:
    explicit Factory(QG_ActionHandler* handler) : LC_ActionFactory(nullptr, handler) {}
    using LC_ActionFactory::createOffsetCurveActions;
};
class Handler final : public QG_ActionHandler {
public:
    Handler() : QG_ActionHandler(nullptr), m_context(this) { setActionContext(&m_context); }
    mutable QList<RS2::ActionType> dispatched;
    std::shared_ptr<RS_ActionInterface> createActionInstance(RS2::ActionType type, void*) const override {
        dispatched.append(type);
        return nullptr; // dispatch observation only; real factory instances are tested separately
    }
private:
    LC_DefaultActionContext m_context;
};
class MenuView final : public QG_GraphicView {
public:
    explicit MenuView(LC_ActionContext* context) : QG_GraphicView(nullptr, nullptr, context) {}
    void redraw(RS2::RedrawMethod = RS2::RedrawAll, bool = false) override {}
};
class MenuFactory final : public LC_MenuFactoryGraphicView {
public:
    MenuFactory(LC_ActionGroupManager* actions, LC_MenusHolder* menus)
        : LC_MenuFactoryGraphicView(nullptr, actions, menus) {}
    using LC_MenuFactoryGraphicView::createGVMenuEntitySpecific;
};

void settleLinks() {
    QEventLoop loop;
    QTimer::singleShot(80, &loop, &QEventLoop::quit);
    loop.exec();
}
QAction* named(QMenu& menu, const QString& text) {
    for (auto* action : menu.actions()) {
        if (action->text() == text) return action;
        if (action->menu()) if (auto* result = named(*action->menu(), text)) return result;
    }
    return nullptr;
}
struct Drawing {
    const bool ready{lc::test::application() != nullptr};
    RS_Graphic graphic;
    lc::test::TestGraphicView view;
    Context context;
    Drawing() {
        graphic.initForNewDocument();
        view.setDocument(&graphic);
        context.setDocumentAndView(&graphic, &view);
    }
    ~Drawing() { context.active.reset(); }
    LC_OffsetCurve* native() {
        LC_EllipseOffsetDefinition definition;
        definition.basis = {{0, 0}, {1, 0}, 1., 0., .1, false};
        definition.distance = .25;
        definition.relTolerance = .01;
        RS_Line cache(nullptr, {1.25, 0}, {1.25*std::cos(.1), 1.25*std::sin(.1)});
        auto curve = LC_OffsetCurve::adopt(cache, definition, .01);
        REQUIRE(curve);
        curve->setLayer(graphic.findLayer("0"));
        auto* result = curve.release();
        graphic.addEntity(result);
        return result;
    }
    int ellipses() const {
        int count = 0;
        for (const auto* entity : graphic)
            if (!entity->isDeleted() && entity->rtti() == RS2::EntityEllipse) ++count;
        return count;
    }
};
struct Properties {
    Drawing& drawing;
    LC_ActionGroupManager actions{nullptr};
    std::unique_ptr<LC_PropertySheetWidget> sheet;
    LC_EntityPropertyContainerProvider provider;
    LC_PropertyContainer root;
    explicit Properties(Drawing& value) : drawing(value) {
        sheet = std::make_unique<LC_PropertySheetWidget>(nullptr, &drawing.context, &actions);
        sheet->getOptions()->showComputed = false;
        sheet->getOptions()->showSingleEntityCommands = true;
        sheet->getOptions()->showMultiEntityCommands = true;
        sheet->setGraphicView(&drawing.view);
        QApplication::processEvents();
        provider.init(sheet.get(), &drawing.context);
        provider.setGraphicView(&drawing.view);
    }
    ~Properties() {
        if (sheet) sheet->setGraphicView(nullptr);
        drawing.context.active.reset();
    }
    void fill(RS2::EntityType type, const QList<RS_Entity*>& entities) {
        provider.fillPropertyContainerForSelection(&drawing.graphic, &root, type, entities);
    }
    LC_PropertyAction* link(const QString& name) {
        auto found = root.findChildProperties(name);
        REQUIRE(found.size() == 1);
        auto* result = dynamic_cast<LC_PropertyAction*>(found.front());
        REQUIRE(result);
        return result;
    }
};
void noUndo(RS_Document& document) {
    bool undo = true, redo = true;
    document.collectUndoState(undo, redo);
    CHECK_FALSE(undo);
    CHECK_FALSE(redo);
}
}

TEST_CASE("I UI registers real QActions outside tool groups without command aliases",
          "[lc-offsetcurve][i-ui][gui]") {
    Drawing d;
    Handler handler;
    LC_ActionGroupManager actions(nullptr);
    auto* modify = new LC_ActionGroup(&actions, "modify", "Modify", "Modify", ":/icons/offset.lci");
    actions.addActionGroup("modify", modify, true);
    Factory factory(&handler);
    factory.createOffsetCurveActions(&actions);
    actions.completeInit();
    auto* group = actions.getGroupByName("offset_curve");
    REQUIRE(group);
    CHECK_FALSE(actions.toolGroups().contains(group));
    CHECK(modify->actions().isEmpty());
    const auto id = GENERATE(RS2::ActionOffsetCurveUpdateSource, RS2::ActionOffsetCurveRestoreOriginal);
    auto* action = actions.getActionByType(id);
    REQUIRE(action);
    CHECK(action->actionGroup() == group);
    CHECK(action->text() == (id == RS2::ActionOffsetCurveUpdateSource ? "Update from Source" : "Restore Original"));
    CHECK_FALSE(action->icon().isNull());
    CHECK(RS_COMMANDS->getCommandForAction(id).isEmpty());
    handler.setDocumentAndView(&d.graphic, &d.view);
    action->trigger();
    REQUIRE(handler.dispatched.size() == 1);
    CHECK(handler.dispatched.front() == id);
    CHECK(factory.modifyActions.isEmpty());

    auto made = LC_ActionsHandlerFactory::createActionInstance(id, &d.context);
    REQUIRE(made);
    CHECK(made->rtti() == id);
    if (id == RS2::ActionOffsetCurveUpdateSource)
        CHECK(dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(made.get()));
    else CHECK(dynamic_cast<LC_ActionOffsetCurveRestoreOriginal*>(made.get()));
}

TEST_CASE("I UI native context proxies use real source actions and exclude control editing",
          "[lc-offsetcurve][i-ui][gui]") {
    Drawing d;
    auto* curve = d.native();
    Handler handler;
    handler.setDocumentAndView(&d.graphic, &d.view);
    LC_ActionGroupManager actions(nullptr);
    Factory actionFactory(&handler);
    actionFactory.createOffsetCurveActions(&actions);
    for (const auto& name : {"ModifyOffset", "DrawSplineExplode", "ModifyRevertDirection",
                            "DrawSplinePointsAdd", "DrawSplinePointsAppend", "DrawSplinePointsRemove",
                            "DrawSplinePointsDelTwo"}) {
        auto* action = new QAction(name, &actions);
        action->setIconText(name);
        actions.getActionsMap().insert(name, action);
    }
    LC_MenusHolder menus;
    MenuFactory factory(&actions, &menus);
    MenuView view(&d.context);
    view.setDocument(&d.graphic);
    struct RestoreContext {
        Drawing& d;
        ~RestoreContext() { d.context.setDocumentAndView(&d.graphic, &d.view); }
    } restore{d};
    d.context.setDocumentAndView(&d.graphic, &view);
    const auto id = GENERATE(RS2::ActionOffsetCurveUpdateSource, RS2::ActionOffsetCurveRestoreOriginal);
    const QString title = id == RS2::ActionOffsetCurveUpdateSource ? "Update from Source" : "Restore Original";
    QMenu menu;
    factory.createGVMenuEntitySpecific(&menu, &view, curve, {2, 3});
    auto* proxy = named(menu, title);
    REQUIRE(proxy);
    proxy->trigger();
    REQUIRE(handler.dispatched.size() == 1);
    CHECK(handler.dispatched.front() == id);
    CHECK(d.context.getContextMenuActionContextEntity() == curve);
    CHECK(d.context.getContextMenuActionClickPosition() == RS_Vector(2, 3));
    for (const auto& name : {"DrawSplinePointsAdd", "DrawSplinePointsAppend",
                            "DrawSplinePointsRemove", "DrawSplinePointsDelTwo"})
        CHECK_FALSE(named(menu, name));
    actions.getActionsMap().value(id == RS2::ActionOffsetCurveUpdateSource ?
        "OffsetCurveUpdateSource" : "OffsetCurveRestoreOriginal")->setEnabled(false);
    QMenu disabled;
    factory.createGVMenuEntitySpecific(&disabled, &view, curve, {2, 3});
    CHECK_FALSE(named(disabled, title));
    RS_Spline ordinary(nullptr, curve->getData());
    QMenu ordinaryMenu;
    factory.createGVMenuEntitySpecific(&ordinaryMenu, &view, &ordinary, {2, 3});
    CHECK_FALSE(named(ordinaryMenu, "Update from Source"));
    CHECK_FALSE(named(ordinaryMenu, "Restore Original"));
    CHECK(named(ordinaryMenu, "DrawSplinePointsAdd"));
}

TEST_CASE("I UI popup source action normalizes display chord to native owner",
          "[lc-offsetcurve][i-ui][gui]") {
    Drawing d;
    auto* curve = d.native();
    Handler handler;
    handler.setDocumentAndView(&d.graphic, &d.view);
    LC_ActionGroupManager actions(nullptr);
    Factory actionFactory(&handler);
    actionFactory.createOffsetCurveActions(&actions);
    LC_MenusHolder menus;
    MenuFactory factory(&actions, &menus);
    MenuView view(&d.context);
    view.setDocument(&d.graphic);
    d.context.setDocumentAndView(&d.graphic, &view);
    QStringList names{"OffsetCurveUpdateSource", "OffsetCurveRestoreOriginal"};
    std::unique_ptr<QMenu> menu(factory.createGraphicViewPopupMenu(&view, curve->entityAt(0), {3, 4}, names, false));
    REQUIRE(menu);
    auto* proxy = named(*menu, "Restore Original");
    REQUIRE(proxy);
    proxy->trigger();
    CHECK(d.context.getContextMenuActionContextEntity() == curve);
    REQUIRE(handler.dispatched.size() == 1);
    CHECK(handler.dispatched.front() == RS2::ActionOffsetCurveRestoreOriginal);
    CHECK(QMetaObject::invokeMethod(menu.get(), "aboutToHide", Qt::DirectConnection));
    CHECK_FALSE(curve->isSelected());
    d.context.clearContextMenuActionContext();
    d.context.setDocumentAndView(&d.graphic, &d.view);
}

TEST_CASE("I UI single property links activate real actions with explicit native context",
          "[lc-offsetcurve][i-ui][properties]") {
    Drawing d;
    auto* curve = d.native();
    if (!curve->isSelected()) d.graphic.select(QList<RS_Entity*>{curve}, true);
    Properties p(d);
    p.fill(RS2::EntityOffsetCurve, {curve});
    const int index = GENERATE(0, 1);
    p.link("offsetSourceSingle")->invokeClick(index);
    settleLinks();
    REQUIRE(d.context.activations.size() == 1);
    CHECK(d.context.activationEntity == curve);
    CHECK(curve->isSelected());
    if (index == 0) {
        REQUIRE(dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(d.context.active.get()));
        CHECK_FALSE(d.context.active->isFinished());
        CHECK(d.ellipses() == 0);
        noUndo(d.graphic);
    } else {
        auto* action = dynamic_cast<LC_ActionOffsetCurveRestoreOriginal*>(d.context.active.get());
        REQUIRE(action);
        CHECK(action->lastReport().restored == 1);
        CHECK(d.ellipses() == 1);
        REQUIRE(d.graphic.undo());
        CHECK(d.ellipses() == 0);
        CHECK_FALSE(d.graphic.undo());
    }
}

TEST_CASE("I UI selected set links preserve mixed selection and clear stale single context",
          "[lc-offsetcurve][i-ui][properties]") {
    Drawing d;
    auto* first = d.native();
    auto* second = d.native();
    auto* line = new RS_Line(&d.graphic, {0, 4}, {1, 4});
    line->setLayer(d.graphic.findLayer("0"));
    d.graphic.addEntity(line);
    const bool mixed = GENERATE(false, true);
    QList<RS_Entity*> selected{first, second};
    if (mixed) selected.append(line);
    d.graphic.select(selected);
    Properties p(d);
    p.fill(mixed ? RS2::EntityUnknown : RS2::EntityOffsetCurve, selected);
    d.context.saveContextMenuActionContext(line, {0, 4}, false);
    const int index = GENERATE(0, 1);
    p.link("offsetSourceSelection")->invokeClick(index);
    settleLinks();
    REQUIRE(d.context.activations.size() == 1);
    CHECK(d.context.activationEntity == nullptr);
    CHECK(first->isSelected());
    CHECK(second->isSelected());
    CHECK(line->isSelected() == mixed);
    if (index == 1) {
        auto* action = dynamic_cast<LC_ActionOffsetCurveRestoreOriginal*>(d.context.active.get());
        REQUIRE(action);
        CHECK(action->lastReport().restored == 1);
        CHECK(action->lastReport().alreadyPresent == 1);
        CHECK(action->lastReport().refused == 0);
        REQUIRE(d.graphic.undo());
        CHECK(d.ellipses() == 0);
        CHECK_FALSE(d.graphic.undo());
    } else noUndo(d.graphic);
}

TEST_CASE("I UI ordinary selected sets do not acquire native source links",
          "[lc-offsetcurve][i-ui][properties]") {
    Drawing d;
    auto* first = new RS_Line(&d.graphic, {0, 4}, {1, 4});
    auto* second = new RS_Line(&d.graphic, {0, 5}, {1, 5});
    for (auto* line : {first, second}) {
        line->setLayer(d.graphic.findLayer("0"));
        d.graphic.addEntity(line);
        if (!line->isSelected()) d.graphic.select(QList<RS_Entity*>{line}, true);
    }
    Properties p(d);
    p.fill(RS2::EntityUnknown, {first, second});
    CHECK(p.root.findChildProperties("offsetSourceSelection").isEmpty());
    CHECK(p.root.findChildProperties("offsetSourceSingle").isEmpty());
    noUndo(d.graphic);
}

TEST_CASE("I UI bulk source links admit unique live native targets without dereferencing supplied stale addresses",
          "[lc-offsetcurve][i-ui][properties][bulk][lifetime]") {
    Drawing d;
    auto* curve = d.native();
    QList<RS_Entity*> ordinary;
    ordinary.reserve(4096);
    for (int i = 0; i < 4096; ++i) {
        auto* line = new RS_Line(&d.graphic, {double(i), 10}, {double(i) + 1, 10});
        line->setLayer(d.graphic.findLayer("0"));
        d.graphic.addEntity(line);
        ordinary.append(line);
    }
    auto selected = ordinary;
    selected.append(curve);
    d.graphic.select(selected);
    auto* stale = d.native();
    REQUIRE(d.graphic.removeEntity(stale));
    RS_Graphic foreign;
    foreign.initForNewDocument();
    auto* foreignLine = new RS_Line(&foreign, {0, 0}, {1, 0});
    foreign.addEntity(foreignLine);
    Properties p(d);
    p.sheet->setShouldHandleSelectionChange(false);
    LC_PropertiesProviderOffsetCurve::createSourceActionLinks(
        &p.root, p.sheet.get(), &d.context, ordinary, false);
    CHECK(p.root.findChildProperties("offsetSourceSelection").isEmpty());
    CHECK(p.root.findChildProperties("offsetSourceSingle").isEmpty());
    auto supplied = ordinary;
    supplied.append(curve);
    supplied.append(curve); // Owner-normalized duplicate addresses are one target.
    supplied.append(stale); // Identity only: already freed and absent from the live document.
    supplied.append(foreignLine);
    supplied.append(nullptr);
    const auto fingerprint = RS_Spline::geometryFingerprint(curve->getData());
    const auto count = d.graphic.count();
    LC_PropertiesProviderOffsetCurve::createSourceActionLinks(
        &p.root, p.sheet.get(), &d.context, supplied, true);
    p.link("offsetSourceSingle")->invokeClick(1);
    settleLinks();
    REQUIRE(d.context.activations.size() == 1);
    CHECK(d.context.activations.front() == RS2::ActionOffsetCurveRestoreOriginal);
    CHECK(d.context.activationEntity == curve);
    CHECK(d.graphic.count() == count + 1);
    CHECK(d.ellipses() == 1);
    CHECK(RS_Spline::geometryFingerprint(curve->getData()) == fingerprint);
    CHECK(curve->isSelected());
    for (const auto* entity : ordinary) CHECK(entity->isSelected());
    CHECK(foreign.count() == 1);
    CHECK_FALSE(foreignLine->isSelected());
    REQUIRE(d.graphic.undo());
    CHECK(d.ellipses() == 0);
    CHECK_FALSE(curve->isDeleted());
    CHECK_FALSE(d.graphic.undo());
    REQUIRE(d.graphic.redo());
    CHECK(d.ellipses() == 1);
}

TEST_CASE("I UI delayed property links refuse stale epoch context owner deletion and widget lifetime",
          "[lc-offsetcurve][i-ui][properties][lifetime]") {
    Drawing d;
    auto* curve = d.native();
    if (!curve->isSelected()) d.graphic.select(QList<RS_Entity*>{curve}, true);
    Properties p(d);
    p.fill(RS2::EntityOffsetCurve, {curve});
    p.link("offsetSourceSingle")->invokeClick(1);
    const unsigned change = GENERATE(0u, 1u, 2u, 3u, 4u);
    RS_EntityContainer foreign;
    struct Restore {
        Drawing& d;
        LC_OffsetCurve* curve;
        bool parent = false;
        ~Restore() {
            if (parent) curve->setParent(&d.graphic);
            d.context.setDocumentAndView(&d.graphic, &d.view);
        }
    } restore{d, curve};
    if (change == 0) p.sheet->offsetPropertyQueue()->cancel();
    if (change == 1) d.context.setDocumentAndView(nullptr, nullptr);
    if (change == 2) REQUIRE(d.graphic.removeEntity(curve));
    if (change == 3) p.sheet.reset();
    if (change == 4) { restore.parent = true; curve->setParent(&foreign); }
    settleLinks();
    CHECK(d.context.activations.isEmpty());
    CHECK(d.ellipses() == 0);
    noUndo(d.graphic);
}

TEST_CASE("I UI source links discard unrelated popup identity without dereferencing or deselecting it",
          "[lc-offsetcurve][i-ui][properties][lifetime]") {
    Drawing d;
    auto* curve = d.native();
    auto* unrelated = new RS_Line(&d.graphic, {0, 4}, {1, 4});
    unrelated->setLayer(d.graphic.findLayer("0"));
    d.graphic.addEntity(unrelated);
    d.graphic.select(QList<RS_Entity*>{curve, unrelated});
    Properties p(d);
    const bool single = GENERATE(false, true);
    const bool erased = GENERATE(false, true);
    const int index = GENERATE(0, 1);
    p.fill(single ? RS2::EntityOffsetCurve : RS2::EntityUnknown,
        single ? QList<RS_Entity*>{curve} : QList<RS_Entity*>{curve, unrelated});
    // Suppression keeps this real row/epoch live across the unrelated removal.
    // No target pointer or epoch is repaired to make the delayed link execute.
    p.sheet->setShouldHandleSelectionChange(false);
    const auto epoch = p.sheet->offsetPropertyQueue()->epoch();
    d.context.saveContextMenuActionContext(unrelated, {0, 4}, true);
    p.link(single ? "offsetSourceSingle" : "offsetSourceSelection")->invokeClick(index);
    if (erased) REQUIRE(d.graphic.removeEntity(unrelated));
    CHECK(p.sheet->offsetPropertyQueue()->epoch() == epoch);
    CHECK(d.context.getContextMenuActionContextEntity() == unrelated); // identity only after removal
    settleLinks();
    REQUIRE(d.context.activations.size() == 1);
    CHECK(d.context.activationEntity == (single ? curve : nullptr));
    CHECK(curve->isSelected());
    if (!erased) CHECK(unrelated->isSelected());
    if (index == 1) {
        REQUIRE(d.ellipses() == 1);
        REQUIRE(d.graphic.undo()); CHECK(d.ellipses() == 0); CHECK_FALSE(d.graphic.undo());
    } else noUndo(d.graphic);
}

TEST_CASE("I UI source links refuse closing views before creation and delayed activation",
          "[lc-offsetcurve][i-ui][properties][lifetime][close]") {
    Drawing d;
    auto* curve = d.native();
    d.graphic.select(QList<RS_Entity*>{curve});
    Properties p(d);
    const bool beforeFill = GENERATE(false, true);
    const int index = GENERATE(0, 1);
    p.sheet->setShouldHandleSelectionChange(false);
    if (beforeFill) d.view.beginClose();
    p.fill(RS2::EntityOffsetCurve, {curve});
    if (beforeFill) {
        CHECK(p.root.findChildProperties("offsetSourceSingle").isEmpty());
        CHECK(p.root.findChildProperties("offsetSourceSelection").isEmpty());
    } else {
        p.link("offsetSourceSingle")->invokeClick(index);
        const auto epoch = p.sheet->offsetPropertyQueue()->epoch();
        d.view.beginClose();
        CHECK(p.sheet->offsetPropertyQueue()->epoch() == epoch);
    }
    REQUIRE(d.view.isClosing());
    settleLinks();
    CHECK(d.context.activations.isEmpty()); CHECK_FALSE(d.context.active);
    CHECK_FALSE(curve->isDeleted()); CHECK(curve->isSelected());
    CHECK(d.ellipses() == 0); noUndo(d.graphic);
}

TEST_CASE("I UI source link refuses to launch over an accepted pending property edit",
          "[lc-offsetcurve][i-ui][properties]") {
    Drawing d;
    auto* curve = d.native();
    if (!curve->isSelected()) d.graphic.select(QList<RS_Entity*>{curve}, true);
    Properties p(d);
    p.fill(RS2::EntityOffsetCurve, {curve});
    auto* queue = p.sheet->offsetPropertyQueue();
    auto epoch = queue->epoch();
    auto basis = curve->definition().basis;
    basis.center = {4, 8};
    queue->apply(epoch, {curve}, [basis](LC_OffsetCurve& candidate, const LC_OffsetRunLimits& limits) {
        return candidate.setBasis(basis, limits);
    });
    REQUIRE(queue->pendingCount() == 1);
    p.link("offsetSourceSingle")->invokeClick(1);
    settleLinks();
    CHECK(d.context.activations.isEmpty());
    CHECK(d.context.messages.join(" ").contains("pending property edits"));
    CHECK(d.ellipses() == 0);
    REQUIRE(d.graphic.undo());
    CHECK_FALSE(curve->isDeleted());
    CHECK_FALSE(d.graphic.undo());
}
