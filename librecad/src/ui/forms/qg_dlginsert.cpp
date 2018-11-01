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
#include "qg_dlginsert.h"

#include "rs_insert.h"
#include "rs_graphic.h"
#include "rs_math.h"

/*
 *  Constructs a QG_DlgInsert as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgInsert::QG_DlgInsert(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgInsert::~QG_DlgInsert()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgInsert::languageChange()
{
    retranslateUi(this);
}

void QG_DlgInsert::setInsert(RS_Insert& i) {
    insert = &i;
    //pen = insert->getPen();
    wPen->setPen(insert->getPen(false), true, false, "Pen");
    RS_Graphic* graphic = insert->getGraphic();
    if (graphic) {
        cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = insert->getLayer(false);
    if (lay) {
        cbLayer->setLayer(*lay);
    }
    QString s;
    s.setNum(insert->getInsertionPoint().x);
    leInsertionPointX->setText(s);
    s.setNum(insert->getInsertionPoint().y);
    leInsertionPointY->setText(s);
    s.setNum(insert->getScale().x);
    leScaleX->setText(s);
    s.setNum(insert->getScale().y);
    leScaleY->setText(s);
    s.setNum(RS_Math::rad2deg(insert->getAngle()));
    leAngle->setText(s);
    s.setNum(insert->getRows());
    leRows->setText(s);
    s.setNum(insert->getCols());
    leCols->setText(s);
    s.setNum(insert->getSpacing().y);
    leRowSpacing->setText(s);
    s.setNum(insert->getSpacing().x);
    leColSpacing->setText(s);
}

void QG_DlgInsert::updateInsert() {
    insert->setInsertionPoint(RS_Vector(RS_Math::eval(leInsertionPointX->text()),
                                  RS_Math::eval(leInsertionPointY->text())));
    insert->setScale(RS_Vector(RS_Math::eval(leScaleX->text()),
                                RS_Math::eval(leScaleY->text())));
    insert->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
    insert->setRows(RS_Math::round(RS_Math::eval(leRows->text())));
    insert->setCols(RS_Math::round(RS_Math::eval(leCols->text())));
    insert->setSpacing(RS_Vector(RS_Math::eval(leColSpacing->text()),
                                 RS_Math::eval(leRowSpacing->text())));
    insert->setPen(wPen->getPen());
    insert->setLayer(cbLayer->currentText());
}

