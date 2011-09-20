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
#include "qg_cadtoolbarmain.h"

#include "qg_cadtoolbar.h"

/*
 *  Constructs a QG_CadToolBarMain as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarMain::QG_CadToolBarMain(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CadToolBarMain::~QG_CadToolBarMain()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CadToolBarMain::languageChange()
{
    retranslateUi(this);
}

void QG_CadToolBarMain::init() {
        actionHandler= NULL;
}

void QG_CadToolBarMain::setCadToolBar(QG_CadToolBar* tb) {
    if (tb!=NULL) {
        actionHandler= tb->getActionHandler();
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_CadToolBarMain::setCadToolBar(): No valid toolbar set.");
    }
    if (actionHandler!=NULL) {
        connect(bMenuLine, SIGNAL(clicked()),
                tb, SLOT(showToolBarLines()));
        connect(bMenuArc, SIGNAL(clicked()),
                tb, SLOT(showToolBarArcs()));
        connect(bMenuCircle, SIGNAL(clicked()),
                tb, SLOT(showToolBarCircles()));
        connect(bMenuEllipse, SIGNAL(clicked()),
                tb, SLOT(showToolBarEllipses()));
        connect(bMenuSpline, SIGNAL(clicked()),
                actionHandler, SLOT(slotDrawSpline()));
        connect(bMenuPolyline, SIGNAL(clicked()),
                tb, SLOT(showToolBarPolylines()));
        connect(bMenuPoint, SIGNAL(clicked()),
                this, SLOT(slotDrawPoint()));

        connect(bMenuText, SIGNAL(clicked()),
                actionHandler, SLOT(slotDrawText()));
        connect(bMenuDim, SIGNAL(clicked()),
                tb, SLOT(showToolBarDim()));
        connect(bMenuHatch, SIGNAL(clicked()),
                actionHandler, SLOT(slotDrawHatch()));
        connect(bMenuImage, SIGNAL(clicked()),
                actionHandler, SLOT(slotDrawImage()));

        connect(bMenuModify, SIGNAL(clicked()),
                tb, SLOT(showToolBarModify()));
        connect(bMenuInfo, SIGNAL(clicked()),
                tb, SLOT(showToolBarInfo()));

        connect(bMenuBlock, SIGNAL(clicked()),
                actionHandler, SLOT(slotBlocksCreate()));
        connect(bMenuSelect, SIGNAL(clicked()),
                tb, SLOT(showToolBarSelect()));
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_CadToolBarMain::setCadToolBar(): No valid action handler set.");
    }
}

void QG_CadToolBarMain::slotDrawPoint() {
    if( bMenuPoint->isChecked()) {
        bMenuPoint->setChecked(false);
        actionHandler->getCurrentAction()->finish();
        return;
    }
    bMenuPoint->setChecked(true);
    actionHandler->slotDrawPoint();
}
