/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

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
 ******************************************************************************/

#ifndef LC_UNDOABLERELZERO_H
#define LC_UNDOABLERELZERO_H

#include "rs_undoable.h"
#include "rs_vector.h"
#include "rs_graphicview.h"

class LC_UndoableRelZero:public RS_Undoable
{
public:
    LC_UndoableRelZero(RS_GraphicView* view, const RS_Vector &mFrom, const RS_Vector &mTo);
    void undoStateChanged(bool undone) override;
protected:
    RS_GraphicView* graphicView = nullptr;
    RS_Vector m_From = RS_Vector(0,0,0);
    RS_Vector m_To = RS_Vector(0,0,0);
};

#endif // LC_UNDOABLERELZERO_H
