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
#ifndef QG_CADTOOLBAR_H
#define QG_CADTOOLBAR_H

class QG_ActionHandler;

#include "rs_actioninterface.h"
#include "qg_cadtoolbararcs.h"
#include "qg_cadtoolbarcircles.h"
#include "qg_cadtoolbardim.h"
#include "qg_cadtoolbarellipses.h"
#include "qg_cadtoolbarinfo.h"
#include "qg_cadtoolbarlines.h"
#include "qg_cadtoolbarmain.h"
#include "qg_cadtoolbarmodify.h"
#include "qg_cadtoolbarpoints.h"
#include "qg_cadtoolbarpolylines.h"
#include "qg_cadtoolbarselect.h"
#include "qg_cadtoolbarsnap.h"
#include "qg_cadtoolbarsplines.h"
#include "ui_qg_cadtoolbar.h"

class QG_CadToolBar : public QWidget, public Ui::QG_CadToolBar
{
    Q_OBJECT

public:
    QG_CadToolBar(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBar();

    virtual QG_ActionHandler * getActionHandler();

public slots:
    virtual void back();
    virtual void forceNext();
    virtual void mouseReleaseEvent( QMouseEvent * e );
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void createSubToolBars( QG_ActionHandler * ah );
    virtual void showToolBar( int id );
    virtual void showToolBarMain();
    virtual void showToolBarPoints();
    virtual void showToolBarLines();
    virtual void showToolBarArcs();
    virtual void showToolBarEllipses();
    virtual void showToolBarSplines();
    virtual void showToolBarPolylines();
    virtual void showToolBarCircles();
    virtual void showToolBarInfo();
    virtual void showToolBarModify();
    virtual void showToolBarSnap();
    virtual void showToolBarDim();
    virtual void showToolBarSelect();
    virtual void showToolBarSelect( RS_ActionInterface * selectAction, int nextAction );

signals:
    void signalBack();
    void signalNext();

protected:
    QG_CadToolBarSplines* tbSplines;
    QG_CadToolBarInfo* tbInfo;
    QG_ActionHandler* actionHandler;
    QWidget* currentTb;
    QG_CadToolBarMain* tbMain;
    QG_CadToolBarDim* tbDim;
    QG_CadToolBarLines* tbLines;
    QG_CadToolBarPoints* tbPoints;
    QG_CadToolBarEllipses* tbEllipses;
    QG_CadToolBarArcs* tbArcs;
    QG_CadToolBarModify* tbModify;
    QG_CadToolBarCircles* tbCircles;
    QG_CadToolBarSnap* tbSnap;
    QG_CadToolBarSelect* tbSelect;
    QG_CadToolBarPolylines* tbPolylines;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBAR_H
