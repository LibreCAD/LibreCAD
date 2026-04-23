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

#ifndef LC_PROPERTYQSTRINGFILEVIEW_H
#define LC_PROPERTYQSTRINGFILEVIEW_H

#include "lc_property_qstring_invalid_view_base.h"

class LC_PropertyQStringFileView : public LC_PropertyQStringInvalidViewBase {
    Q_OBJECT Q_DISABLE_COPY(LC_PropertyQStringFileView)

public:
    static const QByteArray VIEW_NAME;
    static const QByteArray ATTR_OPTIONS;
    static const QByteArray ATTR_DEFAULT_DIR;
    static const QByteArray ATTR_VIEW_MODE;
    static const QByteArray ATTR_FILE_MODE;
    static const QByteArray ATTR_DEFAULT_SUFFIX;
    static const QByteArray ATTR_FILENAME_FILTER;
    static const QByteArray ATTR_FILENAME_FILTERS;
    static const QByteArray ATTR_SHOW_RELATIVE_PATH;
    static const QByteArray ATTR_ACCEPT_MODE;

    explicit LC_PropertyQStringFileView(LC_PropertyQString* property);

protected:
    void doApplyAttributes(const LC_PropertyViewDescriptor& info) override;
    bool doPropertyValueToStr(QString& strValue) const override;
    bool doGetToolTip(QString& strValue) override;
    QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) override;
    bool doCheckPropertyValid() const override;
    QString getDefaultDirectory() const;
    QString getAbsoluteFilePath() const;
    QString getRelativeFilePath() const;
    bool isShowRelativePath() const;

private:
    LC_PropertyViewDescriptor m_editorAttributes;
};
#endif
