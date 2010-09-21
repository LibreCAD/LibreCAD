/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DlgArc::setArc(RS_Arc& a) {
    arc = &a;
    //pen = arc->getPen();
    wPen->setPen(arc->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = arc->getGraphic();
    if (graphic!=NULL) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = arc->getLayer(false);
    if (lay!=NULL) {
        cbLayer->setLayer(*lay);
    }
    QString s;
    s.setNum(arc->getCenter().x);
    leCenterX->setText(s);
    s.setNum(arc->getCenter().y);
    leCenterY->setText(s);
    s.setNum(arc->getRadius());
    leRadius->setText(s);
    s.setNum(RS_Math::rad2deg(arc->getAngle1()));
    leAngle1->setText(s);
    s.setNum(RS_Math::rad2deg(arc->getAngle2()));
    leAngle2->setText(s);
    cbReversed->setChecked(arc->isReversed());
}

void QG_DlgArc::updateArc() {
    arc->setCenter(RS_Vector(RS_Math::eval(leCenterX->text()),
                                  RS_Math::eval(leCenterY->text())));
    arc->setRadius(RS_Math::eval(leRadius->text()));
    arc->setAngle1(RS_Math::deg2rad(RS_Math::eval(leAngle1->text())));
    arc->setAngle2(RS_Math::deg2rad(RS_Math::eval(leAngle2->text())));
    arc->setReversed(cbReversed->isChecked());
    arc->setPen(wPen->getPen());
    arc->setLayer(cbLayer->currentText());
    arc->calculateEndpoints();
    arc->calculateBorders();
}

