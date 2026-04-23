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

#ifndef PROPERTY_QPOINT_VIEW_H
#define PROPERTY_QPOINT_VIEW_H

#include "lc_property_rsvector.h"
#include "lc_property_view_typed_compound.h"

class LC_PropertyRSVectorView : public LC_PropertyViewTypedCompound<LC_PropertyRSVector> {
    Q_OBJECT Q_DISABLE_COPY(LC_PropertyRSVectorView)

public:
    static const QByteArray VIEW_NAME;
    static const QByteArray ATTR_SUFFIX;
    static const QByteArray ATTR_X_DISPLAY_NAME;
    static const QByteArray ATTR_X_DESCRIPTION;
    static const QByteArray ATTR_Y_DISPLAY_NAME;
    static const QByteArray ATTR_Y_DESCRIPTION;
    static const QByteArray ATTR_FORMAT_AS_INT;

    explicit LC_PropertyRSVectorView(LC_PropertyRSVector* property);
    static void formatVectorToString(const RS_Vector& value, const QString& suffix, const LC_Formatter* formatter, QString& strValue, bool asInt);

    void invalidateCached() override;

    bool isFormatAsInt() const {return m_formatAsInt;}

protected:
    void doApplyAttributes(const LC_PropertyViewDescriptor& atts) override;
    QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) override;
    QString m_cachedStrValue = "";
    bool doPropertyValueToStrForView(QString& strValue) override;
    void vectorToString(const RS_Vector& value, const QString& suffix, QString& strValue);

private:
    QString m_suffix;
    bool m_formatAsInt {false};
};

#endif
