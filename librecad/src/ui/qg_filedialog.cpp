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

#include <QMessageBox>

#include "rs_settings.h"
#include "rs_system.h"

#if QT_VERSION < 0x040400
#include "emu_qt44.h"
#endif

//#define USEQTDIALOG 1

void QG_FileDialog::getType(const QString filter) {
    if (filter== fLff) {
        ftype = RS2::FormatLFF;
    } else if (filter == fCxf) {
        ftype = RS2::FormatCXF;
    } else if (filter == fDxfR12) {
        ftype = RS2::FormatDXF12;
    } else if (filter == fDxfrw2000 || filter == fDxfrw) {
        ftype = RS2::FormatDXFRW;
    } else if (filter == fJww) {
        ftype = RS2::FormatJWW;
    } else if (filter == fDxf1) {
        ftype = RS2::FormatDXF1;
    } else if (filter == fDxfR12) {
        ftype = RS2::FormatDXF12;
    } else  if (filter == fDxf2000 || filter == fDxf){
        ftype = RS2::FormatDXF;
    }
}

QG_FileDialog::QG_FileDialog(QWidget* parent, Qt::WindowFlags f, FileType type)
                            :QFileDialog(parent, f)
{
#if QT_VERSION >= 0x040500
#ifdef USEQTDIALOG
    setOption ( QFileDialog::DontUseNativeDialog, true );
#else
    setOption ( QFileDialog::DontUseNativeDialog, false );
#endif
    setOption ( QFileDialog::HideNameFilterDetails, false );
#endif // QT_VERSION
    ftype= RS2::FormatDXF;
    fDxf2000 = tr("Drawing Exchange DXF 2000 %1").arg("(*.dxf)");
#ifdef USE_DXFRW
    fDxfrw2000 = tr("New Drawing Exchange DXF 2000 %1").arg("(*.dxf)");
    fDxfrw = tr("New Drawing Exchange %1").arg("(*.dxf)");
#endif
    fDxfR12 = tr("Drawing Exchange DXF R12 %1").arg("(*.dxf)");
    fLff = tr("LFF Font %1").arg("(*.lff)");
    fCxf = tr("QCad Font %1").arg("(*.cxf)");
    fJww = tr("Jww Drawing %1").arg("(*.jww)");
    fDxf = tr("Drawing Exchange %1").arg("(*.dxf)");
    fDxf1 = tr("QCad 1.x file %1").arg("(*.dxf)");
    switch(type){
    case BlockFile:
        name=tr("Block", "block file");
        break;
    default:
        name=tr("Drawing", "drawing file");
    }
}

QG_FileDialog::~QG_FileDialog(){
}

QString QG_FileDialog::getExtension (RS2::FormatType type){
    switch (type) {
    case RS2::FormatLFF:
        return QString(".lff");
    case RS2::FormatJWW:
        return QString(".jww");
    case RS2::FormatCXF:
        return QString(".cxf");
    default:
        return QString(".dxf");
    }
}

QString QG_FileDialog::getOpenFile(RS2::FormatType* type){
//    bool fileAccepted = false;
    setAcceptMode ( QFileDialog::AcceptOpen );
    // read default settings:
    RS_SETTINGS->beginGroup("/Paths");
    QString defDir = RS_SETTINGS->readEntry("/Open",
                                              RS_SYSTEM->getHomeDir());
    RS_SETTINGS->endGroup();

    RS_DEBUG->print("defDir: %s", defDir.toLatin1().data());
    QString fn = "";
    QStringList filters;
#ifdef USE_DXFRW
    filters << fDxf << fDxfrw  << fDxf1 << fLff << fCxf << fJww;
#else
    filters << fDxf << fDxf1 << fLff << fCxf << fJww;
#endif

    setWindowTitle(tr("Open %1").arg(name));
#if QT_VERSION >= 0x040400
    setNameFilters(filters);
#endif
    setDirectory(defDir);
    setFileMode(QFileDialog::ExistingFile);
#if QT_VERSION >= 0x040400
    selectNameFilter(fDxfrw);
#endif
    ftype= RS2::FormatDXFRW;
    RS_DEBUG->print("defFilter: %s", fDxfrw.toLatin1().data());

    /* preview RVT PORT preview is currently not supported by QT4
    RS_Graphic* gr = new RS_Graphic;
    QG_GraphicView* prev = new QG_GraphicView(parent);
    prev->setContainer(gr);
    prev->setBorders(1, 1, 1, 1);
    fileDlg->setContentsPreviewEnabled(true);
    fileDlg->setContentsPreview(prev, prev); */

    if (exec()==QDialog::Accepted) {
        QStringList fl = selectedFiles();
        if (!fl.isEmpty()) {
            fn = fl[0];
        }
        fn = QDir::convertSeparators( QFileInfo(fn).absoluteFilePath() );

        if (type!=NULL) {
                getType(selectedFilter());
                *type = ftype;
        }

    // store new default settings:
        RS_SETTINGS->beginGroup("/Paths");
        RS_SETTINGS->writeEntry("/Open", QFileInfo(fn).absolutePath());
        RS_SETTINGS->writeEntry("/OpenFilter", selectedFilter());
        RS_SETTINGS->endGroup();
    }

    RS_DEBUG->print("QG_FileDialog::getOpenFileName: fileName: %s", fn.toLatin1().data());
    RS_DEBUG->print("QG_FileDialog::getOpenFileName: OK");

    // RVT PORT delete prev;
    // RVT PORT delete gr;
    return fn;
}

QString QG_FileDialog::getSaveFile(RS2::FormatType* type){
    setAcceptMode ( QFileDialog::AcceptSave );
    // read default settings:
    RS_SETTINGS->beginGroup("/Paths");
    QString defDir = RS_SETTINGS->readEntry("/Save",
                                              RS_SYSTEM->getHomeDir());
/*    QString defFilter = RS_SETTINGS->readEntry("/SaveFilter",
                                                 "Drawing Exchange DXF 2000 (*.dxf)");*/
    RS_SETTINGS->endGroup();

    if(!defDir.endsWith("/") && !defDir.endsWith("\\"))
        defDir += QDir::separator();

    RS_DEBUG->print("defDir: %s", defDir.toLatin1().data());

    // setup filters
    QStringList filters;

#ifdef USE_DXFRW
    filters << fDxf2000 << fDxfrw2000  << fDxfR12 << fLff << fCxf << fJww;
#else
    filters << fDxf2000 << fDxfR12 << fLff << fCxf << fJww;
#endif

    ftype = RS2::FormatDXF;
    RS_DEBUG->print("defFilter: %s", fDxf2000.toLatin1().data());

    if (type!=NULL)
        *type = ftype;

    // when defFilter is added the below should use the default extension.
    // generate an untitled name
    QString fn = "Untitled";
    if(QFile::exists( defDir + fn + getExtension( ftype ) ))
    {
        int fileCount = 1;
        while(QFile::exists( defDir + fn + QString("%1").arg(fileCount) +
                             getExtension(ftype)) )
            ++fileCount;
        fn += QString("%1").arg(fileCount);
    }

    // initialize dialog properties
    setWindowTitle(tr("Save %1 As").arg(name));
    setFileMode(QFileDialog::AnyFile);
    setDirectory(defDir);
    setFilters(filters);
#if QT_VERSION >= 0x040400
    selectNameFilter(fDxf2000);
#endif
    selectFile(fn);


    // only return non empty string when we have a complete, user approved, file name.
    if (exec()!=QDialog::Accepted)
        return QString("");

    QStringList fl = selectedFiles();
    if (fl.isEmpty())
        return QString("");

    QFileInfo fi = QFileInfo( fl[0] );
    fn = QDir::convertSeparators( fi.absoluteFilePath() );

    // append default extension:
    if (fi.fileName().indexOf('.')==-1)
        fn += getExtension(ftype);

    // store new default settings:
    RS_SETTINGS->beginGroup("/Paths");
    RS_SETTINGS->writeEntry("/Save", fi.absolutePath());
    //RS_SETTINGS->writeEntry("/SaveFilter", fileDlg->selectedFilter());
    RS_SETTINGS->endGroup();

    return fn;
}


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
#ifdef USE_DXFRW
    filters.append("New Drawing Exchange DXF 2000 (*.DXF)");
#endif
    filters.append("Drawing Exchange DXF R12 (*.dxf)");
    filters.append("LFF Font (*.lff)");
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
                if (fileDlg->selectedFilter()=="LFF Font (*.lff)") {
                    fn+=".lff";
                } else if (fileDlg->selectedFilter()=="Font (*.cxf)") {
                        fn+=".cxf";
                } else {
                    fn+=".dxf";
                }
            }

            // set format:
            if (type!=NULL) {
                if (fileDlg->selectedFilter()=="LFF Font (*.lff)") {
                    *type = RS2::FormatLFF;
                } else if (fileDlg->selectedFilter()=="Font (*.cxf)") {
                    *type = RS2::FormatCXF;
                } else if (fileDlg->selectedFilter()=="Drawing Exchange DXF R12 (*.dxf)") {
                    *type = RS2::FormatDXF12;
#ifdef USE_DXFRW
#if QT_VERSION >= 0x040400
                } else if (fileDlg->selectedNameFilter()=="New Drawing Exchange DXF 2000 (*.DXF)") {
                    *type = RS2::FormatDXFRW;
#endif // QT_VERSION
#endif // USE_DXFFRW
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
#ifdef USE_DXFRW
    QString fDxfrw(QObject::tr("New Drawing Exchange %1").arg("(*.dxf)"));
#endif
    QString fDxf1(QObject::tr("QCad 1.x file %1").arg("(*.dxf *.DXF)"));
    QString fLff(QObject::tr("LFF Font %1").arg("(*.lff)"));
    QString fCxf(QObject::tr("Font %1").arg("(*.cxf)"));
    QString fJww(QObject::tr("Jww %1").arg("(*.jww)"));

    RS_DEBUG->print("fDxf: %s", fDxf.toLatin1().data());
#ifdef USE_DXFRW
    RS_DEBUG->print("fDxfrw: %s", fDxfrw.toLatin1().data());
#endif
    RS_DEBUG->print("fDxf1: %s", fDxf1.toLatin1().data());
    RS_DEBUG->print("fCxf: %s", fCxf.toLatin1().data());
    RS_DEBUG->print("fJww: %s", fJww.toLatin1().data());

    QString fn = "";
    bool cancel = false;

    QFileDialog* fileDlg = new QFileDialog(parent, "File Dialog");

    QStringList filters;
    filters.append(fDxf);
#ifdef USE_DXFRW
    filters.append(fDxfrw);
#endif
    filters.append(fDxf1);
    filters.append(fLff);
    filters.append(fCxf);
    filters.append(fJww);

#if QT_VERSION >= 0x040400
    fileDlg->setNameFilters(filters);
#else
    emu_qt44_QFileDialog_setNameFilters(*fileDlg, filters);
#endif
    fileDlg->setFileMode(QFileDialog::ExistingFile);
    fileDlg->setWindowTitle(QObject::tr("Open Drawing"));
    fileDlg->setDirectory(defDir);
#if QT_VERSION >= 0x040400
    fileDlg->selectNameFilter(defFilter);
#endif

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
#if QT_VERSION >= 0x040400
        if (type!=NULL) {
            if (fileDlg->selectedNameFilter()==fDxf1) {
                *type = RS2::FormatDXF1;
            } else if (fileDlg->selectedNameFilter()==fDxf) {
                *type = RS2::FormatDXF;
#ifdef USE_DXFRW
            } else if (fileDlg->selectedNameFilter()==fDxfrw) {
                *type = RS2::FormatDXFRW;
#endif
            } else if (fileDlg->selectedNameFilter()==fCxf) {
                *type = RS2::FormatCXF;
            } else if (fileDlg->selectedNameFilter()==fJww) {
                *type = RS2::FormatJWW;
            }
        }
#endif
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

