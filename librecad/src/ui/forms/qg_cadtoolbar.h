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
#ifndef QG_CADTOOLBAR_H
#define QG_CADTOOLBAR_H

#include <QToolBar>
#include <vector>
#include <map>
#include "rs.h"

class QG_ActionHandler;
class LC_CadToolBarInterface;
class RS_ActionInterface;
class QActions;

class QG_CadToolBar : public QToolBar
{
    Q_OBJECT

public:
    QG_CadToolBar(QWidget* parent = 0, const char* name = 0);
	~QG_CadToolBar() = default;

	/**
	 * @brief populateSubToolBars add actions to subtoolbars
	 * @param actions
	 */
	void populateSubToolBar(const std::vector<QAction*>& actions, RS2::ToolBarId toolbarID);
    virtual QSize 	sizeHint() const;

public slots:
	void back();
	void forceNext();
	void mouseReleaseEvent( QMouseEvent * e );
	void contextMenuEvent( QContextMenuEvent * e );
	void setActionHandler( QG_ActionHandler * ah );
	/** \brief showToolBar show the toolbar by id
      * if restoreAction is true, also, start the action specified by the checked button of the toolbar
      **/
	void showToolBar(RS2::ToolBarId id, bool restoreAction = true );
	void showToolBarMain();
	void showToolBarLines();
	void showToolBarArcs();
	void showToolBarEllipses();
	void showToolBarSplines();
	void showToolBarPolylines();
	void showToolBarCircles();
	void showToolBarInfo();
	void showToolBarModify();
	void showToolBarDim();
	void showToolBarSelect();
	void showToolBarSelect( RS_ActionInterface * selectAction, int nextAction );
	void showPreviousToolBar(bool cleanup = true);
	void showCadToolBar(RS2::ActionType actionType, bool cleanup=false);
	void resetToolBar();

signals:
    void signalBack();
    void signalNext();

protected:

	/**
	 * @brief m_toolbars holds cad toolbars managed by this LC_CadToolBar instance
	 */
	std::map<RS2::ToolBarId, LC_CadToolBarInterface*> m_toolbars;
	QG_ActionHandler* actionHandler;

	std::vector<LC_CadToolBarInterface*> activeToolbars;

protected slots:
//    virtual void languageChange();
    void hideSubToolBars();
    void showSubToolBar();

private:
	void init();
	void finishCurrentAction(bool resetToolBar=false);
};

#endif // QG_CADTOOLBAR_H
