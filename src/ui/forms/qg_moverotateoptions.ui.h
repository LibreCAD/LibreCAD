/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_MoveRotateOptions::destroy() {
    RS_SETTINGS->beginGroup("/Modify");
    RS_SETTINGS->writeEntry("/MoveRotate", leAngle->text());
    RS_SETTINGS->endGroup();
}


void QG_MoveRotateOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionModifyMoveRotate) {
        action = (RS_ActionModifyMoveRotate*)a;

        QString sa;
        if (update) {
            sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
        } else {
            RS_SETTINGS->beginGroup("/Modify");
            sa = RS_SETTINGS->readEntry("/MoveRotate", "30");
            RS_SETTINGS->endGroup();
            action->setAngle(RS_Math::deg2rad(sa.toDouble()));
        }
        leAngle->setText(sa);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CircleOptions::setAction: wrong action type");
        action = NULL;
    }

}

void QG_MoveRotateOptions::updateAngle(const QString& a) {
    if (action!=NULL) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(a)));
    }
}
