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
#include <QContextMenuEvent>
#include <QToolBar>

#include "qg_snaptoolbar.h"
#include "rs_settings.h"
#include "qg_actionhandler.h"
#include "lc_actiongroupmanager.h"
#include "lc_snapoptionswidgetsholder.h"

QAction* QG_SnapToolBar::justAddAction(QString name, const QMap<QString, QAction*> &actionsMap){
    auto* action = actionsMap[name];
    addAction(action);
    return action;
}

QAction* QG_SnapToolBar::addOwnAction(QString name, const QMap<QString, QAction*> &actionsMap){
    auto* action = actionsMap[name];
    connect(action, &QAction::triggered,  this, &QG_SnapToolBar::actionTriggered);
    addAction(action);
    return action;
}

/*
 *  Constructs a QG_Cadhttps://apps.e-signlive.ca/error?error=eyJjb2RlIjo0MDEsIm1lc3NhZ2VLZXkiOiJlcnJvci51bmF1dGhvcmlzZWQuc2Vzc2lvbkV4cGlyZWQiLCJ0ZWNobmljYWwiOiJTZXNzaW9uIGlzIGV4cGlyZWQifQ%3D%3DToolBarSnap as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */

QG_SnapToolBar::QG_SnapToolBar(QWidget* parent, QG_ActionHandler* ah, LC_ActionGroupManager* agm, const QMap<QString, QAction*> &actionsMap)
	: QToolBar(parent), actionHandler(ah){

    auto action = justAddAction("ExclusiveSnapMode", actionsMap);
    connect(action, &QAction::triggered, agm, &LC_ActionGroupManager::toggleExclusiveSnapMode);

    snapMiddleManual = justAddAction("SnapMiddleManual", actionsMap);
    connect(snapMiddleManual, &QAction::triggered, actionHandler, &QG_ActionHandler::slotSnapMiddleManual);

    snapFree = addOwnAction("SnapFree", actionsMap);
    snapGrid = addOwnAction("SnapGrid", actionsMap);
    snapEnd = addOwnAction("SnapEnd", actionsMap);
    snapOnEntity = addOwnAction("SnapEntity", actionsMap);
    snapCenter = addOwnAction("SnapCenter", actionsMap);
    snapMiddle = addOwnAction("SnapMiddle", actionsMap);
    snapDistance =  addOwnAction("SnapDistance", actionsMap);
    snapIntersection = addOwnAction("SnapIntersection", actionsMap);

    addSeparator();

    restrictHorizontal = addOwnAction("RestrictHorizontal", actionsMap);
    restrictVertical = addOwnAction("RestrictVertical", actionsMap);

    restrictOrthogonal = justAddAction("RestrictOrthogonal", actionsMap);
    connect(restrictOrthogonal, &QAction::triggered, this, &QG_SnapToolBar::slotRestrictOrthogonal);

    restrictNothing = justAddAction("RestrictNothing", actionsMap);
    connect(restrictNothing, &QAction::triggered, this, &QG_SnapToolBar::slotRestrictNothing);
    // todo - in general, restrict nothing has no practical sense at all - as buttons are toggled, the amount of clicks is the same
    // todo - so probably, it's better to keep it hidden for now.
    // todo - However, it may be practical, if "restrict to angle" will be implemented
    restrictNothing->setVisible(false);

    addSeparator();

    bRelZero = justAddAction("SetRelativeZero", actionsMap);
    connect(bRelZero, &QAction::triggered, actionHandler, &QG_ActionHandler::slotSetRelativeZero);

    //connect(bRelZero, SIGNAL(triggered()), this, SLOT(slotSetRelativeZero()));
    bLockRelZero = justAddAction("LockRelativeZero", actionsMap);
    connect(bLockRelZero, &QAction::triggered, actionHandler, &QG_ActionHandler::slotLockRelativeZero);

    //restore snapMode from saved preferences

    setSnaps( RS_SnapMode::fromInt(LC_GET_ONE_INT("Snap", "SnapMode", 0)));
}

void QG_SnapToolBar::slotUnsetSnapMiddleManual(){
    snapMiddleManual->setChecked(false);
}

void QG_SnapToolBar::saveSnapMode(){
    //@write default snap mode from prefrences.
    unsigned int snapFlags {RS_SnapMode::toInt( getSnaps())};
    LC_SET_ONE("Snap", "SnapMode", QString::number(snapFlags));
    // no need to delete child widgets, Qt does it all for us
}

void QG_SnapToolBar::setSnaps ( RS_SnapMode const& s ){
    snapFree->setChecked(s.snapFree);
    snapGrid->setChecked(s.snapGrid);
    snapEnd->setChecked(s.snapEndpoint);
    snapOnEntity->setChecked(s.snapOnEntity);
    snapCenter->setChecked(s.snapCenter);
    snapMiddle->setChecked(s.snapMiddle);
    snapDistance->setChecked(s.snapDistance);
    snapIntersection->setChecked(s.snapIntersection);


    bool restHorizontal = s.restriction == RS2::RestrictHorizontal;
    bool restOrtho = s.restriction == RS2::RestrictOrthogonal;
    bool restVertical = s.restriction == RS2::RestrictVertical;

    restrictHorizontal->setChecked(restHorizontal || restOrtho);
    restrictVertical->setChecked(restVertical || restOrtho);
    restrictOrthogonal->setChecked(restOrtho);
    restrictNothing->setChecked(s.restriction==RS2::RestrictNothing);
}

RS_SnapMode QG_SnapToolBar::getSnaps( void ) const{
    RS_SnapMode s;

    s.snapFree         = snapFree->isChecked();
    s.snapGrid         = snapGrid->isChecked();
    s.snapEndpoint     = snapEnd->isChecked();
    s.snapOnEntity     = snapOnEntity->isChecked();
    s.snapCenter       = snapCenter->isChecked();
    s.snapMiddle       = snapMiddle->isChecked();
    s.snapDistance       = snapDistance->isChecked();
    s.snapIntersection = snapIntersection->isChecked();
    // removed Restrict Othogonal button
    // todo simplify internal restrict rules
    int const rH = (restrictHorizontal != nullptr && restrictHorizontal->isChecked())? 1:0;
    int const rV = (restrictVertical != nullptr && restrictVertical->isChecked())? 2: 0;
    switch (rH + rV) {
        case 3:
            s.restriction = RS2::RestrictOrthogonal;
            break;
        case 2:
            s.restriction = RS2::RestrictVertical;
            break;
        case 1:
            s.restriction = RS2::RestrictHorizontal;
            break;
        default:
            s.restriction = RS2::RestrictNothing;
    }
    return s;
}

bool QG_SnapToolBar::lockedRelativeZero() const{
    return bLockRelZero->isChecked();
}

void QG_SnapToolBar::setLockedRelativeZero(bool on){
    bLockRelZero->setChecked(on);
    bLockRelZero->setToolTip(tr("Relative zero position is %1").arg(on ? tr("locked") : tr("unlocked")));
}

/* Slots */

void QG_SnapToolBar::slotRestrictNothing(bool checked){
    if (checked) {
        restrictVertical->setChecked(!checked);
        restrictHorizontal->setChecked(!checked);
        restrictOrthogonal->setChecked(!checked);
        restrictNothing->setChecked(checked);
        actionTriggered();
    }
}

void QG_SnapToolBar::slotRestrictOrthogonal(bool checked){
    restrictVertical->setChecked(checked);
    restrictHorizontal->setChecked(checked);
    restrictNothing->setChecked(!checked);
    actionTriggered();
}

void QG_SnapToolBar::slotEnableRelativeZeroSnaps(const bool enabled){
    bRelZero->setEnabled(enabled);
    bLockRelZero->setEnabled(enabled);
}

void QG_SnapToolBar::actionTriggered(){
    actionHandler->slotSetSnaps(getSnaps());
}

LC_SnapOptionsWidgetsHolder *QG_SnapToolBar::getSnapOptionsHolder() {
    auto* snapOptionsHolder = new LC_SnapOptionsWidgetsHolder(this);
    snapOptionsHolder->setLocatedOnLeft(false);
    addWidget(snapOptionsHolder);
    return snapOptionsHolder;
}
