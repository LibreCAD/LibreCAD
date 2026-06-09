/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#ifndef LC_PROPERTYRECTVIEW_H
#define LC_PROPERTYRECTVIEW_H
#include "lc_property_rect.h"
#include "lc_property_view_typed_compound.h"

class LC_Formatter;

class LC_PropertyRectView : public LC_PropertyViewTypedCompound<LC_PropertyRect> {
    Q_OBJECT Q_DISABLE_COPY(LC_PropertyRectView)

public:
    static const QByteArray VIEW_NAME;
    explicit LC_PropertyRectView(LC_PropertyRect* property);
    static void formatMarginsToString(const LC_MarginsRect& value, const LC_Formatter* formatter, QString& strValue);
    void invalidateCached() override;
protected:
    // void doApplyAttributes(const LC_PropertyViewDescriptor& atts) override;
    QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) override;
    bool doPropertyValueToStrForView(QString& strValue) override;
    void marginsToString(const LC_MarginsRect& value, QString& strValue);
    QString m_cachedStrValue = "";
};

#endif
