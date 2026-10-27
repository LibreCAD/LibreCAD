/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_propertieseditingwidget_offsetcurve.h"

#include <QCheckBox>
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <algorithm>
#include <cmath>
#include <limits>
#include "lc_offsetcurveeditsnapshot.h"
#include "../../../lib/offset_worker/lc_offsetworkerclient.h"
#include "../../../lib/offset_worker/lc_offsetworkeredit.h"
#include "lc_graphicviewport.h"
#include "lc_offsetcurveui.h"
#include "lc_offsetcurvereadutils.h"
#include "lc_pointpickbutton.h"
#include "rs_math.h"
#include "ui_lc_propertieseditingwidget_offsetcurve.h"

namespace {
struct OffsetDialogContext {
    LC_GraphicViewport* viewport = nullptr;
    RS_Document* document = nullptr; // Identity only; never dereferenced after an event turn.
    std::array<RS_Vector, 3> world;
    double angle = 0;
    static OffsetDialogContext capture(LC_GraphicViewport* viewport) {
        OffsetDialogContext context; context.viewport = viewport;
        if (viewport) {
            context.document = viewport->getDocument();
            context.world = {viewport->toWorld({0, 0}), viewport->toWorld({1, 0}), viewport->toWorld({0, 1})};
            context.angle = viewport->toWorldAngle(viewport->toAbsUCSAngle(0));
        }
        return context;
    }
    bool matches(LC_GraphicViewport* current) const {
        if (current != viewport) return false;
        if (!current) return true;
        if (current->getDocument() != document) return false;
        const auto other = capture(current);
        for (std::size_t i = 0; i < world.size(); ++i)
            if (world[i].x != other.world[i].x || world[i].y != other.world[i].y ||
                world[i].z != other.world[i].z || world[i].valid != other.world[i].valid) return false;
        return angle == other.angle;
    }
};
}

LC_PropertiesEditingWidgetOffsetCurve::LC_PropertiesEditingWidgetOffsetCurve(QWidget* parent)
    : LC_EntityPropertiesEditorWidget(parent), ui(new Ui::LC_OffsetCurvePropertiesEditingWidget),
      m_limits(LC_OffsetCurveUI::sheetLimits()), m_worker(std::make_unique<LC_OffsetWorkerClient>(this)) {
    ui->setupUi(this);
    const auto bind = [this](QLineEdit* editor, Field field) {
        connect(editor, &QLineEdit::editingFinished, this,
            [this, editor, field] { finishField(editor, field); });
    };
    bind(ui->leDistance, Field::Distance);
    bind(ui->leCenterX, Field::CenterX); bind(ui->leCenterY, Field::CenterY);
    bind(ui->leMajor, Field::Major); bind(ui->leMinor, Field::Minor);
    bind(ui->leRotation, Field::Rotation);
    bind(ui->leAngle1, Field::Start); bind(ui->leAngle2, Field::End);
    connect(ui->cbTowardCentre, &QCheckBox::toggled, this, [this](bool inward) {
        if (m_refreshing || m_entity == nullptr) return;
        auto next = proposedDefinition(); next.towardCentre = inward;
        apply(next);
    });
    connect(ui->cbReversed, &QCheckBox::toggled, this, [this](bool reversed) {
        if (m_refreshing || m_entity == nullptr) return;
        auto next = proposedDefinition(); auto& basis = next.basis;
        if (basis.reversed != reversed) {
            if (basis.angle1 != 0 || basis.angle2 != 0) std::swap(basis.angle1, basis.angle2);
            basis.reversed = reversed;
        }
        apply(next);
    });
}

LC_PropertiesEditingWidgetOffsetCurve::~LC_PropertiesEditingWidgetOffsetCurve() {
    detachPreparation();
    delete ui;
}

void LC_PropertiesEditingWidgetOffsetCurve::detachPreparation() {
    m_entity = nullptr;
    m_busy = false; m_proposedDefinition.reset(); m_inputSnapshot.reset();
    m_worker.reset(); // Disconnect/reap before releasing the widget or caller's clone owner.
}

LC_EllipseOffsetDefinition LC_PropertiesEditingWidgetOffsetCurve::proposedDefinition() const {
    return m_proposedDefinition ? *m_proposedDefinition : m_entity->definition();
}

std::array<QLineEdit*, 8> LC_PropertiesEditingWidgetOffsetCurve::definitionEditors() const {
    return {ui->leDistance, ui->leCenterX, ui->leCenterY, ui->leMajor,
        ui->leMinor, ui->leRotation, ui->leAngle1, ui->leAngle2};
}

void LC_PropertiesEditingWidgetOffsetCurve::restoreCommittedText() const {
    // A revoked context may already have destroyed the formatter's document.
    // Restore value-owned display text, without visiting a borrowed viewport/doc.
    const QScopedValueRollback<bool> refreshing(m_refreshing, true);
    const auto editors = definitionEditors();
    for (std::size_t i = 0; i < editors.size(); ++i) if (!editors[i]->isModified()) {
        const QSignalBlocker blocked(editors[i]);
        editors[i]->setText(m_committedText[i]);
        editors[i]->setProperty("_offsetCurveLoadedText", m_committedText[i]);
    }
    const QSignalBlocker side(ui->cbTowardCentre), reversed(ui->cbReversed);
    ui->cbTowardCentre->setChecked(m_committedSide);
    ui->cbReversed->setChecked(m_committedReversed);
}

void LC_PropertiesEditingWidgetOffsetCurve::setBusy(bool busy) const {
    const bool changed = m_busy != busy;
    m_busy = busy;
    ui->gbDefinition->setEnabled(m_entity && m_viewport && m_entity->hasEditableDefinition() &&
        m_generation < std::numeric_limits<qint64>::max() && m_sourceRevision < std::numeric_limits<qint64>::max());
    ui->wPickPointCenter->setEnabled(!busy);
    for (auto* picker : {ui->tbPickDistance, ui->tbPickMajor, ui->tbPickMinor, ui->tbPickRotation})
        picker->setEnabled(!busy);
    if (changed) emit preparationBusyChanged(busy); // No member access after user slots.
}

void LC_PropertiesEditingWidgetOffsetCurve::cancelPreparation() const {
    if (!m_worker) return;
    const bool wasBusy = m_busy;
    if (m_generation < std::numeric_limits<qint64>::max()) ++m_generation;
    m_worker->cancel();
    m_proposedDefinition.reset(); m_inputSnapshot.reset();
    if (wasBusy) {
        restoreCommittedText();
        ui->lRefusal->setText(tr("Offset computation was cancelled; the value was not changed."));
    }
    setBusy(false);
}

void LC_PropertiesEditingWidgetOffsetCurve::finishPendingInput() const {
    auto* editor = qobject_cast<QLineEdit*>(QApplication::focusWidget());
    if (!m_busy && editor && isAncestorOf(editor) && editor->isModified()) emit editor->editingFinished();
}

void LC_PropertiesEditingWidgetOffsetCurve::setEntity(RS_Entity* entity) {
    if (!m_worker) return;
    const QPointer<LC_PropertiesEditingWidgetOffsetCurve> alive(this);
    if (m_sourceRevision == std::numeric_limits<qint64>::max()) {
        m_entity = nullptr;
        cancelPreparation();
        return;
    }
    ++m_sourceRevision;
    const auto revision = m_sourceRevision;
    const auto cancelledGeneration = m_generation < std::numeric_limits<qint64>::max() ? m_generation + 1 : m_generation;
    cancelPreparation();
    if (!alive || !m_worker || revision != m_sourceRevision || cancelledGeneration != m_generation) return;
    m_entity = entity != nullptr && entity->rtti() == RS2::EntityOffsetCurve
        ? static_cast<LC_OffsetCurve*>(entity) : nullptr;
    m_lastResult.reset(); m_lastWorkerResult.reset(); m_attempts = 0;
    ui->lRefusal->clear();
    refresh(nullptr, true);
}

void LC_PropertiesEditingWidgetOffsetCurve::refresh(QLineEdit* completed, bool force) const {
    const QScopedValueRollback<bool> refreshing(m_refreshing, true);
    const bool available = m_entity != nullptr && m_entity->hasEditableDefinition() && m_viewport != nullptr;
    const auto text = [completed, force](QLineEdit* editor, const QString& value) {
        // Another dirty editor belongs to the user, not to this field's refresh.
        if (force || editor == completed || !editor->isModified()) {
            const QSignalBlocker blocked(editor);
            editor->setText(value);
            editor->setProperty("_offsetCurveLoadedText", value);
            editor->setModified(false);
        }
    };
    ui->gbDefinition->setEnabled(available && m_generation < std::numeric_limits<qint64>::max() &&
        m_sourceRevision < std::numeric_limits<qint64>::max());
    ui->wPickPointCenter->setEnabled(!m_busy);
    for (auto* picker : {ui->tbPickDistance, ui->tbPickMajor, ui->tbPickMinor, ui->tbPickRotation})
        picker->setEnabled(!m_busy);
    if (m_entity == nullptr || m_viewport == nullptr) return;
    const auto definition = proposedDefinition();
    const auto& basis = definition.basis;
    const auto center = toUIStr(basis.center);
    text(ui->leDistance, asString(definition.distance));
    text(ui->leCenterX, center.first); text(ui->leCenterY, center.second);
    text(ui->leMajor, asString(basis.majorP.magnitude()));
    text(ui->leMinor, asString(basis.majorP.magnitude() * basis.ratio));
    text(ui->leRotation, toUIAngleDeg(basis.majorP.angle()));
    text(ui->leAngle1, asStringAngleDeg(basis.angle1));
    text(ui->leAngle2, asStringAngleDeg(basis.angle2));
    {
        const QSignalBlocker side(ui->cbTowardCentre), reversed(ui->cbReversed);
        ui->cbTowardCentre->setChecked(definition.towardCentre);
        ui->cbReversed->setChecked(basis.reversed);
    }
    const bool arc = available && !m_entity->sourceIsWhole();
    ui->leAngle1->setEnabled(arc); ui->leAngle2->setEnabled(arc);
    const auto length = LC_OffsetCurveRead::cachedLength(*m_entity);
    ui->lLengthValue->setText(length ? asString(*length) : tr("Unavailable"));
    ui->lState->setText(!m_entity->isDefined() ? tr("Undefined") :
        !m_entity->isConsistent() ? tr("Inconsistent") : tr("Defined"));
    if (!available) {
        LC_OffsetCurveEditResult state;
        state.refusal = !m_entity->isDefined()
            ? LC_OffsetCurveRefusal::Undefined : LC_OffsetCurveRefusal::Inconsistent;
        ui->lRefusal->setText(LC_OffsetCurveUI::refusalText(state));
    }
}

void LC_PropertiesEditingWidgetOffsetCurve::inputRefused(QLineEdit* editor, const QString& reason) const {
    const QPointer<const LC_PropertiesEditingWidgetOffsetCurve> alive(this);
    const auto revision = m_sourceRevision;
    const auto cancelledGeneration = m_generation < std::numeric_limits<qint64>::max() ? m_generation + 1 : m_generation;
    cancelPreparation();
    if (!alive || revision != m_sourceRevision || cancelledGeneration != m_generation) return;
    m_lastResult.reset(); // no native setter was called for a parse/UI-domain failure
    m_lastWorkerResult.reset();
    refresh(editor);
    ui->lRefusal->setText(reason);
}

void LC_PropertiesEditingWidgetOffsetCurve::finishField(QLineEdit* editor, Field field) const {
    if (m_refreshing || m_entity == nullptr || !editor->isModified()) return;
    if (editor->text() == editor->property("_offsetCurveLoadedText").toString()) {
        editor->setModified(false); // never parse a rounded, unchanged display back into the definition
        return;
    }
    const bool angle = field == Field::Rotation || field == Field::Start || field == Field::End;
    double value = 0;
    bool valid = false;
    if (angle) valid = toDoubleAngle(editor->text(), value, 0., false);
    else value = RS_Math::eval(editor->text(), &valid);
    if (!valid || !std::isfinite(value)) {
        inputRefused(editor, tr("Enter a finite numeric expression; the value was not changed."));
        return;
    }
    // Consume editingFinished before rendering dependent fields. Return plus
    // focus-out must not submit a second edit or retain a stale dirty value.
    editor->setModified(false);
    applyValue(field, angle ? RS_Math::deg2rad(value) : value);
}

void LC_PropertiesEditingWidgetOffsetCurve::apply(const LC_EllipseOffsetDefinition& definition) const {
    if (m_entity == nullptr) return;
    const QPointer<const LC_PropertiesEditingWidgetOffsetCurve> alive(this);
    bool published = false;
    if (!m_proposedDefinition) {
        const auto editors = definitionEditors();
        for (std::size_t i = 0; i < editors.size(); ++i)
            m_committedText[i] = editors[i]->property("_offsetCurveLoadedText").toString();
        m_committedSide = m_entity->definition().towardCentre;
        m_committedReversed = m_entity->definition().basis.reversed;
    }
    const auto failure = [this, &published](const QString& text) {
        if (!published) m_lastResult.reset();
        m_proposedDefinition.reset(); m_inputSnapshot.reset();
        if (!published) restoreCommittedText();
        ui->lRefusal->setText(text); setBusy(false);
    };
    if (m_generation == std::numeric_limits<qint64>::max() ||
        m_sourceRevision == std::numeric_limits<qint64>::max() || m_attempts == std::numeric_limits<std::size_t>::max()) {
        m_worker->cancel();
        failure(tr("The offset editing session is no longer current; the value was not changed."));
        return;
    }
    const auto generation = ++m_generation;
    const auto revision = m_sourceRevision;
    auto* const target = m_entity;
    m_worker->cancel();
    m_lastWorkerResult.reset();
    ++m_attempts;
    try {
        const auto guard = m_preparationGuard;
        const bool current = !guard || guard();
        if (!alive || generation != m_generation || target != m_entity) return;
        if (!current) { failure(tr("The offset editing input changed; the value was not changed.")); return; }
        const auto context = OffsetDialogContext::capture(m_viewport);
        if (!target->hasEditableDefinition()) {
            m_lastResult = target->prepareDefinition(definition, m_limits).result;
            m_proposedDefinition.reset(); m_inputSnapshot.reset(); refresh();
            ui->lRefusal->setText(LC_OffsetCurveUI::refusalText(*m_lastResult)); setBusy(false);
            return;
        }
        std::shared_ptr<LC_OffsetCurveEditSnapshot> snapshot = LC_OffsetCurveEditSnapshot::capture(*target);
        if (!alive || generation != m_generation || target != m_entity) return;
        if (!snapshot) { failure(tr("The offset input exceeds the bounded snapshot limits; the value was not changed.")); return; }
        auto prepared = LC_OffsetWorkerEdit::prepare(*target, definition, m_limits);
        if (!alive || generation != m_generation || target != m_entity) return;
        const bool matches = snapshot->matches(*target);
        if (!alive || generation != m_generation || target != m_entity) return;
        if (!matches || !context.matches(m_viewport)) {
            failure(tr("The offset editing input changed; the value was not changed.")); return;
        }
        if (!prepared.generate()) {
            m_lastResult = LC_OffsetWorkerEdit::applyLocal(*target, std::move(prepared));
            if (!alive || generation != m_generation || target != m_entity) return;
            published = *m_lastResult && m_lastResult->changed;
            if (*m_lastResult && m_lastResult->changed) ++m_sourceRevision;
            m_proposedDefinition.reset(); m_inputSnapshot.reset(); refresh();
            ui->lRefusal->setText(*m_lastResult ? QString{} : LC_OffsetCurveUI::refusalText(*m_lastResult));
            setBusy(false);
            return;
        }
        m_inputSnapshot = std::move(snapshot); // Only the latest job owns a large input witness.
        m_proposedDefinition = prepared.generate()->definition;
        m_lastResult.reset();
        LC_OffsetWorkerRequest::Request request{{generation, generation, revision, 0}, *prepared.generate()};
        const bool accepted = m_worker->submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [alive, generation, revision, target, context](LC_OffsetWorkerClient::Outcome outcome) {
                if (!alive || generation != alive->m_generation || revision != alive->m_sourceRevision ||
                    target != alive->m_entity || !alive->m_busy) return;
                const auto* self = alive.data();
                bool published = false;
                const auto refuse = [self, &published](const QString& text) {
                    if (!published) self->m_lastResult.reset();
                    self->m_proposedDefinition.reset(); self->m_inputSnapshot.reset();
                    if (!published) self->restoreCommittedText();
                    self->ui->lRefusal->setText(text); self->setBusy(false);
                };
                try {
                    if (outcome.status != LC_OffsetWorkerClient::Status::Completed || !outcome.completed) {
                        const auto text = outcome.status == LC_OffsetWorkerClient::Status::TimedOut ?
                            self->tr("Offset computation timed out; the value was not changed.") :
                            outcome.status == LC_OffsetWorkerClient::Status::Cancelled ||
                                outcome.status == LC_OffsetWorkerClient::Status::Superseded ?
                            self->tr("Offset computation was cancelled; the value was not changed.") :
                            self->tr("The offset worker could not complete the request; the value was not changed.");
                        refuse(text); return;
                    }
                    if (const auto* result = std::get_if<LC_OffsetWorkerResult::Native>(&outcome.completed->reply().result))
                        self->m_lastWorkerResult = static_cast<const LC_OffsetCurveEditResult&>(*result);
                    const auto guard = self->m_preparationGuard;
                    const bool current = !guard || guard();
                    if (!alive || generation != alive->m_generation || target != alive->m_entity) return;
                    const auto snapshot = self->m_inputSnapshot; // Retain across reentrant getters/cancellation.
                    const bool matches = current && context.matches(self->m_viewport) && snapshot && snapshot->matches(*target);
                    if (!alive || generation != alive->m_generation || target != alive->m_entity) return;
                    if (!matches || !context.matches(self->m_viewport)) {
                        refuse(self->tr("The offset editing input changed; the value was not changed.")); return;
                    }
                    self->m_lastResult = outcome.completed->apply(*target);
                    if (!alive || generation != alive->m_generation || target != alive->m_entity) return;
                    published = *self->m_lastResult && self->m_lastResult->changed;
                    if (*self->m_lastResult && self->m_lastResult->changed) ++self->m_sourceRevision;
                    self->m_proposedDefinition.reset(); self->m_inputSnapshot.reset(); self->refresh();
                    self->ui->lRefusal->setText(*self->m_lastResult ? QString{} : LC_OffsetCurveUI::refusalText(*self->m_lastResult));
                    self->setBusy(false);
                } catch (const std::bad_alloc&) {
                    if (alive) refuse(published ? self->tr("Offset prepared, but the editor could not be refreshed.") :
                        self->tr("Not enough memory to prepare the offset; the value was not changed."));
                }
            });
        if (!accepted) { failure(tr("The offset worker could not start the request; the value was not changed.")); return; }
        const bool wasBusy = m_busy;
        m_busy = true;
        refresh(); ui->lRefusal->setText(tr("Calculating offset..."));
        if (!wasBusy) emit preparationBusyChanged(true);
    } catch (const std::bad_alloc&) {
        if (alive) failure(published ? tr("Offset prepared, but the editor could not be refreshed.") :
            tr("Not enough memory to prepare the offset; the value was not changed."));
    }
}

void LC_PropertiesEditingWidgetOffsetCurve::applyValue(Field field, double value) const {
    if (m_entity == nullptr || m_viewport == nullptr) return;
    auto next = proposedDefinition(); auto& basis = next.basis;
    if (!m_entity->hasEditableDefinition()) {
        // Let D-edit classify stale/undefined state before local radius checks.
        apply(next);
        return;
    }
    if (field == Field::Distance) {
        next.distance = value; apply(next);
        return;
    }
    const double major = basis.majorP.magnitude(), minor = major * basis.ratio;
    switch (field) {
    case Field::CenterX:
    case Field::CenterY: {
        auto center = toUCSVector(basis.center);
        double& component = field == Field::CenterX ? center.x : center.y;
        if (component != value) {
            component = value;
            basis.center = toWCSVector(center);
        }
        break;
    }
    case Field::Major:
        if (!(value > 0) || value < minor) {
            LC_OffsetCurveEditResult invalid;
            inputRefused(ui->leMajor, LC_OffsetCurveUI::refusalText(invalid)); return;
        }
        if (value != major) {
            basis.majorP = basis.majorP * (value / major);
            basis.ratio = minor / value;
        }
        break;
    case Field::Minor:
        if (!(value > 0) || value > major) {
            LC_OffsetCurveEditResult invalid;
            inputRefused(ui->leMinor, LC_OffsetCurveUI::refusalText(invalid)); return;
        }
        if (value != minor) basis.ratio = value / major;
        break;
    case Field::Rotation:
        if (value != toUCSAngle(basis.majorP.angle())) {
            const double world = m_viewport->toWorldAngle(m_viewport->toAbsUCSAngle(value));
            basis.majorP = RS_Vector::polar(major, world);
        }
        break;
    case Field::Start: basis.angle1 = value; break;
    case Field::End: basis.angle2 = value; break;
    case Field::Distance: break;
    }
    apply(next);
}

void LC_PropertiesEditingWidgetOffsetCurve::setupInteractiveInputWidgets() {
    pickPointSetup(ui->wPickPointCenter, "center", ui->leCenterX, ui->leCenterY);
    pickDistanceSetup(ui->tbPickDistance, "distance", ui->leDistance);
    pickDistanceSetup(ui->tbPickMajor, "major", ui->leMajor);
    pickDistanceSetup(ui->tbPickMinor, "minor", ui->leMinor);
    pickAngleSetup(ui->tbPickRotation, "rotation", ui->leRotation);
    // Raw ellipse parameters are not WCS bearings: deliberately no angle picker.
}

void LC_PropertiesEditingWidgetOffsetCurve::interactiveInputUpdate(
    LC_ActionContext::InteractiveInputInfo::InputType inputType, const QString& tag,
    double valueOne, double valueTwo) const {
    using Input = LC_ActionContext::InteractiveInputInfo;
    if (m_entity == nullptr) return;
    if (inputType == Input::POINT && tag == "center") {
        auto next = proposedDefinition();
        next.basis.center = RS_Vector{valueOne, valueTwo, next.basis.center.z};
        apply(next);
    }
    else if (inputType == Input::DISTANCE) {
        if (tag == "distance") applyValue(Field::Distance, valueOne);
        else if (tag == "major") applyValue(Field::Major, valueOne);
        else if (tag == "minor") applyValue(Field::Minor, valueOne);
    }
    else if (inputType == Input::ANGLE && tag == "rotation") {
        // PickAngle returns the same UCS/basis radians that the rotation editor displays.
        applyValue(Field::Rotation, valueOne);
    }
}
