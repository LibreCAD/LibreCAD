/****************************************************************************
*
* Options for QuickInfo widget related functions

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/
#include "lc_quickinfowidgetoptions.h"
#include "rs_settings.h"

void LC_QuickInfoOptions::load(){
    RS_Pen highlightPen;

    LC_GROUP_GUARD("QuickInfoWidget");
    {
        displayDistanceAndAngle = LC_GET_BOOL("ShowDistanceAndAngle", true);
        displayEntityBoundaries = LC_GET_BOOL("ShowEntityBoundaries", true);
        displayPointsPath = LC_GET_BOOL("ShowPointsPathOnPreview", true);
        displayPolylineDetailed = LC_GET_BOOL("ShowPolylineDetails", true);
        selectEntitiesInDefaultActionByCTRL = LC_GET_BOOL("SelectEntityInDefaultAction", true);
        autoSelectEntitiesInDefaultAction = LC_GET_BOOL("AutoSelectEntityInDefaultAction", true);

        RS_Color color = QColor(LC_GET_STR("penHighlightColor", "red"));
        RS2::LineType lineType = static_cast<RS2::LineType> (LC_GET_INT("penHighlightLineType", RS2::LineType::SolidLine));
        RS2::LineWidth lineWidth = static_cast<RS2::LineWidth> (LC_GET_INT("penHighlightLineWidth", RS2::LineWidth::WidthDefault));
        highlightPen = RS_Pen(color, lineWidth, lineType);
    }

    pen = highlightPen;
}

void LC_QuickInfoOptions::save() const{
    LC_GROUP_GUARD("QuickInfoWidget");
    {
        LC_SET("ShowDistanceAndAngle", displayDistanceAndAngle);
        LC_SET("ShowEntityBoundaries", displayEntityBoundaries);
        LC_SET("ShowPointsPathOnPreview", displayPointsPath);
        LC_SET("ShowPolylineDetails", displayPolylineDetailed);
        LC_SET("SelectEntityInDefaultAction", selectEntitiesInDefaultActionByCTRL);
        LC_SET("AutoSelectEntityInDefaultAction", autoSelectEntitiesInDefaultAction);
        LC_SET("penHighlightColor", pen.getColor().name());
        LC_SET("penHighlightLineType", pen.getLineType());
        LC_SET("penHighlightLineWidth", pen.getWidth());
    }
}
