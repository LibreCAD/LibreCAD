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
#ifndef QG_CADTOOLBARSNAP_H
#define QG_CADTOOLBARSNAP_H

class QG_CadToolBar;
class QG_ActionHandler;

#include "ui_qg_cadtoolbarsnap.h"

class QG_CadToolBarSnap : public QWidget, public Ui::QG_CadToolBarSnap
{
    Q_OBJECT

public:
    QG_CadToolBarSnap(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarSnap();

public slots:
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void snapFree();
    virtual void snapGrid();
    virtual void snapEndpoint();
    virtual void snapOnEntity();
    virtual void snapCenter();
    virtual void snapMiddle();
    virtual void snapDist();
    virtual void snapIntersection();
    virtual void snapIntersectionManual();
    virtual void restrictNothing();
    virtual void restrictOrthogonal();
    virtual void restrictHorizontal();
    virtual void restrictVertical();
    virtual void disableSnaps();
    virtual void disableRestrictions();
    virtual void setSnapMode( int sm );
    virtual void setSnapRestriction( int sr );
    virtual void setRelativeZero();
    virtual void lockRelativeZero( bool on );
    virtual void setLockRelativeZero( bool on );
    virtual void back();

protected:
    QG_CadToolBar* cadToolBar;
    QG_ActionHandler* actionHandler;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBARSNAP_H
