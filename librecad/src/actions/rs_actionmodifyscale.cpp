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

#include "rs_actionmodifyscale.h"

#include <QAction>
#include <QMouseEvent>

#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_preview.h"

struct RS_ActionModifyScale::Points {
    RS_ScaleData data;
    RS_Vector sourcePoint;
    RS_Vector targetPoint;
};

namespace {

/**
 * @brief getFactor find the factor to scale source to target around reference
 *        target = reference + (source - reference) * factor
 * @param reference - the reference
 * @param source - the source
 * @param target - the target
 * @return double - factor
 */
double getFactor(double reference, double source, double target) {
    const double dxOld = source - reference;
    const double dxNew = target - reference;
    return (std::abs(dxOld) > RS_TOLERANCE && std::abs(dxNew)) ? dxNew/dxOld : 1.;
}

}

RS_ActionModifyScale::RS_ActionModifyScale(RS_EntityContainer& container,
                                           RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Scale Entities",
                                container, graphicView)
    , pPoints(std::make_unique<Points>())
{
    actionType=RS2::ActionModifyScale;
}

RS_ActionModifyScale::~RS_ActionModifyScale() = default;


void RS_ActionModifyScale::init(int status) {
    RS_ActionInterface::init(status);

}

void RS_ActionModifyScale::trigger() {

    RS_DEBUG->print("RS_ActionModifyScale::trigger()");
    deletePreview();
    if(pPoints->data.factor.valid){
        RS_Modification m(*container, graphicView);
        m.scale(pPoints->data);

        RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
    }
}



void RS_ActionModifyScale::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyScale::mouseMoveEvent begin");

    if (getStatus()!=ShowDialog) {

        switch (getStatus()) {
        case SetReferencePoint:
            pPoints->data.referencePoint = snapPoint(e);
            break;

        case SetSourcePoint:
            pPoints->sourcePoint = snapPoint(e);
            break;

        case SetTargetPoint:
            pPoints->targetPoint = getTargetPoint(e);
            showPreview();
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionModifyScale::mouseMoveEvent end");
}

RS_Vector RS_ActionModifyScale::getTargetPoint(QMouseEvent* e)
{
    if (!pPoints->data.isotropicScaling)
        return snapPoint(e);
    RS_Vector mouse = graphicView->toGraph(e->position());
    // project mouse to the line (center, source)
    RS_Line centerSourceLine{nullptr, {pPoints->data.referencePoint, pPoints->sourcePoint}};
    RS_Vector projected = centerSourceLine.getNearestPointOnEntity(mouse, false);
    snapPoint(projected, true);
    return projected;
}

void RS_ActionModifyScale::showPreview()
{
    deletePreview();
    preview->addSelectionFrom(*container);
    findFactor();

    // RS_Modification only considers selected
    for(auto* entity: *preview)
        entity->setSelected(true);

    RS_Modification m(*preview, graphicView);
    m.scale(pPoints->data);

    for(auto* entity: *preview)
        entity->setSelected(false);
    drawPreview();
}

void RS_ActionModifyScale::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        if (getStatus() != ShowDialog){
            RS_CoordinateEvent ce(snapPoint(e));
            coordinateEvent(&ce);
        }
    } else if (e->button()==Qt::RightButton && getStatus() != SetSourcePoint) {
        deletePreview();
        init(getStatus()-1);
    }
}

void RS_ActionModifyScale::coordinateEvent(RS_CoordinateEvent* e) {

    if (e==nullptr || getStatus() == ShowDialog) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();
    switch(getStatus()) {
    case SetReferencePoint:
    {
        setStatus(ShowDialog);
        pPoints->data.referencePoint = mouse;
        graphicView->setRelativeZero(mouse);
        if (RS_DIALOGFACTORY->requestScaleDialog(pPoints->data)) {
            if (!pPoints->data.toFindFactor) {
                trigger();
                finish();
            } else {
                if (pPoints->data.toFindFactor)
                    setStatus(SetSourcePoint);
                else
                    setStatus(SetReferencePoint);
            }
        }
    }
    break;

    case SetSourcePoint:
        if (mouse.squaredTo(pPoints->data.referencePoint) > RS_TOLERANCE2) {
            pPoints->sourcePoint = mouse;
            setStatus(SetTargetPoint);
        }
        break;

    case SetTargetPoint:
        if (mouse.squaredTo(pPoints->data.referencePoint) > RS_TOLERANCE2) {
            pPoints->targetPoint = mouse;
            trigger();
            finish();
        }
        break;
    default:
        break;
    }
}

void RS_ActionModifyScale::findFactor()
{
    auto& p0 = pPoints->data.referencePoint;
    auto& p1 = pPoints->sourcePoint;
    auto& p2 = pPoints->targetPoint;
    pPoints->data.factor.x = getFactor(p0.x, p1.x, p2.x);
    pPoints->data.factor.y = getFactor(p0.y, p1.y, p2.y);
    pPoints->data.factor.valid = true;
}

void RS_ActionModifyScale::updateMouseButtonHints() {
    switch (getStatus()) {
        /*case Select:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Pick entities to scale"),
                                           tr("Cancel"));
            break;*/
    case SetReferencePoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify scale center"),
                                            tr("Cancel"));
        break;
        // Find the scale factors to scale the pPoints->sourcePoint to pPoints->targetPoint
    case SetSourcePoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point"),
                                            tr("Cancel"));
        break;
    case SetTargetPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify target point"),
                                            tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void RS_ActionModifyScale::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
