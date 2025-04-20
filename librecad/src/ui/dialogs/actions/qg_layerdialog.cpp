/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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

#include <QMessageBox>

#include "rs_layer.h"
#include "rs_layerlist.h"

/*
 *  Constructs a QG_LayerDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_LayerDialog::QG_LayerDialog(QWidget* parent, QString name)
    : LC_Dialog(parent, "LayerProperties") {
    setObjectName(name);
    setupUi(this);
    init();
}

QG_LayerDialog::~QG_LayerDialog() {
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LayerDialog::languageChange() {
    retranslateUi(this);
}

void QG_LayerDialog::setLayer(RS_Layer* l) {
    m_layer = l;
    m_layerName = m_layer->getName();
    leName->setText(m_layerName);
    wPen->setPen(m_layer->getPen(), false, false, tr("Default Pen"));
    cbConstructionLayer->setChecked(l->isConstruction());

    if (m_layer->getName() == "0") {
        leName->setEnabled(false);
    }
}

void QG_LayerDialog::updateLayer() {
    m_layer->setName(leName->text());
    m_layer->setPen(wPen->getPen());
    m_layer->setConstruction(cbConstructionLayer->isChecked());
}

void QG_LayerDialog::validate() {
    if (m_layerList &&
        (m_editLayer == false || m_layerName != leName->text())) {
        RS_Layer* l = m_layerList->find(leName->text());
        if (l) {
            QMessageBox::information(parentWidget(),
                                     QMessageBox::tr("Layer Properties"),
                                     QMessageBox::tr("Layer with a name \"%1\" "
                                         "already exists. Please specify "
                                         "a different name.")
                                     .arg(leName->text()),
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

void QG_LayerDialog::setLayerList(RS_LayerList* ll) {
    m_layerList = ll;
}

void QG_LayerDialog::init() {
    leName->setFocus();
    m_layer = NULL;
    m_layerList = NULL;
    m_layerName = "";
    m_editLayer = false;
}

void QG_LayerDialog::setEditLayer(bool el) {
    m_editLayer = el;
}

//! @return a reference to the QLineEdit object.
QLineEdit* QG_LayerDialog::getQLineEdit() {
    return leName;
}
