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

#include "lc_layertreeitem.h"
#include "rs_settings.h"

/**
 * Returns pen that will be used as default for creation of the layer with given type
 * @param layerType type of layer
 * @return default pen based on settings for the layer type
 */
RS_Pen LC_LayerTreeModelOptions::getDefaultPen(const int layerType) const{
    RS_Pen result;
    switch (layerType){
        case RS_Layer::LayerType::NORMAL:
            return defaultPenNormal;
        case RS_Layer::LayerType::DIMENSIONAL:
            return defaultPenDimensional;
        case RS_Layer::LayerType::INFORMATIONAL:
            return defaultPenInformational;
        case RS_Layer::LayerType::ALTERNATE_POSITION:
            return defaultPenAlternatePosition;
        default:
            // in general, this function should not be called for other types of layers...
            // TODO - throw error there?
            break;
    }
    return result;
}

void LC_LayerTreeModelOptions::save() const{
    LC_GROUP_GUARD("Widget.LayerTree");
    {
        LC_SET("activeLayerBgColor", activeLayerBgColor.name());
        LC_SET("selectedItemBgColor", selectedItemBgColor.name());
        LC_SET("virtualLayerBgColor", virtualLayerBgColor.name());
        LC_SET("showGrid", showGrid);

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

        RS_Settings::writePen("NormalLayer", defaultPenNormal);
        RS_Settings::writePen("DimensionalLayer", defaultPenDimensional);
        RS_Settings::writePen("InfoLayer", defaultPenInformational);
        RS_Settings::writePen("AltPosLayer", defaultPenAlternatePosition);
    }
}

void LC_LayerTreeModelOptions::load(){
    LC_GROUP_GUARD("Widget.LayerTree");
    {
        const LC_LayerTreeModelOptions defaults;
        activeLayerBgColor = QColor(LC_GET_STR("activeLayerBgColor", defaults.activeLayerBgColor.name()));
        selectedItemBgColor = QColor(LC_GET_STR("selectedItemBgColor", defaults.selectedItemBgColor.name()));
        virtualLayerBgColor = QColor(LC_GET_STR("virtualLayerBgColor", defaults.virtualLayerBgColor.name()));
        showGrid = LC_GET_BOOL("showGrid", true);

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

        defaultPenNormal = RS_Settings::readPen("NormalLayer", defaults.defaultPenNormal);
        defaultPenDimensional = RS_Settings::readPen("DimensionalLayer", defaults.defaultPenDimensional);
        defaultPenInformational = RS_Settings::readPen("InfoLayer", defaults.defaultPenInformational);
        defaultPenAlternatePosition = RS_Settings::readPen("AltPosLayer", defaults.defaultPenAlternatePosition);
    }
}
