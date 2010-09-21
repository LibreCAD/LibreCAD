/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_LibraryInsertOptions::destroy() {
    RS_SETTINGS->beginGroup("/LibraryInsert");
    RS_SETTINGS->writeEntry("/LibraryInsertAngle", leAngle->text());
    RS_SETTINGS->writeEntry("/LibraryInsertFactor", leFactor->text());
    RS_SETTINGS->endGroup();
}

void QG_LibraryInsertOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionLibraryInsert) {
        action = (RS_ActionLibraryInsert*)a;

        QString sAngle;
        QString sFactor;
        if (update) {
            sAngle = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sFactor = QString("%1").arg(action->getFactor());
        } else {
            RS_SETTINGS->beginGroup("/LibraryInsert");
            sAngle = RS_SETTINGS->readEntry("/LibraryInsertAngle", "0.0");
            sFactor = RS_SETTINGS->readEntry("/LibraryInsertFactor", "1.0");
            RS_SETTINGS->endGroup();
        }
	leAngle->setText(sAngle);
	leFactor->setText(sFactor);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LibraryInsertOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_LibraryInsertOptions::updateData() {
    if (action!=NULL) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
        action->setFactor(RS_Math::eval(leFactor->text()));
    }
}
