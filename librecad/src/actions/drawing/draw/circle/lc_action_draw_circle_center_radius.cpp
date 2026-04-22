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

#include "lc_action_draw_circle_center_radius.h"

#include "lc_circle_center_radius_options_filler.h"
#include "lc_circle_center_radius_options_widget.h"
#include "rs_circle.h"
#include "rs_document.h"
#include "rs_entitycontainer.h"

/**
 * Constructor.
 */
LC_ActionDrawCircleCenterRadius::LC_ActionDrawCircleCenterRadius(LC_ActionContext* actionContext)
    : LC_ActionDrawCircleBase("ActionDrawCircleCenterRadius", actionContext, RS2::ActionDrawCircleCenterRadius) {
}

LC_ActionDrawCircleCenterRadius::LC_ActionDrawCircleCenterRadius(const QString& name, LC_ActionContext* actionContext, const RS2::ActionType type)
    : LC_ActionDrawCircleBase(name, actionContext, type) {
}

LC_ActionDrawCircleCenterRadius::~LC_ActionDrawCircleCenterRadius() = default;

void LC_ActionDrawCircleCenterRadius::doSaveOptions() {
    save("Radius", m_radius);
}

void LC_ActionDrawCircleCenterRadius::doLoadOptions() {
    m_radius = loadDouble("Radius", 1.0);
}

void LC_ActionDrawCircleCenterRadius::reset() {
}

void LC_ActionDrawCircleCenterRadius::init(const int status) {
    LC_ActionDrawCircleBase::init(status);
}

RS_Entity* LC_ActionDrawCircleCenterRadius::doTriggerCreateEntity() {
    auto* circle = new RS_Circle(m_document, RS_CircleData(m_center, m_radius));
    switch (getStatus()) {
        case SetCenter:
            moveRelativeZero(circle->getCenter());
            break;
        case SetRadius:
            break;
        default:
            break;
    }
    return circle;
}

void LC_ActionDrawCircleCenterRadius::doTriggerCompletion([[maybe_unused]] bool success) {
    setStatus(SetCenter);
}

void LC_ActionDrawCircleCenterRadius::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetCenter: {
            if (!trySnapToRelZeroCoordinateEvent(e)) {
                m_center = mouse;
                previewToCreateCircle(RS_CircleData(m_center, m_radius));
                previewRefSelectablePoint(m_center);
            }
            else {
                setStatus(-1);
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawCircleCenterRadius::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "radius") {
        setRadius(distance);
        return true;
    }
    return false;
}

bool LC_ActionDrawCircleCenterRadius::isInVisualSnapStatus(int status) {
    return (status == SetCenter);
}

void LC_ActionDrawCircleCenterRadius::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& pos) {
    switch (status) {
        case SetCenter: {
            m_center = pos;
            trigger();
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawCircleCenterRadius::doProcessCommand(const int status, const QString& command) {
    bool accept = false;
    switch (status) {
        case SetCenter: {
            if (checkCommand("radius", command)) {
                deletePreview();
                setStatus(SetRadius);
                accept = true;
            }
            break;
        }
        case SetRadius: {
            bool ok = false;
            // fixme - review processing and add more messages if needed
            const double r = RS_Math::eval(command, &ok);
            if (ok && r > RS_TOLERANCE) {
               m_radius = r;
                accept = true;
                trigger();
            }
            else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            break;
        }
        default:
            break;
    }
    return accept;
}

bool LC_ActionDrawCircleCenterRadius::setRadiusStr(const QString& sr) {
    bool ok = false;
    const double r = RS_Math::eval(sr, &ok);
    if (!ok) {
        // fixme - good candidate for generic utility method, may be useful for setting values via ui
        commandMessage(tr("radius=%1 is invalid (expression)").arg(sr));
    }
    else if (std::signbit(r)) {
        commandMessage(tr("radius=%1 is invalid (negative)").arg(sr));
        ok = false;
    }
    else if (r <= RS_TOLERANCE) {
        commandMessage(tr("radius=%1 is invalid (zero)").arg(sr));
        ok = false;
    }
    else {
        m_radius = r;
    }
    return ok;
}

QStringList LC_ActionDrawCircleCenterRadius::getAvailableCommands() {
    QStringList cmd;
    switch (getStatus()) {
        case SetCenter:
            cmd += command("radius");
            break;
        default:
            break;
    }
    return cmd;
}

void LC_ActionDrawCircleCenterRadius::updateActionPrompt() {
    switch (getStatus()) {
        case SetCenter:
            updatePromptTRCancel(tr("Specify circle center"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetRadius:
            updatePromptTRBack(tr("Specify circle radius"));
            break;
        default:
            updatePrompt();
            break;
    }
}

void LC_ActionDrawCircleCenterRadius::setRadius(const double val){
    m_radius = val;
}

double LC_ActionDrawCircleCenterRadius::getRadius() const {
    return m_radius;
}

LC_ActionOptionsWidget* LC_ActionDrawCircleCenterRadius::createOptionsWidget() {
    return new LC_CircleCenterRadiusOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawCircleCenterRadius::createOptionsFiller() {
    return new LC_CircleCenterRadiusOptionsFiller();
}
