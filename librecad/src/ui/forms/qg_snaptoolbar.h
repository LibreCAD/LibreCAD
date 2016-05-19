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
#ifndef QG_SNAPTOOLBAR_H
#define QG_SNAPTOOLBAR_H

class QG_CadToolBar;
class QG_ActionHandler;
class LC_ActionGroupManager;

#include <QToolBar>

#include "rs_snapper.h"

#include "ui_qg_snaptoolbar.h"

class QG_SnapToolBar : public QToolBar
{
    Q_OBJECT

public:
    QG_SnapToolBar(QWidget* parent
                 , QG_ActionHandler* ah
                 , LC_ActionGroupManager* agm);
	~QG_SnapToolBar() = default;

	RS_SnapMode getSnaps ( void ) const;
    void saveSnapMode(void );
    virtual void setActionHandler(QG_ActionHandler* ah);
	bool lockedRelativeZero() const;
    void setLockedRelativeZero(bool on);

public slots:
	void setSnaps(RS_SnapMode const & s);

private slots:
    void actionTriggered(void);
    void slotRestrictOrthogonal(bool checked);
    void slotRestrictNothing(bool checked);

private:

	QG_ActionHandler* actionHandler;

    QAction *snapFree;
    QAction *snapGrid;
    QAction *snapEnd;
    QAction *snapOnEntity;
    QAction *snapCenter;
    QAction *snapMiddle;
    QAction *snapDistance;
    QAction *snapIntersection;


    QAction *restrictHorizontal;
    QAction *restrictVertical;
    QAction *restrictOrthogonal;
    QAction *restrictNothing;
    QAction *bRelZero;
    QAction *bLockRelZero;
	RS_SnapMode snapMode;

};

#endif // QG_SNAPTOOLBAR_H
