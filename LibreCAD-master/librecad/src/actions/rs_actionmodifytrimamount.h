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

#ifndef RS_ACTIONMODIFYTRIMAMOUNT_H
#define RS_ACTIONMODIFYTRIMAMOUNT_H

#include "rs_previewactioninterface.h"


/**
 * This action class can handle user events to trim entities by a given
 * amount.
 *
 * @author Andrew Mustun
 */
class RS_ActionModifyTrimAmount : public RS_ActionInterface {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        ChooseTrimEntity      /**< Choosing the entity to trim. */
    };

public:
    RS_ActionModifyTrimAmount(RS_EntityContainer& container,
                        RS_GraphicView& graphicView);
	~RS_ActionModifyTrimAmount() override;

	void init(int status=0) override;

	void trigger() override;

	//void mouseMoveEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

	void commandEvent(RS_CommandEvent* e) override;
		QStringList getAvailableCommands() override;

	void hideOptions() override;
	void showOptions() override;

	void updateMouseButtonHints() override;
	void updateMouseCursor() override;

	double getDistance() const {
		return distance;
	}
	bool getByTotal() const {
		return byTotal ;
	}

	void setDistance(double d) {
		distance = d;
	}
	void setByTotal(bool on) {
		byTotal = on;
	}

private:
    RS_Entity* trimEntity;
	std::unique_ptr<RS_Vector> trimCoord;
	double distance;
    bool byTotal;
        /**
         * Commands
         */
        /*
        QString cmdDistance;
        QString cmdDistance2;
        QString cmdDistance3;
        */
};

#endif
