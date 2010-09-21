/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_ImageOptions::destroy() {
    RS_SETTINGS->beginGroup("/Image");
    RS_SETTINGS->writeEntry("/ImageAngle", leAngle->text());
    RS_SETTINGS->writeEntry("/ImageFactor", leFactor->text());
    RS_SETTINGS->endGroup();
}

void QG_ImageOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawImage) {
        action = (RS_ActionDrawImage*)a;

        QString sAngle;
        QString sFactor;
        if (update) {
            sAngle = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sFactor = QString("%1").arg(action->getFactor());
        } else {
            RS_SETTINGS->beginGroup("/Image");
            sAngle = RS_SETTINGS->readEntry("/ImageAngle", "0.0");
            sFactor = RS_SETTINGS->readEntry("/ImageFactor", "1.0");
            RS_SETTINGS->endGroup();
        }
	leAngle->setText(sAngle);
	leFactor->setText(sFactor);
        updateData();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_ImageOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_ImageOptions::updateData() {
    if (action!=NULL) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
        action->setFactor(RS_Math::eval(leFactor->text()));
    }
}
