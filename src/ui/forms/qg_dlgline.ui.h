/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DlgLine::setLine(RS_Line& l) {
    line = &l;
    //pen = line->getPen();
    wPen->setPen(line->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = line->getGraphic();
    if (graphic!=NULL) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = line->getLayer(false);
    if (lay!=NULL) {
        cbLayer->setLayer(*lay);
    }
    QString s;
    s.setNum(line->getStartpoint().x);
    leStartX->setText(s);
    s.setNum(line->getStartpoint().y);
    leStartY->setText(s);
    s.setNum(line->getEndpoint().x);
    leEndX->setText(s);
    s.setNum(line->getEndpoint().y);
    leEndY->setText(s);
}

void QG_DlgLine::updateLine() {
    line->setStartpoint(RS_Vector(RS_Math::eval(leStartX->text()),
                                  RS_Math::eval(leStartY->text())));
    line->setEndpoint(RS_Vector(RS_Math::eval(leEndX->text()),
                                RS_Math::eval(leEndY->text())));
    line->setPen(wPen->getPen());
    line->setLayer(cbLayer->currentText());
}

