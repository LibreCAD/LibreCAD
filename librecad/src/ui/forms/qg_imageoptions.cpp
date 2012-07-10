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
#include "qg_imageoptions.h"

#include "rs_actioninterface.h"
#include "rs_actiondrawimage.h"
#include "rs_settings.h"

/*
 *  Constructs a QG_ImageOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_ImageOptions::QG_ImageOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_ImageOptions::~QG_ImageOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_ImageOptions::languageChange()
{
    retranslateUi(this);
}

void QG_ImageOptions::destroy() {
    RS_SETTINGS->beginGroup("/Image");
    RS_SETTINGS->writeEntry("/ImageAngle", leAngle->text());
    RS_SETTINGS->writeEntry("/ImageFactor", leFactor->text());
    RS_SETTINGS->endGroup();
}

void QG_ImageOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawImage) {
        action = (RS_ActionDrawImage*)a;

        QString sAngle;
        QString sFactor;
        if (update) {
            sAngle = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sFactor = QString("%1").arg(action->getFactor());
        } else {
            RS_SETTINGS->beginGroup("/Image");
            sAngle = RS_SETTINGS->readEntry("/ImageAngle", "0.0");
            sFactor = RS_SETTINGS->readEntry("/ImageFactor", "1.0");
            RS_SETTINGS->endGroup();
        }
    leAngle->setText(sAngle);
    leFactor->setText(sFactor);
        updateData();
        updateFactor();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_ImageOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_ImageOptions::updateData() {
    if (action!=NULL) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
    }
}

void QG_ImageOptions::updateDPI() {
    if (action!=NULL) {

        double f = action->dpiToScale(RS_Math::eval(leDPI->text()));
        leFactor->blockSignals(true);
        leFactor->setText(QString::number(f));
        leFactor->blockSignals(false);
        action->setFactor(f);
    }
}

void QG_ImageOptions::updateFactor() {
    if (action!=NULL) {
        double f = RS_Math::eval(leFactor->text());
        double dpi = action->scaleToDpi(f);
        leDPI->blockSignals(true);
        leDPI->setText(QString::number(dpi));
        leDPI->blockSignals(false);
        action->setFactor(f);
    }
}




