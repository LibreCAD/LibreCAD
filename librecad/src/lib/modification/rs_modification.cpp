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

#include <QSet>

#include "rs_modification.h"

#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_clipboard.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_graphicview.h"
#include "rs_graphic.h"
#include "rs_information.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_mtext.h"
#include "rs_polyline.h"
#include "rs_text.h"
#include "rs_units.h"
#include "lc_splinepoints.h"
#include "lc_undosection.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

namespace {
// fixme - hm, is it actually needed to mix the logic of modification and ui/undo?
/**
 * @brief getPasteScale - find scaling factor for pasting
 * @param const RS_PasteData& data - RS_PasteData
 * @param RS_Graphic *& source - source graphic. If source is nullptr, the graphic on the clipboard is used instead
 * @param const RS_Graphic& graphic - the target graphic
 * @return
 */
    RS_Vector getPasteScale(const RS_PasteData &data, RS_Graphic *&source, const RS_Graphic &graphic){

        // adjust scaling factor for units conversion in case of clipboard paste
        double factor = (RS_TOLERANCE < std::abs(data.factor)) ? data.factor : 1.0;
        // select source for paste
        if (source == nullptr){
            RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::paste: add graphic source from clipboard");
            source = RS_CLIPBOARD->getGraphic();
            // graphics from the clipboard need to be scaled. From the part lib not:
            RS2::Unit sourceUnit = source->getUnit();
            RS2::Unit targetUnit = graphic.getUnit();
            factor = RS_Units::convert(factor, sourceUnit, targetUnit);
        }
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::paste: pasting scale factor: %g", factor);
        // scale factor as vector
        return {factor, factor};
    }

/**
 * @brief addNewBlock() - create a new block
 * @param name - name of the new block to create
 * @param graphic - the target graphic
 * @return RS_Block - the block created
 */
    RS_Block *addNewBlock(const QString &name, RS_Graphic &graphic){
        RS_BlockData db = RS_BlockData(name, {0.0, 0.0}, false);
        auto *b = new RS_Block(&graphic, db);
        b->reparent(&graphic);
        graphic.addBlock(b);
        return b;
    }

    RS_VectorSolutions findIntersection(const RS_Entity &trimEntity, const RS_Entity &limitEntity, double tolerance = 1e-4){

        RS_VectorSolutions sol;
        if (limitEntity.isAtomic()){
            // intersection(s) of the two entities:
            return RS_Information::getIntersection(&trimEntity, &limitEntity, false);
        }
        if (limitEntity.isContainer()){
            auto ec = dynamic_cast<const RS_EntityContainer *>(&limitEntity);

            for (RS_Entity *e = ec->firstEntity(RS2::ResolveAll); e != nullptr;
                 e = ec->nextEntity(RS2::ResolveAll)) {

                RS_VectorSolutions s2 = RS_Information::getIntersection(&trimEntity,
                                                                        e, false);

                std::copy_if(s2.begin(), s2.end(), std::back_inserter(sol), [e, tolerance](const RS_Vector &vp){
                    return vp.valid && e->isPointOnEntity(vp, tolerance);
                });
            }
        }
        return sol;
    }

    RS_Arc *trimCircle(RS_Circle *circle, const RS_Vector &trimCoord, const RS_VectorSolutions &sol){
        double aStart = 0.;
        double aEnd = 2. * M_PI;
        switch (sol.size()) {
            case 0:
                break;
            case 1:
                aStart = circle->getCenter().angleTo(sol.at(0));
                aEnd = aStart + 2. * M_PI;
                break;
            default:
            case 2:
                //trim according to intersections
                std::vector<double> angles;
                const auto &center0 = circle->getCenter();
                for (const RS_Vector &vp: sol) {
                    angles.push_back(center0.angleTo(vp));
                }
                //sort intersections by angle to circle center
                std::sort(angles.begin(), angles.end());
                const double a0 = center0.angleTo(trimCoord);
                for (size_t i = 0; i < angles.size(); ++i) {
                    aStart = angles.at(i);
                    aEnd = angles.at((i + 1) % angles.size());
                    if (RS_Math::isAngleBetween(a0, aStart, aEnd, false))
                        break;
                }
                break;
        }
        RS_ArcData arcData(circle->getCenter(),
                           circle->getRadius(),
                           aStart,
                           aEnd,
                           false);
        return new RS_Arc(circle->getParent(), arcData);
    }

/**
 * @brief getIdFlagString create a string by the entity and ID and type ID.
 * @param entity - entity, could be nullptr
 * @return std::string - "ID/typeID", or an empty string, if the input entity is nullptr
 */
    std::string getIdFlagString(RS_Entity *entity){
        if (entity == nullptr) return {};
        return std::to_string(entity->getId()) + "/" + std::to_string(entity->rtti());
    }

// Support fillet trimming for whole ellipses
    RS_AtomicEntity *trimEllipseForRound(RS_AtomicEntity *entity, const RS_Arc &arcFillet){
        if (entity == nullptr)
            return entity;
        if (entity->rtti() != RS2::EntityEllipse)
            return entity;
        auto ellipse = dynamic_cast<RS_Ellipse *>(entity);
        if (ellipse->isEllipticArc())
            return entity;
        RS_Vector tangent = entity->getNearestPointOnEntity(arcFillet.getCenter(), false);
        RS_Line line{nullptr, {arcFillet.getCenter(), tangent}};
        RS_Vector middle = arcFillet.getMiddlePoint();
        RS_Vector opposite = arcFillet.getCenter() + (arcFillet.getCenter() - middle).normalized() * ellipse->getMinorRadius() * 0.01;
        RS_Vector trimCoord = ellipse->getNearestPointOnEntity(opposite, false);
        RS_VectorSolutions sol = RS_Information::getIntersection(entity, &line, false);
        ellipse->prepareTrim(trimCoord, sol);
        return entity;
    }

// A quick fix for rounding on circles
    RS_AtomicEntity *trimCircleForRound(RS_AtomicEntity *entity, const RS_Arc &arcFillet){
        if (entity == nullptr)
            return entity;
        if (entity->rtti() == RS2::EntityEllipse)
            return trimEllipseForRound(entity, arcFillet);
        if (entity->rtti() != RS2::EntityCircle)
            return entity;
        RS_Line line{nullptr, {arcFillet.getCenter(), entity->getCenter()}};
        RS_Vector middle = arcFillet.getMiddlePoint();
        // prefer acute angle for fillet
        // Use a trimCoord at the opposite side of the arc wrt to the
        RS_Vector opposite = arcFillet.getCenter() + (arcFillet.getCenter() - middle).normalized() * entity->getRadius() * 0.01;
        RS_Vector trimCoord = entity->getNearestPointOnEntity(opposite, true);
        RS_VectorSolutions sol = RS_Information::getIntersection(entity, &line, false);
        RS_Arc *arc = trimCircle(dynamic_cast<RS_Circle *>(entity), trimCoord, sol);
        delete entity;
        return arc;
    }

    inline bool isOneOfPoints(const RS_Vector& candidate, const RS_Vector& point1, const RS_Vector& point2) {
       bool result = point1.distanceTo(candidate) < RS_TOLERANCE || point2.distanceTo(candidate) < RS_TOLERANCE;
       return result;
    }
}


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

    if (container == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::remove: no valid container");
        return;
    }

    std::vector<RS_Entity*> selectedEntities;
    container->collectSelected(selectedEntities, false);
    if (!selectedEntities.empty()) {
        remove(selectedEntities);
    }
    else{
       RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::remove: no valid container is selected");
    }
}

void RS_Modification::remove(const std::vector<RS_Entity*> &entitiesList){
    LC_UndoSection undo( document);

    for(auto e: entitiesList) {
        e->setSelected(false);
        e->changeUndoState();
        undo.addUndoable(e);
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

    std::vector<RS_Entity*> selectedEntities;
    container->collectSelected(selectedEntities, false);
    if (!selectedEntities.empty()) {
        revertDirection(selectedEntities);
    }
}

void RS_Modification::revertDirection(const std::vector<RS_Entity*> &entitiesList){
    std::vector<RS_Entity*> addList;    
    for(auto e: entitiesList) {
            RS_Entity* ec = e->clone();
            ec->revertDirection();
            addList.push_back(ec);            
    }

    deleteOriginalAndAddNewEntities(addList, entitiesList,false, true);
    addList.clear();
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::revertDirection: OK");
}

/**
 * Changes the attributes of all selected
 */
bool RS_Modification::changeAttributes(RS_AttributesData& data){
    return changeAttributes(data, container);
}

bool RS_Modification::changeAttributes(
    RS_AttributesData& data,
    RS_EntityContainer* cont) {

    if (cont == nullptr) {
        return false;
    }
    std::vector<RS_Entity *> selectedEntities;
    container->collectSelected(selectedEntities, false);
    return changeAttributes(data, selectedEntities, cont);
}

bool RS_Modification::changeAttributes(RS_AttributesData& data, const std::vector<RS_Entity*> &entitiesList, RS_EntityContainer *cont){
    LC_UndoSection  undo(document);
    QList<RS_Entity*> clones;
    QSet<RS_Block*> blocks;

    for (auto en: entitiesList) {
        if (data.applyBlockDeep && en->rtti() == RS2::EntityInsert) {
            RS_Block *bl = dynamic_cast<RS_Insert *>(en)->getBlockForInsert();
            blocks << bl;
        }

        RS_Entity *cl = en->clone();
        RS_Pen pen = cl->getPen(false);

        if (data.changeLayer) {
            cl->setLayer(data.layer);
        }

        if (data.changeColor) {
            pen.setColor(data.pen.getColor());
        }
        if (data.changeLineType) {
            pen.setLineType(data.pen.getLineType());
        }
        if (data.changeWidth) {
            pen.setWidth(data.pen.getWidth());
        }
        cl->setPen(pen);

        if (graphicView) {
            graphicView->deleteEntity(en);
        }

        en->setSelected(false);
        cl->setSelected(false);

        clones << cl;

        if (graphic != nullptr) {
            en->setUndoState(true);
            graphic->addUndoable(en);
        }
    }

    for (auto block: blocks) {
        for (auto en: *block) {
            if (en == nullptr) continue;
            en->setSelected(true);
        }
        changeAttributes(data, block);
    }

    for (auto cl: clones) {
        cont->addEntity(cl);

        if (graphicView != nullptr) {;
            graphicView->drawEntity(cl);
        }

        if (graphic != nullptr) {
            graphic->addUndoable(cl);
        }
    }

    if (graphic != nullptr) {
        graphic->updateInserts();
    }

    cont->calculateBorders();

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

    bool selectedEntityFound{false};
    std::vector<RS_Entity *> selected;
    collectSelectedEntities(selected);

    selectedEntityFound = !selected.empty();
    if (selectedEntityFound) {
        RS_Vector refPoint;

        if (ref.valid) {
            refPoint = ref;
        } else { // no ref-point set, determine center of selection
            RS_BoundData bound = getBoundingRect(selected);
            refPoint =  bound.getCenter();
        }

        for (auto e: selected) {
            copyEntity(e, refPoint, cut);
        }
        selected.clear();
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copy: OK");
    }
    else{
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::copy: no valid container is selected");
    }
}

 void RS_Modification::collectSelectedEntities(std::vector<RS_Entity *> &selected) const{
    for (auto e: *container) {
        if (e != nullptr  && e->isSelected()) {
            selected.push_back(e);
        }
    }
}

RS_BoundData RS_Modification::getBoundingRect(std::vector<RS_Entity *> &selected)  {
    RS_Vector selectionCenter;
    RS_Vector min = RS_Vector(10e10, 10e10,0);
    RS_Vector max = RS_Vector(-10e10, -10e10,0);
    for (auto e: selected) {
        const RS_Vector &entityMin = e->getMin();
        const RS_Vector &entityMax = e->getMax();

        min.x = std::min(min.x, entityMin.x);
        min.y = std::min(min.y, entityMin.y);
        max.x = std::max(max.x, entityMax.x);
        max.y = std::max(max.y, entityMax.y);
    }

    RS_BoundData result(min, max);
    return result;
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
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyEntity: to clipboard: %s", getIdFlagString(e).c_str());
    RS_Entity* c = e->clone();

    c->move(-ref);

    // issue #1616: copy&paste a rotated block results in a double rotated block
    // At this point the copied block entities are already rotated, but at
    // pasting, RS_Insert::update() would still rotate the entities again and
    // cause double rotation.
    bool isBlock = c->rtti() == RS2::EntityInsert;
    double angle = isBlock ? dynamic_cast<RS_Insert*>(c)->getAngle() : 0.;
    // issue #1616: A quick fix: rotate back all block entities in the clipboard back by the
    // rotation angle before pasting
    if (isBlock && std::abs(std::remainder(angle, 2. * M_PI)) > RS_TOLERANCE_ANGLE)
    {
        auto* insert = dynamic_cast<RS_Insert*>(c);
        //insert->rotate(insert->getData().insertionPoint, - angle);
        insert->setAngle(0.);
    }

    RS_CLIPBOARD->addEntity(c);
    copyLayers(e);
    copyBlocks(e);

    // set layer to the layer clone:
    c->setLayer(e->getLayer()->getName());

    if (cut) {
        LC_UndoSection undo( document);
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyEntity: cut ID/flag: %s", getIdFlagString(e).c_str());
        e->changeUndoState();
        undo.addUndoable(e);

        // delete entity in graphic view:
        if (graphicView) {
            graphicView->deleteEntity(e);
        }
        e->setSelected(false);
    } else {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyEntity: delete in view ID/flag: %s", getIdFlagString(e).c_str());
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
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyLayers: copy insert entity ID/flag layers: %s", getIdFlagString(e).c_str());
        RS_Block* b = ((RS_Insert*)e)->getBlockForInsert();
        if (!b) {
            RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::copyLayers: could not find block for insert entity");
            return;
        }
        for(auto e2: *b) {
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

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyBlocks: get insert entity ID/flag block: %s", getIdFlagString(e).c_str());
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
            RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::copyBlocks: process insert-into-insert blocks for %s", getIdFlagString(e).c_str());
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

    // scale factor as vector
    RS_Vector vfactor = getPasteScale(data, source, *graphic);
    // select source for paste
    if (source == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::%s(): line %d: no source found", __func__, __LINE__);
        return;
    }

    // default insertion point for container
    RS_Vector ip = data.insertionPoint;

    // remember active layer before inserting absent layers
    RS_Layer *layer = graphic->getActiveLayer();

    // insert absent layers from source to graphic
    if (!pasteLayers(source)) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::paste: unable to copy due to absence of needed layers");
        return;
    }

    if (layer == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::paste: unable to select layer to paste in");
        return;
    }
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::paste: selected layer: %s", layer->getName().toLatin1().data());
    graphic->activateLayer(layer);

    // hash for renaming duplicated blocks
    QHash<QString, QString> blocksDict;

    // create block to paste entities as a whole
    QString name_old = (data.blockName != nullptr) ? data.blockName : "paste-block";
    QString name_new = (graphic->findBlock(name_old) != nullptr) ? graphic->getBlockList()->newName(name_old) : name_old;
    if (graphic->findBlock(name_old) != nullptr) {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::paste: paste block name: %s", name_new.toLatin1().data());
    }
    blocksDict[name_old] = name_new;

    // create block
    RS_Block* b = addNewBlock(name_new, *graphic);

    // create insert object for the paste block
    RS_InsertData di = RS_InsertData(b->getName(), ip, vfactor, data.angle, 1, 1, RS_Vector(0.0,0.0));
    auto* i = new RS_Insert(document, di);
    i->setLayerToActive();
    i->setPenToActive();
    i->reparent(document);
    document->addEntity(i);

    // copy sub-blocks, inserts and entities from source to the paste block
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::paste: copy content to the paste block");
    for(auto e: *source) {

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
    LC_UndoSection undo(document, handleUndo);
    if (!data.asInsert) {
        // no inserts should be selected except from paste block and insert
        container->setSelected(false);
        i->setSelected(true);
        explode(false, true);
        document->removeEntity(i);
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
bool RS_Modification::pasteContainer(RS_Entity* entity, RS_EntityContainer* containerToPaste, QHash<QString, QString>blocksDict, RS_Vector insertionPoint) {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert");

    auto* insert = dynamic_cast<RS_Insert*>(entity);
    if (insert == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: no container to process");
        return false;
    }

    // get block for this insert object
    RS_Block* insertBlock = insert->getBlockForInsert();
    if (insertBlock == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: no block to process");
        return false;
    }
    // get name for this insert object
    QString name_old = insertBlock->getName();
    QString name_new = name_old;
    if (name_old != insert->getName()) {
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
    RS_Block* blockClone = addNewBlock(name_new, *graphic);
    // create insert for the new block
    RS_InsertData di = RS_InsertData(name_new, insertionPoint, RS_Vector(1.0, 1.0), 0.0, 1, 1, RS_Vector(0.0,0.0));
    auto* insertClone = new RS_Insert(containerToPaste, di);
    insertClone->reparent(containerToPaste);
    containerToPaste->addEntity(insertClone);

    // set the same layer in clone as in source
    QString ln = entity->getLayer()->getName();
    RS_Layer* layer = graphic->getLayerList()->find(ln);
    if (!layer) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: unable to select layer to paste in");
        return false;
    }
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: selected layer: %s", layer->getName().toLatin1().data());
    insertClone->setLayer(layer);
    insertClone->setPen(entity->getPen(false));

    // get relative insertion point
    RS_Vector ip{0.0, 0.0};
    if (containerToPaste->getId() != graphic->getId()) {
        ip = blockClone->getBasePoint();
    }

    // copy content of block/insert to destination
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: copy content to the subcontainer");
    for(auto* e: *insert) {

        if(!e) {
            RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Modification::pasteInsert: nullptr entity in block");
            continue;
        }

        if (e->rtti() == RS2::EntityInsert) {
            RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: process sub-insert for %s", ((RS_Insert*)e)->getName().toLatin1().data());
            if (!pasteContainer(e, blockClone, blocksDict, ip)) {
                RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: unable to paste entity to sub-insert");
                return false;
            }
        } else {
            if (!pasteEntity(e, blockClone)) {
                RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: unable to paste entity");
                return false;
            }
        }
    }

    insertClone->update();
    insertClone->setSelected(false);

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: OK");
    return true;
}



/**
 * Paste entity in supplied container
 *
 **/
bool RS_Modification::pasteEntity(RS_Entity* entity, RS_EntityContainer* containerToPaste) {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteEntity");

    if (!entity) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteEntity: no entity to process");
        return false;
    }

    // create entity copy to paste
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteEntity ID/flag: %s", getIdFlagString(entity).c_str());
    RS_Entity* e = entity->clone();

    // set the same layer in clone as in source
    QString ln = entity->getLayer()->getName();
    RS_Layer* layer = graphic->getLayerList()->find(ln);
    if (!layer) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Modification::pasteInsert: unable to select layer to paste in");
        return false;
    }
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Modification::pasteInsert: selected layer: %s", layer->getName().toLatin1().data());
    e->setLayer(layer);
    e->setPen(entity->getPen(false));

    // scaling entity doesn't needed as it scaled with insert object
    // paste entity
    e->reparent(containerToPaste);
    containerToPaste->addEntity(e);
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
    auto* pl1 = new RS_Polyline(container,
                        RS_PolylineData(firstPoint, RS_Vector(0.0,0.0), 0));
    auto* pl2 = new RS_Polyline(container);
    RS_Polyline* pl = pl1;	// Current polyline
	RS_Line* line = nullptr;
	[[maybe_unused]] RS_Arc* arc = nullptr;

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



/*
    Deletes a node from a set of lines, as if it were a polyline; and if no other
    lines are near the selected node/point, then it deletes the line itself.

    - by Melwyn Francis Carlo
*/
void RS_Modification::deleteLineNode(RS_Line* line, const RS_Vector& node)
{
    RS_DEBUG->print("RS_Modification::deleteLineNode");

	if (!container)
    {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::deleteLineNode: no valid container");
		return;
    }

	if (!node.valid)
    {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::deleteLineNode: node not valid");
		return;
    }


    bool nodeIsStartPoint { true };

    if (node == line->getEndpoint()) nodeIsStartPoint = false;


    for (unsigned int i = 0; i < document->count(); i++) // fixme - iterating over all entities
    {
        if (document->entityAt(i)->rtti() == RS2::EntityLine)
        {
            RS_DEBUG->print("RS_Modification::deleteLineNode: connecting (another) line found");

            auto *anotherLine = (RS_Line *) (document->entityAt(i));

            if (line == anotherLine) continue;

            RS_Vector startEndPoints[2] { line->getStartpoint(), line->getEndpoint() };

            if (nodeIsStartPoint)
            {
                RS_DEBUG->print("RS_Modification::deleteLineNode: node is original line's start point");

                if (node.distanceTo(anotherLine->getStartpoint()) < RS_TOLERANCE)
                {
                    startEndPoints[0] = anotherLine->getEndpoint();
                }
                else if (node.distanceTo(anotherLine->getEndpoint()) < RS_TOLERANCE)
                {
                    startEndPoints[0] = anotherLine->getStartpoint();
                }
                else
                {
                    continue;
                }
            }
            else
            {
                RS_DEBUG->print("RS_Modification::deleteLineNode: node is original line's end point");

                if (node.distanceTo(anotherLine->getStartpoint()) < RS_TOLERANCE)
                {
                    startEndPoints[1] = anotherLine->getEndpoint();
                }
                else if (node.distanceTo(anotherLine->getEndpoint()) < RS_TOLERANCE)
                {
                    startEndPoints[1] = anotherLine->getStartpoint();
                }
                else
                {
                    continue;
                }
            }


            RS_DEBUG->print("RS_Modification::deleteLineNode: adding new line and deleting another line");

            RS_Line* newLine { new RS_Line(container, startEndPoints[0], startEndPoints[1]) };

            newLine->setLayer(line->getLayer());
            newLine->setPen(line->getPen());

            container->addEntity(newLine);

            if (graphicView)
            {
                graphicView->drawEntity(newLine);
                graphicView->deleteEntity(anotherLine);
            }

            RS_DEBUG->print("RS_Modification::deleteLineNode: handling new and another line's undo");

            if (handleUndo)
            {
                LC_UndoSection undo(document);

                newLine->setUndoState(false);
                anotherLine->setUndoState(true);

                undo.addUndoable(newLine);
                undo.addUndoable(anotherLine);
            }

            break;
        }
    }


    RS_DEBUG->print("RS_Modification::deleteLineNode: deleting original line");

    if (graphicView) graphicView->deleteEntity(line);

    RS_DEBUG->print("RS_Modification::deleteLineNode: handling original line's undo");

    if (handleUndo)
    {
        LC_UndoSection undo(document);
        line->setUndoState(true);
        undo.addUndoable(line);
    }

    RS_DEBUG->print("RS_Modification::deleteLineNode: OK");
}




/**
 * Deletes a node from a polyline.
 *
 * @param node The node to delete.
 *
 * @return Pointer to the new polyline or nullptr.
 */

RS_Polyline* RS_Modification::deletePolylineNode(RS_Polyline& polyline,
        const RS_Vector& node, bool createOnly) {

    RS_DEBUG->print("RS_Modification::deletePolylineNode");

    if (!container){
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::addPolylineNode: no valid container");
        return nullptr;
    }

    if (!node.valid){
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::deletePolylineNode: node not valid");
        return nullptr;
    }

    // check if the polyline is no longer there after deleting the node:
    if (polyline.count() == 1){
        RS_Entity *e = polyline.firstEntity();
        if (e && e->isAtomic()){
            auto ae = dynamic_cast<RS_AtomicEntity *>(e);
            if (node.distanceTo(ae->getStartpoint()) < 1.0e-6 ||
                node.distanceTo(ae->getEndpoint()) < 1.0e-6){

                if (graphicView){
                    graphicView->deleteEntity(&polyline);
                }

                if (handleUndo){
                    LC_UndoSection undo(document);
                    polyline.setUndoState(true);
                    undo.addUndoable(&polyline);
                }
            }
        }
        return nullptr;
    }

    auto* newPolyline = new RS_Polyline(container);
    newPolyline->setClosed(polyline.isClosed());
    if (!createOnly){
        newPolyline->setSelected(polyline.isSelected());
        newPolyline->setLayer(polyline.getLayer());
        newPolyline->setPen(polyline.getPen());
    }

    // copy polyline and drop deleted node:
    bool first = true;
    bool lastDropped = false;
    RS_Entity* lastEntity = polyline.lastEntity();
    for (auto e: polyline) {

        if (e->isAtomic()){
            auto ae = dynamic_cast<RS_AtomicEntity *>(e);
            double bulge = 0.0;
            if (ae->rtti() == RS2::EntityArc){
                RS_DEBUG->print("RS_Modification::deletePolylineNode: arc segment");
                bulge = ((RS_Arc *) ae)->getBulge();
            } else {
                RS_DEBUG->print("RS_Modification::deletePolylineNode: line segment");
                bulge = 0.0;
            }

            // last entity is closing entity and will be added below with endPolyline()
            if (e == lastEntity && polyline.isClosed()){
                continue;
            }

            // first vertex (startpoint)
            if (first && node.distanceTo(ae->getStartpoint()) > 1.0e-6){
                RS_DEBUG->print("RS_Modification::deletePolylineNode: first node: %f/%f",
                                ae->getStartpoint().x, ae->getStartpoint().y);

                newPolyline->setNextBulge(bulge);
                newPolyline->addVertex(ae->getStartpoint());
                first = false;
            }

            // normal node (not deleted):
            if (first == false && node.distanceTo(ae->getEndpoint()) > 1.0e-6){
                RS_DEBUG->print("RS_Modification::deletePolylineNode: normal vertex found: %f/%f",
                                ae->getEndpoint().x, ae->getEndpoint().y);
                if (lastDropped){
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

    // add new polyline:
    RS_DEBUG->print("RS_Modification::deletePolylineNode: adding new polyline");

    container->addEntity(newPolyline);
    if (!createOnly){
        if (graphicView){
            graphicView->deleteEntity(&polyline);
            graphicView->drawEntity(newPolyline);
        }

        RS_DEBUG->print("RS_Modification::deletePolylineNode: handling undo");
        if (handleUndo){
            LC_UndoSection undo(document);

            polyline.setUndoState(true);
            undo.addUndoable(&polyline);
            undo.addUndoable(newPolyline);
        }
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

RS_Polyline *RS_Modification::deletePolylineNodesBetween(
    RS_Polyline &polyline,
    const RS_Vector &node1, const RS_Vector &node2, bool createOnly){

    RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween");

    if (!container){
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::addPolylineNodesBetween: no valid container");
        return nullptr;
    }

    if (node1.valid == false || node2.valid == false){
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::deletePolylineNodesBetween: node not valid");
        return nullptr;
    }

    if (node1.distanceTo(node2) < 1.0e-6){
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::deletePolylineNodesBetween: nodes are identical");
        return nullptr;
    }

    // check if there's nothing to delete:
    for (auto e: polyline) {

        if (e->isAtomic()){
            auto *ae = dynamic_cast<RS_AtomicEntity *>(e);
            /// FIXME- RS_TOLERANCE?
            if ((node1.distanceTo(ae->getStartpoint()) < RS_TOLERANCE  &&
                 node2.distanceTo(ae->getEndpoint()) < 1.0e-6) ||
                (node2.distanceTo(ae->getStartpoint()) < 1.0e-6 &&
                 node1.distanceTo(ae->getEndpoint()) < 1.0e-6)){

                RS_DEBUG->print(RS_Debug::D_WARNING,
                                "RS_Modification::deletePolylineNodesBetween: nothing to delete");
                return nullptr;
            }
        }
    }


    // check if the start point is involved:
    const RS_Vector &polylineStartpoint = polyline.getStartpoint();
    bool startpointInvolved = isOneOfPoints(polylineStartpoint, node1, node2);


    // check which part of the polyline has to be deleted:
    bool deleteStart = false;
    if (polyline.isClosed()){
        bool found = false;
        double length1 = 0.0;
        double length2 = 0.0;
        RS_Entity *e = polyline.firstEntity();

        if (startpointInvolved){
            if (e->isAtomic()){
                auto *ae = dynamic_cast<RS_AtomicEntity *>(e);
                length1 += ae->getLength();
            }
            e = polyline.nextEntity();
        }
        for (; e; e = polyline.nextEntity()) {

            if (e->isAtomic()){
                auto *ae = dynamic_cast<RS_AtomicEntity *>(e);

                if (isOneOfPoints(ae->getStartpoint(), node1, node2)){
                    found = !found;
                }

                if (found){
                    length2 += ae->getLength();
                } else {
                    length1 += ae->getLength();
                }
            }
        }
        if (length1 < length2){
            deleteStart = true;
        } else {
            deleteStart = false;
        }
    }

    auto *newPolyline = new RS_Polyline(container);
    newPolyline->setClosed(polyline.isClosed());
    if (!createOnly){
        newPolyline->setSelected(polyline.isSelected());
        newPolyline->setLayer(polyline.getLayer());
        newPolyline->setPen(polyline.getPen());
    }

    if (startpointInvolved && deleteStart && polyline.isClosed()){
        newPolyline->setNextBulge(0.0);
        newPolyline->addVertex(polylineStartpoint);
    }

    // copy polyline and drop deleted nodes:
    bool first = true;
    bool removing = deleteStart;
    bool done = false;
    bool nextIsStraight = false;
    RS_Entity *lastEntity = polyline.lastEntity();
    int i = 0;
    double bulge = 0.0;

    for (auto e: polyline) {

        RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: entity: %d", i++);
        RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: removing: %d", (int) removing);

        if (e->isAtomic()){
            auto ae = dynamic_cast<RS_AtomicEntity *>(e);
            if (ae->rtti() == RS2::EntityArc){
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: arc segment");
                auto arc = dynamic_cast<RS_Arc *>(ae);
                bulge = arc->getBulge();
            } else {
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: line segment");
                bulge = 0.0;
            }

            const RS_Vector &endpoint = ae->getEndpoint();
            const RS_Vector &startpoint = ae->getStartpoint();

            // last entity is closing entity and will be added below with endPolyline()
            if (e == lastEntity && polyline.isClosed()){
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: dropping last vertex of closed polyline");
                continue;
            }

            // first vertex (startpoint)
            if (first){
                if (!removing){
                    RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: first node: %f/%f", startpoint.x, startpoint.y);
                    newPolyline->setNextBulge(bulge);
                    newPolyline->addVertex(startpoint);
                    first = false;
                }
            }


            // stop removing nodes:

            if (removing == true && isOneOfPoints(endpoint, node1, node2)){
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: stop removing at: %f/%f",endpoint.x, endpoint.y);
                removing = false;
                done = true;
                if (first == false){
                    nextIsStraight = true;
                }
            }

            // normal node (not deleted):
            if (removing == false && (done == false || deleteStart == false)){
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: normal vertex shouldRemove: %f/%f", endpoint.x, endpoint.y);
                if (nextIsStraight){
                    bulge = 0.0;
                    nextIsStraight = false;
                }
                newPolyline->setNextBulge(bulge);
                newPolyline->addVertex(endpoint);
            }
                // drop deleted node:
            else {
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: deleting vertex: %f/%f", endpoint.x, endpoint.y);
            }

            // start to remove nodes from now on:
            if (done == false && removing == false && isOneOfPoints(endpoint, node1, node2)){
                RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: start removing at: %f/%f", endpoint.x, endpoint.y);
                removing = true;
            }

            if (done){
                done = false;
            }
        } else {
            RS_DEBUG->print(RS_Debug::D_WARNING,"RS_Modification::deletePolylineNodesBetween: Polyline contains non-atomic entities");
        }
    }

    RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: ending polyline");
    newPolyline->setNextBulge(polyline.getClosingBulge());
    newPolyline->endPolyline();

    // add new polyline:
    RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: adding new polyline");
    container->addEntity(newPolyline);
    if (!createOnly){
        if (graphicView){
            graphicView->deleteEntity(&polyline);
            graphicView->drawEntity(newPolyline);
        }

        RS_DEBUG->print("RS_Modification::deletePolylineNodesBetween: handling undo");
        if (handleUndo){
            LC_UndoSection undo(document);

            polyline.setUndoState(true);
            undo.addUndoable(&polyline);
            undo.addUndoable(newPolyline);
        }
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
RS_Polyline *RS_Modification::polylineTrim(
    RS_Polyline &polyline,
    RS_AtomicEntity &segment1,
    RS_AtomicEntity &segment2,
    bool createOnly){

    RS_DEBUG->print("RS_Modification::polylineTrim");

    if (!container){
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::addPolylineNodesBetween: no valid container");
        return nullptr;
    }

    if (segment1.getParent() != &polyline || segment2.getParent() != &polyline){
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::polylineTrim: segments not in polyline");
        return nullptr;
    }

    if (&segment1 == &segment2){
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::polylineTrim: segments are identical");
        return nullptr;
    }

    RS_VectorSolutions sol;
    sol = RS_Information::getIntersection(&segment1, &segment2, false);

    if (sol.getNumber() == 0){
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::polylineTrim: segments cannot be trimmed");
        return nullptr;
    }

    // check which segment comes first in the polyline:
    RS_AtomicEntity *firstSegment;
    if (polyline.findEntity(&segment1) > polyline.findEntity(&segment2)){
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

    auto *newPolyline = new RS_Polyline(container);
    newPolyline->setClosed(polyline.isClosed());
    if (!createOnly){
        newPolyline->setSelected(polyline.isSelected());
        newPolyline->setLayer(polyline.getLayer());
        newPolyline->setPen(polyline.getPen());
    }

    // normal trimming: start removing nodes at trim segment. ends stay the same
    if (!reverseTrim){
        // copy polyline, trim segments and drop between nodes:
        bool first = true;
        bool removing = false;
        bool nextIsStraight = false;
        RS_Entity *lastEntity = polyline.lastEntity();
        for (auto e: polyline) {

            if (e->isAtomic()){
                auto ae = dynamic_cast<RS_AtomicEntity *>(e);
                double bulge = 0.0;
                if (ae->rtti() == RS2::EntityArc){
                    RS_DEBUG->print("RS_Modification::polylineTrim: arc segment");
                    bulge = ((RS_Arc *) ae)->getBulge();
                } else {
                    RS_DEBUG->print("RS_Modification::polylineTrim: line segment");
                    bulge = 0.0;
                }

                // last entity is closing entity and will be added below with endPolyline()
                if (e == lastEntity && polyline.isClosed()){
                    RS_DEBUG->print("RS_Modification::polylineTrim: "
                                    "dropping last vertex of closed polyline");
                    continue;
                }

                // first vertex (startpoint)
                if (first){
                    RS_DEBUG->print("RS_Modification::polylineTrim: first node: %f/%f",
                                    ae->getStartpoint().x, ae->getStartpoint().y);

                    newPolyline->setNextBulge(bulge);
                    newPolyline->addVertex(ae->getStartpoint());
                    first = false;
                }

                // trim and start removing nodes:
                if (!removing && (ae == &segment1 || ae == &segment2)){
                    RS_DEBUG->print("RS_Modification::polylineTrim: "
                                    "start removing at trim point %f/%f",
                                    sol.get(0).x, sol.get(0).y);
                    newPolyline->setNextBulge(0.0);
                    newPolyline->addVertex(sol.get(0));
                    removing = true;
                    nextIsStraight = true;
                }

                    // stop removing nodes:
                else if (removing && (ae == &segment1 || ae == &segment2)){
                    RS_DEBUG->print("RS_Modification::polylineTrim: stop removing at: %f/%f",
                                    ae->getEndpoint().x, ae->getEndpoint().y);
                    removing = false;
                }

                // normal node (not deleted):
                if (!removing){
                    RS_DEBUG->print("RS_Modification::polylineTrim: normal vertex found: %f/%f",
                                    ae->getEndpoint().x, ae->getEndpoint().y);
                    if (nextIsStraight){
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
        RS_Entity *lastEntity = polyline.lastEntity();
        for (auto e: polyline) {

            if (e->isAtomic()){
                auto *ae = dynamic_cast<RS_AtomicEntity *>(e);
                double bulge = 0.0;
                if (ae->rtti() == RS2::EntityArc){
                    RS_DEBUG->print("RS_Modification::polylineTrim: arc segment");
                    auto arc = dynamic_cast<RS_Arc *>(ae);
                    bulge = arc ->getBulge();
                } else {
                    RS_DEBUG->print("RS_Modification::polylineTrim: line segment");
                    bulge = 0.0;
                }

                // last entity is closing entity and will be added below with endPolyline()
                if (e == lastEntity && polyline.isClosed()){
                    RS_DEBUG->print("RS_Modification::polylineTrim: "
                                    "dropping last vertex of closed polyline");
                    continue;
                }

                // trim and stop removing nodes:
                if (removing == true && (ae == &segment1 || ae == &segment2)){
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
                else if (removing == false && (ae == &segment1 || ae == &segment2)){
                    RS_DEBUG->print("RS_Modification::polylineTrim: start removing at: %f/%f",
                                    ae->getEndpoint().x, ae->getEndpoint().y);
                    newPolyline->setNextBulge(0.0);
                    // start of new polyline:
                    newPolyline->addVertex(sol.get(0));
                    removing = true;
                }

                // normal node (not deleted):
                if (removing == false){
                    RS_DEBUG->print("RS_Modification::polylineTrim: normal vertex found: %f/%f",
                                    ae->getEndpoint().x, ae->getEndpoint().y);
                    if (nextIsStraight){
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
    if (!createOnly){
        if (graphicView){
            graphicView->deleteEntity(&polyline);
            graphicView->drawEntity(newPolyline);
        }

        RS_DEBUG->print("RS_Modification::polylineTrim: handling undo");
        if (handleUndo){
            LC_UndoSection undo(document);

            polyline.setUndoState(true);
            undo.addUndoable(&polyline);
            undo.addUndoable(newPolyline);
        }
    }

    return newPolyline;
}

/**
 * Moves all selected entities with the given data for the move
 * modification.
 */
bool RS_Modification::move(RS_MoveData &data, bool previewOnly, [[maybe_unused]]RS_EntityContainer* previewContainer){
    if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::move: no valid container");
        return false;
    }
    std::vector<RS_Entity*> selectedEntities;
    container->collectSelected(selectedEntities, false);
    return move(data, selectedEntities, previewOnly, true);
}


bool RS_Modification::move(RS_MoveData& data, const std::vector<RS_Entity*> &entitiesList, bool forPreviewOnly, bool keepSelected) {

    int numberOfCopies = data.obtainNumberOfCopies();
    std::vector<RS_Entity*> addList;

    // too slow:
    for(auto e: entitiesList){
        // Create new entities
        for (int num= 1; num <= numberOfCopies; num++) {
            RS_Entity* ec = e->clone();

            ec->move(data.offset*num);
            if (!forPreviewOnly){
                if (data.useCurrentLayer) {
                    ec->setLayerToActive();
                }
                if (data.useCurrentAttributes) {
                    ec->setPenToActive();
                }
            }
            if (ec->rtti()==RS2::EntityInsert) {
                ((RS_Insert*)ec)->update();
            }
            // since 2.0.4.0: keep selection
            ec->setSelected(keepSelected);
            addList.push_back(ec);
        }
    }

    deleteOriginalAndAddNewEntities(addList, entitiesList, forPreviewOnly, !data.keepOriginals);
    addList.clear();
    return true;
}


/**
 * Offset all selected entities with the given mouse position and distance
 *
 *@Author: Dongxu Li
 */
bool RS_Modification::offset(const RS_OffsetData& data, [[maybe_unused]]bool previewOnly,[[maybe_unused]] RS_EntityContainer* previewContainer) {
    if (container == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::offset: no valid container");
        return false;
    }

    std::vector<RS_Entity*> selectedEntities;
    container->collectSelected(selectedEntities, false);
    return offset(data, selectedEntities, false, true);
}

bool RS_Modification::offset(const RS_OffsetData& data, const std::vector<RS_Entity*> &entitiesList, bool forPreviewOnly, bool keepSelected) {
    std::vector<RS_Entity*> addList;

    int numberOfCopies = data.obtainNumberOfCopies();

    // Create new entities
    // too slow:
    for(auto e: entitiesList){
        for (int num=1; num<= numberOfCopies; num++) {
            auto ec = e->clone();
            //highlight is used by trim actions. do not carry over flag
            ec->setHighlighted(false);

            if (!ec->offset(data.coord, num*data.distance)) {
                delete ec;
                continue;
            }
            if (!forPreviewOnly) {
                if (data.useCurrentLayer) {
                    ec->setLayerToActive();
                }
                if (data.useCurrentAttributes) {
                    ec->setPenToActive();
                }
            }
            if (ec->rtti()==RS2::EntityInsert) {
                auto* insert = dynamic_cast<RS_Insert *>(ec);
                insert->update();
            }
            // since 2.0.4.0: keep selection
            ec->setSelected(keepSelected);
            addList.push_back(ec);
        }
    }

    deleteOriginalAndAddNewEntities(addList, entitiesList, forPreviewOnly, !data.keepOriginals);
    addList.clear();
    return true;
}

/**
 * Rotates all selected entities with the given data for the rotation.
 */
bool RS_Modification::rotate(RS_RotateData &data, bool forPreviewOnly){
    if (!container){
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::rotate: no valid container");
        return false;
    }

    std::vector<RS_Entity*> selectedEntities;
    container->collectSelected(selectedEntities, false);
    return rotate(data, selectedEntities, forPreviewOnly);
}

bool RS_Modification::rotate(RS_RotateData& data, const std::vector<RS_Entity*> &entitiesList, bool forPreviewOnly) {
    std::vector<RS_Entity *> addList;
    // Create new entities

    int numberOfCopies = data.obtainNumberOfCopies();
    for (auto e: entitiesList) {
        for (int num = 1; num <= numberOfCopies; num++) {

            RS_Entity *ec = e->clone();
            ec->setSelected(false);

            double rotationAngle = data.angle * num;
            ec->rotate(data.center, rotationAngle);

            bool rotateTwice = data.twoRotations;
            double distance = data.refPoint.distanceTo(data.center);
            if (distance < RS_TOLERANCE){
                rotateTwice = false;
            }

            if (rotateTwice) {
                RS_Vector rotatedRefPoint = data.refPoint;
                rotatedRefPoint.rotate(data.center, rotationAngle);

                double secondRotationAngle = data.secondAngle;
                if (data.secondAngleIsAbsolute){
                    secondRotationAngle -= rotationAngle;
                }
                ec->rotate(rotatedRefPoint, secondRotationAngle);
            }
            if (!forPreviewOnly) {
                if (data.useCurrentLayer) {
                    ec->setLayerToActive();
                }
                if (data.useCurrentAttributes) {
                    ec->setPenToActive();
                }
            }
            if (ec->rtti() == RS2::EntityInsert) {
                ((RS_Insert *) ec)->update();
            }
            addList.push_back(ec);
        }
    }

    deleteOriginalAndAddNewEntities(addList, entitiesList, forPreviewOnly, !data.keepOriginals);
    addList.clear();
    
    return true;
}

/**
 * Moves all selected entities with the given data for the scale
 * modification.
 */
bool RS_Modification::scale(RS_ScaleData& data, bool forPreviewOnly) {
    if (container == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::scale: no valid container");
        return false;
    }
    
    std::vector<RS_Entity*> selectedEntities;
    container->collectSelected(selectedEntities, false);
    return scale(data, selectedEntities, forPreviewOnly);
}

bool RS_Modification::scale(RS_ScaleData& data, const std::vector<RS_Entity*> &entitiesList, bool forPreviewOnly) {
    std::vector<RS_Entity*> selectedList,addList;

    for(auto ec: entitiesList){
        if ( !data.isotropicScaling ) {
            RS2::EntityType rtti = ec->rtti();
            if (rtti == RS2::EntityCircle ) {
                //non-isotropic scaling, replacing selected circles with ellipses
                auto* c=dynamic_cast<RS_Circle*>(ec);
                ec= new RS_Ellipse{container,
                                   {c->getCenter(), {c->getRadius(),0.},
                                    1.,
                                    0., 0., false}};
            } else if (rtti == RS2::EntityArc ) {
                //non-isotropic scaling, replacing selected arcs with ellipses
                auto *c=dynamic_cast<RS_Arc*>(ec);
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

    int numberOfCopies = data.obtainNumberOfCopies();

    // Create new entities
    for(RS_Entity* e: selectedList) {
        if (e != nullptr) {
            for (int num= 1; num <= numberOfCopies; num++) {
                auto ec = e->clone();
                ec->setSelected(false);

                ec->scale(data.referencePoint, RS_Math::pow(data.factor, num));

                if (!forPreviewOnly) {
                    if (data.useCurrentLayer) {
                        ec->setLayerToActive();
                    }
                    if (data.useCurrentAttributes) {
                        ec->setPenToActive();
                    }
                }
                
                if (ec->rtti()==RS2::EntityInsert) {
                    auto insert = dynamic_cast<RS_Insert *>(ec);
                    insert->update();
                }
                addList.push_back(ec);
            }
        }
    }
    selectedList.clear();
    deleteOriginalAndAddNewEntities(addList,entitiesList, forPreviewOnly, !data.keepOriginals);
    addList.clear();

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

    std::vector<RS_Entity*> selectedEntities;
    container->collectSelected(selectedEntities, false);
    return mirror(data, selectedEntities, false);
}

bool RS_Modification::mirror(RS_MirrorData& data, const std::vector<RS_Entity*> &entitiesList, bool forPreviewOnly) {

    std::vector<RS_Entity*> addList;

//    int numberOfCopies = obtainNumberOfCopies(data);
    int numberOfCopies = 1; // fixme - think about support of multiple copies.... may it be be something like moving the central point of selection? Like mirror+move?

    // Create new entities

    for(auto e: entitiesList){
        for (int num=1; num<=numberOfCopies; ++num) {
            RS_Entity* ec = e->clone();
            ec->setSelected(false);

            ec->mirror(data.axisPoint1, data.axisPoint2);
            if (!forPreviewOnly) {
                if (data.useCurrentLayer) {
                    ec->setLayerToActive();
                }
                if (data.useCurrentAttributes) {
                    ec->setPenToActive();
                }
            }
            if (ec->rtti()==RS2::EntityInsert) {
                ((RS_Insert*)ec)->update();
            }
            addList.push_back(ec);
        }
    }

    deleteOriginalAndAddNewEntities(addList, entitiesList, forPreviewOnly, !data.keepOriginals);
    addList.clear();
    return true;
}

/**
 * Rotates entities around two centers with the given parameters.
 */
bool RS_Modification::rotate2(RS_Rotate2Data& data,[[maybe_unused]] bool forPreviewOnly) {
    if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::rotate2: no valid container");
        return false;
    }
    std::vector<RS_Entity*> selectedEntities;
    container->collectSelected(selectedEntities, false);
    return rotate2(data, selectedEntities, false);
}

bool RS_Modification::rotate2(RS_Rotate2Data& data, const std::vector<RS_Entity*> &entitiesList, bool forPreviewOnly) {

    std::vector<RS_Entity*> addList;

    int numberOfCopies = data.obtainNumberOfCopies();

    // Create new entities

    for(auto e: entitiesList){
        for (int num= 1; num <= numberOfCopies; num++) {
            RS_Entity* ec = e->clone();
            ec->setSelected(false);

            double angle1ForCopy = /*data.sameAngle1ForCopies ?  data.angle1 :*/ data.angle1 * num;
            double angle2ForCopy = data.sameAngle2ForCopies ?  data.angle2 : data.angle2 * num;

            ec->rotate(data.center1, angle1ForCopy);

            RS_Vector center2 = data.center2;
            center2.rotate(data.center1, angle1ForCopy);

            ec->rotate(center2, angle2ForCopy);
            if (!forPreviewOnly) {
                if (data.useCurrentLayer) {
                    ec->setLayerToActive();
                }
                if (data.useCurrentAttributes) {
                    ec->setPenToActive();
                }
            }
            if (ec->rtti()==RS2::EntityInsert) {
                ((RS_Insert*)ec)->update();
            }
            addList.push_back(ec);
        }
    }

    deleteOriginalAndAddNewEntities(addList, entitiesList, forPreviewOnly, !data.keepOriginals);
    addList.clear();
    return true;
}

void RS_Modification::deleteOriginalAndAddNewEntities(const std::vector<RS_Entity*> &addList, const std::vector<RS_Entity*> &originalEntities, bool addOnly, bool deleteOriginals, bool forceUndoable){
    LC_UndoSection undo( document, handleUndo); // bundle remove/add entities in one undoCycle
    if (addOnly) {
        for (RS_Entity *e: addList) {
            if (e != nullptr) {
                container->addEntity(e);
            }
        }
    } else {
        deselectOriginals(originalEntities,deleteOriginals);
        addNewEntities(addList, forceUndoable);
    }
}

/**
 * Moves and rotates entities with the given parameters.
 */
bool RS_Modification::moveRotate(RS_MoveRotateData& data, bool previewOnly, [[maybe_unused]]RS_EntityContainer* previewContainer) {
    if (!container) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::moveRotate: no valid container");
        return false;
    }

    std::vector<RS_Entity*> selectedEntities;
    container->collectSelected(selectedEntities, false);
    return moveRotate(data, selectedEntities, previewOnly);
}

bool RS_Modification::moveRotate(RS_MoveRotateData &data, const std::vector<RS_Entity*> &entitiesList, bool forPreviewOnly){
    std::vector<RS_Entity*> addList;

    int numberOfCopies = data.obtainNumberOfCopies();

    // Create new entities
    for(auto e: entitiesList){
        for (int num=1; num <= numberOfCopies; ++num) {
            RS_Entity* ec = e->clone();
            ec->setSelected(false);

            const RS_Vector &offset = data.offset * num;
            ec->move(offset);
            double angleForCopy = data.sameAngleForCopies ?  data.angle : data.angle * num;
            ec->rotate(data.referencePoint + offset, angleForCopy);
            if (forPreviewOnly) {
                if (data.useCurrentLayer) {
                    ec->setLayerToActive();
                }
                if (data.useCurrentAttributes) {
                    ec->setPenToActive();
                }
            }
            if (ec->rtti()==RS2::EntityInsert) {
                ((RS_Insert*)ec)->update();
            }
            addList.push_back(ec);
        }
    }

    deleteOriginalAndAddNewEntities(addList, entitiesList, forPreviewOnly, !data.keepOriginals);
    addList.clear();
    return true;
}

/**
 * Deselects all selected entities and removes them if remove is true;
 *
 * @param remove true: Remove entities.
 */
void RS_Modification::deselectOriginals(bool remove) {
    LC_UndoSection undo(document, handleUndo);

    for (auto e: *container) {
        if (e != nullptr) {
            if (e->isSelected()) {
                e->setSelected(false);
                if (remove) {
                    e->changeUndoState();
                    undo.addUndoable(e);
                }
            }
        }
    }
}

void RS_Modification::deselectOriginals(const std::vector<RS_Entity*> &entitiesList, bool remove) {
    LC_UndoSection undo(document, handleUndo);

    for (auto e: entitiesList) {
        e->setSelected(false);
        if (remove) {
            e->changeUndoState();
            undo.addUndoable(e);
        }
    }
}

/**
 * Adds the given entities to the container and draws the entities if
 * there's a graphic view available.
 *
 * @param addList Entities to add.
 */
void RS_Modification::addNewEntities(const std::vector<RS_Entity*>& addList, bool forceUndoable)
{
    LC_UndoSection undo( document, handleUndo || forceUndoable);

    for (RS_Entity* e: addList) {
        if (e) {
            container->addEntity(e);
            undo.addUndoable(e);
        }
    }

    container->calculateBorders();

    if (graphicView) { // fixme - remove
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
 * @param forPreview true: used in preview, no entities are added to the document.
 *
 */
LC_TrimResult RS_Modification::trim(const RS_Vector& trimCoord,
                           RS_AtomicEntity* trimEntity,
                           const RS_Vector& limitCoord,
                           RS_Entity* limitEntity,
                           bool both,
                           bool forPreview) {

    LC_TrimResult result;
    if (trimEntity == nullptr || limitEntity == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::trim: At least one entity is nullptr");
        return result;
    }

    if (both && !limitEntity->isAtomic()) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::trim: limitEntity is not atomic");
    }
    if(trimEntity->isLocked()|| !trimEntity->isVisible()) return result;

    RS_VectorSolutions sol = findIntersection(*trimEntity, *limitEntity);

//if intersection are in start or end point can't trim/extend in this point, remove from solution. sf.net #3537053
    if (trimEntity->rtti()==RS2::EntityLine){
        auto *lin = dynamic_cast<RS_Line *>(trimEntity);
        for (unsigned int i=0; i< sol.size(); i++) {
            RS_Vector v = sol.at(i);
            if (v == lin->getStartpoint())
                sol.removeAt(i);
            else if (v == lin->getEndpoint())
                sol.removeAt(i);
        }
    }

	if (!sol.hasValid()) {
        return both ? trim( limitCoord, (RS_AtomicEntity*)limitEntity, trimCoord, trimEntity, false, forPreview) : result;
    }

	RS_AtomicEntity* trimmed1 = nullptr;
	RS_AtomicEntity* trimmed2 = nullptr;

    if (trimEntity->rtti()==RS2::EntityCircle) {
        // convert a circle into a trimmable arc, need to start from intersections
        trimmed1 = trimCircle(dynamic_cast<RS_Circle*>(trimEntity), trimCoord, sol);
    } else {
        trimmed1 = (RS_AtomicEntity*)trimEntity->clone();
        trimmed1->setHighlighted(false);
    }

    // trim trim entity
	size_t ind = 0;
    RS_Vector is(false), is2(false);

    //RS2::Ending ending = trimmed1->getTrimPoint(trimCoord, is);
    if ( trimEntity->trimmable() ) {
        is = trimmed1->prepareTrim(trimCoord, sol);
    } else {
		is = sol.getClosest(limitCoord, nullptr, &ind);
		//sol.getClosest(limitCoord, nullptr, &ind);
        RS_DEBUG->print("RS_Modification::trim: limitCoord: %f/%f", limitCoord.x, limitCoord.y);
        RS_DEBUG->print("RS_Modification::trim: sol.get(0): %f/%f", sol.get(0).x, sol.get(0).y);
        RS_DEBUG->print("RS_Modification::trim: sol.get(1): %f/%f", sol.get(1).x, sol.get(1).y);
        RS_DEBUG->print("RS_Modification::trim: ind: %lu", ind);
        is2 = sol.get(ind==0 ? 1 : 0);
        //RS_Vector is2 = sol.get(ind);
        RS_DEBUG->print("RS_Modification::trim: is2: %f/%f", is2.x, is2.y);

    }
    if (!forPreview) {
        // remove trim entity from view:
        if (graphicView) {
            graphicView->deleteEntity(trimEntity);
        }
    }

    // remove limit entity from view:
    bool trimBoth= both && !limitEntity->isLocked() && limitEntity->isVisible();
    if (trimBoth) {
        trimmed2 = (RS_AtomicEntity*)limitEntity->clone();
        if (!forPreview) {
            trimmed2->setHighlighted(false);
            if (graphicView) {
                graphicView->deleteEntity(limitEntity);
            }
        }
    }

    trimEnding(trimCoord, trimmed1, is);



    // trim limit entity:
    if (trimBoth) {
        if ( trimmed2->trimmable()) {
            is2 = trimmed2->prepareTrim(limitCoord, sol);
        }
        else {
            is2 = sol.getClosest(trimCoord);
        }

        trimEnding(limitCoord, trimmed2, is2);
    }

    if (!forPreview) {
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
    }
    if (!forPreview) {
        if (handleUndo) {
            LC_UndoSection undo(document);

            undo.addUndoable(trimmed1);
            trimEntity->setUndoState(true);
            undo.addUndoable(trimEntity);
            if (trimBoth) {
                undo.addUndoable(trimmed2);
                limitEntity->setUndoState(true);
                undo.addUndoable(limitEntity);
            }
        }
    }
    result.result = true;
    result.trimmed1 = trimmed1;
    result.trimmed2 = trimmed2;
    result.intersection1 = is;
    result.intersection2 = is2;

    if (trimmed1->isArc()){
        result.intersection1 = trimmed1->getStartpoint();
        result.intersection2 = trimmed1->getEndpoint();
    }

    return result;
}

void RS_Modification::trimEnding(const RS_Vector &trimCoord, RS_AtomicEntity *trimmed1, const RS_Vector &is) const {
    RS2::Ending ending = trimmed1->getTrimPoint(trimCoord, is);
    switch (ending) {
        case RS2::EndingStart: {
            trimmed1->trimStartpoint(is);
            break;
        }
        case RS2::EndingEnd: {
            trimmed1->trimEndpoint(is);
            break;
        }
        default:
            break;
    }
}


/**
 * Trims or extends the given trimEntity by the given amount.
 *
 * @param trimCoord Coordinate which defines which endpoint of the
 *   trim entity to trim.
 * @param trimEntity Entity which will be trimmed.
 * @param dist Amount to trim by.
 */
RS_Entity* RS_Modification::trimAmount(const RS_Vector& trimCoord,
                                 RS_AtomicEntity* trimEntity,
                                 double dist,
                                 bool trimBoth,
                                 bool &trimStart, bool &trimEnd,
                                 bool forPreview) {

    if (!trimEntity){
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::trimAmount: Entity is nullptr");
        return nullptr;
    }

    if (trimEntity->isLocked() || !trimEntity->isVisible()) {
        return nullptr;
    }

    RS_AtomicEntity *trimmed = nullptr;

    // remove trim entity:
    trimmed = (RS_AtomicEntity *) trimEntity->clone();

    if (graphicView != nullptr){
        graphicView->deleteEntity(trimEntity);
    }

    // trim trim entity

    trimStart = false;
    trimEnd = false;
    if (trimBoth){
        RS_Vector isStart = trimmed->getNearestDist(-dist, trimmed->getStartpoint());
        RS_Vector isEnd = trimmed->getNearestDist(-dist, trimmed->getEndpoint());

        trimmed->trimStartpoint(isStart);
        trimmed->trimEndpoint(isEnd );
        trimStart = true;
        trimEnd = true;
    }
    else {
        RS_Vector is = trimmed->getNearestDist(-dist, trimCoord);

        if (trimCoord.distanceTo(trimmed->getStartpoint()) <
            trimCoord.distanceTo(trimmed->getEndpoint())){
            trimmed->trimStartpoint(is);
            trimStart = true;
        } else {
            trimmed->trimEndpoint(is);
            trimEnd = true;
        }
    }

    if (!forPreview){
        // add new trimmed trim entity:
        if (container != nullptr){
            container->addEntity(trimmed);
        }

        if (graphicView != nullptr){
            graphicView->drawEntity(trimmed);
        }

        if (handleUndo){
            LC_UndoSection undo(document);

            undo.addUndoable(trimmed);
            trimEntity->setUndoState(true);
            undo.addUndoable(trimEntity);
        }
    }
    return trimmed;
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
        a=dynamic_cast<RS_Circle*>(cutEntity)->getCenter().angleTo(cutCoord);
        cut1 = new RS_Arc(cutEntity->getParent(),
                          RS_ArcData(dynamic_cast<RS_Circle*>(cutEntity) ->getCenter(),
                                     dynamic_cast<RS_Circle*>(cutEntity) ->getRadius(),
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
        const auto* const ellipse=static_cast<const RS_Ellipse*>(cutEntity);
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
                              const RS_Vector& offset,
                              bool removeOriginals) {

    if (!offset.valid) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::stretch: Offset invalid");
        return false;
    }

    std::vector<RS_Entity*> addList;

// Create new entities
    for(auto e: *container){ // fixme - sand - iteration over all entities in container
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
    if (removeOriginals) { // todo - so far, it seems better to stay with
        deselectOriginals(true); // fixme - entities are not selected, so this is error - fix it
    }
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
LC_BevelResult* RS_Modification::bevel(
    const RS_Vector &coord1, RS_AtomicEntity *entity1,
    const RS_Vector &coord2, RS_AtomicEntity *entity2,
    RS_BevelData &data,
    bool previewOnly){

    RS_DEBUG->print("RS_Modification::bevel");

    if (!(entity1 && entity2)){
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::bevel: At least one entity is nullptr");
        return nullptr;
    }
    if (entity1->isLocked() || !entity1->isVisible()) return nullptr;
    if (entity2->isLocked() || !entity2->isVisible()) return nullptr;

    RS_EntityContainer *baseContainer = container;
    bool isPolyline = false;
//    bool isClosedPolyline = false;

    LC_UndoSection undo(document, handleUndo);

    // find out whether we're bevelling within a polyline:

    auto* result = new LC_BevelResult();

    //fixme - that check should be in action too
    if (entity1->getParent() &&
        entity1->getParent()->rtti() == RS2::EntityPolyline){
        RS_DEBUG->print("RS_Modification::bevel: trimming polyline segments");
        if (entity1->getParent() != entity2->getParent()){
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "RS_Modification::bevel: entities not in the same polyline");
            result->error = LC_BevelResult::ERR_NOT_THE_SAME_POLYLINE;
            return result;
        }
        //TODO: check if entity1 & entity2 are lines.
        //bevel only can be with lines.

        // clone polyline for undo
        auto *cl = dynamic_cast<RS_EntityContainer *>(entity1->getParent()->clone());
        baseContainer = cl;
        if (handleUndo && !previewOnly){
            container->addEntity(cl);
            //cl->setUndoState(true);
            undo.addUndoable(cl);

            undo.addUndoable(entity1->getParent());
            entity1->getParent()->setUndoState(true);

        }

        entity1 = (RS_AtomicEntity *) baseContainer->entityAt(entity1->getParent()->findEntity(entity1));
        entity2 = (RS_AtomicEntity *) baseContainer->entityAt(entity2->getParent()->findEntity(entity2));

        //baseContainer = entity1->getParent();
        isPolyline = true;
//        isClosedPolyline = ((RS_Polyline*)entity1)->isClosed();
    }

    RS_DEBUG->print("RS_Modification::bevel: getting intersection");

    RS_VectorSolutions sol =
        RS_Information::getIntersection(entity1, entity2, false);

    if (sol.getNumber() == 0){
        result->error = LC_BevelResult::ERR_NO_INTERSECTION;
        return result;
    }

    RS_AtomicEntity *trimmed1 = nullptr;
    RS_AtomicEntity *trimmed2 = nullptr;


    result->polyline = isPolyline;

    //if (data.trim || isPolyline) {
    if (isPolyline){
        trimmed1 = entity1;
        trimmed2 = entity2;
        //Always trim if are working with a polyline, to work with trim==false
        //bevel can't be part of the polyline
        data.trim = true;
    } else {
        trimmed1 = (RS_AtomicEntity *) entity1->clone();
        trimmed2 = (RS_AtomicEntity *) entity2->clone();
    }

    // remove trim entity (on screen):
    if (data.trim || isPolyline){
        if (graphicView){
            if (isPolyline){
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
    result->intersectionPoint = is;

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

    result->trimStart1 = start1;

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
    result->trimStart2 = start2;

    // find definitive bevel points
    RS_DEBUG->print("RS_Modification::bevel: find definitive bevel points");
    RS_Vector bp1 = trimmed1->getNearestDist(data.length1, start1);
    RS_Vector bp2 = trimmed2->getNearestDist(data.length2, start2);

    // final trim:
    RS_DEBUG->print("RS_Modification::bevel: final trim");
    if (data.trim){
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
        if (!isPolyline && !previewOnly){
            container->addEntity(trimmed1);
            container->addEntity(trimmed2);
        }
        if (graphicView){
            if (!isPolyline){
                graphicView->drawEntity(trimmed1);
                graphicView->drawEntity(trimmed2);
            }
        }
    }


    // add bevel line:
    RS_DEBUG->print("RS_Modification::bevel: add bevel line");
    RS_Line *bevel;

    if (previewOnly){
        bevel = new RS_Line(nullptr, bp1, bp2);
    }
    else{
        bevel = new RS_Line(baseContainer, bp1, bp2);
    }

    result->bevel = bevel;

    if (!isPolyline){
        if (!previewOnly){
            baseContainer->addEntity(bevel);
        }
    } else {
        int idx1 = baseContainer->findEntity(trimmed1);
        int idx2 = baseContainer->findEntity(trimmed2);
        int idx = idx1;
        //Verify correct order segment in polylines
        if (idx1 > idx2){
            //inverted, reorder it (swap).
            idx1 = idx2;
            idx2 = idx;
            RS_AtomicEntity *trimmedTmp = trimmed1;
            trimmed1 = trimmed2;
            trimmed2 = trimmedTmp;
        }
        idx = idx1;

        if (!previewOnly){
            bevel->setSelected(baseContainer->isSelected());
            bevel->setLayer(baseContainer->getLayer());
            bevel->setPen(baseContainer->getPen());
        }

        // insert bevel at the right position:
        if (trimmed1 == baseContainer->first() && trimmed2 == baseContainer->last() && baseContainer->count() > 2){
            //bevel are from last and first segments, add at the end
            if (trimmed2->getEndpoint().distanceTo(bevel->getStartpoint()) > 1.0e-4){
                bevel->reverse();
            }
            idx = idx2;
        } else {
            //consecutive segments
            if (trimmed1->getEndpoint().distanceTo(bevel->getStartpoint()) > 1.0e-4){
                bevel->reverse();
            }
        }
        baseContainer->insertEntity(idx + 1, bevel);
    }

    result->trimmed1 = trimmed1;
    result->trimmed2 = trimmed2;

    if (isPolyline){
        auto* polyline = dynamic_cast<RS_Polyline *>(baseContainer);
        polyline->updateEndpoints();
        result->polyline = polyline;
    }

    if (graphicView){
        if (isPolyline){
            graphicView->drawEntity(baseContainer);
        } else {
            graphicView->drawEntity(bevel);
        }
    }

    RS_DEBUG->print("RS_Modification::bevel: handling undo");

    if (handleUndo && !previewOnly){
        if (!isPolyline && data.trim){
            undo.addUndoable(trimmed1);
            entity1->setUndoState(true);
            undo.addUndoable(entity1);

            undo.addUndoable(trimmed2);
            entity2->setUndoState(true);
            undo.addUndoable(entity2);
        }

        if (!isPolyline){
            undo.addUndoable(bevel);
        }
    }
//Do not delete trimmed* if are part of a polyline
    if (!(data.trim || isPolyline)){
        RS_DEBUG->print("RS_Modification::bevel: delete trimmed elements");
        delete trimmed1;
        delete trimmed2;
        RS_DEBUG->print("RS_Modification::bevel: delete trimmed elements: ok");
    }

    return result;

}



/**
 * Rounds a corner.
 *
 * @param coord Mouse coordinate to specify the rounding.
 * @param entity1 First entity of the corner.
 * @param entity2 Second entity of the corner.
 * @param data Radius and trim flag.
 */
LC_RoundResult* RS_Modification::round(const RS_Vector& coord,
                            const RS_Vector& coord1,
                            RS_AtomicEntity* entity1,
                            const RS_Vector& coord2,
                            RS_AtomicEntity* entity2,
                            RS_RoundData& data){

    if (!(entity1 && entity2)){
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::round: At least one entity is nullptr");
        return nullptr;
    }
    if (entity1->isLocked() || !entity1->isVisible()) return nullptr;
    if (entity2->isLocked() || !entity2->isVisible()) return nullptr;

    auto* result = new LC_RoundResult();

    RS_EntityContainer *baseContainer = container;
    bool isPolyline = false;
//    bool isClosedPolyline = false;

    LC_UndoSection undo(document, handleUndo);
    // find out whether we're rounding within a polyline:
    if (entity1->getParent() &&
        entity1->getParent()->rtti() == RS2::EntityPolyline){

        if (entity1->getParent() != entity2->getParent()){
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "RS_Modification::round: entities not in "
                            "the same polyline");
            result->error = LC_RoundResult::ERR_NOT_THE_SAME_POLYLINE;
            return result;
        }

        // clone polyline for undo
        auto cl = dynamic_cast<RS_EntityContainer *>(entity1->getParent()->clone());
        baseContainer = cl;

        if (handleUndo){
            container->addEntity(cl);
            undo.addUndoable(cl);
            undo.addUndoable(entity1->getParent());
            entity1->getParent()->setUndoState(true);
        }

        entity1 = (RS_AtomicEntity *) baseContainer->entityAt(entity1->getParent()->findEntity(entity1));
        entity2 = (RS_AtomicEntity *) baseContainer->entityAt(entity2->getParent()->findEntity(entity2));

        isPolyline = true;
        result->polyline = true;
//        isClosedPolyline = ((RS_Polyline*)entity1->getParent())->isClosed();
    }

    // create 2 tmp parallels
    RS_Creation creation(nullptr, nullptr);
    RS_Entity *par1 = creation.createParallel(coord, data.radius, 1, entity1);
    RS_Entity *par2 = creation.createParallel(coord, data.radius, 1, entity2);

    if ((par1 == nullptr) || (par2 == nullptr)) {
        result->error = LC_RoundResult::NO_PARALLELS;
        return result;
    }

    RS_VectorSolutions sol2 =
        RS_Information::getIntersection(entity1, entity2, false);

    RS_VectorSolutions sol =
        RS_Information::getIntersection(par1, par2, false);

    if (sol.getNumber() == 0){
        result->error = LC_RoundResult::ERR_NO_INTERSECTION;
        return result;
    }

    // there might be two intersections: choose the closest:
    RS_Vector is = sol.getClosest(coord);
    RS_Vector p1 = entity1->getNearestPointOnEntity(is, false);
    RS_Vector p2 = entity2->getNearestPointOnEntity(is, false);
    double ang1 = is.angleTo(p1);
    double ang2 = is.angleTo(p2);
    bool reversed = (RS_Math::getAngleDifference(ang1, ang2) > M_PI);
    auto *arc = new RS_Arc(baseContainer,
                             RS_ArcData(is,
                                        data.radius,
                                        ang1, ang2,
                                        reversed));

    result->round = arc;

    RS_AtomicEntity *trimmed1 = nullptr;
    RS_AtomicEntity *trimmed2 = nullptr;

    if (data.trim || isPolyline){
        if (isPolyline){
            trimmed1 = entity1;
            trimmed2 = entity2;
        } else {
            trimmed1 = (RS_AtomicEntity *) entity1->clone();
            trimmed2 = (RS_AtomicEntity *) entity2->clone();
        }

        // remove trim entity:
        if (graphicView){
            if (isPolyline){
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
                result->trim1Mode = LC_RoundResult::TRIM_START;
                break;
            case RS2::EndingEnd:
                trimmed1->trimEndpoint(p1);
                result->trim1Mode = LC_RoundResult::TRIM_END;
                break;
            default:
                trimmed1 = trimCircleForRound(trimmed1, *arc);
                result->trim1Mode = LC_RoundResult::TRIM_CIRCLE;
                break;
        }

        is2 = sol2.getClosest(coord1);
        RS2::Ending ending2 = trimmed2->getTrimPoint(coord2, is2);
        switch (ending2) {
            case RS2::EndingStart:
                trimmed2->trimStartpoint(p2);
                result -> trim2Mode = LC_RoundResult::TRIM_START;
                break;
            case RS2::EndingEnd:
                trimmed2->trimEndpoint(p2);
                result->trim2Mode = LC_RoundResult::TRIM_END;
                break;
            default:
                trimmed2 = trimCircleForRound(trimmed2, *arc);
                result->trim2Mode = LC_RoundResult::TRIM_CIRCLE;
                break;
        }

        // add new trimmed entities:
        if (!isPolyline){
            container->addEntity(trimmed1);
            container->addEntity(trimmed2);
        }
        if (graphicView){
            if (!isPolyline){
                graphicView->drawEntity(trimmed1);
                graphicView->drawEntity(trimmed2);
            }
        }
    }

    // add rounding:
    if (!isPolyline){
        baseContainer->addEntity(arc);
    } else {
        // find out which base entity is before the rounding:
        int idx1 = baseContainer->findEntity(trimmed1);
        int idx2 = baseContainer->findEntity(trimmed2);

        arc->setSelected(baseContainer->isSelected());
        arc->setLayer(baseContainer->getLayer());
        arc->setPen(baseContainer->getPen());

        RS_DEBUG->print("RS_Modification::round: idx1<idx2: %d", (int) (idx1 < idx2));
        RS_DEBUG->print("RS_Modification::round: idx1!=0: %d", (int) (idx1 != 0));
        RS_DEBUG->print("RS_Modification::round: idx2==0: %d", (int) (idx2 == 0));
        RS_DEBUG->print("RS_Modification::round: idx1==(int)baseContainer->count()-1: %d",
                        (int) (idx1 == (int) baseContainer->count() - 1));

        bool insertAfter1 = ((idx1 < idx2 && idx1 != 0) || (idx1 == 0 && idx2 == 1) ||
                             (idx2 == 0 && idx1 == (int) baseContainer->count() - 1));

        // insert rounding at the right position:
        //if ((idx1<idx2 && idx1!=0) ||
        //	(idx2==0 && idx1==(int)baseContainer->count()-1)) {
        //if (idx1<idx2) {
        if (insertAfter1){
            if (trimmed1->getEndpoint().distanceTo(arc->getStartpoint()) > 1.0e-4){
                arc->reverse();
            }
            baseContainer->insertEntity(idx1 + 1, arc);
        } else {
            if (trimmed2->getEndpoint().distanceTo(arc->getStartpoint()) > 1.0e-4){
                arc->reverse();
            }
            baseContainer->insertEntity(idx2 + 1, arc);
        }
    }

    result->trimmed1 = trimmed1;
    result->trimmed2 = trimmed2;

    if (isPolyline){
        ((RS_Polyline *) baseContainer)->updateEndpoints();
    }

    if (graphicView){
        if (isPolyline){
            graphicView->drawEntity(baseContainer);
        } else {
            graphicView->drawEntity(arc);
        }
    }

    if (handleUndo){
        if (!isPolyline && data.trim){
            undo.addUndoable(trimmed1);
            entity1->setUndoState(true);
            undo.addUndoable(entity1);

            undo.addUndoable(trimmed2);
            entity2->setUndoState(true);
            undo.addUndoable(entity2);
        }

        if (!isPolyline){
            undo.addUndoable(arc);
        }
    }

    delete par1;
    delete par2;

    return result;
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

    if (ec == nullptr || e == nullptr || clone == nullptr) {
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
            if (e != nullptr) {
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
 * @param forceUndoableOperation - flag that indicates that explode should be in undo section regardless of handleUndo flag (for paste transform)
 */
bool RS_Modification::explode(const bool remove /*= true*/,  const bool forceUndoableOperation) {
    if (container == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::explode: no valid container for addinge entities");
        return false;
    }

    std::vector<RS_Entity*> selectedEntities;
    container->collectSelected(selectedEntities, false);
    return explode(selectedEntities, remove, forceUndoableOperation);
}

bool RS_Modification::explode(const std::vector<RS_Entity*> &entitiesList, const bool remove, const bool forceUndoableOperation) {
    if (container->isLocked() || ! container->isVisible()) return false;

    std::vector<RS_Entity*> addList;

    for(auto e: entitiesList){
        if (e->isContainer()) {

            // add entities from container:
            auto* ec = (RS_EntityContainer*)e;
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
                case RS2::EntityDimArc:
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

                if (e2 != nullptr) {
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

    deleteOriginalAndAddNewEntities(addList, entitiesList, false, remove, forceUndoableOperation);
    addList.clear();

    container->updateInserts();
    return true;
}

bool RS_Modification::explodeTextIntoLetters() {
    if (container == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::explodeTextIntoLetters: no valid container for adding entities");
        return false;
    }

    std::vector<RS_Entity*> selectedEntities;
    container->collectSelected(selectedEntities, false);
    return explodeTextIntoLetters(selectedEntities);
}

bool RS_Modification::explodeTextIntoLetters(const std::vector<RS_Entity*> &entitiesList) {

    if(container->isLocked() || ! container->isVisible()) return false;

    std::vector<RS_Entity*> addList;

    for(auto e: entitiesList){
        if (e->rtti()==RS2::EntityMText) {
            // add letters of text:
            auto text = dynamic_cast<RS_MText *>(e);
            explodeTextIntoLetters(text, addList);
        } else if (e->rtti()==RS2::EntityText) {
            // add letters of text:
            auto text = dynamic_cast<RS_Text *>(e);
            explodeTextIntoLetters(text, addList);
        } else {
            e->setSelected(false);
        }
    }

    deleteOriginalAndAddNewEntities(addList, entitiesList,false, true);
    addList.clear();
    return true;
}

bool RS_Modification::explodeTextIntoLetters(RS_MText* text, std::vector<RS_Entity*>& addList) {

    if (text == nullptr) {
        return false;
    }

    if(text->isLocked() || ! text->isVisible()) return false;

    // iterate though lines:
    for(auto e2: *text){

        if (e2 == nullptr) {
            break;
        }


        // text lines:
        if (e2->rtti()==RS2::EntityContainer) {

            auto line = dynamic_cast<RS_EntityContainer *>(e2);

            // iterate though letters:
            for(auto e3: *line){

                if (e3 == nullptr) {
                    break;
                }

                if (e3->rtti()==RS2::EntityMText) { // super / sub texts:
                    auto e3MText = dynamic_cast<RS_MText *>(e3);
                    explodeTextIntoLetters(e3MText, addList);
                }
                else if (e3->rtti()==RS2::EntityInsert) {    // normal letters:
                    auto letter = dynamic_cast<RS_Insert *>(e3);

                    auto tl = new RS_MText(
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

    if (text == nullptr) {
        return false;
    }

    if(text->isLocked() || ! text->isVisible()) return false;

    // iterate though letters:
    for(auto e2: *text){

        if (e2 == nullptr) {
            break;
        }

        if (e2->rtti()==RS2::EntityInsert) {

            auto letter = dynamic_cast<RS_Insert *>(e2);

            auto tl = new RS_Text(
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
    if (container == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::moveRef: no valid container");
        return false;
    }
    if(container->isLocked() || ! container->isVisible()) return false;

    std::vector<RS_Entity*> addList;

    // Create new entities
    for(auto e: *container){ // fixme - iterating all entities for selection
        if (e != nullptr && e->isSelected()) {
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
