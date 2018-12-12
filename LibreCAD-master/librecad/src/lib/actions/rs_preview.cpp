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

#include "rs_preview.h"
#include "rs_entitycontainer.h"
#include "rs_line.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_settings.h"

/**
 * Constructor.
 */
RS_Preview::RS_Preview(RS_EntityContainer* parent)
		: RS_EntityContainer(parent, true) {

    RS_SETTINGS->beginGroup("/Appearance");
    maxEntities = RS_SETTINGS->readNumEntry("/MaxPreview", 100);
    RS_SETTINGS->endGroup();
}

/**
 * Adds an entity to this preview and removes any attributes / layer
 * connections before that.
 */
void RS_Preview::addEntity(RS_Entity* entity) {
	if (!entity || entity->isUndone()) {
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
			new RS_Line(this, {min.x, min.y}, {max.x, min.y});
        RS_Line* l2 =
			new RS_Line(this, {max.x, min.y}, {max.x, max.y});
        RS_Line* l3 =
			new RS_Line(this, {max.x, max.y}, {min.x, max.y});
        RS_Line* l4 =
			new RS_Line(this, {min.x, max.y}, {min.x, min.y});

        RS_EntityContainer::addEntity(l1);
        RS_EntityContainer::addEntity(l2);
        RS_EntityContainer::addEntity(l3);
        RS_EntityContainer::addEntity(l4);

        delete entity;
        entity = nullptr;
    } else {
        entity->setLayer(nullptr);
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
    if (!entity) {
        return;
    }

    RS_Entity* clone = entity->clone();
    clone->reparent(this);
    addEntity(clone);
}

/**
 * Adds all entities from 'container' to the preview (unselected).
 */
void RS_Preview::addAllFrom(RS_EntityContainer& container) {
	int c=0;
	for(auto e: container){

        if (c<maxEntities) {
            RS_Entity* clone = e->clone();
            clone->setSelected(false);
            clone->reparent(this);

            c+=clone->countDeep();
            addEntity(clone);
            // clone might be nullptr after this point
        }
    }
}

/**
 * Adds all selected entities from 'container' to the preview (unselected).
 */
void RS_Preview::addSelectionFrom(RS_EntityContainer& container) {
	int c=0;
	for(auto e: container){

        if (e->isSelected() && c<maxEntities) {
            RS_Entity* clone = e->clone();
            clone->setSelected(false);
            clone->reparent(this);

            c+=clone->countDeep();
            addEntity(clone);
            // clone might be nullptr after this point
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

	for(auto e: container){

        if (e->isVisible() &&
                e->rtti()!=RS2::EntityHatch &&
                ((e->isInWindow(v1, v2)) ||
                 e->hasEndpointsWithinWindow(v1, v2)) &&

                c<maxEntities) {

            RS_Entity* clone = e->clone();
            //clone->setSelected(false);
            clone->reparent(this);

            c+=clone->countDeep();
            addEntity(clone);
            // clone might be nullptr after this point
        }
    }
}

void RS_Preview::draw(RS_Painter* painter, RS_GraphicView* view,
                              double& patternOffset) {

    if (!(painter && view)) {
        return;
    }

    foreach (auto e, entities)
    {
        e->draw(painter, view, patternOffset);
    }
}
