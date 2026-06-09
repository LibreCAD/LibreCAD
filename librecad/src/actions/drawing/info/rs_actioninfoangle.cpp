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
#include "lc_linemath.h"
#include "rs_debug.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_polyline.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

struct RS_ActionInfoAngle::ActionData {
    RS_Vector point1;
    RS_Vector point2;
    RS_Vector intersection;
    RS_Entity* entity1 = nullptr;
    RS_Entity* entity2 = nullptr;
};

RS_ActionInfoAngle::RS_ActionInfoAngle(LC_ActionContext* actionContext)
    : RS_PreviewActionInterface("Info Angle", actionContext, RS2::ActionInfoAngle), m_actionData(std::make_unique<ActionData>()) {
}

RS_ActionInfoAngle::~RS_ActionInfoAngle() = default;

void RS_ActionInfoAngle::init(const int status) {
    RS_PreviewActionInterface::init(status);
}

void RS_ActionInfoAngle::drawSnapper() {
    // disable snapper
}

void RS_ActionInfoAngle::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(entity)) {
        const auto polyline = static_cast<RS_Polyline*>(contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    if (isLine(entity)) {
        m_actionData->point1 = entity->getNearestPointOnEntity(clickPos);
        m_actionData->entity1 = entity;
        setStatus(SetEntity2);
    }
}

void RS_ActionInfoAngle::doTrigger() {
    RS_DEBUG->print("RS_ActionInfoAngle::trigger()");

    int status = getStatus();
    switch (status) {
        case SetEntity1: {
            if (m_actionData->entity1 != nullptr) {
                auto line = dynamic_cast<RS_Line*>(m_actionData->entity1);
                double wcsLineAngle = line->getAngle1();
                double angleComplementary, angleSupplementary, alt;

                double uscsAngle = toUCSBasisAngle(wcsLineAngle);
                RS_Math::calculateAngles(uscsAngle, angleComplementary, angleSupplementary, alt);

                QString strAngle = formatAngleRaw(uscsAngle);
                QString complimenatryStr = formatAngleRaw(angleComplementary);
                QString supplimentaryStr = formatAngleRaw(angleSupplementary);
                QString altStr = formatAngleRaw(alt);

                commandMessage("---");
                const QString& msgTemplate = tr("Angle: %1\nComplementary: %2\nSupplementary: %3\nAlternative: %4\n");
                const QString& msg = msgTemplate.arg(strAngle, complimenatryStr, supplimentaryStr, altStr);

                commandMessage(msg);
                m_actionData->entity1 = nullptr;
                setStatus(SetEntity1);
            }
            break;
        }
        case SetEntity2: {
            if (m_actionData->entity1 != nullptr && m_actionData->entity2 != nullptr) {
                const RS_VectorSolutions& sol = RS_Information::getIntersection(m_actionData->entity1, m_actionData->entity2, false);
                if (sol.hasValid()) {
                    m_actionData->intersection = sol.get(0);
                    double angle1 = m_actionData->intersection.angleTo(m_actionData->point1);
                    double angle2 = m_actionData->intersection.angleTo(m_actionData->point2);
                    double angle = remainder(angle2 - angle1, 2. * M_PI);

                    double angleComplementary, angleSupplementary, alt;
                    RS_Math::calculateAngles(angle, angleComplementary, angleSupplementary, alt);

                    QString str = formatAngleRaw(angle);
                    QString complimenatryStr = formatAngleRaw(angleComplementary);
                    QString supplimentaryStr = formatAngleRaw(angleSupplementary);
                    QString altStr = formatAngleRaw(alt);

                    RS_Vector ucsIntersection = toUCS(m_actionData->intersection);
                    QString intersectX = formatLinear(ucsIntersection.x);
                    QString intersectY = formatLinear(ucsIntersection.y);

                    RS_Vector wcsRelPoint = getRelativeZero(); // fixme - ucs - review this, why relative zero is invoked there?
                    RS_Vector intersectRel;
                    if (wcsRelPoint.valid) {
                        RS_Vector ucsRelPoint = toUCS(wcsRelPoint);
                        intersectRel = ucsIntersection - ucsRelPoint;
                    }
                    else {
                        intersectRel = ucsIntersection;
                    }

                    QString intersectRelX = formatLinear(intersectRel.x);
                    QString intersectRelY = formatLinear(intersectRel.y);

                    const QString& msgTemplate = tr(
                        "Angle: %1\nComplementary: %2\nSupplementary: %3\nAlternative: %4\nIntersection: (%5 , %6)\nIntersection :@(%7, %8)");
                    const QString& msg = msgTemplate.arg(str, complimenatryStr, supplimentaryStr, altStr, intersectX, intersectY,
                                                         intersectRelX, intersectRelY);
                    commandMessage("---");
                    commandMessage(msg);
                    commandMessage("");
                }
                else {
                    commandMessage(tr("Lines are parallel"));
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionInfoAngle::onMouseMoveEvent(const int status, const LC_MouseEvent* event) {
    const RS_Vector mouse = event->graphPoint;
    switch (status) {
        case SetEntity1: {
            const auto en = catchAndDescribe(event, RS2::ResolveAll);
            if (isLine(en)) {
                const RS_Vector p = en->getNearestPointOnEntity(mouse);
                highlightHover(en);
                previewRefSelectablePoint(p);
                if (event->isControl) {
                    const auto line = static_cast<RS_Line*>(en);
                    updateInfoCursor1(line);

                    if (m_showRefEntitiesOnPreview) {
                        const RS_Vector endpointToUse = line->getEndpoint();
                        previewSnapAngleMark(p, endpointToUse);
                    }
                }
            }
            break;
        }
        case SetEntity2: {
            const auto en = catchAndDescribe(event, RS2::ResolveAll);
            highlightSelected(m_actionData->entity1);
            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_actionData->point1);
            }
            if (isLine(en)) {
                const RS_VectorSolutions& sol = RS_Information::getIntersection(m_actionData->entity1, en, false);
                if (sol.hasValid()) {
                    highlightHover(en);
                    if (m_showRefEntitiesOnPreview) {
                        const RS_Vector p2 = en->getNearestPointOnEntity(mouse);
                        previewRefSelectablePoint(p2);
                        const RS_Vector intersection = sol.get(0);
                        updateInfoCursor2(p2, intersection);
                        previewRefArc(intersection, m_actionData->point1, p2, true);
                        previewRefPoint(intersection);
                        previewRefLine(intersection, m_actionData->point1);
                        previewRefLine(intersection, p2);
                    }
                }
                else {
                    if (m_infoCursorOverlayPrefs->enabled) {
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

void RS_ActionInfoAngle::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->graphPoint;
    switch (status) {
        case SetEntity1:
            m_actionData->entity1 = catchEntityByEvent(e, RS2::ResolveAll);
            if (isLine(m_actionData->entity1)) {
                m_actionData->point1 = m_actionData->entity1->getNearestPointOnEntity(mouse);
                if (e->isControl) {
                    trigger();
                }
                else {
                    setStatus(SetEntity2);
                }
            }
            break;

        case SetEntity2:
            m_actionData->entity2 = catchEntityByEvent(e, RS2::ResolveAll);
            if (isLine(m_actionData->entity2)) {
                m_actionData->point2 = m_actionData->entity2->getNearestPointOnEntity(mouse);
                trigger();
                setStatus(SetEntity1);
            }
            break;

        default:
            break;
    }
}

void RS_ActionInfoAngle::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionInfoAngle::updateActionPrompt() {
    switch (getStatus()) {
        case SetEntity1:
            updatePromptTRCancel(tr("Specify first line"), MOD_CTRL(tr("Single Line Mode")));
            break;
        case SetEntity2:
            updatePromptTRBack(tr("Specify second line"), MOD_CTRL(tr("Restart with first line selection")));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType RS_ActionInfoAngle::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}

void RS_ActionInfoAngle::updateInfoCursor1(const RS_Line* line) const {
    if (m_infoCursorOverlayPrefs->enabled) {
        const double wcsLineAngle = line->getAngle1();
        double angleComplementary, angleSupplementary, alt;
        double uscsAngle = toUCSBasisAngle(wcsLineAngle);
        RS_Math::calculateAngles(uscsAngle, angleComplementary, angleSupplementary, alt);

        msgStart().string(tr("Angle Info")).rawAngle(tr("Angle:"), uscsAngle).rawAngle(tr("Complementary:"), angleComplementary).
                   rawAngle(tr("Supplementary:"), angleSupplementary).rawAngle(tr("Alternative: "), alt).
                   vector(tr("Line From:"), line->getStartpoint()).vector(tr("Line To:"), line->getEndpoint()).
                   wcsAngle(tr("Line Angle 1:"), line->getAngle1()).wcsAngle(tr("Line Angle 2:"), line->getAngle2()).
                   toInfoCursorZone2(true);
    }
}

void RS_ActionInfoAngle::updateInfoCursor2(const RS_Vector& mouse, const RS_Vector& intersection) const {
    if (m_infoCursorOverlayPrefs->enabled) {
        double angle = LC_LineMath::angleFor3Points(m_actionData->point1, intersection, mouse);
        const double angle1 = intersection.angleTo(m_actionData->point1);
        const double angle2 = intersection.angleTo(mouse);

        double angleComplementary, angleSupplementary, angleAlt;
        RS_Math::calculateAngles(angle, angleComplementary, angleSupplementary, angleAlt);

        msgStart().string(tr("Angle Info")).rawAngle(tr("Angle:"), angle).rawAngle(tr("Complementary:"), angleComplementary).
                   rawAngle(tr("Supplementary:"), angleSupplementary).rawAngle(tr("Alternative: "), angleAlt).
                   vector(tr("Intersection:"), intersection).wcsAngle(tr("Line 1 Angle:"), angle1).wcsAngle(tr("Line 2 Angle:"), angle2).
                   toInfoCursorZone2(true);
    }
}
