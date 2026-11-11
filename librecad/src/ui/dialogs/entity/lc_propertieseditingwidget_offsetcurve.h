/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_PROPERTIESEDITINGWIDGET_OFFSETCURVE_H
#define LC_PROPERTIESEDITINGWIDGET_OFFSETCURVE_H

#include <functional>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <QCborArray>
#include "lc_entitypropertieseditorwidget.h"
#include "lc_offsetcurve.h"

namespace Ui { class LC_OffsetCurvePropertiesEditingWidget; }
class LC_OffsetWorkerClient;
class LC_OffsetCurveEditSnapshot;
class LC_OffsetSourceMutationWatch;
class RS_Ellipse;
class RS_Entity;
class RS_Document;
class QHideEvent;
class QShowEvent;
class QTimer;

/** Edits only the caller-owned dialog clone, never the document entity. */
class LC_PropertiesEditingWidgetOffsetCurve final : public LC_EntityPropertiesEditorWidget {
    Q_OBJECT
public:
    explicit LC_PropertiesEditingWidgetOffsetCurve(QWidget* parent = nullptr);
    ~LC_PropertiesEditingWidgetOffsetCurve() override;
    void setEntity(RS_Entity* entity) override;
    void setupInteractiveInputWidgets() override;
    void interactiveInputUpdate(LC_ActionContext::InteractiveInputInfo::InputType inputType,
        const QString& tag, double valueOne, double valueTwo) const override;
    void setEditLimits(const LC_OffsetRunLimits& limits) { m_limits = limits; }
    const std::optional<LC_OffsetCurveEditResult>& lastEditResult() const { return m_lastResult; }
    std::size_t editAttemptCount() const { return m_attempts; }
    const LC_OffsetCurve* editedEntity() const { return m_entity; }
    bool isPreparing() const { return m_busy; }
    void cancelPreparation() const;
    void finishPendingInput() const;
    void setPreparationGuard(std::function<bool()> guard) { m_preparationGuard = std::move(guard); }
    const std::optional<LC_OffsetCurveEditResult>& lastWorkerResult() const { return m_lastWorkerResult; }

signals:
    void preparationBusyChanged(bool busy) const;

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    friend class LC_DlgEntityProperties;
    void detachPreparation();
    enum class Field { Distance, CenterX, CenterY, Major, Minor, Rotation, Start, End };
    struct SourceWitness { qulonglong id; QCborArray geometry; };
    void finishField(QLineEdit* editor, Field field) const;
    void applyValue(Field field, double value) const;
    void apply(const LC_EllipseOffsetDefinition& definition,
               std::optional<SourceWitness> source = std::nullopt) const;
    void applySourceDefinition(const LC_OffsetSourceDefinition& definition,
                               std::optional<SourceWitness> source = std::nullopt) const;
    LC_EllipseOffsetDefinition proposedDefinition() const;
    void setBusy(bool busy) const;
    std::array<QLineEdit*, 8> definitionEditors() const;
    void restoreCommittedText() const;
    void refresh(QLineEdit* completed = nullptr, bool force = false,
                 bool refreshSources = true) const;
    void inputRefused(QLineEdit* editor, const QString& reason) const;
    void refreshSourceChoices(bool preserveWatch = false,
                              const RS_Entity* knownMatch = nullptr,
                              bool highlightAlreadyResolved = false) const;
    void refreshSourceHighlight(bool preserveWatch = false, const RS_Entity* knownMatch = nullptr,
                                bool highlightAlreadyResolved = false) const;
    void checkSourceHighlight() const;
    void beginSourceHighlightScan(bool clearExisting);
    void processSourceHighlightScanSlice();
    void invalidateSourceHighlightScan();
    void clearSourceWatch() const;
    void completeSourceHighlightScan();
    RS_Entity* findSourceEntity(qulonglong id) const;
    bool sourceMatches(const SourceWitness& source) const;
    void selectCurrentSource() const;
    Ui::LC_OffsetCurvePropertiesEditingWidget* ui;
    LC_OffsetCurve* m_entity = nullptr;
    mutable qulonglong m_preferredSourceId = 0;
    mutable std::shared_ptr<void> m_sourceHighlight;
    mutable std::weak_ptr<void> m_highlightLifetime;
    mutable LC_GraphicViewport* m_highlightViewport = nullptr;
    mutable qulonglong m_highlightSourceId = 0;
    mutable LC_OffsetSourceGeometry::Kind m_highlightKind = LC_OffsetSourceGeometry::Kind::Invalid;
    mutable QCborArray m_highlightGeometry;
    mutable bool m_sourceCanSelect = false;
    QTimer* m_highlightTimer = nullptr;
    QTimer* m_highlightSliceTimer = nullptr;
    std::unique_ptr<LC_OffsetSourceMutationWatch> m_sourceMutationWatch;
    mutable std::uint64_t m_watchMutationRevision = 0;
    std::uint64_t m_scanMutationRevision = 0;
    std::uint64_t m_scanEntityListRevision = 0;
    std::uint64_t m_scanUndoEpoch = 0;
    std::size_t m_scanEntityCount = 0;
    std::size_t m_scanIndex = 0;
    std::size_t m_scanMatchCount = 0;
    std::size_t m_scanCandidateIndex = 0;
    std::size_t m_scanPreferredIndex = 0;
    qulonglong m_scanCandidateId = 0;
    qulonglong m_scanPreferredId = 0;
    qulonglong m_scanPreviousSourceId = 0;
    bool m_scanHasCandidate = false;
    bool m_scanHasPreferred = false;
    bool m_scanPreviousHighlightVisible = false;
    mutable RS_Document* m_scanDocument = nullptr;
    mutable LC_GraphicViewport* m_scanViewport = nullptr;
    LC_OffsetSourceGeometry::Kind m_scanKind = LC_OffsetSourceGeometry::Kind::Invalid;
    QCborArray m_scanGeometry;
    mutable bool m_highlightScanActive = false;
    bool m_scanRestartQueued = false;
    bool m_sourceHighlightVisible = false;
    LC_OffsetRunLimits m_limits;
    mutable bool m_refreshing = false;
    mutable std::optional<LC_OffsetCurveEditResult> m_lastResult;
    mutable std::size_t m_attempts = 0;
    std::unique_ptr<LC_OffsetWorkerClient> m_worker;
    mutable qint64 m_generation = 0;
    mutable qint64 m_sourceRevision = 1;
    mutable bool m_busy = false;
    mutable std::optional<LC_EllipseOffsetDefinition> m_proposedDefinition;
    mutable std::shared_ptr<LC_OffsetCurveEditSnapshot> m_inputSnapshot;
    mutable std::optional<LC_OffsetCurveEditResult> m_lastWorkerResult;
    std::function<bool()> m_preparationGuard;
    mutable std::array<QString, 8> m_committedText;
    mutable bool m_committedSide = false, m_committedReversed = false;
};
#endif
