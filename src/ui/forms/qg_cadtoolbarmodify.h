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
#ifndef QG_CADTOOLBARMODIFY_H
#define QG_CADTOOLBARMODIFY_H

class QG_CadToolBar;

#include "intermediate/ui/ui_qg_cadtoolbarmodify.h"

class QG_CadToolBarModify : public QWidget, public Ui::QG_CadToolBarModify
{
    Q_OBJECT

public:
    QG_CadToolBarModify(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CadToolBarModify();

public slots:
    virtual void mousePressEvent( QMouseEvent * e );
    virtual void contextMenuEvent( QContextMenuEvent * e );
    virtual void setCadToolBar( QG_CadToolBar * tb );
    virtual void modifyMove();
    virtual void modifyRotate();
    virtual void modifyScale();
    virtual void modifyMirror();
    virtual void modifyMoveRotate();
    virtual void modifyRotate2();
    virtual void modifyTrim();
    virtual void modifyTrim2();
    virtual void modifyTrimAmount();
    virtual void modifyCut();
    virtual void modifyBevel();
    virtual void modifyRound();
    virtual void modifyEntity();
    virtual void modifyDelete();
    virtual void modifyAttributes();
    virtual void modifyStretch();
    virtual void modifyExplode();
    virtual void modifyExplodeText();
    virtual void back();

protected:
    QG_ActionHandler* actionHandler;
    QG_CadToolBar* cadToolBar;

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_CADTOOLBARMODIFY_H
