/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_BevelOptions::destroy() {
    RS_SETTINGS->beginGroup("/Modify");
    RS_SETTINGS->writeEntry("/BevelLength1", leLength1->text());
    RS_SETTINGS->writeEntry("/BevelLength2", leLength2->text());
    RS_SETTINGS->writeEntry("/BevelTrim", (int)cbTrim->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_BevelOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionModifyBevel) {
        action = (RS_ActionModifyBevel*)a;

        QString sd1;
        QString sd2;
		QString st;
        if (update) {
            sd1 = QString("%1").arg(action->getLength1());
            sd2 = QString("%1").arg(action->getLength2());
            st = QString("%1").arg((int)action->isTrimOn());
        } else {
            RS_SETTINGS->beginGroup("/Modify");
            sd1 = RS_SETTINGS->readEntry("/BevelLength1", "1.0");
            sd2 = RS_SETTINGS->readEntry("/BevelLength2", "1.0");
            st = RS_SETTINGS->readEntry("/BevelTrim", "1");
            RS_SETTINGS->endGroup();
        }
		leLength1->setText(sd1);
		leLength2->setText(sd2);
    	cbTrim->setChecked(st=="1");
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_BevelOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_BevelOptions::updateData() {
    if (action!=NULL) {
        action->setTrim(cbTrim->isChecked());
        action->setLength1(RS_Math::eval(leLength1->text()));
        action->setLength2(RS_Math::eval(leLength2->text()));
    }
}
