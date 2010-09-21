/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_ArcOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/ArcReversed", (int)rbNeg->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_ArcOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawArc) {
        action = (RS_ActionDrawArc*)a;

        bool reversed;
        if (update) {
            reversed = action->isReversed();
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            reversed = RS_SETTINGS->readNumEntry("/ArcReversed", 0);
            RS_SETTINGS->endGroup();
            action->setReversed(reversed);
        }
        rbNeg->setChecked(reversed);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_ArcOptions::setAction: wrong action type");
        action = NULL;
    }

}


/*void QG_ArcOptions::init() {
    data = NULL;
    RS_SETTINGS->beginGroup("/Draw");
    bool reversed = RS_SETTINGS->readNumEntry("/ArcReversed", 0);
    RS_SETTINGS->endGroup();
    
    rbNeg->setChecked(reversed);
}*/



/*void QG_ArcOptions::setData(RS_ArcData* d) {
    data = d;
    updateDirection(false);
}*/

void QG_ArcOptions::updateDirection(bool /*pos*/) {
    if (action!=NULL) {
        action->setReversed(!(rbPos->isChecked()));
    }
}
