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

#include <QMouseEvent>

#include "rs_actioninfoarea.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_infoarea.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_units.h"

RS_ActionInfoArea::RS_ActionInfoArea(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Info Area",
                               container, graphicView)
    , m_infoArea(std::make_unique<RS_InfoArea>()){
    actionType=RS2::ActionInfoArea;
}

RS_ActionInfoArea::~RS_ActionInfoArea() = default;

void RS_ActionInfoArea::init(int status) {
    RS_PreviewActionInterface::init(status);
    if(status==SetFirstPoint){
        m_infoArea = std::make_unique<RS_InfoArea>();
    }
    //RS_DEBUG->print( "RS_ActionInfoArea::init: %d" ,status );
}

void RS_ActionInfoArea::doTrigger() {
    RS_DEBUG->print("RS_ActionInfoArea::trigger()");
    display(false);
    lastPointRequested = false;
    init(SetFirstPoint);
}
// fixme - sand - consider displaying information in EntityInfo widget
// fixme - sand - add area info to entity info widget for coordinates mode
//todo: we regenerate the whole preview, it's possible to generate needed lines only
/** display area circumference and preview of polygon **/
void RS_ActionInfoArea::display(bool forPreview){
    if (m_infoArea->size() < 1){
        return;
    }
    switch (m_infoArea->size()) {
        case 1: {
            if (showRefEntitiesOnPreview && forPreview) {
                previewRefSelectablePoint(m_infoArea->at(0));
            }
            break;
        }
        case 2: {
            if (forPreview) {
                previewLine(m_infoArea->at(0), m_infoArea->at(1));
                if (showRefEntitiesOnPreview) {
                    previewRefLine(m_infoArea->at(0), m_infoArea->at(1));
                    previewRefPoint(m_infoArea->at(0));
                    previewRefSelectablePoint(m_infoArea->at(1));
                }
            }
            break;
        }
        default: {
            QString const length = formatLinear(m_infoArea->getCircumference());
            double area = m_infoArea->getArea();
            if (forPreview) {
                for (int i = 0; i < m_infoArea->size(); i++) {
                    previewLine(m_infoArea->at(i), m_infoArea->at((i + 1) % m_infoArea->size()));
                    if (showRefEntitiesOnPreview) {
                        previewRefLine(m_infoArea->at(i), m_infoArea->at((i + 1) % m_infoArea->size()));
                    }
                }
                if (showRefEntitiesOnPreview) {
                    for (const RS_Vector& point: *m_infoArea) {
                        previewRefPoint(point);
                    }
                    previewRefSelectablePoint(m_infoArea->back());
                }
                if (infoCursorOverlayPrefs->enabled) {
                    QString msg = "\n";
                    msg.append(tr("Circumference: %1").arg(length));
                    msg.append("\n");
                    msg.append(tr("Area: %1 %2^2").arg(area).arg(RS_Units::unitToString(m_unit)));
                    appendInfoCursorZoneMessage(msg, 2, true);
                }
            }
            else {
                commandMessage("---");
                commandMessage(tr("Circumference: %1").arg(length));
                commandMessage(tr("Area: %1 %2^2").arg(area).arg(RS_Units::unitToString(m_unit)));
                commandMessage("");
            }
            break;
        }
    }
}

void RS_ActionInfoArea::mouseMoveEvent(QMouseEvent* e) {
    //RS_DEBUG->print("RS_ActionInfoArea::mouseMoveEvent begin");
    deletePreview();
    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    switch (status){
        case SetFirstPoint: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetNextPoint: {
            mouse = getSnapAngleAwarePoint(e, m_infoArea->back(), mouse, true);
            m_infoArea->push_back(mouse);
            display(true);
            m_infoArea->pop_back();
            break;
        }
        default:
            break;
    }
    drawPreview();
    //RS_DEBUG->print("RS_ActionInfoArea::mouseMoveEvent end");
}

void RS_ActionInfoArea::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
    if (status == SetNextPoint){
        snap = getSnapAngleAwarePoint(e, m_infoArea->back(), snap);
    }
    lastPointRequested = isControl(e);
    fireCoordinateEvent(snap);
}

void RS_ActionInfoArea::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *e) {
    trigger();
//    initPrevious(status);
}

void RS_ActionInfoArea::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    bool shouldComplete = m_infoArea->duplicated(mouse) || lastPointRequested;
    if (shouldComplete){
        m_infoArea->push_back(mouse);
        commandMessage(tr("Closing Point: %1").arg(formatVector(mouse)));
        trigger();
        return;
    }
    moveRelativeZero(mouse);

    m_infoArea->push_back(mouse);
    commandMessage(tr("Point: %1").arg(formatVector(mouse)));
    switch (status) {
        case SetFirstPoint: {
            setStatus(SetNextPoint);
            break;
        }
        case SetNextPoint: {
            display(true);
            break;
        }
        default:
            break;
    }
}

void RS_ActionInfoArea::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetFirstPoint:
            updateMouseWidgetTRCancel(tr("Specify first point of polygon"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetNextPoint: {
            bool enoughPointsForArea = m_infoArea->size() > 1;
            updateMouseWidgetTRCancel(tr("Specify next point of polygon"), enoughPointsForArea ? MOD_SHIFT_AND_CTRL_ANGLE(tr("Specify point and complete")) :
                                                                           MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionInfoArea::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
