/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DimLinearOptions::destroy() {
    RS_SETTINGS->beginGroup("/Dimension");
    RS_SETTINGS->writeEntry("/Angle", leAngle->text());
    RS_SETTINGS->endGroup();
}

void QG_DimLinearOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDimLinear) {
        action = (RS_ActionDimLinear*)a;

        QString sa;
        if (update) {
            sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
        } else {
            RS_SETTINGS->beginGroup("/Dimension");
            sa = RS_SETTINGS->readEntry("/Angle", "0.0");
            RS_SETTINGS->endGroup();
        }
        leAngle->setText(sa);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_DimLinearOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_DimLinearOptions::updateAngle(const QString & a) {
    if (action!=NULL) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(a)));
    }
}

void QG_DimLinearOptions::setHor() {
    leAngle->setText("0");
}

void QG_DimLinearOptions::setVer() {
    leAngle->setText("90");
}
