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

#include "qc_dialogfactory.h"

#include <QMdiArea>

#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"

#include "qg_blockwidget.h"
#include "qg_graphicview.h"

#include "rs_blocklist.h"
#include "rs_debug.h"
#include "rs_grid.h"



QC_DialogFactory::QC_DialogFactory(QWidget* parent, QToolBar* ow,  LC_SnapOptionsWidgetsHolder *snapOptionsHolder) :
  QG_DialogFactory(parent, ow, snapOptionsHolder)
{}

/**
 * Closes the window that is editing the given block.
 */
void QC_DialogFactory::closeEditBlockWindow(RS_Block* block) {
    auto& appWindow = QC_ApplicationWindow::getAppWindow();
    QC_MDIWindow* blockWindow = appWindow->getWindowWithDoc(block);

    if (blockWindow != nullptr) {
        appWindow->closeWindow(blockWindow);
    }
}
