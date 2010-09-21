/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_ImageOptionsDialog::init() {
    graphicSize = RS_Vector(0.0,0.0);
    updateEnabled = true;
    
    RS_SETTINGS->beginGroup("/ExportImage");
    leWidth->setText(RS_SETTINGS->readEntry("/Width", "640"));
    leHeight->setText(RS_SETTINGS->readEntry("/Height", "480"));
    if (RS_SETTINGS->readEntry("/BlackBackground", "0")=="1") {
        rbBlack->setChecked(true);
    }
    /*if (RS_SETTINGS->readEntry("/Blackwhite", "0")=="1") {
        rbBlackwhite->setChecked(true);
    }*/
    RS_SETTINGS->endGroup();
}

void QG_ImageOptionsDialog::setGraphicSize(const RS_Vector& s) {
    graphicSize = s;
}

void QG_ImageOptionsDialog::ok() {
    RS_SETTINGS->beginGroup("/ExportImage");
    RS_SETTINGS->writeEntry("/Width", leWidth->text());
    RS_SETTINGS->writeEntry("/Height", leHeight->text());
    RS_SETTINGS->writeEntry("/BlackBackground", (int)rbBlack->isChecked());
    //RS_SETTINGS->writeEntry("/Blackwhite", (int)rbBlackwhite->isChecked());
    RS_SETTINGS->endGroup();
    
    accept();
}

void QG_ImageOptionsDialog::sizeChanged() {
    if (updateEnabled) {
    updateEnabled = false;
    cbResolution->setCurrentText("auto");
    updateEnabled = true;
    }
}

void  QG_ImageOptionsDialog::resolutionChanged() {
    if (updateEnabled) {
    updateEnabled = false;
    bool ok = false;
    double res = RS_Math::eval(cbResolution->currentText(), &ok);
    if (!ok) {
        res = 1.0;
    }
    int w = RS_Math::round(res * graphicSize.x);
    int h = RS_Math::round(res * graphicSize.y);
    leWidth->setText(QString("%1").arg(w));
    leHeight->setText(QString("%1").arg(h));
    updateEnabled = true;
    }
}

QSize QG_ImageOptionsDialog::getSize() {
    return QSize(RS_Math::round(RS_Math::eval(leWidth->text())), 
                 RS_Math::round(RS_Math::eval(leHeight->text())));
}

bool QG_ImageOptionsDialog::isBackgroundBlack() {
    return rbBlack->isChecked();
}

/*bool QG_ImageOptionsDialog::isBlackwhite() {
    return rbBlackwhite->isChecked();
}*/
