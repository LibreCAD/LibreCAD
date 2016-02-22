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
#include<QVBoxLayout>
#include<QToolButton>
#include<QToolBar>
#include<QAction>
#include "qg_cadtoolbararcs.h"
#include "qg_cadtoolbar.h"
#include "qg_actionhandler.h"
#include "rs_debug.h"

/*
 *  Constructs a QG_CadToolBarArcs as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarArcs::QG_CadToolBarArcs(QG_CadToolBar* parent, Qt::WindowFlags fl):
	LC_CadToolBarInterface(parent, fl)
{
	initToolBars();
}

void QG_CadToolBarArcs::addSubActions(const std::vector<QAction*>& actions, bool addGroup)
{
	RS_DEBUG->print("QG_CadToolBarArcs::addSubActions(): begin\n");
	LC_CadToolBarInterface::addSubActions(actions, addGroup);
	std::vector<QAction**> const buttons={
		 &bArc, &bArc3P, &bArcParallel, &bArcTangential
	};
	assert(buttons.size()==actions.size());

	for(size_t i=0; i<buttons.size(); ++i)
		*buttons[i]=actions[i];
	RS_DEBUG->print("QG_CadToolBarArcs::addSubActions(): end\n");

}

//restore action from checked button
void QG_CadToolBarArcs::restoreAction()
{
	if(!(actionHandler&&bArc)) return;
    if ( bArc ->isChecked() ) {
        actionHandler->slotDrawArc();
        return;
    }
    if ( bArc3P ->isChecked() ) {
        actionHandler->slotDrawArc3P();
        return;
    }
    if ( bArcParallel ->isChecked() ) {
        actionHandler->slotDrawArcParallel();
        return;
    }
    if ( bArcTangential ->isChecked() ) {
        actionHandler->slotDrawArcTangential();
        return;
    }
    //clear all action
	m_pHidden->setChecked(true);
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
	if(currentAction ) {
        currentAction->finish(false); //finish the action, but do not update toolBar
    }
}

void QG_CadToolBarArcs::resetToolBar()
{
	m_pHidden->setChecked(true);
}

void QG_CadToolBarArcs::on_bBack_clicked()
{
	finishCurrentAction(true);
	cadToolBar->showPreviousToolBar();
}

void QG_CadToolBarArcs::showCadToolBar(RS2::ActionType actionType) {
	if(!bArc) return;
	switch(actionType){
	case RS2::ActionDrawArc:
		bArc->setChecked(true);
		return;
	case RS2::ActionDrawArc3P:
		bArc3P->setChecked(true);
		return;
	case RS2::ActionDrawArcParallel:
		bArcParallel->setChecked(true);
		return;
	case RS2::ActionDrawArcTangential:
		bArcTangential->setChecked(true);
		return;
	default:
		m_pHidden->setChecked(true);
		return;
	}
}
//EOF
