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

