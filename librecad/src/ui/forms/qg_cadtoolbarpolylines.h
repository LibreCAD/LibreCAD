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
#ifndef QG_CADTOOLBARPOLYLINES_H
#define QG_CADTOOLBARPOLYLINES_H

class QG_CadToolBar;
class QG_ActionHandler;

#include "lc_cadtoolbarinterface.h"

class QG_CadToolBarPolylines : public LC_CadToolBarInterface
{
    Q_OBJECT

public:
	QG_CadToolBarPolylines(QG_CadToolBar* parent = 0, Qt::WindowFlags fl = 0);
	~QG_CadToolBarPolylines() = default;
    //restore action from checked button
	virtual void restoreAction();
	RS2::ToolBarId rtti() const
	{
		return RS2::ToolBarPolylines;
	}
	virtual void addSubActions(const std::vector<QAction*>& actions, bool addGroup=true);

public slots:
    virtual void resetToolBar();
    virtual void showCadToolBar(RS2::ActionType actionType);

private slots:
    void on_bBack_clicked();
private:
	QAction *bPolyline=nullptr,*bPolylineAdd=nullptr,*bPolylineAppend=nullptr,*bPolylineDel=nullptr,
	*bPolylineDelBetween=nullptr,*bPolylineTrim=nullptr,*bPolylineEquidistant=nullptr,*bPolylineSegment=nullptr;
};

#endif // QG_CADTOOLBARPOLYLINES_H
