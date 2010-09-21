/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_TrimAmountOptions::destroy() {
    RS_SETTINGS->beginGroup("/Modify");
    RS_SETTINGS->writeEntry("/TrimAmount", leDist->text());
    RS_SETTINGS->endGroup();
}

void QG_TrimAmountOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionModifyTrimAmount) {
        action = (RS_ActionModifyTrimAmount*)a;

        QString sd;
        // settings from action:
        if (update) {
            sd = QString("%1").arg(action->getDistance());
        }
        // settings from config file:
        else {
            RS_SETTINGS->beginGroup("/Modify");
            sd = RS_SETTINGS->readEntry("/TrimAmount", "1.0");
            RS_SETTINGS->endGroup();
        }

        leDist->setText(sd);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_ModifyTrimAmountOptions::setAction: wrong action type");
        this->action = NULL;
    }
}

void QG_TrimAmountOptions::updateDist(const QString& d) {
    if (action!=NULL) {
        action->setDistance(RS_Math::eval(d, 1.0));
    }
}
