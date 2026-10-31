/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
/** Restore embedded source values without a persistent source relationship. */
#ifndef LC_ACTIONOFFSETCURVERESTOREORIGINAL_H
#define LC_ACTIONOFFSETCURVERESTOREORIGINAL_H

#include <cstddef>

#include "lc_actionpreselectionawarebase.h"
#include "lc_selectedsetlistener.h"
#include <QPointer>

class LC_ActionOffsetCurveRestoreOriginal : public LC_ActionPreSelectionAwareBase, private LC_SelectedSetListener {
    Q_OBJECT
public:
    explicit LC_ActionOffsetCurveRestoreOriginal(LC_ActionContext* context);
    void init(int status) override;
    void trigger() override;

    struct Outcome {
        unsigned long long targetId = 0;
        QString refusal;
        bool restored = false;
        bool alreadyPresent = false;
    };
    struct Report {
        unsigned restored = 0;
        unsigned alreadyPresent = 0;
        unsigned refused = 0;
        QList<Outcome> outcomes;
    };
    const Report& lastReport() const { return m_report; }

protected:
    void doInitWithContextEntity(RS_Entity* entity, const RS_Vector& position) override;
    void onSelectionCompleted(bool singleEntity, bool fromInit) override;
    bool isEntityAllowedToSelect(RS_Entity* entity) const override;
    bool isAllowTriggerOnEmptySelection() override { return false; }
    bool doTriggerModifications(LC_DocumentModificationBatch& batch) override;
    void doTriggerSelectionUpdate(bool keepSelected, const LC_DocumentModificationBatch& batch) override;
    void doTriggerCompletion(bool success) override;
    void updateActionPromptForSelection() override;
    std::size_t m_comparisonLimit = std::size_t{1} << 20;

private:
    struct Target {
        RS_Entity* address = nullptr;
        unsigned long long id = 0;
    };
    void selectionChanged() override {}
    void selectedSetDestroyed() override;
    QPointer<RS_GraphicView> m_liveView;
    bool currentContext() const;
    RS_Entity* liveMember(const RS_Entity* address) const;
    void publishReport();

    QList<Target> m_targets;
    Report m_report;
};

#endif
