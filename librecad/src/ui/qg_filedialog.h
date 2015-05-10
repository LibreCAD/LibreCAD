/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
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

#ifndef QG_FILEDIALOG_H
#define QG_FILEDIALOG_H

#include <QFileDialog>
#include "rs.h"

/**
 * File Open / Save dialogs.
 */
class QG_FileDialog : public QFileDialog {

public:
    enum FileType{
        DrawingFile=0,
        BlockFile=1
    };
    /**
*@ FileType, used to set dialog window title, currently, should be either "drawing" or "block"
**/
    QG_FileDialog(QWidget* parent=0, Qt::WindowFlags f=0, FileType type = DrawingFile);
    virtual ~QG_FileDialog();

    QString getOpenFile(RS2::FormatType* type=nullptr);
    QString getSaveFile(RS2::FormatType* type=nullptr);

    static QString getOpenFileName(QWidget* parent, RS2::FormatType* type=nullptr);
    static QString getSaveFileName(QWidget* parent, RS2::FormatType* type=nullptr);

private:
    void getType(const QString filter);
    QString getExtension (RS2::FormatType type) const;
    RS2::FormatType ftype;
    QString fDxfrw2007;
    QString fDxfrw2004;
    QString fDxfrw2000;
    QString fDxfrw14;
    QString fDxfrw12;
    QString fDxfrw;
#ifdef DWGSUPPORT
    QString fDwg;
#endif
    QString fDxf1;
    QString fLff;
    QString fCxf;
    QString fJww;
    QString name;

};

#endif
