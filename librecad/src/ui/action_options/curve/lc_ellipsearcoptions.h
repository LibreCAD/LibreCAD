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

#ifndef LC_ELLIPSEARCOPTIONS_H
#define LC_ELLIPSEARCOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidgetbase.h"
#include "rs_actiondrawellipseaxis.h"

namespace Ui {
    class LC_EllipseArcOptions;
}

class LC_EllipseArcOptions : public LC_ActionOptionsWidgetBase{
Q_OBJECT

public:
    LC_EllipseArcOptions();
    virtual ~LC_EllipseArcOptions() override;

public slots:
    void onDirectionChanged(bool);
    void languageChange() override;
protected:
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
protected:
    RS_ActionDrawEllipseAxis* action = nullptr;
    std::unique_ptr<Ui::LC_EllipseArcOptions> ui;
    void setReversedToActionAndView(bool reversed);
};

#endif // LC_ELLIPSEARCOPTIONS_H
