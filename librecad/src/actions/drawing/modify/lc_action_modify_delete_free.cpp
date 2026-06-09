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

#include "lc_action_modify_delete_free.h"

#include "rs_document.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_vector.h"

struct LC_ActionModifyDeleteFree::ActionData {
    RS_Vector v1;
    RS_Vector v2;
};

LC_ActionModifyDeleteFree::LC_ActionModifyDeleteFree(LC_ActionContext* actionContext)
    : RS_ActionInterface("ActionModifyDeleteFree", actionContext, RS2::ActionModifyDeleteFree),
      m_actionData(std::make_unique<ActionData>()) {
    LC_ActionModifyDeleteFree::init(0);
}

LC_ActionModifyDeleteFree::~LC_ActionModifyDeleteFree() = default;

void LC_ActionModifyDeleteFree::init(const int status) {
    RS_ActionInterface::init(status);
    m_polyline = nullptr;
    m_entity1 = nullptr;
    m_entity2 = nullptr;
    m_actionData.reset(new ActionData{});
    RS_SnapMode* s = getSnapMode();
    s->snapOnEntity = true;
}

void LC_ActionModifyDeleteFree::trigger() {
    if (m_entity1 != nullptr && m_entity2 != nullptr) {
        const RS_EntityContainer* parent = m_entity2->getParent();
        if (parent != nullptr) {
            if (parent->rtti() == RS2::EntityPolyline) {
                if (parent->getId() == m_polyline->getId()) {
                    // splits up the polyline in the container:
                    RS_Polyline* pl1 = nullptr;
                    RS_Polyline* pl2 = nullptr;

                    // FIXME - TEMPORARAY TO COMPILE!!! Add property cut for polyline!
                    LC_DocumentModificationBatch ctx;
                    RS_Modification::splitPolyline(m_polyline, *m_entity1, m_actionData->v1, *m_entity2, m_actionData->v2, &pl1, &pl2, ctx);

                    // draws the new polylines on the screen:
                    redraw(RS2::RedrawDrawing);

                    init(0);
                }
                else {
                    commandMessage(tr("Entities not in the same polyline."));
                }
            }
            else {
                commandMessage(tr("Parent of second entity is not a polyline"));
            }
        }
        else {
            commandMessage(tr("Parent of second entity is nullptr"));
        }
    }
    else {
        commandMessage(tr("One of the chosen entities is nullptr"));
    }
}

// fixme - add constants for statuses
void LC_ActionModifyDeleteFree::onMouseLeftButtonRelease(const int status, QMouseEvent* e) {
    switch (status) {
        case 0: {
            m_actionData->v1 = snapPoint(e);
            m_entity1 = getKeyEntity();
            if (m_entity1 != nullptr) {
                RS_EntityContainer* parent = m_entity1->getParent();
                if (parent != nullptr) {
                    if (parent->rtti() == RS2::EntityPolyline) {
                        m_polyline = dynamic_cast<RS_Polyline*>(parent);
                        setStatus(1);
                    }
                    else {
                        commandMessage(tr("Parent of first entity is not a polyline"));
                    }
                }
                else {
                    commandMessage(tr("Parent of first entity is nullptr"));
                }
            }
            else {
                commandMessage(tr("First entity is nullptr"));
            }
            break;
        }
        case 1: {
            m_actionData->v2 = snapPoint(e);
            m_entity2 = getKeyEntity();

            if (m_entity2 != nullptr) {
                trigger();
            }
            else {
                commandMessage(tr("Second entity is nullptr"));
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyDeleteFree::onMouseRightButtonRelease(const int status, [[maybe_unused]] QMouseEvent* e) {
    initPrevious(status);
}

void LC_ActionModifyDeleteFree::updateActionPrompt() {
    switch (getStatus()) {
        case 0:
            updatePromptTRCancel(tr("Specify first break point on a polyline"));
            break;
        case 1:
            updatePromptTRBack(tr("Specify second break point on the same polyline"));
            break;
        default:
            updatePrompt();
            break;
    }
}
