/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_LineRelAngleOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawLineRelAngle) {
        action = (RS_ActionDrawLineRelAngle*)a;
        if (action->hasFixedAngle()) {
            lAngle->setDisabled(true);
            leAngle->setDisabled(true);
        }

        QString sa;
        QString sl;

        // settings from action:
        if (update) {
            sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sl = QString("%1").arg(action->getLength());
        }
        // settings from config file:
        else {
            RS_SETTINGS->beginGroup("/Draw");
            if (!action->hasFixedAngle()) {
                sa = RS_SETTINGS->readEntry("/LineRelAngleAngle", "30.0");
            } else {
                sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            }
            sl = RS_SETTINGS->readEntry("/LineRelAngleLength", "10.0");
            RS_SETTINGS->endGroup();
        }

        leAngle->setText(sa);
        leLength->setText(sl);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LineRelAngleOptions::setAction: wrong action type");
        this->action = NULL;
    }
}

void QG_LineRelAngleOptions::destroy() {
    if (action!=NULL) {
        RS_SETTINGS->beginGroup("/Draw");
        if (!action->hasFixedAngle()) {
            RS_SETTINGS->writeEntry("/LineRelAngleAngle", 
				RS_Math::rad2deg(action->getAngle()));
        }
        RS_SETTINGS->writeEntry("/LineRelAngleLength", action->getLength());
        RS_SETTINGS->endGroup();
    }
}

void QG_LineRelAngleOptions::updateAngle(const QString& a) {
    if (action!=NULL && !action->hasFixedAngle()) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(a)));
    }
}

void QG_LineRelAngleOptions::updateLength(const QString& l) {
    if (action!=NULL) {
        action->setLength(RS_Math::eval(l));
    }
}
