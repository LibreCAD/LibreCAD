/****************************************************************************
**
  * Create option widget used to draw equidistant polylines

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#ifndef QG_POLYLINEEQUIDISTANTOPTIONS_H
#define QG_POLYLINEEQUIDISTANTOPTIONS_H

#include "rs_actionpolylineequidistant.h"
#include "ui_qg_polylineequidistantoptions.h"

class RS_ActionInterface;
class RS_ActionDrawLineRelAngle;
/*
  * Create option widget used to draw equidistant polylines
  *
  *@Author Dongxu Li
 */

class QG_PolylineEquidistantOptions : public QWidget, public Ui::QG_PolylineEquidistantOptions
{
    Q_OBJECT

public:
    QG_PolylineEquidistantOptions(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~QG_PolylineEquidistantOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateDist( const QString & l );
    virtual void updateNumber( const QString & l );

protected:
    RS_ActionPolylineEquidistant* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_POLYLINEEQUIDISTANTOPTIONS_H
