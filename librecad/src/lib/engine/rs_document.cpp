/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
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
#include "rs_pen.h"


/**
 * Constructor.
 *
 * @param parent Parent of the document. Often that's nullptr but
 *        for blocks it's the blocklist.
 */
RS_Document::RS_Document(RS_EntityContainer* parent)
        : RS_EntityContainer(parent), RS_Undo()
        ,activePen(new RS_Pen())
{
    RS_DEBUG->print("RS_Document::RS_Document() ");

    filename = "";
    autosaveFilename = "Unnamed";
	formatType = RS2::FormatUnknown;
    setModified(false);
    RS_Color col(RS2::FlagByLayer);
    *activePen = RS_Pen(col, RS2::WidthByLayer, RS2::LineByLayer);

    gv = nullptr;//used to read/save current view
}

RS_Document::RS_Document(RS_Document const& rhs):
    modified(rhs.modified)
  ,activePen(new RS_Pen(*rhs.activePen))
  ,filename(rhs.filename)
  ,autosaveFilename(rhs.autosaveFilename)
  ,formatType(rhs.formatType)
  ,gv(rhs.gv)
{}

RS_Document& RS_Document::operator = (RS_Document const& rhs)
{
    modified=rhs.modified;
    activePen.reset(new RS_Pen(*rhs.activePen));
    filename=rhs.filename;
    autosaveFilename=rhs.autosaveFilename;
    formatType=rhs.formatType;
    gv=rhs.gv;
    return *this;
}

RS_Document::~RS_Document(){}

RS_Pen RS_Document::getActivePen() const {
    return *activePen;
}

/**
 * Sets the currently active drawing pen to p.
 */
void RS_Document::setActivePen(RS_Pen const& p) {
    *activePen = p;
}

bool RS_Document::isDocument() const {
    return true;
}

/**
 * Removes an entity from the entiy container. Implementation
 * from RS_Undo.
 */
void RS_Document::removeUndoable(RS_Undoable* u) {
    if (u && u->undoRtti()==RS2::UndoableEntity) {
        removeEntity((RS_Entity*)u);
    }
}
