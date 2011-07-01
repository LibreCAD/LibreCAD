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

#ifndef RS_ACTIONBLOCKSINSERT_H
#define RS_ACTIONBLOCKSINSERT_H

#include "rs_previewactioninterface.h"
#include "rs_insert.h"

/**
 * This action class can handle user events for inserting blocks into the
 * current drawing.
 *
 * @author Andrew Mustun
 */
class RS_ActionBlocksInsert : public RS_PreviewActionInterface {
	Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetTargetPoint,    /**< Setting the reference point. */
		SetAngle,          /**< Setting angle in the command line. */
		SetFactor,         /**< Setting factor in the command line. */
		SetColumns,        /**< Setting columns in the command line. */
		SetRows,           /**< Setting rows in the command line. */
		SetColumnSpacing,  /**< Setting column spacing in the command line. */
		SetRowSpacing      /**< Setting row spacing in the command line. */
    };

public:
    RS_ActionBlocksInsert(RS_EntityContainer& container,
                        RS_GraphicView& graphicView);
    ~RS_ActionBlocksInsert();
	
	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);
	
	virtual RS2::ActionType rtti() {
		return RS2::ActionBlocksInsert;
	}

    virtual void init(int status=0);

	void reset();

    virtual void trigger();

    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

    virtual void coordinateEvent(RS_CoordinateEvent* e);
    virtual void commandEvent(RS_CommandEvent* e);
        virtual QStringList getAvailableCommands();

	virtual void showOptions();
	virtual void hideOptions();

    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();

	double getAngle() {
		return data.angle;
	}

	void setAngle(double a) {
		data.angle = a;
	}

	double getFactor() {
		return data.scaleFactor.x;
	}

	void setFactor(double f) {
		data.scaleFactor = RS_Vector(f, f);
	}

	int getColumns() {
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
	}

protected:
	RS_Block* block;
	RS_InsertData data;
	
	/** Last status before entering option. */
	Status lastStatus;
};

#endif
