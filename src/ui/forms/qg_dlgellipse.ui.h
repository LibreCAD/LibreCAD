/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DlgEllipse::setEllipse(RS_Ellipse& e) {
    ellipse = &e;
    //pen = ellipse->getPen();
    wPen->setPen(ellipse->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = ellipse->getGraphic();
    if (graphic!=NULL) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = ellipse->getLayer(false);
    if (lay!=NULL) {
        cbLayer->setLayer(*lay);
    }
    QString s;
    s.setNum(ellipse->getCenter().x);
    leCenterX->setText(s);
    s.setNum(ellipse->getCenter().y);
    leCenterY->setText(s);
    s.setNum(ellipse->getMajorP().magnitude());
    leMajor->setText(s);
    s.setNum(ellipse->getMajorP().magnitude()*ellipse->getRatio());
    leMinor->setText(s);
    s.setNum(RS_Math::rad2deg(ellipse->getMajorP().angle()));
    leRotation->setText(s);
    s.setNum(RS_Math::rad2deg(ellipse->getAngle1()));
    leAngle1->setText(s);
    s.setNum(RS_Math::rad2deg(ellipse->getAngle2()));
    leAngle2->setText(s);
    cbReversed->setChecked(ellipse->isReversed());
}

void QG_DlgEllipse::updateEllipse() {
    ellipse->setCenter(RS_Vector(RS_Math::eval(leCenterX->text()),
                                  RS_Math::eval(leCenterY->text())));
    RS_Vector v;
    v.setPolar(RS_Math::eval(leMajor->text()), 
               RS_Math::deg2rad(RS_Math::eval(leRotation->text())));
    ellipse->setMajorP(v);
    if (RS_Math::eval(leMajor->text())>1.0e-6) {
    	ellipse->setRatio(RS_Math::eval(leMinor->text())/RS_Math::eval(leMajor->text()));
    }
    else {
        ellipse->setRatio(1.0);
    }
    ellipse->setAngle1(RS_Math::deg2rad(RS_Math::eval(leAngle1->text())));
    ellipse->setAngle2(RS_Math::deg2rad(RS_Math::eval(leAngle2->text())));
    ellipse->setReversed(cbReversed->isChecked());
    ellipse->setPen(wPen->getPen());
    ellipse->setLayer(cbLayer->currentText());
}

