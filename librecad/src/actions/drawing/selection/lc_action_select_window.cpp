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
#include "lc_action_select_window.h"

#include <QMouseEvent>

#include "lc_cursoroverlayinfo.h"
#include "lc_graphicviewport.h"
#include "lc_selectwindowoptions.h"
#include "rs_selection.h"

struct LC_ActionSelectWindow::Points {
    RS_Vector v1;
    RS_Vector v2;
};

/**
 * Constructor.
 *
 * @param actionContext
 * @param select true: select window. false: invertSelectionOperation window
 */
LC_ActionSelectWindow::LC_ActionSelectWindow(LC_ActionContext* actionContext, const bool select)
    : RS_ActionSelectBase(select ? "ActionSelectWindow" : "ActionDeselectWindow", actionContext,
                          select ? RS2::ActionSelectWindow : RS2::ActionDeselectWindow), m_actionData(std::make_unique<Points>()),
      m_select(select) {
}

LC_ActionSelectWindow::LC_ActionSelectWindow(RS2::EntityType typeToSelect, LC_ActionContext* actionContext)
    : RS_ActionSelectBase("ActionSelectWindow" , actionContext,
                          RS2::ActionSelectWindow ), m_actionData(std::make_unique<Points>()),
      m_select(true) {
    if (typeToSelect == RS2::EntityUnknown) {
        setSelectAllEntityTypes(true);
    }
    else {
        m_entityTypesToSelect.append(typeToSelect);
        setSelectAllEntityTypes(false);
    }
}

LC_ActionSelectWindow::LC_ActionSelectWindow(const QString& name, LC_ActionContext* actionContext,
                                             const RS2::ActionType actionType, const RS2::EntityType typeToSelect, const bool select)
    : RS_ActionSelectBase(name, actionContext, actionType), m_actionData(std::make_unique<Points>()), m_select(select) {
    if (typeToSelect == RS2::EntityUnknown) {
        setSelectAllEntityTypes(true);
    }
    else {
        m_entityTypesToSelect.append(typeToSelect);
        setSelectAllEntityTypes(false);
    }
}

LC_ActionSelectWindow::~LC_ActionSelectWindow() = default;

void LC_ActionSelectWindow::doSaveOptions() {
    RS_ActionSelectBase::doSaveOptions(); // fixme - complete
}

void LC_ActionSelectWindow::doLoadOptions() {
    RS_ActionSelectBase::doLoadOptions(); // fixme - complete
}

void LC_ActionSelectWindow::init(const int status) {
    RS_PreviewActionInterface::init(status);
    m_actionData = std::make_unique<Points>();
    m_selectIntersecting = false;
    //snapMode.clear();
    //snapMode.restriction = RS2::RestrictNothing;
}

void LC_ActionSelectWindow::doTrigger() {
    if (m_actionData->v1.valid && m_actionData->v2.valid) {
        if (toGuiDX(m_actionData->v1.distanceTo(m_actionData->v2)) > 10) {
            // restore selection box to ucs
            const RS_Vector ucsP1 = toUCS(m_actionData->v1);
            const RS_Vector ucsP2 = toUCS(m_actionData->v2);

            const bool selectIntersecting = (ucsP1.x > ucsP2.x) || m_selectIntersecting;
            RS_Selection s(m_document, m_viewport);
            bool doSelect = m_select;
            if (m_invertSelectionOperation) {
                doSelect = !doSelect;
            }
            // expand selection wcs to ensure that selection box in ucs is full within bounding rect in wcs
            RS_Vector wcsP1, wcsP2;
            m_viewport->worldBoundingBox(ucsP1, ucsP2, wcsP1, wcsP2);

            if (m_selectAllEntityTypes) {
                s.selectWindow(RS2::EntityType::EntityUnknown, wcsP1, wcsP2, doSelect, selectIntersecting);
            }
            else {
                s.selectWindow(m_entityTypesToSelect, wcsP1, wcsP2, doSelect, selectIntersecting);
            }
            init(SetCorner1);
        }
    }
}

void LC_ActionSelectWindow::onMouseMoveEvent([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    const RS_Vector snapped = e->graphPoint;
    updateCoordinateWidgetByRelZero(snapped);
    if (getStatus() == SetCorner2 && m_actionData->v1.valid) {
        m_actionData->v2 = snapped;
        drawOverlayBox(m_actionData->v1, m_actionData->v2);
        if (isInfoCursorForModificationEnabled()) {
            // restore selection box to ucs
            const RS_Vector ucsP1 = toUCS(m_actionData->v1);
            const RS_Vector ucsP2 = toUCS(m_actionData->v2);
            const bool cross = (ucsP1.x > ucsP2.x) || e->isControl;
            const bool deselect = e->isShift ? m_select : !m_select;
            QString msg = deselect ? tr("De-Selecting") : tr("Selecting");
            msg.append(tr(" entities "));
            msg.append(cross ? tr("that intersect with box") : tr("that are within box"));
            m_infoCursorOverlayData->setZone2(msg);
            const RS_Vector pos = e->graphPoint;
            forceUpdateInfoCursor(pos);
        }
    }
}

void LC_ActionSelectWindow::onMouseLeftButtonPress(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetCorner1:
            if (e->isControl) {
                finish();
            }
            else {
                m_actionData->v1 = e->graphPoint;
                setStatus(SetCorner2);
            }
            break;
        default:
            break;
    }
}

void LC_ActionSelectWindow::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    if (status == SetCorner2) {
        m_actionData->v2 = e->graphPoint;
        m_selectIntersecting = e->isControl;
        m_invertSelectionOperation = e->isShift;
        trigger();
    }
}

void LC_ActionSelectWindow::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    if (status == SetCorner2) {
        deletePreview();
    }
    initPrevious(status);
}

void LC_ActionSelectWindow::updateActionPrompt() {
    switch (getStatus()) {
        case SetCorner1:
            updatePromptTRCancel(tr("Click and drag for the selection window") + " " + getSelectionCompletionHintMsg());
            break;
        case SetCorner2:
            updatePromptTRBack(tr("Choose second edge"), MOD_SHIFT_AND_CTRL(m_select ? tr("De-select entities") : tr("Select entities"),
                                                                            m_select ? tr("Select Intersecting") : tr(
                                                                                "De-select intersecting")));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionSelectWindow::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}

QList<RS2::EntityType> LC_ActionSelectWindow::getEntityTypesToSelect() {
    return m_entityTypesToSelect;
}

LC_ActionOptionsWidget* LC_ActionSelectWindow::createOptionsWidget() {
    return new LC_SelectWindowOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionSelectWindow::createOptionsFiller() {
    return RS_ActionSelectBase::createOptionsFiller(); // fixme - complete
}

void LC_ActionSelectWindow::selectionFinishedByKey([[maybe_unused]] QKeyEvent* e, [[maybe_unused]] bool escape) {
    finish();
}

bool LC_ActionSelectWindow::isSelectAllEntityTypes() const {
    return m_selectAllEntityTypes;
}

void LC_ActionSelectWindow::setSelectAllEntityTypes(const bool val) {
    m_selectAllEntityTypes = val;
}

void LC_ActionSelectWindow::setEntityTypesToSelect(const QList<RS2::EntityType>& types) {
    m_entityTypesToSelect = types;
}
