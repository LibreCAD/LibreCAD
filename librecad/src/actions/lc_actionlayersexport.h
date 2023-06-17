/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 Melwyn Francis Carlo
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#ifndef LC_ACTIONLAYERSEXPORT_H
#define LC_ACTIONLAYERSEXPORT_H

#if defined(_MSC_VER) && _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "rs_actioninterface.h"


class QString;
class RS_EntityContainer;
class RS_GraphicView;
class RS_LayerList;


/*
    This action class exports the current selected layers as a drawing file, 
    either as individual files, or combined within a single file.
*/


class LC_ActionLayersExport : public RS_ActionInterface
{
    Q_OBJECT

    public:

        enum Mode
        {
            SelectedMode = 0,
            VisibleMode 
        };

        LC_ActionLayersExport( RS_EntityContainer& document, 
                               RS_GraphicView& graphicView, 
                               RS_LayerList* inputLayersList, 
                               Mode inputExportMode);

        void init(int status=0) override;

        void trigger() override;

    private:

        RS_LayerList* layersList = nullptr;

        Mode exportMode = SelectedMode;
};
#endif // LC_ACTIONLAYERSEXPORT_H
