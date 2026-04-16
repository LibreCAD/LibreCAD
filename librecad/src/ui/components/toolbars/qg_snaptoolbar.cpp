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

#include "qg_snaptoolbar.h"

#include <QMenu>
#include <QToolButton>

#include "lc_actiongroupmanager.h"
#include "lc_snapoptionswidgetsholder.h"
#include "lc_visual_snap_data.h"
#include "qg_actionhandler.h"
#include "rs_actioninterface.h"
#include "rs_settings.h"

QAction* QG_SnapToolBar::justAddAction(const QString& name, const QMap<QString, QAction*>& actionsMap) {
    auto* action = actionsMap[name];
    addAction(action);
    return action;
}

QAction* QG_SnapToolBar::addOwnAction(const QString& name, const QMap<QString, QAction*>& actionsMap) {
    auto* action = actionsMap[name];
    connect(action, &QAction::triggered, this, &QG_SnapToolBar::actionTriggered);
    addAction(action);
    return action;
}

void QG_SnapToolBar::addVisualSnapAction(QMenu* menu, const QMap<QString, QAction*>& actionsMap, const char* actionName, const char* settingKey) {
    auto vsActionAutoAddSnap = actionsMap[actionName];
    connect(vsActionAutoAddSnap, &QAction::toggled, [settingKey, this](bool toggled) {
        LC_SET_ONE("Snap", settingKey, toggled);
        auto currentAction = m_actionHandler->getCurrentAction();
        if (currentAction != nullptr) {
            currentAction->refreshBySettings();
        }
    });
    menu->addAction(vsActionAutoAddSnap);
}

QG_SnapToolBar::QG_SnapToolBar(QWidget* parent, QG_ActionHandler* ah, const LC_ActionGroupManager* agm,
                               const QMap<QString, QAction*>& actionsMap)
    : QToolBar(parent), m_actionHandler(ah) {

    m_actionSnapVisual = addOwnAction("SnapVisual", actionsMap);

    QWidget *button = widgetForAction(m_actionSnapVisual);
    auto tb = dynamic_cast<QToolButton*>(button);
    m_actionSnapVisualLock = actionsMap["SnapVisualLock"];
    if (tb != nullptr) {
        tb->setPopupMode(QToolButton::MenuButtonPopup);
        auto menu = new QMenu();
        menu->addAction(m_actionSnapVisualLock);
        menu->addSeparator();
        addVisualSnapAction(menu, actionsMap, "SnapVisualAutoAddSnap", "VSSnapAutoAddSnapPoint");
        addVisualSnapAction(menu, actionsMap, "SnapVisualAutoAddSnapLast", "VSSnapAutoAddLastSnapPointOnly");
        addVisualSnapAction(menu, actionsMap, "SnapVisualAngleSnap", "VSAngleSnapStepRaysVertexes");
        addVisualSnapAction(menu, actionsMap, "SnapVisualRelAngleSnap", "VSAngleSnapStepRaysRelative");
        addVisualSnapAction(menu, actionsMap, "SnapVisualDynDistance", "VSVertexVertexDistanceCircles");
        addVisualSnapAction(menu, actionsMap, "SnapVisualDistanceTan", "VSVertexVertexDistanceTangents");
        menu->addSeparator();
        addVisualSnapAction(menu, actionsMap, "SnapVisualShowFarGuides", "VSShowNotSnappableGuides");
        tb->setMenu(menu);
    }

    const auto action = justAddAction("ExclusiveSnapMode", actionsMap);
    connect(action, &QAction::triggered, agm, &LC_ActionGroupManager::toggleExclusiveSnapMode);

    // fixme - sand - rework this !
    m_actionSnapMiddleManual = justAddAction("SnapMiddleManual", actionsMap);
    connect(m_actionSnapMiddleManual, &QAction::triggered, m_actionHandler, &QG_ActionHandler::slotSnapMiddleManual);

    // m_actionSnapMiddleManual = addOwnAction("SnapMiddleManual", actionsMap);

    m_actionSnapFree = addOwnAction("SnapFree", actionsMap);
    m_actionSnapGrid = addOwnAction("SnapGrid", actionsMap);
    m_actionSnapEnd = addOwnAction("SnapEnd", actionsMap);
    m_actionSnapOnEntity = addOwnAction("SnapEntity", actionsMap);
    m_actionSnapCenter = addOwnAction("SnapCenter", actionsMap);
    m_actionSnapMiddle = addOwnAction("SnapMiddle", actionsMap);
    m_actionSnapDistance = addOwnAction("SnapDistance", actionsMap);
    m_actionSnapIntersection = addOwnAction("SnapIntersection", actionsMap);

    addSeparator();

    m_actionRestrictHorizontal = addOwnAction("RestrictHorizontal", actionsMap);
    m_actionRestrictVertical = addOwnAction("RestrictVertical", actionsMap);

    m_actionRestrictOrthogonal = justAddAction("RestrictOrthogonal", actionsMap);
    connect(m_actionRestrictOrthogonal, &QAction::triggered, this, &QG_SnapToolBar::slotRestrictOrthogonal);

    m_actionRestrictNothing = justAddAction("RestrictNothing", actionsMap);
    connect(m_actionRestrictNothing, &QAction::triggered, this, &QG_SnapToolBar::slotRestrictNothing);
    // todo - in general, restrict nothing has no practical sense at all - as buttons are toggled, the amount of clicks is the same
    // todo - so probably, it's better to keep it hidden for now.
    // todo - However, it may be practical, if "restrict to angle" will be implemented
    m_actionRestrictNothing->setVisible(false);

    addSeparator();

    m_actionRelZero = justAddAction("SetRelativeZero", actionsMap);

    m_actionLockRelZero = justAddAction("LockRelativeZero", actionsMap);
    m_actionLockRelZero->setCheckable(true);
    connect(m_actionLockRelZero, &QAction::triggered, m_actionHandler, &QG_ActionHandler::slotLockRelativeZero);

    connect(m_actionSnapVisual, &QAction::triggered, m_actionHandler, [this](bool toggled) {
        m_actionSnapVisualLock->setEnabled(toggled);
    });

    //restore snapMode from saved preferences
    setSnaps(RS_SnapMode::fromInt(LC_GET_ONE_INT("Snap", "SnapMode", 0)));
}

void QG_SnapToolBar::setGraphicView(RS_GraphicView* gview) {
    if (gview != nullptr) {
        auto visualSnapData = gview->getVisualSnapData();
        bool locked = visualSnapData->isContentLocked();
        m_actionSnapVisualLock->blockSignals(true);
        m_actionSnapVisualLock->setChecked(locked);
        m_actionSnapVisualLock->blockSignals(false);
        if (locked) {
            m_actionSnapVisual->setIcon(QIcon(":/icons/snap_visual_lock.lci"));
        }
        else {
            m_actionSnapVisual->setIcon(QIcon(":/icons/snap_visual.lci"));
        }

    }
}


void QG_SnapToolBar::slotUnsetSnapMiddleManual() const {
    m_actionSnapMiddleManual->setChecked(false);
}

void QG_SnapToolBar::saveSnapMode() const {
    //@write default snap mode from prefrences.
    const unsigned int snapFlags{RS_SnapMode::toInt(getSnaps())};
    LC_SET_ONE("Snap", "SnapMode", QString::number(snapFlags));
    // no need to delete child widgets, Qt does it all for us
}

void QG_SnapToolBar::setSnaps(const RS_SnapMode& s) const {
    m_actionSnapVisual->setChecked(s.snapVisual);
    m_actionSnapFree->setChecked(s.snapFree);
    m_actionSnapGrid->setChecked(s.snapGrid);
    m_actionSnapEnd->setChecked(s.snapEndpoint);
    m_actionSnapOnEntity->setChecked(s.snapOnEntity);
    m_actionSnapCenter->setChecked(s.snapCenter);
    m_actionSnapMiddle->setChecked(s.snapMiddle);
    m_actionSnapDistance->setChecked(s.snapDistance);
    m_actionSnapIntersection->setChecked(s.snapIntersection);

    const bool restHorizontal = s.restriction == RS2::RestrictHorizontal;
    const bool restOrtho = s.restriction == RS2::RestrictOrthogonal;
    const bool restVertical = s.restriction == RS2::RestrictVertical;

    m_actionRestrictHorizontal->setChecked(restHorizontal || restOrtho);
    m_actionRestrictVertical->setChecked(restVertical || restOrtho);
    m_actionRestrictOrthogonal->setChecked(restOrtho);
    m_actionRestrictNothing->setChecked(s.restriction == RS2::RestrictNothing);
}

RS_SnapMode QG_SnapToolBar::getSnaps() const {
    RS_SnapMode s;
    s.snapVisual = m_actionSnapVisual->isChecked();
    s.snapFree = m_actionSnapFree->isChecked();
    s.snapGrid = m_actionSnapGrid->isChecked();
    s.snapEndpoint = m_actionSnapEnd->isChecked();
    s.snapOnEntity = m_actionSnapOnEntity->isChecked();
    s.snapCenter = m_actionSnapCenter->isChecked();
    s.snapMiddle = m_actionSnapMiddle->isChecked();
    s.snapDistance = m_actionSnapDistance->isChecked();
    s.snapIntersection = m_actionSnapIntersection->isChecked();
    // removed Restrict Othogonal button
    // todo simplify internal restrict rules
    const int rH = (m_actionRestrictHorizontal != nullptr && m_actionRestrictHorizontal->isChecked()) ? 1 : 0;
    const int rV = (m_actionRestrictVertical != nullptr && m_actionRestrictVertical->isChecked()) ? 2 : 0;
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

bool QG_SnapToolBar::lockedRelativeZero() const {
    return m_actionLockRelZero->isChecked();
}

void QG_SnapToolBar::setLockedRelativeZero(const bool on) const {
    m_actionLockRelZero->setChecked(on);
    m_actionLockRelZero->setToolTip(tr("Relative zero position is %1").arg(on ? tr("locked") : tr("unlocked")));
}

void QG_SnapToolBar::setUCSActive(const bool on) const {
    m_ucsMode->setChecked(on);
    m_ucsMode->setToolTip(tr("Coordinate system: %1").arg(on ? tr("User") : tr("World")));
}

/* Slots */

void QG_SnapToolBar::slotRestrictNothing(const bool checked) const {
    if (checked) {
        m_actionRestrictVertical->setChecked(false);
        m_actionRestrictHorizontal->setChecked(false);
        m_actionRestrictOrthogonal->setChecked(false);
        m_actionRestrictNothing->setChecked(true);
        actionTriggered();
    }
}

void QG_SnapToolBar::slotRestrictOrthogonal(const bool checked) const {
    m_actionRestrictVertical->setChecked(checked);
    m_actionRestrictHorizontal->setChecked(checked);
    m_actionRestrictNothing->setChecked(!checked);
    actionTriggered();
}

void QG_SnapToolBar::slotEnableRelativeZeroSnaps(const bool enabled) const {
    m_actionRelZero->setEnabled(enabled);
    m_actionLockRelZero->setEnabled(enabled);
}

void QG_SnapToolBar::actionTriggered() const {
    m_actionHandler->setSnaps(getSnaps());
}

LC_SnapOptionsWidgetsHolder* QG_SnapToolBar::getSnapOptionsHolder() {
    auto* snapOptionsHolder = new LC_SnapOptionsWidgetsHolder(this);
    snapOptionsHolder->setLocatedOnLeft(false);
    addWidget(snapOptionsHolder);
    return snapOptionsHolder;
}
