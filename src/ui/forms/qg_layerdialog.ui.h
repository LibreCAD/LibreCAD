/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

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
