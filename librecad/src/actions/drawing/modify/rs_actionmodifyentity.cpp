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

#include "lc_dlgdimension.h"
#include "lc_entitypropertieseditor.h"
#include "lc_quickinfowidget.h"
#include "qc_applicationwindow.h"
#include "qg_dlghatch.h"
#include "qg_dlgmtext.h"
#include "qg_dlgtext.h"
#include "rs_debug.h"
#include "rs_dimension.h"
#include "rs_entity.h"
#include "rs_eventhandler.h"
#include "rs_graphicview.h"
#include "rs_text.h"

RS_ActionModifyEntity::RS_ActionModifyEntity(LC_ActionContext *actionContext)
		:RS_PreviewActionInterface("Modify Entity", actionContext, RS2::ActionModifyEntity){
}

RS_ActionModifyEntity::~RS_ActionModifyEntity() {
    delete m_propertiesEditor;
}

void RS_ActionModifyEntity::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    m_entity = contextEntity;
    m_invokedForSingleEntity = true;
    m_modifyCursor = false;
}

void RS_ActionModifyEntity::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (m_entity != nullptr) {
        trigger();
    }
}

void  RS_ActionModifyEntity::notifyFinished() {
    m_graphicView->notifyLastActionFinished();
}

void RS_ActionModifyEntity::onLateRequestCompleted(bool shouldBeSkipped) {
    if (shouldBeSkipped) {
        if (m_invokedForSingleEntity) {
            m_entity = nullptr;
            delete m_clonedEntity;
            finish(false);
        }
        else {
            setStatus(ShowDialog);
        }
    }
    else {
        setStatus(EditComplete);
        completeEditing();
        if (m_invokedForSingleEntity) {
            finish(false);
        }
        else {
            setStatus(ShowDialog);
        }
    }
    m_graphicView->redraw(RS2::RedrawDrawing);
    m_allowExternalTermination = true;
}

void RS_ActionModifyEntity::completeEditing() {
    m_clonedEntity->update();

    m_entity->setSelected(false);
    m_clonedEntity->setSelected(false);

    m_clonedEntity->calculateBorders();
    m_container->addEntity(m_clonedEntity);

    m_entity->setSelected(false);
    m_clonedEntity->setSelected(false);

    unsigned long cloneEntityId = m_clonedEntity->getId();
    unsigned long originalEntityId = m_entity->getId();

    if (m_document != nullptr) {
        undoCycleReplace(m_entity, m_clonedEntity);
    }

    // hm... probably there is a better way to notify (signal, broadcasting etc) without direct dependency?
    LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
    if (entityInfoWidget != nullptr){
        entityInfoWidget->onEntityPropertiesEdited(originalEntityId, cloneEntityId);
    }
}

void RS_ActionModifyEntity::doTrigger() {
    int status = getStatus();
    if (status == ShowDialog) {
        if (m_entity != nullptr) {
            bool selected = m_entity->isSelected();
            // RAII style: restore the highlighted status
            /*std::shared_ptr<bool> scopedFlag(&selected, [this](bool* pointer) {
                if (pointer != nullptr && m_entity->isSelected() != *pointer) {
                    setDisplaySelected(*pointer);
                }});*/
            // Always show the entity being edited as "Selected"
            setDisplaySelected(true);

            setStatus(InEditing);

            LC_EntityPropertiesDlg* editDialog {nullptr};
            QWidget* parent = QC_ApplicationWindow::getAppWindow().get();
            m_clonedEntity = m_entity->clone();
            bool hasDialog = true;
            switch (m_entity->rtti()) {
                case RS2::EntityPoint:
                case RS2::EntityLine:
                case RS2::EntityArc:
                case RS2::EntityCircle:
                case RS2::EntityEllipse:
                case RS2::EntityHyperbola:
                case RS2::EntityParabola:
                case RS2::EntitySpline:
                case RS2::EntitySplinePoints:
                case RS2::EntityInsert:
                case RS2::EntityPolyline:
                case RS2::EntityImage: {
                    // editing via delayed invocation in editor to support interactive input
                    m_propertiesEditor = new LC_EntityPropertiesEditor(m_actionContext, this);
                    m_allowExternalTermination = false;
                    m_propertiesEditor->editEntity(parent, m_clonedEntity, m_viewport);
                    hasDialog = false;
                    break;
                }
                case RS2::EntityDimAligned:
                case RS2::EntityDimAngular:
                case RS2::EntityDimDiametric:
                case RS2::EntityDimRadial:
                case RS2::EntityDimArc:
                case RS2::EntityDimOrdinate:
                case RS2::EntityDimLinear: {
                    editDialog = new LC_DlgDimension(parent, m_viewport, static_cast<RS_Dimension*>(m_clonedEntity));
                    break;
                }
                case RS2::EntityMText: {
                    editDialog = new QG_DlgMText(parent, m_viewport, static_cast<RS_MText*>(m_clonedEntity), false);
                    break;
                }
                case RS2::EntityText: {
                    editDialog = new QG_DlgText(parent, m_viewport, static_cast<RS_Text*>(m_clonedEntity), false);
                    break;
                }
                case RS2::EntityHatch: {
                    editDialog = new QG_DlgHatch(parent, m_viewport, static_cast<RS_Hatch*>(m_clonedEntity), false);
                    break;
                }
                default:
                    hasDialog = false;
                    break;
            }

            if (hasDialog) {
                m_allowExternalTermination = false;
                if (editDialog->exec()) {
                    editDialog->updateEntity();
                    m_container->addEntity(m_clonedEntity);

                    if (m_document != nullptr) {
                        undoCycleReplace(m_entity, m_clonedEntity);
                    }
                    if (!selected) {
                        m_clonedEntity->setSelected(false);
                    }
                }
                else {
                    if (!selected) {
                        m_entity->setSelected(false);
                    }
                    delete m_clonedEntity;
                }
                m_allowExternalTermination = true;
                m_graphicView->redraw(RS2::RedrawDrawing);
                setStatus(ShowDialog);
                delete editDialog;
            }
        }
        else {
            RS_DEBUG->print("RS_ActionModifyEntity::trigger: Entity is NULL\n");
        }
    }
    else if (status == InEditing) {
        completeEditing();
        setStatus(EditComplete);
    }
}

void RS_ActionModifyEntity::setDisplaySelected(bool highlighted){
    if (m_entity != nullptr) {
        m_entity->setSelected(highlighted);
    }
}

void RS_ActionModifyEntity::onMouseMoveEvent([[maybe_unused]]int status, LC_MouseEvent *e) {
    if (status == ShowDialog) {
        RS_Entity* entity = catchAndDescribe(e);
        if (entity != nullptr){
            highlightHoverWithRefPoints(entity, true);
        }
    }
}

void RS_ActionModifyEntity::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    if (status == ShowDialog) {
        m_entity = catchEntityByEvent(e);
        if (m_entity != nullptr) {
            trigger();
        }
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
