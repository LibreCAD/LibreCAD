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

#ifndef LC_ALIGN_H
#define LC_ALIGN_H

#include "rs_vector.h"

class RS_Entity;

namespace LC_Align
{
    enum Align {
        NONE, LEFT_TOP, MIDDLE, RIGHT_BOTTOM
    };

    enum AlignMode{
        ENTITY, POSITION, DRAWING
    };

    RS_Vector getReferencePoint(const RS_Vector &min, const RS_Vector &max, int horizontalAlign, int verticalAlign);
    RS_Entity* createCloneMovedToOffset(const RS_Entity *e, const RS_Vector &offset, bool updateAttributes);
    RS_Entity* createCloneMovedToTarget(const RS_Entity *e, const RS_Vector &targetPoint, bool updateAttributes, int horizontalAlign, int verticalAlign);
    void collectSelectionBounds( std::vector<RS_Entity*> selectedEntities, RS_Vector &boxMin, RS_Vector &boxMax);
    bool getVerticalRefCoordinate(const RS_Vector &min, const RS_Vector &max, int verticalAlign, double &refCoordinate);
    bool getHorizontalRefCoordinate(const RS_Vector &min, const RS_Vector &max, int horizontalAlign, double &refCoordinate);
};

#endif // LC_ALIGN_H
