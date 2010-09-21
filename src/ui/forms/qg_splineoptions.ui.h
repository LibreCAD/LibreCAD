/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_SplineOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/SplineDegree", cbDegree->currentText().toInt());
    RS_SETTINGS->writeEntry("/SplineClosed", (int)cbClosed->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_SplineOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawSpline) {
        action = (RS_ActionDrawSpline*)a;
        
        int degree;
        bool closed;
         if (update) {
            degree = action->getDegree();
            closed = action->isClosed();
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            degree = RS_SETTINGS->readNumEntry("/SplineDegree", 3);
            closed = RS_SETTINGS->readNumEntry("/SplineClosed", 0);
            RS_SETTINGS->endGroup();
            action->setDegree(degree);
            action->setClosed(closed);
        }
        cbDegree->setCurrentText(QString("%1").arg(degree));
        cbClosed->setChecked(closed);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_SplineOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_SplineOptions::setClosed(bool c) {
    if (action!=NULL) {
        action->setClosed(c);
    }
}

void QG_SplineOptions::undo() {
    if (action!=NULL) {
        action->undo();
    }
}

void QG_SplineOptions::setDegree(const QString& deg) {
    if (action!=NULL) {
        action->setDegree(deg.toInt());
    }
}
