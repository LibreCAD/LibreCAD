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

#ifndef RS_ACTIONDRAWIMAGE_H
#define RS_ACTIONDRAWIMAGE_H

#include "rs_previewactioninterface.h"
#include "rs_image.h"
#include "rs_units.h"

/**
 * This action class can handle user events for inserting bitmaps into the
 * current drawing.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawImage : public RS_PreviewActionInterface {
	Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
		ShowDialog,        /**< Dialog. */
                SetTargetPoint,    /**< Setting the reference point. */
		SetAngle,          /**< Setting angle in the command line. */
                SetFactor,          /**< Setting factor in the command line. */
                SetDPI              /**< Setting dpi in the command line. */
		//SetColumns,        /**< Setting columns in the command line. */
		//SetRows,           /**< Setting rows in the command line. */
		//SetColumnSpacing,  /**< Setting column spacing in the command line. */
		//SetRowSpacing      /**< Setting row spacing in the command line. */
    };

public:
    RS_ActionDrawImage(RS_EntityContainer& container,
                        RS_GraphicView& graphicView);
    ~RS_ActionDrawImage();

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);
	
	virtual RS2::ActionType rtti() {
		return RS2::ActionDrawImage;
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
//    virtual void updateToolBar();

	double getAngle() {
		return data.uVector.angle();
	}

	void setAngle(double a) {
		double l = data.uVector.magnitude();
		data.uVector.setPolar(l, a);
		data.vVector.setPolar(l, a+M_PI/2);
	}

	double getFactor() {
		return data.uVector.magnitude();
	}

	void setFactor(double f) {
		double a = data.uVector.angle();
		data.uVector.setPolar(f, a);
		data.vVector.setPolar(f, a+M_PI/2);
	}

    double scaleToDpi(double scale);
    double dpiToScale(double dpi);

protected:
	RS_ImageData data;
        QImage img;
	
	/** Last status before entering option. */
	Status lastStatus;
};

#endif
