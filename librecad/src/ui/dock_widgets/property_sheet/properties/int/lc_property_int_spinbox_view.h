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

#ifndef LC_PROPERTYINTSPINBOXVIEW_H
#define LC_PROPERTYINTSPINBOXVIEW_H

#include "lc_property_int.h"
#include "lc_property_view_typed.h"

class LC_PropertyIntSpinBoxView : public LC_PropertyViewTyped<LC_PropertyInt> {
    Q_DISABLE_COPY(LC_PropertyIntSpinBoxView)

public:
    static const QByteArray VIEW_NAME;
    static const QByteArray ATTR_MIN;
    static const QByteArray ATTR_MAX;
    static const QByteArray ATTR_STEP;
    static const QByteArray ATTR_SUFFIX;

    explicit LC_PropertyIntSpinBoxView(LC_PropertyInt& property);
    int getMinValue() const;
    int getMaxValue() const;
    int getCurrentValue() const;
    int getStepValue() const;

protected:
    bool doAcceptKeyPressedForInplaceEdit(QKeyEvent* keyEvent) const override;
    QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) override;
    void doApplyAttributes(const LC_PropertyViewDescriptor& attrs) override;
    bool doPropertyValueToStr(QString& strValue) const override;

private:
    QString m_suffix;
    QVariant m_min;
    QVariant m_max;
    QVariant m_step;
};

#endif
