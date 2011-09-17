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
#include <iostream>

#include <QContextMenuEvent>
#include <QToolBar>

#include "qg_snaptoolbar.h"

/*
 *  Constructs a QG_CadToolBarSnap as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_SnapToolBar::QG_SnapToolBar( const QString & title, QWidget * parent )
    : QToolBar(title, parent) {
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_SnapToolBar::~QG_SnapToolBar()
{
    // no need to delete child widgets, Qt does it all for us
}

void QG_SnapToolBar::setSnaps ( RS_SnapMode s )
{
    snapEnd->setChecked(s.snapEndpoint);
    snapOnEntity->setChecked(s.snapOnEntity);
    snapCenter->setChecked(s.snapCenter);
    snapMiddle->setChecked(s.snapMiddle);
    snapIntersection->setChecked(s.snapIntersection);
}

RS_SnapMode QG_SnapToolBar::getSnaps ( void )
{
    RS_SnapMode s;

    s.snapEndpoint     = snapEnd->isChecked();
    s.snapOnEntity     = snapOnEntity->isChecked();
    s.snapCenter       = snapCenter->isChecked();
    s.snapMiddle       = snapMiddle->isChecked();
    s.snapIntersection = snapIntersection->isChecked();

    return s;
}

void QG_SnapToolBar::init()
{
    /*snapFree = new QAction(QIcon(":/extui/snapfree.png"), "Snap Free", this);
    snapFree->setCheckable(true);
    connect(snapFree, SIGNAL(triggered()), this, SLOT(actionTriggered()));
    this->addAction(snapFree);*/
    snapEnd = new QAction(QIcon(":/extui/snapendpoint.png"), "Snap on Endpoints", this);
    snapEnd->setCheckable(true);
    connect(snapEnd, SIGNAL(triggered()), this, SLOT(actionTriggered()));
    this->addAction(snapEnd);
    snapOnEntity = new QAction(QIcon(":/extui/snaponentity.png"), "Snap on Entity", this);
    snapOnEntity->setCheckable(true);
    connect(snapOnEntity, SIGNAL(triggered()), this, SLOT(actionTriggered()));
    this->addAction(snapOnEntity);
    snapCenter = new QAction(QIcon(":/extui/snapcenter.png"), "Snap Center", this);
    snapCenter->setCheckable(true);
    connect(snapCenter, SIGNAL(triggered()), this, SLOT(actionTriggered()));
    this->addAction(snapCenter);
    snapMiddle = new QAction(QIcon(":/extui/snapmiddle.png"), "Snap Middle", this);
    snapMiddle->setCheckable(true);
    connect(snapMiddle, SIGNAL(triggered()), this, SLOT(actionTriggered()));
    this->addAction(snapMiddle);
    /*snapDistance = new QAction(QIcon(":/extui/snapdist.png"), "Snap Distance", this);
    snapDistance->setCheckable(true);
    connect(snapDistance, SIGNAL(triggered()), this, SLOT(actionTriggered()));
    this->addAction(snapDistance);*/
    snapIntersection = new QAction(QIcon(":/extui/snapintersection.png"), "Snap Intersection", this);
    snapIntersection->setCheckable(true);
    connect(snapIntersection, SIGNAL(triggered()), this, SLOT(actionTriggered()));
    this->addAction(snapIntersection);

    this->addSeparator();

    QAction *a;

    a = new QAction(QIcon(":/extui/restrictorthogonal.png"), "Restrict Orthogonal", this);
    a->setCheckable(true);
    connect(a, SIGNAL(triggered()), this, SLOT(actionTriggered()));
    this->addAction(a);
    a = new QAction(QIcon(":/extui/restricthorizontal.png"), "Restrict Horizontal", this);
    a->setCheckable(true);
    connect(a, SIGNAL(triggered()), this, SLOT(actionTriggered()));
    this->addAction(a);
    a = new QAction(QIcon(":/extui/restrictvertical.png"), "Restrict Vertical", this);
    a->setCheckable(true);
    connect(a, SIGNAL(triggered()), this, SLOT(actionTriggered()));
    this->addAction(a);

    this->addSeparator();

    a = new QAction(QIcon(":/extui/relzeromove.png"), "Relitave zero move", this);
    a->setCheckable(false);
    connect(a, SIGNAL(triggered()), this, SLOT(actionTriggered()));
    this->addAction(a);
    a = new QAction(QIcon(":/extui/relzerolock.png"), "Relative zero lock", this);
    a->setCheckable(false);
    connect(a, SIGNAL(triggered()), this, SLOT(actionTriggered()));
    this->addAction(a);
}

/* Slots */

void QG_SnapToolBar::actionTriggered()
{
    std::cerr << "Triggered" <<std::endl;
    emit snapsChanged(getSnaps());
}

