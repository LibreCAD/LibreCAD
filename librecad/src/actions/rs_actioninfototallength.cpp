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

#include <QAction>
#include "rs_actioninfototallength.h"

#include "rs_dialogfactory.h"
#include "rs_graphic.h"
#include "rs_debug.h"


RS_ActionInfoTotalLength::RS_ActionInfoTotalLength(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Info Total Length",
					container, graphicView)
{
	actionType=RS2::ActionInfoTotalLength;
}

void RS_ActionInfoTotalLength::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

void RS_ActionInfoTotalLength::trigger() {

    RS_DEBUG->print("RS_ActionInfoTotalLength::trigger()");
	double l=container->totalSelectedLength();

	if (l>0.0) {
		QString len= RS_Units::formatLinear(l,
											graphic->getUnit(),
											graphic->getLinearFormat(), graphic->getLinearPrecision());
        RS_DIALOGFACTORY->commandMessage(
            tr("Total Length of selected entities: %1").arg(len));
    } else {
        RS_DIALOGFACTORY->commandMessage(tr("At least one of the selected "
                                            "entities cannot be measured."));
    }

    finish(false);
}

// EOF
