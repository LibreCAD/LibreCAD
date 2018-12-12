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

#ifndef RS_ACTIONSELECT_H
#define RS_ACTIONSELECT_H

#include <set>
#include "rs_actioninterface.h"
#include "qg_actionhandler.h"


/**
 * This action class can handle user events to select entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionSelect : public RS_ActionInterface {
        Q_OBJECT
public:

    RS_ActionSelect(QG_ActionHandler* a_handler,
                    RS_EntityContainer& container,
                    RS_GraphicView& graphicView,
                    RS2::ActionType nextAction,
					std::initializer_list<RS2::EntityType> const& entityTypeList=std::initializer_list<RS2::EntityType>());

	void init(int status) override;
	void resume() override;
	//void keyPressEvent(QKeyEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;
	void updateMouseButtonHints() override;
	void updateMouseCursor() override;
	int countSelected();
	void keyPressEvent(QKeyEvent* e) override;

private:
	std::initializer_list<RS2::EntityType> const entityTypeList;
    RS2::ActionType nextAction;
    QG_ActionHandler* action_handler;
};

#endif
