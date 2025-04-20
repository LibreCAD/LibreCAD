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
#include "rs_actionselectwindow.h"

#include <QMouseEvent>

#include "lc_cursoroverlayinfo.h"
#include "lc_graphicviewport.h"
#include "lc_selectwindowoptions.h"
#include "rs_debug.h"
#include "rs_selection.h"

struct RS_ActionSelectWindow::Points {
    RS_Vector v1;
    RS_Vector v2;
};

/**
 * Constructor.
 *
 * @param select true: select window. false: invertSelectionOperation window
 */
RS_ActionSelectWindow::RS_ActionSelectWindow(LC_ActionContext *actionContext,bool select)
    : LC_OverlayBoxAction("Select Window",actionContext, RS2::ActionSelectWindow)
    , m_actionData(std::make_unique<Points>())
    , m_select(select){
}

RS_ActionSelectWindow::RS_ActionSelectWindow(enum RS2::EntityType typeToSelect,LC_ActionContext *actionContext,bool select)
    : LC_OverlayBoxAction("Select Window",actionContext, RS2::ActionSelectWindow)
    , m_actionData(std::make_unique<Points>())
    , m_select(select){
    if (typeToSelect == RS2::EntityUnknown){
        setSelectAllEntityTypes(true);
    }
    else{
        m_entityTypesToSelect.append(typeToSelect);
        setSelectAllEntityTypes(false);
    }
}

RS_ActionSelectWindow::~RS_ActionSelectWindow() = default;


void RS_ActionSelectWindow::init(int status) {
    RS_PreviewActionInterface::init(status);
    m_actionData = std::make_unique<Points>();
    m_selectIntersecting = false;
    //snapMode.clear();
    //snapMode.restriction = RS2::RestrictNothing;
}

void RS_ActionSelectWindow::doTrigger() {
    if (m_actionData->v1.valid && m_actionData->v2.valid){
        if (toGuiDX(m_actionData->v1.distanceTo(m_actionData->v2)) > 10){
            // restore selection box to ucs
            RS_Vector ucsP1 = toUCS(m_actionData->v1);
            RS_Vector ucsP2 = toUCS(m_actionData->v2);

            bool selectIntersecting = (ucsP1.x > ucsP2.x) || m_selectIntersecting;
            RS_Selection s(*m_container, m_viewport);
            bool doSelect = m_select;
            if (m_invertSelectionOperation){
                doSelect = !doSelect;
            }
            // expand selection wcs to ensure that selection box in ucs is full within bounding rect in wcs
            RS_Vector wcsP1, wcsP2;
            m_viewport->worldBoundingBox(ucsP1, ucsP2, wcsP1, wcsP2);

            if (m_selectAllEntityTypes) {
                s.selectWindow(RS2::EntityType::EntityUnknown, wcsP1, wcsP2, doSelect, selectIntersecting);
            }
            else{
                s.selectWindow(m_entityTypesToSelect, wcsP1, wcsP2, doSelect, selectIntersecting);
            }
            init(SetCorner1);
        }
    }
}

void RS_ActionSelectWindow::onMouseMoveEvent([[maybe_unused]]int status, LC_MouseEvent *e) {
    RS_Vector snapped = e->graphPoint;
    updateCoordinateWidgetByRelZero(snapped);
    if (getStatus()==SetCorner2 && m_actionData->v1.valid) {
        m_actionData->v2 = snapped;
        drawOverlayBox(m_actionData->v1, m_actionData->v2);
        if (isInfoCursorForModificationEnabled()) {
            // restore selection box to ucs
            RS_Vector ucsP1 = toUCS(m_actionData->v1);
            RS_Vector ucsP2 = toUCS(m_actionData->v2);
            bool cross = (ucsP1.x > ucsP2.x) || e->isControl;
            bool deselect = e->isShift ? m_select : !m_select;
            QString msg = deselect ? tr("De-Selecting") : tr("Selecting");
            msg.append(tr(" entities "));
            msg.append(cross? tr("that intersect with box") : tr("that are within box"));
            m_infoCursorOverlayData->setZone2(msg);
            const RS_Vector pos = e->graphPoint;
            forceUpdateInfoCursor(pos);
        }
    }
}

void RS_ActionSelectWindow::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case SetCorner1:
            m_actionData->v1 = toGraph(e);
            setStatus(SetCorner2);
            break;
        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionSelectWindow::mousePressEvent(): %f %f",
                    m_actionData->v1.x, m_actionData->v1.y);
}

void RS_ActionSelectWindow::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_DEBUG->print("RS_ActionSelectWindow::mouseReleaseEvent()");
    if (status==SetCorner2) {
        m_actionData->v2 = e->graphPoint;
        m_selectIntersecting = e->isControl;
        m_invertSelectionOperation = e->isShift;
        trigger();
    }
}

void RS_ActionSelectWindow::onMouseRightButtonRelease(int status, [[maybe_unused]] LC_MouseEvent *e) {
    RS_DEBUG->print("RS_ActionSelectWindow::mouseReleaseEvent()");
    if (status==SetCorner2) {
        deletePreview();
    }
    initPrevious(status);
}

void RS_ActionSelectWindow::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetCorner1:
            updateMouseWidgetTRCancel(tr("Click and drag for the selection window"));
            break;
        case SetCorner2:
            updateMouseWidgetTRBack(tr("Choose second edge"), MOD_SHIFT_AND_CTRL(m_select ? tr("De-select entities") : tr("Select entities"), m_select ? tr("Select Intersecting") : tr("De-select intersecting")));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionSelectWindow::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

QList<RS2::EntityType> RS_ActionSelectWindow::getEntityTypesToSelect(){
    return m_entityTypesToSelect;
}

LC_ActionOptionsWidget *RS_ActionSelectWindow::createOptionsWidget() {
    return new LC_SelectWindowOptions();
}

bool RS_ActionSelectWindow::isSelectAllEntityTypes() {
    return m_selectAllEntityTypes;
}

void RS_ActionSelectWindow::setSelectAllEntityTypes(bool val){
    m_selectAllEntityTypes  = val;
}

void RS_ActionSelectWindow::setEntityTypesToSelect(QList<RS2::EntityType> types) {
    m_entityTypesToSelect = types;
}
