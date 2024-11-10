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
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "settings/rs_settings.h"

/**
 * Constructor.
 */
RS_Preview::RS_Preview(RS_EntityContainer* parent)
        : RS_EntityContainer(parent, true){

    maxEntities = LC_GET_ONE_INT("Appearance", "MaxPreview", 100);
    RS_Color highLight = QColor(LC_GET_ONE_STR("Colors", "highlight", RS_Settings::highlight));
    setPen(RS_Pen(highLight, RS2::Width00, RS2::SolidLine));
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

    int rtti = entity->rtti();

    bool addBorder = false;
    bool refEntity = false;

    switch (rtti) {
        case RS2::EntityImage:
        case RS2::EntityHatch:
        case RS2::EntityInsert:
            addBorder = true;
            break;
        case RS2::EntityRefPoint:
        case RS2::EntityRefLine:
        case RS2::EntityRefCircle:
        case RS2::EntityRefEllipse:
        case RS2::EntityRefArc: {
            refEntity = true;
            break;
        }
        case RS2::EntitySpline:
            break;
        case RS2::EntityMText:
            break;
        case RS2::EntityText:
            break;
        default: {
            if (entity->isContainer()) {
                if (entity->countDeep() > maxEntities-countDeep()) {
                    addBorder = true;
                }
            }
        }
    }

    if (addBorder){
        RS_Vector min = entity->getMin();
        RS_Vector max = entity->getMax();
        auto l1 = new RS_Line(this, {min.x, min.y}, {max.x, min.y});
        auto *l2 = new RS_Line(this, {max.x, min.y}, {max.x, max.y});
        auto *l3 = new RS_Line(this, {max.x, max.y}, {min.x, max.y});
        auto *l4 = new RS_Line(this, {min.x, max.y}, {min.x, min.y});

        RS_EntityContainer::addEntity(l1);
        RS_EntityContainer::addEntity(l2);
        RS_EntityContainer::addEntity(l3);
        RS_EntityContainer::addEntity(l4);

        delete entity;
    } else {
        entity->setLayer(nullptr);
        entity->setSelected(false);
        entity->reparent(this);
       // Don't set this pen, let drawing routines decide entity->setPenToActive();
       if (refEntity){
           referenceEntities.append(entity);
           if (autoUpdateBorders) {
               adjustBorders(entity);
           }
       }
       else{
           RS_EntityContainer::addEntity(entity);
       }
    }
}

void RS_Preview::clear() {
    if (isOwner()) {
        while (!referenceEntities.isEmpty()) {
            delete referenceEntities.takeFirst();
        }
    } else {
        referenceEntities.clear();
    }
    RS_EntityContainer::clear();
}

/**
 * Clones the given entity and adds the clone to the preview.
 */
void RS_Preview::addCloneOf(RS_Entity* entity) {
    if (!entity) {
        return;
    }

    RS_Entity* clone = entity->cloneProxy();
    clone->reparent(this);
    addEntity(clone);
}

/**
 * Adds all entities from 'container' to the preview (unselected).
 */
void RS_Preview::addAllFrom(RS_EntityContainer& container) {
    unsigned int c=0;
    for(auto e: container){
        if (c < maxEntities) {
            RS_Entity* clone = e->cloneProxy();
            clone->setSelected(false);
            clone->reparent(this);

            c+=clone->countDeep();
            addEntity(clone);
            // clone might be nullptr after this point
        }
    }
}
// fixme - sand - use cloneProxy()
/**
 * Adds all selected entities from 'container' to the preview (unselected).
 */
void RS_Preview::addSelectionFrom(RS_EntityContainer& container) {
    unsigned int c=0;
    for(auto e: container){ // fixme - sand - wow - iterating over all entities!!! Rework selection
        if (e->isSelected() && c<maxEntities) {
            RS_Entity* clone = e->cloneProxy();
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
    unsigned int c=0;

    for (auto e: container) {
        if (e->isVisible() && e->rtti() != RS2::EntityHatch &&
            ((e->isInWindow(v1, v2)) || e->hasEndpointsWithinWindow(v1, v2)) &&
            c < maxEntities) {

            RS_Entity *clone = e->cloneProxy();
            //clone->setSelected(false);
            clone->reparent(this);

            c += clone->countDeep();
            addEntity(clone);
            // clone might be nullptr after this point
        }
    }
}

void RS_Preview::draw(RS_Painter* painter, RS_GraphicView* view,
                              double& patternOffset) {
    for (auto e: std::as_const(entities)){
        e->drawDraft(painter, view, patternOffset);
    }
}

void RS_Preview::addReferenceEntitiesToContainer(RS_EntityContainer *container){
    for (auto en: std::as_const(referenceEntities)){
        container->addEntity(en);
    }
}
