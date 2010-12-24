/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include <qmainwindow.h>

#include <qlist.h>

#include "qc_graphicview.h"

#include "qg_layerwidget.h"
#include "qg_recentfiles.h"
#include "qg_pentoolbar.h"
//Added by qt3to4:
#include <QCloseEvent>

#include "rs_document.h"


/**
 * MDI document window. Contains a document and a view (window).
 *
 * @author Andrew Mustun
 */
class QC_MDIWindow: public QMainWindow {
    Q_OBJECT

public:
    QC_MDIWindow(RS_Document* doc,
                 QWidget* parent,
                 const char* name=NULL,
                 Qt::WindowFlags wflags=Qt::WDestructiveClose);
    ~QC_MDIWindow();

    void initDoc(RS_Document* doc=NULL);
    void initView();

public slots:

    void slotPenChanged(RS_Pen p);

    void slotFileNew();
    bool slotFileOpen(const QString& fileName, RS2::FormatType type);
    bool slotFileSave(bool &cancelled);
    bool slotFileSaveAs(bool &cancelled);
    bool slotFileClose(bool force);
    void slotFilePrint();

public:
    /** @return Pointer to graphic view */
    QC_GraphicView* getGraphicView() {
        return graphicView;
    }

    /** @return Pointer to document */
    RS_Document* getDocument() {
        return document;
    }
	
    /** @return Pointer to graphic or NULL */
    RS_Graphic* getGraphic() {
        return document->getGraphic();
    }

	/** @return Pointer to current event handler */
	RS_EventHandler* getEventHandler() {
		if (graphicView!=NULL) {
			return graphicView->getEventHandler();
		}
		else {
			return NULL;
		}
	}

    void addChildWindow(QC_MDIWindow* w);
    void removeChildWindow(QC_MDIWindow* w);
    QC_MDIWindow* getPrintPreview();

    /**
     * Sets the parent window that will be notified if this 
     */
    void setParentWindow(QC_MDIWindow* p) {
        RS_DEBUG->print("setParentWindow");
        parentWindow = p;
    }

    /**
     * @return The MDI window id.
     */
    int getId() {
        return id;
    }

	bool closeMDI(bool force, bool ask=true);

	void setForceClosing(bool on) {
		forceClosing = on;
	}

    friend std::ostream& operator << (std::ostream& os, QC_MDIWindow& w);

signals:
    void signalClosing();

protected:
    void closeEvent(QCloseEvent*);

private:
    /** window ID */
    int id;
    /** ID counter */
    static int idCounter;
    /** Graphic view */
    QC_GraphicView* graphicView;
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
    QC_MDIWindow* parentWindow;

	/**
	 * If flag is set, the user will not be asked about closing this file.
	 */
	bool forceClosing;
};


#endif

