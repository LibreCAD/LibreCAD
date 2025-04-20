/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include <boost/version.hpp>

#include "lc_dlgabout.h"

#include <QClipboard>
#include <QPainter>

#include "main.h"
#include "qc_applicationwindow.h"
#include "ui_lc_dlgabout.h"

#if defined(Q_OS_LINUX)
#include <QThread>
#endif

namespace {

void aboutImageLabels(QLabel* label)
{
    if (label == nullptr)
        return;

    QString versionLabel = LCReleaseLabel();
    label->setPixmap(QPixmap{":images/librecad01_" + versionLabel.toLower() + ".png"});
    auto pixmap = label->pixmap();

    QPainter painter(&pixmap);
    const double factorX = pixmap.width()/551.;
    const double factorY = pixmap.height()/171.;
    painter.setPen(QColor(255, 0, 0, 128));
    QRectF labelRect{QPointF{280.*factorX, 125.*factorY}, QPointF{480.*factorX, 165.*factorY}};
    QFont font;
    font.setPixelSize(int(labelRect.height()) - 2);
    painter.setFont(font);
    painter.drawText(labelRect,Qt::AlignRight, versionLabel);
    painter.end();
    label->setPixmap(pixmap);
}
}

LC_DlgAbout::LC_DlgAbout(QWidget *parent)
    : LC_Dialog(parent, "About")
    , ui(std::make_unique<Ui::LC_DlgAbout>()){
    ui->setupUi(this);
    auto* appWindow = static_cast<QC_ApplicationWindow*>(parent);

    // Compiler macro list in Qt source tree
    // Src/qtbase/src/corelib/global/qcompilerdetection.h

    m_info = QString(
            tr("Version: <b>%1</b>").arg(XSTR(LC_VERSION)) + "<br/>" +
            #if defined(Q_CC_CLANG)
            tr("Compiler: Clang %1.%2.%3").arg(__clang_major__).arg(__clang_minor__).arg(__clang_patchlevel__) + "<br/>" +
            #elif defined(Q_CC_GNU)
            tr("Compiler: GNU GCC %1.%2.%3").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__) + "<br/>" +
            #elif defined(Q_CC_MSVC)
            tr("Compiler: Microsoft Visual C++") + "<br/>" +
            #endif
            tr("Compiled on: %1").arg(__DATE__) + "<br/>" +
            tr("Qt Version: %1").arg(qVersion()) + "<br/>" +
            tr("Boost Version: %1.%2.%3").arg(BOOST_VERSION / 100000).arg(BOOST_VERSION / 100 % 1000).arg(BOOST_VERSION % 100)
        );
    ui->lVersionInfo->setText(m_info);


    ui->lLinks->setText(QString(R"(<a href="https://github.com/LibreCAD/LibreCAD/graphs/contributors">%1</a>)"
                                "<br/>"
                                R"(<a href="https://github.com/LibreCAD/LibreCAD/blob/master/LICENSE">%2</a>)"
                                "<br/>"
                                R"(<a href="https://github.com/LibreCAD/LibreCAD/">%3</a>)")
                            .arg(tr("Contributors"))
                            .arg(tr("License"))
                            .arg(tr("The Code")));

    aboutImageLabels(ui->lblLogo);

    connect(ui->pbCopy, &QPushButton::clicked, this, &LC_DlgAbout::copyInfo);
    connect(ui->pbCheckNewVersion, &QPushButton::clicked, appWindow, &QC_ApplicationWindow::forceCheckForNewVersion);
}

LC_DlgAbout::~LC_DlgAbout() = default;

void LC_DlgAbout::copyInfo(){
    QString text = m_info;
#if QT_VERSION >= 0x050400
    text += "\n" + tr("System") + ": " + QSysInfo::prettyProductName();
#endif
    QClipboard* clipboard = QApplication::clipboard();
    text.replace("<br>", "\n").replace("<b>","").replace("</b>",""); // one time code so it's ok
    clipboard->setText(text, QClipboard::Clipboard);

    if (clipboard->supportsSelection()) {
        clipboard->setText(text, QClipboard::Selection);
    }

#if defined(Q_OS_LINUX)
    QThread::msleep(1); //workaround for copied text not being available...
#endif
}
