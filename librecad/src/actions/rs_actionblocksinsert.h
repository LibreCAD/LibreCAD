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

class RS_Block;
struct RS_InsertData;

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
		SetUndefined = -1, /**< Setting undefined for initialisation. */
		SetTargetPoint = 0,/**< Setting the reference point. */
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
	~RS_ActionBlocksInsert() override;

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

	double getAngle() const;

	void setAngle(double a);

	double getFactor() const;

	void setFactor(double f);

	int getColumns() const;

	void setColumns(int c);
	
	int getRows() const;

	void setRows(int r);

	double getColumnSpacing() const;

	void setColumnSpacing(double cs);
	
	double getRowSpacing() const;

	void setRowSpacing(double rs);

protected:
	RS_Block* block;
	std::unique_ptr<RS_InsertData> data;
	
	/** Last status before entering option. */
	Status lastStatus;
};

#endif
