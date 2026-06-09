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

#include "rs_actiondimleader.h"

#include <QKeyEvent>

#include "rs_leader.h"

struct RS_ActionDimLeader::ActionData {
    std::vector<RS_Vector> points;
};

RS_ActionDimLeader::RS_ActionDimLeader(LC_ActionContext* actionContext)
    : LC_SingleEntityCreationAction("Draw leaders", actionContext, RS2::ActionDimLeader), m_actionData(std::make_unique<ActionData>()) {
    reset();
}

RS_ActionDimLeader::~RS_ActionDimLeader() = default;

void RS_ActionDimLeader::reset() const {
    m_actionData->points.clear();
}

void RS_ActionDimLeader::init(const int status) {
    RS_PreviewActionInterface::init(status);
    reset();
}

RS_Entity* RS_ActionDimLeader::doTriggerCreateEntity() {
    if (!m_actionData->points.empty()) {
        auto* leaderEntity = new RS_Leader(m_document, RS_LeaderData(true, ""));
        for (const auto& vp : m_actionData->points) {
            leaderEntity->addVertex(vp);
        }
        return leaderEntity;
    }
    return nullptr;
}

void RS_ActionDimLeader::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetStartpoint:
            trySnapToRelZeroCoordinateEvent(e);
            break;
        case SetEndpoint: {
            if (!m_actionData->points.empty()) {
                mouse = getSnapAngleAwarePoint(e, m_actionData->points.back(), mouse, true);

                // fill in lines that were already set:
                RS_Vector last(false);
                for (const auto& v : m_actionData->points) {
                    if (last.valid) {
                        previewLine(last, v);
                    }
                    last = v;
                }

                const RS_Vector& p = m_actionData->points.back();
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(p);
                    previewRefSelectablePoint(mouse);
                }
                previewLine(p, mouse);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDimLeader::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector snapped = e->snapPoint;
    switch (status) {
        case SetStartpoint: {
            break;
        }
        case SetEndpoint: {
            if (!m_actionData->points.empty()) {
                snapped = getSnapAngleAwarePoint(e, m_actionData->points.back(), snapped);
            }
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snapped);
}

void RS_ActionDimLeader::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    if (status == SetEndpoint) {
        trigger();
        reset();
        setStatus(SetStartpoint);
    }
    else {
        deletePreview();
        initPrevious(status);
    }
}

void RS_ActionDimLeader::keyPressEvent(QKeyEvent* e) {
    if (getStatus() == SetEndpoint && e->key() == Qt::Key_Enter) {
        trigger();
        reset();
        setStatus(SetStartpoint);
    }
}

void RS_ActionDimLeader::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& pos) {
    switch (status) {
        case SetStartpoint: {
            m_actionData->points.clear();
            m_actionData->points.push_back(pos);
            setStatus(SetEndpoint);
            moveRelativeZero(pos);
            break;
        }
        case SetEndpoint: {
            m_actionData->points.push_back(pos);
            moveRelativeZero(pos);
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDimLeader::doProcessCommand([[maybe_unused]] int status, const QString& command) {
    // enter to finish
    bool accept = false;
    // fixme - EMPTY COMMAND!!! change to enter processing?
    if (command.isEmpty()) {
        trigger();
        reset();
        setStatus(SetStartpoint);
        accept = true;
    }
    return accept;
}

void RS_ActionDimLeader::updateActionPrompt() {
    switch (getStatus()) {
        case SetStartpoint:
            updatePromptTRCancel(tr("Specify target point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetEndpoint:
            updatePrompt(tr("Specify next point"), tr("Finish"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType RS_ActionDimLeader::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}
