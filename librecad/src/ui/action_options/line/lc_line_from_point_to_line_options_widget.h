/****************************************************************************
**
* Options widget for "LineFromPointToLine" action.

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
#ifndef LC_LINEFROMPOINTTOLINEOPTIONS_H
#define LC_LINEFROMPOINTTOLINEOPTIONS_H

#include "lc_action_options_widget.h"

class LC_ActionDrawLineFromPointToLine;

namespace Ui {
    class LC_LineFromPointToLineOptionsWidget;
}
/**
 * UI options for LC_ActionDrawLineFromPointToLine
 */
class LC_LineFromPointToLineOptionsWidget :public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    explicit LC_LineFromPointToLineOptionsWidget();
    ~LC_LineFromPointToLineOptionsWidget() override;
protected slots:
    void languageChange() override;
    void onAngleEditingFinished();
    void onLengthEditingFinished();
    void onEndOffsetEditingFinished();
    void onSnapModeIndexChanged(int index) const;
    void onSizeModeIndexChanged(int index) const;
    void onOrthogonalClicked(bool value) const;
protected:
    void doUpdateByAction(RS_ActionInterface *a) override;
private:
    LC_ActionDrawLineFromPointToLine* m_action = nullptr;
    Ui::LC_LineFromPointToLineOptionsWidget *ui;
};

#endif
