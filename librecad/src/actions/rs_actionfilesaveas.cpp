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

#include "rs_actionfilesaveas.h"

#include <QAction>
#include "rs_graphic.h"

RS_ActionFileSaveAs::RS_ActionFileSaveAs(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Add Layer", container, graphicView) {}


QAction* RS_ActionFileSaveAs::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Save Drawing As")
	QAction* action = new QAction(tr("Save &as..."), NULL);
#if QT_VERSION >= 0x040600
        action->setIcon(QIcon::fromTheme("document-save-as", QIcon(":/actions/filesaveas.png")));
#else
        action->setIcon(QIcon(":/actions/filesaveas.png"));
#endif
// SaveAs was itroduces at 4.5 and later
#if QT_VERSION >= 0x040500
	action->setShortcut(QKeySequence::SaveAs);
#endif
    //action->zetStatusTip(tr("Saves the current drawing under a new filename"));
	return action;
}

void RS_ActionFileSaveAs::trigger() {
    RS_DEBUG->print("RS_ActionFileSaveAs::trigger");

    QString fileName; // = RS_DIALOGFACTORY->requestFileSaveAsDialog();
    if (graphic!=NULL && !fileName.isEmpty()) {
        graphic->saveAs(fileName, RS2::FormatUnknown);
    }
    finish(false);
}



void RS_ActionFileSaveAs::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
