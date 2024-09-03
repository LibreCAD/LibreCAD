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

#ifndef RS_ACTIONDIMLINEAR_H
#define RS_ACTIONDIMLINEAR_H

#include "rs_actiondimension.h"
#include "rs_constructionline.h"
#include "lc_actiondimlinearbase.h"

struct RS_DimLinearData;

/**
 * This action class can handle user events to draw 
 * aligned dimensions.
 *
 * @author Andrew Mustun
 */
class RS_ActionDimLinear: public LC_ActionDimLinearBase {
Q_OBJECT
public:
/**
 * Varitions of this action.
 */
    enum Variation {
        AnyAngle,
        Horizontal,
        Vertical
    };

public:
    RS_ActionDimLinear(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView,
        double angle = 0.0, bool fixedAngle = false,
        RS2::ActionType type = RS2::ActionDimLinear);
    ~RS_ActionDimLinear() override;
    void preparePreview() override;
    QStringList getAvailableCommands() override;
//    void showOptions() override;
    double getAngle() const;
    void setAngle(double a);
    bool hasFixedAngle() const;
protected:

    /**
     * Aligned dimension data.
     */
    std::unique_ptr<RS_DimLinearData> edata;
    /**
     * Is the angle fixed?
     */
    bool fixedAngle = false;
/** Last status before entering text or angle. */
    Status lastStatus = SetExtPoint1;
    void reset() override;
    RS_Vector getExtensionPoint1() override;
    RS_Vector getExtensionPoint2() override;
    double getDimAngle() override;
    void setExtensionPoint1(RS_Vector p) override;
    void setExtensionPoint2(RS_Vector p) override;
    RS_Entity *createDim(RS_EntityContainer* parent) override;

    bool doProcessCommand(int status, const QString &command) override;
};

#endif
