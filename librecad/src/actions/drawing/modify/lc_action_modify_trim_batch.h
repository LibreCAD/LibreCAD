/*
 * This file is part of the LibreCAD project.
 * Copyright (C) 2026 LibreCAD.org
 * Licensed under GPL-2.0-or-later.
 */
#ifndef LC_ACTION_MODIFY_TRIM_BATCH_H
#define LC_ACTION_MODIFY_TRIM_BATCH_H

#include <memory>

#include "lc_actionpreselectionawarebase.h"
#include "lc_trimbatch.h"

class LC_ActionModifyTrimBatch : public LC_ActionPreSelectionAwareBase {
    Q_OBJECT
public:
    explicit LC_ActionModifyTrimBatch(LC_ActionContext* actionContext);
    ~LC_ActionModifyTrimBatch() override;
    void init(int status) override;
    void finish() override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector& coord) override;

protected:
    enum Status { SelectTargets = InitialActionStatus, SelectLimit, ChooseSide };
    std::unique_ptr<LC_TrimLimitSet> m_limits;
    std::unique_ptr<LC_TrimBatch> m_batch;
    std::vector<int> m_cursorSides;
    LC_TrimBatch::Stats m_lastStats;

    bool isEntityAllowedToSelect(RS_Entity* entity) const override;
    void onSelectionCompleted(bool singleEntity, bool fromInit) override;
    void selectionFinishedByKey(QKeyEvent* event, bool escape) override;
    bool isAllowSelectionFinishByEnterForEmptySelection() override;
    bool isAllowTriggerOnEmptySelection() override;
    void onMouseLeftButtonReleaseSelected(int status, const LC_MouseEvent* event) override;
    void onMouseRightButtonReleaseSelected(int status, const LC_MouseEvent* event) override;
    void onMouseMoveEventSelected(int status, const LC_MouseEvent* event) override;
    void updateActionPromptForSelection() override;
    void updateActionPromptForSelected(int status) override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    bool doTriggerModifications(LC_DocumentModificationBatch& context) override;
    void doTriggerCompletion(bool success) override;
    void doTriggerSelectionUpdate(bool keepSelected, const LC_DocumentModificationBatch& context) override;

    RS_Entity* catchLimitEntity(const LC_MouseEvent* event) const;
    void rebuildBatch();
    void previewForSide(const RS_Vector& point);
};

#endif
