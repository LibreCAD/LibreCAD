/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 Melwyn Francis Carlo
** Copyright (C) 2024 Dongxu Li <dongxuli2011@gmail.com>
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

#include "lc_actionsnapmiddlemanual.h"

#include "qc_applicationwindow.h"
#include "qg_snaptoolbar.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_pen.h"

constexpr double g_defaultRatio = 0.5;

struct LC_ActionSnapMiddleManual::SnapMiddleManualData {
    SnapMiddleManualData()= default;
    ~SnapMiddleManualData() = default;

    double percentage = g_defaultRatio;
    RS_Vector startPoint{false};
    RS_Vector endPoint{false};
    RS_Pen currentAppPen{};
};

/*
    This action class can snap (set) the relative-zero marker
    in the middle between two points chosen by the user.

    The middle point, by default, rests at a poistion 50% relative 
    to the position of the first point chosen, that is, 
    at the center of the imaginary line connecting the 
    two user-defined points.

    This percentage, however, can be modified by the user 
    in order to place the marker at a different position 
    along the imaginary line.
*/


LC_ActionSnapMiddleManual::LC_ActionSnapMiddleManual(LC_ActionContext *actionContext): // todo - is copy of pen really necessary there?
    RS_PreviewActionInterface("Snap Middle Manual", actionContext, RS2::ActionSnapMiddleManual),
    m_actionData{std::make_unique<SnapMiddleManualData>()}{
    RS_DEBUG->print("LC_ActionSnapMiddleManual::LC_ActionSnapMiddleManual");

    m_actionData->currentAppPen = m_document->getActivePen();
    const RS_Pen snapMiddleManual_pen { RS_Pen(RS_Color(255,0,0), RS2::Width01, RS2::DashDotLineTiny) };
    m_document->setActivePen(snapMiddleManual_pen);
}

LC_ActionSnapMiddleManual::~LC_ActionSnapMiddleManual() = default;

void LC_ActionSnapMiddleManual::init(int status){
    RS_DEBUG->print("LC_ActionSnapMiddleManual::init");
    m_document->setActivePen(m_actionData->currentAppPen); // fixme - sand - check this, it looks like invalid pen is set
    RS_PreviewActionInterface::init(status);
    m_actionData->percentage = g_defaultRatio;
    drawSnapper();
}

void LC_ActionSnapMiddleManual::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    if (status == SetEndPoint){
        /* Snapping to an angle defined by settings, if the shift key is pressed. */
        mouse = getSnapAngleAwarePoint(e, m_actionData->startPoint, mouse,true);

        auto *line = previewLine(m_actionData->startPoint, mouse);
        previewRefSelectablePoint(line->getMiddlePoint());
        if (m_showRefEntitiesOnPreview){
            previewRefLine(m_actionData->startPoint, mouse);
            previewRefPoint(m_actionData->startPoint);
            previewRefPoint(mouse);
        }
    } else if (getStatus() == SetPercentage){
        if (m_predecessor != nullptr){
            if (m_predecessor->getName().compare("Snap Middle Manual") == 0){
                m_predecessor->init(-1);
                init(-1);
            }
        }
    }
}

void LC_ActionSnapMiddleManual::onMouseLeftButtonRelease([[maybe_unused]] int status, LC_MouseEvent *e) {
    RS_Vector snapped = e->snapPoint;
    snapped = getSnapAngleAwarePoint(e, m_actionData->startPoint, snapped);
    fireCoordinateEvent(snapped);
}

void LC_ActionSnapMiddleManual::fireUnsetMiddleManual() {
    auto snapToolbar = QC_ApplicationWindow::getAppWindow()->getSnapToolBar();
    snapToolbar->slotUnsetSnapMiddleManual();
}

void LC_ActionSnapMiddleManual::onMouseRightButtonRelease(int status, [[maybe_unused]] LC_MouseEvent *e) {
    deletePreview();

    switch (status) {
        case SetPercentage:
        case SetStartPoint: {
            finish();
            fireUnsetMiddleManual();
            break;
        }
        default:
            setStatus(SetPercentage);
            init(getStatus());
    }
}

void LC_ActionSnapMiddleManual::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPercentage:
        case SetStartPoint: {
            m_actionData->startPoint = mouse;
            setStatus(SetEndPoint);
            moveRelativeZero(mouse);
            updateMouseButtonHints();
            break;
        }
        case SetEndPoint: {
            /* Refuse zero length lines. */
            if ((mouse - m_actionData->startPoint).squared() > RS_TOLERANCE2) {
                m_actionData->endPoint = mouse;

                const RS_Vector middleManualPoint =
                    m_actionData->startPoint + (m_actionData->endPoint - m_actionData->startPoint) * m_actionData->percentage;

                moveRelativeZero(middleManualPoint);

                if (m_predecessor != nullptr) {
                    if (m_predecessor->getName().compare("Default") != 0) {
                        fireUnsetMiddleManual();
                        m_document->setActivePen(m_actionData->currentAppPen);
                        RS_CoordinateEvent new_e(middleManualPoint);
                        m_predecessor->coordinateEvent(&new_e);
                        init(-1);
                    }
                }

                setStatus(SetPercentage);
                updateMouseButtonHints();
                init(getStatus());
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionSnapMiddleManual::doProcessCommand(int status, const QString& command) {
    bool accepted = false;
    QString inputCommand = command.toLower();

    switch (status) {
        case SetPercentage: {
            bool ok = false;
            m_actionData->percentage = RS_Math::eval(inputCommand, &ok) / 100.;
            if (ok){
                setStatus(SetStartPoint);
                accepted = true;
                updateMouseButtonHints();
            } else {
                m_actionData->percentage = g_defaultRatio;
            }
            break;
        }
        case SetStartPoint: {
            if (checkCommand("help", inputCommand)){
                commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
                accepted = true;
            }
            break;
        }
        case SetEndPoint: {
            if (checkCommand("close", inputCommand)){
                setStatus(-1);
                accepted = true;
                updateMouseButtonHints();
            }
            break;
        }
        default:
            break;
    }
    return accepted;
}

QStringList LC_ActionSnapMiddleManual::doGetAvailableCommands(int status){
    QStringList actionCommandsList;
    switch (status) {
        case SetEndPoint:
            actionCommandsList += command("close");
            break;
        default:
            break;
    }
    return actionCommandsList;
}

void LC_ActionSnapMiddleManual::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetPercentage:
            updateMouseWidgetTRCancel(tr("Specify percentage / start-point"));
            break;
        case SetStartPoint:
            updateMouseWidgetTRCancel(tr("Specify start point"));
            break;
        case SetEndPoint:
            updateMouseWidgetTRBack(tr("Specify end point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}
