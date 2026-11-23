/*
 * This file is part of the LibreCAD project.
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 * Licensed under GPL-2.0-or-later.
 */
#include "lc_action_modify_trim_batch.h"

#include <QKeyEvent>

#include "lc_actioninfomessagebuilder.h"
#include "rs_atomicentity.h"
#include "rs_document.h"
#include "rs_information.h"
#include "rs_settings.h"

LC_ActionModifyTrimBatch::LC_ActionModifyTrimBatch(LC_ActionContext* actionContext)
    : LC_ActionPreSelectionAwareBase("ActionModifyTrimBatch", actionContext, RS2::ActionModifyTrimBatch,
                                     {RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle, RS2::EntityEllipse}),
      m_limits(std::make_unique<LC_TrimLimitSet>()) {}

LC_ActionModifyTrimBatch::~LC_ActionModifyTrimBatch() = default;

void LC_ActionModifyTrimBatch::init(const int status) {
    RS_PreviewActionInterface::init(status);
    if (status < 0) {
        m_selectedEntities.clear();
        m_batch.reset();
        m_limits = std::make_unique<LC_TrimLimitSet>();
        return;
    }
    if (!m_selectionComplete) {
        const unsigned int count = collectSelectedEntities();
        if (count > 0) onSelectionCompleted(false, true);
    }
}
void LC_ActionModifyTrimBatch::finish() {
    m_batch.reset();
    m_limits = std::make_unique<LC_TrimLimitSet>();
    LC_ActionPreSelectionAwareBase::finish();
}

bool LC_ActionModifyTrimBatch::isEntityAllowedToSelect(RS_Entity* entity) const {
    return entity != nullptr && entity->isAtomic() && isReplaceableByTrim(*entity) && !entity->isLocked() &&
           entity->isVisible() && (entity->rtti() == RS2::EntityLine || entity->rtti() == RS2::EntityArc ||
                                   entity->rtti() == RS2::EntityCircle || entity->rtti() == RS2::EntityEllipse);
}

void LC_ActionModifyTrimBatch::onSelectionCompleted([[maybe_unused]] const bool singleEntity, const bool fromInit) {
    setSelectionComplete(false, fromInit);
    if (!m_selectionComplete) return;
    setStatus(SelectLimit);
    updateActionPrompt();
}

bool LC_ActionModifyTrimBatch::isAllowSelectionFinishByEnterForEmptySelection() {
    return m_selectionComplete;
}

bool LC_ActionModifyTrimBatch::isAllowTriggerOnEmptySelection() { return false; }

RS_Entity* LC_ActionModifyTrimBatch::catchLimitEntity(const LC_MouseEvent* event) const {
    RS_Entity* entity = catchEntityByEvent(event, RS2::ResolveAllButTextImage);
    if (entity != nullptr && entity->getParent() != nullptr &&
        (entity->getParent()->rtti() == RS2::EntityPolyline || entity->getParent()->rtti() == RS2::EntitySpline)) {
        entity = entity->getParent();
    }
    return entity;
}

void LC_ActionModifyTrimBatch::rebuildBatch() {
    m_limits->build();
    m_batch = std::make_unique<LC_TrimBatch>(m_selectedEntities, *m_limits);
}

void LC_ActionModifyTrimBatch::onMouseLeftButtonReleaseSelected(const int status, const LC_MouseEvent* event) {
    if (status == SelectLimit) {
        RS_Entity* limit = catchLimitEntity(event);
        if (limit == nullptr) return;
        m_limits = std::make_unique<LC_TrimLimitSet>();
        m_limits->add(*limit);
        rebuildBatch();
        if (!m_batch->anyCut()) {
            commandMessage(tr("None of the selected entities crosses the limit"));
            m_batch.reset();
            m_limits = std::make_unique<LC_TrimLimitSet>();
            return;
        }
        setStatus(ChooseSide);
        previewForSide(event->snapPoint);
    }
    else if (status == ChooseSide) {
        m_cursorSides = m_batch->cursorSides(event->snapPoint);
        if (std::none_of(m_cursorSides.begin(), m_cursorSides.end(), [](int side) { return side != 0; })) return;
        trigger();
    }
}

void LC_ActionModifyTrimBatch::onMouseRightButtonReleaseSelected(const int status, [[maybe_unused]] const LC_MouseEvent* event) {
    if (status == ChooseSide) {
        deletePreview();
        m_batch.reset();
        m_limits = std::make_unique<LC_TrimLimitSet>();
        setStatus(SelectLimit);
    }
    else if (status == SelectLimit) {
        deletePreviewAndHighlights();
        m_selectionComplete = false;
        m_selectedEntities.clear();
        setStatus(SelectTargets);
    }
}

void LC_ActionModifyTrimBatch::previewForSide(const RS_Vector& point) {
    if (m_batch == nullptr) return;
    deletePreview();
    m_cursorSides = m_batch->cursorSides(point);
    auto removed = m_batch->removedPieces(m_cursorSides);
    const int pieces = static_cast<int>(removed.size());
    for (auto& piece : removed) {
        if (piece != nullptr) previewEntity(piece.release());
    }
    if (m_showRefEntitiesOnPreview) {
        for (const RS_Vector& cutPoint : m_batch->cutPoints()) previewRefPoint(cutPoint);
    }
    if (isInfoCursorForModificationEnabled()) {
        msg(tr("Trim Batch")).string(tr("Removed"), QString::number(pieces)).toInfoCursorZone2(false);
    }
    redrawDrawing();
}

void LC_ActionModifyTrimBatch::onMouseMoveEventSelected(const int status, const LC_MouseEvent* event) {
    if (status == ChooseSide) previewForSide(event->snapPoint);
}

void LC_ActionModifyTrimBatch::onCoordinateEvent(const int status, [[maybe_unused]] const bool isZero, const RS_Vector& coord) {
    if (status == ChooseSide && m_batch != nullptr) {
        m_cursorSides = m_batch->cursorSides(coord);
        if (std::any_of(m_cursorSides.begin(), m_cursorSides.end(), [](int side) { return side != 0; })) trigger();
    }
}

bool LC_ActionModifyTrimBatch::doTriggerModifications(LC_DocumentModificationBatch& context) {
    if (m_batch == nullptr) return false;
    m_lastStats = m_batch->apply(m_cursorSides, context);
    return m_lastStats.trimmed > 0;
}

void LC_ActionModifyTrimBatch::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& context) {
    if (!context.success) return;
    if (!keepSelected) {
        unselectAll();
        m_selectedEntities.clear();
        return;
    }
    QList<RS_Entity*> nextSelection = context.entitiesToAdd;
    QList<RS_Entity*> toUnselect;
    for (RS_Entity* selected : std::as_const(m_selectedEntities)) {
        if (context.entitiesToDelete.contains(selected)) toUnselect.append(selected);
        else nextSelection.append(selected);
    }
    if (!toUnselect.isEmpty()) unselect(toUnselect);
    if (!nextSelection.isEmpty()) select(nextSelection);
    m_selectedEntities = nextSelection;
}

void LC_ActionModifyTrimBatch::doTriggerCompletion(const bool success) {
    if (!success) return;
    commandMessage(tr("Trimmed %1 entities, removed %2 pieces (%3 undetermined kept)")
                       .arg(m_lastStats.trimmed).arg(m_lastStats.removed).arg(m_lastStats.undetermined));
    deletePreview();
    m_batch.reset();
    m_limits = std::make_unique<LC_TrimLimitSet>();
    if (isKeepModifiedEntitiesSelected()) {
        setStatus(SelectLimit);
    }
    else {
        m_selectionComplete = false;
        setStatus(SelectTargets);
    }
}

void LC_ActionModifyTrimBatch::selectionFinishedByKey(QKeyEvent* event, const bool escape) {
    if (escape) {
        if (getStatus() == ChooseSide) {
            m_batch.reset();
            m_limits = std::make_unique<LC_TrimLimitSet>();
            setStatus(SelectLimit);
        }
        else if (getStatus() == SelectLimit) {
            m_selectionComplete = false;
            m_selectedEntities.clear();
            setStatus(SelectTargets);
        }
        else {
            LC_ActionPreSelectionAwareBase::selectionFinishedByKey(event, true);
        }
        return;
    }
    if (getStatus() == ChooseSide && m_batch != nullptr) {
        if (std::any_of(m_cursorSides.begin(), m_cursorSides.end(), [](int side) { return side != 0; })) trigger();
        return;
    }
    LC_ActionPreSelectionAwareBase::selectionFinishedByKey(event, false);
}

RS2::CursorType LC_ActionModifyTrimBatch::doGetMouseCursorSelected(const int status) {
    return status == ChooseSide ? RS2::CadCursor : RS2::CadCursor;
}

void LC_ActionModifyTrimBatch::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select entities to trim") + getSelectionCompletionHintMsg());
}

void LC_ActionModifyTrimBatch::updateActionPromptForSelected(const int status) {
    switch (status) {
    case SelectLimit:
        updatePromptTRBack(tr("Select limiting entity"));
        break;
    case ChooseSide:
        updatePromptTRBack(tr("Specify the side to keep"));
        break;
    default:
        updateActionPromptForSelection();
        break;
    }
}
