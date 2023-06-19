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
#include <cmath>
#include <iostream>

#include "qc_mdiwindow.h"

#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrinter>

#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QFileInfo>
#include <QMdiArea>
#include <QMessageBox>
#include <QPainter>
#include <QPoint>
#include <QTimer>

#include "lc_rect.h"
#include "qg_filedialog.h"
#include "qg_graphicview.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_mtext.h"
#include "rs_pen.h"
#include "rs_settings.h"

struct QC_MDIWindow::AutoPanData
{
    std::unique_ptr<QTimer> panTimer;

    QPoint panOffset;

    const int scrollbarWidth{15 /* pixels */};

    const double panOffsetMagnitude = {10.0};

    const double panTimerInterval_minimum{10.0};
    const double panTimerInterval_maximum{100.0};

    RS_Vector probedAreaOffset = RS_Vector(50 /* pixels */, 50 /* pixels */);
};

/**
 * Constructor.
 *
 * @param doc Pointer to an existing document of nullptr if a new
 *   document shall be created for this window.
 * @param parent An instance of QMdiArea.
 */
QC_MDIWindow::QC_MDIWindow(RS_Document *doc, QWidget *parent, Qt::WindowFlags wflags)
    : QMdiSubWindow(parent, wflags)
    , m_owner{doc == nullptr}
    , m_panData{std::make_unique<QC_MDIWindow::AutoPanData>()}
{
    setAttribute(Qt::WA_DeleteOnClose);
    cadMdiArea = qobject_cast<QMdiArea *>(parent);

    if (doc == nullptr) {
        document = new RS_Graphic();
        document->newDoc();
    } else {
        document = doc;
    }

    graphicView = new QG_GraphicView(this, {}, document);
    graphicView->setObjectName("graphicview");

    connect(graphicView,
            SIGNAL(previous_zoom_state(bool)),
            parent->window(),
            SLOT(setPreviousZoomEnable(bool)));

    graphicView->installEventFilter(this);

    setWidget(graphicView);

    /** ID counter */
    static unsigned idCounter = 0;
    id = idCounter++;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
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
    if (!(graphicView && graphicView->isCleanUp())) {
        //do not clear layer/block lists, if application is being closed

        if (document->getLayerList()) {
            document->getLayerList()->removeListener(graphicView);
            document->getLayerList()->removeListener(this);
        }

        if (document->getBlockList()) {
            document->getBlockList()->removeListener(graphicView);
            document->getBlockList()->removeListener(this);
        }

        if (m_owner) {
            delete document;
        }
        document = nullptr;
    }
}

QG_GraphicView *QC_MDIWindow::getGraphicView() const
{
    return graphicView;
}

/** @return Pointer to document */
RS_Document *QC_MDIWindow::getDocument() const
{
    return document;
}

unsigned QC_MDIWindow::getId() const
{
    return id;
}

RS_EventHandler* QC_MDIWindow::getEventHandler() const
{
    if (graphicView) {
        return graphicView->getEventHandler();
    } else {
        return nullptr;
    }
}

void QC_MDIWindow::setParentWindow(QC_MDIWindow *p)
{
    RS_DEBUG->print("QC_MDIWindow::setParentWindow");
    parentWindow = p;
}

QC_MDIWindow *QC_MDIWindow::getParentWindow() const
{
    RS_DEBUG->print("QC_MDIWindow::getParentWindow");
    return parentWindow;
}

RS_Graphic *QC_MDIWindow::getGraphic() const
{
    return document->getGraphic();
}

/**
 * Adds another MDI window to the list of known windows that
 * depend on this one. This can be another view or a view for
 * a particular block.
 */
void QC_MDIWindow::addChildWindow(QC_MDIWindow *w)
{
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
void QC_MDIWindow::removeChildWindow(QC_MDIWindow *w)
{
    //    RS_DEBUG->print("%s %s()", __FILE__, __func__);
    if (childWindows.size() > 0) {
        if (childWindows.contains(w)) {
            childWindows.removeAll(w);
            //            suc=true;
        }
    }

    //    bool suc = childWindows.removeAll(w);
    //    RS_DEBUG->print("successfully removed child window: %d", (int)suc);

    //    RS_DEBUG->print("children: %d", childWindows.count());
}

QList<QC_MDIWindow *> &QC_MDIWindow::getChildWindows()
{
    return childWindows;
}

/**
 * @return pointer to the print preview of this drawing or nullptr.
 */
QC_MDIWindow *QC_MDIWindow::getPrintPreview()
{
    for (auto *w : childWindows) {
        if (w->getGraphicView()->isPrintPreview()) {
            return w;
        }
    }
    return nullptr;
}

/**
 * Called by Qt when the user closes this MDI window.
 */
void QC_MDIWindow::closeEvent(QCloseEvent *ce)
{
    RS_DEBUG->print("QC_MDIWindow::closeEvent begin");

    emit(signalClosing(this));
    ce->accept(); // handling delegated to QApplication

    RS_DEBUG->print("QC_MDIWindow::closeEvent end");
}

/**
 * Called when the current pen (color, style, width) has changed.
 * Sets the active pen for the document in this MDI window.
 */
void QC_MDIWindow::slotPenChanged(const RS_Pen &pen)
{
    RS_DEBUG->print("QC_MDIWindow::slotPenChanged() begin");
    if (document != nullptr)
        document->setActivePen(pen);

    RS_DEBUG->print("QC_MDIWindow::slotPenChanged() end");
}

/**
 * Creates a new empty document in this MDI window.
 */
void QC_MDIWindow::slotFileNew()
{
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
bool QC_MDIWindow::slotFileNewTemplate(const QString &fileName, RS2::FormatType type)
{
    RS_DEBUG->print("QC_MDIWindow::slotFileNewTemplate begin");

    bool ret = false;

    if (document == nullptr || fileName.isEmpty())
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
bool QC_MDIWindow::slotFileOpen(const QString &fileName, RS2::FormatType type)
{
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

void QC_MDIWindow::slotZoomAuto()
{
    if (graphicView) {
        if (graphicView->isPrintPreview()) {
            graphicView->zoomPage();
        } else {
            graphicView->zoomAuto();
        }
    }
}

void QC_MDIWindow::drawChars()
{
    RS_BlockList *bl = document->getBlockList();
    double sep = document->getGraphic()->getVariableDouble("LetterSpacing", 3.0);
    double h = sep / 3;
    sep = sep * 3;
    for (int i = 0; i < bl->count(); ++i) {
        RS_Block *ch = bl->at(i);
        RS_InsertData
            data(ch->getName(), RS_Vector(i * sep, 0), RS_Vector(1, 1), 0, 1, 1, RS_Vector(0, 0));
        RS_Insert *in = new RS_Insert(document, data);
        document->addEntity(in);
        QFileInfo info(document->getFilename());
        QString uCode = (ch->getName()).mid(1, 4);
        RS_MTextData datatx(RS_Vector(i * sep, -h),
                            h,
                            4 * h,
                            RS_MTextData::VATop,
                            RS_MTextData::HALeft,
                            RS_MTextData::ByStyle,
                            RS_MTextData::AtLeast,
                            1,
                            uCode,
                            "standard",
                            0);
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
bool QC_MDIWindow::slotFileSave(bool &cancelled, bool isAutoSave)
{
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
                QFileInfo info(document->getFilename());
                if (!info.isWritable())
                    return false;
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
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
bool QC_MDIWindow::slotFileSaveAs(bool &cancelled)
{
    RS_DEBUG->print("QC_MDIWindow::slotFileSaveAs");
    bool ret = false;
    cancelled = false;
    RS2::FormatType t = RS2::FormatDXFRW;

    QG_FileDialog dlg(this);
    QString fn = dlg.getSaveFile(&t);
    if (document && !fn.isEmpty()) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
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

void QC_MDIWindow::slotFilePrint()
{
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
std::ostream &operator<<(std::ostream &os, QC_MDIWindow &w)
{
    os << "QC_MDIWindow[" << w.getId() << "]:\n";
    if (w.parentWindow) {
        os << "  parentWindow: " << w.parentWindow->getId() << "\n";
    } else {
        os << "  parentWindow: nullptr\n";
    }
    int i = 0;
    for (auto p : const_cast<const QList<QC_MDIWindow *> &>(w.childWindows)) {
        os << "  childWindow[" << i++ << "]: " << p->getId() << "\n";
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

/*
    Decides when to auto-pan the CAD area, and by how much.
    - by Melwyn Francis Carlo <carlo.melwyn@outlook.com>
*/
bool QC_MDIWindow::eventFilter(QObject *obj, QEvent *event)
{
    const QEvent::Type currentEventType = event->type();
    if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL) {
        DEBUG_HEADER
        RS_DEBUG->print(RS_Debug::D_INFORMATIONAL,
                        QString(" Current event type = %1\n")
                            .arg(currentEventType)
                            .toStdString()
                            .c_str());
    }

    if (currentEventType == QEvent::MouseMove) /* QEvent::MouseMove = 5 */
    {
        if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL) {
            //RS_DEBUG->print(RS_Debug::D_INFORMATIONAL,  QString("Status = %1, Action Type = %2").arg(status).arg(actionType)
        }

        RS_SETTINGS->beginGroupGuard("/Appearance");
        const bool autopanEnabled = (bool) RS_SETTINGS->readNumEntry("/Autopanning", 0);

        if (!autopanEnabled)
            return QObject::eventFilter(obj, event);

        //if (((status > 0) && (actionType > 1)) || ((status == 2) && (actionType == 1))) {
        //            QC_MDIWindow *mdiWindow = QC_ApplicationWindow::getAppWindow()
        //                                          ->QC_ApplicationWindow::getMDIWindow();

        auto *drawingArea = widget();
        if (drawingArea == nullptr)
            return false;
        const RS_Vector cadArea_minCoord(drawingArea->x(), drawingArea->y());

        const RS_Vector cadArea_maxCoord(drawingArea->x() + drawingArea->width(),
                                         drawingArea->y() + drawingArea->height());

        const LC_Rect cadArea_actual(cadArea_minCoord, cadArea_maxCoord);

        const LC_Rect cadArea_unprobed(cadArea_minCoord + m_panData->probedAreaOffset,
                                       cadArea_maxCoord - m_panData->probedAreaOffset);
        if (cadArea_unprobed.width() < 0. || cadArea_unprobed.height() < 0.)
            return false;

        RS_Vector mouseCoord(((QMouseEvent *) event)->x(), ((QMouseEvent *) event)->y());

        if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL) {
            std::cout << " Unprobed CAD area width and height = " << cadArea_unprobed.width() << "/"
                      << cadArea_unprobed.height() << std::endl
                      << " Actual   CAD area width and height = " << cadArea_actual.width() << "/"
                      << cadArea_actual.height() << std::endl
                      << " Mouse (cursor) position            = " << mouseCoord << std::endl
                      << std::endl;
        }

        if (cadArea_actual.inArea(mouseCoord) && !cadArea_unprobed.inArea(mouseCoord)) {
            mouseCoord.y = cadArea_actual.height() - mouseCoord.y;

            const RS_Vector cadArea_centerPoint((cadArea_minCoord + cadArea_maxCoord) / 2.0);

            double panOffset_angle{cadArea_centerPoint.angleTo(mouseCoord)};

            /* It would be better if the below value was calculated in the code that deals with resizing the CAD area. */
            const double quarterAngle{
                cadArea_centerPoint.angleTo(cadArea_actual.upperRightCorner())};

            double percentageFactor;

            if (((panOffset_angle > quarterAngle)
                 && (panOffset_angle <= (M_PI - quarterAngle)))
                || ((panOffset_angle > (quarterAngle + M_PI))
                    && (panOffset_angle <= (M_PI + M_PI - quarterAngle)))) {
                percentageFactor = (std::abs((mouseCoord - cadArea_centerPoint).y)
                                    - (cadArea_unprobed.height() / 2.0))
                                   / ((cadArea_actual.height() / 2.0)
                                      - (cadArea_unprobed.height() / 2.0));
            } else {
                percentageFactor = (std::abs((mouseCoord - cadArea_centerPoint).x)
                                    - (cadArea_unprobed.width() / 2.0))
                                   / ((cadArea_actual.width() / 2.0)
                                      - (cadArea_unprobed.width() / 2.0));
            }

            const double panTimerInterval{
                m_panData->panTimerInterval_minimum
                + ((m_panData->panTimerInterval_maximum - m_panData->panTimerInterval_minimum)
                   * (1.0 - percentageFactor))};

            RS_Vector offset = RS_Vector::polar(m_panData->panOffsetMagnitude,
                                                M_PI - panOffset_angle);
            m_panData->panOffset = {static_cast<int>(offset.x), static_cast<int>(offset.y)};

            if (m_panData->panTimer != nullptr) {
                m_panData->panTimer->setInterval(panTimerInterval);
            } else {
                m_panData->panTimer = std::make_unique<QTimer>(this);
                connect(m_panData->panTimer.get(), &QTimer::timeout, this, &QC_MDIWindow::autoPan);
                m_panData->panTimer->start(panTimerInterval);
            }

            if (RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL) {
                std::cout << " CAD area centre point                = " << cadArea_centerPoint
                          << std::endl
                          << " Actual CAD area quarter angle (deg)  = "
                          << quarterAngle * 180.0 / M_PI << std::endl
                          << " Percentage factor                    = " << percentageFactor
                          << std::endl
                          << " Pan offset angle (radians)           = " << panOffset_angle
                          << std::endl
                          << " Pan offset angle (degrees)           = "
                          << panOffset_angle * 180.0 / M_PI << std::endl
                          << " Pan offset vector                    = " << m_panData->panOffset.x()
                          << ", " << m_panData->panOffset.y()
                          << std::endl
                          //<< " Pan timer interval (ms)              = " << m_panData->panTimer->interfac
                          << std::endl
                          << " Mouse (cursor) position (adjusted)   = " << mouseCoord << std::endl
                          << " Mouse position w.r.t. centre point   = "
                          << mouseCoord - cadArea_centerPoint << std::endl
                          << std::endl
                          << std::endl;
            }

            return QObject::eventFilter(obj, event);
        }
        // }
    }

    if ((currentEventType != QEvent::Paint)       /* QEvent::Paint   =  12 */
        && (currentEventType != QEvent::ToolTip)) /* QEvent::ToolTip = 110 */
        m_panData->panTimer.reset();

    return QObject::eventFilter(obj, event);
}

/*
    Auto-pans the CAD area.
    - by Melwyn Francis Carlo <carlo.melwyn@outlook.com>
*/
void QC_MDIWindow::autoPan() const
{
    RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, " Timer is ticking!");

    graphicView->zoomPan(m_panData->panOffset.x(), m_panData->panOffset.y());
}
