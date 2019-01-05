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
#include<cmath>
#include "rs_modification.h"

#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_graphicview.h"
#include "rs_clipboard.h"
#include "rs_creation.h"
#include "rs_graphic.h"
#include "rs_information.h"
#include "rs_insert.h"
#include "rs_block.h"
#include "rs_polyline.h"
#include "rs_mtext.h"
#include "rs_text.h"
#include "rs_layer.h"
#include "lc_splinepoints.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "lc_undosection.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

RS_PasteData::RS_PasteData(RS_Vector _insertionPoint,
		double _factor,
		double _angle,
		bool _asInsert,
		const QString& _blockName):
		insertionPoint(_insertionPoint)
		,factor(_factor)
		,angle(_angle)
		,asInsert(_asInsert)
		,blockName(_blockName)
{
}

/**
 * Default constructor.
 *
 * @param container The container to which we will add
 *        entities. Usually that's an RS_Graphic entity but
 *        it can also be a polyline, text, ...
 * @param graphicView Pointer to graphic view or nullptr if you don't want the
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
void RS_Modification::remove() {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::remove");

	if (!container) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::remove: no valid container");
        return;
    }

    LC_UndoSection undo( document);
	// not safe (?)
    for(auto e: *container) {
        if (e && e->isSelected()) {
            e->setSelected(false);
            e->changeUndoState();
            undo.addUndoable(e);
        } else {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::remove: no valid container is selected");
        }
    }

    graphicView->redraw(RS2::RedrawDrawing);

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::remove: OK");
}



/**
 * Revert direction of selected entities.
 */
void RS_Modification::revertDirection() {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::revertDirection");

	if (!container) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::revertDirection: no valid container");
		return;
	}

	std::vector<RS_Entity*> addList;
    for(auto e: *container) {
		if (e && e->isSelected()) {
			RS_Entity* ec = e->clone();
			ec->revertDirection();
			addList.push_back(ec);
        } else {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::revertDirection: no valid container is selected");
        }
	}

    LC_UndoSection undo( document, handleUndo); // bundle remove/add entities in one undoCycle
    deselectOriginals(true);
	addNewEntities(addList);

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::revertDirection: OK");
}



/**
 * Changes the attributes of container sub-entities. Recursive
 */
bool RS_Modification::changeAttributes(RS_AttributesData& data, RS_EntityContainer* container, std::vector<RS_Entity*> addList) {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::changeAttributes");

    if (!container) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::changeAttributes: no valid container");
        return false;
    }

    for(auto e: *container) {

        if (!e) {
            RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::changeAttributes: nullptr in container");
            return false;
        }

        e->setSelected(false);
        RS_Pen pen = e->getPen(false);

        if (data.changeLayer==true) {
            e->setLayer(data.layer);
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
        e->setPen(pen);

        if (e->isContainer()) {
            if (e->rtti() == RS2::EntityInsert) {
                RS_Block* eb = static_cast<RS_Insert*>(e)->getBlockForInsert();
                changeAttributes(data, (RS_EntityContainer*)eb, addList);
            } else {
                changeAttributes(data, (RS_EntityContainer*)e, addList);
            }
        }
        e->update();
    }

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::changeAttributes: OK");
    return true;
}



/**
 * Changes the attributes of all selected
 */
bool RS_Modification::changeAttributes(RS_AttributesData& data) {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::changeAttributes");

    if (!container) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::changeAttributes: no valid container");
        return false;
    }

    LC_UndoSection  undo(document);
    std::vector<RS_Entity*> addList;

    for(auto e: *container) {
//        for (unsigned i=0; i<container->count(); ++i) {
//        RS_Entity* e = container->entityAt(i);
        if (e && e->isSelected()) {

            e->setSelected(false);
            RS_Pen pen = e->getPen(false);

            if (data.changeLayer==true) {
                e->setLayer(data.layer);
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
            e->setPen(pen);

            if (e->isContainer()) {
                if (e->rtti() == RS2::EntityInsert) {
                    RS_Block* eb = static_cast<RS_Insert*>(e)->getBlockForInsert();
                    changeAttributes(data, (RS_EntityContainer*)eb, addList);
                } else {
                    changeAttributes(data, (RS_EntityContainer*)e, addList);
                }
            }

            e->update();

            //if (data.useCurrentLayer) {
            //    ec->setLayerToActive();
            //}
            //if (data.useCurrentAttributes) {
            //    ec->setPenToActive();
            //}
            //if (ec->rtti()==RS2::EntityInsert) {
            //    ((RS_Insert*)ec)->update();
            //}
        } else {
            RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Modification::changeAttributes: no valid container is selected");
        }
    }

    deselectOriginals(true);

    if (graphicView) {
        graphicView->redraw(RS2::RedrawDrawing);
    }

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::changeAttributes: OK");
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

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copy");

	if (!container) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::copy: no valid container");
        return;
    }

    RS_CLIPBOARD->clear();
    if (graphic) {
        RS_CLIPBOARD->getGraphic()->setUnit(graphic->getUnit());
    } else {
        RS_CLIPBOARD->getGraphic()->setUnit(RS2::None);
    }

    // start undo cycle for the container if we're cutting
    LC_UndoSection undo( document, cut && handleUndo);

	// copy entities / layers / blocks
	for(auto e: *container){
        //for (unsigned i=0; i<container->count(); ++i) {
        //RS_Entity* e = container->entityAt(i);
        if (e && e->isSelected()) {
            copyEntity(e, ref, cut);
        } else {
            RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Modification::copy: no valid container is selected");
        }
    }

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copy: OK");
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
void RS_Modification::copyEntity(RS_Entity* e, const RS_Vector& ref, const bool cut) {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyEntity");

    if (!e || !e->isSelected()) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::copyEntity: no entity is selected");
        return;
    }

    // add entity to clipboard:
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyEntity: to clipboard: %d/%d", e->getId(), e->rtti());
    RS_Entity* c = e->clone();
    c->move(-ref);

    RS_CLIPBOARD->addEntity(c);
    copyLayers(e);
    copyBlocks(e);

    // set layer to the layer clone:
    c->setLayer(e->getLayer()->getName());

    if (cut) {
        LC_UndoSection undo( document);
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyEntity: cut ID/flag: %d/%d", e->getId(), e->rtti());
        e->changeUndoState();
        undo.addUndoable(e);

        // delete entity in graphic view:
        if (graphicView) {
            graphicView->deleteEntity(e);
        }
        e->setSelected(false);
    } else {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyEntity: delete in view ID/flag: %d/%d", e->getId(), e->rtti());
        // delete entity in graphic view:
        if (graphicView) {
            graphicView->deleteEntity(e);
        }
        e->setSelected(false);
        if (graphicView) {
            graphicView->drawEntity(e);
        }
    }

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyEntity: OK");
}



/**
 * Copies all layers of the given entity to the clipboard.
 */
void RS_Modification::copyLayers(RS_Entity* e) {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyLayers");

	if (!e) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::copyLayers: no entity is selected");
        return;
    }

    // add layer(s) of the entity insert can also be into any layer
    RS_Layer* l = e->getLayer();
    if (!l) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::copyLayers: no valid layer found");
        return;
    }

    if (!RS_CLIPBOARD->hasLayer(l->getName())) {
        RS_CLIPBOARD->addLayer(l->clone());
    }

    // special handling of inserts:
    if (e->rtti()==RS2::EntityInsert) {
        // insert: add layer(s) of subentities:
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyLayers: copy insert entity ID/flag layers: %d/%d", e->getId(), e->rtti());
        RS_Block* b = ((RS_Insert*)e)->getBlockForInsert();
        if (!b) {
            RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::copyLayers: could not find block for insert entity");
            return;
        }
        for(auto e2: *b) {
            //for (unsigned i=0; i<b->count(); ++i) {
            //RS_Entity* e2 = b->entityAt(i);
            copyLayers(e2);
        }
    } else {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyLayers: skip noninsert entity");
    }

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyLayers: OK");
}



/**
 * Copies all blocks of the given entity to the clipboard.
 */
void RS_Modification::copyBlocks(RS_Entity* e) {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyBlocks");

	if (!e) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::copyBlocks: no entity to process");
        return;
    }

    // add block of the entity only if it's an insert
    if (e->rtti()!=RS2::EntityInsert) {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyBlocks: skip non-insert entity");
        return;
    }

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyBlocks: get insert entity ID/flag block: %d/%d", e->getId(), e->rtti());
    RS_Block* b = ((RS_Insert*)e)->getBlockForInsert();
    if (!b) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::copyBlocks: could not find block for insert entity");
        return;
    }
    // add block of an insert
    QString bn = b->getName();
    if (!RS_CLIPBOARD->hasBlock(bn)) {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyBlocks: add block name: %s", bn.toLatin1().data());
        RS_CLIPBOARD->addBlock((RS_Block*)b->clone());
    }
    //find insert into insert
    for(auto e2: *b) {
        //call copyBlocks only if entity are insert
        if (e2->rtti()==RS2::EntityInsert) {
            RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyBlocks: process insert-into-insert blocks for %d/%d", e2->getId(), e2->rtti());
            copyBlocks(e2);
        }
    }

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyBlocks: OK");
}



/**
 * Pastes all entities from the clipboard into the container.
 * Layers and blocks that are needed are also copied if the container is
 * or is part of an RS_Graphic.
 *
 * @param data Paste data.
 * @param source The source from where to paste. nullptr means the source
 *      is the clipboard.
 */
void RS_Modification::paste(const RS_PasteData& data, RS_Graphic* source) {

    RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, "RS_Modification::paste");

	if (!graphic) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::paste: graphic is nullptr");
        return;
    }

    // adjust scaling factor for units conversion in case of clipboard paste
    double factor = (RS_TOLERANCE < fabs(data.factor)) ? data.factor : 1.0;
    // scale factor as vector
    RS_Vector vfactor = RS_Vector(factor, factor);
    // select source for paste
	if (!source) {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::paste: add graphic source from clipboard");
        source = RS_CLIPBOARD->getGraphic();
        // graphics from the clipboard need to be scaled. From the part lib not:
        RS2::Unit sourceUnit = source->getUnit();
        RS2::Unit targetUnit = graphic->getUnit();
        factor = RS_Units::convert(1.0, sourceUnit, targetUnit);
        vfactor = RS_Vector(factor, factor);
    } else {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::paste: add graphic source from parts library");
    }
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::paste: pasting scale factor: %d", factor);

    // default insertion point for container
    RS_Vector ip = data.insertionPoint;

    // remember active layer before inserting absent layers
    RS_Layer *l = graphic->getActiveLayer();

    // insert absent layers from source to graphic
    if (!pasteLayers(source)) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::paste: unable to copy due to absence of needed layers");
        return;
    }

    // select the same layer in graphic as in source
    /*
    auto a_layer = source->getActiveLayer();
    if (!a_layer)
    {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::paste: copy wasn't properly finalized");
        return;
    }
    QString ln = a_layer->getName();
    RS_Layer* l = graphic->getLayerList()->find(ln);
    */
    if (!l) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::paste: unable to select layer to paste in");
        return;
    }
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::paste: selected layer: %s", l->getName().toLatin1().data());
    graphic->activateLayer(l);

    // hash for renaming duplicated blocks
    QHash<QString, QString> blocksDict;

    // create block to paste entities as a whole
    QString name_old = "paste-block";
    if (data.blockName != NULL) {
        name_old = data.blockName;
    }
    QString name_new = name_old;
    if (graphic->findBlock(name_old)) {
        name_new = graphic->getBlockList()->newName(name_old);
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::paste: paste block name: %s", name_new.toLatin1().data());
    }
    blocksDict[name_old] = name_new;

    // create block
    RS_BlockData db = RS_BlockData(name_new, RS_Vector(0.0, 0.0), false);
    RS_Block* b = new RS_Block(graphic, db);
    b->reparent(graphic);
    graphic->addBlock(b);

    // create insert object for the paste block
    RS_InsertData di = RS_InsertData(b->getName(), ip, vfactor, data.angle, 1, 1, RS_Vector(0.0,0.0));
    RS_Insert* i = new RS_Insert(graphic, di);
    i->setLayerToActive();
    i->setPenToActive();
    i->reparent(graphic);
    graphic->addEntity(i);

    // copy sub- blocks, inserts and entities from source to the paste block
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::paste: copy content to the paste block");
    for(auto e: * static_cast<RS_EntityContainer*>(source)) {

        if (!e) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::paste: nullptr entity in source");
            continue;
        }

        // paste subcontainers
        if (e->rtti() == RS2::EntityInsert) {
            if (!pasteContainer(e, b, blocksDict, RS_Vector(0.0, 0.0))) {
                RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::paste: unable to paste due to subcontainer paste error");
                return;
            }
            // clear selection due to the following processing of selected entities
            e->setSelected(false);
        } else {
            // paste individual entities including Polylines, etc.
            if (!pasteEntity(e, b)) {
                RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::paste: unable to paste due to entity paste error");
                return;
            }
            // clear selection due to the following processing of selected entities
            e->setSelected(false);
        }
    }

    // update insert
    i->update();
    i->setSelected(false);

    // unblock all entities if not pasting as a new block by demand
    LC_UndoSection undo( document, handleUndo);
    if (!data.asInsert) {
        // no inserts should be selected except from paste block and insert
        container->setSelected( false);
        i->setSelected(true);
        explode( false);
        graphic->removeEntity(i);
        b->clear();
        // if this call a destructor for the block?
        graphic->removeBlock(b);
    } else {
        undo.addUndoable(i);
    }


    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::paste: OK");
}



/**
 * Create layers in destination graphic corresponding to entity to be copied
 *
 **/
bool RS_Modification::pasteLayers(RS_Graphic* source) {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteLayers");

    if (!source) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteLayers: no valid graphic found");
        return false;
    }

    RS_LayerList* lrs=source->getLayerList();
    for(RS_Layer* l: *lrs) {

        if(!l) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::pasteLayers: nullptr layer in source");
            continue;
        }

        // add layers if absent
        QString ln = l->getName();
        if (!graphic->findLayer(ln)) {
            graphic->addLayer(l->clone());
            RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteLayers: layer added: %s", ln.toLatin1().data());
        }
    }

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteLayers: OK");
    return true;
}



/**
 * Create inserts and blocks in destination graphic corresponding to entity to be copied
 *
 **/
bool RS_Modification::pasteContainer(RS_Entity* entity, RS_EntityContainer* container, QHash<QString, QString>blocksDict, RS_Vector insertionPoint) {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert");

    if (!entity || entity->rtti() != RS2::EntityInsert) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: no container to process");
        return false;
    }

    RS_Insert* i = (RS_Insert*)entity;
    // get block for this insert object
    RS_Block* ib = i->getBlockForInsert();
    if (!ib) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: no block to process");
        return false;
    }
    // get name for this insert object
    QString name_old = ib->getName();
    QString name_new = name_old;
    if (name_old != i->getName()) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: block and insert names don't coincide");
        return false;
    }
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: processing container: %s", name_old.toLatin1().data());
    // rename if needed
    if (graphic->findBlock(name_old)) {
        name_new = graphic->getBlockList()->newName(name_old);
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: new block name: %s", name_new.toLatin1().data());
    }
    blocksDict[name_old] = name_new;
    // make new block in the destination
    RS_BlockData db = RS_BlockData(name_new, RS_Vector(0.0, 0.0), false);
    RS_Block* bc = new RS_Block(graphic, db);
    bc->reparent(graphic);
    graphic->addBlock(bc);
    // create insert for the new block
    RS_InsertData di = RS_InsertData(name_new, insertionPoint, RS_Vector(1.0, 1.0), i->getAngle(), 1, 1, RS_Vector(0.0,0.0));
    RS_Insert* ic = new RS_Insert(container, di);
    ic->reparent(container);
    container->addEntity(ic);

    // set the same layer in clone as in source
    QString ln = entity->getLayer()->getName();
    RS_Layer* l = graphic->getLayerList()->find(ln);
    if (!l) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: unable to select layer to paste in");
        return false;
    }
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: selected layer: %s", l->getName().toLatin1().data());
    ic->setLayer(l);
    ic->setPen(entity->getPen(false));

    // get relative insertion point
    RS_Vector ip = RS_Vector(0.0, 0.0);
    if (container->getId() != graphic->getId()) {
        ip = bc->getBasePoint();
    }

    // copy content of block/insert to destination
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: copy content to the subcontainer");
    for(auto* e: *i) {

        if(!e) {
            RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Modification::pasteInsert: nullptr entity in block");
            continue;
        }

        if (e->rtti() == RS2::EntityInsert) {
            RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: process sub-insert for %s", ((RS_Insert*)e)->getName().toLatin1().data());
            if (!pasteContainer(e, (RS_EntityContainer*)bc, blocksDict, ip)) {
                RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: unable to paste entity to sub-insert");
                return false;
            }
        } else {
            if (!pasteEntity(e, (RS_EntityContainer*)bc)) {
                RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: unable to paste entity");
                return false;
            }
        }
    }

    ic->update();
    ic->setSelected(false);

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: OK");
    return true;
}



/**
 * Paste entity in supplied container
 *
 **/
bool RS_Modification::pasteEntity(RS_Entity* entity, RS_EntityContainer* container) {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteEntity");

    if (!entity) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteEntity: no entity to process");
        return false;
    }

    // create entity copy to paste
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteEntity ID/flag: %d/%d", entity->getId(), entity->rtti());
    RS_Entity* e = entity->clone();

    // set the same layer in clone as in source
    QString ln = entity->getLayer()->getName();
    RS_Layer* l = graphic->getLayerList()->find(ln);
    if (!l) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: unable to select layer to paste in");
        return false;
    }
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: selected layer: %s", l->getName().toLatin1().data());
    e->setLayer(l);
    e->setPen(entity->getPen(false));

    // scaling entity doesn't needed as it scaled with insert object
    // paste entity
    e->reparent(container);
    container->addEntity(e);
    e->setSelected(false);

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteEntity: OK");
    return true;
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
 *        1st resulting new polyline. Pass nullptr if you don't
 *        need those pointers.
 * @param polyline2 Pointer to a polyline pointer which will hold the
 *        2nd resulting new polyline. Pass nullptr if you don't
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

	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::splitPolyline: no valid container");
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
	RS_Line* line = nullptr;
	RS_Arc* arc = nullptr;

    if (polyline1) {
        *polyline1 = pl1;
    }
    if (polyline2) {
        *polyline2 = pl2;
    }

	for(auto e: polyline){

        if (e->rtti()==RS2::EntityLine) {
            line = (RS_Line*)e;
			arc = nullptr;
        } else if (e->rtti()==RS2::EntityArc) {
            arc = (RS_Arc*)e;
			line = nullptr;
        } else {
			line = nullptr;
			arc = nullptr;
        }

        if (line /*|| arc*/) {

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
//					pl = nullptr;
                } else {
                    // Trim startpoint of entity to second vector
                    pl = pl2;
                    pl->setStartpoint(v);
                    pl->addVertex(line->getEndpoint(), 0.0);
                }
            } else {
                // Add entities to polylines
                if (line && pl) {
                    pl->addVertex(line->getEndpoint(), 0.0);
                }
            }
        }
    }

    container->addEntity(pl1);
    container->addEntity(pl2);
    //container->removeEntity(&polyline);
    polyline.changeUndoState();
	Q_UNUSED( arc ); /* TNick: set but not used */
    return true;
}



/**
 * Adds a node to the given polyline. The new node is placed between
 * the start and end point of the given segment.
 *
 * @param node The position of the new node.
 *
 * @return Pointer to the new polyline or nullptr.
 */
RS_Polyline* RS_Modification::addPolylineNode(RS_Polyline& polyline,
        const RS_AtomicEntity& segment,
        const RS_Vector& node) {
    RS_DEBUG->print("RS_Modification::addPolylineNode");

	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::addPolylineNode: no valid container");
		return nullptr;
    }

    if (segment.getParent()!=&polyline) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::addPolylineNode: segment not part of the polyline");
		return nullptr;
    }

    RS_Polyline* newPolyline = new RS_Polyline(container);
    newPolyline->setClosed(polyline.isClosed());
    newPolyline->setSelected(polyline.isSelected());
    newPolyline->setLayer(polyline.getLayer());
    newPolyline->setPen(polyline.getPen());

    // copy polyline and add new node:
    bool first = true;
    RS_Entity* lastEntity = polyline.lastEntity();
	for(auto e: polyline){

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

				if (ae!=lastEntity || !polyline.isClosed()) {
                    newPolyline->setNextBulge(0.0);
                    newPolyline->addVertex(ae->getEndpoint());
                }
            } else {
                RS_DEBUG->print("RS_Modification::addPolylineNode: normal vertex found: %f/%f",
                                ae->getEndpoint().x, ae->getEndpoint().y);

				if (ae!=lastEntity || !polyline.isClosed()) {
                    newPolyline->setNextBulge(bulge);
                    newPolyline->addVertex(ae->getEndpoint());
                }
            }
        } else {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "RS_Modification::addPolylineNode: Polyline contains non-atomic entities");
        }
    }

    newPolyline->setNextBulge(polyline.getClosingBulge());
    newPolyline->endPolyline();

    // add new polyline:
    container->addEntity(newPolyline);
    if (graphicView) {
        graphicView->deleteEntity(&polyline);
        graphicView->drawEntity(newPolyline);
    }

    if (handleUndo) {
        LC_UndoSection undo( document);

        polyline.setUndoState(true);
        undo.addUndoable(&polyline);
        undo.addUndoable(newPolyline);
    }

    return newPolyline;
}




/**
 * Deletes a node from a polyline.
 *
 * @param node The node to delete.
 *
 * @return Pointer to the new polyline or nullptr.
 */

RS_Polyline* RS_Modification::deletePolylineNode(RS_Polyline& polyline,
        const RS_Vector& node) {

    RS_DEBUG->print("RS_Modification::deletePolylineNode");

	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::addPolylineNode: no valid container");
		return nullptr;
    }

	if (!node.valid) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::deletePolylineNode: node not valid");
		return nullptr;
    }

    // check if the polyline is no longer there after deleting the node:
    if (polyline.count()==1) {
        RS_Entity* e = polyline.firstEntity();
        if (e && e->isAtomic()) {
            RS_AtomicEntity* ae = (RS_AtomicEntity*)e;
            if (node.distanceTo(ae->getStartpoint())<1.0e-6 ||
                    node.distanceTo(ae->getEndpoint())<1.0e-6) {

                if (graphicView) {
                    graphicView->deleteEntity(&polyline);
                }

                if (handleUndo) {
                    LC_UndoSection undo( document);
                    polyline.setUndoState(true);
                    undo.addUndoable(&polyline);
                }
            }
        }
		return nullptr;
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
	for(auto e: polyline){

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
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "RS_Modification::deletePolylineNode: Polyline contains non-atomic entities");
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
    if (graphicView) {
        graphicView->deleteEntity(&polyline);
        graphicView->drawEntity(newPolyline);
    }

    RS_DEBUG->print("RS_Modification::deletePolylineNode: handling undo");
    if (handleUndo) {
        LC_UndoSection undo( document);

        polyline.setUndoState(true);
        undo.addUndoable(&polyline);
        undo.addUndoable(newPolyline);
    }

    return newPolyline;
}




/**
 * Deletes all nodes between the two given nodes (exclusive).
 *
 * @param node1 First limiting node.
 * @param node2 Second limiting node.
 *
 * @return Pointer to the new polyline or nullptr.
 */

RS_Polyline* RS_Modification::deletePolylineNodesBetween(RS_Polyline& polyline,
        RS_AtomicEntity& segment, const RS_Vector& node1, const RS_Vector& node2) {
    Q_UNUSED(segment);
    RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween");

	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::addPolylineNodesBetween: no valid container");
		return nullptr;
    }

    if (node1.valid==false || node2.valid==false) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::deletePolylineNodesBetween: node not valid");
		return nullptr;
    }

    if (node1.distanceTo(node2)<1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::deletePolylineNodesBetween: nodes are identical");
		return nullptr;
    }

    // check if there's nothing to delete:
	for(auto e: polyline){

        if (e->isAtomic()) {
            RS_AtomicEntity* ae = (RS_AtomicEntity*)e;

            if ((node1.distanceTo(ae->getStartpoint())<1.0e-6 &&
                    node2.distanceTo(ae->getEndpoint())<1.0e-6) ||
                    (node2.distanceTo(ae->getStartpoint())<1.0e-6 &&
                     node1.distanceTo(ae->getEndpoint())<1.0e-6)) {

                RS_DEBUG->print(RS_Debug::D_WARNING,
                                "RS_Modification::deletePolylineNodesBetween: nothing to delete");
				return nullptr;
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
        for (; e; e=polyline.nextEntity()) {

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
	for(auto e: polyline){

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
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "RS_Modification::deletePolylineNodesBetween: Polyline contains non-atomic entities");
        }
    }

    RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: ending polyline");
    newPolyline->setNextBulge(polyline.getClosingBulge());
    newPolyline->endPolyline();

    // add new polyline:
    RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: adding new polyline");
    container->addEntity(newPolyline);
    if (graphicView) {
        graphicView->deleteEntity(&polyline);
        graphicView->drawEntity(newPolyline);
    }

    RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: handling undo");
    if (handleUndo) {
        LC_UndoSection undo( document);

        polyline.setUndoState(true);
        undo.addUndoable(&polyline);
        undo.addUndoable(newPolyline);
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
 * @return Pointer to the new polyline or nullptr.
 */

RS_Polyline* RS_Modification::polylineTrim(RS_Polyline& polyline,
        RS_AtomicEntity& segment1,
        RS_AtomicEntity& segment2) {

    RS_DEBUG->print("RS_Modification::polylineTrim");

	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::addPolylineNodesBetween: no valid container");
		return nullptr;
    }

    if (segment1.getParent()!=&polyline || segment2.getParent()!=&polyline) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::polylineTrim: segments not in polyline");
		return nullptr;
    }

    if (&segment1==&segment2) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::polylineTrim: segments are identical");
		return nullptr;
    }

    RS_VectorSolutions sol;
    sol = RS_Information::getIntersection(&segment1, &segment2, false);

    if (sol.getNumber()==0) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::polylineTrim: segments cannot be trimmed");
		return nullptr;
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
											firstSegment->getStartpoint().angleTo(sol.get(0)), M_PI_2);
    //reverseTrim = reverseTrim || !RS_Math::isSameDirection(segment2.getDirection1(),
	//	segment2.getStartpoint().angleTo(sol.get(0)), M_PI_2);

    RS_Polyline* newPolyline = new RS_Polyline(container);
    newPolyline->setClosed(polyline.isClosed());
    newPolyline->setSelected(polyline.isSelected());
    newPolyline->setLayer(polyline.getLayer());
    newPolyline->setPen(polyline.getPen());

    // normal trimming: start removing nodes at trim segment. ends stay the same
	if (!reverseTrim) {
        // copy polyline, trim segments and drop between nodes:
        bool first = true;
        bool removing = false;
        bool nextIsStraight = false;
		RS_Entity* lastEntity = polyline.lastEntity();
		for(auto e: polyline){

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
				if (!removing && (ae==&segment1 || ae==&segment2)) {
                    RS_DEBUG->print("RS_Modification::polylineTrim: "
                                    "start removing at trim point %f/%f",
                                    sol.get(0).x, sol.get(0).y);
                    newPolyline->setNextBulge(0.0);
                    newPolyline->addVertex(sol.get(0));
                    removing = true;
                    nextIsStraight = true;
                }

                // stop removing nodes:
				else if (removing && (ae==&segment1 || ae==&segment2)) {
                    RS_DEBUG->print("RS_Modification::polylineTrim: stop removing at: %f/%f",
                                    ae->getEndpoint().x, ae->getEndpoint().y);
                    removing = false;
                }

                // normal node (not deleted):
				if (!removing) {
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
                RS_DEBUG->print(RS_Debug::D_WARNING,
                                "RS_Modification::polylineTrim: Polyline contains non-atomic entities");
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
		for(auto e: polyline){

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
                RS_DEBUG->print(RS_Debug::D_WARNING,
                                "RS_Modification::polylineTrim: Polyline contains non-atomic entities");
            }
        }
    }

    RS_DEBUG->print("RS_Modification::polylineTrim: ending polyline");
    newPolyline->setNextBulge(polyline.getClosingBulge());
    newPolyline->endPolyline();

    // add new polyline:
    RS_DEBUG->print("RS_Modification::polylineTrim: adding new polyline");
    container->addEntity(newPolyline);
    if (graphicView) {
        graphicView->deleteEntity(&polyline);
        graphicView->drawEntity(newPolyline);
    }

    RS_DEBUG->print("RS_Modification::polylineTrim: handling undo");
    if (handleUndo) {
        LC_UndoSection undo( document);

        polyline.setUndoState(true);
        undo.addUndoable(&polyline);
        undo.addUndoable(newPolyline);
    }

    return newPolyline;
}




/**
 * Moves all selected entities with the given data for the move
 * modification.
 */
bool RS_Modification::move(RS_MoveData& data) {
	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::move: no valid container");
        return false;
    }

	std::vector<RS_Entity*> addList;

    // Create new entities
    for (int num=1;
            num<=data.number || (data.number==0 && num<=1);
            num++) {
        // too slow:
        //for (unsigned i=0; i<container->count(); ++i) {
		//RS_Entity* e = container->entityAt(i);
		for(auto e: *container){
			if (e && e->isSelected()) {
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
				addList.push_back(ec);
            }
        }
    }

    LC_UndoSection undo( document, handleUndo); // bundle remove/add entities in one undoCycle
    deselectOriginals(data.number==0);
    addNewEntities(addList);

    return true;
}


/**
 * Offset all selected entities with the given mouse position and distance
 *
 *@Author: Dongxu Li
 */
bool RS_Modification::offset(const RS_OffsetData& data) {
	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::offset: no valid container");
        return false;
    }

	std::vector<RS_Entity*> addList;

    // Create new entities
    for (int num=1;
            num<=data.number || (data.number==0 && num<=1);
            num++) {
        // too slow:
		for(auto e: *container){
			if (e && e->isSelected()) {
                RS_Entity* ec = e->clone();
				//highlight is used by trim actions. do not carry over flag
				ec->setHighlighted(false);

				if (!ec->offset(data.coord, num*data.distance)) {
                    delete ec;
                    continue;
                }
                if (data.useCurrentLayer) {
                    ec->setLayerToActive();
                }
                if (data.useCurrentAttributes) {
                    ec->setPenToActive();
                }
                if (ec->rtti()==RS2::EntityInsert) {
					static_cast<RS_Insert*>(ec)->update();
                }
                // since 2.0.4.0: keep selection
                ec->setSelected(true);
				addList.push_back(ec);
            }
        }
    }

    LC_UndoSection undo( document, handleUndo); // bundle remove/add entities in one undoCycle
    deselectOriginals(data.number==0);
    addNewEntities(addList);

    return true;
}




/**
 * Rotates all selected entities with the given data for the rotation.
 */
bool RS_Modification::rotate(RS_RotateData& data) {
	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::rotate: no valid container");
        return false;
    }

	std::vector<RS_Entity*> addList;

    // Create new entities
    for (int num=1;
            num<=data.number || (data.number==0 && num<=1);
			num++) {
		for(auto e: *container){
            //for (unsigned i=0; i<container->count(); ++i) {
            //RS_Entity* e = container->entityAt(i);

            if (e && e->isSelected()) {
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
				addList.push_back(ec);
            }
        }
    }

    LC_UndoSection undo( document, handleUndo); // bundle remove/add entities in one undoCycle
    deselectOriginals(data.number==0);
    addNewEntities(addList);

    return true;
}



/**
 * Moves all selected entities with the given data for the scale
 * modification.
 */
bool RS_Modification::scale(RS_ScaleData& data) {
	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::scale: no valid container");
        return false;
    }

	std::vector<RS_Entity*> selectedList,addList;

	for(auto ec: *container){
        if (ec->isSelected() ) {
            if ( fabs(data.factor.x - data.factor.y) > RS_TOLERANCE ) {
                    if ( ec->rtti() == RS2::EntityCircle ) {
    //non-isotropic scaling, replacing selected circles with ellipses
				RS_Circle *c=static_cast<RS_Circle*>(ec);
				ec= new RS_Ellipse{container,
				{c->getCenter(), {c->getRadius(),0.},
						1.,
						0., 0., false}};
            } else if ( ec->rtti() == RS2::EntityArc ) {
    //non-isotropic scaling, replacing selected arcs with ellipses
				RS_Arc *c=static_cast<RS_Arc*>(ec);
				ec= new RS_Ellipse{container,
								   {c->getCenter(),
								   {c->getRadius(),0.},
								   1.0,
								   c->getAngle1(),
								   c->getAngle2(),
								   c->isReversed()}};
            }
            }
			selectedList.push_back(ec);

        }
    }

    // Create new entities
    for (int num=1;
            num<=data.number || (data.number==0 && num<=1);
            num++) {

		for(RS_Entity* e: selectedList) {
            //for (RS_Entity* e=container->firstEntity();
            //        e;
            //        e=container->nextEntity()) {
            //for (unsigned i=0; i<container->count(); ++i) {
            //RS_Entity* e = container->entityAt(i);
			if (e) {
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
				addList.push_back(ec);
            }
        }
    }

    LC_UndoSection undo( document, handleUndo); // bundle remove/add entities in one undoCycle
    deselectOriginals(data.number==0);
    addNewEntities(addList);

    return true;
}



/**
 * Mirror all selected entities with the given data for the mirror
 * modification.
 */
bool RS_Modification::mirror(RS_MirrorData& data) {
	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::mirror: no valid container");
        return false;
    }

	std::vector<RS_Entity*> addList;

    // Create new entities
    for (int num=1;
            num<=(int)data.copy || (data.copy==false && num<=1);
			++num) {
		for(auto e: *container){
            //for (unsigned i=0; i<container->count(); ++i) {
            //RS_Entity* e = container->entityAt(i);

            if (e && e->isSelected()) {
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
				addList.push_back(ec);
            }
        }
    }

    LC_UndoSection undo( document, handleUndo); // bundle remove/add entities in one undoCycle
    deselectOriginals(data.copy==false);
    addNewEntities(addList);

    return true;
}



/**
 * Rotates entities around two centers with the given parameters.
 */
bool RS_Modification::rotate2(RS_Rotate2Data& data) {
	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::rotate2: no valid container");
        return false;
    }

	std::vector<RS_Entity*> addList;

    // Create new entities
    for (int num=1;
            num<=data.number || (data.number==0 && num<=1);
            num++) {

		for(auto e: *container){
            //for (unsigned i=0; i<container->count(); ++i) {
            //RS_Entity* e = container->entityAt(i);

            if (e && e->isSelected()) {
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
				addList.push_back(ec);
            }
        }
    }

    LC_UndoSection undo( document, handleUndo); // bundle remove/add entities in one undoCycle
    deselectOriginals(data.number==0);
    addNewEntities(addList);

    return true;
}



/**
 * Moves and rotates entities with the given parameters.
 */
bool RS_Modification::moveRotate(RS_MoveRotateData& data) {
	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::moveRotate: no valid container");
        return false;
    }

	std::vector<RS_Entity*> addList;

    // Create new entities
    for (int num=1;
            num<=data.number || (data.number==0 && num<=1);
			++num) {
		for(auto e: *container){
            //for (unsigned i=0; i<container->count(); ++i) {
            //RS_Entity* e = container->entityAt(i);

            if (e && e->isSelected()) {
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
				addList.push_back(ec);
            }
        }
    }

    LC_UndoSection undo( document, handleUndo); // bundle remove/add entities in one undoCycle
    deselectOriginals(data.number==0);
    addNewEntities(addList);

    return true;
}



/**
 * Deselects all selected entities and removes them if remove is true;
 *
 * @param remove true: Remove entities.
 */
void RS_Modification::deselectOriginals(bool remove)
{
    LC_UndoSection undo( document, handleUndo);

    for (auto e: *container) {

        //for (unsigned i=0; i<container->count(); ++i) {
        //RS_Entity* e = container->entityAt(i);

        if (e) {
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
                    //if (graphicView) {
                    //    graphicView->deleteEntity(e);
                    //}
                    e->changeUndoState();
                    undo.addUndoable(e);
                } else {
                    //if (graphicView) {
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
void RS_Modification::addNewEntities(std::vector<RS_Entity*>& addList)
{
    LC_UndoSection undo( document, handleUndo);

    for (RS_Entity* e: addList) {
        if (e) {
            container->addEntity(e);
            undo.addUndoable(e);
        }
    }

    if (graphicView) {
        graphicView->redraw(RS2::RedrawDrawing);
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

	if (!(trimEntity && limitEntity)) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
						"RS_Modification::trim: At least one entity is nullptr");
        return false;
    }

    if (both && !limitEntity->isAtomic()) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::trim: limitEntity is not atomic");
    }
    if(trimEntity->isLocked()|| !trimEntity->isVisible()) return false;

    RS_VectorSolutions sol;
    if (limitEntity->isAtomic()) {
        // intersection(s) of the two entities:
        sol = RS_Information::getIntersection(trimEntity, limitEntity, false);
    } else if (limitEntity->isContainer()) {
        RS_EntityContainer* ec = (RS_EntityContainer*)limitEntity;

        //sol.alloc(128);

        for (RS_Entity* e=ec->firstEntity(RS2::ResolveAll); e;
                e=ec->nextEntity(RS2::ResolveAll)) {
            //for (int i=0; i<container->count(); ++i) {
            //    RS_Entity* e = container->entityAt(i);

            if (e) {

                RS_VectorSolutions s2 = RS_Information::getIntersection(trimEntity,
                                        e, false);

                if (s2.hasValid()) {
					for (const RS_Vector& vp: s2){
						if (vp.valid) {
							if (e->isPointOnEntity(vp, 1.0e-4)) {
								sol.push_back(vp);
                            }
                        }
                    }
                    //break;
                }
            }
        }
    }
//if intersection are in start or end point can't trim/extend in this point, remove from solution. sf.net #3537053
    if (trimEntity->rtti()==RS2::EntityLine){
        RS_Line *lin = (RS_Line *)trimEntity;
        for (unsigned int i=0; i< sol.size(); i++) {
            RS_Vector v = sol.at(i);
            if (v == lin->getStartpoint())
                sol.removeAt(i);
            else if (v == lin->getEndpoint())
                sol.removeAt(i);
        }
    }

	if (!sol.hasValid()) {
        return both ? trim( limitCoord, (RS_AtomicEntity*)limitEntity, trimCoord, trimEntity, false) : false;
    }

	RS_AtomicEntity* trimmed1 = nullptr;
	RS_AtomicEntity* trimmed2 = nullptr;

    if (trimEntity->rtti()==RS2::EntityCircle) {
        // convert a circle into a trimmable arc, need to start from intersections
        RS_Circle* c = static_cast<RS_Circle*>(trimEntity);
        double aStart=0.;
        double aEnd=2.*M_PI;
        switch(sol.size()){
        case 0:
            break;
        case 1:
            aStart=c->getCenter().angleTo(sol.at(0));
            aEnd=aStart+2.*M_PI;
            break;
        default:
        case 2:
            //trim according to intersections
			std::vector<double> angles;
            const auto& center0=c->getCenter();
			for(const RS_Vector& vp : sol){
				angles.push_back(center0.angleTo(vp));
            }
            //sort intersections by angle to circle center
            std::sort(angles.begin(), angles.end());
            const double a0=center0.angleTo(trimCoord);
			for(size_t i=0; i<angles.size(); ++i){
                aStart=angles.at(i);
                aEnd=angles.at( (i+1)%angles.size());
                if(RS_Math::isAngleBetween(a0, aStart, aEnd, false))
                    break;
            }
            break;
        }
        RS_ArcData d(c->getCenter(),
                     c->getRadius(),
                     aStart,
                     aEnd,
                     false);
        trimmed1 = new RS_Arc(trimEntity->getParent(), d);
    } else {
        trimmed1 = (RS_AtomicEntity*)trimEntity->clone();
        trimmed1->setHighlighted(false);
    }

    // trim trim entity
	size_t ind = 0;
    RS_Vector is, is2;

    //RS2::Ending ending = trimmed1->getTrimPoint(trimCoord, is);
    if ( trimEntity->trimmable() ) {
        is = trimmed1->prepareTrim(trimCoord, sol);
    } else {
		is = sol.getClosest(limitCoord, nullptr, &ind);
		//sol.getClosest(limitCoord, nullptr, &ind);
        RS_DEBUG->print("RS_Modification::trim: limitCoord: %f/%f", limitCoord.x, limitCoord.y);
        RS_DEBUG->print("RS_Modification::trim: sol.get(0): %f/%f", sol.get(0).x, sol.get(0).y);
        RS_DEBUG->print("RS_Modification::trim: sol.get(1): %f/%f", sol.get(1).x, sol.get(1).y);
        RS_DEBUG->print("RS_Modification::trim: ind: %d", ind);
        is2 = sol.get(ind==0 ? 1 : 0);
        //RS_Vector is2 = sol.get(ind);
        RS_DEBUG->print("RS_Modification::trim: is2: %f/%f", is2.x, is2.y);

    }

    // remove trim entity from view:
    if (graphicView) {
        graphicView->deleteEntity(trimEntity);
    }

    // remove limit entity from view:
    bool trimBoth= both && !limitEntity->isLocked() && limitEntity->isVisible();
    if (trimBoth) {
        trimmed2 = (RS_AtomicEntity*)limitEntity->clone();
        trimmed2->setHighlighted(false);
        if (graphicView) {
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
    if (trimBoth) {
        if ( trimmed2->trimmable())
            is2 = trimmed2->prepareTrim(limitCoord, sol);
         else
            is2 = sol.getClosest(trimCoord);

        RS2::Ending ending = trimmed2->getTrimPoint(limitCoord, is2);

        switch (ending) {
        case RS2::EndingStart:
            trimmed2->trimStartpoint(is2);
            break;
        case RS2::EndingEnd:
            trimmed2->trimEndpoint(is2);
            break;
        default:
            break;
        }
    }

    // add new trimmed trim entity:
    container->addEntity(trimmed1);
    if (graphicView) {
        graphicView->drawEntity(trimmed1);
    }

    // add new trimmed limit entity:
    if (trimBoth) {
        container->addEntity(trimmed2);
        if (graphicView) {
            graphicView->drawEntity(trimmed2);
        }
    }

    if (handleUndo) {
        LC_UndoSection undo( document);

        undo.addUndoable(trimmed1);
        trimEntity->setUndoState(true);
        undo.addUndoable(trimEntity);
        if (trimBoth) {
            undo.addUndoable(trimmed2);
            limitEntity->setUndoState(true);
            undo.addUndoable(limitEntity);
        }
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

	if (!trimEntity) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
						"RS_Modification::trimAmount: Entity is nullptr");
        return false;
    }
    if(trimEntity->isLocked() || ! trimEntity->isVisible()) return false;

	RS_AtomicEntity* trimmed = nullptr;

    // remove trim entity:
    trimmed = (RS_AtomicEntity*)trimEntity->clone();
    if (graphicView) {
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

    if (graphicView) {
        graphicView->drawEntity(trimmed);
    }

    if (handleUndo) {
        LC_UndoSection undo( document);

        undo.addUndoable(trimmed);
        trimEntity->setUndoState(true);
        undo.addUndoable(trimEntity);
    }

    return true;
}



/**
 * Cuts the given entity at the given point.
 */
bool RS_Modification::cut(const RS_Vector& cutCoord,
                          RS_AtomicEntity* cutEntity) {

#ifndef EMU_C99
    using std::isnormal;
#endif

	if (!cutEntity) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
						"RS_Modification::cut: Entity is nullptr");
        return false;
    }
    if(cutEntity->isLocked() || ! cutEntity->isVisible()) return false;

    if (!cutCoord.valid) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::cut: Point invalid.");
        return false;
    }

    // cut point is at endpoint of entity:
    if (cutCoord.distanceTo(cutEntity->getStartpoint())<RS_TOLERANCE ||
            cutCoord.distanceTo(cutEntity->getEndpoint())<RS_TOLERANCE) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::cut: Cutting point on endpoint");
        return false;
    }

    // delete cut entity on the screen:
    if (graphicView) {
        graphicView->deleteEntity(cutEntity);
    }

	RS_AtomicEntity* cut1 = nullptr;
	RS_AtomicEntity* cut2 = nullptr;
    double a;

    switch (cutEntity->rtti()) {
    case RS2::EntityCircle:
        // convert to a whole 2 pi range arc
        //RS_Circle* c = (RS_Circle*)cutEntity;
        a=static_cast<RS_Circle*>(cutEntity)->getCenter().angleTo(cutCoord);
        cut1 = new RS_Arc(cutEntity->getParent(),
                          RS_ArcData(static_cast<RS_Circle*>(cutEntity) ->getCenter(),
                                     static_cast<RS_Circle*>(cutEntity) ->getRadius(),
                                     a,a+2.*M_PI, false));
        cut1->setPen(cutEntity->getPen(false));
        cut1->setLayer(cutEntity->getLayer(false));
		//cut2 = nullptr; // cut2 is nullptr by default
        break;

        // handle ellipse arc the using the default method
    case RS2::EntitySplinePoints: // interpolation spline can be closed
		// so we cannot use the default implementation
        cut2 = ((LC_SplinePoints*)cutEntity)->cut(cutCoord);
		cut1 = (RS_AtomicEntity*)cutEntity->clone();

        cut1->setPen(cutEntity->getPen(false));
        cut1->setLayer(cutEntity->getLayer(false));
		if(cut2)
		{
		    cut2->setPen(cutEntity->getPen(false));
		    cut2->setLayer(cutEntity->getLayer(false));
		}
		break;
    case RS2::EntityEllipse:
    // ToDo, to really handle Ellipse Arcs properly, we need to create a new class RS_EllipseArc, keep RS_Ellipse for whole range Ellipses
    {
        const RS_Ellipse* const ellipse=static_cast<const RS_Ellipse*>(cutEntity);
        if(RS_Math::isSameDirection( ellipse ->getAngle1(), ellipse ->getAngle2(), RS_TOLERANCE_ANGLE)
                && ! /*std::*/isnormal(ellipse->getAngle1())
                && ! /*std::*/isnormal(ellipse->getAngle2())
                ) {
            // whole ellipse, convert to a whole range elliptic arc
            a=ellipse->getEllipseAngle(cutCoord);
			cut1 = new RS_Ellipse{cutEntity->getParent(),
					RS_EllipseData{ellipse ->getCenter(),
					ellipse ->getMajorP(),
					ellipse ->getRatio(),
					a,a+2.*M_PI,
					ellipse ->isReversed()
		}
		};
            cut1->setPen(cutEntity->getPen(false));
            cut1->setLayer(cutEntity->getLayer(false));
			//cut2 = nullptr; // cut2 is nullptr by default
            break;
        }else{
            //elliptic arc
            //missing "break;" line is on purpose
            //elliptic arc should be handled by default:
            //do not insert between here and default:
        }
    }
    // fall-through
    default:
        cut1 = (RS_AtomicEntity*)cutEntity->clone();
        cut2 = (RS_AtomicEntity*)cutEntity->clone();

        cut1->trimEndpoint(cutCoord);
        cut2->trimStartpoint(cutCoord);
    }
    // add new cut entity:
    container->addEntity(cut1);
    if (cut2) {
        container->addEntity(cut2);
    }

    if (graphicView) {
        graphicView->drawEntity(cut1);
        if (cut2) {
            graphicView->drawEntity(cut2);
        }
    }

    if (handleUndo) {
        LC_UndoSection undo( document);

        undo.addUndoable(cut1);
        if (cut2) {
            undo.addUndoable(cut2);
        }
        cutEntity->setUndoState(true);
        undo.addUndoable(cutEntity);
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

	std::vector<RS_Entity*> addList;

	// Create new entities
	for(auto e: *container){
		if (e &&
                e->isVisible() &&
                !e->isLocked() ) {
//            &&
            if (  (e->isInWindow(firstCorner, secondCorner) ||
                    e->hasEndpointsWithinWindow(firstCorner, secondCorner))) {

                RS_Entity* ec = e->clone();
                ec->stretch(firstCorner, secondCorner, offset);
				addList.push_back(ec);
                e->setSelected(true);
            }
        }
    }

    LC_UndoSection undo( document, handleUndo); // bundle remove/add entities in one undoCycle
    deselectOriginals(true);
    addNewEntities(addList);

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

	if (!(entity1 && entity2)) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
						"RS_Modification::bevel: At least one entity is nullptr");
        return false;
    }
    if(entity1->isLocked() || ! entity1->isVisible()) return false;
    if(entity2->isLocked() || ! entity2->isVisible()) return false;

    RS_EntityContainer* baseContainer = container;
    bool isPolyline = false;
//    bool isClosedPolyline = false;

    LC_UndoSection undo( document, handleUndo);

    // find out whether we're bevelling within a polyline:
    if (entity1->getParent() &&
            entity1->getParent()->rtti()==RS2::EntityPolyline) {
        RS_DEBUG->print("RS_Modification::bevel: trimming polyline segments");
        if (entity1->getParent()!=entity2->getParent()) {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "RS_Modification::bevel: entities not in the same polyline");
            return false;
        }
        //TODO: check if entity1 & entity2 are lines.
        //bevel only can be with lines.

        // clone polyline for undo
        if (handleUndo) {
            RS_EntityContainer* cl =
                (RS_EntityContainer*)entity1->getParent()->clone();
            container->addEntity(cl);
            //cl->setUndoState(true);
            undo.addUndoable(cl);

            undo.addUndoable(entity1->getParent());
            entity1->getParent()->setUndoState(true);

            baseContainer = cl;
        }

        entity1 = (RS_AtomicEntity*)baseContainer->entityAt(entity1->getParent()->findEntity(entity1));
        entity2 = (RS_AtomicEntity*)baseContainer->entityAt(entity2->getParent()->findEntity(entity2));

        //baseContainer = entity1->getParent();
        isPolyline = true;
//        isClosedPolyline = ((RS_Polyline*)entity1)->isClosed();
    }

    RS_DEBUG->print("RS_Modification::bevel: getting intersection");

    RS_VectorSolutions sol =
        RS_Information::getIntersection(entity1, entity2, false);

    if (sol.getNumber()==0) {
        return false;
    }

	RS_AtomicEntity* trimmed1 = nullptr;
	RS_AtomicEntity* trimmed2 = nullptr;

    //if (data.trim || isPolyline) {
    if (isPolyline) {
        trimmed1 = entity1;
        trimmed2 = entity2;
        //Always trim if are working with a polyline, to work with trim==false
        //bevel can't be part of the polyline
        data.trim = true;
    } else {
        trimmed1 = (RS_AtomicEntity*)entity1->clone();
        trimmed2 = (RS_AtomicEntity*)entity2->clone();
    }

    // remove trim entity (on screen):
	if (data.trim || isPolyline) {
        if (graphicView) {
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
	if (data.trim) {
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
		if (!isPolyline) {
            container->addEntity(trimmed1);
            container->addEntity(trimmed2);
        }
        if (graphicView) {
            if (!isPolyline) {
                graphicView->drawEntity(trimmed1);
                graphicView->drawEntity(trimmed2);
            }
        }
    }


    // add bevel line:
    RS_DEBUG->print("RS_Modification::bevel: add bevel line");
	RS_Line* bevel = new RS_Line{baseContainer, {bp1, bp2}};

	if (!isPolyline) {
        baseContainer->addEntity(bevel);
    } else {
        int idx1 = baseContainer->findEntity(trimmed1);
        int idx2 = baseContainer->findEntity(trimmed2);
        int idx = idx1;
        //Verify correct order segment in polylines
        if (idx1 > idx2){
            //inverted, reorder it (swap).
            idx1 = idx2;
            idx2 = idx;
            RS_AtomicEntity* trimmedTmp = trimmed1;
            trimmed1 = trimmed2;
            trimmed2 = trimmedTmp;
        }
        idx = idx1;

        bevel->setSelected(baseContainer->isSelected());
        bevel->setLayer(baseContainer->getLayer());
        bevel->setPen(baseContainer->getPen());

        // insert bevel at the right position:
        if (trimmed1 == baseContainer->first() && trimmed2 == baseContainer->last()){
            //bevel are from last and first segments, add at the end
            if (trimmed2->getEndpoint().distanceTo(bevel->getStartpoint())>1.0e-4) {
                bevel->reverse();
            }
            idx = idx2;
        } else{
            //consecutive segments
            if (trimmed1->getEndpoint().distanceTo(bevel->getStartpoint())>1.0e-4) {
                bevel->reverse();
            }
        }
        baseContainer->insertEntity(idx+1, bevel);
    }

    if (isPolyline) {
        ((RS_Polyline*)baseContainer)->updateEndpoints();
    }

    if (graphicView) {
        if (isPolyline) {
            graphicView->drawEntity(baseContainer);
        } else {
            graphicView->drawEntity(bevel);
        }
    }

    RS_DEBUG->print("RS_Modification::bevel: handling undo");

    if (handleUndo) {
		if (!isPolyline && data.trim) {
            undo.addUndoable(trimmed1);
            entity1->setUndoState(true);
            undo.addUndoable(entity1);

            undo.addUndoable(trimmed2);
            entity2->setUndoState(true);
            undo.addUndoable(entity2);
        }

		if (!isPolyline) {
            undo.addUndoable(bevel);
        }
    }
//Do not delete trimmed* if are part of a polyline
	if (!(data.trim || isPolyline)) {
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

	if (!(entity1 && entity2)) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
						"RS_Modification::round: At least one entity is nullptr");
        return false;
    }
    if(entity1->isLocked() || ! entity1->isVisible()) return false;
    if(entity2->isLocked() || ! entity2->isVisible()) return false;

    RS_EntityContainer* baseContainer = container;
    bool isPolyline = false;
//    bool isClosedPolyline = false;

    LC_UndoSection undo( document, handleUndo);
    // find out whether we're rounding within a polyline:
    if (entity1->getParent() &&
            entity1->getParent()->rtti()==RS2::EntityPolyline) {

        if (entity1->getParent()!=entity2->getParent()) {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "RS_Modification::round: entities not in "
                            "the same polyline");
            return false;
        }

        // clone polyline for undo
        if (handleUndo) {
            RS_EntityContainer* cl =
                (RS_EntityContainer*)entity1->getParent()->clone();
            container->addEntity(cl);
            undo.addUndoable(cl);

            undo.addUndoable(entity1->getParent());
            entity1->getParent()->setUndoState(true);

            baseContainer = cl;
        }

        entity1 = (RS_AtomicEntity*)baseContainer->entityAt(entity1->getParent()->findEntity(entity1));
        entity2 = (RS_AtomicEntity*)baseContainer->entityAt(entity2->getParent()->findEntity(entity2));

        isPolyline = true;
//        isClosedPolyline = ((RS_Polyline*)entity1->getParent())->isClosed();
    }

    // create 2 tmp parallels
	RS_Creation creation(nullptr, nullptr);
    RS_Entity* par1 = creation.createParallel(coord, data.radius, 1, entity1);
    RS_Entity* par2 = creation.createParallel(coord, data.radius, 1, entity2);

    RS_VectorSolutions sol2 =
        RS_Information::getIntersection(entity1, entity2, false);

    RS_VectorSolutions sol =
        RS_Information::getIntersection(par1, par2, false);

    if (sol.getNumber()==0) {
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


	RS_AtomicEntity* trimmed1 = nullptr;
	RS_AtomicEntity* trimmed2 = nullptr;

    if (data.trim || isPolyline) {
        if (isPolyline) {
            trimmed1 = entity1;
            trimmed2 = entity2;
        } else {
            trimmed1 = (RS_AtomicEntity*)entity1->clone();
            trimmed2 = (RS_AtomicEntity*)entity2->clone();
        }

        // remove trim entity:
        if (graphicView) {
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
		if (!isPolyline) {
            container->addEntity(trimmed1);
            container->addEntity(trimmed2);
        }
        if (graphicView) {
            if (!isPolyline) {
                graphicView->drawEntity(trimmed1);
                graphicView->drawEntity(trimmed2);
            }
        }
    }

    // add rounding:
	if (!isPolyline) {
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

        bool insertAfter1 = ((idx1<idx2 && idx1!=0) ||(idx1==0 && idx2==1) ||
                            (idx2==0 && idx1==(int)baseContainer->count()-1));

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

    if (graphicView) {
        if (isPolyline) {
            graphicView->drawEntity(baseContainer);
        } else {
            graphicView->drawEntity(arc);
        }
    }

    if (handleUndo) {
		if (!isPolyline && data.trim) {
            undo.addUndoable(trimmed1);
            entity1->setUndoState(true);
            undo.addUndoable(entity1);

            undo.addUndoable(trimmed2);
            entity2->setUndoState(true);
            undo.addUndoable(entity2);
        }

		if (!isPolyline) {
            undo.addUndoable(arc);
        }
    }

    delete par1;
    delete par2;

    return true;
}


/**
 * Repetitive recursive block of code for the explode() method.
 */
static void update_exploded_children_recursively(
        RS_EntityContainer* ec,
        RS_Entity* e,
        RS_Entity* clone,
        RS2::ResolveLevel rl,
        bool resolveLayer,
        bool resolvePen) {

    if (!ec) {
        return;
    }
    if (!e) {
        return;
    }
    if (!clone) {
        return;
    }

    if (resolveLayer) {
        clone->setLayer(ec->getLayer());
    } else {
        clone->setLayer(e->getLayer());
    }

    if (resolvePen) {
        //clone->setPen(ec->getPen(true));
        clone->setPen(ec->getPen(false));
    } else {
        clone->setPen(e->getPen(false));
    }

    clone->update();

    if (clone->isContainer()) {
        // Note: reassigning ec and e here, so keep
        // that in mind when writing code below this block.
        ec = (RS_EntityContainer*) clone;
        for (e = ec->firstEntity(rl); e; e = ec->nextEntity(rl)) {
            if (e) {
                // Run the same code for every children recursively
                update_exploded_children_recursively(ec, clone, e,
                        rl, resolveLayer, resolvePen);
            }
        }
    }
}


/**
 * Removes the selected entity containers and adds the entities in them as
 * new single entities.
 */
bool RS_Modification::explode(const bool remove /*= true*/)
{
    if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::explode: no valid container for addinge entities");
        return false;
    }
	if (container->isLocked() || ! container->isVisible()) return false;

	std::vector<RS_Entity*> addList;

    for(auto e: *container){
        //for (unsigned i=0; i<container->count(); ++i) {
        //RS_Entity* e = container->entityAt(i);

        if (e && e->isSelected()) {
            if (e->isContainer()) {

                // add entities from container:
                RS_EntityContainer* ec = (RS_EntityContainer*)e;
                //ec->setSelected(false);

                // iterate and explode container:
                //for (unsigned i2=0; i2<ec->count(); ++i2) {
                //    RS_Entity* e2 = ec->entityAt(i2);
                RS2::ResolveLevel rl;
                bool resolvePen;
                bool resolveLayer;

                switch (ec->rtti()) {
                case RS2::EntityMText:
                case RS2::EntityText:
                case RS2::EntityHatch:
                case RS2::EntityPolyline:
                    rl = RS2::ResolveAll;
                    resolveLayer = true;
                    resolvePen = true;
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

                for (RS_Entity* e2 = ec->firstEntity(rl); e2;
                        e2 = ec->nextEntity(rl)) {

                    if (e2) {
                        RS_Entity* clone = e2->clone();
                        clone->setSelected(false);
                        clone->reparent(container);

                        addList.push_back(clone);

                        // In order to fix bug #819 and escape similar issues,
                        // we have to update all children of exploded entity,
                        // even those (below the tree) which are not direct
                        // subjects to the current explode() call.
                        update_exploded_children_recursively(ec, e2, clone,
                                rl, resolveLayer, resolvePen);
/*
                        if (resolveLayer) {
                            clone->setLayer(ec->getLayer());
                        } else {
                            clone->setLayer(e2->getLayer());
                        }

//                        clone->setPen(ec->getPen(resolvePen));
                        if (resolvePen) {
                            clone->setPen(ec->getPen(true));
                        } else {
                            clone->setPen(e2->getPen(false));
                        }

						addList.push_back(clone);

                        clone->update();
*/
                    }
                }
            } else {
                e->setSelected(false);
            }
        }
    }

    LC_UndoSection undo( document, handleUndo); // bundle remove/add entities in one undoCycle
    deselectOriginals( remove);
    addNewEntities(addList);
    container->updateInserts();

    return true;
}



bool RS_Modification::explodeTextIntoLetters() {
	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::explodeTextIntoLetters: no valid container for addinge entities");
        return false;
    }
    if(container->isLocked() || ! container->isVisible()) return false;

	std::vector<RS_Entity*> addList;

	for(auto e: *container){
        if (e && e->isSelected()) {
            if (e->rtti()==RS2::EntityMText) {
                // add letters of text:
                RS_MText* text = (RS_MText*)e;
                explodeTextIntoLetters(text, addList);
            } else if (e->rtti()==RS2::EntityText) {
                // add letters of text:
                RS_Text* text = (RS_Text*)e;
                explodeTextIntoLetters(text, addList);
            } else {
                e->setSelected(false);
            }
        }
    }

    LC_UndoSection undo( document, handleUndo); // bundle remove/add entities in one undoCycle
    deselectOriginals(true);
    addNewEntities(addList);

    return true;
}


bool RS_Modification::explodeTextIntoLetters(RS_MText* text, std::vector<RS_Entity*>& addList) {

	if (!text) {
        return false;
    }

    if(text->isLocked() || ! text->isVisible()) return false;

    // iterate though lines:
	for(auto e2: *text){

		if (!e2) {
            break;
        }


        // text lines:
        if (e2->rtti()==RS2::EntityContainer) {

            RS_EntityContainer* line = (RS_EntityContainer*)e2;

            // iterate though letters:
			for(auto e3: *line){

				if (!e3) {
                    break;
                }

                // super / sub texts:
                if (e3->rtti()==RS2::EntityMText) {
                    explodeTextIntoLetters((RS_MText*)e3, addList);
                }

                // normal letters:
                else if (e3->rtti()==RS2::EntityInsert) {

                    RS_Insert* letter = (RS_Insert*)e3;

                    RS_MText* tl = new RS_MText(
                        container,
                        RS_MTextData(letter->getInsertionPoint(),
                                    text->getHeight(),
                                    100.0,
                                    RS_MTextData::VABottom, RS_MTextData::HALeft,
                                    RS_MTextData::LeftToRight, RS_MTextData::Exact,
                                    1.0,
                                    letter->getName(),
                                    text->getStyle(),
                                    letter->getAngle(),
                                    RS2::Update));

                    tl->setLayer(text->getLayer());
                    tl->setPen(text->getPen());

					addList.push_back(tl);
                    tl->update();
                }
            }
        }
    }

    return true;
}

bool RS_Modification::explodeTextIntoLetters(RS_Text* text, std::vector<RS_Entity*>& addList) {

	if (!text) {
        return false;
    }

    if(text->isLocked() || ! text->isVisible()) return false;

    // iterate though letters:
	for(auto e2: *text){

		if (!e2) {
            break;
        }

        if (e2->rtti()==RS2::EntityInsert) {

            RS_Insert* letter = (RS_Insert*)e2;

            RS_Text* tl = new RS_Text(
                        container,
                        RS_TextData(letter->getInsertionPoint(),
                                    letter->getInsertionPoint(),
                                    text->getHeight(),
                                    text->getWidthRel(), RS_TextData::VABaseline,
                                    RS_TextData::HALeft, RS_TextData::None, /*text->getTextGeneration(),*/
                                    letter->getName(),
                                    text->getStyle(),
                                    letter->getAngle(),
                                    RS2::Update));

            tl->setLayer(text->getLayer());
            tl->setPen(text->getPen());

			addList.push_back(tl);
            tl->update();
        }
    }

    return true;
}


/**
 * Moves all reference points of selected entities with the given data.
 */
bool RS_Modification::moveRef(RS_MoveRefData& data) {
	if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::moveRef: no valid container");
        return false;
    }
    if(container->isLocked() || ! container->isVisible()) return false;

	std::vector<RS_Entity*> addList;

    // Create new entities
	for(auto e: *container){
		if (e && e->isSelected()) {
            RS_Entity* ec = e->clone();

            ec->moveRef(data.ref, data.offset);
            // since 2.0.4.0: keep it selected
            ec->setSelected(true);
			addList.push_back(ec);
        }
    }

    LC_UndoSection undo( document, handleUndo); // bundle remove/add entities in one undoCycle
    deselectOriginals(true);
    addNewEntities(addList);

    return true;
}

// EOF
