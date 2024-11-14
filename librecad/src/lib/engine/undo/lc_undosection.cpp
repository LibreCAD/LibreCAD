/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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

#include "lc_undosection.h"
#include "rs_document.h"

LC_UndoSection::LC_UndoSection(RS_Document *doc, const bool handleUndo /*= true*/) :
    document( doc),
    valid( handleUndo && nullptr != doc)
{
    if (valid) {
        document->startUndoCycle();
    }
}

LC_UndoSection::~LC_UndoSection()
{
    if (valid) {
        document->endUndoCycle();
    }
}

void LC_UndoSection::addUndoable(RS_Undoable *undoable)
{
    if (valid) {
        document->addUndoable( undoable);
    }
}
