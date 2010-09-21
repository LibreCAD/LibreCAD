/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_CircleOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/CircleRadius", leRadius->text());
    RS_SETTINGS->endGroup();
}

void QG_CircleOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawCircleCR) {
        action = (RS_ActionDrawCircleCR*)a;

        QString sr;
        if (update) {
            sr = QString("%1").arg(action->getRadius());
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sr = RS_SETTINGS->readEntry("/CircleRadius", "1.0");
            RS_SETTINGS->endGroup();
            action->setRadius(sr.toDouble());
        }
        leRadius->setText(sr);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CircleOptions::setAction: wrong action type");
        action = NULL;
    }

}


/*void QG_CircleOptions::setData(RS_CircleData* d) {
    data = d;
 
    RS_SETTINGS->beginGroup("/Draw");
    QString r = RS_SETTINGS->readEntry("/CircleRadius", "1.0");
    RS_SETTINGS->endGroup();
 
    leRadius->setText(r);
}*/

void QG_CircleOptions::updateRadius(const QString& r) {
    if (action!=NULL) {
        action->setRadius(RS_Math::eval(r));
    }
}
