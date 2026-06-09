/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */
#include "qg_selectionwidget.h"

#include <QSettings>
#include <QTimer>

#include "rs_document.h"
#include "rs_graphicview.h"

/*
 *  Constructs a QG_SelectionWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_SelectionWidget::QG_SelectionWidget(QWidget* parent, const char* name, const Qt::WindowFlags fl)
    : QWidget(parent, fl){
    setObjectName(name);
    setupUi(this);

    lEntities->setText("0");

    m_auxDataMode = false;

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect( m_timer, &QTimer::timeout, this, &QG_SelectionWidget::removeAuxData);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_SelectionWidget::~QG_SelectionWidget(){
    // no need to delete child widgets, Qt does it all for us
    delete m_timer;
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_SelectionWidget::languageChange(){
    retranslateUi(this);
}

void QG_SelectionWidget::setNumber(const int n) const {
    if (m_auxDataMode)    {
        QSettings settings("QGDialogFactory", "QGSelectionWidget");
        settings.setValue("lEntities_text", n);
    }
    else /* if (!auxDataMode) */{
        QString str;
        str.setNum(n);
        lEntities->setText(str);
    }
}

void QG_SelectionWidget::setTotalLength(const double l) const {
    QString str;
    str.setNum(l, 'g', 6);
    lTotalLength->setText(str);
}


void QG_SelectionWidget::flashAuxData(const QString& header, const QString& data, const unsigned int timeout, const bool flash){
    if (flash){
        if (!m_auxDataMode){
            m_auxDataMode = true;
            QSettings settings("QGDialogFactory", "QGSelectionWidget");

            settings.setValue("lLabelLength_minWidth",  lLabelLength->minimumWidth());
            settings.setValue("lLabelLength_minHeight", lLabelLength->minimumHeight());
            settings.setValue("lLabelLength_maxWidth",  lLabelLength->maximumWidth());
            settings.setValue("lLabelLength_maxHeight", lLabelLength->maximumHeight());
            lLabelLength->setMinimumSize(0, 0);
            lLabelLength->setMaximumSize(0, 0);

            settings.setValue("lTotalLength_minWidth",  lTotalLength->minimumWidth());
            settings.setValue("lTotalLength_minHeight", lTotalLength->minimumHeight());
            settings.setValue("lTotalLength_maxWidth",  lTotalLength->maximumWidth());
            settings.setValue("lTotalLength_maxHeight", lTotalLength->maximumHeight());
            lTotalLength->setMinimumSize(0, 0);
            lTotalLength->setMaximumSize(0, 0);

            settings.setValue("lLabel_w",    lLabel->minimumWidth());
            settings.setValue("lLabel_text", lLabel->text());
            lLabel->setMinimumWidth(235);

            settings.setValue("lEntities_w",    lEntities->minimumWidth());
            lEntities->setMinimumWidth(235);
        }

        lLabel->setText(header);

        lEntities->setText(data);

        m_timer->setInterval(timeout);
        m_timer->start();
    }
    else {
        if (m_auxDataMode) {
            m_timer->stop();
            removeAuxData();
        }
    }
}


void QG_SelectionWidget::removeAuxData(){
    m_auxDataMode = false;

    const QSettings settings("QGDialogFactory", "QGSelectionWidget");

    lLabelLength->setMinimumSize( settings.value("lLabelLength_minWidth").toInt(),
                                  settings.value("lLabelLength_minHeight").toInt());

    lLabelLength->setMaximumSize( settings.value("lLabelLength_maxWidth").toInt(),
                                  settings.value("lLabelLength_maxHeight").toInt());

    lTotalLength->setMinimumSize( settings.value("lTotalLength_minWidth").toInt(),
                                  settings.value("lTotalLength_minHeight").toInt());

    lTotalLength->setMaximumSize( settings.value("lTotalLength_maxWidth").toInt(),
                                  settings.value("lTotalLength_maxHeight").toInt());

    lLabel->setMinimumWidth(settings.value("lLabel_w").toInt());
    lLabel->setText(settings.value("lLabel_text").toString());

    lEntities->setMinimumWidth(settings.value("lEntities_w").toInt());
    lEntities->setText(settings.value("lEntities_text").toString());
}

void QG_SelectionWidget::setGraphicView(RS_GraphicView* gview) {
    if (m_document != nullptr) {
        m_document->getSelection()->removeListener(this);
    }
    if (gview == nullptr) {
        removeAuxData();
        setNumber(0);
        setTotalLength(0);
        m_document = nullptr;
    }
    else {
        m_document = gview->getDocument();
        if (m_document != nullptr) {
           m_document->getSelection()->addListener(this);
        }
        selectionChanged();
    }
}

void QG_SelectionWidget::selectionChanged() {
    int number = 0;
    double length = 0.0;
    if (m_document != nullptr) {
        const RS_Document::LC_SelectionInfo &info = m_document->getSelectionInfo();
        number = info.entitiesCount;
        length = info.totalLength;
    }
    setNumber(number);
    setTotalLength(length);
}
