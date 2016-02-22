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
#include "qg_cadtoolbardim.h"
#include "qg_cadtoolbar.h"
#include "qg_actionhandler.h"

/*
 *  Constructs a QG_CadToolBarDim as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarDim::QG_CadToolBarDim(QG_CadToolBar* parent, Qt::WindowFlags fl)
	:LC_CadToolBarInterface(parent, fl)
{
	initToolBars();
}

void QG_CadToolBarDim::addSubActions(const std::vector<QAction*>& actions, bool addGroup)
{
	LC_CadToolBarInterface::addSubActions(actions, addGroup);
	std::vector<QAction**> const buttons={
		 &bAligned, &bLinear, &bLinearHor, &bLinearVer, &bRadial,
		 &bDiametric, &bAngular, &bLeader
	};

	assert(buttons.size()==actions.size());

	for(size_t i=0; i<buttons.size(); ++i)
		*buttons[i]=actions[i];
}

//restore action from checked button
void QG_CadToolBarDim::restoreAction()
{
	if(!(actionHandler && bAligned)) return;
	if ( bAligned ->isChecked() ) {
        actionHandler->slotDimAligned();
        return;
    }
    if ( bLinear ->isChecked() ) {
        actionHandler->slotDimLinear();
        return;
    }
    if ( bLinearHor ->isChecked() ) {
        actionHandler->slotDimLinearHor();
        return;
    }
    if ( bLinearVer ->isChecked() ) {
        actionHandler->slotDimLinearVer();
        return;
    }
    if ( bRadial ->isChecked() ) {
        actionHandler->slotDimRadial();
        return;
    }
    if ( bDiametric ->isChecked() ) {
        actionHandler->slotDimDiametric();
        return;
    }
    if ( bAngular ->isChecked() ) {
        actionHandler->slotDimAngular();
        return;
    }
    if ( bLeader ->isChecked() ) {
        actionHandler->slotDimLeader();
        return;
    }
    //clear all action
	m_pHidden->setChecked(true);
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
	if(currentAction ) {
        currentAction->finish(false); //finish the action, but do not update toolBar
    }
}
void QG_CadToolBarDim::resetToolBar()
{
	m_pHidden->setChecked(true);
}

void QG_CadToolBarDim::on_bBack_clicked()
{
	finishCurrentAction(true);
	cadToolBar->showPreviousToolBar();
}

void  QG_CadToolBarDim::showCadToolBar(RS2::ActionType actionType){
	if(!bAligned) return;
    switch(actionType){
    case RS2::ActionDimAligned:
        bAligned->setChecked(true);
        return;
    case RS2::ActionDimLinear:
        bLinear->setChecked(true);
        return;
    case RS2::ActionDimLinearVer:
        bLinearVer->setChecked(true);
        return;
    case RS2::ActionDimLinearHor:
        bLinearHor->setChecked(true);
        return;
    case RS2::ActionDimRadial:
        bRadial->setChecked(true);
        return;
    case RS2::ActionDimDiametric:
        bDiametric->setChecked(true);
        return;
    case RS2::ActionDimAngular:
        bAngular->setChecked(true);
        return;
    case RS2::ActionDimLeader:
        bLeader->setChecked(true);
        return;
    default:
		m_pHidden->setChecked(true);
        return;
    }
}
