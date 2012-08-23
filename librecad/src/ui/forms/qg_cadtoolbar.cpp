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
//#include <iostream>
#include "qg_cadtoolbar.h"
#include "rs_dialogfactory.h"

/*
 *  Constructs a QG_CadToolBar as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBar::QG_CadToolBar(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setObjectName(name);
    setupUi(this);
    toolbars.clear();
    toolbarIDs.clear();
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CadToolBar::~QG_CadToolBar()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CadToolBar::languageChange()
{
    retranslateUi(this);
}


void QG_CadToolBar::init() {
    setCursor(Qt::ArrowCursor);
    actionHandler = NULL;
    //    currentTb = NULL;
    //    previousID = RS2::ToolBarNone;
    //    savedID = RS2::ToolBarNone;

    tbMain = NULL;

    //    tbPoints = NULL;
    tbLines = NULL;
    tbArcs = NULL;
    tbCircles = NULL;
    tbEllipses = NULL;
    //    tbSplines = NULL;
    tbPolylines = NULL;

    tbDim = NULL;

    tbModify = NULL;
    tbInfo = NULL;
    tbSelect = NULL;
    //    tbSnap = NULL;
}

/**
 * @return Pointer to action handler or NULL.
 */
QG_ActionHandler* QG_CadToolBar::getActionHandler() {
    return actionHandler;
}

/**
 * Called from the sub toolbar
 */
void QG_CadToolBar::back() {
    showPreviousToolBar(true);
    //    emit(signalBack());
}

/**
 * Called from the application.
 */
void QG_CadToolBar::forceNext() {
    if(toolbars.size()==0) return;
    if (toolbars.last()!=NULL && toolbars.last()==tbSelect) {
        tbSelect->runNextAction();
    }
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
void QG_CadToolBar::createSubToolBars(QG_ActionHandler* ah) {
    actionHandler = ah;
    tbMain = new QG_CadToolBarMain(this);
    tbMain->setCadToolBar(this);

    //    tbPoints = new QG_CadToolBarPoints(this);
    //    tbPoints->setCadToolBar(this);
    //    tbPoints->hide();

    tbLines = new QG_CadToolBarLines(this);
    tbLines->setCadToolBar(this);
    tbLines->hide();

    tbArcs = new QG_CadToolBarArcs(this);
    tbArcs->setCadToolBar(this);
    tbArcs->hide();

    tbCircles = new QG_CadToolBarCircles(this);
    tbCircles->setCadToolBar(this);
    tbCircles->hide();

    tbEllipses = new QG_CadToolBarEllipses(this);
    tbEllipses->setCadToolBar(this);
    tbEllipses->hide();

    //    tbSplines = new QG_CadToolBarSplines(this);
    //    tbSplines->setCadToolBar(this);
    //    tbSplines->hide();

    tbPolylines = new QG_CadToolBarPolylines(this);
    tbPolylines->setCadToolBar(this);
    tbPolylines->hide();

    tbDim = new QG_CadToolBarDim(this);
    tbDim->setCadToolBar(this);
    tbDim->hide();

    tbInfo = new QG_CadToolBarInfo(this);
    tbInfo->setCadToolBar(this);
    tbInfo->hide();

    tbModify = new QG_CadToolBarModify(this);
    tbModify->setCadToolBar(this);
    tbModify->hide();

    //    tbSnap = NULL;
    //tbSnap = new QG_CadToolBarSnap(this);
    //tbSnap->setCadToolBar(this);
    //tbSnap->hide();

    tbSelect = new QG_CadToolBarSelect(this);
    tbSelect->setCadToolBar(this);
    tbSelect->hide();

    showToolBarMain();
}

void QG_CadToolBar::hideSubToolBars(){
    for(auto it=toolbars.begin();it != toolbars.end();it++){
        (*it)->hide();
    }
}

void QG_CadToolBar::showSubToolBar(){
    if (!toolbars.last()->isVisible()) { // On OSX, without this line LibreCAD wuld crash. Not sure if it's a Qt problem or 'somewhere' logic within LibreCAD
        //shift down to show the handle to move the toolbar
        //has to be 20, 10 is not enough
        toolbars.last()->move(0,20);
        toolbars.last()->show();
    }
}
void QG_CadToolBar::showPreviousToolBar(bool cleanup) {
    // cleanup mouse hint when showing previous tool bar, bug#3480121
    RS_DIALOGFACTORY->updateMouseWidget("","",false);
    if(cleanup){
        if(actionHandler != NULL) {
            RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
            if(currentAction != NULL) {
                currentAction->finish(false); //finish the action, but do not update toolBar
            }
        }
        if(toolbars.size()>1){
            if(toolbars.last() != NULL) toolbars.last() ->setVisible(false);
            toolbars.pop_back();
            toolbarIDs.pop_back();
        }
        //        std::cout<<"QG_CadToolBar::showPreviousToolBar(true): toolbars.size()="<<toolbars.size()<<std::endl;
        showToolBar(toolbarIDs.last());
    }else{
        hideSubToolBars();
        //        std::cout<<"QG_CadToolBar::showPreviousToolBar(false): toolbars.size()="<<toolbars.size()<<std::endl;
        if(toolbars.size()>1){
            //            std::cout<<"QG_CadToolBar::showPreviousToolBar(false): hide:"<<toolbarIDs[toolbars.size()-1]<<std::endl;
            if(toolbars.last()== NULL) toolbars.last()->setVisible(false);
            toolbars.pop_back();
            toolbarIDs.pop_back();

        }

        //        std::cout<<"QG_CadToolBar::showPreviousToolBar(false): toolbars.size()="<<toolbars.size()<<std::endl;
        showSubToolBar();
    }
}

void QG_CadToolBar::showToolBar(RS2::ToolBarId id, bool restoreAction ) {
    QWidget* newTb = NULL;
    switch (id) {
    default:
    case RS2::ToolBarMain:
        if(restoreAction) tbMain->restoreAction();
        newTb = tbMain;
        break;
    case RS2::ToolBarLines:
        if(restoreAction) tbLines->restoreAction();
        newTb = tbLines;
        break;
    case RS2::ToolBarArcs:
        if(restoreAction) tbArcs->restoreAction();
        newTb = tbArcs;
        break;
    case RS2::ToolBarEllipses:
        if(restoreAction) tbEllipses->restoreAction();
        newTb = tbEllipses;
        break;
    case RS2::ToolBarPolylines:
        if(restoreAction) tbPolylines->restoreAction();
        newTb = tbPolylines;
        break;
    case RS2::ToolBarCircles:
        if(restoreAction) tbCircles->restoreAction();
        newTb = tbCircles;
        break;
    case RS2::ToolBarInfo:
        if(restoreAction) tbInfo->restoreAction();
        newTb = tbInfo;
        break;
    case RS2::ToolBarModify:
        if(restoreAction) tbModify->restoreAction();
        newTb = tbModify;
        break;
    case RS2::ToolBarDim:
        if(restoreAction) tbDim->restoreAction();
        newTb = tbDim;
        break;
    case RS2::ToolBarSelect:
        newTb = tbSelect;
        break;
    }
    hideSubToolBars();
    int i0=toolbarIDs.indexOf(id)+1;
    if(i0>0 && i0<toolbarIDs.size()){
        toolbars.erase(toolbars.begin()+i0,toolbars.end());
        toolbarIDs.erase(toolbarIDs.begin()+i0,toolbarIDs.end());
    }
    if (newTb!=NULL) {
        if(!( toolbarIDs.size()>0 && id == toolbarIDs.last())) {
            toolbarIDs.push_back(id);
            toolbars.push_back(newTb);
        }
    }
    showSubToolBar();
}

void QG_CadToolBar::resetToolBar() {
    QWidget* currentTb=toolbars.last();
    if(currentTb == tbMain) {
        tbMain->resetToolBar();
        return;
    }
    if(currentTb == tbLines) {
        tbLines->resetToolBar();
        return;
    }
    if(currentTb == tbArcs) {
        tbArcs->resetToolBar();
        return;
    }
    if(currentTb == tbCircles) {
        tbCircles->resetToolBar();
        return;
    }
    if(currentTb == tbEllipses) {
        tbEllipses->resetToolBar();
        return;
    }
    if(currentTb == tbPolylines) {
        tbPolylines->resetToolBar();
        return;
    }
    if(currentTb == tbDim) {
        tbDim->resetToolBar();
        return;
    }
    if(currentTb == tbInfo) {
        tbInfo->resetToolBar();
        return;
    }
    if(currentTb == tbModify) {
        tbModify->resetToolBar();
        return;
    }
}

void QG_CadToolBar::showToolBarMain() {
    showToolBar(RS2::ToolBarMain);
}

//void QG_CadToolBar::showToolBarPoints() {
//    //not needed
//    //showToolBar(RS2::ToolBarPoints);
//}

void QG_CadToolBar::showToolBarLines() {
    showToolBar(RS2::ToolBarLines);
}

void QG_CadToolBar::showToolBarArcs() {
    showToolBar(RS2::ToolBarArcs);
}

void QG_CadToolBar::showToolBarEllipses() {
    showToolBar(RS2::ToolBarEllipses);
}

//void QG_CadToolBar::showToolBarSplines() {
//    showToolBar(RS2::ToolBarSplines);
//}

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

//void QG_CadToolBar::showToolBarSnap() {
//    showToolBar(RS2::ToolBarSnap);
//}

void QG_CadToolBar::showToolBarDim() {
    showToolBar(RS2::ToolBarDim);
}

void QG_CadToolBar::showToolBarSelect() {
    showToolBarSelect(NULL, -1);
}

void QG_CadToolBar::showToolBarSelect(RS_ActionInterface* selectAction,
                                      int nextAction) {

    tbSelect->setNextAction(nextAction);
    tbSelect->setSelectAction(selectAction);
    showToolBar(RS2::ToolBarSelect);
}

void QG_CadToolBar::showCadToolBar(RS2::ActionType actionType, bool cleanup){
    switch(actionType){
    //no op
    case RS2::ActionFileNew:
    case RS2::ActionFileOpen:
    case RS2::ActionFileSave:
    case RS2::ActionFileSaveAs:
    case RS2::ActionFileExport:
    case RS2::ActionFileClose:
    case RS2::ActionFilePrint:
    case RS2::ActionFilePrintPreview:
    case RS2::ActionFileQuit:
    case RS2::ActionPrintPreview:
    case RS2::ActionEditUndo:
    case RS2::ActionEditRedo:
    case RS2::ActionEditCut:
    case RS2::ActionEditCutNoSelect:
    case RS2::ActionEditCopy:
    case RS2::ActionEditCopyNoSelect:
    case RS2::ActionEditPaste:
    case RS2::ActionViewStatusBar:
    case RS2::ActionViewLayerList:
    case RS2::ActionViewBlockList:
    case RS2::ActionViewCommandLine:
    case RS2::ActionViewLibrary:
    case RS2::ActionViewPenToolbar:
    case RS2::ActionViewOptionToolbar:
    case RS2::ActionViewCadToolbar:
    case RS2::ActionViewFileToolbar:
    case RS2::ActionViewEditToolbar:
    case RS2::ActionViewSnapToolbar:
    case RS2::ActionViewGrid:
    case RS2::ActionViewDraft:
    case RS2::ActionZoomIn:
    case RS2::ActionZoomOut:
    case RS2::ActionZoomAuto:
    case RS2::ActionZoomWindow:
    case RS2::ActionZoomPan:
    case RS2::ActionZoomRedraw:
    case RS2::ActionZoomPrevious:
    case RS2::ActionSelect:
    case RS2::ActionSelectSingle:
    case RS2::ActionSelectContour:
    case RS2::ActionSelectWindow:
    case RS2::ActionDeselectWindow:
    case RS2::ActionSelectAll:
    case RS2::ActionDeselectAll:
    case RS2::ActionSelectIntersected:
    case RS2::ActionDeselectIntersected:
    case RS2::ActionSelectInvert:
    case RS2::ActionSelectLayer:
    case RS2::ActionSelectDouble:
    case RS2::ActionDrawHatch:
    case RS2::ActionDrawHatchNoSelect:
    case RS2::ActionEditKillAllActions:
    case RS2::ActionSnapFree:
    case RS2::ActionSnapGrid:
    case RS2::ActionSnapEndpoint:
    case RS2::ActionSnapOnEntity:
    case RS2::ActionSnapCenter:
    case RS2::ActionSnapMiddle:
    case RS2::ActionSnapDist:
    case RS2::ActionSnapIntersection:
    case RS2::ActionSnapIntersectionManual:
    case RS2::ActionRestrictNothing:
    case RS2::ActionRestrictOrthogonal:
    case RS2::ActionRestrictHorizontal:
    case RS2::ActionRestrictVertical:
    case RS2::ActionSetRelativeZero:
    case RS2::ActionLockRelativeZero:
    case RS2::ActionUnlockRelativeZero:
    case RS2::ActionLayersDefreezeAll:
    case RS2::ActionLayersFreezeAll:
    case RS2::ActionLayersAdd:
    case RS2::ActionLayersRemove:
    case RS2::ActionLayersEdit:
    case RS2::ActionLayersToggleView:
    case RS2::ActionLayersToggleLock:
    case RS2::ActionLayersTogglePrint:
    case RS2::ActionBlocksDefreezeAll:
    case RS2::ActionBlocksFreezeAll:
    case RS2::ActionBlocksAdd:
    case RS2::ActionBlocksRemove:
    case RS2::ActionBlocksAttributes:
    case RS2::ActionBlocksEdit:
    case RS2::ActionBlocksInsert:
    case RS2::ActionBlocksToggleView:
    case RS2::ActionBlocksCreate:
    case RS2::ActionBlocksCreateNoSelect:
    case RS2::ActionBlocksExplode:
    case RS2::ActionBlocksExplodeNoSelect:
    case RS2::ActionModifyExplodeText:
    default:
        return;
        //default action resets toolbar, issue#295
    case RS2::ActionDefault:
        resetToolBar();
        break;
    case RS2::ActionDrawImage:
    case RS2::ActionDrawPoint:
    case RS2::ActionDrawSpline:
    case RS2::ActionDrawMText:
        showToolBar(RS2::ToolBarMain, false);
        if(tbMain != NULL){
            tbMain->showCadToolBar(actionType);
        }
        break;
    case RS2::ActionDrawArc:
    case RS2::ActionDrawArc3P:
    case RS2::ActionDrawArcParallel:
    case RS2::ActionDrawArcTangential:
        showToolBar(RS2::ToolBarArcs, false);
       if(tbArcs != NULL){
           tbArcs->showCadToolBar(actionType);
       }
        break;
    case RS2::ActionDrawCircle:
    case RS2::ActionDrawCircle2P:
    case RS2::ActionDrawCircle3P:
    case RS2::ActionDrawCircleCR:
    case RS2::ActionDrawCircleParallel:
    case RS2::ActionDrawCircleInscribe:
    case RS2::ActionDrawCircleTan2:
        showToolBar(RS2::ToolBarCircles, false);
       if(tbCircles != NULL){
           tbCircles->showCadToolBar(actionType);
       }
        break;
    case RS2::ActionDrawEllipseArcAxis:
    case RS2::ActionDrawEllipseAxis:
    case RS2::ActionDrawEllipseFociPoint:
    case RS2::ActionDrawEllipse4Points:
    case RS2::ActionDrawEllipseCenter3Points:
    case RS2::ActionDrawEllipseInscribe:
        showToolBar(RS2::ToolBarEllipses, false);
        if(tbEllipses != NULL){
            tbEllipses->showCadToolBar(actionType);
        }
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
        showToolBar(RS2::ToolBarLines, false);
        if(tbLines != NULL){
            tbLines->showCadToolBar(actionType);
        }
        break;
    case RS2::ActionDrawPolyline:
    case RS2::ActionPolylineAdd:
    case RS2::ActionPolylineAppend:
    case RS2::ActionPolylineDel:
    case RS2::ActionPolylineDelBetween:
    case RS2::ActionPolylineTrim:
    case RS2::ActionPolylineEquidistant:
    case RS2::ActionPolylineSegment:
        showToolBar(RS2::ToolBarPolylines, false);
        if(tbPolylines != NULL){
            tbPolylines->showCadToolBar(actionType);
        }
        break;
    case RS2::ActionDimAligned:
    case RS2::ActionDimLinear:
    case RS2::ActionDimLinearVer:
    case RS2::ActionDimLinearHor:
    case RS2::ActionDimRadial:
    case RS2::ActionDimDiametric:
    case RS2::ActionDimAngular:
    case RS2::ActionDimLeader:
        showToolBar(RS2::ToolBarDim, false);
        if(tbDim != NULL){
            tbDim->showCadToolBar(actionType);
        }
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
        showToolBar(RS2::ToolBarModify, false);
        if(tbModify != NULL){
            tbModify->showCadToolBar(actionType);
        }
        break;
    case RS2::ActionInfoInside:
    case RS2::ActionInfoDist:
    case RS2::ActionInfoDist2:
    case RS2::ActionInfoAngle:
    case RS2::ActionInfoTotalLength:
    case RS2::ActionInfoTotalLengthNoSelect:
    case RS2::ActionInfoArea:
        showToolBar(RS2::ToolBarInfo, false);
        if(tbInfo != NULL){
            tbInfo->showCadToolBar(actionType);
        }
        break;



    }
    if(cleanup){
        if(actionHandler != NULL) {
            RS_ActionInterface* currentAction =actionHandler->getCurrentAction();
            if(currentAction != NULL) {
                currentAction->finish(false); //finish the action, but do not update toolBar
            }
        }
    }
}

