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
#ifndef QG_COORDINATEWIDGET_H
#define QG_COORDINATEWIDGET_H

#include "lc_graphicviewaware.h"
#include "rs.h"
#include "rs_vector.h"
#include "ui_qg_coordinatewidget.h"

class LC_GraphicViewport;
class RS_Graphic;

class QG_CoordinateWidget : public QWidget, public LC_GraphicViewAware,  public Ui::QG_CoordinateWidget{
    Q_OBJECT
public:
    QG_CoordinateWidget(QWidget *parent = nullptr, const char *name = nullptr, Qt::WindowFlags fl = {});
    ~QG_CoordinateWidget() override;
    void clearContent();
    void setGraphicView(RS_GraphicView* gv) override;
public slots:
    void setCoordinates(const RS_Vector & wcsAbs, const RS_Vector & wcsDelta, bool updateFormat ); // fixme - check why updateFormat is always true
protected slots:
    void languageChange();
    void setCoordinates(double ucsX, double ucsY, double ucsDeltaX, double ucsDeltaY, bool updateFormat );
private:
    RS_Graphic* m_graphic = nullptr;
    RS_GraphicView *m_graphicView = nullptr;
    LC_GraphicViewport* m_viewport = nullptr;
    int m_linearPrecision = 0;
    RS2::LinearFormat m_linearFormat = RS2::Decimal;
    int m_anglePrecision = 0;
    RS2::AngleFormat m_angleFormat = RS2::DegreesDecimal;
    RS_Vector m_absoluteCoordinates;
    RS_Vector m_relativeCoordinates;
};

#endif // QG_COORDINATEWIDGET_H
