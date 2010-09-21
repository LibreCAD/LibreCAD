/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_PrintPreviewOptions::init() {
    updateDisabled = false;
    imperialScales
    << "1\" = 1\""
    << "1\" = 2\""
    << "1\" = 4\""
    << "1\" = 8\""
    << "1\" = 16\""
    << "1\" = 32\""
    << "1\" = 64\""
    << "1\" = 128\""
    << "1\" = 256\"";

    metricScales
    << "1:1" << "1:2" << "1:5" << "1:10"
    << "1:20" << "1:25" << "1:50" << "1:75" << "1:100"
    << "1:125" << "1:150" << "1:175" << "1:200"
    << "1:250" << "1:500" << "1:750" << "1:1000"
    << "1:2500" << "1:5000" << "1:7500" << "1:10000"
    << "1:25000" << "1:50000" << "1:75000" << "1:100000"
    << "2:1" << "5:1" << "10:1"
    << "20:1" << "25:1" << "50:1" << "75:1" << "100:1"
    << "125:1" << "150:1" << "175:1" << "200:1"
    << "250:1" << "500:1" << "750:1" << "1000:1"
    << "2500:1" << "5000:1" << "7500:1" << "10000:1"
    << "25000:1" << "50000:1" << "75000:1" << "100000:1";   
    
}

void QG_PrintPreviewOptions::destroy() {
    /*
    RS_SETTINGS->beginGroup("/PrintPreview");
    RS_SETTINGS->writeEntry("/PrintPreviewAngle", leAngle->text());
    RS_SETTINGS->writeEntry("/PrintPreviewFactor", leFactor->text());
    RS_SETTINGS->endGroup();
    */
}

void QG_PrintPreviewOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionPrintPreview) {
        action = (RS_ActionPrintPreview*)a;

        updateDisabled = true;
        RS2::Unit u = action->getUnit();
        if (u==RS2::Inch) {
            cbScale->insertStringList(imperialScales);
        } else {
            cbScale->insertStringList(metricScales);
        }
        
        //if (update) {
        QString s;
        s.setNum(action->getScale());
        cbScale->setCurrentText(s);
    //}
        
        updateDisabled = false;

        /*
        QString sAngle;
        QString sFactor;
        if (update) {
            sAngle = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sFactor = QString("%1").arg(action->getFactor());
    } else {
            RS_SETTINGS->beginGroup("/PrintPreview");
            sAngle = RS_SETTINGS->readEntry("/PrintPreviewAngle", "0.0");
            sFactor = RS_SETTINGS->readEntry("/PrintPreviewFactor", "1.0");
            RS_SETTINGS->endGroup();
    }
        leAngle->setText(sAngle);
        leFactor->setText(sFactor);
        updateData();
        */
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_PrintPreviewOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_PrintPreviewOptions::updateData() {
    if (action!=NULL) {
        /*
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
        action->setFactor(RS_Math::eval(leFactor->text()));
        */
    }
}

void QG_PrintPreviewOptions::center() {
    if (action!=NULL) {
        action->center();
    }
}

void QG_PrintPreviewOptions::setBlackWhite(bool on) {
    if (action!=NULL) {
        action->setBlackWhite(on);
    }
}

void QG_PrintPreviewOptions::fit() {
    if (action!=NULL) {
        action->fit();
    }
}

void QG_PrintPreviewOptions::scale(const QString& s) {
    if (updateDisabled) {
        return;
    }
    
    if (s.contains(':')) {
        bool ok1 = false;
        bool ok2 = false;
        int i = s.find(':');
        double n = s.left(i).toDouble(&ok1);
        double d = s.mid(i+1).toDouble(&ok2);
        if (ok1 && ok2 && d>1.0e-6 && n>0.0) {
            action->setScale(n/d);
        }
    } else if (s.contains('=')) {
        bool ok = false;
        int i = s.find('=');
        double d = s.mid(i+2, s.length()-i-3).toDouble(&ok);
        if (ok && d>1.0e-6) {
            action->setScale(1.0/d);
        }
    } else {
        bool ok = false;
        double f = RS_Math::eval(s, &ok);
        if (ok) {
            action->setScale(f);
        }
    }
}

