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

#include <QMouseEvent>

#include "lc_quickinfowidget.h"
#include "qc_applicationwindow.h"
#include "rs_actionmodifyentity.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"

RS_ActionModifyEntity::RS_ActionModifyEntity(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
		:RS_PreviewActionInterface("Modify Entity", container, graphicView)
		,en(nullptr){
	actionType=RS2::ActionModifyEntity;
}

void RS_ActionModifyEntity::setDisplaySelected(bool highlighted){
    if (en != nullptr) {
        en->setSelected(highlighted);
        graphicView->drawEntity(en);
    }
}

void RS_ActionModifyEntity::trigger() {
    if (en != nullptr) {
        std::unique_ptr<RS_Entity> clone{en->clone()};
        bool selected = en->isSelected();
        // RAII style: restore the highlighted status
        std::shared_ptr<bool> scopedFlag(&selected, [this](bool* pointer) {
            if (pointer != nullptr && en->isSelected() != *pointer) {
                setDisplaySelected(*pointer);
            }});
        // Always show the entity being edited as "Selected"
        setDisplaySelected(true);

        unsigned long originalEntityId = en->getId();

        if (RS_DIALOGFACTORY->requestModifyEntityDialog(clone.get())) {
            container->addEntity(clone.get());

            en->setSelected(false);

            clone->setSelected(false);
            graphicView->drawEntity(clone.get());

            if (document) {
                document->startUndoCycle();
                document->addUndoable(clone.get());
                deleteEntityUndoable(en);
                document->endUndoCycle();
            }

            unsigned long cloneEntityId = clone->getId();

            // hm... probably there is a better way to notify (signal, broadcasting etc) without direct dependency?
            LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
            if (entityInfoWidget != nullptr){
                entityInfoWidget->onEntityPropertiesEdited(originalEntityId, cloneEntityId);
            }

            clone.release();
            updateSelectionWidget();
        }
    } else {
        RS_DEBUG->print("RS_ActionModifyEntity::trigger: Entity is NULL\n");
    }
}

void RS_ActionModifyEntity::mouseMoveEvent(QMouseEvent *e) {
    RS_Entity* entity = catchEntity(e);
    deleteHighlights();
    if (entity != nullptr){
        highlightHoverWithRefPoints(entity, true);
    }
    drawHighlights();
}

void RS_ActionModifyEntity::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    en = catchEntity(e);
    if (en != nullptr) {
        trigger();
    }
}

void RS_ActionModifyEntity::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    initPrevious(status);
}

RS2::CursorType RS_ActionModifyEntity::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void RS_ActionModifyEntity::updateMouseButtonHints() {
    updateMouseWidgetTRCancel(tr("Click on entity to modify"));
}
