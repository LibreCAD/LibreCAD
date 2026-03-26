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

#include "lc_action_draw_line_angle.h"

#include "lc_action_draw_line_radiant.h"
#include "lc_line_angle_options_filler.h"
#include "lc_line_angle_options_widget.h"
#include "rs_document.h"
#include "rs_line.h"


LC_ActionDrawLineAngle::LC_ActionDrawLineAngle(LC_ActionContext* actionContext, const bool fixedAngle, const RS2::ActionType actionType)
    : LC_SingleEntityCreationAction("", actionContext, actionType) {
    switch (actionType) {
        case RS2::ActionDrawLineAngle: {
            m_optionsSettingsGroupName = "ActionDrawLineAngle";
            break;
        }
        case RS2::ActionDrawLineHorizontal: {
            m_optionsSettingsGroupName = "ActionDrawLineHorizontal";
            break;
        }
        case RS2::ActionDrawLineVertical: {
            m_optionsSettingsGroupName = "ActionDrawLineVertical";
            break;
        }
        default:
            break;
    }
    m_fixedAngle = fixedAngle;
    reset();
}

LC_ActionDrawLineAngle::~LC_ActionDrawLineAngle() = default;

void LC_ActionDrawLineAngle::doSaveOptions() {
    if (hasFixedAngle()) {
        save("InAnglesBasis", m_orthoToAnglesBasis);
    } else {
        save("Angle", m_ucsBasisAngleRad);
    }

    save("Length", m_lineLength);
    save("SnapPoint", m_lineSnapMode);
    save("LengthType", m_lengthType);
}

void LC_ActionDrawLineAngle::doLoadOptions() {
    if (hasFixedAngle()) {
        m_orthoToAnglesBasis = loadBool("InAnglesBasis", false);
    } else {
        m_ucsBasisAngleRad = loadDouble("Angle", m_ucsBasisAngleRad);
    }

    m_lineLength = loadDouble("Length", 1.0);
    m_lineSnapMode = static_cast<SnapMode>(loadInt("SnapPoint", 0));
    m_lengthType = static_cast<LengthType>(loadInt("LengthType", 0));
}


void LC_ActionDrawLineAngle::reset() {
    m_lineData = {{}, {}};
}

void LC_ActionDrawLineAngle::init(const int status) {
    reset();
    RS_PreviewActionInterface::init(status);
}

void LC_ActionDrawLineAngle::initFromSettings() {
    RS_PreviewActionInterface::initFromSettings();
    if (m_actionType == RS2::ActionDrawLineVertical) {
        m_ucsBasisAngleRad = M_PI_2;
    }
    else if (m_actionType == RS2::ActionDrawLineHorizontal) {
        m_ucsBasisAngleRad = 0;
    }
}

RS_Entity* LC_ActionDrawLineAngle::doTriggerCreateEntity() {
    preparePreview();
    auto* line = new RS_Line{m_document, m_lineData};
    return line;
}

void LC_ActionDrawLineAngle::doTriggerCompletion([[maybe_unused]] bool success) {
    if (!m_persistRelativeZero) {
        RS_Vector& newRelZero = m_lineData.startpoint;
        if (m_lineSnapMode == SNAP_MIDDLE) {
            // snap to middle
            newRelZero = (m_lineData.startpoint + m_lineData.endpoint) * 0.5;
        }
        moveRelativeZero(newRelZero);
    }
    m_persistRelativeZero = false;
    setMainStatus(SetPos);
}

bool LC_ActionDrawLineAngle::isInVisualSnapStatus(int status) {
    return (status == SetPos) || (status == SetPoint2);
}


void LC_ActionDrawLineAngle::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector position = e->snapPoint;
    if (status == SetPos) {
        position = getRelZeroAwarePoint(e, position);
        m_pos = position;
        m_alternateDirection = e->isControl;
        if (isFreeLineMode()) {
            if (m_showRefEntitiesOnPreview) {
                previewRefSelectablePoint(position);
            }
        }
        else {
            preparePreview();
            previewToCreateLine(m_lineData.startpoint, m_lineData.endpoint);
            if (m_showRefEntitiesOnPreview) {
                previewRefSelectablePoint(position);
            }
        }
    }
    else if (status == SetPoint2) {
        m_secondPoint = position;
        preparePreview();
        previewToCreateLine(m_lineData.startpoint, m_lineData.endpoint);
        if (m_showRefEntitiesOnPreview) {
            previewRefSelectablePoint(position);
            previewRefSelectablePoint(m_lineData.endpoint);
            previewRefPoint(m_lineData.startpoint);
            if (isFreeLineMode() && m_lineSnapMode == SNAP_MIDDLE) {
                previewRefPoint(m_pos);
            }
        }
    }
}

void LC_ActionDrawLineAngle::setLengthType(LengthType type, bool doUpdateOptions) {
    m_lengthType = type;
    setMainStatus(SetPos);
    if (doUpdateOptions) {
        updateOptions();
    }
}

bool LC_ActionDrawLineAngle::isFreeLineMode() const {
    return m_lengthType == FREE;
}

void LC_ActionDrawLineAngle::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector position = e->snapPoint;
    if (status == SetPos) {
        const bool shiftPressed = e->isShift;
        // potentially, we could eliminate this and set line position on mouse move and complete action there. however,
        // it seems explicit set of position on click is more consistent with default behavior of the action?
        m_alternateDirection = e->isControl;
        if (shiftPressed) {
            const RS_Vector relZero = getRelativeZero();
            if (relZero.valid) {
                position = relZero;
                m_persistRelativeZero = true;
            }
        }
        else {
            m_persistRelativeZero = false;
        }
        fireCoordinateEvent(position);
    }
    else if (status == SetPoint2){
        fireCoordinateEvent(position);
    }
}

void LC_ActionDrawLineAngle::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    switch (status) {
        case SetPoint2: {
            setStatus(SetPos);
            break;
        }
        case SetPos: {
            setStatus(-1);
            break;
        }
        default:
            setStatus(SetPos);
            break;
    }
}

void LC_ActionDrawLineAngle::preparePreview() {
    RS_Vector p1, p2;
    double angle = m_ucsBasisAngleRad;
    if (m_alternateDirection) {
        if (m_actionType == RS2::ActionDrawLineVertical) {
            angle = 0.0;
        }
        else if (m_actionType == RS2::ActionDrawLineHorizontal) {
            angle = M_PI_2;
        }
        else {
            angle = M_PI - angle;
        }
    }
    double wcsAngleRad = adjustRelativeAngleSignByBasis(angle);
    if (hasFixedAngle()) {
        if (m_orthoToAnglesBasis) {
            wcsAngleRad = toWorldAngleFromUCSBasis(wcsAngleRad);
        }
        else {
            wcsAngleRad = toWorldAngle(wcsAngleRad);
        }
    }
    else {
        wcsAngleRad = toWorldAngleFromUCSBasis(wcsAngleRad);
    }

    double len = m_lineLength;

    if (isFreeLineMode()) {
       len = m_pos.distanceTo(m_secondPoint);
       if (m_lineSnapMode == SNAP_MIDDLE) {
         len = len*2;
       };
    }
    else {
        if (!hasFixedAngle()) {
            switch (m_lengthType) {
                case BY_X: {
                    const double ucsAngle = toUCSAngle(wcsAngleRad);
                    const double correctedAngle = RS_Math::correctAngle0ToPi(ucsAngle);
                    const double cosA = std::cos(correctedAngle);
                    if (LC_LineMath::isMeaningful(cosA)) {
                        len = len / cosA;
                    }
                    break;
                }
                case BY_Y: {
                    const double ucsAngle = toUCSAngle(wcsAngleRad);
                    const double correctedAngle = RS_Math::correctAngle0ToPi(ucsAngle);
                    const double sinA = std::sin(correctedAngle);
                    if (LC_LineMath::isMeaningful(sinA)) {
                        len = len / sinA;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    if (m_lineSnapMode == SNAP_END) {
        p2.setPolar(-len, wcsAngleRad);
    }
    else {
        p2.setPolar(len, wcsAngleRad);
    }

    // Middle:
    if (m_lineSnapMode == SNAP_MIDDLE) {
        p1 = m_pos - (p2 / 2);
    }
    else {
        p1 = m_pos;
    }

    p2 += p1;

    if (isFreeLineMode() && (m_lineSnapMode != SNAP_MIDDLE)) {
        RS_Vector projectionPoint = LC_LineMath::getNearestPointOnInfiniteLine(m_secondPoint, p1,p2);
        if (projectionPoint.distanceTo(p2) > projectionPoint.distanceTo(p1)) {
            p2 = p2.rotate(p1, M_PI);
        }
    }
    m_lineData = {p1, p2};
}

void LC_ActionDrawLineAngle::onCoordinateEvent(const int status, [[maybe_unused]] const bool isZero, const RS_Vector& pos) {
    switch (status) {
        case SetPos: {
            m_pos = pos;
            addSnappedPointToVisualSnap(pos);
            if (isFreeLineMode()) {
                setMainStatus(SetPoint2);
            }
            else {
                trigger();
            }
            break;
        }
        case SetPoint2: {
            m_secondPoint = pos;
            addSnappedPointToVisualSnap(pos);
            trigger();
            break;
        }
        case SetAngle: {
            if (isZero) {
                m_ucsBasisAngleRad = 0.0;
                updateOptions();
                restoreMainStatus();
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawLineAngle::doProcessCommand(const int status, const QString& command) {
    bool accept = true;
    if (!m_fixedAngle && checkCommand("angle", command)) {
        setStatus(SetAngle);
    }
    else if (checkCommand("length", command)) {
        setStatus(SetLength);
    }
    else if (checkCommand("lentype", command)) {
        setStatus(SetLengthType);
    }
    if (checkCommand("snap", command)) {
        setStatus(SetSnapPoint);
    }
    else {
        switch (status) {
            case SetAngle: {
                double ucsBasisAngleRad;
                const bool ok = parseToUCSBasisAngle(command, ucsBasisAngleRad);
                if (ok) {
                    accept = true;
                    ucsBasisAngleRad = ucsBasisAngleRad;
                }
                else {
                    commandMessage(tr("Not a valid expression for angle"));
                }
                updateOptions();
                restoreMainStatus();
                break;
            }
            case SetLength: {
                bool ok;
                const double l = RS_Math::eval(command, &ok);
                if (ok) {
                    accept = true;
                    m_lineLength = l;
                }
                else {
                    commandMessage(tr("Not a valid expression for length"));
                }
                updateOptions();
                restoreMainStatus();
                break;
            }
            case SetSnapPoint: {
                if (tr("s") == command) {
                    m_lineSnapMode = SNAP_START;
                    updateOptions();
                    restoreMainStatus();
                }
                else if (tr("m") == command) {
                    m_lineSnapMode = SNAP_MIDDLE;
                    updateOptions();
                    restoreMainStatus();
                }
                else if (tr("e") == command) {
                    m_lineSnapMode = SNAP_END;
                    updateOptions();
                    restoreMainStatus();
                }
                else {
                    accept = false;
                }
                break;
            }
            case SetLengthType: {
                if (tr("line") == command) {
                    setLengthType(LINE);
                    restoreMainStatus();
                    updateOptions();
                }
                else if (tr("x") == command) {
                    setLengthType(BY_X);
                    restoreMainStatus();
                    updateOptions();
                }
                else if (tr("y") == command) {
                    setLengthType(BY_Y);
                    restoreMainStatus();
                    updateOptions();
                }
                else if (tr("free") == command) {
                    setLengthType(FREE);
                    restoreMainStatus();
                    updateOptions();
                }
                else {
                    accept = false;
                }
                break;
            }
            default:
                accept = false;
        }
    }
    return accept;
}

void LC_ActionDrawLineAngle::setLineSnapMode(const int sp)  {
    m_lineSnapMode = static_cast<SnapMode>(sp);
}

int LC_ActionDrawLineAngle::getLineSnapMode() const {
    return m_lineSnapMode;
}

void LC_ActionDrawLineAngle::setUcsAngleDegrees(const double ucsRelAngleDegrees)  {
    m_ucsBasisAngleRad = RS_Math::deg2rad(ucsRelAngleDegrees);
}

double LC_ActionDrawLineAngle::getUcsAngleDegrees() const {
    return RS_Math::rad2deg(m_ucsBasisAngleRad);
}

void LC_ActionDrawLineAngle::setLength(const double l) {
    m_lineLength = l;
}

double LC_ActionDrawLineAngle::getLength() const {
    return m_lineLength;
}

bool LC_ActionDrawLineAngle::hasFixedAngle() const {
    switch (rtti()) {
        case RS2::ActionDrawLineHorizontal:
        case RS2::ActionDrawLineVertical:
            return true;
        default:
            return false;
    }
}

QStringList LC_ActionDrawLineAngle::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
        case SetPos:
            if (!m_fixedAngle) {
                cmd += command("angle");
            }
            cmd += command("length");
            cmd += command("lentype");
            cmd += command("snap");
            break;
        default:
            break;
    }
    return cmd;
}

void LC_ActionDrawLineAngle::updateActionPrompt() {
    switch (getStatus()) {
        case SetPos: {
            updatePromptTRCancel(tr("Specify position"), MOD_SHIFT_AND_CTRL(MSG_REL_ZERO, tr("Alternate Direction")));
            break;
        }
        case SetAngle: {
            updatePromptTRBack(tr("Enter angle:"));
            break;
        }
        case SetLength: {
            updatePromptTRBack(tr("Enter length:"));
            break;
        }
        case SetLengthType: {
            QString typesStr;
            if (m_fixedAngle) {
                typesStr = QString("[%1|%2]:").arg(tr("line"), tr("free"));
            }
            else {
                typesStr = QString("[%1|%2|%3|%4]:").arg(tr("line"), tr("x"), tr("y"), tr("free"));
            }
            updatePromptTRBack(tr("Enter length type ") + typesStr);
            break;
        }
        case SetPoint2: {
            updatePromptTRBack(tr("Specify second point"));
            break;
        }
        case SetSnapPoint: {
            updatePromptTRBack(tr("Enter snap type ") + QString("[%1|%2|%3]:").arg(tr("s"), tr("m"),tr("e")));
            break;
        }
        default: {
            updatePrompt();
            break;
        }
    }
}

RS2::CursorType LC_ActionDrawLineAngle::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

LC_ActionOptionsWidget* LC_ActionDrawLineAngle::createOptionsWidget() {
    return new LC_LineAngleOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawLineAngle::createOptionsFiller() {
    return new LC_LineAngleOptionsFiller();
}

void LC_ActionDrawLineAngle::setInAngleBasis(const bool b) {
    m_orthoToAnglesBasis = b;
}

bool LC_ActionDrawLineAngle::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        m_ucsBasisAngleRad = angleRad;
        return true;
    }
    return false;
}

bool LC_ActionDrawLineAngle::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "length") {
        setLength(distance);
        return true;
    }
    return false;
}
