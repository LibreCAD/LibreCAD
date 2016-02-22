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
#include "qg_cadtoolbarcircles.h"
#include "qg_cadtoolbar.h"
#include "qg_actionhandler.h"

/*
 *  Constructs a QG_CadToolBarCircles as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarCircles::QG_CadToolBarCircles(QG_CadToolBar* parent, Qt::WindowFlags fl)
	:LC_CadToolBarInterface(parent, fl)
{
	initToolBars();
}


void QG_CadToolBarCircles::addSubActions(const std::vector<QAction*>& actions, bool addGroup)
{
	LC_CadToolBarInterface::addSubActions(actions, addGroup);
	std::vector<QAction**> const buttons=	{
		&bCircle, &bCircleCR, &bCircle2P, &bCircle2PR, &bCircle3P,
		&bCircleParallel, &bCircleInscribe, &bCircleTan1_2P, &bCircleTan2,
		&bCircleTan2_1P, &bCircleTan3
	};

	assert(buttons.size()==actions.size());

	for(size_t i=0; i<buttons.size(); ++i)
		*buttons[i]=actions[i];
}

void QG_CadToolBarCircles::back() {
	if (cadToolBar) {
        cadToolBar->back();
    }
}
//restore action from checked button
void QG_CadToolBarCircles::restoreAction()
{
	if(!(actionHandler && bCircle)) return;
    if ( bCircle ->isChecked() ) {
        actionHandler->slotDrawCircle();
        return;
    }
    if ( bCircleCR ->isChecked() ) {
        actionHandler->slotDrawCircleCR();
        return;
    }
    if ( bCircle2P ->isChecked() ) {
        actionHandler->slotDrawCircle2P();
        return;
    }
    if ( bCircle2PR ->isChecked() ) {
        actionHandler->slotDrawCircle2PR();
        return;
    }
    if ( bCircle3P ->isChecked() ) {
        actionHandler->slotDrawCircle3P();
        return;
    }
    if ( bCircleParallel ->isChecked() ) {
        actionHandler->slotDrawCircleParallel();
        return;
    }
    if ( bCircleInscribe ->isChecked() ) {
        actionHandler->slotDrawCircleInscribe();
        return;
    }
    if ( bCircleTan2 ->isChecked() ) {
        actionHandler->slotDrawCircleTan2();
        return;
    }
    if ( bCircleTan3 ->isChecked() ) {
        actionHandler->slotDrawCircleTan3();
        return;
    }
    if(bCircleTan2_1P->isChecked()){
        actionHandler->slotDrawCircleTan2_1P();
        return;
    }
    if(bCircleTan1_2P->isChecked()){
        actionHandler->slotDrawCircleTan1_2P();
        return;
    }
    //clear all action
	m_pHidden->setChecked(true);
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
	if(currentAction ) {
        currentAction->finish(false); //finish the action, but do not update toolBar
    }
}

void QG_CadToolBarCircles::resetToolBar()
{
	m_pHidden->setChecked(true);
}

void QG_CadToolBarCircles::on_bBack_clicked()
{
	finishCurrentAction(true);
	cadToolBar->showPreviousToolBar();
}

void QG_CadToolBarCircles::showCadToolBar(RS2::ActionType actionType){
	if(!bCircle) return;
    switch(actionType){
    case RS2::ActionDrawCircle:
        bCircle->setChecked(true);
        return;
    case RS2::ActionDrawCircle2P:
        bCircle2P->setChecked(true);
        return;
    case RS2::ActionDrawCircle2PR:
        bCircle2PR->setChecked(true);
        return;
    case RS2::ActionDrawCircle3P:
        bCircle3P->setChecked(true);
        return;
    case RS2::ActionDrawCircleCR:
        bCircleCR->setChecked(true);
        return;
    case RS2::ActionDrawCircleParallel:
        bCircleParallel->setChecked(true);
        return;
    case RS2::ActionDrawCircleInscribe:
        bCircleInscribe->setChecked(true);
        return;
    case RS2::ActionDrawCircleTan2:
        bCircleTan2->setChecked(true);
        return;
    case RS2::ActionDrawCircleTan3:
        bCircleTan3->setChecked(true);
        return;
    case RS2::ActionDrawCircleTan1_2P:
        bCircleTan1_2P->setChecked(true);
        return;
    case RS2::ActionDrawCircleTan2_1P:
        bCircleTan2_1P->setChecked(true);
        return;
    default:
		m_pHidden->setChecked(true);
        return;
    }
}
//EOF
