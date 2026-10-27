/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_PROPERTIESEDITINGWIDGET_OFFSETCURVE_H
#define LC_PROPERTIESEDITINGWIDGET_OFFSETCURVE_H

#include <functional>
#include <array>
#include <memory>
#include <optional>
#include "lc_entitypropertieseditorwidget.h"
#include "lc_offsetcurve.h"

namespace Ui { class LC_OffsetCurvePropertiesEditingWidget; }
class LC_OffsetWorkerClient;
class LC_OffsetCurveEditSnapshot;

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

private:
    friend class LC_DlgEntityProperties;
    void detachPreparation();
    enum class Field { Distance, CenterX, CenterY, Major, Minor, Rotation, Start, End };
    void finishField(QLineEdit* editor, Field field) const;
    void applyValue(Field field, double value) const;
    void apply(const LC_EllipseOffsetDefinition& definition) const;
    LC_EllipseOffsetDefinition proposedDefinition() const;
    void setBusy(bool busy) const;
    std::array<QLineEdit*, 8> definitionEditors() const;
    void restoreCommittedText() const;
    void refresh(QLineEdit* completed = nullptr, bool force = false) const;
    void inputRefused(QLineEdit* editor, const QString& reason) const;
    Ui::LC_OffsetCurvePropertiesEditingWidget* ui;
    LC_OffsetCurve* m_entity = nullptr;
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
