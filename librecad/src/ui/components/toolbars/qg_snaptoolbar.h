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

#include <QToolBar>

#include "rs_snapper.h"

class LC_SnapOptionsWidgetsHolder;
class QG_ActionHandler;

class LC_ActionGroupManager;

class QG_SnapToolBar : public QToolBar {
    Q_OBJECT
public:
    QG_SnapToolBar(QWidget* parent, QG_ActionHandler* ah, const LC_ActionGroupManager* agm, const QMap<QString, QAction*>& actionsMap);
    ~QG_SnapToolBar() override = default;

    RS_SnapMode getSnaps() const;
    void saveSnapMode() const;
    bool lockedRelativeZero() const;
    void setLockedRelativeZero(bool on) const;
    void setUCSActive(bool on) const;
    LC_SnapOptionsWidgetsHolder* getSnapOptionsHolder();

public
    slots :
    void setSnaps(const RS_SnapMode& s) const;
    void slotEnableRelativeZeroSnaps(bool) const;
    void slotUnsetSnapMiddleManual() const;

private
    slots :
    void actionTriggered() const;
    void slotRestrictOrthogonal(bool checked) const;
    void slotRestrictNothing(bool checked) const;

private:
    QAction* addOwnAction(const QString& name, const QMap<QString, QAction*>& actionsMap);
    QAction* justAddAction(const QString& name, const QMap<QString, QAction*>& actionsMap);

    QG_ActionHandler* m_actionHandler;

    QAction* m_actionSnapVisual;
    QAction* m_actionSnapFree;
    QAction* m_actionSnapGrid;
    QAction* m_actionSnapEnd;
    QAction* m_actionSnapOnEntity;
    QAction* m_actionSnapCenter;
    QAction* m_actionSnapMiddle;
    QAction* m_actionSnapDistance;
    QAction* m_actionSnapIntersection;
    QAction* m_actionSnapMiddleManual;

    QAction* m_actionRestrictHorizontal;
    QAction* m_actionRestrictVertical;
    QAction* m_actionRestrictOrthogonal;
    QAction* m_actionRestrictNothing;
    QAction* m_actionRelZero;
    QAction* m_actionLockRelZero;

    QAction* m_ucsMode{nullptr};
    RS_SnapMode m_snapMode;
};

#endif
