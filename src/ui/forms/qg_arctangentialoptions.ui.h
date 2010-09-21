/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_ArcTangentialOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/ArcTangentialRadius", leRadius->text());    
    RS_SETTINGS->endGroup();
}

void QG_ArcTangentialOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawArcTangential) {
        action = (RS_ActionDrawArcTangential*)a;

        QString sr;
        if (update) {
            sr = QString("%1").arg(action->getRadius());
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sr = RS_SETTINGS->readEntry("/ArcTangentialRadius", "1.0");
            RS_SETTINGS->endGroup();
            action->setRadius(sr.toDouble());
        }
        leRadius->setText(sr);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_ArcTangentialOptions::setAction: wrong action type");
        action = NULL;
    }

}


/*void QG_ArcTangentialOptions::init() {
    data = NULL;
    RS_SETTINGS->beginGroup("/Draw");
    bool reversed = RS_SETTINGS->readNumEntry("/ArcReversed", 0);
    RS_SETTINGS->endGroup();
    
    rbNeg->setChecked(reversed);
}*/



/*void QG_ArcTangentialOptions::setData(RS_ArcData* d) {
    data = d;
    updateDirection(false);
}*/

void QG_ArcTangentialOptions::updateRadius(const QString& s) {
    if (action!=NULL) {
        action->setRadius(RS_Math::eval(s));
    }
}
