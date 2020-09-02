/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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


#include "rs_document.h"
#include "rs_debug.h"


/**
 * Constructor.
 *
 * @param parent Parent of the document. Often that's NULL but
 *        for blocks it's the blocklist.
 */
RS_Document::RS_Document(RS_EntityContainer* parent)
        : RS_EntityContainer(parent), RS_Undo() {

    RS_DEBUG->print("RS_Document::RS_Document() ");

    filename = "";
    autosaveFilename = "Unnamed";
	formatType = RS2::FormatUnknown;
    setModified(false);
    RS_Color col(RS2::FlagByLayer);
    activePen = RS_Pen(col, RS2::WidthByLayer, RS2::LineByLayer);

    gv = NULL;//used to read/save current view
}

/**
 * Overwritten to set modified flag when undo cycle finished with undoable(s).
 */
void RS_Document::endUndoCycle()
{
    if (hasUndoable()) {
        setModified(true);
    }

    RS_Undo::endUndoCycle();
}

 void RS_Document::sendTelemetryData()
 {
	 if (!telemetryData.shxFontsConverted && !telemetryData.ttfFontsConverted && !telemetryData.fontConversionClicks
		 && !telemetryData.textShapingClicks && !telemetryData.trimExcessClicks)
		 return;
	 if (telemetryData.fontConversionClicks)
		 LC_TELEMETRY->AddProperty("Fonts Manually Converted", QString("%1").arg(telemetryData.fontConversionClicks));
	 if (telemetryData.shxFontsConverted)
		 LC_TELEMETRY->AddProperty("SHX Fonts Converted", QString("%1").arg(telemetryData.shxFontsConverted));
	 if (telemetryData.textShapingClicks)
		 LC_TELEMETRY->AddProperty("Shape Text Clicks", QString("%1").arg(telemetryData.textShapingClicks));
	 if (telemetryData.trimExcessClicks)
		 LC_TELEMETRY->AddProperty("Trim Excess Clicks", QString("%1").arg(telemetryData.trimExcessClicks));
	 if (telemetryData.ttfFontsConverted)
		 LC_TELEMETRY->AddProperty("TTF Fonts Converted", QString("%1").arg(telemetryData.ttfFontsConverted));
	 LC_TELEMETRY->TrackEvent("File Save");
	 LC_TELEMETRY->RemoveProperty("Fonts Manually Converted");
	 LC_TELEMETRY->RemoveProperty("SHX Fonts Converted");
	 LC_TELEMETRY->RemoveProperty("Shape Text Clicks");
	 LC_TELEMETRY->RemoveProperty("Trim Excess Clicks");
	 LC_TELEMETRY->RemoveProperty("TTF Fonts Converted");
	 telemetryData.fontConversionClicks = 0;
	 telemetryData.shxFontsConverted = 0;
	 telemetryData.textShapingClicks = 0;
	 telemetryData.trimExcessClicks = 0;
	 telemetryData.ttfFontsConverted = 0;
 }

