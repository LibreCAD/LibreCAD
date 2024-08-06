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

#ifndef RS_ACTIONMODIFYSCALE_H
#define RS_ACTIONMODIFYSCALE_H

#include "rs_previewactioninterface.h"
#include "lc_actionmodifybase.h"

/**
 * This action class can handle user events to move entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionModifyScale : public LC_ActionModifyBase {
    Q_OBJECT
public:
    RS_ActionModifyScale(RS_EntityContainer& container,
                         RS_GraphicView& graphicView);
    ~RS_ActionModifyScale() override;

    void init(int status) override;
    void trigger() override;
    bool isIsotropicScaling();
    void setIsotropicScaling(bool enable);
    double getFactorX();
    void setFactorX(double val);
    double getFactorY();
    void setFactorY(double val);
    bool isExplicitFactor();
    void setExplicitFactor(bool val);
    QStringList getAvailableCommands() override;
protected:
    /**
     * Action States.
     */
    enum Status {
        SetReferencePoint,    /**< Setting the reference point. */
        SetSourcePoint,         /**< Set the source point to find scaling factor */
        SetTargetPoint,        /**< Set the target point to scale the source point to */
    };

    struct Points;
    std::unique_ptr<Points> pPoints;
    // set scaling target point to support isotropic or xy-scaling
    RS_Vector getTargetPoint(QMouseEvent* e);
    void findFactor();
    void showPreview();
    void showPreview(RS_ScaleData &previewData);
    void determineScaleFactor(RS_ScaleData& data, const RS_Vector &reference, const RS_Vector &source, const RS_Vector &target);
    LC_ModifyOperationFlags *getModifyOperationFlags() override;
    void mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *pEvent) override;
    void mouseRightButtonReleaseEventSelected(int status, QMouseEvent *pEvent) override;
    void mouseMoveEventSelected(QMouseEvent *e) override;
    void updateMouseButtonHintsForSelection() override;
    void updateMouseButtonHintsForSelected(int status) override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    bool doProcessCommand(int status, const QString &command) override;
    void tryTrigger();
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
};
#endif
