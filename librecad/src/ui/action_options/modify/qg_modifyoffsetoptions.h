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

#ifndef QG_MODIFYOFFSETOPTIONS_H
#define QG_MODIFYOFFSETOPTIONS_H


#include "lc_actionoptionswidgetbase.h"

class RS_ActionInterface;
class RS_ActionModifyOffset;

namespace Ui {
class Ui_ModifyOffsetOptions;
}

class QG_ModifyOffsetOptions : public LC_ActionOptionsWidgetBase{
Q_OBJECT
public:
    QG_ModifyOffsetOptions();
    ~QG_ModifyOffsetOptions() override;
protected:
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void doSaveSettings() override;
    void setDistanceToActionAndView(QString qString);
    void setDistanceFixedToActionAndView(bool val);
protected slots:
    void languageChange() override;
    void onDistEditingFinished();
    void onFixedDistanceClicked(bool val);
    void cbKeepOriginalsClicked(bool val);
    void cbMultipleCopiesClicked(bool val);
    void cbUseCurrentAttributesClicked(bool val);
    void cbUseCurrentLayerClicked(bool val);
    void onNumberOfCopiesValueChanged(int number);
private:
    std::unique_ptr<Ui::Ui_ModifyOffsetOptions> ui;
    RS_ActionModifyOffset* action = nullptr;
    void setCopiesNumberToActionAndView(int number);
    void setUseMultipleCopiesToActionAndView(bool copies);
    void setUseCurrentLayerToActionAndView(bool val);
    void setUseCurrentAttributesToActionAndView(bool val);
    void setKeepOriginalsToActionAndView(bool val);
};

#endif
