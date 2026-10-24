/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include "lc_action_offsetcurve_updatesource.h"

#include <memory>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include <QMap>
#include <QSet>
#include <QStringList>

#include "lc_actioncontext.h"
#include "lc_busycursorguard.h"
#include "lc_offsetcurveui.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "lc_selectedset.h"

LC_ActionOffsetCurveUpdateSource::LC_ActionOffsetCurveUpdateSource(LC_ActionContext* context)
    : LC_ActionPreSelectionAwareBase("ActionOffsetCurveUpdateSource", context,
          RS2::ActionOffsetCurveUpdateSource, {RS2::EntityOffsetCurve}) {
    m_liveView = m_graphicView;
    if (m_document) m_document->getSelection()->addListener(this);
}

void LC_ActionOffsetCurveUpdateSource::selectedSetDestroyed() {
    // The tracked selection registration is already removed; its document is partly destroyed.
    m_document = nullptr;
    m_graphic = nullptr;
    m_keyEntity = nullptr;
    if (m_preview) m_preview->setParent(nullptr);
    m_selectedEntities.clear();
    m_selectionComplete = false;
    m_finished = true;
    setFinished();
}

bool LC_ActionOffsetCurveUpdateSource::currentContext() const {
    // Selection destruction nulls m_document; a live view/context pointer alone cannot vouch for it.
    return m_actionContext && m_document && m_liveView && !m_liveView->isClosing() &&
        m_liveView.data() == m_graphicView && m_actionContext->getDocument() == m_document &&
        m_actionContext->getGraphicView() == m_liveView.data() &&
        m_liveView->getViewPort() == m_viewport && m_liveView->getDocument() == m_document &&
        (m_document->rtti() == RS2::EntityGraphic || m_document->rtti() == RS2::EntityBlock) &&
        m_document->isOwner() && m_document->isVisible() && !m_document->isLocked();
}

RS_Entity* LC_ActionOffsetCurveUpdateSource::liveMember(const RS_Entity* address) const {
    if (!address || !currentContext()) return nullptr;
    // Membership precedes dereferencing a pointer retained across mouse events.
    const int index = m_document->findEntityIndex(address);
    if (index < 0) return nullptr;
    auto* entity = m_document->entityAt(index);
    if (!entity || entity->getParent() != m_document || entity->isDeleted()) return nullptr;
    return entity;
}

void LC_ActionOffsetCurveUpdateSource::init(const int status) {
    if (!currentContext()) {
        m_report = {};
        m_report.sourceRefusal = tr("The editable document or view has changed.");
        publishReport();
        return;
    }
    if (mayInitWithContextEntity(status)) {
        const auto* saved = m_actionContext->getContextMenuActionContextEntity();
        if (saved && !liveMember(saved))
            m_actionContext->saveContextMenuActionContext(nullptr, RS_Vector(false), false);
    }
    LC_ActionPreSelectionAwareBase::init(status);
}

bool LC_ActionOffsetCurveUpdateSource::isEntityAllowedToSelect(RS_Entity* entity) const {
    entity = liveMember(entity);
    // Existing selected native targets stay in the attempt inventory even if
    // their visibility/lock/definition changes before the source is picked.
    return entity && entity->rtti() == RS2::EntityOffsetCurve &&
        dynamic_cast<LC_OffsetCurve*>(entity);
}

void LC_ActionOffsetCurveUpdateSource::doInitWithContextEntity(RS_Entity* entity, const RS_Vector&) {
    if (!isEntityAllowedToSelect(entity)) return;
    // The base helper toggles selection. Context activation must not deselect
    // an already-selected target or disturb unrelated preselection.
    if (!entity->getFlag(RS2::FlagSelected)) m_document->select(QList<RS_Entity*>{entity}, true);
    m_selectedEntities = {entity};
    onSelectionCompleted(true, true);
    redrawDrawing();
}

void LC_ActionOffsetCurveUpdateSource::onSelectionCompleted(bool, const bool fromInit) {
    if (!currentContext()) return;
    if (!fromInit) collectSelectedEntities();
    m_targets.clear();
    QSet<RS_Entity*> seen;
    for (auto* address : std::as_const(m_selectedEntities)) {
        auto* entity = liveMember(address);
        if (!isEntityAllowedToSelect(entity) || seen.contains(entity)) continue;
        seen.insert(entity);
        m_targets.append({entity, entity->getId()});
    }
    m_selectionComplete = !m_targets.isEmpty();
    if (!m_selectionComplete) commandMessage(tr("Select offset curves first."));
    updateActionPrompt();
    updateMouseCursor();
}

QString LC_ActionOffsetCurveUpdateSource::targetRefusal(RS_Entity* entity) const {
    if (!entity) return tr("Target is no longer a live top-level entity in this document.");
    if (entity->rtti() != RS2::EntityOffsetCurve || !dynamic_cast<LC_OffsetCurve*>(entity))
        return tr("Target is not an offset curve.");
    if (!static_cast<const LC_OffsetCurve*>(entity)->isOwner()) return tr("Target does not own its display cache.");
    if (!entity->isVisible()) return tr("Target is hidden.");
    if (entity->isLocked()) return tr("Target is locked.");
    const auto* curve = static_cast<const LC_OffsetCurve*>(entity);
    if (!curve->isDefined()) return tr("Target has no editable offset definition.");
    if (!curve->isConsistent()) return tr("Target cache no longer matches its stored definition.");
    return {};
}

QString LC_ActionOffsetCurveUpdateSource::sourceRefusal(RS_Entity* entity) const {
    if (!entity) return tr("Source must be a live top-level ellipse in the same editable document.");
    if (entity->rtti() != RS2::EntityEllipse || !dynamic_cast<RS_Ellipse*>(entity))
        return tr("Source is not an ellipse.");
    if (!entity->isVisible()) return tr("Source ellipse is hidden.");
    // A locked source is read-only input, not a replacement target.
    return {};
}

RS_Ellipse* LC_ActionOffsetCurveUpdateSource::sourceAt(const LC_MouseEvent* event) const {
    if (!event || !currentContext()) return nullptr;
    // The general editing picker excludes locked layers. This source is read-only;
    // inspect only directly owned ellipses, retaining the normal range and tie order.
    double nearest = getCatchDistance(getSnapRange(), m_catchEntityGuiRange);
    RS_EntityContainer nearby(nullptr, false);
    m_document->appendNearby(event->graphPoint, nearest, nearby);
    RS_Ellipse* result = nullptr;
    for (auto* entity : nearby) {
        if (!entity || entity->getParent() != m_document || entity->isDeleted() ||
            !sourceRefusal(entity).isEmpty()) continue;
        auto* ellipse = static_cast<RS_Ellipse*>(entity);
        const double distance = ellipse->getDistanceToPoint(event->graphPoint);
        if (std::isfinite(distance) && distance >= 0 && distance <= nearest) {
            nearest = distance;
            result = ellipse;
        }
    }
    return result;
}

void LC_ActionOffsetCurveUpdateSource::onMouseMoveEventSelected(int, const LC_MouseEvent* event) {
    if (auto* ellipse = sourceAt(event)) highlightHover(ellipse);
}

void LC_ActionOffsetCurveUpdateSource::onMouseLeftButtonReleaseSelected(int, const LC_MouseEvent* event) {
    if (!currentContext()) {
        trigger();
        return;
    }
    auto* ellipse = sourceAt(event);
    if (!ellipse) {
        commandMessage(tr("Pick a visible top-level source ellipse in this editable document."));
        return;
    }
    useSource(ellipse);
}

void LC_ActionOffsetCurveUpdateSource::useSource(RS_Entity* source) {
    m_source = source;
    auto* member = liveMember(source);
    m_sourceId = member ? member->getId() : 0;
    trigger();
    m_source = nullptr;
    m_sourceId = 0;
}

void LC_ActionOffsetCurveUpdateSource::trigger() {
    if (!currentContext()) {
        m_report = {};
        m_report.sourceRefusal = tr("The editable document or view has changed.");
        for (const auto& target : std::as_const(m_targets)) {
            m_report.outcomes.append({target.id, std::nullopt, m_report.sourceRefusal});
            ++m_report.refused;
        }
        publishReport();
        return;
    }
    if (!m_selectionComplete || m_targets.isEmpty()) {
        commandMessage(tr("Select offset curves first."));
        return;
    }
    LC_ActionPreSelectionAwareBase::trigger();
}

bool LC_ActionOffsetCurveUpdateSource::doTriggerModifications(LC_DocumentModificationBatch& batch) {
    batch.dontSetActiveLayerAndPen();
    m_report = {};
    auto* source = liveMember(m_source);
    if (source && source->getId() != m_sourceId) source = nullptr;
    m_report.sourceRefusal = sourceRefusal(source);
    std::optional<RS_EllipseData> basis;
    if (m_report.sourceRefusal.isEmpty()) basis = static_cast<RS_Ellipse*>(source)->getData();

    struct Staged {
        LC_OffsetCurve* original;
        EntityId id;
        std::unique_ptr<LC_OffsetCurve> clone;
        std::unique_ptr<LC_OffsetCurveEditSnapshot> snapshot;
        qsizetype outcomeIndex;
    };
    std::vector<Staged> staged;
    {
        LC_BusyCursorGuard busy;
        for (const auto& target : std::as_const(m_targets)) {
            Outcome outcome;
            outcome.targetId = target.id;
            auto* entity = liveMember(target.address);
            if (entity && entity->getId() != target.id) entity = nullptr;
            outcome.refusal = targetRefusal(entity);
            if (outcome.refusal.isEmpty()) outcome.refusal = m_report.sourceRefusal;
            if (!outcome.refusal.isEmpty()) {
                ++m_report.refused;
                m_report.outcomes.append(std::move(outcome));
                continue;
            }

            auto* original = static_cast<LC_OffsetCurve*>(entity);
            auto snapshot = LC_OffsetCurveEditSnapshot::capture(*original);
            if (!snapshot) {
                outcome.refusal = tr("Target cache or metadata exceeds safe clone admission.");
                ++m_report.refused;
                m_report.outcomes.append(std::move(outcome));
                continue;
            }
            std::unique_ptr<LC_OffsetCurve> clone(static_cast<LC_OffsetCurve*>(original->clone()));
            clone->setParent(nullptr); // Private setters must not scan the live selection domain.
            if (liveMember(original) != original || original->getId() != target.id ||
                !targetRefusal(original).isEmpty() || !snapshot->matches(*original)) {
                outcome.refusal = tr("Target changed while its private replacement was prepared.");
                ++m_report.refused;
                m_report.outcomes.append(std::move(outcome));
                continue;
            }
            clone->clearSelectionFlag();
            // H supplies the shared formatter/guard. No create/adopt/checkCache,
            // update(), fallback, or second setter call belongs in this wrapper.
            outcome.edit = clone->setBasis(*basis, m_limits);
            if (!*outcome.edit) {
                outcome.refusal = LC_OffsetCurveUI::refusalText(*outcome.edit);
                ++m_report.refused;
            } else if (!outcome.edit->changed) {
                ++m_report.unchanged;
            } else {
                staged.push_back({original, target.id, std::move(clone), std::move(snapshot),
                    m_report.outcomes.size()});
                ++m_report.updated;
            }
            m_report.outcomes.append(std::move(outcome));
        }
    }
    // Complete every potentially allocating revalidation before preparing a handoff.
    qsizetype published = 0;
    for (auto& entry : staged) {
        if (liveMember(entry.original) != entry.original || entry.original->getId() != entry.id ||
            !targetRefusal(entry.original).isEmpty() || !entry.snapshot->matches(*entry.original)) {
            --m_report.updated;
            ++m_report.refused;
            m_report.outcomes[entry.outcomeIndex].refusal =
                tr("Target changed while its private replacement was prepared.");
            entry.clone.reset();
            continue;
        }
        ++published;
    }
    if (published == 0) return false;

    // Preserve caller prefixes and all candidate owners if either list preparation throws.
    LC_DocumentModificationBatch prepared;
    prepared.entitiesToAdd = batch.entitiesToAdd;
    prepared.entitiesToDelete = batch.entitiesToDelete;
    const auto maximum = std::numeric_limits<qsizetype>::max();
    if (published > maximum - prepared.entitiesToAdd.size() ||
        published > maximum - prepared.entitiesToDelete.size())
        throw std::length_error("Update from Source batch exceeds native list capacity");
    // reserve() can retain already-reserved shared storage on supported Qt.
    prepared.entitiesToAdd.detach();
    prepared.entitiesToDelete.detach();
    prepared.entitiesToAdd.reserve(prepared.entitiesToAdd.size() + published);
    prepared.entitiesToDelete.reserve(prepared.entitiesToDelete.size() + published);
    for (const auto& entry : staged)
        if (entry.clone) prepared.replace(entry.original, entry.clone.get());

    // No validation, allocation or virtual callback below the ownership boundary.
    static_assert(noexcept(batch.entitiesToAdd.swap(prepared.entitiesToAdd)));
    static_assert(noexcept(batch.entitiesToDelete.swap(prepared.entitiesToDelete)));
    static_assert(noexcept(std::declval<std::unique_ptr<LC_OffsetCurve>&>().release()));
    batch.entitiesToAdd.swap(prepared.entitiesToAdd);
    batch.entitiesToDelete.swap(prepared.entitiesToDelete);
    for (auto& entry : staged) entry.clone.release();
    return true;
}

void LC_ActionOffsetCurveUpdateSource::doTriggerSelectionUpdate(
    const bool keepSelected, const LC_DocumentModificationBatch& batch) {
    if (keepSelected) select(batch.entitiesToAdd);
    // Refused/no-op originals and unrelated selection were never deselected.
}

void LC_ActionOffsetCurveUpdateSource::publishReport() {
    QString message = tr("Update from Source: %1 updated, %2 unchanged, %3 refused.")
        .arg(m_report.updated).arg(m_report.unchanged).arg(m_report.refused);
    QMap<QString, unsigned> reasons;
    for (const auto& outcome : std::as_const(m_report.outcomes))
        if (!outcome.refusal.isEmpty()) ++reasons[outcome.refusal];
    if (reasons.isEmpty() && !m_report.sourceRefusal.isEmpty())
        reasons[m_report.sourceRefusal] = 1;
    QStringList details;
    for (auto it = reasons.cbegin(); it != reasons.cend(); ++it)
        details.append(tr("%1 (%2)").arg(it.key()).arg(it.value()));
    if (!details.isEmpty()) message += " " + details.join("; ");
    commandMessage(message);
}

void LC_ActionOffsetCurveUpdateSource::doTriggerCompletion(bool) {
    publishReport();
    if (m_report.sourceRefusal.isEmpty() && currentContext()) finish();
    else if (!currentContext()) { m_finished = true; setFinished(); }
}

void LC_ActionOffsetCurveUpdateSource::onMouseRightButtonReleaseSelected(int, const LC_MouseEvent*) {
    if (currentContext()) finish();
}

void LC_ActionOffsetCurveUpdateSource::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select offset curves") + getSelectionCompletionHintMsg(),
        MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Complete selection")));
}

void LC_ActionOffsetCurveUpdateSource::updateActionPromptForSelected(int) {
    updatePromptTRCancel(tr("Select source ellipse"));
}

RS2::CursorType LC_ActionOffsetCurveUpdateSource::doGetMouseCursorSelected(int) {
    return RS2::SelectCursor;
}
