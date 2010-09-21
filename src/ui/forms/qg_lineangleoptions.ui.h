/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_LineAngleOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawLineAngle) {
        action = (RS_ActionDrawLineAngle*)a;
        if (action->hasFixedAngle()) {
            lAngle->hide();
            leAngle->hide();
        }

        QString sa;
        QString sl;
		int sp;

        // settings from action:
        if (update) {
            sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sl = QString("%1").arg(action->getLength());
			sp = action->getSnapPoint();
        }
        // settings from config file:
        else {
            RS_SETTINGS->beginGroup("/Draw");
            if (!action->hasFixedAngle()) {
                sa = RS_SETTINGS->readEntry("/LineAngleAngle", "30.0");
            } else {
                sa = QString("%1").arg(action->getAngle());
            }
            sl = RS_SETTINGS->readEntry("/LineAngleLength", "10.0");
            sp = RS_SETTINGS->readNumEntry("/LineAngleSnapPoint", 0);
            RS_SETTINGS->endGroup();
			action->setSnapPoint(sp);
        }

        leAngle->setText(sa);
        leLength->setText(sl);
		cbSnapPoint->setCurrentItem(sp);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_LineAngleOptions::setAction: wrong action type");
        this->action = NULL;
    }
}

void QG_LineAngleOptions::destroy() {
    if (action!=NULL) {
        RS_SETTINGS->beginGroup("/Draw");
        if (!action->hasFixedAngle()) {
            RS_SETTINGS->writeEntry("/LineAngleAngle", RS_Math::rad2deg(action->getAngle()));
        }
        RS_SETTINGS->writeEntry("/LineAngleLength", action->getLength());
        RS_SETTINGS->writeEntry("/LineAngleSnapPoint", action->getSnapPoint());
        RS_SETTINGS->endGroup();
    }
}

void QG_LineAngleOptions::updateAngle(const QString& a) {
    if (action!=NULL && !action->hasFixedAngle()) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(a)));
    }
}

void QG_LineAngleOptions::updateLength(const QString& l) {
    if (action!=NULL) {
        action->setLength(RS_Math::eval(l));
    }
}

void QG_LineAngleOptions::updateSnapPoint(int sp) {
    if (action!=NULL) {
        action->setSnapPoint(sp);
    }
}
