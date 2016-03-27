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

#ifndef RS_ACTIONLOCKRELATIVEZERO_H
#define RS_ACTIONLOCKRELATIVEZERO_H

#include "rs_previewactioninterface.h"


/**
 * This action class can handle user events to lock or unlock
 * the relative Zero point.
 * It overrides but retains the locking of the relative Zero.
 *
 * @author Ulf Lehnert
 */
class RS_ActionLockRelativeZero : public RS_ActionInterface {
	Q_OBJECT
public:
    RS_ActionLockRelativeZero(RS_EntityContainer& container,
                             RS_GraphicView& graphicView,
							 bool on);

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);

	void init(int status) override;
	void trigger() override;

private:
	bool on;
};

#endif
