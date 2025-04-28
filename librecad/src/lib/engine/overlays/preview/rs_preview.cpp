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

#include "lc_graphicviewport.h"
#include "rs_color.h"
#include "rs_line.h"
#include "rs_pen.h"
#include "rs_settings.h"

/**
 * Constructor.
 */
RS_Preview::RS_Preview(RS_EntityContainer* parent, LC_GraphicViewport* viewport)
        : RS_EntityContainer(parent, true), m_viewport{viewport}{

// fixme - sand - ucs - check when preview is created and whether this may be delegated to actio init?

    m_maxEntities = LC_GET_ONE_INT("Appearance", "MaxPreview", 100);
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
        //        case RS2::EntityImage:
        case RS2::EntityHatch:
            addBorder = true;
            break;
        /*case RS2::EntityInsert: {
            addBorder = true;
            break;
        }*/
        case RS2::EntityRefPoint:
        case RS2::EntityRefLine:
        case RS2::EntityRefConstructionLine:
        case RS2::EntityRefCircle:
        case RS2::EntityRefEllipse:
        case RS2::EntityRefArc: {
            refEntity = true;
            break;
        }
        // todo - sand - hm... spline and texts are not included into limit of preview entities?
        case RS2::EntitySpline:
            break;
        case RS2::EntityMText:
            break;
        case RS2::EntityText:
            break;
        case RS2::EntityPoint:
            break;
        default: {
            if (entity->isContainer()) {
                if (entity->countDeep() > m_maxEntities - countDeep()) {
                    addBorder = true;
                }
            }
        }
    }

    if (addBorder) {

        RS_Line *l1, *l2, *l3, *l4;

        RS_Vector min = entity->getMin();
        RS_Vector max = entity->getMax();

        if (m_viewport->hasUCS()) {
            RS_Vector c2, c4;
            calcRectCorners(min, max, c2, c4);
            l1 = new RS_Line(this, {min.x, min.y}, {c2.x, c2.y});
            l2 = new RS_Line(this, {c2.x, c2.y}, {max.x, max.y});
            l3 = new RS_Line(this, {max.x, max.y}, {c4.x, c4.y});
            l4 = new RS_Line(this, {c4.x, c4.y}, {min.x, min.y});
        }
        else {
            l1 = new RS_Line(this, {min.x, min.y}, {max.x, min.y});
            l2 = new RS_Line(this, {max.x, min.y}, {max.x, max.y});
            l3 = new RS_Line(this, {max.x, max.y}, {min.x, max.y});
            l4 = new RS_Line(this, {min.x, max.y}, {min.x, min.y});
        }

        RS_EntityContainer::addEntity(l1);
        RS_EntityContainer::addEntity(l2);
        RS_EntityContainer::addEntity(l3);
        RS_EntityContainer::addEntity(l4);

        delete entity;
    }
    else {
        entity->setLayer(nullptr);
        entity->setSelected(false);
        entity->reparent(this);
        // Don't set this pen, let drawing routines decide entity->setPenToActive();
        if (refEntity) {
            m_referenceEntities.append(entity);
            if (getAutoUpdateBorders()) {
                adjustBorders(entity);
            }
        }
        else {
            RS_EntityContainer::addEntity(entity);
        }
    }
}

void RS_Preview::calcRectCorners(const RS_Vector &worldCorner1, const RS_Vector &worldCorner3, RS_Vector &worldCorner2, RS_Vector &worldCorner4) const {
    RS_Vector ucsCorner1 = m_viewport->toUCS(worldCorner1);
    RS_Vector ucsCorner3 = m_viewport->toUCS(worldCorner3);
    RS_Vector ucsCorner2 = RS_Vector(ucsCorner1.x, ucsCorner3.y);
    RS_Vector ucsCorner4 = RS_Vector(ucsCorner3.x, ucsCorner1.y);
    worldCorner2 =  m_viewport->toWorld(ucsCorner2);
    worldCorner4 =  m_viewport->toWorld(ucsCorner4);
}

void RS_Preview::clear() {
    if (isOwner()) {
        while (!m_referenceEntities.isEmpty()) {
            delete m_referenceEntities.takeFirst();
        }
    } else {
        m_referenceEntities.clear();
    }
    RS_EntityContainer::clear();
}

/**
 * Clones the given entity and adds the clone to the preview.
 */
void RS_Preview::addCloneOf(RS_Entity* entity, [[maybe_unused]]LC_GraphicViewport* view) {
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
void RS_Preview::addAllFrom(RS_EntityContainer& container, [[maybe_unused]]LC_GraphicViewport* view) {
    unsigned int c=0;
    for(auto e: container){
        if (c < m_maxEntities) {
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
 * Adds all selected entities from 'container' to the preview (unselected).
 */
void RS_Preview::addSelectionFrom(RS_EntityContainer& container, [[maybe_unused]]LC_GraphicViewport* view) {
    unsigned int c=0;
    for(auto e: container){ // fixme - sand - wow - iterating over all entities!!! Rework selection
        if (e->isSelected() && c<m_maxEntities) {
            RS_Entity* clone = e->cloneProxy();

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
void RS_Preview::addStretchablesFrom(RS_EntityContainer& container, [[maybe_unused]]LC_GraphicViewport* view,
                                     const RS_Vector& v1, const RS_Vector& v2) {
    unsigned int c=0;

    for (auto e: container) {
        if (e->isVisible() && e->rtti() != RS2::EntityHatch &&
            ((e->isInWindow(v1, v2)) || e->hasEndpointsWithinWindow(v1, v2)) &&
            c < m_maxEntities) {

            RS_Entity *clone = e->cloneProxy();
            //clone->setSelected(false);
            clone->reparent(this);

            c += clone->countDeep();
            addEntity(clone);
            // clone might be nullptr after this point
        }
    }
}

void RS_Preview::draw(RS_Painter* painter) {
//    bool drawTextsAsDraftsForPreview = view->isDrawTextsAsDraftForPreview();
// fixme - ucs - achieve view - store as field? This temporary for compilation...
    bool drawTextsAsDraftsForPreview = false;

    for (auto e: std::as_const(*this)) {
        int type = e->rtti();
        switch (type) {
            case RS2::EntityMText:
            case RS2::EntityText: {
                if (drawTextsAsDraftsForPreview){
                    e->drawDraft(painter);
                }
                else {
                    e->draw(painter);
                }
                break;
            }
            case RS2::EntityImage: {
                e->drawDraft(painter);
                break;
            }
            default:
                e->draw(painter);
        }
    }
}

void RS_Preview::addReferenceEntitiesToContainer(RS_EntityContainer *container){
    for (auto en: std::as_const(m_referenceEntities)){
        container->addEntity(en);
    }
}

int RS_Preview::getMaxAllowedEntities() {
    return m_maxEntities;
}
