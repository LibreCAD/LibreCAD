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
#include "qg_cadtoolbarpolylines.h"
#include "qg_cadtoolbar.h"
#include "qg_actionhandler.h"

/*
 *  Constructs a QG_CadToolBarPolylines as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarPolylines::QG_CadToolBarPolylines(QG_CadToolBar* parent, Qt::WindowFlags fl)
	:LC_CadToolBarInterface(parent, fl)
{
	initToolBars();
}


void QG_CadToolBarPolylines::addSubActions(const std::vector<QAction*>& actions, bool addGroup)
{
	LC_CadToolBarInterface::addSubActions(actions, addGroup);
	std::vector<QAction**> const buttons={
		&bPolyline, &bPolylineAdd, &bPolylineAppend, &bPolylineDel,
		&bPolylineDelBetween, &bPolylineTrim, &bPolylineEquidistant,
		&bPolylineSegment
	};

	assert(buttons.size()==actions.size());

	for(size_t i=0; i<buttons.size(); ++i)
		*buttons[i]=actions[i];
}

//restore action from checked button
void QG_CadToolBarPolylines::restoreAction()
{
	if(!(actionHandler && bPolyline)) return;
	if ( bPolyline ->isChecked() ) {
        actionHandler->slotDrawPolyline();
        return;
    }
    if ( bPolylineAdd ->isChecked() ) {
        actionHandler->slotPolylineAdd();
        return;
    }
    if ( bPolylineAppend ->isChecked() ) {
        actionHandler->slotPolylineAppend();
        return;
    }
    if ( bPolylineDel ->isChecked() ) {
        actionHandler->slotPolylineDel();
        return;
    }
    if ( bPolylineDelBetween ->isChecked() ) {
        actionHandler->slotPolylineDelBetween();
        return;
    }
    if ( bPolylineTrim ->isChecked() ) {
        actionHandler->slotPolylineTrim();
        return;
    }
    if ( bPolylineEquidistant ->isChecked() ) {
        actionHandler->slotPolylineEquidistant();
        return;
    }
    if ( bPolylineSegment ->isChecked() ) {
        actionHandler->slotPolylineSegment();
        return;
    }
	m_pHidden->setChecked(true);
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
	if(currentAction ) {
        currentAction->finish(false); //finish the action, but do not update toolBar
    }
}

void QG_CadToolBarPolylines::resetToolBar()
{
	m_pHidden->setChecked(true);
}

void QG_CadToolBarPolylines::on_bBack_clicked()
{
	finishCurrentAction(true);
	cadToolBar->showPreviousToolBar();
}

void QG_CadToolBarPolylines::showCadToolBar(RS2::ActionType actionType){
	if(!bPolyline) return;
    switch(actionType){
    case RS2::ActionDrawPolyline:
        bPolyline->setChecked(true);
        return;
    case RS2::ActionPolylineAdd:
        bPolylineAdd->setChecked(true);
        return;
    case RS2::ActionPolylineAppend:
        bPolylineAppend->setChecked(true);
        return;
    case RS2::ActionPolylineDel:
        bPolylineDel->setChecked(true);
        return;
    case RS2::ActionPolylineDelBetween:
        bPolylineDelBetween->setChecked(true);
        return;
    case RS2::ActionPolylineTrim:
        bPolylineTrim->setChecked(true);
        return;
    case RS2::ActionPolylineEquidistant:
        bPolylineEquidistant->setChecked(true);
        return;
    case RS2::ActionPolylineSegment:
        bPolylineSegment->setChecked(true);
        return;
        default:
		m_pHidden->setChecked(true);
        return;
    }
}

//EOF
