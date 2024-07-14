/****************************************************************************
**
* Options widget for "LineJoin" action.

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
**********************************************************************/
#ifndef LC_LINEJOINOPTIONS_H
#define LC_LINEJOINOPTIONS_H

#include "lc_actionmodifylinejoin.h"
#include "lc_actionoptionswidgetbase.h"

namespace Ui {
class LC_LineJoinOptions;
}

class LC_LineJoinOptions : public LC_ActionOptionsWidgetBase {
Q_OBJECT

public:
    explicit LC_LineJoinOptions();
    ~LC_LineJoinOptions() override;

protected slots:
    void languageChange() override;
    void doSetAction( RS_ActionInterface * a, bool update) override;
    void onUsePolylineClicked(bool value);
    void onAttributesSourceIndexChanged(int index);
    void onEdgeModelLine1IndexChanged(int index);
    void onEdgeModelLine2IndexChanged(int index);
    void onRemoveOriginalsClicked(bool value);
protected:
    void doSaveSettings() override;
private:
    Ui::LC_LineJoinOptions *ui;
    LC_ActionModifyLineJoin *action;

    void setEdgeModeLine1ToActionAndView(int index);
    void setEdgeModeLine2ToActionAndView(int index);
    void setUsePolylineToActionAndView(bool value);
    void setAttributesSourceToActionAndView(int index);
    void setRemoveOriginalsToActionAndView(bool value);
};

#endif // LC_LINEJOINOPTIONS_H
