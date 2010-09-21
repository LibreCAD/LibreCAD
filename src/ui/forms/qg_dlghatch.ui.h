//Added by qt3to4:
#include <QResizeEvent>
#include <QShowEvent>
/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DlgHatch::init() {
    pattern=NULL;
    hatch = NULL;
    isNew = false;

    preview = new RS_EntityContainer();
    gvPreview->setContainer(preview);
    gvPreview->setBorders(15,15,15,15);

    cbPattern->init();

}

void QG_DlgHatch::polish() {
    QDialog::polish();
    gvPreview->zoomAuto();
}

void QG_DlgHatch::showEvent ( QShowEvent * e) {
    QDialog::showEvent(e);
    gvPreview->zoomAuto();
}

void QG_DlgHatch::destroy() {
    if (isNew) {
        RS_SETTINGS->beginGroup("/Draw");
        RS_SETTINGS->writeEntry("/HatchSolid", (int)cbSolid->isChecked());
        RS_SETTINGS->writeEntry("/HatchPattern", cbPattern->currentText());
        RS_SETTINGS->writeEntry("/HatchScale", leScale->text());
        RS_SETTINGS->writeEntry("/HatchAngle", leAngle->text());
        RS_SETTINGS->writeEntry("/HatchPreview",
                                (int)cbEnablePreview->isChecked());
        RS_SETTINGS->endGroup();
    }
	delete preview;
}


void QG_DlgHatch::setHatch(RS_Hatch& h, bool isNew) {
    hatch = &h;
    this->isNew = isNew;
    
    RS_SETTINGS->beginGroup("/Draw");
    QString enablePrev = RS_SETTINGS->readEntry("/HatchPreview", "0");
    RS_SETTINGS->endGroup();
    
    cbEnablePreview->setChecked(enablePrev=="1");

    // read defaults from config file:
    if (isNew) {
        RS_SETTINGS->beginGroup("/Draw");
        QString solid = RS_SETTINGS->readEntry("/HatchSolid", "0");
        QString pat = RS_SETTINGS->readEntry("/HatchPattern", "ANSI31");
        QString scale = RS_SETTINGS->readEntry("/HatchScale", "1.0");
        QString angle = RS_SETTINGS->readEntry("/HatchAngle", "0.0");
        RS_SETTINGS->endGroup();

        cbSolid->setChecked(solid=="1");
        setPattern(pat);
        leScale->setText(scale);
        leAngle->setText(angle);
    }
    // initialize dialog based on given hatch:
    else {
        cbSolid->setChecked(hatch->isSolid());
        setPattern(hatch->getPattern());
        QString s;
        s.setNum(hatch->getScale());
        leScale->setText(s);
        s.setNum(RS_Math::rad2deg(hatch->getAngle()));
        leAngle->setText(s);
    }
}

void QG_DlgHatch::updateHatch() {
    if (hatch!=NULL) {
        hatch->setSolid(cbSolid->isChecked());
        hatch->setPattern(cbPattern->currentText());
        hatch->setScale(RS_Math::eval(leScale->text()));
        hatch->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
    }
}

void QG_DlgHatch::setPattern(const QString& p) {
    if (!RS_PATTERNLIST->contains(p)) {
        cbPattern->insertItem(p);
    }
    cbPattern->setCurrentText(p);
    pattern = cbPattern->getPattern();
}

void QG_DlgHatch::resizeEvent ( QResizeEvent * ) {
    updatePreview(NULL);
}

void QG_DlgHatch::updatePreview() {
    updatePreview(NULL);
}

void QG_DlgHatch::updatePreview(RS_Pattern* ) {
    if (preview==NULL) {
        return;
    }
    if (hatch==NULL || !cbEnablePreview->isChecked()) {
        preview->clear();
        gvPreview->zoomAuto();
        return;
    }

    QString patName = cbPattern->currentText();
    bool isSolid = cbSolid->isChecked();
    double prevSize;
    //double scale = RS_Math::eval(leScale->text(), 1.0);
    double angle = RS_Math::deg2rad(RS_Math::eval(leAngle->text(), 0.0));
    if (pattern!=NULL) {
        prevSize = pattern->getSize().x*10;
    } else {
        prevSize = 10.0;
    }

    preview->clear();

    RS_Hatch* prevHatch = new RS_Hatch(preview,
                                       RS_HatchData(isSolid, 0.2, angle, patName));
    prevHatch->setPen(hatch->getPen());

    RS_EntityContainer* loop = new RS_EntityContainer(prevHatch);
    loop->setPen(RS_Pen(RS2::FlagInvalid));
    loop->addEntity(new RS_Line(loop,
                                RS_LineData(RS_Vector(0.0,0.0),
                                            RS_Vector(10.0,0.0))));
    loop->addEntity(new RS_Line(loop,
                                RS_LineData(RS_Vector(10.0,0.0),
                                            RS_Vector(10.0,10.0))));
    loop->addEntity(new RS_Line(loop,
                                RS_LineData(RS_Vector(10.0,10.0),
                                            RS_Vector(0.0,10.0))));
    loop->addEntity(new RS_Line(loop,
                                RS_LineData(RS_Vector(0.0,10.0),
                                            RS_Vector(0.0,0.0))));
    prevHatch->addEntity(loop);
    preview->addEntity(prevHatch);
    if (!isSolid) {
        prevHatch->update();
    }

    gvPreview->zoomAuto();

}
