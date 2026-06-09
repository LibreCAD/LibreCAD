/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_CopyUtils_H
#define LC_CopyUtils_H

#include <QList>

#include "rs_vector.h"

struct LC_DocumentModificationBatch;
class RS_Graphic;
class RS_Entity;

namespace LC_CopyUtils {
    /**
    * Holds the data needed for pasting.
    */
    struct RS_PasteData {
        RS_PasteData(const RS_Vector& insertionPoint, double factor, double angle);
        explicit RS_PasteData(const RS_Vector& point):insertionPoint{point}{}
        //! Insertion point.
        RS_Vector insertionPoint;
        //! Scale factor.
        double factor = 1.;
        //! Rotation angle.
        double angle = 0.;
    };
    RS_Vector getInterGraphicsScaleFactor(double userFactor, const RS_Graphic *source, const RS_Graphic* destination);
    void copy(const RS_Vector& ref, QList<RS_Entity*>& entities, const RS_Graphic* graphic);
    void paste(const RS_PasteData& data, RS_Graphic* graphic, LC_DocumentModificationBatch& ctx);
}

#endif
