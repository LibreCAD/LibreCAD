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

#include "rs_actionmodifyentity.h"

#include "lc_quickinfowidget.h"
#include "qc_applicationwindow.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_entity.h"
#include "rs_graphicview.h"
#
RS_ActionModifyEntity::RS_ActionModifyEntity(LC_ActionContext *actionContext, RS_Entity *entity)
		:RS_PreviewActionInterface("Modify Entity", actionContext, RS2::ActionModifyEntity)
		,m_entity(entity){
   m_modifyCursor = entity == nullptr;
}

void RS_ActionModifyEntity::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (m_entity != nullptr) {
        trigger();
        finish(false);
    }
}

void RS_ActionModifyEntity::doTrigger() {
    if (m_entity != nullptr) {
        std::unique_ptr<RS_Entity> clone{m_entity->clone()};
        bool selected = m_entity->isSelected();
        // RAII style: restore the highlighted status
        std::shared_ptr<bool> scopedFlag(&selected, [this](bool* pointer) {
            if (pointer != nullptr && m_entity->isSelected() != *pointer) {
                setDisplaySelected(*pointer);
            }});
        // Always show the entity being edited as "Selected"
        setDisplaySelected(true);

        unsigned long originalEntityId = m_entity->getId();

        m_graphicView->setForcedActionKillAllowed(false);
        if (RS_DIALOGFACTORY->requestModifyEntityDialog(clone.get(), m_viewport)) {
            m_container->addEntity(clone.get());

            m_entity->setSelected(false);
            clone->setSelected(false);

            if (m_document) {
                undoCycleReplace(m_entity, clone.get());
            }

            unsigned long cloneEntityId = clone->getId();

            // hm... probably there is a better way to notify (signal, broadcasting etc) without direct dependency?
            LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
            if (entityInfoWidget != nullptr){
                entityInfoWidget->onEntityPropertiesEdited(originalEntityId, cloneEntityId);
            }

            clone.release();
        }
        m_graphicView->setForcedActionKillAllowed(true);
    } else {
        RS_DEBUG->print("RS_ActionModifyEntity::trigger: Entity is NULL\n");
    }
}

void RS_ActionModifyEntity::setDisplaySelected(bool highlighted){
    if (m_entity != nullptr) {
        m_entity->setSelected(highlighted);
    }
}

void RS_ActionModifyEntity::onMouseMoveEvent([[maybe_unused]]int status, LC_MouseEvent *e) {
    RS_Entity* entity = catchAndDescribe(e);
    if (entity != nullptr){
        highlightHoverWithRefPoints(entity, true);
    }
}

void RS_ActionModifyEntity::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    m_entity = catchEntityByEvent(e);
    if (m_entity != nullptr) {
        trigger();
    }
}

void RS_ActionModifyEntity::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    initPrevious(status);
}

RS2::CursorType RS_ActionModifyEntity::doGetMouseCursor([[maybe_unused]] int status){
    if (m_modifyCursor) {
        return RS2::SelectCursor;
    }
    else{
        return RS2::NoCursorChange;
    }
}

void RS_ActionModifyEntity::updateMouseButtonHints() {
    updateMouseWidgetTRCancel(tr("Click on entity to modify"));
}
