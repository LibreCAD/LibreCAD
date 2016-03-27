/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Dongxu Li ( dongxuli2011@gmail.com )
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#ifndef RS_ACTIONMODIFYOFFSET_H
#define RS_ACTIONMODIFYOFFSET_H

#include "rs_previewactioninterface.h"

class RS_OffsetData;

/**
 * This action class create entity by offset
 *
 * @author Dongxu Li
 */
class RS_ActionModifyOffset : public RS_PreviewActionInterface {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetPosition       /**< Setting the direction of offset*/
    };

public:
    RS_ActionModifyOffset(RS_EntityContainer& container,
                     RS_GraphicView& graphicView);
	~RS_ActionModifyOffset() override;

//    void reset();

	void init(int status=0) override;
	void trigger() override;

	void mouseMoveEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

	//        void coordinateEvent(RS_CoordinateEvent* e) override;
//    void commandEvent(RS_CommandEvent* e) override;
//    QStringList getAvailableCommands() override;

	void hideOptions() override;
	void showOptions() override;

	void updateMouseButtonHints() override;
	void updateMouseCursor() override;

protected:

	std::unique_ptr<RS_OffsetData> data;
};
#endif
