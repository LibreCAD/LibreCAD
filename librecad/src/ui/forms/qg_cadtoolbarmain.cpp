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
#include<QCoreApplication>
#include<QMouseEvent>
#include<QAction>
#include<tuple>
#include<utility>
#include "qg_cadtoolbarmain.h"
#include "qg_cadtoolbar.h"
#include "qg_actionhandler.h"

/*
 *  Constructs a QG_CadToolBarMain as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarMain::QG_CadToolBarMain(QG_CadToolBar* parent, Qt::WindowFlags fl)
	:LC_CadToolBarInterface(parent, fl)
{
	initToolBars();
}

void QG_CadToolBarMain::addSubActions(const std::vector<QAction*>& actions, bool /*addGroup*/)
{
	const std::initializer_list<std::pair<QAction**, RS2::ActionType>> actionTypes=
	{
		std::make_pair(&bMenuText, RS2::ActionDrawMText),
		std::make_pair(&bMenuImage, RS2::ActionDrawImage),
		std::make_pair(&bMenuPoint, RS2::ActionDrawPoint),
		std::make_pair(&bMenuBlock, RS2::ActionBlocksCreate),
		std::make_pair(&bMenuHatch, RS2::ActionDrawHatch)
	};
	for(auto a: actions){
		auto it0=std::find_if(actionTypes.begin(), actionTypes.end(),
							  [&a](const std::pair<QAction**, RS2::ActionType>& a0)->bool{
			return a->data() == a0.second;
		});
		if(it0==actionTypes.end()) return;
		* it0->first = a;
	}
	if(std::any_of(actionTypes.begin(), actionTypes.end(),
				   [](const std::pair<QAction**, RS2::ActionType>& a)->bool{
				   return !*(a.first);
})) return;
	const std::initializer_list<std::tuple<QAction**, QString, const char*>> buttons={
		std::make_tuple(&bMenuLine, "menuline", R"(Show toolbar "Lines")"),
		std::make_tuple(&bMenuArc, "menuarc", R"(Show toolbar "Arcs")"),
		std::make_tuple(&bMenuCircle, "menucircle", R"(Show toolbar "Circles")"),
		std::make_tuple(&bMenuEllipse, "menuellipse", R"(Show toolbar "Ellipses")"),
		std::make_tuple(&bMenuPolyline, "menupolyline", R"(Show toolbar "Polylines")"),
		std::make_tuple(&bMenuSpline, "menuspline", R"(Show toolbar "Splines")"),
		std::make_tuple(&bMenuDim, "dimhor", R"(Show toolbar "Dimensions")"),
//		std::make_tuple(&bMenuHatch, "menuhatch", R"(Create Hatch)"),
		std::make_tuple(&bMenuModify, "menuedit", R"(Show toolbar "Modify")"),
		std::make_tuple(&bMenuInfo, "menumeasure", R"(Show toolbar "Info")"),
		std::make_tuple(&bMenuSelect, "menuselect", R"(Show toolbar "Select")")
	};
	std::vector<QAction*> listAction;
	for(const auto& a: buttons){
		QAction* p=new QAction(QIcon(":/extui/"+std::get<1>(a)+".png"),
					  QCoreApplication::translate("main", std::get<2>(a)), this);
		*std::get<0>(a)=p;
		listAction.push_back(p);
	}
	auto it = std::find(listAction.begin(), listAction.end(),bMenuDim);

	//add draw actions
	listAction.insert(it, bMenuText);

	QAction* nullAction=new QAction(this);
	nullAction->setEnabled(false);
	listAction.insert(it, nullAction);

	listAction.insert(it, bMenuPoint);

	it = std::find(listAction.begin(), listAction.end(),bMenuModify);
	listAction.insert(it, bMenuImage);
	it = std::find(listAction.begin(), listAction.end(),bMenuSelect);
	listAction.insert(it, bMenuBlock);
	it = std::find(listAction.begin(), listAction.end(),bMenuImage);
	listAction.insert(it, bMenuHatch);
	LC_CadToolBarInterface::addSubActions(listAction, false);
	for(auto a: actionTypes){
		(*a.first)->setCheckable(true);
		m_pActionGroup->addAction(*a.first);
	}
	if(actionHandler)
		setActionHandler(actionHandler);
}

void QG_CadToolBarMain::setActionHandler(QG_ActionHandler* ah)
{
	actionHandler=ah;
	if(!bMenuLine) return;
	connect(bMenuLine, SIGNAL(triggered()),
			cadToolBar, SLOT(showToolBarLines()));
	connect(bMenuArc, SIGNAL(triggered()),
			cadToolBar, SLOT(showToolBarArcs()));
	connect(bMenuCircle, SIGNAL(triggered()),
			cadToolBar, SLOT(showToolBarCircles()));
	connect(bMenuEllipse, SIGNAL(triggered()),
			cadToolBar, SLOT(showToolBarEllipses()));
	connect(bMenuSpline, SIGNAL(triggered()),
			cadToolBar, SLOT(showToolBarSplines()));
	connect(bMenuPolyline, SIGNAL(triggered()),
			cadToolBar, SLOT(showToolBarPolylines()));

	connect(bMenuDim, SIGNAL(triggered()),
			cadToolBar, SLOT(showToolBarDim()));

	connect(bMenuModify, SIGNAL(triggered()),
			cadToolBar, SLOT(showToolBarModify()));
	connect(bMenuInfo, SIGNAL(triggered()),
			cadToolBar, SLOT(showToolBarInfo()));

	connect(bMenuBlock, SIGNAL(triggered()),
			actionHandler, SLOT(slotBlocksCreate()));
	connect(bMenuSelect, SIGNAL(triggered()),
			cadToolBar, SLOT(showToolBarSelect()));

}

//clear current action
void QG_CadToolBarMain::finishCurrentAction(bool resetToolBar)
{
	if(!actionHandler) return;
    RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
	if(currentAction ) {
        currentAction->finish(resetToolBar); //finish the action, but do not update toolBar
    }
}

void QG_CadToolBarMain::slotDrawMText()
{
    finishCurrentAction();
    actionHandler->slotDrawMText();
}

void QG_CadToolBarMain::slotDrawImage()
{
    finishCurrentAction();
    actionHandler->slotDrawImage();
}

//restore action from checked button
void QG_CadToolBarMain::restoreAction()
{
	if(!(actionHandler&&bMenuPoint)) return;
	if ( bMenuPoint ->isChecked() ) {
        actionHandler->slotDrawPoint();
        return;
    }
	m_pHidden->setChecked(true);
    finishCurrentAction();
}

void QG_CadToolBarMain::resetToolBar()
{
	killAllActions();
	m_pHidden->setChecked(true);
}

void QG_CadToolBarMain::mousePressEvent(QMouseEvent* e)
{
	if (e->button()==Qt::RightButton && cadToolBar) {
		resetToolBar();
	}
}


void QG_CadToolBarMain::showCadToolBar(RS2::ActionType actionType) {
	if(!bMenuImage) return;
    switch(actionType){
    case RS2::ActionDrawImage:
        bMenuImage->setChecked(true);
        break;
    case RS2::ActionDrawPoint:
        bMenuPoint->setChecked(true);
        break;
    case RS2::ActionDrawMText:
        bMenuText->setChecked(true);
        break;
    default:
		m_pHidden->setChecked(true);
        break;
    }
}
