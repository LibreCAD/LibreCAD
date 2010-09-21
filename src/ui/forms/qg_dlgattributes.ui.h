/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DlgAttributes::setData(RS_AttributesData* data, RS_LayerList& layerList) {
    this->data = data;
    
    //pen = line->getPen();
    wPen->setPen(data->pen, true, true, "Pen");
    
    //RS_Graphic* graphic = line->getGraphic();
    //if (graphic!=NULL) {
        cbLayer->init(layerList, false, true);
    //}
    //cbLayer->setLayer(data->layer);
    //RS_Layer* lay = line->getLayer(false);
    //if (lay!=NULL) {
    //    cbLayer->setLayer(*lay);
    //}
}

void QG_DlgAttributes::updateData() {
    data->pen = wPen->getPen();
    data->layer = cbLayer->currentText();
    
    data->changeColor = !wPen->isColorUnchanged();
    data->changeLineType = !wPen->isLineTypeUnchanged();
    data->changeWidth = !wPen->isWidthUnchanged();
    
    data->changeLayer = !cbLayer->isUnchanged();
}

