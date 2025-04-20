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

#ifndef LC_ACTIONPASTETOPOINTS_H
#define LC_ACTIONPASTETOPOINTS_H

#include "lc_actionpreselectionawarebase.h"

class LC_ActionPasteToPoints: public LC_ActionPreSelectionAwareBase{
    Q_OBJECT
public:
    LC_ActionPasteToPoints(LC_ActionContext *actionContext);
    double getAngle() const;
    void setAngle(double angle);
    double getScaleFactor() const;
    void setScaleFactor(double scaleFactor);
    bool isRemovePointAfterPaste() const;
    void setRemovePointAfterPaste(bool removePointAfterPaste);
    void init(int status) override;
protected:
    LC_ActionOptionsWidget *createOptionsWidget() override;
    bool isAllowTriggerOnEmptySelection() override;
    bool isEntityAllowedToSelect(RS_Entity *ent) const override;
    void updateMouseButtonHintsForSelection() override;
    void doTrigger(bool selected) override;

    double m_angle = 0.0;
    double m_scaleFactor = 1.0;
    bool m_removePointAfterPaste = false;
};

#endif // LC_ACTIONPASTETOPOINTS_H
