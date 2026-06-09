/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 sand1024
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#include "lc_penitem.h"

#include "rs_pen.h"

LC_PenItem::LC_PenItem(QString name, const RS_Pen& pen):
    m_name{name}{
    setPen(pen);
}

LC_PenItem::LC_PenItem(const QString& name):
    m_name{name}{
    setPen(RS_Pen());
}

void LC_PenItem::setPen(const RS_Pen& pen){
    m_lineType = pen.getLineType();
    m_lineWidth = pen.getWidth();
    m_color = pen.getColor();
}

RS2::LineType LC_PenItem::getLineType() const {
    return m_lineType;
}

RS2::LineWidth LC_PenItem::getLineWidth() const {
    return m_lineWidth;
}

RS_Color LC_PenItem::getColor(){
    return m_color;
}

void LC_PenItem::setLineTypeIcon(const QIcon &icon){
    m_iconLineType = icon;
}

void LC_PenItem::setLineTypeName(const QString& typeName){
    m_lineTypeName = typeName;
}

void LC_PenItem::setLineWidthIcon(const QIcon &icon){
    m_iconLineWidth = icon;
}

void LC_PenItem::setLineWidthName(const QString& name){
    m_lineWidthName = name;
}

void LC_PenItem::setColorIcon(const QIcon &icon){
    m_iconColor = icon;
}

void LC_PenItem::setLineType(const RS2::LineType type){
   m_lineType = type;
}

void LC_PenItem::setLineWidth(const RS2::LineWidth width){
    m_lineWidth = width;
}

void LC_PenItem::setColor(const RS_Color& col){
    m_color = col;
}
