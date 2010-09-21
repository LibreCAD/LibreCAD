/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_LineOptions::setAction(RS_ActionInterface* a) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawLine) {
        action = (RS_ActionDrawLine*)a;
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LineOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_LineOptions::close() {
    if (action!=NULL) {
        action->close();
    }
}

void QG_LineOptions::undo() {
    if (action!=NULL) {
        action->undo();
    }
}
