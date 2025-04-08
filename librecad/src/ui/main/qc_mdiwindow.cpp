/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 Shawn Curry (noneyabiz@mail.wasent.cz)
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

#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>

#include <QCloseEvent>
#include <QMessageBox>
#include <QMdiArea>
#include <QPainter>

#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"

#include "qg_exitdialog.h"
#include "qg_filedialog.h"
#include "qg_graphicview.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_mtext.h"
#include "rs_pen.h"
#include "lc_printpreviewview.h"
/**
 * Constructor.
 *
 * @param doc Pointer to an existing document of NULL if a new
 *   document shall be created for this window.
 * @param parent An instance of QMdiArea.
 */
QC_MDIWindow::QC_MDIWindow(RS_Document *doc, QWidget *parent, bool printPreview, LC_ActionContext* actionContext)
    : QMdiSubWindow(parent, {Qt::WA_DeleteOnClose})
    , m_owner{doc == nullptr}{
    setAttribute(Qt::WA_DeleteOnClose);
    m_cadMdiArea=qobject_cast<QMdiArea*>(parent);

    if (doc==nullptr) {
        m_document = new RS_Graphic();
        m_document->newDoc();
    } else {
        m_document = doc;
    }

    setupGraphicView(parent, printPreview, actionContext);

    static unsigned idCounter = 0;
    id = idCounter++;

    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    addWidgetsListeners();
}

/**
 * Destructor.
 *
 * Deletes the document associated with this window.
 */
QC_MDIWindow::~QC_MDIWindow(){
    try {
        if (!(m_graphicView != nullptr && m_graphicView->isCleanUp())) {
            //do not clear layer/block lists, if application is being closed
            removeWidgetsListeners();
            if (m_owner) {
                delete m_document;
            }
            m_document = nullptr;
        }
    } catch (...) {
        LC_ERR << __func__ << "(): received exception";
    }
}

void QC_MDIWindow::setupGraphicView(QWidget *parent, bool printPreview, LC_ActionContext* actionContext){
    if (printPreview){
        m_graphicView = new LC_PrintPreviewView(this, m_document, actionContext);
        m_graphicView->initView();
    }
    else{
        m_graphicView = new QG_GraphicView(this, m_document, actionContext);
        m_graphicView->initView();
    }
    m_graphicView->setPrintPreview(printPreview);
    m_graphicView->setObjectName("graphicview");

    auto receiver = dynamic_cast<QC_ApplicationWindow *>(parent->window());
    if (receiver != nullptr) {
        connect(m_graphicView, &RS_GraphicView::previous_zoom_state, receiver, &QC_ApplicationWindow::setPreviousZoomEnable);
    }

    setWidget(m_graphicView);
}

void QC_MDIWindow::addWidgetsListeners(){
    if (m_document != nullptr) {
        RS_Graphic* graphic = m_document -> getGraphic();
        if (graphic != nullptr) {
            graphic->setModificationListener(this);
        }
    }
}

void QC_MDIWindow::removeWidgetsListeners(){
    if (m_document != nullptr) {
        RS_Graphic* graphic = m_document -> getGraphic();
        if (graphic != nullptr) {
            graphic->setModificationListener(nullptr);
        }
    }
}


QG_GraphicView* QC_MDIWindow::getGraphicView() const{
    return m_graphicView;
}

RS_Document* QC_MDIWindow::getDocument() const{
	return m_document;
}

QString QC_MDIWindow::getDocumentFileName() const{
    auto graphic = m_document->getGraphic();
    if (graphic == nullptr) {
        return "";
    }
    return graphic->getFilename();
}

unsigned QC_MDIWindow::getId() const{
	return id;
}

RS_EventHandler* QC_MDIWindow::getEventHandler() const{
    if (m_graphicView) {
        return m_graphicView->getEventHandler();
    }
    return nullptr;
}

void QC_MDIWindow::setParentWindow(QC_MDIWindow* p) {
	m_parentWindow = p;
}

QC_MDIWindow* QC_MDIWindow::getParentWindow() const {
	return m_parentWindow;
}

RS_Graphic* QC_MDIWindow::getGraphic() const {
	return m_document->getGraphic();
}

/**
 * Adds another MDI window to the list of known windows that
 * depend on this one. This can be another view or a view for
 * a particular block.
 */
void QC_MDIWindow::addChildWindow(QC_MDIWindow* w) {
    m_childWindows.append(w);
    w->setParentWindow(this);

    int size = (int)m_childWindows.count(); // well, yes, loss of precision... yet for windows amount that's fine.
    RS_DEBUG->print("children: %d", size);
}

/**
 * Removes a child window.
 *
 * @see addChildWindow
 */
void QC_MDIWindow::removeChildWindow(QC_MDIWindow* w) {
    if(m_childWindows.size()>0 ){
        if(m_childWindows.contains(w)){
            m_childWindows.removeAll(w);
        }
    }
}

QList<QC_MDIWindow*>& QC_MDIWindow::getChildWindows(){
	return m_childWindows;
}

/**
 * @return pointer to the print preview of this drawing or NULL.
 */
QC_MDIWindow* QC_MDIWindow::getPrintPreview() {
    for(auto* w: m_childWindows){
        if(w != nullptr && w->getGraphicView()->isPrintPreview()){
			return w;
		}
	}
	return nullptr;
}

/**
 * Called by Qt when the user closes this MDI window.
 */
// fixme - sand - files - remove, fully delegate to main window
void QC_MDIWindow::closeEvent(QCloseEvent* ce) {
    bool cancel = false;
    bool hasParent = getParentWindow() != nullptr;
    const auto& appWin = QC_ApplicationWindow::getAppWindow();

    if (getDocument()->isModified() && !hasParent) {
        int exitChoice = QG_ExitDialog::Save;

        switch (m_saveOnClosePolicy) {
            case ASK: {
                exitChoice = appWin->showCloseDialog(this);
                break;
            }
            case SAVE: {
                exitChoice = QG_ExitDialog::Save;
                break;
            }
            case DONT_SAVE: {
                exitChoice = QG_ExitDialog::DontSave;
                break;
            }
            default:
                break;
        }
        switch (exitChoice) {
            case QG_ExitDialog::Save:
                cancel = !appWin->doSave(this);
                break;
            case QG_ExitDialog::DontSave:
                break;
            case QG_ExitDialog::Cancel:
            default:
                cancel = true;
                break;
        }
    }


    if (cancel){
        appWin->autoSaveCurrentDrawing();
        m_saveOnClosePolicy = CANCEL;
        ce->ignore();
    } else {
        appWin->doClose(this);
        appWin->doArrangeWindows(RS2::CurrentMode);
        ce->accept(); // handling delegated to QApplication
    }
}

/**
 * Called when the current pen (color, style, width) has changed.
 * Sets the active pen for the document in this MDI window.
 */
void QC_MDIWindow::slotPenChanged(const RS_Pen& pen) {
	if (m_document != nullptr) {
        m_document->setActivePen(pen);
    }
}

/**
 * Creates a new empty document in this MDI window.
 */
void QC_MDIWindow::slotFileNew() {
	if (m_document != nullptr && m_graphicView != nullptr) {
        m_document->newDoc();
        m_graphicView->redraw();
    }
}

/**
 * Creates a new document, loading template, in this MDI window.
 */
bool QC_MDIWindow::loadDocumentFromTemplate(const QString& fileName, RS2::FormatType type) {
    return m_documentsStorage->loadDocumentFromTemplate(m_document, m_graphicView, fileName, type);
}

/**
 * Opens the given file in this MDI window.
 */
bool QC_MDIWindow::loadDocument(const QString& fileName, RS2::FormatType type) {
    removeWidgetsListeners();
    bool loaded = m_documentsStorage->loadDocument(m_document, fileName, type);
    addWidgetsListeners();
    if (loaded) {
        RS_Graphic* graphic = m_document->getGraphic();
        if (graphic != nullptr) {
            RS_GraphicView *gv = graphic->getGraphicView(); // fixme - eliminate this dependency!
            if (gv != nullptr) {
                // fixme - sand - review and probably move initialization of UCS - as normal support of VIEWPORT will be available
                // todo - not sure whether this is right place for setting up current wcs.
                // Actually, it seems that it's better to rely on reading viewport (were setting for the offset and zoom are set.
                // however, must probably with proper support of VIEW, they will be reworked too..
                // So let it have here for now so far
                LC_GraphicViewport* viewport = gv->getViewPort();
                viewport->initAfterDocumentOpen();
            }
        }

        // fixme - sand - move support of fonts in some separate space?
        if (fileName.endsWith(".lff") || fileName.endsWith(".cxf")) {
            // fixme - sand - move to upper layer
            drawChars();
            m_graphicView->zoomAuto(false);
        } else
            m_graphicView->redraw();
    } else {

    }
    return loaded;
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
bool QC_MDIWindow::saveDocument(bool &cancelled, bool isAutoSave) {
    bool result = m_documentsStorage->saveDocument(m_document, m_graphicView, cancelled);
    setWindowModified(m_document->isModified());
    return result;
}

bool QC_MDIWindow::autoSaveDocument(QString& autosaveFileName){
    bool result = m_documentsStorage->autoSaveDocument(m_document, m_graphicView, autosaveFileName);
    return result;
}

/**
 * Saves the current file. The user is asked for a new filename
 * and format.
 *
 * @return true if the file was saved successfully or the user cancelled.
 *         false if the file could not be saved or the document
 *         is invalid.
 */
bool QC_MDIWindow::saveDocumentAs(bool &cancelled) {
    bool result = m_documentsStorage->saveDocumentAs(m_document, m_graphicView, cancelled);
    setWindowModified(m_document->isModified());
    return result;
}

void QC_MDIWindow::zoomAuto() {
	if(m_graphicView){
        if(m_graphicView->isPrintPreview()){
            m_graphicView->getViewPort()->zoomPage();
        }else{
            m_graphicView->zoomAuto();
        }
    }
}

bool QC_MDIWindow::isModified(){
    return getDocument()->isModified();
}

void QC_MDIWindow::drawChars() {
    RS_BlockList* bl = m_document->getBlockList();
    double sep = m_document->getGraphic()->getVariableDouble("LetterSpacing", 3.0);
    double h = sep/3;
    sep = sep*3;
    for (int i=0; i<bl->count(); ++i) {
        RS_Block* ch = bl->at(i);
        RS_InsertData data(ch->getName(), RS_Vector(i*sep,0), RS_Vector(1,1), 0, 1, 1, RS_Vector(0,0));
        auto in = new RS_Insert(m_document, data);
        m_document->addEntity(in);
        QString uCode = (ch->getName()).mid(1,4);
        RS_MTextData datatx(RS_Vector(i*sep,-h), h, 4*h, RS_MTextData::VATop,
                           RS_MTextData::HALeft, RS_MTextData::ByStyle, RS_MTextData::AtLeast,
                           1, uCode, "standard", 0);
/*        RS_MTextData datatx(RS_Vector(i*sep,-h), h, 4*h, RS2::VAlignTop,
                           RS2::HAlignLeft, RS2::ByStyle, RS2::AtLeast,
                           1, uCode, info.baseName(), 0);*/
        auto tx = new RS_MText(m_document, datatx);
        m_document->addEntity(tx);
    }

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
	if (w.m_parentWindow) {
        os << "  parentWindow: " << w.m_parentWindow->getId() << "\n";
    } else {
        os << "  parentWindow: nullptr\n";
    }
    int i=0;
    for(auto p: const_cast<const QList<QC_MDIWindow*>&>(w.m_childWindows)){
		os << "  childWindow[" << i++ << "]: "
		   << p->getId() << "\n";
	}
    return os;
}

/**
 * Return true if this window has children (QC_MDIWindow).
 */
bool QC_MDIWindow::has_children() const{
    return !m_childWindows.isEmpty();
}

void QC_MDIWindow::graphicModified(const RS_Graphic* g, bool modified){
    setWindowModified(modified);
    auto& appWin = QC_ApplicationWindow::getAppWindow();
    if (appWin !=nullptr) {
        appWin->setSaveEnable(modified);
    }
}

void QC_MDIWindow::undoStateChanged(const RS_Graphic *g, bool undoAvailable, bool redoAvailable){
    auto& appWin = QC_ApplicationWindow::getAppWindow();
    if (appWin !=nullptr) {
        appWin->setRedoEnable(redoAvailable);
        appWin->setUndoEnable(undoAvailable);
    }
}

QString QC_MDIWindow::getFileName() const{
    RS_Graphic* graphic = m_document->getGraphic();
    if (graphic == nullptr) {
        return "";
    }
    return graphic->getFilename();
}
