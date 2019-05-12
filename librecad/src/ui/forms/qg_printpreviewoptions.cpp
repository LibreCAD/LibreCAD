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
#include<cmath>
#include "qg_printpreviewoptions.h"
#include "rs_settings.h"

#include "rs_actionprintpreview.h"
#include "rs_math.h"
#include "ui_qg_printpreviewoptions.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_PrintPreviewOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_PrintPreviewOptions::QG_PrintPreviewOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, defaultScales{0}
	, ui(new Ui::Ui_PrintPreviewOptions{})
{
	ui->setupUi(this);
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_PrintPreviewOptions::~QG_PrintPreviewOptions()
{
	saveSettings();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_PrintPreviewOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_PrintPreviewOptions::init() {
    imperialScales
            << "1\" = 1\""
            << "1\" = 2\""
            << "1\" = 4\""
            << "1\" = 8\""
            << "1\" = 16\""
            << "1\" = 32\""
            << "1\" = 64\""
            << "1\" = 128\""
            << "1\" = 256\"";

    metricScales
            << "1:1" << "1:2" << "1:5" << "1:10"
            << "1:20" << "1:25" << "1:50" << "1:75" << "1:100"
            << "1:125" << "1:150" << "1:175" << "1:200"
            << "1:250" << "1:500" << "1:750" << "1:1000"
            << "1:2500" << "1:5000" << "1:7500" << "1:10000"
            << "1:25000" << "1:50000" << "1:75000" << "1:100000"
            << "2:1" << "5:1" << "10:1"
            << "20:1" << "25:1" << "50:1" << "75:1" << "100:1"
            << "125:1" << "150:1" << "175:1" << "200:1"
            << "250:1" << "500:1" << "750:1" << "1000:1"
            << "2500:1" << "5000:1" << "7500:1" << "10000:1"
            << "25000:1" << "50000:1" << "75000:1" << "100000:1";
    RS_SETTINGS->beginGroup("/PrintPreview");
    updateDisabled= RS_SETTINGS->readNumEntry("/PrintScaleFixed", 0)!=0;
    blackWhiteDisabled= RS_SETTINGS->readNumEntry("/BlackWhiteSet", 0)!=0;
    RS_SETTINGS->endGroup();
	action=nullptr;
    //make sure user scale is accepted
	ui->cbScale->setInsertPolicy(QComboBox::InsertAtTop);
}

void QG_PrintPreviewOptions::saveSettings() {
    RS_SETTINGS->beginGroup("/PrintPreview");
    RS_SETTINGS->writeEntry("/PrintScaleFixed", updateDisabled?1:0);
    RS_SETTINGS->writeEntry("/BlackWhiteSet", QString(blackWhiteDisabled?"1":"0"));
	RS_SETTINGS->writeEntry("/PrintScaleValue", ui->cbScale->currentText());
    RS_SETTINGS->endGroup();
	action=nullptr;
}

/** print scale fixed to saved value **/
void QG_PrintPreviewOptions::setScaleFixed(bool fixed)
{
	if (action) action->setPaperScaleFixed(fixed);
    updateDisabled=fixed;
	ui->cbScale->setDisabled(fixed);
	ui->bFit->setVisible(!fixed);
	if(ui->cFixed->isChecked() != fixed) {
		ui->cFixed->setChecked(fixed);
    }
    RS_SETTINGS->beginGroup("/PrintPreview");
    RS_SETTINGS->writeEntry("/PrintScaleFixed", updateDisabled?1:0);
	RS_SETTINGS->writeEntry("/PrintScaleValue", ui->cbScale->currentText());
    RS_SETTINGS->endGroup();
}

void QG_PrintPreviewOptions::setAction(RS_ActionInterface* a, bool update) {

	if (a && a->rtti()==RS2::ActionFilePrintPreview) {
        action = static_cast<RS_ActionPrintPreview*>(a);
        /** fixed scale **/
        if(update){
//                        std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
//                        std::cout<<"update="<<update<<" : updateDisabled="<<updateDisabled <<std::endl;
//                        std::cout<<"update="<<update<<" : action->getPaperScaleFixed()="<<action->getPaperScaleFixed() <<std::endl;
            if(updateDisabled||action->getPaperScaleFixed()){
				if (!action->getPaperScaleFixed()){
                    RS_SETTINGS->beginGroup("/PrintPreview");
                    QString&& s=RS_SETTINGS->readEntry("/PrintScaleValue", "1:1");
                    RS_SETTINGS->endGroup();
                    updateDisabled=false;
                    scale(s);
                }
                updateDisabled=true;
                setScaleFixed(true);
            }else{
				double currScale = action->getScale();
                if(  currScale > RS_TOLERANCE)
                    scale (currScale);
                else
                    fit();
                updateScaleBox();
                setScaleFixed(false);
            }
        }else{
            double f=action->getScale();
            bool btmp=updateDisabled;
            updateDisabled = true;
			ui->cbScale->setDuplicatesEnabled(false);
            RS2::Unit u = action->getUnit();
            if (u==RS2::Inch) {
				ui->cbScale->insertItems(0,imperialScales);
            } else {
				ui->cbScale->insertItems(0,metricScales);
            }
			defaultScales=ui->cbScale->count();
            updateScaleBox(f);
            updateDisabled = btmp;
            setScaleFixed(updateDisabled);
        }
        setBlackWhite(blackWhiteDisabled);

    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_PrintPreviewOptions::setAction: wrong action type");
		action = nullptr;
    }
}

void QG_PrintPreviewOptions::updateData() {
    if (action) {
        /*
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
        action->setFactor(RS_Math::eval(leFactor->text()));
        */
    }
}

void QG_PrintPreviewOptions::center() {
    if (action) {
        action->center();
    }
}

void QG_PrintPreviewOptions::setBlackWhite(bool on) {
    if (action) {
		if(ui->bBlackWhite->isChecked() != on) {
			ui->bBlackWhite->setChecked(on);
        }
        blackWhiteDisabled = on;
        action->setBlackWhite(on);
    }
}

void QG_PrintPreviewOptions::fit() {
    if(updateDisabled) return;
    if (action) {
        action->fit();
        updateScaleBox();
    }
}


void QG_PrintPreviewOptions::scale(const double& factor) {
	double f=fabs(factor); // do we need negative factor at all?
    if(action->setScale(f, false)){
        //        std::cout<<"QG_PrintPreviewOptions::scale(const QString& s): line: "<<__LINE__<<" s="<<factor<<std::endl;
        updateScaleBox(f);
    }
}

void QG_PrintPreviewOptions::scale(const QString& s0) {
    QString s;
    if (updateDisabled) {
		s=ui->cbScale->currentText();
    }else{
        s=s0;
    }
    //    std::cout<<"QG_PrintPreviewOptions::scale(const QString& s): s="<<qPrintable(s)<<std::endl;
    double factor(1.);

    if (s.contains(':')) {
        bool ok1 = false;
        bool ok2 = false;
        int i = s.indexOf(':');
        double n = RS_Math::eval(s.left(i),&ok1);
        double d = RS_Math::eval(s.mid(i+1),&ok2);
        if (ok1 && ok2 && d>1.0e-6 ){
            factor=n/d;
        }
    } else if (s.contains('=')) {
        bool ok = false;
        int i = s.indexOf('=');
        double d = RS_Math::eval(s.mid(i+2, s.length()-i-3),&ok);
        if (ok && d>1.0e-6) {
            factor=1.0/d;
        }
    } else {
        bool ok = false;
        double f = RS_Math::eval(s, &ok);
        if (ok) {
            factor=f;
        }
    }
    factor=fabs(factor); // do we need negative factor at all?
//    if(factor<1.0e-6 || factor>1.0e6) {
//        if(factor>1.0e6){
//            action->printWarning(tr("Paper scale factor larger than 1.0e6"));
//        }else{
//            action->printWarning(tr("Paper scale factor smaller than 1.0e-6"));
//        }
//        return;
//    }
    if(action->setScale(factor, false)){
        //        std::cout<<"QG_PrintPreviewOptions::scale(const QString& s): line: "<<__LINE__<<" s="<<factor<<std::endl;
        updateScaleBox(factor);
    }
}

//update the scalebox to
void QG_PrintPreviewOptions::updateScaleBox(){
    updateScaleBox(action->getScale());
}

void QG_PrintPreviewOptions::updateScaleBox(const double& f){
    //    std::cout<<"void QG_PrintPreviewOptions::updateScaleBox() f="<<f<<std::endl;
    int i;
	for(i=0;i<ui->cbScale->count();i++){
		QString s=ui->cbScale->itemText(i);
        int i0 = s.indexOf(':');
        bool ok1,ok2;
        double n = s.left(i0).toDouble(&ok1);
        double d = s.mid(i0+1).toDouble(&ok2);
        if(! (ok1 && ok2)|| fabs(d)<RS_TOLERANCE) continue;

        if(fabs(f-n/d)<RS_TOLERANCE) break;
    }
	if(i<ui->cbScale->count()){
		ui->cbScale->setCurrentIndex(i);
		//        std::cout<<"QG_PrintPreviewOptions::updateScaleBox(): old: "<<qPrintable(ui->cbScale->currentText())<<std::endl;
        return;
    }
    QString s("");
    if(f>1.){
        s=QString("%1:1").arg(f);
    }else{
        if(fabs(f)>RS_TOLERANCE) s=QString("1:%1").arg(1./f);
    }
	if(ui->cbScale->count()>defaultScales){
        i=defaultScales;
		ui->cbScale->setItemText(defaultScales,s);
    }else{
		ui->cbScale->addItem(s);
		i=ui->cbScale->count()-1;
    }
	ui->cbScale->setCurrentIndex(i);
	//    std::cout<<"QG_PrintPreviewOptions::updateScaleBox(): new: "<<qPrintable(ui->cbScale->currentText())<<std::endl;
}

//void QG_PrintPreviewOptions::updateScaleBox(const QString& s) {
//    if(ui->cbScale->count()>defaultScales) std::cout<<"ui->cbScale->last()="<<qPrintable(ui->cbScale->itemText(defaultScales))<<std::endl;
//    std::cout<<"void QG_PrintPreviewOptions::updateScaleBox(QString) s="<<qPrintable(s)<<std::endl;
//    int index=ui->cbScale->findText(s);
//    std::cout<<"QG_PrintPreviewOptions::updateScaleBox(): ui->cbScale->findText(s)="<<index<<std::endl;
//    //add the current sccale, bug#343794
//    if(index>=defaultScales){
//        index=defaultScales;
//        ui->cbScale->setItemText(defaultScales,s);
//    }else{
//        if(index<0){
//            ui->cbScale->addItem(s);
//            index=ui->cbScale->count() -1;
//        }
//    }

//    ui->cbScale->setCurrentIndex(index);
//}

void QG_PrintPreviewOptions::calcPagesNum() {
    if (action) {
        action->calcPagesNum();
    }
}
