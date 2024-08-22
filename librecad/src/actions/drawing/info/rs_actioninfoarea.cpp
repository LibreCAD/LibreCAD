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
    , ia(std::make_unique<RS_InfoArea>()){
    actionType=RS2::ActionInfoArea;
}

RS_ActionInfoArea::~RS_ActionInfoArea() = default;

void RS_ActionInfoArea::init(int status) {
    RS_PreviewActionInterface::init(status);

    if(status==SetFirstPoint){
        deletePreview();
        ia = std::make_unique<RS_InfoArea>();
    }

    //RS_DEBUG->print( "RS_ActionInfoArea::init: %d" ,status );
}

void RS_ActionInfoArea::trigger() {

    RS_DEBUG->print("RS_ActionInfoArea::trigger()");
    display();

    init(SetFirstPoint);
}
// fixme - sand - consider displaying information in EntityInfo widget
// fixme - sand - add area info to entity info widget for coordinates mode
//todo: we regenerate the whole preview, it's possible to generate needed lines only
/** display area circumference and preview of polygon **/
void RS_ActionInfoArea::display(){
    deletePreview();
    if (ia->size() < 1){
        return;
    }
    switch (ia->size()) {
        case 1: {
            if (showRefEntitiesOnPreview) {
                previewRefSelectablePoint(ia->at(0));
            }
            break;
        }
        case 2: {
            previewLine(ia->at(0), ia->at(1));
            if (showRefEntitiesOnPreview) {
                previewRefLine(ia->at(0), ia->at(1));
                previewRefPoint(ia->at(0));
                previewRefSelectablePoint(ia->at(1));
            }
            break;
        }
        default: {
            for (int i = 0; i < ia->size(); i++) {
                previewLine(ia->at(i), ia->at((i + 1) % ia->size()));
                if (showRefEntitiesOnPreview) {
                    previewRefLine(ia->at(i), ia->at((i + 1) % ia->size()));
                }
            }
            if (showRefEntitiesOnPreview) {
                for (int i = 0; i < ia->size() - 1; i++) {
                    previewRefPoint(ia->at(i));
                }
                previewRefSelectablePoint(ia->at(ia->size() - 1));
            }

            QString const linear = RS_Units::formatLinear(ia->getCircumference(),
                                                          graphic->getUnit(),
                                                          graphic->getLinearFormat(),
                                                          graphic->getLinearPrecision());
            commandMessage("---\n");
            commandMessage(tr("Circumference: %1").arg(linear));
            commandMessage(tr("Area: %1 %2^2").arg(ia->getArea())
                               .arg(RS_Units::unitToString(graphic->getUnit())));
            break;
        }
    }
    drawPreview();
}

void RS_ActionInfoArea::mouseMoveEvent(QMouseEvent* e) {
    //RS_DEBUG->print("RS_ActionInfoArea::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    switch (status){
        case SetFirstPoint: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetNextPoint: {
            mouse = getSnapAngleAwarePoint(e, ia->back(), mouse, true);
            ia->push_back(mouse);
            display();
            ia->pop_back();
            break;
        }
        default:
            break;
    }
    //RS_DEBUG->print("RS_ActionInfoArea::mouseMoveEvent end");
}

void RS_ActionInfoArea::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
    if (status == SetNextPoint){
        snap = getSnapAngleAwarePoint(e, ia->back(), snap);
    }
    fireCoordinateEvent(snap);
}

void RS_ActionInfoArea::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    initPrevious(status);
}

void RS_ActionInfoArea::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    if (ia->duplicated(mouse)){
        ia->push_back(mouse);
        commandMessage(tr("Closing Point: %1/%2").arg(mouse.x).arg(mouse.y));
        trigger();
        return;
    }
    moveRelativeZero(mouse);

    ia->push_back(mouse);
    commandMessage(tr("Point: %1/%2").arg(mouse.x).arg(mouse.y));
    switch (status) {
        case SetFirstPoint: {
            setStatus(SetNextPoint);
            break;
        }
        case SetNextPoint: {
            display();
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
        case SetNextPoint:
            updateMouseWidgetTRCancel(tr("Specify next point of polygon"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionInfoArea::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
