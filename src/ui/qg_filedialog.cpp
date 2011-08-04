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

#include "qg_filedialog.h"

#include <QFileDialog>
#include <QMessageBox>

#include "rs_settings.h"
#include "rs_system.h"


/**
 * Shows a dialog for choosing a file name. Saving the file is up to
 * the caller.
 *
 * @param type Will contain the file type that was chosen by the user.
 *             Can be NULL to be ignored.
 *
 * @return File name with path and extension to determine the file type
 *         or an empty string if the dialog was cancelled.
 */
QString QG_FileDialog::getSaveFileName(QWidget* parent, RS2::FormatType* type) {
    // read default settings:
    RS_SETTINGS->beginGroup("/Paths");
    QString defDir = RS_SETTINGS->readEntry("/Save",
                                              RS_SYSTEM->getHomeDir());
    QString defFilter = RS_SETTINGS->readEntry("/SaveFilter",
                                                 "Drawing Exchange DXF 2000 (*.dxf)");
    //QString defFilter = "Drawing Exchange (*.dxf)";
    RS_SETTINGS->endGroup();

    // prepare file save as dialog:
    QFileDialog* fileDlg = new QFileDialog(parent,"Save as");
    QStringList filters;
    bool done = false;
    bool cancel = false;
    QString fn = "";

    filters.append("Drawing Exchange DXF 2000 (*.dxf)");
    filters.append("Drawing Exchange DXF R12 (*.dxf)");
    filters.append("Font (*.cxf)");
    filters.append("JWW (*.jww)");

    fileDlg->setFilters(filters);
    fileDlg->setFileMode(QFileDialog::AnyFile);
    fileDlg->setWindowTitle(QObject::tr("Save Drawing As"));
    fileDlg->setDirectory(defDir);
    fileDlg->setAcceptMode(QFileDialog::AcceptSave);
    fileDlg->selectFilter(defFilter);

    // run dialog:
    do {
        // accepted:
        if (fileDlg->exec()==QDialog::Accepted) {
            QStringList fl = fileDlg->selectedFiles();
            if (!fl.isEmpty())
                fn = fl[0];
            fn = QDir::convertSeparators( QFileInfo(fn).absoluteFilePath() );
            cancel = false;

            // append default extension:
            // TODO, since we are starting to suppor tmore extensions, we need to find a better way to add the default
            if (QFileInfo(fn).fileName().indexOf('.')==-1) {
                if (fileDlg->selectedFilter()=="Font (*.cxf)") {
                    fn+=".cxf";
                } else {
                    fn+=".dxf";
                }
            }

            // set format:
            if (type!=NULL) {
                if (fileDlg->selectedFilter()=="Font (*.cxf)") {
                    *type = RS2::FormatCXF;
                } else if (fileDlg->selectedFilter()=="Drawing Exchange DXF R12 (*.dxf)") {
                    *type = RS2::FormatDXF12;
                } else if (fileDlg->selectedFilter()=="JWW (*.jww)") {
                    *type = RS2::FormatJWW;
                } else {
                    *type = RS2::FormatDXF;
                }
            }

#if !defined (_WIN32) && !defined (__APPLE__)
            // overwrite warning:
            if(QFileInfo(fn).exists()) {
                int choice =
                        QMessageBox::warning(parent, QObject::tr("Save Drawing As"),
                                             QObject::tr("%1 already exists.\n"
                                                         "Do you want to replace it?")
                                             .arg(fn),
                                             QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,QMessageBox::Cancel);

                switch (choice) {
                case QMessageBox::Yes:
                    done = true;
                    break;
                default:
                    done = false;
                    break;
                }
            } else {
                done = true;
            }
#else
            done = true;
#endif


        } else {
            done = true;
            cancel = true;
            fn = "";
        }
    } while(!done);

    // store new default settings:
    if (!cancel) {
        RS_SETTINGS->beginGroup("/Paths");
        RS_SETTINGS->writeEntry("/Save", QFileInfo(fn).absolutePath());
        //RS_SETTINGS->writeEntry("/SaveFilter", fileDlg->selectedFilter());
        RS_SETTINGS->endGroup();
    }

    delete fileDlg;

    return fn;
}



/**
 * Shows a dialog for choosing a file name. Opening the file is up to
 * the caller.
 *
 * @return File name with path and extension to determine the file type
 *         or an empty string if the dialog was cancelled.
 */
QString QG_FileDialog::getOpenFileName(QWidget* parent, RS2::FormatType* type) {
    RS_DEBUG->print("QG_FileDialog::getOpenFileName");

    // read default settings:
    RS_SETTINGS->beginGroup("/Paths");
    QString defDir = RS_SETTINGS->readEntry("/Open",
                                              RS_SYSTEM->getHomeDir());
    //QString defFilter = RS_SETTINGS->readEntry("/OpenFilter",
    //                      "Drawing Exchange (*.dxf *.DXF)");
    QString defFilter = "Drawing Exchange (*.dxf *.DXF)";
    RS_SETTINGS->endGroup();

    RS_DEBUG->print("defDir: %s", defDir.toLatin1().data());
    RS_DEBUG->print("defFilter: %s", defFilter.toLatin1().data());

    QString fDxf(QObject::tr("Drawing Exchange %1").arg("(*.dxf *.DXF)"));
    QString fDxf1(QObject::tr("QCad 1.x file %1").arg("(*.dxf *.DXF)"));
    QString fCxf(QObject::tr("Font %1").arg("(*.cxf)"));
    QString fJww(QObject::tr("Jww %1").arg("(*.jww)"));

    RS_DEBUG->print("fDxf: %s", fDxf.toLatin1().data());
    RS_DEBUG->print("fDxf1: %s", fDxf1.toLatin1().data());
    RS_DEBUG->print("fCxf: %s", fCxf.toLatin1().data());
    RS_DEBUG->print("fJww: %s", fJww.toLatin1().data());

    QString fn = "";
    bool cancel = false;

    QFileDialog* fileDlg = new QFileDialog(parent, "File Dialog");

    QStringList filters;
    filters.append(fDxf);
    filters.append(fDxf1);
    filters.append(fCxf);
    filters.append(fJww);

    fileDlg->setFilters(filters);
    fileDlg->setFileMode(QFileDialog::ExistingFile);
    fileDlg->setWindowTitle(QObject::tr("Open Drawing"));
    fileDlg->setDirectory(defDir);
    fileDlg->selectFilter(defFilter);

    /** preview RVT PORT preview is currently not supported by QT4
    RS_Graphic* gr = new RS_Graphic;
    QG_GraphicView* prev = new QG_GraphicView(parent);
    prev->setContainer(gr);
    prev->setBorders(1, 1, 1, 1);
    fileDlg->setContentsPreviewEnabled(true);
    fileDlg->setContentsPreview(prev, prev);
    */

    if (fileDlg->exec()==QDialog::Accepted) {
        QStringList fl = fileDlg->selectedFiles();
        if (!fl.isEmpty())
            fn = fl[0];
        fn = QDir::convertSeparators( QFileInfo(fn).absoluteFilePath() );
        if (type!=NULL) {
            if (fileDlg->selectedFilter()==fDxf1) {
                *type = RS2::FormatDXF1;
            } else if (fileDlg->selectedFilter()==fDxf) {
                *type = RS2::FormatDXF;
            } else if (fileDlg->selectedFilter()==fCxf) {
                *type = RS2::FormatCXF;
            } else if (fileDlg->selectedFilter()==fJww) {
                *type = RS2::FormatJWW;
            }
        }
        cancel = false;
    } else {
        cancel = true;
    }

    // store new default settings:
    if (!cancel) {
        RS_SETTINGS->beginGroup("/Paths");
        RS_SETTINGS->writeEntry("/Open", QFileInfo(fn).absolutePath());
        RS_SETTINGS->writeEntry("/OpenFilter", fileDlg->selectedFilter());
        RS_SETTINGS->endGroup();
    }

    RS_DEBUG->print("QG_FileDialog::getOpenFileName: fileName: %s", fn.toLatin1().data());
    RS_DEBUG->print("QG_FileDialog::getOpenFileName: OK");

    // RVT PORT delete prev;
    // RVT PORT delete gr;
    delete fileDlg;

    return fn;
}

// EOF

