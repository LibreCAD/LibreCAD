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

#include "lc_action_modify_entity.h"

#include "lc_dlg_dimension.h"
#include "lc_entitypropertieseditor.h"
#include "lc_quickinfowidget.h"
#include "qc_applicationwindow.h"
#include "qg_dlg_hatch.h"
#include "qg_dlg_mtext.h"
#include "qg_dlg_text.h"
#include "rs_debug.h"
#include "rs_dimension.h"
#include "rs_entity.h"
#include "rs_graphicview.h"
#include "rs_text.h"

LC_ActionModifyEntity::LC_ActionModifyEntity(LC_ActionContext* actionContext)
    : LC_UndoableDocumentModificationAction("ActionModifyEntity", actionContext, RS2::ActionModifyEntity) {
}

LC_ActionModifyEntity::~LC_ActionModifyEntity() {
    delete m_propertiesEditor;
}

void LC_ActionModifyEntity::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPos) {
    m_entity = contextEntity;
    m_invokedForSingleEntity = true;
    m_modifyCursor = false;
}

void LC_ActionModifyEntity::init(const int status) {
    RS_PreviewActionInterface::init(status);
    if (m_entity != nullptr) {
        trigger();
    }
}

void LC_ActionModifyEntity::notifyFinished() const {
    m_graphicView->notifyLastActionFinished();
}

void LC_ActionModifyEntity::onLateRequestCompleted(const bool shouldBeSkipped) {
    if (shouldBeSkipped) {
        if (m_invokedForSingleEntity) {
            m_entity = nullptr;
            delete m_clonedEntity;
            finish();
        }
        else {
            setStatus(ShowDialog);
        }
    }
    else {
        trigger();
        if (m_invokedForSingleEntity) {
            finish();
        }
        else {
            setStatus(ShowDialog);
        }
    }
    m_graphicView->redraw(RS2::RedrawDrawing);
    m_allowExternalTermination = true;
}

bool LC_ActionModifyEntity::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    const int status = getStatus();
    if (status == ShowDialog) {
        if (m_entity != nullptr) {
            const bool selected = m_entity->isSelected();
            select(m_entity);
            setStatus(InEditing);
            LC_EntityPropertiesDlg* editDialog{nullptr};
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
                if (editDialog->exec() != 0) {
                    editDialog->updateEntity();
                    ctx.replace(m_entity, m_clonedEntity);
                    if (!selected) {
                        m_clonedEntity->clearSelectionFlag();
                    }
                }
                else {
                    if (!selected) {
                        unselect(m_entity);
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
        m_clonedEntity->update();
        m_clonedEntity->calculateBorders();
        m_clonedEntity->clearSelectionFlag();

        const unsigned long cloneEntityId = m_clonedEntity->getId();
        const unsigned long originalEntityId = m_entity->getId();

        ctx.replace(m_entity, m_clonedEntity);

        // hm... probably there is a better way to notify (signal, broadcasting etc) without direct dependency?
        LC_QuickInfoWidget* entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
        if (entityInfoWidget != nullptr) {
            entityInfoWidget->onEntityPropertiesEdited(originalEntityId, cloneEntityId);
        }
        setStatus(EditComplete);
    }
    ctx.dontSetActiveLayerAndPen();
    return true;
}

void LC_ActionModifyEntity::doTriggerCompletion(const bool success) {
    LC_UndoableDocumentModificationAction::doTriggerCompletion(success);
}

void LC_ActionModifyEntity::onMouseMoveEvent([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    if (status == ShowDialog) {
        const RS_Entity* entity = catchAndDescribe(e);
        if (entity != nullptr) {
            highlightHoverWithRefPoints(entity, true);
        }
    }
}

void LC_ActionModifyEntity::onMouseLeftButtonRelease([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    if (status == ShowDialog) {
        m_entity = catchEntityByEvent(e);
        if (m_entity != nullptr) {
            trigger();
        }
    }
}

void LC_ActionModifyEntity::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    initPrevious(status);
}

RS2::CursorType LC_ActionModifyEntity::doGetMouseCursor([[maybe_unused]] int status) {
    if (m_modifyCursor) {
        return RS2::SelectCursor;
    }
    return RS2::NoCursorChange;
}

void LC_ActionModifyEntity::updateActionPrompt() {
    updatePromptTRCancel(tr("Click on entity to modify"));
}
