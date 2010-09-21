/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_CadToolBarMain::init() {
#ifndef RS_PROF
	bMenuPolyline->hide();
#endif
}

void QG_CadToolBarMain::setCadToolBar(QG_CadToolBar* tb) {
    QG_ActionHandler* ah = NULL;
    if (tb!=NULL) {
        ah = tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarMain::setCadToolBar(): No valid toolbar set.");
    }
    if (ah!=NULL) {
        connect(bMenuPoint, SIGNAL(clicked()),
                ah, SLOT(slotDrawPoint()));
        connect(bMenuLine, SIGNAL(clicked()),
                tb, SLOT(showToolBarLines()));
        connect(bMenuArc, SIGNAL(clicked()),
                tb, SLOT(showToolBarArcs()));
        connect(bMenuCircle, SIGNAL(clicked()),
                tb, SLOT(showToolBarCircles()));
        connect(bMenuEllipse, SIGNAL(clicked()),
                tb, SLOT(showToolBarEllipses()));
        connect(bMenuSpline, SIGNAL(clicked()),
                ah, SLOT(slotDrawSpline()));
        connect(bMenuPolyline, SIGNAL(clicked()),
                tb, SLOT(showToolBarPolylines()));
        
        connect(bMenuText, SIGNAL(clicked()),
                ah, SLOT(slotDrawText()));
        connect(bMenuDim, SIGNAL(clicked()),
                tb, SLOT(showToolBarDim()));
        connect(bMenuHatch, SIGNAL(clicked()),
                ah, SLOT(slotDrawHatch()));
        connect(bMenuImage, SIGNAL(clicked()),
                ah, SLOT(slotDrawImage()));
        
        connect(bMenuModify, SIGNAL(clicked()),
                tb, SLOT(showToolBarModify()));
        connect(bMenuInfo, SIGNAL(clicked()),
                tb, SLOT(showToolBarInfo()));

        connect(bMenuBlock, SIGNAL(clicked()),
                ah, SLOT(slotBlocksCreate()));
        connect(bMenuSelect, SIGNAL(clicked()),
                tb, SLOT(showToolBarSelect()));
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_CadToolBarMain::setCadToolBar(): No valid action handler set.");
    }
}
