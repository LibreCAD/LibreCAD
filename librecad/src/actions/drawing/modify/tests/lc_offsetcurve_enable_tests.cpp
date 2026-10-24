/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <QtWidgets>
#include <QPointer>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>
#include "lc_action_modify_offset.h"
#include "lc_actiontestsupport.h"
#include "lc_offset_options_filler.h"
#include "lc_offset_options_widget.h"
#include "lc_offsetcurve.h"
#include "lc_properties_sheet.h"
#include "lc_property_bool.h"
#include "lc_property_container.h"
#include "lc_testsettingsisolation.h"
#include "qc_applicationwindow.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_selection.h"
#include "rs_settings.h"
#include "rs_system.h"

namespace {
using Plain = LC_OffsetPlainReason;
using Refusal = LC_OffsetCurveAgain::Refusal;

class Settings final {
public:
    Settings() {
        for (const auto& fullKey : RS_SETTINGS->getAllKeys()) {
            QString key = fullKey;
            if (key.startsWith('/')) key.remove(0, 1);
            if (key.startsWith("ActionModifyOffset/"))
                saved.emplace(key.mid(QStringLiteral("ActionModifyOffset/").size()),
                    RS_SETTINGS->getSettings()->value(fullKey));
        }
    }
    ~Settings() {
        const auto group = RS_SETTINGS->beginGroupGuard("ActionModifyOffset");
        QStringList keys{"Distance", "DistanceFixed", "UseCurrentLayer", "UseCurrentAttributes",
                         "KeepOriginals", "MultipleCopies", "Copies", "EditableOffsetCurve"};
        for (const auto& fullKey : RS_SETTINGS->getAllKeys()) {
            QString key = fullKey;
            if (key.startsWith('/')) key.remove(0, 1);
            if (key.startsWith("ActionModifyOffset/")) keys.append(key.mid(QStringLiteral("ActionModifyOffset/").size()));
        }
        keys.removeDuplicates();
        for (const auto& key : keys) RS_SETTINGS->remove(key);
        for (const auto& [key, value] : saved) RS_SETTINGS->write(key, value);
    }
    void removeNative() const {
        const auto group = RS_SETTINGS->beginGroupGuard("ActionModifyOffset");
        RS_SETTINGS->remove("EditableOffsetCurve");
    }
    void native(bool enabled) const { LC_SET_ONE("ActionModifyOffset", "EditableOffsetCurve", enabled); }
    bool containsNative() const {
        return RS_SETTINGS->getSettings()->contains("/ActionModifyOffset/EditableOffsetCurve");
    }
private:
    std::map<QString, QVariant> saved;
};

void settle(int milliseconds = 40) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}
void key(QWidget& widget, int code) {
    for (auto type : {QEvent::KeyPress, QEvent::KeyRelease}) {
        QKeyEvent event(type, code, Qt::NoModifier);
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

class Graphic final : public RS_Graphic {
public:
    using RS_Graphic::countUndoCycles;
};
class Context final : public LC_ActionContext {
public:
    QStringList messages;
    void commandMessage(const QString& value) override { messages.append(value); }
};
class Probe final : public LC_ActionModifyOffset {
public:
    explicit Probe(LC_ActionContext* context) : LC_ActionModifyOffset(context) {}
    using LC_ActionModifyOffset::SetReferencePoint;
    using LC_ActionModifyOffset::createOptionsFiller;
    using LC_ActionModifyOffset::createOptionsWidget;
    using LC_ActionModifyOffset::keptPlainMessage;
    using LC_ActionModifyOffset::keptPlainReason;
    using LC_ActionModifyOffset::onMouseLeftButtonReleaseSelected;
    using LC_ActionModifyOffset::onMouseMoveEventSelected;
    using RS_PreviewActionInterface::deletePreviewAndHighlights;
    using RS_PreviewActionInterface::drawPreviewAndHighlights;
    using RS_PreviewActionInterface::m_preview;
    std::optional<LC_OffsetBatchOutcome> committed;
    bool hasPending() const { return m_pendingOutcome != nullptr; }
    void deliver(LC_OffsetBatchOutcome outcome, bool success) {
        m_pendingOutcome = std::make_unique<LC_OffsetBatchOutcome>(std::move(outcome));
        doTriggerCompletion(success);
    }
protected:
    void doTriggerCompletion(bool success) override {
        if (m_pendingOutcome) committed = *m_pendingOutcome; // observe real diagnostics before message cleanup
        LC_ActionModifyOffset::doTriggerCompletion(success);
    }
};

struct Fixture {
    QApplication* app = lc::test::application();
    Settings settings;
    Graphic graphic;
    lc::test::TestGraphicView view;
    Context context;
    std::unique_ptr<Probe> action;
    Fixture() {
        REQUIRE(app != nullptr); REQUIRE(lc::test::SettingsIsolation::isActive());
        graphic.initForNewDocument(); graphic.onLoadingCompleted(); view.setDocument(&graphic);
        context.setDocumentAndView(&graphic, &view);
        action = std::make_unique<Probe>(&context);
    }
    ~Fixture() { settle(); action.reset(); } // compact option delegates queue a bounded UI reload
    RS_Ellipse* ellipse(const RS_EllipseData& basis = {{0, 0}, {5, 0}, 1}) {
        auto* result = new RS_Ellipse(&graphic, basis);
        result->setLayer(graphic.getActiveLayer()); graphic.addEntity(result); return result;
    }
    LC_OffsetCurve* native() {
        RS_Ellipse source{&graphic, RS_EllipseData{{0, 0}, {1, 0}, 1}};
        auto made = LC_OffsetCurve::create(source, .25, false, {}, .01);
        REQUIRE(made); made.entity->setParent(&graphic); made.entity->setLayer(graphic.getActiveLayer());
        auto* result = made.entity.release(); graphic.addEntity(result); return result;
    }
    void start(const QList<RS_Entity*>& sources, double distance, bool keep = false) {
        action->loadOptions();
        action->setDistanceFixed(true); action->setDistance(distance);
        action->setKeepOriginals(keep); action->setUseMultipleCopies(false);
        action->setUseCurrentLayer(false); action->setUseCurrentAttributes(false);
        graphic.select(sources);
        action->init(Probe::SetReferencePoint);
    }
    void hover(const RS_Vector& point) {
        action->deletePreviewAndHighlights();
        const auto event = lc::test::eventAt(point.x, point.y);
        action->onMouseMoveEventSelected(Probe::SetReferencePoint, &event);
        action->drawPreviewAndHighlights();
    }
    void commit(const RS_Vector& point) {
        const auto event = lc::test::eventAt(point.x, point.y);
        action->onMouseLeftButtonReleaseSelected(Probe::SetReferencePoint, &event);
    }
    std::vector<std::uint64_t> previewFingerprints() const {
        std::vector<std::uint64_t> result;
        for (const auto* entity : *action->m_preview) {
            REQUIRE(entity->rtti() != RS2::EntityOffsetCurve);
            const auto* spline = dynamic_cast<const RS_Spline*>(entity); REQUIRE(spline);
            result.push_back(RS_Spline::geometryFingerprint(spline->getData()));
        }
        return result;
    }
};

// The compact options factory uses the real property-sheet provider in the app singleton.
std::unique_ptr<QC_ApplicationWindow>* windowOwner = nullptr;
class WindowCleanup final : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;
    void testRunEnded(const Catch::TestRunStats&) override {
        if (windowOwner) windowOwner->reset();
    }
};
CATCH_REGISTER_LISTENER(WindowCleanup)
void prepareCompactFactory() {
    REQUIRE(lc::test::application()); REQUIRE(lc::test::SettingsIsolation::isActive());
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
    (void)initialized; windowOwner = &QC_ApplicationWindow::getAppWindow();
}

LC_OffsetSourceOutcome plainSource(Plain reason) {
    LC_OffsetSourceOutcome source;
    source.status = LC_OffsetSourceStatus::Succeeded;
    source.copiesMade = source.copiesRequested = source.copiesKeptPlain = 1;
    source.keptPlain.push_back({1, reason, std::nullopt});
    return source; // deliberately no dereferenceable source pointer in formatter unit cases
}
QString notice(const LC_OffsetSourceOutcome& source) {
    LC_OffsetBatchOutcome batch; batch.sources.append(source); return Probe::keptPlainMessage(batch);
}
}

TEST_CASE("Editable offset interactive default loads on without constructor or settings writes",
          "[gui][offsetcurve][enable][options]") {
    Fixture f; f.settings.removeNative();
    CHECK_FALSE(f.action->isEditableOffsetCurve()); CHECK_FALSE(f.settings.containsNative());
    CHECK_FALSE(RS_OffsetData{}.asOffsetCurve); // direct requests keep their independent opt-in contract
    f.action->loadOptions();
    CHECK(f.action->isEditableOffsetCurve()); CHECK_FALSE(f.settings.containsNative());
    f.action->setEditableOffsetCurve(false);
    f.action->loadOptions(); CHECK(f.action->isEditableOffsetCurve());
}

TEST_CASE("Editable offset explicit saved off and on survive the normal options lifecycle",
          "[gui][offsetcurve][enable][options]") {
    for (bool enabled : {false, true}) {
        Fixture f; f.settings.native(enabled);
        CHECK_FALSE(f.action->isEditableOffsetCurve());
        f.action->loadOptions(); CHECK(f.action->isEditableOffsetCurve() == enabled);
        f.action->setEditableOffsetCurve(!enabled); f.action->saveOptions();
        Probe restored(&f.context);
        CHECK_FALSE(restored.isEditableOffsetCurve()); restored.loadOptions();
        CHECK(restored.isEditableOffsetCurve() == !enabled);
        CHECK(LC_GET_ONE_BOOL("ActionModifyOffset", "EditableOffsetCurve", enabled) == !enabled);
    }
}

TEST_CASE("Actual offset option widget factory registers the persisted native checkbox and clicked path",
          "[gui][offsetcurve][enable][options][widget]") {
    Fixture f; f.settings.removeNative(); f.action->loadOptions();
    std::unique_ptr<LC_ActionOptionsWidget> widget(f.action->createOptionsWidget());
    REQUIRE(dynamic_cast<LC_OffsetOptionsWidget*>(widget.get()));
    widget->setAction(f.action.get()); widget->show(); widget->activateWindow(); QApplication::processEvents();
    auto* box = widget->findChild<QCheckBox*>("cbEditableOffsetCurve"); REQUIRE(box);
    CHECK(box->text() == "Editable Offset Curve"); CHECK(box->isChecked());
    int clicks = 0;
    QObject::connect(box, &QCheckBox::clicked, widget.get(), [&](bool) { ++clicks; });
    click(*box);
    CHECK(clicks == 1); CHECK_FALSE(f.action->isEditableOffsetCurve());
    f.action->saveOptions();
    Probe next(&f.context); next.loadOptions(); CHECK_FALSE(next.isEditableOffsetCurve());
    click(*box); CHECK(clicks == 2); CHECK(f.action->isEditableOffsetCurve());
    CHECK(f.context.messages.isEmpty()); CHECK_FALSE(f.action->committed);
}

TEST_CASE("Offset widget population blocks all dependent signals for the full update and restores prior blocks",
          "[gui][offsetcurve][enable][options][signals]") {
    Fixture f;
    std::unique_ptr<LC_ActionOptionsWidget> widget(f.action->createOptionsWidget());
    int changed = 0;
    for (auto* box : widget->findChildren<QCheckBox*>())
        QObject::connect(box, &QCheckBox::toggled, widget.get(), [&](bool) { ++changed; });
    for (auto* spin : widget->findChildren<QSpinBox*>())
        QObject::connect(spin, &QSpinBox::valueChanged, widget.get(), [&](int) { ++changed; });
    for (auto* edit : widget->findChildren<QLineEdit*>())
        QObject::connect(edit, &QLineEdit::textChanged, widget.get(), [&](const QString&) { ++changed; });
    f.action->setEditableOffsetCurve(true); f.action->setCopiesNumber(7);
    f.action->setUseMultipleCopies(true); f.action->setKeepOriginals(true); f.action->setDistance(3.5);
    widget->setAction(f.action.get());
    CHECK(changed == 0); CHECK(f.action->getCopiesNumber() == 7);
    auto* box = widget->findChild<QCheckBox*>("cbEditableOffsetCurve"); REQUIRE(box);
    {
        const QSignalBlocker prior(box);
        f.action->setEditableOffsetCurve(false); f.action->setCopiesNumber(3);
        widget->setAction(f.action.get(), true);
        CHECK(box->signalsBlocked()); CHECK_FALSE(box->isChecked()); CHECK(changed == 0);
    }
    CHECK_FALSE(box->signalsBlocked()); CHECK(f.action->getCopiesNumber() == 3);
    CHECK_FALSE(f.action->committed); CHECK(f.context.messages.isEmpty());
}

TEST_CASE("Actual compact offset options factory exposes and edits the same saved native option",
          "[gui][offsetcurve][enable][options][compact]") {
    prepareCompactFactory();
    Fixture f; f.settings.removeNative(); f.action->loadOptions();
    std::unique_ptr<LC_ActionOptionsPropertiesFiller> filler(f.action->createOptionsFiller());
    REQUIRE(dynamic_cast<LC_OffsetOptionsFiller*>(filler.get()));
    filler->setAction(f.action.get());
    LC_PropertyContainer root;
    filler->fillToolOptionsContainer(&root);
    const auto rows = root.findChildProperties("$a_editableOffsetCurve"); REQUIRE(rows.size() == 1);
    auto* option = dynamic_cast<LC_PropertyBool*>(rows.front()); REQUIRE(option); CHECK(option->value());
    LC_PropertiesSheet sheet(nullptr, &root);
    sheet.resize(520, 360); sheet.show(); sheet.activateWindow(); QApplication::processEvents();
    REQUIRE(sheet.setActiveProperty(option, true)); sheet.setFocus();
    key(sheet, Qt::Key_Space);
    CHECK_FALSE(option->value()); CHECK_FALSE(f.action->isEditableOffsetCurve());
    settle(); f.action->saveOptions();
    Probe next(&f.context); next.loadOptions(); CHECK_FALSE(next.isEditableOffsetCurve());
    key(sheet, Qt::Key_Space); CHECK(option->value()); CHECK(f.action->isEditableOffsetCurve());
    settle(); CHECK(f.context.messages.isEmpty()); CHECK_FALSE(f.action->committed);
}

TEST_CASE("Loaded native option does not change hover geometry or produce preview notices",
          "[gui][offsetcurve][enable][preview]") {
    Fixture f; f.settings.removeNative(); auto* source = f.ellipse();
    f.start({source}, .5, true);
    REQUIRE(f.action->isEditableOffsetCurve());
    std::unique_ptr<LC_ActionOptionsWidget> widget(f.action->createOptionsWidget());
    widget->setAction(f.action.get()); widget->show(); QApplication::processEvents();
    auto* box = widget->findChild<QCheckBox*>("cbEditableOffsetCurve"); REQUIRE(box);
    click(*box); REQUIRE_FALSE(f.action->isEditableOffsetCurve()); f.hover(RS_Vector{20, 20});
    const auto plain = f.previewFingerprints(); REQUIRE_FALSE(plain.empty());
    click(*box); REQUIRE(f.action->isEditableOffsetCurve()); f.hover(RS_Vector{20, 20});
    CHECK(f.previewFingerprints() == plain); CHECK(f.context.messages.isEmpty());
    CHECK_FALSE(f.action->hasPending()); CHECK_FALSE(f.action->committed);
    CHECK(f.graphic.countUndoCycles() == 0);
    CHECK_FALSE(source->isDeleted()); CHECK(source->rtti() == RS2::EntityEllipse);
    f.commit(RS_Vector{20, 20});
    REQUIRE(f.action->committed); const auto& result = f.action->committed->sources.front();
    // Commit discards the hover cache after source promotion; adoption uses the real committed producer.
    REQUIRE(result.complete()); CHECK(result.reusedCacheCopies == 0); CHECK(result.cacheEngineRuns == 1);
    CHECK(result.rebaseEngineRuns == 0);
    REQUIRE(result.createdEntities.size() == 1);
    CHECK(result.createdEntities.front()->rtti() == RS2::EntityOffsetCurve);
    CHECK(f.context.messages.isEmpty());
}

TEST_CASE("Compact native option reload is cancelled when its action dies before the timer",
          "[gui][offsetcurve][enable][options][compact][lifetime]") {
    prepareCompactFactory();
    Fixture f; f.settings.removeNative(); f.action->loadOptions();
    const QPointer<Probe> action(f.action.get());
    {
        std::unique_ptr<LC_ActionOptionsPropertiesFiller> filler(f.action->createOptionsFiller());
        REQUIRE(dynamic_cast<LC_OffsetOptionsFiller*>(filler.get()));
        filler->setAction(f.action.get());
        LC_PropertyContainer root;
        filler->fillToolOptionsContainer(&root);
        const auto rows = root.findChildProperties("$a_editableOffsetCurve"); REQUIRE(rows.size() == 1);
        auto* option = dynamic_cast<LC_PropertyBool*>(rows.front()); REQUIRE(option); REQUIRE(option->value());
        LC_PropertiesSheet sheet(nullptr, &root);
        sheet.resize(520, 360); sheet.show(); QApplication::processEvents();
        REQUIRE(sheet.setActiveProperty(option, true)); sheet.setFocus();
        key(sheet, Qt::Key_Space);
        REQUIRE_FALSE(option->value()); REQUIRE_FALSE(f.action->isEditableOffsetCurve());
    }
    f.action.reset(); // Retire property callbacks first; the independent delayed reload must be cancelled too.
    REQUIRE(action.isNull());
    settle();
    CHECK(f.graphic.isEmpty()); CHECK(f.graphic.countUndoCycles() == 0);
    CHECK(f.context.messages.isEmpty());
}

TEST_CASE("Default loaded native option adopts one committed ellipse offset with one undo batch",
          "[gui][offsetcurve][enable][action]") {
    Fixture f; f.settings.removeNative(); auto* source = f.ellipse();
    f.start({source}, .5); f.commit(RS_Vector{20, 20});
    REQUIRE(f.action->committed); REQUIRE(f.action->committed->sources.size() == 1);
    const auto& result = f.action->committed->sources.front();
    REQUIRE(result.complete()); REQUIRE(result.createdEntities.size() == 1);
    CHECK(result.copiesKeptPlain == 0); CHECK(result.keptPlain.empty());
    CHECK(result.rebaseEngineRuns == 0); CHECK(result.cacheEngineRuns == 1);
    auto* created = dynamic_cast<LC_OffsetCurve*>(result.createdEntities.front()); REQUIRE(created);
    CHECK(created->hasEditableDefinition()); CHECK(f.context.messages.isEmpty()); CHECK_FALSE(f.action->hasPending());
    CHECK(f.graphic.countUndoCycles() == 1);
    REQUIRE(f.graphic.undo()); CHECK_FALSE(source->isDeleted()); CHECK(created->isDeleted());
    REQUIRE(f.graphic.redo()); CHECK(source->isDeleted()); CHECK_FALSE(created->isDeleted());
}

TEST_CASE("Saved option off keeps the established plain ellipse commit and message contract",
          "[gui][offsetcurve][enable][action]") {
    Fixture f; f.settings.native(false); auto* source = f.ellipse();
    f.start({source}, .5); CHECK_FALSE(f.action->isEditableOffsetCurve()); f.commit(RS_Vector{20, 20});
    REQUIRE(f.action->committed); const auto& result = f.action->committed->sources.front();
    REQUIRE(result.complete()); REQUIRE(result.createdEntities.size() == 1);
    CHECK(result.createdEntities.front()->rtti() == RS2::EntitySpline);
    CHECK(result.keptPlain.empty()); CHECK(result.rebases.empty()); CHECK(f.context.messages.isEmpty());
}

TEST_CASE("Actual multi-piece native requests report committed copies not piece counts",
          "[gui][offsetcurve][enable][action][messages]") {
    for (int count : {1, 2}) {
        Fixture f; f.settings.removeNative();
        QList<RS_Entity*> sources;
        const double pi = std::acos(-1.);
        for (int i = 0; i < count; ++i)
            sources.append(f.ellipse(RS_EllipseData{{0, 0}, {100, 0}, .05, pi / 12, 7 * pi / 4, false}));
        f.start(sources, 4); f.commit(RS_Vector{0, 0});
        REQUIRE(f.action->committed); REQUIRE(f.action->committed->sources.size() == count);
        for (const auto& source : f.action->committed->sources) {
            REQUIRE(source.complete()); CHECK(source.copiesKeptPlain == 1);
            REQUIRE(source.keptPlain.size() == 1); CHECK(source.keptPlain.front().reason == Plain::SeveralPieces);
            REQUIRE(source.createdEntities.size() > 1);
            for (const auto* entity : source.createdEntities) CHECK(entity->rtti() != RS2::EntityOffsetCurve);
        }
        REQUIRE(f.context.messages.size() == 1);
        CHECK(f.context.messages.front() == QString("Offset copies kept plain: %1 (sources: %1). %1: the offset has several pieces").arg(count));
        CHECK(f.graphic.countUndoCycles() == 1); REQUIRE(f.graphic.undo()); REQUIRE(f.graphic.redo());
    }
}

TEST_CASE("Actual zero-sum native repeat reports plain fallback without invented producer or checker runs",
          "[gui][offsetcurve][enable][action][again]") {
    Fixture f; f.settings.removeNative(); auto* source = f.native();
    f.start({source}, .25); f.commit(RS_Vector{0, 0});
    REQUIRE(f.action->committed); const auto& result = f.action->committed->sources.front();
    REQUIRE(result.complete()); CHECK(result.rebaseEngineRuns == 0); CHECK(result.cacheEngineRuns == 1);
    REQUIRE(result.rebases.size() == 1); CHECK(result.rebases.front().engineRuns == 0);
    REQUIRE(result.rebases.front().proposalRefusal); CHECK(*result.rebases.front().proposalRefusal == Refusal::ZeroSum);
    CHECK_FALSE(result.rebases.front().nativeRefusal); CHECK_FALSE(result.rebases.front().engineStatus);
    CHECK_FALSE(result.rebases.front().checkReason);
    REQUIRE(result.createdEntities.size() == 1); CHECK(result.createdEntities.front()->rtti() != RS2::EntityOffsetCurve);
    REQUIRE(f.context.messages.size() == 1);
    CHECK(f.context.messages.front() == "Offset copies kept plain: 1 (sources: 1). 1: the accumulated offset is exactly zero");
}

TEST_CASE("Actual failed ellipse and ordinary successful sibling keep the existing failure message pin",
          "[gui][offsetcurve][enable][action][messages]") {
    Fixture f; f.settings.removeNative(); auto* source = f.ellipse();
    auto* line = new RS_Line(&f.graphic, RS_Vector{-10, 20}, RS_Vector{10, 20}); f.graphic.addEntity(line);
    f.start({source, line}, 6); f.commit(RS_Vector{0, 0});
    REQUIRE(f.action->committed); REQUIRE(f.action->committed->sources.size() == 2);
    CHECK_FALSE(f.action->committed->sources[0].succeeded());
    CHECK(f.action->committed->sources[0].keptPlain.empty());
    CHECK(f.action->committed->sources[1].complete()); CHECK(f.action->committed->sources[1].keptPlain.empty());
    REQUIRE(f.context.messages.size() == 1);
    CHECK(f.context.messages.front() == "1 of 2 selected entities could not be offset: nothing is left at this distance");
    CHECK_FALSE(source->isDeleted());
}

TEST_CASE("Plain notice formatting ignores failed and non-top-level diagnostics without touching source pointers",
          "[gui][offsetcurve][enable][messages]") {
    auto failed = plainSource(Plain::SeveralPieces); failed.status = LC_OffsetSourceStatus::LimitExceeded;
    CHECK(notice(failed).isEmpty());
    CHECK(notice(plainSource(Plain::NotTopLevel)).isEmpty());
    LC_OffsetBatchOutcome empty; CHECK(Probe::keptPlainMessage(empty).isEmpty());
    for (auto reason : {Plain::SeveralPieces, Plain::CheckFailed, Plain::NotCacheRepresentable,
                        Plain::OutputBudget, Plain::ZeroSum, Plain::RebaseRefused, Plain::UndefinedDefinition}) {
        const auto source = plainSource(reason);
        CHECK_FALSE(Probe::keptPlainReason(source, source.keptPlain.front()).isEmpty());
        CHECK(notice(source).startsWith("Offset copies kept plain: 1 (sources: 1). "));
    }
}

TEST_CASE("Plain rebase notices preserve every structural refusal and absent component stages",
          "[gui][offsetcurve][enable][messages][again]") {
    for (auto reason : {Refusal::None, Refusal::InvalidDefinition, Refusal::NonCanonicalBasis,
                        Refusal::InvalidIncrement, Refusal::UnresolvedSide, Refusal::InvalidCurrentProfile,
                        Refusal::ZeroSum, Refusal::NonFiniteCombination, Refusal::InexactCombination,
                        Refusal::FirstNotProvedUntrimmed, Refusal::FinalNotProvedUntrimmed}) {
        CAPTURE(static_cast<int>(reason));
        auto source = plainSource(Plain::RebaseRefused);
        LC_OffsetRebaseOutcome attempt; attempt.copy = 1; attempt.proposalRefusal = reason;
        source.rebases.push_back(attempt);
        const auto text = Probe::keptPlainReason(source, source.keptPlain.front());
        CHECK_FALSE(text.isEmpty()); CHECK_FALSE(text.contains("Producer:")); CHECK_FALSE(text.contains("Checker:"));
        if (reason == Refusal::None) CHECK(text.contains("remaining work allowance"));
        if (reason == Refusal::ZeroSum) CHECK(text.contains("exactly zero"));
        if (reason == Refusal::InexactCombination) CHECK(text.contains("represented exactly"));
        if (reason == Refusal::FirstNotProvedUntrimmed) CHECK(text.contains("first inward profile"));
        if (reason == Refusal::FinalNotProvedUntrimmed) CHECK(text.contains("final arc profile"));
    }
}

TEST_CASE("Plain notices distinguish native producer checker admission and output budget refusals",
          "[gui][offsetcurve][enable][messages]") {
    auto source = plainSource(Plain::RebaseRefused);
    LC_OffsetRebaseOutcome attempt; attempt.copy = 1; attempt.proposalRefusal = Refusal::None;
    attempt.nativeRefusal = LC_OffsetCurveRefusal::Engine; attempt.engineStatus = LC_CurveOffsetStatus::LimitExceeded;
    source.rebases = {attempt};
    const auto producer = notice(source); CHECK(producer.contains("bounded producer limit"));
    CHECK_FALSE(producer.contains("Checker:"));
    source.rebases.front().nativeRefusal = LC_OffsetCurveRefusal::CheckFailed;
    source.rebases.front().engineStatus = LC_CurveOffsetStatus::Ok;
    source.rebases.front().checkReason = LC_OffsetCheckResult::Reason::LimitExceeded;
    const auto checker = notice(source); CHECK(checker.contains("bounded cache validation limit"));
    CHECK(checker.contains("Producer:")); CHECK(checker.contains("Checker:")); CHECK(checker != producer);
    source.rebases.front().nativeRefusal = LC_OffsetCurveRefusal::TooSmall;
    source.rebases.front().engineStatus.reset(); source.rebases.front().checkReason.reset();
    const auto admission = notice(source); CHECK(admission.contains("too small"));
    CHECK_FALSE(admission.contains("Producer:")); CHECK_FALSE(admission.contains("Checker:"));
    CHECK(notice(plainSource(Plain::OutputBudget)).contains("output budget"));
    auto cache = plainSource(Plain::CheckFailed);
    CHECK_FALSE(notice(cache).contains("Checker:"));
    cache.keptPlain.front().checkReason = LC_OffsetCheckResult::Reason::OffCurve;
    CHECK(notice(cache).contains("outside the offset tolerance"));
}

TEST_CASE("Plain notice aggregates counts into four bounded reason buckets and a counted remainder",
          "[gui][offsetcurve][enable][messages][limits]") {
    LC_OffsetBatchOutcome batch;
    for (auto reason : {Plain::SeveralPieces, Plain::OutputBudget, Plain::ZeroSum,
                        Plain::UndefinedDefinition, Plain::NotCacheRepresentable, Plain::CheckFailed})
        batch.sources.append(plainSource(reason));
    const auto text = Probe::keptPlainMessage(batch);
    CHECK(text.startsWith("Offset copies kept plain: 6 (sources: 6). "));
    CHECK(text.contains("2 further copies had other plain fallback reasons"));
    CHECK(text.count("; ") == 4); CHECK(text.size() < 2048);
    auto repeated = plainSource(Plain::SeveralPieces);
    repeated.copiesMade = repeated.copiesRequested = repeated.copiesKeptPlain = RS_OffsetData::kMaximumOffsetCopies;
    repeated.keptPlain.clear();
    for (int copy = 1; copy <= RS_OffsetData::kMaximumOffsetCopies; ++copy)
        repeated.keptPlain.push_back({copy, Plain::SeveralPieces, std::nullopt});
    CHECK(notice(repeated) == "Offset copies kept plain: 100 (sources: 1). 100: the offset has several pieces");
}

TEST_CASE("Offset completion assembles native diagnostics before cleanup and preserves short-series messages",
          "[gui][offsetcurve][enable][messages][completion]") {
    Fixture f; f.action->init(Probe::SetReferencePoint);
    auto source = plainSource(Plain::ZeroSum);
    source.copiesRequested = 3; source.stoppedBy = LC_OffsetSourceStatus::OffsetFailed;
    source.engineStatus = LC_CurveOffsetStatus::FitFailed;
    LC_OffsetBatchOutcome batch; batch.sources.append(source);
    f.action->deliver(batch, true);
    REQUIRE(f.context.messages.size() == 2);
    CHECK(f.context.messages[0] == "Only 1 of 3 copies were made: the offset cannot be fitted within the tolerance; the original was kept");
    CHECK(f.context.messages[1] == "Offset copies kept plain: 1 (sources: 1). 1: the accumulated offset is exactly zero");
    CHECK_FALSE(f.action->hasPending()); CHECK(f.action->isFinished());
}

