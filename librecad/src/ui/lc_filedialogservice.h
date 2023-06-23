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

#ifndef LC_FILEDIALOGSERVICE_H
#define LC_FILEDIALOGSERVICE_H


#if defined(_MSC_VER) && _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "rs.h"

class QString;

/*
    This service class centralizes the file I/O user interface.
*/
namespace LC_FileDialogService
{
        enum FileDialogMode
        {
            /* List of open modes. */


            /* List of save modes.*/
            SaveDrawing = 0,
            ExportLayersSelected = 1,
            ExportLayersVisible = 2
        };

        struct FileDialogResult
        {
            QString dirPath;
            QString filePath;
            QString fileName;
            QString fileExtension;
            RS2::FormatType fileType = RS2::FormatUnknown;
            int checkState = 0;
        };

        FileDialogResult getFileDetails(FileDialogMode const& fileDialogMode);
}
#endif // LC_FILEDIALOGSERVICE_H
