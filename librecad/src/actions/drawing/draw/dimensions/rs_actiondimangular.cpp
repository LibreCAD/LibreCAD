/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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

#include "rs_actiondimangular.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dimangular.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_math.h"
#include "rs_preview.h"

RS_ActionDimAngular::RS_ActionDimAngular(RS_EntityContainer& container,
                                         RS_GraphicView& graphicView) :
    RS_ActionDimension( "Draw Angular Dimensions", container, graphicView){
    reset();
}

RS_ActionDimAngular::~RS_ActionDimAngular() = default;

void RS_ActionDimAngular::reset(){
    RS_ActionDimension::reset();

    actionType = RS2::ActionDimAngular;
    edata = std::make_unique<RS_DimAngularData>( RS_Vector( false),
                                        RS_Vector( false),
                                        RS_Vector( false),
                                        RS_Vector( false));
    updateOptions();
}

void RS_ActionDimAngular::trigger(){
    RS_PreviewActionInterface::trigger();

    if (line1->getStartpoint().valid && line2->getStartpoint().valid) {
        auto* newEntity = new RS_DimAngular( container,*data,*edata);

        newEntity->setLayerToActive();
        newEntity->setPenToActive();
        newEntity->update();
        container->addEntity(newEntity);

        addToDocumentUndoable(newEntity);

        RS_Vector rz {graphicView->getRelativeZero()};
        setStatus( SetLine1);
        graphicView->redraw( RS2::RedrawDrawing);
        moveRelativeZero( rz);
        RS_Snapper::finish();
    }
    else {
        RS_DEBUG->print( "RS_ActionDimAngular::trigger: Entity is nullptr\n");
    }
}

void RS_ActionDimAngular::mouseMoveEvent(QMouseEvent* e){
    RS_DEBUG->print( "RS_ActionDimAngular::mouseMoveEvent begin");
    deleteHighlights();
    RS_Vector snap = snapPoint(e);
    switch (getStatus()) {
        case SetLine1: {
            RS_Entity *en = catchEntity(e, RS2::EntityLine, RS2::ResolveAll);
            if (en != nullptr){
                highlightHover(en);
            }
            break;
        }
        case SetLine2: {
            RS_Entity *en = catchEntity(e, RS2::EntityLine, RS2::ResolveAll);
            if (en != nullptr && en != line1){
                highlightHover(en);
            }
            highlightSelected(line1);
            break;
        }
        case SetPos: {
            snap = getFreeSnapAwarePoint(e, snap);
            if (setData(snap)){
                deletePreview();
                auto *d = new RS_DimAngular(preview.get(), *data, *edata);
                highlightSelected(line1);
                highlightSelected(line2);

                if (showRefEntitiesOnPreview) {
                    double radius = snap.distanceTo(center);

                    // draw reference points for all quadrants
                    for (double angle: angles) {
                        RS_Vector vec = RS_Vector::polar(radius, angle);
                        RS_Vector refPoint = center + vec;
                        previewRefPoint(refPoint);
                    }
                }

                d->update();
                previewEntity(d);
                drawPreview();
            }
            break;
        }
        default:
            break;
    }
    drawHighlights();
    RS_DEBUG->print("RS_ActionDimAngular::mouseMoveEvent end");
}

void RS_ActionDimAngular::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    const RS_Vector &pos = toGraph(e);
    switch (status) {
        case SetLine1: {
            RS_Entity *en = catchEntity(e, RS2::EntityLine,RS2::ResolveAll);
            if (en != nullptr){
                line1 = dynamic_cast<RS_Line *>(en);
                click1 = line1->getNearestPointOnEntity(pos);
                setStatus(SetLine2);
            }
            break;
        }
        case SetLine2: {
            RS_Entity *en = catchEntity(e, RS2::EntityLine,RS2::ResolveAll);
            if (en != nullptr){
                if (en != line1){
                    line2 = dynamic_cast<RS_Line *>(en);
                    click2 = line2->getNearestPointOnEntity(pos);
                    if (setData(click2, true)){
                        moveRelativeZero(center);
                        setStatus(SetPos);
                    }
                }
            }
            break;
        }
        case SetPos: {
            RS_Vector snap = snapPoint(e);
            snap = getFreeSnapAwarePoint(e, snap);
            fireCoordinateEvent(snap);
            break;
        }
        default:
            break;
    }
}

void RS_ActionDimAngular::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDimAngular::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetPos:
            if (setData(pos)){
                trigger();
                reset();
                setStatus(SetLine1);
            }
            break;

        default:
            break;
    }
}

bool RS_ActionDimAngular::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    // setting new text label:
    if (SetText == getStatus()) {
        setText( c);
        updateOptions();
        graphicView->enableCoordinateInput();
        setStatus( lastStatus);
        accept = true;
    }
    else if (checkCommand( "text", c)) { // command: text
        lastStatus = static_cast<Status>(status);
        graphicView->disableCoordinateInput();
        setStatus( SetText);
        accept = true;
    }
    return accept;
}

QStringList RS_ActionDimAngular::getAvailableCommands(){
    QStringList cmd;

    switch (getStatus()) {
    case SetLine1:
    case SetLine2:
    case SetPos:
        cmd += command( QStringLiteral( "text"));
        break;
    default:
        break;
    }
    return cmd;
}

// REVIEW - PROBABLY THERE SHOULD BE BACK INSTEAD OF CANCEL
void RS_ActionDimAngular::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetLine1:
            updateMouseWidgetTRCancel(tr("Select first line"));
            break;
        case SetLine2:
            updateMouseWidgetTRCancel(tr("Select second line"));
            break;
        case SetPos:
            updateMouseWidgetTRCancel(tr("Specify dimension arc line location"), MOD_SHIFT_FREE_SNAP);
            break;
        case SetText:
            updateMouseWidget(tr("Enter dimension text:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

/**
 * Justify one of the angle lines to ensure that the starting point
 * of the line has the same angle from the intersection point as the
 * selection click point and it is further away than the line end point
 *
 * @param line A selected line for the dimension
 * @param click The click pos which selected the line
 * @param center The intersection of the 2 lines to dimension
 */
RS_LineData RS_ActionDimAngular::justify(RS_Line* line, const RS_Vector &click){
    RS_Vector vStartPoint( line->getStartpoint());

    RS_LineData lineData = line->getData();

    if( ! RS_Math::equal( vStartPoint.angleTo(center), click.angleTo( center), RS_TOLERANCE_ANGLE)
        || vStartPoint.distanceTo( center) < click.distanceTo( center)) {
        lineData.reverse();
    }
    return lineData;
}

/**
 * Create a sorted array with angles from the lines intersection point
 * to the starting points and their revers angles.
 * Ensure, that line1 and line2 are in CCW order.
 * Compute an offset for quadrant() method.
 *
 * @param line A selected line for the dimension
 * @param click The click pos which selected the line
 * @param center The intersection of the 2 lines to dimension
 */
void RS_ActionDimAngular::lineOrder(const RS_Vector &dimPos, RS_LineData& ld1, RS_LineData& ld2){
    if( ! center.valid) {
        return;
    }

    // starting point angles and selection point angle from intersection point
    double  a0  =  (dimPos - center).angle();
    double  a1  = (ld1.startpoint - center).angle();
    double  a2  = (ld2.startpoint - center).angle();

    // swap lines if necessary to ensure CCW order
    if( RS_Math::correctAnglePlusMinusPi( a1 - a0) > RS_Math::correctAnglePlusMinusPi( a2 - a0)) {
        std::swap(ld1, ld2);
        std::swap(a1, a2);
    }

    // sorted array with starting point and reverse angles
    angles.clear();
    angles.push_back( a1);
    angles.push_back( RS_Math::correctAngle( a1 + M_PI));
    angles.push_back( a2);
    angles.push_back( RS_Math::correctAngle( a2 + M_PI));
    std::sort( angles.begin(), angles.end());

    // find starting quadrant and compute the offset for quadrant() method
    int startQuadrant = 0;
    for( auto angle : angles) {
        if( RS_Math::equal( a1, angle, RS_TOLERANCE_ANGLE)) {
            break;
        }
        ++startQuadrant;
    }
    quadrantOffset = 0x03 & (4 - startQuadrant);
}

/**
 * Find the quadrant of \p angle relative to 1st quadrant.
 * When the angle lines are selected, the starting quadrant
 * is shifted to become 0 by \p quadrantOffset.
 * This is the criterion how the angles dimension is drawn.
 *
 * @param angle The angle, e.g. mouse or coordinate position
 * @return The quadrant of \p angle, relative to the 1st selection quadrant
 */
int RS_ActionDimAngular::determineQuadrant(const double angle){
    if(angles.empty()) {
        return 0;
    }

    double a1 {RS_Math::correctAnglePlusMinusPi( angles.at(0) - angle)};
    double a2 {RS_Math::correctAnglePlusMinusPi( angles.at(1) - angle)};

    int angleQuadrant {0};
    if( 0.0 < a1 && 0.0 < a2) {
        angleQuadrant = 3;
    }
    else if( 0.0 >= a1 && 0.0 >= a2) {
        angleQuadrant = 1;
    }
    else if( 0.0 < a1 && 0.0 >= a2) {
        angleQuadrant = 2;
    }

    return (0x03 & (angleQuadrant + quadrantOffset));
}

/**
 * On \p mouseMoveEvent, \p mouseReleaseEvent and \p coordinateEvent
 * this method sets the dimension data appropriate to the mouse
 * cursor/coordinate in \p dimPos.
 * When \p calcCenter is true, the intersection point and other static
 * values are computed. This is only necessary, when line selection changes,
 * e.g. on \p mouseReleaseEvent. For \p mouseMoveEvent calcCenter is false.
 *
 * @param dimPos The mouse/coordinate position
 * @param calcCenter If true, the center and corresponding values are calculated
 * @return true If the dimension data were set, false is a parameter is invalid
 */
bool RS_ActionDimAngular::setData(const RS_Vector &dimPos, const bool calcCenter /*= false*/){
    bool result = false;
    if (line1->getStartpoint().valid && line2->getStartpoint().valid){

        if (!center.valid || calcCenter){
            RS_VectorSolutions sol = RS_Information::getIntersectionLineLine(line1, line2);
            center = sol.get(0);
        }

        RS_LineData ld1 = justify(line1, click1);
        RS_LineData ld2 = justify(line2, click2);

        if (center.valid){
            if (calcCenter){
                lineOrder(dimPos, ld1, ld2);
            }

            edata->definitionPoint4 = dimPos;
            double angleFromCenter = (dimPos - center).angle();
            int quad = determineQuadrant(angleFromCenter);
            switch (quad) {
                default:
                case 0:
                    edata->definitionPoint1 = ld1.endpoint;
                    edata->definitionPoint2 = ld1.startpoint;
                    edata->definitionPoint3 = ld2.endpoint;
                    data->definitionPoint = ld2.startpoint;
                    break;

                case 1:
                    edata->definitionPoint1 = ld2.endpoint;
                    edata->definitionPoint2 = ld2.startpoint;
                    edata->definitionPoint3 = ld1.startpoint;
                    data->definitionPoint = ld1.endpoint;
                    break;

                case 2:
                    edata->definitionPoint1 = ld2.endpoint;
                    edata->definitionPoint2 = ld2.startpoint;
                    edata->definitionPoint3 = ld1.endpoint;
                    data->definitionPoint = ld1.startpoint;
                    break;

                case 3:
                    edata->definitionPoint1 = ld2.startpoint;
                    edata->definitionPoint2 = ld2.endpoint;
                    edata->definitionPoint3 = ld1.endpoint;
                    data->definitionPoint = ld1.startpoint;
                    break;
            }
            result = true;
        }
    }
    return result;
}
