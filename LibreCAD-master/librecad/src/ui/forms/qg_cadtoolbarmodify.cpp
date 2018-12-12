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
#include "qg_cadtoolbarmodify.h"
#include "qg_cadtoolbar.h"
#include "qg_actionhandler.h"

/*
 *  Constructs a QG_CadToolBarModify as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarModify::QG_CadToolBarModify(QG_CadToolBar* parent, Qt::WindowFlags fl)
	:LC_CadToolBarInterface(parent, fl)
{
	initToolBars();
}

void QG_CadToolBarModify::addSubActions(const std::vector<QAction*>& actions, bool addGroup)
{
	LC_CadToolBarInterface::addSubActions(actions, addGroup);
	std::vector<QAction**> const buttons={
		&bMove, &bRotate, &bScale, &bMirror, &bMoveRotate, &bRotate2,
		&bRevertDirection, &bTrim, &bTrim2, &bTrimAmount, &bOffset, &bBevel,
		&bRound, &bCut, &bStretch, &bEntity, &bAttributes, &bDelete,
		&bDeleteQuick, &bExplodeText, &bExplode
	};

	assert(buttons.size()==actions.size());

	for(size_t i=0; i<buttons.size(); ++i)
		*buttons[i]=actions[i];
}


//restore action from checked
void QG_CadToolBarModify::restoreAction() {
	if(!(actionHandler&&bMove)) return;
	if ( bMove ->isChecked() ) {
        actionHandler->slotModifyMove();
        return;
    }
    if ( bRotate ->isChecked() ) {
        actionHandler->slotModifyRotate();
        return;
    }
    if ( bScale ->isChecked() ) {
        actionHandler->slotModifyScale();
        return;
    }
    if ( bMirror ->isChecked() ) {
        actionHandler->slotModifyMirror();
        return;
    }
    if ( bMoveRotate ->isChecked() ) {
        actionHandler->slotModifyMoveRotate();
        return;
    }
    if ( bRotate2 ->isChecked() ) {
        actionHandler->slotModifyRotate2();
        return;
    }
    if ( bTrim ->isChecked() ) {
        actionHandler->slotModifyTrim();
        return;
    }
    if ( bTrim2 ->isChecked() ) {
        actionHandler->slotModifyTrim2();
        return;
    }
    if ( bTrimAmount ->isChecked() ) {
        actionHandler->slotModifyTrimAmount();
        return;
    }
    if ( bBevel ->isChecked() ) {
        actionHandler->slotModifyBevel();
        return;
    }
    if ( bRound ->isChecked() ) {
        actionHandler->slotModifyRound();
        return;
    }
    if ( bCut ->isChecked() ) {
        actionHandler->slotModifyCut();
        return;
    }
    if ( bStretch ->isChecked() ) {
        actionHandler->slotModifyStretch();
        return;
    }
    if ( bEntity ->isChecked() ) {
        actionHandler->slotModifyEntity();
        return;
    }
    if ( bAttributes ->isChecked() ) {
        actionHandler->slotModifyAttributes();
        return;
    }
    if ( bDelete ->isChecked() ) {
        actionHandler->slotModifyDelete();
        return;
    }
    if ( bExplode ->isChecked() ) {
        actionHandler->slotBlocksExplode();
        return;
    }
    if ( bExplodeText ->isChecked() ) {
        actionHandler->slotModifyExplodeText();
        return;
	}
    if ( bOffset ->isChecked() ) {
        actionHandler->slotModifyOffset();
        return;
    }
    if ( bRevertDirection ->isChecked() ) {
        actionHandler->slotModifyRevertDirection();
        return;
    }
	m_pHidden->setChecked(true);
    //clear all action
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
	if(currentAction ) {
        currentAction->finish(false); //finish the action, but do not update toolBar
    }
}

void QG_CadToolBarModify::resetToolBar()
{
	m_pHidden->setChecked(true);
}

void QG_CadToolBarModify::on_bBack_clicked()
{
	finishCurrentAction(true);
   cadToolBar->showToolBar(RS2::ToolBarMain);
}

void QG_CadToolBarModify::showCadToolBar(RS2::ActionType actionType) {
	if(!bAttributes) return;
    switch(actionType){
    case RS2::ActionModifyAttributes:
        bAttributes->setChecked(true);
        return;
    case RS2::ActionModifyDelete:
        bDelete->setChecked(true);
        return;
//    case RS2::ActionModifyDeleteQuick:
//    case RS2::ActionModifyDeleteFree:
    case RS2::ActionModifyMove:
        bMove->setChecked(true);
        return;
//    case RS2::ActionModifyMoveNoSelect:
    case RS2::ActionModifyRotate:
        bRotate->setChecked(true);
        return;
//    case RS2::ActionModifyRotateNoSelect:
    case RS2::ActionModifyScale:
        bScale->setChecked(true);
        return;
//    case RS2::ActionModifyScaleNoSelect:
    case RS2::ActionModifyMirror:
        bMirror->setChecked(true);
        return;
//    case RS2::ActionModifyMirrorNoSelect:
    case RS2::ActionModifyMoveRotate:
        bMoveRotate->setChecked(true);
        return;
//    case RS2::ActionModifyMoveRotateNoSelect:
    case RS2::ActionModifyRotate2:
        bRotate2->setChecked(true);
        return;
//    case RS2::ActionModifyRotate2NoSelect:
    case RS2::ActionModifyEntity:
        bEntity->setChecked(true);
        return;
    case RS2::ActionModifyTrim:
        bTrim->setChecked(true);
        return;
    case RS2::ActionModifyTrim2:
        bTrim2->setChecked(true);
        return;
    case RS2::ActionModifyTrimAmount:
        bTrimAmount->setChecked(true);
        return;
    case RS2::ActionModifyCut:
        bCut->setChecked(true);
        return;
    case RS2::ActionModifyStretch:
        bStretch->setChecked(true);
        return;
    case RS2::ActionModifyBevel:
        bBevel->setChecked(true);
        return;
    case RS2::ActionModifyRound:
        bRound->setChecked(true);
        return;
    case RS2::ActionModifyOffset:
        bOffset->setChecked(true);
        return;
    case RS2::ActionModifyRevertDirection:
        bRevertDirection->setChecked(true);
        return;
//    case RS2::ActionModifyExplodeText:
//        bEntityText->setChecked(true);
        //    case RS2::ActionModifyOffsetNoSelect:
    default:
		m_pHidden->setChecked(true);
        return;
    }
}
