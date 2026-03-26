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


#include "lc_action_modify_base.h"

struct RS_OffsetData;

/**
 * This action class create entity by offset
 *
 * @author Dongxu Li
 */
class LC_ActionModifyOffset : public LC_ActionModifyBase {
    Q_OBJECT
public:
    explicit LC_ActionModifyOffset(LC_ActionContext *actionContext);
    ~LC_ActionModifyOffset() override;
    double getDistance() const;
    void setDistance(double distance) const;
    bool isFixedDistance() const {return m_distanceIsFixed;}
    void setDistanceFixed(bool value);
protected:
    /**
     * Action States.
     */
    enum Status {
        SetReferencePoint,
        SetPosition       /**< Setting the direction of offset*/
    };

    bool m_distanceIsFixed = true;
    RS_Vector m_referencePoint = RS_Vector(false);
    std::unique_ptr<RS_OffsetData> m_offsetData;

    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void onMouseLeftButtonReleaseSelected(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonReleaseSelected(int status, const LC_MouseEvent* event) override;
    void updateMouseButtonHintsForSelection() override;
    void updateMouseButtonHintsForSelected(int status) override;
    bool isAllowTriggerOnEmptySelection() override;
    LC_ModifyOperationFlags *getModifyOperationFlags() override;
    void onMouseMoveEventSelected(int status, const LC_MouseEvent* e) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    void doTriggerCompletion(bool success) override;
    void doTriggerSelectionUpdate(bool keepSelected, const LC_DocumentModificationBatch& ctx) override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
