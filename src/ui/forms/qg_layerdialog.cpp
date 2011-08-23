/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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
#include "qg_layerdialog.h"

#include <qvariant.h>
#include <qmessagebox.h>
#include "qg_widgetpen.h"

/*
 *  Constructs a QG_LayerDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_LayerDialog::QG_LayerDialog(QWidget* parent, const char* name, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, name, modal, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LayerDialog::~QG_LayerDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LayerDialog::languageChange()
{
    retranslateUi(this);
}

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
