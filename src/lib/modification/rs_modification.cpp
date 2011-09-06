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

#include "rs_modification.h"

#include "rs_graphicview.h"
#include "rs_clipboard.h"
#include "rs_creation.h"
//#include "rs_entity.h"
#include "rs_graphic.h"
#include "rs_information.h"
#include "rs_insert.h"
#include "rs_block.h"
#include "rs_polyline.h"
#include "rs_text.h"
#include "rs_layer.h"



/**
 * Default constructor.
 *
 * @param container The container to which we will add
 *        entities. Usually that's an RS_Graphic entity but
 *        it can also be a polyline, text, ...
 * @param graphicView Pointer to graphic view or NULL if you don't want the
 *        any views to be updated.
 * @param handleUndo true: Handle undo functionalitiy.
 */
RS_Modification::RS_Modification(RS_EntityContainer& container,
                                 RS_GraphicView* graphicView,
                                 bool handleUndo) {
    this->container = &container;
    this->graphicView = graphicView;
    this->handleUndo = handleUndo;
    graphic = container.getGraphic();
    document = container.getDocument();
}



/**
 * Deletes all selected entities.
 */
void RS_Modification::remove
() {
    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::remove: no valid container",
                        RS_Debug::D_WARNING);
        return;
    }

    if (document!=NULL) {
        document->startUndoCycle();
    }

    // not safe (?)
    for (RS_Entity* e=container->firstEntity(); e!=NULL;
            e=container->nextEntity()) {

        if (e!=NULL && e->isSelected()) {
            e->setSelected(false);
            e->changeUndoState();
            if (document!=NULL) {
                document->addUndoable(e);
            }
        }
    }

    if (document!=NULL) {
        document->endUndoCycle();
    }

    graphicView->redraw(RS2::RedrawDrawing);
}



/**
 * Changes the attributes of all selected
 */
bool RS_Modification::changeAttributes(RS_AttributesData& data) {
    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::changeAttributes: no valid container",
                        RS_Debug::D_WARNING);
        return false;
    }

    QList<RS_Entity*> addList;

    if (document!=NULL) {
        document->startUndoCycle();
    }

    for (RS_Entity* e=container->firstEntity(); e!=NULL;
            e=container->nextEntity()) {
        //for (uint i=0; i<container->count(); ++i) {
        //RS_Entity* e = container->entityAt(i);
        if (e!=NULL && e->isSelected()) {
            RS_Entity* ec = e->clone();
            ec->setSelected(false);

            RS_Pen pen = ec->getPen(false);

            if (data.changeLayer==true) {
                ec->setLayer(data.layer);
            }

            if (data.changeColor==true) {
                pen.setColor(data.pen.getColor());
            }
            if (data.changeLineType==true) {
                pen.setLineType(data.pen.getLineType());
            }
            if (data.changeWidth==true) {
                pen.setWidth(data.pen.getWidth());
            }

            ec->setPen(pen);

            //if (data.useCurrentLayer) {
            //    ec->setLayerToActive();
            //}
            //if (data.useCurrentAttributes) {
            //    ec->setPenToActive();
            //}
            //if (ec->rtti()==RS2::EntityInsert) {
            //    ((RS_Insert*)ec)->update();
            //}
            ec->update();
            addList.append(ec);
        }
    }

    deselectOriginals(true);
    addNewEntities(addList);

    if (document!=NULL) {
        document->endUndoCycle();
    }

    if (graphicView!=NULL) {
        graphicView->redraw(RS2::RedrawDrawing);
    }

    return true;
}


/**
 * Copies all selected entities from the given container to the clipboard.
 * Layers and blocks that are needed are also copied if the container is
 * or is part of an RS_Graphic.
 *
 * @param container The entity container.
 * @param ref Reference point. The entities will be moved by -ref.
 * @param cut true: cut instead of copying, false: copy
 */
void RS_Modification::copy(const RS_Vector& ref, const bool cut) {

    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::copy: no valid container",
                        RS_Debug::D_WARNING);
        return;
    }

    RS_CLIPBOARD->clear();
    if (graphic!=NULL) {
        RS_CLIPBOARD->getGraphic()->setUnit(graphic->getUnit());
    } else {
        RS_CLIPBOARD->getGraphic()->setUnit(RS2::None);
    }

    // start undo cycle for the container if we're cutting
    if (cut && document!=NULL) {
        document->startUndoCycle();
    }

    // copy entities / layers / blocks
    for (RS_Entity* e=container->firstEntity(); e!=NULL;
            e=container->nextEntity()) {
        //for (uint i=0; i<container->count(); ++i) {
        //RS_Entity* e = container->entityAt(i);

        if (e!=NULL && e->isSelected()) {
            copyEntity(e, ref, cut);
        }
    }

    if (cut && document!=NULL) {
        document->endUndoCycle();
    }
}



/**
 * Copies the given entity from the given container to the clipboard.
 * Layers and blocks that are needed are also copied if the container is
 * or is part of an RS_Graphic.
 *
 * @param e The entity.
 * @param ref Reference point. The entities will be moved by -ref.
 * @param cut true: cut instead of copying, false: copy
 */
void RS_Modification::copyEntity(RS_Entity* e, const RS_Vector& ref,
                                 const bool cut) {

    if (e!=NULL && e->isSelected()) {
        // delete entity in graphic view:
        if (cut) {
            if (graphicView!=NULL) {
                graphicView->deleteEntity(e);
            }
            e->setSelected(false);
        } else {
            if (graphicView!=NULL) {
                graphicView->deleteEntity(e);
            }
            e->setSelected(false);
            if (graphicView!=NULL) {
                graphicView->drawEntity(e);
            }
        }

        // add entity to clipboard:
        RS_Entity* c = e->clone();
        c->move(-ref);
        RS_CLIPBOARD->addEntity(c);

        copyLayers(e);
        copyBlocks(e);

        // set layer to the layer clone:
        RS_Layer* l = e->getLayer();
        if (l!=NULL) {
            c->setLayer(l->getName());
        }

        // make sure all sub entities point to layers of the clipboard
        if (c->isContainer()) {
            RS_EntityContainer* ec = (RS_EntityContainer*)c;

            for (RS_Entity* e2 = ec->firstEntity(RS2::ResolveAll); e2!=NULL;
                    e2 = ec->nextEntity(RS2::ResolveAll)) {

                //RS_Entity* e2 = ec->entityAt(i);
                RS_Layer* l2 = e2->getLayer();

                if (l2!=NULL) {
                    e2->setLayer(l2->getName());
                }
            }
        }

        if (cut) {
            e->changeUndoState();
            if (document!=NULL) {
                document->addUndoable(e);
            }
        }
    }

}



/**
 * Copies all layers of the given entity to the clipboard.
 */
void RS_Modification::copyLayers(RS_Entity* e) {

    if (e==NULL) {
        return;
    }

    // add layer(s) of the entity if it's not an insert
    //  (inserts are on layer '0'):
    if (e->rtti()!=RS2::EntityInsert) {
        RS_Layer* l = e->getLayer();
        if (l!=NULL) {
            if (!RS_CLIPBOARD->hasLayer(l->getName())) {
                RS_CLIPBOARD->addLayer(l->clone());
            }
        }
    }

    // special handling of inserts:
    else {
        // insert: add layer(s) of subentities:
        RS_Block* b = ((RS_Insert*)e)->getBlockForInsert();
        if (b!=NULL) {
            for (RS_Entity* e2=b->firstEntity(); e2!=NULL;
                    e2=b->nextEntity()) {
                //for (uint i=0; i<b->count(); ++i) {
                //RS_Entity* e2 = b->entityAt(i);
                copyLayers(e2);
            }
        }
    }
}



/**
 * Copies all blocks of the given entity to the clipboard.
 */
void RS_Modification::copyBlocks(RS_Entity* e) {

    if (e==NULL) {
        return;
    }

    // add block of the entity if it's an insert
    if (e->rtti()==RS2::EntityInsert) {
        RS_Block* b = ((RS_Insert*)e)->getBlockForInsert();
        if (b!=NULL) {
            // add block of an insert:
            if (!RS_CLIPBOARD->hasBlock(b->getName())) {
                RS_CLIPBOARD->addBlock((RS_Block*)b->clone());
            }

            for (RS_Entity* e2=b->firstEntity(); e2!=NULL;
                    e2=b->nextEntity()) {
                //for (uint i=0; i<b->count(); ++i) {
                //RS_Entity* e2 = b->entityAt(i);
                copyBlocks(e2);
            }
        }
    }
}



/**
 * Pastes all entities from the clipboard into the container.
 * Layers and blocks that are needed are also copied if the container is
 * or is part of an RS_Graphic.
 *
 * @param data Paste data.
 * @param source The source from where to paste. NULL means the source
 *      is the clipboard.
 */
void RS_Modification::paste(const RS_PasteData& data, RS_Graphic* source) {

    if (graphic==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::paste: Graphic is NULL");
        return;
    }

    double factor = 1.0;

    if (source==NULL) {
        source = RS_CLIPBOARD->getGraphic();

        // graphics from the clipboard need to be scaled. from the part lib not:
        RS2::Unit sourceUnit = source->getUnit();
        RS2::Unit targetUnit = graphic->getUnit();
        factor = RS_Units::convert(1.0, sourceUnit, targetUnit);
    }

    if (document!=NULL) {
        document->startUndoCycle();
    }


    // insert layers:
    if (graphic!=NULL) {
        RS_Layer* layer = graphic->getActiveLayer();
        for(uint i=0; i<source->countLayers(); ++i) {
            RS_Layer* l = source->layerAt(i);
            if (l!=NULL) {
                if (graphic->findLayer(l->getName())==NULL) {
                    graphic->addLayer(l->clone());
                }
            }
        }
        graphic->activateLayer(layer);
    }

    // insert blocks:
    if (graphic!=NULL) {
        for(uint i=0; i<source->countBlocks(); ++i) {
            RS_Block* b = source->blockAt(i);
            if (b!=NULL) {
                if (graphic->findBlock(b->getName())==NULL) {
                    RS_Block* bc = (RS_Block*)b->clone();
                    bc->reparent(container);
                    //bc->scale(bc->getBasePoint(), RS_Vector(factor, factor));
                    // scale block but don't scale inserts in block
                    //  (they already scale with their block)
                    for(uint i2=0; i2<bc->count(); ++i2) {
                        RS_Entity* e = bc->entityAt(i2);
                        if (e!=NULL && e->rtti()!=RS2::EntityInsert) {
                            e->scale(bc->getBasePoint(),
                                     RS_Vector(factor, factor));
                        } else {
                            RS_Vector ip = ((RS_Insert*)e)->getInsertionPoint();
                            ip.scale(bc->getBasePoint(),
                                     RS_Vector(factor, factor));
                            ((RS_Insert*)e)->setInsertionPoint(ip);
                            e->update();
                        }
                    }

                    graphic->addBlock(bc);
                }
            }
        }
    }

    // add entities to this host (graphic or a new block)
    RS_EntityContainer* host = container;
    QString blockName;

    // create new block:
    if (graphic!=NULL) {
        if (data.asInsert==true) {
            RS_BlockList* blkList = graphic->getBlockList();
            if (blkList!=NULL) {
                blockName = blkList->newName(data.blockName);

                RS_Block* blk =
                    new RS_Block(graphic,
                                 RS_BlockData(blockName,
                                              RS_Vector(0.0,0.0), false));
                graphic->addBlock(blk);

                host = blk;
            }
        }
    }

    // insert entities:
    //for (uint i=0; i<((RS_EntityContainer*)source)->count(); ++i) {
    //RS_Entity* e = source->entityAt(i);
    for (RS_Entity* e=((RS_EntityContainer*)source)->firstEntity();
            e!=NULL;
            e=((RS_EntityContainer*)source)->nextEntity()) {

        if (e!=NULL) {

            QString layerName = "0";
            RS_Layer* layer = e->getLayer();
            if (layer!=NULL) {
                layerName = layer->getName();
            }
            RS_Entity* e2 = e->clone();
            e2->reparent(host);
            if (data.asInsert==false) {
                e2->move(data.insertionPoint);
            }
            // don't adjust insert factor - block was already adjusted to unit
            if (e2->rtti()==RS2::EntityInsert) {
                RS_Vector ip = ((RS_Insert*)e2)->getInsertionPoint();
                ip.scale(data.insertionPoint, RS_Vector(factor, factor));
                ((RS_Insert*)e2)->setInsertionPoint(ip);
                e2->update();
            } else {
                e2->scale(data.insertionPoint, RS_Vector(factor, factor));
            }
            host->addEntity(e2);
            e2->setLayer(layerName);

            // make sure all sub entities point to layers of the container
            if (e2->isContainer()) {
                RS_EntityContainer* ec = (RS_EntityContainer*)e2;

                for (RS_Entity* e3 = ec->firstEntity(RS2::ResolveAll); e3!=NULL;
                        e3 = ec->nextEntity(RS2::ResolveAll)) {

                    //RS_Entity* e3 = ec->entityAt(i);
                    RS_Layer* l2 = e3->getLayer();
                    if (l2!=NULL) {
                        e3->setLayer(l2->getName());
                    }
                }
            }

            if (document!=NULL && data.asInsert==false) {
                document->addUndoable(e2);
            }
        }
    }

    if (data.asInsert==true) {
        RS_Insert* ins =
            new RS_Insert(container,
                          RS_InsertData(
                              blockName,
                              data.insertionPoint,
                              RS_Vector(data.factor, data.factor),
                              data.angle,
                              1,1,RS_Vector(0.0,0.0)));
        container->addEntity(ins);
        ins->setLayerToActive();
        ins->setPenToActive();

        if (document!=NULL) {
            document->addUndoable(ins);
        }
    }

    if (document!=NULL) {
        document->endUndoCycle();
    }
}


/**
 * Splits a polyline into two leaving out a gap.
 *
 * @param polyline The original polyline
 * @param e1 1st entity on which the first cutting point is.
 * @param v1 1st cutting point.
 * @param e2 2nd entity on which the first cutting point is.
 * @param v2 2nd cutting point.
 * @param polyline1 Pointer to a polyline pointer which will hold the
 *        1st resulting new polyline. Pass NULL if you don't
 *        need those pointers.
 * @param polyline2 Pointer to a polyline pointer which will hold the
 *        2nd resulting new polyline. Pass NULL if you don't
 *        need those pointers.
 *
 * @todo Support arcs in polylines, check for wrong parameters
 *
 * @return true
 */
bool RS_Modification::splitPolyline(RS_Polyline& polyline,
                                    RS_Entity& e1, RS_Vector v1,
                                    RS_Entity& e2, RS_Vector v2,
                                    RS_Polyline** polyline1,
                                    RS_Polyline** polyline2) const {

    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::splitPolyline: no valid container",
                        RS_Debug::D_WARNING);
        return false;
    }

    RS_Entity* firstEntity = polyline.firstEntity();
    RS_Vector firstPoint(false);
    if (firstEntity->rtti()==RS2::EntityLine) {
        firstPoint = ((RS_Line*)firstEntity)->getStartpoint();
    }
    RS_Polyline* pl1 =
        new RS_Polyline(container,
                        RS_PolylineData(firstPoint, RS_Vector(0.0,0.0), 0));
    RS_Polyline* pl2 = new RS_Polyline(container);
    RS_Polyline* pl = pl1;	// Current polyline
    RS_Line* line = NULL;
    RS_Arc* arc = NULL;

    if (polyline1!=NULL) {
        *polyline1 = pl1;
    }
    if (polyline2!=NULL) {
        *polyline2 = pl2;
    }

    for (RS_Entity* e = polyline.firstEntity();
            e != NULL;
            e = polyline.nextEntity()) {

        if (e->rtti()==RS2::EntityLine) {
            line = (RS_Line*)e;
            arc = NULL;
        } else if (e->rtti()==RS2::EntityArc) {
            arc = (RS_Arc*)e;
            line = NULL;
        } else {
            line = NULL;
            arc = NULL;
        }

        if (line!=NULL /*|| arc!=NULL*/) {

            if (e==&e1 && e==&e2) {
                // Trim within a single entity:
                RS_Vector sp = line->getStartpoint();
                double dist1 = (v1-sp).magnitude();
                double dist2 = (v2-sp).magnitude();
                pl->addVertex(dist1<dist2 ? v1 : v2, 0.0);
                pl = pl2;
                pl->setStartpoint(dist1<dist2 ? v2 : v1);
                pl->addVertex(line->getEndpoint(), 0.0);
            } else if (e==&e1 || e==&e2) {
                // Trim entities:
                RS_Vector v = (e==&e1 ? v1 : v2);
                if (pl==pl1) {
                    // Trim endpoint of entity to first vector
                    pl->addVertex(v, 0.0);
                    pl = NULL;
                } else {
                    // Trim startpoint of entity to second vector
                    pl = pl2;
                    pl->setStartpoint(v);
                    pl->addVertex(line->getEndpoint(), 0.0);
                }
            } else {
                // Add entities to polylines
                if (line!=NULL && pl!=NULL) {
                    pl->addVertex(line->getEndpoint(), 0.0);
                }
            }
        }
    }

    container->addEntity(pl1);
    container->addEntity(pl2);
    //container->removeEntity(&polyline);
    polyline.changeUndoState();

    return true;
}



/**
 * Adds a node to the given polyline. The new node is placed between
 * the start and end point of the given segment.
 *
 * @param node The position of the new node.
 *
 * @return Pointer to the new polyline or NULL.
 */
RS_Polyline* RS_Modification::addPolylineNode(RS_Polyline& polyline,
        const RS_AtomicEntity& segment,
        const RS_Vector& node) {
    RS_DEBUG->print("RS_Modification::addPolylineNode");

    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::addPolylineNode: no valid container",
                        RS_Debug::D_WARNING);
        return NULL;
    }

    if (segment.getParent()!=&polyline) {
        RS_DEBUG->print("RS_Modification::addPolylineNode: "
                        "segment not part of the polyline",
                        RS_Debug::D_WARNING);
        return NULL;
    }

    RS_Polyline* newPolyline = new RS_Polyline(container);
    newPolyline->setClosed(polyline.isClosed());
    newPolyline->setSelected(polyline.isSelected());
    newPolyline->setLayer(polyline.getLayer());
    newPolyline->setPen(polyline.getPen());

    // copy polyline and add new node:
    bool first = true;
    RS_Entity* lastEntity = polyline.lastEntity();
    for (RS_Entity* e=polyline.firstEntity(); e!=NULL;
            e=polyline.nextEntity()) {

        if (e->isAtomic()) {
            RS_AtomicEntity* ae = (RS_AtomicEntity*)e;
            double bulge = 0.0;
            if (ae->rtti()==RS2::EntityArc) {
                RS_DEBUG->print("RS_Modification::addPolylineNode: arc segment");
                bulge = ((RS_Arc*)ae)->getBulge();
            } else {
                RS_DEBUG->print("RS_Modification::addPolylineNode: line segment");
                bulge = 0.0;
            }

            if (first) {
                RS_DEBUG->print("RS_Modification::addPolylineNode: first segment: %f/%f",
                                ae->getStartpoint().x, ae->getStartpoint().y);

                newPolyline->setNextBulge(bulge);
                newPolyline->addVertex(ae->getStartpoint());
                first = false;
            }

            // segment to split:
            if (ae==&segment) {
                RS_DEBUG->print("RS_Modification::addPolylineNode: split segment found");

                RS_DEBUG->print("RS_Modification::addPolylineNode: node: %f/%f",
                                node.x, node.y);

                newPolyline->setNextBulge(0.0);
                newPolyline->addVertex(node);

                RS_DEBUG->print("RS_Modification::addPolylineNode: after node: %f/%f",
                                ae->getEndpoint().x, ae->getEndpoint().y);

                if (ae!=lastEntity || polyline.isClosed()==false) {
                    newPolyline->setNextBulge(0.0);
                    newPolyline->addVertex(ae->getEndpoint());
                }
            } else {
                RS_DEBUG->print("RS_Modification::addPolylineNode: normal vertex found: %f/%f",
                                ae->getEndpoint().x, ae->getEndpoint().y);

                if (ae!=lastEntity || polyline.isClosed()==false) {
                    newPolyline->setNextBulge(bulge);
                    newPolyline->addVertex(ae->getEndpoint());
                }
            }
        } else {
            RS_DEBUG->print("RS_Modification::addPolylineNode: "
                            "Polyline contains non-atomic entities",
                            RS_Debug::D_WARNING);
        }
    }

    newPolyline->setNextBulge(polyline.getClosingBulge());
    newPolyline->endPolyline();

    // add new polyline:
    container->addEntity(newPolyline);
    if (graphicView!=NULL) {
        graphicView->deleteEntity(&polyline);
        graphicView->drawEntity(newPolyline);
    }

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();

        polyline.setUndoState(true);
        document->addUndoable(&polyline);
        document->addUndoable(newPolyline);

        document->endUndoCycle();
    }

    return newPolyline;
}




/**
 * Deletes a node from a polyline.
 *
 * @param node The node to delete.
 *
 * @return Pointer to the new polyline or NULL.
 */

RS_Polyline* RS_Modification::deletePolylineNode(RS_Polyline& polyline,
        const RS_Vector& node) {

    RS_DEBUG->print("RS_Modification::deletePolylineNode");

    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::addPolylineNode: no valid container",
                        RS_Debug::D_WARNING);
        return NULL;
    }

    if (node.valid==false) {
        RS_DEBUG->print("RS_Modification::deletePolylineNode: "
                        "node not valid",
                        RS_Debug::D_WARNING);
        return NULL;
    }

    // check if the polyline is no longer there after deleting the node:
    if (polyline.count()==1) {
        RS_Entity* e = polyline.firstEntity();
        if (e!=NULL && e->isAtomic()) {
            RS_AtomicEntity* ae = (RS_AtomicEntity*)e;
            if (node.distanceTo(ae->getStartpoint())<1.0e-6 ||
                    node.distanceTo(ae->getEndpoint())<1.0e-6) {

                if (graphicView!=NULL) {
                    graphicView->deleteEntity(&polyline);
                }

                if (document!=NULL && handleUndo) {
                    document->startUndoCycle();
                    polyline.setUndoState(true);
                    document->addUndoable(&polyline);
                    document->endUndoCycle();
                }
            }
        }
        return NULL;
    }

    RS_Polyline* newPolyline = new RS_Polyline(container);
    newPolyline->setClosed(polyline.isClosed());
    newPolyline->setSelected(polyline.isSelected());
    newPolyline->setLayer(polyline.getLayer());
    newPolyline->setPen(polyline.getPen());

    // copy polyline and drop deleted node:
    bool first = true;
    bool lastDropped = false;
    RS_Entity* lastEntity = polyline.lastEntity();
    for (RS_Entity* e=polyline.firstEntity(); e!=NULL;
            e=polyline.nextEntity()) {

        if (e->isAtomic()) {
            RS_AtomicEntity* ae = (RS_AtomicEntity*)e;
            double bulge = 0.0;
            if (ae->rtti()==RS2::EntityArc) {
                RS_DEBUG->print("RS_Modification::deletePolylineNode: arc segment");
                bulge = ((RS_Arc*)ae)->getBulge();
            } else {
                RS_DEBUG->print("RS_Modification::deletePolylineNode: line segment");
                bulge = 0.0;
            }

            // last entity is closing entity and will be added below with endPolyline()
            if (e==lastEntity && polyline.isClosed()) {
                continue;
            }

            // first vertex (startpoint)
            if (first && node.distanceTo(ae->getStartpoint())>1.0e-6) {
                RS_DEBUG->print("RS_Modification::deletePolylineNode: first node: %f/%f",
                                ae->getStartpoint().x, ae->getStartpoint().y);

                newPolyline->setNextBulge(bulge);
                newPolyline->addVertex(ae->getStartpoint());
                first = false;
            }

            // normal node (not deleted):
            if (first==false && node.distanceTo(ae->getEndpoint())>1.0e-6) {
                RS_DEBUG->print("RS_Modification::deletePolylineNode: normal vertex found: %f/%f",
                                ae->getEndpoint().x, ae->getEndpoint().y);
                if (lastDropped) {
                    //bulge = 0.0;
                }
                newPolyline->setNextBulge(bulge);
                newPolyline->addVertex(ae->getEndpoint());
                lastDropped = false;
            }

            // drop deleted node:
            else {
                RS_DEBUG->print("RS_Modification::deletePolylineNode: deleting vertex: %f/%f",
                                ae->getEndpoint().x, ae->getEndpoint().y);
                lastDropped = true;
            }
        } else {
            RS_DEBUG->print("RS_Modification::deletePolylineNode: "
                            "Polyline contains non-atomic entities",
                            RS_Debug::D_WARNING);
        }
    }

    RS_DEBUG->print("RS_Modification::deletePolylineNode: ending polyline");
    newPolyline->setNextBulge(polyline.getClosingBulge());
    newPolyline->endPolyline();

    //if (newPolyline->count()==1) {
    //}

    // add new polyline:
    RS_DEBUG->print("RS_Modification::deletePolylineNode: adding new polyline");
    container->addEntity(newPolyline);
    if (graphicView!=NULL) {
        graphicView->deleteEntity(&polyline);
        graphicView->drawEntity(newPolyline);
    }

    RS_DEBUG->print("RS_Modification::deletePolylineNode: handling undo");
    if (document!=NULL && handleUndo) {
        document->startUndoCycle();

        polyline.setUndoState(true);
        document->addUndoable(&polyline);
        document->addUndoable(newPolyline);

        document->endUndoCycle();
    }

    return newPolyline;
}




/**
 * Deletes all nodes between the two given nodes (exclusive).
 *
 * @param node1 First limiting node.
 * @param node2 Second limiting node.
 *
 * @return Pointer to the new polyline or NULL.
 */

RS_Polyline* RS_Modification::deletePolylineNodesBetween(RS_Polyline& polyline,
        RS_AtomicEntity& segment, const RS_Vector& node1, const RS_Vector& node2) {
    Q_UNUSED(segment);
    RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween");

    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::addPolylineNodesBetween: no valid container",
                        RS_Debug::D_WARNING);
        return NULL;
    }

    if (node1.valid==false || node2.valid==false) {
        RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: "
                        "node not valid",
                        RS_Debug::D_WARNING);
        return NULL;
    }

    if (node1.distanceTo(node2)<1.0e-6) {
        RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: "
                        "nodes are identical",
                        RS_Debug::D_WARNING);
        return NULL;
    }

    // check if there's nothing to delete:
    for (RS_Entity* e=polyline.firstEntity(); e!=NULL;
            e=polyline.nextEntity()) {

        if (e->isAtomic()) {
            RS_AtomicEntity* ae = (RS_AtomicEntity*)e;

            if ((node1.distanceTo(ae->getStartpoint())<1.0e-6 &&
                    node2.distanceTo(ae->getEndpoint())<1.0e-6) ||
                    (node2.distanceTo(ae->getStartpoint())<1.0e-6 &&
                     node1.distanceTo(ae->getEndpoint())<1.0e-6)) {

                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: "
                                "nothing to delete",
                                RS_Debug::D_WARNING);
                return NULL;
            }
        }
    }


    // check if the start point is involved:
    bool startpointInvolved = false;
    if (node1.distanceTo(polyline.getStartpoint())<1.0e-6 ||
            node2.distanceTo(polyline.getStartpoint())<1.0e-6) {
        startpointInvolved = true;
    }


    // check which part of the polyline has to be deleted:
    bool deleteStart = false;
    if (polyline.isClosed()) {
        bool found = false;
        double length1 = 0.0;
        double length2 = 0.0;
        RS_Entity* e=polyline.firstEntity();

        if (startpointInvolved) {
            if (e->isAtomic()) {
                RS_AtomicEntity* ae = (RS_AtomicEntity*)e;
                length1+=ae->getLength();
            }
            e = polyline.nextEntity();
        }
        for (; e!=NULL; e=polyline.nextEntity()) {

            if (e->isAtomic()) {
                RS_AtomicEntity* ae = (RS_AtomicEntity*)e;

                if (node1.distanceTo(ae->getStartpoint())<1.0e-6 ||
                        node2.distanceTo(ae->getStartpoint())<1.0e-6) {

                    found = !found;
                }

                if (found) {
                    length2+=ae->getLength();
                } else {
                    length1+=ae->getLength();
                }
            }
        }
        if (length1<length2) {
            deleteStart = true;
        } else {
            deleteStart = false;
        }
    }

    RS_Polyline* newPolyline = new RS_Polyline(container);
    newPolyline->setClosed(polyline.isClosed());
    newPolyline->setSelected(polyline.isSelected());
    newPolyline->setLayer(polyline.getLayer());
    newPolyline->setPen(polyline.getPen());

    if (startpointInvolved && deleteStart && polyline.isClosed()) {
        newPolyline->setNextBulge(0.0);
        newPolyline->addVertex(polyline.getStartpoint());
    }

    // copy polyline and drop deleted nodes:
    bool first = true;
    bool removing = deleteStart;
    bool done = false;
    bool nextIsStraight = false;
    RS_Entity* lastEntity = polyline.lastEntity();
    int i=0;
    double bulge = 0.0;
    for (RS_Entity* e=polyline.firstEntity(); e!=NULL;
            e=polyline.nextEntity()) {

        RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: entity: %d", i++);
        RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: removing: %d", (int)removing);

        if (e->isAtomic()) {
            RS_AtomicEntity* ae = (RS_AtomicEntity*)e;
            if (ae->rtti()==RS2::EntityArc) {
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: arc segment");
                bulge = ((RS_Arc*)ae)->getBulge();
            } else {
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: line segment");
                bulge = 0.0;
            }

            // last entity is closing entity and will be added below with endPolyline()
            if (e==lastEntity && polyline.isClosed()) {
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: "
                                "dropping last vertex of closed polyline");
                continue;
            }

            // first vertex (startpoint)
            if (first) {
                if (!removing) {
                    RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: first node: %f/%f",
                                    ae->getStartpoint().x, ae->getStartpoint().y);
                    newPolyline->setNextBulge(bulge);
                    newPolyline->addVertex(ae->getStartpoint());
                    first = false;
                }
            }

            // stop removing nodes:
            if (removing==true &&
                    (node1.distanceTo(ae->getEndpoint())<1.0e-6 ||
                     node2.distanceTo(ae->getEndpoint())<1.0e-6)) {
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: "
                                "stop removing at: %f/%f",
                                ae->getEndpoint().x, ae->getEndpoint().y);
                removing = false;
                done = true;
                if (first==false) {
                    nextIsStraight = true;
                }
            }

            // normal node (not deleted):
            if (removing==false && (done==false || deleteStart==false)) {
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: "
                                "normal vertex found: %f/%f",
                                ae->getEndpoint().x, ae->getEndpoint().y);
                if (nextIsStraight) {
                    bulge = 0.0;
                    nextIsStraight = false;
                }
                newPolyline->setNextBulge(bulge);
                newPolyline->addVertex(ae->getEndpoint());
            }

            // drop deleted node:
            else {
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: "
                                "deleting vertex: %f/%f",
                                ae->getEndpoint().x, ae->getEndpoint().y);
            }

            // start to remove nodes from now on:
            if (done==false && removing==false &&
                    (node1.distanceTo(ae->getEndpoint())<1.0e-6 ||
                     node2.distanceTo(ae->getEndpoint())<1.0e-6)) {
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: "
                                "start removing at: %f/%f",
                                ae->getEndpoint().x, ae->getEndpoint().y);
                removing = true;
            }

            if (done) {
                done=false;
            }
        } else {
            RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: "
                            "Polyline contains non-atomic entities",
                            RS_Debug::D_WARNING);
        }
    }

    RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: ending polyline");
    newPolyline->setNextBulge(polyline.getClosingBulge());
    newPolyline->endPolyline();

    // add new polyline:
    RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: adding new polyline");
    container->addEntity(newPolyline);
    if (graphicView!=NULL) {
        graphicView->deleteEntity(&polyline);
        graphicView->drawEntity(newPolyline);
    }

    RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: handling undo");
    if (document!=NULL && handleUndo) {
        document->startUndoCycle();

        polyline.setUndoState(true);
        document->addUndoable(&polyline);
        document->addUndoable(newPolyline);

        document->endUndoCycle();
    }

    return newPolyline;
}




/**
 * Trims two segments of a polyline all nodes between the two trim segments
 * are removed.
 *
 * @param polyline The polyline entity.
 * @param segment1 First segment to trim.
 * @param segment2 Second segment to trim.
 *
 * @return Pointer to the new polyline or NULL.
 */

RS_Polyline* RS_Modification::polylineTrim(RS_Polyline& polyline,
        RS_AtomicEntity& segment1,
        RS_AtomicEntity& segment2) {

    RS_DEBUG->print("RS_Modification::polylineTrim");

    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::addPolylineNodesBetween: no valid container",
                        RS_Debug::D_WARNING);
        return NULL;
    }

    if (segment1.getParent()!=&polyline || segment2.getParent()!=&polyline) {
        RS_DEBUG->print("RS_Modification::polylineTrim: "
                        "segments not in polyline",
                        RS_Debug::D_WARNING);
        return NULL;
    }

    if (&segment1==&segment2) {
        RS_DEBUG->print("RS_Modification::polylineTrim: "
                        "segments are identical",
                        RS_Debug::D_WARNING);
        return NULL;
    }

    RS_VectorSolutions sol;
    sol = RS_Information::getIntersection(&segment1, &segment2, false);

    if (sol.getNumber()==0) {
        RS_DEBUG->print("RS_Modification::polylineTrim: "
                        "segments cannot be trimmed",
                        RS_Debug::D_WARNING);
        return NULL;
    }

    // check which segment comes first in the polyline:
    RS_AtomicEntity* firstSegment;
    if (polyline.findEntity(&segment1) > polyline.findEntity(&segment2)) {
        firstSegment = &segment2;
    } else {
        firstSegment = &segment1;
    }

    // find out if we need to trim towards the open part of the polyline
    bool reverseTrim;
    reverseTrim = !RS_Math::isSameDirection(firstSegment->getDirection1(),
                                            firstSegment->getStartpoint().angleTo(sol.get(0)), M_PI/2.0);
    //reverseTrim = reverseTrim || !RS_Math::isSameDirection(segment2.getDirection1(),
    //	segment2.getStartpoint().angleTo(sol.get(0)), M_PI/2.0);

    RS_Polyline* newPolyline = new RS_Polyline(container);
    newPolyline->setClosed(polyline.isClosed());
    newPolyline->setSelected(polyline.isSelected());
    newPolyline->setLayer(polyline.getLayer());
    newPolyline->setPen(polyline.getPen());

    // normal trimming: start removing nodes at trim segment. ends stay the same
    if (reverseTrim==false) {
        // copy polyline, trim segments and drop between nodes:
        bool first = true;
        bool removing = false;
        bool nextIsStraight = false;
        RS_Entity* lastEntity = polyline.lastEntity();
        for (RS_Entity* e=polyline.firstEntity(); e!=NULL;
                e=polyline.nextEntity()) {

            if (e->isAtomic()) {
                RS_AtomicEntity* ae = (RS_AtomicEntity*)e;
                double bulge = 0.0;
                if (ae->rtti()==RS2::EntityArc) {
                    RS_DEBUG->print("RS_Modification::polylineTrim: arc segment");
                    bulge = ((RS_Arc*)ae)->getBulge();
                } else {
                    RS_DEBUG->print("RS_Modification::polylineTrim: line segment");
                    bulge = 0.0;
                }

                // last entity is closing entity and will be added below with endPolyline()
                if (e==lastEntity && polyline.isClosed()) {
                    RS_DEBUG->print("RS_Modification::polylineTrim: "
                                    "dropping last vertex of closed polyline");
                    continue;
                }

                // first vertex (startpoint)
                if (first) {
                    RS_DEBUG->print("RS_Modification::polylineTrim: first node: %f/%f",
                                    ae->getStartpoint().x, ae->getStartpoint().y);

                    newPolyline->setNextBulge(bulge);
                    newPolyline->addVertex(ae->getStartpoint());
                    first = false;
                }

                // trim and start removing nodes:
                if (removing==false && (ae==&segment1 || ae==&segment2)) {
                    RS_DEBUG->print("RS_Modification::polylineTrim: "
                                    "start removing at trim point %f/%f",
                                    sol.get(0).x, sol.get(0).y);
                    newPolyline->setNextBulge(0.0);
                    newPolyline->addVertex(sol.get(0));
                    removing = true;
                    nextIsStraight = true;
                }

                // stop removing nodes:
                else if (removing==true && (ae==&segment1 || ae==&segment2)) {
                    RS_DEBUG->print("RS_Modification::polylineTrim: stop removing at: %f/%f",
                                    ae->getEndpoint().x, ae->getEndpoint().y);
                    removing = false;
                }

                // normal node (not deleted):
                if (removing==false) {
                    RS_DEBUG->print("RS_Modification::polylineTrim: normal vertex found: %f/%f",
                                    ae->getEndpoint().x, ae->getEndpoint().y);
                    if (nextIsStraight) {
                        newPolyline->setNextBulge(0.0);
                        nextIsStraight = false;
                    } else {
                        newPolyline->setNextBulge(bulge);
                    }
                    newPolyline->addVertex(ae->getEndpoint());
                }
            } else {
                RS_DEBUG->print("RS_Modification::polylineTrim: "
                                "Polyline contains non-atomic entities",
                                RS_Debug::D_WARNING);
            }
        }
    }

    // reverse trimming: remove nodes at the ends and keep those in between
    else {
        // copy polyline, trim segments and drop between nodes:
        //bool first = true;
        bool removing = true;
        bool nextIsStraight = false;
        RS_Entity* lastEntity = polyline.lastEntity();
        for (RS_Entity* e=polyline.firstEntity(); e!=NULL;
                e=polyline.nextEntity()) {

            if (e->isAtomic()) {
                RS_AtomicEntity* ae = (RS_AtomicEntity*)e;
                double bulge = 0.0;
                if (ae->rtti()==RS2::EntityArc) {
                    RS_DEBUG->print("RS_Modification::polylineTrim: arc segment");
                    bulge = ((RS_Arc*)ae)->getBulge();
                } else {
                    RS_DEBUG->print("RS_Modification::polylineTrim: line segment");
                    bulge = 0.0;
                }

                // last entity is closing entity and will be added below with endPolyline()
                if (e==lastEntity && polyline.isClosed()) {
                    RS_DEBUG->print("RS_Modification::polylineTrim: "
                                    "dropping last vertex of closed polyline");
                    continue;
                }

                // trim and stop removing nodes:
                if (removing==true && (ae==&segment1 || ae==&segment2)) {
                    RS_DEBUG->print("RS_Modification::polylineTrim: "
                                    "stop removing at trim point %f/%f",
                                    sol.get(0).x, sol.get(0).y);
                    newPolyline->setNextBulge(0.0);
                    // start of new polyline:
                    newPolyline->addVertex(sol.get(0));
                    removing = false;
                    nextIsStraight = true;
                }

                // start removing nodes again:
                else if (removing==false && (ae==&segment1 || ae==&segment2)) {
                    RS_DEBUG->print("RS_Modification::polylineTrim: start removing at: %f/%f",
                                    ae->getEndpoint().x, ae->getEndpoint().y);
                    newPolyline->setNextBulge(0.0);
                    // start of new polyline:
                    newPolyline->addVertex(sol.get(0));
                    removing = true;
                }

                // normal node (not deleted):
                if (removing==false) {
                    RS_DEBUG->print("RS_Modification::polylineTrim: normal vertex found: %f/%f",
                                    ae->getEndpoint().x, ae->getEndpoint().y);
                    if (nextIsStraight) {
                        newPolyline->setNextBulge(0.0);
                        nextIsStraight = false;
                    } else {
                        newPolyline->setNextBulge(bulge);
                    }
                    newPolyline->addVertex(ae->getEndpoint());
                }
            } else {
                RS_DEBUG->print("RS_Modification::polylineTrim: "
                                "Polyline contains non-atomic entities",
                                RS_Debug::D_WARNING);
            }
        }
    }

    RS_DEBUG->print("RS_Modification::polylineTrim: ending polyline");
    newPolyline->setNextBulge(polyline.getClosingBulge());
    newPolyline->endPolyline();

    // add new polyline:
    RS_DEBUG->print("RS_Modification::polylineTrim: adding new polyline");
    container->addEntity(newPolyline);
    if (graphicView!=NULL) {
        graphicView->deleteEntity(&polyline);
        graphicView->drawEntity(newPolyline);
    }

    RS_DEBUG->print("RS_Modification::polylineTrim: handling undo");
    if (document!=NULL && handleUndo) {
        document->startUndoCycle();

        polyline.setUndoState(true);
        document->addUndoable(&polyline);
        document->addUndoable(newPolyline);

        document->endUndoCycle();
    }

    return newPolyline;
}




/**
 * Moves all selected entities with the given data for the move
 * modification.
 */
bool RS_Modification::move(RS_MoveData& data) {
    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::move: no valid container",
                        RS_Debug::D_WARNING);
        return false;
    }

    QList<RS_Entity*> addList;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    // Create new entites
    for (int num=1;
            num<=data.number || (data.number==0 && num<=1);
            num++) {
        // too slow:
        //for (uint i=0; i<container->count(); ++i) {
        //RS_Entity* e = container->entityAt(i);
        for (RS_Entity* e=container->firstEntity();
                e!=NULL;
                e=container->nextEntity()) {
            if (e!=NULL && e->isSelected()) {
                RS_Entity* ec = e->clone();

                ec->move(data.offset*num);
                if (data.useCurrentLayer) {
                    ec->setLayerToActive();
                }
                if (data.useCurrentAttributes) {
                    ec->setPenToActive();
                }
                if (ec->rtti()==RS2::EntityInsert) {
                    ((RS_Insert*)ec)->update();
                }
                // since 2.0.4.0: keep selection
                ec->setSelected(true);
                addList.append(ec);
            }
        }
    }

    deselectOriginals(data.number==0);
    addNewEntities(addList);

    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
    }

    if (graphicView!=NULL) {
        graphicView->redraw(RS2::RedrawDrawing);
    }
    return true;
}




/**
 * Rotates all selected entities with the given data for the rotation.
 */
bool RS_Modification::rotate(RS_RotateData& data) {
    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::rotate: no valid container",
                        RS_Debug::D_WARNING);
        return false;
    }

    QList<RS_Entity*> addList;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    // Create new entites
    for (int num=1;
            num<=data.number || (data.number==0 && num<=1);
            num++) {
        for (RS_Entity* e=container->firstEntity();
                e!=NULL;
                e=container->nextEntity()) {
            //for (uint i=0; i<container->count(); ++i) {
            //RS_Entity* e = container->entityAt(i);

            if (e!=NULL && e->isSelected()) {
                RS_Entity* ec = e->clone();
                ec->setSelected(false);

                ec->rotate(data.center, data.angle*num);
                if (data.useCurrentLayer) {
                    ec->setLayerToActive();
                }
                if (data.useCurrentAttributes) {
                    ec->setPenToActive();
                }
                if (ec->rtti()==RS2::EntityInsert) {
                    ((RS_Insert*)ec)->update();
                }
                addList.append(ec);
            }
        }
    }

    deselectOriginals(data.number==0);
    addNewEntities(addList);

    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
    }
    if (graphicView!=NULL) {
        graphicView->redraw(RS2::RedrawDrawing);
    }

    return true;
}



/**
 * Moves all selected entities with the given data for the scale
 * modification.
 */
bool RS_Modification::scale(RS_ScaleData& data) {
    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::scale: no valid container",
                        RS_Debug::D_WARNING);
        return false;
    }

    QList<RS_Entity*> selectedList,addList;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }
    for (RS_Entity* ec=container->firstEntity();
            ec!=NULL;
            ec=container->nextEntity()) {
        if (ec->isSelected() ) {
            if ( fabs(data.factor.x - data.factor.y) > RS_TOLERANCE ) {
                    if ( ec->rtti() == RS2::EntityCircle ) {
    //non-isotropic scaling, replacing selected circles with ellipses
                RS_Circle *c=(RS_Circle*) ec;
                RS_EllipseData d(
                    c->getCenter(),
                    RS_Vector(c->getRadius(),0.),
                    1.0,
                    0.,
                    2.*M_PI,
                    false);
                ec= new RS_Ellipse(container,d);
            } else if ( ec->rtti() == RS2::EntityArc ) {
    //non-isotropic scaling, replacing selected arcs with ellipses
                RS_Arc *c=(RS_Arc*) ec;
                RS_EllipseData d(
                    c->getCenter(),
                    RS_Vector(c->getRadius(),0.),
                    1.0,
                    c->getAngle1(),
                    c->getAngle2(),
                    c->isReversed());
                ec= new RS_Ellipse(container,d);
            }
            }
            selectedList.append(ec);

        }
    }


    // Create new entites
    for (int num=1;
            num<=data.number || (data.number==0 && num<=1);
            num++) {

        for(QList<RS_Entity*>::iterator pe=selectedList.begin();
                pe != selectedList.end();
                pe++ ) {
            RS_Entity* e= *pe;
            //for (RS_Entity* e=container->firstEntity();
            //        e!=NULL;
            //        e=container->nextEntity()) {
            //for (uint i=0; i<container->count(); ++i) {
            //RS_Entity* e = container->entityAt(i);
            if (e!=NULL ) {
                RS_Entity* ec = e->clone();
                ec->setSelected(false);

                ec->scale(data.referencePoint, RS_Math::pow(data.factor, num));
                if (data.useCurrentLayer) {
                    ec->setLayerToActive();
                }
                if (data.useCurrentAttributes) {
                    ec->setPenToActive();
                }
                if (ec->rtti()==RS2::EntityInsert) {
                    ((RS_Insert*)ec)->update();
                }
                addList.append(ec);
            }
        }
    }

    deselectOriginals(data.number==0);
    addNewEntities(addList);

    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
    }

    if (graphicView!=NULL) {
        graphicView->redraw(RS2::RedrawDrawing);
    }
    return true;
}



/**
 * Mirror all selected entities with the given data for the mirror
 * modification.
 */
bool RS_Modification::mirror(RS_MirrorData& data) {
    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::mirror: no valid container",
                        RS_Debug::D_WARNING);
        return false;
    }

    QList<RS_Entity*> addList;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    // Create new entites
    for (int num=1;
            num<=(int)data.copy || (data.copy==false && num<=1);
            num++) {
        for (RS_Entity* e=container->firstEntity();
                e!=NULL;
                e=container->nextEntity()) {
            //for (uint i=0; i<container->count(); ++i) {
            //RS_Entity* e = container->entityAt(i);

            if (e!=NULL && e->isSelected()) {
                RS_Entity* ec = e->clone();
                ec->setSelected(false);

                ec->mirror(data.axisPoint1, data.axisPoint2);
                if (data.useCurrentLayer) {
                    ec->setLayerToActive();
                }
                if (data.useCurrentAttributes) {
                    ec->setPenToActive();
                }
                if (ec->rtti()==RS2::EntityInsert) {
                    ((RS_Insert*)ec)->update();
                }
                addList.append(ec);
            }
        }
    }

    deselectOriginals(data.copy==false);
    addNewEntities(addList);

    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
    }

    if (graphicView!=NULL) {
        graphicView->redraw(RS2::RedrawDrawing);
    }
    return true;
}



/**
 * Rotates entities around two centers with the given parameters.
 */
bool RS_Modification::rotate2(RS_Rotate2Data& data) {
    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::rotate2: no valid container",
                        RS_Debug::D_WARNING);
        return false;
    }

    QList<RS_Entity*> addList;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    // Create new entites
    for (int num=1;
            num<=data.number || (data.number==0 && num<=1);
            num++) {

        for (RS_Entity* e=container->firstEntity();
                e!=NULL;
                e=container->nextEntity()) {
            //for (uint i=0; i<container->count(); ++i) {
            //RS_Entity* e = container->entityAt(i);

            if (e!=NULL && e->isSelected()) {
                RS_Entity* ec = e->clone();
                ec->setSelected(false);

                ec->rotate(data.center1, data.angle1*num);
                RS_Vector center2 = data.center2;
                center2.rotate(data.center1, data.angle1*num);

                ec->rotate(center2, data.angle2*num);
                if (data.useCurrentLayer) {
                    ec->setLayerToActive();
                }
                if (data.useCurrentAttributes) {
                    ec->setPenToActive();
                }
                if (ec->rtti()==RS2::EntityInsert) {
                    ((RS_Insert*)ec)->update();
                }
                addList.append(ec);
            }
        }
    }

    deselectOriginals(data.number==0);
    addNewEntities(addList);

    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
    }

    if (graphicView!=NULL) {
        graphicView->redraw(RS2::RedrawDrawing);
    }
    return true;
}



/**
 * Moves and rotates entities with the given parameters.
 */
bool RS_Modification::moveRotate(RS_MoveRotateData& data) {
    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::moveRotate: no valid container",
                        RS_Debug::D_WARNING);
        return false;
    }

    QList<RS_Entity*> addList;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    // Create new entites
    for (int num=1;
            num<=data.number || (data.number==0 && num<=1);
            num++) {
        for (RS_Entity* e=container->firstEntity();
                e!=NULL;
                e=container->nextEntity()) {
            //for (uint i=0; i<container->count(); ++i) {
            //RS_Entity* e = container->entityAt(i);

            if (e!=NULL && e->isSelected()) {
                RS_Entity* ec = e->clone();
                ec->setSelected(false);

                ec->move(data.offset*num);
                ec->rotate(data.referencePoint + data.offset*num,
                           data.angle*num);
                if (data.useCurrentLayer) {
                    ec->setLayerToActive();
                }
                if (data.useCurrentAttributes) {
                    ec->setPenToActive();
                }
                if (ec->rtti()==RS2::EntityInsert) {
                    ((RS_Insert*)ec)->update();
                }
                addList.append(ec);
            }
        }
    }

    deselectOriginals(data.number==0);
    addNewEntities(addList);

    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
    }
    if (graphicView!=NULL) {
        graphicView->redraw(RS2::RedrawDrawing);
    }

    return true;
}



/**
 * Deselects all selected entities and removes them if remove is true;
 *
 * @param remove true: Remove entites.
 */
void RS_Modification::deselectOriginals(bool remove
                                       ) {
    for (RS_Entity* e=container->firstEntity();
            e!=NULL;
            e=container->nextEntity()) {

        //for (uint i=0; i<container->count(); ++i) {
        //RS_Entity* e = container->entityAt(i);

        if (e!=NULL) {
            bool selected = false;

            /*
                  if (e->isAtomic()) {
                      RS_AtomicEntity* ae = (RS_AtomicEntity*)e;
                      if (ae->isStartpointSelected() ||
                              ae->isEndpointSelected()) {

                          selected = true;
                      }
                  }
            */

            if (e->isSelected()) {
                selected = true;
            }

            if (selected) {
                e->setSelected(false);
                if (remove
                   ) {
                    //if (graphicView!=NULL) {
                    //    graphicView->deleteEntity(e);
                    //}
                    e->changeUndoState();
                    if (document!=NULL && handleUndo) {
                        document->addUndoable(e);
                    }
                } else {
                    //if (graphicView!=NULL) {
                    //    graphicView->drawEntity(e);
                    //}
                }
            }
        }
    }
}



/**
 * Adds the given entities to the container and draws the entities if
 * there's a graphic view available.
 *
 * @param addList Entities to add.
 */
void RS_Modification::addNewEntities(QList<RS_Entity*>& addList) {
    for (int i = 0; i < addList.size(); ++i) {
        /*        if (addList.at(i) == "Jane")
                    cout << "Found Jane at position " << i << endl;*/
        if (addList.at(i) != NULL) {
            container->addEntity(addList.at(i));
            if (document!=NULL && handleUndo) {
                document->addUndoable(addList.at(i));
            }
            //if (graphicView!=NULL) {
            //    graphicView->drawEntity(e);
            //}
        }
    }
}



/**
 * Trims or extends the given trimEntity to the intersection point of the
 * trimEntity and the limitEntity.
 *
 * @param trimCoord Coordinate which defines which endpoint of the
 *   trim entity to trim.
 * @param trimEntity Entity which will be trimmed.
 * @param limitCoord Coordinate which defines the intersection to which the
 *    trim entity will be trimmed.
 * @param limitEntity Entity to which the trim entity will be trimmed.
 * @param both true: Trim both entities. false: trim trimEntity only.
 */
bool RS_Modification::trim(const RS_Vector& trimCoord,
                           RS_AtomicEntity* trimEntity,
                           const RS_Vector& limitCoord,
                           RS_Entity* limitEntity,
                           bool both) {

    if (trimEntity==NULL || limitEntity==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::trim: At least one entity is NULL");
        return false;
    }

    if (both && !limitEntity->isAtomic()) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::trim: limitEntity is not atomic");
    }

    RS_VectorSolutions sol;
    if (limitEntity->isAtomic()) {
        // intersection(s) of the two entities:
        sol = RS_Information::getIntersection(trimEntity, limitEntity, false);
    } else if (limitEntity->isContainer()) {
        RS_EntityContainer* ec = (RS_EntityContainer*)limitEntity;

        sol.alloc(128);
        int i=0;

        for (RS_Entity* e=ec->firstEntity(RS2::ResolveAll); e!=NULL;
                e=ec->nextEntity(RS2::ResolveAll)) {
            //for (int i=0; i<container->count(); ++i) {
            //    RS_Entity* e = container->entityAt(i);

            if (e!=NULL) {

                RS_VectorSolutions s2 = RS_Information::getIntersection(trimEntity,
                                        e, false);

                if (s2.hasValid()) {
                    for (int k=0; k<s2.getNumber(); ++k) {
                        if (i<128 && s2.get(k).valid) {
                            if (e->isPointOnEntity(s2.get(k), 1.0e-4)) {
                                sol.set(i++, s2.get(k));
                            }
                        }
                    }
                    //break;
                }
            }
        }
    }

    if (sol.hasValid()==false) {
        return false;
    }

    RS_AtomicEntity* trimmed1 = NULL;
    RS_AtomicEntity* trimmed2 = NULL;

    if (trimEntity->rtti()==RS2::EntityCircle) {
        // convert a circle into a trimmable arc
        RS_Circle* c = (RS_Circle*)trimEntity;
        RS_ArcData d(c->getCenter(),
                     c->getRadius(),
                     0.,
                     2*M_PI,
                     false);
        trimmed1 = new RS_Arc(trimEntity->getParent(), d);
    } else {
        trimmed1 = (RS_AtomicEntity*)trimEntity->clone();
        trimmed1->setHighlighted(false);
    }

    // trim trim entity
    int ind = 0;
    RS_Vector is, is2;

    //RS2::Ending ending = trimmed1->getTrimPoint(trimCoord, is);
    if (
        trimEntity->rtti()==RS2::EntityEllipse
        || trimEntity->rtti()==RS2::EntityArc
        || trimEntity->rtti()==RS2::EntityCircle
        || trimEntity->rtti()==RS2::EntityLine
    ) {
        is = trimmed1->prepareTrim(trimCoord, sol);
    } else {
        is = sol.getClosest(limitCoord, NULL, &ind);
        //sol.getClosest(limitCoord, NULL, &ind);
        RS_DEBUG->print("RS_Modification::trim: limitCoord: %f/%f", limitCoord.x, limitCoord.y);
        RS_DEBUG->print("RS_Modification::trim: sol.get(0): %f/%f", sol.get(0).x, sol.get(0).y);
        RS_DEBUG->print("RS_Modification::trim: sol.get(1): %f/%f", sol.get(1).x, sol.get(1).y);
        RS_DEBUG->print("RS_Modification::trim: ind: %d", ind);
        is2 = sol.get(ind==0 ? 1 : 0);
        //RS_Vector is2 = sol.get(ind);
        RS_DEBUG->print("RS_Modification::trim: is2: %f/%f", is2.x, is2.y);

    }

    // remove trim entity from view:
    if (graphicView!=NULL) {
        graphicView->deleteEntity(trimEntity);
    }

    // remove limit entity from view:
    if (both) {
        trimmed2 = (RS_AtomicEntity*)limitEntity->clone();
        trimmed2->setHighlighted(false);
        if (graphicView!=NULL) {
            graphicView->deleteEntity(limitEntity);
        }
    }

    RS2::Ending ending = trimmed1->getTrimPoint(trimCoord, is);
    switch (ending) {
    case RS2::EndingStart:
        trimmed1->trimStartpoint(is);
        break;
    case RS2::EndingEnd:
        trimmed1->trimEndpoint(is);
        break;
    default:
        break;
    }

    // trim limit entity:
    if (both) {
        RS_Vector is = sol.getClosest(limitCoord);

        RS2::Ending ending = trimmed2->getTrimPoint(limitCoord, is);

        switch (ending) {
        case RS2::EndingStart:
            trimmed2->trimStartpoint(is);
            break;
        case RS2::EndingEnd:
            trimmed2->trimEndpoint(is);
            break;
        default:
            break;
        }
    }

    // add new trimmed trim entity:
    container->addEntity(trimmed1);
    if (graphicView!=NULL) {
        graphicView->drawEntity(trimmed1);
    }

    // add new trimmed limit entity:
    if (both) {
        container->addEntity(trimmed2);
        if (graphicView!=NULL) {
            graphicView->drawEntity(trimmed2);
        }
    }

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
        document->addUndoable(trimmed1);
        trimEntity->setUndoState(true);
        document->addUndoable(trimEntity);
        if (both) {
            document->addUndoable(trimmed2);
            limitEntity->setUndoState(true);
            document->addUndoable(limitEntity);
        }
        document->endUndoCycle();
    }

    return true;
}



/**
 * Trims or extends the given trimEntity by the given amount.
 *
 * @param trimCoord Coordinate which defines which endpoint of the
 *   trim entity to trim.
 * @param trimEntity Entity which will be trimmed.
 * @param dist Amount to trim by.
 */
bool RS_Modification::trimAmount(const RS_Vector& trimCoord,
                                 RS_AtomicEntity* trimEntity,
                                 double dist) {

    if (trimEntity==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::trimAmount: Entity is NULL");
        return false;
    }

    RS_AtomicEntity* trimmed = NULL;

    // remove trim entity:
    trimmed = (RS_AtomicEntity*)trimEntity->clone();
    if (graphicView!=NULL) {
        graphicView->deleteEntity(trimEntity);
    }

    // trim trim entity
    RS_Vector is = trimmed->getNearestDist(-dist, trimCoord);
    if (trimCoord.distanceTo(trimmed->getStartpoint()) <
            trimCoord.distanceTo(trimmed->getEndpoint())) {
        trimmed->trimStartpoint(is);
    } else {
        trimmed->trimEndpoint(is);
    }

    // add new trimmed trim entity:
    container->addEntity(trimmed);

    if (graphicView!=NULL) {
        graphicView->drawEntity(trimmed);
    }

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
        document->addUndoable(trimmed);
        trimEntity->setUndoState(true);
        document->addUndoable(trimEntity);
        document->endUndoCycle();
    }

    return true;
}



/**
 * Cuts the given entity at the given point.
 */
bool RS_Modification::cut(const RS_Vector& cutCoord,
                          RS_AtomicEntity* cutEntity) {

    if (cutEntity==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::cut: Entity is NULL");
        return false;
    }

    if (!cutCoord.valid) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::cut: Point invalid.");
        return false;
    }

    // cut point is at endpoint of entity:
    if (cutCoord.distanceTo(cutEntity->getStartpoint())<1.0e-6 ||
            cutCoord.distanceTo(cutEntity->getEndpoint())<1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::cut: Cutting point on endpoint");
        return false;
    }

    // delete cut entity on the screen:
    if (graphicView!=NULL) {
        graphicView->deleteEntity(cutEntity);
    }

    RS_AtomicEntity* cut1 = NULL;
    RS_AtomicEntity* cut2 = NULL;

    // create new two halves:
    if (cutEntity->rtti()==RS2::EntityCircle) {
        RS_Circle* c = (RS_Circle*)cutEntity;
        cut1 = new RS_Arc(cutEntity->getParent(),
                          RS_ArcData(c->getCenter(),
                                     c->getRadius(),
                                     0.0,0.0, false));
        cut1->setPen(cutEntity->getPen(false));
        cut1->setLayer(cutEntity->getLayer(false));
        cut2 = NULL;

        cut1->trimEndpoint(cutCoord);
        cut1->trimStartpoint(cutCoord);
    } else {
        cut1 = (RS_AtomicEntity*)cutEntity->clone();
        cut2 = (RS_AtomicEntity*)cutEntity->clone();

        cut1->trimEndpoint(cutCoord);
        cut2->trimStartpoint(cutCoord);
    }

    // add new cut entity:
    container->addEntity(cut1);
    if (cut2!=NULL) {
        container->addEntity(cut2);
    }

    if (graphicView!=NULL) {
        graphicView->drawEntity(cut1);
        if (cut2!=NULL) {
            graphicView->drawEntity(cut2);
        }
    }

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
        document->addUndoable(cut1);
        if (cut2!=NULL) {
            document->addUndoable(cut2);
        }
        cutEntity->setUndoState(true);
        document->addUndoable(cutEntity);
        document->endUndoCycle();
    }

    return true;
}



/**
 * Stretching.
 */
bool RS_Modification::stretch(const RS_Vector& firstCorner,
                              const RS_Vector& secondCorner,
                              const RS_Vector& offset) {

    if (!offset.valid) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::stretch: Offset invalid");
        return false;
    }

    QList<RS_Entity*> addList;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    // Create new entites
    for (RS_Entity* e=container->firstEntity();
            e!=NULL;
            e=container->nextEntity()) {
        if (e!=NULL &&
                e->isVisible() &&
                !e->isLocked() ) {
//            &&
            if (  (e->isInWindow(firstCorner, secondCorner) ||
                    e->hasEndpointsWithinWindow(firstCorner, secondCorner))) {

                RS_Entity* ec = e->clone();
                ec->stretch(firstCorner, secondCorner, offset);
                addList.append(ec);
                e->setSelected(true);
            }
        }
    }

    deselectOriginals(true);
    addNewEntities(addList);

    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
    }

    if (graphicView!=NULL) {
        graphicView->redraw(RS2::RedrawDrawing);
    }
    return true;
}



/**
 * Bevels a corner.
 *
 * @param coord1 Mouse coordinate to specify direction from intersection.
 * @param entity1 First entity of the corner.
 * @param coord2 Mouse coordinate to specify direction from intersection.
 * @param entity2 Second entity of the corner.
 * @param data Lengths and trim flag.
 */
bool RS_Modification::bevel(const RS_Vector& coord1, RS_AtomicEntity* entity1,
                            const RS_Vector& coord2, RS_AtomicEntity* entity2,
                            RS_BevelData& data) {

    RS_DEBUG->print("RS_Modification::bevel");

    if (entity1==NULL || entity2==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::bevel: At least one entity is NULL");
        return false;
    }

    RS_EntityContainer* baseContainer = container;
    bool isPolyline = false;
    bool isClosedPolyline = false;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    // find out whether we're bevelling within a polyline:
    if (entity1->getParent()!=NULL && entity1->getParent()->rtti()==RS2::EntityPolyline) {
        RS_DEBUG->print("RS_Modification::bevel: trimming polyline segments");
        if (entity1->getParent()!=entity2->getParent()) {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "RS_Modification::bevel: entities not in the same polyline");
            return false;
        }
        // clone polyline for undo
        if (document!=NULL && handleUndo) {
            RS_EntityContainer* cl =
                (RS_EntityContainer*)entity1->getParent()->clone();
            container->addEntity(cl);
            //cl->setUndoState(true);
            document->addUndoable(cl);

            document->addUndoable(entity1->getParent());
            entity1->getParent()->setUndoState(true);

            baseContainer = cl;
        }

        entity1 = (RS_AtomicEntity*)baseContainer->entityAt(entity1->getParent()->findEntity(entity1));
        entity2 = (RS_AtomicEntity*)baseContainer->entityAt(entity2->getParent()->findEntity(entity2));

        //baseContainer = entity1->getParent();
        isPolyline = true;
        isClosedPolyline = ((RS_Polyline*)entity1)->isClosed();
    }

    RS_DEBUG->print("RS_Modification::bevel: getting intersection");

    RS_VectorSolutions sol =
        RS_Information::getIntersection(entity1, entity2, false);

    if (sol.getNumber()==0) {
        return false;
    }

    RS_AtomicEntity* trimmed1 = NULL;
    RS_AtomicEntity* trimmed2 = NULL;

    //if (data.trim || isPolyline) {
    if (isPolyline) {
        trimmed1 = entity1;
        trimmed2 = entity2;
    } else {
        trimmed1 = (RS_AtomicEntity*)entity1->clone();
        trimmed2 = (RS_AtomicEntity*)entity2->clone();
    }

    // remove trim entity (on screen):
    if (data.trim==true || isPolyline) {
        if (graphicView!=NULL) {
            if (isPolyline) {
                graphicView->deleteEntity(baseContainer);
            } else {
                graphicView->deleteEntity(entity1);
                graphicView->deleteEntity(entity2);
            }
        }
    }

    // trim entities to intersection
    RS_DEBUG->print("RS_Modification::bevel: trim entities to intersection 01");
    bool start1 = false;
    RS_Vector is = sol.getClosest(coord2);
    RS2::Ending ending1 = trimmed1->getTrimPoint(coord1, is);
    switch (ending1) {
    case RS2::EndingStart:
        trimmed1->trimStartpoint(is);
        start1 = true;
        break;
    case RS2::EndingEnd:
        trimmed1->trimEndpoint(is);
        start1 = false;
        break;
    default:
        break;
    }

    RS_DEBUG->print("RS_Modification::bevel: trim entities to intersection 02");
    bool start2 = false;
    is = sol.getClosest(coord1);
    RS2::Ending ending2 = trimmed2->getTrimPoint(coord2, is);
    switch (ending2) {
    case RS2::EndingStart:
        trimmed2->trimStartpoint(is);
        start2 = true;
        break;
    case RS2::EndingEnd:
        trimmed2->trimEndpoint(is);
        start2 = false;
        break;
    default:
        break;
    }
    //}


    // find definitive bevel points
    RS_DEBUG->print("RS_Modification::bevel: find definitive bevel points");
    RS_Vector bp1 = trimmed1->getNearestDist(data.length1, start1);
    RS_Vector bp2 = trimmed2->getNearestDist(data.length2, start2);

    // final trim:
    RS_DEBUG->print("RS_Modification::bevel: final trim");
    if (data.trim==true) {
        switch (ending1) {
        case RS2::EndingStart:
            trimmed1->trimStartpoint(bp1);
            break;
        case RS2::EndingEnd:
            trimmed1->trimEndpoint(bp1);
            break;
        default:
            break;
        }

        switch (ending2) {
        case RS2::EndingStart:
            trimmed2->trimStartpoint(bp2);
            break;
        case RS2::EndingEnd:
            trimmed2->trimEndpoint(bp2);
            break;
        default:
            break;
        }

        // add new trimmed entities:
        if (isPolyline==false) {
            container->addEntity(trimmed1);
            container->addEntity(trimmed2);
        }
        if (graphicView!=NULL) {
            if (!isPolyline) {
                graphicView->drawEntity(trimmed1);
                graphicView->drawEntity(trimmed2);
            }
        }
    }


    // add bevel line:
    RS_DEBUG->print("RS_Modification::bevel: add bevel line");
    RS_Line* bevel = new RS_Line(baseContainer, RS_LineData(bp1, bp2));

    if (isPolyline==false) {
        baseContainer->addEntity(bevel);
    } else {
        int idx1 = baseContainer->findEntity(trimmed1);
        int idx2 = baseContainer->findEntity(trimmed2);

        bevel->setSelected(baseContainer->isSelected());
        bevel->setLayer(baseContainer->getLayer());
        bevel->setPen(baseContainer->getPen());

        bool insertAfter1 = false;
        if (!isClosedPolyline) {
            insertAfter1 = (idx1<idx2);
        }
        else {
            insertAfter1 = ((idx1<idx2 && idx1!=0) ||
                            (idx2==0 && idx1==(int)baseContainer->count()-1));
        }

        // insert bevel at the right position:
        //if ((idx1<idx2 && idx1!=0) ||
        //	(idx2==0 && idx1==(int)baseContainer->count()-1)) {
        if (insertAfter1) {
            if (trimmed1->getEndpoint().distanceTo(bevel->getStartpoint())>1.0e-4) {
                bevel->reverse();
            }
            baseContainer->insertEntity(idx1+1, bevel);
        } else {
            if (trimmed2->getEndpoint().distanceTo(bevel->getStartpoint())>1.0e-4) {
                bevel->reverse();
            }
            baseContainer->insertEntity(idx2+1, bevel);
        }
    }

    if (isPolyline) {
        ((RS_Polyline*)baseContainer)->updateEndpoints();
    }

    if (graphicView!=NULL) {
        if (isPolyline) {
            graphicView->drawEntity(baseContainer);
        } else {
            graphicView->drawEntity(bevel);
        }
    }

    RS_DEBUG->print("RS_Modification::bevel: handling undo");

    if (document!=NULL && handleUndo) {
        //document->startUndoCycle();

        if (isPolyline==false && data.trim==true) {
            document->addUndoable(trimmed1);
            entity1->setUndoState(true);
            document->addUndoable(entity1);

            document->addUndoable(trimmed2);
            entity2->setUndoState(true);
            document->addUndoable(entity2);
        }

        if (isPolyline==false) {
            document->addUndoable(bevel);
        }

        document->endUndoCycle();
    }

    if (data.trim==false) {
        RS_DEBUG->print("RS_Modification::bevel: delete trimmed elements");
        delete trimmed1;
        delete trimmed2;
        RS_DEBUG->print("RS_Modification::bevel: delete trimmed elements: ok");
    }

    return true;

}



/**
 * Rounds a corner.
 *
 * @param coord Mouse coordinate to specify the rounding.
 * @param entity1 First entity of the corner.
 * @param entity2 Second entity of the corner.
 * @param data Radius and trim flag.
 */
bool RS_Modification::round(const RS_Vector& coord,
                            const RS_Vector& coord1,
                            RS_AtomicEntity* entity1,
                            const RS_Vector& coord2,
                            RS_AtomicEntity* entity2,
                            RS_RoundData& data) {

    if (entity1==NULL || entity2==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::round: At least one entity is NULL");
        return false;
    }

    RS_EntityContainer* baseContainer = container;
    bool isPolyline = false;
    bool isClosedPolyline = false;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    // find out whether we're rounding within a polyline:
    if (entity1->getParent()!=NULL &&
            entity1->getParent()->rtti()==RS2::EntityPolyline) {

        if (entity1->getParent()!=entity2->getParent()) {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "RS_Modification::round: entities not in "
                            "the same polyline");
            if (document!=NULL && handleUndo) {
                document->endUndoCycle();
            }
            return false;
        }

        // clone polyline for undo
        if (document!=NULL && handleUndo) {
            RS_EntityContainer* cl =
                (RS_EntityContainer*)entity1->getParent()->clone();
            container->addEntity(cl);
            document->addUndoable(cl);

            document->addUndoable(entity1->getParent());
            entity1->getParent()->setUndoState(true);

            baseContainer = cl;
        }

        entity1 = (RS_AtomicEntity*)baseContainer->entityAt(entity1->getParent()->findEntity(entity1));
        entity2 = (RS_AtomicEntity*)baseContainer->entityAt(entity2->getParent()->findEntity(entity2));

        isPolyline = true;
        isClosedPolyline = ((RS_Polyline*)entity1)->isClosed();
    }

    // create 2 tmp parallels
    RS_Creation creation(NULL, NULL);
    RS_Entity* par1 = creation.createParallel(coord, data.radius, 1, entity1);
    RS_Entity* par2 = creation.createParallel(coord, data.radius, 1, entity2);

    RS_VectorSolutions sol2 =
        RS_Information::getIntersection(entity1, entity2, false);

    RS_VectorSolutions sol =
        RS_Information::getIntersection(par1, par2, false);

    if (sol.getNumber()==0) {
        if (document!=NULL && handleUndo) {
            document->endUndoCycle();
        }
        return false;
    }

    // there might be two intersections: choose the closest:
    RS_Vector is = sol.getClosest(coord);
    RS_Vector p1 = entity1->getNearestPointOnEntity(is, false);
    RS_Vector p2 = entity2->getNearestPointOnEntity(is, false);
    double ang1 = is.angleTo(p1);
    double ang2 = is.angleTo(p2);
    bool reversed = (RS_Math::getAngleDifference(ang1, ang2)>M_PI);

    RS_Arc* arc = new RS_Arc(baseContainer,
                             RS_ArcData(is,
                                        data.radius,
                                        ang1, ang2,
                                        reversed));


    RS_AtomicEntity* trimmed1 = NULL;
    RS_AtomicEntity* trimmed2 = NULL;

    if (data.trim || isPolyline) {
        if (isPolyline) {
            trimmed1 = entity1;
            trimmed2 = entity2;
        } else {
            trimmed1 = (RS_AtomicEntity*)entity1->clone();
            trimmed2 = (RS_AtomicEntity*)entity2->clone();
        }

        // remove trim entity:
        if (graphicView!=NULL) {
            if (isPolyline) {
                graphicView->deleteEntity(baseContainer);
            } else {
                graphicView->deleteEntity(entity1);
                graphicView->deleteEntity(entity2);
            }
        }

        // trim entities to intersection
        RS_Vector is2 = sol2.getClosest(coord2);
        RS2::Ending ending1 = trimmed1->getTrimPoint(coord1, is2);
        switch (ending1) {
        case RS2::EndingStart:
            trimmed1->trimStartpoint(p1);
            break;
        case RS2::EndingEnd:
            trimmed1->trimEndpoint(p1);
            break;
        default:
            break;
        }

        is2 = sol2.getClosest(coord1);
        RS2::Ending ending2 = trimmed2->getTrimPoint(coord2, is2);
        switch (ending2) {
        case RS2::EndingStart:
            trimmed2->trimStartpoint(p2);
            break;
        case RS2::EndingEnd:
            trimmed2->trimEndpoint(p2);
            break;
        default:
            break;
        }

        // add new trimmed entities:
        if (isPolyline==false) {
            container->addEntity(trimmed1);
            container->addEntity(trimmed2);
        }
        if (graphicView!=NULL) {
            if (!isPolyline) {
                graphicView->drawEntity(trimmed1);
                graphicView->drawEntity(trimmed2);
            }
        }
    }

    // add rounding:
    if (isPolyline==false) {
        baseContainer->addEntity(arc);
    } else {
        // find out which base entity is before the rounding:
        int idx1 = baseContainer->findEntity(trimmed1);
        int idx2 = baseContainer->findEntity(trimmed2);

        arc->setSelected(baseContainer->isSelected());
        arc->setLayer(baseContainer->getLayer());
        arc->setPen(baseContainer->getPen());

        RS_DEBUG->print("RS_Modification::round: idx1<idx2: %d", (int)(idx1<idx2));
        RS_DEBUG->print("RS_Modification::round: idx1!=0: %d", (int)(idx1!=0));
        RS_DEBUG->print("RS_Modification::round: idx2==0: %d", (int)(idx2==0));
        RS_DEBUG->print("RS_Modification::round: idx1==(int)baseContainer->count()-1: %d",
                        (int)(idx1==(int)baseContainer->count()-1));

        bool insertAfter1 = false;
        if (!isClosedPolyline) {
            insertAfter1 = (idx1<idx2);
        }
        else {
            insertAfter1 = ((idx1<idx2 && idx1!=0) ||
                            (idx2==0 && idx1==(int)baseContainer->count()-1));
        }

        // insert rounding at the right position:
        //if ((idx1<idx2 && idx1!=0) ||
        //	(idx2==0 && idx1==(int)baseContainer->count()-1)) {
        //if (idx1<idx2) {
        if (insertAfter1) {
            if (trimmed1->getEndpoint().distanceTo(arc->getStartpoint())>1.0e-4) {
                arc->reverse();
            }
            baseContainer->insertEntity(idx1+1, arc);
        } else {
            if (trimmed2->getEndpoint().distanceTo(arc->getStartpoint())>1.0e-4) {
                arc->reverse();
            }
            baseContainer->insertEntity(idx2+1, arc);
        }
    }

    if (isPolyline) {
        ((RS_Polyline*)baseContainer)->updateEndpoints();
    }

    if (graphicView!=NULL) {
        if (isPolyline) {
            graphicView->drawEntity(baseContainer);
        } else {
            graphicView->drawEntity(arc);
        }
    }

    if (document!=NULL && handleUndo) {
        if (isPolyline==false && data.trim==true) {
            document->addUndoable(trimmed1);
            entity1->setUndoState(true);
            document->addUndoable(entity1);

            document->addUndoable(trimmed2);
            entity2->setUndoState(true);
            document->addUndoable(entity2);
        }

        if (isPolyline==false) {
            document->addUndoable(arc);
        }

        document->endUndoCycle();
    }

    delete par1;
    delete par2;

    return true;
}



/**
 * Removes the selected entity containers and adds the entities in them as
 * new single entities.
 */
bool RS_Modification::explode() {

    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::explode: no valid container"
                        " for addinge entities",
                        RS_Debug::D_WARNING);
        return false;
    }

    QList<RS_Entity*> addList;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    for (RS_Entity* e=container->firstEntity();
            e!=NULL;
            e=container->nextEntity()) {
        //for (uint i=0; i<container->count(); ++i) {
        //RS_Entity* e = container->entityAt(i);

        if (e!=NULL && e->isSelected()) {
            if (e->isContainer()) {

                // add entities from container:
                RS_EntityContainer* ec = (RS_EntityContainer*)e;
                //ec->setSelected(false);

                // iterate and explode container:
                //for (uint i2=0; i2<ec->count(); ++i2) {
                //    RS_Entity* e2 = ec->entityAt(i2);
                RS2::ResolveLevel rl;
                bool resolvePen;
                bool resolveLayer;

                switch (ec->rtti()) {
                case RS2::EntityText:
                case RS2::EntityHatch:
                case RS2::EntityPolyline:
                    rl = RS2::ResolveAll;
                    resolveLayer = true;
                    resolvePen = false;
                    break;

                case RS2::EntityInsert:
                    resolvePen = false;
                    resolveLayer = false;
                    rl = RS2::ResolveNone;
                    break;

                case RS2::EntityDimAligned:
                case RS2::EntityDimLinear:
                case RS2::EntityDimRadial:
                case RS2::EntityDimDiametric:
                case RS2::EntityDimAngular:
                case RS2::EntityDimLeader:
                    rl = RS2::ResolveNone;
                    resolveLayer = true;
                    resolvePen = false;
                    break;

                default:
                    rl = RS2::ResolveAll;
                    resolveLayer = true;
                    resolvePen = false;
                    break;
                }

                for (RS_Entity* e2 = ec->firstEntity(rl); e2!=NULL;
                        e2 = ec->nextEntity(rl)) {

                    if (e2!=NULL) {
                        RS_Entity* clone = e2->clone();
                        clone->setSelected(false);
                        clone->reparent(container);

                        if (resolveLayer) {
                            clone->setLayer(ec->getLayer());
                        } else {
                            clone->setLayer(e2->getLayer());
                        }

                        clone->setPen(ec->getPen(resolvePen));

                        addList.append(clone);

                        clone->update();
                    }
                }
            } else {
                e->setSelected(false);
            }
        }
    }

    deselectOriginals(true);
    addNewEntities(addList);

    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
    }

    if (graphicView!=NULL) {
        graphicView->redraw(RS2::RedrawDrawing);
    }

    return true;
}



bool RS_Modification::explodeTextIntoLetters() {
    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::explodeTextIntoLetters: no valid container"
                        " for addinge entities",
                        RS_Debug::D_WARNING);
        return false;
    }

    QList<RS_Entity*> addList;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    for (RS_Entity* e=container->firstEntity();
            e!=NULL;
            e=container->nextEntity()) {
        if (e!=NULL && e->isSelected()) {
            if (e->rtti()==RS2::EntityText) {
                // add letters of text:
                RS_Text* text = (RS_Text*)e;
                explodeTextIntoLetters(text, addList);
            } else {
                e->setSelected(false);
            }
        }
    }

    deselectOriginals(true);
    addNewEntities(addList);

    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
    }

    if (graphicView!=NULL) {
        graphicView->redraw(RS2::RedrawDrawing);
    }

    return true;
}



bool RS_Modification::explodeTextIntoLetters(RS_Text* text, QList<RS_Entity*>& addList) {

    if (text==NULL) {
        return false;
    }

    // iterate though lines:
    for (RS_Entity* e2 = text->firstEntity(); e2!=NULL;
            e2 = text->nextEntity()) {

        if (e2==NULL) {
            break;
        }


        // text lines:
        if (e2->rtti()==RS2::EntityContainer) {

            RS_EntityContainer* line = (RS_EntityContainer*)e2;

            // iterate though letters:
            for (RS_Entity* e3 = line->firstEntity(); e3!=NULL;
                    e3 = line->nextEntity()) {

                if (e3==NULL) {
                    break;
                }

                // super / sub texts:
                if (e3->rtti()==RS2::EntityText) {
                    explodeTextIntoLetters((RS_Text*)e3, addList);
                }

                // normal letters:
                else if (e3->rtti()==RS2::EntityInsert) {

                    RS_Insert* letter = (RS_Insert*)e3;

                    RS_Text* tl = new RS_Text(
                        container,
                        RS_TextData(letter->getInsertionPoint(),
                                    text->getHeight(),
                                    100.0,
                                    RS2::VAlignBottom, RS2::HAlignLeft,
                                    RS2::LeftToRight, RS2::Exact,
                                    1.0,
                                    letter->getName(),
                                    text->getStyle(),
                                    letter->getAngle(),
                                    RS2::Update));

                    tl->setLayer(text->getLayer());
                    tl->setPen(text->getPen());

                    addList.append(tl);
                    tl->update();
                }
            }
        }
    }

    return true;
}



/**
 * Moves all reference points of selected entities with the given data.
 */
bool RS_Modification::moveRef(RS_MoveRefData& data) {
    if (container==NULL) {
        RS_DEBUG->print("RS_Modification::moveRef: no valid container",
                        RS_Debug::D_WARNING);
        return false;
    }

    QList<RS_Entity*> addList;

    if (document!=NULL && handleUndo) {
        document->startUndoCycle();
    }

    // Create new entites
    for (RS_Entity* e=container->firstEntity();
            e!=NULL;
            e=container->nextEntity()) {
        if (e!=NULL && e->isSelected()) {
            RS_Entity* ec = e->clone();

            ec->moveRef(data.ref, data.offset);
            // since 2.0.4.0: keep it selected
            ec->setSelected(true);
            addList.append(ec);
        }
    }

    deselectOriginals(true);
    addNewEntities(addList);

    if (document!=NULL && handleUndo) {
        document->endUndoCycle();
    }

    if (graphicView!=NULL) {
        graphicView->redraw(RS2::RedrawDrawing);
    }
    return true;
}

// EOF
