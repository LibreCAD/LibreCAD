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

#include "rs_actioninfoangle.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_cursoroverlayinfo.h"
#include "rs_debug.h"
#include "rs_information.h"
#include "rs_line.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

struct RS_ActionInfoAngle::ActionData {
	RS_Vector point1;
	RS_Vector point2;

	RS_Vector intersection;
};

// fixme - sand - adding information about angle to entity info view

RS_ActionInfoAngle::RS_ActionInfoAngle(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Info Angle", actionContext, RS2::ActionInfoAngle)
    ,m_entity1(nullptr)
    ,m_entity2(nullptr)
    ,m_actionData(std::make_unique<ActionData>()){
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
            if (m_entity1 != nullptr){
                auto line = dynamic_cast<RS_Line*>(m_entity1);
                double angle1 = line->getAngle1();
                double angle2 = line->getAngle2();
                QString strAngle1 = formatWCSAngle(angle1);
                if (angle1 < 0.) {
                    strAngle1 += " or ";
                    strAngle1 += formatWCSAngle(angle1 + 2. * M_PI);
                }
                QString strAngle2 = formatWCSAngle(angle2);
                if (angle2 < 0.) {
                    strAngle2 += " or ";
                    strAngle2 += formatWCSAngle(angle2 + 2. * M_PI);
                }
                commandMessage("---");
                const QString &msgTemplate = tr("Angle 1: %1\nAngle 2: %2");
                const QString &msg = msgTemplate.arg(strAngle1, strAngle2);
                commandMessage(msg);
                m_entity1 = nullptr;
                setStatus(SetEntity1);
            }
            break;
        }
        case SetEntity2:{
            if (m_entity1 != nullptr && m_entity2 != nullptr){
                RS_VectorSolutions const &sol = RS_Information::getIntersection(m_entity1, m_entity2, false);
                if (sol.hasValid()) {
                    m_actionData->intersection = sol.get(0);
                    double angle1 = m_actionData->intersection.angleTo(m_actionData->point1);
                    double angle2 = m_actionData->intersection.angleTo(m_actionData->point2);
                    double angle = remainder(angle2 - angle1, 2. * M_PI);
                    QString str = formatAngleRaw(angle);

                    RS_Vector ucsIntersection = toUCS(m_actionData->intersection);
                    QString intersectX = formatLinear(ucsIntersection.x);
                    QString intersectY = formatLinear(ucsIntersection.y);
                    if (angle < 0.) {
                        str += " or ";
                        str += formatAngleRaw(angle + 2. * M_PI);
                    }

                    RS_Vector wcsRelPoint = getRelativeZero(); // fixme - ucs - review this, why relative zero is invoked there?
                    RS_Vector ucsRelPoint = toUCS(wcsRelPoint);
                    RS_Vector intersectRel;
                    if (wcsRelPoint.valid) {
                        intersectRel = ucsIntersection - ucsRelPoint;
                    } else {
                        intersectRel = ucsIntersection;
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

void RS_ActionInfoAngle::onMouseMoveEvent(int status, LC_MouseEvent *event) {
    RS_Vector mouse = event->graphPoint;
    switch (status) {
        case SetEntity1: {
            auto en = catchAndDescribe(event, RS2::ResolveAll);
            if (isLine(en)){
                RS_Vector p = en->getNearestPointOnEntity(mouse);
                highlightHover(en);
                previewRefSelectablePoint(p);
            }
            break;
        }
        case SetEntity2: {
            auto en = catchAndDescribe(event, RS2::ResolveAll);
            highlightSelected(m_entity1);
            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_actionData->point1);
            }
            if (isLine(en)){
                RS_VectorSolutions const &sol = RS_Information::getIntersection(m_entity1, en, false);
                if (sol.hasValid()){
                    highlightHover(en);
                    if (m_showRefEntitiesOnPreview) {
                        RS_Vector p2 = en->getNearestPointOnEntity(mouse);
                        previewRefSelectablePoint(p2);
                        RS_Vector intersection = sol.get(0);
                        updateInfoCursor(p2,intersection);
                        previewRefArc(intersection, m_actionData->point1, p2, true);
                        previewRefPoint(intersection);
                        previewRefLine(intersection, m_actionData->point1);
                        previewRefLine(intersection, p2);
                    }
                }
                else{
                    if (m_infoCursorOverlayPrefs->enabled){
                        appendInfoCursorZoneMessage(tr("Lines are parallel"), 2, false);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionInfoAngle::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->graphPoint;
    switch (status) {
        case SetEntity1:
            m_entity1 = catchEntityByEvent(e, RS2::ResolveAll);
            if (isLine(m_entity1)){
                m_actionData->point1 = m_entity1->getNearestPointOnEntity(mouse);
                if (e->isControl){
                    trigger();
                }
                else {
                    setStatus(SetEntity2);
                }
            }
            break;

        case SetEntity2:
            m_entity2 = catchEntityByEvent(e, RS2::ResolveAll);
            if (isLine(m_entity2)){
                m_actionData->point2 = m_entity2->getNearestPointOnEntity(mouse);
                trigger();
                setStatus(SetEntity1);
            }
            break;

        default:
            break;
    }
}

void RS_ActionInfoAngle::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
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
    if (m_infoCursorOverlayPrefs->enabled){
        double angle1 = intersection.angleTo(m_actionData->point1);
        double angle2 = intersection.angleTo(point2);
        double angle = remainder(angle2 - angle1, 2. * M_PI);

        auto builder = msg(tr("Info"))
            .rawAngle(tr("Angle:"), angle);
        if (angle < 0) {
            builder.rawAngle(tr("Angle (alt): "), angle + 2. * M_PI);
        }
        builder.vector(tr("Intersection:"), intersection)
               .wcsAngle(tr("Line 1 Angle:"), angle1)
               .wcsAngle(tr("Line 2 Angle:"), angle2)
               .toInfoCursorZone2(true);
    }
}
