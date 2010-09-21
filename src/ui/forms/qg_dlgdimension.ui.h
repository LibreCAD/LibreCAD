/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/

void QG_DlgDimension::setDim(RS_Dimension& d) {
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
}

void QG_DlgDimension::updateDim() {
    dim->setLabel(wLabel->getLabel());
}

