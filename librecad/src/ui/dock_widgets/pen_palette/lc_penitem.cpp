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

LC_PenItem::LC_PenItem(QString name, const RS_Pen& pen):
    name{name}
{
    setPen(pen);
}

LC_PenItem::LC_PenItem(QString name):
    name{name}
{
    setPen(RS_Pen());
}

void LC_PenItem::setPen(const RS_Pen& newPen){
    lineType = newPen.getLineType();
    lineWidth = newPen.getWidth();
    color = newPen.getColor();
}

RS2::LineType LC_PenItem::getLineType(){
    return lineType;
}

RS2::LineWidth LC_PenItem::getLineWidth(){
    return lineWidth;
}

RS_Color LC_PenItem::getColor(){
    return color;
}

void LC_PenItem::setLineTypeIcon(QIcon &icon){
    iconLineType = icon;
}

void LC_PenItem::setLineTypeName(QString typeName){
    lineTypeName = typeName;
}

void LC_PenItem::setLineWidthIcon(QIcon &icon){
    iconLineWidth = icon;
}

void LC_PenItem::setLineWidthName(QString wName){
    lineWidthName = wName;
}

void LC_PenItem::setColorIcon(QIcon &icon){
    iconColor = icon;
}

void LC_PenItem::setLineType(RS2::LineType type){
   lineType = type;
}

void LC_PenItem::setLineWidth(RS2::LineWidth width){
    lineWidth = width;
}

void LC_PenItem::setColor(const RS_Color& col){
    color = col;
}
