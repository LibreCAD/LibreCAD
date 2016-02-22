/****************************************************************************
**
  * snap to equidistant points on entities

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

#ifndef QG_SNAPMIDDLEOPTIONS_H
#define QG_SNAPMIDDLEOPTIONS_H

#include<memory>
#include<QWidget>

namespace Ui {
class Ui_SnapMiddleOptions;
}

/**
  * snap to equidistant points on entities
  *@Author: Dongxu Li
  */
class QG_SnapMiddleOptions : public QWidget
{
    Q_OBJECT

public:
    QG_SnapMiddleOptions(int& i, QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~QG_SnapMiddleOptions();

public slots:
    virtual void setMiddlePoints(int& i, bool initial=true);
    virtual void updateMiddlePoints();

protected:
    int* middlePoints;

protected slots:
    virtual void languageChange();

private slots:
    void on_sbMiddlePoints_valueChanged(int arg1);

private:
	void saveSettings();
	std::unique_ptr<Ui::Ui_SnapMiddleOptions> ui;
};

#endif // QG_SNAPMIDDLEOPTIONS_H
