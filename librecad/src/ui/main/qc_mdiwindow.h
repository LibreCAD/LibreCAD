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
#ifndef QC_MDIWINDOW_H
#define QC_MDIWINDOW_H
#include <QMdiSubWindow>

#include "rs_graphic.h"

class QG_GraphicView;
class LC_DocumentsStorage;
class LC_ActionContext;
class RS_EventHandler;
/**
 * MDI document window. Contains a document and a view (window).
 *
 * @author Andrew Mustun
 */
class QC_MDIWindow:public QMdiSubWindow,
                   public LC_GraphicModificationListener {
    Q_OBJECT
public:
    QC_MDIWindow(RS_Document *doc,QWidget *parent,bool printPreview, LC_ActionContext* actionContext);
    void removeWidgetsListeners() const;
    ~QC_MDIWindow() override;
public slots:
    void slotPenChanged(const RS_Pen &p);
    void slotFileNew();
    bool loadDocumentFromTemplate(const QString &fileName, RS2::FormatType type);
    bool loadDocument(const QString &fileName, RS2::FormatType type);
    bool saveDocument(bool &cancelled, bool isAutoSave = false);
    bool autoSaveDocument(QString &autosaveFileName);
    bool saveDocumentAs(bool &cancelled);
    void slotFilePrint();
public:
    enum SaveOnClosePolicy {
        ASK,
        SAVE,
        DONT_SAVE,
        CANCEL
    };
    /** @return Pointer to graphic view */
    QG_GraphicView *getGraphicView() const;
    /** @return Pointer to document */
    RS_Document *getDocument() const;
    /** @return Pointer to graphic or NULL */
    RS_Graphic *getGraphic() const;

    void addChildWindow(QC_MDIWindow *w);
    void removeChildWindow(QC_MDIWindow *w);
    QList<QC_MDIWindow *> &getChildWindows();
    QC_MDIWindow *getPrintPreview();

    QString getFileName() const;
    /**
     * Sets the parent window that will be notified if this window
     * is closed or NULL.
     */
    void setParentWindow(QC_MDIWindow *p);
    QC_MDIWindow *getParentWindow() const;
    /**
     * @return The MDI window id.
     */
    unsigned getId() const;
    friend std::ostream &operator <<(std::ostream &os, QC_MDIWindow &w);
    bool has_children() const;

    void graphicModified(const RS_Graphic *g, bool modified) override;
    void undoStateChanged(const RS_Graphic* g, bool undoAvailable, bool redoAvailable) override;
    void zoomAuto();
    bool isModified();

    SaveOnClosePolicy getSaveOnClosePolicy() const{
        return m_saveOnClosePolicy;
    }

    void setSaveOnClosePolicy(SaveOnClosePolicy val){
        m_saveOnClosePolicy = val;
    }

protected:
    LC_DocumentsStorage *m_documentsStorage;
    // window ID
    unsigned id = 0;
    // Graphic view
    QG_GraphicView *m_graphicView = nullptr;
    // Document
    RS_Document *m_document = nullptr;
    // Does the window own the document?
    bool m_owner = false;
    // List of known child windows that show blocks of the same drawing.
    QList<QC_MDIWindow *> m_childWindows;
    //  Pointer to parent window which needs to know if this window is closed or NULL.
    QC_MDIWindow *m_parentWindow{nullptr};
    QMdiArea *m_cadMdiArea = nullptr;

    SaveOnClosePolicy m_saveOnClosePolicy = ASK;
    void drawChars(); // fime - sand - files - refactor and remove from there!
    void closeEvent(QCloseEvent *) override;
    void addWidgetsListeners();
    void setupGraphicView(QWidget *parent, bool printPreview, LC_ActionContext* actionContext);
};
#endif
