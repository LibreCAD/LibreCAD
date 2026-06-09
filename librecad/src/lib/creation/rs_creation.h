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

#ifndef RS_CREATION_H
#define RS_CREATION_H

#include <QString>
#include <memory>

#include "rs_ellipse.h"
#include "rs_vector.h"

class RS_Document;
class RS_EntityContainer;
class RS_GraphicView;
class RS_Graphic;
class RS_Entity;
class RS_Arc;
class RS_Circle;
class RS_Ellipse;
class RS_Line;
class LC_SplinePoints;
struct RS_BlockData;
struct RS_ImageData;
class RS_Image;
struct RS_InsertData;
class RS_Insert;
class RS_Block;
class QString;
class LC_GraphicViewport;

/**
 * Namespace for the creation of new entities.
 *
 * @author Andrew Mustun
 */
namespace RS_Creation {
    void createParallelThrough(const RS_Vector& coord, int number, RS_Entity* e, bool symmetric, bool distributeWithin, QList<RS_Entity*>& createdEntities);
    void createParallel(const RS_Vector& coord, double distance, int number, RS_Entity* e, bool symmetric, QList<RS_Entity*>& createdEntities);
    void createParallelLine(const RS_Vector& coord, double distance, int number, const RS_Line* e, bool symmetric, QList<RS_Entity*>& createdEntities);
    void createParallelArc(const RS_Vector& coord, double distance, int number, RS_Arc* e, QList<RS_Entity*>& createdEntities);
    void createParallelCircle(const RS_Vector& coord, double distance, int number, const RS_Circle* e, QList<RS_Entity*>& createdEntities);
    void createParallelSplinePoints(const RS_Vector& coord, double distance, int number, const LC_SplinePoints* e, QList<RS_Entity*>& createdEntities);
    bool createBisector(const RS_Vector& coord1, const RS_Vector& coord2, double length, int num, const RS_Line* l1, const RS_Line* l2, QList<RS_Entity*>& createdLines);
    RS_Line* createTangent1(const RS_Vector& coord, const RS_Vector& point, const RS_Entity* circle, RS_Vector& tangentPoint, RS_Vector& altTangentPoint);
    /**
     * create a tangent line which is orthogonal to the given RS_Line(normal)
     */
    std::unique_ptr<RS_Line> createLineOrthTan(const RS_Vector& coord, const RS_Line* normal, RS_Entity* circle, RS_Vector& alternativeTangent);
    std::vector<std::unique_ptr<RS_Line>> createTangent2(const RS_Entity* circle1, const RS_Entity* circle2);
    RS_Line* createLineRelAngle(const RS_Vector& coord, const RS_Entity* entity, double angle, double length);
    RS_Block* createBlock(const RS_BlockData* data, const RS_Vector& referencePoint, const QList<RS_Entity*>& selectedEntities);
}

#endif
