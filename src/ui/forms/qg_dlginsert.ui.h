/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DlgInsert::setInsert(RS_Insert& i) {
    insert = &i;
    //pen = insert->getPen();
    wPen->setPen(insert->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = insert->getGraphic();
    if (graphic!=NULL) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = insert->getLayer(false);
    if (lay!=NULL) {
        cbLayer->setLayer(*lay);
    }
    QString s;
    s.setNum(insert->getInsertionPoint().x);
    leInsertionPointX->setText(s);
    s.setNum(insert->getInsertionPoint().y);
    leInsertionPointY->setText(s);
    s.setNum(insert->getScale().x);
    leScale->setText(s);
    s.setNum(RS_Math::rad2deg(insert->getAngle()));
    leAngle->setText(s);
    s.setNum(insert->getRows());
    leRows->setText(s);
    s.setNum(insert->getCols());
    leCols->setText(s);
    s.setNum(insert->getSpacing().x);
    leRowSpacing->setText(s);
    s.setNum(insert->getSpacing().y);
    leColSpacing->setText(s);
}

void QG_DlgInsert::updateInsert() {
    insert->setInsertionPoint(RS_Vector(RS_Math::eval(leInsertionPointX->text()),
                                  RS_Math::eval(leInsertionPointY->text())));
    insert->setScale(RS_Vector(RS_Math::eval(leScale->text()), 
                                RS_Math::eval(leScale->text())));
    insert->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
    insert->setRows(RS_Math::round(RS_Math::eval(leRows->text())));
    insert->setCols(RS_Math::round(RS_Math::eval(leCols->text())));
    insert->setSpacing(RS_Vector(RS_Math::eval(leRowSpacing->text()),
                                 RS_Math::eval(leColSpacing->text())));
    insert->setPen(wPen->getPen());
    insert->setLayer(cbLayer->currentText());
}

