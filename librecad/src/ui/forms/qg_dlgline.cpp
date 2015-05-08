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
#include "qg_dlgline.h"

#include "rs_line.h"
#include "rs_graphic.h"
#include "rs_math.h"

/*
 *  Constructs a QG_DlgLine as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgLine::QG_DlgLine(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgLine::~QG_DlgLine()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgLine::languageChange()
{
    retranslateUi(this);
}

void QG_DlgLine::setLine(RS_Line& l) {
    line = &l;
    //pen = line->getPen();
    wPen->setPen(line->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = line->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = line->getLayer(false);
    if (lay) {
        cbLayer->setLayer(*lay);
    }
    QString s;
    s.setNum(line->getStartpoint().x);
    leStartX->setText(s);
    s.setNum(line->getStartpoint().y);
    leStartY->setText(s);
    s.setNum(line->getEndpoint().x);
    leEndX->setText(s);
    s.setNum(line->getEndpoint().y);
    leEndY->setText(s);
}

void QG_DlgLine::updateLine() {
    line->setStartpoint(RS_Vector(RS_Math::eval(leStartX->text()),
                                  RS_Math::eval(leStartY->text())));
    line->setEndpoint(RS_Vector(RS_Math::eval(leEndX->text()),
                                RS_Math::eval(leEndY->text())));
    line->setPen(wPen->getPen());
    line->setLayer(cbLayer->currentText());
}

