/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2012 LibreCAD.org

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
#ifndef QG_CIRCLETAN2OPTIONS_H
#define QG_CIRCLETAN2OPTIONS_H

#include<memory>
#include<QWidget>

class RS_ActionInterface;
class RS_ActionDrawCircleTan2;

namespace Ui {
class Ui_CircleTan2Options;
}

class QG_CircleTan2Options : public QWidget
{
    Q_OBJECT

public:
    QG_CircleTan2Options(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~QG_CircleTan2Options();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateRadius( const QString & l );

protected:
    RS_ActionDrawCircleTan2* action;

protected slots:
    virtual void languageChange();

private:
	void saveSettings();
	std::unique_ptr<Ui::Ui_CircleTan2Options> ui;
};

#endif
// QG_CIRCLETAN2OPTIONS_H
