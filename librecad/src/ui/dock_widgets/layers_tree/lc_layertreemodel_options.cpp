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

#include "lc_layertreeitem.h"
#include "lc_layertreemodel_options.h"
#include "rs_settings.h"

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
    LC_GROUP_GUARD("LayerTree");
    {
        LC_SET("activeLayerBgColor", activeLayerBgColor.name());
        LC_SET("selectedItemBgColor", selectedItemBgColor.name());
        LC_SET("virtualLayerBgColor", virtualLayerBgColor.name());
        LC_SET("gridColor", itemsGridColor.name());

        LC_SET("namingLayerSeparator", layerLevelSeparator);
        LC_SET("namingInfoSuffix", informationalLayerNameSuffix);
        LC_SET("namingDimSuffix", dimensionalLayerNameSuffix);
        LC_SET("namingAltSuffix", alternatePositionLayerNameSuffix);
        LC_SET("namingCopyPrefix", copiedNamePathPrefix);
        LC_SET("namingCopySuffix", copiedNamePathSuffix);

        LC_SET("hideLayerTypeIcons", hideLayerTypeIcons);
        LC_SET("dragDropEnabled", dragDropEnabled);
        LC_SET("showIndentedName", showIndentedName);
        LC_SET("showToolTips", showToolTips);
        LC_SET("renameSecondaryOnPrimary", renameSecondaryLayersOnPrimaryRename);
        LC_SET("indentSize", identSize);

        writePen("NormalLayer", defaultPenNormal);
        writePen("DimensionalLayer", defaultPenDimensional);
        writePen("InfoLayer", defaultPenInformational);
        writePen("AltPosLayer", defaultPenAlternatePosition);
    }
}

void LC_LayerTreeModelOptions::writePen(QString name, RS_Pen const &pen){
    LC_SET("pen" + name + "Color", pen.getColor().name());
    LC_SET("pen" + name + "LineType", pen.getLineType());
    LC_SET("pen" + name + "Width", pen.getWidth());
}

RS_Pen LC_LayerTreeModelOptions::readPen(QString name, RS_Pen &defaultPen){
    RS_Color color = QColor(LC_GET_STR("pen" + name + "Color", defaultPen.getColor().name()));
    // FIXME - well, that's a bit ugly - if there is a mess in setting value, cast from int to short will be illegal. Need additional validation there?
    RS2::LineType lineType = static_cast<RS2::LineType> (LC_GET_INT("pen" + name + "LineType", defaultPen.getLineType()));
    RS2::LineWidth lineWidth = static_cast<RS2::LineWidth> (LC_GET_INT("pen" + name + "Width", defaultPen.getWidth()));

    RS_Pen result = RS_Pen(color, lineWidth, lineType);
    return result;
}

void LC_LayerTreeModelOptions::load(){

    LC_LayerTreeModelOptions defaults;

    LC_GROUP_GUARD("LayerTree");
    {
        activeLayerBgColor = QColor(LC_GET_STR("activeLayerBgColor", defaults.activeLayerBgColor.name()));
        selectedItemBgColor = QColor(LC_GET_STR("selectedItemBgColor", defaults.selectedItemBgColor.name()));
        virtualLayerBgColor = QColor(LC_GET_STR("virtualLayerBgColor", defaults.virtualLayerBgColor.name()));
        itemsGridColor = QColor(LC_GET_STR("gridColor", defaults.itemsGridColor.name()));

        layerLevelSeparator = LC_GET_STR("namingLayerSeparator", defaults.layerLevelSeparator);
        informationalLayerNameSuffix = LC_GET_STR("namingInfoSuffix", defaults.informationalLayerNameSuffix);
        dimensionalLayerNameSuffix = LC_GET_STR("namingDimSuffix", defaults.dimensionalLayerNameSuffix);
        alternatePositionLayerNameSuffix = LC_GET_STR("namingAltSuffix", defaults.alternatePositionLayerNameSuffix);
        copiedNamePathPrefix = LC_GET_STR("namingCopyPrefix", defaults.copiedNamePathPrefix);
        copiedNamePathSuffix = LC_GET_STR("namingCopySuffix", defaults.copiedNamePathSuffix);

        hideLayerTypeIcons = LC_GET_BOOL("hideLayerTypeIcons", defaults.hideLayerTypeIcons);
        dragDropEnabled = LC_GET_BOOL("dragDropEnabled", defaults.dragDropEnabled);
        showIndentedName = LC_GET_BOOL("showIndentedName", defaults.showIndentedName);
        showToolTips = LC_GET_BOOL("showToolTips", defaults.showToolTips);
        renameSecondaryLayersOnPrimaryRename = LC_GET_BOOL("renameSecondaryOnPrimary", defaults.renameSecondaryLayersOnPrimaryRename);

        identSize = LC_GET_INT("indentSize", identSize);

        defaultPenNormal = readPen("NormalLayer", defaults.defaultPenNormal);
        defaultPenDimensional = readPen("DimensionalLayer", defaults.defaultPenDimensional);
        defaultPenInformational = readPen("InfoLayer", defaults.defaultPenInformational);
        defaultPenAlternatePosition = readPen("AltPosLayer", defaults.defaultPenAlternatePosition);
    }
}
