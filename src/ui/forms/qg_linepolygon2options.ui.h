/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_LinePolygon2Options::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/LinePolygon2Number", sbNumber->text());
    RS_SETTINGS->endGroup();
}

void QG_LinePolygon2Options::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawLinePolygon2) {
        action = (RS_ActionDrawLinePolygon2*)a;

        QString sn;
        if (update) {
            sn = QString("%1").arg(action->getNumber());
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sn = RS_SETTINGS->readEntry("/LinePolygon2Number", "3");
            RS_SETTINGS->endGroup();
        }
        sbNumber->setValue(sn.toInt());
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LinePolygon2Options::setAction: wrong action type");
        action = NULL;
    }

}

void QG_LinePolygon2Options::updateNumber(int n) {
    if (action!=NULL) {
        action->setNumber(n);
    }
}
