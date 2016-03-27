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

#ifndef RS_ACTIONDIMENSION_H
#define RS_ACTIONDIMENSION_H

#include "rs_previewactioninterface.h"

struct RS_DimensionData;

/**
 * Base class for dimension actions.
 *
 * @author Andrew Mustun
 */
class RS_ActionDimension : public RS_PreviewActionInterface {
    Q_OBJECT
public:
    RS_ActionDimension(const char* name,
                       RS_EntityContainer& container,
                       RS_GraphicView& graphicView);
	~RS_ActionDimension() override;

	virtual void reset();

	void init(int status=0) override;

	void hideOptions() override;
	void showOptions() override;

	void updateMouseCursor() override;
//    void updateToolBar() override;

	QString getText() const;
	
	void setText(const QString& t);

	const QString& getLabel() const;
	void setLabel(const QString& t);
	const QString& getTol1() const;
	void setTol1(const QString& t);
	const QString& getTol2() const;
	void setTol2(const QString& t);
	bool getDiameter() const;
	void setDiameter(bool d);

    static bool isDimensionAction(RS2::ActionType type);

protected:
    /**
     * Generic dimension data.
     */
	std::unique_ptr<RS_DimensionData> data;

    QString label;
    QString tol1;
    QString tol2;
    bool diameter;

    /**
     * Commands.
     */
    /*
      QString cmdText;
      QString cmdText2;
    */
};

#endif
