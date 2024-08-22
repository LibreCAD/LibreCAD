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
#include "qg_cadtoolbarinfo.h"
#include "qg_cadtoolbar.h"
#include "qg_actionhandler.h"

/*
 *  Constructs a QG_CadToolBarInfo as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarInfo::QG_CadToolBarInfo(QG_CadToolBar* parent, Qt::WindowFlags fl)
	:LC_CadToolBarInterface(parent, fl)
{
	initToolBars();
}

void QG_CadToolBarInfo::addSubActions(const std::vector<QAction*>& actions, bool addGroup)
{
	LC_CadToolBarInterface::addSubActions(actions, addGroup);
	std::vector<QAction**> const buttons={
		 &bDist, &bDist2, &bAngle, &bTotalLength, &bArea
	};

	assert(buttons.size()==actions.size());

	for(size_t i=0; i<buttons.size(); ++i)
		*buttons[i]=actions[i];
}

//restore action from checked button
void QG_CadToolBarInfo::restoreAction()
{
	if(!(actionHandler && bDist)) return;
	//clear all action
    if ( bDist ->isChecked() ) {
        actionHandler->slotInfoDist();
        return;
    }
    if ( bDist2 ->isChecked() ) {
        actionHandler->slotInfoDist2();
        return;
    }
    if ( bAngle ->isChecked() ) {
        actionHandler->slotInfoAngle();
        return;
    }
    if ( bTotalLength ->isChecked() ) {
        actionHandler->slotInfoTotalLength();
        return;
    }
    if ( bArea ->isChecked() ) {
        actionHandler->slotInfoArea();
        return;
    }
    //default to measure point to point distance
    //bDist->setChecked(true);
    //actionHandler->slotInfoDist();
	m_pHidden->setChecked(true);
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
	if(currentAction ) {
        currentAction->finish(false); //finish the action, but do not update toolBar
    }
}
void QG_CadToolBarInfo::resetToolBar()
{
	m_pHidden->setChecked(true);
}

void QG_CadToolBarInfo::on_bBack_clicked()
{
	finishCurrentAction(true);
	cadToolBar->showPreviousToolBar();
}

void QG_CadToolBarInfo:: showCadToolBar(RS2::ActionType actionType){
	if(!bDist) return;
    switch(actionType){
//    case RS2::ActionInfoInside:
    case RS2::ActionInfoDist:
        bDist->setChecked(true);
        return;
    case RS2::ActionInfoDist2:
        bDist2->setChecked(true);
        return;
    case RS2::ActionInfoAngle:
        bAngle->setChecked(true);
        return;
    case RS2::ActionInfoTotalLength:
        bTotalLength->setChecked(true);
        return;
        //    case RS2::ActionInfoTotalLengthNoSelect:
    case RS2::ActionInfoArea:
        bArea->setChecked(true);
        return;
    default:
		m_pHidden->setChecked(true);
    }
}

//EOF
