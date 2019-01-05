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
#include <QList>
#include "rs.h"
#include "rs_layerlistlistener.h"
#include "rs_blocklistlistener.h"

class QG_GraphicView;
class RS_Document;
class RS_Graphic;
class RS_Pen;
class QMdiArea;
class RS_EventHandler;
class QCloseEvent;

/**
 * MDI document window. Contains a document and a view (window).
 *
 * @author Andrew Mustun
 */
class QC_MDIWindow: public QMdiSubWindow,
                    public RS_LayerListListener,
                    public RS_BlockListListener
{
    Q_OBJECT

public:
    QC_MDIWindow(RS_Document* doc,
                 QWidget* parent,
                 Qt::WindowFlags wflags=0);
    ~QC_MDIWindow();

public slots:

	void slotPenChanged(const RS_Pen& p);
    void slotFileNew();
    bool slotFileNewTemplate(const QString& fileName, RS2::FormatType type);
    bool slotFileOpen(const QString& fileName, RS2::FormatType type);
    bool slotFileSave(bool &cancelled, bool isAutoSave=false);
    bool slotFileSaveAs(bool &cancelled);
    bool slotFileClose(bool force);
    void slotFilePrint();
    void slotZoomAuto();

public:
    /** @return Pointer to graphic view */
	QG_GraphicView* getGraphicView() const;

    /** @return Pointer to document */
	RS_Document* getDocument() const;
	
    /** @return Pointer to graphic or NULL */
	RS_Graphic* getGraphic() const;

	/** @return Pointer to current event handler */
	RS_EventHandler* getEventHandler() const;

    void addChildWindow(QC_MDIWindow* w);
    void removeChildWindow(QC_MDIWindow* w);
    QC_MDIWindow* getPrintPreview();

    // Methods from RS_LayerListListener Interface:
    void layerListModified(bool) override {
        setWindowModified(document->isModified());
    }

    // Methods from RS_BlockListListener Interface:
    void blockListModified(bool) override {
        setWindowModified(document->isModified());
    }

    /**
     * Sets the parent window that will be notified if this 
     */
	void setParentWindow(QC_MDIWindow* p);
    /**
     * @return The MDI window id.
     */
	int getId() const;

	bool closeMDI(bool force, bool ask=true);

	void setForceClosing(bool on);

    friend std::ostream& operator << (std::ostream& os, QC_MDIWindow& w);

    bool has_children();

signals:
    void signalClosing(QC_MDIWindow*);

protected:
    void closeEvent(QCloseEvent*);

private:
    void drawChars();

private:
    /** window ID */
    int id;
    /** ID counter */
    static int idCounter;
    /** Graphic view */
    QG_GraphicView* graphicView;
    /** Document */
    RS_Document* document;
    /** Does the window own the document? */
    bool owner;
    /**
     * List of known child windows that show blocks of the same drawing.
     */
    QList<QC_MDIWindow*> childWindows;
    /**
     * Pointer to parent window which needs to know if this window 
     * is closed or NULL.
     */
    QC_MDIWindow* parentWindow{nullptr};
    QMdiArea* cadMdiArea;

	/**
	 * If flag is set, the user will not be asked about closing this file.
	 */
    bool forceClosing{false};
};


#endif

