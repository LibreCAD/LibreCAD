/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cmath>
#include <limits>
#include <memory>
#include <optional>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QMenu>
#include <QMouseEvent>
#include <QSettings>
#include <QTextBrowser>

#include "lc_action_select_window.h"
#include "lc_actioncontext.h"
#include "lc_actiongroupmanager.h"
#include "lc_actiontestsupport.h"
#include "lc_creatorinvoker.h"
#include "lc_dlg_menu_assigner.h"
#include "lc_entitymatchdescriptorsregistry.h"
#include "lc_entitymetauiutils.h"
#include "lc_menuactivator.h"
#include "lc_menufactory.h"
#include "lc_offsetcurve.h"
#include "lc_quickinfoentitydata.h"
#include "lc_quickinfowidget.h"
#include "lc_quickinfowidgetoptions.h"
#include "lc_selectwindowoptions.h"
#include "lc_sourcefonts.h"
#include "lc_testentityfactory.h"
#include "qg_graphicview.h"
#include "rs_line.h"
#include "rs_selection.h"

namespace {
struct Drawing {
    const bool qtReady{lc::test::application() != nullptr};
    RS_Graphic graphic;
    lc::test::TestGraphicView view;
    LC_ActionContext context;
    Drawing() {
        graphic.initForNewDocument();
        view.setDocument(&graphic);
        context.setDocumentAndView(&graphic, &view);
    }
    LC_OffsetCurve* native() {
        auto curve = lc::test::makeOffsetCurve(&graphic);
        REQUIRE(curve);
        REQUIRE(curve->hasEditableDefinition());
        curve->setLayer(graphic.findLayer("0"));
        auto* result = curve.get();
        graphic.addEntity(curve.release());
        return result;
    }
};

class Registry final : public LC_EntityMatchDescriptorsRegistry {
public:
    explicit Registry(LC_ActionContext* context) { initEntityDescriptors(context); }
};

class Info final : public LC_QuickInfoEntityData {
public:
    explicit Info(Drawing& drawing) {
        setOptions(&options);
        setDocumentAndView(&drawing.graphic, drawing.view.getViewPort());
    }
    QString text(const QString& label) const {
        for (const auto* p : m_properties) if (p->label == label) return p->value;
        return {};
    }
    std::optional<double> number(const QString& label) const {
        for (const auto* p : m_properties)
            if (p->label == label && p->type == PROPERTY_TYPE_LINEAR)
                return static_cast<const DoublePropertyInfo*>(p)->data;
        return std::nullopt;
    }
private:
    LC_QuickInfoOptions options;
};

class WindowOptions final : public LC_SelectWindowOptionsWidget {
public:
    using LC_SelectWindowOptionsWidget::doUpdateByAction;
    using LC_SelectWindowOptionsWidget::setEntityTypesToActinAndView;
};

template <typename T>
bool matches(LC_EntityMatchDescriptor* descriptor, const QString& key, RS_Entity* entity,
             T value, LC_PropertyMatchOperation operation = MATCH_OPERATION_EQUALS, T tolerance = {}) {
    auto* property = descriptor->findPropertyDescriptor(key);
    REQUIRE(property);
    std::unique_ptr<LC_EntityMatcher> matcher(property->createMatcher());
    auto* typed = dynamic_cast<LC_GenericEntityMatcher<T>*>(matcher.get());
    REQUIRE(typed);
    typed->update(value, tolerance, operation);
    return matcher->isMatch(entity);
}

class MenuView final : public QG_GraphicView {
public:
    explicit MenuView(LC_ActionContext* context) : QG_GraphicView(nullptr, nullptr, context) {}
    void redraw(RS2::RedrawMethod = RS2::RedrawAll, bool = false) override { ++redraws; }
    unsigned redraws = 0;
};

class MenuFactory final : public LC_MenuFactoryGraphicView {
public:
    MenuFactory(LC_ActionGroupManager* actions, LC_MenusHolder* holder)
        : LC_MenuFactoryGraphicView(nullptr, actions, holder) {}
    using LC_MenuFactoryGraphicView::createGVMenuEntitySpecific;
};

QAction* findAction(QMenu& menu, const QString& text) {
    for (auto* action : menu.actions()) {
        if (action->text() == text) return action;
        if (action->menu()) if (auto* found = findAction(*action->menu(), text)) return found;
    }
    return nullptr;
}

void addAction(LC_ActionGroupManager& manager, const QString& name) {
    auto* action = new QAction(name, &manager);
    action->setIconText(name);
    manager.getActionsMap().insert(name, action);
}

void seedActions(LC_ActionGroupManager& manager) {
    for (const auto& name : {"ModifyOffset", "DrawSplineExplode", "ModifyRevertDirection",
                            "DrawSplinePointsAdd", "DrawSplinePointsAppend", "DrawSplinePointsRemove",
                            "DrawSplinePointsDelTwo", "ModifyMove", "EntityInfo"})
        addAction(manager, name);
}

class SettingsGroup final {
public:
    SettingsGroup(QSettings& settings, QString group) : settings(settings), group(std::move(group)) {
        settings.beginGroup(this->group);
        for (const auto& key : settings.allKeys()) previous.insert(key, settings.value(key));
        settings.remove("");
        settings.endGroup();
    }
    ~SettingsGroup() {
        settings.beginGroup(group);
        settings.remove("");
        for (auto it = previous.cbegin(); it != previous.cend(); ++it) settings.setValue(it.key(), it.value());
        settings.endGroup();
    }
private:
    QSettings& settings;
    QString group;
    QMap<QString, QVariant> previous;
};

LC_MenuActivator activator(RS2::EntityType type) {
    LC_MenuActivator result("", false, false, false, LC_MenuActivator::RIGHT,
                            LC_MenuActivator::CLICK_RELEASE, true, type);
    result.update();
    return result;
}

void lengthUnavailable(Drawing& drawing, LC_OffsetCurve* curve) {
    Registry registry(&drawing.context);
    auto* descriptor = registry.findEntityMatchDescriptor(RS2::EntityOffsetCurve);
    REQUIRE(descriptor);
    CHECK_FALSE(matches<double>(descriptor, "length", curve, 0., MATCH_OPERATION_ALL));
    CHECK_FALSE(matches<double>(descriptor, "length", curve, 0., MATCH_OPERATION_NOT_EQUALS));
    Info info(drawing);
    REQUIRE(info.processEntity(curve));
    CHECK_FALSE(info.number("Cached Length"));
    CHECK(info.text("Cached Length") == "Unavailable");
}
}

TEST_CASE("F names count native objects under their own translated type", "[lc-offsetcurve][f-names][gui]") {
    REQUIRE(lc::test::application());
    unsigned nativeRows = 0;
    for (const auto& [name, type] : LC_EntityMetaUIUtils::getEntityTypeNamesList())
        if (type == RS2::EntityOffsetCurve) { ++nativeRows; CHECK(name == "Offset Curve"); }
    CHECK(nativeRows == 1);
    QComboBox combo;
    LC_EntityMetaUIUtils::setupSelectionEntityTypesCombobox(&combo,
        {{RS2::EntityOffsetCurve, 2}, {RS2::EntitySpline, 1}});
    REQUIRE(combo.count() == 3);
    CHECK(combo.itemText(0) == "All (3)");
    CHECK(combo.findData(RS2::EntityOffsetCurve) > 0);
    CHECK(combo.findData(RS2::EntitySpline) > 0);
    QComboBox nativeOnly;
    LC_EntityMetaUIUtils::setupSelectionEntityTypesCombobox(&nativeOnly, {{RS2::EntityOffsetCurve, 1}});
    CHECK(nativeOnly.currentData().toUInt() == RS2::EntityOffsetCurve);
}

TEST_CASE("F native match metadata is distinct and does not expose a control net", "[lc-offsetcurve][f-names]") {
    Drawing drawing;
    Registry registry(&drawing.context);
    auto* descriptor = registry.findEntityMatchDescriptor(RS2::EntityOffsetCurve);
    REQUIRE(descriptor);
    CHECK(descriptor != registry.findEntityMatchDescriptor(RS2::EntitySpline));
    CHECK(descriptor->getEntityType() == RS2::EntityOffsetCurve);
    for (const auto& key : {"layer", "defined", "consistent", "editableDefinition", "source", "distance", "towardCentre", "length"})
        CHECK(descriptor->findPropertyDescriptor(key) != nullptr);
    for (const auto& key : {"points", "degree", "controlPointX", "vertexY", "weights", "knots"})
        CHECK(descriptor->findPropertyDescriptor(key) == nullptr);
    CHECK(descriptor->findPropertyDescriptor("distance")->getPropertyType() == ENTITY_PROPERTY_LENGTH);
    CHECK(descriptor->findPropertyDescriptor("length")->getPropertyType() == ENTITY_PROPERTY_LENGTH);
    CHECK(descriptor->findPropertyDescriptor("towardCentre")->getPropertyType() == ENTITY_PROPERTY_BOOL);
}

TEST_CASE("F native matchers compare actual numbers and reject foreign carriers", "[lc-offsetcurve][f-names]") {
    Drawing drawing;
    auto* curve = drawing.native();
    Registry registry(&drawing.context);
    auto* descriptor = registry.findEntityMatchDescriptor(RS2::EntityOffsetCurve);
    REQUIRE(descriptor);
    CHECK(matches<double>(descriptor, "length", curve, curve->getLength(), MATCH_OPERATION_EQUALS, 1e-12));
    CHECK(matches<double>(descriptor, "distance", curve, 1.));
    CHECK_FALSE(matches<double>(descriptor, "distance", curve, 2.));
    CHECK(matches<QString>(descriptor, "source", curve, QString("ELLIPSE")));
    RS_Line line(nullptr, RS_LineData({0, 0}, {1, 0}));
    for (auto operation : {MATCH_OPERATION_EQUALS, MATCH_OPERATION_NOT_EQUALS, MATCH_OPERATION_ALL}) {
        CHECK_FALSE(matches<double>(descriptor, "distance", &line, 1., operation));
        CHECK_FALSE(matches<double>(descriptor, "distance", nullptr, 1., operation));
    }
    curve->scale({0, 0}, {2, 2});
    REQUIRE(curve->hasEditableDefinition());
    CHECK(matches<double>(descriptor, "distance", curve, 1.5, MATCH_OPERATION_GREATER));
    CHECK_FALSE(matches<double>(descriptor, "distance", curve, 3.));
}

TEST_CASE("F unavailable definition values never match including inequality and all", "[lc-offsetcurve][f-names]") {
    Drawing drawing;
    auto* curve = drawing.native();
    const bool undefined = GENERATE(false, true);
    if (undefined) curve->scale({0, 0}, {2, 3});
    else curve->getData().controlPoints.front().x += .25;
    REQUIRE_FALSE(curve->hasEditableDefinition());
    CHECK(curve->isDefined() == !undefined);
    Registry registry(&drawing.context);
    auto* descriptor = registry.findEntityMatchDescriptor(RS2::EntityOffsetCurve);
    REQUIRE(descriptor);
    for (auto operation : {MATCH_OPERATION_EQUALS, MATCH_OPERATION_NOT_EQUALS, MATCH_OPERATION_ALL}) {
        CHECK_FALSE(matches<double>(descriptor, "distance", curve, 42., operation));
        CHECK_FALSE(matches<QString>(descriptor, "source", curve, QString("ELLIPSE"), operation));
        CHECK_FALSE(matches<bool>(descriptor, "towardCentre", curve, false, operation));
    }
    CHECK(matches<bool>(descriptor, "editableDefinition", curve, false));
    CHECK(matches<double>(descriptor, "length", curve, curve->getLength(), MATCH_OPERATION_EQUALS, 1e-12));
}

TEST_CASE("F physical side does not follow cache traversal or source reversal", "[lc-offsetcurve][f-names]") {
    Drawing drawing;
    const bool inward = GENERATE(false, true);
    const bool reversed = GENERATE(false, true);
    LC_EllipseOffsetDefinition definition;
    const double a = reversed ? .2501 : .25, b = reversed ? .25 : .2501;
    definition.basis = RS_EllipseData{{0, 0}, {4, 0}, 1., a, b, reversed};
    definition.distance = 1.;
    definition.towardCentre = inward;
    definition.relTolerance = .01;
    const double radius = inward ? 3. : 5.;
    RS_Line cache(nullptr, RS_LineData({radius*std::cos(a), radius*std::sin(a)},
                                      {radius*std::cos(b), radius*std::sin(b)}));
    auto curve = LC_OffsetCurve::adopt(cache, definition, .01);
    REQUIRE(curve);
    Info info(drawing);
    Registry registry(&drawing.context);
    auto* descriptor = registry.findEntityMatchDescriptor(RS2::EntityOffsetCurve);
    REQUIRE(descriptor);
    for (unsigned pass = 0; pass != 2; ++pass) {
        REQUIRE(info.processEntity(curve.get()));
        CHECK(info.text("Side") == (inward ? "Inward" : "Outward"));
        CHECK(matches<bool>(descriptor, "towardCentre", curve.get(), inward));
        curve->revertDirection();
        REQUIRE(curve->hasEditableDefinition());
    }
}

TEST_CASE("F cached length refuses non-2D or nonfinite display endpoints", "[lc-offsetcurve][f-names]") {
    Drawing drawing;
    auto* curve = drawing.native();
    const double z = GENERATE(1., std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN());
    auto* line = dynamic_cast<RS_Line*>(curve->entityAt(0));
    REQUIRE(line);
    auto point = line->getStartpoint(); point.z = z;
    line->setStartpoint(point);
    lengthUnavailable(drawing, curve);
}

TEST_CASE("F cached length refuses a display child with a foreign parent", "[lc-offsetcurve][f-names]") {
    Drawing drawing;
    auto* curve = drawing.native();
    auto* child = curve->entityAt(0);
    REQUIRE(child);
    struct RestoreParent {
        RS_Entity* child;
        RS_EntityContainer* parent;
        ~RestoreParent() { child->setParent(parent); }
    } restore{child, child->getParent()};
    child->setParent(&drawing.graphic);
    lengthUnavailable(drawing, curve);
}

TEST_CASE("F cached length never enters a malformed nested display container", "[lc-offsetcurve][f-names]") {
    Drawing drawing;
    auto* curve = drawing.native();
    class Nested final : public RS_EntityContainer {
    public:
        explicit Nested(RS_EntityContainer* parent) : RS_EntityContainer(parent) {}
        double getLength() const override { ++reads; return 999.; }
        mutable unsigned reads = 0;
    };
    auto* nested = new Nested(curve);
    curve->addEntity(nested);
    lengthUnavailable(drawing, curve);
    CHECK(nested->reads == 0);
}

TEST_CASE("F real selection options include native under the Spline checkbox", "[lc-offsetcurve][f-names][gui]") {
    Drawing drawing;
    auto* curve = drawing.native();
    LC_ActionSelectWindow action(&drawing.context, true);
    action.setSelectAllEntityTypes(false);
    action.setEntityTypesToSelect({RS2::EntityOffsetCurve});
    WindowOptions widget;
    widget.doUpdateByAction(&action);
    auto* spline = widget.findChild<QCheckBox*>("cbSpline");
    auto* all = widget.findChild<QCheckBox*>("cbAll");
    REQUIRE(spline); REQUIRE(all);
    CHECK(spline->isChecked());
    spline->setChecked(false);
    CHECK_FALSE(action.getEntityTypesToSelect().contains(RS2::EntityOffsetCurve));
    spline->setChecked(true);
    const auto types = action.getEntityTypesToSelect();
    CHECK(types.contains(RS2::EntitySpline));
    CHECK(types.contains(RS2::EntityOffsetCurve));
    RS_Selection selection(&drawing.graphic, drawing.view.getViewPort());
    selection.selectWindow(types, {-10, -10}, {10, 10}, true, false);
    CHECK(curve->isSelected());
    all->setChecked(true);
    CHECK(action.isSelectAllEntityTypes());
}

TEST_CASE("F native quick info refreshes same identity without control rows", "[lc-offsetcurve][f-names]") {
    Drawing drawing;
    auto* curve = drawing.native();
    Info info(drawing);
    REQUIRE(info.processEntity(curve->entityAt(0)));
    CHECK(info.getEntityId() == curve->getId());
    REQUIRE(info.number("Distance"));
    CHECK(*info.number("Distance") == 1.);
    CHECK(info.text("Side") == "Outward");
    CHECK(info.text("Source") == "Ellipse (embedded definition)");
    const auto before = info.generateView();
    CHECK(before.contains("OFFSET CURVE"));
    CHECK_FALSE(before.contains("Control Point"));
    CHECK_FALSE(before.contains("Degree"));
    const auto shortText = info.getEntityDescription(curve, RS2::DescriptionCatched);
    const auto longText = info.getEntityDescription(curve, RS2::DescriptionLong);
    CHECK(shortText != longText);
    curve->scale({0, 0}, {2, 2});
    REQUIRE(curve->hasEditableDefinition());
    REQUIRE(info.processEntity(curve));
    CHECK(*info.number("Distance") == 2.);
    CHECK(info.generateView() != before);
    curve->getData().controlPoints.front().x += .5;
    const auto controls = curve->getData().controlPoints;
    REQUIRE(info.processEntity(curve));
    CHECK(info.text("Definition") == "Inconsistent");
    CHECK_FALSE(info.number("Distance"));
    CHECK(info.getEntityDescription(curve, RS2::DescriptionLong).contains("Inconsistent"));
    CHECK(curve->getData().controlPoints == controls);
    CHECK(info.getEntityDescription(nullptr, RS2::DescriptionLong).isEmpty());
}

TEST_CASE("F real Quick Info widget displays a native owner passed as a resolved chord", "[lc-offsetcurve][f-names][gui]") {
    Drawing drawing;
    auto* curve = drawing.native();
    LC_QuickInfoWidget widget(nullptr, {});
    widget.setGraphicView(&drawing.view);
    widget.processEntity(curve->entityAt(0));
    auto* text = widget.findChild<QTextBrowser*>("pteInfo");
    REQUIRE(text);
    CHECK(text->toPlainText().contains("OFFSET CURVE"));
    CHECK(text->toPlainText().contains("Outward"));
    CHECK_FALSE(text->toPlainText().contains("Control Point"));
    curve->scale({0, 0}, {2, 3});
    REQUIRE_FALSE(curve->isDefined());
    widget.processEntity(curve);
    CHECK(text->toPlainText().contains("Undefined"));
    CHECK(text->toPlainText().contains("Unavailable"));
    widget.setGraphicView(nullptr);
}

TEST_CASE("F OC activators survive actual assignment copy and serialization", "[lc-offsetcurve][f-names][gui]") {
    REQUIRE(lc::test::application());
    auto original = activator(RS2::EntityOffsetCurve);
    original.setMenuName("NativeMenu");
    CHECK(original.getEntityTypeStr() == "OC");
    CHECK(original.getShortcutView().contains("Offset Curve"));
    std::unique_ptr<LC_MenuActivator> decoded(LC_MenuActivator::fromShortcut(original.getShortcut()));
    REQUIRE(decoded);
    CHECK(decoded->isEntityRequired());
    CHECK(decoded->getEntityType() == RS2::EntityOffsetCurve);
    std::unique_ptr<LC_MenuActivator> copied(original.getCopy());
    CHECK(copied->getEntityType() == RS2::EntityOffsetCurve);
    LC_MenuActivator assigned;
    original.copyTo(assigned);
    assigned.update();
    CHECK(assigned.getEntityTypeStr() == "OC");
    QList<LC_MenuActivator*> existing;
    LC_DlgMenuAssigner dialog(nullptr, copied.get(), &existing);
    auto* combo = dialog.findChild<QComboBox*>("cbEntityContext");
    REQUIRE(combo);
    CHECK(combo->currentData().toString() == "OC");
    combo->setCurrentIndex(combo->findData("SL"));
    CHECK(copied->getEntityType() == RS2::EntitySpline);
    combo->setCurrentIndex(combo->findData("OC"));
    CHECK(copied->getEntityType() == RS2::EntityOffsetCurve);
    CHECK(copied->isEntityRequired());
}

TEST_CASE("F custom menu dispatch promotes native chord locally and keeps ordinary line dispatch", "[lc-offsetcurve][f-names][gui]") {
    Drawing drawing;
    auto* curve = drawing.native();
    auto* store = RS_Settings::instance()->getSettings();
    REQUIRE(store);
    SettingsGroup activators(*store, "Activators");
    QSettings menuStore;
    SettingsGroup menus(menuStore, "CustomMenus");
    for (const auto& [type, name] : {std::pair{RS2::EntityOffsetCurve, "Native"},
                                    std::pair{RS2::EntityLine, "Line"},
                                    std::pair{RS2::EntityUnknown, "Any"},
                                    std::pair{RS2::EntityGraphic, "Either"}}) {
        const auto a = activator(type);
        store->setValue("Activators/" + a.getShortcut(), name);
        menuStore.setValue("CustomMenus/" + QString(name), QStringList{name});
    }
    LC_ActionGroupManager manager(nullptr);
    LC_CreatorInvoker invoker(nullptr, &manager);
    QMouseEvent event(QEvent::MouseButtonRelease, QPointF(0, 0), QPointF(0, 0),
                      Qt::RightButton, Qt::NoButton, Qt::NoModifier);
    QStringList names;
    CHECK_FALSE(invoker.getMenuActionsForMouseEvent(&event, curve, names));
    CHECK(names == QStringList{"Native"});
    CHECK_FALSE(invoker.getMenuActionsForMouseEvent(&event, curve->entityAt(0), names));
    CHECK(names == QStringList{"Native"});
    RS_Line line(nullptr, RS_LineData({0, 0}, {1, 0}));
    CHECK_FALSE(invoker.getMenuActionsForMouseEvent(&event, &line, names));
    CHECK(names == QStringList{"Line"});
    CHECK_FALSE(invoker.getMenuActionsForMouseEvent(&event, nullptr, names));
    CHECK(names == QStringList{"Either"});
}

TEST_CASE("F native context proxies route owner and never expose spline control editing", "[lc-offsetcurve][f-names][gui]") {
    Drawing drawing;
    auto* curve = drawing.native();
    LC_ActionGroupManager manager(nullptr);
    seedActions(manager);
    LC_MenusHolder holder;
    MenuFactory factory(&manager, &holder);
    LC_ActionContext context;
    MenuView view(&context);
    view.setDocument(&drawing.graphic);
    context.setDocumentAndView(&drawing.graphic, &view);
    QMenu menu;
    const RS_Vector click{2, 3};
    factory.createGVMenuEntitySpecific(&menu, &view, curve, click);
    for (const auto& id : {"ModifyOffset", "DrawSplineExplode", "ModifyRevertDirection"}) {
        auto* proxy = findAction(menu, id);
        REQUIRE(proxy);
        unsigned triggered = 0;
        auto connection = QObject::connect(manager.getActionsMap().value(id), &QAction::triggered,
                                           [&] { ++triggered; });
        proxy->trigger();
        CHECK(triggered == 1);
        CHECK(context.getContextMenuActionContextEntity() == curve);
        CHECK(context.getContextMenuActionClickPosition() == click);
        QObject::disconnect(connection);
    }
    for (const auto& id : {"DrawSplinePointsAdd", "DrawSplinePointsAppend", "DrawSplinePointsRemove", "DrawSplinePointsDelTwo"})
        CHECK(findAction(menu, id) == nullptr);
    manager.getActionsMap().value("DrawSplineExplode")->setEnabled(false);
    QMenu disabled;
    factory.createGVMenuEntitySpecific(&disabled, &view, curve, click);
    CHECK(findAction(disabled, "DrawSplineExplode") == nullptr);
    RS_Spline ordinary(nullptr, curve->getData());
    QMenu splineMenu;
    factory.createGVMenuEntitySpecific(&splineMenu, &view, &ordinary, click);
    CHECK(findAction(splineMenu, "DrawSplinePointsAdd") != nullptr);
}

TEST_CASE("F public popup normalizes chord context and cleans temporary owner highlight", "[lc-offsetcurve][f-names][gui]") {
    Drawing drawing;
    auto* curve = drawing.native();
    LC_ActionGroupManager manager(nullptr);
    seedActions(manager);
    LC_MenusHolder holder;
    MenuFactory factory(&manager, &holder);
    LC_ActionContext context;
    MenuView view(&context);
    view.setDocument(&drawing.graphic);
    context.setDocumentAndView(&drawing.graphic, &view);
    QStringList names{"ModifyOffset"};
    std::unique_ptr<QMenu> menu(factory.createGraphicViewPopupMenu(&view, curve->entityAt(0), {3, 4}, names, false));
    REQUIRE(menu);
    CHECK(curve->isSelected());
    auto* proxy = findAction(*menu, "ModifyOffset");
    REQUIRE(proxy);
    proxy->trigger();
    CHECK(context.getContextMenuActionContextEntity() == curve);
    CHECK(QMetaObject::invokeMethod(menu.get(), "aboutToHide", Qt::DirectConnection));
    CHECK_FALSE(curve->isSelected());
    CHECK(view.redraws >= 2);
}
