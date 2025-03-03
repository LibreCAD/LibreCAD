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

#include "rs_actiondrawlineangle.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "qg_lineangleoptions.h"
#include "rs_actioninterface.h"

struct RS_ActionDrawLineAngle::Points {
/**
 * Line data defined so far.
 */
    RS_LineData data;
/**
 * Position.
 */
    RS_Vector pos;
/**
 * Line angle.
 */
    double angle;
/**
 * Line length.
 */
    double length{1.};
/**
 * Is the angle fixed?
 */
    bool fixedAngle;
/**
 * Snap point (start, middle, end).
 */
    int snpPoint{SNAP_START};
};

RS_ActionDrawLineAngle::RS_ActionDrawLineAngle(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView,
    double angle,
    bool fixedAngle, RS2::ActionType actionType)
    :RS_PreviewActionInterface("Draw lines with given angle",
                               container, graphicView), pPoints(std::make_unique<Points>()){

    this->actionType = actionType;
    pPoints->angle = angle;
    pPoints->fixedAngle = fixedAngle;
    reset();
}

RS_ActionDrawLineAngle::~RS_ActionDrawLineAngle() = default;


void RS_ActionDrawLineAngle::reset() {
	pPoints->data = {{}, {}};
}

void RS_ActionDrawLineAngle::init(int status) {
    RS_PreviewActionInterface::init(status);
    reset();
}

void RS_ActionDrawLineAngle::trigger() {
    RS_PreviewActionInterface::trigger();

    preparePreview();
    auto *line = new RS_Line{container, pPoints->data};
    line->setLayerToActive();
    line->setPenToActive();
    container->addEntity(line);

    addToDocumentUndoable(line);

    if (!persistRelativeZero){
        RS_Vector &newRelZero = pPoints->data.startpoint;
        if (pPoints->snpPoint == SNAP_MIDDLE){ // snap to middle
            newRelZero = (pPoints->data.startpoint + pPoints->data.endpoint)*0.5;
        }
        moveRelativeZero(newRelZero);
    }
    persistRelativeZero = false;
    graphicView->redraw(RS2::RedrawDrawing);
    RS_DEBUG->print("RS_ActionDrawLineAngle::trigger(): line added: %lu",
                    line->getId());
}

void RS_ActionDrawLineAngle::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineAngle::mouseMoveEvent begin");

    if (getStatus()==SetPos) {
        RS_Vector position = snapPoint(e);
        position = getRelZeroAwarePoint(e, position);
        pPoints->pos = position;
        deletePreview();
        preparePreview();
        previewLine(pPoints->data.startpoint, pPoints->data.endpoint);
        previewRefSelectablePoint(position);
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionDrawLineAngle::mouseMoveEvent end");
}

void RS_ActionDrawLineAngle::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    if (status==SetPos) {
        bool shiftPressed = isShift(e);
        RS_Vector position = snapPoint(e);
        // potentially, we could eliminate this and set line position on mouse move and complete action there. however,
        // it seems explicit set of position on click is more consistent with default behavior of the action?
        if (shiftPressed){
            RS_Vector relZero = graphicView->getRelativeZero();
            if (relZero.valid){
                position = graphicView->getRelativeZero();
                persistRelativeZero = true;
            }
        }
        fireCoordinateEvent(position);
    }
}

void RS_ActionDrawLineAngle::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawLineAngle::preparePreview(){
    RS_Vector p1, p2;
    // End:
    double angleRad = RS_Math::deg2rad(pPoints->angle);
    if (pPoints->snpPoint == SNAP_END){
        p2.setPolar(-pPoints->length, angleRad);
    } else {
        p2.setPolar(pPoints->length, angleRad);
    }

    // Middle:
    if (pPoints->snpPoint == SNAP_MIDDLE){
        p1 = pPoints->pos - (p2 / 2);
    } else {
        p1 = pPoints->pos;
    }

    p2 += p1;
    pPoints->data = {p1, p2};
}

void RS_ActionDrawLineAngle::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetPos:{
            pPoints->pos = pos;
            trigger();
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDrawLineAngle::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetPos: {
            if (!pPoints->fixedAngle && checkCommand("angle", c)){
                deletePreview();
                setStatus(SetAngle);
                accept = true;
            } else if (checkCommand("length", c)){
                deletePreview();
                setStatus(SetLength);
                accept = true;
            }
            break;
        }
        case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok){
                accept = true;
                pPoints->angle = a;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(SetPos);
            break;
        }
        case SetLength: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok){
                accept = true;
                pPoints->length = l;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(SetPos);
            break;
        }
        default:
            break;
    }
    return accept;
}

void RS_ActionDrawLineAngle::setSnapPoint(int sp){
    pPoints->snpPoint = sp;
}

int RS_ActionDrawLineAngle::getSnapPoint() const{
    return pPoints->snpPoint;
}

void RS_ActionDrawLineAngle::setAngle(double a){
    pPoints->angle = a;
}

double RS_ActionDrawLineAngle::getAngle() const{
    return pPoints->angle;
}

void RS_ActionDrawLineAngle::setLength(double l){
    pPoints->length = l;
}

double RS_ActionDrawLineAngle::getLength() const{
    return pPoints->length;
}

bool RS_ActionDrawLineAngle::hasFixedAngle() const{
    switch (rtti()) {
        case RS2::ActionDrawLineHorizontal:
        case RS2::ActionDrawLineVertical:
            return true;
        default:
            return false;
    }
}

QStringList RS_ActionDrawLineAngle::getAvailableCommands(){
    QStringList cmd;

    switch (getStatus()) {
        case SetPos:
            if (!pPoints->fixedAngle){
                cmd += command("angle");
            }
            cmd += command("length");
            break;
        default:
            break;
    }
    return cmd;
}

void RS_ActionDrawLineAngle::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetPos:
            updateMouseWidgetTRCancel(tr("Specify position"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetAngle:
            updateMouseWidgetTRBack(tr("Enter angle:"));
            break;
        case SetLength:
            updateMouseWidgetTRBack(tr("Enter length:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionDrawLineAngle::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

LC_ActionOptionsWidget* RS_ActionDrawLineAngle::createOptionsWidget(){
    return new QG_LineAngleOptions();
}
