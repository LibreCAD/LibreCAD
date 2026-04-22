/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_action_draw_line_radiant.h"

#include "lc_line_radiant_options_filler.h"
#include "lc_line_radiant_options_widget.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_settings.h"

void LC_ActionDrawLineRadiant::doSaveOptions() {
    save("Radiant1" ,m_radiantPoints[ONE]);
    save("Radiant2" ,m_radiantPoints[TWO]);
    save("Radiant3", m_radiantPoints[THREE]);
    save("Radiant4", m_radiantPoints[FOUR]);
    save("ActiveRadiantIdx", m_activeRadiantIndex);
    save("Length", m_length);
    save("LengthType", m_lengthType);
}

void LC_ActionDrawLineRadiant::doLoadOptions() {
    m_radiantPoints[ONE] = loadVector("Radiant1" ,RS_Vector(-1000, 0));
    m_radiantPoints[TWO] = loadVector("Radiant2" ,RS_Vector(1000, 0));
    m_radiantPoints[THREE] = loadVector("Radiant3", RS_Vector(0, -1000));
    m_radiantPoints[FOUR] = loadVector("Radiant4", RS_Vector(0, 1000));
    m_activeRadiantIndex = static_cast<RadiantIdx>(loadInt("ActiveRadiantIdx", ONE));
    m_length = loadDouble("Length", 1.0);
    m_lengthType = static_cast<LenghtType>(loadInt("LengthType", LenghtType::LINE));
}

LC_ActionDrawLineRadiant::LC_ActionDrawLineRadiant(LC_ActionContext* actionContext) :
    LC_UndoableDocumentModificationAction("ActionDrawLineRadiant", actionContext, RS2::ActionDrawLineRadiant) {
}

void LC_ActionDrawLineRadiant::init(const int status) {
    LC_UndoableDocumentModificationAction::init(status);
}

bool LC_ActionDrawLineRadiant::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    RS_Vector secondPoint;
    if (isFreeLength()) {
        secondPoint = defineLineSecondPointFree(m_secondSnapPoint);
    }
    else {
        secondPoint = defineLineSecondPointAuto(m_startPoint);
    }
    if (secondPoint.valid) {
        if (LC_LineMath::isMeaningfulDistance(m_startPoint, secondPoint)) {
            const auto line = new RS_Line(nullptr, {m_startPoint, secondPoint});
            ctx += line;
            return true;
        }
        commandMessage(tr("Resulting line is too short"));
    }
    return false;
}

void LC_ActionDrawLineRadiant::doTriggerCompletion(const bool success) {
    if (success) {
        addSnappedPointToVisualSnap(m_startPoint);
        moveRelativeZero(m_startPoint);
        setStatus(SetPoint);
    }
}

void LC_ActionDrawLineRadiant::doTriggerSelections(const LC_DocumentModificationBatch& ctx) {
    if (ctx.success) {
        const bool keepSelected = LC_GET_ONE_BOOL("Modify", "KeepModifiedSelected", true);
        if (keepSelected) {
            select(ctx.entitiesToAdd);
        }
    }
}

void LC_ActionDrawLineRadiant::previewRadiantLine(const RS_Vector& snapped, const RS_Vector& secondPoint) const {
    if (secondPoint.valid) {
        if (LC_LineMath::isMeaningfulDistance(snapped, secondPoint)) {
            previewToCreateLine(snapped, secondPoint);
            if (m_showRefEntitiesOnPreview) {
                // previewRefLine(snapped, secondPoint);
                previewRefPoint(secondPoint);
                previewRefPoint(getActiveRadiant());
                previewRefSelectablePoint(snapped);
            }
        }
    }
}

void LC_ActionDrawLineRadiant::onMouseMoveEvent([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    const RS_Vector snapped = e->snapPoint;
    switch (status) {
        case SetPoint: {
            if (!trySnapToRelZeroCoordinateEvent(e)) {
                if (isFreeLength()) {
                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(getActiveRadiant());
                        previewRefSelectablePoint(snapped);
                    }
                }
                else {
                    const RS_Vector secondPoint = defineLineSecondPointAuto(snapped);
                    previewRadiantLine(snapped, secondPoint);
                }
            }
            break;
        }
        case SetPoint2: {
            const RS_Vector secondPoint = defineLineSecondPointFree(snapped);
            previewRadiantLine(m_startPoint, secondPoint);
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawLineRadiant::onMouseLeftButtonRelease([[maybe_unused]]int status, const LC_MouseEvent* e) {
    const RS_Vector snapped = getRelZeroAwarePoint(e, e->snapPoint);
    fireCoordinateEvent(snapped);
}

void LC_ActionDrawLineRadiant::onMouseRightButtonRelease(const int status, [[maybe_unused]]const LC_MouseEvent* e) {
    if (status == SetPoint2) {
        setStatus(SetPoint);
    }
    else {
        setStatus(-1);
    }
}

bool LC_ActionDrawLineRadiant::doProcessCommand(const int status, const QString& command) {
    bool result = true;
    if (checkCommand("length", command)) {
        setStatus(SetLength);
    }
    else if (checkCommand("lentype", command)) {
      setStatus(SetLengthType);
    }
    else if (checkCommand("radiant", command)) {
        setStatus(SetRadiant);
    }
    else if (checkCommand("active", command)) {
        setStatus(SetActive);
    }
    else {
        if (status == SetLength) {
            bool ok = false;
            const double l = RS_Math::eval(command, &ok);
            if (ok && l > RS_TOLERANCE) {
                setLength(l);
                restoreMainStatus();
                updateOptions();
            }
            else {
                result = false;
            }
        }
        else if (status == SetLengthType) {
           if (tr("line") == command) {
               setLengthType(LINE);
               updateOptions();
           }
            else if (tr("x") == command) {
               setLengthType(BY_X);
               updateOptions();
            }
            else if (tr("y") == command) {
               setLengthType(BY_Y);
               updateOptions();
            }
            else if (tr("point") == command) {
               setLengthType(TO_POINT);
               updateOptions();
            }
            else if (tr("free") == command) {
               setLengthType(FREE);
               updateOptions();
            }
            else {
                result = false;
            }
        }
        else if (status == SetActive) {
            bool ok = false;
            const int idx = command.toInt(&ok);
            if (ok) {
                if (idx < 5 && idx > 0) {
                    const auto type = static_cast<RadiantIdx>(idx-1);
                    setActiveRadiantIndex(type);
                    updateOptions();
                    const RS_Vector activePoint = getActiveRadiant();
                    QString activeStr = formatVector(activePoint);
                    commandMessage(tr("Radiant point to use: ")+ activeStr);
                }
                else {
                    commandMessage(tr("Invalid radiant point number."));
                }
            }
            else {
                result = false;
            }
        }
        else {
            result = false;
        }
    }
    return result;
}

void LC_ActionDrawLineRadiant::onCoordinateEvent(const int status, const bool isZero, const RS_Vector& pos) {
    if (isZero) {
        return;
    }
    switch (status) {
        case SetPoint: {
            m_startPoint = pos;
            if (isFreeLength()) {
                setMainStatus(SetPoint2);
            }
            else {
                trigger();
            }
            break;
        }
        case SetPoint2: {
            m_secondSnapPoint = pos;
            trigger();
            break;
        }
        case SetRadiant: {
            setActiveRadiantPoint(pos);
            updateOptions();
            restoreMainStatus();
            break;
        }
        default:
            break;
    }
}

RS_Vector LC_ActionDrawLineRadiant::defineLineSecondPointFree(const RS_Vector& snapped) const {
    const double angle = m_startPoint.angleTo(getActiveRadiant());
    const RS_Vector result = LC_LineMath::calculateEndpointForAngleDirection(angle, m_startPoint, snapped);
    return result;
}

RS_Vector LC_ActionDrawLineRadiant::defineLineSecondPointAuto(const RS_Vector& snapped) const {
    const auto activePoint = getActiveRadiant();
    switch (m_lengthType) {
        case LenghtType::TO_POINT: {
            return activePoint;
        }
        case LenghtType::LINE: {
            const double angle = snapped.angleTo(activePoint);
            const RS_Vector result = snapped.relative(m_length, angle);
            return result;
        }
        case LenghtType::BY_X: {
            const double angle = snapped.angleTo(activePoint);
            // here we perform all calculations in user coordinate system
            const RS_Vector snappedUCS = toUCS(snapped);
            const RS_Vector activeUCS = toUCS(activePoint);

            double ucsX;
            double ucsY;

            if (snappedUCS.x > activeUCS.x) {
                ucsX = snappedUCS.x - m_length;
            }
            else {
                ucsX = snappedUCS.x + m_length;
            }

            const double ucsAngle = toUCSAngle(angle);
            const double correctedAngle = RS_Math::correctAngle0ToPi(ucsAngle);
            if (RS_Math::getAngleDifferenceU(correctedAngle, M_PI_2) < RS_TOLERANCE_ANGLE) {
                ucsY = activePoint.y;
                ucsX = activePoint.x;
            }
            else {
                const double tanAlpha = std::tan(correctedAngle);
                const double leg = m_length * tanAlpha;
                if (snappedUCS.y > activeUCS.y) {
                    ucsY = snappedUCS.y - leg;
                }
                else {
                    ucsY = snappedUCS.y + leg;
                }
            }
            const RS_Vector ucsPoint(ucsX, ucsY);
            return toWorld(ucsPoint);
        }
        case LenghtType::BY_Y: {
            const double len = m_length;
            const double angle = snapped.angleTo(activePoint);

            const RS_Vector snappedUCS = toUCS(snapped);
            const RS_Vector activeUCS = toUCS(activePoint);
            double ucsY;
            if (snappedUCS.y > activeUCS.y) {
                ucsY = snappedUCS.y - len;
            }
            else {
                ucsY = snappedUCS.y + len;
            }
            double ucsX;
            const double ucsAngle = toUCSAngle(angle);
            const double correctedAngle = RS_Math::correctAngle0ToPi(ucsAngle);
            if (RS_Math::getAngleDifferenceU(correctedAngle, 0) < RS_TOLERANCE_ANGLE ||
                RS_Math::getAngleDifferenceU(correctedAngle, M_PI) < RS_TOLERANCE_ANGLE) {
                ucsY = activePoint.y;
                ucsX = activePoint.x;
            }
            else {
                const double tanAlpha = std::tan(correctedAngle);
                const double leg = len / tanAlpha;
                if (snappedUCS.x > activeUCS.x) {
                    ucsX = snappedUCS.x - leg;
                }
                else {
                    ucsX = snappedUCS.x + leg;
                }
            }
            const RS_Vector ucsPoint(ucsX, ucsY);
            return toWorld(ucsPoint);
        }
        default:
            return RS_Vector(false);
    }
}

bool LC_ActionDrawLineRadiant::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "length") {
        setLength(distance);
        return true;
    }
    return false;
}

bool LC_ActionDrawLineRadiant::doUpdatePointByInteractiveInput(const QString& tag, RS_Vector& point) {
    if (tag == "pointX") {
        setActiveX(point.x);
        return true;
    }
    if (tag == "pointY") {
        setActiveY(point.y);
        return true;
    }
    if (tag == "farPoint") {
        setActiveRadiantPoint(point);
        return true;
    }
    return false;
}

RS2::CursorType LC_ActionDrawLineRadiant::doGetMouseCursor([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

void LC_ActionDrawLineRadiant::updateActionPrompt() {
    switch (getStatus()) {
        case SetPoint: {
            updatePromptTRCancel(tr("Specify start point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetPoint2: {
            updatePromptTRCancel(tr("Specify second point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetRadiant: {
            updatePromptTRCancel(tr("Specify radiant point position"));
            break;
        }
        case SetLength: {
            updatePromptTRCancel(tr("Specify line length"));
            break;
        }
         case SetLengthType: {
            updatePromptTRCancel(tr("Specify line length type ") + QString("[%1|%2|%3|%4|%5]").arg(tr("line"), tr("x"), tr("y"), tr("point"), tr("free")));
            break;
        }
        case SetActive: {
            updatePromptTRCancel(tr("Specify number of radiant point to use [1-4]"));
            break;
        }
        default:
            break;
    }
}

QStringList LC_ActionDrawLineRadiant::getAvailableCommands() {
    QStringList cmd;
    cmd << command("length");
    cmd << command("lentype");
    cmd << command("active");
    cmd << command("radiant");
    return cmd;
}

void LC_ActionDrawLineRadiant::setLengthType(const LenghtType type) {
    m_lengthType = type;
    setMainStatus(SetPoint);
}

LC_ActionOptionsWidget* LC_ActionDrawLineRadiant::createOptionsWidget() {
    return new LC_LineRadiantOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawLineRadiant::createOptionsFiller() {
    return new LC_LineRadiantOptionsFiller();
}

bool LC_ActionDrawLineRadiant::isInVisualSnapStatus(int status) {
    return (status == SetPoint) || (status == SetPoint2);
}
