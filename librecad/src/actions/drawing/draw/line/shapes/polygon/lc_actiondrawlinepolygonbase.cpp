#include "lc_actiondrawlinepolygonbase.h"
#include "qg_linepolygonoptions.h"
#include "rs_commandevent.h"

LC_ActionDrawLinePolygonBase::LC_ActionDrawLinePolygonBase( const char *name, RS_EntityContainer &container,
                                                            RS_GraphicView &graphicView,
                                                            RS2::ActionType actionType)
    :RS_PreviewActionInterface(name, container,graphicView,actionType),number(3){}

LC_ActionDrawLinePolygonBase::~LC_ActionDrawLinePolygonBase() = default;

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


LC_ActionOptionsWidget* LC_ActionDrawLinePolygonBase::createOptionsWidget(){
    return new QG_LinePolygonOptions();
}

RS2::CursorType LC_ActionDrawLinePolygonBase::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
