/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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


#include "rs_preview.h"

#include "rs_entitycontainer.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_mouseevent.h"
#include "rs_settings.h"


/**
 * Constructor.
 */
RS_Preview::RS_Preview(RS_EntityContainer* parent)
        : RS_EntityContainer(parent) {

    RS_SETTINGS->beginGroup("/Appearance");
    maxEntities = RS_SETTINGS->readNumEntry("/MaxPreview", 100);
    RS_SETTINGS->endGroup();
}



/**
 * Destructor.
 */
RS_Preview::~RS_Preview() {
    /*
    RS_SETTINGS->beginGroup("/Appearance");
    RS_SETTINGS->writeEntry("/MaxPreview", maxEntities);
    RS_SETTINGS->endGroup();
    */
}



/**
 * Adds an entity to this preview and removes any attributes / layer
 * connectsions before that.
 */
void RS_Preview::addEntity(RS_Entity* entity) {
    if (entity==NULL || entity->isUndone()) {
        return;
    }
	

    // only border preview for complex entities:
    //if ((entity->count() > maxEntities-count()) &&

    bool addBorder = false;

    if (entity->rtti()==RS2::EntityImage || entity->rtti()==RS2::EntityHatch || 
		entity->rtti()==RS2::EntityInsert) {

        addBorder = true;
    } else {
        if (entity->isContainer() && entity->rtti()!=RS2::EntitySpline) {
            if (entity->countDeep() > maxEntities-countDeep()) {
                addBorder = true;
            }
        }
    }

    if (addBorder) {
        RS_Vector min = entity->getMin();
        RS_Vector max = entity->getMax();

        RS_Line* l1 =
            new RS_Line(this,
                        RS_LineData(RS_Vector(min.x, min.y),
                                    RS_Vector(max.x, min.y)));
        RS_Line* l2 =
            new RS_Line(this,
                        RS_LineData(RS_Vector(max.x, min.y),
                                    RS_Vector(max.x, max.y)));
        RS_Line* l3 =
            new RS_Line(this,
                        RS_LineData(RS_Vector(max.x, max.y),
                                    RS_Vector(min.x, max.y)));
        RS_Line* l4 =
            new RS_Line(this,
                        RS_LineData(RS_Vector(min.x, max.y),
                                    RS_Vector(min.x, min.y)));

        RS_EntityContainer::addEntity(l1);
        RS_EntityContainer::addEntity(l2);
        RS_EntityContainer::addEntity(l3);
        RS_EntityContainer::addEntity(l4);

        delete entity;
        entity = NULL;
    } else {
        entity->setLayer(NULL);
        entity->setSelected(false);
        entity->reparent(this);
		// Don't set this pen, let drawing routines decide entity->setPenToActive();
        RS_EntityContainer::addEntity(entity);
    }
}



/**
 * Clones the given entity and adds the clone to the preview.
 */
void RS_Preview::addCloneOf(RS_Entity* entity) {
    if (entity==NULL) {
        return;
    }

    RS_Entity* clone = entity->clone();
    addEntity(clone);
}



/**
 * Adds all entities from 'container' to the preview (unselected).
 */
void RS_Preview::addAllFrom(RS_EntityContainer& container) {
    int c=0;
    for (RS_Entity* e=container.firstEntity();
            e!=NULL; e=container.nextEntity()) {

        if (c<maxEntities) {
            RS_Entity* clone = e->clone();
            clone->setSelected(false);
            clone->reparent(this);

            c+=clone->countDeep();
            addEntity(clone);
            // clone might be NULL after this point
        }
    }
}


/**
 * Adds all selected entities from 'container' to the preview (unselected).
 */
void RS_Preview::addSelectionFrom(RS_EntityContainer& container) {
    int c=0;
    for (RS_Entity* e=container.firstEntity();
            e!=NULL; e=container.nextEntity()) {

        if (e->isSelected() && c<maxEntities) {
            RS_Entity* clone = e->clone();
            clone->setSelected(false);
            clone->reparent(this);

            c+=clone->countDeep();
            addEntity(clone);
            // clone might be NULL after this point
        }
    }
}


/**
 * Adds all entities in the given range and those which have endpoints
 * in the given range to the preview.
 */
void RS_Preview::addStretchablesFrom(RS_EntityContainer& container,
                                     const RS_Vector& v1, const RS_Vector& v2) {

    int c=0;

    for (RS_Entity* e=container.firstEntity();
            e!=NULL; e=container.nextEntity()) {

        if (e->isVisible() &&
                e->rtti()!=RS2::EntityHatch &&
                (e->isInWindow(v1, v2) ||
                 e->hasEndpointsWithinWindow(v1, v2)) && c<maxEntities) {

            RS_Entity* clone = e->clone();
            //clone->setSelected(false);
            clone->reparent(this);

            c+=clone->countDeep();
            addEntity(clone);
            // clone might be NULL after this point
        }
    }
}


