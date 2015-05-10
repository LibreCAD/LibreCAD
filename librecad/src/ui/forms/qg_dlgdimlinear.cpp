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
#include "qg_dlgdimlinear.h"

#include "rs_dimlinear.h"
#include "rs_graphic.h"
#include "rs_math.h"

/*
 *  Constructs a QG_DlgDimLinear as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgDimLinear::QG_DlgDimLinear(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgDimLinear::~QG_DlgDimLinear()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgDimLinear::languageChange()
{
    retranslateUi(this);
}

void QG_DlgDimLinear::setDim(RS_DimLinear& d) {
    dim = &d;
    wPen->setPen(dim->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = dim->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = dim->getLayer(false);
    if (lay) {
        cbLayer->setLayer(*lay);
    }

    wLabel->setLabel(dim->getLabel(false));
    leAngle->setText(QString("%1").arg(RS_Math::rad2deg(dim->getAngle())));
}

void QG_DlgDimLinear::updateDim() {
    dim->setLabel(wLabel->getLabel());
    dim->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text(), 0.0)));
    dim->setPen(wPen->getPen());
    dim->setLayer(cbLayer->currentText());
}

