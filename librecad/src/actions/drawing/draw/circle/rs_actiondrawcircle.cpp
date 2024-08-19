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

#include "rs_actiondrawcircle.h"
#include "rs_circle.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_preview.h"

RS_ActionDrawCircle::RS_ActionDrawCircle(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :LC_ActionDrawCircleBase("Draw circles",
                           container, graphicView)
    , data(std::make_unique<RS_CircleData>())
{
	actionType=RS2::ActionDrawCircle;
}

RS_ActionDrawCircle::~RS_ActionDrawCircle() = default;

void RS_ActionDrawCircle::reset() {
    data = std::make_unique<RS_CircleData>();
}

void RS_ActionDrawCircle::trigger() {
    RS_PreviewActionInterface::trigger();

    auto* circle = new RS_Circle(container,*data);
    circle->setLayerToActive();
    circle->setPenToActive();
    container->addEntity(circle);

    addToDocumentUndoable(circle);

    graphicView->redraw(RS2::RedrawDrawing);
    if (moveRelPointAtCenterAfterTrigger){
        moveRelativeZero(circle->getCenter());
    }

    setStatus(SetCenter);
    reset();

    RS_DEBUG->print("RS_ActionDrawCircle::trigger(): circle added: %lu",
                    circle->getId());
}

void RS_ActionDrawCircle::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawCircle::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
        case SetCenter: {
            data->center = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetRadius: {
            if (data->center.valid){
//                fixme - complete support
                //mouse = getFreeSnapAwarePoint(e, mouse);
                data->radius = data->center.distanceTo(mouse);
                deletePreview();
                previewCircle(*data);
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(data->center);
                    previewRefSelectablePoint(mouse);
                    previewRefLine(data->center, mouse);
                }
                drawPreview();
            }
            break;
        }
    }

    RS_DEBUG->print("RS_ActionDrawCircle::mouseMoveEvent end");
}

void RS_ActionDrawCircle::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetCenter:
            data->center = mouse;
            moveRelativeZero(mouse);
            setStatus(SetRadius);
            break;
        case SetRadius:
            if (data->center.valid){
                moveRelativeZero(mouse);
                data->radius = data->center.distanceTo(mouse);
                trigger();
            }
            //setStatus(SetCenter);
            break;
        default:
            break;
    }
}

bool RS_ActionDrawCircle::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetRadius: {
            bool ok = false;
            double r = RS_Math::eval(c, &ok);
            if (ok && r > RS_TOLERANCE){
                data->radius = r;
                accept = true;
                trigger();
            } else
                commandMessage(tr("Not a valid expression"));
        }
        default:
            break;
    }
    return accept;
}

void RS_ActionDrawCircle::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetCenter:
            updateMouseWidgetTRCancel(tr("Specify center"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetRadius:
            updateMouseWidgetTRBack(tr("Specify point on circle"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
