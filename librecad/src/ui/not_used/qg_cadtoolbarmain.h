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
#ifndef QG_CADTOOLBARMAIN_H
#define QG_CADTOOLBARMAIN_H

#include "lc_cadtoolbarinterface.h"

class QG_CadToolBar;
class QG_ActionHandler;

class QG_CadToolBarMain : public LC_CadToolBarInterface
{
    Q_OBJECT

public:
	QG_CadToolBarMain(QG_CadToolBar* parent = 0, Qt::WindowFlags fl = 0);
	~QG_CadToolBarMain() = default;
	virtual void restoreAction(); //restore action from checked button
    void finishCurrentAction(bool resetToolBar=false); //clear current action
    void resetToolBar();
	virtual void setActionHandler(QG_ActionHandler* ah);
    virtual void showCadToolBar(RS2::ActionType actionType);
	RS2::ToolBarId rtti() const
	{
		return RS2::ToolBarMain;
	}
	virtual void addSubActions(const std::vector<QAction*>& actions, bool addGroup=true);

public slots:
	virtual void mousePressEvent( QMouseEvent * e );

private slots:
    void slotDrawMText();
	void slotDrawImage();
private:
	QAction *bMenuImage=nullptr, *bMenuPoint=nullptr, *bMenuText=nullptr, *bMenuBlock=nullptr;
	QAction *bMenuLine=nullptr, *bMenuArc=nullptr, *bMenuCircle=nullptr, *bMenuEllipse=nullptr, *bMenuSpline=nullptr, *bMenuPolyline=nullptr, *bMenuDim=nullptr, *bMenuHatch=nullptr, *bMenuModify=nullptr, *bMenuInfo=nullptr,
	*bMenuSelect=nullptr;
};
#endif
