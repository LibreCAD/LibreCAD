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

#ifndef LC_PROPERTYCOMBOBOX_H
#define LC_PROPERTYCOMBOBOX_H

#include <QComboBox>

#include "lc_property_view.h"

class LC_PropertyComboBox : public QComboBox {
public:
    explicit LC_PropertyComboBox(LC_PropertyView* view, QWidget* parent = Q_NULLPTR);
    inline LC_PropertyView* getPropertyView() const;
    inline LC_Property* getProperty() const;
    inline LC_Property* getStateProperty() const;
    void disablePaint(bool v) {m_paintDisabled = v;}
protected:
    virtual void doCustomPaint(QPainter& painter, const QRect& rect);
private:
    void paintEvent(QPaintEvent* event) override;
    LC_PropertyView* m_view;
    bool m_paintDisabled = false;
};

#endif
