/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_PROPERTIESEDITINGWIDGET_OFFSETCURVE_H
#define LC_PROPERTIESEDITINGWIDGET_OFFSETCURVE_H

#include <functional>
#include <optional>
#include "lc_entitypropertieseditorwidget.h"
#include "lc_offsetcurve.h"

namespace Ui { class LC_OffsetCurvePropertiesEditingWidget; }

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

private:
    enum class Field { Distance, CenterX, CenterY, Major, Minor, Rotation, Start, End };
    void finishField(QLineEdit* editor, Field field) const;
    void applyValue(Field field, double value) const;
    void apply(const std::function<LC_OffsetCurveEditResult()>& setter) const;
    void refresh(QLineEdit* completed = nullptr, bool force = false) const;
    void inputRefused(QLineEdit* editor, const QString& reason) const;
    Ui::LC_OffsetCurvePropertiesEditingWidget* ui;
    LC_OffsetCurve* m_entity = nullptr;
    LC_OffsetRunLimits m_limits;
    mutable bool m_refreshing = false;
    mutable std::optional<LC_OffsetCurveEditResult> m_lastResult;
    mutable std::size_t m_attempts = 0;
};
#endif
