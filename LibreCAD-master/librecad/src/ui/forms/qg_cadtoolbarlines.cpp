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
#include "qg_cadtoolbarlines.h"
#include "qg_cadtoolbar.h"
#include "qg_actionhandler.h"

/*
 *  Constructs a QG_CadToolBarLines as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarLines::QG_CadToolBarLines(QG_CadToolBar* parent, Qt::WindowFlags fl)
	:LC_CadToolBarInterface(parent, fl)
{
	initToolBars();
}

void QG_CadToolBarLines::addSubActions(const std::vector<QAction*>& actions, bool addGroup)
{
	LC_CadToolBarInterface::addSubActions(actions, addGroup);
	std::vector<QAction**> const &buttons=	{
		&bNormal, &bAngle, &bHorizontal, &bVertical, &bRectangle,
		&bParallel, &bParallelThrough, &bBisector, &bTangent1, &bTangent2, &bOrthTan,
		&bOrthogonal, &bRelAngle, &bPolygon, &bPolygon2, &bFree
	};

	assert(buttons.size()==actions.size());

	for(size_t i=0; i<buttons.size(); ++i)
		*buttons[i]=actions[i];
}

void QG_CadToolBarLines::drawLine() {
	if (actionHandler) {
        actionHandler->slotDrawLine();
    }
}

void QG_CadToolBarLines::drawLineAngle() {
	if (actionHandler) {
        actionHandler->slotDrawLineAngle();
    }
}

void QG_CadToolBarLines::drawLineHorizontal() {
	if (actionHandler) {
        actionHandler->slotDrawLineHorizontal();
    }
}

void QG_CadToolBarLines::drawLineHorVert() {
	if (actionHandler) {
        actionHandler->slotDrawLineHorVert();
    }
}

void QG_CadToolBarLines::drawLineVertical() {
	if (actionHandler) {
        actionHandler->slotDrawLineVertical();
    }
}

void QG_CadToolBarLines::drawLineParallel() {
	if (actionHandler) {
        actionHandler->slotDrawLineParallel();
    }
}

void QG_CadToolBarLines::drawLineParallelThrough() {
	if (actionHandler) {
        actionHandler->slotDrawLineParallelThrough();
    }
}

void QG_CadToolBarLines::drawLineRectangle() {
	if (actionHandler) {
        actionHandler->slotDrawLineRectangle();
    }
}

void QG_CadToolBarLines::drawLineBisector() {
	if (actionHandler) {
        actionHandler->slotDrawLineBisector();
    }
}

void QG_CadToolBarLines::drawLineTangent1() {
	if (actionHandler) {
        actionHandler->slotDrawLineTangent1();
    }
}

void QG_CadToolBarLines::drawLineTangent2() {
	if (actionHandler) {
        actionHandler->slotDrawLineTangent2();
    }
}

void QG_CadToolBarLines::drawLineOrthogonal() {
	if (actionHandler) {
        actionHandler->slotDrawLineOrthogonal();
    }
}

void QG_CadToolBarLines::drawLineOrthTan() {
	if (actionHandler) {
        actionHandler->slotDrawLineOrthTan();
    }
}

void QG_CadToolBarLines::drawLineRelAngle() {
	if (actionHandler) {
        actionHandler->slotDrawLineRelAngle();
    }
}

void QG_CadToolBarLines::drawLineFree() {
	if (actionHandler) {
        actionHandler->slotDrawLineFree();
    }
}

void QG_CadToolBarLines::drawLinePolygon() {
	if (actionHandler) {
        actionHandler->slotDrawLinePolygon();
    }
}

void QG_CadToolBarLines::drawLinePolygon2() {
	if (actionHandler) {
        actionHandler->slotDrawLinePolygon2();
    }
}

//restore action from checked
void QG_CadToolBarLines::restoreAction() {
	if(!actionHandler) return;
	if(!bNormal) return;
    if(bNormal->isChecked()) {
        actionHandler->slotDrawLine();
        return;
    }
    if(bAngle->isChecked()) {
        actionHandler->slotDrawLineAngle();
        return;
    }
    if(bHorizontal->isChecked()) {
        actionHandler->slotDrawLineHorizontal();
        return;
    }
    if(bVertical->isChecked()) {
        actionHandler->slotDrawLineVertical();
        return;
    }
    if(bRectangle->isChecked()) {
        actionHandler->slotDrawLineRectangle();
        return;
    }
    if(bBisector->isChecked()) {
        actionHandler->slotDrawLineBisector();
        return;
    }
    if(bParallel->isChecked()) {
        actionHandler->slotDrawLineParallel();
        return;
    }
    if(bParallelThrough->isChecked()) {
        actionHandler->slotDrawLineParallelThrough();
        return;
    }
    if(bTangent1->isChecked()) {
        actionHandler->slotDrawLineTangent1();
        return;
    }
    if(bTangent2->isChecked()) {
        actionHandler->slotDrawLineTangent2();
        return;
    }
    if(bOrthTan->isChecked()) {
        actionHandler->slotDrawLineOrthTan();
        return;
    }
    if(bOrthogonal->isChecked()) {
        actionHandler->slotDrawLineOrthogonal();
        return;
    }
    if(bRelAngle->isChecked()) {
        actionHandler->slotDrawLineRelAngle();
        return;
    }
    if(bPolygon->isChecked()) {
        actionHandler->slotDrawLinePolygon();
        return;
    }
    if(bPolygon2->isChecked()) {
        actionHandler->slotDrawLinePolygon2();
        return;
    }
    if(bFree->isChecked()) {
        actionHandler->slotDrawLineFree();
        return;
    }
    //clear all action
	m_pHidden->setChecked(true);
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
	if(currentAction ) {
        currentAction->finish(false); //finish the action, but do not update toolBar
    }
}

void QG_CadToolBarLines::resetToolBar()
{
	m_pHidden->setChecked(true);
}

void QG_CadToolBarLines::on_bBack_clicked()
{
	finishCurrentAction(true);
   LC_CadToolBarInterface::back();
}

void QG_CadToolBarLines::showCadToolBar(RS2::ActionType actionType) {
	if(!bNormal) return;
    switch(actionType){
    case RS2::ActionDrawLine:
        bNormal->setChecked(true);
        return;
    case RS2::ActionDrawLineAngle:
        bAngle->setChecked(true);
        return;
    case RS2::ActionDrawLineBisector:
        bBisector->setChecked(true);
        return;
    case RS2::ActionDrawLineFree:
        bFree->setChecked(true);
        return;
    case RS2::RS2::ActionDrawLineVertical:
        bVertical->setChecked(true);
        return;
    case RS2::ActionDrawLineHorizontal:
        bHorizontal->setChecked(true);
        return;
    case RS2::ActionDrawLineOrthogonal:
        bOrthogonal->setChecked(true);
        return;
    case RS2::ActionDrawLineOrthTan:
        bOrthTan->setChecked(true);
        return;
    case RS2::ActionDrawLineParallel:
        bParallel->setChecked(true);
        return;
    case RS2::ActionDrawLineParallelThrough:
        bParallelThrough->setChecked(true);
        return;
    case RS2::ActionDrawLinePolygonCenCor:
        bPolygon->setChecked(true);
        return;
    case RS2::ActionDrawLinePolygonCorCor:
        bPolygon2->setChecked(true);
        return;
    case RS2::ActionDrawLineRectangle:
        bRectangle->setChecked(true);
        return;
    case RS2::ActionDrawLineRelAngle:
        bRelAngle->setChecked(true);
        return;
    case RS2::ActionDrawLineTangent1:
        bTangent1->setChecked(true);
        return;
    case RS2::ActionDrawLineTangent2:
        bTangent2->setChecked(true);
        return;
        default:
		m_pHidden->setChecked(true);
        return;
    }
}
