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
#include "qg_dlgspline.h"

#include "rs_spline.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "qg_widgetpen.h"
#include "qg_layerbox.h"
#include "rs_math.h"

/*
 *  Constructs a QG_DlgSpline as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgSpline::QG_DlgSpline(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgSpline::languageChange()
{
    retranslateUi(this);
}

void QG_DlgSpline::setSpline(RS_Spline& e) {
    spline = &e;

    RS_Graphic* graphic = spline->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = spline->getLayer(false);
    if (lay) {
        cbLayer->setLayer(*lay);
    }

    wPen->setPen(spline, lay, "Pen");
	
    QString s;
    s.setNum(spline->getDegree());
    cbDegree->setCurrentIndex( cbDegree->findText(s) );

    cbClosed->setChecked(spline->isClosed());
    lId->setText(QString("ID: %1").arg(spline->getId()));
}



void QG_DlgSpline::updateSpline() {
    spline->setDegree(RS_Math::round(RS_Math::eval(cbDegree->currentText())));
    spline->setClosed(cbClosed->isChecked());
    spline->setPen(wPen->getPen());
    spline->setLayer(cbLayer->currentText());
	spline->update();
}

