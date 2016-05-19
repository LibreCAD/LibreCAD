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

#ifndef RS_ACTIONLIBRARYINSERT_H
#define RS_ACTIONLIBRARYINSERT_H

#include "rs_previewactioninterface.h"

#include "rs_graphic.h"
#include "rs_creation.h"

/**
 * This action class can handle user events for inserting library items 
 * (or any other DXF files) into the current drawing (as block/insert).
 *
 * @author Andrew Mustun
 */
class RS_ActionLibraryInsert : public RS_PreviewActionInterface {
	Q_OBJECT
    /**
     * Action States.
     */
    enum Status {
        SetTargetPoint,    /**< Setting the reference point. */
		SetAngle,          /**< Setting angle in the command line. */
		SetFactor          /**< Setting factor in the command line. */
		//SetColumns,        /**< Setting columns in the command line. */
		//SetRows,           /**< Setting rows in the command line. */
		//SetColumnSpacing,  /**< Setting column spacing in the command line. */
		//SetRowSpacing      /**< Setting row spacing in the command line. */
    };

public:
    RS_ActionLibraryInsert(RS_EntityContainer& container,
                        RS_GraphicView& graphicView);
	~RS_ActionLibraryInsert() override;

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);
	
	void init(int status=0) override;

	void reset();

	void trigger() override;

	void mouseMoveEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

	void coordinateEvent(RS_CoordinateEvent* e) override;
	void commandEvent(RS_CommandEvent* e) override;
		QStringList getAvailableCommands() override;

	void showOptions() override;
	void hideOptions() override;

	void updateMouseButtonHints() override;
	void updateMouseCursor() override;

	void setFile(const QString& file);

	double getAngle() const;

	void setAngle(double a);

	double getFactor() const;

	void setFactor(double f);

	/*int getColumns() {
		return data.cols;
	}

	void setColumns(int c) {
		data.cols = c;
	}
	
	int getRows() {
		return data.rows;
	}

	void setRows(int r) {
		data.rows = r;
	}

	double getColumnSpacing() {
		return data.spacing.x;
	}

	void setColumnSpacing(double cs) {
		data.spacing.x = cs;
	}
	
	double getRowSpacing() {
		return data.spacing.y;
	}

	void setRowSpacing(double rs) {
		data.spacing.y = rs;
	}*/

protected:
	//RS_Block* block;
	//RS_InsertData data;
	struct Points;
	std::unique_ptr<Points> pPoints;
	
	/** Last status before entering option. */
	Status lastStatus;
};

#endif
