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

#include "rs_selection.h"

#include "rs_line.h"
#include "rs_information.h"
#include "rs_polyline.h"
#include "rs_entity.h"
#include "rs_graphic.h"
#include "rs_layer.h"



/**
 * Default constructor.
 *
 * @param container The container to which we will add
 *        entities. Usually that's an RS_Graphic entity but
 *        it can also be a polyline, text, ...
 */
RS_Selection::RS_Selection(RS_EntityContainer& container,
                           RS_GraphicView* graphicView) {
    this->container = &container;
    this->graphicView = graphicView;
    graphic = container.getGraphic();
}



/**
 * Selects or deselects the given entity.
 */
void RS_Selection::selectSingle(RS_Entity* e) {
	if (e && (! (e->getLayer() && e->getLayer()->isLocked()))) {

        if (graphicView) {
            graphicView->deleteEntity(e);
        }

       	e->toggleSelected();

        if (graphicView) {
            graphicView->drawEntity(e);
        }
    }
}



/**
 * Selects all entities on visible layers.
 */
void RS_Selection::selectAll(bool select) {
    if (graphicView) {
        //graphicView->deleteEntity(container);
    }

	//container->setSelected(select);
	for(auto e: *container){
    //for (unsigned i=0; i<container->count(); ++i) {
        //RS_Entity* e = container->entityAt(i);

        if (e && e->isVisible()) {
            e->setSelected(select);
        }
    }

	if (graphicView) {
        //graphicView->drawEntity(container);
		graphicView->redraw();
    }
}



/**
 * Selects all entities on visible layers.
 */
void RS_Selection::invertSelection() {
    if (graphicView) {
        //graphicView->deleteEntity(container);
    }

	for(auto e: *container){
    //for (unsigned i=0; i<container->count(); ++i) {
        //RS_Entity* e = container->entityAt(i);

        if (e && e->isVisible()) {
            e->toggleSelected();
        }
    }

    if (graphicView) {
        //graphicView->drawEntity(container);
		graphicView->redraw();
    }
}



/**
 * Selects all entities that are completely in the given window.
 *
 * @param v1 First corner of the window to select.
 * @param v2 Second corner of the window to select.
 * @param select true: select, false: deselect
 */
void RS_Selection::selectWindow(const RS_Vector& v1, const RS_Vector& v2,
                                bool select, bool cross) {

    container->selectWindow(v1, v2, select, cross);

    if (graphicView) {
		graphicView->redraw();
    }
}



/**
 * Selects all entities that are intersected by the given line.
 *
 * @param v1 Startpoint of line.
 * @param v2 Endpoint of line.
 * @param select true: select, false: deselect
 */
void RS_Selection::selectIntersected(const RS_Vector& v1, const RS_Vector& v2,
                                     bool select) {

	RS_Line line{v1, v2};
    bool inters;

	for(auto e: *container){
    //for (unsigned i=0; i<container->count(); ++i) {
        //RS_Entity* e = container->entityAt(i);

        if (e && e->isVisible()) {

            inters = false;

            // select containers / groups:
            if (e->isContainer()) {
                RS_EntityContainer* ec = (RS_EntityContainer*)e;

                for (RS_Entity* e2=ec->firstEntity(RS2::ResolveAll); e2;
                        e2=ec->nextEntity(RS2::ResolveAll)) {

                    RS_VectorSolutions sol =
                        RS_Information::getIntersection(&line, e2, true);

                    if (sol.hasValid()) {
                        inters = true;
                    }
                }
            } else {

                RS_VectorSolutions sol =
                    RS_Information::getIntersection(&line, e, true);

                if (sol.hasValid()) {
                    inters = true;
                }
            }

            if (inters) {
                if (graphicView) {
                    graphicView->deleteEntity(e);
                }

                e->setSelected(select);

                if (graphicView) {
                    graphicView->drawEntity(e);
                }
            }
        }
    }

}



/**
 * Selects all entities that are connected to the given entity.
 *
 * @param e The entity where the algorithm starts. Must be an atomic entity.
 */
void RS_Selection::selectContour(RS_Entity* e) {

    if (e==NULL) {
        return;
    }

    if (!e->isAtomic()) {
        return;
    }

    bool select = !e->isSelected();
    RS_AtomicEntity* ae = (RS_AtomicEntity*)e;
    RS_Vector p1 = ae->getStartpoint();
    RS_Vector p2 = ae->getEndpoint();
    bool found = false;

    // (de)select 1st entity:
    if (graphicView) {
        graphicView->deleteEntity(e);
    }
    e->setSelected(select);
    if (graphicView) {
        graphicView->drawEntity(e);
    }

    do {
        found = false;

		for(auto en: *container){
        //for (unsigned i=0; i<container->count(); ++i) {
            //RS_Entity* en = container->entityAt(i);

            if (en && en->isVisible() && 
				en->isAtomic() && en->isSelected()!=select && 
				(!(en->getLayer() && en->getLayer()->isLocked()))) {

                ae = (RS_AtomicEntity*)en;
                bool doit = false;

                // startpoint connects to 1st point
                if (ae->getStartpoint().distanceTo(p1)<1.0e-4) {
                    doit = true;
                    p1 = ae->getEndpoint();
                }

                // endpoint connects to 1st point
                else if (ae->getEndpoint().distanceTo(p1)<1.0e-4) {
                    doit = true;
                    p1 = ae->getStartpoint();
                }

                // startpoint connects to 2nd point
                else if (ae->getStartpoint().distanceTo(p2)<1.0e-4) {
                    doit = true;
                    p2 = ae->getEndpoint();
                }

                // endpoint connects to 1st point
                else if (ae->getEndpoint().distanceTo(p2)<1.0e-4) {
                    doit = true;
                    p2 = ae->getStartpoint();
                }

                if (doit) {
                    if (graphicView) {
                        graphicView->deleteEntity(ae);
                    }
                    ae->setSelected(select);
                    if (graphicView) {
                        graphicView->drawEntity(ae);
                    }
                    found = true;
                }
            }
        }
    } while(found);
}



/**
 * Selects all entities on the given layer.
 */
void RS_Selection::selectLayer(RS_Entity* e) {

    if (e==NULL) {
        return;
    }

    bool select = !e->isSelected();

    RS_Layer* layer = e->getLayer(true);
    if (layer==NULL) {
        return;
    }

    QString layerName = layer->getName();
	selectLayer(layerName, select);
}



/**
 * Selects all entities on the given layer.
 */
void RS_Selection::selectLayer(const QString& layerName, bool select) {

	for(auto en: *container){

        if (en && en->isVisible() && 
				en->isSelected()!=select && 
				(!(en->getLayer() && en->getLayer()->isLocked()))) {

            RS_Layer* l = en->getLayer(true);

            if (l && l->getName()==layerName) {
                if (graphicView) {
                    graphicView->deleteEntity(en);
                }
                en->setSelected(select);
                if (graphicView) {
                    graphicView->drawEntity(en);
                }
            }
        }
    }
}

// EOF
