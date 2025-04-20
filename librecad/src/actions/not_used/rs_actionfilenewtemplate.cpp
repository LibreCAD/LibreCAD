// /****************************************************************************
//
// Utility base class for widgets that represents options for actions
//
// Copyright (C) 2025 LibreCAD.org
// Copyright (C) 2025 sand1024
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
// **********************************************************************
//


#include "rs_actionfilenewtemplate.h"


RS_ActionFileNewTemplate::RS_ActionFileNewTemplate(LC_ActionContext *actionContext)
        :RS_ActionInterface("File New", actionContext, RS2::ActionFileNewTemplate) {}


void RS_ActionFileNewTemplate::trigger() {
    /*
    // Not supported currently
    RS_DEBUG->print("RS_ActionFileNewTemplate::trigger");

    QString fileName; //= RS_DIALOGFACTORY->requestFileNewDialog();
    if (graphic && !fileName.isEmpty()) {
        graphic->open(fileName, );
}
    */
    finish(false);
}


void RS_ActionFileNewTemplate::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}
