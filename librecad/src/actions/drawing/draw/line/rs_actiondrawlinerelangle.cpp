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

#include "rs_actiondrawlinerelangle.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "qg_linerelangleoptions.h"
#include "rs_previewactioninterface.h"

namespace {

//list of entity types supported by current action
const auto enTypeList = EntityTypeList{RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle,RS2::EntityEllipse};
}

RS_ActionDrawLineRelAngle::RS_ActionDrawLineRelAngle(
        RS_EntityContainer& container,
        RS_GraphicView& graphicView,
        double ang,
        bool fixedAngle)
    :RS_PreviewActionInterface("Draw Lines with relative angles",
                               container, graphicView)
    , pos(std::make_unique<RS_Vector>())
    , fixedAngle(fixedAngle){
    angle = RS_Math::rad2deg(ang);
}

RS_ActionDrawLineRelAngle::~RS_ActionDrawLineRelAngle() = default;


RS2::ActionType RS_ActionDrawLineRelAngle::rtti() const{
    if( fixedAngle &&
            RS_Math::getAngleDifference(RS_Math::deg2rad(angle), M_PI_2) < RS_TOLERANCE_ANGLE)
        return RS2::ActionDrawLineOrthogonal;
    else
        return RS2::ActionDrawLineRelAngle;
}

void RS_ActionDrawLineRelAngle::finish(bool updateTB) {
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineRelAngle::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();

    RS_Creation creation(container, graphicView);
    double angleRad = RS_Math::deg2rad(angle);
    RS_Line* line = creation.createLineRelAngle(*pos,
                                                entity,
                                                angleRad,
                                                length);
    if (line == nullptr)
        LC_LOG(RS_Debug::D_ERROR)<<"RS_ActionDrawLineRelAngle::"<<__func__<<"(): cannot create line";

}

void RS_ActionDrawLineRelAngle::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDrawLineRelAngle::mouseMoveEvent begin");
    deleteHighlights();
    switch (getStatus()) {
        case SetEntity: {
            entity = catchEntity(e, enTypeList, RS2::ResolveAll);
            if (entity != nullptr){
                highlightHover(entity);
            }
            break;
        }
        case SetPos: {
            highlightSelected(entity);
            //length = graphicView->toGraphDX(graphicView->getWidth());
            //RS_Vector mouse = snapPoint(e);
            RS_Vector snap = snapPoint(e);
            *pos = getRelZeroAwarePoint(e, snap);

            /*RS_Creation creation(nullptr, nullptr);
                RS_Line* l = creation.createLineRelAngle(mouse,
                             entity,
                             angle,
                             length);*/

            deletePreview();

            RS_Creation creation(preview.get(), nullptr, false);
            double angleRad = RS_Math::deg2rad(angle);
            RS_Line* lineToCreate = creation.createLineRelAngle(*pos,entity, angleRad, length);

            if (showRefEntitiesOnPreview) {
                if (lineToCreate != nullptr) {
                    auto const vp = entity->getNearestPointOnEntity(*pos, false);
                    previewRefPoint(vp);
                    previewRefPoint(lineToCreate->getEndpoint());
                }
            }

            drawPreview();
            break;
        }
        default:
            break;
    }
    drawHighlights();

    RS_DEBUG->print("RS_ActionDrawLineRelAngle::mouseMoveEvent end");
}

void RS_ActionDrawLineRelAngle::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case SetEntity: {
            RS_Entity *en = catchEntity(e, enTypeList, RS2::ResolveAll);
            if (en != nullptr){
                entity = en;
                setStatus(SetPos);
            }
            break;
        }
        case SetPos: {
            const RS_Vector& snap = snapPoint(e);
            RS_Vector position = getRelZeroAwarePoint(e, snap);
            fireCoordinateEvent(position);
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineRelAngle::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawLineRelAngle::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &coord) {
    switch (status) {
        case SetPos: {
            *pos = coord;
            trigger();
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDrawLineRelAngle::doProcessCommand(int status, const QString &c) {
    bool accept = false;

    switch (status) {
        case SetEntity:
        case SetPos: {
            if (!fixedAngle && checkCommand("angle", c)){
                deletePreview();
                setStatus(SetAngle);
                accept = true;
            } else if (checkCommand("length", c)){
                deletePreview();
                setStatus(SetLength);
                accept = true;
            }
            break;
        }
        case SetAngle: {
            bool ok = false;
            double a = RS_Math::eval(c, &ok);
            if (ok){
                accept = true;
                angle = RS_Math::deg2rad(a);
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(SetPos);
            break;
        }
        case SetLength: {
            bool ok = false;
            double l = RS_Math::eval(c, &ok);
            if (ok){
                length = l;
                accept = true;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(SetPos);
            break;
        }
        default:
            break;
    }
    return accept;
}

QStringList RS_ActionDrawLineRelAngle::getAvailableCommands(){
    QStringList cmd;

    switch (getStatus()) {
        case SetPos:
        case SetLength: {
            if (!fixedAngle){
                cmd += command("angle");
            }
            cmd += command("length");
            break;
        }
        default:
            break;
    }

    return cmd;
}

void RS_ActionDrawLineRelAngle::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity:
            updateMouseWidgetTRCancel(tr("Select base entity"));
            break;
        case SetPos:
            updateMouseWidgetTRBack(tr("Specify position"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawLineRelAngle::doGetMouseCursor([[maybe_unused]] int status){
    switch (status) {
        case SetEntity:
            return RS2::SelectCursor;
        case SetPos:
            return RS2::CadCursor;
        default:
            return RS2::NoCursorChange;
    }
}

LC_ActionOptionsWidget* RS_ActionDrawLineRelAngle::createOptionsWidget(){
    return new QG_LineRelAngleOptions();
}
