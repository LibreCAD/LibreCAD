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

#include <cmath>
#include <iostream>
#include <set>

#include <QtGlobal>
#include "lc_looputils.h"

#include "qg_dialogfactory.h"

#include "rs_arc.h"
#include "rs_constructionline.h"
#include "rs_debug.h"
#include "rs_dimension.h"
#include "rs_dialogfactory.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_solid.h"

namespace {

// the tolerance used to check topology of contours in hatching
constexpr double contourTolerance = 1e-8;

// For validate hatch contours, whether an entity in the contour is a closed
// loop itself
bool isClosedLoop(RS_Entity& entity)
{
    switch (entity.rtti()){
    case RS2::EntityCircle:
    // Sub containers are always closed
    case RS2::EntityContainer:
    return true;
    case RS2::EntityEllipse:
    return !static_cast<RS_Ellipse*>(&entity)->isArc();
    default:
    return false;
    }
}

// Find the nearest distance between the endpoints of an entity to a given point
double endPointDistance(const RS_Vector& point, const RS_Entity& entity)
{
    double distance=RS_MAXDOUBLE;
    entity.getNearestEndpoint(point, &distance);
    return distance;
}
}

/**
 * Default constructor.
 *
 * @param owner True if we own and also delete the entities.
 */
RS_EntityContainer::RS_EntityContainer(RS_EntityContainer* parent,
                                       bool owner)
    : RS_Entity(parent) {

    autoDelete=owner;
    //    RS_DEBUG->print("RS_EntityContainer::RS_EntityContainer: "
    //                    "owner: %d", (int)owner);
    subContainer = nullptr;
    //autoUpdateBorders = true;
    entIdx = -1;
}


/**
 * Copy constructor. Makes a deep copy of all entities.
 */
/*
RS_EntityContainer::RS_EntityContainer(const RS_EntityContainer& ec)
 : RS_Entity(ec) {

}
*/



/**
 * Destructor.
 */
RS_EntityContainer::~RS_EntityContainer() {
    if (autoDelete) {
        while (!entities.isEmpty())
            delete entities.takeFirst();
    } else
        entities.clear();
}



RS_Entity* RS_EntityContainer::clone() const{
    RS_DEBUG->print("RS_EntityContainer::clone: ori autoDel: %d",
                    autoDelete);

    RS_EntityContainer* ec = new RS_EntityContainer(*this);
    ec->setOwner(autoDelete);

    RS_DEBUG->print("RS_EntityContainer::clone: clone autoDel: %d",
                    ec->isOwner());

    ec->detach();
    ec->initId();

    return ec;
}



/**
 * Detaches shallow copies and creates deep copies of all subentities.
 * This is called after cloning entity containers.
 */
void RS_EntityContainer::detach() {
    QList<RS_Entity*> tmp;
    bool autoDel = isOwner();
    RS_DEBUG->print("RS_EntityContainer::detach: autoDel: %d",
                    (int)autoDel);
    setOwner(false);

    // make deep copies of all entities:
    for(auto e: entities){
        if (!e->getFlag(RS2::FlagTemp)) {
            tmp.append(e->clone());
        }
    }

    // clear shared pointers:
    entities.clear();
    setOwner(autoDel);

    // point to new deep copies:
    for(auto e: tmp){
        entities.append(e);
        e->reparent(this);
    }
}



void RS_EntityContainer::reparent(RS_EntityContainer* parent) {
    RS_Entity::reparent(parent);

    // All sub-entities:

    for(auto e: entities){
        e->reparent(parent);
    }
}



void RS_EntityContainer::setVisible(bool v) {
    //    RS_DEBUG->print("RS_EntityContainer::setVisible: %d", v);
    RS_Entity::setVisible(v);

    // All sub-entities:

    for(auto e: entities){
        //        RS_DEBUG->print("RS_EntityContainer::setVisible: subentity: %d", v);
        e->setVisible(v);
    }
}



/**
 * @return Total length of all entities in this container.
 */
double RS_EntityContainer::getLength() const {
    double ret = 0.0;

    for(auto e: entities){
        if (e->isVisible()) {
            double l = e->getLength();
            if (l<0.0) {
                ret = -1.0;
                break;
            } else {
                ret += l;
            }
        }
    }

    return ret;
}


/**
 * Selects this entity.
 */
bool RS_EntityContainer::setSelected(bool select) {
    // This entity's select:
    if (RS_Entity::setSelected(select)) {

        // All sub-entity's select:
        for(auto e: entities){
            if (e->isVisible()) {
                e->setSelected(select);
            }
        }
        return true;
    } else {
        return false;
    }
}



/**
 * Toggles select on this entity.
 */
bool RS_EntityContainer::toggleSelected() {
    // Toggle this entity's select:
    if (RS_Entity::toggleSelected()) {

        // Toggle all sub-entity's select:
        /*for (RS_Entity* e=firstEntity(RS2::ResolveNone);
                e;
                e=nextEntity(RS2::ResolveNone)) {
            e->toggleSelected();
    }*/
        return true;
    } else {
        return false;
    }
}


void RS_EntityContainer::setHighlighted(bool on)
{
    for (auto e : entities)
    {
        e->setHighlighted(on);
    }
    RS_Entity::setHighlighted(on);
}


/**
 * Selects all entities within the given area.
 *
 * @param select True to select, False to deselect the entities.
 */
void RS_EntityContainer::selectWindow(enum RS2::EntityType typeToSelect,RS_Vector v1, RS_Vector v2,
                                      bool select, bool cross) {

    bool included;

    for(auto e: entities){

        included = false;
        if (e->isVisible()) {
            if (e->isInWindow(v1, v2)) {
                //e->setSelected(select);
                included = true;
            } else if (cross) {
                RS_EntityContainer l;
                l.addRectangle(v1, v2);
                RS_VectorSolutions sol;

                if (e->isContainer()) {
                    RS_EntityContainer* ec = (RS_EntityContainer*)e;
                    for (RS_Entity* se=ec->firstEntity(RS2::ResolveAll);
                         se && included==false;
                         se=ec->nextEntity(RS2::ResolveAll)) {

                        if (se->rtti() == RS2::EntitySolid){
                            included = static_cast<RS_Solid*>(se)->isInCrossWindow(v1,v2);
                        } else {
                            for (auto line: l) {
                                sol = RS_Information::getIntersection(
                                            se, line, true);
                                if (sol.hasValid()) {
                                    included = true;
                                    break;
                                }
                            }
                        }
                    }
                } else if (e->rtti() == RS2::EntitySolid){
                    included = static_cast<RS_Solid*>(e)->isInCrossWindow(v1,v2);
                } else {
                    for (auto line: l) {
                        sol = RS_Information::getIntersection(e, line, true);
                        if (sol.hasValid()) {
                            included = true;
                            break;
                        }
                    }
                }
            }
        }

        if (included) {
            if(typeToSelect!=RS2::EntityType::EntityUnknown){
                if(typeToSelect==e->rtti()){
                    e->setSelected(select);
                } else {
                    //Do not select
                }
            } else {
                e->setSelected(select);
            }

        }
    }
}



/**
 * Adds a entity to this container and updates the borders of this
 * entity-container if autoUpdateBorders is true.
 */
void RS_EntityContainer::addEntity(RS_Entity* entity) {
    /*
       if (isDocument()) {
           RS_LayerList* lst = getDocument()->getLayerList();
           if (lst) {
               RS_Layer* l = lst->getActive();
               if (l && l->isLocked()) {
                   return;
               }
           }
       }
    */

    if (!entity) return;

    if (entity->rtti()==RS2::EntityImage ||
            entity->rtti()==RS2::EntityHatch) {
        entities.prepend(entity);
    } else {
        entities.append(entity);
    }
    if (autoUpdateBorders) {
        adjustBorders(entity);
    }
}


/**
 * Insert a entity at the end of entities list and updates the
 * borders of this entity-container if autoUpdateBorders is true.
 */
void RS_EntityContainer::appendEntity(RS_Entity* entity){
    if (!entity)
        return;
    entities.append(entity);
    if (autoUpdateBorders)
        adjustBorders(entity);
}

/**
 * Insert a entity at the start of entities list and updates the
 * borders of this entity-container if autoUpdateBorders is true.
 */
void RS_EntityContainer::prependEntity(RS_Entity* entity){
    if (!entity) return;
    entities.prepend(entity);
    if (autoUpdateBorders)
        adjustBorders(entity);
}

/**
 * Move a entity list in this container at the given position,
 * the borders of this entity-container if autoUpdateBorders is true.
 */
void RS_EntityContainer::moveEntity(int index, QList<RS_Entity *>& entList){
    if (entList.isEmpty()) return;
    int ci = 0; //current index for insert without invert order
    bool ret, into = false;
    RS_Entity *mid = nullptr;
    if (index < 1) {
        ci = 0;
    } else if (index >= entities.size() ) {
        ci = entities.size() - entList.size();
    } else {
        into = true;
        mid = entities.at(index);
    }

    for (int i = 0; i < entList.size(); ++i) {
        RS_Entity *e = entList.at(i);
        ret = entities.removeOne(e);
        //if e not exist in entities list remove from entList
        if (!ret) {
            entList.removeAt(i);
        }
    }
    if (into) {
        ci = entities.indexOf(mid);
    }

    for(auto e: entList){
        entities.insert(ci++, e);
    }
}

/**
 * Inserts a entity to this container at the given position and updates
 * the borders of this entity-container if autoUpdateBorders is true.
 */
void RS_EntityContainer::insertEntity(int index, RS_Entity* entity) {
    if (!entity) return;

    entities.insert(index, entity);

    if (autoUpdateBorders) {
        adjustBorders(entity);
    }
}



/**
 * Replaces the entity at the given index with the given entity
 * and updates the borders of this entity-container if autoUpdateBorders is true.
 */
/*RLZ unused function
void RS_EntityContainer::replaceEntity(int index, RS_Entity* entity) {
//RLZ TODO: is needed to delete the old entity? not documented in Q3PtrList
//    investigate in qt3support code if reactivate this function.
    if (!entity) {
        return;
    }

    entities.replace(index, entity);

    if (autoUpdateBorders) {
        adjustBorders(entity);
    }
}RLZ*/



/**
 * Removes an entity from this container and updates the borders of
 * this entity-container if autoUpdateBorders is true.
 */
bool RS_EntityContainer::removeEntity(RS_Entity* entity) {
    //RLZ TODO: in Q3PtrList if 'entity' is nullptr remove the current item-> at.(entIdx)
    //    and sets 'entIdx' in next() or last() if 'entity' is the last item in the list.
    //    in LibreCAD is never called with nullptr
    bool ret = entities.removeOne(entity);

    if (autoDelete && ret) {
        delete entity;
    }
    if (autoUpdateBorders) {
        calculateBorders();
    }
    return ret;
}



/**
 * Erases all entities in this container and resets the borders..
 */
void RS_EntityContainer::clear() {
    if (autoDelete) {
        while (!entities.isEmpty())
            delete entities.takeFirst();
    } else
        entities.clear();
    resetBorders();
}

unsigned int RS_EntityContainer::count() const{
    return entities.size();
}


/**
 * Counts all entities (leaves of the tree).
 */
unsigned int RS_EntityContainer::countDeep() const{
    unsigned int c=0;
    for(auto t: *this){
        c += t->countDeep();
    }
    return c;
}



/**
 * Counts the selected entities in this container.
 */
unsigned RS_EntityContainer::countSelected(bool deep, QList<RS2::EntityType> const& types) {
    unsigned c=0;
    std::set<RS2::EntityType> type{types.cbegin(), types.cend()};

    for (RS_Entity* t: entities){

        if (t->isSelected())
	    if (!types.size() || type.count(t->rtti()))
                c++;

        if (t->isContainer())
            c += static_cast<RS_EntityContainer*>(t)->countSelected(deep);
    }

    return c;
}

/**
 * Counts the selected entities in this container.
 */
double RS_EntityContainer::totalSelectedLength() {
    double ret(0.0);
    for (RS_Entity* e: entities){

        if (e->isVisible() && e->isSelected()) {
            double l = e->getLength();
            if (l>=0.) {
                ret += l;
            }
        }
    }
    return ret;
}


/**
 * Adjusts the borders of this graphic (max/min values)
 */
void RS_EntityContainer::adjustBorders(RS_Entity* entity) {
    //RS_DEBUG->print("RS_EntityContainer::adjustBorders");
    //resetBorders();

    if (entity) {
        // make sure a container is not empty (otherwise the border
        //   would get extended to 0/0):
        if (!entity->isContainer() || entity->count()>0) {
            minV = RS_Vector::minimum(entity->getMin(),minV);
            maxV = RS_Vector::maximum(entity->getMax(),maxV);
        }

        // Notify parents. The border for the parent might
        // also change TODO: Check for efficiency
        //if(parent) {
        //parent->adjustBorders(this);
        //}
    }
}


/**
 * Recalculates the borders of this entity container.
 */
void RS_EntityContainer::calculateBorders() {
    RS_DEBUG->print("RS_EntityContainer::calculateBorders");

    resetBorders();
    for (RS_Entity* e: entities){

        RS_Layer* layer = e->getLayer();

        //        RS_DEBUG->print("RS_EntityContainer::calculateBorders: "
        //                        "isVisible: %d", (int)e->isVisible());

        if (e->isVisible() && !(layer && layer->isFrozen())) {
            e->calculateBorders();
            adjustBorders(e);
        }
    }

    RS_DEBUG->print("RS_EntityContainer::calculateBorders: size 1: %f,%f",
                    getSize().x, getSize().y);

    // needed for correcting corrupt data (PLANS.dxf)
    if (minV.x>maxV.x || minV.x>RS_MAXDOUBLE || maxV.x>RS_MAXDOUBLE
            || minV.x<RS_MINDOUBLE || maxV.x<RS_MINDOUBLE) {

        minV.x = 0.0;
        maxV.x = 0.0;
    }
    if (minV.y>maxV.y || minV.y>RS_MAXDOUBLE || maxV.y>RS_MAXDOUBLE
            || minV.y<RS_MINDOUBLE || maxV.y<RS_MINDOUBLE) {

        minV.y = 0.0;
        maxV.y = 0.0;
    }

    RS_DEBUG->print("RS_EntityContainer::calculateBorders: size: %f,%f",
                    getSize().x, getSize().y);

    //RS_DEBUG->print("  borders: %f/%f %f/%f", minV.x, minV.y, maxV.x, maxV.y);

    //printf("borders: %lf/%lf  %lf/%lf\n", minV.x, minV.y, maxV.x, maxV.y);
    //RS_Entity::calculateBorders();
}

//namespace {
//bool isBoundingBoxValid(RS_Entity* e) {
//	if (!(e->getMin() && e->getMax())) return false;
//	if (!(e->getMin().x <= e->getMax().x)) return false;
//	if (!(e->getMin().y <= e->getMax().y)) return false;
//	if ((e->getMin() - e->getMax()).magnitude() > RS_MAXDOUBLE) return false;
//	return true;
//}
//}

/**
 * Recalculates the borders of this entity container including
 * invisible entities.
 */
void RS_EntityContainer::forcedCalculateBorders() {
    //RS_DEBUG->print("RS_EntityContainer::calculateBorders");

    resetBorders();
    for (RS_Entity* e: entities){

        //RS_Layer* layer = e->getLayer();

        if (e->isContainer()) {
            ((RS_EntityContainer*)e)->forcedCalculateBorders();
        } else {
            e->calculateBorders();
        }
        adjustBorders(e);
    }

    // needed for correcting corrupt data (PLANS.dxf)
    if (minV.x>maxV.x || minV.x>RS_MAXDOUBLE || maxV.x>RS_MAXDOUBLE
            || minV.x<RS_MINDOUBLE || maxV.x<RS_MINDOUBLE) {

        minV.x = 0.0;
        maxV.x = 0.0;
    }
    if (minV.y>maxV.y || minV.y>RS_MAXDOUBLE || maxV.y>RS_MAXDOUBLE
            || minV.y<RS_MINDOUBLE || maxV.y<RS_MINDOUBLE) {

        minV.y = 0.0;
        maxV.y = 0.0;
    }

    //RS_DEBUG->print("  borders: %f/%f %f/%f", minV.x, minV.y, maxV.x, maxV.y);

    //printf("borders: %lf/%lf  %lf/%lf\n", minV.x, minV.y, maxV.x, maxV.y);
    //RS_Entity::calculateBorders();
}

/**
 * Updates all Dimension entities in this container and / or
 * reposition their labels.
 *
 * @param autoText Automatically reposition the text label bool autoText=true
 */
void RS_EntityContainer::updateDimensions(bool autoText) {

    RS_DEBUG->print("RS_EntityContainer::updateDimensions()");

    //for (RS_Entity* e=firstEntity(RS2::ResolveNone);
    //        e;
    //        e=nextEntity(RS2::ResolveNone)) {

    for (RS_Entity* e: entities){
        if (RS_Information::isDimension(e->rtti())) {
            // update and reposition label:
            ((RS_Dimension*)e)->updateDim(autoText);
        } else if(e->rtti()==RS2::EntityDimLeader)
            e->update();
        else if (e->isContainer()) {
            ((RS_EntityContainer*)e)->updateDimensions(autoText);
        }
    }

    RS_DEBUG->print("RS_EntityContainer::updateDimensions() OK");
}



/**
 * Updates all Insert entities in this container.
 */
void RS_EntityContainer::updateInserts() {

    std::string idTypeId = std::to_string(getId()) + "/" + std::to_string(rtti());
    RS_DEBUG->print("RS_EntityContainer::updateInserts() ID/type: %s", idTypeId.c_str());

    for (RS_Entity* e: entities){
        //// Only update our own inserts and not inserts of inserts
        if (e->rtti()==RS2::EntityInsert  /*&& e->getParent()==this*/) {
            ((RS_Insert*)e)->update();
            RS_DEBUG->print("RS_EntityContainer::updateInserts: updated ID/type: %s", idTypeId.c_str());
        } else if (e->isContainer()) {
            if (e->rtti()==RS2::EntityHatch) {
                RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_EntityContainer::updateInserts: skip hatch ID/type: %s", idTypeId.c_str());
            } else {
                RS_DEBUG->print("RS_EntityContainer::updateInserts: update container ID/type: %s", idTypeId.c_str());
                ((RS_EntityContainer*)e)->updateInserts();
            }
        } else {
            RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_EntityContainer::updateInserts: skip entity ID/type: %s", idTypeId.c_str());
        }
    }
    RS_DEBUG->print("RS_EntityContainer::updateInserts() ID/type: %s", idTypeId.c_str());
}



/**
 * Renames all inserts with name 'oldName' to 'newName'. This is
 *   called after a block was rename to update the inserts.
 */
void RS_EntityContainer::renameInserts(const QString& oldName,
                                       const QString& newName) {
    RS_DEBUG->print("RS_EntityContainer::renameInserts()");

    //for (RS_Entity* e=firstEntity(RS2::ResolveNone);
    //        e;
    //        e=nextEntity(RS2::ResolveNone)) {

    for (RS_Entity* e: entities){
        if (e->rtti()==RS2::EntityInsert) {
            RS_Insert* i = ((RS_Insert*)e);
            if (i->getName()==oldName) {
                i->setName(newName);
            }
        }
        if (e->isContainer()) {
            ((RS_EntityContainer*)e)->renameInserts(oldName, newName);
        }
    }

    RS_DEBUG->print("RS_EntityContainer::renameInserts() OK");

}

/**
 * Updates all Spline entities in this container.
 */
void RS_EntityContainer::updateSplines() {

    RS_DEBUG->print("RS_EntityContainer::updateSplines()");

    for (RS_Entity* e: entities){
        //// Only update our own inserts and not inserts of inserts
        if (e->rtti()==RS2::EntitySpline  /*&& e->getParent()==this*/) {
            e->update();
        } else if (e->isContainer() && e->rtti()!=RS2::EntityHatch) {
            static_cast<RS_EntityContainer*>(e)->updateSplines();
        }
    }

    RS_DEBUG->print("RS_EntityContainer::updateSplines() OK");
}


/**
 * Updates the sub entities of this container.
 */
void RS_EntityContainer::update() {
    for (RS_Entity* e: entities){
        e->update();
    }
}

void RS_EntityContainer::addRectangle(RS_Vector const& v0, RS_Vector const& v1)
{
    addEntity(new RS_Line{this, v0, {v1.x, v0.y}});
    addEntity(new RS_Line{this, {v1.x, v0.y}, v1});
    addEntity(new RS_Line{this, v1, {v0.x, v1.y}});
    addEntity(new RS_Line{this, {v0.x, v1.y}, v0});
}

/**
 * Returns the first entity or nullptr if this graphic is empty.
 * @param level
 */
RS_Entity* RS_EntityContainer::firstEntity(RS2::ResolveLevel level) const {
    RS_Entity* e = nullptr;
    entIdx = -1;
    switch (level) {
    case RS2::ResolveNone:
        if (!entities.isEmpty()) {
            entIdx = 0;
            return entities.first();
        }
        break;

    case RS2::ResolveAllButInserts: {
        subContainer=nullptr;
        if (!entities.isEmpty()) {
            entIdx = 0;
            e = entities.first();
        }
        if (e && e->isContainer() && e->rtti()!=RS2::EntityInsert) {
            subContainer = (RS_EntityContainer*)e;
            e = ((RS_EntityContainer*)e)->firstEntity(level);
            // empty container:
            if (!e) {
                subContainer = nullptr;
                e = nextEntity(level);
            }
        }
        return e;
    }
        break;

    case RS2::ResolveAllButTextImage:
    case RS2::ResolveAllButTexts: {
        subContainer=nullptr;
        if (!entities.isEmpty()) {
            entIdx = 0;
            e = entities.first();
        }
        if (e && e->isContainer() && e->rtti()!=RS2::EntityText && e->rtti()!=RS2::EntityMText) {
            subContainer = (RS_EntityContainer*)e;
            e = ((RS_EntityContainer*)e)->firstEntity(level);
            // empty container:
            if (!e) {
                subContainer = nullptr;
                e = nextEntity(level);
            }
        }
        return e;
    }
        break;

    case RS2::ResolveAll: {
        subContainer=nullptr;
        if (!entities.isEmpty()) {
            entIdx = 0;
            e = entities.first();
        }
        if (e && e->isContainer()) {
            subContainer = (RS_EntityContainer*)e;
            e = ((RS_EntityContainer*)e)->firstEntity(level);
            // empty container:
            if (!e) {
                subContainer = nullptr;
                e = nextEntity(level);
            }
        }
        return e;
    }
        break;
    }

    return nullptr;
}



/**
 * Returns the last entity or \p nullptr if this graphic is empty.
 *
 * @param level \li \p 0 Groups are not resolved
 *              \li \p 1 (default) only Groups are resolved
 *              \li \p 2 all Entity Containers are resolved
 */
RS_Entity* RS_EntityContainer::lastEntity(RS2::ResolveLevel level) const {
    RS_Entity* e = nullptr;
    if(!entities.size()) return nullptr;
    entIdx = entities.size()-1;
    switch (level) {
    case RS2::ResolveNone:
        if (!entities.isEmpty())
            return entities.last();
        break;

    case RS2::ResolveAllButInserts: {
        if (!entities.isEmpty())
            e = entities.last();
        subContainer = nullptr;
        if (e && e->isContainer() && e->rtti()!=RS2::EntityInsert) {
            subContainer = (RS_EntityContainer*)e;
            e = ((RS_EntityContainer*)e)->lastEntity(level);
        }
        return e;
    }
        break;
    case RS2::ResolveAllButTextImage:
    case RS2::ResolveAllButTexts: {
        if (!entities.isEmpty())
            e = entities.last();
        subContainer = nullptr;
        if (e && e->isContainer() && e->rtti()!=RS2::EntityText && e->rtti()!=RS2::EntityMText) {
            subContainer = (RS_EntityContainer*)e;
            e = ((RS_EntityContainer*)e)->lastEntity(level);
        }
        return e;
    }
        break;

    case RS2::ResolveAll: {
        if (!entities.isEmpty())
            e = entities.last();
        subContainer = nullptr;
        if (e && e->isContainer()) {
            subContainer = (RS_EntityContainer*)e;
            e = ((RS_EntityContainer*)e)->lastEntity(level);
        }
        return e;
    }
        break;
    }

    return nullptr;
}


/**
 * Returns the next entity or container or \p nullptr if the last entity
 * returned by \p next() was the last entity in the container.
 */
RS_Entity* RS_EntityContainer::nextEntity(RS2::ResolveLevel level) const {

    //set entIdx pointing in next entity and check if is out of range
    ++entIdx;
    switch (level) {
    case RS2::ResolveNone:
        if ( entIdx < entities.size() )
            return entities.at(entIdx);
        break;

    case RS2::ResolveAllButInserts: {
        RS_Entity* e=nullptr;
        if (subContainer) {
            e = subContainer->nextEntity(level);
            if (e) {
                --entIdx; //return a sub-entity, index not advanced
                return e;
            } else {
                if ( entIdx < entities.size() )
                    e = entities.at(entIdx);
            }
        } else {
            if ( entIdx < entities.size() )
                e = entities.at(entIdx);
        }
        if (e && e->isContainer() && e->rtti()!=RS2::EntityInsert) {
            subContainer = (RS_EntityContainer*)e;
            e = ((RS_EntityContainer*)e)->firstEntity(level);
            // empty container:
            if (!e) {
                subContainer = nullptr;
                e = nextEntity(level);
            }
        }
        return e;
    }
        break;

    case RS2::ResolveAllButTextImage:
    case RS2::ResolveAllButTexts: {
        RS_Entity* e=nullptr;
        if (subContainer) {
            e = subContainer->nextEntity(level);
            if (e) {
                --entIdx; //return a sub-entity, index not advanced
                return e;
            } else {
                if ( entIdx < entities.size() )
                    e = entities.at(entIdx);
            }
        } else {
            if ( entIdx < entities.size() )
                e = entities.at(entIdx);
        }
        if (e && e->isContainer() && e->rtti()!=RS2::EntityText && e->rtti()!=RS2::EntityMText ) {
            subContainer = (RS_EntityContainer*)e;
            e = ((RS_EntityContainer*)e)->firstEntity(level);
            // empty container:
            if (!e) {
                subContainer = nullptr;
                e = nextEntity(level);
            }
        }
        return e;
    }
        break;

    case RS2::ResolveAll: {
        RS_Entity* e=nullptr;
        if (subContainer) {
            e = subContainer->nextEntity(level);
            if (e) {
                --entIdx; //return a sub-entity, index not advanced
                return e;
            } else {
                if ( entIdx < entities.size() )
                    e = entities.at(entIdx);
            }
        } else {
            if ( entIdx < entities.size() )
                e = entities.at(entIdx);
        }
        if (e && e->isContainer()) {
            subContainer = (RS_EntityContainer*)e;
            e = ((RS_EntityContainer*)e)->firstEntity(level);
            // empty container:
            if (!e) {
                subContainer = nullptr;
                e = nextEntity(level);
            }
        }
        return e;
    }
        break;
    }
    return nullptr;
}



/**
 * Returns the prev entity or container or \p nullptr if the last entity
 * returned by \p prev() was the first entity in the container.
 */
RS_Entity* RS_EntityContainer::prevEntity(RS2::ResolveLevel level) const {
    //set entIdx pointing in prev entity and check if is out of range
    --entIdx;
    switch (level) {

    case RS2::ResolveNone:
        if (entIdx >= 0)
            return entities.at(entIdx);
        break;

    case RS2::ResolveAllButInserts: {
        RS_Entity* e=nullptr;
        if (subContainer) {
            e = subContainer->prevEntity(level);
            if (e) {
                return e;
            } else {
                if (entIdx >= 0)
                    e = entities.at(entIdx);
            }
        } else {
            if (entIdx >= 0)
                e = entities.at(entIdx);
        }
        if (e && e->isContainer() && e->rtti()!=RS2::EntityInsert) {
            subContainer = (RS_EntityContainer*)e;
            e = ((RS_EntityContainer*)e)->lastEntity(level);
            // empty container:
            if (!e) {
                subContainer = nullptr;
                e = prevEntity(level);
            }
        }
        return e;
    }

    case RS2::ResolveAllButTextImage:
    case RS2::ResolveAllButTexts: {
        RS_Entity* e=nullptr;
        if (subContainer) {
            e = subContainer->prevEntity(level);
            if (e) {
                return e;
            } else {
                if (entIdx >= 0)
                    e = entities.at(entIdx);
            }
        } else {
            if (entIdx >= 0)
                e = entities.at(entIdx);
        }
        if (e && e->isContainer() && e->rtti()!=RS2::EntityText && e->rtti()!=RS2::EntityMText) {
            subContainer = (RS_EntityContainer*)e;
            e = ((RS_EntityContainer*)e)->lastEntity(level);
            // empty container:
            if (!e) {
                subContainer = nullptr;
                e = prevEntity(level);
            }
        }
        return e;
    }

    case RS2::ResolveAll: {
        RS_Entity* e=nullptr;
        if (subContainer) {
            e = subContainer->prevEntity(level);
            if (e) {
                ++entIdx; //return a sub-entity, index not advanced
                return e;
            } else {
                if (entIdx >= 0)
                    e = entities.at(entIdx);
            }
        } else {
            if (entIdx >= 0)
                e = entities.at(entIdx);
        }
        if (e && e->isContainer()) {
            subContainer = (RS_EntityContainer*)e;
            e = ((RS_EntityContainer*)e)->lastEntity(level);
            // empty container:
            if (!e) {
                subContainer = nullptr;
                e = prevEntity(level);
            }
        }
        return e;
    }
    }
    return nullptr;
}



/**
 * @return Entity at the given index or nullptr if the index is out of range.
 */
RS_Entity* RS_EntityContainer::entityAt(int index) {
    if (entities.size() > index && index >= 0)
        return entities.at(index);
    else
        return nullptr;
}

void RS_EntityContainer::setEntityAt(int index,RS_Entity* en){
    if(autoDelete && entities.at(index)) {
        delete entities.at(index);
    }
    entities[index] = en;
}

/**
 * @return Current index.
 */
/*RLZ unused
int RS_EntityContainer::entityAt() {
    return entIdx;
} RLZ unused*/


/**
 * Finds the given entity and makes it the current entity if found.
 */
int RS_EntityContainer::findEntity(RS_Entity const* const entity) {
    entIdx = entities.indexOf(const_cast<RS_Entity*>(entity));
    return entIdx;
}

/**
 * @return The point which is closest to 'coord'
 * (one of the vertices)
 */
RS_Vector RS_EntityContainer::getNearestEndpoint(const RS_Vector& coord,
                                                 double* dist  )const {

    double minDist = RS_MAXDOUBLE;  // minimum measured distance
    double curDist;                 // currently measured distance
    RS_Vector closestPoint(false);  // closest found endpoint
    RS_Vector point;                // endpoint found

    for (RS_Entity* en: entities){

        if (en->isVisible()
                && !en->getParent()->ignoredOnModification()
                ){//no end point for Insert, text, Dim
            point = en->getNearestEndpoint(coord, &curDist);
            if (point.valid && curDist<minDist) {
                closestPoint = point;
                minDist = curDist;
                if (dist) {
                    *dist = minDist;
                }
            }
        }
    }

    return closestPoint;
}


/**
 * @return The point which is closest to 'coord'
 * (one of the vertices)
 */
RS_Vector RS_EntityContainer::getNearestEndpoint(const RS_Vector& coord,
                                                 double* dist,  RS_Entity** pEntity)const {

    double minDist = RS_MAXDOUBLE;  // minimum measured distance
    double curDist;                 // currently measured distance
    RS_Vector closestPoint(false);  // closest found endpoint
    RS_Vector point;                // endpoint found

    //QListIterator<RS_Entity> it = createIterator();
    //RS_Entity* en;
    //while ( (en = it.current())  ) {
    //    ++it;

    for(auto en: entities){
        if (!en->getParent()->ignoredOnModification() ){//no end point for Insert, text, Dim
            //            std::cout<<"find nearest for entity "<<i0<<std::endl;
            point = en->getNearestEndpoint(coord, &curDist);
            if (point.valid && curDist<minDist) {
                closestPoint = point;
                minDist = curDist;
                if (dist) {
                    *dist = minDist;
                }
                if(pEntity){
                    *pEntity=en;
                }
            }
        }
    }

    //    std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
    //    std::cout<<"count()="<<const_cast<RS_EntityContainer*>(this)->count()<<"\tminDist= "<<minDist<<"\tclosestPoint="<<closestPoint;
    //    if(pEntity ) std::cout<<"\t*pEntity="<<*pEntity;
    //    std::cout<<std::endl;
    return closestPoint;
}



RS_Vector RS_EntityContainer::getNearestPointOnEntity(const RS_Vector& coord,
                                                      bool onEntity, double* dist, RS_Entity** entity)const {

    RS_Vector point(false);

    RS_Entity* en = getNearestEntity(coord, dist, RS2::ResolveNone);

    if (en && en->isVisible()
            && !en->getParent()->ignoredSnap()
            ){
        point = en->getNearestPointOnEntity(coord, onEntity, dist, entity);
    }

    return point;
}



RS_Vector RS_EntityContainer::getNearestCenter(const RS_Vector& coord,
                                               double* dist) const{
    double minDist = RS_MAXDOUBLE;  // minimum measured distance
    double curDist = RS_MAXDOUBLE;  // currently measured distance
    RS_Vector closestPoint(false);  // closest found endpoint
    RS_Vector point;                // endpoint found

    for(auto en: entities){

        if (en->isVisible()
                && !en->getParent()->ignoredSnap()
                ){//no center point for spline, text, Dim
            point = en->getNearestCenter(coord, &curDist);
            if (point.valid && curDist<minDist) {
                closestPoint = point;
                minDist = curDist;
            }
        }
    }
    if (dist) {
        *dist = minDist;
    }

    return closestPoint;
}

/** @return the nearest of equidistant middle points of the line. */

RS_Vector RS_EntityContainer::getNearestMiddle(const RS_Vector& coord,
                                               double* dist,
                                               int middlePoints
                                               ) const{
    double minDist = RS_MAXDOUBLE;  // minimum measured distance
    double curDist = RS_MAXDOUBLE;  // currently measured distance
    RS_Vector closestPoint(false);  // closest found endpoint
    RS_Vector point;                // endpoint found

    for(auto en: entities){

        if (en->isVisible()
                && !en->getParent()->ignoredSnap()
                ){//no midle point for spline, text, Dim
            point = en->getNearestMiddle(coord, &curDist, middlePoints);
            if (point.valid && curDist<minDist) {
                closestPoint = point;
                minDist = curDist;
            }
        }
    }
    if (dist) {
        *dist = minDist;
    }

    return closestPoint;
}



RS_Vector RS_EntityContainer::getNearestDist(double distance,
                                             const RS_Vector& coord,
                                             double* dist) const{

    RS_Vector point(false);
    RS_Entity* closestEntity;

    closestEntity = getNearestEntity(coord, nullptr, RS2::ResolveNone);

    if (closestEntity) {
        point = closestEntity->getNearestDist(distance, coord, dist);
    }

    return point;
}



/**
 * @return The intersection which is closest to 'coord'
 */
RS_Vector RS_EntityContainer::getNearestIntersection(const RS_Vector& coord,
                                                     double* dist) {

    double minDist = RS_MAXDOUBLE;  // minimum measured distance
    double curDist = RS_MAXDOUBLE;  // currently measured distance
    RS_Vector closestPoint(false);  // closest found endpoint
    RS_Vector point;                // endpoint found
    RS_VectorSolutions sol;
    RS_Entity* closestEntity;

    closestEntity = getNearestEntity(coord, nullptr, RS2::ResolveAllButTextImage);

    if (closestEntity) {
        for (RS_Entity* en = firstEntity(RS2::ResolveAllButTextImage);
             en;
             en = nextEntity(RS2::ResolveAllButTextImage)) {
            if (
                    !en->isVisible()
                    || en->getParent()->ignoredSnap()
                    ){
                continue;
            }

            sol = RS_Information::getIntersection(closestEntity,
                                                  en,
                                                  true);

            point=sol.getClosest(coord,&curDist,nullptr);
            if(sol.getNumber()>0 && curDist<minDist){
                closestPoint=point;
                minDist=curDist;
            }

        }
    }
    if(dist && closestPoint.valid) {
        *dist = minDist;
    }

    return closestPoint;
}

RS_Vector RS_EntityContainer::getNearestVirtualIntersection(const RS_Vector& coord,
                                                            const double& angle,
                                                            double* dist)
{

    RS_Vector point;                // endpoint found
    RS_VectorSolutions sol;
    RS_Entity* closestEntity;
    RS_Vector second_coord;

    second_coord.set(angle);
    closestEntity = getNearestEntity(coord, nullptr, RS2::ResolveAllButTextImage);

    if (closestEntity)
    {
        RS_ConstructionLineData data(coord,coord + second_coord);
        auto line = new RS_ConstructionLine(this,data);

        sol = RS_Information::getIntersection(closestEntity,line,true);
        if (sol.getVector().empty())
        {
            return coord;
        }
        else
        {
            point=sol.getClosest(coord,dist,nullptr);
            return point;
        }
    }
    else
    {
        return coord;
    }


}


RS_Vector RS_EntityContainer::getNearestRef(const RS_Vector& coord,
                                            double* dist) const{

    double minDist = RS_MAXDOUBLE;  // minimum measured distance
    double curDist;                 // currently measured distance
    RS_Vector closestPoint(false);  // closest found endpoint
    RS_Vector point;                // endpoint found

    for(auto en: entities){

        if (en->isVisible()) {
            point = en->getNearestRef(coord, &curDist);
            if (point.valid && curDist<minDist) {
                closestPoint = point;
                minDist = curDist;
                if (dist) {
                    *dist = minDist;
                }
            }
        }
    }

    return closestPoint;
}


RS_Vector RS_EntityContainer::getNearestSelectedRef(const RS_Vector& coord,
                                                    double* dist) const{

    double minDist = RS_MAXDOUBLE;  // minimum measured distance
    double curDist;                 // currently measured distance
    RS_Vector closestPoint(false);  // closest found endpoint
    RS_Vector point;                // endpoint found

    for(auto en: entities){

        if (en->isVisible() && en->isSelected() && !en->isParentSelected()) {
            point = en->getNearestSelectedRef(coord, &curDist);
            if (point.valid && curDist<minDist) {
                closestPoint = point;
                minDist = curDist;
                if (dist) {
                    *dist = minDist;
                }
            }
        }
    }

    return closestPoint;
}


double RS_EntityContainer::getDistanceToPoint(const RS_Vector& coord,
                                              RS_Entity** entity,
                                              RS2::ResolveLevel level,
                                              double solidDist) const{

    RS_DEBUG->print("RS_EntityContainer::getDistanceToPoint");


    double minDist = RS_MAXDOUBLE;      // minimum measured distance
    double curDist;                     // currently measured distance
    RS_Entity* closestEntity = nullptr;    // closest entity found
    RS_Entity* subEntity = nullptr;

    for(auto e: entities){

        if (e->isVisible() && (e->getLayer()==nullptr || !e->getLayer()->isLocked())) {
            RS_DEBUG->print("entity: getDistanceToPoint");
            RS_DEBUG->print("entity: %d", e->rtti());
            // bug#426, need to ignore Images to find nearest intersections
            if(level==RS2::ResolveAllButTextImage && e->rtti()==RS2::EntityImage) continue;
            curDist = e->getDistanceToPoint(coord, &subEntity, level, solidDist);

            RS_DEBUG->print("entity: getDistanceToPoint: OK");

            /*
             * By using '<=', we will prefer the *last* item in the container if there are multiple
             * entities that are *exactly* the same distance away, which should tend to be the one
             * drawn most recently, and the one most likely to be visible (as it is also the order
             * that the software draws the entities). This makes a difference when one entity is
             * drawn directly over top of another, and it's reasonable to assume that humans will
             * tend to want to reference entities that they see or have recently drawn as opposed
             * to deeper more forgotten and invisible ones...
             */
            if (curDist<=minDist)
            {
                switch(level){
                case RS2::ResolveAll:
                case RS2::ResolveAllButTextImage:
                    closestEntity = subEntity;
                    break;
                default:
                    closestEntity = e;
                }
                minDist = curDist;
            }
        }
    }

    if (entity) {
        *entity = closestEntity;
    }
    RS_DEBUG->print("RS_EntityContainer::getDistanceToPoint: OK");

    return minDist;
}



RS_Entity* RS_EntityContainer::getNearestEntity(const RS_Vector& coord,
                                                double* dist,
                                                RS2::ResolveLevel level) const{

    RS_DEBUG->print("RS_EntityContainer::getNearestEntity");

    RS_Entity* e = nullptr;

    // distance for points inside solids:
    double solidDist = RS_MAXDOUBLE;
    if (dist) {
        solidDist = *dist;
    }

    double d = getDistanceToPoint(coord, &e, level, solidDist);

    if (e && e->isVisible()==false) {
        e = nullptr;
    }

    // if d is negative, use the default distance (used for points inside solids)
    if (dist) {
        *dist = d;
    }
    RS_DEBUG->print("RS_EntityContainer::getNearestEntity: OK");

    return e;
}



/**
 * Rearranges the atomic entities in this container in a way that connected
 * entities are stored in the right order and direction.
 * Non-recoursive. Only affects atomic entities in this container.
 *
 * @retval true all contours were closed
 * @retval false at least one contour is not closed

 * to do: find closed contour by flood-fill
 */
bool RS_EntityContainer::optimizeContours() {
    //    std::cout<<"RS_EntityContainer::optimizeContours: begin"<<std::endl;

    //    DEBUG_HEADER
    //    std::cout<<"loop with count()="<<count()<<std::endl;
    RS_DEBUG->print("RS_EntityContainer::optimizeContours");

    RS_EntityContainer tmp;
    tmp.setAutoUpdateBorders(false);
    bool closed=true;

    /** accept all full circles **/
    QList<RS_Entity*> enList;
    foreach(auto e1, entities){
        if (!e1->isEdge() || e1->isContainer() ) {
            enList<<e1;
            continue;
        }

        //detect circles and whole ellipses
        switch(e1->rtti()){
        case RS2::EntityEllipse:
            if(static_cast<RS_Ellipse*>(e1)->isEllipticArc())
                continue;
            // fall-through
        case RS2::EntityCircle:
            //directly detect circles, bug#3443277
            tmp.addEntity(e1->clone());
            enList<<e1;
            // fall-through
        default:
            continue;
        }

    }
    //    std::cout<<"RS_EntityContainer::optimizeContours: 1"<<std::endl;

    /** remove unsupported entities */
    for(RS_Entity* it: enList)
        removeEntity(it);

    /** check and form a closed contour **/
    //    std::cout<<"RS_EntityContainer::optimizeContours: 2"<<std::endl;
    /** the first entity **/
    RS_Entity* current(nullptr);
    if(count()>0) {
        current=entityAt(0)->clone();
        tmp.addEntity(current);
        removeEntity(entityAt(0));
    }else {
        if(tmp.count()==0) return false;
    }
    //    std::cout<<"RS_EntityContainer::optimizeContours: 3"<<std::endl;
    RS_Vector vpStart;
    RS_Vector vpEnd;
    if(current){
        vpStart=current->getStartpoint();
        vpEnd=current->getEndpoint();
    }
    RS_Entity* next = nullptr;
    //    std::cout<<"RS_EntityContainer::optimizeContours: 4"<<std::endl;
    /** connect entities **/
    const auto errMsg=QObject::tr("Hatch failed due to a gap=%1 between (%2, %3) and (%4, %5)");

    while (count()>0) {
        double dist = 0.;
        RS_Vector vpTmp=getNearestEndpoint(vpEnd,&dist,&next);
        if (dist > contourTolerance) {
            if(vpEnd.squaredTo(vpStart) < contourTolerance) {
                RS_Entity* e2=entityAt(0);
                tmp.addEntity(e2->clone());
                vpStart=e2->getStartpoint();
                vpEnd=e2->getEndpoint();
                removeEntity(e2);
                continue;
            }
            else {
                QG_DIALOGFACTORY->commandMessage(
                            errMsg.arg(dist).arg(vpTmp.x).arg(vpTmp.y).arg(vpEnd.x).arg(vpEnd.y)
                            );
                RS_DEBUG->print(RS_Debug::D_ERROR, "RS_EntityContainer::optimizeContours: hatch failed due to a gap");
                closed=false;
                break;
            }
        }
        if(!next) { 	    //workaround if next is nullptr
            //      	    std::cout<<"RS_EntityContainer::optimizeContours: next is nullptr" <<std::endl;
            RS_DEBUG->print("RS_EntityContainer::optimizeContours: next is nullptr");
            //			closed=false;	//workaround if next is nullptr
            break;			//workaround if next is nullptr
        } 					//workaround if next is nullptr
        if(closed) {
            next->setProcessed(true);
            RS_Entity* eTmp = next->clone();
            if(vpEnd.squaredTo(eTmp->getStartpoint())>vpEnd.squaredTo(eTmp->getEndpoint()))
                eTmp->revertDirection();
            vpEnd=eTmp->getEndpoint();
            tmp.addEntity(eTmp);
            removeEntity(next);
        }
    }
    //    DEBUG_HEADER
    //    if(vpEnd.valid && vpEnd.squaredTo(vpStart) > 1e-8) {
    //		QG_DIALOGFACTORY->commandMessage(errMsg.arg(vpEnd.distanceTo(vpStart))
    //											 .arg(vpStart.x).arg(vpStart.y).arg(vpEnd.x).arg(vpEnd.y));
    //        RS_DEBUG->print("RS_EntityContainer::optimizeContours: hatch failed due to a gap");
    //        closed=false;
    //    }
    //    std::cout<<"RS_EntityContainer::optimizeContours: 5"<<std::endl;


    // add new sorted entities:
    for(auto en: tmp){
        en->setProcessed(false);
        addEntity(en->clone());
        en->reparent(this);
    }
    //    std::cout<<"RS_EntityContainer::optimizeContours: 6"<<std::endl;

    if(closed) {
        RS_DEBUG->print("RS_EntityContainer::optimizeContours: OK");
    }
    else {
        RS_DEBUG->print("RS_EntityContainer::optimizeContours: bad");
    }
    //    std::cout<<"RS_EntityContainer::optimizeContours: end: count()="<<count()<<std::endl;
    //    std::cout<<"RS_EntityContainer::optimizeContours: closed="<<closed<<std::endl;
    return closed;
}


bool RS_EntityContainer::hasEndpointsWithinWindow(const RS_Vector& v1, const RS_Vector& v2) {
    for(auto e: entities){
        if (e->hasEndpointsWithinWindow(v1, v2))  {
            return true;
        }
    }

    return false;
}


void RS_EntityContainer::move(const RS_Vector& offset) {
    moveBorders(offset);
    for(auto* e: entities){
        e->move(offset);
        adjustBorders(e);
    }
    if (autoUpdateBorders)
        calculateBorders();
}



void RS_EntityContainer::rotate(const RS_Vector& center, const double& angle) {
    RS_EntityContainer::rotate(center, RS_Vector{angle});
}


void RS_EntityContainer::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    resetBorders();

    for(auto* e: entities){
        e->rotate(center, angleVector);
        adjustBorders(e);
    }
    if (autoUpdateBorders)
        calculateBorders();
}


void RS_EntityContainer::scale(const RS_Vector& center, const RS_Vector& factor) {
    if (std::abs(factor.x)>RS_TOLERANCE && std::abs(factor.y)>RS_TOLERANCE) {
        scaleBorders(center, factor);
        for(auto* e: entities){
            e->scale(center, factor);
            adjustBorders(e);
        }
        if (autoUpdateBorders)
            calculateBorders();
    }
}



void RS_EntityContainer::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    if (axisPoint1.distanceTo(axisPoint2)>RS_TOLERANCE) {

        resetBorders();
        for(auto* e: entities){
            e->mirror(axisPoint1, axisPoint2);
            adjustBorders(e);
        }
    }
}

RS_Entity& RS_EntityContainer::shear(double k)
{
    for (auto* e: *this)
        e->shear(k);
    calculateBorders();
    return *this;
}


void RS_EntityContainer::stretch(const RS_Vector& firstCorner,
                                 const RS_Vector& secondCorner,
                                 const RS_Vector& offset) {

    if (getMin().isInWindow(firstCorner, secondCorner) &&
            getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    } else {

        for(auto* e: entities){
            e->stretch(firstCorner, secondCorner, offset);
        }
    }

    // some entitiycontainers might need an update (e.g. RS_Leader):
    update();
}



void RS_EntityContainer::moveRef(const RS_Vector& ref,
                                 const RS_Vector& offset) {

    resetBorders();
    for(auto* e: entities){
        e->moveRef(ref, offset);
        adjustBorders(e);
    }
    if (autoUpdateBorders) {
        calculateBorders();
    }
}


void RS_EntityContainer::moveSelectedRef(const RS_Vector& ref,
                                         const RS_Vector& offset) {

    resetBorders();
    for(auto* e: entities){
        e->moveSelectedRef(ref, offset);
        adjustBorders(e);
    }
    if (autoUpdateBorders) {
        calculateBorders();
    }
}

void RS_EntityContainer::revertDirection() {
    // revert entity order in the container
    for(int k = 0; k < entities.size() / 2; ++k) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
        entities.swapItemsAt(k, entities.size() - 1 - k);
#else
        entities.swap(k, entities.size() - 1 - k);
#endif
    }

    // revert each entity itself
    for(RS_Entity* entity: entities)
        entity->revertDirection();
}

/**
 * @brief RS_EntityContainer::draw() draw entities in order
 * @param painter
 * @param view
 */
void RS_EntityContainer::draw(RS_Painter* painter, RS_GraphicView* view, double& /*patternOffset*/)
{
    if (painter == nullptr || view == nullptr)
        return;

    foreach (auto* e, entities)
        view->drawEntity(painter, e);
}

/**
 * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
 * Contour Area =\oint x dy
 * @return line integral \oint x dy along the entity
 */
double RS_EntityContainer::areaLineIntegral() const
{
    //TODO make sure all contour integral is by counter-clockwise
    double contourArea=0.;
    //closed area is always positive
    double closedArea=0.;
    double subArea = 0.;

    // edges:

    RS_Vector previousPoint(false);
    for(unsigned i=0; i < count(); ++i) {
        RS_Entity* e = entities.at(i);
        if (isClosedLoop(*e))
        {
            if (e->isContainer())
                subArea += e->areaLineIntegral();
            else
                closedArea += e->areaLineIntegral();
            continue;
        }
        e->setLayer(getLayer());
        double lineIntegral = e->areaLineIntegral();
        RS_Vector startPoint = e->getStartpoint();
        RS_Vector endPoint = e->getEndpoint();
        LC_ERR<<e->getId()<<": int = "<<lineIntegral<<": "<<startPoint.x<<" - "<<endPoint.x;

        // the line integral is always by the direction: from the start point to the end point
        if (previousPoint.valid) {
            double distance = endPointDistance(previousPoint, *e);
            if (distance > contourTolerance)
                RS_DEBUG->print(RS_Debug::D_ERROR, "%s(): contour area calculation maybe incorrect: gap of %lg found at (%lg, %lg)",
                                __func__, distance, previousPoint.x, previousPoint.y);
        }
        // assume the contour is a simple connected loop
        if (previousPoint.valid && endPoint.squaredTo(previousPoint) <= RS_TOLERANCE15)
        {
            contourArea -= lineIntegral;
            previousPoint = startPoint;
        } else {
            bool useEndPoint = true;
            if (!previousPoint.valid && i + 1 < count()) {
                useEndPoint = endPointDistance(endPoint, *entities.at(i+1))
                        < endPointDistance(startPoint, *entities.at(i+1));
            }
            contourArea += useEndPoint ? lineIntegral : - lineIntegral;
            previousPoint = useEndPoint ? endPoint : startPoint;
        }
    }
    return std::abs(contourArea) + closedArea - subArea;
}

bool RS_EntityContainer::ignoredOnModification() const
{
    switch(rtti()){
    // commented out Insert to allow snapping on block, bug#523
    // case RS2::EntityInsert:         /**Insert*/
    case RS2::EntitySpline:
    case RS2::EntityMText:        /**< Text 15*/
    case RS2::EntityText:         /**< Text 15*/
    case RS2::EntityDimAligned:   /**< Aligned Dimension */
    case RS2::EntityDimLinear:    /**< Linear Dimension */
    case RS2::EntityDimRadial:    /**< Radial Dimension */
    case RS2::EntityDimDiametric: /**< Diametric Dimension */
    case RS2::EntityDimAngular:   /**< Angular Dimension */
    case RS2::EntityDimLeader:    /**< Leader Dimension */
    case RS2::EntityDimArc:       /**< Arc Dimension */
    case RS2::EntityHatch:
        return true;
    default:
        return false;
    }
}

bool RS_EntityContainer::ignoredSnap() const
{
    // issue #652 , disable snap for hatch
    // TODO, should snapping on hatch be a feature enabled by settings?
    if (getParent() && getParent()->rtti() == RS2::EntityHatch)
        return true;
    return ignoredOnModification();
}

QList<RS_Entity *>::const_iterator RS_EntityContainer::begin() const
{
    return entities.begin();
}

QList<RS_Entity *>::const_iterator RS_EntityContainer::end() const
{
    return entities.end();
}

QList<RS_Entity *>::iterator RS_EntityContainer::begin()
{
    return entities.begin();
}

QList<RS_Entity *>::iterator RS_EntityContainer::end()
{
    return entities.end();
}

/**
 * Dumps the entities to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_EntityContainer& ec) {

    static int indent = 0;

    std::string tab(indent * 2, ' ');

    ++indent;

    unsigned long int id = ec.getId();

    os << tab << "EntityContainer[" << id << "]: \n";
    os << tab << "Borders[" << id << "]: "
       << ec.minV << " - " << ec.maxV << "\n";
    //os << tab << "Unit[" << id << "]: "
    //<< RS_Units::unit2string (ec.unit) << "\n";
    if (ec.getLayer()) {
        os << tab << "Layer[" << id << "]: "
           << ec.getLayer()->getName().toLatin1().data() << "\n";
    } else {
        os << tab << "Layer[" << id << "]: <nullptr>\n";
    }
    //os << ec.layerList << "\n";

    os << tab << " Flags[" << id << "]: "
       << (ec.getFlag(RS2::FlagVisible) ? "RS2::FlagVisible" : "");
    os << (ec.getFlag(RS2::FlagUndone) ? " RS2::FlagUndone" : "");
    os << (ec.getFlag(RS2::FlagSelected) ? " RS2::FlagSelected" : "");
    os << "\n";


    os << tab << "Entities[" << id << "]: \n";
    for(auto t: ec){
        switch (t->rtti()) {
        case RS2::EntityInsert:
            os << tab << *((RS_Insert*)t);
            os << tab << *((RS_Entity*)t);
            os << tab << *((RS_EntityContainer*)t);
            break;
        default:
            if (t->isContainer()) {
                os << tab << *((RS_EntityContainer*)t);
            } else {
                os << tab << *t;
            }
            break;
        }
    }
    os << tab << "\n\n";
    --indent;

    return os;
}


RS_Entity* RS_EntityContainer::first() const
{
    return entities.first();
}

RS_Entity* RS_EntityContainer::last() const
{
    return entities.last();
}

const QList<RS_Entity*>& RS_EntityContainer::getEntityList()
{
    return entities;
}

std::vector<std::unique_ptr<RS_EntityContainer>> RS_EntityContainer::getLoops() const
{
    if (entities.empty())
        return {};

    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
    RS_EntityContainer edges(nullptr, false);
    for(auto* e1: entities){
        if (e1 != nullptr && e1->isContainer())
        {
            if (e1->isContainer()){
                auto subLoops = static_cast<RS_EntityContainer*>(e1)->getLoops();
                for (auto& subLoop: subLoops)
                    loops.push_back(std::move(subLoop));
            }
            continue;
        }

        if (!e1->isEdge())
            continue;

        //detect circles and whole ellipses
        switch(e1->rtti()){
        case RS2::EntityEllipse:
        if(static_cast<RS_Ellipse*>(e1)->isEllipticArc()) {
            edges.addEntity(e1);
            break;
        }
        [[fallthrough]];
        case RS2::EntityCircle:
        {
        auto ec = std::make_unique<RS_EntityContainer>(nullptr, false);
        ec->addEntity(e1);
        loops.push_back(std::move(ec));
        break;
        }
        default:
        edges.addEntity(e1);
        }
    }

    //find loops
    while (!edges.isEmpty())
    {
        LC_LoopUtils::LoopExtractor extractor{edges};
        auto subLoops = extractor.extract();
        for (auto& loop: subLoops)
            loops.push_back(std::move(loop));
    }
    return loops;
}

