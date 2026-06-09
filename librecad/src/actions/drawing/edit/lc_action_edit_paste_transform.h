/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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
 ******************************************************************************/

#ifndef LC_ACTIONEDITPASTETRANSFORM_H
#define LC_ACTIONEDITPASTETRANSFORM_H

#include "lc_undoabledocumentmodificationaction.h"
#include "rs_previewactioninterface.h"

class LC_ActionEditPasteTransform : public LC_UndoableDocumentModificationAction {
    Q_OBJECT public:
    explicit LC_ActionEditPasteTransform(LC_ActionContext* actionContext);
    void init(int status) override;
    void setAngle(double angle) const;
    double getFactor() const;
    void setFactor(double factor) const;
    bool isArrayCreated() const;
    void setArrayCreated(bool arrayCreated) const;
    int getArrayXCount() const;
    void setArrayXCount(int arrayXCount) const;
    int getArrayYCount() const;
    void setArrayYCount(int arrayYCount) const;
    double getArraySpacingX() const;
    void setArraySpacingX(double arraySpacing) const;
    double getArraySpacingY() const;
    void setArraySpacingY(double arraySpacing) const;
    double getArrayAngle() const;
    void setArrayAngle(double arrayAngle) const;

    bool isSameAngles() const {
        return m_sameAngles;
    }

    void setSameAngles(const bool val) {
        m_sameAngles = val;
    }

    double getAngle() const;

protected:
    enum Status {
        SetReferencePoint = InitialActionStatus
    };

    std::unique_ptr<RS_Vector> m_referencePoint;
    bool m_invokedWithControl = false;
    bool m_sameAngles = false;

    struct PasteData {
        double angle = 0.0;
        double factor = 1.0;
        bool arrayCreated = false;
        int arrayXCount = 1;
        int arrayYCount = 1;
        RS_Vector arraySpacing = RS_Vector(false);
        double arrayAngle = 0.0;
    };

    std::unique_ptr<PasteData> m_pasteData;

    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void previewMultipleReferencePoints() const;
    void updateActionPrompt() override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector& pos) override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angle) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doTriggerCompletion(bool success) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
