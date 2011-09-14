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

#include "rs_entitycontainer.h"
#include "rs_line.h"
#include "rs_polyline.h"
#include "rs_insert.h"
#include "rs_image.h"
#include "rs_block.h"


/**
 * Data needed to insert library items.
 */
struct RS_LibraryInsertData {
        QString file;
	RS_Vector insertionPoint;
	double factor;
	double angle;
};


/**
 * Class for the creation of new entities.
 * This class is bound to an entity container in which the
 * entities are stored. 
 *
 * @author Andrew Mustun
 */
class RS_Creation {
public:
    RS_Creation(RS_EntityContainer* container,
                RS_GraphicView* graphicView=NULL,
                bool handleUndo=true);

    RS_Entity* createParallelThrough(const RS_Vector& coord,
                              int number,
                              RS_Entity* e);

    RS_Entity* createParallel(const RS_Vector& coord,
                              double distance,
                              int number,
                              RS_Entity* e);

    RS_Line* createParallelLine(const RS_Vector& coord,
                                double distance, int number,
                                RS_Line* e);

    RS_Arc* createParallelArc(const RS_Vector& coord,
                              double distance, int number,
                              RS_Arc* e);

    RS_Circle* createParallelCircle(const RS_Vector& coord,
                                    double distance, int number,
                                    RS_Circle* e);

    RS_Line* createBisector(const RS_Vector& coord1,
                            const RS_Vector& coord2,
                            double length,
                            int num,
                            RS_Line* l1,
                            RS_Line* l2);

    RS_Line* createTangent1(const RS_Vector& coord,
                            const RS_Vector& point,
                            RS_Entity* circle);
/**
 * create a tangent line which is orthogonal to the given RS_Line(normal)
 */
    RS_Line* createLineOrthTan(const RS_Vector& coord,
                            RS_Line* normal,
                            RS_Entity* circle);
    RS_Line* createTangent2(const RS_Vector& coord,
                            RS_Entity* circle1,
                            RS_Entity* circle2);

    RS_Line* createLineRelAngle(const RS_Vector& coord,
                                RS_Entity* entity,
                                double angle,
                                double length);

    RS_Line* createPolygon(const RS_Vector& center,
                           const RS_Vector& corner,
                           int number);

    RS_Line* createPolygon2(const RS_Vector& corner1,
                            const RS_Vector& corner2,
                            int number);

    RS_Insert* createInsert(RS_InsertData& data);
	
    RS_Image* createImage(RS_ImageData& data);

    RS_Block* createBlock(const RS_BlockData& data,
                          const RS_Vector& referencePoint,
                          const bool remove);
						  
    RS_Insert* createLibraryInsert(RS_LibraryInsertData& data);

    //void createPoint(const RS_Vector& p);
    //void createLine2P(const RS_Vector& p1, const RS_Vector& p2);
    //void createRectangle(const RS_Vector& e1, const RS_Vector& e2);
    //RS_Polyline* createPolyline(const RS_Vector& p);

protected:
    RS_EntityContainer* container;
    RS_Graphic* graphic;
    RS_Document* document;
    RS_GraphicView* graphicView;
    bool handleUndo;
};

#endif
