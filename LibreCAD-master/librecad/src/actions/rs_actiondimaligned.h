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

#ifndef RS_ACTIONDIMALIGNED_H
#define RS_ACTIONDIMALIGNED_H

#include "rs_actiondimension.h"

struct RS_DimAlignedData;

/**
 * This action class can handle user events to draw
 * aligned dimensions.
 *
 * @author Andrew Mustun
 */
class RS_ActionDimAligned : public RS_ActionDimension {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetExtPoint1,    /**< Setting the 1st ext point.  */
        SetExtPoint2,    /**< Setting the 2nd ext point. */
        SetDefPoint,     /**< Setting the common def point */
                SetText          /**< Setting the text label in command line */
    };

public:
    RS_ActionDimAligned(RS_EntityContainer& container,
                        RS_GraphicView& graphicView);
	~RS_ActionDimAligned() override;

	void reset() override;

	void trigger() override;
        void preparePreview();

	void mouseMoveEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

		void coordinateEvent(RS_CoordinateEvent* e) override;
	void commandEvent(RS_CommandEvent* e) override;
		QStringList getAvailableCommands() override;

	void hideOptions() override;
	void showOptions() override;

	void updateMouseButtonHints() override;

protected:
		/**
	 * Aligned dimension data.
	 */
		std::unique_ptr<RS_DimAlignedData> edata;

		/** Last status before entering text. */
		Status lastStatus;
}
;

#endif
