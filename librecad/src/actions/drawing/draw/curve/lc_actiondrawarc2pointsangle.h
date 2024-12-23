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

#ifndef LC_ACTIONDRAWARC2POINTSANGLE_H
#define LC_ACTIONDRAWARC2POINTSANGLE_H

#include "lc_actiondrawarc2pointsbase.h"

class LC_ActionDrawArc2PointsAngle:public LC_ActionDrawArc2PointsBase{
    Q_OBJECT
public:
    LC_ActionDrawArc2PointsAngle(RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~LC_ActionDrawArc2PointsAngle() override = default;
protected:
    bool createArcData(RS_ArcData &data, int status, RS_Vector vector, bool alternate, bool reportErrors) override;
    void doPreviewOnPoint2Custom(RS_Arc *pArc) override;
    void setParameterValue(double r) override;
    QString getParameterCommand() override;
    QString getParameterPromptValue() const override;
    QString getAlternativePoint2Prompt() const override;
};

#endif // LC_ACTIONDRAWARC2POINTSANGLE_H
