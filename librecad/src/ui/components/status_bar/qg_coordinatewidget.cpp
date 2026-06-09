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

#include "lc_convert.h"
#include "lc_graphicviewport.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_settings.h"
#include "rs_units.h"

/*
 *  Constructs a QG_CoordinateWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CoordinateWidget::QG_CoordinateWidget(QWidget* parent, const char* name, const Qt::WindowFlags fl)
    : QWidget(parent, fl){
    setObjectName(name);
    setupUi(this);

    lCoord1->setText("");
    lCoord2->setText("");
    lCoord1b->setText("");
    lCoord2b->setText("");

    m_graphic = nullptr;
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CoordinateWidget::~QG_CoordinateWidget()= default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CoordinateWidget::languageChange(){
    retranslateUi(this);
}

void QG_CoordinateWidget::setGraphicView(RS_GraphicView *gv) {
    m_graphicView = gv;
    if (gv != nullptr){
        m_viewport = gv->getViewPort();
        m_graphic = gv->getGraphic(true);
        m_formatter = m_viewport->getFormatter();
        setCoordinates(0.0, 0.0, 0.0, 0.0, true);
    }
    else {
        m_viewport = nullptr;
        m_graphic = nullptr;
        m_formatter = nullptr;
    }
}

void QG_CoordinateWidget::setCoordinates(const RS_Vector& wcsAbs, const RS_Vector& wcsDelta, const bool updateFormat) {
    double ucsX, ucsY, ucsDeltaX, ucsDeltaY;
    if (m_viewport != nullptr){
        m_viewport->toUCS(wcsAbs, ucsX, ucsY);
        m_viewport->toUCSDelta(wcsDelta, ucsDeltaX, ucsDeltaY);
    }
    else{
        ucsX = wcsAbs.x;
        ucsY = wcsAbs.y;
        ucsDeltaX = wcsDelta.x;
        ucsDeltaY = wcsDelta.y;
    }
    setCoordinates(ucsX, ucsY, ucsDeltaX, ucsDeltaY, updateFormat);
}

void QG_CoordinateWidget::clearContent() const {
    lCoord1->setText("0 , 0");
    lCoord2->setText("@  0 , 0");
    lCoord1b->setText("0 < 0");
    lCoord2b->setText("@  0 < 0");
}

void QG_CoordinateWidget::setCoordinates(double ucsX, double ucsY,
                                         double ucsDeltaX, double ucsDeltaY, const bool updateFormat) {
    if (m_graphic != nullptr) {
        if (m_viewport != nullptr) {
            if (updateFormat) {
                m_formatter = m_viewport->getFormatter(); // fixme- fmt - most probably it's not necessary
            }

            if (!LC_GET_ONE_BOOL("Appearance", "UnitlessGrid", true)){
                ucsX  = RS_Units::convert(ucsX);
                ucsY  = RS_Units::convert(ucsY);
                ucsDeltaX = RS_Units::convert(ucsDeltaX);
                ucsDeltaY = RS_Units::convert(ucsDeltaY);
            }

            // abs / rel coordinates:
            const QString absX = m_formatter->formatLinear(ucsX);
            const QString absY = m_formatter->formatLinear(ucsY);
            const QString relX = m_formatter->formatLinear(ucsDeltaX);
            const QString relY = m_formatter->formatLinear(ucsDeltaY);

            lCoord1->setText(absX + " , " + absY);
            lCoord2->setText("@  " + relX + " , " + relY);

            // polar coordinates:
            const auto polarCoordinate = RS_Vector(ucsX, ucsY);
            const QString polarMagnitude = m_formatter->formatLinear(polarCoordinate.magnitude());
            const double ucsAngle = polarCoordinate.angle();

            const double basicUcsAngle = m_viewport->toBasisUCSAngle(ucsAngle);

            QString angleStr = m_formatter->formatRawAngle(basicUcsAngle);

            lCoord1b->setText(polarMagnitude + " < " + angleStr);

            // relative polar
            const auto polarRelativeCoordinate = RS_Vector(ucsDeltaX, ucsDeltaY);
            const QString relPolarMagnitude = m_formatter->formatLinear(polarRelativeCoordinate.magnitude());
            const double relUcsAngle = polarRelativeCoordinate.angle();

            const double basicRelUcsAngle = m_viewport->toBasisUCSAngle(relUcsAngle);

            angleStr = m_formatter->formatRawAngle(basicRelUcsAngle);

            lCoord2b->setText("@  " + relPolarMagnitude + " < " + angleStr);

            m_absoluteCoordinates = RS_Vector(ucsX, ucsY, 0.0);
            m_relativeCoordinates = RS_Vector(ucsDeltaX, ucsDeltaY, 0.0);
        }
        else {
           clearContent();
        }
    }
}
