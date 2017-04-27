#include "rs_actiondrawlinepolygon3.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_creation.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionDrawLinePolygonCenTan::Points {
    /** Center of polygon */
    RS_Vector center;
    /** Edge */
    RS_Vector corner;
};

RS_ActionDrawLinePolygonCenTan::RS_ActionDrawLinePolygonCenTan(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw Polygons (Center,Corner)", container, graphicView)
        , pPoints(new Points{})
        ,number(3)
        ,lastStatus(SetCenter)
{
    actionType=RS2::ActionDrawLinePolygonCenCor;
}

RS_ActionDrawLinePolygonCenTan::~RS_ActionDrawLinePolygonCenTan() = default;

void RS_ActionDrawLinePolygonCenTan::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();

    RS_Creation creation(container, graphicView);
    bool ok = creation.createPolygon3(pPoints->center, pPoints->corner, number);

    if (!ok) {
        RS_DEBUG->print("RS_ActionDrawLinePolygon::trigger:"
                        " No polygon added\n");
    }
}



void RS_ActionDrawLinePolygonCenTan::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLinePolygon::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
    case SetCenter:
        break;

    case SetTangent:
        if (pPoints->center.valid) {
            pPoints->corner = mouse;
            deletePreview();

            RS_Creation creation(preview.get(), nullptr, false);
            creation.createPolygon3(pPoints->center, pPoints->corner, number);

            drawPreview();
        }
        break;

    default:
        break;
    }
}



void RS_ActionDrawLinePolygonCenTan::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void RS_ActionDrawLinePolygonCenTan::coordinateEvent(RS_CoordinateEvent* e) {
    if (!e)  return;

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetCenter:
        pPoints->center = mouse;
        setStatus(SetTangent);
        graphicView->moveRelativeZero(mouse);
        break;

    case SetTangent:
        pPoints->corner = mouse;
        trigger();
        break;

    default:
        break;
    }
}

void RS_ActionDrawLinePolygonCenTan::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetCenter:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify center"),
                                            "");
        break;

    case SetTangent:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify a tangent"), "");
        break;

    case SetNumber:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter number:"), "");
        break;

    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void RS_ActionDrawLinePolygonCenTan::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}

void RS_ActionDrawLinePolygonCenTan::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}

void RS_ActionDrawLinePolygonCenTan::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetCenter:
    case SetTangent:
        if (checkCommand("number", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetNumber);
        }
        break;

    case SetNumber: {
            bool ok;
            int n = c.toInt(&ok);
            if (ok) {
                e->accept();
                if (n>0 && n<10000) {
                    number = n;
                } else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid number. "
                                                        "Try 1..9999"));
            } else
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    default:
        break;
    }
}



QStringList RS_ActionDrawLinePolygonCenTan::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetCenter:
    case SetTangent:
        cmd += command("number");
        break;
    default:
        break;
    }

    return cmd;
}



void RS_ActionDrawLinePolygonCenTan::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}
