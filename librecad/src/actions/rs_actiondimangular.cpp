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

#include <cmath>
#include <QAction>
#include <QMouseEvent>
#include "rs_actiondimangular.h"
#include "rs_dimangular.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_information.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"
#include "rs_math.h"

RS_ActionDimAngular::RS_ActionDimAngular(RS_EntityContainer& container,
                                         RS_GraphicView& graphicView) :
    RS_ActionDimension( "Draw Angular Dimensions", container, graphicView)
{
    reset();
}

RS_ActionDimAngular::~RS_ActionDimAngular() = default;

void RS_ActionDimAngular::reset()
{
    RS_ActionDimension::reset();

    actionType = RS2::ActionDimAngular;
    edata.reset( new RS_DimAngularData( RS_Vector( false),
                                        RS_Vector( false),
                                        RS_Vector( false),
                                        RS_Vector( false)) );
    RS_DIALOGFACTORY->requestOptions( this, true, true);
}

void RS_ActionDimAngular::trigger()
{
    RS_PreviewActionInterface::trigger();

    if (line1.getStartpoint().valid && line2.getStartpoint().valid) {
        RS_DimAngular* newEntity {new RS_DimAngular( container,
                                                     *data,
                                                     *edata)};

        newEntity->setLayerToActive();
        newEntity->setPenToActive();
        newEntity->update();
        container->addEntity(newEntity);

        // upd. undo list:
        if (document) {
            document->startUndoCycle();
            document->addUndoable(newEntity);
            document->endUndoCycle();
        }

        RS_Vector rz {graphicView->getRelativeZero()};
        setStatus( SetLine1);
        graphicView->redraw( RS2::RedrawDrawing);
        graphicView->moveRelativeZero( rz);
        RS_Snapper::finish();
    }
    else {
        RS_DEBUG->print( "RS_ActionDimAngular::trigger: Entity is nullptr\n");
    }
}

void RS_ActionDimAngular::mouseMoveEvent(QMouseEvent* e)
{
    RS_DEBUG->print( "RS_ActionDimAngular::mouseMoveEvent begin");

    switch (getStatus()) {
    case SetPos:
        if( setData( snapPoint(e))) {
            RS_DimAngular *d {new RS_DimAngular( preview.get(), *data, *edata)};

            deletePreview();
            preview->addEntity(d);
            d->update();
            drawPreview();
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDimAngular::mouseMoveEvent end");
}

void RS_ActionDimAngular::mouseReleaseEvent(QMouseEvent* e)
{
    if (Qt::LeftButton == e->button()) {
        switch (getStatus()) {
        case SetLine1: {
            RS_Entity *en {catchEntity( e, RS2::ResolveAll)};
            if (en && RS2::EntityLine == en->rtti()) {
                line1 = *dynamic_cast<RS_Line*>(en);
                click1 = line1.getNearestPointOnEntity( graphicView->toGraph( e->x(), e->y()));
                setStatus(SetLine2);
            }
            break; }

        case SetLine2: {
            RS_Entity *en{catchEntity(e, RS2::ResolveAll)};
            if (en && en->rtti()==RS2::EntityLine) {
                line2 = *dynamic_cast<RS_Line*>(en);
                click2 = line2.getNearestPointOnEntity( graphicView->toGraph( e->x(), e->y()));
                if( setData( click2, true)) {
                    graphicView->moveRelativeZero( center);
                    setStatus(SetPos);
                }
            }
            break; }

        case SetPos: {
            RS_CoordinateEvent ce( snapPoint( e));
            coordinateEvent( &ce);
            break; }
        }
    }
    else if (Qt::RightButton == e->button()) {
        deletePreview();
        init( getStatus() - 1);
    }
}

void RS_ActionDimAngular::coordinateEvent(RS_CoordinateEvent* e)
{
    if ( ! e) {
        return;
    }

    switch (getStatus()) {
    case SetPos:
        if( setData( e->getCoordinate())) {
            trigger();
            reset();
            setStatus( SetLine1);
        }
        break;

    default:
        break;
    }
}

void RS_ActionDimAngular::commandEvent(RS_CommandEvent* e)
{
    QString c( e->getCommand().toLower());

    if (checkCommand( QStringLiteral( "help"), c)) {
        RS_DIALOGFACTORY->commandMessage( msgAvailableCommands()
                                          + getAvailableCommands().join(", "));
        return;
    }

    // setting new text label:
    if (SetText == getStatus()) {
        setText( c);
        RS_DIALOGFACTORY->requestOptions( this, true, true);
        graphicView->enableCoordinateInput();
        setStatus( lastStatus);
        return;
    }

    // command: text
    if (checkCommand( QStringLiteral( "text"), c)) {
        lastStatus = static_cast<Status>(getStatus());
        graphicView->disableCoordinateInput();
        setStatus( SetText);
    }
}

QStringList RS_ActionDimAngular::getAvailableCommands()
{
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

void RS_ActionDimAngular::showOptions()
{
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions( this, true);
}

void RS_ActionDimAngular::hideOptions()
{
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions( this, false);
}

void RS_ActionDimAngular::updateMouseButtonHints()
{
    switch (getStatus()) {
    case SetLine1:
        RS_DIALOGFACTORY->updateMouseWidget( tr("Select first line"),
                                             tr("Cancel"));
        break;

    case SetLine2:
        RS_DIALOGFACTORY->updateMouseWidget( tr("Select second line"),
                                             tr("Cancel"));
        break;

    case SetPos:
        RS_DIALOGFACTORY->updateMouseWidget( tr("Specify dimension arc line location"),
                                             tr("Cancel"));
        break;

    case SetText:
        RS_DIALOGFACTORY->updateMouseWidget( tr("Enter dimension text:"), "");
        break;

    default:
        RS_DIALOGFACTORY->updateMouseWidget();
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
void RS_ActionDimAngular::justify(RS_Line &line, const RS_Vector &click)
{
    RS_Vector vStartPoint( line.getStartpoint());

    if( ! RS_Math::equal( vStartPoint.angleTo(center), click.angleTo( center))
        || vStartPoint.distanceTo( center) < click.distanceTo( center)) {
        line.reverse();
    }
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
void RS_ActionDimAngular::lineOrder(const RS_Vector &dimPos)
{
    if( ! center.valid) {
        return;
    }

    // starting point angles and selection point angle from intersection point
    double  a0  {(dimPos - center).angle()};
    double  a1  {(line1.getStartpoint() - center).angle()};
    double  a2  {(line2.getStartpoint() - center).angle()};

    // swap lines if necessary to ensure CCW order
    if( RS_Math::correctAngle2( a1 - a0) > RS_Math::correctAngle2( a2 - a0)) {
        RS_Line swapLines( line1);
        line1 = line2;
        line2 = swapLines;
        double  swapAngle {a1};
        a1 = a2;
        a2 = swapAngle;
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
        if( RS_Math::equal( a1, angle)) {
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
int RS_ActionDimAngular::quadrant(const double angle)
{
    if( 1 > angles.size()) {
        return 0;
    }

    double a1 {RS_Math::correctAngle2( angles.at(0) - angle)};
    double a2 {RS_Math::correctAngle2( angles.at(1) - angle)};

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
bool RS_ActionDimAngular::setData(const RS_Vector &dimPos, const bool calcCenter /*= false*/)
{
    if( ! line1.getStartpoint().valid || ! line2.getStartpoint().valid) {
        return false;
    }

    if ( ! center.valid || calcCenter) {
        RS_VectorSolutions sol = RS_Information::getIntersectionLineLine( &line1, &line2);
        center = sol.get(0);
    }
    if ( ! center.valid) {
        return false;
    }

    if( calcCenter) {
        justify( line1, click1);
        justify( line2, click2);
        lineOrder( dimPos);
    }

    edata->definitionPoint4 = dimPos;
    switch( quadrant( (dimPos - center).angle())) {
    default:
    case 0:
        edata->definitionPoint1 = line1.getEndpoint();
        edata->definitionPoint2 = line1.getStartpoint();
        edata->definitionPoint3 = line2.getEndpoint();
        data->definitionPoint   = line2.getStartpoint();
        break;

    case 1:
        edata->definitionPoint1 = line2.getEndpoint();
        edata->definitionPoint2 = line2.getStartpoint();
        edata->definitionPoint3 = line1.getStartpoint();
        data->definitionPoint   = line1.getEndpoint();
        break;

    case 2:
        edata->definitionPoint1 = line2.getEndpoint();
        edata->definitionPoint2 = line2.getStartpoint();
        edata->definitionPoint3 = line1.getEndpoint();
        data->definitionPoint   = line1.getStartpoint();
        break;

    case 3:
        edata->definitionPoint1 = line2.getStartpoint();
        edata->definitionPoint2 = line2.getEndpoint();
        edata->definitionPoint3 = line1.getEndpoint();
        data->definitionPoint   = line1.getStartpoint();
        break;
    }

    return true;
}

// EOF
