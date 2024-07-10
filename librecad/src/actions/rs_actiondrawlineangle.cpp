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
#include "rs_settings.h"
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

void RS_ActionDrawLineAngle::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        if (getStatus()==SetPos) {
            bool shiftPressed = e->modifiers() & Qt::ShiftModifier;
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
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
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

void RS_ActionDrawLineAngle::coordinateEvent(RS_CoordinateEvent *e){
    if (!e) return;

    switch (getStatus()) {
        case SetPos:{
            pPoints->pos = e->getCoordinate();
            trigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineAngle::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
        case SetPos: {
            if (!pPoints->fixedAngle && checkCommand("angle", c)){
                deletePreview();
                setStatus(SetAngle);
            } else if (checkCommand("length", c)){
                deletePreview();
                setStatus(SetLength);
            }
            break;
        }
        case SetAngle: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok){
                e->accept();
                pPoints->angle = a;
            } else {
                commandMessageTR("Not a valid expression");
            }
            updateOptions();
            setStatus(SetPos);
            break;
        }
        case SetLength: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok){
                e->accept();
                pPoints->length = l;
            } else {
                commandMessageTR("Not a valid expression");
            }
            updateOptions();
            setStatus(SetPos);
            break;
        }
        default:
            break;
    }
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
            updateMouseWidgetTRCancel("Specify position", MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetAngle:
            updateMouseWidgetTRBack("Enter angle:");
            break;
        case SetLength:
            updateMouseWidgetTRBack("Enter length:");
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionDrawLineAngle::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

void RS_ActionDrawLineAngle::createOptionsWidget(){
    m_optionWidget = std::make_unique<QG_LineAngleOptions>();
}
// EOF
