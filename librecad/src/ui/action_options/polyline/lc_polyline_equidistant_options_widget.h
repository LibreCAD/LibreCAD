/****************************************************************************
**
  * Create option widget used to draw equidistant polylines

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

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

#ifndef QG_POLYLINEEQUIDISTANTOPTIONS_H
#define QG_POLYLINEEQUIDISTANTOPTIONS_H


#include "lc_action_options_widget.h"

class LC_ActionDrawLineRelAngle;
class LC_ActionPolylineEquidistant;

namespace Ui {
    class LC_PolylineEquidistantOptionsWidget;
}
/*
  * Create option widget used to draw equidistant polylines
  *
  *@Author Dongxu Li
 */

class LC_PolylineEquidistantOptionsWidget:public LC_ActionOptionsWidget {
    Q_OBJECT
public:
    LC_PolylineEquidistantOptionsWidget();
    ~LC_PolylineEquidistantOptionsWidget() override;
public slots:
    void languageChange() override;
    void onDistEditingFinished();
    void onNumberValueChanged(int number);
protected:
    void doUpdateByAction(RS_ActionInterface *a) override;
private:
    LC_ActionPolylineEquidistant *m_action = nullptr;
    std::unique_ptr<Ui::LC_PolylineEquidistantOptionsWidget> ui;
};
#endif
