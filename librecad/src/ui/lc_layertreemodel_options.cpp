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

#include "lc_layertreemodel_options.h"
#include <rs_settings.h>
#include <lc_layertreeitem.h>

/**
 * Returns pen that will be used as default for creation of the layer with given type
 * @param layerType type of layer
 * @return default pen based on settings for the layer type
 */
RS_Pen LC_LayerTreeModelOptions::getDefaultPen(int layerType) const{
    RS_Pen result;
    switch (layerType){
        case LC_LayerTreeItem::NORMAL:
            return defaultPenNormal;
        case LC_LayerTreeItem::DIMENSIONAL:
            return defaultPenDimensional;
        case LC_LayerTreeItem::INFORMATIONAL:
            return defaultPenInformational;
        case LC_LayerTreeItem::ALTERNATE_POSITION:
            return defaultPenAlternatePosition;
        default:
            // in general, this function should not be called for other types of layers...
            // TODO - throw error there?
            break;
    }
    return result;
}

void LC_LayerTreeModelOptions::save() const{
    RS_SETTINGS->beginGroup("LayerTree");

    RS_SETTINGS->writeEntry("/activeLayerBgColor", activeLayerBgColor.name());
    RS_SETTINGS->writeEntry("/selectedItemBgColor", selectedItemBgColor.name());
    RS_SETTINGS->writeEntry("/virtualLayerBgColor", virtualLayerBgColor.name());
    RS_SETTINGS->writeEntry("/gridColor", itemsGridColor.name());

    RS_SETTINGS->writeEntry("/namingLayerSeparator", layerLevelSeparator);
    RS_SETTINGS->writeEntry("/namingInfoSuffix", informationalLayerNameSuffix);
    RS_SETTINGS->writeEntry("/namingDimSuffix", dimensionalLayerNameSuffix);
    RS_SETTINGS->writeEntry("/namingAltSuffix", alternatePositionLayerNameSuffix);
    RS_SETTINGS->writeEntry("/namingCopyPrefix", copiedNamePathPrefix);
    RS_SETTINGS->writeEntry("/namingCopySuffix", copiedNamePathSuffix);

    RS_SETTINGS->writeEntry("/hideLayerTypeIcons", hideLayerTypeIcons ? 1: 0);
    RS_SETTINGS->writeEntry("/dragDropEnabled", dragDropEnabled ? 1: 0);
    RS_SETTINGS->writeEntry("/showIndentedName", showIndentedName ? 1: 0);
    RS_SETTINGS->writeEntry("/showToolTips", showToolTips ? 1: 0);
    RS_SETTINGS->writeEntry("/renameSecondaryOnPrimary", renameSecondaryLayersOnPrimaryRename ? 1: 0);
    RS_SETTINGS->writeEntry("/indentSize", identSize);

    writePen("NormalLayer", defaultPenNormal);
    writePen("DimensionalLayer", defaultPenDimensional);
    writePen("InfoLayer", defaultPenInformational);
    writePen("AltPosLayer", defaultPenAlternatePosition);
    RS_SETTINGS->endGroup();
}

void LC_LayerTreeModelOptions::writePen(QString name, RS_Pen const &pen){
    RS_SETTINGS->writeEntry("/pen" + name + "Color", pen.getColor().name());
    RS_SETTINGS->writeEntry("/pen" + name + "LineType", pen.getLineType());
    RS_SETTINGS->writeEntry("/pen" + name + "Width", pen.getWidth());
}

RS_Pen LC_LayerTreeModelOptions::readPen(QString name, RS_Pen &defaultPen){
    RS_Color color = QColor(RS_SETTINGS->readEntry("/pen" + name + "Color", defaultPen.getColor().name()));
    // FIXME - well, that's a bit ugly - if there is a mess in setting value, cast from int to short will be illegal. Need additional validation there?
    RS2::LineType lineType = static_cast<RS2::LineType> (RS_SETTINGS->readNumEntry("/pen" + name + "LineType", defaultPen.getLineType()));
    RS2::LineWidth lineWidth = static_cast<RS2::LineWidth> (RS_SETTINGS->readNumEntry("/pen" + name + "Width", defaultPen.getWidth()));

    RS_Pen result = RS_Pen(color, lineWidth, lineType);
    return result;
}

void LC_LayerTreeModelOptions::load(){

    LC_LayerTreeModelOptions defaults;

    RS_SETTINGS->beginGroup("LayerTree");

    activeLayerBgColor = QColor(RS_SETTINGS->readEntry("/activeLayerBgColor", defaults.activeLayerBgColor.name()));
    selectedItemBgColor = QColor(RS_SETTINGS->readEntry("/selectedItemBgColor", defaults.selectedItemBgColor.name()));
    virtualLayerBgColor =  QColor(RS_SETTINGS->readEntry("/virtualLayerBgColor", defaults.virtualLayerBgColor.name()));
    itemsGridColor =  QColor(RS_SETTINGS->readEntry("/gridColor", defaults.itemsGridColor.name()));

    layerLevelSeparator = RS_SETTINGS->readEntry("/namingLayerSeparator", defaults.layerLevelSeparator);
    informationalLayerNameSuffix = RS_SETTINGS->readEntry("/namingInfoSuffix", defaults.informationalLayerNameSuffix);
    dimensionalLayerNameSuffix = RS_SETTINGS->readEntry("/namingDimSuffix", defaults.dimensionalLayerNameSuffix);
    alternatePositionLayerNameSuffix = RS_SETTINGS->readEntry("/namingAltSuffix", defaults.alternatePositionLayerNameSuffix);
    copiedNamePathPrefix = RS_SETTINGS->readEntry("/namingCopyPrefix", defaults.copiedNamePathPrefix);
    copiedNamePathSuffix = RS_SETTINGS->readEntry("/namingCopySuffix", defaults.copiedNamePathSuffix);

    hideLayerTypeIcons= RS_SETTINGS->readNumEntry("/hideLayerTypeIcons", defaults.hideLayerTypeIcons ? 1: 0) == 1;
    dragDropEnabled = RS_SETTINGS->readNumEntry("/dragDropEnabled", defaults.dragDropEnabled ? 1: 0) == 1;
    showIndentedName = RS_SETTINGS->readNumEntry("/showIndentedName", defaults.showIndentedName ? 1: 0) == 1;
    showToolTips = RS_SETTINGS->readNumEntry("/showToolTips", defaults.showToolTips ? 1: 0) == 1;
    renameSecondaryLayersOnPrimaryRename = RS_SETTINGS->readNumEntry("/renameSecondaryOnPrimary", defaults.renameSecondaryLayersOnPrimaryRename ? 1: 0) == 1;

    identSize=RS_SETTINGS->readNumEntry("/indentSize", identSize);

    defaultPenNormal = readPen("NormalLayer", defaults.defaultPenNormal);
    defaultPenDimensional = readPen("DimensionalLayer", defaults.defaultPenDimensional);
    defaultPenInformational = readPen("InfoLayer", defaults.defaultPenInformational);
    defaultPenAlternatePosition = readPen("AltPosLayer", defaults.defaultPenAlternatePosition);

    RS_SETTINGS->endGroup();
}
