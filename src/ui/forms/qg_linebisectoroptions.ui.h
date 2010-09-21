/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_LineBisectorOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/LineBisectorLength", leLength->text());
    RS_SETTINGS->writeEntry("/LineBisectorNumber", sbNumber->text());
    RS_SETTINGS->endGroup();
}

void QG_LineBisectorOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawLineBisector) {
        action = (RS_ActionDrawLineBisector*)a;

        QString sl;
        QString sn;
        if (update) {
            sl = QString("%1").arg(action->getLength());
            sn = QString("%1").arg(action->getNumber());
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sl = RS_SETTINGS->readEntry("/LineBisectorLength", "1.0");
            sn = RS_SETTINGS->readEntry("/LineBisectorNumber", "1");
            RS_SETTINGS->endGroup();
        }
        leLength->setText(sl);
        sbNumber->setValue(sn.toInt());
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LineBisectorOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_LineBisectorOptions::updateLength(const QString& l) {
    if (action!=NULL) {
        action->setLength(RS_Math::eval(l));
    }
}

void QG_LineBisectorOptions::updateNumber(int n) {
    if (action!=NULL) {
        action->setNumber(n);
    }
}
