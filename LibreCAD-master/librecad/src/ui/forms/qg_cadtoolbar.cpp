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
#include <QMouseEvent>
#include "qg_cadtoolbar.h"
#include "rs_dialogfactory.h"
#include "rs_actioninterface.h"
#include "qg_cadtoolbararcs.h"
#include "qg_cadtoolbarcircles.h"
#include "qg_cadtoolbardim.h"
#include "qg_cadtoolbarellipses.h"
#include "qg_cadtoolbarinfo.h"
#include "qg_cadtoolbarlines.h"
#include "qg_cadtoolbarmain.h"
#include "qg_cadtoolbarmodify.h"
#include "qg_cadtoolbarpolylines.h"
#include "qg_cadtoolbarselect.h"
#include "qg_cadtoolbarsplines.h"
#include "rs_debug.h"

#include "qc_applicationwindow.h"

/*
 *  Constructs a QG_CadToolBar as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBar::QG_CadToolBar(QWidget* parent, const char* name)
    : QToolBar(parent)
	,actionHandler(nullptr)
{
    setObjectName(name);
	setCursor(Qt::ArrowCursor);
#if QT_VERSION >= 0x050500
    auto const dPxlRatio=QC_ApplicationWindow::getAppWindow()->devicePixelRatio();
    setMinimumSize(73*dPxlRatio,400*dPxlRatio);
#else
    setMinimumSize(73,400);
#endif
    setAllowedAreas(Qt::LeftToolBarArea | Qt::RightToolBarArea);
    setFloatable(false);
	init();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
//void QG_CadToolBar::languageChange()
//{
//    retranslateUi(this);
//}

void QG_CadToolBar::init() {
	//create sub cad toolbars
	const std::initializer_list<LC_CadToolBarInterface*> tbs={
		new QG_CadToolBarMain(this)
		,new QG_CadToolBarLines(this)
		,new QG_CadToolBarArcs(this)
		,new QG_CadToolBarCircles(this)
		,new QG_CadToolBarEllipses(this)
		,new QG_CadToolBarSplines(this)
		,new QG_CadToolBarPolylines(this)
		,new QG_CadToolBarDim(this)
		,new QG_CadToolBarInfo(this)
		,new QG_CadToolBarModify(this)
		,new QG_CadToolBarSelect(this)
	};

	for(auto p: tbs){
		p->hide();
		m_toolbars[p->rtti()]= p;
	}
}

QSize 	QG_CadToolBar::sizeHint() const
{
    return QSize(-1, -1);
}

void QG_CadToolBar::populateSubToolBar(const std::vector<QAction*>& actions, RS2::ToolBarId toolbarID)
{
	RS_DEBUG->print("QG_CadToolBar::populateSubToolBar(): begin\n");

	if(!m_toolbars.count(toolbarID)) return;
	LC_CadToolBarInterface*const p = m_toolbars[toolbarID];
	p->addSubActions(actions, true);

	RS_DEBUG->print("QG_CadToolBar::populateSubToolBar(): end\n");
}

/**
 * Called from the sub toolbar
 */
void QG_CadToolBar::back() {
	finishCurrentAction(false);
    showPreviousToolBar(true);
    //    emit(signalBack());
}

void QG_CadToolBar::finishCurrentAction(bool resetToolBar)
{
	if(!actionHandler) return;
	RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
	if(currentAction) {
		currentAction->finish(resetToolBar); //finish the action, but do not update toolBar
	}
}
/**
 * Called from the application.
 */
void QG_CadToolBar::forceNext() {
    if(activeToolbars.size()==0) return;
	auto p=activeToolbars.back();
	if (p && p->rtti() == RS2::ToolBarSelect)
		p->runNextAction();
}

void QG_CadToolBar::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        back();
        e->accept();
    }
}

void QG_CadToolBar::contextMenuEvent(QContextMenuEvent *e) {
    e->accept();
}

/**
 * Creates all tool bars and shows the main toolbar.
 *
 * @param ah Pointer to action handler which will deal with the actions in this tool bar.
 */
void QG_CadToolBar::setActionHandler(QG_ActionHandler* ah) {
	actionHandler = ah;
	for(const auto& p: m_toolbars){
		p.second->setActionHandler(ah);
	}
}

void QG_CadToolBar::hideSubToolBars(){
	for(auto p: activeToolbars){
		p->hide();
	}
}

void QG_CadToolBar::showSubToolBar(){
	LC_CadToolBarInterface* const p = activeToolbars.back();
	if (!p->isVisible()) { // On OSX, without this line LibreCAD wuld crash. Not sure if it's a Qt problem or 'somewhere' logic within LibreCAD
        //shift down to show the handle to move the toolbar
        //has to be 20, 10 is not enough
		p->move(0,20);
		p->show();
    }
    p->resize(size());
    adjustSize();
}

void QG_CadToolBar::showPreviousToolBar(bool cleanup) {
	// cleanup mouse hint when showing previous tool bar, bug#3480121
	RS_DIALOGFACTORY->updateMouseWidget();
//	for(auto p: activeToolbars){
//		qDebug()<<"QG_CadToolBar::showPreviousToolBar():begin "<<p->rtti();
//	}
	if(cleanup){
		if(actionHandler) {
			RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
			if(currentAction && currentAction->rtti() != RS2::ActionDefault) {
				currentAction->finish(false); //finish the action, but do not update toolBar
			}
		}
		if(activeToolbars.size()>1){
			if(activeToolbars.back()) activeToolbars.back() ->setVisible(false);
			activeToolbars.pop_back();
		}
		//        std::cout<<"QG_CadToolBar::showPreviousToolBar(true): toolbars.size()="<<toolbars.size()<<std::endl;
		showToolBar(activeToolbars.back()->rtti());
	}else{
		hideSubToolBars();
		//        std::cout<<"QG_CadToolBar::showPreviousToolBar(false): toolbars.size()="<<toolbars.size()<<std::endl;
		if(activeToolbars.size()>1){
			//            std::cout<<"QG_CadToolBar::showPreviousToolBar(false): hide:"<<toolbarIDs[toolbars.size()-1]<<std::endl;
			if (!activeToolbars.back()) activeToolbars.back()->setVisible(false);
			activeToolbars.pop_back();
		}

		//        std::cout<<"QG_CadToolBar::showPreviousToolBar(false): toolbars.size()="<<toolbars.size()<<std::endl;
		showSubToolBar();
	}
//	for(auto p: activeToolbars){
//		qDebug()<<"QG_CadToolBar::showPreviousToolBar():end "<<p->rtti();
//	}
}

void QG_CadToolBar::showToolBar(RS2::ToolBarId id, bool restoreAction ) {

	LC_CadToolBarInterface* newTb;
	if(m_toolbars.count(id)){
		newTb=m_toolbars[id];
	}else{
		newTb=m_toolbars[RS2::ToolBarMain];
	}
	if(restoreAction) newTb->restoreAction();
	hideSubToolBars();
	auto it=std::find(activeToolbars.begin(), activeToolbars.end(), newTb);
	if(it != activeToolbars.end()){
		activeToolbars.erase(it+1,activeToolbars.end());
	}
	if(!( activeToolbars.size()>0 && newTb == activeToolbars.back())) {
		activeToolbars.push_back(newTb);
	}
	showSubToolBar();
	adjustSize();
}

void QG_CadToolBar::resetToolBar() {
	LC_CadToolBarInterface* currentTb=activeToolbars.back();
	currentTb->resetToolBar();
}

void QG_CadToolBar::showToolBarMain() {
    showToolBar(RS2::ToolBarMain);
}

void QG_CadToolBar::showToolBarLines() {
    showToolBar(RS2::ToolBarLines);
}

void QG_CadToolBar::showToolBarArcs() {
    showToolBar(RS2::ToolBarArcs);
}

void QG_CadToolBar::showToolBarEllipses() {
    showToolBar(RS2::ToolBarEllipses);
}

void QG_CadToolBar::showToolBarSplines() {
    showToolBar(RS2::ToolBarSplines);
}

void QG_CadToolBar::showToolBarPolylines() {
    showToolBar(RS2::ToolBarPolylines);
}

void QG_CadToolBar::showToolBarCircles() {
    showToolBar(RS2::ToolBarCircles);
}

void QG_CadToolBar::showToolBarInfo() {
    showToolBar(RS2::ToolBarInfo);
}

void QG_CadToolBar::showToolBarModify() {
    showToolBar(RS2::ToolBarModify);
}

void QG_CadToolBar::showToolBarDim() {
    showToolBar(RS2::ToolBarDim);
}

void QG_CadToolBar::showToolBarSelect() {
	showToolBarSelect(nullptr, -1);
}

void QG_CadToolBar::showToolBarSelect(RS_ActionInterface* selectAction,
                                      int nextAction) {
	auto p=m_toolbars[RS2::ToolBarSelect];

	p->setNextAction(nextAction);
	p->setSelectAction(selectAction);
    showToolBar(RS2::ToolBarSelect);
	showSubToolBar();
}

void QG_CadToolBar::showCadToolBar(RS2::ActionType actionType, bool cleanup){
	RS2::ToolBarId id=RS2::ToolBarNone;
    switch(actionType){
    //no op
    default:
        return;
        //default action resets toolbar, issue#295
	case RS2::ActionDefault:
		break;
	case RS2::ActionDrawImage:
    case RS2::ActionDrawPoint:
    case RS2::ActionDrawMText:
		id=RS2::ToolBarMain;
		break;
    case RS2::ActionDrawArc:
    case RS2::ActionDrawArc3P:
    case RS2::ActionDrawArcParallel:
    case RS2::ActionDrawArcTangential:
		id=RS2::ToolBarArcs;
        break;
    case RS2::ActionDrawCircle:
    case RS2::ActionDrawCircle2P:
    case RS2::ActionDrawCircle3P:
    case RS2::ActionDrawCircleCR:
    case RS2::ActionDrawCircleParallel:
    case RS2::ActionDrawCircleInscribe:
    case RS2::ActionDrawCircleTan2:
    case RS2::ActionDrawCircleTan2_1P:
    case RS2::ActionDrawCircleTan1_2P:
		id=RS2::ToolBarCircles;
        break;
    case RS2::ActionDrawEllipseArcAxis:
    case RS2::ActionDrawEllipseAxis:
    case RS2::ActionDrawEllipseFociPoint:
    case RS2::ActionDrawEllipse4Points:
    case RS2::ActionDrawEllipseCenter3Points:
    case RS2::ActionDrawEllipseInscribe:
		id=RS2::ToolBarEllipses;
        break;
    case RS2::ActionDrawSpline:
    case RS2::ActionDrawSplinePoints:
		id=RS2::ToolBarSplines;
        break;
    case RS2::ActionDrawLine:
    case RS2::ActionDrawLineAngle:
    case RS2::ActionDrawLineBisector:
    case RS2::ActionDrawLineFree:
    case RS2::ActionDrawLineHorVert:
    case RS2::ActionDrawLineHorizontal:
    case RS2::ActionDrawLineOrthogonal:
    case RS2::ActionDrawLineOrthTan:
    case RS2::ActionDrawLineParallel:
    case RS2::ActionDrawLineParallelThrough:
    case RS2::ActionDrawLinePolygonCenCor:
    case RS2::ActionDrawLinePolygonCorCor:
    case RS2::ActionDrawLineRectangle:
    case RS2::ActionDrawLineRelAngle:
    case RS2::ActionDrawLineTangent1:
    case RS2::ActionDrawLineTangent2:
    case RS2::ActionDrawLineVertical:
		id=RS2::ToolBarLines;
        break;
    case RS2::ActionDrawPolyline:
    case RS2::ActionPolylineAdd:
    case RS2::ActionPolylineAppend:
    case RS2::ActionPolylineDel:
    case RS2::ActionPolylineDelBetween:
    case RS2::ActionPolylineTrim:
    case RS2::ActionPolylineEquidistant:
    case RS2::ActionPolylineSegment:
		id=RS2::ToolBarPolylines;
        break;
    case RS2::ActionDimAligned:
    case RS2::ActionDimLinear:
    case RS2::ActionDimLinearVer:
    case RS2::ActionDimLinearHor:
    case RS2::ActionDimRadial:
    case RS2::ActionDimDiametric:
    case RS2::ActionDimAngular:
    case RS2::ActionDimLeader:
		id=RS2::ToolBarDim;
        break;
    case RS2::ActionModifyAttributes:
    case RS2::ActionModifyAttributesNoSelect:
    case RS2::ActionModifyDelete:
    case RS2::ActionModifyDeleteNoSelect:
    case RS2::ActionModifyDeleteQuick:
    case RS2::ActionModifyDeleteFree:
    case RS2::ActionModifyMove:
    case RS2::ActionModifyMoveNoSelect:
    case RS2::ActionModifyRotate:
    case RS2::ActionModifyRotateNoSelect:
    case RS2::ActionModifyScale:
    case RS2::ActionModifyScaleNoSelect:
    case RS2::ActionModifyMirror:
    case RS2::ActionModifyMirrorNoSelect:
    case RS2::ActionModifyMoveRotate:
    case RS2::ActionModifyMoveRotateNoSelect:
    case RS2::ActionModifyRotate2:
    case RS2::ActionModifyRotate2NoSelect:
    case RS2::ActionModifyEntity:
    case RS2::ActionModifyTrim:
    case RS2::ActionModifyTrim2:
    case RS2::ActionModifyTrimAmount:
    case RS2::ActionModifyCut:
    case RS2::ActionModifyStretch:
    case RS2::ActionModifyBevel:
    case RS2::ActionModifyRound:
    case RS2::ActionModifyOffset:
    case RS2::ActionModifyOffsetNoSelect:
    case RS2::ActionModifyRevertDirection:
    case RS2::ActionModifyRevertDirectionNoSelect:
		id=RS2::ToolBarModify;
        break;
    case RS2::ActionInfoInside:
    case RS2::ActionInfoDist:
    case RS2::ActionInfoDist2:
    case RS2::ActionInfoAngle:
    case RS2::ActionInfoTotalLength:
    case RS2::ActionInfoTotalLengthNoSelect:
    case RS2::ActionInfoArea:
		id=RS2::ToolBarInfo;
        break;
    }
	if(id != RS2::ToolBarNone){
		m_toolbars[id]->showCadToolBar(actionType);
		showToolBar(id, false);
	}
    if(cleanup){
		if(actionHandler ) {
            RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
			if(currentAction ) {
                currentAction->finish(false); //finish the action, but do not update toolBar
            }
        }
    }
}

