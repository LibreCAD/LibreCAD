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
#include "qg_widgetpen.h"

#include <QVariant>
#include "qg_colorbox.h"
#include "qg_widthbox.h"
#include "qg_linetypebox.h"

/*
 *  Constructs a QG_WidgetPen as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_WidgetPen::QG_WidgetPen(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

}

void QG_WidgetPen::setPen(RS_Pen pen, bool showByLayer, 
                          bool showUnchanged, const QString& title) {
    cbColor->init(showByLayer, showUnchanged);
    cbWidth->init(showByLayer, showUnchanged);
    cbLineType->init(showByLayer, showUnchanged);
    if (!showUnchanged) {
       cbColor->setColor(pen.getColor());
       cbWidth->setWidth(pen.getWidth());
       cbLineType->setLineType(pen.getLineType());
    }

    if (!title.isEmpty()) {
        bgPen->setTitle(title);
    }
}

RS_Pen QG_WidgetPen::getPen() {
    RS_Pen pen;

    pen.setColor(cbColor->getColor());
    pen.setWidth(cbWidth->getWidth());
    pen.setLineType(cbLineType->getLineType());

    return pen;
}

bool QG_WidgetPen::isColorUnchanged() {
    return cbColor->isUnchanged();
}

bool QG_WidgetPen::isLineTypeUnchanged() {
    return cbLineType->isUnchanged();
}

bool QG_WidgetPen::isWidthUnchanged() {
    return cbWidth->isUnchanged();
}
/*
 *  Destroys the object and frees any allocated resources
 */
QG_WidgetPen::~QG_WidgetPen()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_WidgetPen::languageChange()
{
    retranslateUi(this);
}

