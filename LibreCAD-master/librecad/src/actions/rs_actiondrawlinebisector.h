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

#ifndef RS_ACTIONDRAWLINEBISECTOR_H
#define RS_ACTIONDRAWLINEBISECTOR_H

#include "rs_previewactioninterface.h"

class RS_Line;

/**
 * This action class can handle user events to draw bisectors.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawLineBisector : public RS_PreviewActionInterface {
	Q_OBJECT
private:
    enum Status {
        SetLine1,     /**< Choose the 1st line. */
        SetLine2,     /**< Choose the 2nd line. */
		SetLength,    /**< Set length in command line. */
		SetNumber     /**< Set number in command line. */
    };

public:
    RS_ActionDrawLineBisector(RS_EntityContainer& container,
                              RS_GraphicView& graphicView);
	~RS_ActionDrawLineBisector() override;
	
	void init(int status=0) override;

	void trigger() override;
	void mouseMoveEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

	void commandEvent(RS_CommandEvent* e) override;
		QStringList getAvailableCommands() override;
	
	void hideOptions() override;
	void showOptions() override;

	void updateMouseButtonHints() override;
	void updateMouseCursor() override;
	
	void setLength(double l);

	double getLength() const;
	
	void setNumber(int n);

	int getNumber() const;

private:
    /** Closest bisector. */
    RS_Line* bisector;
    /** First chosen entity */
    RS_Line* line1;
    /** Second chosen entity */
    RS_Line* line2;
    /** Length of the bisector. */
    double length;
	/** Number of bisectors to create. */
	int number;
	struct Points;
	std::unique_ptr<Points> pPoints;
	/** Last status before entering length or number. */
	Status lastStatus;
};

#endif
