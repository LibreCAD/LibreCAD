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

#include "qc_mdiwindow.h"

#include <qprinter.h>
//Added by qt3to4:
#include <QCloseEvent>

#include "rs_graphic.h"
#include "qg_exitdialog.h"
#include "qg_filedialog.h"
#include "rs_insert.h"
#include "rs_text.h"

#include <QMainWindow>
#include <qcursor.h>
#include <qpainter.h>
#include <QMessageBox>
#include <QPrintDialog>
#include <QFileInfo>

int QC_MDIWindow::idCounter = 0;

/**
 * Constructor.
 *
 * @param doc Pointer to an existing document of NULL if a new
 *   document shall be created for this window.
 * @param parent Parent widget. Usually a workspace.
 */
QC_MDIWindow::QC_MDIWindow(RS_Document* doc,
                           QWidget* parent, Qt::WindowFlags wflags)
        : QMainWindow(parent, wflags) {

    setAttribute(Qt::WA_DeleteOnClose);
    owner = false;
        forceClosing = false;
    initDoc(doc);
    initView();
    id = idCounter++;
    //childWindows.setAutoDelete(false);
    parentWindow = NULL;

    if (document!=NULL) {
        if (document->getLayerList()!=NULL) {
            // Link the graphic view to the layer widget
            document->getLayerList()->addListener(graphicView);
        }
        if (document->getBlockList()!=NULL) {
            // Link the graphic view to the block widget
            document->getBlockList()->addListener(graphicView);
        }
    }
//    setFocusPolicy(Qt::ClickFocus);
    showMaximized();
}



/**
 * Destructor.
 *
 * Deletes the document associated with this window.
 */
QC_MDIWindow::~QC_MDIWindow() {
    if (document->getLayerList()!=NULL) {
        document->getLayerList()->removeListener(graphicView);
    }

    if (document->getBlockList()!=NULL) {
        document->getBlockList()->removeListener(graphicView);
    }

    if (owner==true && document!=NULL) {
        delete document;
    }
    document = NULL;
}



/**
 * Adds another MDI window to the list of known windows that
 * depend on this one. This can be another view or a view for
 * a particular block.
 */
void QC_MDIWindow::addChildWindow(QC_MDIWindow* w) {
    RS_DEBUG->print("RS_MDIWindow::addChildWindow()");

    childWindows.append(w);
    w->setParentWindow(this);

    RS_DEBUG->print("children: %d", childWindows.count());
}



/**
 * Removes a child window.
 *
 * @see addChildWindow
 */
void QC_MDIWindow::removeChildWindow(QC_MDIWindow* w) {
//    RS_DEBUG->print("RS_MDIWindoqapplication.h>w::removeChildWindow()");
    if(childWindows.size()>0 ){
        if(childWindows.contains(w)){
            childWindows.removeAll(w);
//            suc=true;
        }
    }

//    bool suc = childWindows.removeAll(w);
//    RS_DEBUG->print("successfully removed child window: %d", (int)suc);

//    RS_DEBUG->print("children: %d", childWindows.count());

}



/**
 * @return pointer to the print preview of this drawing or NULL.
 */
QC_MDIWindow* QC_MDIWindow::getPrintPreview() {
        while (!childWindows.isEmpty()) {
                QC_MDIWindow *tmp=childWindows.takeFirst();
        if (tmp->getGraphicView()->isPrintPreview()) {
                        return tmp;
                }
        }

        return NULL;
}



/**
 * closes this MDI window.
 *
 * @param force Disable cancel button (demo versions)
 * @param ask Ask user before closing.
 */
bool QC_MDIWindow::closeMDI(bool force, bool ask) {
    // should never happen:
    if (document==NULL) {
        return true;
    }

    bool ret = false;

    bool isBlock = (parentWindow!=NULL);

    // This is a block and we don't need to ask the user for closing
    //   since it's still available in the parent drawing after closing.
    if (isBlock) {
        RS_DEBUG->print("  closing block");
        // tell parent window we're not here anymore.
        if (parentWindow!=NULL) {
            RS_DEBUG->print("    notifying parent about closing this window");
            parentWindow->removeChildWindow(this);
        }
        emit(signalClosing());
        ret = true;
    }

    // This is a graphic document. ask user for closing.
    else if (!ask || slotFileClose(force)) {
        RS_DEBUG->print("  closing graphic");
        // close all child windows:
                while (!childWindows.isEmpty()) {
                        QC_MDIWindow *tmp=childWindows.takeFirst();
                        tmp->close();
                }

        emit(signalClosing());

        ret = true;
    }

    // User decided not to close graphic document:
    else {
        ret = false;
    }

    return (ret || force);
}



/**
 * Called by Qt when the user closes this MDI window.
 */
void QC_MDIWindow::closeEvent(QCloseEvent* ce) {

    RS_DEBUG->print("QC_MDIWindow::closeEvent begin");

    if (closeMDI(false, forceClosing)) {
        ce->accept();
    } else {
        ce->ignore();
    }

    RS_DEBUG->print("QC_MDIWindow::closeEvent end");
}



/**
 * Init the document.
 *
 * @param type Document type. RS:EntityGraphic or RS2::EntityBlock
 * @param container Entity container to be used as document or NULL
 * if a new document should be created.
 */
void QC_MDIWindow::initDoc(RS_Document* doc) {

    RS_DEBUG->print("QC_MDIWindow::initDoc()");

    if (doc==NULL) {
        document = new RS_Graphic();
        document->newDoc();
        owner = true;
    } else {
        document = doc;
        owner = false;
    }

}



/**
 * Init the view.
 */
void QC_MDIWindow::initView() {
    RS_DEBUG->print("QC_MDIWindow::initView()");

    graphicView = new QC_GraphicView(document, this);
    setCentralWidget(graphicView);
//    graphicView->setFocus();
}



/**
 * Called when the current pen (color, style, width) has changed.
 * Sets the active pen for the document in this MDI window.
 */
void QC_MDIWindow::slotPenChanged(RS_Pen pen) {
    RS_DEBUG->print("QC_MDIWindow::slotPenChanged() begin");
    if (document!=NULL) {
        document->setActivePen(pen);
    }
    RS_DEBUG->print("QC_MDIWindow::slotPenChanged() end");
}



/**
 * Creates a new empty document in this MDI window.
 */
void QC_MDIWindow::slotFileNew() {
    RS_DEBUG->print("QC_MDIWindow::slotFileNew begin");
    if (document!=NULL && graphicView!=NULL) {
        document->newDoc();
        graphicView->redraw();
    }
    RS_DEBUG->print("QC_MDIWindow::slotFileNew end");
}



/**
 * Opens the given file in this MDI window.
 */
bool QC_MDIWindow::slotFileOpen(const QString& fileName, RS2::FormatType type) {

    RS_DEBUG->print("QC_MDIWindow::slotFileOpen");
    bool ret = false;

    if (document!=NULL && !fileName.isEmpty()) {
        document->newDoc();

                // cosmetics..
                // RVT_PORT qApp->processEvents(1000);
                qApp->processEvents(QEventLoop::AllEvents, 1000);

        ret = document->open(fileName, type);

        if (ret) {
            //QString message=tr("Loaded document: ")+fileName;
            //statusBar()->showMessage(message, 2000);

            if (fileName.endsWith(".lff") || fileName.endsWith(".cxf"))
                drawChars();

            RS_DEBUG->print("QC_MDIWindow::slotFileOpen: autoZoom");
            graphicView->zoomAuto(false);
            RS_DEBUG->print("QC_MDIWindow::slotFileOpen: autoZoom: OK");
        } else {
            RS_DEBUG->print("QC_MDIWindow::slotFileOpen: failed");
        }
    } else {
        RS_DEBUG->print("QC_MDIWindow::slotFileOpen: cancelled");
        //statusBar()->showMessage(tr("Opening aborted"), 2000);
    }

    RS_DEBUG->print("QC_MDIWindow::slotFileOpen: OK");

    return ret;
}
void QC_MDIWindow::zoomAuto() {
    if(graphicView!=NULL){
        if(graphicView->isPrintPreview()){
            graphicView->zoomPage();
        }else{
            graphicView->zoomAuto();
        }
    }
}
void QC_MDIWindow::drawChars() {

    RS_BlockList* bl = document->getBlockList();
    double sep = document->getGraphic()->getVariableDouble("LetterSpacing", 3.0);
    double h = sep/3;
    sep = sep*3;
    for (int i=0; i<bl->count(); ++i) {
        RS_Block* ch = bl->at(i);
        RS_InsertData data(ch->getName(), RS_Vector(i*sep,0), RS_Vector(1,1), 0, 1, 1, RS_Vector(0,0));
        RS_Insert* in = new RS_Insert(document, data);
        document->addEntity(in);
        QFileInfo info(document->getFilename() );
        QString uCode = (ch->getName()).mid(1,4);
        RS_TextData datatx(RS_Vector(i*sep,-h), h, 4*h, RS2::VAlignTop,
                           RS2::HAlignLeft, RS2::ByStyle, RS2::AtLeast,
                           1, uCode, "standard", 0);
/*        RS_TextData datatx(RS_Vector(i*sep,-h), h, 4*h, RS2::VAlignTop,
                           RS2::HAlignLeft, RS2::ByStyle, RS2::AtLeast,
                           1, uCode, info.baseName(), 0);*/
        RS_Text *tx = new RS_Text(document, datatx);
        document->addEntity(tx);
    }

}


/**
 * Saves the current file.
 *
 * @param  isAutoSave true if this is an "autosave" operation.
 *                    false if this is "Save" operation requested
 *                    by the user.
 * @return true if the file was saved successfully.
 *         false if the file could not be saved or the document
 *         is invalid.
 */
bool QC_MDIWindow::slotFileSave(bool &cancelled, bool isAutoSave) {
    RS_DEBUG->print("QC_MDIWindow::slotFileSave()");
    bool ret = false;
    cancelled = false;

    if (document!=NULL) {
        if (isAutoSave) {
            // Autosave filename is always supposed to be present.
            // Autosave does not change the cursor.
            ret = document->save(true);
        } else {
            if (document->getFilename().isEmpty()) {
                ret = slotFileSaveAs(cancelled);
            } else {
                QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
                ret = document->save();
                QApplication::restoreOverrideCursor();
            }
        }
    }

    return ret;
}



/**
 * Saves the current file. The user is asked for a new filename
 * and format.
 *
 * @return true if the file was saved successfully or the user cancelled.
 *         false if the file could not be saved or the document
 *         is invalid.
 */
bool QC_MDIWindow::slotFileSaveAs(bool &cancelled) {
    RS_DEBUG->print("QC_MDIWindow::slotFileSaveAs");
    bool ret = false;
    cancelled = false;
    RS2::FormatType t = RS2::FormatDXF;

    QString fn = QG_FileDialog::getSaveFileName(this, &t);
    if (document!=NULL && !fn.isEmpty()) {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        ret = document->saveAs(fn, t);
        QApplication::restoreOverrideCursor();
    } else {
        // cancel is not an error - returns true
        ret = true;
        cancelled = true;
    }

    return ret;
}


/**
 * Requests the closing of this MDI window.
 *
 * @param force Force closing by disabling the cancel button (for demo versions).
 */
bool QC_MDIWindow::slotFileClose(bool force) {
    RS_DEBUG->print("QC_MDIWindow::slotFileClose()");

    //return immediately, if forceClosing is set
    if(forceClosing) return true;

    bool succ = true;
    int exit = 0;

    if(document!=NULL && document->isModified()) {
        QG_ExitDialog dlg(this);

        dlg.setForce(force);
        if (document->getFilename().isEmpty()) {
            dlg.setText(tr("Do you really want to close the drawing?"));
        } else {
            QString fn = document->getFilename();
            if (fn.length() > 50) {
                fn = QString("%1...%2").arg(fn.left(24)).arg(fn.right(24));
            }
            dlg.setText(tr("Do you really want to close the file\n%1?")
                        .arg(fn));
        }
        dlg.setTitle(tr("Closing Drawing"));

        bool again;
        bool cancelled;
        do {
            again = false;
            exit = dlg.exec();

            switch (exit) {
            case 0: // cancel
                succ = false;
                forceClosing=false;
                break;
            case 1: // leave
                succ = true;
                forceClosing=true;
                break;
            case 2: // save
                succ = slotFileSave(cancelled);
                again = !succ || cancelled;
                break;
            case 3: // save as
                succ = slotFileSaveAs(cancelled);
                again = !succ || cancelled;
                break;
            default:
                forceClosing=false;
                break;
            }
        } while (again);
    } else {
        succ = true;
    }

    return forceClosing || succ;
}


void QC_MDIWindow::slotFilePrint() {

    RS_DEBUG->print("QC_MDIWindow::slotFilePrint");

    //statusBar()->showMessage(tr("Printing..."));
    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    if (dialog.exec()) {
        QPainter painter;
        painter.begin(&printer);

        ///////////////////////////////////////////////////////////////////
        // TODO: Define printing by using the QPainter methods here

        painter.end();
    };

    //statusBar()->showMessage(tr("Ready."));
}



/**
 * Streams some info about an MDI window to stdout.
 */
std::ostream& operator << (std::ostream& os, QC_MDIWindow& w) {
    os << "QC_MDIWindow[" << w.getId() << "]:\n";
    if (w.parentWindow!=NULL) {
        os << "  parentWindow: " << w.parentWindow->getId() << "\n";
    } else {
        os << "  parentWindow: NULL\n";
    }
    for(int i=0;i<w.childWindows.size();i++){
        QC_MDIWindow *tmp=w.childWindows.at(i);
        os << "  childWindow[" << i << "]: "
           << tmp->getId() << "\n";
    }


    return os;
}

