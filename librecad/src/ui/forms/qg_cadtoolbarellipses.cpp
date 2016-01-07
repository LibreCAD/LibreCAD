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
#include<cassert>
#include<QAction>
#include "qg_cadtoolbarellipses.h"
#include "qg_cadtoolbar.h"
#include "qg_actionhandler.h"

/*
 *  Constructs a QG_CadToolBarEllipses as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarEllipses::QG_CadToolBarEllipses(QG_CadToolBar* parent, Qt::WindowFlags fl)
	:LC_CadToolBarInterface(parent, fl)
{
	initToolBars();
}


void QG_CadToolBarEllipses::addSubActions(const std::vector<QAction*>& actions, bool addGroup)
{
	LC_CadToolBarInterface::addSubActions(actions, addGroup);
	std::vector<QAction**> const buttons={
		&bEllipseAxes, &bEllipseArcAxes , &bEllipseFociPoint ,
		&bEllipse4Points , &bEllipseCenter3Points , &bEllipseInscribe
	};

	assert(buttons.size()==actions.size());

	for(size_t i=0; i<buttons.size(); ++i)
		*buttons[i]=actions[i];
}

//restore action from checked button
void QG_CadToolBarEllipses::restoreAction()
{
	if(!(actionHandler && bEllipseAxes)) return;
	if ( bEllipseAxes ->isChecked() ) {
        actionHandler->slotDrawEllipseAxis();
        return;
    }
    if ( bEllipseArcAxes ->isChecked() ) {
        actionHandler->slotDrawEllipseArcAxis();
        return;
    }
    if ( bEllipseFociPoint ->isChecked() ) {
        actionHandler->slotDrawEllipseFociPoint();
        return;
    }
    if ( bEllipse4Points ->isChecked() ) {
        actionHandler->slotDrawEllipse4Points();
        return;
    }
    if ( bEllipseCenter3Points ->isChecked() ) {
        actionHandler->slotDrawEllipseCenter3Points();
        return;
    }
    if ( bEllipseInscribe ->isChecked() ) {
        actionHandler->slotDrawEllipseInscribe();
        return;
    }
    //clear all action
	m_pHidden->setChecked(true);
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
	if(currentAction ) {
        currentAction->finish(false); //finish the action, but do not update toolBar
    }
}

void QG_CadToolBarEllipses::resetToolBar()
{
	m_pHidden->setChecked(true);
}
void QG_CadToolBarEllipses::on_bBack_clicked()
{
	finishCurrentAction(true);
	cadToolBar->showPreviousToolBar();
}


void QG_CadToolBarEllipses::showCadToolBar(RS2::ActionType actionType) {
	if(!bEllipseAxes) return;
    switch(actionType){
    case RS2::ActionDrawEllipseAxis:
        bEllipseAxes ->setChecked(true);
        return;
    case RS2::ActionDrawEllipseArcAxis:
        bEllipseArcAxes ->setChecked(true);
        return;
    case RS2::ActionDrawEllipseFociPoint:
        bEllipseFociPoint ->setChecked(true);
        return;
    case RS2::ActionDrawEllipse4Points:
        bEllipse4Points ->setChecked(true);
        return;
    case RS2::ActionDrawEllipseCenter3Points:
        bEllipseCenter3Points ->setChecked(true);
        return;
    case RS2::ActionDrawEllipseInscribe:
        bEllipseInscribe ->setChecked(true);
        return;
        default:
		m_pHidden->setChecked(true);
        return;
    }
}
