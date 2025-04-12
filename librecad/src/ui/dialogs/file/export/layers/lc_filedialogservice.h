// /****************************************************************************
//
// Utility base class for widgets that represents options for actions
//
// Copyright (C) 2025 LibreCAD.org
// Copyright (C) 2025 sand1024
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
// **********************************************************************
//

#ifndef LC_FILEDIALOGSERVICE_H
#define LC_FILEDIALOGSERVICE_H


#if defined(_MSC_VER) && _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <QString>
#include "rs.h"

struct LC_LayersExportOptions;
/*
    This service class centralizes the file I/O user interface.
*/
namespace LC_FileDialogService {
    enum FileDialogMode {
        SaveDrawing = 0,
        ExportLayersSelected = 1,
        ExportLayersVisible = 2
    };

    struct FileDialogResult {
        QString dirPath;
        QString filePath;
        QString fileName;
        QString fileExtension;
        RS2::FormatType fileType = RS2::FormatUnknown;
        int checkState = 0;
    };
    FileDialogResult getFileDetails(FileDialogMode const &fileDialogMode);
}
#endif // LC_FILEDIALOGSERVICE_H
