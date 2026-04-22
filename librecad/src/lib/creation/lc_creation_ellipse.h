/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#ifndef LC_CREATIONELLIPSE_H
#define LC_CREATIONELLIPSE_H

#include "rs_ellipse.h"

namespace LC_CreationEllipse {
    bool createEllipseFromCenter3Points(const RS_VectorSolutions& sol, RS_EllipseData &data);
    bool createEllipseFrom4P(const RS_VectorSolutions& sol, RS_EllipseData &data);
    //! \{ \brief from quadratic form
    /** : dn[0] x^2 + dn[1] xy + dn[2] y^2 =1 */
    bool createEllipseFromQuadratic(const std::vector<double>& dn, RS_EllipseData &data);
    /** : generic quadratic: A x^2 + C xy + B y^2 + D x + E y + F =0 */
    bool createEllipseFromQuadratic(const LC_Quadratic& q, RS_EllipseData &data);
    //! \}
    bool createEllipseInscribeQuadrilateral(const std::vector<RS_Line*>& lines,std::vector<RS_Vector> &tangent, RS_EllipseData &data);
}

#endif
