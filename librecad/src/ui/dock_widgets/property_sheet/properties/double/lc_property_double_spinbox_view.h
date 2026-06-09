/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_PROPERTYDOUBLESPINBOXVIEW_H
#define LC_PROPERTYDOUBLESPINBOXVIEW_H

#include "lc_property_double.h"
#include "lc_property_view_typed.h"

class LC_PropertyDoubleSpinBoxView : public LC_PropertyViewTyped<LC_PropertyDouble> {
    Q_DISABLE_COPY(LC_PropertyDoubleSpinBoxView)

public:
    static const QByteArray VIEW_NAME;
    static const QByteArray ATTR_MIN;
    static const QByteArray ATTR_MAX;
    static const QByteArray ATTR_STEP;
    static const QByteArray ATTR_SUFFIX;
    static const QByteArray ATTR_PRECISION;
    static const QByteArray ATTR_MULTIPLIER;

    explicit LC_PropertyDoubleSpinBoxView(LC_PropertyDouble* property);
    double getStepValue() const;
    double getMinValue() const;
    double getMaxValue() const;
    double getMultiplier() const;
    double getCurrentValue() const;

protected:
    QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) override;
    bool doAcceptKeyPressedForInplaceEdit(QKeyEvent* keyEvent) const override;
    bool doPropertyValueToStr(QString& strValue) const override;
    void doApplyAttributes(const LC_PropertyViewDescriptor& attrs) override;

private:
    QString m_suffix;
    double m_multiplier;
    int m_precision;
    QVariant m_min;
    QVariant m_max;
    QVariant m_step;
};
#endif
