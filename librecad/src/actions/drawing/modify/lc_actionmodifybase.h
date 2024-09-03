/****************************************************************************
**
* Abstract base class form modification actions

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

#ifndef LC_ACTIONMODIFYBASE_H
#define LC_ACTIONMODIFYBASE_H

#include "lc_actionpreselectionawarebase.h"
#include "rs_modification.h"

class LC_ActionModifyBase:public LC_ActionPreSelectionAwareBase{

public:
    LC_ActionModifyBase(
        const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView,
        const QList<RS2::EntityType> &entityTypeList = {}, bool countSelectionDeep = false);

    void setUseCurrentLayer(bool b);
    bool isUseCurrentLayer();
    void setUseCurrentAttributes(bool b);
    bool isUseCurrentAttributes();
    int getCopiesNumber();
    void setCopiesNumber(int value);
    bool isUseMultipleCopies();
    void setUseMultipleCopies(bool val);
    bool isKeepOriginals();
    void setKeepOriginals(bool b);
protected:
    virtual bool isShowModifyActionDialog();
    void selectionCompleted(bool singleEntity, bool fromInit) override;
    virtual LC_ModifyOperationFlags* getModifyOperationFlags()=0;
};

#endif // LC_ACTIONMODIFYBASE_H
