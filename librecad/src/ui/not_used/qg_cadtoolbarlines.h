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
#ifndef QG_CADTOOLBARLINES_H
#define QG_CADTOOLBARLINES_H

class QG_CadToolBar;

#include "qg_actionhandler.h"
#include "lc_cadtoolbarinterface.h"

class QG_CadToolBarLines : public LC_CadToolBarInterface
{
    Q_OBJECT

public:
	QG_CadToolBarLines(QG_CadToolBar* parent = 0, Qt::WindowFlags fl = 0);
	~QG_CadToolBarLines() = default;
	virtual void restoreAction(); //restore action from checked button
	RS2::ToolBarId rtti() const
	{
		return RS2::ToolBarLines;
	}
	virtual void addSubActions(const std::vector<QAction*>& actions, bool addGroup=true);

public slots:
    virtual void drawLine();
    virtual void drawLineAngle();
    virtual void drawLineHorizontal();
    virtual void drawLineHorVert();
    virtual void drawLineVertical();
    virtual void drawLineParallel();
    virtual void drawLineParallelThrough();
    virtual void drawLineRectangle();
    virtual void drawLineBisector();
    virtual void drawLineTangent1();
    virtual void drawLineTangent2();
    virtual void drawLineOrthogonal();
    virtual void drawLineOrthTan();
    virtual void drawLineRelAngle();
    virtual void drawLineFree();
    virtual void drawLinePolygon();
	virtual void drawLinePolygon2();
    virtual void resetToolBar();
    virtual void showCadToolBar(RS2::ActionType actionType);

private slots:
	void on_bBack_clicked();
private:
	QAction *bNormal=nullptr, *bAngle=nullptr, *bHorizontal=nullptr, *bVertical=nullptr, *bRectangle=nullptr, *bBisector=nullptr,
	*bParallel=nullptr, *bParallelThrough=nullptr, *bTangent1=nullptr, *bTangent2=nullptr, *bOrthTan=nullptr,
	*bOrthogonal=nullptr, *bRelAngle=nullptr, *bPolygon=nullptr, *bPolygon2=nullptr, *bFree=nullptr;
};

#endif // QG_CADTOOLBARLINES_H
