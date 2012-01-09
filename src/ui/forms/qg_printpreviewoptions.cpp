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
#include "qg_printpreviewoptions.h"

#include <qvariant.h>
#include "qg_printpreviewoptions.ui.h"
/*
 *  Constructs a QG_PrintPreviewOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_PrintPreviewOptions::QG_PrintPreviewOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_PrintPreviewOptions::~QG_PrintPreviewOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_PrintPreviewOptions::languageChange()
{
    retranslateUi(this);
}

void QG_PrintPreviewOptions::init() {
    updateDisabled = false;
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

}

void QG_PrintPreviewOptions::destroy() {
    /*
    RS_SETTINGS->beginGroup("/PrintPreview");
    RS_SETTINGS->writeEntry("/PrintPreviewAngle", leAngle->text());
    RS_SETTINGS->writeEntry("/PrintPreviewFactor", leFactor->text());
    RS_SETTINGS->endGroup();
    */
}

void QG_PrintPreviewOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionPrintPreview) {
        action = static_cast<RS_ActionPrintPreview*>(a);
        if(update==false){
            fit();
        }
        updateDisabled = true;
        cbScale->setDuplicatesEnabled(false);
        RS2::Unit u = action->getUnit();
        if (u==RS2::Inch) {
            cbScale->insertItems(0,imperialScales);
        } else {
            cbScale->insertItems(0,metricScales);
        }
        defaultScales=cbScale->count();
        updateScaleBox();
        //if (update) {
        //        QString s;
        //        s.setNum(action->getScale());
        //        int index=cbScale->findText(s);
        //        //add the current sccale, bug#343794
        //        if(index<0){
        //            cbScale->addItem(s);
        //            index=cbScale->count()-1;
        //        }
        //        cbScale->setCurrentIndex(index);
        //}

        updateDisabled = false;

        /*
        QString sAngle;
        QString sFactor;
        if (update) {
            sAngle = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sFactor = QString("%1").arg(action->getFactor());
    } else {
            RS_SETTINGS->beginGroup("/PrintPreview");
            sAngle = RS_SETTINGS->readEntry("/PrintPreviewAngle", "0.0");
            sFactor = RS_SETTINGS->readEntry("/PrintPreviewFactor", "1.0");
            RS_SETTINGS->endGroup();
    }
        leAngle->setText(sAngle);
        leFactor->setText(sFactor);
        updateData();
        */
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_PrintPreviewOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_PrintPreviewOptions::updateData() {
    if (action!=NULL) {
        /*
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
        action->setFactor(RS_Math::eval(leFactor->text()));
        */
    }
}

void QG_PrintPreviewOptions::center() {
    if (action!=NULL) {
        action->center();
    }
}

void QG_PrintPreviewOptions::setBlackWhite(bool on) {
    if (action!=NULL) {
        action->setBlackWhite(on);
    }
}

void QG_PrintPreviewOptions::fit() {
    if (action!=NULL) {
        action->fit();
        updateScaleBox();
    }
}

void QG_PrintPreviewOptions::scale(const QString& s) {
    if (updateDisabled) {
        return;
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
    if(factor<1.0e-6 || factor>1.0e6) {
        if(factor>1.0e6){
            action->printWarning(tr("Paper scale factor larger than 1.0e6"));
        }else{
            action->printWarning(tr("Paper scale factor smaller than 1.0e-6"));
        }
        return;
    }
    if(action->setScale(factor)){
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
    for(i=0;i<cbScale->count();i++){
        QString s=cbScale->itemText(i);
        int i0 = s.indexOf(':');
        bool ok1,ok2;
        double n = s.left(i0).toDouble(&ok1);
        double d = s.mid(i0+1).toDouble(&ok2);
        if(! (ok1 && ok2)|| fabs(d)<RS_TOLERANCE) continue;

        if(fabs(f-n/d)<RS_TOLERANCE) break;
    }
    if(i<cbScale->count()){
        cbScale->setCurrentIndex(i);
        //        std::cout<<"QG_PrintPreviewOptions::updateScaleBox(): old: "<<qPrintable(cbScale->currentText())<<std::endl;
        return;
    }
    QString s("");
    if(f>1.){
        s=QString("%1:1").arg(f);
    }else{
        if(fabs(f)>RS_TOLERANCE) s=QString("1:%1").arg(1./f);
    }
    if(cbScale->count()>defaultScales){
        i=defaultScales;
        cbScale->setItemText(defaultScales,s);
    }else{
        cbScale->addItem(s);
        i=cbScale->count()-1;
    }
    cbScale->setCurrentIndex(i);
    //    std::cout<<"QG_PrintPreviewOptions::updateScaleBox(): new: "<<qPrintable(cbScale->currentText())<<std::endl;
}

//EOF
