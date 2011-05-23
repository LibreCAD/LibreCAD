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
#ifndef QG_CADTOOLBARSELECT_H
#define QG_CADTOOLBARSELECT_H

class QG_CadToolBar;

#include "ui_qg_cadtoolbarselect.h"

class QG_CadToolBarSelect : public QWidget, public Ui::QG_CadToolBarSelect
{
    Q_OBJECT

public:
    QG_CadToolBarSelect(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarSelect();

public slots:
    virtual void mousePressEvent( QMouseEvent * e );
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void selectSingle();
    virtual void selectContour();
    virtual void deselectAll();
    virtual void selectAll();
    virtual void selectWindow();
    virtual void deselectWindow();
    virtual void selectIntersected();
    virtual void deselectIntersected();
    virtual void selectInvert();
    virtual void selectLayer();
    virtual void setSelectAction( RS_ActionInterface * selectAction );
    virtual void setNextAction( int nextAction );
    virtual void runNextAction();
    virtual void back();

protected:
    QG_CadToolBar* cadToolBar;
    QG_ActionHandler* actionHandler;

protected slots:
    virtual void languageChange();

private:
    int nextAction;
    RS_ActionInterface* selectAction;

    void init();

};

#endif // QG_CADTOOLBARSELECT_H
