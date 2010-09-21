/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DlgCircle::setCircle(RS_Circle& c) {
    circle = &c;
    //pen = circle->getPen();
    wPen->setPen(circle->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = circle->getGraphic();
    if (graphic!=NULL) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = circle->getLayer(false);
    if (lay!=NULL) {
        cbLayer->setLayer(*lay);
    }
    QString s;
    s.setNum(circle->getCenter().x);
    leCenterX->setText(s);
    s.setNum(circle->getCenter().y);
    leCenterY->setText(s);
    s.setNum(circle->getRadius());
    leRadius->setText(s);
}

void QG_DlgCircle::updateCircle() {
    circle->setCenter(RS_Vector(RS_Math::eval(leCenterX->text()),
                                  RS_Math::eval(leCenterY->text())));
    circle->setRadius(RS_Math::eval(leRadius->text()));
    circle->setPen(wPen->getPen());
    circle->setLayer(cbLayer->currentText());
    circle->calculateBorders();
}

