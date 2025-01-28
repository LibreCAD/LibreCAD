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


#include "qc_applicationwindow.h"

#include "qg_dialogfactory.h"

#include "rs_block.h"
#include "rs_dialogfactory.h"
#include "rs_entity.h"
#include "rs_information.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_selection.h"
#include "lc_graphicviewport.h"

/**
 * Default constructor.
 *
 * @param container The container to which we will add
 *        entities. Usually that's an RS_Graphic entity but
 *        it can also be a polyline, text, ...
 */
RS_Selection::RS_Selection(
    RS_EntityContainer &container,
    LC_GraphicViewport *graphicView):
    container{&container}, graphic{container.getGraphic()}, graphicView{graphicView}{
}

/**
 * Selects or deselects the given entity.
 */
void RS_Selection::selectSingle(RS_Entity *e){
    if (e && (!(e->getLayer() && e->getLayer()->isLocked()))){

        e->toggleSelected();

        if (graphicView){
            if (e->isSelected() && (e->rtti() == RS2::EntityInsert)){
                const RS_Block *selectedBlock = dynamic_cast<RS_Insert *>(e)->getBlockForInsert();

                if (selectedBlock != nullptr){
                    // Display the selected block as active in the block widget
                    QC_ApplicationWindow::getAppWindow()->showBlockActivated(selectedBlock);
                    // Display the selected block name
                    QG_DIALOGFACTORY->displayBlockName(selectedBlock->getName(), true);
                }
            } else {
                QG_DIALOGFACTORY->displayBlockName("", false);
            }
            graphicView->notifyChanged();
        }
    }
}

/**
 * Selects all entities on visible layers.
 */
void RS_Selection::selectAll(bool select){
    if (graphicView){
        for (auto e: *container) {
            if (e && e->isVisible()){
                e->setSelected(select);
                // fixme - sand - selectAll by entity type - check whether it will not break plugin interface:
                // NOTE:
                // this is actually bad practice and development by side-effect.
                // type to select for graphic view is set only in  Doc_plugin_interface::getSelectByType
                // and in general it's hardly that this flag is propertly used (as later
                // RS_ActionSelectSingle with own check is invoked.
                // So better just create separate function with explicit type of entity, if one will be really
                // necessary.
               /* if (graphicView->getTypeToSelect() == RS2::EntityType::EntityUnknown){
                    e->setSelected(select);
                } else {
                    if (e->rtti() == graphicView->getTypeToSelect()){
                        e->setSelected(select);
                    }
                }*/
            }
        }
        graphicView->notifyChanged();
    }
}

/**
 * Selects all entities on visible layers.
 */
void RS_Selection::invertSelection(){
    for (auto e: *container) {
        if (e && e->isVisible()){
            e->toggleSelected();
        }
    }

    graphicView->notifyChanged();
}

/**
 * Selects all entities that are completely in the given window.
 *
 * @param v1 First corner of the window to select.
 * @param v2 Second corner of the window to select.
 * @param select true: select, false: invertSelectionOperation
 */
void RS_Selection::selectWindow(
    enum RS2::EntityType typeToSelect, const RS_Vector &v1, const RS_Vector &v2,
    bool select, bool cross){
    container->selectWindow(typeToSelect, v1, v2, select, cross);
    graphicView->notifyChanged();
}

void RS_Selection::selectWindow(const QList<RS2::EntityType> &typesToSelect, const RS_Vector &v1, const RS_Vector &v2,
    bool select, bool cross){

    container->selectWindow(typesToSelect, v1, v2, select, cross);
    graphicView->notifyChanged();
}

/**
 * Selects all entities that are intersected by the given line.
 *
 * @param v1 Startpoint of line.
 * @param v2 Endpoint of line.
 * @param select true: select, false: invertSelectionOperation
 */
void RS_Selection::selectIntersected(const RS_Vector &v1, const RS_Vector &v2, bool select){
    RS_Line line{v1, v2};
    bool inters;

    for (auto e: *container) { // fixme - iteration over ALL entities, limit area
        if (e && e->isVisible()){
            inters = false;

            // select containers / groups:
            if (e->isContainer()){
                auto *ec = (RS_EntityContainer *) e;

                for (RS_Entity *e2 = ec->firstEntity(RS2::ResolveAll); e2;
                     e2 = ec->nextEntity(RS2::ResolveAll)) {

                    RS_VectorSolutions sol =
                        RS_Information::getIntersection(&line, e2, true);

                    if (sol.hasValid()){
                        inters = true;
                    }
                }
            } else {

                RS_VectorSolutions sol =
                    RS_Information::getIntersection(&line, e, true);

                if (sol.hasValid()){
                    inters = true;
                }
            }

            if (inters){
                e->setSelected(select);
            }
        }
    }
    graphicView->notifyChanged();
}

/**
 * Selects all entities that are connected to the given entity.
 *
 * @param e The entity where the algorithm starts. Must be an atomic entity.
 */
void RS_Selection::selectContour(RS_Entity *e){

    if (e == nullptr){
        return;
    }

    if (!e->isAtomic()){
        return;
    }

    bool select = !e->isSelected();
    auto *ae = (RS_AtomicEntity *) e;
    RS_Vector p1 = ae->getStartpoint();
    RS_Vector p2 = ae->getEndpoint();
    bool found = false;

    // (de)select 1st entity:
    e->setSelected(select);

    do {// fixme - hm...iterating over all entities of drawing in cycle???? too nice for me...
        found = false;
        // fixme - iterating over all entities of drawing
        for (auto en: *container) {

            if (en && en->isVisible() &&
                en->isAtomic() && en->isSelected() != select &&
                (!(en->getLayer() && en->getLayer()->isLocked()))){

                ae = (RS_AtomicEntity *) en;
                bool doit = false;

                // startpoint connects to 1st point
                if (ae->getStartpoint().distanceTo(p1) < 1.0e-4){
                    doit = true;
                    p1 = ae->getEndpoint();
                }

                    // endpoint connects to 1st point
                else if (ae->getEndpoint().distanceTo(p1) < 1.0e-4){
                    doit = true;
                    p1 = ae->getStartpoint();
                }

                    // startpoint connects to 2nd point
                else if (ae->getStartpoint().distanceTo(p2) < 1.0e-4){
                    doit = true;
                    p2 = ae->getEndpoint();
                }

                    // endpoint connects to 1st point
                else if (ae->getEndpoint().distanceTo(p2) < 1.0e-4){
                    doit = true;
                    p2 = ae->getStartpoint();
                }

                if (doit){
                    ae->setSelected(select);
                    found = true;
                }
            }
        }
    } while (found);
    graphicView->notifyChanged();
}

/**
 * Selects all entities on the given layer.
 */
void RS_Selection::selectLayer(RS_Entity *e){
    if (e == nullptr)
        return;

    bool select = !e->isSelected();
    RS_Layer *layer = e->getLayer(true);
    if (layer == nullptr)
        return;

    QString layerName = layer->getName();
    selectLayer(layerName, select);
}

/**
 * Selects all entities on the given layer.
 */
void RS_Selection::selectLayer(const QString &layerName, bool select){
    for (auto en: *container) {
        // fixme - review and make more efficient... why check for locking upfront? Why just not use layer pointers but names?
        if (en && en->isVisible() &&
            en->isSelected() != select &&
            (!(en->getLayer() && en->getLayer()->isLocked()))){

            RS_Layer *l = en->getLayer(true);

            if (l != nullptr && l->getName() == layerName){
                en->setSelected(select);
            }
        }
    }
    graphicView->notifyChanged();
}
