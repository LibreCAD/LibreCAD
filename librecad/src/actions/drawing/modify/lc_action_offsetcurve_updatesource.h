/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_ACTIONOFFSETCURVEUPDATESOURCE_H
#define LC_ACTIONOFFSETCURVEUPDATESOURCE_H

#include <optional>
#include <utility>

#include "lc_actionpreselectionawarebase.h"
#include "lc_selectedsetlistener.h"
#include <QPointer>
#include "lc_offsetcurve.h"

class RS_Ellipse;

/** Explicit, value-only source replacement; no persistent source relationship. */
class LC_ActionOffsetCurveUpdateSource : public LC_ActionPreSelectionAwareBase, private LC_SelectedSetListener {
    Q_OBJECT
public:
    using EntityId = decltype(std::declval<const RS_Entity&>().getId());
    explicit LC_ActionOffsetCurveUpdateSource(LC_ActionContext* context);
    void init(int status) override;
    void trigger() override;

    struct Outcome {
        EntityId targetId = 0;
        std::optional<LC_OffsetCurveEditResult> edit;
        QString refusal;
    };
    struct Report {
        unsigned updated = 0;
        unsigned unchanged = 0;
        unsigned refused = 0;
        QList<Outcome> outcomes;
        QString sourceRefusal;
    };
    const Report& lastReport() const { return m_report; }

protected:
    void doInitWithContextEntity(RS_Entity* entity, const RS_Vector& position) override;
    void onSelectionCompleted(bool singleEntity, bool fromInit) override;
    bool isEntityAllowedToSelect(RS_Entity* entity) const override;
    bool isAllowTriggerOnEmptySelection() override { return false; }
    void onMouseLeftButtonReleaseSelected(int status, const LC_MouseEvent* event) override;
    void onMouseRightButtonReleaseSelected(int status, const LC_MouseEvent* event) override;
    void onMouseMoveEventSelected(int status, const LC_MouseEvent* event) override;
    void updateActionPromptForSelection() override;
    void updateActionPromptForSelected(int status) override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    bool doTriggerModifications(LC_DocumentModificationBatch& batch) override;
    void doTriggerSelectionUpdate(bool keepSelected, const LC_DocumentModificationBatch& batch) override;
    void doTriggerCompletion(bool success) override;

    void useSource(RS_Entity* source);
    LC_OffsetRunLimits m_limits{};

private:
    struct Target {
        RS_Entity* address = nullptr;
        EntityId id = 0;
    };
    void selectionChanged() override {}
    void selectedSetDestroyed() override;
    QPointer<RS_GraphicView> m_liveView;
    bool currentContext() const;
    RS_Entity* liveMember(const RS_Entity* address) const;
    QString targetRefusal(RS_Entity* entity) const;
    QString sourceRefusal(RS_Entity* entity) const;
    RS_Ellipse* sourceAt(const LC_MouseEvent* event) const;
    void publishReport();

    QList<Target> m_targets;
    RS_Entity* m_source = nullptr;
    EntityId m_sourceId = 0;
    Report m_report;
};

#endif
