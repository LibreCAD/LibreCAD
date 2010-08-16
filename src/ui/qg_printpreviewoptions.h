/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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
#ifndef QG_PRINTPREVIEWOPTIONS_H
#define QG_PRINTPREVIEWOPTIONS_H

#include <qvariant.h>


#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>
#include "rs_actionprintpreview.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_PrintPreviewOptions
{
public:
    QHBoxLayout *hboxLayout;
    QComboBox *cbScale;
    QToolButton *bBlackWhite;
    QToolButton *bCenter;
    QToolButton *bFit;
    QFrame *sep1_2;

    void setupUi(QWidget *QG_PrintPreviewOptions)
    {
        if (QG_PrintPreviewOptions->objectName().isEmpty())
            QG_PrintPreviewOptions->setObjectName(QString::fromUtf8("QG_PrintPreviewOptions"));
        QG_PrintPreviewOptions->resize(200, 22);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_PrintPreviewOptions->sizePolicy().hasHeightForWidth());
        QG_PrintPreviewOptions->setSizePolicy(sizePolicy);
        QG_PrintPreviewOptions->setMinimumSize(QSize(200, 22));
        hboxLayout = new QHBoxLayout(QG_PrintPreviewOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        cbScale = new QComboBox(QG_PrintPreviewOptions);
        cbScale->setObjectName(QString::fromUtf8("cbScale"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(cbScale->sizePolicy().hasHeightForWidth());
        cbScale->setSizePolicy(sizePolicy1);
        cbScale->setMinimumSize(QSize(110, 0));
        cbScale->setEditable(true);

        hboxLayout->addWidget(cbScale);

        bBlackWhite = new QToolButton(QG_PrintPreviewOptions);
        bBlackWhite->setObjectName(QString::fromUtf8("bBlackWhite"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(bBlackWhite->sizePolicy().hasHeightForWidth());
        bBlackWhite->setSizePolicy(sizePolicy2);
        bBlackWhite->setCheckable(true);
        const QIcon icon = qt_get_icon(image0_ID);
        bBlackWhite->setIcon(icon);

        hboxLayout->addWidget(bBlackWhite);

        bCenter = new QToolButton(QG_PrintPreviewOptions);
        bCenter->setObjectName(QString::fromUtf8("bCenter"));
        sizePolicy2.setHeightForWidth(bCenter->sizePolicy().hasHeightForWidth());
        bCenter->setSizePolicy(sizePolicy2);
        const QIcon icon1 = qt_get_icon(image1_ID);
        bCenter->setIcon(icon1);

        hboxLayout->addWidget(bCenter);

        bFit = new QToolButton(QG_PrintPreviewOptions);
        bFit->setObjectName(QString::fromUtf8("bFit"));
        sizePolicy2.setHeightForWidth(bFit->sizePolicy().hasHeightForWidth());
        bFit->setSizePolicy(sizePolicy2);
        const QIcon icon2 = qt_get_icon(image2_ID);
        bFit->setIcon(icon2);

        hboxLayout->addWidget(bFit);

        sep1_2 = new QFrame(QG_PrintPreviewOptions);
        sep1_2->setObjectName(QString::fromUtf8("sep1_2"));
        sizePolicy2.setHeightForWidth(sep1_2->sizePolicy().hasHeightForWidth());
        sep1_2->setSizePolicy(sizePolicy2);
        sep1_2->setFrameShape(QFrame::VLine);
        sep1_2->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1_2);


        retranslateUi(QG_PrintPreviewOptions);
        QObject::connect(bCenter, SIGNAL(clicked()), QG_PrintPreviewOptions, SLOT(center()));
        QObject::connect(bBlackWhite, SIGNAL(toggled(bool)), QG_PrintPreviewOptions, SLOT(setBlackWhite(bool)));
        QObject::connect(cbScale, SIGNAL(textChanged(QString)), QG_PrintPreviewOptions, SLOT(scale(QString)));
        QObject::connect(cbScale, SIGNAL(activated(QString)), QG_PrintPreviewOptions, SLOT(scale(QString)));
        QObject::connect(bFit, SIGNAL(clicked()), QG_PrintPreviewOptions, SLOT(fit()));

        QMetaObject::connectSlotsByName(QG_PrintPreviewOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_PrintPreviewOptions)
    {
        QG_PrintPreviewOptions->setWindowTitle(QApplication::translate("QG_PrintPreviewOptions", "Print Preview Options", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        QG_PrintPreviewOptions->setProperty("toolTip", QVariant(QString()));
#endif // QT_NO_TOOLTIP
        bBlackWhite->setText(QString());
#ifndef QT_NO_TOOLTIP
        bBlackWhite->setProperty("toolTip", QVariant(QApplication::translate("QG_PrintPreviewOptions", "Toggle Black / White mode", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bCenter->setText(QString());
#ifndef QT_NO_TOOLTIP
        bCenter->setProperty("toolTip", QVariant(QApplication::translate("QG_PrintPreviewOptions", "Center to page", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        bFit->setText(QString());
#ifndef QT_NO_TOOLTIP
        bFit->setProperty("toolTip", QVariant(QApplication::translate("QG_PrintPreviewOptions", "Fit to page", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
    } // retranslateUi


protected:
    enum IconID
    {
        image0_ID,
        image1_ID,
        image2_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    static const unsigned char image0_data[] = { 
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x12,
    0x08, 0x06, 0x00, 0x00, 0x00, 0x56, 0xce, 0x8e, 0x57, 0x00, 0x00, 0x00,
    0x46, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x3c, 0x76, 0xec, 0x18,
    0x03, 0x35, 0x00, 0x13, 0x55, 0x4c, 0xa1, 0xa6, 0x41, 0x2c, 0x0c, 0x0c,
    0x0c, 0x0c, 0x56, 0x56, 0x56, 0xff, 0x29, 0x31, 0xe4, 0xd8, 0xb1, 0x63,
    0x8c, 0x2c, 0x30, 0xce, 0xff, 0xff, 0xb8, 0xcd, 0x62, 0x64, 0x64, 0x24,
    0x68, 0xd8, 0xe0, 0x0b, 0xa3, 0x51, 0x83, 0x46, 0xb4, 0x41, 0xf0, 0x94,
    0x4d, 0x4c, 0xea, 0xc5, 0x07, 0x18, 0x87, 0x6f, 0x31, 0x02, 0x00, 0x9d,
    0xef, 0x0b, 0xd0, 0xf4, 0xfe, 0x09, 0x55, 0x00, 0x00, 0x00, 0x00, 0x49,
    0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};

    static const unsigned char image1_data[] = { 
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x12,
    0x08, 0x06, 0x00, 0x00, 0x00, 0x56, 0xce, 0x8e, 0x57, 0x00, 0x00, 0x00,
    0x64, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xdd, 0x94, 0x41, 0x0a, 0xc0,
    0x20, 0x0c, 0x04, 0x67, 0x8b, 0xff, 0x7f, 0x56, 0xbe, 0x95, 0x1e, 0x4a,
    0x41, 0x0f, 0x69, 0x13, 0x9a, 0x5e, 0x5c, 0x10, 0xf6, 0x20, 0xc3, 0x18,
    0x51, 0x99, 0x19, 0x1d, 0x39, 0x5a, 0x28, 0x9d, 0x20, 0x01, 0xde, 0x01,
    0x1a, 0x00, 0xee, 0xdf, 0x58, 0x92, 0x7e, 0x9e, 0x91, 0x74, 0xad, 0xa7,
    0x9e, 0x02, 0x01, 0xdc, 0xa7, 0x75, 0x5f, 0x7b, 0xc9, 0x68, 0xb5, 0x13,
    0x8a, 0x34, 0x2a, 0xa0, 0x6c, 0xc6, 0xdb, 0x86, 0xec, 0x8d, 0x86, 0x46,
    0xd1, 0xb0, 0x4b, 0x46, 0xb3, 0x44, 0xd4, 0xd3, 0x46, 0xd5, 0xb4, 0x3d,
    0x11, 0xed, 0xfb, 0x8d, 0x9c, 0x97, 0x9d, 0x1a, 0xd2, 0x37, 0x9d, 0x77,
    0x78, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60,
    0x82
};

    static const unsigned char image2_data[] = { 
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x12,
    0x08, 0x06, 0x00, 0x00, 0x00, 0x56, 0xce, 0x8e, 0x57, 0x00, 0x00, 0x00,
    0x63, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xdd, 0x94, 0x41, 0x0a, 0xc0,
    0x20, 0x0c, 0x04, 0x67, 0xa5, 0xff, 0x7f, 0x56, 0xbe, 0x95, 0x1e, 0x8a,
    0xad, 0x82, 0x94, 0x5a, 0xf7, 0x50, 0xba, 0x20, 0xe4, 0x60, 0x86, 0x5d,
    0x4c, 0x54, 0x44, 0xe0, 0x50, 0xb1, 0x50, 0x9c, 0x20, 0x01, 0xe9, 0x00,
    0x6d, 0x00, 0x99, 0x6b, 0x2c, 0x49, 0x57, 0x34, 0xe9, 0x38, 0x6f, 0xea,
    0xd3, 0x51, 0x55, 0x35, 0xd6, 0x1a, 0xbc, 0xab, 0x5b, 0x50, 0x19, 0x5d,
    0x7a, 0xaa, 0xb6, 0xa7, 0x8b, 0x36, 0xab, 0xa1, 0xa3, 0x55, 0x7d, 0x3c,
    0xda, 0xec, 0xf3, 0x77, 0xbd, 0x40, 0x3a, 0x06, 0xd2, 0xb6, 0x22, 0xfa,
    0xef, 0x37, 0xb2, 0x03, 0xdf, 0x1d, 0x1f, 0xe1, 0x88, 0xaa, 0xcf, 0x90,
    0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};

    switch (id) {
        case image0_ID:  { QImage img; img.loadFromData(image0_data, sizeof(image0_data), "PNG"); return QPixmap::fromImage(img); }
        case image1_ID:  { QImage img; img.loadFromData(image1_data, sizeof(image1_data), "PNG"); return QPixmap::fromImage(img); }
        case image2_ID:  { QImage img; img.loadFromData(image2_data, sizeof(image2_data), "PNG"); return QPixmap::fromImage(img); }
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_PrintPreviewOptions: public Ui_QG_PrintPreviewOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_PrintPreviewOptions : public QWidget, public Ui::QG_PrintPreviewOptions
{
    Q_OBJECT

public:
    QG_PrintPreviewOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_PrintPreviewOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateData();
    virtual void center();
    virtual void setBlackWhite( bool on );
    virtual void fit();
    virtual void scale( const QString & s );

protected:
    RS_ActionPrintPreview* action;

protected slots:
    virtual void languageChange();

private:
    QStringList imperialScales;
    QStringList metricScales;
    bool updateDisabled;

    void init();
    void destroy();

};

#endif // QG_PRINTPREVIEWOPTIONS_H
