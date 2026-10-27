/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <QtWidgets>
#include <QScopeGuard>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <memory>
#include <limits>
#include <sstream>
#include <utility>
#include <type_traits>
#include <tuple>
#include <vector>
#include "drw_base.h"
#include "lc_action_modify_entity.h"
#include "lc_actioninteractivepickposition.h"
#include "lc_actiontestsupport.h"
#include "lc_dlg_entityproperties.h"
#include "lc_graphicviewport.h"
#include "lc_entitypropertieseditor.h"
#include "lc_selectedset.h"
#include "lc_offsetcurve.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "../../../../lib/offset_worker/lc_offsetworkerclient.h"
#include "rs_point.h"
#include <tuple>
#include "lc_offsetcurveui.h"
#include "lc_pointpickbutton.h"
#include "lc_propertieseditingwidget_offsetcurve.h"
#include "lc_propertieseditingwidget_ellipse.h"
#include "lc_propertieseditingwidget_line.h"
#include "lc_testsettingsisolation.h"
#include "qc_applicationwindow.h"
#include "qg_layerbox.h"
#include "qg_widgetpen.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_system.h"
#include "rs_undo.h"

namespace {
using NativeWidget = LC_PropertiesEditingWidgetOffsetCurve;
using Input = LC_ActionContext::InteractiveInputInfo;
constexpr int preparationWatchdogMs = 3500;

void requireNativeHelper() {
    const auto path = LC_OffsetWorkerClient::helperPath();
    INFO("The real fixed-path offset helper must be built beside this test executable: " << path.toStdString());
    REQUIRE(QFileInfo(path).isFile());
    REQUIRE(QFileInfo(path).isExecutable());
}

bool pollEventsUntil(const std::function<bool()>& ready) {
    if (ready()) return true;
    QEventLoop loop;
    QTimer poll, watchdog;
    bool expired = false;
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
        if (ready()) loop.quit();
    });
    QObject::connect(&watchdog, &QTimer::timeout, &loop, [&] {
        expired = true;
        loop.quit();
    });
    poll.start(1);
    watchdog.setSingleShot(true); watchdog.start(preparationWatchdogMs);
    loop.exec();
    return !expired && ready();
}

void awaitPreparation(NativeWidget& widget) {
    const QPointer<NativeWidget> live(&widget);
    const auto cancelOnUnwind = qScopeGuard([&] {
        if (live && live->isPreparing()) live->cancelPreparation();
    });
    if (widget.isPreparing()) requireNativeHelper();
    const bool completed = pollEventsUntil([&] { return !live || !live->isPreparing(); });
    INFO("Native preparation did not settle within the 3500ms GUI-test watchdog");
    REQUIRE(completed);
    REQUIRE(live);
    CHECK_FALSE(live->isPreparing());
}

void checkGeneratedWorkerStages(const NativeWidget& widget) {
    REQUIRE(widget.lastWorkerResult());
    const auto& result = *widget.lastWorkerResult();
    REQUIRE(result); CHECK(result.changed); CHECK(result.engineRuns == 1);
    REQUIRE(result.engineStatus); CHECK(*result.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.checkReason); CHECK(*result.checkReason == LC_OffsetCheckResult::Reason::None);
    CHECK(result.checkWork.cacheSpans > 0); CHECK(result.checkWork.vertices > 0);
}

void observeCancelledReplies() {
    // Cover the complete terminate/reap window, not just one posted callback.
    QEventLoop loop;
    QTimer::singleShot(int(LC_OffsetWorkerClient::Deadlines::terminate +
        LC_OffsetWorkerClient::Deadlines::reap + 25), &loop, &QEventLoop::quit);
    loop.exec();
}

class Graphic final : public RS_Graphic {
public:
    using RS_Graphic::countUndoCycles;
};

std::uint64_t bits(double value) {
    std::uint64_t result;
    static_assert(sizeof(result) == sizeof(value));
    std::memcpy(&result, &value, sizeof(result));
    return result;
}
void pointEqual(const RS_Vector& a, const RS_Vector& b) {
    CHECK(bits(a.x) == bits(b.x)); CHECK(bits(a.y) == bits(b.y));
    CHECK(bits(a.z) == bits(b.z)); CHECK(a.valid == b.valid);
}
void definitionEqual(const LC_EllipseOffsetDefinition& a, const LC_EllipseOffsetDefinition& b) {
    pointEqual(a.basis.center, b.basis.center); pointEqual(a.basis.majorP, b.basis.majorP);
    CHECK(bits(a.basis.ratio) == bits(b.basis.ratio));
    CHECK(bits(a.basis.angle1) == bits(b.basis.angle1)); CHECK(bits(a.basis.angle2) == bits(b.basis.angle2));
    CHECK(a.basis.reversed == b.basis.reversed); CHECK(a.basis.isArc == b.basis.isArc);
    CHECK(bits(a.basis.angleDegrees) == bits(b.basis.angleDegrees));
    CHECK(bits(a.basis.startAngleDegrees) == bits(b.basis.startAngleDegrees));
    CHECK(bits(a.basis.otherAngleDegrees) == bits(b.basis.otherAngleDegrees));
    CHECK(bits(a.basis.angularLength) == bits(b.basis.angularLength));
    CHECK(bits(a.distance) == bits(b.distance)); CHECK(bits(a.relTolerance) == bits(b.relTolerance));
    CHECK(a.towardCentre == b.towardCentre); CHECK(a.cacheReversed == b.cacheReversed);
}

struct Snapshot {
    LC_EllipseOffsetDefinition definition;
    RS_SplineData spline;
    std::vector<RS_Vector> grips;
    LC_OffsetCurve::OwnedTail tail;
    double tolerance;
    bool defined, consistent;
    unsigned flags, ownedFlags;
    unsigned long long id;
    RS_Layer* layer;
    RS_Pen pen;
    RS_EntityContainer* parent;
    quint32 handle;
    const RS_Vector* storage;
    std::vector<const RS_Entity*> children;
    explicit Snapshot(const LC_OffsetCurve& curve)
        : definition(curve.definition()), spline(curve.getData()), grips(curve.cachedDefinitionGrips()),
          tail(curve.unknownOwnedTail()), tolerance(curve.checkedWorldTolerance()),
          defined(curve.isDefined()), consistent(curve.isConsistent()), flags(curve.getFlags()),
          ownedFlags(curve.unknownOwnedFlags()), id(curve.getId()), layer(curve.getLayer()),
          pen(curve.getPen(false)), parent(curve.getParent()), handle(curve.sourceHandle()),
          storage(curve.getData().controlPoints.data()) {
        for (const auto* child : curve) children.push_back(child);
    }
    void geometry(const LC_OffsetCurve& curve, bool stableStorage = true) const {
        definitionEqual(curve.definition(), definition);
        CHECK(RS_Spline::geometryFingerprint(curve.getData()) == RS_Spline::geometryFingerprint(spline));
        REQUIRE(curve.getData().controlPoints.size() == spline.controlPoints.size());
        for (std::size_t i = 0; i < spline.controlPoints.size(); ++i) pointEqual(curve.getData().controlPoints[i], spline.controlPoints[i]);
        CHECK(curve.getData().knotslist == spline.knotslist); CHECK(curve.getData().weights == spline.weights);
        REQUIRE(curve.cachedDefinitionGrips().size() == grips.size());
        for (std::size_t i = 0; i < grips.size(); ++i) pointEqual(curve.cachedDefinitionGrips()[i], grips[i]);
        CHECK(bits(curve.checkedWorldTolerance()) == bits(tolerance));
        CHECK(curve.isDefined() == defined); CHECK(curve.isConsistent() == consistent);
        CHECK(curve.unknownOwnedFlags() == ownedFlags);
        REQUIRE(curve.unknownOwnedTail().size() == tail.size());
        for (std::size_t i = 0; i < tail.size(); ++i) {
            const auto& actual = curve.unknownOwnedTail()[i];
            CHECK(actual.code == tail[i].code);
            REQUIRE(actual.value.index() == tail[i].value.index());
            std::visit([&](const auto& expected) {
                using Value = std::decay_t<decltype(expected)>;
                const auto& value = std::get<Value>(actual.value);
                if constexpr (std::is_same_v<Value, RS_Vector>) pointEqual(value, expected);
                else if constexpr (std::is_same_v<Value, double>) CHECK(bits(value) == bits(expected));
                else CHECK(value == expected);
            }, tail[i].value);
        }
        if (stableStorage) {
            CHECK(curve.getData().controlPoints.data() == storage);
            REQUIRE(curve.count() == children.size());
            for (std::size_t i = 0; i < children.size(); ++i) CHECK(curve.entityAt(static_cast<int>(i)) == children[i]);
        }
    }
    void exact(const LC_OffsetCurve& curve) const {
        geometry(curve);
        CHECK(curve.getId() == id); CHECK(curve.getFlags() == flags);
        CHECK(curve.getLayer() == layer); CHECK(curve.getPen(false) == pen);
        CHECK(curve.getParent() == parent); CHECK(curve.sourceHandle() == handle);
        CHECK(curve.getUserDefVar("H-dialog") == "retained");
        REQUIRE(curve.getDrwExtData().size() == 2);
        CHECK(*curve.getDrwExtData()[0]->content.s == "FOREIGN_H_DIALOG");
        CHECK(*curve.getDrwExtData()[1]->content.s == "unchanged");
    }
};

std::unique_ptr<LC_OffsetCurve> makeArc(Graphic& graphic) {
    LC_EllipseOffsetDefinition definition;
    definition.basis = RS_EllipseData{{0, 0}, {1, 0}, 1, 0, .1, false};
    definition.distance = .25; definition.relTolerance = .01;
    const RS_Line cache{nullptr, RS_Vector{1.25, 0}, RS_Vector{1.25 * std::cos(.1), 1.25 * std::sin(.1)}};
    auto result = LC_OffsetCurve::adopt(cache, definition, .01);
    REQUIRE(result);
    result->setParent(&graphic); result->setLayer(graphic.getActiveLayer());
    result->setPen(RS_Pen{RS_Color{17, 31, 47}, RS2::Width11, RS2::DashLine});
    result->setSourceHandle(0x71);
    result->setUserDefVar("H-dialog", "retained");
    result->setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("FOREIGN_H_DIALOG")),
                          std::make_shared<DRW_Variant>(1000, std::string("unchanged"))});
    REQUIRE(result->setOwnedExtensions(0x10, {{1000, QStringLiteral("owned")},
        {1010, RS_Vector{3, 4}}, {1040, .125}, {1070, std::int16_t{7}}}));
    return result;
}

std::unique_ptr<LC_OffsetCurve> makeDyadicRing(Graphic& graphic) {
    // Match the exact-translation fixture used by the native edit tests.
    RS_SplineData data{3, false};
    std::array<RS_Vector, 4> net{{{2, 0}, {2, 9.0/8}, {9.0/8, 2}, {0, 2}}};
    for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
        for (unsigned i = quadrant ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i]);
        for (auto& point : net) point = {-point.y, point.x};
    }
    for (unsigned i = 0; i <= 4; ++i)
        for (unsigned j = 0; j < (i == 0 || i == 4 ? 4u : 3u); ++j) data.knotslist.push_back(i);
    data.weights.assign(data.controlPoints.size(), 1); data.m_closedFlag = true;
    const RS_Spline cache{nullptr, data};
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1}; definition.distance = 1; definition.relTolerance = .01;
    auto result = LC_OffsetCurve::adopt(cache, definition, .02);
    REQUIRE(result);
    result->setParent(&graphic); result->setLayer(graphic.getActiveLayer());
    return result;
}

void key(QWidget& widget, int code, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
    for (auto type : {QEvent::KeyPress, QEvent::KeyRelease}) {
        QKeyEvent event(type, code, modifiers);
        QApplication::sendEvent(&widget, &event);
    }
}
void click(QWidget& widget) {
    widget.setFocus(Qt::MouseFocusReason);
    const QPointF point(widget.rect().center());
    for (auto type : {QEvent::MouseButtonPress, QEvent::MouseButtonRelease}) {
        QMouseEvent event(type, point, QPointF(widget.mapToGlobal(point.toPoint())), Qt::LeftButton,
            type == QEvent::MouseButtonPress ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(&widget, &event);
    }
}
void type(QLineEdit& editor, const QString& text) {
    editor.setFocus(Qt::OtherFocusReason);
    REQUIRE(editor.hasFocus());
    editor.selectAll();
    for (const auto character : text) {
        QKeyEvent event(QEvent::KeyPress, 0, Qt::NoModifier, QString(character));
        QApplication::sendEvent(&editor, &event);
    }
}
QLineEdit& edit(QWidget& widget, const char* name) {
    auto* result = widget.findChild<QLineEdit*>(name);
    REQUIRE(result != nullptr); return *result;
}
NativeWidget& native(LC_DlgEntityProperties& dialog) {
    auto* result = dialog.findChild<NativeWidget*>();
    REQUIRE(result != nullptr); return *result;
}
void submitInput(QWidget& widget, const char* name, const QString& value) {
    auto& editor = edit(widget, name);
    type(editor, value);
    key(editor, Qt::Key_Tab); // actual focus-out editingFinished, not a direct slot invocation
}
void enter(QWidget& widget, const char* name, const QString& value) {
    submitInput(widget, name, value);
    if (auto* offset = qobject_cast<NativeWidget*>(&widget)) awaitPreparation(*offset);
    else QApplication::processEvents();
}
void button(LC_DlgEntityProperties& dialog, QDialogButtonBox::StandardButton which) {
    auto* box = dialog.findChild<QDialogButtonBox*>("buttonBox");
    REQUIRE(box != nullptr); REQUIRE(box->button(which) != nullptr);
    click(*box->button(which));
}
void choose(QComboBox& box, int index) {
    REQUIRE(index >= 0); REQUIRE(index < box.count());
    click(box); // navigate the popup without activating intermediate entries such as Custom Colour
    auto* popup = box.view(); REQUIRE(popup != nullptr);
    key(*popup, Qt::Key_Home);
    for (int i = 0; i < index; ++i) key(*popup, Qt::Key_Down);
    key(*popup, Qt::Key_Return);
    CHECK(box.currentIndex() == index);
}

struct Fixture {
    QApplication* app = lc::test::application();
    Graphic graphic;
    lc::test::TestGraphicView view;
    std::unique_ptr<LC_OffsetCurve> curve;
    std::unique_ptr<LC_DlgEntityProperties> dialog;
    Fixture() {
        REQUIRE(app != nullptr);
        graphic.initForNewDocument(); graphic.onLoadingCompleted(); view.setDocument(&graphic);
        curve = makeArc(graphic);
    }
    void show(std::function<bool()> nativeGuard = {}) {
        dialog = std::make_unique<LC_DlgEntityProperties>(nullptr, view.getViewPort(), curve.get(),
            Input::NOTNEEDED, QString{}, 0, 0, std::move(nativeGuard));
        dialog->show(); dialog->activateWindow();
        QApplication::processEvents();
    }
};

// Exercise the real application singleton required by ModifyEntity, with the
// same isolated settings/source-resource initialization as lc_docklayout_tests.
std::unique_ptr<QC_ApplicationWindow>* windowOwner = nullptr;
class WindowCleanup final : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;
    void testRunEnded(const Catch::TestRunStats&) override {
        if (windowOwner != nullptr) windowOwner->reset();
    }
};
CATCH_REGISTER_LISTENER(WindowCleanup)
QC_ApplicationWindow& actionWindow() {
    REQUIRE(lc::test::application() != nullptr);
    REQUIRE(lc::test::SettingsIsolation::isActive());
    static const bool initialized = [] {
        qApp->setQuitOnLastWindowClosed(false);
        RS_DEBUG->setLevel(RS_Debug::D_NOTHING);
        RS_SYSTEM->init("LibreCAD", "3.0", "librecad", QString(LIBRECAD_SOURCE_DIR) + "/unix");
        QSettings settings;
        settings.setValue("Startup/FirstLoad", true);
        settings.setValue("Defaults/AutoBackupDocument", false);
        settings.setValue("Paths/OtherSettingsDir", lc::test::SettingsIsolation::directory());
        return true;
    }();
    (void)initialized;
    windowOwner = &QC_ApplicationWindow::getAppWindow();
    return **windowOwner;
}

class Context final : public LC_ActionContext {
public:
    QStringList messages;
    void commandMessage(const QString& message) override { messages.append(message); }
    std::unique_ptr<LC_ActionInteractivePickPosition> picker;
    void setCurrentAction(RS2::ActionType type, void*) override {
        REQUIRE(type == RS2::ActionInteractivePickPoint);
        picker = std::make_unique<LC_ActionInteractivePickPosition>(this, type);
        picker->init(0);
        QTimer::singleShot(0, picker.get(), [this] {
            RS_CoordinateEvent position(RS_Vector{2, -3});
            picker->coordinateEvent(&position); // actual picker stores the WCS result and requests reopen
        });
    }
};
class Action final : public LC_ActionModifyEntity {
public:
    explicit Action(LC_ActionContext* context) : LC_ActionModifyEntity(context) {}
    std::function<void()> beforeNativeCommit;
    void start(RS_Entity* entity) {
        doInitWithContextEntity(entity, RS_Vector{0, 0});
        init(ShowDialog); // real delayed editor, trigger and document batch
    }
protected:
    bool doTriggerModifications(LC_DocumentModificationBatch& batch) override {
        if (getStatus() == InEditing && beforeNativeCommit) beforeNativeCommit();
        return LC_ActionModifyEntity::doTriggerModifications(batch);
    }
};

struct ActionFixture {
    QApplication* app = lc::test::application();
    QC_ApplicationWindow& window = actionWindow();
    Graphic graphic;
    lc::test::TestGraphicView view;
    Context context;
    std::unique_ptr<Action> action;
    LC_OffsetCurve* original = nullptr;
    ActionFixture() {
        graphic.initForNewDocument(); graphic.onLoadingCompleted(); view.setDocument(&graphic);
        context.setDocumentAndView(&graphic, &view);
        auto curve = makeArc(graphic); original = curve.release(); graphic.addEntity(original);
        graphic.setModified(false);
        action = std::make_unique<Action>(&context);
    }
    void drive(const std::function<void(LC_DlgEntityProperties&, unsigned)>& script) {
        QEventLoop loop;
        QTimer poll, timeout;
        QPointer<LC_DlgEntityProperties> handled;
        unsigned openings = 0;
        bool expired = false;
        std::exception_ptr exception;
        poll.setInterval(5);
        QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
            auto* dialog = qobject_cast<LC_DlgEntityProperties*>(QApplication::activeModalWidget());
            if (dialog != nullptr && dialog != handled) {
                dialog->activateWindow();
                if (!dialog->isActiveWindow()) return;
                handled = dialog;
                try { script(*dialog, ++openings); }
                catch (...) { exception = std::current_exception(); dialog->reject(); }
            }
            if (openings && action->mayBeTerminatedExternally() && QApplication::activeModalWidget() == nullptr)
                loop.quit();
        });
        timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, [&] {
            expired = true;
            if (auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) dialog->reject();
            loop.quit();
        });
        poll.start(); timeout.start(preparationWatchdogMs);
        action->start(original);
        loop.exec();
        poll.stop(); timeout.stop();
        if (exception) std::rethrow_exception(exception);
        REQUIRE_FALSE(expired); REQUIRE(openings > 0);
    }
    LC_OffsetCurve& current() {
        LC_OffsetCurve* result = nullptr;
        for (auto* entity : graphic) if (!entity->isDeleted() && entity->rtti() == RS2::EntityOffsetCurve) {
            REQUIRE(result == nullptr); result = static_cast<LC_OffsetCurve*>(entity);
        }
        REQUIRE(result != nullptr); return *result;
    }
};
}

TEST_CASE("Native properties dialog constructs without control net edits or geometry work",
          "[gui][dialog][offsetcurve]") {
    Fixture f; const Snapshot before(*f.curve); f.show();
    auto& widget = native(*f.dialog);
    CHECK(widget.editAttemptCount() == 0); CHECK_FALSE(widget.lastEditResult());
    CHECK_FALSE(widget.lastWorkerResult());
    CHECK_FALSE(widget.isPreparing());
    CHECK(widget.findChildren<QLineEdit*>().size() == 8);
    CHECK(widget.findChild<QTableWidget*>() == nullptr);
    CHECK(widget.findChild<QLabel*>("lRefusal")->wordWrap());
    CHECK(widget.findChild<QLabel*>("lState")->text() == "Defined");
    before.exact(*f.curve);
}

TEST_CASE("Native dialog real field completion edits every source and offset scalar once",
          "[gui][dialog][offsetcurve][fields]") {
    struct Case { const char* name; const char* input; };
    for (const auto& test : {Case{"leDistance", ".3"}, Case{"leCenterX", "2"}, Case{"leCenterY", "-3"},
                            Case{"leMajor", "1.25"}, Case{"leMinor", ".875"}, Case{"leRotation", "15"},
                            Case{"leAngle1", "1"}, Case{"leAngle2", "7"}}) {
        CAPTURE(test.name);
        Fixture f; f.show(); auto& widget = native(*f.dialog);
        enter(widget, test.name, test.input);
        REQUIRE(widget.lastEditResult()); const auto& result = *widget.lastEditResult();
        REQUIRE(result); CHECK(result.changed); CHECK(result.engineRuns <= 1);
        CHECK(widget.editAttemptCount() == 1); CHECK(f.curve->hasEditableDefinition());
        if (std::string(test.name).find("Center") == std::string::npos) {
            CHECK(result.engineRuns == 1); REQUIRE(result.engineStatus); REQUIRE(result.checkReason);
            CHECK(*result.engineStatus == LC_CurveOffsetStatus::Ok);
            CHECK(*result.checkReason == LC_OffsetCheckResult::Reason::None);
            CHECK(result.checkWork.cacheSpans > 0); CHECK(result.checkWork.vertices > 0);
        }
        const auto& definition = f.curve->definition();
        const std::string field = test.name;
        if (field == "leDistance") CHECK(definition.distance == .3);
        if (field == "leCenterX") CHECK(definition.basis.center.x == 2);
        if (field == "leCenterY") CHECK(definition.basis.center.y == -3);
        if (field == "leMajor") CHECK(definition.basis.majorP.magnitude() == Catch::Approx(1.25));
        if (field == "leMinor") CHECK(definition.basis.ratio == Catch::Approx(.875));
        if (field == "leRotation") CHECK(definition.basis.majorP.angle() == Catch::Approx(RS_Math::deg2rad(15.)));
        if (field == "leAngle1") CHECK(definition.basis.angle1 == Catch::Approx(RS_Math::deg2rad(1.)));
        if (field == "leAngle2") CHECK(definition.basis.angle2 == Catch::Approx(RS_Math::deg2rad(7.)));
        CHECK(widget.findChild<QLabel*>("lRefusal")->text().isEmpty());
    }
}

TEST_CASE("Native dialog side and reversed traversal use checked single edits",
          "[gui][dialog][offsetcurve][fields]") {
    for (const auto* name : {"cbTowardCentre", "cbReversed"}) {
        Fixture f; f.show(); auto& widget = native(*f.dialog);
        auto* box = widget.findChild<QCheckBox*>(name); REQUIRE(box);
        click(*box);
        REQUIRE(widget.isPreparing());
        awaitPreparation(widget);
        REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
        CHECK(widget.lastEditResult()->changed); CHECK(widget.lastEditResult()->engineRuns == 1);
        CHECK(widget.editAttemptCount() == 1); CHECK(f.curve->hasEditableDefinition());
        if (std::string(name) == "cbTowardCentre") CHECK(f.curve->definition().towardCentre);
        else {
            CHECK(f.curve->definition().basis.reversed);
            CHECK(f.curve->definition().basis.angle1 == .1);
            CHECK(f.curve->definition().basis.angle2 == 0);
        }
    }
}

TEST_CASE("Native dialog refusal and invalid expressions restore exact clone state",
          "[gui][dialog][offsetcurve][refusal]") {
    for (const auto* input : {"0", "-1", "not_a_number("}) {
        CAPTURE(input);
        Fixture f; f.show(); auto& widget = native(*f.dialog);
        const Snapshot before(*f.curve); const auto text = edit(widget, "leDistance").text();
        QGuiApplication::setOverrideCursor(Qt::CrossCursor);
        const auto restoreCursor = qScopeGuard([] { QGuiApplication::restoreOverrideCursor(); });
        submitInput(widget, "leDistance", input);
        CHECK_FALSE(widget.isPreparing());
        CHECK_FALSE(widget.lastWorkerResult());
        REQUIRE(QGuiApplication::overrideCursor()); CHECK(QGuiApplication::overrideCursor()->shape() == Qt::CrossCursor);
        before.exact(*f.curve);
        CHECK(edit(widget, "leDistance").text() == text);
        CHECK_FALSE(widget.findChild<QLabel*>("lRefusal")->text().isEmpty());
        if (std::string(input) != "not_a_number(") {
            REQUIRE(widget.lastEditResult());
            CHECK(widget.lastEditResult()->refusal == LC_OffsetCurveRefusal::InvalidInput);
            CHECK(widget.lastEditResult()->engineRuns == 0);
        }
        else { CHECK_FALSE(widget.lastEditResult()); CHECK(widget.editAttemptCount() == 0); }
    }
}

TEST_CASE("Native dialog bounded producer and checker refusals preserve accounting and restore",
          "[gui][dialog][offsetcurve][limits]") {
    for (bool checker : {false, true}) {
        Fixture f;
        const RS_Ellipse source{&f.graphic, RS_EllipseData{{0, 0}, {5, 0}, 1}};
        auto whole = LC_OffsetCurve::create(source, 1, false, {}, 1e-4);
        REQUIRE(whole); f.curve = std::move(whole.entity); f.show();
        auto& widget = native(*f.dialog);
        const Snapshot before(*f.curve);
        auto limits = LC_OffsetCurveUI::sheetLimits();
        if (checker) limits.checkLimits.maxCacheSpans = 1; else limits.maxSamples = 1;
        widget.setEditLimits(limits);
        enter(widget, "leDistance", "2");
        REQUIRE(widget.lastEditResult()); const auto& result = *widget.lastEditResult();
        CHECK_FALSE(result); CHECK(result.engineRuns == 1); CHECK(widget.editAttemptCount() == 1);
        REQUIRE(result.engineStatus);
        if (checker) {
            CHECK(result.refusal == LC_OffsetCurveRefusal::CheckFailed);
            CHECK(*result.engineStatus == LC_CurveOffsetStatus::Ok);
            REQUIRE(result.checkReason); CHECK(*result.checkReason == LC_OffsetCheckResult::Reason::LimitExceeded);
        }
        else {
            CHECK(result.refusal == LC_OffsetCurveRefusal::Engine);
            CHECK(*result.engineStatus == LC_CurveOffsetStatus::LimitExceeded); CHECK_FALSE(result.checkReason);
        }
        before.geometry(*f.curve);
        CHECK(widget.findChild<QLabel*>("lRefusal")->text().contains("bounded"));
    }
}

TEST_CASE("Native dialog untouched rounded and equivalent exact values do not regenerate",
          "[gui][dialog][offsetcurve][noop]") {
    Fixture f;
    REQUIRE(f.curve->setDistance(.251234567890123));
    f.show(); auto& widget = native(*f.dialog); const Snapshot before(*f.curve);
    for (const auto* name : {"leDistance", "leCenterX", "leCenterY", "leMajor", "leMinor",
                             "leRotation", "leAngle1", "leAngle2"}) {
        auto& editor = edit(widget, name);
        editor.setFocus(); key(editor, Qt::Key_Tab);
    }
    CHECK(widget.editAttemptCount() == 0); before.exact(*f.curve);
    const auto shown = edit(widget, "leDistance").text();
    submitInput(widget, "leDistance", shown); // explicitly typing the rounded display is still unchanged
    CHECK_FALSE(widget.isPreparing());
    CHECK(widget.editAttemptCount() == 0); before.exact(*f.curve);
    submitInput(widget, "leDistance", ".251234567890123+0");
    CHECK_FALSE(widget.isPreparing());
    REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
    CHECK_FALSE(widget.lastEditResult()->changed); CHECK(widget.lastEditResult()->engineRuns == 0);
    CHECK_FALSE(widget.lastWorkerResult());
    before.exact(*f.curve);
}

TEST_CASE("Native field Return followed by focus out does not submit twice",
          "[gui][dialog][offsetcurve][noop]") {
    Fixture f;
    NativeWidget widget;
    widget.setGraphicViewport(f.view.getViewPort()); widget.setEntity(f.curve.get());
    widget.show(); widget.activateWindow(); QApplication::processEvents();
    auto& editor = edit(widget, "leDistance");
    type(editor, ".3"); key(editor, Qt::Key_Return);
    REQUIRE(widget.isPreparing());
    key(editor, Qt::Key_Tab);
    CHECK(widget.editAttemptCount() == 1);
    awaitPreparation(widget);
    REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
    CHECK(widget.editAttemptCount() == 1); const Snapshot after(*f.curve);
    key(editor, Qt::Key_Tab); QApplication::processEvents();
    CHECK(widget.editAttemptCount() == 1); after.exact(*f.curve);
}

TEST_CASE("Native dialog refresh preserves another unfinished field",
          "[gui][dialog][offsetcurve][fields]") {
    Fixture f; f.show(); auto& widget = native(*f.dialog);
    auto& distance = edit(widget, "leDistance");
    type(distance, ".3");
    // Toggling by a keyboard event does not need to steal the dirty editor's focus.
    auto* side = widget.findChild<QCheckBox*>("cbTowardCentre"); REQUIRE(side);
    key(*side, Qt::Key_Space);
    REQUIRE(widget.isPreparing());
    awaitPreparation(widget);
    CHECK(distance.isModified()); CHECK(distance.text() == ".3");
    key(distance, Qt::Key_Tab);
    awaitPreparation(widget);
    CHECK(f.curve->definition().distance == .3);
    CHECK(f.curve->definition().towardCentre); CHECK(widget.editAttemptCount() == 2);
}

TEST_CASE("Native dialog source coordinates and bearings honor UCS while parameters stay raw",
          "[gui][dialog][offsetcurve][ucs]") {
    Fixture f;
    f.view.getViewPort()->createUCS(RS_Vector{10, 20}, RS_Math::deg2rad(90.));
    f.show(); auto& widget = native(*f.dialog);
    enter(widget, "leCenterX", "2");
    const auto center = f.view.getViewPort()->toUCS(f.curve->definition().basis.center);
    CHECK(center.x == Catch::Approx(2)); CHECK(center.y == Catch::Approx(10));
    enter(widget, "leRotation", "15");
    auto* viewport = f.view.getViewPort();
    const auto bearing = viewport->toBasisUCSAngle(viewport->toUCSAngle(f.curve->definition().basis.majorP.angle()));
    CHECK(bearing == Catch::Approx(RS_Math::deg2rad(15.)));
    enter(widget, "leAngle1", "1");
    CHECK(f.curve->definition().basis.angle1 == Catch::Approx(RS_Math::deg2rad(1.)));
}

TEST_CASE("Native dialog unavailable definitions disable geometry but retain attribute editors",
          "[gui][dialog][offsetcurve][state]") {
    for (bool undefined : {false, true}) {
        Fixture f;
        if (undefined) f.curve->scale(RS_Vector{0, 0}, RS_Vector{2, 3});
        else f.curve->getData().controlPoints.front().x += .25;
        const Snapshot before(*f.curve); f.show(); auto& widget = native(*f.dialog);
        CHECK_FALSE(edit(widget, "leDistance").isEnabled());
        CHECK_FALSE(widget.findChild<LC_PointPickButton*>("wPickPointCenter")->isEnabled());
        CHECK(f.dialog->findChild<QG_LayerBox*>("cbLayer")->isEnabled());
        CHECK(f.dialog->findChild<QG_WidgetPen*>("wPen")->isEnabled());
        CHECK(widget.editAttemptCount() == 0); before.exact(*f.curve);
    }
}

TEST_CASE("Native dialog computed length refuses unsafe display caches without geometry work",
          "[gui][dialog][offsetcurve][state]") {
    for (unsigned damage = 0; damage < 3; ++damage) {
        Fixture f;
        auto* child = f.curve->entityAt(0);
        REQUIRE(child);
        const auto restoreParent = qScopeGuard([&] {
            if (damage == 2) child->setParent(f.curve.get());
        });
        if (damage == 0) f.curve->clear();
        if (damage == 1) {
            auto* line = dynamic_cast<RS_Line*>(child);
            REQUIRE(line);
            line->setEndpoint({std::numeric_limits<double>::quiet_NaN(), 1});
        }
        if (damage == 2) child->setParent(nullptr);
        f.show();
        auto& widget = native(*f.dialog);
        auto* length = widget.findChild<QLabel*>("lLengthValue");
        REQUIRE(length);
        CHECK(length->text() == "Unavailable");
        CHECK(widget.editAttemptCount() == 0);
        CHECK_FALSE(widget.lastEditResult());
    }
}

TEST_CASE("Native dialog rechecks a cache invalidated after population",
          "[gui][dialog][offsetcurve][state]") {
    Fixture f; f.show(); auto& widget = native(*f.dialog);
    f.curve->getData().controlPoints.front().x += .25;
    const Snapshot before(*f.curve);
    enter(widget, "leMajor", "-1");
    REQUIRE(widget.lastEditResult());
    CHECK(widget.lastEditResult()->refusal == LC_OffsetCurveRefusal::Inconsistent);
    CHECK(widget.lastEditResult()->engineRuns == 0); before.exact(*f.curve);
    CHECK_FALSE(edit(widget, "leDistance").isEnabled());
}

TEST_CASE("Native dialog real helper preparation keeps the Qt heartbeat alive and seals its clone",
          "[gui][dialog][offsetcurve][async][worker][heartbeat]") {
    Fixture f; requireNativeHelper();
    const Snapshot original(*f.curve);
    auto clone = std::unique_ptr<LC_OffsetCurve>(static_cast<LC_OffsetCurve*>(f.curve->clone()));
    REQUIRE(clone);
    NativeWidget widget;
    widget.setGraphicViewport(f.view.getViewPort()); widget.setEntity(clone.get());
    widget.show(); widget.activateWindow(); QApplication::processEvents();
    const Snapshot before(*clone);
    std::vector<bool> busyChanges;
    QObject observer;
    QObject::connect(&widget, &NativeWidget::preparationBusyChanged, &observer,
        [&](bool busy) { busyChanges.push_back(busy); });
    unsigned heartbeats = 0, busyHeartbeats = 0;
    QTimer heartbeat;
    QObject::connect(&heartbeat, &QTimer::timeout, &observer, [&] {
        ++heartbeats;
        if (widget.isPreparing()) ++busyHeartbeats;
    });
    heartbeat.start(0);
    submitInput(widget, "leDistance", ".3");
    REQUIRE(widget.isPreparing()); CHECK_FALSE(widget.lastEditResult());
    CHECK(widget.editAttemptCount() == 1);
    before.exact(*clone); original.exact(*f.curve);
    awaitPreparation(widget);
    heartbeat.stop();
    CHECK(heartbeats > 0); CHECK(busyHeartbeats > 0);
    CHECK((busyChanges == std::vector<bool>{true, false}));
    REQUIRE(widget.lastEditResult()); const auto& result = *widget.lastEditResult();
    REQUIRE(result); CHECK(result.changed); CHECK(result.engineRuns == 1);
    REQUIRE(result.engineStatus); CHECK(*result.engineStatus == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.checkReason); CHECK(*result.checkReason == LC_OffsetCheckResult::Reason::None);
    CHECK(result.checkWork.cacheSpans > 0); CHECK(result.checkWork.vertices > 0);
    checkGeneratedWorkerStages(widget);
    CHECK(clone->definition().distance == .3); CHECK(clone->hasEditableDefinition());
    CHECK(clone->getId() == before.id); CHECK(clone->getParent() == before.parent);
    CHECK(clone->sourceHandle() == before.handle); CHECK(clone->getFlags() == before.flags);
    CHECK(clone->getPen(false) == before.pen); CHECK(clone->getLayer() == before.layer);
    original.exact(*f.curve);
}

TEST_CASE("Native dialog exact translations remain immediate without a preparation job",
          "[gui][dialog][offsetcurve][async][noop]") {
    Fixture f; f.curve = makeDyadicRing(f.graphic); f.show(); auto& widget = native(*f.dialog);
    std::vector<bool> busyChanges;
    QObject observer;
    QObject::connect(&widget, &NativeWidget::preparationBusyChanged, &observer,
        [&](bool busy) { busyChanges.push_back(busy); });
    const Snapshot before(*f.curve);
    submitInput(widget, "leCenterX", "4");
    CHECK_FALSE(widget.isPreparing()); CHECK(busyChanges.empty());
    CHECK_FALSE(widget.lastWorkerResult());
    REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
    CHECK(widget.lastEditResult()->changed); CHECK(widget.lastEditResult()->engineRuns == 0);
    CHECK(widget.editAttemptCount() == 1);
    CHECK(f.curve->definition().basis.center.x == 4);
    CHECK(f.curve->getId() == before.id); CHECK(f.curve->hasEditableDefinition());
    REQUIRE(f.curve->getData().controlPoints.size() == before.spline.controlPoints.size());
    for (std::size_t i = 0; i < before.spline.controlPoints.size(); ++i)
        pointEqual(f.curve->getData().controlPoints[i], before.spline.controlPoints[i] + RS_Vector{4, 0});
    CHECK(f.curve->getData().knotslist == before.spline.knotslist);
    CHECK(f.curve->getData().weights == before.spline.weights);
    REQUIRE(f.curve->cachedDefinitionGrips().size() == before.grips.size());
    for (std::size_t i = 0; i < before.grips.size(); ++i)
        pointEqual(f.curve->cachedDefinitionGrips()[i], before.grips[i] + RS_Vector{4, 0});
    auto expected = before.definition; expected.basis.center = RS_Vector{4, 0};
    definitionEqual(f.curve->definition(), expected);
    CHECK(bits(f.curve->checkedWorldTolerance()) == bits(before.tolerance));
    CHECK(f.curve->getParent() == before.parent); CHECK(f.curve->getPen(false) == before.pen);
    CHECK(f.curve->sourceHandle() == before.handle); CHECK(f.curve->getLayer() == before.layer);
}

TEST_CASE("Native dialog explicit cancellation and rejection suppress late helper replies and recover",
          "[gui][dialog][offsetcurve][async][cancel]") {
    for (const bool reject : {false, true}) {
        CAPTURE(reject);
        Fixture f; requireNativeHelper(); f.show(); auto& widget = native(*f.dialog);
        const Snapshot before(*f.curve);
        submitInput(widget, "leDistance", ".3");
        REQUIRE(widget.isPreparing()); before.exact(*f.curve);
        if (reject) f.dialog->reject();
        else widget.cancelPreparation();
        CHECK_FALSE(widget.isPreparing());
        if (reject) CHECK_FALSE(f.dialog->isVisible());
        observeCancelledReplies();
        before.exact(*f.curve);
        if (reject) {
            f.dialog->show(); f.dialog->activateWindow(); QApplication::processEvents();
        }
        enter(widget, "leDistance", ".4");
        REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
        CHECK(widget.lastEditResult()->changed); CHECK(widget.lastEditResult()->engineRuns == 1);
        CHECK(widget.editAttemptCount() == 2); CHECK(f.curve->definition().distance == .4);
        CHECK(f.curve->hasEditableDefinition()); CHECK(f.curve->getId() == before.id);
    }
}

TEST_CASE("Native widget rebinding or clearing its target cancels the old helper generation",
          "[gui][dialog][offsetcurve][async][rebind]") {
    for (const bool clear : {false, true}) {
        CAPTURE(clear);
        Fixture f; requireNativeHelper(); f.show(); auto& widget = native(*f.dialog);
        auto replacement = makeArc(f.graphic);
        const auto detachTarget = qScopeGuard([&] { widget.setEntity(nullptr); });
        const Snapshot before(*f.curve), other(*replacement);
        submitInput(widget, "leDistance", ".3");
        REQUIRE(widget.isPreparing());
        widget.setEntity(clear ? nullptr : replacement.get());
        CHECK_FALSE(widget.isPreparing()); CHECK_FALSE(widget.lastEditResult());
        CHECK(widget.editAttemptCount() == 0);
        CHECK(widget.editedEntity() == (clear ? nullptr : replacement.get()));
        observeCancelledReplies();
        before.exact(*f.curve); other.exact(*replacement);
        widget.setEntity(replacement.get());
        enter(widget, "leDistance", ".4");
        REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
        CHECK(widget.lastEditResult()->engineRuns == 1); CHECK(widget.editAttemptCount() == 1);
        CHECK(replacement->definition().distance == .4); CHECK(replacement->hasEditableDefinition());
        before.exact(*f.curve);
    }
}

TEST_CASE("Native widget destruction cancels an outstanding real helper without applying its reply",
          "[gui][dialog][offsetcurve][async][destruction]") {
    Fixture f; requireNativeHelper(); const Snapshot before(*f.curve);
    auto widget = std::make_unique<NativeWidget>();
    widget->setGraphicViewport(f.view.getViewPort()); widget->setEntity(f.curve.get());
    widget->show(); widget->activateWindow(); QApplication::processEvents();
    const QPointer<NativeWidget> live(widget.get());
    submitInput(*widget, "leDistance", ".3");
    REQUIRE(widget->isPreparing()); before.exact(*f.curve);
    QElapsedTimer elapsed; elapsed.start();
    widget.reset();
    CHECK(live.isNull()); CHECK(elapsed.elapsed() < preparationWatchdogMs);
    observeCancelledReplies(); before.exact(*f.curve);
    f.show(); enter(native(*f.dialog), "leDistance", ".4");
    REQUIRE(native(*f.dialog).lastEditResult()); REQUIRE(*native(*f.dialog).lastEditResult());
    CHECK(native(*f.dialog).lastEditResult()->engineRuns == 1);
    CHECK(f.curve->definition().distance == .4); CHECK(f.curve->hasEditableDefinition());
}

TEST_CASE("Native dialog rapid proposals coalesce pending scalars and traversal before applying only the latest",
          "[gui][dialog][offsetcurve][async][supersession]") {
    Fixture f; requireNativeHelper(); f.show(); auto& widget = native(*f.dialog);
    const Snapshot before(*f.curve);
    submitInput(widget, "leDistance", ".3"); REQUIRE(widget.isPreparing());
    submitInput(widget, "leMajor", "1.25");
    submitInput(widget, "leMinor", ".875");
    submitInput(widget, "leDistance", ".4");
    auto* reversed = widget.findChild<QCheckBox*>("cbReversed"); REQUIRE(reversed);
    key(*reversed, Qt::Key_Space);
    REQUIRE(widget.isPreparing()); CHECK_FALSE(widget.lastEditResult());
    CHECK(widget.editAttemptCount() == 5); before.exact(*f.curve);
    awaitPreparation(widget);
    REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
    CHECK(widget.lastEditResult()->changed); CHECK(widget.lastEditResult()->engineRuns == 1);
    CHECK(widget.lastEditResult()->checkWork.cacheSpans > 0);
    auto expected = before.definition;
    expected.distance = .4; expected.basis.majorP = RS_Vector{1.25, 0};
    expected.basis.ratio = .875 / 1.25; expected.basis.reversed = true;
    std::swap(expected.basis.angle1, expected.basis.angle2);
    const RS_Ellipse expectedSource{nullptr, expected.basis};
    expected.basis = expectedSource.getData();
    definitionEqual(f.curve->definition(), expected);
    CHECK(f.curve->hasEditableDefinition()); CHECK(f.curve->getId() == before.id);
    const Snapshot latest(*f.curve);
    observeCancelledReplies(); latest.exact(*f.curve);
    CHECK(widget.editAttemptCount() == 5);
}

TEST_CASE("Native dialog an invalid latest proposal cancels older valid preparation",
          "[gui][dialog][offsetcurve][async][supersession][refusal]") {
    struct Case { const char* field; const char* input; bool nativeRefusal; };
    for (const auto& test : {Case{"leDistance", "not_a_number(", false},
                            Case{"leMajor", "-1", false}, Case{"leDistance", "0", true}}) {
        CAPTURE(test.field, test.input);
        Fixture f; requireNativeHelper(); f.show(); auto& widget = native(*f.dialog);
        const Snapshot before(*f.curve);
        submitInput(widget, "leDistance", ".3"); REQUIRE(widget.isPreparing());
        submitInput(widget, test.field, test.input);
        CHECK_FALSE(widget.isPreparing()); before.exact(*f.curve);
        if (test.nativeRefusal) {
            REQUIRE(widget.lastEditResult());
            CHECK(widget.lastEditResult()->refusal == LC_OffsetCurveRefusal::InvalidInput);
            CHECK(widget.lastEditResult()->engineRuns == 0);
        } else CHECK_FALSE(widget.lastEditResult());
        CHECK_FALSE(widget.findChild<QLabel*>("lRefusal")->text().isEmpty());
        const auto attempts = widget.editAttemptCount();
        CHECK(attempts == (test.nativeRefusal ? 2u : 1u));
        observeCancelledReplies(); before.exact(*f.curve);
        CHECK(widget.editAttemptCount() == attempts);
        enter(widget, "leDistance", ".4");
        REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
        CHECK(widget.lastEditResult()->engineRuns == 1);
        CHECK(f.curve->definition().distance == .4); CHECK(f.curve->hasEditableDefinition());
    }
}

TEST_CASE("Native dialog direct Accept and field Return veto pending preparation then accept once",
          "[gui][dialog][offsetcurve][async][accept][return]") {
    for (const bool returnKey : {false, true}) {
        CAPTURE(returnKey);
        Fixture f; requireNativeHelper(); f.show(); auto& widget = native(*f.dialog);
        const Snapshot before(*f.curve);
        unsigned accepted = 0;
        QObject observer;
        QObject::connect(f.dialog.get(), &QDialog::accepted, &observer, [&] { ++accepted; });
        auto& editor = edit(widget, "leDistance"); type(editor, ".3");
        if (returnKey) key(editor, Qt::Key_Return);
        else f.dialog->accept();
        CHECK(f.dialog->isVisible()); REQUIRE(widget.isPreparing());
        CHECK(accepted == 0); CHECK(widget.editAttemptCount() == 1);
        CHECK_FALSE(widget.lastEditResult()); before.exact(*f.curve);
        key(editor, Qt::Key_Return);
        CHECK(f.dialog->isVisible()); CHECK(accepted == 0); CHECK(widget.editAttemptCount() == 1);
        awaitPreparation(widget);
        REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
        CHECK(widget.lastEditResult()->engineRuns == 1);
        const Snapshot prepared(*f.curve);
        if (returnKey) key(editor, Qt::Key_Return);
        else f.dialog->accept();
        CHECK_FALSE(f.dialog->isVisible()); CHECK(f.dialog->result() == QDialog::Accepted);
        CHECK(accepted == 1); CHECK(widget.editAttemptCount() == 1);
        prepared.exact(*f.curve);
    }
}

TEST_CASE("Native dialog point picker closure is vetoed until focused pending input finishes",
          "[gui][dialog][offsetcurve][async][picker]") {
    Fixture f; requireNativeHelper(); f.show(); auto& widget = native(*f.dialog);
    auto* picker = widget.findChild<LC_PointPickButton*>("wPickPointCenter"); REQUIRE(picker);
    const Snapshot before(*f.curve);
    type(edit(widget, "leDistance"), ".3");
    click(*picker->getButton());
    REQUIRE(widget.isPreparing()); CHECK(f.dialog->isVisible());
    CHECK(f.dialog->isInteractiveInputRequested() == Input::NOTNEEDED);
    CHECK(f.dialog->getInteractiveInputTag().isEmpty());
    before.exact(*f.curve);
    awaitPreparation(widget);
    REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
    CHECK(widget.lastEditResult()->engineRuns == 1); CHECK(widget.editAttemptCount() == 1);
    const Snapshot prepared(*f.curve);
    click(*picker->getButton());
    CHECK_FALSE(f.dialog->isVisible()); CHECK(f.dialog->result() == QDialog::Accepted);
    CHECK(f.dialog->isInteractiveInputRequested() == Input::POINT);
    CHECK(f.dialog->getInteractiveInputTag() == "center");
    prepared.exact(*f.curve);
}

TEST_CASE("Native dialog helper replies cannot overwrite a changed input snapshot",
          "[gui][dialog][offsetcurve][async][snapshot][stale]") {
    for (unsigned change = 0; change < 4; ++change) {
        CAPTURE(change);
        Fixture f; requireNativeHelper(); f.show(); auto& widget = native(*f.dialog);
        submitInput(widget, "leDistance", ".3"); REQUIRE(widget.isPreparing());
        if (change == 0) f.curve->getData().controlPoints.front().x += .125;
        if (change == 1) {
            auto basis = f.curve->definition().basis; basis.center = RS_Vector{2, -3};
            const auto translated = f.curve->setBasis(basis);
            REQUIRE(translated); REQUIRE(translated.changed); CHECK(translated.engineRuns <= 1);
        }
        if (change == 2) f.curve->setSourceHandle(0x99);
        if (change == 3) {
            auto* line = dynamic_cast<RS_Line*>(f.curve->entityAt(0)); REQUIRE(line);
            line->setEndpoint(line->getEndpoint() + RS_Vector{.125, 0});
        }
        const Snapshot changed(*f.curve);
        awaitPreparation(widget);
        changed.exact(*f.curve); CHECK(f.curve->definition().distance == .25);
        CHECK_FALSE(widget.lastEditResult()); checkGeneratedWorkerStages(widget);
        CHECK(widget.editAttemptCount() == 1);
        observeCancelledReplies(); changed.exact(*f.curve);
        CHECK_FALSE(widget.lastEditResult()); checkGeneratedWorkerStages(widget);
    }
}

TEST_CASE("Native dialog revoked preparation guards retain child stages without adopting the reply",
          "[gui][dialog][offsetcurve][async][context][guard][worker]") {
    for (const bool constructorGuard : {false, true}) {
        CAPTURE(constructorGuard);
        Fixture f; requireNativeHelper();
        bool current = true;
        unsigned guardCalls = 0;
        const auto guard = [&] {
            ++guardCalls;
            return current;
        };
        if (constructorGuard) f.show(guard);
        else f.show();
        auto& widget = native(*f.dialog);
        if (!constructorGuard) widget.setPreparationGuard(guard);
        const auto clearGuard = qScopeGuard([&] {
            widget.cancelPreparation(); widget.setPreparationGuard({});
        });
        const Snapshot before(*f.curve);
        submitInput(widget, "leDistance", ".3");
        REQUIRE(widget.isPreparing()); CHECK_FALSE(widget.lastEditResult());
        CHECK_FALSE(widget.lastWorkerResult()); before.exact(*f.curve);
        const auto admittedCalls = guardCalls; REQUIRE(admittedCalls > 0);
        current = false; // Revoke owner authority before Qt can deliver the real child's reply.
        awaitPreparation(widget);
        CHECK(guardCalls > admittedCalls);
        CHECK_FALSE(widget.lastEditResult()); checkGeneratedWorkerStages(widget);
        CHECK(widget.editAttemptCount() == 1); CHECK(widget.editedEntity() == f.curve.get());
        CHECK_FALSE(widget.findChild<QLabel*>("lRefusal")->text().isEmpty());
        CHECK(f.graphic.countUndoCycles() == 0); before.exact(*f.curve);
        observeCancelledReplies();
        CHECK_FALSE(widget.lastEditResult()); checkGeneratedWorkerStages(widget);
        before.exact(*f.curve);
        current = true;
        submitInput(widget, "leDistance", ".3");
        REQUIRE(widget.isPreparing()); CHECK_FALSE(widget.lastEditResult());
        CHECK_FALSE(widget.lastWorkerResult()); before.exact(*f.curve);
        awaitPreparation(widget);
        REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
        CHECK(widget.lastEditResult()->changed); CHECK(widget.lastEditResult()->engineRuns == 1);
        checkGeneratedWorkerStages(widget);
        CHECK(widget.editAttemptCount() == 2); CHECK(widget.editedEntity() == f.curve.get());
        CHECK(f.curve->definition().distance == .3); CHECK(f.curve->hasEditableDefinition());
        CHECK(f.curve->getId() == before.id); CHECK(f.curve->getParent() == before.parent);
        CHECK(f.curve->getLayer() == before.layer); CHECK(f.curve->getPen(false) == before.pen);
        CHECK(f.curve->sourceHandle() == before.handle); CHECK(f.curve->getFlags() == before.flags);
        CHECK(widget.findChild<QLabel*>("lRefusal")->text().isEmpty());
        CHECK(f.graphic.countUndoCycles() == 0);
    }
}

TEST_CASE("Native widget viewport or document context swaps invalidate pending helper replies",
          "[gui][dialog][offsetcurve][async][context][stale]") {
    for (const bool viewportSwap : {false, true}) {
        CAPTURE(viewportSwap);
        Fixture f; requireNativeHelper();
        Graphic other; other.initForNewDocument(); other.onLoadingCompleted();
        lc::test::TestGraphicView otherView; otherView.setDocument(&other);
        auto* currentView = &f.view;
        f.show([&] { return currentView == &f.view && currentView->getDocument() == &f.graphic; });
        auto& widget = native(*f.dialog);
        const auto restoreContext = qScopeGuard([&] {
            widget.cancelPreparation(); widget.setPreparationGuard({});
            widget.setGraphicViewport(f.view.getViewPort());
            f.view.setDocument(&f.graphic);
        });
        const Snapshot before(*f.curve);
        submitInput(widget, "leDistance", ".3"); REQUIRE(widget.isPreparing());
        if (viewportSwap) {
            currentView = &otherView; widget.setGraphicViewport(currentView->getViewPort());
        }
        else f.view.setDocument(&other);
        awaitPreparation(widget);
        before.exact(*f.curve); CHECK(other.isEmpty());
        CHECK_FALSE(widget.lastEditResult()); checkGeneratedWorkerStages(widget);
        CHECK(f.graphic.countUndoCycles() == 0); CHECK(other.countUndoCycles() == 0);
        observeCancelledReplies(); before.exact(*f.curve);
        CHECK_FALSE(widget.lastEditResult()); checkGeneratedWorkerStages(widget);
    }
}

TEST_CASE("Native dialog angle direction changes revoke pending helper adoption",
          "[gui][dialog][offsetcurve][async][context][angle-direction]") {
    Fixture f; requireNativeHelper();
    f.graphic.setAnglesBase(M_PI);
    f.show(); auto& widget = native(*f.dialog);
    const Snapshot before(*f.curve);
    auto* viewport = f.view.getViewPort();
    const auto zero = viewport->toWorldAngle(viewport->toAbsUCSAngle(0));
    const auto nonzero = viewport->toWorldAngle(viewport->toAbsUCSAngle(.25));
    const bool direction = f.graphic.areAnglesCounterClockWise();
    submitInput(widget, "leDistance", ".3"); REQUIRE(widget.isPreparing());
    f.graphic.setAnglesCounterClockwise(!direction);
    CHECK(viewport->toWorldAngle(viewport->toAbsUCSAngle(0)) == zero);
    CHECK(viewport->toWorldAngle(viewport->toAbsUCSAngle(.25)) != nonzero);
    awaitPreparation(widget);
    before.exact(*f.curve); CHECK_FALSE(widget.lastEditResult());
    checkGeneratedWorkerStages(widget);
    CHECK(f.graphic.countUndoCycles() == 0);
    f.graphic.setAnglesCounterClockwise(direction);
    enter(widget, "leDistance", ".4");
    REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
    CHECK(f.curve->definition().distance == .4);
}

TEST_CASE("Actual native Edit Properties commits once and undo redo preserve native state",
          "[gui][dialog][offsetcurve][action][undo]") {
    ActionFixture f; const Snapshot before(*f.original);
    std::unique_ptr<Snapshot> edited;
    f.drive([&](LC_DlgEntityProperties& dialog, unsigned opening) {
        REQUIRE(opening == 1); auto& widget = native(dialog);
        enter(widget, "leDistance", ".3");
        REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
        CHECK(widget.lastEditResult()->engineRuns == 1);
        REQUIRE(widget.editedEntity());
        edited = std::make_unique<Snapshot>(*widget.editedEntity());
        before.exact(f.current());
        button(dialog, QDialogButtonBox::Ok);
        CHECK(widget.editAttemptCount() == 1);
    });
    CHECK(f.graphic.countUndoCycles() == 1);
    auto& committed = f.current();
    CHECK(&committed != f.original); CHECK(committed.definition().distance == .3);
    CHECK(committed.hasEditableDefinition()); CHECK(committed.getPen(false) == before.pen);
    REQUIRE(edited); edited->geometry(committed); // Accept did not rebuild even the display children
    const Snapshot after(committed);
    REQUIRE(f.graphic.undo()); CHECK(&f.current() == f.original); before.geometry(f.current(), false);
    REQUIRE(f.graphic.redo()); CHECK(&f.current() == &committed); after.geometry(f.current(), false);
    CHECK(f.graphic.countUndoCycles() == 1);
}

namespace {
std::string dialogUndoHistory(const Graphic& graphic) {
    std::ostringstream out;
    out << static_cast<const RS_Undo&>(graphic);
    return out.str();
}
std::weak_ptr<DRW_Variant> cloneOnlyWitness(NativeWidget& widget) {
    auto* clone = const_cast<LC_OffsetCurve*>(widget.editedEntity());
    REQUIRE(clone);
    auto data = clone->getDrwExtData();
    data.push_back(std::make_shared<DRW_Variant>(1001, std::string("H_DIALOG_CLONE_ONLY")));
    auto witness = std::make_shared<DRW_Variant>(1000, std::string("staged-only"));
    data.push_back(witness);
    clone->setDrwExtData(data);
    return witness;
}
void queueNativeAccept(LC_DlgEntityProperties& dialog, std::exception_ptr& error,
    std::function<void()> mutation) {
    const QPointer<LC_DlgEntityProperties> live(&dialog);
    QTimer::singleShot(0, &dialog, [live, &error, mutation = std::move(mutation)] {
        if (!live) return;
        try {
            mutation();
            button(*live, QDialogButtonBox::Ok);
        } catch (...) {
            error = std::current_exception();
            if (live) live->reject();
        }
    });
}
RS_Line* makePendingRedo(ActionFixture& fixture) {
    auto* marker = new RS_Line(&fixture.graphic, RS_Vector{-2, 0}, RS_Vector{-1, 0});
    REQUIRE(fixture.graphic.undoableModify(fixture.view.getViewPort(), [&](LC_DocumentModificationBatch& batch) {
        batch.add(marker); return true;
    }));
    REQUIRE(fixture.graphic.undo());
    REQUIRE(marker->isDeleted());
    fixture.graphic.setModified(false);
    return marker;
}
}

TEST_CASE("Queued native dialog lock or removal before Accept never replaces and releases its clone",
          "[gui][dialog][offsetcurve][action][stale][ownership]") {
    for (const bool remove : {false, true}) {
        CAPTURE(remove);
        ActionFixture f;
        auto* marker = makePendingRedo(f);
        const Snapshot source(*f.original);
        const auto history = dialogUndoHistory(f.graphic);
        const auto initialCount = f.graphic.count();
        const auto cycles = f.graphic.countUndoCycles();
        std::weak_ptr<DRW_Variant> cloneLifetime;
        std::exception_ptr queuedError;
        f.drive([&](LC_DlgEntityProperties& dialog, unsigned opening) {
            REQUIRE(opening == 1);
            auto& widget = native(dialog);
            enter(widget, "leDistance", ".3");
            REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
            REQUIRE(widget.lastEditResult()->engineRuns == 1);
            cloneLifetime = cloneOnlyWitness(widget);
            REQUIRE_FALSE(cloneLifetime.expired());
            queueNativeAccept(dialog, queuedError, [&] {
                if (remove) {
                    auto* original = f.original;
                    f.original = nullptr;
                    REQUIRE(f.graphic.removeEntity(original)); // actual owned deletion, not just a flag
                } else {
                    REQUIRE(f.original->getLayer(false));
                    f.original->getLayer(false)->lock(true);
                }
            });
        });
        if (queuedError) std::rethrow_exception(queuedError);
        CHECK(cloneLifetime.expired());
        CHECK(f.graphic.count() == initialCount - (remove ? 1u : 0u));
        CHECK(f.graphic.countUndoCycles() == cycles);
        CHECK(dialogUndoHistory(f.graphic) == history);
        CHECK_FALSE(f.graphic.isModified());
        REQUIRE_FALSE(f.context.messages.isEmpty());
        CHECK(f.context.messages.last().contains("not applied"));
        CHECK(f.action->mayBeTerminatedExternally());
        if (!remove) {
            source.exact(*f.original);
            f.original->getLayer(false)->lock(false);
        }
        REQUIRE(f.graphic.redo()); CHECK_FALSE(marker->isDeleted());
    }
}

TEST_CASE("Queued native dialog context or view swap cannot publish into either document",
          "[gui][dialog][offsetcurve][action][stale][context]") {
    for (const bool viewOnly : {false, true}) {
        CAPTURE(viewOnly);
        ActionFixture f;
        Graphic other; other.initForNewDocument(); other.onLoadingCompleted();
        const auto restoreContext = qScopeGuard([&] {
            f.view.setDocument(&f.graphic); f.context.setDocumentAndView(&f.graphic, &f.view);
        });
        const Snapshot source(*f.original);
        const auto history = dialogUndoHistory(f.graphic);
        const auto otherHistory = dialogUndoHistory(other);
        const auto count = f.graphic.count();
        std::weak_ptr<DRW_Variant> cloneLifetime;
        std::exception_ptr queuedError;
        f.drive([&](LC_DlgEntityProperties& dialog, unsigned opening) {
            REQUIRE(opening == 1);
            cloneLifetime = cloneOnlyWitness(native(dialog));
            queueNativeAccept(dialog, queuedError, [&] {
                if (viewOnly) f.view.setDocument(&other);
                else f.context.setDocumentAndView(&other, &f.view);
            });
        });
        if (queuedError) std::rethrow_exception(queuedError);
        CHECK(cloneLifetime.expired());
        CHECK(f.graphic.count() == count); CHECK(other.isEmpty());
        CHECK(dialogUndoHistory(f.graphic) == history); CHECK(dialogUndoHistory(other) == otherHistory);
        source.exact(*f.original);
    }
}

TEST_CASE("Queued native dialog source geometry or owner changes are not overwritten on Accept",
          "[gui][dialog][offsetcurve][action][stale][baseline]") {
    for (const bool changeOwner : {false, true}) {
        CAPTURE(changeOwner);
        ActionFixture f;
        Graphic other; other.initForNewDocument(); other.onLoadingCompleted();
        const auto restoreOwner = qScopeGuard([&] {
            if (f.original) f.original->setParent(&f.graphic);
        });
        const auto history = dialogUndoHistory(f.graphic);
        const auto count = f.graphic.count();
        std::unique_ptr<Snapshot> changedSource;
        std::weak_ptr<DRW_Variant> cloneLifetime;
        std::exception_ptr queuedError;
        f.drive([&](LC_DlgEntityProperties& dialog, unsigned opening) {
            REQUIRE(opening == 1);
            cloneLifetime = cloneOnlyWitness(native(dialog));
            queueNativeAccept(dialog, queuedError, [&] {
                if (changeOwner) f.original->setParent(&other);
                else {
                    const auto edit = f.original->setDistance(.4);
                    REQUIRE(edit); REQUIRE(edit.changed); REQUIRE(edit.engineRuns == 1);
                }
                changedSource = std::make_unique<Snapshot>(*f.original);
            });
        });
        if (queuedError) std::rethrow_exception(queuedError);
        REQUIRE(changedSource); changedSource->exact(*f.original);
        CHECK(cloneLifetime.expired()); CHECK(f.graphic.count() == count);
        CHECK(dialogUndoHistory(f.graphic) == history); CHECK_FALSE(f.graphic.isModified());
    }
}

TEST_CASE("Native dialog validates before its first delayed dereference of a removed source",
          "[gui][dialog][offsetcurve][action][stale][delayed]") {
    ActionFixture f;
    auto tag = std::make_shared<DRW_Variant>(1000, std::string("source-and-pending-clone"));
    const std::weak_ptr<DRW_Variant> lifetime(tag);
    f.original->setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("H_DIALOG_LIFETIME")), tag});
    tag.reset();
    const auto history = dialogUndoHistory(f.graphic);
    QEventLoop loop;
    QTimer poll, timeout;
    bool dialogOpened = false;
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
        if (auto* dialog = qobject_cast<LC_DlgEntityProperties*>(QApplication::activeModalWidget())) {
            dialogOpened = true;
            dialog->reject();
        }
        if (f.action->mayBeTerminatedExternally()) loop.quit();
    });
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    poll.start(5); timeout.setSingleShot(true); timeout.start(1000);
    f.action->start(f.original);
    REQUIRE_FALSE(lifetime.expired());
    auto* original = f.original; f.original = nullptr;
    REQUIRE(f.graphic.removeEntity(original));
    REQUIRE_FALSE(lifetime.expired()); // the pending clone still owns the only remaining payload
    loop.exec();
    poll.stop(); timeout.stop();
    CHECK_FALSE(dialogOpened); CHECK(lifetime.expired());
    CHECK(f.action->mayBeTerminatedExternally()); CHECK(f.graphic.isEmpty());
    CHECK(dialogUndoHistory(f.graphic) == history); CHECK_FALSE(f.graphic.isModified());
}

TEST_CASE("Actual native dialog Accept retains attribute only same value and refused geometry edits",
          "[gui][dialog][offsetcurve][action][attributes]") {
    for (bool refused : {false, true}) {
        ActionFixture f; const Snapshot before(*f.original);
        auto* layer = new RS_Layer("H-dialog-target"); f.graphic.addLayer(layer);
        f.drive([&](LC_DlgEntityProperties& dialog, unsigned) {
            auto& widget = native(dialog);
            if (refused) enter(widget, "leDistance", "0");
            else enter(widget, "leDistance", ".25+0");
            REQUIRE(widget.lastEditResult()); CHECK(widget.lastEditResult()->engineRuns == 0);
            auto* layers = dialog.findChild<QG_LayerBox*>("cbLayer"); REQUIRE(layers);
            choose(*layers, layers->findText("H-dialog-target", Qt::MatchExactly));
            REQUIRE(layers->getLayer() == layer);
            auto* pen = dialog.findChild<QG_WidgetPen*>("wPen"); REQUIRE(pen);
            choose(*pen->cbColor, pen->cbColor->findData(QColor(Qt::red)));
            button(dialog, QDialogButtonBox::Ok);
        });
        CHECK(f.graphic.countUndoCycles() == 1); CHECK(&f.current() != f.original);
        before.geometry(f.current(), false); CHECK(f.current().getLayer() == layer);
        const auto finalPen = f.current().getPen(false);
        CHECK(finalPen.getColor() == RS_Color(Qt::red)); CHECK_FALSE(finalPen == before.pen);
        REQUIRE(f.graphic.undo()); CHECK(f.current().getLayer() == before.layer);
        REQUIRE(f.graphic.redo()); CHECK(f.current().getLayer() == layer); CHECK(f.current().getPen(false) == finalPen);
    }
}

TEST_CASE("Actual native dialog Cancel after geometry and layer edits preserves original and undo",
          "[gui][dialog][offsetcurve][action][cancel]") {
    ActionFixture f;
    f.graphic.select(QList<RS_Entity*>{f.original});
    const Snapshot before(*f.original);
    auto* layer = new RS_Layer("H-dialog-target"); f.graphic.addLayer(layer); f.graphic.setModified(false);
    f.drive([&](LC_DlgEntityProperties& dialog, unsigned) {
        enter(native(dialog), "leDistance", ".3");
        auto* layers = dialog.findChild<QG_LayerBox*>("cbLayer"); REQUIRE(layers);
        choose(*layers, layers->findText("H-dialog-target", Qt::MatchExactly));
        auto* pen = dialog.findChild<QG_WidgetPen*>("wPen"); REQUIRE(pen);
        choose(*pen->cbColor, pen->cbColor->findData(QColor(Qt::red)));
        button(dialog, QDialogButtonBox::Cancel);
    });
    CHECK(&f.current() == f.original); before.exact(*f.original);
    CHECK(f.graphic.countUndoCycles() == 0); CHECK_FALSE(f.graphic.isModified());
}

TEST_CASE("Actual center picker closes and reopens native dialog with one atomic edit",
          "[gui][dialog][offsetcurve][action][picker]") {
    ActionFixture f; const Snapshot before(*f.original);
    f.view.getViewPort()->createUCS(RS_Vector{10, 20}, RS_Math::deg2rad(90.));
    f.drive([&](LC_DlgEntityProperties& dialog, unsigned opening) {
        auto& widget = native(dialog);
        if (opening == 1) {
            auto* picker = widget.findChild<LC_PointPickButton*>("wPickPointCenter"); REQUIRE(picker);
            click(*picker->getButton());
            CHECK(dialog.isInteractiveInputRequested() == Input::POINT);
            CHECK(dialog.getInteractiveInputTag() == "center");
        }
        else {
            REQUIRE(opening == 2);
            awaitPreparation(widget);
            REQUIRE(widget.lastEditResult());
            REQUIRE(*widget.lastEditResult()); CHECK(widget.editAttemptCount() == 1);
            CHECK(widget.lastEditResult()->engineRuns <= 1);
            CHECK(edit(widget, "leCenterX").text() != QString{});
            button(dialog, QDialogButtonBox::Ok);
        }
    });
    CHECK(f.current().definition().basis.center.x == 2); CHECK(f.current().definition().basis.center.y == -3);
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo()); before.geometry(f.current(), false);
    REQUIRE(f.graphic.redo()); CHECK(f.current().definition().basis.center.x == 2);
}


TEST_CASE("Native scalar picker replay uses the same bounded field setter",
          "[gui][dialog][offsetcurve][picker]") {
    struct Case { const char* tag; Input::InputType type; double value; };
    for (const auto& test : {Case{"distance", Input::DISTANCE, .3}, Case{"major", Input::DISTANCE, 1.25},
                            Case{"minor", Input::DISTANCE, .875},
                            Case{"rotation", Input::ANGLE, RS_Math::deg2rad(15.)}}) {
        CAPTURE(test.tag);
        Fixture f;
        f.view.getViewPort()->createUCS(RS_Vector{10, 20}, RS_Math::deg2rad(90.));
        LC_DlgEntityProperties dialog(nullptr, f.view.getViewPort(), f.curve.get(), test.type, test.tag, test.value, 0);
        dialog.show(); QApplication::processEvents(); auto& widget = native(dialog);
        awaitPreparation(widget);
        REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
        CHECK(widget.editAttemptCount() == 1); CHECK(widget.lastEditResult()->engineRuns == 1);
        CHECK(widget.lastEditResult()->checkWork.cacheSpans > 0);
        if (std::string(test.tag) == "distance") CHECK(f.curve->definition().distance == .3);
        if (std::string(test.tag) == "major") CHECK(f.curve->definition().basis.majorP.magnitude() == Catch::Approx(1.25));
        if (std::string(test.tag) == "minor") CHECK(f.curve->definition().basis.ratio == Catch::Approx(.875));
        if (std::string(test.tag) == "rotation") {
            auto* viewport = f.view.getViewPort();
            const auto bearing = viewport->toBasisUCSAngle(viewport->toUCSAngle(f.curve->definition().basis.majorP.angle()));
            CHECK(bearing == Catch::Approx(test.value));
        }
    }
}

TEST_CASE("Actual native same-value Accept still publishes one clone without geometry work",
          "[gui][dialog][offsetcurve][action][noop]") {
    ActionFixture f; const Snapshot before(*f.original);
    std::unique_ptr<Snapshot> edited;
    f.drive([&](LC_DlgEntityProperties& dialog, unsigned) {
        auto& widget = native(dialog);
        enter(widget, "leDistance", ".25+0");
        REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
        CHECK_FALSE(widget.lastEditResult()->changed); CHECK(widget.lastEditResult()->engineRuns == 0);
        REQUIRE(widget.editedEntity()); edited = std::make_unique<Snapshot>(*widget.editedEntity());
        button(dialog, QDialogButtonBox::Ok);
        CHECK(widget.editAttemptCount() == 1);
    });
    CHECK(&f.current() != f.original); CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(edited); edited->geometry(f.current()); before.geometry(f.current(), false);
    REQUIRE(f.graphic.undo()); CHECK(&f.current() == f.original);
    REQUIRE(f.graphic.redo()); CHECK(&f.current() != f.original);
}

TEST_CASE("Actual native Accept completes its focused dirty field exactly once",
          "[gui][dialog][offsetcurve][action][fields]") {
    ActionFixture f; std::unique_ptr<Snapshot> edited;
    f.drive([&](LC_DlgEntityProperties& dialog, unsigned) {
        auto& widget = native(dialog);
        type(edit(widget, "leDistance"), ".3"); // no Tab/Return before clicking OK
        button(dialog, QDialogButtonBox::Ok);
        CHECK(dialog.isVisible()); REQUIRE(widget.isPreparing());
        CHECK_FALSE(widget.lastEditResult());
        CHECK(f.graphic.countUndoCycles() == 0); CHECK(&f.current() == f.original);
        awaitPreparation(widget);
        REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
        CHECK(widget.editAttemptCount() == 1); CHECK(widget.lastEditResult()->engineRuns == 1);
        REQUIRE(widget.editedEntity()); edited = std::make_unique<Snapshot>(*widget.editedEntity());
        button(dialog, QDialogButtonBox::Ok);
        CHECK(widget.editAttemptCount() == 1);
    });
    CHECK(f.current().definition().distance == .3); CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(edited); edited->geometry(f.current());
}

TEST_CASE("Actual native dialog accepts attributes on unavailable definitions without rebuilding",
          "[gui][dialog][offsetcurve][action][state]") {
    for (bool undefined : {false, true}) {
        ActionFixture f;
        if (undefined) f.original->scale(RS_Vector{0, 0}, RS_Vector{2, 3});
        else f.original->getData().controlPoints.front().x += .25;
        const Snapshot before(*f.original);
        auto* layer = new RS_Layer("H-dialog-target"); f.graphic.addLayer(layer);
        std::unique_ptr<Snapshot> edited;
        f.drive([&](LC_DlgEntityProperties& dialog, unsigned) {
            auto& widget = native(dialog);
            CHECK_FALSE(edit(widget, "leDistance").isEnabled());
            auto* layers = dialog.findChild<QG_LayerBox*>("cbLayer"); REQUIRE(layers);
            choose(*layers, layers->findText("H-dialog-target", Qt::MatchExactly));
            REQUIRE(widget.editedEntity()); edited = std::make_unique<Snapshot>(*widget.editedEntity());
            button(dialog, QDialogButtonBox::Ok);
            CHECK(widget.editAttemptCount() == 0);
        });
        CHECK(f.current().getLayer() == layer); CHECK(f.graphic.countUndoCycles() == 1);
        REQUIRE(edited); edited->geometry(f.current()); before.geometry(f.current(), false);
        REQUIRE(f.graphic.undo()); CHECK(&f.current() == f.original);
        REQUIRE(f.graphic.redo()); CHECK(f.current().getLayer() == layer);
    }
}

TEST_CASE("Ordinary ellipse properties retain their own widget and existing setters",
          "[gui][dialog][offsetcurve][ordinary]") {
    Fixture f;
    RS_Ellipse ellipse{&f.graphic, RS_EllipseData{{0, 0}, {5, 0}, .5}};
    LC_DlgEntityProperties dialog(nullptr, f.view.getViewPort(), &ellipse, Input::NOTNEEDED, {}, 0, 0);
    dialog.show(); dialog.activateWindow(); QApplication::processEvents();
    CHECK(dialog.findChild<NativeWidget*>() == nullptr);
    auto* widget = dialog.findChild<LC_PropertiesEditingWidgetEllipse*>(); REQUIRE(widget);
    enter(*widget, "leMajor", "6");
    CHECK(ellipse.rtti() == RS2::EntityEllipse);
    CHECK(ellipse.getMajorRadius() == Catch::Approx(6));
    CHECK(ellipse.getMinorRadius() == Catch::Approx(2.5));
}

namespace {
auto r03Metadata(const LC_OffsetCurve& curve) {
    return std::make_tuple(curve.sourceHandle(), curve.lineage(), curve.ownerLineage(),
        curve.xDictHandle(), curve.materialHandle(), curve.plotStyleHandle(), curve.shadowMode(),
        curve.shadowHandle(), curve.fullVisualStyleHandle(), curve.faceVisualStyleHandle(),
        curve.edgeVisualStyleHandle());
}
void r03Pen(const RS_Pen& actual, const RS_Pen& expected) {
    CHECK(actual == expected); CHECK(actual.getFlags() == expected.getFlags());
    CHECK(actual.getLineTypeId() == expected.getLineTypeId());
    CHECK(bits(actual.getScreenWidth()) == bits(expected.getScreenWidth()));
    CHECK(bits(actual.getAlpha()) == bits(expected.getAlpha()));
    CHECK(bits(actual.dashOffset()) == bits(expected.dashOffset()));
    CHECK(actual.getColor().colorName() == expected.getColor().colorName());
}
/** Independent getter observations AFTER the late edit, before Accept. */
struct R03LastEdit {
    Snapshot geometry;
    decltype(r03Metadata(std::declval<const LC_OffsetCurve&>())) metadata;
    std::vector<quint32> reactors;
    std::vector<std::pair<QString, QString>> variables;
    RS_Vector start, end;
    unsigned childFlags;
    RS_Pen childPen;
    RS_Layer* childLayer;
    explicit R03LastEdit(const LC_OffsetCurve& curve)
        : geometry(curve), metadata(r03Metadata(curve)), reactors(curve.reactorHandles()),
          start(curve.entityAt(0)->getStartpoint()), end(curve.entityAt(0)->getEndpoint()),
          childFlags(curve.entityAt(0)->getFlags()), childPen(curve.entityAt(0)->getPen(false)),
          childLayer(curve.entityAt(0)->getLayer(false)) {
        for (const auto& key : curve.getAllKeys()) variables.emplace_back(key, curve.getUserDefVar(key));
    }
    void check(const LC_OffsetCurve& curve) const {
        geometry.geometry(curve);
        CHECK(curve.getId() == geometry.id); CHECK(curve.getFlags() == geometry.flags);
        CHECK(curve.getLayer() == geometry.layer); r03Pen(curve.getPen(false), geometry.pen);
        CHECK(r03Metadata(curve) == metadata); CHECK(curve.reactorHandles() == reactors);
        REQUIRE(curve.userDefVarCount() == variables.size());
        for (const auto& variable : variables) CHECK(curve.getUserDefVar(variable.first) == variable.second);
        CHECK(curve.entityAt(0)->getFlags() == childFlags);
        r03Pen(curve.entityAt(0)->getPen(false), childPen);
        CHECK(curve.entityAt(0)->getLayer(false) == childLayer);
        pointEqual(curve.entityAt(0)->getStartpoint(), start); pointEqual(curve.entityAt(0)->getEndpoint(), end);
    }
};
}

TEST_CASE("R03 native dialog preserves late provenance tail variables reactors and child attributes",
          "[gui][dialog][offsetcurve][action][r03][metadata]") {
    for (unsigned change = 0; change < 26; ++change) {
        CAPTURE(change);
        ActionFixture f;
        auto* childLayer = new RS_Layer("R03_CHILD_LAYER"); f.graphic.addLayer(childLayer);
        f.original->setLineage(2001); f.original->setOwnerLineage(2002);
        f.original->setXDictHandle(81); f.original->setMaterialHandle(82);
        f.original->setPlotStyleHandle(83); f.original->setShadowMode(1);
        f.original->setShadowHandle(84); f.original->setVisualStyleHandles(85, 86, 87);
        f.original->setReactorHandles({90, 91});
        if (change == 25) REQUIRE(f.original->setOwnedExtensions(0x10, {{1040, 0.}}));
        f.graphic.setModified(false);
        auto before = LC_OffsetCurveEditSnapshot::capture(*f.original); REQUIRE(before);
        const auto history = dialogUndoHistory(f.graphic);
        const auto count = f.graphic.count();
        std::unique_ptr<R03LastEdit> last;
        std::weak_ptr<DRW_Variant> cloneLifetime;
        std::exception_ptr queuedError;
        f.drive([&](LC_DlgEntityProperties& dialog, unsigned opening) {
            REQUIRE(opening == 1); auto& widget = native(dialog);
            enter(widget, "leDistance", ".3");
            REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
            REQUIRE(widget.lastEditResult()->engineRuns == 1);
            cloneLifetime = cloneOnlyWitness(widget);
            queueNativeAccept(dialog, queuedError, [&] {
                auto& curve = *f.original; auto* child = curve.entityAt(0); REQUIRE(child);
                switch (change) {
                case 0: curve.setSourceHandle(991); break;
                case 1: curve.setLineage(3001); break;
                case 2: curve.setOwnerLineage(3002); break;
                case 3: curve.setXDictHandle(993); break;
                case 4: curve.setMaterialHandle(994); break;
                case 5: curve.setPlotStyleHandle(995); break;
                case 6: curve.setShadowMode(2); break;
                case 7: curve.setShadowHandle(996); break;
                case 8: curve.setVisualStyleHandles(1001, 86, 87); break;
                case 9: curve.setVisualStyleHandles(85, 1002, 87); break;
                case 10: curve.setVisualStyleHandles(85, 86, 1003); break;
                case 11: curve.setReactorHandles({17, 19}); break;
                case 12: REQUIRE(curve.setOwnedExtensions(0x20, curve.unknownOwnedTail())); break;
                case 13: REQUIRE(curve.setOwnedExtensions(0x10, {{1000, QStringLiteral("last-tail")}})); break;
                case 14:
                    curve.delUserDefVar("H-dialog");
                    curve.setUserDefVar("H-dialog", "last-value");
                    break;
                case 15: curve.setUserDefVar("last-key", "last-value"); break;
                case 16: curve.delUserDefVar("H-dialog"); break;
                case 17: child->setVisible(false); break;
                case 18: child->setPen(RS_Pen(RS_Color(9, 23, 47), RS2::Width11, RS2::DashLine)); break;
                case 19: child->setLayer(childLayer); break;
                case 20: { auto pen = child->getPen(false); pen.setAlpha(.5f); child->setPen(pen); break; }
                case 21: { auto pen = child->getPen(false); pen.setDashOffset(.125); child->setPen(pen); break; }
                case 22: {
                    auto pen = child->getPen(false); auto color = pen.getColor();
                    color.setColorName("R03_CHILD_COLOR"); pen.setColor(color); child->setPen(pen); break;
                }
                case 23: {
                    auto* line = dynamic_cast<RS_Line*>(child); REQUIRE(line);
                    line->setEndpoint(line->getEndpoint() + RS_Vector{.125, 0}); break;
                }
                case 24: curve.setDrwExtData({std::make_shared<DRW_Variant>(1000, std::string("last-stream"))}); break;
                case 25: REQUIRE(curve.setOwnedExtensions(0x10, {{1040, -0.}})); break;
                }
                REQUIRE_FALSE(before->matches(curve)); // Each family must independently invalidate its baseline.
                last = std::make_unique<R03LastEdit>(curve);
            });
        });
        if (queuedError) std::rethrow_exception(queuedError);
        REQUIRE(last); last->check(*f.original);
        CHECK(&f.current() == f.original); CHECK(f.original->definition().distance == .25);
        CHECK(cloneLifetime.expired()); CHECK(f.graphic.count() == count);
        CHECK(dialogUndoHistory(f.graphic) == history); CHECK(f.graphic.countUndoCycles() == 0);
        CHECK_FALSE(f.graphic.isModified());
        REQUIRE_FALSE(f.context.messages.isEmpty()); CHECK(f.context.messages.last().contains("not applied"));
    }
}

TEST_CASE("R03 native dialog checks active EED payload bits and reference provenance in every definition state",
          "[gui][dialog][offsetcurve][action][r03][eed]") {
    for (unsigned state = 0; state < 3; ++state) for (unsigned change = 0; change < 12; ++change) {
        CAPTURE(state, change);
        ActionFixture f;
        if (state == 1) f.original->scale({0, 0}, {2, 3});
        if (state == 2) f.original->getData().controlPoints.front().x += .25;
        const Snapshot source(*f.original);
        std::vector<std::shared_ptr<DRW_Variant>> tags{
            std::make_shared<DRW_Variant>(1001, std::string("FOREIGN_R03")),
            std::make_shared<DRW_Variant>(1000, std::string("before")),
            std::make_shared<DRW_Variant>(1070, std::int32_t{1}),
            std::make_shared<DRW_Variant>(1071, std::int64_t{2}),
            std::make_shared<DRW_Variant>(1040, 0.),
            std::make_shared<DRW_Variant>(1010, DRW_Coord{1, 2, 0}),
            std::make_shared<DRW_Variant>(1004, std::vector<std::uint8_t>{0, 255}),
            std::make_shared<DRW_Variant>(1003, std::string("fixture-layer"))};
        tags.back()->setDwgRawLayerReference(77, DRW::AC1032);
        f.original->setDrwExtData(tags);
        auto before = LC_OffsetCurveEditSnapshot::capture(*f.original); REQUIRE(before);
        const auto history = dialogUndoHistory(f.graphic);
        const auto count = f.graphic.count();
        std::weak_ptr<DRW_Variant> cloneLifetime;
        std::exception_ptr queuedError;
        f.drive([&](LC_DlgEntityProperties& dialog, unsigned opening) {
            REQUIRE(opening == 1); auto& widget = native(dialog);
            auto* pen = dialog.findChild<QG_WidgetPen*>("wPen"); REQUIRE(pen);
            choose(*pen->cbColor, pen->cbColor->findData(QColor(Qt::red)));
            CHECK(widget.editAttemptCount() == 0); CHECK_FALSE(widget.lastEditResult());
            cloneLifetime = cloneOnlyWitness(widget);
            queueNativeAccept(dialog, queuedError, [&] {
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
                CHECK(before->matches(*f.original) == (change == 11)); // Unchanged full metadata is a positive control.
            });
        });
        if (queuedError) std::rethrow_exception(queuedError);
        if (change == 11) {
            auto& committed = f.current(); CHECK(&committed != f.original);
            source.geometry(committed, false); CHECK(committed.getPen(false).getColor() == RS_Color(Qt::red));
            CHECK_FALSE(cloneLifetime.expired()); CHECK(f.graphic.countUndoCycles() == 1);
            REQUIRE(committed.getDrwExtData().size() == tags.size() + 2);
            for (std::size_t i = 0; i < tags.size(); ++i) CHECK(committed.getDrwExtData()[i] == tags[i]);
            REQUIRE(f.graphic.undo()); CHECK(&f.current() == f.original);
            REQUIRE(f.graphic.redo()); CHECK(&f.current() == &committed);
        } else {
            CHECK(&f.current() == f.original); source.geometry(*f.original);
            r03Pen(f.original->getPen(false), source.pen);
            CHECK(cloneLifetime.expired()); CHECK(f.graphic.count() == count);
            CHECK(dialogUndoHistory(f.graphic) == history); CHECK(f.graphic.countUndoCycles() == 0);
            CHECK_FALSE(f.graphic.isModified()); CHECK(f.original->getDrwExtData() == tags);
            if (change == 0) CHECK(*tags[1]->content.s == "after");
            if (change == 1) CHECK(tags[2]->i_val() == 42);
            if (change == 2) CHECK(tags[3]->i64_val() == (std::int64_t{1} << 40));
            if (change == 3) CHECK(bits(tags[4]->d_val()) == bits(-0.));
            if (change == 4) CHECK(bits(tags[5]->coord()->z) == bits(-0.));
            if (change == 5) CHECK(tags[6]->binary()->front() == 128);
            if (change == 6) CHECK(tags[7]->isLayerRef());
            if (change == 7 || change == 8) {
                const auto reference = tags[7]->activeDwgLayerReference(); REQUIRE(reference);
                CHECK(reference->reference == (change == 7 ? 99 : 77));
                CHECK(reference->sourceVersion == (change == 7 ? DRW::AC1032 : DRW::AC1015));
            }
            if (change == 9) CHECK(tags[1]->code() == 1006);
            if (change == 10) { CHECK(tags[1]->type() == DRW_Variant::INTEGER); CHECK(tags[1]->i_val() == 42); }
        }
    }
}

TEST_CASE("Native dialog rejects unsafe display trees before virtual child cloning",
          "[gui][dialog][offsetcurve][action][r03][display]") {
    class CountedTree final : public RS_EntityContainer {
    public:
        CountedTree(RS_EntityContainer* parent, unsigned& calls) : RS_EntityContainer(parent), calls(calls) {}
        RS_Entity* clone() const override { ++calls; return RS_EntityContainer::clone(); }
        unsigned& calls;
    };
    class CountedLine final : public RS_Line {
    public:
        CountedLine(RS_EntityContainer* parent, const RS_Vector& start, const RS_Vector& end, unsigned& calls)
            : RS_Line(parent, start, end), calls(calls) {}
        RS_Entity* clone() const override { ++calls; return RS_Line::clone(); }
        unsigned& calls;
    };
    for (const bool nested : {false, true}) {
        CAPTURE(nested);
        ActionFixture f;
        RS_EntityContainer foreign(nullptr);
        unsigned clones = 0;
        RS_Entity* displaced = nullptr;
        if (nested) f.original->addEntity(new CountedTree(f.original, clones));
        else {
            const auto start = f.original->getStartpoint(), end = f.original->getEndpoint();
            f.original->clear();
            displaced = new CountedLine(f.original, start, end, clones);
            f.original->addEntity(displaced);
            displaced->setParent(&foreign);
        }
        const auto restore = qScopeGuard([&] { if (displaced) displaced->setParent(f.original); });
        REQUIRE_FALSE(LC_OffsetCurveEditSnapshot::capture(*f.original));
        f.graphic.setModified(false);
        const Snapshot before(*f.original);
        const auto history = dialogUndoHistory(f.graphic);
        f.action->start(f.original);
        QEventLoop loop;
        QTimer poll, timeout;
        bool opened = false;
        QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
            if (auto* dialog = qobject_cast<LC_DlgEntityProperties*>(QApplication::activeModalWidget())) {
                opened = true;
                dialog->reject();
            }
        });
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        poll.start(5);
        timeout.setSingleShot(true);
        timeout.start(60);
        loop.exec();
        CHECK_FALSE(opened);
        CHECK(clones == 0);
        before.exact(*f.original);
        CHECK(f.graphic.countUndoCycles() == 0);
        CHECK(dialogUndoHistory(f.graphic) == history);
        CHECK_FALSE(f.graphic.isModified());
    }
}

TEST_CASE("R03 native snapshot refusals precede cloning and delayed dialog creation",
          "[gui][dialog][offsetcurve][action][r03][budget]") {
    using Guard = LC_OffsetCurveEditSnapshot;
    class CountedLine final : public RS_Line {
    public:
        CountedLine(RS_EntityContainer* parent, const RS_Vector& start, const RS_Vector& end, unsigned& calls)
            : RS_Line(parent, start, end), calls(calls) {}
        RS_Entity* clone() const override { ++calls; return RS_Line::clone(); }
        unsigned& calls;
    };
    for (unsigned kind = 0; kind < 6; ++kind) {
        CAPTURE(kind);
        unsigned clones = 0;
        ActionFixture f;
        const auto start = f.original->getStartpoint(), end = f.original->getEndpoint();
        f.original->clear();
        f.original->addEntity(new CountedLine(f.original, start, end, clones));
        if (kind == 0)
            for (unsigned i = 0; i < LC_OffsetCurve::MaximumOwnedTailItems; ++i)
                f.original->setUserDefVar(QString("extra-%1").arg(i), "v");
        if (kind == 1) f.original->setDrwExtData(std::vector<std::shared_ptr<DRW_Variant>>(
            Guard::MaximumSnapshotItems + 1, std::make_shared<DRW_Variant>(1070, std::int32_t{1})));
        if (kind == 2) f.original->setDrwExtData({std::make_shared<DRW_Variant>(1000,
            std::string(Guard::MaximumSnapshotBytes + 1, 'x'))});
        if (kind == 3) f.original->setDrwExtData({std::make_shared<DRW_Variant>(1004,
            std::vector<std::uint8_t>(Guard::MaximumSnapshotBytes + 1, 128))});
        if (kind == 4) f.original->setDrwExtData({
            std::make_shared<DRW_Variant>(1000, std::string(Guard::MaximumSnapshotBytes/2, 'x')),
            std::make_shared<DRW_Variant>(1004, std::vector<std::uint8_t>(Guard::MaximumSnapshotBytes/2 + 1, 128))});
        if (kind == 5) f.original->setDrwExtData({nullptr});
        f.graphic.setModified(false);
        CHECK_FALSE(Guard::capture(*f.original));
        const auto history = dialogUndoHistory(f.graphic);
        const auto count = f.graphic.count();
        const auto fingerprint = RS_Spline::geometryFingerprint(f.original->getData());
        REQUIRE(f.graphic.countUndoCycles() == 0);
        f.action->start(f.original);
        CHECK(clones == 0);
        QEventLoop loop; QTimer poll, timeout; bool opened = false;
        QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
            if (auto* dialog = qobject_cast<LC_DlgEntityProperties*>(QApplication::activeModalWidget())) {
                opened = true; dialog->reject();
            }
        });
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        poll.start(5); timeout.setSingleShot(true); timeout.start(60); loop.exec();
        poll.stop(); timeout.stop();
        CHECK_FALSE(opened); CHECK(clones == 0); CHECK(&f.current() == f.original);
        CHECK(f.graphic.count() == count); CHECK(dialogUndoHistory(f.graphic) == history);
        CHECK(RS_Spline::geometryFingerprint(f.original->getData()) == fingerprint);
        CHECK(f.original->definition().distance == .25); CHECK_FALSE(f.graphic.isModified());
    }
}


// Lifetime controls run the real delayed editor and nested QDialog::exec loop.
namespace {
class LifetimeAction final : public LC_ActionModifyEntity {
public:
    explicit LifetimeAction(Context& context, unsigned& callbacks)
        : LC_ActionModifyEntity(&context), m_callbacks(callbacks) {}
    void prime(RS_Entity* entity) { doInitWithContextEntity(entity, {0, 0}); }
    void start(RS_Entity* entity) { prime(entity); init(ShowDialog); }
    void onLateRequestCompleted(bool skip) override {
        ++m_callbacks;
        LC_ActionModifyEntity::onLateRequestCompleted(skip);
    }
private:
    unsigned& m_callbacks;
};

struct ModalLifetimeFixture {
    QApplication* app = lc::test::application();
    QC_ApplicationWindow& window = actionWindow();
    std::unique_ptr<Graphic> drawing = std::make_unique<Graphic>();
    Graphic fallback;
    lc::test::TestGraphicView view;
    Context context;
    unsigned callbacks = 0;
    int listenersBeforeAction = 0;
    LC_OffsetCurve* original = nullptr;
    std::unique_ptr<LifetimeAction> action;
    ModalLifetimeFixture() {
        REQUIRE(app);
        drawing->initForNewDocument(); drawing->onLoadingCompleted();
        fallback.initForNewDocument(); fallback.onLoadingCompleted(); fallback.setModified(false);
        view.setDocument(drawing.get()); context.setDocumentAndView(drawing.get(), &view);
        auto curve = makeArc(*drawing); original = curve.release(); drawing->addEntity(original);
        drawing->setModified(false);
        listenersBeforeAction = drawing->getSelection()->listenerCount();
        action = std::make_unique<LifetimeAction>(context, callbacks);
    }
    void restoreContext() {
        // setDocument(nullptr) is not a supported viewport operation.
        view.setDocument(&fallback); context.setDocumentAndView(&fallback, &view);
    }
    ~ModalLifetimeFixture() {
        restoreContext();
        action.reset();
        view.beginClose();
    }
};

void observeModalLifetime(const std::function<void()>& start,
    const std::function<void(LC_DlgEntityProperties&)>& queuedOperation) {
    QEventLoop loop;
    QTimer poll, timeout;
    QPointer<LC_DlgEntityProperties> modal;
    bool opened = false, expired = false;
    std::exception_ptr error;
    poll.setInterval(5);
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
        auto* dialog = qobject_cast<LC_DlgEntityProperties*>(QApplication::activeModalWidget());
        if (dialog && !opened) {
            dialog->activateWindow();
            if (!dialog->isActiveWindow()) return;
            opened = true; modal = dialog;
            QTimer::singleShot(0, &loop, [&, live = modal] {
                try {
                    REQUIRE(live);
                    REQUIRE(QApplication::activeModalWidget() == live.data());
                    queuedOperation(*live);
                } catch (...) {
                    error = std::current_exception();
                    if (live) live->reject();
                }
            });
        }
        if (opened && modal.isNull() && !QApplication::activeModalWidget()) loop.quit();
    });
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&] {
        expired = true;
        if (auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) dialog->reject();
        loop.quit();
    });
    poll.start(); timeout.start(3000);
    start(); loop.exec();
    poll.stop(); timeout.stop();
    if (error) std::rethrow_exception(error);
    REQUIRE_FALSE(expired); REQUIRE(opened); REQUIRE(modal.isNull());
}

// Wait beyond the 20ms delayed-opening timer; an unexpected dialog is rejected so
// this negative control cannot hang the test process.
bool modalOpensDuringDelay(const std::function<void()>& start) {
    QEventLoop loop;
    QTimer poll, timeout;
    bool opened = false;
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
        if (auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
            opened = true; dialog->reject();
        }
    });
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    poll.start(5); timeout.setSingleShot(true); timeout.start(80);
    start(); loop.exec();
    return opened;
}
}

TEST_CASE("Native dialog rejects a removed initial context address before selected or type reads",
          "[gui][dialog][offsetcurve][action][lifetime]") {
    ModalLifetimeFixture f;
    auto* removed = f.original;
    f.drawing->removeEntity(removed); f.original = nullptr;
    const auto history = dialogUndoHistory(*f.drawing);
    const auto count = f.drawing->count();
    CHECK_FALSE(modalOpensDuringDelay([&] { f.action->start(removed); }));
    CHECK(f.drawing->count() == count); CHECK(dialogUndoHistory(*f.drawing) == history);
    CHECK(f.callbacks == 0); REQUIRE_FALSE(f.context.messages.isEmpty());
}

TEST_CASE("Native source removal before delayed opening refuses without constructing stale widgets",
          "[gui][dialog][offsetcurve][action][lifetime]") {
    ModalLifetimeFixture f;
    const auto history = dialogUndoHistory(*f.drawing);
    std::weak_ptr<DRW_Variant> sharedPayload = f.original->getDrwExtData().at(1);
    bool removed = false;
    CHECK_FALSE(modalOpensDuringDelay([&] {
        f.action->start(f.original);
        QTimer::singleShot(0, &f.view, [&] {
            f.drawing->removeEntity(f.original); f.original = nullptr; removed = true;
            CHECK_FALSE(sharedPayload.expired()); // Delayed session still owns the native clone.
        });
    }));
    REQUIRE(removed); CHECK(sharedPayload.expired());
    CHECK(f.drawing->count() == 0); CHECK(dialogUndoHistory(*f.drawing) == history);
    CHECK(f.drawing->countUndoCycles() == 0); CHECK(f.callbacks == 1);
}

TEST_CASE("Native dialog document death cancels before delay and during an active modal",
          "[gui][dialog][offsetcurve][action][lifetime][document]") {
    for (const bool active : {false, true}) {
        CAPTURE(active);
        ModalLifetimeFixture f;
        auto* const deadIdentity = f.drawing.get();
        QPointer<LifetimeAction> liveAction(f.action.get());
        std::weak_ptr<DRW_Variant> clonePayload;
        if (active) {
            observeModalLifetime([&] { f.action->start(f.original); }, [&](LC_DlgEntityProperties& dialog) {
                auto& widget = native(dialog);
                clonePayload = cloneOnlyWitness(widget);
                auto& pending = edit(widget, "leDistance"); pending.setFocus(); REQUIRE(pending.hasFocus()); pending.setText(".3");
                CHECK(widget.editAttemptCount() == 0);
                f.drawing.reset(); f.original = nullptr; // View AND context intentionally retain the dead address.
                CHECK(f.view.getDocument() == deadIdentity);
                CHECK(f.context.getDocument() == deadIdentity);
                CHECK_FALSE(dialog.isVisible());
                CHECK_FALSE(clonePayload.expired()); // The executing stack owns it until dlg's destruction.
                CHECK(widget.editAttemptCount() == 0); CHECK_FALSE(widget.lastEditResult());
            });
        } else {
            clonePayload = f.original->getDrwExtData().at(1);
            CHECK_FALSE(modalOpensDuringDelay([&] {
                f.action->start(f.original);
                QTimer::singleShot(0, &f.view, [&] { f.drawing.reset(); f.original = nullptr; });
            }));
        }
        REQUIRE(liveAction); CHECK(liveAction->isFinished()); CHECK(liveAction->mayBeTerminatedExternally());
        CHECK(f.callbacks == 0); CHECK(clonePayload.expired());
        f.restoreContext(); // Only after modal unwind, not as a substitute for the lifetime guard.
        CHECK(f.fallback.countUndoCycles() == 0); CHECK_FALSE(f.fallback.isModified());
    }
}

TEST_CASE("Native modal action editor and actual handler close retain the clone only until unwind",
          "[gui][dialog][offsetcurve][action][lifetime][close]") {
    for (unsigned destroy = 0; destroy < 3; ++destroy) {
        CAPTURE(destroy);
        ModalLifetimeFixture f;
        const Snapshot original(*f.original);
        auto source = LC_OffsetCurveEditSnapshot::capture(*f.original); REQUIRE(source);
        const auto history = dialogUndoHistory(*f.drawing);
        const auto count = f.drawing->count();
        std::weak_ptr<DRW_Variant> clonePayload;
        QPointer<LifetimeAction> action;
        std::shared_ptr<LifetimeAction> handlerOwner;
        if (destroy == 2) {
            f.action.reset();
            handlerOwner = std::make_shared<LifetimeAction>(f.context, f.callbacks);
            action = handlerOwner.get(); action->prime(f.original);
        } else {
            action = f.action.get();
        }
        observeModalLifetime([&] {
            if (destroy == 2) {
                REQUIRE(f.view.setCurrentAction(handlerOwner)); // The real event handler is the SOLE owner.
                REQUIRE(f.view.getCurrentAction() == handlerOwner.get());
                handlerOwner.reset();
            } else {
                f.action->start(f.original);
            }
        }, [&](LC_DlgEntityProperties& dialog) {
            auto& widget = native(dialog);
            clonePayload = cloneOnlyWitness(widget);
            QPointer<LC_EntityPropertiesEditor> editor = action->findChild<LC_EntityPropertiesEditor*>();
            REQUIRE(editor);
            auto& pending = edit(widget, "leDistance"); pending.setFocus(); REQUIRE(pending.hasFocus()); pending.setText(".3");
            CHECK(widget.editAttemptCount() == 0);
            if (destroy == 0) f.action.reset(); // Queued callback while editor's exec() is active.
            if (destroy == 1) delete editor.data();
            if (destroy == 2) {
                f.view.beginClose(); // quiesceForClose -> actual active action deletion.
                CHECK(f.view.isClosing()); CHECK_FALSE(f.view.getCurrentAction());
            }
            CHECK(editor.isNull());
            CHECK(action.isNull() == (destroy != 1));
            CHECK_FALSE(dialog.isVisible()); CHECK_FALSE(clonePayload.expired());
            CHECK(widget.editAttemptCount() == 0); CHECK_FALSE(widget.lastEditResult());
        });
        CHECK(clonePayload.expired()); CHECK(f.callbacks == 0);
        REQUIRE(source->matches(*f.original)); original.exact(*f.original);
        CHECK(f.drawing->count() == count); CHECK(dialogUndoHistory(*f.drawing) == history);
        CHECK(f.drawing->countUndoCycles() == 0); CHECK_FALSE(f.drawing->isModified());
        if (destroy == 1) { REQUIRE(action); CHECK(action->isFinished()); }
    }
}

TEST_CASE("Native dialog lifetime registration detaches from its exact original selection after context swap",
          "[gui][dialog][offsetcurve][action][lifetime][listeners]") {
    ModalLifetimeFixture f;
    const auto otherListeners = f.fallback.getSelection()->listenerCount();
    REQUIRE(f.drawing->getSelection()->listenerCount() == f.listenersBeforeAction + 1);
    f.restoreContext();
    f.action.reset();
    CHECK(f.drawing->getSelection()->listenerCount() == f.listenersBeforeAction);
    CHECK(f.fallback.getSelection()->listenerCount() == otherListeners);
    CHECK(f.callbacks == 0);
}


// These controls cover the base-init context route and reuse after an ordinary dialog.
namespace {
class ReusableDialogAction final : public LC_ActionModifyEntity {
public:
    explicit ReusableDialogAction(Context& context) : LC_ActionModifyEntity(&context) {}
    void begin() { init(InitialActionStatus); }
    void pick(const RS_Vector& point) {
        auto event = lc::test::eventAt(point.x, point.y);
        onMouseLeftButtonRelease(ShowDialog, &event);
    }
};

struct OrdinaryCloneObservations {
    unsigned cloned = 0, destroyed = 0;
    RS_Line* liveClone = nullptr;
};
class ObservedDialogLine final : public RS_Line {
public:
    ObservedDialogLine(Graphic& document, OrdinaryCloneObservations& observations)
        : RS_Line(&document, {10, 0}, {11, 0}), m_observations(observations) {
        setLayer(document.getActiveLayer());
    }
    RS_Entity* clone() const override {
        auto* result = new ObservedDialogLine(*this);
        result->m_isClone = true;
        ++m_observations.cloned;
        m_observations.liveClone = result;
        return result;
    }
    ~ObservedDialogLine() override {
        if (m_isClone) {
            ++m_observations.destroyed;
            if (m_observations.liveClone == this) m_observations.liveClone = nullptr;
        }
    }
private:
    OrdinaryCloneObservations& m_observations;
    bool m_isClone = false;
};
}

TEST_CASE("ModifyEntity drops removed saved context before base init attempts unselection",
          "[gui][dialog][offsetcurve][action][lifetime][context-init]") {
    for (const bool removed : {false, true}) {
        CAPTURE(removed);
        ModalLifetimeFixture f;
        f.drawing->select(QList<RS_Entity*>{f.original});
        REQUIRE(f.original->isSelected());
        f.context.saveContextMenuActionContext(f.original, {1.25, 0}, true);
        if (removed) {
            f.drawing->removeEntity(f.original); f.original = nullptr;
            const auto history = dialogUndoHistory(*f.drawing);
            CHECK_FALSE(modalOpensDuringDelay([&] { f.action->init(RS_ActionInterface::InitialActionStatus); }));
            CHECK(f.context.getContextMenuActionContextEntity() == nullptr);
            CHECK_FALSE(f.context.getContextMenuActionClickPosition().valid);
            CHECK(f.drawing->count() == 0); CHECK(dialogUndoHistory(*f.drawing) == history);
            CHECK(f.callbacks == 0); REQUIRE_FALSE(f.context.messages.isEmpty());
        } else {
            // Live saved context retains normal base-init unselection and native cancellation behavior.
            const auto fingerprint = RS_Spline::geometryFingerprint(f.original->getData());
            observeModalLifetime([&] { f.action->init(RS_ActionInterface::InitialActionStatus); },
                [&](LC_DlgEntityProperties& dialog) { button(dialog, QDialogButtonBox::Cancel); });
            CHECK(f.context.getContextMenuActionContextEntity() == nullptr);
            CHECK_FALSE(f.original->isSelected());
            CHECK(RS_Spline::geometryFingerprint(f.original->getData()) == fingerprint);
            CHECK(f.drawing->countUndoCycles() == 0); CHECK(f.callbacks == 1);
        }
    }
}

TEST_CASE("An ordinary-first reused editor retains native ownership and destruction cleanup",
          "[gui][dialog][offsetcurve][action][lifetime][editor-reuse]") {
    for (const bool firstAccepted : {false, true}) {
        CAPTURE(firstAccepted);
        OrdinaryCloneObservations observations; // Outlives every source/undo-owned clone.
        ModalLifetimeFixture f;
        f.action.reset();
        auto* line = new ObservedDialogLine(*f.drawing, observations); f.drawing->addEntity(line);
        f.drawing->setModified(false);
        auto action = std::make_unique<ReusableDialogAction>(f.context);
        QPointer<LC_EntityPropertiesEditor> firstEditor;
        observeModalLifetime([&] { action->begin(); action->pick({10.5, 0}); },
            [&](LC_DlgEntityProperties& dialog) {
                REQUIRE(dialog.findChild<LC_PropertiesEditingWidgetLine*>());
                firstEditor = action->findChild<LC_EntityPropertiesEditor*>();
                REQUIRE(firstEditor);
                button(dialog, firstAccepted ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel);
            });
        REQUIRE(firstEditor); CHECK_FALSE(action->isFinished()); CHECK(action->mayBeTerminatedExternally());
        CHECK(observations.cloned == 1); CHECK(observations.destroyed == (firstAccepted ? 0 : 1));
        const auto history = dialogUndoHistory(*f.drawing);
        const auto count = f.drawing->count();
        auto source = LC_OffsetCurveEditSnapshot::capture(*f.original); REQUIRE(source);
        std::weak_ptr<DRW_Variant> nativeClone;
        observeModalLifetime([&] {
            f.context.saveContextMenuActionContext(f.original, {1.25, 0}, false);
            action->init(RS_ActionInterface::InitialActionStatus);
        }, [&](LC_DlgEntityProperties& dialog) {
            auto& widget = native(dialog);
            auto* reused = action->findChild<LC_EntityPropertiesEditor*>();
            REQUIRE(reused == firstEditor.data()); // No fresh editor may hide the ordinary-first path.
            nativeClone = cloneOnlyWitness(widget);
            delete reused;
            CHECK(firstEditor.isNull()); CHECK(action->isFinished()); CHECK(action->mayBeTerminatedExternally());
            CHECK_FALSE(nativeClone.expired()); // Active modal session, not the dead editor, owns it.
        });
        CHECK(nativeClone.expired()); REQUIRE(source->matches(*f.original));
        CHECK(f.original->definition().distance == .25);
        CHECK(f.drawing->count() == count); CHECK(dialogUndoHistory(*f.drawing) == history);
        CHECK(f.drawing->countUndoCycles() == (firstAccepted ? 1 : 0));
    }
}

TEST_CASE("Ordinary delayed dialogs abandon Accept and Cancel safely after document or view context swaps",
          "[gui][dialog][offsetcurve][action][lifetime][ordinary-context]") {
    for (const bool accepted : {false, true}) for (const bool differentView : {false, true}) {
        CAPTURE(accepted, differentView);
        OrdinaryCloneObservations observations;
        ModalLifetimeFixture f;
        auto* line = new ObservedDialogLine(*f.drawing, observations); f.drawing->addEntity(line);
        f.drawing->setModified(false);
        Graphic other;
        other.initForNewDocument(); other.onLoadingCompleted(); other.setModified(false);
        lc::test::TestGraphicView otherView; otherView.setDocument(&other);
        const auto history = dialogUndoHistory(*f.drawing);
        const auto count = f.drawing->count();
        const auto start = line->getStartpoint(), end = line->getEndpoint();
        const auto layer = line->getLayer(false);
        const auto pen = line->getPen(false);
        auto nativeSource = LC_OffsetCurveEditSnapshot::capture(*f.original); REQUIRE(nativeSource);
        observeModalLifetime([&] { f.action->start(line); }, [&](LC_DlgEntityProperties& dialog) {
            auto* widget = dialog.findChild<LC_PropertiesEditingWidgetLine*>(); REQUIRE(widget);
            enter(*widget, "leEndX", "12"); // A real widget edit changes only the private ordinary clone.
            REQUIRE(observations.liveClone); CHECK(observations.liveClone->getEndpoint().x == 12);
            pointEqual(line->getEndpoint(), end);
            if (differentView) f.context.setDocumentAndView(&other, &otherView);
            else f.restoreContext();
            button(dialog, accepted ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel);
            CHECK(observations.liveClone != nullptr); // Clone survives its widgets until unwind.
        });
        REQUIRE(f.action); CHECK(f.action->isFinished()); CHECK(f.action->mayBeTerminatedExternally());
        CHECK(observations.cloned == 1); CHECK(observations.destroyed == 1); CHECK_FALSE(observations.liveClone);
        pointEqual(line->getStartpoint(), start); pointEqual(line->getEndpoint(), end);
        CHECK(line->getLayer(false) == layer); r03Pen(line->getPen(false), pen);
        REQUIRE(nativeSource->matches(*f.original));
        CHECK(f.drawing->count() == count); CHECK(dialogUndoHistory(*f.drawing) == history);
        CHECK(f.drawing->countUndoCycles() == 0); CHECK_FALSE(f.drawing->isModified());
        CHECK(other.countUndoCycles() == 0); CHECK_FALSE(other.isModified());
        CHECK(f.fallback.countUndoCycles() == 0); CHECK_FALSE(f.fallback.isModified());
        CHECK(f.callbacks == 1);
        f.restoreContext(); // Restore before the borrowed alternative view is destroyed.
    }
}

TEST_CASE("R03 native modal Accept rechecks undo epoch after a successful final source endpoint read",
          "[gui][dialog][offsetcurve][action][r03][final-read][epoch]") {
    class EndpointReadLine final : public RS_Line {
    public:
        EndpointReadLine(RS_EntityContainer* parent, const RS_Vector& start, const RS_Vector& end)
            : RS_Line(parent, start, end) {}
        std::function<void()> onFinalRead;
        mutable unsigned remaining = 0, callbacks = 0;
        mutable RS_Vector returned{false};
        RS_Vector getEndpoint() const override {
            const auto endpoint = RS_Line::getEndpoint();
            if (remaining && --remaining == 0) {
                ++callbacks; returned = endpoint; onFinalRead();
            }
            return endpoint;
        }
    };
    for (const bool changeEpoch : {false, true}) {
        CAPTURE(changeEpoch);
        ActionFixture f;
        REQUIRE(f.original->count() > 0);
        const auto index = f.original->count() - 1;
        auto* old = dynamic_cast<RS_Line*>(f.original->entityAt(index));
        REQUIRE(old);
        const auto start = old->RS_Line::getStartpoint();
        const auto end = old->RS_Line::getEndpoint();
        auto* line = new EndpointReadLine(f.original, start, end);
        line->setPen(old->getPen(false)); line->setLayer(old->getLayer(false));
        REQUIRE(f.original->removeEntity(old));
        f.original->insertEntity(index, line);
        REQUIRE(f.original->hasEditableDefinition());
        f.graphic.setModified(false);
        const Snapshot before(*f.original);
        const auto undoBefore = dialogUndoHistory(f.graphic);
        const auto cycles = f.graphic.countUndoCycles();
        const auto count = f.graphic.count();
        unsigned admissions = 0;
        bool undoReturned = true;
        std::uint64_t admittedEpoch = 0, readEpoch = 0, afterReadEpoch = 0;
        const auto disarm = qScopeGuard([&] {
            f.action->beforeNativeCommit = {};
            line->remaining = 0; line->onFinalRead = {};
        });
        f.action->beforeNativeCommit = [&] {
            ++admissions; admittedEpoch = f.graphic.undoStateEpoch();
            // Only arm inside the real InEditing transaction, after all modal
            // guards. matches() reads cachedLength's endpoint, then its final
            // point comparison; the latter must still return the original value.
            line->onFinalRead = [&] {
                readEpoch = f.graphic.undoStateEpoch();
                if (changeEpoch) undoReturned = f.graphic.undo();
                afterReadEpoch = f.graphic.undoStateEpoch();
            };
            line->remaining = 2;
        };
        f.drive([&](LC_DlgEntityProperties& dialog, unsigned opening) {
            REQUIRE(opening == 1);
            auto& widget = native(dialog);
            enter(widget, "leDistance", ".3");
            REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
            CHECK(widget.lastEditResult()->changed); CHECK(widget.lastEditResult()->engineRuns == 1);
            CHECK(widget.editAttemptCount() == 1); before.exact(*f.original);
            button(dialog, QDialogButtonBox::Ok);
        });
        REQUIRE(admissions == 1); REQUIRE(line->callbacks == 1); CHECK(line->remaining == 0);
        pointEqual(line->returned, end); pointEqual(line->RS_Line::getEndpoint(), end);
        CHECK(readEpoch == admittedEpoch);
        if (changeEpoch) {
            CHECK_FALSE(undoReturned); CHECK(afterReadEpoch == readEpoch + 1);
            CHECK(f.graphic.undoStateEpoch() == afterReadEpoch);
            CHECK(&f.current() == f.original); before.exact(*f.original);
            CHECK_FALSE(f.original->isDeleted()); CHECK_FALSE(f.graphic.isModified());
            CHECK(f.graphic.count() == count); CHECK(f.graphic.countUndoCycles() == cycles);
            CHECK(dialogUndoHistory(f.graphic) == undoBefore);
        } else {
            // The same getter and real edit must succeed when its callback
            // leaves the admitted context/epoch unchanged.
            CHECK(afterReadEpoch == readEpoch);
            CHECK(&f.current() != f.original); CHECK(f.current().definition().distance == .3);
            CHECK(f.original->isDeleted()); CHECK(f.graphic.count() == count + 1);
            CHECK(f.graphic.countUndoCycles() == cycles + 1); before.geometry(*f.original);
        }
    }
}

namespace {
class GuardCompletionRequestor final : public QObject, public LC_LateCompletionRequestor {
public:
    explicit GuardCompletionRequestor(unsigned& callbacks) : m_callbacks(callbacks) {}
    void onLateRequestCompleted(bool) override { ++m_callbacks; }
private:
    unsigned& m_callbacks;
};

enum class EditorGuardStage { Initial, Final, Late };
enum class EditorGuardMutation { Cancel, DestroyEditor, DestroyRequestor };

void exerciseEditorGuardReentry(EditorGuardStage stage, EditorGuardMutation mutation,
    bool guardResult, bool acceptedOrSkipped = false) {
    Fixture f;
    Context context;
    context.setDocumentAndView(&f.graphic, &f.view);
    const Snapshot before(*f.curve);
    unsigned callbacks = 0, unrelatedCallbacks = 0;
    auto requestor = std::make_unique<GuardCompletionRequestor>(callbacks);
    GuardCompletionRequestor unrelated(unrelatedCallbacks);
    QPointer<LC_EntityPropertiesEditor> editor = new LC_EntityPropertiesEditor(&context, requestor.get());
    editor->setParent(requestor.get());
    auto session = std::make_shared<LC_EntityPropertiesEditSession>();
    session->entity.reset(f.curve->clone());
    REQUIRE(session->entity);
    auto token = std::make_shared<unsigned>(1);
    const std::weak_ptr<unsigned> captureLifetime(token);
    bool armed = stage != EditorGuardStage::Final;
    bool guardReturned = false, captureRetained = false;
    unsigned guardCalls = 0, mutations = 0;
    const auto input = context.getInteractiveInputInfo();
    const auto clearInput = qScopeGuard([&] {
        input->requestor = nullptr; input->state = Input::NONE;
    });
    const auto start = [&] {
        editor->editEntity(nullptr, session->entity.get(), f.view.getViewPort(),
            [&, keep = std::move(token)] {
                ++guardCalls;
                if (!armed) return true;
                ++mutations;
                switch (mutation) {
                case EditorGuardMutation::Cancel: editor->cancelGuardedEdit(); break;
                case EditorGuardMutation::DestroyEditor: delete editor.data(); break;
                case EditorGuardMutation::DestroyRequestor: requestor.reset(); break;
                }
                // The executing callable must survive clearing its owning editor's guard.
                captureRetained = !captureLifetime.expired() && *keep == 1;
                guardReturned = true;
                return guardResult;
            }, session);
    };
    if (stage == EditorGuardStage::Final) {
        observeModalLifetime(start, [&](LC_DlgEntityProperties& dialog) {
            REQUIRE(guardCalls == 1);
            armed = true;
            if (acceptedOrSkipped) dialog.accept();
            else dialog.reject();
        });
    } else {
        CHECK_FALSE(modalOpensDuringDelay([&] {
            start();
            if (stage == EditorGuardStage::Late) {
                // Cancellation must not clear another live requestor's input state.
                input->requestor = &unrelated; input->state = Input::REQUESTED;
                editor->onLateRequestCompleted(acceptedOrSkipped);
            }
        }));
    }
    CHECK(guardCalls == (stage == EditorGuardStage::Final ? 2u : 1u));
    CHECK(mutations == 1); CHECK(guardReturned); CHECK(captureRetained);
    CHECK(captureLifetime.expired());
    CHECK(editor.isNull() == (mutation != EditorGuardMutation::Cancel));
    CHECK(bool(requestor) == (mutation != EditorGuardMutation::DestroyRequestor));
    CHECK(callbacks == 0); CHECK(unrelatedCallbacks == 0);
    if (stage == EditorGuardStage::Late) {
        CHECK(input->requestor == &unrelated); CHECK(input->state == Input::REQUESTED);
    }
    before.exact(*f.curve);
    CHECK(f.graphic.countUndoCycles() == 0);
}
}

TEST_CASE("Native editor initial guard reentry abandons cancelled or destroyed owners",
          "[gui][dialog][offsetcurve][review-regression][guard-reentry][editor-guard-initial]") {
    for (const auto mutation : {EditorGuardMutation::Cancel, EditorGuardMutation::DestroyEditor,
                               EditorGuardMutation::DestroyRequestor}) {
        for (const bool guardResult : {false, true}) {
            CAPTURE(static_cast<int>(mutation), guardResult);
            exerciseEditorGuardReentry(EditorGuardStage::Initial, mutation, guardResult);
        }
    }
}

TEST_CASE("Native editor final modal guard reentry suppresses stale Accept and Cancel completion",
          "[gui][dialog][offsetcurve][review-regression][guard-reentry][editor-guard-final]") {
    for (const auto mutation : {EditorGuardMutation::Cancel, EditorGuardMutation::DestroyEditor,
                               EditorGuardMutation::DestroyRequestor}) {
        for (const bool guardResult : {false, true}) for (const bool accepted : {false, true}) {
            CAPTURE(static_cast<int>(mutation), guardResult, accepted);
            exerciseEditorGuardReentry(EditorGuardStage::Final, mutation, guardResult, accepted);
        }
    }
}

TEST_CASE("Native editor late input guard reentry preserves unrelated input and suppresses reopening",
          "[gui][dialog][offsetcurve][review-regression][guard-reentry][editor-guard-late]") {
    for (const auto mutation : {EditorGuardMutation::Cancel, EditorGuardMutation::DestroyEditor,
                               EditorGuardMutation::DestroyRequestor}) {
        for (const bool guardResult : {false, true}) for (const bool skipped : {false, true}) {
            CAPTURE(static_cast<int>(mutation), guardResult, skipped);
            exerciseEditorGuardReentry(EditorGuardStage::Late, mutation, guardResult, skipped);
        }
    }
}

TEST_CASE("Native widget false busy reentry preserves its newest bound target",
          "[gui][dialog][offsetcurve][review-regression][async][reentrant-rebind]") {
    for (const bool clear : {false, true}) {
        CAPTURE(clear);
        Fixture f; requireNativeHelper(); f.show(); auto& widget = native(*f.dialog);
        auto outerTarget = makeArc(f.graphic), newestTarget = makeArc(f.graphic);
        REQUIRE(newestTarget->setDistance(.4));
        const Snapshot original(*f.curve), outer(*outerTarget), newest(*newestTarget);
        const auto detachTarget = qScopeGuard([&] { widget.setEntity(nullptr); });
        QObject observer;
        unsigned rebound = 0;
        QObject::connect(&widget, &NativeWidget::preparationBusyChanged, &observer, [&](bool busy) {
            if (!busy && !rebound) {
                ++rebound;
                widget.setEntity(newestTarget.get());
            }
        });
        submitInput(widget, "leDistance", ".3");
        REQUIRE(widget.isPreparing());
        widget.setEntity(clear ? nullptr : outerTarget.get());
        REQUIRE(rebound == 1); REQUIRE(widget.editedEntity() == newestTarget.get());
        CHECK_FALSE(widget.isPreparing()); CHECK_FALSE(widget.lastEditResult());
        CHECK(widget.editAttemptCount() == 0);
        observeCancelledReplies();
        REQUIRE(widget.editedEntity() == newestTarget.get());
        original.exact(*f.curve); outer.exact(*outerTarget); newest.exact(*newestTarget);
        enter(widget, "leDistance", ".5");
        REQUIRE(widget.lastEditResult()); REQUIRE(*widget.lastEditResult());
        CHECK(widget.lastEditResult()->changed); CHECK(widget.editAttemptCount() == 1);
        CHECK(newestTarget->definition().distance == .5);
        CHECK(widget.editedEntity() == newestTarget.get());
        original.exact(*f.curve); outer.exact(*outerTarget);
    }
}

TEST_CASE("Native field completion keeps committed text after a valid viewport context refusal",
          "[gui][dialog][offsetcurve][review-regression][context-refusal-field]") {
    Fixture f;
    Graphic other; other.initForNewDocument(); other.onLoadingCompleted();
    lc::test::TestGraphicView otherView; otherView.setDocument(&other);
    otherView.getViewPort()->createUCS(RS_Vector{10, 20}, RS_Math::deg2rad(90.));
    f.show(); auto& widget = native(*f.dialog);
    const Snapshot before(*f.curve);
    const std::array<const char*, 8> fields{{"leDistance", "leCenterX", "leCenterY", "leMajor",
        "leMinor", "leRotation", "leAngle1", "leAngle2"}};
    std::array<QString, 8> committed;
    for (std::size_t i = 0; i < fields.size(); ++i) committed[i] = edit(widget, fields[i]).text();
    const auto restoreContext = qScopeGuard([&] {
        widget.cancelPreparation(); widget.setPreparationGuard({});
        widget.setGraphicViewport(f.view.getViewPort());
    });
    unsigned guardCalls = 0;
    widget.setPreparationGuard([&] {
        ++guardCalls;
        widget.setGraphicViewport(otherView.getViewPort());
        return false;
    });
    submitInput(widget, "leDistance", ".3");
    REQUIRE(guardCalls == 1);
    CHECK(widget.editAttemptCount() == 1); CHECK_FALSE(widget.isPreparing());
    CHECK_FALSE(widget.lastEditResult()); CHECK_FALSE(widget.lastWorkerResult());
    CHECK_FALSE(widget.findChild<QLabel*>("lRefusal")->text().isEmpty());
    for (std::size_t i = 0; i < fields.size(); ++i) {
        CAPTURE(fields[i]);
        CHECK(edit(widget, fields[i]).text() == committed[i]);
        CHECK_FALSE(edit(widget, fields[i]).isModified());
    }
    auto& distance = edit(widget, "leDistance");
    distance.setFocus(); key(distance, Qt::Key_Tab);
    CHECK(guardCalls == 1); CHECK(widget.editAttemptCount() == 1);
    before.exact(*f.curve);
    CHECK(f.graphic.countUndoCycles() == 0); CHECK(other.countUndoCycles() == 0);
}
