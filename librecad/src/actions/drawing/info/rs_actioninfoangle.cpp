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

#include <cmath>

#include <QMouseEvent>

#include "rs_actioninfoangle.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_units.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

struct RS_ActionInfoAngle::Points {
	RS_Vector point1;
	RS_Vector point2;

	RS_Vector intersection;
};

// fixme - sand - adding information about angle to entity info view

RS_ActionInfoAngle::RS_ActionInfoAngle(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Info Angle",
						   container, graphicView)
		,entity1(nullptr)
        ,entity2(nullptr)
    , pPoints(std::make_unique<Points>()){
    actionType = RS2::ActionInfoAngle;
}

RS_ActionInfoAngle::~RS_ActionInfoAngle() = default;

void RS_ActionInfoAngle::init(int status){
    RS_PreviewActionInterface::init(status);
}

void RS_ActionInfoAngle::trigger(){
    RS_DEBUG->print("RS_ActionInfoAngle::trigger()");

    if (entity1 != nullptr && entity2 != nullptr){
        RS_VectorSolutions const &sol = RS_Information::getIntersection(entity1, entity2, false);

        if (sol.hasValid()){
            pPoints->intersection = sol.get(0);

            if (pPoints->intersection.valid && pPoints->point1.valid && pPoints->point2.valid){
                double angle1 = pPoints->intersection.angleTo(pPoints->point1);
                double angle2 = pPoints->intersection.angleTo(pPoints->point2);
                double angle = remainder(angle2 - angle1, 2. * M_PI);
                RS2::AngleFormat angleFormat = graphic->getAngleFormat();
                int anglePrec = graphic->getAnglePrecision();
                QString str = RS_Units::formatAngle(angle, angleFormat, anglePrec);
                RS2::LinearFormat linearFormat = graphic->getLinearFormat();
                RS2::Unit linearUnit = graphic->getUnit();
                int linearPrecision = graphic->getLinearPrecision();
                QString intersectX = RS_Units::formatLinear(pPoints->intersection.x, linearUnit,linearFormat, linearPrecision);
                QString intersectY = RS_Units::formatLinear(pPoints->intersection.y, linearUnit,linearFormat, linearPrecision);
                if (angle < 0.){
                    str += " or ";
                    str += RS_Units::formatAngle(angle + 2. * M_PI, angleFormat, anglePrec);
                }

                RS_Vector relPoint = graphicView->getRelativeZero();
                RS_Vector intersectRel;
                if (relPoint.valid){
                    intersectRel = pPoints->intersection - relPoint;
                }
                else{
                    intersectRel = pPoints->intersection;
                }

                QString intersectRelX = RS_Units::formatLinear(intersectRel.x, linearUnit,linearFormat, linearPrecision);
                QString intersectRelY = RS_Units::formatLinear(intersectRel.y, linearUnit,linearFormat, linearPrecision);

                const QString &msgTemplate = tr("Angle: %1\nIntersection: (%2 , %3)\nIntersection :@(%4, %5)");
                const QString &msg = msgTemplate.arg(str, intersectX, intersectY, intersectRelX, intersectRelY);
                commandMessage("---");
                commandMessage(msg);

            }
        } else {
            commandMessage(tr("Lines are parallel"));
        }
    }
}

void RS_ActionInfoAngle::mouseMoveEvent(QMouseEvent *event){
    deleteHighlights();
    deletePreview();

    int status = getStatus();
    snapPoint(event);

    RS_Vector mouse = toGraph(event);

    switch (status) {
        case SetEntity1: {
            auto en = catchEntity(event, RS2::ResolveAll);
            if (isLine(en)){
                RS_Vector p = en->getNearestPointOnEntity(mouse);
                highlightHover(en);
                previewRefSelectablePoint(p);
            }
            break;
        }
        case SetEntity2: {
            auto en = catchEntity(event, RS2::ResolveAll);
            if (isLine(en)){
                RS_VectorSolutions const &sol = RS_Information::getIntersection(entity1, en, false);
                if (sol.hasValid()){
                    highlightHover(en);
                    if (showRefEntitiesOnPreview) {
                        RS_Vector p2 = en->getNearestPointOnEntity(mouse);
                        previewRefSelectablePoint(p2);
                        RS_Vector intersection = sol.get(0);
                        previewRefArc(intersection, pPoints->point1, p2, true);
                        previewRefPoint(intersection);
                        previewRefLine(intersection, pPoints->point1);
                        previewRefLine(intersection, p2);
                    }
                }
            }
            highlightSelected(entity1);
            if (showRefEntitiesOnPreview) {
                previewRefPoint(pPoints->point1);
            }
            break;
        }
        default:
            break;
    }

    drawPreview();
    drawHighlights();
}

void RS_ActionInfoAngle::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector mouse = toGraph(e);
    switch (status) {
        case SetEntity1:
            entity1 = catchEntity(e, RS2::ResolveAll);
            if (isLine(entity1)){
                pPoints->point1 = entity1->getNearestPointOnEntity(mouse);
                setStatus(SetEntity2);
            }
            break;

        case SetEntity2:
            entity2 = catchEntity(e, RS2::ResolveAll);
            if (isLine(entity2)){
                pPoints->point2 = entity2->getNearestPointOnEntity(mouse);
                trigger();
                setStatus(SetEntity1);
            }
            break;

        default:
            break;
    }
}

void RS_ActionInfoAngle::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionInfoAngle::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity1:
            updateMouseWidgetTRCancel(tr("Specify first line"));
            break;
        case SetEntity2:
            updateMouseWidgetTRBack(tr("Specify second line"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionInfoAngle::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
