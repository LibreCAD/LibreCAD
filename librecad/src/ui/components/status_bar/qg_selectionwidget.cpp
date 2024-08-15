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
#include "qg_selectionwidget.h"

#include <QTimer>
#include <QSettings>

#include "rs_settings.h"

/*
 *  Constructs a QG_SelectionWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_SelectionWidget::QG_SelectionWidget(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setObjectName(name);
    setupUi(this);

    lEntities->setText("0");

    auxDataMode = false;

    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect( timer, &QTimer::timeout, this, &QG_SelectionWidget::removeAuxData);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_SelectionWidget::~QG_SelectionWidget()
{
    // no need to delete child widgets, Qt does it all for us
    delete timer;
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_SelectionWidget::languageChange()
{
    retranslateUi(this);
}

void QG_SelectionWidget::setNumber(int n)
{
    if (auxDataMode)
    {
        QSettings settings("QGDialogFactory", "QGSelectionWidget");
        settings.setValue("lEntities_text", n);
    }
    else /* if (!auxDataMode) */
    {
        QString str;
        str.setNum(n);
        lEntities->setText(str);
    }
}

void QG_SelectionWidget::setTotalLength(double l) {
    QString str;
    str.setNum(l, 'g', 6);
    lTotalLength->setText(str);
}


void QG_SelectionWidget::flashAuxData( const QString& header, 
                                       const QString& data, 
                                       const unsigned int& timeout, 
                                       const bool& flash)
{
    if (flash)
    {
        QSettings settings("QGDialogFactory", "QGSelectionWidget");

        if (!auxDataMode)
        {
            auxDataMode = true;

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

        timer->setInterval(timeout);
        timer->start();
    }
    else
    {
        if (auxDataMode)
        {
            timer->stop();
            removeAuxData();
        }
    }
}


void QG_SelectionWidget::removeAuxData()
{
    auxDataMode = false;

    QSettings settings("QGDialogFactory", "QGSelectionWidget");

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
