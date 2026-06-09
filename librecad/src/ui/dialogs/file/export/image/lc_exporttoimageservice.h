/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_EXPORTTOIMAGESERVICE_H
#define LC_EXPORTTOIMAGESERVICE_H
#include <QObject>

#include "lc_appwindowaware.h"

class LC_AppWindowDialogsInvoker;
class RS_Graphic;

class LC_ExportToImageService:public QObject, public LC_AppWindowAware{
    Q_OBJECT
public:
    LC_ExportToImageService(QC_ApplicationWindow* mainWin, LC_AppWindowDialogsInvoker* dlgHelper)
        : LC_AppWindowAware{mainWin},
          m_dlgHelpr{dlgHelper} {
    }
    bool exportGraphicsToImage(RS_Graphic* graphic, const QString& documentFileName) const;
private:
    LC_AppWindowDialogsInvoker* m_dlgHelpr;
};

#endif
