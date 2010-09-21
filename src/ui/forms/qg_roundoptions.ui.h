/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_RoundOptions::destroy() {
    RS_SETTINGS->beginGroup("/Modify");
    RS_SETTINGS->writeEntry("/RoundRadius", leRadius->text());
    RS_SETTINGS->writeEntry("/RoundTrim", (int)cbTrim->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_RoundOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionModifyRound) {
        action = (RS_ActionModifyRound*)a;

        QString sr;
	QString st;
        if (update) {
            sr = QString("%1").arg(action->getRadius());
            st = QString("%1").arg((int)action->isTrimOn());
        } else {
            RS_SETTINGS->beginGroup("/Modify");
            sr = RS_SETTINGS->readEntry("/RoundRadius", "1.0");
            st = RS_SETTINGS->readEntry("/RoundTrim", "1");
            RS_SETTINGS->endGroup();
        }
	leRadius->setText(sr);
    	cbTrim->setChecked(st=="1");
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LineParallelOptions::setAction: wrong action type");
        action = NULL;
    }
}


void QG_RoundOptions::updateData() {
    if (action!=NULL) {
        action->setTrim(cbTrim->isChecked());
        action->setRadius(RS_Math::eval(leRadius->text()));
    }
}
