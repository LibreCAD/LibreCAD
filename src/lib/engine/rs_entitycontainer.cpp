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


#include "rs_entitycontainer.h"

#include "rs_debug.h"
#include "rs_dimension.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_insert.h"
#include "rs_spline.h"
#include "rs_information.h"
#include "rs_graphicview.h"

bool RS_EntityContainer::autoUpdateBorders = true;

/**
 * Default constructor.
 *
 * @param owner True if we own and also delete the entities.
 */
RS_EntityContainer::RS_EntityContainer(RS_EntityContainer* parent,
                                       bool owner)
        : RS_Entity(parent) {

    autoDelete=owner;
        RS_DEBUG->print("RS_EntityContainer::RS_EntityContainer: "
		"owner: %d", (int)owner);
    subContainer = NULL;
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
    clear();
}



RS_Entity* RS_EntityContainer::clone() {
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
    for (RS_Entity* e=firstEntity();
            e!=NULL;
            e=nextEntity()) {
        if (!e->getFlag(RS2::FlagTemp)) {
            tmp.append(e->clone());
        }
    }

    // clear shared pointers:
    entities.clear();
    setOwner(autoDel);

    // point to new deep copies:
    for (int i = 0; i < tmp.size(); ++i) {
        RS_Entity* e = tmp.at(i);
        entities.append(e);
        e->reparent(this);
    }
}



void RS_EntityContainer::reparent(RS_EntityContainer* parent) {
    RS_Entity::reparent(parent);

    // All sub-entities:
    for (RS_Entity* e=firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=nextEntity(RS2::ResolveNone)) {
        e->reparent(parent);
    }
}



/**
 * Called when the undo state changed. Forwards the event to all sub-entities.
 *
 * @param undone true: entity has become invisible.
 *               false: entity has become visible.
 */
void RS_EntityContainer::undoStateChanged(bool undone) {

    RS_Entity::undoStateChanged(undone);

    // ! don't pass on to subentities. undo list handles them
    // All sub-entities:
    /*for (RS_Entity* e=firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=nextEntity(RS2::ResolveNone)) {
    	e->setUndoState(undone);
}*/
}



void RS_EntityContainer::setVisible(bool v) {
    RS_DEBUG->print("RS_EntityContainer::setVisible: %d", v);
    RS_Entity::setVisible(v);

    // All sub-entities:
    for (RS_Entity* e=firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=nextEntity(RS2::ResolveNone)) {
        RS_DEBUG->print("RS_EntityContainer::setVisible: subentity: %d", v);
        e->setVisible(v);
    }
}



/**
 * @return Total length of all entities in this container.
 */
double RS_EntityContainer::getLength() {
    double ret = 0.0;

    for (RS_Entity* e=firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=nextEntity(RS2::ResolveNone)) {
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
        for (RS_Entity* e=firstEntity(RS2::ResolveNone);
                e!=NULL;
                e=nextEntity(RS2::ResolveNone)) {
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
                e!=NULL;
                e=nextEntity(RS2::ResolveNone)) {
            e->toggleSelected();
    }*/
        return true;
    } else {
        return false;
    }
}



/**
 * Selects all entities within the given area.
 *
 * @param select True to select, False to deselect the entities.
 */
void RS_EntityContainer::selectWindow(RS_Vector v1, RS_Vector v2,
                                      bool select, bool cross) {

    bool included;

    for (RS_Entity* e=firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=nextEntity(RS2::ResolveNone)) {

        included = false;

        if (e->isVisible()) {
            if (e->isInWindow(v1, v2)) {
                //e->setSelected(select);
                included = true;
            } else if (cross==true) {
                RS_Line l[] =
                    {
                        RS_Line(NULL, RS_LineData(v1, RS_Vector(v2.x, v1.y))),
                        RS_Line(NULL, RS_LineData(RS_Vector(v2.x, v1.y), v2)),
                        RS_Line(NULL, RS_LineData(v2, RS_Vector(v1.x, v2.y))),
                        RS_Line(NULL, RS_LineData(RS_Vector(v1.x, v2.y), v1))
                    };
                RS_VectorSolutions sol;

                if (e->isContainer()) {
                    RS_EntityContainer* ec = (RS_EntityContainer*)e;
                    for (RS_Entity* se=ec->firstEntity(RS2::ResolveAll);
                            se!=NULL && included==false;
                            se=ec->nextEntity(RS2::ResolveAll)) {

                        for (int i=0; i<4; ++i) {
                            sol = RS_Information::getIntersection(
                                      se, &l[i], true);
                            if (sol.hasValid()) {
                                included = true;
                                break;
                            }
                        }
                    }
                } else {
                    for (int i=0; i<4; ++i) {
                        sol = RS_Information::getIntersection(e, &l[i], true);
                        if (sol.hasValid()) {
                            included = true;
                            break;
                        }
                    }
                }
            }
        }

        if (included) {
            e->setSelected(select);
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
           if (lst!=NULL) {
               RS_Layer* l = lst->getActive();
               if (l!=NULL && l->isLocked()) {
                   return;
               }
           }
       }
    */

    if (entity==NULL) {
        return;
    }

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
 * Inserts a entity to this container at the given position and updates 
 * the borders of this entity-container if autoUpdateBorders is true.
 */
void RS_EntityContainer::insertEntity(int index, RS_Entity* entity) {
    if (entity==NULL) {
        return;
    }

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
    if (entity==NULL) {
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
//RLZ TODO: in Q3PtrList if 'entity' is NULL remove the current item-> at.(entIdx)
//    and sets 'entIdx' in next() or last() if 'entity' is the last item in the list.
//    in LibreCAD is never called with NULL
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


/**
 * Counts all entities (branches of the tree). 
 */
unsigned long int RS_EntityContainer::count() {
    return entities.count();
}


/**
 * Counts all entities (leaves of the tree). 
 */
unsigned long int RS_EntityContainer::countDeep() {
    unsigned long int c=0;

    for (RS_Entity* t=firstEntity(RS2::ResolveNone);
            t!=NULL;
            t=nextEntity(RS2::ResolveNone)) {
        c+=t->countDeep();
    }

    return c;
}



/**
 * Counts the selected entities in this container.
 */
unsigned long int RS_EntityContainer::countSelected() {
    unsigned long int c=0;

    for (RS_Entity* t=firstEntity(RS2::ResolveNone);
            t!=NULL;
            t=nextEntity(RS2::ResolveNone)) {

        if (t->isSelected()) {
            c++;
        }
    }

    return c;
}



/**
 * Adjusts the borders of this graphic (max/min values)
 */
void RS_EntityContainer::adjustBorders(RS_Entity* entity) {
    //RS_DEBUG->print("RS_EntityContainer::adjustBorders");
    //resetBorders();

    if (entity!=NULL) {
        // make sure a container is not empty (otherwise the border
        //   would get extended to 0/0):
        if (!entity->isContainer() || entity->count()>0) {
            minV = RS_Vector::minimum(entity->getMin(),minV);
            maxV = RS_Vector::maximum(entity->getMax(),maxV);
        }

        // Notify parents. The border for the parent might
        // also change TODO: Check for efficiency
        //if(parent!=NULL) {
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
    for (RS_Entity* e=firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=nextEntity(RS2::ResolveNone)) {

        RS_Layer* layer = e->getLayer();

        RS_DEBUG->print("RS_EntityContainer::calculateBorders: "
                        "isVisible: %d", (int)e->isVisible());

        if (e->isVisible() && (layer==NULL || !layer->isFrozen())) {
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

    RS_DEBUG->print("RS_EntityCotnainer::calculateBorders: size: %f,%f",
                    getSize().x, getSize().y);

    //RS_DEBUG->print("  borders: %f/%f %f/%f", minV.x, minV.y, maxV.x, maxV.y);

    //printf("borders: %lf/%lf  %lf/%lf\n", minV.x, minV.y, maxV.x, maxV.y);
    //RS_Entity::calculateBorders();
}



/**
 * Recalculates the borders of this entity container including 
 * invisible entities.
 */
void RS_EntityContainer::forcedCalculateBorders() {
    //RS_DEBUG->print("RS_EntityContainer::calculateBorders");

    resetBorders();
    for (RS_Entity* e=firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=nextEntity(RS2::ResolveNone)) {

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
 * Updates all Dimension entities in this container and 
 * reposition their labels.
 */
void RS_EntityContainer::updateDimensions() {

    RS_DEBUG->print("RS_EntityContainer::updateDimensions()");

    //for (RS_Entity* e=firstEntity(RS2::ResolveNone);
    //        e!=NULL;
    //        e=nextEntity(RS2::ResolveNone)) {

    RS_Entity* e;
    for (int i = 0; i < entities.size(); ++i) {
        e = entities.at(i);
        if (RS_Information::isDimension(e->rtti())) {
            // update and reposition label:
            ((RS_Dimension*)e)->update(true);
        } else if (e->isContainer()) {
            ((RS_EntityContainer*)e)->updateDimensions();
        }
    }

    RS_DEBUG->print("RS_EntityContainer::updateDimensions() OK");
}



/**
 * Updates all Insert entities in this container.
 */
void RS_EntityContainer::updateInserts() {

    RS_DEBUG->print("RS_EntityContainer::updateInserts()");

    //for (RS_Entity* e=firstEntity(RS2::ResolveNone);
    //        e!=NULL;
    //        e=nextEntity(RS2::ResolveNone)) {
    RS_Entity* e;
    for (int i = 0; i < entities.size(); ++i) {
        e = entities.at(i);
        //// Only update our own inserts and not inserts of inserts
        if (e->rtti()==RS2::EntityInsert  /*&& e->getParent()==this*/) {
            ((RS_Insert*)e)->update();
        } else if (e->isContainer() && e->rtti()!=RS2::EntityHatch) {
            ((RS_EntityContainer*)e)->updateInserts();
        }
    }

    RS_DEBUG->print("RS_EntityContainer::updateInserts() OK");
}



/**
 * Renames all inserts with name 'oldName' to 'newName'. This is
 *   called after a block was rename to update the inserts.
 */
void RS_EntityContainer::renameInserts(const QString& oldName,
                                       const QString& newName) {
    RS_DEBUG->print("RS_EntityContainer::renameInserts()");

    //for (RS_Entity* e=firstEntity(RS2::ResolveNone);
    //        e!=NULL;
    //        e=nextEntity(RS2::ResolveNone)) {

    RS_Entity* e;
    for (int j = 0; j < entities.size(); ++j) {
        e = entities.at(j);
        if (e->rtti()==RS2::EntityInsert) {
            RS_Insert* i = ((RS_Insert*)e);
            if (i->getName()==oldName) {
                i->setName(newName);
            }
        } else if (e->isContainer()) {
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

    //for (RS_Entity* e=firstEntity(RS2::ResolveNone);
    //        e!=NULL;
    //        e=nextEntity(RS2::ResolveNone)) {
    RS_Entity* e;
    for (int i = 0; i < entities.size(); ++i) {
        e = entities.at(i);
        //// Only update our own inserts and not inserts of inserts
        if (e->rtti()==RS2::EntitySpline  /*&& e->getParent()==this*/) {
            ((RS_Spline*)e)->update();
        } else if (e->isContainer() && e->rtti()!=RS2::EntityHatch) {
            ((RS_EntityContainer*)e)->updateSplines();
        }
    }

    RS_DEBUG->print("RS_EntityContainer::updateSplines() OK");
}


/**
 * Updates the sub entities of this container. 
 */
void RS_EntityContainer::update() {
    //for (RS_Entity* e=firstEntity(RS2::ResolveNone);
    //        e!=NULL;
    //        e=nextEntity(RS2::ResolveNone)) {
    for (int i = 0; i < entities.size(); ++i) {
        entities.at(i)->update();
    }
}



/**
 * Returns the first entity or NULL if this graphic is empty.
 * @param level 
 */
RS_Entity* RS_EntityContainer::firstEntity(RS2::ResolveLevel level) {
//check if the entities list is empty, if not set entIdx pointing in first entity
    if (entities.isEmpty()) {
        entIdx = -1;
        return NULL;
    } else
        entIdx = 0;
    switch (level) {
    case RS2::ResolveNone:
        return entities.first();
        break;

    case RS2::ResolveAllButInserts: {
            subContainer=NULL;
            RS_Entity* e = entities.first();
            if (e!=NULL && e->isContainer() && e->rtti()!=RS2::EntityInsert) {
                subContainer = (RS_EntityContainer*)e;
                e = ((RS_EntityContainer*)e)->firstEntity(level);
                // emtpy container:
                if (e==NULL) {
                    subContainer = NULL;
                    e = nextEntity(level);
                }
            }
            return e;
        }
        break;

    case RS2::ResolveAll: {
            subContainer=NULL;
            RS_Entity* e = entities.first();
            if (e!=NULL && e->isContainer()) {
                subContainer = (RS_EntityContainer*)e;
                e = ((RS_EntityContainer*)e)->firstEntity(level);
                // emtpy container:
                if (e==NULL) {
                    subContainer = NULL;
                    e = nextEntity(level);
                }
            }
            return e;
        }
        break;
    }

	return NULL;
}



/**
 * Returns the last entity or \p NULL if this graphic is empty.
 *
 * @param level \li \p 0 Groups are not resolved
 *              \li \p 1 (default) only Groups are resolved
 *              \li \p 2 all Entity Containers are resolved
 */
RS_Entity* RS_EntityContainer::lastEntity(RS2::ResolveLevel level) {
//check if the entities list is empty, if not set entIdx pointing in last entity
    if (entities.isEmpty()) {
        entIdx = -1;
        return NULL;
    } else
        entIdx = entities.size()-1;
    switch (level) {
    case RS2::ResolveNone:
        return entities.last();
        break;

    case RS2::ResolveAllButInserts: {
            RS_Entity* e = entities.last();
            subContainer = NULL;
            if (e!=NULL && e->isContainer() && e->rtti()!=RS2::EntityInsert) {
                subContainer = (RS_EntityContainer*)e;
                e = ((RS_EntityContainer*)e)->lastEntity(level);
            }
            return e;
        }
        break;

    case RS2::ResolveAll: {
            RS_Entity* e = entities.last();
            subContainer = NULL;
            if (e!=NULL && e->isContainer()) {
                subContainer = (RS_EntityContainer*)e;
                e = ((RS_EntityContainer*)e)->lastEntity(level);
            }
            return e;
        }
        break;
    }

	return NULL;
}



/**
 * Returns the next entity or container or \p NULL if the last entity 
 * returned by \p next() was the last entity in the container.
 */
RS_Entity* RS_EntityContainer::nextEntity(RS2::ResolveLevel level) {

//set entIdx pointing in next entity and check if is out of range
    ++entIdx;
//assuming that empty list return size == 0
    if (entIdx >= entities.size())
        return NULL;
    switch (level) {
    case RS2::ResolveNone:
        return entities.at(entIdx);
        break;

    case RS2::ResolveAllButInserts: {
            RS_Entity* e=NULL;
            if (subContainer!=NULL) {
                e = subContainer->nextEntity(level);
                if (e!=NULL) {
                    return e;
                } else {
                    e = entities.at(entIdx);
                }
            } else {
                e = entities.at(entIdx);
            }
            if (e!=NULL && e->isContainer() && e->rtti()!=RS2::EntityInsert) {
                subContainer = (RS_EntityContainer*)e;
                e = ((RS_EntityContainer*)e)->firstEntity(level);
                // emtpy container:
                if (e==NULL) {
                    subContainer = NULL;
                    e = nextEntity(level);
                }
            }
            return e;
        }
        break;

    case RS2::ResolveAll: {
            RS_Entity* e=NULL;
            if (subContainer!=NULL) {
                e = subContainer->nextEntity(level);
                if (e!=NULL) {
                    return e;
                } else {
                    e = entities.at(entIdx);
                }
            } else {
                e = entities.at(entIdx);
            }
            if (e!=NULL && e->isContainer()) {
                subContainer = (RS_EntityContainer*)e;
                e = ((RS_EntityContainer*)e)->firstEntity(level);
                // emtpy container:
                if (e==NULL) {
                    subContainer = NULL;
                    e = nextEntity(level);
                }
            }
            return e;
        }
        break;
    }
	return NULL;
}



/**
 * Returns the prev entity or container or \p NULL if the last entity 
 * returned by \p prev() was the first entity in the container.
 */
RS_Entity* RS_EntityContainer::prevEntity(RS2::ResolveLevel level) {
//set entIdx pointing in prev entity and check if is out of range
    if (entities.isEmpty()) {
        entIdx = -1;
        return NULL;
    } else
        --entIdx;
    if (entIdx < 0)
        return NULL;
    switch (level) {

    case RS2::ResolveNone:
        return entities.at(entIdx);
        break;
    
	case RS2::ResolveAllButInserts: {
            RS_Entity* e=NULL;
            if (subContainer!=NULL) {
                e = subContainer->prevEntity(level);
                if (e!=NULL) {
                    return e;
                } else {
                    e = entities.at(entIdx);
                }
            } else {
                e = entities.at(entIdx);
            }
            if (e!=NULL && e->isContainer() && e->rtti()!=RS2::EntityInsert) {
                subContainer = (RS_EntityContainer*)e;
                e = ((RS_EntityContainer*)e)->lastEntity(level);
                // emtpy container:
                if (e==NULL) {
                    subContainer = NULL;
                    e = prevEntity(level);
                }
            }
            return e;
        }

    case RS2::ResolveAll: {
            RS_Entity* e=NULL;
            if (subContainer!=NULL) {
                e = subContainer->prevEntity(level);
                if (e!=NULL) {
                    return e;
                } else {
                    e = entities.at(entIdx);
                }
            } else {
                e = entities.at(entIdx);
            }
            if (e!=NULL && e->isContainer()) {
                subContainer = (RS_EntityContainer*)e;
                e = ((RS_EntityContainer*)e)->lastEntity(level);
                // emtpy container:
                if (e==NULL) {
                    subContainer = NULL;
                    e = prevEntity(level);
                }
            }
            return e;
        }
    }
	return NULL;
}



/**
 * @return Entity at the given index or NULL if the index is out of range.
 */
RS_Entity* RS_EntityContainer::entityAt(int index) {
    if (entities.size() > index && index >= 0)
        return entities.at(index);
    else
        return NULL;
}



/**
 * @return Current index.
 */
int RS_EntityContainer::entityAt() {
    return entIdx;
}


/**
 * Finds the given entity and makes it the current entity if found.
 */
int RS_EntityContainer::findEntity(RS_Entity* entity) {
    entIdx = entities.indexOf(entity);
    return entIdx;
}



/**
 * Returns the copy to a new iterator for traversing the entities.
 */
QListIterator<RS_Entity*> RS_EntityContainer::createIterator() {
    return QListIterator<RS_Entity*>(entities);
}



/**
 * @return The point which is closest to 'coord' 
 * (one of the vertexes)
 */
RS_Vector RS_EntityContainer::getNearestEndpoint(const RS_Vector& coord,
        double* dist) {

    double minDist = RS_MAXDOUBLE;  // minimum measured distance
    double curDist;                 // currently measured distance
    RS_Vector closestPoint(false);  // closest found endpoint
    RS_Vector point;                // endpoint found

    //RS_PtrListIterator<RS_Entity> it = createIterator();
    //RS_Entity* en;
    //while ( (en = it.current()) != NULL ) {
    //    ++it;
    for (RS_Entity* en = firstEntity();
            en != NULL;
            en = nextEntity()) {

        if (en->isVisible()) {
            point = en->getNearestEndpoint(coord, &curDist);
            if (point.valid && curDist<minDist) {
                closestPoint = point;
                minDist = curDist;
                if (dist!=NULL) {
                    *dist = curDist;
                }
            }
        }
    }

    return closestPoint;
}



RS_Vector RS_EntityContainer::getNearestPointOnEntity(const RS_Vector& coord,
        bool onEntity, double* dist, RS_Entity** entity) {

    RS_Vector point(false);

    RS_Entity* e = getNearestEntity(coord, dist, RS2::ResolveNone);

    if (e!=NULL && e->isVisible()) {
        point = e->getNearestPointOnEntity(coord, onEntity, dist, entity);
    }

    return point;
}



RS_Vector RS_EntityContainer::getNearestCenter(const RS_Vector& coord,
        double* dist) {

    RS_Vector point(false);
    RS_Entity* closestEntity;

    //closestEntity = getNearestEntity(coord, NULL, RS2::ResolveAll);
    closestEntity = getNearestEntity(coord, NULL, RS2::ResolveNone);

    if (closestEntity!=NULL) {
        point = closestEntity->getNearestCenter(coord, dist);
    }

    return point;
}



RS_Vector RS_EntityContainer::getNearestMiddle(const RS_Vector& coord,
        double* dist) {

    RS_Vector point(false);
    RS_Entity* closestEntity;

    closestEntity = getNearestEntity(coord, NULL, RS2::ResolveNone);

    if (closestEntity!=NULL) {
        point = closestEntity->getNearestMiddle(coord, dist);
    }

    return point;


    /*
       double minDist = RS_MAXDOUBLE;  // minimum measured distance
       double curDist;                 // currently measured distance
       RS_Vector closestPoint;         // closest found endpoint
       RS_Vector point;                // endpoint found

       for (RS_Entity* en = firstEntity();
               en != NULL;
               en = nextEntity()) {

           if (en->isVisible()) {
               point = en->getNearestMiddle(coord, &curDist);
               if (curDist<minDist) {
                   closestPoint = point;
                   minDist = curDist;
                   if (dist!=NULL) {
                       *dist = curDist;
                   }
               }
           }
       }

       return closestPoint;
    */
}



RS_Vector RS_EntityContainer::getNearestDist(double distance,
        const RS_Vector& coord,
        double* dist) {

    RS_Vector point(false);
    RS_Entity* closestEntity;

    closestEntity = getNearestEntity(coord, NULL, RS2::ResolveNone);

    if (closestEntity!=NULL) {
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
    double curDist;                 // currently measured distance
    RS_Vector closestPoint(false); // closest found endpoint
    RS_Vector point;                // endpoint found
    RS_VectorSolutions sol;
    RS_Entity* closestEntity;

    closestEntity = getNearestEntity(coord, NULL, RS2::ResolveAll);

    if (closestEntity!=NULL) {
        for (RS_Entity* en = firstEntity(RS2::ResolveAll);
                en != NULL;
                en = nextEntity(RS2::ResolveAll)) {

            if (en->isVisible() && en!=closestEntity) {
                sol = RS_Information::getIntersection(closestEntity,
                                                      en,
                                                      true);

                for (int i=0; i<4; i++) {
                    point = sol.get(i);
                    if (point.valid) {
                        curDist = coord.distanceTo(point);

                        if (curDist<minDist) {
                            closestPoint = point;
                            minDist = curDist;
                            if (dist!=NULL) {
                                *dist = curDist;
                            }
                        }
                    }
                }
            }
        }
        //}
    }

    return closestPoint;
}



RS_Vector RS_EntityContainer::getNearestRef(const RS_Vector& coord,
        double* dist) {

    double minDist = RS_MAXDOUBLE;  // minimum measured distance
    double curDist;                 // currently measured distance
    RS_Vector closestPoint(false);  // closest found endpoint
    RS_Vector point;                // endpoint found

    for (RS_Entity* en = firstEntity();
            en != NULL;
            en = nextEntity()) {

        if (en->isVisible()) {
            point = en->getNearestRef(coord, &curDist);
            if (point.valid && curDist<minDist) {
                closestPoint = point;
                minDist = curDist;
                if (dist!=NULL) {
                    *dist = curDist;
                }
            }
        }
    }

    return closestPoint;
}


RS_Vector RS_EntityContainer::getNearestSelectedRef(const RS_Vector& coord,
        double* dist) {

    double minDist = RS_MAXDOUBLE;  // minimum measured distance
    double curDist;                 // currently measured distance
    RS_Vector closestPoint(false);  // closest found endpoint
    RS_Vector point;                // endpoint found

    for (RS_Entity* en = firstEntity();
            en != NULL;
            en = nextEntity()) {

        if (en->isVisible() && en->isSelected() && !en->isParentSelected()) {
            point = en->getNearestSelectedRef(coord, &curDist);
            if (point.valid && curDist<minDist) {
                closestPoint = point;
                minDist = curDist;
                if (dist!=NULL) {
                    *dist = curDist;
                }
            }
        }
    }

    return closestPoint;
}


double RS_EntityContainer::getDistanceToPoint(const RS_Vector& coord,
        RS_Entity** entity,
        RS2::ResolveLevel level,
        double solidDist) {

    RS_DEBUG->print("RS_EntityContainer::getDistanceToPoint");

    double minDist = RS_MAXDOUBLE;      // minimum measured distance
    double curDist;                     // currently measured distance
    RS_Entity* closestEntity = NULL;    // closest entity found
    RS_Entity* subEntity = NULL;

    //int k=0;
    for (RS_Entity* e = firstEntity(level);
            e != NULL;
            e = nextEntity(level)) {

        if (e->isVisible()) {
            RS_DEBUG->print("entity: getDistanceToPoint");
            RS_DEBUG->print("entity: %d", e->rtti());
            curDist = e->getDistanceToPoint(coord, &subEntity, level, solidDist);

            RS_DEBUG->print("entity: getDistanceToPoint: OK");

            if (curDist<minDist) {
                if (level!=RS2::ResolveAll) {
                    closestEntity = e;
                } else {
                    closestEntity = subEntity;
                }
                minDist = curDist;
            }
        }
    }

    if (entity!=NULL) {
        *entity = closestEntity;
    }
    RS_DEBUG->print("RS_EntityContainer::getDistanceToPoint: OK");

    return minDist;
}



RS_Entity* RS_EntityContainer::getNearestEntity(const RS_Vector& coord,
        double* dist,
        RS2::ResolveLevel level) {

    RS_DEBUG->print("RS_EntityContainer::getNearestEntity");

    RS_Entity* e = NULL;

    // distance for points inside solids:
    double solidDist = RS_MAXDOUBLE;
    if (dist!=NULL) {
        solidDist = *dist;
    }

    double d = getDistanceToPoint(coord, &e, level, solidDist);

    if (e!=NULL && e->isVisible()==false) {
        e = NULL;
    }

    // if d is negative, use the default distance (used for points inside solids)
    if (dist!=NULL) {
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
 */
bool RS_EntityContainer::optimizeContours() {

    RS_DEBUG->print("RS_EntityContainer::optimizeContours");

    RS_Vector current(false);
    RS_Vector start(false);
    RS_EntityContainer tmp;

    bool changed = false;
    bool closed = true;

    for (uint ci=0; ci<count(); ++ci) {
        RS_Entity* e1=entityAt(ci);

        if (e1!=NULL && e1->isEdge() && !e1->isContainer() &&
                !e1->isProcessed()) {

            RS_AtomicEntity* ce = (RS_AtomicEntity*)e1;

            // next contour start:
            ce->setProcessed(true);
            tmp.addEntity(ce->clone());
            current = ce->getEndpoint();
            start = ce->getStartpoint();

            // find all connected entities:
            bool done;
            do {
                done = true;
                for (uint ei=0; ei<count(); ++ei) {
                    RS_Entity* e2=entityAt(ei);

                    if (e2!=NULL && e2->isEdge() && !e2->isContainer() &&
                            !e2->isProcessed()) {

                        RS_AtomicEntity* e = (RS_AtomicEntity*)e2;

                        if (e->getStartpoint().distanceTo(current) <
                                1.0e-4) {

                            e->setProcessed(true);
                            tmp.addEntity(e->clone());
                            current = e->getEndpoint();

                            done=false;
                        } else if (e->getEndpoint().distanceTo(current) <
                                   1.0e-4) {

                            e->setProcessed(true);
                            RS_AtomicEntity* cl = (RS_AtomicEntity*)e->clone();
                            cl->reverse();
                            tmp.addEntity(cl);
                            current = cl->getEndpoint();

                            changed = true;
                            done=false;
                        }
                    }
                }
                if (!done) {
                    changed = true;
                }
            } while (!done);

            if (current.distanceTo(start)>1.0e-4) {
                closed = false;
            }
        }
    }

    // remove all atomic entities:
    bool done;
    do {
        done = true;
        for (RS_Entity* en=firstEntity(); en!=NULL; en=nextEntity()) {
            if (!en->isContainer()) {
                removeEntity(en);
                done = false;
                break;
            }
        }
    } while (!done);

    // add new sorted entities:
    for (RS_Entity* en=tmp.firstEntity(); en!=NULL; en=tmp.nextEntity()) {
        en->setProcessed(false);
        addEntity(en->clone());
    }

    RS_DEBUG->print("RS_EntityContainer::optimizeContours: OK");
    return closed;
}


bool RS_EntityContainer::hasEndpointsWithinWindow(RS_Vector v1, RS_Vector v2) {
    for (RS_Entity* e=firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=nextEntity(RS2::ResolveNone)) {
        if (e->hasEndpointsWithinWindow(v1, v2))  {
            return true;
        }
    }

    return false;
}


void RS_EntityContainer::move(RS_Vector offset) {
    for (RS_Entity* e=firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=nextEntity(RS2::ResolveNone)) {
        e->move(offset);
    }
    if (autoUpdateBorders) {
        calculateBorders();
    }
}



void RS_EntityContainer::rotate(RS_Vector center, double angle) {
    for (RS_Entity* e=firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=nextEntity(RS2::ResolveNone)) {
        e->rotate(center, angle);
    }
    if (autoUpdateBorders) {
        calculateBorders();
    }
}



void RS_EntityContainer::scale(RS_Vector center, RS_Vector factor) {
    if (fabs(factor.x)>RS_TOLERANCE && fabs(factor.y)>RS_TOLERANCE) {
        for (RS_Entity* e=firstEntity(RS2::ResolveNone);
                e!=NULL;
                e=nextEntity(RS2::ResolveNone)) {
            e->scale(center, factor);
        }
    }
    if (autoUpdateBorders) {
        calculateBorders();
    }
}



void RS_EntityContainer::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    if (axisPoint1.distanceTo(axisPoint2)>1.0e-6) {
        for (RS_Entity* e=firstEntity(RS2::ResolveNone);
                e!=NULL;
                e=nextEntity(RS2::ResolveNone)) {
            e->mirror(axisPoint1, axisPoint2);
        }
    }
}


void RS_EntityContainer::stretch(RS_Vector firstCorner,
                                 RS_Vector secondCorner,
                                 RS_Vector offset) {

    if (getMin().isInWindow(firstCorner, secondCorner) &&
            getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    } else {
        for (RS_Entity* e=firstEntity(RS2::ResolveNone);
                e!=NULL;
                e=nextEntity(RS2::ResolveNone)) {
            e->stretch(firstCorner, secondCorner, offset);
        }
    }

    // some entitiycontainers might need an update (e.g. RS_Leader):
    update();
}



void RS_EntityContainer::moveRef(const RS_Vector& ref,
                                 const RS_Vector& offset) {

    for (RS_Entity* e=firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=nextEntity(RS2::ResolveNone)) {
        e->moveRef(ref, offset);
    }
    if (autoUpdateBorders) {
        calculateBorders();
    }
}


void RS_EntityContainer::moveSelectedRef(const RS_Vector& ref,
        const RS_Vector& offset) {

    for (RS_Entity* e=firstEntity(RS2::ResolveNone);
            e!=NULL;
            e=nextEntity(RS2::ResolveNone)) {
        e->moveSelectedRef(ref, offset);
    }
    if (autoUpdateBorders) {
        calculateBorders();
    }
}



void RS_EntityContainer::draw(RS_Painter* painter, RS_GraphicView* view,
                              double /*patternOffset*/) {

    if (painter==NULL || view==NULL) {
        return;
    }

    for (RS_Entity* e=firstEntity(RS2::ResolveNone);
            e!=NULL;
            e = nextEntity(RS2::ResolveNone)) {

        view->drawEntity(painter, e);
    }
}


/**
 * Dumps the entities to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_EntityContainer& ec) {

    static int indent = 0;

    char* tab = new char[indent*2+1];
    for(int i=0; i<indent*2; ++i) {
        tab[i] = ' ';
    }
    tab[indent*2] = '\0';

    ++indent;

    unsigned long int id = ec.getId();

    os << tab << "EntityContainer[" << id << "]: \n";
    os << tab << "Borders[" << id << "]: "
    << ec.minV << " - " << ec.maxV << "\n";
    //os << tab << "Unit[" << id << "]: "
    //<< RS_Units::unit2string (ec.unit) << "\n";
    if (ec.getLayer()!=NULL) {
        os << tab << "Layer[" << id << "]: "
        << ec.getLayer()->getName().toLatin1().data() << "\n";
    } else {
        os << tab << "Layer[" << id << "]: <NULL>\n";
    }
    //os << ec.layerList << "\n";

    os << tab << " Flags[" << id << "]: "
    << (ec.getFlag(RS2::FlagVisible) ? "RS2::FlagVisible" : "");
    os << (ec.getFlag(RS2::FlagUndone) ? " RS2::FlagUndone" : "");
    os << (ec.getFlag(RS2::FlagSelected) ? " RS2::FlagSelected" : "");
    os << "\n";


    os << tab << "Entities[" << id << "]: \n";
    for (RS_Entity* t=ec.firstEntity();
            t!=NULL;
            t=ec.nextEntity()) {

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

    delete[] tab;
    return os;
}

