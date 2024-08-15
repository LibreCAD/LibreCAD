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

    RS_SETTINGS->beginGroup("/QuickInfoWidget");
    displayDistanceAndAngle = RS_SETTINGS->readNumEntry("/ShowDistanceAndAngle", 1) == 1;
    displayEntityBoundaries = RS_SETTINGS->readNumEntry("/ShowEntityBoundaries", 1) == 1;
    displayPointsPath = RS_SETTINGS->readNumEntry("/ShowPointsPathOnPreview", 1) == 1;
    displayPolylineDetailed = RS_SETTINGS->readNumEntry("/ShowPolylineDetails", 1) == 1;
    selectEntitiesInDefaultActionByCTRL = RS_SETTINGS->readNumEntry("/SelectEntityInDefaultAction", 1) == 1;
    autoSelectEntitiesInDefaultAction = RS_SETTINGS->readNumEntry("/AutoSelectEntityInDefaultAction",   1) == 1;

    RS_Color color = QColor(RS_SETTINGS->readEntry("/penHighlightColor", "red"));
    RS2::LineType lineType = static_cast<RS2::LineType> (RS_SETTINGS->readNumEntry("/penHighlightLineType", RS2::LineType::SolidLine));
    RS2::LineWidth lineWidth = static_cast<RS2::LineWidth> (RS_SETTINGS->readNumEntry("/penHighlightLineWidth", RS2::LineWidth::WidthDefault));

    highlightPen = RS_Pen(color, lineWidth, lineType);

    RS_SETTINGS->endGroup();

    pen = highlightPen;
}

void LC_QuickInfoOptions::save() const{
    RS_SETTINGS->beginGroup("/QuickInfoWidget");
    RS_SETTINGS->writeEntry("/ShowDistanceAndAngle", displayDistanceAndAngle ? 1 : 0);
    RS_SETTINGS->writeEntry("/ShowEntityBoundaries", displayEntityBoundaries ? 1 : 0);
    RS_SETTINGS->writeEntry("/ShowPointsPathOnPreview", displayPointsPath ? 1 : 0);
    RS_SETTINGS->writeEntry("/ShowPolylineDetails", displayPolylineDetailed ? 1 : 0);

    RS_SETTINGS->writeEntry("/SelectEntityInDefaultAction", selectEntitiesInDefaultActionByCTRL ? 1 : 0);
    RS_SETTINGS->writeEntry("/AutoSelectEntityInDefaultAction",  autoSelectEntitiesInDefaultAction? 1 : 0);
    RS_SETTINGS->writeEntry("/penHighlightColor", pen.getColor().name());
    RS_SETTINGS->writeEntry("/penHighlightLineType", pen.getLineType());
    RS_SETTINGS->writeEntry("/penHighlightLineWidth", pen.getWidth());
    RS_SETTINGS->endGroup();
}
