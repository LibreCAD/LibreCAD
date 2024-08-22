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

#ifndef LC_LAYERTREEMODEL_OPTIONS_H
#define LC_LAYERTREEMODEL_OPTIONS_H

#include <QColor>
#include <QString>
#include "rs_pen.h"


/**
 * Implementation of customization options used by LC_LayerTreeWidget
 */

struct LC_LayerTreeModelOptions{
public:
    // naming convention settings
    QString layerLevelSeparator{"-"};
    QString informationalLayerNameSuffix{"_meta"};
    QString dimensionalLayerNameSuffix{"+"};
    QString alternatePositionLayerNameSuffix{"_pos"};
    QString copiedNamePathSuffix {")"};
    QString copiedNamePathPrefix {"(Copy"};
    // display settings
    bool showIndentedName = {true};
    int identSize = {4};
    bool showToolTips{true};
    bool renameSecondaryLayersOnPrimaryRename {true};
    bool hideLayerTypeIcons{false};
    bool dragDropEnabled {true};
    // colors
    QColor matchedItemColor {QColor("blue")};
    QColor itemsGridColor {QColor(Qt::lightGray)};
    QColor virtualLayerBgColor {QColor( 245,245,245)};
    QColor selectedItemBgColor {QColor( 245,245,245)};
    QColor activeLayerBgColor {QColor( "white")};
    // default pens
    RS_Pen defaultPenNormal = RS_Pen(Qt::black, RS2::Width00,RS2::SolidLine);
    RS_Pen defaultPenDimensional = RS_Pen(Qt::blue, RS2::Width02,RS2::SolidLine);;
    RS_Pen defaultPenInformational = RS_Pen(Qt::magenta, RS2::Width02,RS2::SolidLine);;
    RS_Pen defaultPenAlternatePosition =  RS_Pen(Qt::cyan, RS2::Width00,RS2::SolidLine);;

    RS_Pen getDefaultPen(int layerType) const;
    void load();
    void save() const;

private:
    static void writePen(QString name, RS_Pen const &pen);
    static RS_Pen readPen(QString name, RS_Pen &defaultPen);
};
#endif //QG_LAYERTREEMODEL_OPTIONS_H
