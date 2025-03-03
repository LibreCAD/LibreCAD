/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 Dongxu Li (dongxuli2011 at gmail.com)
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
#ifndef QG_CADTOOLBARSELECT_H
#define QG_CADTOOLBARSELECT_H

class QG_CadToolBar;
class QG_ActionHandler;
class RS_ActionInterface;

#include "lc_cadtoolbarinterface.h"

class QG_CadToolBarSelect : public LC_CadToolBarInterface
{
    Q_OBJECT

public:
	QG_CadToolBarSelect(QG_CadToolBar* parent = 0, Qt::WindowFlags fl = 0);
	~QG_CadToolBarSelect() = default;
	RS2::ToolBarId rtti() const
	{
		return RS2::ToolBarSelect;
	}

public slots:
    virtual void setSelectAction( RS_ActionInterface * selectAction );
    virtual void setNextAction( int nextAction );
	virtual void runNextAction();
	virtual void mousePressEvent( QMouseEvent * e );

private slots:
    void on_bBack_clicked();

private:
    int nextAction;
	RS_ActionInterface* selectAction;
};

#endif // QG_CADTOOLBARSELECT_H
