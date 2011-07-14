/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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
public:
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
    virtual ~RS_ActionLibraryInsert();

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);
	
	virtual RS2::ActionType rtti() {
		return RS2::ActionLibraryInsert;
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

        void setFile(const QString& file);

	double getAngle() {
		return data.angle;
	}

	void setAngle(double a) {
		data.angle = a;
	}

	double getFactor() {
		return data.factor;
	}

	void setFactor(double f) {
		data.factor = f;
	}

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
	RS_Graphic prev;
	RS_LibraryInsertData data;
	
	/** Last status before entering option. */
	Status lastStatus;
};

#endif
