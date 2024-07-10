#include "lc_actiondrawlinepolygonbase.h"
#include "qg_linepolygonoptions.h"
#include "rs_commandevent.h"

LC_ActionDrawLinePolygonBase::LC_ActionDrawLinePolygonBase( const char *name, RS_EntityContainer &container,
                                                            RS_GraphicView &graphicView,
                                                            RS2::ActionType actionType)
    :RS_PreviewActionInterface(name, container,graphicView,actionType),number(3){}

LC_ActionDrawLinePolygonBase::~LC_ActionDrawLinePolygonBase() = default;

void LC_ActionDrawLinePolygonBase::parseNumber(RS_CommandEvent *e, const QString &c){
    int result;
    bool ok;
    int n = c.toInt(&ok);
    if (ok){
        e->accept();
        if (n > 0 && n < 10000){ // fixme - check range to conform to UI
            result = n;
        } else
            commandMessageTR("Not a valid number. Try 1..9999");
    } else {
        commandMessageTR("Not a valid expression");
    }
    number = result;
}


void LC_ActionDrawLinePolygonBase::createOptionsWidget(){
    m_optionWidget = std::make_unique<QG_LinePolygonOptions>();
}

RS2::CursorType LC_ActionDrawLinePolygonBase::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
