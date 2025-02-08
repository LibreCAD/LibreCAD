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
#include "qg_coordinatewidget.h"

#include "rs_graphic.h"
#include "rs_settings.h"
#include "rs_vector.h"
#include "rs_units.h"
#include "lc_graphicviewport.h"

/*
 *  Constructs a QG_CoordinateWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CoordinateWidget::QG_CoordinateWidget(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, fl){
    setObjectName(name);
    setupUi(this);

    lCoord1->setText("");
    lCoord2->setText("");
    lCoord1b->setText("");
    lCoord2b->setText("");

    graphic = nullptr;
    prec = 4;
    format = RS2::Decimal;
    aprec = 2;
    aformat = RS2::DegreesDecimal;
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CoordinateWidget::~QG_CoordinateWidget(){
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CoordinateWidget::languageChange(){
    retranslateUi(this);
}

void QG_CoordinateWidget::setGraphic(RS_Graphic* g, RS_GraphicView *gv) {
    graphic = g;
    graphicView = gv;
    viewport = nullptr;
    if (gv != nullptr){
        viewport = gv->getViewPort();
    }
    if (g != nullptr) {
//        setCoordinates(graphicView->toWorld(RS_Vector(0.0, 0.0)), graphicView->toWorld(RS_Vector(0.0, 0.0)), true);
        setCoordinates(0.0, 0.0, 0.0, 0.0, true);
    }
}

void QG_CoordinateWidget::setCoordinates(const RS_Vector& wcsAbs, const RS_Vector& wcsDelta, bool updateFormat) {
    double ucsX, ucsY, ucsDeltaX, ucsDeltaY;
    if (viewport != nullptr){        
        viewport->toUCS(wcsAbs, ucsX, ucsY);
        viewport->toUCSDelta(wcsDelta, ucsDeltaX, ucsDeltaY);
    }
    else{
        ucsX = wcsAbs.x;
        ucsY = wcsAbs.y;
        ucsDeltaX = wcsDelta.x;
        ucsDeltaY = wcsDelta.y;
    }
    setCoordinates(ucsX, ucsY, ucsDeltaX, ucsDeltaY, updateFormat);
}

void QG_CoordinateWidget::clearContent(){
    lCoord1->setText("0 , 0");
    lCoord2->setText("@  0 , 0");
    lCoord1b->setText("0 < 0");
    lCoord2b->setText("@  0 < 0");
}

void QG_CoordinateWidget::setCoordinates(double ucsX, double ucsY,
                                         double ucsDeltaX, double ucsDeltaY, bool updateFormat) {

    if (graphic != nullptr) {
        if (updateFormat) {
            format = graphic->getLinearFormat();
            prec = graphic->getLinearPrecision();
            aformat = graphic->getAngleFormat();
            aprec = graphic->getAnglePrecision();
        }

        if (!LC_GET_ONE_BOOL("Appearance", "UnitlessGrid", true)){
            ucsX  = RS_Units::convert(ucsX);
            ucsY  = RS_Units::convert(ucsY);
            ucsDeltaX = RS_Units::convert(ucsDeltaX);
            ucsDeltaY = RS_Units::convert(ucsDeltaY);
        }

        // abs / rel coordinates:
        RS2::Unit unit = graphic->getUnit();
        QString absX = RS_Units::formatLinear(ucsX, unit, format, prec);
        QString absY = RS_Units::formatLinear(ucsY, unit, format, prec);
        QString relX = RS_Units::formatLinear(ucsDeltaX, unit, format, prec);
        QString relY = RS_Units::formatLinear(ucsDeltaY, unit, format, prec);

        lCoord1->setText(absX + " , " + absY);
        lCoord2->setText("@  " + relX + " , " + relY);

        // polar coordinates:
        RS_Vector v;
        v = RS_Vector(ucsX, ucsY);
        QString str;
        QString rStr = RS_Units::formatLinear(v.magnitude(),unit,format, prec);
        double ucsAngle = v.angle();
        if (viewport != nullptr) {            
            ucsAngle = viewport->toBasisUCSAngle(ucsAngle);
        }

        QString aStr = RS_Units::formatAngle(ucsAngle, aformat, aprec);

        str = rStr + " < " + aStr;
        lCoord1b->setText(str);

        v = RS_Vector(ucsDeltaX, ucsDeltaY);
        rStr = RS_Units::formatLinear(v.magnitude(),unit,format, prec);
        double relUcsAngle = v.angle();
        if (viewport != nullptr) {
            relUcsAngle = viewport->toBasisUCSAngle(relUcsAngle);
        }
        aStr = RS_Units::formatAngle(relUcsAngle, aformat, aprec);

        lCoord2b->setText("@  " + rStr + " < " + aStr);

        absoluteCoordinates = RS_Vector(ucsX, ucsY, 0.0);
        relativeCoordinates = RS_Vector(ucsDeltaX, ucsDeltaY, 0.0);
    }
}
