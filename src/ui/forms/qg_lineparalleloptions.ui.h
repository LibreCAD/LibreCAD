/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_LineParallelOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/LineParallelDistance", leDist->text());
    RS_SETTINGS->writeEntry("/LineParallelNumber", sbNumber->text());
    RS_SETTINGS->endGroup();
}

void QG_LineParallelOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawLineParallel) {
        action = (RS_ActionDrawLineParallel*)a;

        QString sd;
        QString sn;
        if (update) {
            sd = QString("%1").arg(action->getDistance());
            sn = QString("%1").arg(action->getNumber());
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sd = RS_SETTINGS->readEntry("/LineParallelDistance", "1.0");
            sn = RS_SETTINGS->readEntry("/LineParallelNumber", "1");
            RS_SETTINGS->endGroup();
        }
        leDist->setText(sd);
        sbNumber->setValue(sn.toInt());
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LineParallelOptions::setAction: wrong action type");
        action = NULL;
    }

}

void QG_LineParallelOptions::updateDist(const QString& d) {
    if (action!=NULL) {
        action->setDistance(RS_Math::eval(d));
    }
}

void QG_LineParallelOptions::updateNumber(int n) {
    if (action!=NULL) {
        action->setNumber(n);
    }
}
