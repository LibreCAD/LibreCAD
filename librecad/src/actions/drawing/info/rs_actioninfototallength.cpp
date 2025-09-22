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

#include "rs_actioninfototallength.h"

#include "lc_actioninfomessagebuilder.h"
#include "rs_debug.h"
#include "rs_entitycontainer.h"

RS_ActionInfoTotalLength::RS_ActionInfoTotalLength(LC_ActionContext *actionContext)
        :LC_ActionPreSelectionAwareBase("Info Total Length",actionContext, RS2::ActionInfoTotalLength){
}

void RS_ActionInfoTotalLength::drawSnapper() {
    // disable snapper;
}

bool RS_ActionInfoTotalLength::isAllowTriggerOnEmptySelection() {
    return false;
}

void RS_ActionInfoTotalLength::doTrigger([[maybe_unused]]bool selected) {
    RS_DEBUG->print("RS_ActionInfoTotalLength::trigger()");
    double l=m_container->totalSelectedLength();

    if (l>0.0) {
        QString len= formatLinear(l);
        commandMessage(tr("Total Length of selected entities: %1").arg(len));
    } else {
        commandMessage(tr("At least one of the selected entities cannot be measured."));
    }

    finish(false);
}

void RS_ActionInfoTotalLength::finishMouseMoveOnSelection([[maybe_unused]] LC_MouseEvent *event) {
    const RS_EntityContainer::LC_SelectionInfo &selectionInfo = m_container->getSelectionInfo();
    unsigned int selectedCount = selectionInfo.count;
    auto builder = msgStart().string(tr("Selected:"), QString::number(selectedCount));
    if (selectedCount > 0) {
        builder.linear(tr("Total Length:"),selectionInfo.length);
    }
    builder.toInfoCursorZone2(true);
}


void RS_ActionInfoTotalLength::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to measure total length (Enter to complete)"), MOD_SHIFT_AND_CTRL(tr("Select contour"),tr("Select and finish")));
}
