/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_propertieseditingwidget_offsetcurve.h"

#include <QCheckBox>
#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <limits>
#include "lc_offsetcurveeditsnapshot.h"
#include "lc_offsetsource.h"
#include "../../../lib/offset_worker/lc_offsetworkerclient.h"
#include "../../../lib/offset_worker/lc_offsetworkeredit.h"
#include "../../../lib/offset_worker/lc_offsetworkerrequest.h"
#include "lc_graphicviewport.h"
#include "lc_offsetcurveui.h"
#include "lc_offsetcurvereadutils.h"
#include "lc_pointpickbutton.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_math.h"
#include "rs_spline.h"
#include "lc_splinepoints.h"
#include "ui_lc_propertieseditingwidget_offsetcurve.h"

namespace {
QString sourceKindName(LC_OffsetSourceGeometry::Kind kind) {
    using Kind = LC_OffsetSourceGeometry::Kind;
    switch (kind) {
    case Kind::Ellipse: return QObject::tr("Ellipse");
    case Kind::Spline: return QObject::tr("Spline");
    case Kind::SplinePoints: return QObject::tr("Spline points");
    case Kind::Parabola: return QObject::tr("Parabola");
    case Kind::BoundedHyperbola: return QObject::tr("Bounded hyperbola");
    case Kind::Line: return QObject::tr("Line");
    case Kind::Arc: return QObject::tr("Arc");
    case Kind::Circle: return QObject::tr("Circle");
    case Kind::Polyline: return QObject::tr("Polyline");
    case Kind::Invalid: break;
    }
    return QObject::tr("Unsupported");
}

std::optional<LC_OffsetSourceGeometry::Kind> sourceKind(const RS_Entity& entity) {
    const auto geometry = LC_OffsetSourceGeometry::capture(entity);
    if (!geometry) return std::nullopt;
    return geometry->kind();
}

bool mayMatchSourceKind(const RS_Entity& entity, LC_OffsetSourceGeometry::Kind kind) {
    using Kind = LC_OffsetSourceGeometry::Kind;
    switch (kind) {
    case Kind::Ellipse: return dynamic_cast<const RS_Ellipse*>(&entity) != nullptr;
    case Kind::Spline: return dynamic_cast<const RS_Spline*>(&entity) != nullptr;
    case Kind::SplinePoints: return dynamic_cast<const LC_SplinePoints*>(&entity) != nullptr;
    case Kind::Parabola: return dynamic_cast<const LC_Parabola*>(&entity) != nullptr;
    case Kind::BoundedHyperbola: return dynamic_cast<const LC_Hyperbola*>(&entity) != nullptr;
    case Kind::Line: return dynamic_cast<const RS_Line*>(&entity) != nullptr;
    case Kind::Arc: return dynamic_cast<const RS_Arc*>(&entity) != nullptr;
    case Kind::Circle: return dynamic_cast<const RS_Circle*>(&entity) != nullptr;
    case Kind::Polyline: return dynamic_cast<const RS_Polyline*>(&entity) != nullptr;
    case Kind::Invalid: return false;
    }
    return false;
}

bool sourceVariantSelectable(const RS_Entity& entity) {
    const auto geometry = LC_OffsetSourceGeometry::capture(entity);
    if (!geometry || !LC_OffsetWorkerRequest::Values::isSourceGeometryEncodable(*geometry))
        return false;
    if (const auto* points = dynamic_cast<const LC_SplinePoints*>(&entity))
        return !points->getData().fitTangentsConstrained ||
               points->fitTangentsMatchCurve();
    if (const auto* spline = dynamic_cast<const RS_Spline*>(&entity))
        return !spline->getData().fitTangentsConstrained ||
               spline->fitTangentsMatchCurve();
    return true;
}

struct OffsetDialogContext {
    LC_GraphicViewport* viewport = nullptr;
    RS_Document* document = nullptr; // Identity only; never dereferenced after an event turn.
    std::array<RS_Vector, 3> world;
    double angle = 0;
    bool counterClockwise = true;
    static OffsetDialogContext capture(LC_GraphicViewport* viewport) {
        OffsetDialogContext context; context.viewport = viewport;
        if (viewport) {
            context.document = viewport->getDocument();
            context.world = {viewport->toWorld({0, 0}), viewport->toWorld({1, 0}), viewport->toWorld({0, 1})};
            context.angle = viewport->toWorldAngle(viewport->toAbsUCSAngle(0));
            context.counterClockwise = viewport->areAnglesCounterClockwise();
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
        return angle == other.angle && counterClockwise == other.counterClockwise;
    }
};
}

LC_PropertiesEditingWidgetOffsetCurve::LC_PropertiesEditingWidgetOffsetCurve(QWidget* parent)
    : LC_EntityPropertiesEditorWidget(parent), ui(new Ui::LC_OffsetCurvePropertiesEditingWidget),
      m_limits(LC_OffsetCurveUI::sheetLimits()), m_worker(std::make_unique<LC_OffsetWorkerClient>(this)) {
    ui->setupUi(this);
    m_highlightTimer = new QTimer(this);
    m_highlightTimer->setObjectName("offsetSourceHighlightTimer");
    m_highlightTimer->setInterval(1000);
    connect(m_highlightTimer, &QTimer::timeout, this, [this] { checkSourceHighlight(); });
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
        if (m_entity->hasGeneralizedSourceDefinition() && m_entity->sourceDefinition()) {
            auto next = *m_entity->sourceDefinition();
            next.signedDistance = std::copysign(std::abs(next.signedDistance), inward ? -1.0 : 1.0);
            applySourceDefinition(next);
            return;
        }
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
    connect(ui->cbSource, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (m_refreshing || m_entity == nullptr || index < 0) return;
        bool validId = false;
        const auto id = ui->cbSource->itemData(index).toULongLong(&validId);
        auto* source = validId ? findSourceEntity(id) : nullptr;
        if (source == nullptr) return;
        const auto geometry = LC_OffsetSourceGeometry::capture(*source);
        if (!geometry || !sourceVariantSelectable(*source) ||
            static_cast<int>(geometry->kind()) != ui->cbSourceType->currentData().toInt()) return;
        const auto encoding = LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*geometry);
        if (!encoding) return;
        const SourceWitness witness{id, *encoding};
        if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(source);
            ellipse && m_entity->hasEditableDefinition()) {
            auto next = proposedDefinition();
            next.basis = ellipse->getData();
            apply(next, witness);
            return;
        }
        LC_OffsetSourceDefinition next;
        if (m_entity->sourceDefinition()) next = *m_entity->sourceDefinition();
        else {
            const auto& current = m_entity->definition();
            next.signedDistance = current.towardCentre ? -current.distance : current.distance;
            next.relativeTolerance = current.relTolerance;
            next.cacheReversed = current.cacheReversed;
        }
        next.source = *geometry;
        applySourceDefinition(next, witness);
    });
    connect(ui->cbSourceType, qOverload<int>(&QComboBox::currentIndexChanged), this,
        [this](int) { if (!m_refreshing) refreshSourceChoices(); });
    connect(ui->tbSelectSource, &QToolButton::clicked, this,
        [this] { selectCurrentSource(); });
}

LC_PropertiesEditingWidgetOffsetCurve::~LC_PropertiesEditingWidgetOffsetCurve() {
    detachPreparation();
    delete ui;
}

void LC_PropertiesEditingWidgetOffsetCurve::detachPreparation() {
    clearSourceWatch();
    m_entity = nullptr;
    m_busy = false; m_proposedDefinition.reset(); m_inputSnapshot.reset();
    m_worker.reset(); // Disconnect/reap before releasing the widget or caller's clone owner.
}

void LC_PropertiesEditingWidgetOffsetCurve::showEvent(QShowEvent* event) {
    LC_EntityPropertiesEditorWidget::showEvent(event);
    m_sourceHighlightVisible = true;
    refreshSourceHighlight();
}

void LC_PropertiesEditingWidgetOffsetCurve::hideEvent(QHideEvent* event) {
    m_sourceHighlightVisible = false;
    clearSourceWatch();
    LC_EntityPropertiesEditorWidget::hideEvent(event);
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
    ui->gbDefinition->setEnabled(m_entity && m_viewport && (m_entity->hasEditableDefinition() || m_entity->hasGeneralizedSourceDefinition()) &&
        m_generation < std::numeric_limits<qint64>::max() && m_sourceRevision < std::numeric_limits<qint64>::max());
    ui->leDistance->setEnabled(!busy && m_entity && m_viewport &&
        (m_entity->hasEditableDefinition() || m_entity->hasGeneralizedSourceDefinition()));
    ui->cbTowardCentre->setEnabled(!busy && m_entity && m_viewport &&
        (m_entity->hasEditableDefinition() || m_entity->hasGeneralizedSourceDefinition()));
    ui->cbReversed->setEnabled(!busy && m_entity && m_entity->hasEditableDefinition());
    ui->wPickPointCenter->setEnabled(!busy);
    ui->cbSourceType->setEnabled(!busy && m_entity && m_viewport);
    ui->cbSource->setEnabled(!busy && m_entity && m_viewport);
    ui->tbSelectSource->setEnabled(!busy && m_sourceCanSelect);
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
        if (m_entity && m_entity->hasGeneralizedSourceDefinition()) refresh(nullptr, true);
        else restoreCommittedText();
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
    clearSourceWatch();
    m_preferredSourceId = 0;
    {
        const QSignalBlocker blocked(ui->cbSourceType);
        ui->cbSourceType->clear();
    }
    m_lastResult.reset(); m_lastWorkerResult.reset(); m_attempts = 0;
    ui->lRefusal->clear();
    refresh(nullptr, true);
}

void LC_PropertiesEditingWidgetOffsetCurve::refresh(QLineEdit* completed, bool force) const {
    const QScopedValueRollback<bool> refreshing(m_refreshing, true);
    const bool available = m_entity != nullptr && (m_entity->hasEditableDefinition() || m_entity->hasGeneralizedSourceDefinition()) && m_viewport != nullptr;
    const bool ellipseDefinition = available && m_entity->hasEditableDefinition();
    const bool generalizedDefinition = available && m_entity->hasGeneralizedSourceDefinition();
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
    ui->cbSourceType->setEnabled(!m_busy && m_entity && m_viewport);
    ui->cbSource->setEnabled(!m_busy && m_entity && m_viewport);
    for (auto* picker : {ui->tbPickDistance, ui->tbPickMajor, ui->tbPickMinor, ui->tbPickRotation})
        picker->setEnabled(!m_busy);
    refreshSourceChoices();
    if (m_entity == nullptr || m_viewport == nullptr) return;
    for (auto* editor : definitionEditors()) editor->setEnabled(ellipseDefinition);
    ui->leDistance->setEnabled(!m_busy && available);
    ui->cbTowardCentre->setText(generalizedDefinition ? tr("Right side") : tr("Toward centre"));
    ui->cbTowardCentre->setEnabled(!m_busy && (ellipseDefinition || generalizedDefinition));
    ui->cbReversed->setEnabled(ellipseDefinition);
    for (auto* picker : {ui->tbPickDistance, ui->tbPickMajor, ui->tbPickMinor, ui->tbPickRotation})
        picker->setEnabled(!m_busy && ellipseDefinition);
    ui->wPickPointCenter->setEnabled(!m_busy && ellipseDefinition);
    const auto definition = proposedDefinition();
    const auto& basis = definition.basis;
    const auto center = toUIStr(basis.center);
    const double displayDistance = m_entity->hasGeneralizedSourceDefinition() && m_entity->sourceDefinition()
        ? std::abs(m_entity->sourceDefinition()->signedDistance) : definition.distance;
    text(ui->leDistance, asString(displayDistance));
    text(ui->leCenterX, center.first); text(ui->leCenterY, center.second);
    text(ui->leMajor, asString(basis.majorP.magnitude()));
    text(ui->leMinor, asString(basis.majorP.magnitude() * basis.ratio));
    text(ui->leRotation, toUIAngleDeg(basis.majorP.angle()));
    text(ui->leAngle1, asStringAngleDeg(basis.angle1));
    text(ui->leAngle2, asStringAngleDeg(basis.angle2));
    {
        const QSignalBlocker side(ui->cbTowardCentre), reversed(ui->cbReversed);
        const bool rightSide = generalizedDefinition && m_entity->sourceDefinition()
            ? m_entity->sourceDefinition()->signedDistance < 0 : definition.towardCentre;
        ui->cbTowardCentre->setChecked(rightSide);
        ui->cbReversed->setChecked(basis.reversed);
    }
    const bool arc = ellipseDefinition && !m_entity->sourceIsWhole();
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

void LC_PropertiesEditingWidgetOffsetCurve::refreshSourceChoices(bool preserveWatch) const {
    auto* document = m_viewport ? m_viewport->getDocument() : nullptr;
    if (document == nullptr || m_entity == nullptr) {
        ui->cbSourceType->clear();
        ui->cbSourceType->addItem(tr("Source type unavailable"));
        ui->cbSource->clear();
        ui->cbSource->addItem(tr("Source not available"));
        m_sourceCanSelect = false;
        ui->tbSelectSource->setEnabled(false);
        clearSourceWatch();
        return;
    }

    std::optional<LC_OffsetSourceDefinition> embedded;
    if (m_entity->sourceDefinition()) embedded = *m_entity->sourceDefinition();
    else {
        const auto& current = proposedDefinition();
        const auto ellipse = LC_OffsetSourceGeometry{current.basis};
        embedded = LC_OffsetSourceDefinition{ellipse,
            current.towardCentre ? -current.distance : current.distance,
            current.relTolerance, current.cacheReversed};
    }
    bool hasRequestedKind = false;
    const int requestedKindValue = ui->cbSourceType->currentData().toInt(&hasRequestedKind);
    const QSignalBlocker typeBlocked(ui->cbSourceType);
    const QSignalBlocker sourceBlocked(ui->cbSource);
    const auto selectedKind = embedded ? embedded->source.kind() : LC_OffsetSourceGeometry::Kind::Invalid;
    ui->cbSourceType->clear();
    for (const auto kind : LC_OffsetSourceGeometry::SupportedKinds) {
        ui->cbSourceType->addItem(sourceKindName(kind), static_cast<int>(kind));
    }
    const auto wantedKind = hasRequestedKind
        ? static_cast<LC_OffsetSourceGeometry::Kind>(requestedKindValue) : selectedKind;
    int typeIndex = ui->cbSourceType->findData(static_cast<int>(wantedKind));
    if (typeIndex < 0) typeIndex = 0;
    ui->cbSourceType->setCurrentIndex(typeIndex);
    const auto actualKind = static_cast<LC_OffsetSourceGeometry::Kind>(ui->cbSourceType->currentData().toInt());

    ui->cbSource->clear();
    int matchedIndex = -1;
    int matchingSources = 0;
    int preferredIndex = -1;
    bool embeddedSourceUnsupported = false;
    std::optional<QCborArray> embeddedEncoding;
    if (embedded) embeddedEncoding =
        LC_OffsetWorkerRequest::Values::encodeSourceGeometry(embedded->source);
    for (auto* entity : document->getEntityList()) {
        if (entity == nullptr || entity->getFlag(RS2::FlagDeleted)) continue;
        const auto geometry = LC_OffsetSourceGeometry::capture(*entity);
        if (!geometry || geometry->kind() != wantedKind) continue;
        std::optional<QCborArray> candidateEncoding;
        if (embedded && selectedKind == actualKind)
            candidateEncoding = LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*geometry);
        const bool matchesEmbedded = embeddedEncoding && candidateEncoding &&
            *embeddedEncoding == *candidateEncoding;
        if (matchesEmbedded) ++matchingSources;
        if (!sourceVariantSelectable(*entity)) {
            embeddedSourceUnsupported |= matchesEmbedded;
            continue;
        }
        const auto id = static_cast<qulonglong>(entity->getId());
        const auto label = tr("%1 %2").arg(sourceKindName(geometry->kind())).arg(id);
        ui->cbSource->addItem(label, QVariant::fromValue<qulonglong>(id));
        if (matchesEmbedded) {
            matchedIndex = ui->cbSource->count() - 1;
            if (id == m_preferredSourceId) preferredIndex = matchedIndex;
        }
    }
    if (preferredIndex >= 0) matchedIndex = preferredIndex;
    else if (matchingSources != 1) matchedIndex = -1;
    const bool hasMatchingSource = matchedIndex >= 0;
    if (!hasMatchingSource) {
        const QString placeholder = embedded && selectedKind == wantedKind
            ? matchingSources > 1 ? tr("Embedded source (ambiguous)")
              : embeddedSourceUnsupported ? tr("Embedded source (unsupported)")
                                          : tr("Embedded source (not found)")
            : ui->cbSource->count() == 0 ? tr("No supported source entities") : tr("Choose a source");
        ui->cbSource->insertItem(0, placeholder);
        matchedIndex = 0;
    }
    ui->cbSource->setCurrentIndex(matchedIndex);
    const auto* selected = hasMatchingSource ? findSourceEntity(ui->cbSource->currentData().toULongLong()) : nullptr;
    m_sourceCanSelect = selected && selected->isVisible() && !selected->isLocked();
    ui->tbSelectSource->setEnabled(m_sourceCanSelect && !m_busy);
    refreshSourceHighlight(preserveWatch, selectedKind == actualKind ? selected : nullptr);
}

void LC_PropertiesEditingWidgetOffsetCurve::clearSourceWatch() const {
    m_highlightTimer->stop();
    m_sourceHighlight.reset();
    m_highlightLifetime.reset();
    m_highlightViewport = nullptr;
    m_highlightSourceId = 0;
    m_highlightKind = LC_OffsetSourceGeometry::Kind::Invalid;
    m_highlightGeometry = {};
}

void LC_PropertiesEditingWidgetOffsetCurve::refreshSourceHighlight(bool preserveWatch,
                                                                   const RS_Entity* knownMatch) const {
    const QPointer<const LC_PropertiesEditingWidgetOffsetCurve> alive(this);
    auto previous = std::move(m_sourceHighlight);
    previous.reset();
    if (!preserveWatch) clearSourceWatch();
    if (!alive || !m_sourceHighlightVisible || !m_entity || !m_viewport) return;
    auto* document = m_viewport->getDocument();
    if (!document) return;
    std::optional<LC_OffsetSourceGeometry> embedded;
    if (m_entity->sourceDefinition()) embedded = m_entity->sourceDefinition()->source;
    else if (m_entity->hasEditableDefinition()) embedded = LC_OffsetSourceGeometry{m_entity->definition().basis};
    if (!embedded) return;
    const auto encoding = LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*embedded);
    if (!encoding) return;
    if (!preserveWatch) {
        const auto lifetime = m_viewport->watchOffsetSourceHighlightLifetime();
        if (!alive || lifetime.expired()) return;
        m_highlightViewport = m_viewport;
        m_highlightLifetime = lifetime;
        m_highlightGeometry = *encoding;
        m_highlightKind = embedded->kind();
        m_highlightTimer->start();
    }
    const RS_Entity* match = nullptr;
    const RS_Entity* preferred = nullptr;
    int count = 0;
    if (knownMatch && !knownMatch->isDeleted() && sourceVariantSelectable(*knownMatch)) {
        const auto geometry = LC_OffsetSourceGeometry::capture(*knownMatch);
        const auto candidate = geometry
            ? LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*geometry) : std::nullopt;
        if (geometry && geometry->kind() == embedded->kind() && candidate && *candidate == *encoding) {
            match = knownMatch;
        }
    }
    if (!match) {
        for (auto* entity : document->getEntityList()) {
            if (!entity || entity->isDeleted()) continue;
            const auto geometry = LC_OffsetSourceGeometry::capture(*entity);
            if (!geometry || geometry->kind() != embedded->kind()) continue;
            const auto candidate = LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*geometry);
            if (!candidate || *candidate != *encoding) continue;
            ++count;
            if (sourceVariantSelectable(*entity)) {
                match = entity;
                if (entity->getId() == m_preferredSourceId) preferred = entity;
            }
        }
        if (preferred) match = preferred;
        else if (count != 1) match = nullptr;
    }
    if (!match) {
        if (preserveWatch) m_highlightSourceId = 0;
        return;
    }
    m_highlightSourceId = match->getId();
    auto highlight = m_viewport->highlightOffsetSource(*match);
    if (alive && highlight) {
        m_sourceHighlight = std::move(highlight);
        m_highlightViewport = m_viewport;
        m_highlightLifetime = m_viewport->offsetSourceHighlightLifetime();
        m_highlightSourceId = match->getId();
        m_highlightKind = embedded->kind();
        m_highlightGeometry = *encoding;
        m_highlightTimer->start();
    }
}

void LC_PropertiesEditingWidgetOffsetCurve::checkSourceHighlight() const {
    if (!m_sourceHighlightVisible || !m_highlightViewport ||
        m_viewport != m_highlightViewport || m_highlightLifetime.expired()) {
        clearSourceWatch();
        return;
    }
    try {
        const auto* document = m_highlightViewport->getDocument();
        if (!document) {
            clearSourceWatch();
            return;
        }
        const RS_Entity* match = nullptr;
        const RS_Entity* preferred = nullptr;
        std::size_t matches = 0;
        for (auto* entity : document->getEntityList()) {
            if (!entity || entity->isDeleted() || !mayMatchSourceKind(*entity, m_highlightKind)) continue;
            const auto geometry = LC_OffsetSourceGeometry::capture(*entity);
            const auto encoding = geometry
                ? LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*geometry) : std::nullopt;
            if (!encoding || *encoding != m_highlightGeometry) continue;
            ++matches;
            if (sourceVariantSelectable(*entity)) {
                match = entity;
                if (entity->getId() == m_preferredSourceId) preferred = entity;
            }
        }
        if (preferred) match = preferred;
        else if (matches != 1) match = nullptr;
        const qulonglong id = match ? match->getId() : 0;
        const bool visible = match && match->isVisible();
        if (id != m_highlightSourceId || visible != static_cast<bool>(m_sourceHighlight)) {
            refreshSourceChoices(true);
        } else {
            bool validId = false;
            const auto selectedId = ui->cbSource->currentData().toULongLong(&validId);
            m_sourceCanSelect = validId && match && selectedId == id && visible && !match->isLocked();
            ui->tbSelectSource->setEnabled(!m_busy && m_sourceCanSelect);
        }
    } catch (const std::bad_alloc&) {
        clearSourceWatch();
    }
}

RS_Entity* LC_PropertiesEditingWidgetOffsetCurve::findSourceEntity(qulonglong id) const {
    auto* document = m_viewport ? m_viewport->getDocument() : nullptr;
    if (document == nullptr) return nullptr;
    for (auto* entity : document->getEntityList()) {
        if (entity == nullptr || entity->getFlag(RS2::FlagDeleted) || entity->getId() != id) continue;
        const auto kind = sourceKind(*entity);
        if (kind && sourceVariantSelectable(*entity) &&
            static_cast<int>(*kind) == ui->cbSourceType->currentData().toInt()) return entity;
    }
    return nullptr;
}

bool LC_PropertiesEditingWidgetOffsetCurve::sourceMatches(const SourceWitness& source) const {
    const auto* entity = findSourceEntity(source.id);
    if (!entity) return false;
    const auto geometry = LC_OffsetSourceGeometry::capture(*entity);
    if (!geometry) return false;
    const auto encoding = LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*geometry);
    return encoding && *encoding == source.geometry;
}

void LC_PropertiesEditingWidgetOffsetCurve::selectCurrentSource() const {
    if (m_entity == nullptr || m_viewport == nullptr) return;
    bool validId = false;
    const auto id = ui->cbSource->currentData().toULongLong(&validId);
    auto* source = validId ? findSourceEntity(id) : nullptr;
    auto* document = m_viewport->getDocument();
    if (source == nullptr || document == nullptr || !source->isVisible() || source->isLocked()) return;
    document->select(QList<RS_Entity*>{source}, true);
    m_viewport->notifyChanged(RS2::RedrawDrawing);
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

void LC_PropertiesEditingWidgetOffsetCurve::apply(const LC_EllipseOffsetDefinition& definition,
                                                   std::optional<SourceWitness> source) const {
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
        refreshSourceChoices();
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
        if (!matches || !context.matches(m_viewport) || (source && !sourceMatches(*source))) {
            failure(tr("The offset editing input changed; the value was not changed.")); return;
        }
        if (!prepared.generate()) {
            m_lastResult = LC_OffsetWorkerEdit::applyLocal(*target, std::move(prepared));
            if (!alive || generation != m_generation || target != m_entity) return;
            if (*m_lastResult && source) m_preferredSourceId = source->id;
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
            [alive, generation, revision, target, context, source](LC_OffsetWorkerClient::Outcome outcome) {
                if (!alive || generation != alive->m_generation || revision != alive->m_sourceRevision ||
                    target != alive->m_entity || !alive->m_busy) return;
                const auto* self = alive.data();
                bool published = false;
                const auto refuse = [self, &published](const QString& text) {
                    if (!published) self->m_lastResult.reset();
                    self->m_proposedDefinition.reset(); self->m_inputSnapshot.reset();
                    if (!published) self->restoreCommittedText();
                    self->refreshSourceChoices();
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
                    if (!matches || !context.matches(self->m_viewport) ||
                        (source && !self->sourceMatches(*source))) {
                        refuse(self->tr("The offset editing input changed; the value was not changed.")); return;
                    }
                    self->m_lastResult = outcome.completed->apply(*target);
                    if (!alive || generation != alive->m_generation || target != alive->m_entity) return;
                    if (*self->m_lastResult && source) self->m_preferredSourceId = source->id;
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

void LC_PropertiesEditingWidgetOffsetCurve::applySourceDefinition(const LC_OffsetSourceDefinition& definition,
                                                                   std::optional<SourceWitness> source) const {
    if (m_entity == nullptr || m_viewport == nullptr || !m_worker) return;
    const QPointer<const LC_PropertiesEditingWidgetOffsetCurve> alive(this);
    if (m_generation == std::numeric_limits<qint64>::max() ||
        m_sourceRevision == std::numeric_limits<qint64>::max() ||
        m_attempts == std::numeric_limits<std::size_t>::max()) {
        ui->lRefusal->setText(tr("The offset editing session is no longer current; the source was not changed."));
        return;
    }
    const auto generation = ++m_generation;
    const auto revision = m_sourceRevision;
    auto* const target = m_entity;
    m_worker->cancel();
    ++m_attempts;
    m_lastResult.reset();
    m_lastWorkerResult.reset();
    try {
        const auto guard = m_preparationGuard;
        if (guard && !guard()) {
            refresh(nullptr, true);
            ui->lRefusal->setText(tr("The offset editing input changed; the source was not changed."));
            return;
        }
        const auto context = OffsetDialogContext::capture(m_viewport);
        auto snapshot = LC_OffsetCurveEditSnapshot::capture(*target);
        if (!alive || generation != m_generation || target != m_entity) return;
        if (!snapshot) {
            refresh(nullptr, true);
            ui->lRefusal->setText(tr("The offset input exceeds the bounded snapshot limits; the source was not changed."));
            return;
        }
        auto prepared = LC_OffsetWorkerEdit::prepareSourceDefinition(*target, definition, m_limits);
        if (source && !sourceMatches(*source)) {
            refresh(nullptr, true);
            ui->lRefusal->setText(tr("The offset editing input changed; the source was not changed."));
            return;
        }
        if (!prepared.generateSource()) {
            m_lastResult = prepared.result();
            refresh(nullptr, true);
            ui->lRefusal->setText(LC_OffsetCurveUI::refusalText(*m_lastResult));
            return;
        }
        m_inputSnapshot = std::move(snapshot);
        LC_OffsetWorkerRequest::Request request{{generation, generation, revision, 0}, *prepared.generateSource()};
        const bool accepted = m_worker->submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [alive, generation, revision, target, context, source](LC_OffsetWorkerClient::Outcome outcome) mutable {
                if (!alive || generation != alive->m_generation || revision != alive->m_sourceRevision ||
                    target != alive->m_entity || !alive->m_busy) return;
                const auto* self = alive.data();
                const auto refuse = [self](const QString& text) {
                    self->m_inputSnapshot.reset();
                    self->refresh(nullptr, true);
                    self->ui->lRefusal->setText(text);
                    self->setBusy(false);
                };
                try {
                    if (outcome.status != LC_OffsetWorkerClient::Status::Completed || !outcome.completed) {
                        refuse(outcome.status == LC_OffsetWorkerClient::Status::TimedOut
                            ? self->tr("Offset computation timed out; the source was not changed.")
                            : self->tr("The offset worker could not complete the request; the source was not changed."));
                        return;
                    }
                    if (const auto* result = std::get_if<LC_OffsetWorkerResult::Native>(&outcome.completed->reply().result))
                        self->m_lastWorkerResult = static_cast<const LC_OffsetCurveEditResult&>(*result);
                    const auto guard = self->m_preparationGuard;
                    const bool current = !guard || guard();
                    if (!alive || generation != alive->m_generation || target != alive->m_entity) return;
                    const auto snapshot = self->m_inputSnapshot;
                    const bool matches = current && context.matches(self->m_viewport) && snapshot && snapshot->matches(*target);
                    if (!alive || generation != alive->m_generation || target != alive->m_entity) return;
                    if (!matches || !context.matches(self->m_viewport) ||
                        (source && !self->sourceMatches(*source))) {
                        refuse(self->tr("The offset editing input changed; the source was not changed."));
                        return;
                    }
                    self->m_lastResult = outcome.completed->apply(*target);
                    if (!alive || generation != alive->m_generation || target != alive->m_entity) return;
                    if (*self->m_lastResult && source) self->m_preferredSourceId = source->id;
                    if (*self->m_lastResult && self->m_lastResult->changed &&
                        self->m_sourceRevision < std::numeric_limits<qint64>::max()) ++self->m_sourceRevision;
                    self->m_inputSnapshot.reset();
                    self->refresh();
                    self->ui->lRefusal->setText(*self->m_lastResult ? QString{} :
                        LC_OffsetCurveUI::refusalText(*self->m_lastResult));
                    self->setBusy(false);
                } catch (const std::bad_alloc&) {
                    if (alive) refuse(self->tr("Not enough memory to replace the source; the value was not changed."));
                }
            });
        if (!accepted) {
            m_inputSnapshot.reset();
            refresh(nullptr, true);
            ui->lRefusal->setText(tr("The offset worker could not start the request; the source was not changed."));
            return;
        }
        const bool wasBusy = m_busy;
        m_busy = true;
        refresh();
        ui->lRefusal->setText(tr("Calculating replacement source..."));
        if (!wasBusy) emit preparationBusyChanged(true);
    } catch (const std::bad_alloc&) {
        if (alive) {
            m_inputSnapshot.reset();
            refresh(nullptr, true);
            ui->lRefusal->setText(tr("Not enough memory to replace the source; the value was not changed."));
        }
    }
}

void LC_PropertiesEditingWidgetOffsetCurve::applyValue(Field field, double value) const {
    if (m_entity == nullptr || m_viewport == nullptr) return;
    if (m_entity->hasGeneralizedSourceDefinition() && m_entity->sourceDefinition()) {
        if (field != Field::Distance) return;
        if (value < 0.0) {
            inputRefused(ui->leDistance, tr("Enter a positive distance; use the Right side option to reverse direction."));
            return;
        }
        auto next = *m_entity->sourceDefinition();
        next.signedDistance = std::copysign(value, next.signedDistance);
        applySourceDefinition(next);
        return;
    }
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
