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

#ifndef LC_UNDOSECTION_H
#define LC_UNDOSECTION_H

class RS_Document;
class RS_Undoable;

/** \brief This class is a wrapper for RS_Undo methods
 *
 * It's important that calls to RS_Undo::startUndoCycle() and
 * RS_Undo::endUndoCycle() are balanced.
 * On instantiation the constructor calls startUndoCycle().
 * The class handles also validation of the RS_Document pointer.
 * When the instance is leaving the scope, the destructor
 * calls endUndoCycle().
 * This way the balance is guaranteed.
 * It simplifies undo handling specially in RS_Creation and RS_Modification classes
*/
class LC_UndoSection
{
public:
    LC_UndoSection(RS_Document * doc, const bool handleUndo = true);
    ~LC_UndoSection();

    void addUndoable(RS_Undoable * undoable);

private:
    RS_Document *document {nullptr};
    bool valid {true};
};

#endif // LC_UNDOSECTION_H
