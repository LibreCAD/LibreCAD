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
#include<iostream>
#include "qc_mdiwindow.h"

#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>

#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QMessageBox>
#include <QFileInfo>
#include <QMdiArea>
#include <QPainter>

#include "rs_graphic.h"
#include "rs_settings.h"
#include "qg_exitdialog.h"
#include "qg_filedialog.h"
#include "rs_insert.h"
#include "rs_mtext.h"
#include "rs_pen.h"
#include "qg_graphicview.h"
#include "rs_debug.h"

int QC_MDIWindow::idCounter = 0;

/**
 * Constructor.
 *
 * @param doc Pointer to an existing document of NULL if a new
 *   document shall be created for this window.
 * @param parent An instance of QMdiArea.
 */
QC_MDIWindow::QC_MDIWindow(RS_Document* doc, QWidget* parent, Qt::WindowFlags wflags)
                            : QMdiSubWindow(parent, wflags)
{
    setAttribute(Qt::WA_DeleteOnClose);
    cadMdiArea=qobject_cast<QMdiArea*>(parent);

    if (doc==nullptr) {
        document = new RS_Graphic();
        document->newDoc();
        owner = true;
    } else {
        document = doc;
        owner = false;
    }

    graphicView = new QG_GraphicView(this, 0, document);
    graphicView->setObjectName("graphicview");

    connect(graphicView, SIGNAL(previous_zoom_state(bool)),
            parent->window(), SLOT(setPreviousZoomEnable(bool)));

    setWidget(graphicView);

    id = idCounter++;
    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
	if (document) {
		if (document->getLayerList()) {
            // Link the graphic view to the layer widget
            document->getLayerList()->addListener(graphicView);
            // Link this window to the layer widget
            document->getLayerList()->addListener(this);
        }
		if (document->getBlockList()) {
            // Link the graphic view to the block widget
            document->getBlockList()->addListener(graphicView);
            // Link this window to the block widget
            document->getBlockList()->addListener(this);
        }
    }
}

/**
 * Destructor.
 *
 * Deletes the document associated with this window.
 */
QC_MDIWindow::~QC_MDIWindow()
{
    RS_DEBUG->print("~QC_MDIWindow");
	if(!(graphicView && graphicView->isCleanUp())){

		//do not clear layer/block lists, if application is being closed

		if (document->getLayerList()) {
			document->getLayerList()->removeListener(graphicView);
			document->getLayerList()->removeListener(this);
		}

		if (document->getBlockList()) {
			document->getBlockList()->removeListener(graphicView);
			document->getBlockList()->removeListener(this);
		}

		if (owner==true && document) {
			delete document;
		}
		document = nullptr;
	}
}

QG_GraphicView* QC_MDIWindow::getGraphicView() const
{
    return (graphicView) ? graphicView : nullptr;
}

/** @return Pointer to document */
RS_Document* QC_MDIWindow::getDocument() const{
	return document;
}

int QC_MDIWindow::getId() const{
	return id;
}

void QC_MDIWindow::setForceClosing(bool on) {
	forceClosing = on;
}

RS_EventHandler* QC_MDIWindow::getEventHandler() const{
	if (graphicView) {
		return graphicView->getEventHandler();
	}
	else {
		return nullptr;
	}
}

void QC_MDIWindow::setParentWindow(QC_MDIWindow* p) {
	RS_DEBUG->print("setParentWindow");
	parentWindow = p;
}

RS_Graphic* QC_MDIWindow::getGraphic() const {
	return document->getGraphic();
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
//    RS_DEBUG->print("%s %s()", __FILE__, __func__);
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
	for(auto w: childWindows){
		if(w->getGraphicView()->isPrintPreview()){
			return w;
		}
	}
	return nullptr;
}



/**
 * closes this MDI window.
 *
 * @param force Disable cancel button (demo versions)
 * @param ask Ask user before closing.
 */
bool QC_MDIWindow::closeMDI(bool force, bool ask)
{
    RS_DEBUG->print("QC_MDIWindow::closeMDI begin");
    // should never happen:
    if (document==NULL) {
        return true;
    }

    bool ret = false;

    // This is a block and we don't need to ask the user for closing
    //   since it's still available in the parent drawing after closing.
    if (parentWindow)
    {
        RS_DEBUG->print("  closing block");
        RS_DEBUG->print("  notifying parent about closing this window");
        parentWindow->removeChildWindow(this);
        emit(signalClosing(this));
        ret = true;
    }

    // This is a graphic document. ask user for closing.
    else if (!ask || slotFileClose(force)) {
        RS_DEBUG->print("  closing graphic");

        emit(signalClosing(this));

        if (childWindows.length() > 0)
        {
            for(auto p: childWindows)
            {
                cadMdiArea->removeSubWindow(p);
                p->close();
            }
		childWindows.clear();
        }

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

    auto view = getGraphicView();
    view->killAllActions();

    RS_DEBUG->print("QC_MDIWindow::closeEvent begin");
    if (forceClosing) {
        ce->accept();

        return;
    }

    if (closeMDI(false)) {
        ce->accept();
    } else {
        ce->ignore();
    }

    RS_DEBUG->print("QC_MDIWindow::closeEvent end");
}

/**
 * Called when the current pen (color, style, width) has changed.
 * Sets the active pen for the document in this MDI window.
 */
void QC_MDIWindow::slotPenChanged(const RS_Pen& pen) {
    RS_DEBUG->print("QC_MDIWindow::slotPenChanged() begin");
	if (document) {
        document->setActivePen(pen);
    }
    RS_DEBUG->print("QC_MDIWindow::slotPenChanged() end");
}


/**
 * Creates a new empty document in this MDI window.
 */
void QC_MDIWindow::slotFileNew() {
    RS_DEBUG->print("QC_MDIWindow::slotFileNew begin");
	if (document && graphicView) {
        document->newDoc();
        graphicView->redraw();
    }
    RS_DEBUG->print("QC_MDIWindow::slotFileNew end");
}


/**
 * Creates a new document, loading template, in this MDI window.
 */
bool QC_MDIWindow::slotFileNewTemplate(const QString& fileName, RS2::FormatType type) {
    RS_DEBUG->print("QC_MDIWindow::slotFileNewTemplate begin");

    bool ret = false;

    if (document==NULL || fileName.isEmpty())
        return ret;

    document->newDoc();
    ret = document->loadTemplate(fileName, type);
    if (ret) {
        RS_DEBUG->print("QC_MDIWindow::slotFileNewTemplate: autoZoom");
        graphicView->zoomAuto(false);
    } else
        RS_DEBUG->print("QC_MDIWindow::slotFileNewTemplate: failed");

    RS_DEBUG->print("QC_MDIWindow::slotFileNewTemplate end");
    return ret;
}

/**
 * Opens the given file in this MDI window.
 */
bool QC_MDIWindow::slotFileOpen(const QString& fileName, RS2::FormatType type) {

    RS_DEBUG->print("QC_MDIWindow::slotFileOpen");
    bool ret = false;

	if (document && !fileName.isEmpty()) {
        document->newDoc();

                // cosmetics..
                // RVT_PORT qApp->processEvents(1000);
                qApp->processEvents(QEventLoop::AllEvents, 1000);

        ret = document->open(fileName, type);

        if (ret) {
            //QString message=tr("Loaded document: ")+fileName;
            //statusBar()->showMessage(message, 2000);

            if (fileName.endsWith(".lff") || fileName.endsWith(".cxf")) {
                drawChars();

                RS_DEBUG->print("QC_MDIWindow::slotFileOpen: autoZoom");
                graphicView->zoomAuto(false);
                RS_DEBUG->print("QC_MDIWindow::slotFileOpen: autoZoom: OK");
            } else
                graphicView->redraw();
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

void QC_MDIWindow::slotZoomAuto() {
	if(graphicView){
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
        RS_MTextData datatx(RS_Vector(i*sep,-h), h, 4*h, RS_MTextData::VATop,
                           RS_MTextData::HALeft, RS_MTextData::ByStyle, RS_MTextData::AtLeast,
                           1, uCode, "standard", 0);
/*        RS_MTextData datatx(RS_Vector(i*sep,-h), h, 4*h, RS2::VAlignTop,
                           RS2::HAlignLeft, RS2::ByStyle, RS2::AtLeast,
                           1, uCode, info.baseName(), 0);*/
        RS_MText *tx = new RS_MText(document, datatx);
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

	if (document) {
        document->setGraphicView(graphicView);
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
    RS2::FormatType t = RS2::FormatDXFRW;

    QG_FileDialog dlg(this);
    QString fn = dlg.getSaveFile(&t);
	if (document && !fn.isEmpty()) {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        document->setGraphicView(graphicView);
        ret = document->saveAs(fn, t, true);
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

	if(document && document->isModified()) {
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
	if (w.parentWindow) {
        os << "  parentWindow: " << w.parentWindow->getId() << "\n";
    } else {
        os << "  parentWindow: NULL\n";
    }
	int i=0;
	for(auto p: w.childWindows){
		os << "  childWindow[" << i++ << "]: "
		   << p->getId() << "\n";
	}

    return os;
}

/**
 * Return true if this window has children (QC_MDIWindow).
 */
bool QC_MDIWindow::has_children()
{
    return !childWindows.isEmpty();
}
