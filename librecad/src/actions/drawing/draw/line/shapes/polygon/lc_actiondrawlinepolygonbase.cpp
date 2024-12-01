#include "rs_math.h"
#include "rs_actiondrawlinepolygon2.h"
#include "rs_actioninterface.h"
#include "rs_debug.h"
#include "rs_preview.h"
#include "rs_coordinateevent.h"
#include "rs_point.h"
#include "rs_creation.h"
#include "rs_graphicview.h"
#include "rs_dialogfactory.h"
#include <QMouseEvent>
#include "lc_actiondrawlinepolygonbase.h"
#include "qg_linepolygonoptions.h"
#include "rs_commandevent.h"

LC_ActionDrawLinePolygonBase::LC_ActionDrawLinePolygonBase( const char *name, RS_EntityContainer &container,
                                                            RS_GraphicView &graphicView,
                                                            RS2::ActionType actionType)
    :RS_PreviewActionInterface(name, container,graphicView,actionType),number(3), pPoints(std::make_unique<Points>()), lastStatus(SetPoint1){}

LC_ActionDrawLinePolygonBase::~LC_ActionDrawLinePolygonBase() = default;

void LC_ActionDrawLinePolygonBase::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLinePolygon2::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
        case SetPoint1: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetPoint2: {
            deletePreview();
            if (pPoints->point1.valid){
                mouse = getSnapAngleAwarePoint(e, pPoints->point1, mouse, true);
                previewPolygon(mouse);
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->point1);
                    previewRefLine(pPoints->point1,mouse);
                    previewRefSelectablePoint(mouse);
                    previewAdditionalReferences();
                }
            }
            drawPreview();
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawLinePolygonBase::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector coord = snapPoint(e);
    if (status ==SetPoint2){
        coord = getSnapAngleAwarePoint(e, pPoints->point1, coord);
    }
    fireCoordinateEvent(coord);
}

void LC_ActionDrawLinePolygonBase::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawLinePolygonBase::onCoordinateEvent(int status,  [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPoint1: {
            pPoints->point1 = mouse;
            setStatus(SetPoint2);
            moveRelativeZero(mouse);
            break;
        }
        case SetPoint2: {
            pPoints->point2 = mouse;
            trigger();
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawLinePolygonBase::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetPoint1:
        case SetPoint2: {
            if (checkCommand("number", c)){
                deletePreview();
                lastStatus = (Status) status;
                setStatus(SetNumber);
                accept = true;
            }
            break;
        }
        case SetNumber: {
            accept = parseNumber(c);
            if (accept){
                updateOptions();
                setStatus(lastStatus);
            }
            break;
        }
        default:
            break;
    }
    return accept;
}

bool LC_ActionDrawLinePolygonBase::parseNumber(const QString &c){
    bool ok;
    int n = c.toInt(&ok);
    if (ok){
        if (n > 0 && n < 10000){ // fixme - check range to conform to UI
            number = n;
        } else
            commandMessage(tr("Not a valid number. Try 1..9999"));
    } else {
        commandMessage(tr("Not a valid expression"));
    }
    return ok;
}

QStringList LC_ActionDrawLinePolygonBase::getAvailableCommands() {
    QStringList cmd;
    switch (getStatus()) {
        case SetPoint1:
        case SetPoint2:
            cmd += command("number");
            break;
        default:
            break;
    }
    return cmd;
}

void LC_ActionDrawLinePolygonBase::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetPoint1:
            updateMouseWidgetTRCancel(getPoint1Hint(), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(getPoint2Hint(), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetNumber:
            updateMouseWidget(tr("Enter number:"),"");
            break;
        default:
            updateMouseWidget();
            break;
    }
}

QString LC_ActionDrawLinePolygonBase::getPoint1Hint() const { return tr("Specify center"); }

RS2::CursorType LC_ActionDrawLinePolygonBase::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

LC_ActionOptionsWidget* LC_ActionDrawLinePolygonBase::createOptionsWidget(){
    return new QG_LinePolygonOptions();
}
