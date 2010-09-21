/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/

void QG_DlgDimLinear::setDim(RS_DimLinear& d) {
    dim = &d;
    wPen->setPen(dim->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = dim->getGraphic();
    if (graphic!=NULL) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = dim->getLayer(false);
    if (lay!=NULL) {
        cbLayer->setLayer(*lay);
    }
    
    wLabel->setLabel(dim->getLabel(false));
    leAngle->setText(QString("%1").arg(RS_Math::rad2deg(dim->getAngle())));
}

void QG_DlgDimLinear::updateDim() {
    dim->setLabel(wLabel->getLabel());
    dim->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text(), 0.0)));
}

