/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2014 Dongxu Li (dongxuli2011@gmail.com)

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
#ifndef QG_CADTOOLBARSPLINES_H
#define QG_CADTOOLBARSPLINES_H

class QG_CadToolBar;
class QG_ActionHandler;

#include "lc_cadtoolbarinterface.h"

class QG_CadToolBarSplines : public LC_CadToolBarInterface
{
    Q_OBJECT

public:
	QG_CadToolBarSplines(QG_CadToolBar* parent = 0, Qt::WindowFlags fl = 0);
	~QG_CadToolBarSplines() = default;
    //restore action from checked button
	virtual void restoreAction();
	RS2::ToolBarId rtti() const
	{
		return RS2::ToolBarSplines;
	}
	virtual void addSubActions(const std::vector<QAction*>& actions, bool addGroup);


public slots:
    virtual void resetToolBar();
    virtual void showCadToolBar(RS2::ActionType actionType);

private slots:
	void on_bBack_clicked();
private:
	QAction *bSpline=nullptr, *bSplineInt=nullptr;
};

#endif // QG_CADTOOLBARSPLINES_H
