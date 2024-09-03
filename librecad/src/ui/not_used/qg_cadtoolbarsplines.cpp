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
#include<cassert>
#include<QAction>
#include "qg_cadtoolbarsplines.h"
#include "qg_cadtoolbar.h"
#include "qg_actionhandler.h"

/*
 *  Constructs a QG_CadToolBarSplines as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarSplines::QG_CadToolBarSplines(QG_CadToolBar* parent, Qt::WindowFlags fl)
	:LC_CadToolBarInterface(parent, fl)
{
	initToolBars();
}

void QG_CadToolBarSplines::addSubActions(const std::vector<QAction*>& actions, bool addGroup)
{
	LC_CadToolBarInterface::addSubActions(actions, addGroup);
	std::vector<QAction**> const buttons={&bSpline, &bSplineInt};
	assert(buttons.size()==actions.size());

	for(size_t i=0; i<buttons.size(); ++i)
		*buttons[i]=actions[i];

}

//restore action from checked button
void QG_CadToolBarSplines::restoreAction()
{
	if(!(actionHandler&&bSpline)) return;
    if ( bSpline ->isChecked() ) {
        actionHandler->slotDrawSpline();
        return;
    }
    if(bSplineInt->isChecked()){
        actionHandler->slotDrawSplinePoints();
        return;
    }
	m_pHidden->setChecked(true);
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
	if(currentAction ) {
        currentAction->finish(false); //finish the action, but do not update toolBar
    }
}

void QG_CadToolBarSplines::resetToolBar()
{
	m_pHidden->setChecked(true);
}


void QG_CadToolBarSplines::showCadToolBar(RS2::ActionType actionType) {
	if(!bSpline) return;
    switch(actionType){
    case RS2::ActionDrawSpline:
        bSpline->setChecked(true);
        return;
    case RS2::ActionDrawSplinePoints:
        bSplineInt->setChecked(true);
        return;
    default:
		m_pHidden->setChecked(true);
        return;
    }
}

void QG_CadToolBarSplines::on_bBack_clicked()
{
	finishCurrentAction(true);
   LC_CadToolBarInterface::back();
}
