/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_ACTIONOFFSETCURVEUPDATESOURCE_H
#define LC_ACTIONOFFSETCURVEUPDATESOURCE_H

#include <optional>
#include <utility>
#include <vector>

#include "lc_actionpreselectionawarebase.h"
#include "lc_selectedsetlistener.h"
#include <QPointer>
#include "lc_offsetcurve.h"

class RS_Ellipse;
class LC_OffsetWorkerClient;
class QTimer;

/** Explicit, value-only source replacement; no persistent source relationship. */
class LC_ActionOffsetCurveUpdateSource : public LC_ActionPreSelectionAwareBase, private LC_SelectedSetListener {
    Q_OBJECT
public:
    using EntityId = decltype(std::declval<const RS_Entity&>().getId());
    explicit LC_ActionOffsetCurveUpdateSource(LC_ActionContext* context);
    ~LC_ActionOffsetCurveUpdateSource() override;
    void init(int status) override;
    void trigger() override;
    void finish() override;
    void suspend() override;
    void keyPressEvent(QKeyEvent* event) override;
    bool isPreparing() const { return bool(m_work); }

    struct Outcome {
        EntityId targetId = 0;
        std::optional<LC_OffsetCurveEditResult> edit;
        QString refusal;
        std::optional<LC_OffsetRunLimits> workerReservation;
        bool exactWorkerWorkKnown = false;
    };
    struct Report {
        unsigned updated = 0;
        unsigned unchanged = 0;
        unsigned refused = 0;
        QList<Outcome> outcomes;
        QString sourceRefusal;
    };
    /** Borrowed current work or frozen final report; copy values across event turns. */
    const Report& lastReport() const;

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
    struct PreparedRow;
    struct Work;
    void cancelPreparation(const QString& reason = {});
    void detachContext(bool documentDestroyed);
    void beginPreparation();
    void advance(const std::shared_ptr<Work>& work);
    void completePreparation(const std::shared_ptr<Work>& work, const QString& reason = {});
    bool currentWork(const std::shared_ptr<Work>& work) const;
    bool currentRow(const std::shared_ptr<Work>& work, std::size_t index) const;
    bool publishPrepared(LC_DocumentModificationBatch& batch, std::vector<PreparedRow>& staged,
        RS_Document* admittedDocument, std::uint64_t admittedEpoch, Report& report,
        const std::shared_ptr<Work>& work = {});
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
    std::unique_ptr<LC_OffsetWorkerClient> m_worker;
    std::unique_ptr<QTimer> m_deadline;
    std::shared_ptr<Work> m_work;
    qint64 m_serial = 0, m_requestId = 0;
};

#endif
