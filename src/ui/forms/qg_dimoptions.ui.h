/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DimOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/DimLabel", leLabel->text());
    RS_SETTINGS->writeEntry("/DimTol1", leTol1->text());
    RS_SETTINGS->writeEntry("/DimTol2", leTol2->text());
    RS_SETTINGS->endGroup();
}

void QG_DimOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && RS_ActionDimension::isDimensionAction(a->rtti())) {
        action = (RS_ActionDimension*)a;
        
        QString st;
        QString stol1;
        QString stol2;
        bool diam;
        if (update) {
            st = action->getLabel();
            stol1 = action->getTol1();
            stol2 = action->getTol2();
            diam = action->getDiameter();
        } else {
            //st = "";
            RS_SETTINGS->beginGroup("/Draw");
            st = RS_SETTINGS->readEntry("/DimLabel", "");
            stol1 = RS_SETTINGS->readEntry("/DimTol1", "");
            stol2 = RS_SETTINGS->readEntry("/DimTol2", "");
            diam = (bool)RS_SETTINGS->readNumEntry("/DimDiameter", 0);
            RS_SETTINGS->endGroup();
        }
        leLabel->setText(st);
        leTol1->setText(stol1);
        leTol2->setText(stol2);
        bDiameter->setOn(diam);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_DimensionOptions::setAction: wrong action type");
        action = NULL;
    }
}



void QG_DimOptions::updateLabel() {
    if (action!=NULL) {
        action->setText("");
        action->setLabel(leLabel->text());
        action->setDiameter(bDiameter->isOn());
        action->setTol1(leTol1->text());
        action->setTol2(leTol2->text());
        
        action->setText(action->getText());
  }
}

void QG_DimOptions::insertSign(const QString& c) {
    leLabel->insert(c);
}


