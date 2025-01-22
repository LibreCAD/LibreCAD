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
#include "rs_line.h"
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


void RS_ActionInfoAngle::drawSnapper() {
    // disable snapper
}

void RS_ActionInfoAngle::doTrigger() {
    RS_DEBUG->print("RS_ActionInfoAngle::trigger()");

    int status = getStatus();
    switch (status){
        case SetEntity1: {
            if (entity1 != nullptr){
                auto line = dynamic_cast<RS_Line*>(entity1);
                double angle1 = line->getAngle1();
                double angle2 = line->getAngle2();
                QString strAngle1 = formatAngle(angle1);
                if (angle1 < 0.) {
                    strAngle1 += " or ";
                    strAngle1 += formatAngle(angle1 + 2. * M_PI);
                }
                QString strAngle2 = formatAngle(angle2);
                if (angle2 < 0.) {
                    strAngle2 += " or ";
                    strAngle2 += formatAngle(angle2 + 2. * M_PI);
                }
                commandMessage("---");
                const QString &msgTemplate = tr("Angle 1: %1\nAngle 2: %2");
                const QString &msg = msgTemplate.arg(strAngle1, strAngle2);
                commandMessage(msg);
                entity1 = nullptr;
                setStatus(SetEntity1);
            }
            break;
        }
        case SetEntity2:{
            if (entity1 != nullptr && entity2 != nullptr){
                RS_VectorSolutions const &sol = RS_Information::getIntersection(entity1, entity2, false);
                if (sol.hasValid()) {
                    pPoints->intersection = sol.get(0);
                    double angle1 = pPoints->intersection.angleTo(pPoints->point1);
                    double angle2 = pPoints->intersection.angleTo(pPoints->point2);
                    double angle = remainder(angle2 - angle1, 2. * M_PI);
                    QString str = formatAngle(angle);
                    QString intersectX = formatLinear(pPoints->intersection.x);
                    QString intersectY = formatLinear(pPoints->intersection.y);
                    if (angle < 0.) {
                        str += " or ";
                        str += formatAngle(angle + 2. * M_PI);
                    }

                    RS_Vector relPoint = graphicView->getRelativeZero();
                    RS_Vector intersectRel;
                    if (relPoint.valid) {
                        intersectRel = pPoints->intersection - relPoint;
                    } else {
                        intersectRel = pPoints->intersection;
                    }

                    QString intersectRelX = formatLinear(intersectRel.x);
                    QString intersectRelY = formatLinear(intersectRel.y);

                    const QString &msgTemplate = tr("Angle: %1\nIntersection: (%2 , %3)\nIntersection :@(%4, %5)");
                    const QString &msg = msgTemplate.arg(str, intersectX, intersectY, intersectRelX, intersectRelY);
                    commandMessage("---");
                    commandMessage(msg);

                } else {
                    commandMessage(tr("Lines are parallel"));
                }
            }
            break;
        }
        default:
            break;
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
            auto en = catchEntityOnPreview(event, RS2::ResolveAll);
            if (isLine(en)){
                RS_Vector p = en->getNearestPointOnEntity(mouse);
                highlightHover(en);
                previewRefSelectablePoint(p);
            }
            break;
        }
        case SetEntity2: {
            auto en = catchEntityOnPreview(event, RS2::ResolveAll);
            highlightSelected(entity1);
            if (showRefEntitiesOnPreview) {
                previewRefPoint(pPoints->point1);
            }
            if (isLine(en)){
                RS_VectorSolutions const &sol = RS_Information::getIntersection(entity1, en, false);
                if (sol.hasValid()){
                    highlightHover(en);
                    if (showRefEntitiesOnPreview) {
                        RS_Vector p2 = en->getNearestPointOnEntity(mouse);
                        previewRefSelectablePoint(p2);
                        RS_Vector intersection = sol.get(0);
                        updateInfoCursor(p2,intersection);
                        previewRefArc(intersection, pPoints->point1, p2, true);
                        previewRefPoint(intersection);
                        previewRefLine(intersection, pPoints->point1);
                        previewRefLine(intersection, p2);
                    }
                }
                else{
                    if (infoCursorOverlayPrefs->enabled){
                        appendInfoCursorZoneMessage(tr("Lines are parallel"), 2, false);
                    }
                }
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
                if (isControl(e)){
                    trigger();
                }
                else {
                    setStatus(SetEntity2);
                }
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
            updateMouseWidgetTRCancel(tr("Specify first line"), MOD_CTRL(tr("Single Line Mode")));
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

void RS_ActionInfoAngle::updateInfoCursor(const RS_Vector &point2, const RS_Vector &intersection) {
    if (infoCursorOverlayPrefs->enabled){
        double angle1 = intersection.angleTo(pPoints->point1);
        double angle2 = intersection.angleTo(point2);
        double angle = remainder(angle2 - angle1, 2. * M_PI);
        QString str = formatAngle(angle);

        LC_InfoMessageBuilder msg(tr("Info"));
        msg.add(tr("Angle:"),formatAngle(angle));
        if (angle < 0) {
            msg.add(tr("Angle (alt): "), formatAngle(angle + 2. * M_PI));
        }
        msg.add(tr("Intersection:"), formatVector(intersection));
        msg.add(tr("Line 1 Angle:"), formatAngle(angle1));
        msg.add(tr("Line 2 Angle:"), formatAngle(angle2));
        appendInfoCursorZoneMessage(msg.toString(), 2, true);
    }
}
