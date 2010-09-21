/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/

void QG_LayerDialog::setLayer(RS_Layer* l) {
    layer = l;	
	layerName = layer->getName();
    leName->setText(layerName);
    wPen->setPen(layer->getPen(), false, false, tr("Default Pen"));

    if (layer->getName()=="0") {
        leName->setEnabled(false);
    }
}

void QG_LayerDialog::updateLayer() {
    layer->setName(leName->text().latin1());
    layer->setPen(wPen->getPen());
}

void QG_LayerDialog::validate() {
	if (layerList != NULL && 
		(editLayer == FALSE || layerName != leName->text().latin1())) {
		RS_Layer* l = layerList->find(leName->text().latin1());
		if (l != NULL) {
			QMessageBox::information(parentWidget(),
									 QMessageBox::tr("Layer Properties"),
									 QMessageBox::tr("Layer with a name \"%1\" "
													 "already exists. Please specify "
													 "a different name.")
									 .arg(leName->text().latin1()),
									 QMessageBox::Ok);
			leName->setFocus();
			leName->selectAll();
		}
		else
			accept();
	}
	else	
		accept();
}

void QG_LayerDialog::setLayerList( RS_LayerList * ll ){
    layerList = ll;
}

void QG_LayerDialog::init(){
	layer = NULL;
	layerList = NULL;
	layerName = "";
	editLayer = FALSE;
}

void QG_LayerDialog::setEditLayer( bool el ){
	editLayer = el;
}
