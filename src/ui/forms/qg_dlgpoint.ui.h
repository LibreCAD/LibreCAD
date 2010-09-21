/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DlgPoint::setPoint(RS_Point& p) {
    point = &p;

    wPen->setPen(point->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = point->getGraphic();
    if (graphic!=NULL) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = point->getLayer(false);
    if (lay!=NULL) {
        cbLayer->setLayer(*lay);
    }

    QString s;
    s.setNum(point->getPos().x);
    lePosX->setText(s);
    s.setNum(point->getPos().y);
    lePosY->setText(s);
}

void QG_DlgPoint::updatePoint() {
    point->setPos(RS_Vector(RS_Math::eval(lePosX->text()),
                            RS_Math::eval(lePosY->text())));
    point->setPen(wPen->getPen());
    point->setLayer(cbLayer->currentText());
}

