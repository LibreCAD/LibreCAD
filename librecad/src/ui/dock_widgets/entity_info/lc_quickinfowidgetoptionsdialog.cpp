/****************************************************************************
*
* Options Dialog for QuickInfo widget related functions

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/
#include "lc_quickinfowidgetoptionsdialog.h"
#include "lc_quickinfowidgetoptions.h"
#include "ui_lc_quickinfowidgetoptionsdialog.h"

LC_QuickInfoWidgetOptionsDialog::LC_QuickInfoWidgetOptionsDialog(QWidget *parent, LC_QuickInfoOptions *opts):
    LC_Dialog(parent, "QuickInfoOptions"),
    ui(new Ui::LC_QuickInfoWidgetOptionsDialog){
    ui->setupUi(this);

    m_options = opts;

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &LC_QuickInfoWidgetOptionsDialog::validate);
    connect(ui->cbDefaultActionAuto, &QCheckBox::clicked, this, &LC_QuickInfoWidgetOptionsDialog::onDefaultActionAutoClicked);

    ui->cbDisplayDistance->setChecked(m_options->displayDistanceAndAngle);
    ui->cbDrawPointsPath->setChecked(m_options->displayPointsPath);
    ui->cbEntityBoundaries->setChecked(m_options->displayEntityBoundaries);
    ui->cbInDefaultAction->setChecked(m_options->selectEntitiesInDefaultActionByCTRL);
    ui->cbDefaultActionAuto->setChecked(m_options->autoSelectEntitiesInDefaultAction);
    ui->cbDetailedPolyline->setChecked(m_options->displayPolylineDetailed);
    RS_Pen highlightPen = m_options->pen;
    ui->wHighlightPen->setPen(highlightPen, false, false, tr("Points highlight pen"));
    ui->cbInDefaultAction->setEnabled(!m_options->autoSelectEntitiesInDefaultAction);
}

LC_QuickInfoWidgetOptionsDialog::~LC_QuickInfoWidgetOptionsDialog(){
    delete ui;
}

void LC_QuickInfoWidgetOptionsDialog::validate(){
    RS_Pen pen = ui->wHighlightPen->getPen();
    m_options->pen = pen;
    m_options->displayDistanceAndAngle = ui->cbDisplayDistance->isChecked();
    m_options->displayPointsPath = ui->cbDrawPointsPath->isChecked();
    m_options->displayEntityBoundaries = ui->cbEntityBoundaries->isChecked();
    m_options->selectEntitiesInDefaultActionByCTRL = ui->cbInDefaultAction->isChecked();
    m_options->autoSelectEntitiesInDefaultAction = ui->cbDefaultActionAuto->isChecked();
    m_options->displayPolylineDetailed = ui->cbDetailedPolyline->isChecked();
    accept();
}

void LC_QuickInfoWidgetOptionsDialog::onDefaultActionAutoClicked(){
    ui->cbInDefaultAction->setEnabled(!ui->cbDefaultActionAuto->isChecked());
}

void LC_QuickInfoWidgetOptionsDialog::languageChange(){
   ui->retranslateUi(this);
}
